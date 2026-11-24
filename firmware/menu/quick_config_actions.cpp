#include "../product_config.h"
#include "../core/directional_hotkeys.h"

#include "quick_config.h"
#include "../core/n64_assignment_runtime.h"
#include "../core/negcon_input_modes.h"
#include "../platform/buzzer.h"
#include "../core/rumble_test_runtime.h"
#include "../core/controller_feedback_config.h"
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
#include "../features/usb2db15/usb2db15_arcade_mapping.h"
#include "../input/usb_host/input_usb/input_usb.h"
#endif
#if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER && defined(ENABLE_INPUT_USB_BT)
#include "../input/usb_bt_composite/Input_UsbBt.h"
#endif

bool QuickConfigMenu::pointerSourceAvailable(uint8_t value) {
  const controller_state_t& frame = selectedControllerFrame();
  const PointerMouseSource source = static_cast<PointerMouseSource>(value);
  return source == POINTER_MOUSE_OFF ||
      pointerMouseStickSourceAvailable(
          source, frame.HAS_ANALOG_STICK_MAIN,
          frame.HAS_ANALOG_STICK_AUX) ||
      (source == POINTER_MOUSE_TOUCHPAD &&
       (strcmp(frame.controller_type_name, "DualShock4") == 0 ||
        strcmp(frame.controller_type_name, "DualSense") == 0 ||
        strcmp(frame.controller_type_name, "OctaProPS4") == 0 ||
        strcmp(frame.controller_type_name, "OctaProPS5") == 0));
}

void QuickConfigMenu::cyclePointerMouseSource(bool forward) {
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  // Mouse output has no Off: the left stick moves it unless Right is chosen.
  // Going back to the left restores the saved choice DInput uses (Off or Left).
  if (temp_output_mode == OUTPUT_ROTARY_MOUSE) {
    const controller_state_t& frame = selectedControllerFrame();
    const PointerMouseSource saved = pointerMouseConfigSource(menu_touchpad_mouse);
    if (temp_pointer_source == POINTER_MOUSE_RIGHT_STICK) {
      temp_pointer_source = saved == POINTER_MOUSE_RIGHT_STICK ? POINTER_MOUSE_OFF : saved;
    } else if (frame.HAS_ANALOG_STICK_MAIN && frame.HAS_ANALOG_STICK_AUX) {
      temp_pointer_source = POINTER_MOUSE_RIGHT_STICK;
    }
    return;
  }
#endif
  for (uint8_t step = 0; step < 4; ++step) {
    temp_pointer_source = forward
        ? static_cast<uint8_t>((temp_pointer_source + 1) % 4)
        : static_cast<uint8_t>((temp_pointer_source + 3) % 4);
    if (pointerSourceAvailable(temp_pointer_source)) return;
  }
  temp_pointer_source = POINTER_MOUSE_OFF;
}

void QuickConfigMenu::setTempRumbleTech(bool enabled) {
  temp_snes_rumbletech = enabled ? 1 : 0;
  if (enabled && temp_rumble == 0) {
    temp_rumble = RUMBLE_MODE_VARIABLE;
  }
}

