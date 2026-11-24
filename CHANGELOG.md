# Classic2USB Changelog

User-facing changes to the Classic2USB firmware and its tools. Versions follow
the scheme in the README's Versioning section. The release package's
`release.md` repeats the section for its version.

## Unreleased

- Restore upstream global Hold Start/Guide behavior and Hotkeys On/Off;
  remove Twin Stick-only timing restriction. Adjustable duration remains
  on a separate branch.
- Revert experimental HKT-7500 OLED artwork pending upstream source
  publication. Existing OLED presentation is unchanged. Rebuild required.

## 1.0.2

### Inputs

- Sega Dreamcast HKT-7500 Twin Stick support with separate D-pad routing and
  remappable Pause (D). Twin Stick D-pad 2 directions map to XInput buttons
  and DInput buttons 15–18 by default.
- Hold Start retains upstream one-second Home/Guide behavior, controlled by
  global Hotkeys On/Off. No HKT-7500-specific hold shortcut.

## 1.0.1

### Inputs

- **Famicom Network Controller (HVC-051)** support.
- **PlayStation dance pads** are recognised only after Up and Down are held
  together briefly, so leverless pads no longer switch the D-pad to buttons by
  mistake.

### Outputs

- **Hold Start** for one second to send Home/Guide/OSD, as Start + Down does.
- **neGcon L** can act as the right stick X on PC DInput and XInput.
- **neGcon on DInput or MiSTer DInput:** Analog I and II now map to Cross and
  Square, and the right stick no longer reads fully right.
- **MiSTer GunCon** aims correctly in the PSX core without calibrating. Redo
  calibrations saved with earlier firmware.
- **GunCon Align** applies at once, and Trigger + A + B off-screen no longer
  changes the alignment.

### Auto

- Auto checks the host again when it starts talking after power-up: a
  sleeping PC, a console in rest mode or a booting MiSTer.
- **PC Engine mouse:** Auto no longer takes it for a Genesis pad. It stays a
  mouse while it moves, up and down are no longer swapped, and it tracks three
  times faster without stray sideways jumps.

### Menus and display

- The Factory Reset prompts fit the screen.
- The Genesis six-button picture matches the Saturn layout.

### Adapt.html

- New layout, and a theme picker: Midnight, Amber, Reflex, Green and White.
- The open page comes back after a restart or reconnect.
- The live view shows every OLED screen.

### Adapt Manager 1.1.0

- BOOTSEL recovery asks which product the board is and checks the firmware
  against it.
- A Classic2USB that answers slowly is no longer listed as unidentified.

### Fixes

- If the adapter ever stops responding, it restarts by itself instead of
  needing to be unplugged.
- Fixed a freeze under steady serial traffic.
- Turbo starts at the press, and a quick tap always registers.
- A button held while a menu opens or closes no longer sticks.

### Known issues

- On MiSTer with DInput chosen by hand, a neGcon set to Analog needs a core
  map. Auto is not affected.

## 1.0.0

Tester builds report 1.0.0 or 1.1.0, so Adapt Manager does not offer this
release to them. Testers install it through BOOTSEL, or with Reinstall or
Downgrade in Adapt.html.

First public release of Classic2USB. Tester release candidates RC2 to RC19
(September 8 to October 16, 2026) came before it; those builds also report
firmware 1.0.0, so use the build date on the OLED About page or the build ID
in Adapt.html to tell them apart. RC20 (October 25) reports 1.1.0. The
changes below were made during the tester program, from August 30 to
November 4, 2026.

### Inputs

- **Saturn 3D Control Pad** uses a separate USB identity so MiSTer can keep its
  analog-trigger mappings separate from the digital Saturn pad.
- **Intellivision (INTV) and ColecoVision (Coleco)** are new manual input modes
  for DIY adapters; wiring is in the manual. They are never auto-detected and
  are hidden from the manual mode list until enabled in System Settings ->
  Mode lists. Untested on hardware; no retail adapters exist.
- **CD-i** is a new manual input mode for Philips CD-i gamepads, joysticks,
  mice, trackballs and the Roller Controller, wired like the SNAC CDi adapter
  (wiring is in the manual). Buttons 1 and 2 are A and B; movement drives the
  D-pad and left stick. It is never auto-detected and stays hidden: unlike
  INTV and Coleco, Mode lists cannot enable it. Untested on hardware.
