# Reflex Adapt Classic2USB 1.0.2

This release adds support for the Sega Dreamcast HKT-7500 Twin Stick.

## What's changed

- Identifies the HKT-7500 from its Maple product name and capability mask.
- Adds separate configuration for D-pad 1 and D-pad 2. Each can route to the
  D-pad, left stick, right stick, or buttons.
- Adds a remappable Pause (D) control.
- Sets Twin Stick D-pad 2 defaults for XInput and DInput. Existing Dreamcast
  A/B/X/Y, trigger, thumb, and Start mappings are retained.
- Existing Classic2USB OLED presentation unchanged; dedicated Twin Stick
  artwork deferred pending upstream source.
- Hold Start/Guide keeps upstream global 1,000 ms behavior. Adapt Web Hotkeys
  On/Off controls fixed shortcuts for all supported controllers, not just
  Twin Stick. Adjustable duration is a separate feature branch.

## Validation

The Classic2USB firmware build and software regression checks passed. HKT-7500
identification and D-pad 2 routing were confirmed in earlier physical UAT.
Experimental OLED renderer was removed after RC1 hardware feedback. Rebuild
required to confirm existing display and upstream hotkey behavior.
