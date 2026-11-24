#pragma once

// Quick Config Menu (Mode Menu)
// Accessed via single tap of mode button
// Provides quick access to mode-specific settings for current input mode

#include "../product_config.h"
#include <Arduino.h>
#include "../core/analog_calibration_state.h"
#include "../core/button_remap.h"
#include "../core/quick_config_transaction.h"
#include "../core/classic_analog_range.h"
#include "../core/classic_dual_merge_config.h"
#include "../core/controller_settings_state.h"
#include "../core/device_runtime_state.h"
#include "../core/dpad_mode.h"
#include "../core/controller_frame_state.h"
#include "../core/firmware_support.h"
#include "../core/nes_device_mode.h"
#include "../core/psx_poll_timing.h"
#include "../core/rotary_policy.h"
#include "../core/rotary_output_transform.h"
#include "../core/rotary_mouse_settings.h"
#include "../core/hid_mapping_profiles.h"
#include "menu_input_mode.h"
#include "menu_input.h"
#include "menu_mode_state.h"
#include "menu_runtime_state.h"
#include "menu_working_state.h"
#include "quick_stored_value.h"
#include "rumble_capabilities.h"
#include "../output/output_mode.h"
#include "../output/output_runtime_state.h"
#include "../core/turbo.h"
#include "../input/runtime/input_adapter_runtime.h"
#include "../input/snes/input_snes_runtime_state.h"
#include "../core/settings_store.h"
#include "../core/controller_selection_state.h"
#include "analog_stick_trace.h"
#include "analog_diagnostic.h"

// Quick config menu states
enum QuickConfigState : uint8_t {
  QC_CLOSED = 0,
  QC_MAIN_MENU,       // Main mode menu with categories
  QC_CONFIRM_DEFAULT, // Confirm saving current-mode defaults and exiting
  QC_CONFIRM_DINPUT_RESET, // Confirm deleting the selected generic HID profile
  QC_CONFIRM_NSO_1P, // Confirm enabling a native one-player NSO identity
  QC_TURBO_LIST,      // List of buttons for turbo config
  QC_TURBO_RATE,      // Set rate for selected button
  QC_SETTING_EDIT,    // Editing a setting value
  QC_REMAP_LIST,      // Button remap list
  QC_REMAP_SELECT,    // Selecting remap target
  QC_RANGE_TEST,      // Analog range test display
  QC_GUNCON_ADJUST,   // Guncon offset adjustment
  QC_ANALOG_SUBMENU,  // Analog stick/trigger settings submenu
  QC_HOTKEYS_SUBMENU, // Directional hotkey assignments
  QC_DUAL_MERGE_MAP,  // Two-player input merge ownership map
  QC_RUMBLE_SUBMENU,  // Rumble level and motor test submenu
  QC_STICK_CAL,       // Analog stick calibration
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  QC_USB2DB15_TOURNAMENT,
#endif
  QC_MODE_PICKER,     // Manual input/output choices; staged only on confirm
  QC_CHOICE_PICKER,   // One setting's values as a list; staged only on confirm
};

