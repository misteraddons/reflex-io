#include "quick_config.h"
#include "menu_choice_list.h"
#include "menu_helpers.h"
#include "menu_list_navigation.h"
#include "../core/n64_assignment_runtime.h"
#include "../core/negcon_input_modes.h"
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
#include "../features/usb2db15/usb2db15_arcade_mapping.h"
#endif

// A setting with four or more values opens this list from its row instead
// of cycling in place; two or three values still cycle (owner, 2026-10-25).
// Each setting lists its values in display order. Confirm stages the choice,
// Back keeps the old one, and the Quick Menu Save still commits. A list
// returns to the page that opened it.

namespace {
struct RotaryGainChoice {
  uint8_t gain;  // rotary_settings.h gain code
  char label[5];
};

// Slowest to fastest. Constant, so it stays in flash.
constexpr RotaryGainChoice kRotaryGainChoices[] = {
  {16, "1/16"}, {15, "1/15"}, {14, "1/14"}, {13, "1/13"}, {12, "1/12"},
  {11, "1/11"}, {10, "1/10"}, {9, "1/9"}, {8, "1/8"}, {7, "1/7"}, {6, "1/6"},
  {5, "1/5"}, {4, "1/4"}, {3, "1/3"}, {2, "1/2"}, {1, "1x"}, {19, "3/2"},
  {17, "2x"}, {23, "3x"}, {18, "4x"}, {20, "8x"}, {21, "12x"}, {22, "16x"},
};
constexpr uint8_t kRotaryGainChoiceCount = sizeof(kRotaryGainChoices) / sizeof(kRotaryGainChoices[0]);
constexpr uint8_t kRotaryGainOneIndex = 15;

uint8_t rotaryGainChoiceIndex(uint8_t gain) {
  for (uint8_t i = 0; i < kRotaryGainChoiceCount; ++i) {
    if (kRotaryGainChoices[i].gain == gain) return i;
  }
  return kRotaryGainOneIndex;
}

constexpr const char* kPointerSpeedLabels[POINTER_MOUSE_SPEED_MAX] = {
  "1", "2", "3", "4", "5", "6", "7", "8",
  "9", "10", "11", "12", "13", "14", "15", "16",
};

// The Wheel list in display order. Default is not a choice: it shows as the
// source it resolves to, and choosing that source stores Default again.
constexpr uint8_t kMouseWheelChoices[] = {
  ROTARY_MOUSE_WHEEL_RIGHT_STICK, ROTARY_MOUSE_WHEEL_LEFT_STICK,
  ROTARY_MOUSE_WHEEL_DPAD, ROTARY_MOUSE_WHEEL_OFF,
};
constexpr const char* kMouseWheelLabels[] = {"Right Joy", "Left Joy", "D-pad", "Off"};
constexpr uint8_t kMouseWheelChoiceCount = sizeof(kMouseWheelChoices);

uint8_t mouseWheelChoiceIndex(uint8_t wheel) {
  for (uint8_t i = 0; i < kMouseWheelChoiceCount; ++i) {
    if (kMouseWheelChoices[i] == wheel) return i;
  }
  return 0;
}

#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
bool isMouseButtonItem(QuickConfigItem item) {
  return item >= QCI_MOUSE_LEFT && item <= QCI_MOUSE_BUTTON5;
}
#endif

template <typename T>
uint8_t swapStaged(T& field, uint8_t value) {
  const uint8_t previous = static_cast<uint8_t>(field);
  field = static_cast<T>(value);
  return previous;
}

// Remap rows and hotkeys keep their rules in their own cycle, so the list
// walks that cycle instead of repeating them. The walk starts one step before
// the row's first value and stops when it comes round again.
template <typename Step>
uint8_t walkChoices(uint8_t& stored, uint8_t before_first, uint8_t* values, uint8_t capacity,
                    Step step) {
  const uint8_t saved = stored;
  stored = before_first;
  step();
  const uint8_t first = stored;
  uint8_t count = 0;
  do {
    values[count++] = stored;
    step();
  } while (stored != first && count < capacity);
  stored = saved;
  return count;
}
}  // namespace

const char* QuickConfigMenu::rotaryGainLabel(uint8_t gain) {
  return kRotaryGainChoices[rotaryGainChoiceIndex(gain)].label;
}

const char* QuickConfigMenu::mouseWheelName(uint8_t wheel) {
  return kMouseWheelLabels[mouseWheelChoiceIndex(wheel)];
}

