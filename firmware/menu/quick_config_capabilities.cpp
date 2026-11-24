#include "../product_config.h"

#include "quick_config.h"
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
#include "../input/jvs/jvs_analog_layout_runtime.h"
#endif

#include "menu_capabilities.h"
#include "../core/dreamcast_twin_stick.h"
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
#include "../features/usb2db15/usb2db15_arcade_mapping.h"
#endif
#if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER && defined(ENABLE_INPUT_USB_BT)
  #include "../input/usb_bt_composite/Input_UsbBt.h"
#elif ADAPT_POLICY_QUICK_SELECTED_CONTROLLER && defined(ENABLE_INPUT_USB)
  #include "../input/usb_host/input_usb/input_usb.h"
#endif

namespace {

bool isPsxMode(DeviceEnum mode) {
#ifdef ENABLE_INPUT_PSX
  return mode == RZORD_PSX;
#else
  (void)mode;
  return false;
#endif
}

outputMode_t resolvedCapabilityOutputMode(outputMode_t mode) {
  return (mode == OUTPUT_AUTO) ? get_effective_output_mode() : mode;
}

}  // namespace

bool QuickConfigMenu::hasAnalogSticks(DeviceEnum mode) {
  return menuModeCurrentControllerHasAnalogStick(mode);
}

bool QuickConfigMenu::hasAnalogTriggers(DeviceEnum mode) {
  return menuModeCurrentControllerHasAnalogTriggers(mode);
}

bool QuickConfigMenu::isSnesMode(DeviceEnum mode) {
#ifdef ENABLE_INPUT_SNES
  return mode == RZORD_SNES;
#else
  (void)mode;
  return false;
#endif
}

bool QuickConfigMenu::hasRumble(DeviceEnum mode) {
#ifdef ENABLE_INPUT_USB
  if (mode == RZORD_USB) {
    return !isLiveInputSelection() || rumbleMotorCount() > 0;
  }
#endif
#ifdef ENABLE_INPUT_ESP32_SPI
  if (mode == RZORD_ESP32_SPI) {
    return !isLiveInputSelection() || rumbleMotorCount() > 0;
  }
#endif
  return menuModeCurrentRumbleCapabilities(mode).motor_count > 0;
}

uint8_t QuickConfigMenu::rumbleMotorCount() {
#if ADAPT_POLICY_QUICK_SELECTED_CONTROLLER
  if (temp_input_mode == RZORD_USB) {
#ifdef ENABLE_INPUT_USB_BT
    return usbBtPlayerRumbleMotorCount(selectedControllerPlayer());
#elif defined(ENABLE_INPUT_USB)
    const usb_input_device_t* device =
        get_usb_device(selectedControllerPlayer());
    if (device == nullptr || !device->connected) return 0;
    switch (device->report.CONFIG.DEVICE_TYPE) {
      case DEVICE_TYPE_XINPUT:
      case DEVICE_TYPE_XID:
      case DEVICE_TYPE_PS3:
      case DEVICE_TYPE_PS4:
      case DEVICE_TYPE_PS5:
      case DEVICE_TYPE_SWITCH:
        return 2;
      default:
        return 0;
    }
#endif
  }
#endif
#ifdef ENABLE_INPUT_USB
  if (temp_input_mode == RZORD_USB) {
    return 2;
  }
#endif
#ifdef ENABLE_INPUT_ESP32_SPI
  if (temp_input_mode == RZORD_ESP32_SPI) {
    return 2;
  }
#endif
  return menuModeCurrentRumbleCapabilities(temp_input_mode).motor_count;
}

RumbleStrengthSupport QuickConfigMenu::rumbleStrengthSupportForMode(
    DeviceEnum mode) {
#ifdef ENABLE_INPUT_USB
  if (mode == RZORD_USB) {
    return RumbleStrengthSupport::Variable;
  }
#endif
#ifdef ENABLE_INPUT_ESP32_SPI
  if (mode == RZORD_ESP32_SPI) {
    return RumbleStrengthSupport::Variable;
  }
#endif
  return potentialRumbleCapabilitiesForMode(mode).strength;
}