// Mode menu items - all possible items
enum QuickConfigItem : uint8_t {
  QCI_PLAYER = 0,
  QCI_INPUT_MODE,
  QCI_OUTPUT_MODE,
  QCI_WIN_OUTPUT,
  QCI_MOUSE_SOURCE,
  QCI_TOUCHPAD_MOUSE,
  QCI_DUALSENSE_PLAYER_LEDS,
  QCI_DUALSENSE_LIGHTBAR,
  QCI_USB2DB15_PRESENTATION,
  QCI_ANALOG_MENU,
  QCI_DEADZONE,
  QCI_RANGE_TEST,
  QCI_DPAD_BUTTONS,
  QCI_DPAD2_MODE,
  QCI_SOCD,
  QCI_STICK_INVERT,
  QCI_RUMBLETECH,
  QCI_RUMBLE,
  QCI_TRIGGERS,
  QCI_SWITCH_RTRIG_STICK,
  QCI_GC_L_SWITCH,
  QCI_REMAP,
  QCI_RESET_DINPUT_MAP,
  QCI_BTN_MAP,       // Button mapping: Name (A→A) or Position (layout swap)
  QCI_N64_Z,         // Z button policy: N64 L1/L2, GameCube R1/Back
  QCI_N64_CSTICK,    // N64 C buttons: Auto, Buttons, or Stick
  QCI_N64_RANGE,     // Classic analog range: Raw, Norm, Cal, or Learn
  QCI_NSO_SPECIAL,   // NSO special (Switch Pro output only)
  QCI_POWERPAD,      // NES Power Pad mode
  QCI_CLASSIC_DUAL_MERGE, // Merge P1 movement + selected P2 controls
  QCI_GUNCON_OFFSET,
  QCI_TURBO,
  QCI_JOGCON_MODE,
  QCI_JOGCON_FORCE,
  QCI_WHEEL_SENS,
  QCI_JOGCON_DIGITAL,
  QCI_JOGCON_WHEEL_AXIS,
  QCI_SPINNER_SPEED,
  QCI_PADDLE_GAIN,
  QCI_ROTARY_REVERSE,
  QCI_DRIVING_ENDS,
  QCI_STICK_CAL,     // Analog stick calibration
  QCI_DISCARD_EXIT,
  QCI_EXIT,
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  QCI_USB2DB15_ACTIVATION,
  QCI_USB2DB15_AUTO_SHOOT,
  QCI_USB2DB15_TOURNAMENT,
  QCI_USB2DB15_SAVE_PROFILE,
#endif
  QCI_START_LEFT,
  QCI_START_RIGHT,
  QCI_START_UP,
  QCI_START_DOWN,
  QCI_HOTKEYS_MENU,
  QCI_PSX_POLL_RATE, // retired 2026-10-25 (Adapt.html, SET PSX_TIMING);
  QCI_PSX_CMD_GAP,   // kept so no later item moves
  QCI_NEGCON_I,      // neGcon I: Analog or Digital, negcon_input_modes.h
  QCI_NEGCON_II,
  QCI_NEGCON_L,
  QCI_NEGCON_GAIN,   // neGcon Analog gain for I, II and L
  QCI_ROTARY_MOUSE_AXIS, // Mouse output: rotation moves X or Y, or the wheel
  QCI_MOUSE_WHEEL,       // Mouse output: what scrolls the wheel
  QCI_MOUSE_LEFT,        // Mouse output: the controller button for each
  QCI_MOUSE_MIDDLE,      // mouse button (rotary_mouse_settings.h)
  QCI_MOUSE_RIGHT,
  QCI_MOUSE_BUTTON4,
  QCI_MOUSE_BUTTON5,
  QCI_MOUSE_PLAYERS,     // One mouse from P1, or one independent mouse per port
  QCI_ANALOG_SETUP,
  QCI_JVS_MENU_HOLD,
  QCI_JVS_HOME_HOTKEY,
  QCI_PS2_REMOTE_OUTPUT,
  QCI_JVS_HOLD_START,
  QCI_COUNT
};

struct QuickConfigSnapshot {
  uint8_t state;
  uint8_t cursor;
  uint8_t top;
  uint8_t item;
  uint8_t count;
  uint8_t flags;
};

enum RumbleMenuItem : uint8_t {
  QCI_RUMBLE_LEVEL = 0,
  QCI_RUMBLE_HEAVY,       // reserved; heavy strength follows Level
  QCI_RUMBLE_LIGHT,       // reserved; light strength follows Level
  QCI_RUMBLE_TEST_HEAVY,
  QCI_RUMBLE_TEST_LIGHT,
  QCI_RUMBLE_TEST_BOTH,
  QCI_RUMBLE_RAMP_HEAVY,
  QCI_RUMBLE_BACK,
  QCI_RUMBLE_COUNT
};

class QuickConfigMenu {
private:
  QuickConfigState state;
  uint8_t selected_index;    // Index in visible items
  uint8_t turbo_index;       // Index in turbo button list
  uint8_t remap_index;       // Index in remap list
  uint8_t analog_index;      // Index in analog submenu
  uint8_t hotkey_index;      // Index in hotkey submenu
  uint8_t dual_merge_index;  // Index in 2P merge ownership list
  uint8_t rumble_index;      // Index in rumble submenu
  bool needs_redraw;
  QuickConfigItem editing_item;
  uint8_t mode_picker_index = 0;
  uint8_t choice_picker_index = 0;
  // The open list's values, in the order the row offers them.
  static constexpr uint8_t kChoiceCapacity = 24;
  uint8_t choice_values[kChoiceCapacity] = {};
  uint8_t choice_count = 0;
  QuickConfigState choice_return_state = QC_MAIN_MENU;

