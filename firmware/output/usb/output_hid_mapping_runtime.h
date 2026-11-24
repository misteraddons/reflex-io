#pragma once

#include <cstring>

#include "../../core/button_map_mode.h"
#ifdef __cplusplus
extern "C++" {
#endif
#include "../../core/button_remap.h"
#ifdef __cplusplus
}
#endif
#include "../../core/dreamcast_twin_stick.h"
#include "../../core/negcon_input_modes.h"
#include "../../core/rotary_settings.h"
#include "../../core/rotary_motion_state.h"
#include "../../menu/menu_runtime_state.h"

// Internal HID-family mapping helpers. This stays header-only so it can reuse
// the shared controller-state accessors, output reports, and runtime flags that
// still live in out_usb.h.

inline void set_digital_from_analog(uint8_t port, uint8_t threshold = 100) {
  controller_state_t& frame = controllerFrame(port);
  if (frame.HAS_ANALOG_DPAD) {
    frame.PAD_U |= (frame.ANALOG_PAD_U > threshold);
    frame.PAD_D |= (frame.ANALOG_PAD_D > threshold);
    frame.PAD_L |= (frame.ANALOG_PAD_L > threshold);
    frame.PAD_R |= (frame.ANALOG_PAD_R > threshold);
  }
  if (frame.HAS_ANALOG_MAIN_BUTTONS) {
     // todo implement
  }
  if (frame.HAS_ANALOG_TRIGGERS) {
    frame.L2 |= (frame.ANALOG_L2 > threshold);
    frame.R2 |= (frame.ANALOG_R2 > threshold);
  }
}

// Button Map only applies to Nintendo-family layouts. Other controllers stay on name mapping.
inline bool inputModeUsesButtonMapMode() {
  return buttonMapModeAppliesToInputMode(deviceMode);
}

inline bool isPositionButtonMapActive() {
  return effectivePositionButtonMap(
    deviceMode,
    get_effective_output_mode(),
    is_nso_special_active(),
    button_map_mode);
}

inline bool isPositionButtonMapActiveForXInputOutput() {
#ifdef ENABLE_INPUT_N64
  // XInput keeps the N64's named A/B buttons while C buttons use the right stick.
  if (deviceMode == RZORD_N64) {
    return false;
  }
#endif
#ifdef ENABLE_INPUT_VBOY
  // VB Auto/Stick keeps named A/B; Buttons assigns right-D-pad to ABXY and
  // physical A/B to RT/LT. Neither layout should apply a position swap.
  if (deviceMode == RZORD_VBOY) {
    return false;
  }
#endif
  return isPositionButtonMapActive();
}

inline bool isPositionButtonMapActiveForGenericHidOutput() {
  return isPositionButtonMapActive();
}

inline bool input_has_independent_digital_triggers(const controller_state_t& frame) {
  #ifdef ENABLE_INPUT_PSX
  if (deviceMode == RZORD_PSX) {
    return true;
  }
  #endif
  #ifdef ENABLE_INPUT_WII
  // Original Wii Classic pads have analog L/R plus separate digital ZL/ZR.
  // Do not synthesize ZL/ZR from the analog trigger axes.
  if (deviceMode == RZORD_WII &&
      std::strcmp(frame.controller_type_name, "Classic") == 0) {
    return true;
  }
  #endif
  return false;
}

// True on an identity whose GameControllerDB rows read L2/R2 from axes 4/5
// (DualShock 2, Saturn 3D): those axes then follow the trigger buttons at
// 0/255 whenever no analog level is sent.
inline bool hid_output_digital_trigger_axes() {
#if ADAPT_POLICY_DYNAMIC_MISTER_INPUT_IDENTITY
  return (bcd_device_version.platform == BCD_PLAT_PSX &&
          bcd_device_version.platform_sub == BCD_DUALSHOCK2_PLATFORM_SUB) ||
         (bcd_device_version.platform == BCD_PLAT_SATURN &&
          bcd_device_version.platform_sub == BCD_SATURN_3D_PLATFORM_SUB);
#elif ADAPT_POLICY_ALWAYS_REFLEX_PSX_MISTER_IDENTITY
  // Fixed products always report the DualShock 2 identity.
  return true;
#else
  return false;
#endif
}

