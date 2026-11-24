#include "../product_config.h"
#include "../core/directional_hotkeys.h"

#include <string.h>

#include "quick_config.h"
#include "../core/negcon_input_modes.h"
#include "../core/remap_pairs.h"
#include "../core/rotary_settings.h"
#include "../core/settings_registry.h"
#include "../core/controller_feedback_config.h"
#include "../input/runtime/input_frame_runtime.h"
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
#include "../features/usb2db15/usb2db15_arcade_mapping.h"
#include "../features/usb2db15/usb2db15_arcade_session.h"
#include "../features/usb2db15/usb2db15_game_artwork.h"
#endif

#ifdef ENABLE_INPUT_PSX
#include "../input/psx/input_psx_guncon_runtime_state.h"
#include "../input/psx/input_psx_jogcon_runtime_state.h"
#endif

namespace {

PerModeQuickSettings buildDefaultQuickSettingsForMode(DeviceEnum mode) {
  PerModeQuickSettings settings{};
  for (uint8_t i = 0; i < static_cast<uint8_t>(SettingId::Count); ++i) {
    const SettingId id = static_cast<SettingId>(i);
    if (!settingIsPerMode(id)) {
      continue;
    }
    writeRecordSettingValue(settings, id, defaultSettingValue(id, mode));
  }
  for (uint8_t i = 0; i < PER_MODE_TURBO_SIZE; ++i) {
    settings.turbo_rates[i] = TURBO_OFF;
  }
  for (uint8_t i = 0; i < PER_MODE_REMAP_SIZE; ++i) {
    settings.remaps[i] = i;
  }
  sanitizePerModeQuickSettings(mode, settings);
  return settings;
}

AnalogCalibrationData calibrationFromQuickSettings(
    const PerModeQuickSettings& settings) {
  return {
    .lx = { .raw_min = settings.analog_cal_lx_min,
            .raw_max = settings.analog_cal_lx_max, .raw_center = 0 },
    .ly = { .raw_min = settings.analog_cal_ly_min,
            .raw_max = settings.analog_cal_ly_max, .raw_center = 0 },
    .rx = { .raw_min = settings.analog_cal_rx_min,
            .raw_max = settings.analog_cal_rx_max, .raw_center = 0 },
    .ry = { .raw_min = settings.analog_cal_ry_min,
            .raw_max = settings.analog_cal_ry_max, .raw_center = 0 },
    .enabled = settings.analog_cal_enabled != 0,
  };
}

}  // namespace

bool QuickConfigMenu::canEditModeSpecificSettings() {
  return isConcreteModeForPerSettings(temp_input_mode);
}

bool QuickConfigMenu::isLiveInputSelection() {
  return temp_input_mode == deviceMode;
}

const TurboButtonConfig& QuickConfigMenu::getSelectedTurboConfig() {
  #ifdef ENABLE_INPUT_PSX
  if (temp_input_mode == RZORD_PSX && isLiveInputSelection()) {
    const char* typeName = selectedControllerFrame().controller_type_name;
    if (strncmp(typeName, "JogCon", 6) == 0) {
      return getTurboButtonConfig(TURBO_MODE_PSX_JOG);
    }
    if (strcmp(typeName, "neGcon") == 0) {
      return getTurboButtonConfig(TURBO_MODE_PSX_NEGCON);
    }
  }
  #endif
  return getTurboButtonConfig(getTurboInputModeForDeviceMode(temp_input_mode));
}

