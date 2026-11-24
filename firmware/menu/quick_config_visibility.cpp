#include "../product_config.h"
#include "../core/directional_hotkeys.h"

#include "quick_config.h"
#include "menu_choice_list.h"
#include "menu_capabilities.h"
#include "menu_descriptors.h"
#include "../core/settings_registry.h"
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
#include "menu_jvs_analog.h"
#include "../input/jvs/jvs_analog_layout_runtime.h"
#endif

#ifdef ENABLE_INPUT_JAGUAR
#include "../input/jaguar/input_jaguar_runtime_state.h"
#endif

#include <string.h>

#include "../core/button_map_mode.h"
#include "../core/controller_feedback_config.h"
#include "../core/negcon_input_modes.h"
#include "../input/runtime/input_frame_runtime.h"
#include "../output/output_capabilities.h"
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
#include "../features/usb2db15/usb2db15_arcade_mapping.h"
#include "../features/usb2db15/usb2db15_arcade_session.h"
#include "../features/usb2db15/usb2db15_player_tuning.h"
#include "../input/usb_host/input_usb/input_usb.h"
#endif
#if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER && defined(ENABLE_INPUT_USB_BT)
#include "../input/usb_bt_composite/Input_UsbBt.h"
#endif

static outputMode_t quickConfigResolvedOutputMode(outputMode_t output) {
  return output == OUTPUT_AUTO ? get_effective_output_mode() : output;
}

static bool quickConfigPs2RemoteSelectorVisible(outputMode_t requestedOutput,
                                                bool liveSelection) {
  if (!settingSupportedOnProduct(SettingId::Ps2RemoteOutput)) return false;
  if (!liveSelection) return false;
  const controller_state_t& frame = selectedControllerFrame();
  if (!frame.connected || frame.extended_button_count != 64) return false;
  requestedOutput = canonicalizeOutputMode(requestedOutput);
  return requestedOutput == OUTPUT_AUTO || requestedOutput == OUTPUT_HID ||
         requestedOutput == OUTPUT_MISTER;
}

static bool quickConfigIsLiveNeGcon(DeviceEnum mode, bool liveSelection) {
#ifdef ENABLE_INPUT_PSX
  const controller_state_t& frame = selectedControllerFrame();
  return mode == RZORD_PSX && liveSelection && frame.connected &&
         strcmp(frame.controller_type_name, "neGcon") == 0;
#else
  (void)mode;
  (void)liveSelection;
  return false;
#endif
}

// On Mouse output the Analog page is the Buttons page.
bool QuickConfigMenu::mousePageActive() {
  return quickConfigResolvedOutputMode(temp_output_mode) == OUTPUT_ROTARY_MOUSE;
}

bool QuickConfigMenu::hasExplicitRotaryOutput() {
  const outputMode_t output = quickConfigResolvedOutputMode(temp_output_mode);
  return output == OUTPUT_MISTER_PADDLE || output == OUTPUT_MISTER_SPINNER ||
         output == OUTPUT_ROTARY_MOUSE;
}

uint8_t QuickConfigMenu::effectiveTempJogconMode() {
  const controller_state_t& frame = selectedControllerFrame();
  // Pointer/mouse operation forces the producer into Spinner mode.  Its
  // stored Paddle/Wheel/Fake preference remains intact for normal operation.
  if (strcmp(frame.controller_type_name, "JogCon-S") == 0 &&
      !frame.HAS_ANALOG_STICK_MAIN) {
    return 0;
  }
  return temp_jogcon_mode <= 3 ? temp_jogcon_mode : 0;
}

RotaryInputKind QuickConfigMenu::detectedRotaryInputKind(DeviceEnum mode) {
  if (!isLiveInputSelection()) return RotaryInputKind::None;
  const controller_state_t& frame = selectedControllerFrame();
  if (!frame.connected) return RotaryInputKind::None;

  RotaryInputKind kind = RotaryInputKind::None;
#ifdef ENABLE_INPUT_PSX
  if (mode == RZORD_PSX) {
    if (strcmp(frame.controller_type_name, "neGcon") == 0) {
      kind = RotaryInputKind::NeGcon;
    } else if (strncmp(frame.controller_type_name, "JogCon", 6) == 0) {
      kind = RotaryInputKind::JogCon;
    }
  }
#endif
#ifdef ENABLE_INPUT_NES
  if (mode == RZORD_NES && temp_powerpad >= NES_DEVICE_VAUS_NES &&
      strncmp(frame.controller_type_name, "Vaus", 4) == 0) {
    kind = RotaryInputKind::Vaus;
  }
#endif
#ifdef ENABLE_INPUT_DRIVING
  if ((mode == RZORD_DRIVING || isDrivingFallbackActive(mode)) &&
      strncmp(frame.controller_type_name, "Driving", 7) == 0) {
    kind = RotaryInputKind::Driving;
  }
#endif
#ifdef ENABLE_INPUT_JAGUAR
  if (mode == RZORD_JAGUAR &&
      jaguarRotaryActivePorts[selectedControllerPlayer()]) {
    kind = RotaryInputKind::Jaguar;
  }
#endif
  return rotaryOutputSupportsInput(kind,
      quickConfigResolvedOutputMode(temp_output_mode), effectiveTempJogconMode()) ? kind : RotaryInputKind::None;
}

RotaryTuning QuickConfigMenu::getTempRotaryTuning(DeviceEnum mode) {
  const RotaryInputKind kind = detectedRotaryInputKind(mode);
  const uint8_t profile = kind == RotaryInputKind::JogCon
      ? temp_jogcon_rotary_profile : temp_rotary_profile;
  return resolveRotaryTuning(kind, quickConfigResolvedOutputMode(temp_output_mode),
      profile, temp_paddle_divisor, temp_rotary_reverse, temp_spinner,
      temp_wheel_sens, effectiveTempJogconMode());
}

void QuickConfigMenu::setTempRotaryGain(uint8_t gain) {
  const RotaryInputKind kind = detectedRotaryInputKind(temp_input_mode);
  const RotaryTuning tuning = getTempRotaryTuning(temp_input_mode);
  uint8_t& profile = kind == RotaryInputKind::JogCon
      ? temp_jogcon_rotary_profile : temp_rotary_profile;
  profile = encodeRotaryProfile(gain, tuning.reverse);
}

void QuickConfigMenu::toggleTempRotaryReverse() {
  const RotaryInputKind kind = detectedRotaryInputKind(temp_input_mode);
  const RotaryTuning tuning = getTempRotaryTuning(temp_input_mode);
  uint8_t& profile = kind == RotaryInputKind::JogCon
      ? temp_jogcon_rotary_profile : temp_rotary_profile;
  profile = encodeRotaryProfile(tuning.gain, !tuning.reverse);
}

bool QuickConfigMenu::hasContextualRotaryInput(DeviceEnum mode) {
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  return detectedRotaryInputKind(mode) != RotaryInputKind::None;
#else
  // Rotary sources are classic inputs and their tuning applies to USB output.
  (void)mode;
  return false;
#endif
}

bool QuickConfigMenu::hasDrivingEndsControl(DeviceEnum mode) {
#ifdef ENABLE_INPUT_DRIVING
  // Ends shapes an absolute position; relative motion has no ends.
  return detectedRotaryInputKind(mode) == RotaryInputKind::Driving &&
         !rotaryOutputQueuesMotion(RotaryInputKind::Driving,
             quickConfigResolvedOutputMode(temp_output_mode));
#else
  (void)mode;
  return false;
#endif
}

bool QuickConfigMenu::tempDrivingEndsWrap(DeviceEnum mode) {
  if (rotaryProfileConfigured(temp_rotary_profile) ||
      quickConfigResolvedOutputMode(temp_output_mode) == OUTPUT_MISTER_PADDLE) {
    return temp_driving_wrap;
  }
#ifdef ENABLE_INPUT_DRIVING
  // Before a profile edit, retain the historic output behavior: Atari Driving
  // wraps while the SMS fallback clamps. Editing Ends records an explicit
  // profile, after which this setting applies consistently to every output.
  return mode == RZORD_DRIVING;
#else
  (void)mode;
  return false;
#endif
}