// Under the neGcon pad identity (ReflexPSNeGcon) axes 4/5 carry Square and
// Cross for every port, so a neGcon's analog II and I read as those buttons
// in every core: its pressure where it sends one, any other press at 0/255.
inline bool hid_output_negcon_pad_axes() {
#if ADAPT_POLICY_DYNAMIC_MISTER_INPUT_IDENTITY
  return bcd_device_version.platform == BCD_PLAT_PSX &&
         bcd_device_version.platform_sub == BCD_NEGCON_PAD_PLATFORM_SUB;
#else
  return false;
#endif
}

inline bool hid_output_saturn_mission_paddle_axis(const controller_state_t& frame) {
  return deviceMode == RZORD_SATURN &&
         std::strcmp(frame.controller_type_name, "Mission") == 0;
}

inline bool hid_output_dreamcast_wheel_dpad_buttons(const controller_state_t& frame) {
#ifdef ENABLE_INPUT_DREAMCAST
  return deviceMode == RZORD_DREAMCAST &&
         (std::strncmp(frame.controller_type_name, "Whl", 3) == 0 ||
          std::strcmp(frame.controller_type_name, "Wheel") == 0);
#else
  (void)frame;
  return false;
#endif
}

inline uint32_t applyPositionButtonMapToOutputButtons(uint32_t buttons) {
  return applyNintendoPositionButtonMap(buttons, isPositionButtonMapActiveForGenericHidOutput());
}

inline uint32_t mapPs3GenericHidButtons(uint32_t buttons) {
  // Runtime PS3 mode uses the clean generic HID transport. PS3 labels generic
  // buttons in the same order as the PS3 minimal descriptor: Square, Cross,
  // Circle, Triangle, L1, R1, L2, R2, Select, Start, L3, R3, PS.
  const uint32_t mappedInputs =
    INPUT_A | INPUT_B | INPUT_X | INPUT_Y |
    INPUT_L1 | INPUT_R1 | INPUT_L2 | INPUT_R2 |
    INPUT_L3 | INPUT_R3 | INPUT_START | INPUT_SELECT |
    INPUT_HOME | INPUT_CAPTURE;
  uint32_t mapped = buttons & ~mappedInputs;

  if (buttons & INPUT_X) mapped |= (1UL << 0);
  if (buttons & INPUT_A) mapped |= (1UL << 1);
  if (buttons & INPUT_B) mapped |= (1UL << 2);
  if (buttons & INPUT_Y) mapped |= (1UL << 3);
  if (buttons & INPUT_L1) mapped |= (1UL << 4);
  if (buttons & INPUT_R1) mapped |= (1UL << 5);
  if (buttons & INPUT_L2) mapped |= (1UL << 6);
  if (buttons & INPUT_R2) mapped |= (1UL << 7);
  if (buttons & INPUT_SELECT) mapped |= (1UL << 8);
  if (buttons & INPUT_START) mapped |= (1UL << 9);
  if (buttons & INPUT_L3) mapped |= (1UL << 10);
  if (buttons & INPUT_R3) mapped |= (1UL << 11);
  if (buttons & INPUT_HOME) mapped |= (1UL << 12);
  if (buttons & INPUT_CAPTURE) mapped |= (1UL << 13);

  return mapped;
}