bool QuickConfigMenu::hasVariableRumbleStrength() {
  return rumbleStrengthSupportForMode(temp_input_mode) ==
         RumbleStrengthSupport::Variable;
}

void QuickConfigMenu::cycleTempRumbleLevel(bool forward) {
  RumbleStrengthSupport support =
    menuModeCurrentRumbleCapabilities(temp_input_mode).strength;
#ifdef ENABLE_INPUT_USB
  if (temp_input_mode == RZORD_USB) {
    support = rumbleStrengthSupportForMode(temp_input_mode);
  }
#endif
#ifdef ENABLE_INPUT_ESP32_SPI
  if (temp_input_mode == RZORD_ESP32_SPI) {
    support = rumbleStrengthSupportForMode(temp_input_mode);
  }
#endif
  temp_rumble = cycleRumbleLevelForSupport(
    support,
    temp_rumble,
    forward
  );
}

bool QuickConfigMenu::isRumbleMenuItemVisible(RumbleMenuItem item) {
  const uint8_t motors = rumbleMotorCount();
  switch (item) {
    case QCI_RUMBLE_LEVEL:
    case QCI_RUMBLE_BACK:
      return true;
    case QCI_RUMBLE_TEST_HEAVY:
    case QCI_RUMBLE_TEST_LIGHT:
      return motors >= 2;
    case QCI_RUMBLE_TEST_BOTH:
      return motors >= 1;
    case QCI_RUMBLE_RAMP_HEAVY:
      return motors >= 1 && hasVariableRumbleStrength();
    case QCI_RUMBLE_HEAVY:
    case QCI_RUMBLE_LIGHT:
    default:
      return false;
  }
}

void QuickConfigMenu::advanceRumbleIndex(bool forward) {
  for (uint8_t attempts = 0; attempts < QCI_RUMBLE_COUNT; ++attempts) {
    if (forward) {
      rumble_index = (uint8_t)((rumble_index + 1) % QCI_RUMBLE_COUNT);
    } else {
      rumble_index = (rumble_index == 0) ? (QCI_RUMBLE_COUNT - 1) : (uint8_t)(rumble_index - 1);
    }
    if (isRumbleMenuItemVisible((RumbleMenuItem)rumble_index)) {
      return;
    }
  }
  rumble_index = QCI_RUMBLE_LEVEL;
}

bool QuickConfigMenu::isGuncon(DeviceEnum mode) {
  const controller_state_t& frame = selectedControllerFrame();
  return isPsxMode(mode) && isLiveInputSelection() && frame.connected &&
         strcmp(frame.controller_type_name, "GunCon") == 0;
}

bool QuickConfigMenu::isJogcon(DeviceEnum mode) {
  return isPsxMode(mode);
}

bool QuickConfigMenu::isDrivingFallbackActive(DeviceEnum mode) {
  #ifdef ENABLE_INPUT_DRIVING
  if (mode == RZORD_SMS) {
    for (uint8_t i = 0; i < MAX_USB_OUT; ++i) {
      if (strncmp(controllerFrameConst(i).controller_type_name, "Driving", 7) == 0) {
        return true;
      }
    }
  }
  #endif
  return false;
}

bool QuickConfigMenu::hasSpinner(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
    #endif
    #if defined(ENABLE_INPUT_DRIVING) || defined(ENABLE_INPUT_SMS) || defined(ENABLE_INPUT_JAGUAR)
      #ifdef ENABLE_INPUT_DRIVING
      if (mode == RZORD_SMS) {
        return isDrivingFallbackActive(mode);
      }
      #endif
      return true;
    #endif
    default:
      return false;
  }
}

bool QuickConfigMenu::isNintendoController(DeviceEnum mode) {
  return menuModeIsNintendoController(mode);
}