void QuickConfigMenu::toggleTempDrivingEnds() {
  const RotaryTuning tuning = getTempRotaryTuning(temp_input_mode);
  const bool wrap = tempDrivingEndsWrap(temp_input_mode);
  temp_driving_wrap = !wrap;
  temp_rotary_profile = encodeRotaryProfile(tuning.gain, tuning.reverse);
}

uint8_t QuickConfigMenu::negconInputForItem(QuickConfigItem item) {
  switch (item) {
    case QCI_NEGCON_I: return NEGCON_INPUT_I;
    case QCI_NEGCON_II: return NEGCON_INPUT_II;
    case QCI_NEGCON_L: return NEGCON_INPUT_L;
    default: return 0;
  }
}

// The layout the staged Output and D-pad rows would give a neGcon.
static NegconOutputLayout quickConfigNegconLayout(outputMode_t tempOutput, uint8_t tempDpadMode) {
  const outputMode_t output = canonicalizeOutputMode(quickConfigResolvedOutputMode(tempOutput));
  return negconOutputLayout(output, output_supports_analog_triggers(output),
                            tempDpadMode != DPAD_MODE_RIGHT_STICK);
}

bool QuickConfigMenu::tempNegconInputHasPressure(uint8_t input) {
  return negconInputHasPressure(quickConfigNegconLayout(temp_output_mode, temp_dpad_mode), input);
}

bool QuickConfigMenu::tempNegconInputDigital(uint8_t input) {
  return negconInputDigital(temp_negcon_digital, input,
                            quickConfigNegconLayout(temp_output_mode, temp_dpad_mode));
}

void QuickConfigMenu::toggleTempNegconInput(uint8_t input) {
  temp_negcon_digital = toggleNegconInput(
    temp_negcon_digital, input, quickConfigNegconLayout(temp_output_mode, temp_dpad_mode));
}

static bool quickConfigSelectionHasTouchCursor() {
  const controller_state_t& frame = selectedControllerFrame();
  if (!frame.connected) {
    return false;
  }
  const char* type = frame.controller_type_name;
  return strcmp(type, "DualShock4") == 0 ||
         strcmp(type, "DualSense") == 0 ||
         strcmp(type, "OctaProPS4") == 0 ||
         strcmp(type, "OctaProPS5") == 0;
}

static bool quickConfigSelectionHasPointerSource() {
  const controller_state_t& frame = selectedControllerFrame();
  return frame.connected &&
         (frame.HAS_ANALOG_STICK_MAIN || frame.HAS_ANALOG_STICK_AUX ||
          quickConfigSelectionHasTouchCursor());
}

#if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER && defined(ENABLE_INPUT_USB_BT)
static bool quickConfigSelectionCanMapDinput() {
  return usbBtPlayerCanMapDinput(selectedControllerPlayer());
}
#endif

static bool quickConfigOutputSupportsPointerMouse(
    outputMode_t mode, uint8_t winOutput) {
  mode = canonicalizeOutputMode(mode);
  return mode == OUTPUT_HID || mode == OUTPUT_MISTER ||
         (mode == OUTPUT_AUTO && winOutput == 0);
}

void QuickConfigMenu::buildAnalogItems() {
  analog_count = 0;
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
  return;  // Analog Setup owns axis test, inversion and calibration.
#endif
  if (menuModeUsesBasicKeypadQuickConfig(temp_input_mode)) {
    return;
  }
#if defined(ADAPT_PRIMARY_EGRESS_DB15)
  // DB15 has no analog output. Stick selection belongs in the top-level Move
  // row; calibration, deadzone, inversion, and pointer settings do not apply.
  return;
#endif
  if (hasExplicitRotaryOutput()) {
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    // On Mouse output this page is the Buttons page: the controller button
    // for each mouse button. The other Mouse rows are on the main page.
    if (mousePageActive() && canEditModeSpecificSettings()) {
      if (getRemapButtonCount(temp_input_mode) > 0) {
        addAnalogItem(QCI_MOUSE_LEFT);
        addAnalogItem(QCI_MOUSE_MIDDLE);
        addAnalogItem(QCI_MOUSE_RIGHT);
        addAnalogItem(QCI_MOUSE_BUTTON4);
        addAnalogItem(QCI_MOUSE_BUTTON5);
      }
    }
#endif
    return;
  }
  DeviceEnum mode = temp_input_mode;
  const bool contextualRotary = hasContextualRotaryInput(mode);
  const bool liveSelection = isLiveInputSelection();
  const uint8_t frameCount =
    max_devices < MAX_USB_OUT ? max_devices : MAX_USB_OUT;
  AnalogDiagnosticTarget diagnostic = analogDiagnosticNoTarget();
  if (liveSelection) {
#if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER
    diagnostic = analogDiagnosticTargetForPort(
      deviceMode, controller_frames, frameCount,
      selectedControllerPlayer());
#else
    diagnostic = analogDiagnosticDefaultTarget(
      deviceMode, controller_frames, frameCount);
#endif
  }

#if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER || defined(ADAPT_INPUT_RETRO)
  if (!contextualRotary && liveSelection && quickConfigSelectionHasPointerSource() &&
      quickConfigOutputSupportsPointerMouse(temp_output_mode,
                                            temp_win_output)) {
    addAnalogItem(QCI_MOUSE_SOURCE);
    addAnalogItem(QCI_TOUCHPAD_MOUSE);
  }
#endif

  if (analogDiagnosticTargetIsValid(diagnostic)) {
    addAnalogItem(QCI_RANGE_TEST);
#if ADAPT_POLICY_QUICK_STICK_CALIBRATION
    if (diagnostic.kind == AnalogDiagnosticKind::Stick &&
        diagnostic.port == 0 && !isN64(mode)) {
      addAnalogItem(QCI_STICK_CAL);
    }
#endif
  }

  if (hasClassicAnalogRange(mode) && hasAnalogSticks(mode)) {
    addAnalogItem(QCI_N64_RANGE);
  }

  if (!contextualRotary && hasAnalogSticks(mode)) {
    #if ADAPT_POLICY_QUICK_STICK_CALIBRATION
    addAnalogItem(QCI_DEADZONE);
    #endif
    addAnalogItem(QCI_STICK_INVERT);
  }

}

void QuickConfigMenu::buildHotkeyItems() {
  hotkey_count = 0;
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
  hotkey_items[hotkey_count++] = QCI_START_LEFT;
  hotkey_items[hotkey_count++] = QCI_START_RIGHT;
  hotkey_items[hotkey_count++] = QCI_START_UP;
  hotkey_items[hotkey_count++] = QCI_JVS_MENU_HOLD;
  hotkey_items[hotkey_count++] = QCI_JVS_HOME_HOTKEY;
  hotkey_items[hotkey_count++] = QCI_JVS_HOLD_START;
  return;
#endif
  if (menuModeUsesBasicKeypadQuickConfig(temp_input_mode)) {
    return;
  }
#if ADAPT_POLICY_FIXED_HOTKEY_ASSIGNMENTS
  return;
#endif
  if (hasExplicitRotaryOutput() || !canEditModeSpecificSettings()) return;
#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
  if (isLiveInputSelection() && selectedControllerFrame().connected) {
    hotkey_items[hotkey_count++] = QCI_START_LEFT;
    hotkey_items[hotkey_count++] = QCI_START_RIGHT;
    hotkey_items[hotkey_count++] = QCI_START_UP;
    hotkey_items[hotkey_count++] = QCI_START_DOWN;
  }
#elif defined(ADAPT_CAP_DIRECTIONAL_HOTKEYS)
  if (directionalHotkeyKind(temp_input_mode,
        isLiveInputSelection() ? selectedControllerFrame().controller_type_name : "") != direction_hotkey::None) {
    hotkey_items[hotkey_count++] = QCI_START_LEFT;
    hotkey_items[hotkey_count++] = QCI_START_RIGHT;
    hotkey_items[hotkey_count++] = QCI_START_UP;
  }
#endif
}