void QuickConfigMenu::cycleMainMenuItemNext() {
  if (on_bottom_row) {
    bottom_index = (bottom_index + 1) % 3;
    bottom_right = bottom_index == 2;
    return;
  }

  if (visible_count == 0 || selected_index >= visible_count) {
    clampVisibleSelection();
    return;
  }

  QuickConfigItem item = visible_items[selected_index];

  if (needsSubmenu(item)) {
    enterSubmenu(item);
    return;
  }

  if (!isDirectlyEditable(item)) return;

  switch (item) {
    case QCI_PLAYER:
      cycleSelectedControllerPlayer(true);
#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
      loadDirectionalControllerHotkeys();
#endif
      rebuildVisibleAndClampSelection();
      break;

    case QCI_INPUT_MODE:
      do {
        temp_input_mode = (DeviceEnum)(temp_input_mode + 1);
        if (temp_input_mode >= RZORD_LAST) {
          temp_input_mode = (DeviceEnum)1;
        }
      } while (should_hide_input_mode(temp_input_mode));
      loadTempQuickSettingsForMode(temp_input_mode);
      rebuildVisibleAndClampSelection();
      break;

    case QCI_OUTPUT_MODE:
      temp_output_mode = cycle_visible_output_mode(temp_output_mode, true);
      restageRemapsForOutput();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_WIN_OUTPUT:
      temp_win_output = (temp_win_output + 1) % (HOST_OUTPUT_PREFERENCE_MAX + 1);
      break;

    case QCI_PS2_REMOTE_OUTPUT:
      temp_ps2_remote_output = !temp_ps2_remote_output;
      break;

    case QCI_MOUSE_SOURCE:
      cyclePointerMouseSource(true);
      break;

    case QCI_DUALSENSE_PLAYER_LEDS:
    case QCI_DUALSENSE_LIGHTBAR:
      temp_dualsense_led_policy ^= 1u << (item == QCI_DUALSENSE_LIGHTBAR);
      break;

    case QCI_USB2DB15_PRESENTATION:
      temp_usb2db15_presentation = (temp_usb2db15_presentation + 1) % 3;
      break;

    case QCI_DPAD_BUTTONS:
      cycleTempDpadMode(true);
      break;
    case QCI_DPAD2_MODE:
      cycleTempDpad2Mode(true);
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert + 1) % (getStickInvertMax(temp_input_mode) + 1);
      break;

    case QCI_RUMBLETECH:
      toggleTempRumbleTech();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_RUMBLE:
      cycleTempRumbleLevel(true);
      break;

    case QCI_TRIGGERS:
      cycleTempTriggerMode(true);
      break;

    case QCI_SWITCH_RTRIG_STICK:
      temp_triggers = (temp_triggers == TRIGGER_MODE_RSTICK)
                        ? TRIGGER_MODE_ANALOG
                        : TRIGGER_MODE_RSTICK;
      break;

    case QCI_GC_L_SWITCH:
      temp_gamecube_l_switch =
          temp_gamecube_l_switch == GAMECUBE_L_SWITCH_L
            ? GAMECUBE_L_SWITCH_ZL
            : GAMECUBE_L_SWITCH_L;
      break;

#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    case QCI_ROTARY_REVERSE:
      toggleTempRotaryReverse();
      break;

    case QCI_DRIVING_ENDS:
      toggleTempDrivingEnds();
      break;
#endif

#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_I:
      temp_negcon_digital ^= NEGCON_INPUT_I;
      break;

    case QCI_NEGCON_II:
      temp_negcon_digital ^= NEGCON_INPUT_II;
      break;

    case QCI_NEGCON_L:
      toggleTempNegconInput(NEGCON_INPUT_L);
      break;
#endif

    case QCI_WHEEL_SENS:
      temp_wheel_sens = (temp_wheel_sens + 1) % 3;
      break;

    case QCI_JOGCON_WHEEL_AXIS:
      temp_jogcon_wheel_axis = !temp_jogcon_wheel_axis;
      break;

#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    case QCI_MOUSE_PLAYERS:
      temp_mouse.two_players = !temp_mouse.two_players;
      break;
    case QCI_ROTARY_MOUSE_AXIS:
      cycleTempMouseAxis(true);
      break;
#endif

    case QCI_BTN_MAP:
      temp_btn_map = !temp_btn_map;
      break;

    case QCI_START_LEFT: case QCI_START_RIGHT: case QCI_START_UP: case QCI_START_DOWN:
    case QCI_JVS_MENU_HOLD: case QCI_JVS_HOME_HOTKEY: case QCI_JVS_HOLD_START:
      cycleHotkeyItem(item, true);
      break;
    case QCI_N64_Z:
      temp_n64_z = !temp_n64_z;
      break;

    case QCI_N64_CSTICK:
      temp_n64_cstick = (temp_n64_cstick + 1) % 3;
      break;

    case QCI_NSO_SPECIAL:
      cycleTempSwitchDevice(true);
      break;

    case QCI_CLASSIC_DUAL_MERGE:
      temp_classic_dual_merge = !temp_classic_dual_merge;
      break;

    default:
      break;
  }
}