void QuickConfigMenu::loadTempQuickSettingsRecord(DeviceEnum mode,
                                                  const PerModeQuickSettings& settings) {
  temp_deadzone = settings.deadzone_percent / 5;
  temp_socd = (socdMode_t)settings.socd;
  temp_dpad_mode = settings.dpad_mode;
  temp_stick_invert = settings.stick_invert;
  temp_snes_rumbletech = 1;
  temp_rumble = normalizeRumbleLevelForSupport(
    rumbleStrengthSupportForMode(mode),
    settings.rumble_level
  );
  temp_triggers = settings.trigger_mode;
  temp_guncon_x = settings.guncon_offset_x;
  temp_guncon_y = settings.guncon_offset_y;
  temp_spinner = settings.spinner_speed;
  temp_jogcon_mode = settings.jogcon_mode;
  temp_jogcon_force = settings.jogcon_force;
  temp_wheel_sens = rotaryLegacyLowBits(settings.wheel_sensitivity);
  temp_jogcon_digital = rotaryLegacyLowBits(settings.jogcon_digital_map);
  temp_jogcon_wheel_axis = settings.jogcon_wheel_axis;
  temp_btn_map = settings.button_map_mode;
#if ADAPT_POLICY_QUICK_RETAIN_RETRO_FIELDS
  temp_gamecube_l_switch = settings.gamecube_l_switch_mode;
#else
  temp_gamecube_l_switch = GAMECUBE_L_SWITCH_ZL;
#endif
  temp_n64_z = settings.n64_z_mode;
  temp_n64_cstick = settings.n64_cstick_mode;
  const uint8_t psxPollTiming =
    psx_poll::storedInRecord(mode) ? settings.psx_poll_timing : 0;
  temp_psx_poll_rate = psx_poll::rate(psxPollTiming);
  temp_psx_cmd_gap = psx_poll::gap(psxPollTiming);
#if defined(ADAPT_CAP_DIRECTIONAL_HOTKEYS) && !defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
  direction_hotkey::read(settings, temp_direction_hotkeys);
#endif
  temp_n64_range = sanitizeClassicAnalogRange(settings.n64_analog_range);
  temp_wii_range = sanitizeClassicAnalogRange(
    readRecordSettingValue(settings, SettingId::WiiAnalogRange));
  temp_paddle_divisor = static_cast<uint8_t>(readRecordSettingValue(
    settings, SettingId::PaddleDivisor));
  temp_rotary_reverse = readRecordSettingValue(
    settings, SettingId::RotaryReverse) != 0;
  temp_driving_wrap = readRecordSettingValue(
    settings, SettingId::DrivingWrap) != 0;
  temp_rotary_profile = rotaryProfileFromJogconDigitalMap(settings.jogcon_digital_map);
  temp_jogcon_rotary_profile = rotaryProfileFromWheelSensitivity(settings.wheel_sensitivity);
  temp_nso_special = sanitize_switch_device_selection(mode, settings.nso_special);
  temp_powerpad = sanitizeNesDeviceMode(settings.powerpad_mode);
  temp_negcon_digital = negconInputsStoredInRecord(mode)
    ? negconDigitalInputsFromRaw(settings.psx_negcon_inputs) : 0;
  temp_negcon_gain = negconInputsStoredInRecord(mode)
    ? negconGainFromRaw(settings.psx_negcon_inputs) : 0;
  temp_classic_dual_merge = classicDualMergeEnabledForMode(mode) ? 1 : 0;
  temp_classic_dual_merge_p2_mask = storedClassicDualMergeP2Mask(mode);

  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    temp_rates[i] = (settings.turbo_rates[i] < TURBO_RATE_LAST) ? (TurboRate)settings.turbo_rates[i] : TURBO_OFF;
  }

  clampLoadedQuickSettings(mode, settings.rumble_level);
}

void QuickConfigMenu::clampLoadedQuickSettings(DeviceEnum mode, uint8_t storedRumble) {
  const uint8_t storedDpadMode = temp_dpad_mode;
  const uint8_t storedStickInvert = temp_stick_invert;
  const uint8_t storedTriggers = temp_triggers;
  if (temp_deadzone > 6) temp_deadzone = 6;
  if (temp_socd >= SOCD_LAST_ENUM) temp_socd = SOCD_OFF;
  if (!isTempDpadModeAllowed(temp_dpad_mode)) {
    temp_dpad_mode = DPAD_MODE_DPAD;
  }
  if (temp_stick_invert > getStickInvertMax(mode)) {
    temp_stick_invert = getStickInvertMax(mode);
  }
  if (!isTempTriggerModeAllowed(temp_triggers)) {
    temp_triggers = TRIGGER_MODE_ANALOG;
  }
  stored_dpad_mode = {storedDpadMode, temp_dpad_mode};
  stored_stick_invert = {storedStickInvert, temp_stick_invert};
  stored_rumble = {storedRumble, temp_rumble};
  stored_triggers = {storedTriggers, temp_triggers};
}

void QuickConfigMenu::loadTempQuickSettingsForMode(DeviceEnum mode) {
  PerModeQuickSettings settings;
  readPerModeQuickSettings(mode, settings);
  loadTempQuickSettingsRecord(mode, settings);
  uint8_t remaps[REMAP_MAX_BUTTONS];
  readRemapsForPair(mode, temp_output_mode, remaps);
  transaction.stage(remaps, calibrationFromQuickSettings(settings));
  temp_dpad2_mode = dreamcastTwinStickModeFromRemaps(remaps);
  transaction_staged_mode = mode;
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  temp_mouse = rotaryMouseSettingsRead(mode);
#endif
}

