#include "button_remap.h"

#include "../product_config.h"
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
#include "../features/usb2db15/usb2db15_arcade_mapping.h"
#include "../features/usb2db15/usb2db15_action_bindings.h"
#endif

#include <string.h>

#include "controller_selection_state.h"
#include "dreamcast_twin_stick.h"
#include "eeprom_helper.h"
#include "n64_assignment_runtime.h"
#include "settings_store.h"

namespace {

#define REMAP_SLOT_A       0
#define REMAP_SLOT_B       1
#define REMAP_SLOT_X       2
#define REMAP_SLOT_Y       3
#define REMAP_SLOT_L1      4
#define REMAP_SLOT_R1      5
#define REMAP_SLOT_L2      6
#define REMAP_SLOT_R2      7
#define REMAP_SLOT_L3      8
#define REMAP_SLOT_R3      9
#define REMAP_SLOT_START  10
#define REMAP_SLOT_SELECT 11
#define REMAP_SLOT_HOME   12
#define REMAP_SLOT_CAPTURE 13
#define REMAP_SLOT_EXTRA1 14
#define REMAP_SLOT_EXTRA4 15
#define REMAP_SLOT_EXTRA0 16
#define REMAP_SLOT_EXTRA2 17
#define REMAP_SLOT_EXTRA3 18
// D-pad directions (remap_table.h); only products with ADAPT_CAP_DPAD_REMAP
// list them as remap rows.
#define REMAP_SLOT_DPAD_UP    19
#define REMAP_SLOT_DPAD_DOWN  20
#define REMAP_SLOT_DPAD_LEFT  21
#define REMAP_SLOT_DPAD_RIGHT 22
#if defined(ADAPT_CAP_DPAD_REMAP)
#define REMAP_DPAD_ROWS , REMAP_SLOT_DPAD_UP, REMAP_SLOT_DPAD_DOWN, REMAP_SLOT_DPAD_LEFT, REMAP_SLOT_DPAD_RIGHT
#else
#define REMAP_DPAD_ROWS
#endif

constexpr uint8_t kGenericRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_L3, REMAP_SLOT_R3, REMAP_SLOT_SELECT, REMAP_SLOT_START,
  REMAP_SLOT_HOME, REMAP_SLOT_CAPTURE REMAP_DPAD_ROWS
};

#ifdef ENABLE_INPUT_SMS
constexpr uint8_t kSmsRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_JPC
constexpr uint8_t kJpcRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_SELECT, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

// Intellivision and ColecoVision: side/fire buttons, then keys 1 3 5 7 9,
// Clear or *, 0, Enter or #. Keys 2/8/4/6 are the D-pad rows.
#ifdef ENABLE_INPUT_INTV
constexpr uint8_t kIntvRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L3, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_SELECT, REMAP_SLOT_R3, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_COLECOVISION
constexpr uint8_t kColecoVisionRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L3, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_SELECT, REMAP_SLOT_R3, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

// CD-i: buttons 1 and 2 (a pad's button 3 presses both).
#ifdef ENABLE_INPUT_CDI
constexpr uint8_t kCdiRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_DRIVING
constexpr uint8_t kDrivingRemapSlots[] = {
  REMAP_SLOT_A
};
#endif

#ifdef ENABLE_INPUT_NES
constexpr uint8_t kNesRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_SELECT, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_SNES
constexpr uint8_t kSnesRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_SELECT, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_VBOY
constexpr uint8_t kVirtualBoyRemapSlots[] = {
  REMAP_SLOT_X, REMAP_SLOT_L2, REMAP_SLOT_Y, REMAP_SLOT_R2,
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_L1, REMAP_SLOT_R1,
  REMAP_SLOT_START, REMAP_SLOT_SELECT REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_PSX
constexpr uint8_t kPsxRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_L3, REMAP_SLOT_R3, REMAP_SLOT_SELECT, REMAP_SLOT_START REMAP_DPAD_ROWS
};

constexpr uint8_t kPsxJogRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_START, REMAP_SLOT_SELECT REMAP_DPAD_ROWS
};