void QuickConfigMenu::buildVisibleItems() {
  visible_count = 0;
  DeviceEnum mode = temp_input_mode;
  const bool remoteOutput =
    quickConfigResolvedOutputMode(temp_output_mode) == OUTPUT_REMOTE;
  if (remoteOutput) {
    analog_count = 0;
    hotkey_count = 0;
  } else {
    buildAnalogItems();
    buildHotkeyItems();
  }
  const bool contextualRotary = hasContextualRotaryInput(mode);
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  // Mouse output hides what a mouse cannot use (owner, 2026-10-25): Rumble,
  // Triggers, D-Pad, Button Remap (the Buttons page chooses the clicks),
  // Button Map and 2P Merge.
  const bool mouseOutput = mousePageActive();
#else
  const bool mouseOutput = false;
#endif

  #if ADAPT_POLICY_QUICK_SHOW_INPUT_MODE
    addVisibleItem(QCI_INPUT_MODE);
  #endif
  #if ADAPT_POLICY_QUICK_SHOW_OUTPUT_MODE
    addVisibleItem(QCI_OUTPUT_MODE);
  #endif
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  if (mouseOutput && canEditModeSpecificSettings()) addVisibleItem(QCI_MOUSE_PLAYERS);
#endif
  #if defined(ADAPT_CAP_JVS_INPUT_STATUS)
    addVisibleItem(QCI_ANALOG_SETUP);
    addVisibleItem(QCI_TURBO);
    addVisibleItem(QCI_REMAP);
    addVisibleItem(QCI_HOTKEYS_MENU);
    if (jvsCurrentAnalogHasCenteredAxes()) {
      addVisibleItem(QCI_DEADZONE);
    }
    const auto output = canonicalizeOutputMode(quickConfigResolvedOutputMode(temp_output_mode));
    if (jvsCurrentAnalogHasPedals()) {
      addVisibleItem(output == OUTPUT_SWITCHPRO || output == OUTPUT_SWITCH2PRO
          ? QCI_SWITCH_RTRIG_STICK : QCI_TRIGGERS);
    }
    addVisibleItem(QCI_DPAD_BUTTONS);
#if ADAPT_POLICY_QUICK_SHOW_SOCD
    addVisibleItem(QCI_SOCD);
#endif
    return;
  #endif
  #if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER

    if (strcmp(selectedControllerFrame().controller_type_name,
               "DualSense") == 0) {
      addVisibleItem(QCI_DUALSENSE_PLAYER_LEDS);
      addVisibleItem(QCI_DUALSENSE_LIGHTBAR);
    }
  #endif
#if defined(ADAPT_PRIMARY_INGRESS_USB_DEVICE) && defined(ADAPT_PRIMARY_EGRESS_USB_HOST)
#if !ADAPT_POLICY_SYSTEM_PC_MODE
  if (temp_output_mode == OUTPUT_AUTO) {
    addVisibleItem(QCI_WIN_OUTPUT);
  }
#endif
#endif

  if (quickConfigPs2RemoteSelectorVisible(temp_output_mode,
                                           isLiveInputSelection())) {
    addVisibleItem(QCI_PS2_REMOTE_OUTPUT);
  }
  // Remote output does not consume generic remap, turbo, analog, or rotary
  // settings. Keep this page to the requested modes and its output preference.
  if (remoteOutput) {
    return;
  }

#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  addVisibleItem(QCI_USB2DB15_PRESENTATION);
  addVisibleItem(QCI_USB2DB15_TOURNAMENT);
  if (selectedControllerFrame().connected)
    addVisibleItem(QCI_USB2DB15_SAVE_PROFILE);
  if (inputusb_generic_can_map(0)) addVisibleItem(QCI_RESET_DINPUT_MAP);
  const auto& arcadeFrame = selectedControllerFrame();
  if (arcadeFrame.connected && (arcadeFrame.HAS_ANALOG_STICK_MAIN ||
                                arcadeFrame.HAS_ANALOG_STICK_AUX))
    addVisibleItem(QCI_USB2DB15_ACTIVATION);
  if (usb2db15HasAutoShoot() && !menu_kiosk_mode)
    addVisibleItem(QCI_USB2DB15_AUTO_SHOOT);
  // All action positions now come from the assignment wizard. A separate B6
  // source switch would clear/remap the user's learned assignments.
#endif

  if (!canEditModeSpecificSettings()) {
    return;
  }

  if (menuModeUsesBasicKeypadQuickConfig(mode)) {
    #if ADAPT_POLICY_TOURNAMENT_KIOSK
    if (!menu_kiosk_mode)
    #endif
    addVisibleItem(QCI_TURBO);
    if (isLiveInputSelection()) {
      addVisibleItem(QCI_REMAP);
    }
    return;
  }

  #if !defined(ADAPT_PRIMARY_EGRESS_DB15)
  if (hasRumble(mode) && !mouseOutput) {
    addVisibleItem(QCI_RUMBLE);
  }
  #endif

  #if ADAPT_POLICY_TOURNAMENT_KIOSK
  if (!menu_kiosk_mode)
  #endif
  addVisibleItem(QCI_TURBO);
  if (isLiveInputSelection() && !mouseOutput) {
    addVisibleItem(QCI_REMAP);
#if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER && defined(ENABLE_INPUT_USB_BT)
    if (quickConfigSelectionCanMapDinput()) {
      addVisibleItem(QCI_RESET_DINPUT_MAP);
    }
#endif
  }

  const outputMode_t selectedOutput = quickConfigResolvedOutputMode(temp_output_mode);
  const bool fishingLive = isLiveInputSelection() &&
      std::strcmp(selectedControllerFrame().controller_type_name, "Fishing") == 0 &&
      selectedControllerFrame().connected;
  if (buttonMapModeIsUserSelectable(
        mode, selectedOutput, isNsoSpecialActiveForSelection(mode)) && !mouseOutput) {
    addVisibleItem(QCI_BTN_MAP);
  }
#if !ADAPT_POLICY_FIXED_HOTKEY_ASSIGNMENTS
  if (hotkey_count > 0) {
    addVisibleItem(QCI_HOTKEYS_MENU);
  }
#endif
  if (isGameCube(mode)) {
    addVisibleItem(QCI_GC_L_SWITCH);
  }

  const bool nsoSpecialActive = isNsoSpecialActiveForSelection(mode);
  if (menuShouldShowN64ZMode(mode, selectedOutput, nsoSpecialActive)) {
    addVisibleItem(QCI_N64_Z);
  }
  if (isN64(mode)) {
    if (!nsoSpecialActive) {
      addVisibleItem(QCI_N64_CSTICK);
    }
  }
  if (output_mode_supports_switch_device_selection(selectedOutput) &&
      input_mode_supports_nso_special(mode) &&
      input_mode_allows_nso_special_for_connected_players(
        mode, connectedInputFrameCount(inputPortCount()))) {
    addVisibleItem(QCI_NSO_SPECIAL);
  }


  #ifdef ENABLE_INPUT_VBOY
  if (mode == RZORD_VBOY) {
    addVisibleItem(QCI_N64_CSTICK);
  }
  #endif

  #ifdef ENABLE_INPUT_NES
  if (mode == RZORD_NES) {
    addVisibleItem(QCI_POWERPAD);
  }
  #endif

#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  if (mouseOutput) {
    const bool stick = isLiveInputSelection() &&
                       rotaryMouseStickFrame(selectedControllerFrame());
    if (stick) {
      addVisibleItem(QCI_MOUSE_SOURCE);
      addVisibleItem(QCI_TOUCHPAD_MOUSE);
    }
    // A stick or the D-pad can scroll.
    const bool dpad = getRemapButtonDisplayIndex(mode, REMAP_DPAD_FIRST_SLOT) <
                      getRemapButtonCount(mode);
    const bool nativeMouse = isLiveInputSelection() && rotaryMouseNativeFrame(selectedControllerFrame());
    if ((stick || dpad) && !nativeMouse) addVisibleItem(QCI_MOUSE_WHEEL);
  }
