#include "../product_config.h"
#include "remap_pairs.h"

#include <string.h>

#include "button_remap.h"
#include "controller_selection_state.h"
#include "dreamcast_twin_stick.h"
#include "device_runtime_state.h"
#include "../input/runtime/input_frame_runtime.h"
#include "remap_pair_store.h"
#include "settings_store.h"
#include "../output/output_capabilities.h"
#include "../output/output_runtime_state.h"
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
#include "../features/usb2db15/usb2db15_action_bindings.h"
#endif

static_assert(REMAP_PAIR_BUTTONS == REMAP_MAX_BUTTONS, "remap pair width");
static_assert(REMAP_RECORD_BUTTONS == PER_MODE_REMAP_SIZE, "legacy remap width");
#if defined(ADAPT_CAP_DPAD_REMAP) && !ADAPT_REMAPS_PER_OUTPUT
#error "D-pad remap entries are stored only in remap pairs"
#endif

namespace {

bool loadedValid = false;
DeviceEnum loadedInput = RZORD_NONE;
outputMode_t loadedOutput = OUTPUT_MISTER;
bool loadedTwinStick = false;

bool dreamcastTwinStickConnected() {
  return deviceMode == RZORD_DREAMCAST &&
         controllerFramesContainDreamcastTwinStick(controller_frames, inputPortCount());
}

bool setDreamcastTwinStickDefaults(uint8_t* remaps, outputMode_t selected) {
  const bool uninitialized =
      remaps[DREAMCAST_TWIN_STICK_MODE_SLOT] < DREAMCAST_TWIN_STICK_MODE_BASE ||
      remaps[DREAMCAST_TWIN_STICK_MODE_SLOT] >= DREAMCAST_TWIN_STICK_MODE_BASE + 4;
  if (!uninitialized) return false;

  remaps[DREAMCAST_TWIN_STICK_MODE_SLOT] =
      dreamcastTwinStickModeEncoding(DPAD_MODE_BUTTONS);
  const outputMode_t output = remapPairOutputFor(selected);
  if (output == OUTPUT_XINPUT || output == OUTPUT_XINPUTW || output == OUTPUT_XINPUT2P) {
    remaps[16] = 4;   // D-Pad 2 Up -> LB
    remaps[14] = 5;   // D-Pad 2 Down -> RB
    remaps[17] = 8;   // D-Pad 2 Left -> LS
    remaps[18] = 9;   // D-Pad 2 Right -> RS
    remaps[15] = 11;  // Pause (D) -> Back
  }
  return true;
}

#if !ADAPT_REMAPS_PER_OUTPUT
void writeRecordRemaps(DeviceEnum input, const uint8_t (&remaps)[REMAP_MAX_BUTTONS]) {
  PerModeSettingsRecord settings{};
  readPerModeSettings(input, settings);
  remapTableToRecord(settings.remaps, remaps);
  sanitizePerModeSettings(input, settings);
  writePerModeSettings(input, settings);
}

#else
void retireLegacyRemaps(DeviceEnum input) {
  PerModeSettingsRecord settings{};
  if (!readPerModeSettings(input, settings) || remapRecordIsIdentity(settings.remaps)) return;
  for (uint8_t i = 0; i < REMAP_RECORD_BUTTONS; ++i) settings.remaps[i] = i;
  sanitizePerModeSettings(input, settings);
  writePerModeSettings(input, settings);
}
#endif

}  // namespace

outputMode_t remapPairOutputFor(outputMode_t selected) {
  outputMode_t mode = canonicalizeOutputMode(selected);
  if (mode == OUTPUT_AUTO) {
    mode = autoDetectState == AUTO_STATE_IDLE
        ? canonicalizeOutputMode(auto_detect_probe_output_mode(autoDetectProbeStage))
        : canonicalizeOutputMode(outputMode);
    // Auto never selects a MiSTer peripheral mode; those are PSX promotions.
    if (output_is_specialized_mister_psx_mode(mode)) mode = OUTPUT_MISTER;
  }
  return sanitizeRuntimeOutputMode(mode);
}