constexpr uint8_t kPsxNegconRemapSlots[] = {
  REMAP_SLOT_B, REMAP_SLOT_Y, REMAP_SLOT_A, REMAP_SLOT_X,
  REMAP_SLOT_R1, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_SATURN
constexpr uint8_t kSaturnRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_R1, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_L2, REMAP_SLOT_R2, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_MEGADRIVE
constexpr uint8_t kMegadriveRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_R1, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R2, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_PCE
constexpr uint8_t kPceRemapSlots[] = {
  REMAP_SLOT_B, REMAP_SLOT_A, REMAP_SLOT_R1, REMAP_SLOT_L1,
  REMAP_SLOT_X, REMAP_SLOT_Y, REMAP_SLOT_SELECT, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_NEOGEO
constexpr uint8_t kNeoGeoRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_SELECT, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_3DO
constexpr uint8_t k3doRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_L1,
  REMAP_SLOT_R1, REMAP_SLOT_START, REMAP_SLOT_SELECT REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_JAGUAR
constexpr uint8_t kJaguarRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_L3,
  REMAP_SLOT_EXTRA1, REMAP_SLOT_R3, REMAP_SLOT_L2, REMAP_SLOT_EXTRA4,
  REMAP_SLOT_R2, REMAP_SLOT_L1, REMAP_SLOT_Y, REMAP_SLOT_R1,
  REMAP_SLOT_EXTRA2, REMAP_SLOT_EXTRA0, REMAP_SLOT_EXTRA3,
  REMAP_SLOT_SELECT, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_N64
constexpr uint8_t kN64RemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_L2, REMAP_SLOT_L1, REMAP_SLOT_R1,
  REMAP_SLOT_X, REMAP_SLOT_L3, REMAP_SLOT_Y, REMAP_SLOT_R3, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_GAMECUBE
constexpr uint8_t kGameCubeRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L2, REMAP_SLOT_R2, REMAP_SLOT_SELECT, REMAP_SLOT_START REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_DREAMCAST
constexpr uint8_t kDreamcastRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L2, REMAP_SLOT_R2, REMAP_SLOT_START REMAP_DPAD_ROWS
};
constexpr uint8_t kDreamcastTwinStickRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L2, REMAP_SLOT_R2, REMAP_SLOT_START,
  REMAP_SLOT_EXTRA0, REMAP_SLOT_EXTRA1, REMAP_SLOT_EXTRA2,
  REMAP_SLOT_EXTRA3, REMAP_SLOT_EXTRA4 REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_WII
constexpr uint8_t kWiiRemapSlots[] = {
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_X, REMAP_SLOT_Y,
  REMAP_SLOT_L1, REMAP_SLOT_R1, REMAP_SLOT_L2, REMAP_SLOT_R2,
  REMAP_SLOT_START, REMAP_SLOT_SELECT, REMAP_SLOT_HOME REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_JVS
// Cabinet order. Each panel role keeps its contract bit (jvs_panel_layout.h):
// P1-P4 are X Y R1 L1, K1-K4 are A B R2 L2, Push9/10 are L3/R3, Coin arrives
// as Select and Service as Home. Rows are named by cabinet button.
constexpr uint8_t kJvsRemapSlots[] = {
  REMAP_SLOT_X, REMAP_SLOT_Y, REMAP_SLOT_R1, REMAP_SLOT_L1,
  REMAP_SLOT_A, REMAP_SLOT_B, REMAP_SLOT_R2, REMAP_SLOT_L2,
  REMAP_SLOT_L3, REMAP_SLOT_R3, REMAP_SLOT_START, REMAP_SLOT_SELECT,
  REMAP_SLOT_HOME REMAP_DPAD_ROWS
};
#endif

#ifdef ENABLE_INPUT_PSX
bool livePsxControllerIsNeGcon(DeviceEnum mode) {
  return mode == RZORD_PSX &&
         mode == deviceMode &&
         strcmp(selectedControllerFrame().controller_type_name, "neGcon") == 0;
}
#endif

const uint8_t* getRemapSlotTable(DeviceEnum mode, uint8_t* count) {
  *count = 0;
  switch (mode) {
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      *count = sizeof(kSmsRemapSlots) / sizeof(kSmsRemapSlots[0]);
      return kSmsRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      *count = sizeof(kJpcRemapSlots) / sizeof(kJpcRemapSlots[0]);
      return kJpcRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_INTV
    case RZORD_INTV:
      *count = sizeof(kIntvRemapSlots) / sizeof(kIntvRemapSlots[0]);
      return kIntvRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_COLECOVISION
    case RZORD_COLECOVISION:
      *count = sizeof(kColecoVisionRemapSlots) / sizeof(kColecoVisionRemapSlots[0]);
      return kColecoVisionRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_CDI
    case RZORD_CDI:
      *count = sizeof(kCdiRemapSlots) / sizeof(kCdiRemapSlots[0]);
      return kCdiRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      *count = sizeof(kDrivingRemapSlots) / sizeof(kDrivingRemapSlots[0]);
      return kDrivingRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      *count = sizeof(kNesRemapSlots) / sizeof(kNesRemapSlots[0]);
      return kNesRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      *count = sizeof(kSnesRemapSlots) / sizeof(kSnesRemapSlots[0]);
      return kSnesRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      *count = sizeof(kVirtualBoyRemapSlots) / sizeof(kVirtualBoyRemapSlots[0]);
      return kVirtualBoyRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (livePsxControllerIsNeGcon(mode)) {
        *count = sizeof(kPsxNegconRemapSlots) / sizeof(kPsxNegconRemapSlots[0]);
        return kPsxNegconRemapSlots;
      }
      *count = sizeof(kPsxRemapSlots) / sizeof(kPsxRemapSlots[0]);
      return kPsxRemapSlots;
    case RZORD_PSX_JOG:
      *count = sizeof(kPsxJogRemapSlots) / sizeof(kPsxJogRemapSlots[0]);
      return kPsxJogRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      *count = sizeof(kSaturnRemapSlots) / sizeof(kSaturnRemapSlots[0]);
      return kSaturnRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE:
      *count = sizeof(kMegadriveRemapSlots) / sizeof(kMegadriveRemapSlots[0]);
      return kMegadriveRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      *count = sizeof(kPceRemapSlots) / sizeof(kPceRemapSlots[0]);
      return kPceRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      *count = sizeof(kNeoGeoRemapSlots) / sizeof(kNeoGeoRemapSlots[0]);
      return kNeoGeoRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      *count = sizeof(k3doRemapSlots) / sizeof(k3doRemapSlots[0]);
      return k3doRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      *count = sizeof(kJaguarRemapSlots) / sizeof(kJaguarRemapSlots[0]);
      return kJaguarRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      *count = sizeof(kN64RemapSlots) / sizeof(kN64RemapSlots[0]);
      return kN64RemapSlots;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      *count = sizeof(kGameCubeRemapSlots) / sizeof(kGameCubeRemapSlots[0]);
      return kGameCubeRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      if (mode == deviceMode && selectedControllerFrame().connected &&
          controllerFrameIsDreamcastTwinStick(selectedControllerFrame())) {
        *count = sizeof(kDreamcastTwinStickRemapSlots) / sizeof(kDreamcastTwinStickRemapSlots[0]);
        return kDreamcastTwinStickRemapSlots;
      }
      *count = sizeof(kDreamcastRemapSlots) / sizeof(kDreamcastRemapSlots[0]);
      return kDreamcastRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      *count = sizeof(kWiiRemapSlots) / sizeof(kWiiRemapSlots[0]);
      return kWiiRemapSlots;
    #endif
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:
      *count = sizeof(kJvsRemapSlots) / sizeof(kJvsRemapSlots[0]);
      return kJvsRemapSlots;
    #endif
    default:
      break;
  }
  return nullptr;
}

}  // namespace

const char* const remap_button_names[REMAP_MAX_BUTTONS] = {
  "A", "B", "X", "Y", "L1", "R1", "L2", "R2",
  "L3", "R3", "Sta", "Sel", "Home", "Cap", "Ext1", "Ext4",
  "Ext0", "Ext2", "Ext3"
#if defined(ADAPT_CAP_DPAD_REMAP)
  , "Up", "Down", "Left", "Right"
#endif
};
static_assert(REMAP_MAX_BUTTONS == REMAP_RECORD_BUTTONS + REMAP_DPAD_SLOTS, "remap table width");

const char* remapDpadName(uint8_t slot) {
  return remapSlotIsDpad(slot) ? remap_button_names[slot] : nullptr;
}

uint8_t nextRemapTarget(DeviceEnum mode, outputMode_t output, uint8_t source,
                        uint8_t current, bool forward) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  // Cabinet rows step through actions, not the controller's own buttons.
  if (usb2db15ArcadeSessionReady()) {
    return usb2db15ArcadeNextBinding(source, current, forward);
  }
#endif
  if (mode == RZORD_DREAMCAST && deviceMode == mode &&
      selectedControllerFrame().connected &&
      controllerFrameIsDreamcastTwinStick(selectedControllerFrame())) {
    const uint8_t outputCount = remapOutputFamily(output) == n64_assignment::Family::DInput
        ? 19 : 14;
    const uint8_t count = outputCount + (remapSlotIsDpad(source) ? 1 : 0);
    int8_t index = forward ? -1 : 0;
    for (uint8_t i = 0; i < count; ++i) {
      const uint8_t candidate = i < outputCount ? i : source;
      if (candidate == current) { index = i; break; }
    }
    for (uint8_t step = 0; step < count; ++step) {
      index = forward ? (int8_t)((index + 1) % count)
                      : (int8_t)((index + count - 1) % count);
      const uint8_t candidate = index < outputCount ? (uint8_t)index : source;
      if (remapValueAllowed(mode, source, candidate) &&
          strcmp(remapOutputButtonName(output, candidate), "--") != 0) return candidate;
    }
    return current;
  }
  const uint8_t count = getRemapButtonCount(mode);
  if (count == 0) return current;
  uint8_t display = getRemapButtonDisplayIndex(mode, current);
  if (display == 0xFF) display = getRemapButtonDisplayIndex(mode, source);
  if (display == 0xFF) display = 0;
  for (uint8_t step = 0; step < count; ++step) {
    display = forward ? (uint8_t)((display + 1) % count) : (uint8_t)((display + count - 1) % count);
    const uint8_t candidate = getRemapButtonSlot(mode, display);
    if (candidate != 0xFF && remapValueAllowed(mode, source, candidate)) return candidate;
  }
  return current;
}