void QuickConfigMenu::restageRemapsForOutput() {
  if (transaction_staged_mode != temp_input_mode) return;
  uint8_t remaps[REMAP_MAX_BUTTONS];
  readRemapsForPair(temp_input_mode, temp_output_mode, remaps);
  transaction.resetRemaps(remaps);
  temp_dpad2_mode = dreamcastTwinStickModeFromRemaps(remaps);
}

bool QuickConfigMenu::saveStagedRemaps() {
  if (transaction_staged_mode != temp_input_mode) return true;
  return writeRemapsForPair(temp_input_mode, temp_output_mode, transaction.remaps());
}

bool QuickConfigMenu::saveStagedMouseSettings() {
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  if (transaction_staged_mode != temp_input_mode || !canEditModeSpecificSettings()) return true;
  const bool countChanged = rotaryMousePlayerCount(rotaryMouseSettingsRead(temp_input_mode)) !=
                            rotaryMousePlayerCount(temp_mouse);
  if (!rotaryMouseSettingsWrite(temp_input_mode, temp_mouse)) return false;
  // USB interfaces are fixed at enumeration; a saved player-count change
  // needs the same descriptor restart as an output-mode change.
  if (countChanged && isLiveInputSelection() && mousePageActive()) {
    menu_usb_descriptor_reboot_required = 1;
  }
  return true;
#else
  return true;
#endif
}

#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
void QuickConfigMenu::loadDirectionalControllerHotkeys() {
  const uint8_t port = selectedControllerPlayer();
  temp_direction_hotkey_identity = directionalHotkeyEditIdentity(port);
  temp_direction_hotkey_home = 0;
  memset(temp_direction_hotkeys, 0, sizeof(temp_direction_hotkeys));
  // A missing/changed controller deliberately starts at Auto.  Saving still
  // validates the captured identity, so a replacement can never inherit this
  // controller's staged shortcuts.
  loadDirectionalHotkeySelection(port, temp_direction_hotkeys,
                                 temp_direction_hotkey_home);
}
#endif

void QuickConfigMenu::resetTempQuickSettingsToDefaults() {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  temp_usb2db15_activation = 2;
  temp_usb2db15_auto_shoot = 0;
#endif
  const PerModeQuickSettings settings = buildDefaultQuickSettingsForMode(temp_input_mode);
  // PSX Poll Rate and Cmd Gap have no Quick Menu row, so Default keeps them.
  const uint8_t psxPollRate = temp_psx_poll_rate;
  const uint8_t psxCmdGap = temp_psx_cmd_gap;
  loadTempQuickSettingsRecord(temp_input_mode, settings);
  temp_psx_poll_rate = psxPollRate;
  temp_psx_cmd_gap = psxCmdGap;
  if (isSnesMode(temp_input_mode)) {
    temp_snes_rumbletech = 1;
  }
  temp_ps2_remote_output = 0;
  temp_dualsense_led_policy = DUALSENSE_LED_ALL;
  temp_classic_dual_merge = 0;
  temp_classic_dual_merge_p2_mask = defaultClassicDualMergeP2Mask(temp_input_mode);
  uint8_t remaps[REMAP_MAX_BUTTONS];
  remapTableFromRecord(remaps, settings.remaps);
  transaction.resetToDefaults(remaps);
  transaction_staged_mode = temp_input_mode;
  resetTempRemapsToDefaults();
  temp_mouse = RotaryMouseSettings{};
#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
  memset(temp_direction_hotkeys, 0, sizeof(temp_direction_hotkeys));
  temp_direction_hotkey_home = 0;
#endif
  rebuildVisibleAndClampSelection();
}

void QuickConfigMenu::resetTempRemapsToDefaults() {
  uint8_t remaps[REMAP_MAX_BUTTONS];
  remapTableFromRecord(remaps, buildDefaultQuickSettingsForMode(temp_input_mode).remaps);
  if (temp_input_mode == RZORD_DREAMCAST && isLiveInputSelection() &&
      controllerFrameIsDreamcastTwinStick(selectedControllerFrame())) {
    remaps[DREAMCAST_TWIN_STICK_MODE_SLOT] =
        dreamcastTwinStickModeEncoding(DPAD_MODE_BUTTONS);
    if (remapPairOutputFor(temp_output_mode) == OUTPUT_XINPUT ||
        remapPairOutputFor(temp_output_mode) == OUTPUT_XINPUT2P) {
      remaps[16] = 4;
      remaps[14] = 5;
      remaps[17] = 8;
      remaps[18] = 9;
      remaps[15] = 11;
    }
    temp_dpad2_mode = DPAD_MODE_BUTTONS;
  }
  transaction.resetRemaps(remaps);
}