void QuickConfigMenu::cycleMainMenuItemPrev() {
  if (on_bottom_row) {
    bottom_index = (bottom_index == 0) ? 2 : bottom_index - 1;
    bottom_right = bottom_index == 2;
    return;
  }

  if (visible_count == 0 || selected_index >= visible_count) {
    clampVisibleSelection();
    return;
  }

  QuickConfigItem item = visible_items[selected_index];

  if (needsSubmenu(item)) {
    enterSubmenu(item);
    return;
  }

  if (!isDirectlyEditable(item)) return;

  switch (item) {
    case QCI_PLAYER:
      cycleSelectedControllerPlayer(false);
#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
      loadDirectionalControllerHotkeys();
#endif
      rebuildVisibleAndClampSelection();
      break;

    case QCI_INPUT_MODE:
      do {
        if (temp_input_mode <= 1) {
          temp_input_mode = (DeviceEnum)(RZORD_LAST - 1);
        } else {
          temp_input_mode = (DeviceEnum)(temp_input_mode - 1);
        }
      } while (should_hide_input_mode(temp_input_mode));
      loadTempQuickSettingsForMode(temp_input_mode);
      rebuildVisibleAndClampSelection();
      break;

    case QCI_OUTPUT_MODE:
      temp_output_mode = cycle_visible_output_mode(temp_output_mode, false);
      restageRemapsForOutput();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_WIN_OUTPUT:
      temp_win_output = (temp_win_output == 0) ? HOST_OUTPUT_PREFERENCE_MAX : temp_win_output - 1;
      break;

    case QCI_PS2_REMOTE_OUTPUT:
      temp_ps2_remote_output = !temp_ps2_remote_output;
      break;

    case QCI_MOUSE_SOURCE:
      cyclePointerMouseSource(false);
      break;

    case QCI_DUALSENSE_PLAYER_LEDS:
    case QCI_DUALSENSE_LIGHTBAR:
      temp_dualsense_led_policy ^= 1u << (item == QCI_DUALSENSE_LIGHTBAR);
      break;

    case QCI_USB2DB15_PRESENTATION:
      temp_usb2db15_presentation = (temp_usb2db15_presentation + 2) % 3;
      break;

    case QCI_DPAD_BUTTONS:
      cycleTempDpadMode(false);
      break;
    case QCI_DPAD2_MODE:
      cycleTempDpad2Mode(false);
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert == 0)
                            ? getStickInvertMax(temp_input_mode)
                            : temp_stick_invert - 1;
      break;

    case QCI_RUMBLETECH:
      toggleTempRumbleTech();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_RUMBLE:
      cycleTempRumbleLevel(false);
      break;

    case QCI_TRIGGERS:
      cycleTempTriggerMode(false);
      break;

    case QCI_SWITCH_RTRIG_STICK:
      temp_triggers = (temp_triggers == TRIGGER_MODE_RSTICK)
                        ? TRIGGER_MODE_ANALOG
                        : TRIGGER_MODE_RSTICK;
      break;

    case QCI_GC_L_SWITCH:
      temp_gamecube_l_switch =
          temp_gamecube_l_switch == GAMECUBE_L_SWITCH_L
            ? GAMECUBE_L_SWITCH_ZL
            : GAMECUBE_L_SWITCH_L;
      break;

#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    case QCI_ROTARY_REVERSE:
      toggleTempRotaryReverse();
      break;

    case QCI_DRIVING_ENDS:
      toggleTempDrivingEnds();
      break;
#endif

#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_I:
      temp_negcon_digital ^= NEGCON_INPUT_I;
      break;

    case QCI_NEGCON_II:
      temp_negcon_digital ^= NEGCON_INPUT_II;
      break;

    case QCI_NEGCON_L:
      toggleTempNegconInput(NEGCON_INPUT_L);
      break;
#endif

    case QCI_WHEEL_SENS:
      temp_wheel_sens = (temp_wheel_sens == 0) ? 2 : temp_wheel_sens - 1;
      break;

    case QCI_JOGCON_WHEEL_AXIS:
      temp_jogcon_wheel_axis = !temp_jogcon_wheel_axis;
      break;

#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    case QCI_MOUSE_PLAYERS:
      temp_mouse.two_players = !temp_mouse.two_players;
      break;
    case QCI_ROTARY_MOUSE_AXIS:
      cycleTempMouseAxis(false);
      break;
#endif

    case QCI_BTN_MAP:
      temp_btn_map = !temp_btn_map;
      break;

    case QCI_START_LEFT: case QCI_START_RIGHT: case QCI_START_UP: case QCI_START_DOWN:
    case QCI_JVS_MENU_HOLD: case QCI_JVS_HOME_HOTKEY: case QCI_JVS_HOLD_START:
      cycleHotkeyItem(item, false);
      break;
    case QCI_N64_Z:
      temp_n64_z = !temp_n64_z;
      break;

    case QCI_N64_CSTICK:
      temp_n64_cstick = (temp_n64_cstick == 0) ? 2 : temp_n64_cstick - 1;
      break;

    case QCI_NSO_SPECIAL:
      cycleTempSwitchDevice(false);
      break;

    case QCI_CLASSIC_DUAL_MERGE:
      temp_classic_dual_merge = !temp_classic_dual_merge;
      break;

    default:
      break;
  }
}