bool psxHasL3R3() {
  #ifdef ENABLE_INPUT_PSX
  if (deviceMode != RZORD_PSX) return false;
  if (MAX_USB_OUT == 0) return false;
  const char* type = selectedControllerFrame().controller_type_name;
  if (strcmp(type, "Digital") == 0 ||
      strcmp(type, "beatmania") == 0 ||
      strcmp(type, "FlightStick") == 0 ||
      strcmp(type, "neGcon") == 0 ||
      strncmp(type, "JogCon", 6) == 0 ||
      strcmp(type, "GunCon") == 0 ||
      strcmp(type, "Mouse") == 0 ||
      strcmp(type, "IR Remote") == 0) {
    return false;
  }
  return true;
  #else
  return false;
  #endif
}

uint32_t getInputButtonMask(DeviceEnum mode) {
  switch (mode) {
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_L3 |
             BTN_BIT_R3 | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L2 | BTN_BIT_R2 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
#ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (livePsxControllerIsNeGcon(mode)) {
        return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
               BTN_BIT_R1 | BTN_BIT_START;
      }
      if (psxHasL3R3()) {
        return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
               BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
               BTN_BIT_L3 | BTN_BIT_R3 | BTN_BIT_SELECT | BTN_BIT_START;
      }
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_SELECT | BTN_BIT_START;
    case RZORD_PSX_JOG:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_L3 | BTN_BIT_R3 | BTN_BIT_SELECT | BTN_BIT_START |
             BTN_BIT_EXTRA0 | BTN_BIT_EXTRA1 | BTN_BIT_EXTRA2 |
             BTN_BIT_EXTRA3 | BTN_BIT_EXTRA4;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_SELECT | BTN_BIT_START | BTN_BIT_HOME;
    #endif
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_L3 | BTN_BIT_R3 | BTN_BIT_SELECT | BTN_BIT_START | BTN_BIT_HOME;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L2 | BTN_BIT_R2 | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      return BTN_BIT_A;
    #endif
    #ifdef ENABLE_INPUT_ATARI
    case RZORD_ATARI:
      return BTN_BIT_A;
    #endif
    #ifdef ENABLE_INPUT_INTV
    case RZORD_INTV:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_L3 | BTN_BIT_R3 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_COLECOVISION
    case RZORD_COLECOVISION:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_L2 | BTN_BIT_R2 |
             BTN_BIT_L3 | BTN_BIT_R3 | BTN_BIT_SELECT | BTN_BIT_START;
    #endif
    #ifdef ENABLE_INPUT_CDI
    case RZORD_CDI:
      return BTN_BIT_A | BTN_BIT_B;
    #endif
    default:
      return BTN_BIT_A | BTN_BIT_B | BTN_BIT_X | BTN_BIT_Y |
             BTN_BIT_L1 | BTN_BIT_R1 | BTN_BIT_SELECT | BTN_BIT_START;
  }
}