inline void __not_in_flash_func(map_hid_output)(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  const outputMode_t effectiveOutputMode = get_effective_output_mode();
#ifdef ENABLE_INPUT_DREAMCAST
  const bool twinStick = deviceMode == RZORD_DREAMCAST &&
                         controllerFrameIsDreamcastTwinStick(frame);
  const uint8_t twinStickDpad2Mode = twinStick
      ? dreamcastTwinStickModeFromRemaps(active_remaps) : DPAD_MODE_BUTTONS;
#else
  const bool twinStick = false;
  const uint8_t twinStickDpad2Mode = DPAD_MODE_BUTTONS;
#endif
  if ((effectiveOutputMode == OUTPUT_HID || effectiveOutputMode == OUTPUT_MISTER) &&
      dpad_mode == DPAD_MODE_DPAD &&
      !(twinStick && (twinStickDpad2Mode == DPAD_MODE_LEFT_STICK ||
                      twinStickDpad2Mode == DPAD_MODE_RIGHT_STICK)) &&
      !frame.HAS_ANALOG_STICK_MAIN &&
      !frame.HAS_ANALOG_STICK_AUX &&
      !frame.HAS_ANALOG_TRIGGERS &&
      !frame.HAS_ANALOG_DPAD &&
      !frame.HAS_ANALOG_MAIN_BUTTONS) {
    uint32_t buttons = applyPositionButtonMapToOutputButtons(
      applyZButtonOutputMode(frame.digital_buttons, frame));
#ifdef ENABLE_INPUT_DREAMCAST
    if (twinStick && twinStickDpad2Mode != DPAD_MODE_BUTTONS) {
      buttons &= ~((uint32_t)0x0F << 14);
    }
#endif
    // The hat cannot show opposed directions, so a dance pad keeps them as
    // buttons, as the analog path does for Up+Down.
    const bool opposed = (frame.PAD_U && frame.PAD_D) || (frame.PAD_L && frame.PAD_R);
    _hidgp.buttons = opposed
      ? buttons : buttons & ~(INPUT_PAD_U | INPUT_PAD_D | INPUT_PAD_L | INPUT_PAD_R);
    _hidgp.hat = directionsToHat(port);
    if (twinStick && twinStickDpad2Mode == DPAD_MODE_DPAD) {
      const DreamcastTwinStickDirections d2 = dreamcastTwinStickDpad2(frame);
      _hidgp.hat = dreamcastTwinStickHat(frame.PAD_U || d2.up,
          frame.PAD_D || d2.down, frame.PAD_L || d2.left,
          frame.PAD_R || d2.right);
    }
    _hidgp.x = 0x80;
    _hidgp.y = 0x80;
    _hidgp.z = 0x80;
    _hidgp.rx = 0x80;
    const bool triggerAxes = hid_output_digital_trigger_axes();
    if (hid_output_negcon_pad_axes()) {
      _hidgp.ry = (buttons & INPUT_X) ? 0xff : 0x00;
      _hidgp.rz = (buttons & INPUT_A) ? 0xff : 0x00;
    } else {
      _hidgp.ry = triggerAxes ? ((buttons & INPUT_L2) ? 0xff : 0x00) : 0x80;
      _hidgp.rz = triggerAxes ? ((buttons & INPUT_R2) ? 0xff : 0x00) : 0x80;
    }
    debug_hid_x[port] = _hidgp.x;
    debug_hid_y[port] = _hidgp.y;
    debug_hid_z[port] = _hidgp.z;
    debug_hid_rx[port] = _hidgp.rx;
    return;
  }

  const analog_stick_precision input_precision = frame.sticks_precision_bits;
  const analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;
  const uint8_t analog_mid = 0x80;
  bool has_analog_triggers = frame.HAS_ANALOG_TRIGGERS;
#ifdef ENABLE_INPUT_DREAMCAST
  if (deviceMode == RZORD_DREAMCAST) {
    has_analog_triggers = true;
  }
#endif
  const uint8_t frameTriggerMode = triggerModeForFrame(frame);
  const bool analog_trigger_mode =
    has_analog_triggers &&
    (frameTriggerMode == TRIGGER_MODE_ANALOG ||
     frameTriggerMode == TRIGGER_MODE_RSTICK ||
     frameTriggerMode == TRIGGER_MODE_BOTH);

  bool isPopnOrGuitarFreaks = frame.PAD_L && frame.PAD_R;
  bool dancePadDetected = frame.PAD_U && frame.PAD_D && !isPopnOrGuitarFreaks;
  const bool dreamcastWheelDpadButtons = hid_output_dreamcast_wheel_dpad_buttons(frame);

  uint8_t effective_dpad_mode = effective_dpad_mode_for_sticks(
    std::strcmp(frame.controller_type_name, "Fishing") == 0 ? uint8_t(0) : dpad_mode,
    frame.HAS_ANALOG_STICK_MAIN,
    frame.HAS_ANALOG_STICK_AUX);
  if (dancePadDetected || dreamcastWheelDpadButtons) {
    effective_dpad_mode = DPAD_MODE_BUTTONS;
  }

  uint32_t outputButtons = applyPositionButtonMapToOutputButtons(
    applyZButtonOutputMode(frame.digital_buttons, frame));
#ifdef ENABLE_INPUT_DREAMCAST
  if (twinStick && twinStickDpad2Mode != DPAD_MODE_BUTTONS) {
    outputButtons &= ~((uint32_t)0x0F << 14);
  }
#endif
  outputButtons = output_apply_n64_c_buttons_to_face_buttons(outputButtons, frame);
  // Only HID/MiSTer carry trigger axes. The generic PS3 report has none, so
  // analog-only triggers stay digital L2/R2 there in every trigger mode.
  const bool sends_trigger_axes =
    effectiveOutputMode == OUTPUT_HID || effectiveOutputMode == OUTPUT_MISTER;
  if (has_analog_triggers &&
      !input_has_independent_digital_triggers(frame) &&
      !sends_trigger_axes) {
    if (frame.ANALOG_L2 > TRIGGER_DIGITAL_THRESHOLD) outputButtons |= INPUT_L2;
    if (frame.ANALOG_R2 > TRIGGER_DIGITAL_THRESHOLD) outputButtons |= INPUT_R2;
  }

  uint32_t hidButtons = outputButtons;
  if (effective_dpad_mode == DPAD_MODE_BUTTONS) {
    _hidgp.hat = 0x8;
  } else {
    _hidgp.hat = directionsToHat(port);
    hidButtons &= ~(INPUT_PAD_U | INPUT_PAD_D | INPUT_PAD_L | INPUT_PAD_R);
  }
  if (effectiveOutputMode == OUTPUT_PS3) {
    hidButtons = mapPs3GenericHidButtons(hidButtons);
  }
  _hidgp.buttons = hidButtons;

  int8_t lx = convertAnalogPrecision(frame.LX, input_precision, output_precision);
  int8_t ly = convertAnalogPrecision(frame.LY, input_precision, output_precision);
  int8_t rx = convertAnalogPrecision(frame.RX, input_precision, output_precision);
  int8_t ry = convertAnalogPrecision(frame.RY, input_precision, output_precision);

  if (effective_dpad_mode == DPAD_MODE_LEFT_STICK || effective_dpad_mode == DPAD_MODE_RIGHT_STICK) {
    int8_t dpad_x = 0;
    int8_t dpad_y = 0;
    if (frame.PAD_L) dpad_x = -127;
    if (frame.PAD_R) dpad_x = 127;
    if (frame.PAD_U) dpad_y = -127;
    if (frame.PAD_D) dpad_y = 127;

    if (effective_dpad_mode == DPAD_MODE_LEFT_STICK) {
      lx = dpad_x;
      ly = dpad_y;
    } else {
      rx = dpad_x;
      ry = dpad_y;
    }
    _hidgp.hat = 0x8;
  }

#ifdef ENABLE_INPUT_DREAMCAST
  if (twinStick) {
    const DreamcastTwinStickDirections d2 = dreamcastTwinStickDpad2(frame);
    if (twinStickDpad2Mode == DPAD_MODE_DPAD) {
      // Combine sources only when both target the native hat. Otherwise D2
      // owns the hat and must not inherit D1's analog/button routing.
      _hidgp.hat = effective_dpad_mode == DPAD_MODE_DPAD
          ? dreamcastTwinStickHat(frame.PAD_U || d2.up,
                frame.PAD_D || d2.down, frame.PAD_L || d2.left,
                frame.PAD_R || d2.right)
          : dreamcastTwinStickHat(d2.up, d2.down, d2.left, d2.right);
    } else if (twinStickDpad2Mode == DPAD_MODE_LEFT_STICK) {
      // D-Pad 2 wins while active if both sources target the same stick;
      // otherwise D-Pad 1's current value remains in place.
      if (d2.left || d2.right) lx = dreamcastTwinStickAxis(d2.left, d2.right, 127);
      if (d2.up || d2.down) ly = dreamcastTwinStickAxis(d2.up, d2.down, 127);
    } else if (twinStickDpad2Mode == DPAD_MODE_RIGHT_STICK) {
      if (d2.left || d2.right) rx = dreamcastTwinStickAxis(d2.left, d2.right, 127);
      if (d2.up || d2.down) ry = dreamcastTwinStickAxis(d2.up, d2.down, 127);
    }
  }
#endif

  if (stick_invert & 0x01) ly = -ly;
  if (stick_invert & 0x02) ry = -ry;

  _hidgp.x = lx + analog_mid;
  _hidgp.y = ly + analog_mid;
  _hidgp.z = rx + analog_mid;
  _hidgp.rx = ry + analog_mid;
  debug_hid_x[port] = _hidgp.x;
  debug_hid_y[port] = _hidgp.y;
  debug_hid_z[port] = _hidgp.z;
  debug_hid_rx[port] = _hidgp.rx;

  _hidgp.ry = analog_mid;
  _hidgp.rz = analog_mid;
  if (sends_trigger_axes && hid_output_negcon_pad_axes()) {
    const bool negconPressure = analog_trigger_mode &&
      std::strcmp(frame.controller_type_name, "neGcon") == 0;
    const uint8_t square = (outputButtons & INPUT_X) ? 0xff : 0x00;
    const uint8_t cross = (outputButtons & INPUT_A) ? 0xff : 0x00;
    _hidgp.ry = negconPressure && frame.ANALOG_L2 > square ? frame.ANALOG_L2 : square;
    _hidgp.rz = negconPressure && frame.ANALOG_R2 > cross ? frame.ANALOG_R2 : cross;
  } else if (sends_trigger_axes) {
    if (analog_trigger_mode) {
      _hidgp.ry = frame.ANALOG_L2;
      _hidgp.rz = frame.ANALOG_R2;
    } else if (has_analog_triggers) {
      // Digital mode releases the axes, except where the rows read L2/R2
      // from them: there they follow the buttons, or L2/R2 never press.
      const bool axisTriggers = hid_output_digital_trigger_axes();
      _hidgp.ry = (axisTriggers && (outputButtons & INPUT_L2)) ? 0xff : 0x00;
      _hidgp.rz = (axisTriggers && (outputButtons & INPUT_R2)) ? 0xff : 0x00;
    } else if (!has_analog_triggers && hid_output_digital_trigger_axes()) {
      _hidgp.ry = (outputButtons & INPUT_L2) ? 0xff : 0x00;
      _hidgp.rz = (outputButtons & INPUT_R2) ? 0xff : 0x00;
    } else if (hid_output_saturn_mission_paddle_axis(frame)) {
      _hidgp.rz = frame.paddle;
    }
  }
}