bool QuickConfigMenu::isN64(DeviceEnum mode) {
  return menuModeIsN64Controller(mode);
}

bool QuickConfigMenu::isGameCube(DeviceEnum mode) {
#ifdef ENABLE_INPUT_GAMECUBE
  return mode == RZORD_GAMECUBE;
#else
  (void)mode;
  return false;
#endif
}

bool QuickConfigMenu::hasClassicAnalogRange(DeviceEnum mode) {
  return classicModeHasRangeSetting(mode);
}

uint8_t QuickConfigMenu::getTempClassicAnalogRange() {
  return (temp_input_mode == RZORD_N64) ? temp_n64_range : temp_wii_range;
}

void QuickConfigMenu::cycleTempClassicAnalogRange(bool forward) {
  uint8_t& value = (temp_input_mode == RZORD_N64) ? temp_n64_range : temp_wii_range;
  value = sanitizeClassicAnalogRange(value);
  if (forward) {
    value = (value + 1) % CLASSIC_ANALOG_RANGE_COUNT;
  } else {
    value = (value == 0) ? (CLASSIC_ANALOG_RANGE_COUNT - 1) : value - 1;
  }
}

void QuickConfigMenu::cycleTempSwitchDevice(bool forward) {
  const uint8_t maxValue = max_switch_device_selection_for_input_mode(temp_input_mode);
  temp_nso_special = sanitize_switch_device_selection(temp_input_mode, temp_nso_special);
  if (forward) {
    temp_nso_special = (uint8_t)((temp_nso_special + 1) % (maxValue + 1));
  } else {
    temp_nso_special = temp_nso_special == 0 ? maxValue : (uint8_t)(temp_nso_special - 1);
  }
}

bool QuickConfigMenu::hasRightStick(DeviceEnum mode) {
  return menuModeCurrentControllerHasRightStick(mode);
}

uint8_t QuickConfigMenu::getDpadModeMax(outputMode_t mode) {
#if defined(ADAPT_PRIMARY_EGRESS_DB15)
  (void)mode;
  return DPAD_MODE_RIGHT_STICK;
#else
  return output_supports_dpad_buttons(
             canonicalizeOutputMode(resolvedCapabilityOutputMode(mode))) ? 3 : 2;
#endif
}

bool QuickConfigMenu::isTempDpadModeAllowed(uint8_t mode) {
  switch (mode) {
    case DPAD_MODE_DPAD:
      return true;
    case DPAD_MODE_LEFT_STICK:
      #if defined(ADAPT_PRIMARY_EGRESS_DB15)
      return selectedControllerFrame().connected &&
             selectedControllerFrame().HAS_ANALOG_STICK_MAIN;
      #else
      return true;
      #endif
    case DPAD_MODE_RIGHT_STICK:
      #if defined(ADAPT_PRIMARY_EGRESS_DB15)
      return hasRightStick(temp_input_mode);
      #else
      return true;
      #endif
    case DPAD_MODE_BUTTONS:
      #if defined(ADAPT_PRIMARY_EGRESS_DB15)
      return false;
      #else
      return output_supports_dpad_buttons(
          canonicalizeOutputMode(resolvedCapabilityOutputMode(temp_output_mode)));
      #endif
    default:
      return false;
  }
}

bool QuickConfigMenu::isTempDpad2ModeAllowed(uint8_t mode) {
  if (mode == DPAD_MODE_BUTTONS) {
#ifdef ENABLE_INPUT_DREAMCAST
    const controller_state_t& frame = selectedControllerFrame();
    if (temp_input_mode == RZORD_DREAMCAST && isLiveInputSelection() &&
        frame.connected && controllerFrameIsDreamcastTwinStick(frame)) {
      return true;
    }
#endif
  }
  return isTempDpadModeAllowed(mode);
}