- **Native mice**: SNES, Mega Drive, Saturn, PlayStation, N64, 3DO, PC Engine
  and Dreamcast mice drive the new Mouse output. With Output on Auto, a PC or
  MiSTer host and only mice connected, the adapter selects Mouse. Hardware
  testing is still open.
- **Sega Master System Sports Pad** (US model, switch on Sports) is read as a
  native mouse in SMS mode: the ball moves the pointer and buttons 1 and 2 are
  left and right click. The adapter recognises it while the ball is still, so
  leave it resting for a moment after plugging it in. Untested on hardware.
- **PlayStation**
  - Auto detection reads controller replies before the clock edge and, when
    that read fails, makes one bounded hardware-SPI attempt before moving on.
    This fixed SCPH-1080 pads that worked only in manual PSX mode on the
    tester's setup.
  - A DualShock 2 is switched into pressure mode after a model query, so
    pressure-sensitive buttons reach outputs that carry them. Other pads are
    not sent the pressure command.
  - **Poll Rate** (Auto, 1000 to 60 Hz) and **Cmd Gap** (Auto, 0.25 to
    16 ms), set in Adapt.html or Adapt Manager; the Quick Menu does not show
    them.
  - On Auto timing, a first-generation DualShock now gets a 2 ms command gap
    and a 3 ms poll period after a SCPH-1200 stuck-stick report: about 330 Hz
    instead of about 1.3 kHz, roughly 1.2 ms more average input delay, and
    both ports slow while either holds a DualShock. Choosing a Cmd Gap
    restores faster polling for pads that cope with it.
  - Pop'n and GuitarFreaks controllers are recognised only from buttons held
    since the pad connected, so a DDR L+R jump or a key press no longer
    switches a pad into those layouts.
  - CrazyHit is armed per port after its button combination is held for one
    second; an auto-detected dance pad no longer overwrites saved settings.
  - neGcon I, II and L can each be **Analog** (pressure only) or **Digital**
    (press only). A Quick Menu **Gain** (1x, 1.25x, 1.5x, 2x) lets a shorter
    squeeze reach full scale.
  - Holding a beatmania controller's five keys with Start and Select for one
    second shows a beatmania picture on the OLED (the controller cannot be
    told apart from a digital pad automatically).
- **N64**: Learn now learns each corner, so worn or third-party sticks reach
  full diagonals. Several N64 buttons may be remapped to the same output.
- **GameCube**: Learn reaches full output in every direction, corners included.
- **Wii Classic / Classic Pro**: Learn reaches full output at 75 counts from
  the learned centre, so full output arrives earlier in the stick's travel
  (the tester confirmed the fix on their setup).
- **NES / SNES / Virtual Boy**: Auto needs two matching reads before it picks
  a family.
- **Genesis**: with two pads and 2P Merge, an 8BitDo M30 on port 2 offers its
  Heart and Star buttons.
- **JogCon**: the wheel no longer drifts to one side (stick centring is no
  longer applied to rotary controllers). The tester reports it fixed.
- **Rotary controllers** (Namco Volume/neGcon, JogCon, NES/Famicom Vaus, Atari
  Driving, Jaguar rotary): one **Rotary Gain** (1/16 to 16x) and **Reverse**
  setting replaces the separate speed and sensitivity rows. Rotary Gain opens
  a list of its speeds, slowest to fastest. NES/Famicom Vaus
  support is a hardware-test candidate.
- **Remapping**: every input with a D-pad lists Up, Down, Left and Right in the
  remap menu (a direction can become a button). Remaps are now stored per
  input and output pair; existing remaps move to the output in use.
- A controller unplugged while a button is held no longer leaves that button
  pressed on the host.

### Outputs

- **Switch 2 Pro** (`057E:2069`, one player) is new. Select it by hand, or set
  System Settings -> Switch2 to `Switch2 Pro` so Auto uses it on a Switch 2.
  **Preliminary, pending testing: it has not been run on a Switch 2.**
- **PS3** now uses the native DualShock 3 identity (`054C:0268`) in the
  standard firmware. Pressure for face buttons, shoulders and D-pad comes
  only from controllers that report it (a DualShock 2 in pressure mode), and
  analog L2/R2 only from controllers with analog triggers; every other
  controller sends those fully on or off. Host rumble reaches DualShock and
  DualShock 2 pads. The owner reports PS3 console output, Home and rumble
  working.