void QuickConfigMenu::cycleSettingValue() {
  switch (editing_item) {
    case QCI_DEADZONE:
      temp_deadzone = (temp_deadzone + 1) % 7;
      break;

    case QCI_SOCD:
      temp_socd = (socdMode_t)((temp_socd + 1) % SOCD_LAST_ENUM);
      break;

    case QCI_DPAD_BUTTONS:
      cycleTempDpadMode(true);
      break;
    case QCI_DPAD2_MODE:
      cycleTempDpad2Mode(true);
      break;

    case QCI_WIN_OUTPUT:
      temp_win_output = (temp_win_output + 1) % (HOST_OUTPUT_PREFERENCE_MAX + 1);
      break;

    case QCI_PS2_REMOTE_OUTPUT:
      temp_ps2_remote_output = !temp_ps2_remote_output;
      break;

    case QCI_MOUSE_SOURCE:
      cyclePointerMouseSource(true);
      break;

    case QCI_TOUCHPAD_MOUSE:
      temp_touchpad_mouse = temp_touchpad_mouse >= POINTER_MOUSE_SPEED_MAX
          ? 1
          : static_cast<uint8_t>(temp_touchpad_mouse + 1);
      break;

    case QCI_DUALSENSE_PLAYER_LEDS:
    case QCI_DUALSENSE_LIGHTBAR:
      temp_dualsense_led_policy ^=
          1u << (editing_item == QCI_DUALSENSE_LIGHTBAR);
      break;

#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_ACTIVATION:
      temp_usb2db15_activation = (temp_usb2db15_activation + 1) % 6;
      break;
    case QCI_USB2DB15_AUTO_SHOOT:
      temp_usb2db15_auto_shoot = (temp_usb2db15_auto_shoot + 1) % 7;
      break;
#endif
    case QCI_USB2DB15_PRESENTATION:
      temp_usb2db15_presentation = (temp_usb2db15_presentation + 1) % 3;
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert + 1) % (getStickInvertMax(temp_input_mode) + 1);
      break;

    case QCI_RUMBLETECH:
      toggleTempRumbleTech();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_RUMBLE:
      cycleTempRumbleLevel(true);
      break;

    case QCI_TRIGGERS:
      cycleTempTriggerMode(true);
      break;

    case QCI_SWITCH_RTRIG_STICK:
      temp_triggers = (temp_triggers == TRIGGER_MODE_RSTICK)
                        ? TRIGGER_MODE_ANALOG
                        : TRIGGER_MODE_RSTICK;
      break;

#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    case QCI_ROTARY_REVERSE:
      toggleTempRotaryReverse();
      break;

    case QCI_DRIVING_ENDS:
      toggleTempDrivingEnds();
      break;
#endif

#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_I:
      temp_negcon_digital ^= NEGCON_INPUT_I;
      break;

    case QCI_NEGCON_II:
      temp_negcon_digital ^= NEGCON_INPUT_II;
      break;

    case QCI_NEGCON_L:
      toggleTempNegconInput(NEGCON_INPUT_L);
      break;
    case QCI_NEGCON_GAIN:
      temp_negcon_gain = (temp_negcon_gain + 1) % NEGCON_GAIN_COUNT;
      break;
#endif

    case QCI_GC_L_SWITCH:
      temp_gamecube_l_switch =
          temp_gamecube_l_switch == GAMECUBE_L_SWITCH_L
            ? GAMECUBE_L_SWITCH_ZL
            : GAMECUBE_L_SWITCH_L;
      break;

    case QCI_N64_CSTICK:
      temp_n64_cstick = (temp_n64_cstick + 1) % 3;
      break;

    case QCI_N64_RANGE:
      cycleTempClassicAnalogRange(true);
      break;

    case QCI_NSO_SPECIAL:
      cycleTempSwitchDevice(true);
      break;

    case QCI_POWERPAD:
      temp_powerpad = !temp_powerpad;
      break;

    case QCI_CLASSIC_DUAL_MERGE:
      temp_classic_dual_merge = !temp_classic_dual_merge;
      break;

    default:
      break;
  }
}