#endif
  if (analog_count > 0) {
    addVisibleItem(QCI_ANALOG_MENU);
  }

  // A neGcon's I/II/L rows replace Triggers for that controller.
  const bool neGconLive = quickConfigIsLiveNeGcon(mode, isLiveInputSelection());
  #if !defined(ADAPT_PRIMARY_EGRESS_DB15)
  if (hasAnalogTriggers(mode) && !tempOutputIsSwitchPro() && !neGconLive && !mouseOutput) {
    addVisibleItem(QCI_TRIGGERS);
  }
  if (supportsTempTriggerRightStick() && !mouseOutput) {
    addVisibleItem(QCI_SWITCH_RTRIG_STICK);
  }
  #endif

  // Generic HID still supports D-pad-to-stick routing. Keep that saved setting
  // editable because Left Stick routing overrides the neGcon dial's X axis.
  if ((!contextualRotary || !hasExplicitRotaryOutput()) && !mouseOutput && !fishingLive) {
    #if defined(ADAPT_PRIMARY_EGRESS_DB15)
    if (hasAnalogSticks(mode)) {
      addVisibleItem(QCI_DPAD_BUTTONS);
    }
    #else
    addVisibleItem(QCI_DPAD_BUTTONS);
    #endif
    if (mode == RZORD_DREAMCAST && isLiveInputSelection() &&
        selectedControllerFrame().connected &&
        controllerFrameIsDreamcastTwinStick(selectedControllerFrame())) {
      addVisibleItem(QCI_DPAD2_MODE);
    }
  }
  #if ADAPT_POLICY_QUICK_SHOW_SOCD
  if (!contextualRotary) {
    #if ADAPT_POLICY_TOURNAMENT_KIOSK
    if (!menu_kiosk_mode)
    #endif
    addVisibleItem(QCI_SOCD);
  }
  #endif

#if defined(ADAPT_CAP_DUAL_INPUT_MERGE)
  if (!neGconLive && !isAppendedInputMode(mode) && !mouseOutput) {
    addVisibleItem(QCI_CLASSIC_DUAL_MERGE);
  }
#endif

#ifdef ENABLE_INPUT_PSX
  if (!hasExplicitRotaryOutput() && isGuncon(mode)) {
    addVisibleItem(QCI_GUNCON_OFFSET);
  }
#endif

  #ifdef ENABLE_INPUT_PSX
  if (mode == RZORD_PSX && isLiveInputSelection() &&
      selectedControllerFrame().connected &&
      strncmp(selectedControllerFrame().controller_type_name, "JogCon", 6) == 0) {
    const bool mouseSpinner = strcmp(selectedControllerFrame().controller_type_name, "JogCon-S") == 0 &&
                              !selectedControllerFrame().HAS_ANALOG_STICK_MAIN;
    const bool dedicatedRotary = selectedOutput == OUTPUT_MISTER_PADDLE ||
                                 selectedOutput == OUTPUT_MISTER_SPINNER ||
                                 selectedOutput == OUTPUT_ROTARY_MOUSE;
    const uint8_t jogconMode = effectiveTempJogconMode();
    if (dedicatedRotary) {
      if (selectedOutput == OUTPUT_MISTER_PADDLE) addVisibleItem(QCI_JOGCON_FORCE);
    } else if (!mouseSpinner) {
      addVisibleItem(QCI_JOGCON_MODE);
      if (!contextualRotary && (jogconMode == 0 || jogconMode == 3)) {
        addVisibleItem(QCI_SPINNER_SPEED);
      }
      if (jogconMode == 1 || jogconMode == 2) {
        addVisibleItem(QCI_JOGCON_FORCE);
        if (!contextualRotary) addVisibleItem(QCI_WHEEL_SENS);
      }
      if (jogconMode != 3) {
        addVisibleItem(QCI_JOGCON_WHEEL_AXIS);
      } else {
        addVisibleItem(QCI_JOGCON_DIGITAL);
      }
    }
  }
  #endif

  if ((hasSpinner(mode) || fishingLive) && !contextualRotary) {
    addVisibleItem(QCI_SPINNER_SPEED);
  }
  if (contextualRotary) {
    addVisibleItem(QCI_PADDLE_GAIN);
    addVisibleItem(QCI_ROTARY_REVERSE);
    if (mouseOutput) addVisibleItem(QCI_ROTARY_MOUSE_AXIS);
  }
#ifdef ENABLE_INPUT_PSX
  if (neGconLive) {
    // An input with nowhere to send its pressure is always a button.
    if (tempNegconInputHasPressure(NEGCON_INPUT_I)) addVisibleItem(QCI_NEGCON_I);
    if (tempNegconInputHasPressure(NEGCON_INPUT_II)) addVisibleItem(QCI_NEGCON_II);
    if (tempNegconInputHasPressure(NEGCON_INPUT_L)) addVisibleItem(QCI_NEGCON_L);
    // The gain scales Analog pressure only, so it needs one Analog input.
    static constexpr uint8_t kNegconInputs[] = {
      NEGCON_INPUT_I, NEGCON_INPUT_II, NEGCON_INPUT_L};
    bool analogPressure = false;
    for (uint8_t input : kNegconInputs) {
      analogPressure = analogPressure ||
        (tempNegconInputHasPressure(input) && !tempNegconInputDigital(input));
    }
    if (analogPressure) addVisibleItem(QCI_NEGCON_GAIN);
  }
#endif
  if (hasDrivingEndsControl(mode)) {
    addVisibleItem(QCI_DRIVING_ENDS);
  }
  // PSX Poll Rate and Cmd Gap are not Quick Menu rows (owner, 2026-10-25):
  // they are set in Adapt.html or over serial (SET PSX_TIMING).
}