bool remapPairOutputResolved(outputMode_t selected) {
  return canonicalizeOutputMode(selected) != OUTPUT_AUTO ||
         autoDetectState != AUTO_STATE_IDLE;
}

bool sanitizeRemapValuesForMode(DeviceEnum mode, uint8_t* remaps) {
  bool changed = false;
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; ++i) {
    if (!remapValueAllowed(mode, i, remaps[i])) {
      remaps[i] = i;
      changed = true;
    }
  }
  return changed;
}

bool applyDreamcastTwinStickDefaults(DeviceEnum input, outputMode_t selected,
                                     uint8_t* remaps) {
#ifdef ENABLE_INPUT_DREAMCAST
  if (input == RZORD_DREAMCAST && dreamcastTwinStickConnected()) {
    return setDreamcastTwinStickDefaults(remaps, selected);
  }
#else
  (void)input;
  (void)selected;
  (void)remaps;
#endif
  return false;
}

void readRemapsForPair(DeviceEnum input, outputMode_t selected, uint8_t (&remaps)[REMAP_MAX_BUTTONS]) {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; ++i) remaps[i] = i;
  if (!isConcreteModeForPerSettings(input)) return;
  // A non-identity legacy copy is newer than any pair (see loadRemapsForMode).
  PerModeSettingsRecord settings{};
  readPerModeSettings(input, settings);
  if (ADAPT_REMAPS_PER_OUTPUT && remapRecordIsIdentity(settings.remaps)) {
    remapPairRead(input, remapPairOutputFor(selected), remaps);
  } else {
    remapTableFromRecord(remaps, settings.remaps);
  }
  if (input == RZORD_DREAMCAST && dreamcastTwinStickConnected()) {
    (void)setDreamcastTwinStickDefaults(remaps, selected);
  }
  sanitizeRemapValuesForMode(input, remaps);
}

bool remapsFitForPair(DeviceEnum input, outputMode_t selected,
                      const uint8_t (&remaps)[REMAP_MAX_BUTTONS]) {
#if ADAPT_REMAPS_PER_OUTPUT
  if (!isConcreteModeForPerSettings(input)) return true;
  uint8_t sanitized[REMAP_MAX_BUTTONS];
  memcpy(sanitized, remaps, REMAP_MAX_BUTTONS);
  sanitizeRemapValuesForMode(input, sanitized);
  return remapPairCanWrite(input, remapPairOutputFor(selected), sanitized);
#else
  (void)input;
  (void)selected;
  (void)remaps;
  return true;
#endif
}

bool writeRemapsForPair(DeviceEnum input, outputMode_t selected,
                        const uint8_t (&remaps)[REMAP_MAX_BUTTONS]) {
  if (!isConcreteModeForPerSettings(input)) return false;
  uint8_t sanitized[REMAP_MAX_BUTTONS];
  memcpy(sanitized, remaps, REMAP_MAX_BUTTONS);
  sanitizeRemapValuesForMode(input, sanitized);
#if ADAPT_REMAPS_PER_OUTPUT
  if (!remapPairWrite(input, remapPairOutputFor(selected), sanitized)) return false;
  retireLegacyRemaps(input);
#else
  (void)selected;
  writeRecordRemaps(input, sanitized);
#endif
  return true;
}

void loadRemapsFromEEPROM() {
  readRemapsForPair(deviceMode, configuredOutputMode, active_remaps);
}

void writeRemapsToEEPROM() {
  (void)writeRemapsForPair(deviceMode, configuredOutputMode, active_remaps);
}

void saveRemapsToEEPROM() {
  writeRemapsToEEPROM();
}

void noteActiveRemapPair(DeviceEnum input, outputMode_t output) {
  loadedValid = true;
  loadedInput = input;
  loadedOutput = output;
  loadedTwinStick = dreamcastTwinStickConnected();
}

bool activeRemapPairIsStale(DeviceEnum input) {
  return loadedValid && loadedInput == input &&
         (loadedOutput != remapPairOutputFor(configuredOutputMode) ||
          loadedTwinStick != dreamcastTwinStickConnected());
}