// The wheel source the staged setting means for the selected controller.
uint8_t QuickConfigMenu::tempMouseWheel(bool asDefault) {
  static const controller_state_t kNoController{};
  const controller_state_t& frame = isLiveInputSelection() ? selectedControllerFrame() : kNoController;
  return rotaryMouseResolvedWheel(asDefault ? static_cast<uint8_t>(ROTARY_MOUSE_WHEEL_DEFAULT) : temp_mouse.wheel,
                                  frame, static_cast<PointerMouseSource>(temp_pointer_source));
}

uint8_t QuickConfigMenu::mouseRoleForItem(QuickConfigItem item) {
  return static_cast<uint8_t>(item - QCI_MOUSE_LEFT);
}

// The controller button a mouse button uses: its setting, or the default for
// the selected input mode and controller.
uint8_t QuickConfigMenu::tempMouseButtonSlot(uint8_t role) {
  const bool negcon = isLiveInputSelection() &&
      rotaryInputKindForFrame(selectedControllerFrame()) == RotaryInputKind::NeGcon;
  return rotaryMouseButtonSlot(temp_mouse, getInputButtonMask(temp_input_mode),
                               rotaryMouseKeepsFirstLayout(temp_input_mode), negcon, role,
                               isLiveInputSelection() ? &selectedControllerFrame() : nullptr);
}

// Choosing the default button stores Default, so the default keeps following
// the connected controller (a neGcon and a DualShock share PSX mode).
void QuickConfigMenu::setTempMouseButtonSlot(uint8_t role, uint8_t slot) {
  if (role >= ROTARY_MOUSE_ROLE_COUNT) return;
  temp_mouse.buttons[role] = ROTARY_MOUSE_BUTTON_DEFAULT;
  if (tempMouseButtonSlot(role) == slot) return;
  temp_mouse.buttons[role] = slot == ROTARY_MOUSE_NO_SLOT
      ? ROTARY_MOUSE_BUTTON_NONE
      : static_cast<uint8_t>(ROTARY_MOUSE_BUTTON_SLOT_BASE + slot);
}

const char* QuickConfigMenu::mouseButtonName(uint8_t slot) {
  if (slot == ROTARY_MOUSE_NO_SLOT) return "None";
  if (isLiveInputSelection() && rotaryMouseNativeFrame(selectedControllerFrame())) {
    if (slot == 0) return "Left";
    if (slot == 1) return "Right";
    if (slot == 2) return "Middle";
  }
  const uint8_t index = getRemapButtonDisplayIndex(temp_input_mode, slot);
  return index < getRemapButtonCount(temp_input_mode)
      ? getRemapButtonName(temp_input_mode, index)
      : getRemapButtonNameForSlot(temp_input_mode, slot);
}

// X, Y, then the wheel. X and Y are the Axis byte that is JogCon's Wheel Axis
// in the PSX record; the wheel is a Mouse setting, so choosing it leaves that
// byte alone.
void QuickConfigMenu::cycleTempMouseAxis(bool forward) {
  const uint8_t current = temp_mouse.rotary_wheel ? 2 : (temp_jogcon_wheel_axis ? 1 : 0);
  const uint8_t next = static_cast<uint8_t>(forward ? (current + 1) % 3 : (current + 2) % 3);
  temp_mouse.rotary_wheel = next == 2 ? 1 : 0;
  if (next != 2) temp_jogcon_wheel_axis = next;
}

uint8_t QuickConfigMenu::choicePickerCount() const {
  if (editing_item == QCI_TOUCHPAD_MOUSE) return POINTER_MOUSE_SPEED_MAX;
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  if (editing_item == QCI_PADDLE_GAIN) return kRotaryGainChoiceCount;
  if (editing_item == QCI_MOUSE_WHEEL) return kMouseWheelChoiceCount;
  // None, then the input mode's buttons in Button Remap order.
  if (isMouseButtonItem(editing_item)) {
    return static_cast<uint8_t>(1 + getRemapButtonCount(temp_input_mode));
  }
#endif
  return choice_count;
}

// The remap byte the open list edits: N64 rows keep theirs by source index.
uint8_t QuickConfigMenu::remapChoiceSlot() {
  const uint8_t source = getRemapButtonSlot(temp_input_mode, remap_index);
  const auto output = temp_output_mode == OUTPUT_AUTO ? get_effective_output_mode() : temp_output_mode;
  return n64AssignmentSupported(temp_input_mode, output) && !remapSlotIsDpad(source)
      ? n64_assignment::sources[remap_index]
      : source;
}