uint8_t quickConfigItemSettingIds(QuickConfigItem item, DeviceEnum mode,
                                  SettingId* ids, uint8_t maxIds) {
  uint8_t count = 0;
  auto add = [&](SettingId id) {
    if (ids != nullptr && count < maxIds) {
      ids[count] = id;
    }
    ++count;
  };

  switch (item) {
    case QCI_INPUT_MODE: add(SettingId::PersistedInputMode); break;
    case QCI_OUTPUT_MODE: add(SettingId::ConfiguredOutputMode); break;
    case QCI_WIN_OUTPUT: add(SettingId::WinOutput); break;
    case QCI_PS2_REMOTE_OUTPUT: add(SettingId::Ps2RemoteOutput); break;
    case QCI_MOUSE_SOURCE:
    case QCI_TOUCHPAD_MOUSE: add(SettingId::TouchpadMouse); break;
    case QCI_DEADZONE: add(SettingId::Deadzone); break;
    case QCI_DPAD_BUTTONS: add(SettingId::DpadMode); break;
    case QCI_SOCD: add(SettingId::Socd); break;
    case QCI_STICK_INVERT: add(SettingId::StickInvert); break;
    case QCI_RUMBLETECH: add(SettingId::SnesRumbleTech); break;
    case QCI_RUMBLE: add(SettingId::RumbleLevel); break;
    case QCI_TRIGGERS:
    case QCI_SWITCH_RTRIG_STICK:
      add(SettingId::TriggerMode);
      break;
    case QCI_BTN_MAP: add(SettingId::ButtonMapMode); break;
    case QCI_GC_L_SWITCH: add(SettingId::GameCubeLSwitchMode); break;
    case QCI_N64_Z: add(SettingId::N64ZMode); break;
    case QCI_N64_CSTICK: add(SettingId::N64CStickMode); break;
    case QCI_N64_RANGE:
      #ifdef ENABLE_INPUT_N64
      if (mode == RZORD_N64) {
        add(SettingId::N64AnalogRange);
      } else
      #endif
      {
        add(SettingId::WiiAnalogRange);
      }
      break;
    case QCI_NSO_SPECIAL: add(SettingId::NsoSpecial); break;
    case QCI_POWERPAD: add(SettingId::PowerpadMode); break;
    case QCI_CLASSIC_DUAL_MERGE: add(SettingId::ClassicDualMerge); break;
    case QCI_GUNCON_OFFSET:
      add(SettingId::GunconOffsetX);
      add(SettingId::GunconOffsetY);
      break;
    case QCI_JOGCON_MODE: add(SettingId::JogconMode); break;
    case QCI_JOGCON_FORCE: add(SettingId::JogconForce); break;
    case QCI_WHEEL_SENS: add(SettingId::JogconRange); break;
    case QCI_JOGCON_DIGITAL: add(SettingId::JogconDigitalMap); break;
    case QCI_JOGCON_WHEEL_AXIS: add(SettingId::JogconWheelAxis); break;
    // Mouse Axis is the same per-mode byte outside the PSX record.
    case QCI_ROTARY_MOUSE_AXIS: add(SettingId::JogconWheelAxis); break;
    case QCI_SPINNER_SPEED: add(SettingId::SpinnerSpeed); break;
    // Rotary Gain/Reverse write the profile bytes directly so an old record
    // remains on its legacy tuning until the user changes either row.
    case QCI_PADDLE_GAIN:
    case QCI_ROTARY_REVERSE:
      break;
    case QCI_DRIVING_ENDS: add(SettingId::DrivingWrap); break;
    // The neGcon choice shares the NES Device byte, also without a setting ID.
    case QCI_NEGCON_I:
    case QCI_NEGCON_II:
    case QCI_NEGCON_L:
    case QCI_NEGCON_GAIN:
      break;
    case QCI_STICK_CAL:
      add(SettingId::AnalogCalLXMin);
      add(SettingId::AnalogCalLXMax);
      add(SettingId::AnalogCalLYMin);
      add(SettingId::AnalogCalLYMax);
      add(SettingId::AnalogCalRXMin);
      add(SettingId::AnalogCalRXMax);
      add(SettingId::AnalogCalRYMin);
      add(SettingId::AnalogCalRYMax);
      add(SettingId::AnalogCalEnabled);
      break;
    default:
      break;
  }

  return count;
}

uint8_t QuickConfigMenu::debugBuildVisibleItems(DeviceEnum inputMode, outputMode_t outputMode,
                                                uint8_t winOutput, QuickConfigItem* items,
                                                uint8_t maxItems) {
  state = QC_MAIN_MENU;
  selected_index = 0;
  on_bottom_row = false;
  bottom_right = false;
  bottom_index = 0;
  temp_input_mode = inputMode;
  temp_output_mode = canonicalizeOutputMode(outputMode);
  temp_win_output = (winOutput > HOST_OUTPUT_PREFERENCE_MAX) ? HOST_OUTPUT_PREFERENCE_DINPUT : winOutput;
  temp_pointer_source = pointerMouseConfigSource(menu_touchpad_mouse);
  temp_touchpad_mouse = pointerMouseConfigSpeed(menu_touchpad_mouse);

  if (isConcreteModeForPerSettings(temp_input_mode)) {
    loadTempQuickSettingsForMode(temp_input_mode);
  } else {
    temp_deadzone = 0;
    temp_socd = SOCD_OFF;
    temp_dpad_mode = 0;
    temp_stick_invert = 0;
    temp_snes_rumbletech = snes_rumbletech_enabled ? 1 : 0;
    temp_rumble = RUMBLE_MODE_VARIABLE;
    temp_triggers = TRIGGER_MODE_ANALOG;
    temp_spinner = defaultSpinnerSpeedForInputMode(temp_input_mode);
    temp_btn_map = defaultButtonMapModeForInputMode(temp_input_mode);
    temp_gamecube_l_switch = GAMECUBE_L_SWITCH_ZL;
    temp_n64_z = defaultZButtonModeForInputMode(temp_input_mode);
    temp_n64_cstick = defaultN64CStickModeForInputMode(temp_input_mode);
    temp_n64_range = CLASSIC_ANALOG_RANGE_DEFAULT;
    temp_wii_range = CLASSIC_ANALOG_RANGE_DEFAULT;
    temp_rotary_profile = 0;
    temp_jogcon_rotary_profile = 0;
    temp_nso_special = 0;
    temp_powerpad = 0;
    temp_classic_dual_merge = menu_classic_dual_merge;
  }

  buildVisibleItems();

  uint8_t count = 0;
  for (uint8_t i = 0; i < visible_count && count < maxItems; ++i) {
    items[count++] = visible_items[i];
  }

  if (analog_count > 0) {
    for (uint8_t i = 0; i < analog_count && count < maxItems; ++i) {
      items[count++] = analog_items[i];
    }
  }

  return count;
}

const char* QuickConfigMenu::getItemName(QuickConfigItem item) {
  switch (item) {
    case QCI_PLAYER: return "Player";
    case QCI_INPUT_MODE: return "Input";
    case QCI_OUTPUT_MODE: return "Output";
    case QCI_WIN_OUTPUT: return "Win Mode";
    case QCI_PS2_REMOTE_OUTPUT: return "PS2 Remote";
    case QCI_MOUSE_SOURCE:  // the pointer's stick on Mouse output
      return !mousePageActive() ? "Mouse" : "Stick";
    case QCI_TOUCHPAD_MOUSE: return "Pointer Speed";
    case QCI_DUALSENSE_PLAYER_LEDS: return "Player #";
    case QCI_DUALSENSE_LIGHTBAR: return "Light Bar";
    case QCI_USB2DB15_PRESENTATION: return "Home/Idle";
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_ACTIVATION: return "Stick act";
    case QCI_USB2DB15_AUTO_SHOOT: return "Auto Shoot";
    case QCI_USB2DB15_TOURNAMENT: return "Tournament check";
    case QCI_USB2DB15_SAVE_PROFILE: return "Save profile";
#endif
    case QCI_ANALOG_MENU:  // the Buttons page on Mouse output
      return !mousePageActive() ? "Analog" : "Buttons";
    case QCI_MOUSE_WHEEL: return "Wheel";
    case QCI_MOUSE_LEFT: return "Left click";
    case QCI_MOUSE_MIDDLE: return "Middle click";
    case QCI_MOUSE_RIGHT: return "Right click";
    case QCI_MOUSE_BUTTON4: return "Button 4";
    case QCI_MOUSE_BUTTON5: return "Button 5";
    case QCI_HOTKEYS_MENU: return "Hotkeys";
    case QCI_DEADZONE: return "Deadzone";
    case QCI_RANGE_TEST: return "Analog Test";
    case QCI_DPAD_BUTTONS:
      #if defined(ADAPT_PRIMARY_EGRESS_DB15)
      return "Move";
      #else
      return temp_input_mode == RZORD_DREAMCAST && isLiveInputSelection() &&
             selectedControllerFrame().connected &&
             controllerFrameIsDreamcastTwinStick(selectedControllerFrame())
          ? "D-Pad 1" : "DPad";
      #endif
    case QCI_DPAD2_MODE: return "D-Pad 2";
    case QCI_SOCD: return "SOCD";
    case QCI_STICK_INVERT: return "Stick Inv";
    case QCI_RUMBLETECH: return "RumbleTech";
    case QCI_RUMBLE: return "Rumble";
    case QCI_TRIGGERS:
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
      return "Pedal Output";
#else
      return "Triggers";
#endif
    case QCI_JVS_MENU_HOLD: return "Menu Hold";
    case QCI_JVS_HOME_HOTKEY: return "Down+Start";
    case QCI_JVS_HOLD_START: return "Hold Start";
    case QCI_SWITCH_RTRIG_STICK:
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
      return "Pedals to RStick";
#else
      return "RTrig=RStick";
#endif
    case QCI_ANALOG_SETUP: return "Analog Setup";
    case QCI_REMAP: return "Button Remap";
    case QCI_RESET_DINPUT_MAP: return "Remap DInput";
    case QCI_BTN_MAP: return "Button Map";
    case QCI_START_LEFT: return "Start+Left";
    case QCI_START_RIGHT: return "Start+Right";
    case QCI_START_UP: return "Start+Up";
    case QCI_START_DOWN: return "Start+Down";
    case QCI_GC_L_SWITCH: return "L Shoulder";
    case QCI_N64_Z:
      return isGameCube(temp_input_mode) ? "GC Z Btn" : "N64 Z Btn";
    case QCI_N64_CSTICK:
      #ifdef ENABLE_INPUT_VBOY
      if (temp_input_mode == RZORD_VBOY) return "Right DPad";
      #endif
      return "N64 C Buttons";
    case QCI_N64_RANGE: return "Stick Range";
    case QCI_NSO_SPECIAL:
      return "Device";
    case QCI_POWERPAD: return "NES Device";
    case QCI_CLASSIC_DUAL_MERGE: return "2P Merge";
    case QCI_GUNCON_OFFSET: return "GunCon Align";
    case QCI_TURBO: return "Turbo";
    case QCI_JOGCON_MODE: return "JogCon Mode";
    case QCI_JOGCON_FORCE: return "Force";
    case QCI_WHEEL_SENS: return "Wheel Sens";
    case QCI_JOGCON_DIGITAL: return "Dig Output";
    case QCI_JOGCON_WHEEL_AXIS: return "Wheel Axis";
    case QCI_ROTARY_MOUSE_AXIS: return "Mouse Axis";
    case QCI_MOUSE_PLAYERS: return "Mouse Players";
    case QCI_SPINNER_SPEED:
      if (isLiveInputSelection() && selectedControllerFrame().connected &&
          std::strcmp(selectedControllerFrame().controller_type_name, "Fishing") == 0)
        return "Reel Scale";
      #ifdef ENABLE_INPUT_PSX
      if (temp_input_mode == RZORD_PSX &&
          strncmp(selectedControllerFrame().controller_type_name, "JogCon", 6) == 0) {
        return "Spin Sens";
      }
      #endif
      return "Spin Speed";
    case QCI_PADDLE_GAIN: return "Rotary Gain";
    case QCI_ROTARY_REVERSE: return "Reverse";
    case QCI_DRIVING_ENDS: return "Ends";
#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_I: return "I Button";
    case QCI_NEGCON_II: return "II Button";
    case QCI_NEGCON_L: return "L Button";
    case QCI_NEGCON_GAIN: return "Gain";
#endif
    case QCI_STICK_CAL: return "Stick Cal";
    case QCI_DISCARD_EXIT: return "[Cancel]";
    case QCI_EXIT: return "[Save]";
    default: return "?";
  }
}