  // Visible items for current mode
  QuickConfigItem visible_items[QCI_COUNT];
  uint8_t visible_count;
  QuickConfigItem analog_items[QCI_COUNT];
  uint8_t analog_count;
  QuickConfigItem hotkey_items[6];
  uint8_t hotkey_count;

  QuickStoredValue stored_dpad_mode;
  QuickStoredValue stored_stick_invert;
  QuickStoredValue stored_rumble;
  QuickStoredValue stored_triggers;

  // Temporary values
  TurboRate temp_rates[TURBO_BTN_COUNT];
  DeviceEnum temp_input_mode;
  outputMode_t temp_output_mode;
  uint8_t temp_win_output;
  uint8_t temp_ps2_remote_output;
  uint8_t temp_pointer_source;
  uint8_t temp_touchpad_mouse;  // Pointer speed; persisted with source.
  uint8_t temp_dualsense_led_policy;
  uint8_t temp_usb2db15_presentation = 0;
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  uint8_t temp_usb2db15_activation = 2;
  uint8_t temp_usb2db15_auto_shoot = 0;
#endif
  uint8_t temp_usb2db15_b6_source; // 0=R2/RT, 1=L1/LB; session-only
  uint8_t temp_deadzone;
  socdMode_t temp_socd;
  uint8_t temp_dpad_mode;  // 0=DPad, 1=Left Stick, 2=Right Stick, 3=Buttons
  uint8_t temp_dpad2_mode;
  uint8_t temp_stick_invert;
  uint8_t temp_snes_rumbletech;
  uint8_t temp_rumble;
  uint8_t temp_triggers;
  int8_t temp_guncon_x;
  int8_t temp_guncon_y;
  uint8_t temp_spinner;      // 0=0.25x, 1=0.5x, 2=1x, 3=2x, 4=4x
  uint8_t temp_paddle_divisor; // 1=1x, 2..16=1/divisor
  bool temp_rotary_reverse;
  bool temp_driving_wrap;
  uint8_t temp_rotary_profile;        // 0 retains the old per-output tuning
  uint8_t temp_jogcon_rotary_profile; // independent JogCon profile
  uint8_t temp_jogcon_mode;  // 0=Spinner, 1=Paddle, 2=Wheel, 3=Fake Spinner
  uint8_t temp_jogcon_force; // 1=Low, 3=Med-L, 7=Med-H, 15=High
  uint8_t temp_wheel_sens;   // 0=Fine (0.5x), 1=Normal (1x), 2=Coarse (2x)
  uint8_t temp_jogcon_digital; // 0=Left/Right, 1=L3/R3, 2=L1/R1, 3=Up/Down
  uint8_t temp_jogcon_wheel_axis; // 0=X, 1=Y
  RotaryMouseSettings temp_mouse{};  // Mouse output page, per input mode
  uint8_t temp_btn_map;      // 0=Name (A→A), 1=Position (layout swap)
  uint8_t temp_gamecube_l_switch; // 0=ZL, 1=L
  uint8_t temp_n64_z;        // N64: 0=L1, 1=L2; GC: 0=R1, 1=Back
  uint8_t temp_n64_cstick;   // 0=Auto, 1=Buttons, 2=Stick
  uint8_t temp_psx_poll_rate = 0; // psx_poll::kRatePeriodUs index, 0=Auto
  uint8_t temp_psx_cmd_gap = 0;   // psx_poll::kGapUs index, 0=Auto
  uint8_t temp_negcon_digital = 0; // NEGCON_INPUT_* bits set to Digital, plus L_STICK
  uint8_t temp_negcon_gain = 0;    // kNegconGainQuarters index
  uint8_t temp_direction_hotkeys[3] = {};
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
  uint8_t temp_jvs_menu_hold = 0;
  uint8_t temp_jvs_home_hotkey = 0;
  uint8_t temp_jvs_hold_start = 0;
#endif
  uint8_t temp_direction_hotkey_home = 0;  // Modern: 0=Auto, 1=Off, 2=On
  uint32_t temp_direction_hotkey_identity = 0;
  uint8_t temp_n64_range;    // 0=Raw, 1=Normalized, 2=Calibrated, 3=Learn
  uint8_t temp_wii_range;    // 0=Raw, 1=Normalized, 2=Calibrated, 3=Learn (Wii/GC)
  uint8_t temp_nso_special;  // switch_device_selection_t persisted in legacy field
  uint8_t temp_powerpad;     // 0=Off, 1=On
  uint8_t temp_classic_dual_merge; // 0=Off, 1=P1 dpad/start/select + P2 face/shoulders
  uint32_t temp_classic_dual_merge_p2_mask;
  uint8_t guncon_edit_axis;  // 0=X, 1=Y, 2=Done
  bool should_save;          // True if user selected Save, false if Cancel
  bool defaults_restored;    // True when confirmed defaults caused this close
  uint8_t turbo_rate_selection;  // Selection in turbo rate screen (0-N = rates, N+1 = Back)
  int16_t range_last_lx, range_last_ly, range_last_rx, range_last_ry;  // Last values for flicker prevention
  AnalogStickTrace range_trace[2];
  AnalogDiagnosticTarget range_test_target;
  bool range_test_right_stick;
  DeviceEnum range_trace_mode;
  bool range_trace_octagonal;
  char range_trace_type[12] = {};
  bool range_trace_dirty;
  uint32_t range_last_draw_ms;
  bool on_bottom_row;             // True if selection is on bottom row
  bool bottom_right;              // Legacy mirror: true when [Save] is selected
  uint8_t bottom_index;           // 0=[Cancel], 1=[Default], 2=[Save]
  uint8_t default_confirm_index;  // 0=[Cancel], 1=[Default]
  uint8_t dinput_reset_confirm_index; // 0=[Cancel], 1=[Reset]
  uint8_t nso_1p_confirm_index; // 0=[Cancel], 1=[Enable]
  uint8_t reset_dinput_device;
  uint32_t dinput_reset_connection_order;
  HidMappingProfileKey dinput_reset_key;
  bool reset_dinput_completed;
  bool action_error;
  bool return_to_analog_submenu;
  QuickConfigTransaction transaction;
  DeviceEnum transaction_runtime_mode;
  DeviceEnum transaction_staged_mode;

