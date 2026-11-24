#include "../product_config.h"
#include "button_remap.h"
#include "controller_button_pressure.h"
#include "controller_selection_state.h"
#include "remap_pairs.h"
#include "../output/output_runtime_state.h"
#include "../input/shared/input_button_bits.h"
#include "../menu/menu_list_navigation.h"
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
#include "../features/usb2db15/usb2db15_action_bindings.h"
#include "../features/usb2db15/usb2db15_arcade_session.h"
#include "../menu/menu_runtime_state.h"
#endif

ButtonRemapMenu::ButtonRemapMenu()
  : state(REMAP_CLOSED),
    selected_index(0),
    scroll_offset(0),
    button_count(8),
    needs_redraw(true),
    edit_source_index(0),
    active_mode(RZORD_NONE) {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    temp_remap[i] = i;
  }
}

bool ButtonRemapMenu::isOpen() {
  return state != REMAP_CLOSED;
}

bool ButtonRemapMenu::isEditMode() {
  return state == REMAP_EDIT_SOURCE;
}

void ButtonRemapMenu::open(DeviceEnum mode) {
  state = REMAP_NAVIGATE;
  selected_index = 0;
  scroll_offset = 0;
  active_mode = mode;
  button_count = getRemapButtonCount(mode);
  needs_redraw = true;

  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    temp_remap[i] = active_remaps[i];
  }
}

void ButtonRemapMenu::close() {
  state = REMAP_CLOSED;
  needs_redraw = true;
}

void ButtonRemapMenu::loadRemaps(const uint8_t* data) {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    if (usb2db15_bindings::isCombined(data[i])) { temp_remap[i] = data[i]; continue; }
#endif
    temp_remap[i] = remapValueAllowed(active_mode, i, data[i]) ? data[i] : i;
  }
  needs_redraw = true;
}

void ButtonRemapMenu::getRemaps(uint8_t* data) {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    data[i] = temp_remap[i];
  }
}

void ButtonRemapMenu::clearRemaps() {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    temp_remap[i] = i;
  }
  if (active_mode == RZORD_DREAMCAST && selectedControllerFrame().connected &&
      controllerFrameIsDreamcastTwinStick(selectedControllerFrame())) {
    temp_remap[DREAMCAST_TWIN_STICK_MODE_SLOT] =
        dreamcastTwinStickModeEncoding(DPAD_MODE_BUTTONS);
    const outputMode_t output = remapPairOutputFor(configuredOutputMode);
    if (output == OUTPUT_XINPUT || output == OUTPUT_XINPUTW || output == OUTPUT_XINPUT2P) {
      temp_remap[16] = 4;
      temp_remap[14] = 5;
      temp_remap[17] = 8;
      temp_remap[18] = 9;
      temp_remap[15] = 11;
    }
  }
  needs_redraw = true;
}

void ButtonRemapMenu::navigate(bool reverse) {
  if (state == REMAP_EDIT_SOURCE) {
    if (active_mode == RZORD_DREAMCAST && deviceMode == active_mode &&
        selectedControllerFrame().connected &&
        controllerFrameIsDreamcastTwinStick(selectedControllerFrame())) {
      const uint8_t source = getRemapButtonSlot(active_mode, edit_source_index);
      if (source != 0xFF) {
        // When both D-pads share a destination, D-pad 2 is the active editor
        // target; its current assignment is advanced independently.
        temp_remap[source] = nextRemapTarget(active_mode, get_effective_output_mode(),
            source, temp_remap[source], !reverse);
        needs_redraw = true;
      }
      return;
    }
    state = REMAP_NAVIGATE;
    needs_redraw = true;
    return;
  }

  needs_redraw = true;
  uint8_t total_items = button_count + 3;
  if (reverse) selected_index = selected_index == 0 ? total_items - 1 : selected_index - 1;
  else if (++selected_index >= total_items) selected_index = 0;

  scroll_offset = menu_list_navigation::firstVisible(selected_index, button_count, LIST_ROWS);
}

uint8_t ButtonRemapMenu::select() {
  needs_redraw = true;

  if (state == REMAP_EDIT_SOURCE) {
    state = REMAP_NAVIGATE;
    return 0;
  }

  if (selected_index < button_count) {
    edit_source_index = selected_index;
    state = REMAP_EDIT_SOURCE;
    return 0;
  } else if (selected_index == button_count) {
    return 3;
  } else if (selected_index == button_count + 1) {
    // Default resets the edited remaps in place; Save or Cancel decides.
    clearRemaps();
    return 0;
  } else {
    for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
      active_remaps[i] = temp_remap[i];
    }
    return 1;
  }
}