const char* QuickConfigMenu::getItemValue(QuickConfigItem item) {
  static char buf[16];
  switch (item) {
    case QCI_PLAYER: {
      const uint8_t player = selectedControllerPlayer();
      const controller_state_t& frame = selectedControllerFrame();
      if (frame.connected && frame.controller_type_name[0] != '\0') {
        snprintf(buf, sizeof(buf), "P%u %.8s", (unsigned)(player + 1),
                 frame.controller_type_name);
      } else {
        snprintf(buf, sizeof(buf), "P%u Empty", (unsigned)(player + 1));
      }
      return buf;
    }
    case QCI_INPUT_MODE:
      return getInputModeName(temp_input_mode);
    case QCI_OUTPUT_MODE:
      return get_mode_name(temp_output_mode);
    case QCI_WIN_OUTPUT:
      // Same labels as the System Menu: this item cycles every preference.
      return win_output_names[temp_win_output <= HOST_OUTPUT_PREFERENCE_MAX
                                  ? temp_win_output
                                  : HOST_OUTPUT_PREFERENCE_DINPUT];
    case QCI_PS2_REMOTE_OUTPUT:
      return temp_ps2_remote_output ? "Remote" : "Gamepad";
    case QCI_MOUSE_SOURCE:
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
      if (quickConfigResolvedOutputMode(temp_output_mode) == OUTPUT_ROTARY_MOUSE) {
        const controller_state_t& frame = selectedControllerFrame();
        return !frame.HAS_ANALOG_STICK_MAIN ||
               (frame.HAS_ANALOG_STICK_AUX &&
                temp_pointer_source == POINTER_MOUSE_RIGHT_STICK)
            ? "Right Joy" : "Left Joy";
      }
#endif
      switch (temp_pointer_source) {
        case POINTER_MOUSE_LEFT_STICK: return "Left Joy";
        case POINTER_MOUSE_RIGHT_STICK: return "Right Joy";
        case POINTER_MOUSE_TOUCHPAD: return "Touchpad";
        default: return "Off";
      }
    case QCI_TOUCHPAD_MOUSE:
      snprintf(buf, sizeof(buf), "%u", temp_touchpad_mouse);
      return buf;
    case QCI_DUALSENSE_PLAYER_LEDS:
    case QCI_DUALSENSE_LIGHTBAR:
      return (temp_dualsense_led_policy &
              (1u << (item == QCI_DUALSENSE_LIGHTBAR)))
          ? "On" : "Off";
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_ACTIVATION:
      snprintf(buf, sizeof(buf), "%u%%", usb2db15_tuning::kActivationPercent[
          usb2db15_tuning::sanitizeActivation(temp_usb2db15_activation)]);
      return buf;
    case QCI_USB2DB15_AUTO_SHOOT:
      return usb2db15_tuning::cadenceName(temp_usb2db15_auto_shoot);
    case QCI_USB2DB15_TOURNAMENT: return ">";
    case QCI_USB2DB15_SAVE_PROFILE: return ">";
#endif
    case QCI_USB2DB15_PRESENTATION:
      return temp_usb2db15_presentation == 2 ? "Art/Art" : temp_usb2db15_presentation == 1 ? "Live/Art" : "Live/RFLX";
    case QCI_DEADZONE:
      snprintf(buf, sizeof(buf), "%d%%", temp_deadzone * 5);
      return buf;
    case QCI_RANGE_TEST:
      return ">";
    case QCI_DPAD_BUTTONS: {
      #if defined(ADAPT_PRIMARY_EGRESS_DB15)
      static const char* const dpad_mode_names[] = {
        "DPad/Buttons", "Left Stick", "Right Stick", "DPad/Buttons"
      };
      #else
      static const char* const dpad_mode_names[] = {
        "DPad", "Left Stick", "Right Stick", "Buttons"
      };
      #endif
      const uint8_t idx = isTempDpadModeAllowed(temp_dpad_mode)
                             ? temp_dpad_mode
                             : DPAD_MODE_DPAD;
      return dpad_mode_names[idx];
    }
    case QCI_DPAD2_MODE: {
      static const char* const dpad_mode_names[] = {
        "DPad", "Left Stick", "Right Stick", "Buttons"
      };
      const uint8_t idx = temp_dpad2_mode <= DPAD_MODE_BUTTONS
          ? temp_dpad2_mode : DPAD_MODE_BUTTONS;
      return dpad_mode_names[idx];
    }
    case QCI_SOCD:
      switch (temp_socd) {
        case SOCD_OFF: return "Off";
        case SOCD_NEUTRAL: return "Neutral";
        case SOCD_SECOND: return "2nd Win";
        case SOCD_FIRST: return "1st Win";
        default: return "?";
      }
    case QCI_STICK_INVERT:
      if (temp_stick_invert > getStickInvertMax(temp_input_mode)) return "Off";
      switch (temp_stick_invert) {
        case 0: return "Off";
        case 1: return quickConfigIsLiveNeGcon(temp_input_mode, isLiveInputSelection()) ? "Twist" : "Left";
        case 2: return "Right";
        case 3: return "Both";
        default: return "?";
      }
    case QCI_RUMBLETECH:
      return temp_snes_rumbletech ? "Yes" : "No";
    case QCI_RUMBLE:
      if (!hasVariableRumbleStrength()) {
        return temp_rumble == 0 ? "Off" : "On";
      }
      switch (temp_rumble) {
        case 0: return "Off";
        case RUMBLE_MODE_VARIABLE: return "Variable";
        case RUMBLE_MODE_MAX: return "Max";
        default: return "?";
      }
    case QCI_JVS_MENU_HOLD: {
      static const char* const names[] = {"1.0s", "1.0s", "1.0s", "1.5s", "2.0s", "2.5s", "3.0s"};
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
      return names[temp_jvs_menu_hold <= 6 ? temp_jvs_menu_hold : 2];
#else
      return names[2];
#endif
    }
    case QCI_JVS_HOME_HOTKEY:
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
      return temp_jvs_home_hotkey ? "OSD" : "Off";
#else
      return "Off";
#endif
    case QCI_JVS_HOLD_START:
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
      return temp_jvs_hold_start ? "OSD" : "Off";
#else
      return "Off";
#endif
    case QCI_TRIGGERS:
      switch (temp_triggers) {
        case TRIGGER_MODE_DIGITAL: return "Digital";
        case TRIGGER_MODE_BOTH: return "Analog+Digital";
        default: return "Analog";
      }
    case QCI_SWITCH_RTRIG_STICK:
      return temp_triggers == TRIGGER_MODE_RSTICK ? "On" : "Off";
    case QCI_REMAP:
    case QCI_RESET_DINPUT_MAP:
    case QCI_ANALOG_SETUP:
    case QCI_ANALOG_MENU:
    case QCI_HOTKEYS_MENU:
      return ">";
#if defined(ADAPT_CAP_CONTROLLER_HOTKEY_PROFILES)
    case QCI_START_LEFT: case QCI_START_RIGHT: case QCI_START_UP:
      return directionalControllerHotkeyName(temp_direction_hotkeys[item-QCI_START_LEFT],
          item-QCI_START_LEFT, selectedControllerPlayer(),
          temp_output_mode == OUTPUT_AUTO ? get_effective_output_mode() : temp_output_mode);
    case QCI_START_DOWN:
      switch (temp_direction_hotkey_home) {
        case 1: return "Off";
        case 2: return "On";
        default: return "Auto";
      }
#elif defined(ADAPT_CAP_DIRECTIONAL_HOTKEYS)
    case QCI_START_LEFT: case QCI_START_RIGHT: case QCI_START_UP:
      return directionalHotkeyName(temp_direction_hotkeys[item-QCI_START_LEFT],temp_input_mode,
          isLiveInputSelection() ? selectedControllerFrame().controller_type_name : "",
          item-QCI_START_LEFT, temp_output_mode == OUTPUT_AUTO ? get_effective_output_mode() : temp_output_mode);
#endif
    case QCI_BTN_MAP:
      return temp_btn_map ? "Position" : "Name";
    case QCI_GC_L_SWITCH:
      return temp_gamecube_l_switch == GAMECUBE_L_SWITCH_L ? "L" : "ZL";
    case QCI_N64_Z:
      return isGameCube(temp_input_mode)
               ? (temp_n64_z ? "Back" : "R1")
               : (temp_n64_z ? "L2" : "L1");
    case QCI_N64_CSTICK:
      switch (temp_n64_cstick) {
        case 1: return "Buttons";
        case 2: return "Stick";
        default: return "Auto";
      }
    case QCI_N64_RANGE: {
      static const char* const range_labels[] = { "Raw", "Norm", "Cal", "Learn" };
      return range_labels[sanitizeClassicAnalogRange(getTempClassicAnalogRange())];
    }
    case QCI_NSO_SPECIAL:
      return switch_device_selection_label(temp_input_mode, temp_nso_special);
    case QCI_POWERPAD:
      switch (temp_powerpad) {
        case NES_DEVICE_POWERPAD: return "PowerPad";
        case NES_DEVICE_VAUS_NES: return "Vaus NES";
        case NES_DEVICE_VAUS_FAMICOM: return "Vaus FC";
        default: return "Pad";
      }
    case QCI_CLASSIC_DUAL_MERGE:
      return ">";
    case QCI_GUNCON_OFFSET:
      snprintf(buf, sizeof(buf), "%+d,%+d", temp_guncon_x, temp_guncon_y);
      return buf;
    case QCI_TURBO:
      return ">";
    case QCI_JOGCON_MODE: {
      static const char* mode_labels[] = { "Spinner", "Paddle", "Wheel", "Fake Spin" };
      return mode_labels[temp_jogcon_mode <= 3 ? temp_jogcon_mode : 0];
    }
    case QCI_JOGCON_FORCE:
      if (temp_jogcon_force == 1) return "Low";
      if (temp_jogcon_force == 3) return "Med-L";
      if (temp_jogcon_force == 7) return "Med-H";
      return "High";
    case QCI_WHEEL_SENS: {
      static const char* sens_labels[] = { "Fine", "Normal", "Coarse" };
      return sens_labels[temp_wheel_sens];
    }
    case QCI_JOGCON_DIGITAL: {
      static const char* dig_labels[] = { "Left/Right", "L3/R3", "L/R", "Up/Down" };
      return dig_labels[temp_jogcon_digital];
    }
    case QCI_JOGCON_WHEEL_AXIS:
      return temp_jogcon_wheel_axis ? "Y" : "X";
    case QCI_ROTARY_MOUSE_AXIS:
      return temp_mouse.rotary_wheel ? "Wheel" : (temp_jogcon_wheel_axis ? "Y" : "X");
    case QCI_MOUSE_PLAYERS:
      return temp_mouse.two_players ? "2P" : "1P";
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    case QCI_MOUSE_WHEEL:
      return mouseWheelName(tempMouseWheel(false));
    case QCI_MOUSE_LEFT:
    case QCI_MOUSE_MIDDLE:
    case QCI_MOUSE_RIGHT:
    case QCI_MOUSE_BUTTON4:
    case QCI_MOUSE_BUTTON5:
      return mouseButtonName(tempMouseButtonSlot(mouseRoleForItem(item)));
#endif
    case QCI_SPINNER_SPEED:
      return spinner_speed_labels[temp_spinner];
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    case QCI_PADDLE_GAIN:
      return rotaryGainLabel(getTempRotaryTuning(temp_input_mode).gain);
    case QCI_ROTARY_REVERSE:
      return getTempRotaryTuning(temp_input_mode).reverse ? "On" : "Off";
    case QCI_DRIVING_ENDS:
      return tempDrivingEndsWrap(temp_input_mode) ? "Wrap" : "Stop";
#endif
#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_I:
    case QCI_NEGCON_II:
    case QCI_NEGCON_L:
      return tempNegconInputDigital(negconInputForItem(item)) ? "Digital" : "Analog";
    case QCI_NEGCON_GAIN:
      return negconGainName(temp_negcon_gain);
#endif
    case QCI_STICK_CAL:
      return ">";
    default:
      return "";
  }
}