void QuickConfigMenu::cycleSettingValueBack() {
  switch (editing_item) {
    case QCI_DEADZONE:
      temp_deadzone = (temp_deadzone == 0) ? 6 : temp_deadzone - 1;
      break;

    case QCI_SOCD:
      temp_socd = (temp_socd == 0)
                    ? (socdMode_t)(SOCD_LAST_ENUM - 1)
                    : (socdMode_t)(temp_socd - 1);
      break;

    case QCI_DPAD_BUTTONS:
      cycleTempDpadMode(false);
      break;
    case QCI_DPAD2_MODE:
      cycleTempDpad2Mode(false);
      break;

    case QCI_WIN_OUTPUT:
      temp_win_output = (temp_win_output == 0) ? HOST_OUTPUT_PREFERENCE_MAX : temp_win_output - 1;
      break;

    case QCI_PS2_REMOTE_OUTPUT:
      temp_ps2_remote_output = !temp_ps2_remote_output;
      break;

    case QCI_MOUSE_SOURCE:
      cyclePointerMouseSource(false);
      break;

    case QCI_TOUCHPAD_MOUSE:
      temp_touchpad_mouse = temp_touchpad_mouse <= 1
          ? POINTER_MOUSE_SPEED_MAX
          : static_cast<uint8_t>(temp_touchpad_mouse - 1);
      break;

    case QCI_DUALSENSE_PLAYER_LEDS:
    case QCI_DUALSENSE_LIGHTBAR:
      temp_dualsense_led_policy ^=
          1u << (editing_item == QCI_DUALSENSE_LIGHTBAR);
      break;

#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_ACTIVATION:
      temp_usb2db15_activation = (temp_usb2db15_activation + 5) % 6;
      break;
    case QCI_USB2DB15_AUTO_SHOOT:
      temp_usb2db15_auto_shoot = (temp_usb2db15_auto_shoot + 6) % 7;
      break;
#endif
    case QCI_USB2DB15_PRESENTATION:
      temp_usb2db15_presentation = (temp_usb2db15_presentation + 2) % 3;
      break;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert == 0)
                            ? getStickInvertMax(temp_input_mode)
                            : temp_stick_invert - 1;
      break;

    case QCI_RUMBLETECH:
      toggleTempRumbleTech();
      rebuildVisibleAndClampSelection();
      break;

    case QCI_RUMBLE:
      cycleTempRumbleLevel(false);
      break;

    case QCI_TRIGGERS:
      cycleTempTriggerMode(false);
      break;

    case QCI_SWITCH_RTRIG_STICK:
      temp_triggers = (temp_triggers == TRIGGER_MODE_RSTICK)
                        ? TRIGGER_MODE_ANALOG
                        : TRIGGER_MODE_RSTICK;
      break;

#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    case QCI_ROTARY_REVERSE:
      toggleTempRotaryReverse();
      break;

    case QCI_DRIVING_ENDS:
      toggleTempDrivingEnds();
      break;
#endif

#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_I:
      temp_negcon_digital ^= NEGCON_INPUT_I;
      break;

    case QCI_NEGCON_II:
      temp_negcon_digital ^= NEGCON_INPUT_II;
      break;

    case QCI_NEGCON_L:
      toggleTempNegconInput(NEGCON_INPUT_L);
      break;
    case QCI_NEGCON_GAIN:
      temp_negcon_gain =
        (temp_negcon_gain + NEGCON_GAIN_COUNT - 1) % NEGCON_GAIN_COUNT;
      break;
#endif

    case QCI_GC_L_SWITCH:
      temp_gamecube_l_switch =
          temp_gamecube_l_switch == GAMECUBE_L_SWITCH_L
            ? GAMECUBE_L_SWITCH_ZL
            : GAMECUBE_L_SWITCH_L;
      break;

    case QCI_N64_CSTICK:
      temp_n64_cstick = (temp_n64_cstick == 0) ? 2 : temp_n64_cstick - 1;
      break;

    case QCI_N64_RANGE:
      cycleTempClassicAnalogRange(false);
      break;

    case QCI_NSO_SPECIAL:
      cycleTempSwitchDevice(false);
      break;

    case QCI_POWERPAD:
      temp_powerpad = !temp_powerpad;
      break;

    case QCI_CLASSIC_DUAL_MERGE:
      temp_classic_dual_merge = !temp_classic_dual_merge;
      break;

    default:
      break;
  }
}