uint8_t getRemapButtonCount(DeviceEnum mode) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  // Rows are controller sources; their values name cabinet actions.
  if (usb2db15ArcadeSessionReady()) {
    return usb2db15ArcadeRemapSourceCount();
  }
#endif
  uint8_t table_count = 0;
  if (getRemapSlotTable(mode, &table_count) != nullptr) {
    return table_count;
  }
  return sizeof(kGenericRemapSlots) / sizeof(kGenericRemapSlots[0]);
}

uint8_t getRemapButtonSlot(DeviceEnum mode, uint8_t display_index) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  if (usb2db15ArcadeSessionReady()) {
    return usb2db15ArcadeRemapSource(display_index);
  }
#endif
  uint8_t table_count = 0;
  const uint8_t* table = getRemapSlotTable(mode, &table_count);
  if (table != nullptr) {
    return (display_index < table_count) ? table[display_index] : 0xFF;
  }
  return (display_index < getRemapButtonCount(mode)) ? kGenericRemapSlots[display_index] : 0xFF;
}

uint8_t getRemapButtonDisplayIndex(DeviceEnum mode, uint8_t button_slot) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  if (usb2db15ArcadeSessionReady()) {
    const uint8_t count = usb2db15ArcadeRemapSourceCount();
    for (uint8_t i = 0; i < count; ++i) {
      if (usb2db15ArcadeRemapSource(i) == button_slot) return i;
    }
    return 0xFF;
  }
