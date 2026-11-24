#include "../product_config.h"

#include "menu_bridge.h"
#include "menu_bridge_internal.h"

#include "../core/button_remap.h"
#include "../core/controller_selection_state.h"
#include "../core/controller_settings_state.h"
#include "../core/settings_store.h"
#include "../platform/buzzer.h"
#include "../platform/display_runtime_state.h"

#include "mapping_display.h"
#include "menu_analog_test.h"
#include "menu_jvs_analog.h"
#include "menu_jvs_panel.h"
#include "menu_jvs_status.h"
#include "menu_helpers.h"
#include "menu_pad_test.h"
#include "menu_pin_debug.h"
#include "menu_runtime_state.h"
#include "menu_ui_state.h"

namespace {

void redrawMenuAfterOverlayExit(uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right) {
  display.clear();
  renderMenu(selected_visible, scroll_offset, bottom_right);
}

void renderButtonRemap(bool btnChangeJustPressed, bool btnNavigateJustPressed,
                       bool navigatePrevious = false) {
  if (btnNavigateJustPressed) {
    remapMenu.navigate(navigatePrevious);
    buzzer.playMenuNav();
  }

  if (btnChangeJustPressed) {
    if (remapMenu.isEditMode()) {
      const bool twinStick = deviceMode == RZORD_DREAMCAST &&
          selectedControllerFrame().connected &&
          controllerFrameIsDreamcastTwinStick(selectedControllerFrame());
      if (twinStick) {
        remapMenu.select();
        buzzer.playMenuNav();
      } else {
        uint8_t pressed = remapMenu.getFirstPressedButton();
        if (pressed != 0xFF) {
          remapMenu.setDestination(pressed);
          buzzer.playMenuNav();
        }
      }
    } else {
      uint8_t result = remapMenu.select();
      if (result == 1) {
        if (saveButtonRemaps()) {
          buzzer.playSave();
        } else {
          buzzer.playError();
          showMenuActionFeedback(MenuActionFeedback::SaveFailed);
        }
        buttonRemapActive = false;
        remapMenu.close();
        return;
      } else if (result == 3) {
        loadRemapsFromEEPROM();
        buzzer.playMenuNav();
        buttonRemapActive = false;
        remapMenu.close();
        return;
      } else {
        buzzer.playMenuNav();
      }
    }
  }

  if (remapMenu.isEditMode()) {
    const bool twinStick = deviceMode == RZORD_DREAMCAST &&
        selectedControllerFrame().connected &&
        controllerFrameIsDreamcastTwinStick(selectedControllerFrame());
    if (!twinStick) {
      uint8_t pressed = remapMenu.getFirstPressedButton();
      if (pressed != 0xFF) {
        remapMenu.setDestination(pressed);
        buzzer.playMenuNav();
      }
    }
  }

  remapMenu.render(deviceMode);
}

}  // namespace

bool handleMenuToolScreens(bool btnChangeJustPressed, bool btnNavigateJustPressed,
                           const MenuControllerInput& ctrl,
                           uint8_t selected_visible, uint8_t scroll_offset, bool bottom_right) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
  // System-menu physical buttons are injected as controller edges.  Tool
  // overlays predate that path and accept only the two legacy booleans, so
  // translate USB2DB15 Select/Back and Up/Down locally.  Gameplay is never
  // involved: this runs only while a menu overlay owns controller input.
  const bool productChange = btnChangeJustPressed || ctrl.aJust || ctrl.bJust;
  const bool productNavigate = btnNavigateJustPressed || ctrl.upJust || ctrl.downJust;
  const bool productNavigatePrevious = ctrl.upJust && !ctrl.downJust;
#else
  const bool productChange = btnChangeJustPressed;
  const bool productNavigate = btnNavigateJustPressed;
  const bool productNavigatePrevious = false;
#endif
  if (about_screen_active) {
    if (productChange || productNavigate || ctrl.aJust || ctrl.bJust) {
      about_screen_active = false;
      buzzer.playMenuNav();
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (whats_new_screen_active) {
    if (productChange || productNavigate || ctrl.aJust || ctrl.bJust) {
      whats_new_screen_active = false;
      buzzer.playMenuNav();
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (buttonRemapActive) {
#if defined(ADAPT_CAP_ARCADE_MAPPING_SESSION)
    if (ctrl.bJust) {
      loadRemapsFromEEPROM(); // Back discards the draft before leaving.
      buttonRemapActive = false;
      remapMenu.close();
    } else
#endif
    renderButtonRemap(productChange, productNavigate, productNavigatePrevious);
    if (!buttonRemapActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (padTestActive) {
    renderPadTest(productChange);
    if (!padTestActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (pinDebugActive) {
    renderPinDebug(productChange, productNavigate);
    if (!pinDebugActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (analogTestActive) {
#ifdef PRODUCT_JVSIO2USB
    renderJvsAnalogMenu(productChange, productNavigate, ctrl);
#else
    renderAnalogTest(productChange, productNavigate);
#endif
    if (!analogTestActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  if (jvsStatusActive) {
    renderJvsStatus(productChange, productNavigate, ctrl);
    if (!jvsStatusActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

#if defined(ADAPT_BRIDGE_JVS_TO_USB)
  if (jvsPanelActive) {
    renderJvsPanelMenu(productChange, productNavigate, ctrl);
    if (!jvsPanelActive) {
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }
#endif

  if (mappingDisplayActive) {
    renderMappingDisplay();

    if (productChange) {
      mappingDisplayActive = false;
      deactivateMappingDisplay();
      buzzer.playMenuNav();
      redrawMenuAfterOverlayExit(selected_visible, scroll_offset, bottom_right);
    }
    return true;
  }

  return false;
}