void QuickConfigMenu::select() {
  needs_redraw = true;
  const uint8_t turboCount = getVisibleTurboCount();

  switch (state) {
    case QC_MODE_PICKER:
      confirmModePicker();
      break;
    case QC_CHOICE_PICKER:
      confirmChoicePicker();
      break;
    case QC_MAIN_MENU:
      if (on_bottom_row) {
        if (bottom_index == 2) {
          saveOrConfirmNsoSinglePlayer();
        } else if (bottom_index == 1) {
          state = QC_CONFIRM_DEFAULT;
          default_confirm_index = 0;
          buzzer.playMenuNav();
        } else {
          discard();
        }
        break;
      }

      if (visible_count == 0 || selected_index >= visible_count) {
        clampVisibleSelection();
        break;
      }

      enterSubmenu(visible_items[selected_index]);
      break;

    case QC_CONFIRM_DEFAULT:
      if (default_confirm_index == 1) {
        resetTempQuickSettingsToDefaults();
        defaults_restored = true;
        applyAndClose();
      } else {
        state = QC_MAIN_MENU;
        buzzer.playCancel();
      }
      break;

    case QC_CONFIRM_DINPUT_RESET:
      if (dinput_reset_confirm_index == 0) {
        state = QC_MAIN_MENU;
        break;
      }
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION) || (ADAPT_POLICY_QUICK_SELECTED_CONTROLLER && defined(ENABLE_INPUT_USB_BT))
      {
        usb_hid_mapping_target_t target{};
        target.device_index = reset_dinput_device;
        target.connection_order = dinput_reset_connection_order;
        target.key = dinput_reset_key;
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
        const bool prepared = inputusb_generic_prepare_remap(&target);
#else
        const bool prepared = usbBtPrepareCapturedDinputRemap(&target);
#endif
        if (prepared) {
          reset_dinput_completed = true;
          should_save = false;
          close();
        } else {
          reset_dinput_device = 0xFF;
          dinput_reset_connection_order = 0;
          dinput_reset_key = {};
          action_error = true;
          state = QC_MAIN_MENU;
          rebuildVisibleAndClampSelection();
        }
      }
#else
      state = QC_MAIN_MENU;
#endif
      break;

    case QC_CONFIRM_NSO_1P:
      if (nso_1p_confirm_index == 1) {
        applyAndClose();
      } else {
        state = QC_MAIN_MENU;
      }
      break;

    case QC_TURBO_LIST:
      if (turbo_index >= turboCount) {
        state = QC_MAIN_MENU;
      } else {
        openChoicePicker(QCI_TURBO);
      }
      break;

    case QC_REMAP_LIST: {
      uint8_t btn_count = getRemapButtonCount(temp_input_mode);
      if (remap_index == btn_count + 1) {
        // Default: every remap back to its default, staying on this screen.
        resetTempRemapsToDefaults();
        buzzer.playMenuNav();
      } else if (remap_index >= btn_count) {
        state = QC_MAIN_MENU;
      } else {
        openChoicePicker(QCI_REMAP);
      }
      break;
    }

    case QC_ANALOG_SUBMENU:
      handleAnalogSelect();
      break;
    case QC_HOTKEYS_SUBMENU:
      handleHotkeySelect(true);
      break;

    case QC_DUAL_MERGE_MAP:
      handleDualMergeSelect();
      break;

    case QC_RUMBLE_SUBMENU:
      handleRumbleSelect(true);
      break;

#ifdef ENABLE_INPUT_PSX
    case QC_GUNCON_ADJUST:
      handleGunconSelect();
      break;
#endif

#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QC_USB2DB15_TOURNAMENT:
      state = QC_MAIN_MENU;
      break;
#endif
    case QC_RANGE_TEST:
      state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      break;

    case QC_STICK_CAL:
      if (cal_selection == 0) {
        state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      } else if (cal_selection == 1 && cal_has_data) {
        AnalogCalibrationData& calibration = transaction.calibration();
        calibration.lx.raw_min = cal_lx_min;
        calibration.lx.raw_max = cal_lx_max;
        calibration.ly.raw_min = cal_ly_min;
        calibration.ly.raw_max = cal_ly_max;
        calibration.rx.raw_min = cal_rx_min;
        calibration.rx.raw_max = cal_rx_max;
        calibration.ry.raw_min = cal_ry_min;
        calibration.ry.raw_max = cal_ry_max;
        calibration.enabled = true;
        if (classicModeHasRangeSetting(temp_input_mode)) {
          if (temp_input_mode == RZORD_N64) {
            temp_n64_range = CLASSIC_ANALOG_RANGE_CALIBRATED;
          } else {
            temp_wii_range = CLASSIC_ANALOG_RANGE_CALIBRATED;
          }
        }
        state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      } else if (cal_selection == 2) {
        transaction.clearCalibration();
        if (classicModeHasRangeSetting(temp_input_mode)) {
          if (temp_input_mode == RZORD_N64 && temp_n64_range == CLASSIC_ANALOG_RANGE_CALIBRATED) {
            temp_n64_range = CLASSIC_ANALOG_RANGE_DEFAULT;
          } else if (temp_input_mode != RZORD_N64 && temp_wii_range == CLASSIC_ANALOG_RANGE_CALIBRATED) {
            temp_wii_range = CLASSIC_ANALOG_RANGE_DEFAULT;
          }
        }
        state = return_to_analog_submenu ? QC_ANALOG_SUBMENU : QC_MAIN_MENU;
      }
      break;

    default:
      break;
  }
}

bool QuickConfigMenu::saveOrConfirmNsoSinglePlayer() {
  const outputMode_t selectedOutput =
    (temp_output_mode == OUTPUT_AUTO) ? get_effective_output_mode() : temp_output_mode;
  const bool enablingNativeNso =
    output_mode_supports_switch_device_selection(selectedOutput) &&
    input_mode_supports_nso_special(temp_input_mode) &&
    switch_device_selection_is_nso_for_input_mode(temp_input_mode, temp_nso_special) &&
    temp_nso_special != menu_nso_special;
  if (enablingNativeNso) {
    state = QC_CONFIRM_NSO_1P;
    nso_1p_confirm_index = 0;
    buzzer.playMenuNav();
    return false;
  }
  applyAndClose();
  return true;
}

void QuickConfigMenu::openDefaultConfirmation() {
  state = QC_CONFIRM_DEFAULT;
  default_confirm_index = 0;
  needs_redraw = true;
}

