# Reflex Adapt Classic2USB FAQ

## Do I need to reflash when I change controllers or USB output modes?

No. The Classic2USB firmware image contains its supported controller inputs and
USB output modes. Select the input and output modes in the adapter menu or
Adapt browser configurator.

## How do I configure or remap a controller?

Use Quick Config and Button Remap in the OLED menu, or open
[Adapt.html](../web/Adapt.html) in a compatible browser. The
[Classic2USB manual](https://docs.misteraddons.com/products/reflex-adapt/classic2usb/)
covers adapter setup and the full control layout.

## Does the firmware identify the Sega HKT-7500 Twin Stick?

Yes. The Dreamcast profile requires the Twin Stick Maple product name and the
matching capability mask. This keeps other Dreamcast devices, including the
Mission Stick, on their existing profiles. See the
[HKT-7500 Twin Stick guide](hkt-7500-twin-stick.md) for mappings and validation.

## What does holding Start do on the HKT-7500?

Hold Start follows upstream global Hotkeys On/Off. When enabled, holding Start
alone for 1000 ms sends Home/Guide on supported controllers, including HKT-7500.
Hotkeys Off disables fixed Start shortcuts for every supported controller,
not only Twin Stick. Short Start presses remain normal Start. Adjustable
global Hold Start timing is developed on `feature/guide-hold-duration`.

## Where can I get firmware or build it?

Download tested UF2 files from [GitHub Releases](https://github.com/misteraddons/reflex-io/releases).
To build Classic2USB firmware from source, install PlatformIO Core and run
`pio run -e classic2usb` from the repository root. The UF2 is written to
`.pio/build/classic2usb/firmware.uf2`.