- **Switch Pro** sends a report at least every 8 ms, like a real Pro
  Controller; input changes are still sent at once. A tester's intermittent
  missed or repeated D-pad presses in NSO apps were reported fixed on RC19
  (no A/B comparison was run). The NSO identities now also report their
  controller type in their factory data.
- **XInput** (Windows) uses the Xbox 360 wireless receiver transport for both
  players; the separate wired two-player transport was removed. Analog
  triggers and stick behaviour now follow each player's own controller.
- **MiSTer Paddle** and **MiSTer Spinner** outputs for rotary controllers
  (hardware-test candidates).
- **Remote** output: a PS2 DVD remote on its IR receiver appears as a
  64-button USB device. Choose Output: Remote, or System Settings -> PS2 Remote.
  Hardware testing is still open.
- **Mouse** output: a Jaguar rotary controller, an Atari Driving controller,
  a JogCon or a neGcon/Namco Volume moves a USB mouse, one per port. Quick Menu **Mouse Axis** picks
  X (the default), Y or Wheel; Rotary Gain and Reverse apply. An analog stick moves
  it in both directions, with the **Mouse** (Left/Right Joy) and **Pointer
  Speed** settings DInput uses. On Mouse output the Quick Menu sets the
  stick, Pointer Speed, the wheel (right or left stick, the D-pad, or a
  spinner via Mouse Axis: Wheel) and, on its Buttons page, which button
  sends each click; rows a mouse cannot use (Rumble, Triggers, D-Pad,
  Button Remap) are hidden. By
  default PlayStation Square / SNES Y / neGcon II is the left click, Cross /
  B / I the middle, Circle / A / neGcon A the right, and L and R are Back
  and Forward; a right stick scrolls, or the D-pad on a pad without one.
  The Driving fire button, NES A, Master System 1 and Jaguar B stay the
  left click.
  **Preliminary, pending testing.**
- **MiSTer neGcon**: I is sent on Throttle and II on Brake, which the MiSTer
  PSX core reads correctly with Pad 1 set to Wheel-NegCon. The L pedal uses
  its full travel.
- PS4 keepalive reports now carry remaps, turbo and shortcuts while the
  controller is idle. PS4 output still needs user-provided PS4 auth keys.
- DInput, XInput and Keyboard request USB remote wakeup on a new button press
  while the PC is asleep. **Untested** on a PC.
- PS5 output is not offered on Classic2USB; a saved PS5 selection falls back
  to PS4.

### Auto host and input detection

- **Switch 2** is detected as a separate host and gets the Switch2 setting
  (default Switch Pro). Tested on one Switch 2: the detection rule fired,
  the console accepted Switch Pro output and a PlayStation pad drove the
  HOME menu. A Switch 1 comparison run is still to come.
- Detected hosts: Windows (PC Mode preference, default XInput), original Xbox,
  Xbox 360, PS3, PS4, Switch and Switch 2. Every other host, including MiSTer
  and Linux, is not detected and gets the saved MiSTer Mode preference
  (default DInput).
- With Auto input and a native NSO controller identity, hot-swapping
  controllers on a Switch no longer loops through reboots; an identity change
  costs one reset (host-tested).
- **After a firmware update** the first boot starts detection fresh and shows
  `Firmware updated to v1.0.0` for 3 seconds. Every setting is kept, including
  a manual Input or Output. Host-tested only.

### Settings

- **Power loss during a save** no longer loses settings or remaps. A save
  writes a sealed copy first, so it either finishes at the next boot or never
  started. Host-tested only.

### Menus and OLED

- **Every setting with four or more values opens a list** from its row: PC Mode,
  MiSTer Mode, Contrast, Inactive timer, Deadzone, Turbo rates, Button Remap
  targets and the rest. OK opens the list, Up/Down move, OK chooses and Back
  keeps the old value; Left/Right no longer change these rows. PC Mode and MiSTer
  Mode no longer offer PS5 or Xbox One, which this adapter cannot output.
  Preliminary, pending testing.
- The bouncing logo screensaver moves more smoothly.
- **First-boot setup guide** with a practice tour of the two adapter buttons;
  replay it from System Settings -> Setup guide.