bool QuickConfigMenu::finishEditorForHold(bool save) {
  if (!save) return true;  // The outer discard rolls back the whole transaction.

  switch (state) {
    case QC_CONFIRM_DEFAULT:
    case QC_CONFIRM_DINPUT_RESET:
    case QC_CONFIRM_NSO_1P:
      return false;  // A hold is not an explicit confirmation.
    case QC_MODE_PICKER:
      confirmModePicker();
      break;
    case QC_CHOICE_PICKER:
      confirmChoicePicker();
      break;
    case QC_TURBO_RATE:
      if (turbo_rate_selection < TURBO_RATE_LAST) {
        const uint8_t button = getTurboButtonIndexForVisibleIndex(turbo_index);
        if (button != 0xFF) temp_rates[button] = getTurboRateForMenuIndex(turbo_rate_selection);
      }
      break;
    default:
      break;
  }
  return true;
}

bool QuickConfigMenu::saveShortcut() {
  needs_redraw = true;

  switch (state) {
    case QC_MAIN_MENU:
      saveOrConfirmNsoSinglePlayer();
      return true;

    case QC_STICK_CAL:
      if (!cal_has_data) {
        return false;
      }
      cal_selection = 1;
      select();
      return true;

    default:
      return false;
  }
}

void QuickConfigMenu::cycleHotkeyItem(QuickConfigItem item, bool forward) {
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
  if (item == QCI_JVS_MENU_HOLD) {
    temp_jvs_menu_hold = forward ? (temp_jvs_menu_hold < 2 ? 3 : temp_jvs_menu_hold == 6 ? 2 : temp_jvs_menu_hold + 1)
                                 : (temp_jvs_menu_hold <= 2 ? 6 : temp_jvs_menu_hold - 1);
    return;
  }
  if (item == QCI_JVS_HOME_HOTKEY) {
    temp_jvs_home_hotkey = !temp_jvs_home_hotkey;
    return;
  }
  if (item == QCI_JVS_HOLD_START) {
    temp_jvs_hold_start = !temp_jvs_hold_start;
    return;
  }
#endif
#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
  if (item == QCI_START_DOWN) {
    temp_direction_hotkey_home = forward
        ? (uint8_t)((temp_direction_hotkey_home + 1) % 3)
        : (temp_direction_hotkey_home == 0 ? 2 : (uint8_t)(temp_direction_hotkey_home - 1));
  } else if (item >= QCI_START_LEFT && item <= QCI_START_UP) {
    cycleDirectionalControllerHotkey(temp_direction_hotkeys,
        item - QCI_START_LEFT, selectedControllerPlayer(),
        temp_output_mode == OUTPUT_AUTO ? get_effective_output_mode() : temp_output_mode,
        forward);
  }
#elif defined(ADAPT_CAP_DIRECTIONAL_HOTKEYS)
  if (item >= QCI_START_LEFT && item <= QCI_START_UP) {
    cycleDirectionalHotkey(temp_direction_hotkeys, item - QCI_START_LEFT, temp_input_mode,
        isLiveInputSelection() ? selectedControllerFrame().controller_type_name : "",
        temp_output_mode == OUTPUT_AUTO ? get_effective_output_mode() : temp_output_mode, forward);
  }
#else
  (void)item;
  (void)forward;
#endif
}

void QuickConfigMenu::handleHotkeySelect(bool forward) {
  buildHotkeyItems();
  if (hotkey_index >= hotkey_count) {
    state = QC_MAIN_MENU;
  } else if (needsSubmenu(hotkey_items[hotkey_index])) {
    openChoicePicker(hotkey_items[hotkey_index]);
  } else {
    cycleHotkeyItem(hotkey_items[hotkey_index], forward);
  }
  needs_redraw = true;
}

void QuickConfigMenu::handleAnalogSelect() {
  buildAnalogItems();
  if (analog_index >= analog_count) {
    state = QC_MAIN_MENU;
    needs_redraw = true;
    return;
  }

  QuickConfigItem item = analog_items[analog_index];
  // Pointer Speed, the wheel and the buttons are lists (four or more values).
  if (needsSubmenu(item) && item != QCI_RANGE_TEST && item != QCI_STICK_CAL) {
    openChoicePicker(item);
    return;
  }
  switch (item) {
    case QCI_MOUSE_SOURCE:
      cyclePointerMouseSource(true);
      break;

    case QCI_RANGE_TEST:
    case QCI_STICK_CAL:
      enterSubmenu(item);
      return_to_analog_submenu = true;
      return;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert + 1) % (getStickInvertMax(temp_input_mode) + 1);
      break;

    default:
      break;
  }

  needs_redraw = true;
}