bool QuickConfigMenu::isDirectlyEditable(QuickConfigItem item) {
  switch (item) {
    case QCI_START_LEFT: case QCI_START_RIGHT: case QCI_START_UP:
    case QCI_START_DOWN:
    case QCI_JVS_MENU_HOLD:
    case QCI_JVS_HOME_HOTKEY:
    case QCI_JVS_HOLD_START: return true;
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_ACTIVATION:
    case QCI_USB2DB15_AUTO_SHOOT:
      return true;
#endif
    case QCI_PLAYER:
    case QCI_INPUT_MODE:
    case QCI_OUTPUT_MODE:
    case QCI_WIN_OUTPUT:
    case QCI_PS2_REMOTE_OUTPUT:
    case QCI_MOUSE_SOURCE:
    case QCI_TOUCHPAD_MOUSE:
    case QCI_DUALSENSE_PLAYER_LEDS:
    case QCI_DUALSENSE_LIGHTBAR:
    case QCI_USB2DB15_PRESENTATION:
    case QCI_DEADZONE:
    case QCI_DPAD_BUTTONS:
    case QCI_SOCD:
    case QCI_STICK_INVERT:
    case QCI_RUMBLETECH:
    case QCI_TRIGGERS:
    case QCI_SWITCH_RTRIG_STICK:
    case QCI_SPINNER_SPEED:
    case QCI_PADDLE_GAIN:
    case QCI_ROTARY_REVERSE:
    case QCI_DRIVING_ENDS:
#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_I:
    case QCI_NEGCON_II:
    case QCI_NEGCON_L:
    case QCI_NEGCON_GAIN:
#endif
    case QCI_JOGCON_MODE:
    case QCI_JOGCON_FORCE:
    case QCI_WHEEL_SENS:
    case QCI_JOGCON_DIGITAL:
    case QCI_JOGCON_WHEEL_AXIS:
    case QCI_ROTARY_MOUSE_AXIS:
    case QCI_MOUSE_PLAYERS:
    case QCI_BTN_MAP:
    case QCI_GC_L_SWITCH:
    case QCI_N64_Z:
    case QCI_N64_CSTICK:
    case QCI_N64_RANGE:
    case QCI_NSO_SPECIAL:
    case QCI_POWERPAD:
      return true;
    default:
      return false;
  }
}