inline void map_hidjogconmister_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  const analog_stick_precision input_precision = frame.sticks_precision_bits;
  const analog_stick_precision output_precision = ANALOG_STICK_PRECISION_8;
  const uint16_t analog_mid = 0x80;

  _jogcon.direction = directionsToHat(port);
  _jogcon.buttons = applyZButtonOutputMode(frame.digital_buttons, frame);
  if (frame.HOME) {
    _jogcon.buttons |= (uint16_t)(INPUT_HOME | INPUT_CAPTURE);
  }
  const bool spinnerMode = std::strcmp(frame.controller_type_name, "JogCon-S") == 0;
  const bool paddleMode = std::strcmp(frame.controller_type_name, "JogCon-P") == 0 ||
                          std::strcmp(frame.controller_type_name, "JogCon-W") == 0;
  _jogcon.spinner_axis = spinnerMode ? frame.spinner : 0;
  _jogcon.paddle_axis = paddleMode
                           ? convertAnalogPrecision(frame.LX, input_precision, output_precision) + analog_mid
                           : analog_mid;
}

inline void map_hidgunconmister_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);

  _guncon.buttons = applyZButtonOutputMode(frame.digital_buttons, frame);
  _guncon.x = frame.LX;
  _guncon.y = frame.LY;
}

