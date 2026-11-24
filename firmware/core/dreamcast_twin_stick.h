#pragma once

#include <stdint.h>
#include <string.h>

#include "controller_state.h"
#include "dpad_mode.h"

// The Capture entry is unused by Dreamcast. Store D-Pad 2's mode there so the
// existing per-input/output remap record keeps its current width and version.
constexpr uint8_t DREAMCAST_TWIN_STICK_MODE_SLOT = 13;
constexpr uint8_t DREAMCAST_TWIN_STICK_MODE_BASE = 24;

inline bool controllerFrameIsDreamcastTwinStick(const controller_state_t& frame) {
  return strcmp(frame.controller_type_name, "Twin-Stick") == 0;
}

inline bool controllerFramesContainDreamcastTwinStick(
    const controller_state_t* frames, uint8_t frameCount) {
  if (frames == nullptr) return false;
  for (uint8_t i = 0; i < frameCount; ++i) {
    if (frames[i].connected && controllerFrameIsDreamcastTwinStick(frames[i])) return true;
  }
  return false;
}

inline uint8_t dreamcastTwinStickModeFromRemaps(const uint8_t* remaps) {
  if (remaps == nullptr) return DPAD_MODE_BUTTONS;
  const uint8_t encoded = remaps[DREAMCAST_TWIN_STICK_MODE_SLOT];
  if (encoded < DREAMCAST_TWIN_STICK_MODE_BASE ||
      encoded >= DREAMCAST_TWIN_STICK_MODE_BASE + 4) {
    return DPAD_MODE_BUTTONS;
  }
  return encoded - DREAMCAST_TWIN_STICK_MODE_BASE;
}

inline uint8_t dreamcastTwinStickModeEncoding(uint8_t mode) {
  return DREAMCAST_TWIN_STICK_MODE_BASE +
         (mode <= DPAD_MODE_BUTTONS ? mode : DPAD_MODE_BUTTONS);
}

struct DreamcastTwinStickDirections {
  bool up;
  bool down;
  bool left;
  bool right;
};

inline DreamcastTwinStickDirections dreamcastTwinStickDpad2(
    const controller_state_t& frame) {
  return {
    (frame.EXTRA & (1u << 0)) != 0,
    (frame.EXTRA & (1u << 1)) != 0,
    (frame.EXTRA & (1u << 2)) != 0,
    (frame.EXTRA & (1u << 3)) != 0,
  };
}

inline int16_t dreamcastTwinStickAxis(bool negative, bool positive,
                                     int16_t fullScale) {
  if (negative == positive) return 0;
  return negative ? -fullScale : fullScale;
}

inline uint8_t dreamcastTwinStickHat(bool up, bool down, bool left, bool right) {
  if (up && down) up = down = false;
  if (left && right) left = right = false;
  if (up) return right ? 1 : left ? 7 : 0;
  if (down) return right ? 3 : left ? 5 : 4;
  if (right) return 2;
  if (left) return 6;
  return 8;
}