bool QuickConfigMenu::isNsoSpecialActiveForSelection(DeviceEnum mode) {
  const outputMode_t selectedOutput =
    (temp_output_mode == OUTPUT_AUTO) ? get_effective_output_mode() : temp_output_mode;
  return output_mode_supports_switch_device_selection(selectedOutput) &&
         switch_device_selection_is_nso_for_input_mode(mode, temp_nso_special) &&
         input_mode_supports_nso_special(mode) &&
         input_mode_allows_nso_special_for_connected_players(
           mode, connectedInputFrameCount(inputPortCount()));
}

void QuickConfigMenu::getSnapshot(QuickConfigSnapshot& snapshot) {
  snapshot.state = (uint8_t)state;
  snapshot.cursor = selected_index;
  snapshot.top = selected_index > 2 ? (uint8_t)(selected_index - 2) : 0;
  snapshot.item = 0xFF;
  snapshot.count = visible_count;
  snapshot.flags = 0;

  if (on_bottom_row) snapshot.flags |= 0x01;
  if (bottom_right) snapshot.flags |= 0x02;
  if (should_save) snapshot.flags |= 0x04;

  switch (state) {
    case QC_TURBO_LIST:
      snapshot.cursor = turbo_index;
      break;
    case QC_REMAP_LIST:
    case QC_REMAP_SELECT:
      snapshot.cursor = remap_index;
      break;
    case QC_ANALOG_SUBMENU:
      snapshot.cursor = analog_index;
      snapshot.count = analog_count;
      if (analog_index < analog_count) {
        snapshot.item = (uint8_t)analog_items[analog_index];
      }
      break;
    case QC_HOTKEYS_SUBMENU:
      snapshot.cursor = hotkey_index;
      snapshot.count = hotkey_count;
      if (hotkey_index < hotkey_count) {
        snapshot.item = (uint8_t)hotkey_items[hotkey_index];
      }
      break;
    case QC_DUAL_MERGE_MAP:
      snapshot.cursor = dual_merge_index;
      snapshot.count = getDualMergeMenuCount();
      snapshot.item = (uint8_t)QCI_CLASSIC_DUAL_MERGE;
      break;
    case QC_RUMBLE_SUBMENU:
      snapshot.cursor = rumble_index;
      snapshot.count = QCI_RUMBLE_COUNT;
      snapshot.item = (uint8_t)QCI_RUMBLE;
      break;
    case QC_CONFIRM_DINPUT_RESET:
      snapshot.cursor = dinput_reset_confirm_index;
      snapshot.count = 2;
      snapshot.item = (uint8_t)QCI_RESET_DINPUT_MAP;
      break;
    case QC_CONFIRM_NSO_1P:
      snapshot.cursor = nso_1p_confirm_index;
      snapshot.count = 2;
      snapshot.item = (uint8_t)QCI_NSO_SPECIAL;
      break;
    default:
      break;
  }

  if (snapshot.item == 0xFF && selected_index < visible_count) {
    snapshot.item = (uint8_t)visible_items[selected_index];
  }
}

void QuickConfigMenu::open() {
  // Reboot requirements belong to one menu session only. A stale descriptor
  // change must never make an unrelated Pointer Speed save reboot or hang.
  menu_usb_descriptor_reboot_required = 0;
  state = QC_MAIN_MENU;
  selected_index = 0;
  turbo_index = 0;
  remap_index = 0;
  analog_index = 0;
  hotkey_index = 0;
  dual_merge_index = 0;
  on_bottom_row = false;
  bottom_right = false;
  bottom_index = 0;
  default_confirm_index = 0;
  dinput_reset_confirm_index = 0;
  nso_1p_confirm_index = 0;
  reset_dinput_device = 0xFF;
  dinput_reset_connection_order = 0;
  memset(&dinput_reset_key, 0, sizeof(dinput_reset_key));
  reset_dinput_completed = false;
  action_error = false;
  needs_redraw = true;
  should_save = false;
  defaults_restored = false;
  transaction.begin(active_remaps, analogCalibration);
  transaction_runtime_mode = deviceMode;
  transaction_staged_mode = deviceMode;

  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    temp_rates[i] = turbo.getButtonRate((TurboButton)i);
  }
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
  temp_jvs_menu_hold = menu_hotkey_hold_time;
  temp_jvs_home_hotkey = menu_home_hotkey;
  temp_jvs_hold_start = menu_hold_start_hotkey;