- **System Settings -> OK/Back** chooses which face button confirms in menus
  (`X / A` default, `East` or `South`).
- **System Settings -> Mode lists** hides input and output modes you do not
  use from the manual pickers. Auto still detects every family.
- Holding the left device button for 3 seconds on Home returns Input and
  Output to Auto and keeps every other setting.
- **System Settings -> OLED**: Inactive (Screensaver or Blank), Inactive timer
  (Off, 1, 5 or 10 minutes) and a Screensaver picker.
- **System Settings -> Hotkeys** is one On/Off switch (default On) for fixed
  Start shortcuts. On Switch outputs: Start+Down Home, Start+Left NSO menu,
  Start+Right Capture, and Start+Up Select for controllers without Select
  (native NES and Genesis NSO send X instead; native N64 NSO leaves Start+Up
  unassigned). Per-shortcut editors were removed on Classic2USB.
- Long lists scroll with the selection centred. Left/Right only change values
  and never open submenus or run actions; confirmation dialogs start on the
  safe choice.
- Factory reset asks whether to keep or erase button remaps.
- The About page shows the USB polling interval of the active output.
- **System Settings -> What's new** shows a QR code for the release notes.
- Quick Menu Save keeps both a changed Input and a changed Output.

### Tools, USB drive and Adapt Manager

- A **read-only USB drive** is on by default in DInput and the MiSTer outputs
  (System Settings -> USB Drive turns it off). It holds an offline manual,
  this changelog (`Changelog.txt`), the MiSTer `.map` files with
  `install_maps.sh` and `remove_maps.sh`, an Adapt Manager installer and the
  GPL-3.0 text. The MiSTer scripts sit in `Scripts/Reflex_Adapt`, on the drive
  and on the SD card, and the installer moves an older Manager out of `Scripts`. On MiSTer the drive can change the
  numbering of other USB storage; turn it off if that happens.
- **Reflex Adapt Manager 1.0.0** for MiSTer installs Classic2USB and Adapt V1 /
  Legacy firmware and maps, adds **Remove all Classic2USB mappings**, and
  keeps the Reflex-Adapt-Legacy Downloader feed active. Flashing from a
  MiSTer is **untested**.
- **Adapt.html** offers a firmware update only to a positively identified
  Classic2USB, checks the UF2 (and its published SHA-256) before writing it,
  and saves remaps without touching rows it does not show.
- The control serial port is named `Reflex Adapt Control (MiSTer Status)`, so
  MiSTer Main can find it.

### Adapt.html

- **New layout**, with the same colours: a top bar with the connection state,
  section links, a first-run guide, and messages that appear on screen. It
  works at phone width.
- **Settings** now write only what you change, and show Saved or Not saved.
  Before, any change rewrote every setting, including ones the page was not
  showing, such as the N64 NSO device.
- **New tools**: back up and restore settings to a file, reset each tab to
  defaults, choose the real Output, a live view for either player, a pad test,
  press to assign on Remap, Rumble Stop, and OLED screenshots with direct menu
  buttons.
- **Firmware update** now has steps. The page checks GitHub for a release, you
  download the UF2 and choose it, and the page checks it against the published
  hash. Choose UF2 file also works with no internet. It can reinstall or
  downgrade after a confirmation.
- **Memory cards**: the page rereads the card before every write and refuses
  if it changed. Failed writes block further writes until the card is reloaded.
  N64 Controller Pak writes now keep the index checksum, its backup and notes
  9-16. New: Restore Card Image, Verify Against File, a Player 2 PSX card, and
  more image formats. Writing is still being tested, so use a card you can
  afford to lose. **Preliminary, pending testing.**
- Stick calibration is now saved to the adapter. Before, a power cycle lost it.
- The Wii remap rows for + and - were swapped.
- Output can now pin the mode Auto is running. Before, the page told you to
  use the adapter's Quick Menu for that.
- Jaguar Rotary rows show only when a rotary controller is detected.
- **Live view** shows the OLED preview and numeric input/output values.
  Controller illustrations are disabled for this release.

### Fixes

- USB outputs apply the selected SOCD policy to the final report, including
  reports rebuilt after remapping and hotkeys.
- **N64 2P Merge**: C-Down and C-Right now follow Player 2. The rows pointed at
  buttons an N64 pad doesn't have, so those two stayed with Player 1. Saved
  choices carry over.