#endif
  uint8_t table_count = 0;
  const uint8_t* table = getRemapSlotTable(mode, &table_count);
  if (table != nullptr) {
    for (uint8_t i = 0; i < table_count; ++i) {
      if (table[i] == button_slot) {
        return i;
      }
    }
    return 0xFF;
  }
  for (uint8_t i = 0; i < getRemapButtonCount(mode); ++i) {
    if (kGenericRemapSlots[i] == button_slot) {
      return i;
    }
  }
  return 0xFF;
}

const char* getRemapButtonName(DeviceEnum mode, uint8_t index) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  if (usb2db15ArcadeSessionReady()) {
    return usb2db15ArcadeSourceName(usb2db15ArcadeRemapSource(index));
  }
#endif
#if defined(ENABLE_INPUT_INTV) || defined(ENABLE_INPUT_COLECOVISION)
  // Their D-pad rows are keypad keys 2, 8, 4 and 6.
  if (mode == RZORD_INTV || mode == RZORD_COLECOVISION) {
    const uint8_t slot = getRemapButtonSlot(mode, index);
    if (slot >= REMAP_DPAD_FIRST_SLOT && slot < REMAP_DPAD_FIRST_SLOT + 4) {
      static const char* const keys[] = {"2", "8", "4", "6"};
      return keys[slot - REMAP_DPAD_FIRST_SLOT];
    }
  }