#endif
  temp_input_mode = deviceMode;
  temp_output_mode = canonicalizeOutputMode(configuredOutputMode);
  temp_win_output = (menu_win_output > HOST_OUTPUT_PREFERENCE_MAX) ? HOST_OUTPUT_PREFERENCE_DINPUT : menu_win_output;
  temp_ps2_remote_output = menu_ps2_remote_output ? 1 : 0;
  temp_pointer_source = pointerMouseConfigSource(menu_touchpad_mouse);
  temp_touchpad_mouse = pointerMouseConfigSpeed(menu_touchpad_mouse);
#if ADAPT_POLICY_CONTROLLER_FEEDBACK_COMMANDS
  temp_dualsense_led_policy = controllerFeedbackDualSenseLedPolicy();
#else
  temp_dualsense_led_policy = DUALSENSE_LED_ALL;
#endif
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  temp_usb2db15_presentation = static_cast<uint8_t>(usb2db15Presentation());
  temp_usb2db15_activation = usb2db15StickActivation();
  temp_usb2db15_auto_shoot = usb2db15AutoShootCadence();
  temp_usb2db15_b6_source =
      usb2db15ArcadeB6Source() == Usb2Db15B6Source::LeftShoulder ? 1 : 0;
#else
  temp_usb2db15_b6_source = 0;
#endif
  temp_deadzone = deadzone_percent / 5;
  // Edit the user's values, not a transient dance-pad override.
  temp_socd = (socdMode_t)persistedRuntimeSocdMode();
  temp_dpad_mode = persistedRuntimeDpadMode();
  temp_stick_invert = stick_invert;
  temp_snes_rumbletech = 1;
  temp_rumble = normalizeRumbleLevelForSupport(
    rumbleStrengthSupportForMode(temp_input_mode),
    rumble_level
  );
  temp_triggers = trigger_mode;
  temp_guncon_x = menu_guncon_offset_x;
  temp_guncon_y = menu_guncon_offset_y;
  temp_spinner = spinner_speed;
  temp_jogcon_mode = menu_jogcon_mode;
  temp_jogcon_force = menu_jogcon_force;
  temp_wheel_sens = menu_wheel_sensitivity;
  temp_jogcon_digital = menu_jogcon_digital_map;
  temp_jogcon_wheel_axis = menu_jogcon_wheel_axis;
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  temp_mouse = rotaryMouseSettingsRead(deviceMode);
#endif
  temp_btn_map = button_map_mode;
#if ADAPT_POLICY_QUICK_RETAIN_RETRO_FIELDS
  temp_gamecube_l_switch = gamecube_l_switch_mode;
#else
  temp_gamecube_l_switch = GAMECUBE_L_SWITCH_ZL;
#endif
  temp_n64_z = n64_z_mode;
  temp_n64_cstick = n64_cstick_mode;
  temp_psx_poll_rate = psx_poll::rate(psx_poll_timing);
  temp_psx_cmd_gap = psx_poll::gap(psx_poll_timing);
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  memcpy(temp_direction_hotkeys, active_direction_hotkeys, 3);
#elif defined(ADAPT_CAP_DIRECTIONAL_HOTKEYS) && !defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
  PerModeQuickSettings directionalSettings;
  readPerModeQuickSettings(temp_input_mode, directionalSettings);
  direction_hotkey::read(directionalSettings, temp_direction_hotkeys);
#endif
#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
  loadDirectionalControllerHotkeys();
#endif
  temp_n64_range = sanitizeClassicAnalogRange(n64_analog_range);
  temp_wii_range = sanitizeClassicAnalogRange(rotaryAnalogRange(wii_analog_range));
  temp_paddle_divisor = paddle_divisor;
  temp_rotary_reverse = rotary_reverse;
  temp_driving_wrap = driving_wrap;
  temp_rotary_profile = sanitizeRotaryProfile(rotary_profile);
  temp_jogcon_rotary_profile = sanitizeRotaryProfile(jogcon_rotary_profile);
  temp_nso_special = sanitize_switch_device_selection(temp_input_mode, nso_special);
  temp_powerpad = menu_powerpad_mode;
  temp_negcon_digital = negcon_digital_inputs;
  temp_negcon_gain = negcon_gain;
  temp_classic_dual_merge = classicDualMergeEnabledForMode(temp_input_mode) ? 1 : 0;
  temp_classic_dual_merge_p2_mask = storedClassicDualMergeP2Mask(temp_input_mode);
  clampLoadedQuickSettings(temp_input_mode, rumble_level);

  buildVisibleItems();
  clampVisibleSelection();
}

void QuickConfigMenu::openAt(QuickConfigState initial) {
  open();
  state = initial;
  needs_redraw = true;
}