uint8_t QuickConfigMenu::fillRemapChoices() {
  const uint8_t source = getRemapButtonSlot(temp_input_mode, remap_index);
  if (source == 0xFF) return 0;
  uint8_t* remaps = transaction.remaps();
  const auto output = temp_output_mode == OUTPUT_AUTO ? get_effective_output_mode() : temp_output_mode;
  if (n64AssignmentSupported(temp_input_mode, output)) {
    const bool nativeN64 = output == OUTPUT_SWITCHPRO && temp_nso_special != SWITCH_DEVICE_PRO;
    if (remapSlotIsDpad(source)) {
      return walkChoices(remaps[source], n64_assignment::encode(13), choice_values, kChoiceCapacity, [&] {
        cycleN64DpadAssignment(source, remaps, output, nativeN64);
      });
    }
    return walkChoices(remaps[remapChoiceSlot()], n64_assignment::off, choice_values, kChoiceCapacity, [&] {
      cycleN64Assignment(remap_index, remaps, output, temp_n64_cstick, nativeN64);
    });
  }
  uint8_t before_first = getRemapButtonSlot(temp_input_mode, getRemapButtonCount(temp_input_mode) - 1);
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  // Any value that is not an option reads as Off, the last one.
  if (usb2db15ArcadeSessionReady()) before_first = 0xFF;
#endif
  return walkChoices(remaps[source], before_first, choice_values, kChoiceCapacity, [&] {
    remaps[source] = nextRemapTarget(temp_input_mode, output, source, remaps[source], true);
  });
}

uint8_t QuickConfigMenu::fillHotkeyChoices(QuickConfigItem item) {
  uint8_t& stored = temp_direction_hotkeys[item - QCI_START_LEFT];
  const uint8_t count = walkChoices(stored, stored, choice_values, kChoiceCapacity, [&] {
    cycleHotkeyItem(item, true);
  });
  // The walk began at the stored action; the list keeps action order.
  for (uint8_t i = 1; i < count; ++i) {
    const uint8_t value = choice_values[i];
    uint8_t at = i;
    for (; at > 0 && choice_values[at - 1] > value; --at) choice_values[at] = choice_values[at - 1];
    choice_values[at] = value;
  }
  return count;
}

// The values a row offers, in list order. With values null it only counts,
// because a row's count decides whether it is a list at all.
uint8_t QuickConfigMenu::fillChoiceValues(QuickConfigItem item, uint8_t* values) {
  uint8_t count = 0;
  const auto add = [&](uint8_t value) {
    if (values && count < kChoiceCapacity) values[count] = value;
    ++count;
  };
  const auto addRange = [&](uint8_t first, uint8_t length) {
    for (uint8_t i = 0; i < length; ++i) add(static_cast<uint8_t>(first + i));
  };
  switch (item) {
    case QCI_DEADZONE: addRange(0, 7); break;
    case QCI_SOCD: addRange(0, SOCD_LAST_ENUM); break;
    case QCI_SPINNER_SPEED: addRange(0, 5); break;
    case QCI_STICK_INVERT: addRange(0, getStickInvertMax(temp_input_mode) + 1); break;
    case QCI_N64_RANGE: addRange(0, CLASSIC_ANALOG_RANGE_COUNT); break;
    case QCI_POWERPAD: addRange(0, NES_DEVICE_VAUS_FAMICOM + 1); break;
    case QCI_JOGCON_MODE:
    case QCI_JOGCON_DIGITAL: addRange(0, 4); break;
    case QCI_JOGCON_FORCE:
      for (uint8_t force = 1; force <= 15; force = force * 2 + 1) add(force);
      break;
    case QCI_NSO_SPECIAL:
      addRange(0, max_switch_device_selection_for_input_mode(temp_input_mode) + 1);
      break;
    case QCI_DPAD_BUTTONS:
      for (uint8_t mode = 0; mode < 4; ++mode) {
        if (isTempDpadModeAllowed(mode)) add(mode);
      }
      break;
    case QCI_DPAD2_MODE:
      for (uint8_t mode = 0; mode < 4; ++mode) {
        if (isTempDpad2ModeAllowed(mode)) add(mode);
      }
      break;
    case QCI_MOUSE_SOURCE:
      for (uint8_t source = 0; source < 4; ++source) {
        if (pointerSourceAvailable(source)) add(source);
      }
      break;
#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_GAIN: addRange(0, NEGCON_GAIN_COUNT); break;
#endif
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_ACTIVATION: addRange(0, 6); break;
    case QCI_USB2DB15_AUTO_SHOOT: addRange(0, 7); break;
#endif
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
    case QCI_JVS_MENU_HOLD: addRange(2, 5); break;
#endif
    case QCI_TURBO:
      for (uint8_t i = 0; i < TURBO_RATE_LAST; ++i) add(getTurboRateForMenuIndex(i));
      break;
    case QCI_REMAP:
      return fillRemapChoices();
    case QCI_START_LEFT:
    case QCI_START_RIGHT:
    case QCI_START_UP:
      return fillHotkeyChoices(item);
    default:
      break;
  }
  return count;
}

