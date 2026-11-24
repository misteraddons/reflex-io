# Reflex Adapt Classic2USB

Reflex Adapt Classic2USB is RP2040 firmware for connecting classic game
controllers to modern USB hosts. One firmware image contains the supported
controller inputs and USB output modes, so changing a controller or output
mode does not require a firmware reflash.

## Getting started

- **Setup and controller compatibility:** [Classic2USB setup and manual](https://docs.misteraddons.com/products/reflex-adapt/classic2usb/)
- **Firmware:** download tested UF2 files from [GitHub Releases](https://github.com/hyp36rmax/reflex-io/releases).
- **Configure:** use the adapter's OLED menu or the [Adapt browser configurator](web/Adapt.html) to select modes and remap controls. The configurator can also update firmware.
- **MiSTer:** [`reflex_adapt_manager.sh`](tools/release_assets/adapt-manager/mister/Scripts/Reflex_Adapt/reflex_adapt_manager.sh) manages firmware, rollback, and mappings.
- **Hardware:** [Reflex Adapt Classic2USB](https://misteraddons.com/collections/kits/products/reflex-adapt) and [HDMI controller adapters](https://misteraddons.com/products/hdmi-controller-adapters).

## HKT-7500 Dreamcast Twin Stick

The Dreamcast input profile identifies the Sega HKT-7500 Twin Stick by its
Maple product name and capability mask. When detected, D-pad 1 and D-pad 2 can
be configured independently, and Pause (D) can be remapped. The existing
Dreamcast A/B/X/Y, trigger, and thumb controls retain their mappings.

| Control | XInput default | DInput default |
| --- | --- | --- |
| D-pad 1 | D-pad | POV hat |
| D-pad 2 Up / Down / Left / Right | LB / RB / LS / RS | Buttons 15–18 |
| Pause (D) | Back | Button 19 |
| Start | Start | Existing Start |

Existing Classic2USB OLED presentation is retained. Dedicated Twin Stick
OLED artwork is deferred pending upstream source publication. Upstream global
Hotkeys On/Off controls the existing 1000 ms Hold Start/Guide shortcut;
adjustable duration is maintained on a separate branch.

See the [HKT-7500 Twin Stick guide](docs/hkt-7500-twin-stick.md) for detection,
routing, configuration, and validation details.

## Build from source

Install [PlatformIO Core](https://platformio.org/) and run this command from the
repository root:

```bash
pio run -e classic2usb
```

The UF2 is written to `.pio/build/classic2usb/firmware.uf2`. Build timestamps
are embedded in the firmware, so locally built files can differ byte for byte
from a release.

## Documentation

- [Classic2USB setup and manual](https://docs.misteraddons.com/products/reflex-adapt/classic2usb/)
- [Common questions](docs/FAQ.md)
- [HKT-7500 Twin Stick technical guide](docs/hkt-7500-twin-stick.md)
- [Third-party notices and project lineage](NOTICE.md)

## Project lineage and license

Classic2USB builds on the RetroZord and Reflex firmware lineage. See
[NOTICE.md](NOTICE.md) for component attribution and third-party license
locations. The firmware is GPL-3.0-or-later; see [LICENSE](LICENSE).

The original Reflex Adapt V1 firmware is maintained in
[Reflex-Adapt-Legacy](https://github.com/misteraddons/Reflex-Adapt-Legacy).