void ButtonRemapMenu::setDestination(uint8_t button_index) {
  if (state != REMAP_EDIT_SOURCE) return;
  const uint8_t source_slot = getRemapButtonSlot(active_mode, edit_source_index);
  const bool twinStick = active_mode == RZORD_DREAMCAST && deviceMode == active_mode &&
      selectedControllerFrame().connected &&
      controllerFrameIsDreamcastTwinStick(selectedControllerFrame());
  if (!twinStick && getRemapButtonDisplayIndex(active_mode, button_index) == 0xFF) return;
  if (source_slot == 0xFF || !remapValueAllowed(active_mode, source_slot, button_index)) return;

  temp_remap[source_slot] = button_index;
  state = REMAP_NAVIGATE;
  needs_redraw = true;
}

uint8_t ButtonRemapMenu::getFirstPressedButton() {
  if (MAX_USB_OUT == 0) return 0xFF;
  const controller_state_t& r = selectedControllerFrame();

  if (r.A) return 0;
  if (r.B) return 1;
  if (r.X) return 2;
  if (r.Y) return 3;
  if (r.L1) return 4;
  if (r.R1) return 5;
  if (r.L2) return 6;
  if (r.R2) return 7;
  if (r.L3) return 8;
  if (r.R3) return 9;
  if (r.START) return 10;
  if (r.SELECT) return 11;
  if (r.HOME) return 12;
  if (r.CAPTURE) return 13;
  if (r.EXTRA & (1u << 1)) return 14;
  if (r.EXTRA & (1u << 4)) return 15;
  if (r.EXTRA & (1u << 0)) return 16;
  if (r.EXTRA & (1u << 2)) return 17;
  if (r.EXTRA & (1u << 3)) return 18;

  return 0xFF;
}

void ButtonRemapMenu::applyRemap(controller_state_t& report, const uint8_t* remap) {
  uint8_t original[REMAP_MAX_BUTTONS] = {0};
  uint8_t originalAnalog[REMAP_MAX_BUTTONS] = {0};
  original[0] = report.A;
  original[1] = report.B;
  original[2] = report.X;
  original[3] = report.Y;
  original[4] = report.L1;
  original[5] = report.R1;
  original[6] = report.L2;
  original[7] = report.R2;
  original[8] = report.L3;
  original[9] = report.R3;
  original[10] = report.START;
  original[11] = report.SELECT;
  original[12] = report.HOME;
  original[13] = report.CAPTURE;
  const bool remapJaguarExtras = deviceMode == RZORD_JAGUAR;
  const bool twinStick = deviceMode == RZORD_DREAMCAST &&
      controllerFrameIsDreamcastTwinStick(report);
  const bool remapDreamcastExtras = twinStick;
  const uint8_t twinStickDpad2Mode = twinStick
      ? dreamcastTwinStickModeFromRemaps(remap) : DPAD_MODE_BUTTONS;
  if (remapJaguarExtras) {
    original[14] = (report.EXTRA & (1u << 1)) != 0;
    original[15] = (report.EXTRA & (1u << 4)) != 0;
    original[16] = (report.EXTRA & (1u << 0)) != 0;
    original[17] = (report.EXTRA & (1u << 2)) != 0;
    original[18] = (report.EXTRA & (1u << 3)) != 0;
  }
  if (remapDreamcastExtras) {
    original[14] = (report.EXTRA & (1u << 1)) != 0;
    original[15] = (report.EXTRA & (1u << 4)) != 0;
    original[16] = (report.EXTRA & (1u << 0)) != 0;
    original[17] = (report.EXTRA & (1u << 2)) != 0;
    original[18] = (report.EXTRA & (1u << 3)) != 0;
    if (twinStick && twinStickDpad2Mode != DPAD_MODE_BUTTONS) {
      original[14] = original[16] = original[17] = original[18] = 0;
    }
  }
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  // Arcade source IDs preserve GL/GR at 14/15 and add C/Z at 16/17.
  original[14] = (report.EXTRA & (1u << 2)) != 0;
  original[15] = (report.EXTRA & (1u << 3)) != 0;
  original[16] = (report.EXTRA & (1u << 1)) != 0;
  original[17] = (report.EXTRA & (1u << 4)) != 0;
#endif
  // D-pad sources (remap_table.h): Up, Down, Left, Right.
  static constexpr uint32_t kDpadBits[] = {INPUT_PAD_U, INPUT_PAD_D, INPUT_PAD_L, INPUT_PAD_R};
  uint32_t dpadBits = 0;
  uint32_t movedDpadBits = 0;
  for (uint8_t d = 0; d < REMAP_DPAD_SLOTS && d < 4; ++d) {
    const uint8_t slot = REMAP_DPAD_FIRST_SLOT + d;
    original[slot] = (report.digital_buttons & kDpadBits[d]) != 0;
    dpadBits |= kDpadBits[d];
    if (remap[slot] != slot) movedDpadBits |= kDpadBits[d];
  }
  bool mainPressure = report.HAS_ANALOG_MAIN_BUTTONS;
  bool triggerPressure = report.HAS_ANALOG_TRIGGERS;
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; ++i) {
    originalAnalog[i] = i < 8 ? controllerButtonPressure(report, i) :
                              (original[i] ? 255 : 0);
    if (controllerButtonHasPressure(report, i)) {
      if (remap[i] < 6) mainPressure = true;
      else if (remap[i] < 8) triggerPressure = true;
    }
  }

  report.A = 0;
  report.B = 0;
  report.X = 0;
  report.Y = 0;
  report.L1 = 0;
  report.R1 = 0;
  report.L2 = 0;
  report.R2 = 0;
  report.L3 = 0;
  report.R3 = 0;
  report.START = 0;
  report.SELECT = 0;
  report.HOME = 0;
  report.CAPTURE = 0;
  report.digital_buttons &= ~dpadBits;
  // A direction that became a button no longer reports D-pad pressure.
  setControllerPressureForMask(report, movedDpadBits, 0);
  if (remapJaguarExtras) {
    report.EXTRA &= (uint16_t)~((1u << 0) | (1u << 1) | (1u << 2) |
                                (1u << 3) | (1u << 4));
  }
  if (remapDreamcastExtras) {
    if (twinStick && twinStickDpad2Mode != DPAD_MODE_BUTTONS) {
      report.EXTRA &= (uint16_t)~(1u << 4);
    } else {
      report.EXTRA &= (uint16_t)~((1u << 0) | (1u << 1) | (1u << 2) |
                                  (1u << 3) | (1u << 4));
    }
  }
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  report.EXTRA &= (uint16_t)~((1u << 1) | (1u << 2) | (1u << 3) | (1u << 4));
#endif
  report.analog_buttons = 0;
  report.HAS_ANALOG_MAIN_BUTTONS = mainPressure;
  report.HAS_ANALOG_TRIGGERS = triggerPressure;
  uint8_t remappedPressure[8] = {};
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  // Reject every source of an ambiguous cabinet action while Tournament is
  // active. Keep stored bindings intact for normal play; never pick a winner
  // based on which physical button happens to be pressed first.
  const uint32_t forbiddenOutputs = menu_kiosk_mode != 0
      ? usb2db15_bindings::duplicateCabinetOutputs(remap, REMAP_MAX_BUTTONS) : 0;