// Puts a value in the open row's staged field and returns the one it replaced.
uint8_t QuickConfigMenu::swapChoiceValue(uint8_t value) {
  switch (editing_item) {
    case QCI_DEADZONE: return swapStaged(temp_deadzone, value);
    case QCI_SOCD: return swapStaged(temp_socd, value);
    case QCI_SPINNER_SPEED: return swapStaged(temp_spinner, value);
    case QCI_STICK_INVERT: return swapStaged(temp_stick_invert, value);
    case QCI_N64_RANGE:
      return swapStaged(temp_input_mode == RZORD_N64 ? temp_n64_range : temp_wii_range, value);
    case QCI_POWERPAD: return swapStaged(temp_powerpad, value);
    case QCI_JOGCON_MODE: return swapStaged(temp_jogcon_mode, value);
    case QCI_JOGCON_DIGITAL: return swapStaged(temp_jogcon_digital, value);
    case QCI_JOGCON_FORCE: return swapStaged(temp_jogcon_force, value);
    case QCI_NSO_SPECIAL: return swapStaged(temp_nso_special, value);
    case QCI_DPAD_BUTTONS: return swapStaged(temp_dpad_mode, value);
    case QCI_DPAD2_MODE: return swapStaged(temp_dpad2_mode, value);
    case QCI_MOUSE_SOURCE: return swapStaged(temp_pointer_source, value);
#ifdef ENABLE_INPUT_PSX
    case QCI_NEGCON_GAIN: return swapStaged(temp_negcon_gain, value);
#endif
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    case QCI_USB2DB15_ACTIVATION: return swapStaged(temp_usb2db15_activation, value);
    case QCI_USB2DB15_AUTO_SHOOT: return swapStaged(temp_usb2db15_auto_shoot, value);
#endif
#if defined(ADAPT_CAP_JVS_INPUT_STATUS)
    case QCI_JVS_MENU_HOLD: return swapStaged(temp_jvs_menu_hold, value);
#endif
    case QCI_TURBO: {
      const uint8_t button = getTurboButtonIndexForVisibleIndex(turbo_index);
      return button != 0xFF ? swapStaged(temp_rates[button], value) : 0;
    }
    case QCI_REMAP: return swapStaged(transaction.remaps()[remapChoiceSlot()], value);
    case QCI_START_LEFT:
    case QCI_START_RIGHT:
    case QCI_START_UP:
      return swapStaged(temp_direction_hotkeys[editing_item - QCI_START_LEFT], value);
    default:
      return 0;
  }
}

uint8_t QuickConfigMenu::choicePickerCurrent() {
  if (editing_item == QCI_TOUCHPAD_MOUSE) {
    return temp_touchpad_mouse >= 1 && temp_touchpad_mouse <= POINTER_MOUSE_SPEED_MAX
        ? static_cast<uint8_t>(temp_touchpad_mouse - 1)
        : static_cast<uint8_t>(POINTER_MOUSE_SPEED_DEFAULT - 1);
  }
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  if (editing_item == QCI_PADDLE_GAIN) {
    return rotaryGainChoiceIndex(getTempRotaryTuning(temp_input_mode).gain);
  }
  if (editing_item == QCI_MOUSE_WHEEL) return mouseWheelChoiceIndex(tempMouseWheel(false));
  if (isMouseButtonItem(editing_item)) {
    const uint8_t slot = tempMouseButtonSlot(mouseRoleForItem(editing_item));
    if (slot == ROTARY_MOUSE_NO_SLOT) return 0;
    const uint8_t index = getRemapButtonDisplayIndex(temp_input_mode, slot);
    return index < getRemapButtonCount(temp_input_mode) ? static_cast<uint8_t>(index + 1) : 0;
  }
#endif
  const uint8_t stored = swapChoiceValue(0);
  swapChoiceValue(stored);
  for (uint8_t i = 0; i < choice_count; ++i) {
    if (choice_values[i] == stored) return i;
  }
  return 0xFF;  // Not one of the choices: no '*' in the list.
}