bool QuickConfigMenu::takeResetDinputDevice(uint8_t* deviceIndex) {
  if (!reset_dinput_completed) {
    return false;
  }
  if (deviceIndex != nullptr) {
    *deviceIndex = reset_dinput_device;
  }
  reset_dinput_completed = false;
  reset_dinput_device = 0xFF;
  dinput_reset_connection_order = 0;
  memset(&dinput_reset_key, 0, sizeof(dinput_reset_key));
  return true;
}

bool QuickConfigMenu::takeActionError() {
  const bool result = action_error;
  action_error = false;
  return result;
}

void QuickConfigMenu::applyAndClose() {
#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
  // This must be the first commit.  A controller may have been replaced while
  // its Quick page was open; reject that save before remaps or globals change.
  if (temp_direction_hotkey_identity != 0 &&
      !saveDirectionalHotkeySelection(selectedControllerPlayer(),
                                       temp_direction_hotkey_identity,
                                       temp_direction_hotkeys,
                                       temp_direction_hotkey_home)) {
    should_save = false;
    action_error = true;
    needs_redraw = true;
    return;
  }
#endif
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
  if (temp_jvs_menu_hold != menu_hotkey_hold_time) {
    menu_hotkey_hold_time = temp_jvs_menu_hold;
    saveSystemSettingByte(SettingId::HotkeyHoldTime, menu_hotkey_hold_time);
  }
  if (temp_jvs_home_hotkey != menu_home_hotkey) {
    menu_home_hotkey = temp_jvs_home_hotkey;
    saveSystemSettingByte(SettingId::HomeHotkey, menu_home_hotkey);
  }
  if (temp_jvs_hold_start != menu_hold_start_hotkey) {
    menu_hold_start_hotkey = temp_jvs_hold_start;
    saveSystemSettingByte(SettingId::HoldStartHotkey, menu_hold_start_hotkey);
  }
#endif
  should_save = true;
  if (isLiveInputSelection() && transaction_staged_mode == deviceMode) {
    if (temp_input_mode == RZORD_DREAMCAST &&
        controllerFrameIsDreamcastTwinStick(selectedControllerFrame())) {
      transaction.remaps()[DREAMCAST_TWIN_STICK_MODE_SLOT] =
          dreamcastTwinStickModeEncoding(temp_dpad2_mode);
    }
    transaction.commit(active_remaps, analogCalibration);
  }
  const uint8_t newWinOutput = (temp_win_output > HOST_OUTPUT_PREFERENCE_MAX) ? HOST_OUTPUT_PREFERENCE_DINPUT : temp_win_output;
  const bool windowsOutputDescriptorChanged =
    newWinOutput != menu_win_output &&
    configuredOutputMode == OUTPUT_AUTO &&
    autoDetectState == AUTO_STATE_WINDOWS;
  const outputMode_t selectedOutput =
    (temp_output_mode == OUTPUT_AUTO) ? get_effective_output_mode() : temp_output_mode;
  const bool nsoDescriptorChanged =
    isLiveInputSelection() && output_mode_supports_switch_device_selection(selectedOutput) &&
    input_mode_supports_nso_special(temp_input_mode) &&
    (temp_nso_special != menu_nso_special);
  if (newWinOutput != menu_win_output) {
    auto_detect_clear_scratch_state();
  }
  menu_win_output = newWinOutput;
  saveSystemSettingByte(SettingId::WinOutput, menu_win_output);
  bool ps2RemoteDescriptorChanged = false;
  if (settingSupportedOnProduct(SettingId::Ps2RemoteOutput)) {
    ps2RemoteDescriptorChanged =
      temp_ps2_remote_output != menu_ps2_remote_output;
    menu_ps2_remote_output = temp_ps2_remote_output ? 1 : 0;
    saveSystemSettingByte(SettingId::Ps2RemoteOutput, menu_ps2_remote_output);
  }
  menu_touchpad_mouse = pointerMouseEncodeConfigKeepingSpeed(
      static_cast<PointerMouseSource>(temp_pointer_source),
      temp_touchpad_mouse);
  saveSystemSettingByte(SettingId::TouchpadMouse, menu_touchpad_mouse);
#if ADAPT_POLICY_CONTROLLER_FEEDBACK_COMMANDS
  if (controllerFeedbackDualSenseLedPolicy() !=
      temp_dualsense_led_policy) {
    setControllerFeedbackDualSenseLedPolicy(temp_dualsense_led_policy);
    controllerFeedbackConfigSave();
  }
#endif
  // Retain the persisted field for EEPROM compatibility, but RumbleTech
  // support is automatic and no longer user-configurable.
  menu_snes_rumbletech = 1;
  snes_rumbletech_enabled = 1;
  if (windowsOutputDescriptorChanged || nsoDescriptorChanged ||
      ps2RemoteDescriptorChanged) {
    menu_usb_descriptor_reboot_required = 1;
  }
#if defined(ADAPT_CAP_DUAL_INPUT_MERGE)
  if (isLiveInputSelection() &&
      (temp_classic_dual_merge ? 1 : 0) != menu_classic_dual_merge) {
    menu_usb_descriptor_reboot_required = 1;
  }
#endif
#if defined(ADAPT_CAP_DUAL_INPUT_MERGE)
  saveClassicDualMergeEnabledForMode(temp_input_mode, temp_classic_dual_merge != 0);
  saveClassicDualMergeP2MaskForMode(temp_input_mode, temp_classic_dual_merge_p2_mask);
  if (isLiveInputSelection()) {
    menu_classic_dual_merge = temp_classic_dual_merge ? 1 : 0;
    classic_dual_merge_enabled = menu_classic_dual_merge;
    classic_dual_merge_p2_mask = sanitizeClassicDualMergeP2Mask(
      temp_input_mode,
      temp_classic_dual_merge_p2_mask
    );
  }
#else
  // This record is not allocated on products without dual-input merge. Keep
  // shared quick state neutral without routing the hidden item to persistence.
  temp_classic_dual_merge = 0;
  temp_classic_dual_merge_p2_mask = 0;
  menu_classic_dual_merge = 0;
  classic_dual_merge_enabled = 0;
  classic_dual_merge_p2_mask = 0;
#endif

#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  usb2db15SetPresentation(static_cast<Usb2Db15Presentation>(temp_usb2db15_presentation));
  usb2db15SetPlayerTuning(temp_usb2db15_activation, temp_usb2db15_auto_shoot);
  usb2db15ArcadeSetB6Source(
      temp_usb2db15_b6_source == 1
          ? Usb2Db15B6Source::LeftShoulder
          : Usb2Db15B6Source::RightTrigger);
#endif

  if (isLiveInputSelection()) {
    for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
      turbo.setButtonRate((TurboButton)i, temp_rates[i]);
    }

    deadzone_percent = temp_deadzone * 5;
    menu_deadzone_percent = deadzone_percent;
    socdMode = temp_socd;
    menu_socdMode = temp_socd;
    dpad_mode = stored_dpad_mode.saved(temp_dpad_mode);
    menu_dpad_mode = dpad_mode;
    // A connected auto-detected dance pad re-applies its override on the next
    // poll, keeping these new values as the ones to restore.
    forgetTransientDancePadOverride();
    stick_invert = stored_stick_invert.saved(temp_stick_invert);
    menu_stick_invert = stick_invert;
    rumble_level = stored_rumble.saved(temp_rumble);
    menu_rumble_level = rumble_level;
    trigger_mode = stored_triggers.saved(temp_triggers);
    menu_trigger_mode = trigger_mode;
    menu_guncon_offset_x = temp_guncon_x;
    menu_guncon_offset_y = temp_guncon_y;
#ifdef ENABLE_INPUT_PSX
    // The gun reads these, as WebHID's GunCon X/Y writes do; until
    // 2026-11-12 a saved GunCon Align moved the aim only after a restart.
    offsetX = temp_guncon_x;
    offsetY = temp_guncon_y;
#endif
    spinner_speed = temp_spinner;
    menu_jogcon_mode = temp_jogcon_mode;
    menu_jogcon_force = temp_jogcon_force;
    menu_wheel_sensitivity = temp_wheel_sens;
    menu_jogcon_digital_map = temp_jogcon_digital;
    menu_jogcon_wheel_axis = temp_jogcon_wheel_axis;
    button_map_mode = temp_btn_map;
    menu_button_map = temp_btn_map;
#if ADAPT_POLICY_QUICK_RETAIN_RETRO_FIELDS
    gamecube_l_switch_mode = temp_gamecube_l_switch;
    menu_gamecube_l_switch_mode = temp_gamecube_l_switch;
#endif
    n64_z_mode = temp_n64_z;
    menu_n64_z_mode = temp_n64_z;
    if (psx_poll::storedInRecord(temp_input_mode)) {
      psx_poll_timing = psx_poll::encode(temp_psx_poll_rate, temp_psx_cmd_gap);
    }
    n64_cstick_mode = temp_n64_cstick;
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    memcpy(active_direction_hotkeys, temp_direction_hotkeys, 3);
#endif
    menu_n64_cstick_mode = temp_n64_cstick;
    n64_analog_range = temp_n64_range;
    menu_n64_analog_range = temp_n64_range;
    wii_analog_range = temp_wii_range;
    menu_wii_analog_range = temp_wii_range;
    paddle_divisor = temp_paddle_divisor;
    rotary_reverse = temp_rotary_reverse;
    driving_wrap = temp_driving_wrap;
    rotary_profile = sanitizeRotaryProfile(temp_rotary_profile);
    jogcon_rotary_profile = sanitizeRotaryProfile(temp_jogcon_rotary_profile);
    nso_special = sanitize_switch_device_selection(temp_input_mode, temp_nso_special);
    menu_nso_special = nso_special;
    menu_powerpad_mode = temp_powerpad;
    if (negconInputsStoredInRecord(temp_input_mode)) {
      negcon_digital_inputs = temp_negcon_digital & NEGCON_INPUT_STORED_MASK;
      negcon_gain = temp_negcon_gain % NEGCON_GAIN_COUNT;
    }
    #ifdef ENABLE_INPUT_PSX
    jogcon_wheelCenterSet = false;
    #endif
  }

  close();
}