  // Stick calibration state
  int8_t cal_lx_min, cal_lx_max;  // Tracked min/max during calibration
  int8_t cal_ly_min, cal_ly_max;
  int8_t cal_rx_min, cal_rx_max;
  int8_t cal_ry_min, cal_ry_max;
  uint8_t cal_selection;          // 0=Cancel, 1=Save, 2=Clear
  bool cal_has_data;              // True if any movement recorded

  // Check if mode has analog sticks
  bool hasAnalogSticks(DeviceEnum mode);

  // Check if mode has analog triggers
  bool hasAnalogTriggers(DeviceEnum mode);

  // Check if mode supports rumble
  bool hasRumble(DeviceEnum mode);
  bool isSnesMode(DeviceEnum mode);

  // Check if mode can generate relative mouse delta input
  // Check if mode is Guncon
  bool isGuncon(DeviceEnum mode);

  // Check if mode is JogCon
  bool isJogcon(DeviceEnum mode);

  // Check if mode has a spinner/rotary encoder
  bool isDrivingFallbackActive(DeviceEnum mode);
  bool hasSpinner(DeviceEnum mode);
  bool hasContextualRotaryInput(DeviceEnum mode);
  bool hasExplicitRotaryOutput();
  bool hasDrivingEndsControl(DeviceEnum mode);
  RotaryInputKind detectedRotaryInputKind(DeviceEnum mode);
  uint8_t effectiveTempJogconMode();
  RotaryTuning getTempRotaryTuning(DeviceEnum mode);
  void setTempRotaryGain(uint8_t gain);
  static const char* rotaryGainLabel(uint8_t gain);
  void cycleTempMouseAxis(bool forward);
  bool mousePageActive();
  static uint8_t mouseRoleForItem(QuickConfigItem item);
  uint8_t tempMouseButtonSlot(uint8_t role);
  void setTempMouseButtonSlot(uint8_t role, uint8_t slot);
  const char* mouseButtonName(uint8_t slot);
  static const char* mouseWheelName(uint8_t wheel);
  uint8_t tempMouseWheel(bool asDefault);
  void toggleTempRotaryReverse();
  bool tempDrivingEndsWrap(DeviceEnum mode);
  void toggleTempDrivingEnds();
  static uint8_t negconInputForItem(QuickConfigItem item);
  bool tempNegconInputHasPressure(uint8_t input);
  bool tempNegconInputDigital(uint8_t input);
  void toggleTempNegconInput(uint8_t input);

