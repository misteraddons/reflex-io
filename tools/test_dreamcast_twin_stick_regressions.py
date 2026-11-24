#!/usr/bin/env python3
"""Source regression checks for the Dreamcast HKT-7500 remap and routing paths."""

from __future__ import annotations

import re
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def digital_axis(negative: bool, positive: bool, full_scale: int) -> int:
    if negative == positive:
        return 0
    return -full_scale if negative else full_scale


def shared_axis(d1: int, d2_negative: bool, d2_positive: bool) -> int:
    if d2_negative or d2_positive:
        return digital_axis(d2_negative, d2_positive, 127)
    return d1


def hat(up: bool, down: bool, left: bool, right: bool) -> int:
    if up and down:
        up = down = False
    if left and right:
        left = right = False
    if up:
        return 1 if right else 7 if left else 0
    if down:
        return 3 if right else 5 if left else 4
    if right:
        return 2
    if left:
        return 6
    return 8


class TwinStickRemapTests(unittest.TestCase):
    def test_output_destinations_are_not_limited_to_physical_sources(self) -> None:
        source = read("firmware/core/button_remap.cpp")
        signature = re.search(
            r"uint8_t nextRemapTarget\(DeviceEnum mode, outputMode_t output,"
            r".*?\n}\n\nbool psxHasL3R3", source, re.DOTALL)
        self.assertIsNotNone(signature)
        body = signature.group(0)
        self.assertIn("? 19 : 14", body)
        self.assertIn("remapOutputButtonName(output, candidate)", body)
        self.assertIn("remapOutputFamily(output)", body)
        names = read("firmware/core/n64_button_assignment.h")
        xbox_names = re.search(r"xbox\[\]\s*=\s*\{([^}]+)\}", names)
        self.assertIsNotNone(xbox_names)
        destinations = [name.strip().strip('"') for name in xbox_names.group(1).split(",")]
        for name in ("LB", "RB", "LS", "RS", "Back"):
            self.assertIn(name, destinations)

        menu = read("firmware/core/button_remap_menu.cpp")
        self.assertIn("nextRemapTarget(active_mode, get_effective_output_mode()", menu)
        self.assertIn("if (!twinStick && getRemapButtonDisplayIndex", menu)
        bridge = read("firmware/menu/menu_bridge_tools.cpp")
        self.assertGreaterEqual(bridge.count("if (!twinStick)"), 1)
        self.assertIn("if (twinStick) {\n        remapMenu.select();", bridge)

    def test_hid_hat_routing_keeps_sources_independent_and_neutralizes(self) -> None:
        source = read("firmware/output/usb/output_hid_mapping_runtime.h")
        self.assertIn("effective_dpad_mode == DPAD_MODE_DPAD", source)
        self.assertIn("dreamcastTwinStickHat(d2.up, d2.down, d2.left, d2.right)", source)
        d2_routes = source[source.index("if (twinStickDpad2Mode == DPAD_MODE_DPAD)",
                                       source.index("if (twinStick)")):]
        d2_routes = d2_routes[:d2_routes.index("#endif")]
        self.assertNotIn("_hidgp.hat = 0x8", d2_routes)
        xinput = read("firmware/output/xinput/output_xinput_mapping_runtime.h")
        self.assertIn("const bool includeDpad1 = effective_dpad_mode == DPAD_MODE_DPAD", xinput)
        self.assertIn("const uint8_t hat = dreamcastTwinStickHat(up, down, left, right)", xinput)

        # Both sources target the hat: all eight diagonals and deterministic
        # cancellation of opposed inputs. Releasing both returns neutral.
        self.assertEqual(hat(True, False, False, True), 1)
        self.assertEqual(hat(True, False, True, False), 7)
        self.assertEqual(hat(False, False, False, False), 8)
        self.assertEqual(hat(True, True, False, False), 8)
        # D1 analog + D2 POV: only the D2 source is supplied to the hat helper.
        self.assertEqual(hat(False, False, True, False), 6)

    def test_defaults_only_initialize_unmarked_pairs_and_reset_explicitly(self) -> None:
        source = read("firmware/core/remap_pairs.cpp")
        defaults = source[source.index("bool setDreamcastTwinStickDefaults"):
                          source.index("#if !ADAPT_REMAPS_PER_OUTPUT")]
        self.assertIn("if (!uninitialized) return false;", defaults)
        self.assertLess(defaults.index("if (!uninitialized) return false;"),
                        defaults.index("remaps[16] = 4"))
        self.assertIn("remaps[14] = 5", defaults)
        self.assertIn("remaps[17] = 8", defaults)
        self.assertIn("remaps[18] = 9", defaults)
        self.assertIn("remaps[15] = 11", defaults)

        reset = read("firmware/core/button_remap_menu.cpp")
        reset = reset[reset.index("void ButtonRemapMenu::clearRemaps"):
                      reset.index("void ButtonRemapMenu::navigate")]
        self.assertIn("dreamcastTwinStickModeEncoding(DPAD_MODE_BUTTONS)", reset)
        for assignment in ("temp_remap[16] = 4", "temp_remap[14] = 5",
                           "temp_remap[17] = 8", "temp_remap[18] = 9",
                           "temp_remap[15] = 11"):
            self.assertIn(assignment, reset)

    def test_staged_mode_reloads_per_output_without_sharing_pairs(self) -> None:
        state = read("firmware/menu/quick_config_state.cpp")
        restage = state[state.index("void QuickConfigMenu::restageRemapsForOutput"):
                        state.index("bool QuickConfigMenu::saveStagedRemaps")]
        self.assertIn("readRemapsForPair(temp_input_mode, temp_output_mode, remaps)", restage)
        self.assertIn("transaction.resetRemaps(remaps)", restage)
        self.assertIn("temp_dpad2_mode = dreamcastTwinStickModeFromRemaps(remaps)", restage)
        pairs = read("firmware/core/remap_pairs.cpp")
        self.assertIn("remapPairRead(input, remapPairOutputFor(selected), remaps)", pairs)
        self.assertIn("remapPairWrite(input, remapPairOutputFor(selected), sanitized)", pairs)

    def test_twin_stick_dpad2_buttons_mode_is_available_on_xinput(self) -> None:
        capabilities = read("firmware/menu/quick_config_capabilities.cpp")
        dpad2_allowed = capabilities[
            capabilities.index("bool QuickConfigMenu::isTempDpad2ModeAllowed"):
            capabilities.index("void QuickConfigMenu::cycleTempDpadMode")
        ]
        self.assertIn("mode == DPAD_MODE_BUTTONS", dpad2_allowed)
        self.assertIn("temp_input_mode == RZORD_DREAMCAST", dpad2_allowed)
        self.assertIn("controllerFrameIsDreamcastTwinStick(frame)", dpad2_allowed)
        self.assertIn("return true;", dpad2_allowed)

        choices = read("firmware/menu/quick_config_choice_picker.cpp")
        dpad2_choices = choices[choices.index("case QCI_DPAD2_MODE:"):]
        self.assertIn("if (isTempDpad2ModeAllowed(mode)) add(mode);", dpad2_choices)
        self.assertIn("if (isTempDpadModeAllowed(mode)) add(mode);", choices)
        actions = read("firmware/menu/quick_config_capabilities.cpp")
        dpad2_cycle = actions[actions.index("void QuickConfigMenu::cycleTempDpad2Mode"):
                              actions.index("uint8_t QuickConfigMenu::getStickInvertMax")]
        self.assertIn("isTempDpad2ModeAllowed(value)", dpad2_cycle)

    def test_xinput_report_fields_release_and_dinput_button_numbering(self) -> None:
        remap = read("firmware/core/button_remap_menu.cpp")
        self.assertIn("original[16] = (report.EXTRA & (1u << 0)) != 0", remap)
        self.assertIn("original[14] = (report.EXTRA & (1u << 1)) != 0", remap)
        self.assertIn("original[17] = (report.EXTRA & (1u << 2)) != 0", remap)
        self.assertIn("original[18] = (report.EXTRA & (1u << 3)) != 0", remap)
        self.assertIn("original[15] = (report.EXTRA & (1u << 4)) != 0", remap)

        xinput = read("firmware/output/xinput/output_xinput_mapping_runtime.h")
        for line in ("report.lb =", "report.rb = frame.R1", "report.ls =",
                     "report.rs =", "report.back =", "report.start = frame.START"):
            self.assertIn(line, xinput)
        self.assertIn("report.lb = n64XinputLayout ? frame.L1", xinput)
        self.assertIn("report.back = n64XinputLayout ? frame.L2 : frame.SELECT", xinput)

        names = read("firmware/core/n64_assignment_runtime.h")
        self.assertIn('{"B16", "B19", "B15", "B17", "B18"}', names)

    def test_direction_arithmetic_and_standard_dreamcast_fallback(self) -> None:
        stick = read("firmware/core/dreamcast_twin_stick.h")
        self.assertIn("if (negative == positive) return 0;", stick)
        self.assertIn("return negative ? -fullScale : fullScale;", stick)
        self.assertEqual(digital_axis(False, False, 127), 0)
        self.assertEqual(digital_axis(True, True, 127), 0)
        self.assertEqual(digital_axis(True, False, 127), -127)
        self.assertEqual(digital_axis(False, True, 127), 127)
        # Same-stick conflict: active D2 takes priority, then D1 resumes;
        # releasing both yields center.
        self.assertEqual(shared_axis(-127, False, True), 127)
        self.assertEqual(shared_axis(-127, False, False), -127)
        self.assertEqual(shared_axis(0, False, False), 0)

        source = read("firmware/core/button_remap.cpp")
        self.assertIn("controllerFrameIsDreamcastTwinStick(selectedControllerFrame())", source)
        self.assertIn("*count = sizeof(kDreamcastRemapSlots)", source)
        detection = read("firmware/input/dreamcast/input_dreamcast_debug.cpp")
        twin_stick_detection = detection[
            detection.index("bool dreamcastDeviceIsTwinStickController"):
            detection.index("\n}\n}  // namespace", detection.index("bool dreamcastDeviceIsTwinStickController"))
        ]
        self.assertIn('dreamcastDeviceHasProductName(info, "Twin-Stick")', twin_stick_detection)
        self.assertIn('dreamcastDeviceHasProductName(info, "Twin Stick")', twin_stick_detection)
        self.assertIn("info.func_data[0] == kDreamcastTwinStickCapabilityMask", twin_stick_detection)
        self.assertIn("0x0000FEFE", detection)

    def test_hold_start_remains_upstream_global(self) -> None:
        hotkeys = read("firmware/core/controller_runtime_hotkeys.cpp")
        self.assertIn("kHoldStartHomeMs = 1000", hotkeys)
        self.assertIn("const bool holdStartEnabled = holdStartOn;", hotkeys)
        self.assertNotIn("holdStartOn && isTwinStick", hotkeys)
        self.assertNotIn("if (isTwinStick) {", hotkeys)
        self.assertNotIn("dreamcast_twin_stick.h", hotkeys)

    def test_mixed_port_detection_and_routing_are_frame_scoped(self) -> None:
        pairs = read("firmware/core/remap_pairs.cpp")
        self.assertIn("controllerFramesContainDreamcastTwinStick(controller_frames, inputPortCount())", pairs)
        self.assertIn("dreamcastTwinStickConnected()", pairs)

        remap = read("firmware/core/button_remap_menu.cpp")
        self.assertIn("controllerFrameIsDreamcastTwinStick(report)", remap)
        xinput = read("firmware/output/xinput/output_xinput_mapping_runtime.h")
        self.assertIn("controllerFrameIsDreamcastTwinStick(frame)", xinput)
        hid = read("firmware/output/usb/output_hid_mapping_runtime.h")
        self.assertIn("controllerFrameIsDreamcastTwinStick(frame)", hid)


if __name__ == "__main__":
    unittest.main(verbosity=2)