bool QuickConfigMenu::needsSubmenu(QuickConfigItem item) {
  switch (item) {
    case QCI_INPUT_MODE:
    case QCI_OUTPUT_MODE:
      return true;
    // Lists: four or more values.
    case QCI_TOUCHPAD_MOUSE:
    case QCI_DEADZONE:
    case QCI_SOCD:
    case QCI_SPINNER_SPEED:
    case QCI_N64_RANGE:
    case QCI_POWERPAD:
    case QCI_JOGCON_MODE:
    case QCI_JOGCON_FORCE:
    case QCI_JOGCON_DIGITAL:
    case QCI_START_LEFT:
    case QCI_START_RIGHT:
    case QCI_START_UP:
#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_GAIN:
#endif
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_ACTIVATION:
    case QCI_USB2DB15_AUTO_SHOOT:
#endif
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
    case QCI_JVS_MENU_HOLD:
#endif
      return true;
    // These offer fewer values on some pads and outputs, and cycle there.
    case QCI_DPAD_BUTTONS:
    case QCI_DPAD2_MODE:
    case QCI_STICK_INVERT:
    case QCI_MOUSE_SOURCE:
    case QCI_NSO_SPECIAL:
      return fillChoiceValues(item, nullptr) >= kMenuChoiceListMinValues;
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    case QCI_PADDLE_GAIN:
    case QCI_MOUSE_WHEEL:
    case QCI_MOUSE_LEFT:
    case QCI_MOUSE_MIDDLE:
    case QCI_MOUSE_RIGHT:
    case QCI_MOUSE_BUTTON4:
    case QCI_MOUSE_BUTTON5:
      return true;
#endif
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_TOURNAMENT: return true;
    case QCI_USB2DB15_SAVE_PROFILE: return true;
#endif
    case QCI_TURBO:
    case QCI_REMAP:
    case QCI_RESET_DINPUT_MAP:
    case QCI_ANALOG_SETUP:
    case QCI_ANALOG_MENU:
    case QCI_HOTKEYS_MENU:
    case QCI_RANGE_TEST:
    case QCI_STICK_CAL:
    case QCI_GUNCON_OFFSET:
    case QCI_CLASSIC_DUAL_MERGE:
    case QCI_RUMBLE:
      return true;
    default:
      return false;
  }
}

void QuickConfigMenu::enterSubmenu(QuickConfigItem item) {
  return_to_analog_submenu = false;

  switch (item) {
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
    case QCI_ANALOG_SETUP:
      openJvsAnalogWizard();
      break;
#endif
    case QCI_INPUT_MODE:
    case QCI_OUTPUT_MODE:
      openModePicker(item);
      break;
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_SAVE_PROFILE:
      applyAndClose();
      usb2db15ArcadeRequestProfileSave();
      break;
    case QCI_USB2DB15_TOURNAMENT:
      state = QC_USB2DB15_TOURNAMENT;
      break;
#endif
    case QCI_TURBO:
      state = QC_TURBO_LIST;
      turbo_index = 0;
      break;
    case QCI_REMAP:
      state = QC_REMAP_LIST;
      remap_index = 0;
      break;
    case QCI_RESET_DINPUT_MAP:
      {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION) || (ADAPT_POLICY_QUICK_SELECTED_CONTROLLER && defined(ENABLE_INPUT_USB_BT))
        usb_hid_mapping_target_t target{};
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
        const bool captured = inputusb_generic_capture_mapping(0, &target);
#else
        const bool captured = usbBtCapturePlayerDinputMapping(selectedControllerPlayer(), &target);
#endif
        if (captured) {
          reset_dinput_device = target.device_index;
          dinput_reset_connection_order = target.connection_order;
          dinput_reset_key = target.key;
          state = QC_CONFIRM_DINPUT_RESET;
          dinput_reset_confirm_index = 0;
        } else {
          action_error = true;
          rebuildVisibleAndClampSelection();
        }
#endif
      }
      break;
    case QCI_ANALOG_MENU:
      buildAnalogItems();
      analog_index = 0;
      state = QC_ANALOG_SUBMENU;
      break;
    case QCI_HOTKEYS_MENU:
      buildHotkeyItems();
      hotkey_index = 0;
      state = QC_HOTKEYS_SUBMENU;
      break;
    case QCI_RANGE_TEST:
      state = QC_RANGE_TEST;
      range_trace[0].reset();
      range_trace[1].reset();
#if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER
      range_test_target = analogDiagnosticTargetForPort(
        deviceMode, controller_frames,
        max_devices < MAX_USB_OUT ? max_devices : MAX_USB_OUT,
        selectedControllerPlayer());
#else
      range_test_target = analogDiagnosticDefaultTarget(
        deviceMode, controller_frames,
        max_devices < MAX_USB_OUT ? max_devices : MAX_USB_OUT);
#endif
      range_test_right_stick =
        range_test_target.stick == AnalogDiagnosticStick::Aux;
      range_trace_mode = deviceMode;
      range_trace_type[0] = '\0';
      if (analogDiagnosticTargetIsValid(range_test_target)) {
        const controller_state_t& frame =
          controllerFrameConst(range_test_target.port);
        range_trace_octagonal = analogTraceUsesOctagonalGate(
          deviceMode, frame.controller_type_name);
        strncpy(range_trace_type, frame.controller_type_name,
                sizeof(range_trace_type) - 1);
        range_trace_type[sizeof(range_trace_type) - 1] = '\0';
      } else {
        range_trace_octagonal = false;
      }
      range_trace_dirty = true;
      range_last_draw_ms = 0;
      needs_redraw = true;
      break;
    case QCI_STICK_CAL:
      state = QC_STICK_CAL;
      cal_lx_min = 0;
      cal_lx_max = 0;
      cal_ly_min = 0;
      cal_ly_max = 0;
      cal_rx_min = 0;
      cal_rx_max = 0;
      cal_ry_min = 0;
      cal_ry_max = 0;
      cal_selection = 0;
      cal_has_data = false;
      needs_redraw = true;
      break;
#ifdef ENABLE_INPUT_PSX
    case QCI_GUNCON_OFFSET:
      state = QC_GUNCON_ADJUST;
      guncon_edit_axis = 0;
      break;
#endif
    case QCI_CLASSIC_DUAL_MERGE:
      state = QC_DUAL_MERGE_MAP;
      dual_merge_index = 0;
      break;
    case QCI_RUMBLE:
      state = QC_RUMBLE_SUBMENU;
      rumble_index = 0;
      break;
    default:
      if (needsSubmenu(item)) openChoicePicker(item);
      break;
  }
}