inline void map_hidpaddlemister_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  _paddle = _default_paddle;
  if (!frame.connected) return;

  _paddle.buttons = applyZButtonOutputMode(frame.digital_buttons, frame);
  if (frame.HOME) {
    _paddle.buttons |= (uint16_t)(INPUT_HOME | INPUT_CAPTURE);
  }
  _paddle.direction = directionsToHat(port);
  // Volume and neGcon share a protocol; explicit output selection makes both
  // use absolute position without guessing the controller from idle buttons.
  if (std::strcmp(frame.controller_type_name, "neGcon") == 0 ||
      std::strncmp(frame.controller_type_name, "JogCon", 6) == 0 ||
      std::strncmp(frame.controller_type_name, "Vaus", 4) == 0) {
    _paddle.paddle_axis = frame.paddle;
  } else if (std::strncmp(frame.controller_type_name, "Driving", 7) == 0) {
    _paddle.paddle_axis = frame.paddle;
  }
}

inline void map_hidspinnermister_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);
  _spinner = _default_spinner;
  if (!frame.connected) return;
  _spinner.buttons = applyZButtonOutputMode(frame.digital_buttons, frame);
  if (frame.HOME) {
    _spinner.buttons |= (uint16_t)(INPUT_HOME | INPUT_CAPTURE);
  }
  _spinner.direction = directionsToHat(port);
  _spinner.spinner_axis = peekRotaryMotion(port);
}