#endif
  if (const char* direction = remapDpadName(getRemapButtonSlot(mode, index))) return direction;
  switch (mode) {
    #ifdef ENABLE_INPUT_INTV
    case RZORD_INTV: {
      static const char* const names[] = {
        "Top", "Left", "Right", "1", "3", "5", "7", "9", "Clr", "0", "Ent"};
      if (index < sizeof(names) / sizeof(names[0])) return names[index];
      break;
    }
    #endif
    #ifdef ENABLE_INPUT_COLECOVISION
    case RZORD_COLECOVISION: {
      static const char* const names[] = {
        "LFire", "RFire", "SA3", "SA4", "1", "3", "5", "7", "9", "*", "0", "#"};
      if (index < sizeof(names) / sizeof(names[0])) return names[index];
      break;
    }
    #endif
    #ifdef ENABLE_INPUT_SMS
    case RZORD_SMS:
      switch (index) {
        case 0: return "1";
        case 1: return "2";
        case 2: return "Start";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_CDI
    case RZORD_CDI:
      switch (index) {
        case 0: return "1";
        case 1: return "2";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_JPC
    case RZORD_JPC:
      switch (index) {
        case 0: return "1";
        case 1: return "2";
        case 2: return "Sel";
        case 3: return "Run";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_DRIVING
    case RZORD_DRIVING:
      switch (index) {
        case 0: return "Fire";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_NES
    case RZORD_NES:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "Sel";
        case 3: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_SNES
    case RZORD_SNES:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "X";
        case 3: return "Y";
        case 4: return "L";
        case 5: return "R";
        case 6: return "Sel";
        case 7: return "Sta";
      }
      break;
    #endif
#ifdef ENABLE_INPUT_PSX
    case RZORD_PSX:
      if (livePsxControllerIsNeGcon(mode)) {
        switch (index) {
          case 0: return "A";
          case 1: return "B";
          case 2: return "I";
          case 3: return "II";
          case 4: return "R";
          case 5: return "Sta";
        }
        break;
      }
      switch (index) {
        case 0: return "X";
        case 1: return "O";
        case 2: return "Sq";
        case 3: return "Tr";
        case 4: return "L1";
        case 5: return "R1";
        case 6: return "L2";
        case 7: return "R2";
        case 8: return "L3";
        case 9: return "R3";
        case 10: return "Sel";
        case 11: return "Sta";
      }
      break;
    case RZORD_PSX_JOG:
      switch (index) {
        case 0: return "X";
        case 1: return "O";
        case 2: return "Sq";
        case 3: return "Tr";
        case 4: return "L1";
        case 5: return "R1";
        case 6: return "L2";
        case 7: return "R2";
        case 8: return "Sta";
        case 9: return "Sel";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_SATURN
    case RZORD_SATURN:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "X";
        case 4: return "Y";
        case 5: return "Z";
        case 6: return "L";
        case 7: return "R";
        case 8: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_VBOY
    case RZORD_VBOY:
      switch (index) {
        case 0: return "R-Up";
        case 1: return "R-Down";
        case 2: return "R-Left";
        case 3: return "R-Right";
        case 4: return "A";
        case 5: return "B";
        case 6: return "L";
        case 7: return "R";
        case 8: return "Sta";
        case 9: return "Sel";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_MEGADRIVE
    case RZORD_MEGADRIVE:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "X";
        case 4: return "Y";
        case 5: return "Z";
        case 6: return "Mode";
        case 7: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_PCE
    case RZORD_PCE:
      switch (index) {
        case 0: return "I";
        case 1: return "II";
        case 2: return "III";
        case 3: return "IV";
        case 4: return "V";
        case 5: return "VI";
        case 6: return "Sel";
        case 7: return "Run";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_NEOGEO
    case RZORD_NEOGEO:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "D";
        case 4: return "Sel";
        case 5: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_3DO
    case RZORD_3DO:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "L";
        case 4: return "R";
        case 5: return "P";
        case 6: return "X";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_JAGUAR
    case RZORD_JAGUAR:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "C";
        case 3: return "1";
        case 4: return "2";
        case 5: return "3";
        case 6: return "4";
        case 7: return "5";
        case 8: return "6";
        case 9: return "7";
        case 10: return "8";
        case 11: return "9";
        case 12: return "*";
        case 13: return "0";
        case 14: return "#";
        case 15: return "Opt";
        case 16: return "Pau";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_N64
    case RZORD_N64:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "Z";
        case 3: return "L";
        case 4: return "R";
        case 5: return "C-U";
        case 6: return "C-D";
        case 7: return "C-L";
        case 8: return "C-R";
        case 9: return "Sta";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_GAMECUBE
    case RZORD_GAMECUBE:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "X";
        case 3: return "Y";
        case 4: return "L";
        case 5: return "R";
        case 6: return "Z";
        case 7: return "Sta";
        case 8: return "D-U";
        case 9: return "D-D";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_DREAMCAST
    case RZORD_DREAMCAST:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "X";
        case 3: return "Y";
        case 4: return "L";
        case 5: return "R";
        case 6: return "Sta";
        case 7: return "D2 Up";
        case 8: return "D2 Dn";
        case 9: return "D2 Lt";
        case 10: return "D2 Rt";
        case 11: return "Pause(D)";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_WII
    case RZORD_WII:
      switch (index) {
        case 0: return "A";
        case 1: return "B";
        case 2: return "X";
        case 3: return "Y";
        case 4: return "L";
        case 5: return "R";
        case 6: return "ZL";
        case 7: return "ZR";
        case 8: return "+";
        case 9: return "-";
        case 10: return "Hom";
      }
      break;
    #endif
    #ifdef ENABLE_INPUT_JVS
    case RZORD_JVS: {
      // Matches kJvsRemapSlots.
      static const char* const names[] = {
        "P1", "P2", "P3", "P4", "K1", "K2", "K3", "K4",
        "Push9", "Push10", "Start", "Coin", "Svc"};
      if (index < sizeof(names) / sizeof(names[0])) return names[index];
      break;
    }
    #endif
    default:
      break;
  }
  if (index < REMAP_MAX_BUTTONS) {
    return remap_button_names[index];
  }
  return "?";
}

const char* getRemapButtonNameForSlot(DeviceEnum mode, uint8_t button_slot) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  if (usb2db15_bindings::isCombined(button_slot)) return "Multi";
  if (usb2db15ArcadeSessionReady()) {
    const char* name = usb2db15ArcadeControlNameForSlot(button_slot);
    return name != nullptr ? name : "Off";
  }
#endif
  const uint8_t display_index = getRemapButtonDisplayIndex(mode, button_slot);
  if (display_index != 0xFF) {
    return getRemapButtonName(mode, display_index);
  }
  if (button_slot < REMAP_MAX_BUTTONS) {
    return remap_button_names[button_slot];
  }
  return "?";
}

const char* remapTargetName(DeviceEnum mode, outputMode_t output, const uint8_t* remaps,
                            uint8_t index, uint8_t target_slot, uint8_t cstick_mode,
                            bool native_n64, bool position_map) {
  // A direction that stays itself is named as the direction.
  if (const char* direction = remapDpadName(target_slot)) return direction;
  if (n64AssignmentSupported(mode, output)) {
    const uint8_t source = getRemapButtonSlot(mode, index);
    if (remapSlotIsDpad(source)) {
      return n64DpadAssignmentName(remaps[source], output, native_n64);
    }
    return n64AssignmentName(index, remaps, output, cstick_mode, native_n64);
  }
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  if (usb2db15_bindings::isCombined(target_slot)) return "Multi";
  if (usb2db15ArcadeSessionReady()) {
    const char* name = usb2db15ArcadeControlNameForSlot(target_slot);
    return name != nullptr ? name : "Off";
  }
#endif
  if (target_slot < 4 && position_map) target_slot ^= 1;
  return remapOutputButtonName(output, target_slot);
}

ButtonRemapMenu remapMenu;

uint8_t active_remaps[REMAP_MAX_BUTTONS] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
  10, 11, 12, 13, 14, 15, 16, 17, 18
#if defined(ADAPT_CAP_DPAD_REMAP)
  , 19, 20, 21, 22
#endif
};

bool hasActiveRemap() {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    if (active_remaps[i] != i) return true;
  }
  return false;
}

void clearRemapsToDefault() {
  for (uint8_t i = 0; i < REMAP_MAX_BUTTONS; i++) {
    active_remaps[i] = i;
  }
}