const char* QuickConfigMenu::choicePickerLabel(uint8_t index) {
  if (editing_item == QCI_TOUCHPAD_MOUSE) return kPointerSpeedLabels[index];
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
  if (editing_item == QCI_PADDLE_GAIN) return kRotaryGainChoices[index].label;
  if (editing_item == QCI_MOUSE_WHEEL) return kMouseWheelLabels[index];
  if (isMouseButtonItem(editing_item)) {
    return index == 0 ? "None" : getRemapButtonName(temp_input_mode, static_cast<uint8_t>(index - 1));
  }
#endif
  if (editing_item == QCI_TURBO) {
    return getTurboRateFullName(static_cast<TurboRate>(choice_values[index]));
  }
  if (editing_item == QCI_REMAP) {
    // The N64 list opens on Auto, which a row shows as the button it resolves
    // to; here that would read as the same button twice.
    const auto output = temp_output_mode == OUTPUT_AUTO ? get_effective_output_mode() : temp_output_mode;
    const uint8_t source = getRemapButtonSlot(temp_input_mode, remap_index);
    if (index == 0 && n64AssignmentSupported(temp_input_mode, output) && !remapSlotIsDpad(source)) {
      return "Auto";
    }
  }
  // A row already names its value, so each choice is staged while it is named.
  const uint8_t saved = swapChoiceValue(choice_values[index]);
  const char* label = editing_item == QCI_REMAP ? remapRowTarget(remap_index)
                                                : getItemValue(editing_item);
  swapChoiceValue(saved);
  return label;
}

void QuickConfigMenu::openChoicePicker(QuickConfigItem item) {
  editing_item = item;
  choice_count = fillChoiceValues(item, choice_values);
  // A row with nothing to choose stays shut: N64 C buttons on the stick.
  if (choicePickerCount() < 2) return;
  const uint8_t current = choicePickerCurrent();
  choice_picker_index = current < choicePickerCount() ? current : 0;
  choice_return_state = state;
  state = QC_CHOICE_PICKER;
  needs_redraw = true;
}

void QuickConfigMenu::confirmChoicePicker() {
  const uint8_t index = choice_picker_index;
  if (index < choicePickerCount()) {
    if (editing_item == QCI_TOUCHPAD_MOUSE) {
      temp_touchpad_mouse = static_cast<uint8_t>(index + 1);
    }
#if defined(ADAPT_BRIDGE_CLASSIC_TO_USB)
    if (editing_item == QCI_PADDLE_GAIN) {
      setTempRotaryGain(kRotaryGainChoices[index].gain);
    } else if (editing_item == QCI_MOUSE_WHEEL) {
      const uint8_t wheel = kMouseWheelChoices[index];
      temp_mouse.wheel = wheel == tempMouseWheel(true)
          ? static_cast<uint8_t>(ROTARY_MOUSE_WHEEL_DEFAULT) : wheel;
    } else if (isMouseButtonItem(editing_item)) {
      setTempMouseButtonSlot(mouseRoleForItem(editing_item),
                             index == 0 ? ROTARY_MOUSE_NO_SLOT
                                        : getRemapButtonSlot(temp_input_mode, static_cast<uint8_t>(index - 1)));
    }
#endif
    if (index < choice_count) swapChoiceValue(choice_values[index]);
  }
  state = choice_return_state;
  // These two change which rows exist.
  if (editing_item == QCI_JOGCON_MODE || editing_item == QCI_POWERPAD) {
    rebuildVisibleAndClampSelection();
  }
  needs_redraw = true;
}

void QuickConfigMenu::printChoicePickerTitle(SSD1306AsciiWire& target) {
  if (editing_item == QCI_TURBO) {
    char title[32];
    snprintf(title, sizeof(title), "%s Turbo", getTurboButtonNameForVisibleIndex(turbo_index));
    printCenteredMenuTitle(target, title);
  } else if (editing_item == QCI_REMAP) {
    printCenteredMenuTitle(target, getRemapButtonName(temp_input_mode, remap_index));
  } else {
    printCenteredMenuTitle(target, getItemName(editing_item));
  }
}

void QuickConfigMenu::renderChoicePicker(SSD1306AsciiWire& target) {
  printChoicePickerTitle(target);
  const uint8_t count = choicePickerCount();
  const uint8_t current = choicePickerCurrent();
  const uint8_t first = menu_list_navigation::firstVisible(choice_picker_index, count, 6);
  for (uint8_t row = 0; row < 6 && first + row < count; ++row) {
    const uint8_t i = first + row;
    printMenuChoiceRow(target, row + 1, i == choice_picker_index, i == current, choicePickerLabel(i));
  }
  target.setCursor(0, 7);
  printMenuButton(target, "Back", choice_picker_index == count);
}