inline void map_hidnegconmister_output(uint8_t port) {
  const controller_state_t& frame = controllerFrameConst(port);

  _negcon.direction = directionsToHat(port);
  // neGcon I/II reach these Square/Cross bits only when set to Digital; the
  // output transform clears them in Analog (negcon_input_modes.h).
#ifdef ENABLE_INPUT_PSX
  const bool negcon = frame.connected && frameIsNegcon(frame);
#else
  const bool negcon = false;
#endif
  _negcon.square_btn = negcon ? frame.X : 0;
  _negcon.cross_btn = negcon ? frame.A : 0;
  _negcon.circle_btn = frame.B;
  _negcon.triangle_btn = frame.Y;
  _negcon.l1_btn = isN64ZCombined() ? (frame.L1 || frame.L2) : frame.L1;
  _negcon.r1_btn = frame.R1;
  _negcon.l2_btn = isN64ZCombined() ? 0 : frame.L2;
  _negcon.r2_btn = frame.R2;
  _negcon.select_btn = frame.SELECT;
  _negcon.start_btn = frame.START;
  _negcon.l3_btn = frame.L3;
  _negcon.r3_btn = frame.R3;
  _negcon.ps_btn = frame.HOME;
  _negcon.guide2_btn = frame.HOME;
  _negcon.axis_twist = frame.paddle;
  _negcon.axis_paddle = _negcon.axis_twist;
  // A neGcon carries L in RX as a 0-255 pedal value (released 255). No other
  // pad has this pedal.
  _negcon.axis_l = negcon ? static_cast<uint8_t>(frame.RX) : 255;
  // MiSTer maps this identity as a wheel: Throttle -> left stick Y, Brake ->
  // right stick Y, which the PSX core's Wheel-NegCon pad reads as I and II.
  // The neGcon decoder carries I on R2 and II on L2, already released at 255.
  // Other pads and an empty port report pressure released at 0.
  _negcon.axis_i = negcon ? frame.ANALOG_R2 : static_cast<uint8_t>(~frame.ANALOG_R2);
  _negcon.axis_ii = negcon ? frame.ANALOG_L2 : static_cast<uint8_t>(~frame.ANALOG_L2);
}