  // Check if mode is a Nintendo controller (for button map mode)
  bool isNintendoController(DeviceEnum mode);

  // Check if mode is N64
  bool isN64(DeviceEnum mode);

  // Check if mode is GameCube
  bool isGameCube(DeviceEnum mode);

  // Check if mode supports Classic stick range policy
  bool hasClassicAnalogRange(DeviceEnum mode);
  uint8_t getTempClassicAnalogRange();
  void cycleTempClassicAnalogRange(bool forward);
  void cycleTempSwitchDevice(bool forward);

  // Check if mode has right analog stick
  bool hasRightStick(DeviceEnum mode);
  uint8_t getDpadModeMax(outputMode_t mode);
  bool isTempDpadModeAllowed(uint8_t mode);
  bool isTempDpad2ModeAllowed(uint8_t mode);
  void cycleTempDpadMode(bool forward);
  void cycleTempDpad2Mode(bool forward);
  void cyclePointerMouseSource(bool forward);
  uint8_t getStickInvertMax(DeviceEnum mode);
  uint8_t getTriggerModeMax(outputMode_t mode);
  bool isTempTriggerModeAllowed(uint8_t mode);
  bool supportsTempTriggerRightStick();
  bool tempOutputIsSwitchPro();
  void cycleTempTriggerMode(bool forward);
  bool isTurboConfigIndexVisible(uint8_t configIndex);
  uint8_t getVisibleTurboCount();
  uint8_t getTurboConfigIndexForVisibleIndex(uint8_t visibleIndex);
  uint8_t getTurboButtonIndexForVisibleIndex(uint8_t visibleIndex);
  const char* getTurboButtonNameForVisibleIndex(uint8_t visibleIndex);
  uint8_t getDualMergeMenuCount();
  void toggleDualMergeCurrentItem();
  uint8_t rumbleMotorCount();
  RumbleStrengthSupport rumbleStrengthSupportForMode(DeviceEnum mode);
  bool hasVariableRumbleStrength();
  void cycleTempRumbleLevel(bool forward);
  bool isRumbleMenuItemVisible(RumbleMenuItem item);
  void advanceRumbleIndex(bool forward);
  void startTempRumbleTest(bool heavy, bool light);
  void startHeavyRumbleRamp();

  // Get number of analog sticks for mode
  // Returns 0, 1, or 2
  uint8_t getStickCount(DeviceEnum mode);
  bool canEditModeSpecificSettings();
  bool isLiveInputSelection();
  const TurboButtonConfig& getSelectedTurboConfig();
  void loadTempQuickSettingsRecord(DeviceEnum mode, const PerModeQuickSettings& settings);
  void clampLoadedQuickSettings(DeviceEnum mode, uint8_t storedRumble);
  void loadTempQuickSettingsForMode(DeviceEnum mode);
  void resetTempQuickSettingsToDefaults();
  // Remap list Default: staged remaps only, calibration untouched.
  void resetTempRemapsToDefaults();
  // Remaps belong to the input+output pair: show the selected output's set.
  void restageRemapsForOutput();
#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
  void loadDirectionalControllerHotkeys();
#endif
  bool isNsoSpecialActiveForSelection(DeviceEnum mode);