- **PC Engine 2P Merge**: the I and II rows were swapped.
- OLED mirror frames arrive whole. A reply to a command could land inside a
  frame, and turning the mirror off could cut a frame short. Either one could
  stall Adapt.html's preview.
- `OLED ON` now stops the other serial streams, and a MiSTer session refuses
  it, as the documentation always said.
- The adapter no longer freezes when a PC program stops reading the control
  serial port.
- Saved settings are keyed by a fixed mode ID, so a build with a different set
  of input modes cannot move a saved input mode or its per-mode settings onto
  another mode.
- PS4 key upload writes flash once instead of once per chunk.
- Mode-hold save no longer fires on the press that opened System Settings.
- neGcon I no longer leaves the Quick Menu analog test.
- Native PS3: control-request replies match a genuine DualShock 3, and a
  shortcut's consumed buttons no longer leak pressure into the report.
- Hotkey labels for L3/R3, Quick Menu Win Mode labels, neGcon twist
  calibration, Atari paddle timing, the PC Engine controller index and the
  OLED Snake game were corrected.
- Bounds checks on Xbox 360 security, Auto feature-report and 3DO buffers.
- XInput now honours D-Pad mode and Stick Inv.
- Changing Input in Adapt.html no longer replaces Output Auto with the output
  Auto had chosen.
- 2P Merge works with only the second controller plugged in.
- MiSTer neGcon output no longer reads the pedals as pressed in menus or with
  no neGcon connected.
- Keyboard and Remote outputs now carry the report MiSTer's driver needs.
- GC Adapter output applies remaps, turbo and shortcuts on every report, and
  menus no longer reach the host.
- Auto no longer mistakes a single toggling line for an Atari Driving
  Controller, and an unused second port no longer shows a phantom controller.
- A JogCon or GunCon works on either port, and a pad beside a neGcon keeps all
  its buttons and sticks.
- GameCube and N64 sticks no longer drift to an edge at rest.
- Remaps survive a power loss during a settings save.
- Switch 2 Pro output applies remaps and turbo to a controller alone on the
  second port, and menus capture it.
- Quick Menu rows and the rumble test follow a controller alone on the second
  port.
- Quick Menu rows follow the output Auto chose, and saving no longer changes
  settings the connected controller cannot show.
- The button that closes a menu no longer reaches the host while it stays held.
- The DInput stick pointer stops while a menu is open.
- 2P Merge shows one controller on XInput.
- 2P Merge sends rumble to both controllers.
- Digital pads no longer send the D-pad as buttons as well as the hat on DInput
  and MiSTer.
- Factory reset can no longer be undone by a change still waiting to be saved.
- Changing 2P Merge over serial reports the restart it needs.
- A PlayStation pad that cannot be configured no longer freezes the other port
  when plugged in or when a game asks for rumble.
- A false Saturn or Mega Drive detection returns fully to Auto.
- A stuck Wii bus no longer stalls the adapter while it looks for a controller,
  and the Wii Guitar's stick and whammy bar reach full range.
- A 3DO pad unplugged with a button held releases it straight away.
- Keyboard output now follows MAME's keys. Select is Coin (5, or 6 for P2), Home
  is Service (9, or 0 for P2) and Capture is Test (F2).
- A save that cannot store a remap because all 52 remap slots are in use now
  says SAVE FAILED, and serial `REMAP SET` returns an error. The remap used to
  disappear at the next reboot.
- Changing MiSTer Mode from Adapt.html no longer loses an edit made just before
  it.
- Updating from firmware older than August 2026 keeps the settings of every
  input mode.
- The buzzer and menu repeat stay correct past 49 days of uptime.

### Known issues

- Switch 2 Pro output is preliminary; see above. Switch 2 detection is tested
  on one console.
- A Retro-Bit RB-SGA-001 six-button Genesis pad attached at power-on reads as
  a three-button pad; plugging it in after power-on works.
- System Settings truncates long MiSTer Mode values (`MiSTer Mode: Switch Pro`
  shows as `MiSTer Mode: Switch`).
- Intellivision, ColecoVision, MiSTer Paddle/Spinner, NES/Famicom Vaus and USB
  remote wakeup have no hardware pass yet.
- Atari paddles are not supported on the current Classic2USB hardware; the
  Atari Driving Controller is.