void QuickConfigMenu::getTempRates(uint8_t* rates) {
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; i++) {
    rates[i] = temp_rates[i];
  }
}

void QuickConfigMenu::getTempQuickSettings(DeviceEnum mode, PerModeQuickSettings& settings) {
  readPerModeQuickSettings(mode, settings);
  settings.deadzone_percent = temp_deadzone * 5;
  settings.socd = (uint8_t)temp_socd;
  settings.dpad_mode = stored_dpad_mode.saved(temp_dpad_mode);
  settings.stick_invert = stored_stick_invert.saved(temp_stick_invert);
  settings.rumble_level = stored_rumble.saved(temp_rumble);
  settings.trigger_mode = stored_triggers.saved(temp_triggers);
  settings.spinner_speed = temp_spinner;
  settings.jogcon_mode = temp_jogcon_mode;
  settings.jogcon_force = temp_jogcon_force;
  settings.button_map_mode = temp_btn_map;
#if ADAPT_POLICY_QUICK_RETAIN_RETRO_FIELDS
  settings.gamecube_l_switch_mode = temp_gamecube_l_switch;
#endif
  if (psx_poll::storedInRecord(mode)) {
    settings.psx_poll_timing = psx_poll::encode(temp_psx_poll_rate, temp_psx_cmd_gap);
  }
  settings.n64_z_mode = temp_n64_z;
  settings.n64_cstick_mode = temp_n64_cstick;
  settings.n64_analog_range = temp_n64_range;
  writeRecordSettingValue(settings, SettingId::WiiAnalogRange, temp_wii_range);
  writeRecordSettingValue(settings, SettingId::PaddleDivisor, temp_paddle_divisor);
  writeRecordSettingValue(settings, SettingId::RotaryReverse,
                          temp_rotary_reverse ? 1 : 0);
  writeRecordSettingValue(settings, SettingId::DrivingWrap,
                          temp_driving_wrap ? 1 : 0);
  if (negconInputsStoredInRecord(mode)) {
    settings.psx_negcon_inputs = encodeNegconInputs(temp_negcon_digital, temp_negcon_gain);
  } else {
    settings.powerpad_mode = temp_powerpad;
  }
  settings.nso_special = sanitize_switch_device_selection(mode, temp_nso_special);
  settings.guncon_offset_x = temp_guncon_x;
  settings.guncon_offset_y = temp_guncon_y;
#if ADAPT_POLICY_QUICK_RETAIN_RETRO_FIELDS
  // MODERN2USB owns the retired feedback fields for controller/device LED
  // preferences. Preserve the record loaded above when Quick Config saves.
#if defined(ADAPT_CAP_DIRECTIONAL_HOTKEYS) && !defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
#if !ADAPT_POLICY_FIXED_HOTKEY_ASSIGNMENTS
  direction_hotkey::write(settings, temp_direction_hotkeys);
#endif
#else
  settings.reserved_musical_buttons = 0;
#endif
#endif
  settings.wheel_sensitivity = withRotaryProfileInWheelSensitivity(
    temp_wheel_sens, temp_jogcon_rotary_profile);
  settings.jogcon_digital_map = withRotaryProfileInJogconDigitalMap(
    temp_jogcon_digital, temp_rotary_profile);
  settings.jogcon_wheel_axis = temp_jogcon_wheel_axis;
  if (mode == transaction_staged_mode) {
    transaction.copyTo(settings);
  }
  for (uint8_t i = 0; i < TURBO_BTN_COUNT; ++i) {
    settings.turbo_rates[i] = (uint8_t)temp_rates[i];
  }
  sanitizePerModeQuickSettings(mode, settings);
}