void QuickConfigMenu::handleAnalogSelectBack() {
  buildAnalogItems();
  if (analog_index >= analog_count) {
    state = QC_MAIN_MENU;
    needs_redraw = true;
    return;
  }

  QuickConfigItem item = analog_items[analog_index];
  // Pointer Speed, the wheel and the buttons are lists (four or more values).
  if (needsSubmenu(item) && item != QCI_RANGE_TEST && item != QCI_STICK_CAL) {
    openChoicePicker(item);
    return;
  }
  switch (item) {
    case QCI_MOUSE_SOURCE:
      cyclePointerMouseSource(false);
      break;

    case QCI_RANGE_TEST:
    case QCI_STICK_CAL:
      enterSubmenu(item);
      return_to_analog_submenu = true;
      return;

    case QCI_STICK_INVERT:
      temp_stick_invert = (temp_stick_invert == 0)
                            ? getStickInvertMax(temp_input_mode)
                            : temp_stick_invert - 1;
      break;

    default:
      break;
  }

  needs_redraw = true;
}

uint8_t QuickConfigMenu::getDualMergeMenuCount() {
  return (uint8_t)(getClassicDualMergeButtonCount(temp_input_mode) + 1u);
}

void QuickConfigMenu::toggleDualMergeCurrentItem() {
  const uint8_t buttonCount = getClassicDualMergeButtonCount(temp_input_mode);
  if (dual_merge_index == 0) {
    temp_classic_dual_merge = !temp_classic_dual_merge;
    return;
  }
  if (dual_merge_index > buttonCount) {
    state = QC_MAIN_MENU;
    return;
  }

  const uint32_t mask = getClassicDualMergeButtonMask(temp_input_mode, dual_merge_index - 1);
  if (mask != 0) {
    temp_classic_dual_merge_p2_mask ^= mask;
    temp_classic_dual_merge_p2_mask =
      sanitizeClassicDualMergeP2Mask(temp_input_mode, temp_classic_dual_merge_p2_mask);
  }
}

void QuickConfigMenu::handleDualMergeSelect() {
  toggleDualMergeCurrentItem();
  needs_redraw = true;
}

void QuickConfigMenu::handleDualMergeSelectBack() {
  toggleDualMergeCurrentItem();
  needs_redraw = true;
}

void QuickConfigMenu::startTempRumbleTest(bool heavy, bool light) {
  // A diagnostic test must remain usable while gameplay rumble is disabled.
  // It does not alter the saved level and the runtime still clears the motors
  // after the bounded test interval.
  const uint8_t left = heavy ? UINT8_MAX : 0;
  const uint8_t right = light ? UINT8_MAX : 0;
  rumbleRuntimeStartTest(1UL << selectedControllerPlayer(), left, right, 3000);
}

void QuickConfigMenu::startHeavyRumbleRamp() {
  rumbleRuntimeStartHeavyRamp(1UL << selectedControllerPlayer());
}

void QuickConfigMenu::handleRumbleSelect(bool forward) {
  switch ((RumbleMenuItem)rumble_index) {
    case QCI_RUMBLE_LEVEL:
      cycleTempRumbleLevel(forward);
      break;

    case QCI_RUMBLE_TEST_HEAVY:
      startTempRumbleTest(true, false);
      break;

    case QCI_RUMBLE_TEST_LIGHT:
      startTempRumbleTest(false, true);
      break;

    case QCI_RUMBLE_TEST_BOTH:
      startTempRumbleTest(true, true);
      break;

    case QCI_RUMBLE_RAMP_HEAVY:
      startHeavyRumbleRamp();
      break;

    case QCI_RUMBLE_BACK:
      state = QC_MAIN_MENU;
      break;

    default:
      break;
  }

  needs_redraw = true;
}

void QuickConfigMenu::handleGunconSelect() {
  guncon_edit_axis++;
  if (guncon_edit_axis > 2) {
    guncon_edit_axis = 0;
    state = QC_MAIN_MENU;
  }
  needs_redraw = true;
}

void QuickConfigMenu::handleGunconNavigate() {
  if (guncon_edit_axis == 0) {
    temp_guncon_x = (temp_guncon_x + 1);
    if (temp_guncon_x > GUNCON_ALIGNMENT_MAX) temp_guncon_x = GUNCON_ALIGNMENT_MIN;
  } else if (guncon_edit_axis == 1) {
    temp_guncon_y = (temp_guncon_y + 1);
    if (temp_guncon_y > GUNCON_ALIGNMENT_MAX) temp_guncon_y = GUNCON_ALIGNMENT_MIN;
  }

  needs_redraw = true;
}

void QuickConfigMenu::handleGunconNavigateBack() {
  if (guncon_edit_axis == 0) {
    temp_guncon_x = (temp_guncon_x - 1);
    if (temp_guncon_x < GUNCON_ALIGNMENT_MIN) temp_guncon_x = GUNCON_ALIGNMENT_MAX;
  } else if (guncon_edit_axis == 1) {
    temp_guncon_y = (temp_guncon_y - 1);
    if (temp_guncon_y < GUNCON_ALIGNMENT_MIN) temp_guncon_y = GUNCON_ALIGNMENT_MAX;
  }

  needs_redraw = true;
}