  // Build list of visible items based on current mode
  void buildVisibleItems();
  void buildAnalogItems();
  void buildHotkeyItems();
  bool renderMarqueeTick(SSD1306AsciiWire& display);
  void addVisibleItem(QuickConfigItem item) {
    if (visible_count < QCI_COUNT) {
      visible_items[visible_count++] = item;
    }
  }
  void addAnalogItem(QuickConfigItem item) {
    if (analog_count < QCI_COUNT) {
      analog_items[analog_count++] = item;
    }
  }
  void clampVisibleSelection() {
    if (visible_count == 0) {
      selected_index = 0;
      on_bottom_row = true;
      bottom_right = false;
      bottom_index = 0;
    } else if (selected_index >= visible_count) {
      selected_index = visible_count - 1;
    }
  }

  const char* getItemName(QuickConfigItem item);
  const char* getItemValue(QuickConfigItem item);

  void setTempRumbleTech(bool enabled);
  void toggleTempRumbleTech() {
    setTempRumbleTech(temp_snes_rumbletech == 0);
  }

  bool isSubmenu(QuickConfigItem item) {
    return item == QCI_INPUT_MODE || item == QCI_OUTPUT_MODE ||
           item == QCI_TURBO || item == QCI_REMAP || item == QCI_RANGE_TEST ||
           item == QCI_RESET_DINPUT_MAP ||
           item == QCI_GUNCON_OFFSET ||
           item == QCI_ANALOG_SETUP || item == QCI_ANALOG_MENU || item == QCI_HOTKEYS_MENU || item == QCI_CLASSIC_DUAL_MERGE ||
           item == QCI_RUMBLE || item == QCI_STICK_CAL;
  }

  void rebuildVisibleAndClampSelection() {
    buildAnalogItems();
    buildVisibleItems();
    clampVisibleSelection();
    // If selected option becomes unsupported, clamp values to valid ranges.
    if (!isTempDpadModeAllowed(temp_dpad_mode)) temp_dpad_mode = DPAD_MODE_DPAD;
    uint8_t invert_max = getStickInvertMax(temp_input_mode);
    if (temp_stick_invert > invert_max) temp_stick_invert = invert_max;
    uint8_t trigger_max = getTriggerModeMax(temp_output_mode);
    // Right Stick has its own row, so the maximum does not count it.
    if ((temp_triggers > trigger_max && temp_triggers != TRIGGER_MODE_RSTICK) ||
        !isTempTriggerModeAllowed(temp_triggers)) {
      temp_triggers = TRIGGER_MODE_ANALOG;
    }
  }

public:
  static constexpr DeviceEnum defaultInputMode() {
#ifdef ENABLE_INPUT_SNES
    return RZORD_SNES;
#elif defined(ENABLE_INPUT_JVS)
    return RZORD_JVS;
#elif defined(ENABLE_INPUT_USB)
    return RZORD_USB;
#elif defined(ENABLE_INPUT_SATURN)
    return RZORD_SATURN;
#else
    return RZORD_NONE;
#endif
  }