void QuickConfigMenu::cycleTempDpadMode(bool forward) {
  uint8_t value = temp_dpad_mode;
  for (uint8_t attempts = 0; attempts < 4; ++attempts) {
    if (forward) {
      value = (uint8_t)((value + 1) % 4);
    } else {
      value = (value == 0) ? 3 : (uint8_t)(value - 1);
    }
    if (isTempDpadModeAllowed(value)) {
      temp_dpad_mode = value;
      return;
    }
  }
  temp_dpad_mode = DPAD_MODE_DPAD;
}

void QuickConfigMenu::cycleTempDpad2Mode(bool forward) {
  uint8_t value = temp_dpad2_mode;
  for (uint8_t attempts = 0; attempts < 4; ++attempts) {
    value = forward ? static_cast<uint8_t>((value + 1) % 4)
                    : (value == 0 ? 3 : static_cast<uint8_t>(value - 1));
    if (isTempDpad2ModeAllowed(value)) {
      temp_dpad2_mode = value;
      return;
    }
  }
  temp_dpad2_mode = DPAD_MODE_BUTTONS;
}

uint8_t QuickConfigMenu::getStickInvertMax(DeviceEnum mode) {
  return hasRightStick(mode) ? 3 : 1;
}

uint8_t QuickConfigMenu::getTriggerModeMax(outputMode_t mode) {
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
  if (jvsCurrentAnalogHasPedals())
    return TRIGGER_MODE_BOTH;  // Preserve a previously stored Both setting.
#endif
  if (!hasAnalogTriggers(temp_input_mode)) {
    return TRIGGER_MODE_ANALOG;
  }
  if (outputModeSupportsTriggerBothMode(resolvedCapabilityOutputMode(mode)) &&
      menuModeSupportsTriggerBothMode(temp_input_mode)) {
    return TRIGGER_MODE_BOTH;
  }
  return TRIGGER_MODE_DIGITAL;
}

bool QuickConfigMenu::isTempTriggerModeAllowed(uint8_t mode) {
  switch (mode) {
    case TRIGGER_MODE_ANALOG:
    case TRIGGER_MODE_DIGITAL:
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
      if (jvsCurrentAnalogHasPedals()) return true;
#endif
      return hasAnalogTriggers(temp_input_mode);
    case TRIGGER_MODE_RSTICK:
      return supportsTempTriggerRightStick();
    case TRIGGER_MODE_BOTH:
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
      // Older configurations can contain this value. Keep it on an unrelated
      // Quick save even though the JVS selector does not offer it.
      if (jvsCurrentAnalogHasPedals()) return true;
#endif
      return hasAnalogTriggers(temp_input_mode) &&
             outputModeSupportsTriggerBothMode(resolvedCapabilityOutputMode(temp_output_mode)) &&
             menuModeSupportsTriggerBothMode(temp_input_mode);
    default:
      return false;
  }
}

bool QuickConfigMenu::tempOutputIsSwitchPro() {
  const auto output = canonicalizeOutputMode(resolvedCapabilityOutputMode(temp_output_mode));
  return output == OUTPUT_SWITCHPRO || output == OUTPUT_SWITCH2PRO;
}

bool QuickConfigMenu::supportsTempTriggerRightStick() {
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
  return jvsCurrentAnalogHasPedals() && tempOutputIsSwitchPro();
#endif
  return tempOutputIsSwitchPro() &&
         hasAnalogTriggers(temp_input_mode) &&
         menuModeSupportsTriggerAxisMode(temp_input_mode);
}

void QuickConfigMenu::cycleTempTriggerMode(bool forward) {
  uint8_t value = temp_triggers;
  for (uint8_t attempts = 0; attempts < 4; ++attempts) {
    if (forward) {
      value = (uint8_t)((value + 1) % 4);
    } else {
      value = (value == 0) ? 3 : (uint8_t)(value - 1);
    }
    if (value == TRIGGER_MODE_RSTICK) {
      continue;
    }
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
    if (jvsCurrentAnalogHasPedals() && value == TRIGGER_MODE_BOTH)
      continue;
#endif
    if (isTempTriggerModeAllowed(value)) {
      temp_triggers = value;
      return;
    }
  }
  temp_triggers = TRIGGER_MODE_ANALOG;
}

