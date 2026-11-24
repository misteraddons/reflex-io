#pragma once

// Button Remapping System
// Per-input-mode button remapping with hybrid UI:
// - Left side: Scrollable text list of mappings
// - Right side: Static pad display
// - Bottom row: Cancel, Clear, and Save options

#include <Arduino.h>
#include <EEPROM.h>

#include "controller_frame_state.h"
#include "device_runtime_state.h"
#include "controller_state.h"
#include "dreamcast_twin_stick.h"
#include "n64_button_assignment.h"
#include "remap_table.h"
#include "../output/output_mode.h"
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
#include "../features/usb2db15/usb2db15_action_bindings.h"
#endif
#include <SSD1306Ascii/SSD1306AsciiWire.h>

#if ADAPT_POLICY_COOPERATIVE_OLED
#include "../platform/cooperative_oled.h"
extern CooperativeOledAscii display;
#else
extern SSD1306AsciiWire display;
#endif

extern const char* const remap_button_names[REMAP_MAX_BUTTONS];

bool psxHasL3R3();

#define BTN_BIT_A      (1UL << 0)
#define BTN_BIT_B      (1UL << 1)
#define BTN_BIT_X      (1UL << 2)
#define BTN_BIT_Y      (1UL << 3)
#define BTN_BIT_L1     (1UL << 4)
#define BTN_BIT_R1     (1UL << 5)
#define BTN_BIT_L2     (1UL << 6)
#define BTN_BIT_R2     (1UL << 7)
#define BTN_BIT_L3     (1UL << 8)
#define BTN_BIT_R3     (1UL << 9)
#define BTN_BIT_START  (1UL << 10)
#define BTN_BIT_SELECT (1UL << 11)
#define BTN_BIT_HOME   (1UL << 12)
#define BTN_BIT_CAPTURE (1UL << 13)
#define BTN_BIT_EXTRA0 (1UL << 14)
#define BTN_BIT_EXTRA1 (1UL << 15)
#define BTN_BIT_EXTRA2 (1UL << 16)
#define BTN_BIT_EXTRA3 (1UL << 17)
#define BTN_BIT_EXTRA4 (1UL << 18)

uint32_t getInputButtonMask(DeviceEnum mode);
uint8_t getRemapButtonCount(DeviceEnum mode);
uint8_t getRemapButtonSlot(DeviceEnum mode, uint8_t display_index);
uint8_t getRemapButtonDisplayIndex(DeviceEnum mode, uint8_t button_slot);
const char* getRemapButtonName(DeviceEnum mode, uint8_t index);
const char* getRemapButtonNameForSlot(DeviceEnum mode, uint8_t button_slot);
// Destination name for one remap row. Shared by the quick-config and system
// remap screens so both print the same thing for the same stored value:
// N64 output assignments, USB2DB15 combined/arcade bindings, then the plain
// output button name with the Position button map applied.
const char* remapTargetName(DeviceEnum mode, outputMode_t output, const uint8_t* remaps,
                            uint8_t index, uint8_t target_slot, uint8_t cstick_mode,
                            bool native_n64, bool position_map);
inline bool isN64StoredAssignment(DeviceEnum mode, uint8_t slot, uint8_t value) {
#ifdef ENABLE_INPUT_N64
  // An N64 D-pad direction assigned to an output button (never Off).
  if (mode == RZORD_N64 && remapSlotIsDpad(slot)) return n64_assignment::tagged(value);
  return mode == RZORD_N64 && n64_assignment::storedValue(slot, value);
#else
  (void)mode; (void)slot; (void)value;
  return false;
#endif
}
// Whether source slot may hold value: a button becomes another button slot,
// a direction stays itself or becomes a button; N64 assignments and USB2DB15
// combined bindings keep their own encodings.
inline bool remapValueAllowed(DeviceEnum mode, uint8_t source, uint8_t value) {
  if (source >= REMAP_MAX_BUTTONS) return false;
#ifdef ENABLE_INPUT_DREAMCAST
  if (mode == RZORD_DREAMCAST && source == DREAMCAST_TWIN_STICK_MODE_SLOT &&
      value >= DREAMCAST_TWIN_STICK_MODE_BASE &&
      value < DREAMCAST_TWIN_STICK_MODE_BASE + 4) return true;
#endif
  if (isN64StoredAssignment(mode, source, value)) return true;
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  if ((usb2db15_bindings::allowedSources & (1u << source)) && usb2db15_bindings::isCombined(value)) return true;
#endif
  if (value >= REMAP_MAX_BUTTONS) return false;
  // No button becomes a direction and no direction becomes another one.
  if (remapSlotIsDpad(value)) return value == source;
  return true;
}
// Selects a valid destination. Twin Stick destinations are output buttons,
// independent of the input's physical source list.
uint8_t nextRemapTarget(DeviceEnum mode, outputMode_t output, uint8_t source,
                        uint8_t current, bool forward);
// "Up", "Down", "Left", "Right" for a D-pad slot, else nullptr.
const char* remapDpadName(uint8_t slot);

enum RemapMenuState : uint8_t {
  REMAP_CLOSED = 0,
  REMAP_NAVIGATE,
  REMAP_EDIT_SOURCE,
};

extern uint8_t active_remaps[REMAP_MAX_BUTTONS];

class ButtonRemapMenu {
private:
  RemapMenuState state;
  uint8_t selected_index;
  uint8_t scroll_offset;
  uint8_t button_count;
  bool needs_redraw;
  uint8_t temp_remap[REMAP_MAX_BUTTONS];
  uint8_t edit_source_index;
  DeviceEnum active_mode;

  static const uint8_t LIST_ROWS = 6;
  static const uint8_t PAD_START_COL = 66;
  static const uint8_t BOTTOM_ROW = 7;

  void renderButtonList(DeviceEnum mode);
  void renderPadDisplay(DeviceEnum mode);
  void renderBottomRow();

public:
  ButtonRemapMenu();

  bool isOpen();
  bool isEditMode();
  void open(DeviceEnum mode);
  void close();
  void loadRemaps(const uint8_t* data);
  void getRemaps(uint8_t* data);
  void clearRemaps();
  // Default preserves legacy forward navigation; USB2DB15 can supply a
  // physical previous edge without changing other menu callers.
  void navigate(bool reverse = false);
  uint8_t select();
  void setDestination(uint8_t button_index);
  uint8_t getFirstPressedButton();
  static void applyRemap(controller_state_t& report, const uint8_t* remap);
  void render(DeviceEnum mode);
};

extern ButtonRemapMenu remapMenu;
extern uint16_t g_eeprom_remap_index;

bool hasActiveRemap();
void loadRemapsFromEEPROM();
void writeRemapsToEEPROM();
void saveRemapsToEEPROM();
void clearRemapsToDefault();