  QuickConfigMenu()
    : state(QC_CLOSED), selected_index(0), turbo_index(0), remap_index(0),
      analog_index(0), hotkey_index(0), dual_merge_index(0), rumble_index(0),
      needs_redraw(true), visible_count(0),
      analog_count(0), hotkey_count(0),
      temp_input_mode(defaultInputMode()), temp_output_mode(OUTPUT_MISTER),
      temp_win_output(0), temp_ps2_remote_output(0),
      temp_pointer_source(POINTER_MOUSE_OFF),
      temp_touchpad_mouse(POINTER_MOUSE_SPEED_DEFAULT),
      temp_dualsense_led_policy(0x03),
      temp_usb2db15_b6_source(0),
      temp_snes_rumbletech(0), temp_rumble(3),
      temp_gamecube_l_switch(GAMECUBE_L_SWITCH_ZL),
      temp_classic_dual_merge_p2_mask(0),
      guncon_edit_axis(0), should_save(false), defaults_restored(false),
      range_last_lx(0), range_last_ly(0), range_last_rx(0), range_last_ry(0),
      range_test_target(analogDiagnosticNoTarget()),
      range_test_right_stick(false), range_trace_mode(RZORD_NONE),
      range_trace_octagonal(false),
      range_trace_dirty(false), range_last_draw_ms(0),
      on_bottom_row(false), bottom_right(false), bottom_index(0),
      default_confirm_index(0), dinput_reset_confirm_index(0),
      nso_1p_confirm_index(0),
      reset_dinput_device(0xFF), dinput_reset_connection_order(0),
      dinput_reset_key{}, reset_dinput_completed(false),
      action_error(false),
      return_to_analog_submenu(false),
      transaction_runtime_mode(RZORD_NONE), transaction_staged_mode(RZORD_NONE),
      cal_lx_min(0), cal_lx_max(0), cal_ly_min(0), cal_ly_max(0),
      cal_rx_min(0), cal_rx_max(0), cal_ry_min(0), cal_ry_max(0),
      cal_selection(0), cal_has_data(false) {
    for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
      temp_rates[i] = TURBO_OFF;
    }
  }

  bool isOpen() {
    return state != QC_CLOSED;
  }

  // Buttons the open analog test is measuring, which must not also confirm
  // or go back: the neGcon's I, II and L press neutral A, X and L1 past their
  // threshold. Its A button (and Back) still leave the test.
  uint32_t analogTestMeasuredButtons() const {
    return state == QC_RANGE_TEST &&
           range_test_target.kind == AnalogDiagnosticKind::NeGcon
        ? (INPUT_A | INPUT_X | INPUT_L1) : 0;
  }

  void open();
  // Open with a screen already selected, e.g. the turbo list as the last
  // USB2DB15 setup step. Everything open() prepares is prepared first.
  void openAt(QuickConfigState initial);

  void close() {
    state = QC_CLOSED;
    needs_redraw = true;
    // Note: should_save is NOT set here - it's set by applyAndClose() before calling close()
    // Only set to false explicitly when canceling
  }

  void discard() {
    should_save = false;  // Cancel - don't save
    transaction.discardForActiveMode(deviceMode == transaction_runtime_mode,
                                     active_remaps, analogCalibration);
    close();
  }

  bool shouldSave() {
    return should_save;
  }

  bool takeResetDinputDevice(uint8_t* device_index);
  bool takeActionError();

  bool defaultsWereRestored() {
    return defaults_restored;
  }
  void applyAndClose();
  void openDefaultConfirmation();
  bool saveOrConfirmNsoSinglePlayer();
  bool saveShortcut();
  bool finishEditorForHold(bool save);

  // Get temp rates/settings for saving
  void getTempRates(uint8_t* rates);
  void getTempQuickSettings(DeviceEnum mode, PerModeQuickSettings& settings);
  uint8_t debugBuildVisibleItems(DeviceEnum inputMode, outputMode_t outputMode,
                                 uint8_t winOutput, QuickConfigItem* items,
                                 uint8_t maxItems);
  void getSnapshot(QuickConfigSnapshot& snapshot);
  uint8_t debugCopyVisibleItems(QuickConfigItem* items, uint8_t capacity) const {
    const uint8_t count = visible_count < capacity ? visible_count : capacity;
    for (uint8_t i = 0; i < count; ++i) items[i] = visible_items[i];
    return count;
  }

  // Get temp settings for saving
  DeviceEnum getTempInputMode() { return temp_input_mode; }
  // Saves the staged remaps to the selected input+output pair.
  bool saveStagedRemaps();
  bool saveStagedMouseSettings();
  outputMode_t getTempOutputMode() { return temp_output_mode; }
  uint8_t getTempDeadzone() { return temp_deadzone * 5; }
  socdMode_t getTempSocd() { return temp_socd; }
  uint8_t getTempDpadMode() { return temp_dpad_mode; }
  uint8_t getTempStickInvert() { return temp_stick_invert; }
  uint8_t getTempRumble() { return temp_rumble; }
  uint8_t getTempTriggers() { return temp_triggers; }
  int8_t getTempGunconX() { return temp_guncon_x; }
  int8_t getTempGunconY() { return temp_guncon_y; }
  uint8_t getTempBtnMap() { return temp_btn_map; }
  uint8_t getTempN64Z() { return temp_n64_z; }
  uint8_t getTempN64CStickMode() { return temp_n64_cstick; }
  uint8_t getTempN64AnalogRange() { return temp_n64_range; }

  // Handle navigation down (reset/boot button or D-Pad Down)
  void navigate() {
    navigateDown();
  }

  void navigateDown();

  // Handle navigation up (D-Pad Up)
  void navigateUp();

  // Cycle value forward (D-Pad Right) or activate/select in submenus
  // isSelectAction = true when triggered by mode/face button (should execute on bottom row)
  // isSelectAction = false when triggered by right dpad (should toggle on bottom row)
  void cycleValueNext(bool isSelectAction = false);

  // Cycle value backward (D-Pad Left), without activating submenu/Back rows.
  void cycleValuePrev();

  bool canCycleValueWithDpad();

  // Check if item is directly editable with left/right (no submenu needed)
  bool isDirectlyEditable(QuickConfigItem item);

  // Check if item needs a submenu (entered with Select).
  bool needsSubmenu(QuickConfigItem item);

  // Enter submenu for current item after a Select action.
  void enterSubmenu(QuickConfigItem item);

  // Cycle current main menu item value forward (or activate exit items)
  void cycleMainMenuItemNext();

  // Cycle current main menu item value backward (or activate exit items)
  void cycleMainMenuItemPrev();

  void cycleSettingValue();
  void cycleSettingValueBack();

  // Back/cancel action (B button semantics)
  // Main menu: cancel and close
  // Submenus: return to main menu without closing
  void back();

  // Handle selection (mode button)
  void select();
  void handleAnalogSelect();
  void handleAnalogSelectBack();
  void handleHotkeySelect(bool forward);
  void cycleHotkeyItem(QuickConfigItem item, bool forward);
  void handleDualMergeSelect();
  void handleDualMergeSelectBack();
  void handleRumbleSelect(bool forward = true);
  void handleGunconSelect();
  void handleGunconNavigate();
  void handleGunconNavigateBack();

  // Render the menu (called when display is available)
  void render(SSD1306AsciiWire& display);