#endif

  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    if (original[i] || originalAnalog[i]) {
      uint8_t target = remap[i];
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
      if (usb2db15_bindings::isCombined(target)) {
        if (original[i]) report.digital_buttons |=
            usb2db15_bindings::combinedOutput(target, usb2db15MultiBindEnabled()) & ~forbiddenOutputs;
        continue; // Digital cabinet actions; do not synthesize analog pressure.
      }
      if (target < 32 && (forbiddenOutputs & (1u << target))) continue;
#endif
      if (original[i]) {
        switch (target) {
          case 0: report.A = 1; break;
          case 1: report.B = 1; break;
          case 2: report.X = 1; break;
          case 3: report.Y = 1; break;
          case 4: report.L1 = 1; break;
          case 5: report.R1 = 1; break;
          case 6: report.L2 = 1; break;
          case 7: report.R2 = 1; break;
          case 8: report.L3 = 1; break;
          case 9: report.R3 = 1; break;
          case 10: report.START = 1; break;
          case 11: report.SELECT = 1; break;
          case 12: report.HOME = 1; break;
          case 13: report.CAPTURE = 1; break;
          case 14:
            if (remapJaguarExtras || remapDreamcastExtras) report.EXTRA |= (1u << 1);
            break;
          case 15:
            if (remapJaguarExtras || remapDreamcastExtras) report.EXTRA |= (1u << 4);
            break;
          case 16:
            if (remapJaguarExtras || remapDreamcastExtras) report.EXTRA |= (1u << 0);
            break;
          case 17:
            if (remapJaguarExtras || remapDreamcastExtras) report.EXTRA |= (1u << 2);
            break;
          case 18:
            if (remapJaguarExtras || remapDreamcastExtras) report.EXTRA |= (1u << 3);
            break;
          default:
            // A direction left as itself (validation allows no other D-pad target).
            if (remapSlotIsDpad(target)) report.digital_buttons |= kDpadBits[target - REMAP_DPAD_FIRST_SLOT];
            break;
        }
      }
      if (originalAnalog[i]) {
        if (target < 8 && originalAnalog[i] > remappedPressure[target]) {
          remappedPressure[target] = originalAnalog[i];
        }
      }
    }
  }
  for (uint8_t i = 0; i < 8; ++i) {
    setControllerButtonPressure(report, i, remappedPressure[i]);
  }
}