bool QuickConfigMenu::isTurboConfigIndexVisible(uint8_t configIndex) {
  const TurboButtonConfig& cfg = getSelectedTurboConfig();
  if (configIndex >= cfg.count) {
    return false;
  }

  const uint8_t btn = cfg.indices[configIndex];
  const bool analogTriggerSlot = btn == TURBO_BTN_6 || btn == TURBO_BTN_7;
  if (analogTriggerSlot &&
      temp_triggers != TRIGGER_MODE_DIGITAL &&
      hasAnalogTriggers(temp_input_mode) &&
      menuModeSupportsTriggerBothMode(temp_input_mode) &&
      !menuModeHasIndependentDigitalTriggerButtons(temp_input_mode)) {
    return false;
  }
  return true;
}

uint8_t QuickConfigMenu::getVisibleTurboCount() {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  if (usb2db15ArcadeSessionReady()) {
    return usb2db15ArcadeGameButtonCount();
  }
#endif
  const TurboButtonConfig& cfg = getSelectedTurboConfig();
  uint8_t count = 0;
  for (uint8_t i = 0; i < cfg.count; ++i) {
    if (isTurboConfigIndexVisible(i)) {
      ++count;
    }
  }
  return count;
}

uint8_t QuickConfigMenu::getTurboConfigIndexForVisibleIndex(uint8_t visibleIndex) {
  const TurboButtonConfig& cfg = getSelectedTurboConfig();
  uint8_t visible = 0;
  for (uint8_t i = 0; i < cfg.count; ++i) {
    if (!isTurboConfigIndexVisible(i)) {
      continue;
    }
    if (visible == visibleIndex) {
      return i;
    }
    ++visible;
  }
  return 0xFF;
}

uint8_t QuickConfigMenu::getTurboButtonIndexForVisibleIndex(uint8_t visibleIndex) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  if (usb2db15ArcadeSessionReady()) {
    return visibleIndex < usb2db15ArcadeGameButtonCount()
        ? usb2db15ArcadeControlSlot(visibleIndex)
        : 0xFF;
  }
#endif
  const uint8_t configIndex = getTurboConfigIndexForVisibleIndex(visibleIndex);
  if (configIndex == 0xFF) {
    return 0xFF;
  }
  return getSelectedTurboConfig().indices[configIndex];
}

const char* QuickConfigMenu::getTurboButtonNameForVisibleIndex(uint8_t visibleIndex) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  if (usb2db15ArcadeSessionReady()) {
    return visibleIndex < usb2db15ArcadeGameButtonCount()
        ? usb2db15ArcadeControlName(visibleIndex)
        : "?";
  }
#endif
  const uint8_t configIndex = getTurboConfigIndexForVisibleIndex(visibleIndex);
  if (configIndex == 0xFF) {
    return "?";
  }
  return getSelectedTurboConfig().names[configIndex];
}

uint8_t QuickConfigMenu::getStickCount(DeviceEnum mode) {
  if (mode == deviceMode) {
    if (menuModeCurrentControllerHasRightStick(mode)) {
      return 2;
    }
    if (menuModeCurrentControllerHasAnalogStick(mode)) {
      return 1;
    }
    return 0;
  }

  switch (mode) {
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return 1;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      return 1;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
    #endif
    #ifdef ENABLE_INPUT_USB
    case RZORD_USB:
    #endif
    #if defined(ENABLE_INPUT_PSX) || defined(ENABLE_INPUT_GAMECUBE) || defined(ENABLE_INPUT_WII) || defined(ENABLE_INPUT_USB)
      return 2;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      return 1;
    #endif
    default:
      return 0;
  }
}