#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
  // Analog Setup may change the axis mapping while this Quick session stays open.
  // Rebuild rows without sanitizing or discarding other staged settings.
  void refreshJvsCabinet() {
    buildAnalogItems();
    buildVisibleItems();
    clampVisibleSelection();
    needs_redraw = true;
  }
#endif

  // Force redraw on next render (for live updates like range test)
  void forceRedraw() {
    needs_redraw = true;
  }

private:
  void renderMainMenu(SSD1306AsciiWire& display);
  void renderDefaultConfirm(SSD1306AsciiWire& display);
  void renderDinputResetConfirm(SSD1306AsciiWire& display);
  void renderNsoSinglePlayerConfirm(SSD1306AsciiWire& display);
  void renderTurboList(SSD1306AsciiWire& display);
  void renderTurboRate(SSD1306AsciiWire& display);
  void renderSettingEdit(SSD1306AsciiWire& display);
  uint8_t modePickerCount() const;
  void openModePicker(QuickConfigItem item);
  void confirmModePicker();
  void renderModePicker(SSD1306AsciiWire& display);
  uint8_t choicePickerCount() const;
  uint8_t choicePickerCurrent();
  const char* choicePickerLabel(uint8_t index);
  bool pointerSourceAvailable(uint8_t source);
  uint8_t fillChoiceValues(QuickConfigItem item, uint8_t* values);
  uint8_t fillRemapChoices();
  uint8_t fillHotkeyChoices(QuickConfigItem item);
  uint8_t swapChoiceValue(uint8_t value);
  uint8_t remapChoiceSlot();
  const char* remapRowTarget(uint8_t index);
  void printChoicePickerTitle(SSD1306AsciiWire& target);
  void openChoicePicker(QuickConfigItem item);
  void confirmChoicePicker();
  void renderChoicePicker(SSD1306AsciiWire& display);
  void renderAnalogSubmenu(SSD1306AsciiWire& display);
  void renderHotkeysSubmenu(SSD1306AsciiWire& display);
  void renderDualMergeMap(SSD1306AsciiWire& display);
  void renderRumbleSubmenu(SSD1306AsciiWire& display);
  void renderGunconAdjust(SSD1306AsciiWire& display);
  void renderRangeTest(SSD1306AsciiWire& display);
  void renderStickCal(SSD1306AsciiWire& display);
  void renderRemapList(SSD1306AsciiWire& display);
};

// Global quick config menu instance
extern QuickConfigMenu quickConfig;

uint8_t quickConfigItemSettingIds(QuickConfigItem item, DeviceEnum mode,
                                  SettingId* ids, uint8_t maxIds);
