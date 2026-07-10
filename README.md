# Clawdmeter — Attaky Modular Device edition

A Claude Code usage dashboard for **Attaky Modular Device** hardware.
It pairs with your computer over Bluetooth; the splash screen plays
pixel-art Clawd animations that get busier as your usage rate climbs,
and the physical keys double as a BLE HID keyboard for Claude Code
shortcuts.

|              Splash               |              Usage              |                Bluetooth                |
| :-------------------------------: | :-----------------------------: | :-------------------------------------: |
| ![Splash](screenshots/splash.png) | ![Usage](screenshots/usage.png) | ![Bluetooth](screenshots/bluetooth.png) |
|   Splash; touch-toggle anytime    | Session and weekly utilization  |    Connection status and bond reset     |

## Screens

The device boots into the splash and stays there until you press the
**BOOT** button, which cycles between Usage and Bluetooth. Tap the
screen anywhere (except the Reset zone on the Bluetooth screen) to flip
back to the splash; tap again to dismiss it.

While the splash is up, the BOOT button cycles animations instead of
screens. The firmware also auto-rotates every 20 s within the current
usage-rate group, so a long stretch on the splash isn't just one Clawd
on loop.

## Hardware

- **Attaky Core 1.0** — 320 × 240 landscape touchscreen, physical
  buttons, RGB indicator, and a motion sensor (present but unused by
  this firmware). Connects to the host over USB-C for flashing and
  serial.
- **Attaky Power_Standard-Cell_1.0** — Li-Po battery with on-board
  fuel gauge that reports battery percentage to the firmware.
- USB-C cable for flashing firmware and charging.

The hardware POWER button is a long-press power toggle handled at
the board level; firmware doesn't see it.

## Prerequisites

- Linux (tested on Ubuntu) or macOS
- [PlatformIO CLI](https://docs.platformio.org/en/latest/core/installation/index.html)
- A CH340 USB-serial driver (most current macOS/Linux ship one)
- Linux: `curl`, `bluetoothctl`, `busctl` (BlueZ Bluetooth stack)
- macOS: `python3` (the installer sets up a venv with `bleak` and `httpx`)
- Claude Code with an active subscription

## macOS installation

The macOS host pieces — Python daemon, LaunchAgent, and flash helper —
were originally ported by [Chris Davidson (@lorddavidson)](https://github.com/lorddavidson).
Thanks Chris!

### Flash the firmware

The Core's CH340X enumerates as `/dev/cu.usbserial-*` (not
`/dev/cu.usbmodem*`):

```bash
./flash-mac.sh                          # auto-detects /dev/cu.usbserial-*
./flash-mac.sh /dev/cu.usbserial-2120   # or pass an explicit port
```

Flashing runs at 460800 baud (the CH340X is not reliable at 921600).

### Pair the device

After flashing, open **System Settings → Bluetooth** and click
*Connect* next to **"Attaky Claude Monitor"**. The daemon discovers it on
its next scan (~30 s).

If you ever tap **Reset Bluetooth** on the device, it drops its bond;
macOS will then refuse to reconnect with *"Peer removed pairing
information"*. That is expected — *Forget* "Attaky Claude Monitor" in
System Settings → Bluetooth, then connect again.

### Install the daemon

The daemon reads your Claude OAuth token from the macOS Keychain
(service `Claude Code-credentials`), polls usage every 60 s, and pushes
it to the display over BLE.

```bash
./install-mac.sh
```

The installer creates a Python venv in `daemon/.venv/`, installs
`bleak` and `httpx`, renders a LaunchAgent into
`~/Library/LaunchAgents/com.user.claude-usage-daemon.plist`, and loads
it. The first run is launched interactively so macOS prompts for
Bluetooth permission.

Useful commands:

```bash
launchctl list | grep claude-usage                                          # check it's running
tail -F ~/Library/Logs/claude-usage-daemon.out.log                          # live logs
launchctl unload ~/Library/LaunchAgents/com.user.claude-usage-daemon.plist  # stop
launchctl load -w ~/Library/LaunchAgents/com.user.claude-usage-daemon.plist # start
```

## Linux installation

### Flash the firmware

The Core's CH340X enumerates as `/dev/ttyUSB0` (not `/dev/ttyACM0`):

```bash
./flash.sh                       # defaults to /dev/ttyUSB0
./flash.sh /dev/ttyUSB1          # or pass an explicit port
```

### Pair the device

After flashing, the device advertises as **"Attaky Claude Monitor"**. Pair
it once:

```bash
# Scan for the device
bluetoothctl scan le

# When "Attaky Claude Monitor" appears, pair and trust it
bluetoothctl pair AA:BB:CC:DD:EE:FF    # use your device's MAC
bluetoothctl trust AA:BB:CC:DD:EE:FF
```

The MAC address is shown on the Bluetooth screen — press the BOOT
button to cycle to it.

### Install the daemon

The daemon polls your Claude usage every 60 seconds and sends it to the
display over BLE.

```bash
./install.sh
systemctl --user start claude-usage-daemon
```

Check status: `systemctl --user status claude-usage-daemon`

View logs: `journalctl --user -u claude-usage-daemon -f`

## How it works

1. The daemon reads your Claude Code OAuth token (macOS Keychain, or
   `~/.claude/.credentials.json` on Linux).
2. It makes a minimal API call to `api.anthropic.com/v1/messages` — one
   token of Haiku, basically free.
3. The usage numbers come straight out of the response headers
   (`anthropic-ratelimit-unified-5h-utilization` and friends).
4. The daemon connects to the ESP32 over BLE and writes a JSON payload
   to the GATT RX characteristic.
5. The firmware parses it and updates the LVGL dashboard.
6. The firmware tracks the rate of change of session % over a 5-minute
   window and picks splash animations from the matching mood group.
7. The physical keys are independent of all of this — they emit BLE HID
   keyboard reports to the paired host directly (see below).

## Physical buttons

The keys send standard BLE HID keyboard reports, so they act on
whatever window has focus on the paired host — not just Claude Code.

| Key            | Function                                        |
| -------------- | ----------------------------------------------- |
| **D-pad ↑↓←→** | Arrow keys                                      |
| **SELECT**     | Enter / confirm                                 |
| **L1**         | Shift+Tab (Claude Code mode toggle)             |
| **R2**         | Space (Claude Code voice-mode push-to-talk)     |
| **BOOT**       | Cycle screens; on splash, cycle animations      |

The D-pad, SELECT, L1 and R2 keys are read through the AW9523 I²C IO
expander; BOOT is a dedicated GPIO. The hardware POWER button is a
power toggle handled below the firmware and is intentionally not
mapped.

## BLE protocol

The device advertises a custom GATT service alongside the standard HID
keyboard service:

|                            | UUID                                   |
| -------------------------- | -------------------------------------- |
| **Data Service**           | `4c41555a-4465-7669-6365-000000000001` |
| RX Characteristic (write)  | `4c41555a-4465-7669-6365-000000000002` |
| TX Characteristic (notify) | `4c41555a-4465-7669-6365-000000000003` |
| REQ Characteristic (notify)| `4c41555a-4465-7669-6365-000000000004` |
| **HID Service**            | `00001812-0000-1000-8000-00805f9b34fb` |

JSON payload format (written to RX):

```json
{ "s": 45, "sr": 120, "w": 28, "wr": 7200, "st": "allowed", "ok": true }
```

Fields: `s` = session %, `sr` = session reset (minutes), `w` = weekly %,
`wr` = weekly reset (minutes), `st` = status, `ok` = success flag.

## Recompiling fonts

The `firmware/src/font_*.c` files are pre-compiled LVGL bitmap fonts.
The 320×240 build uses the sizes already in the repo
(Serif 34; Sans 28/16/14/12; Mono 18). All three source typefaces are
freely licensed — Source Serif 4 (display serif) and Archivo (UI sans)
under the SIL OFL 1.1 (license texts in `assets/`), DejaVu Sans Mono
under the DejaVu/Bitstream Vera license. You only need this section if
you want to change the sizes.

```bash
npm install -g lv_font_conv
```

Generate each one (one at a time — `lv_font_conv` doesn't like
loop-driven invocations) with `--no-compress` (required for LVGL 9),
e.g.:

```bash
lv_font_conv --font assets/SourceSerif4-Regular.ttf -r 0x20-0x7E \
  --size 34 --format lvgl --bpp 4 --no-compress \
  -o firmware/src/font_serif_34.c --lv-include "lvgl.h"

for size in 28 16 14 12; do
  lv_font_conv --font assets/Archivo-Regular.ttf -r 0x20-0x7E \
    --size $size --format lvgl --bpp 4 --no-compress \
    -o firmware/src/font_sans_${size}.c --lv-include "lvgl.h"
done

lv_font_conv --font assets/DejaVuSansMono.ttf \
  -r 0x20-0x7E,0xB7,0x2026,0x2722,0x2733,0x2736,0x273B,0x273D \
  --size 18 --format lvgl --bpp 4 --no-compress \
  -o firmware/src/font_mono_18.c --lv-include "lvgl.h"
```

**Important:** `lv_font_conv` v1.5.3 outputs LVGL 8 format. Each
generated file must be patched for LVGL 9 compatibility:

1. Remove `#if LVGL_VERSION_MAJOR >= 8` guards around `font_dsc` and the font struct
2. Remove the `.cache` field from `font_dsc`
3. Add `.release_glyph = NULL`, `.kerning = 0`, `.static_bitmap = 0` to the font struct
4. Add `.fallback = NULL`, `.user_data = NULL` to the font struct

Without these patches, fonts compile but render as invisible.

## Converting icons

The UI uses a small set of [Lucide](https://lucide.dev) icons
(bluetooth + battery states) plus the logo, converted to RGB565 /
RGB565A8 C arrays for LVGL. The converter needs `pngjs`:

```bash
cd tools && npm install && cd ..
node tools/png_to_lvgl.js assets/icon_bluetooth_48.png icon_bluetooth_data ICON_BLUETOOTH_WIDTH ICON_BLUETOOTH_HEIGHT
```

Default tint is white (`0xFFFFFF`); Lucide PNGs ship as
black-on-transparent and would render invisible against the dark UI
without it. Pass `--no-tint` for pre-coloured artwork like the logo
(regenerated at 36×36 for the smaller screen). Battery icons use
RGB565A8 (alpha plane) so they blend cleanly over the splash; the rest
are baked RGB565 over the panel colour. Paste the converter output into
`firmware/src/icons.h` (or `logo.h` for the logo).

## Splash animations

The animations come from [claudepix.vercel.app](https://claudepix.vercel.app),
a library of Clawd sprites. `tools/scrape_claudepix.js` evaluates the
site's JavaScript in a Node VM to pull out frame data and palettes,
then `tools/convert_to_c.js` turns everything into RGB565 C arrays and
writes `firmware/src/splash_animations.h`.

To re-pull (e.g. when the source library updates):

```bash
node tools/scrape_claudepix.js
node tools/convert_to_c.js
pio run -d firmware -t upload
```

See `tools/README.md` for details.

## Credits

- Original Clawdmeter by @hermannbjorgvin.
- macOS host pieces (daemon, LaunchAgent, flash helper) by
  Chris Davidson (@lorddavidson).
- Attaky Core 1.0 hardware adaptation by the Attaky project.
- Pixel-art Clawd animation by @amaanbuilds. Frame data and palettes
  scraped + converted by the tooling in `tools/`.
- Lucide icon set (MIT) for the bluetooth and battery UI glyphs.
- UI typefaces: Source Serif 4 (Adobe, SIL OFL 1.1), Archivo
  (Omnibus-Type, SIL OFL 1.1) and DejaVu Sans Mono (Bitstream Vera
  license). OFL license texts ship in `assets/`.

## Licensing gray area warning

Unlike upstream, this fork does NOT bundle Anthropic's proprietary
brand fonts (Tiempos Text, Styrene B) — the UI was rebuilt on the
freely licensed typefaces listed above. The remaining gray area is the
Clawd mascot artwork. The original author's statement:

> The software in this repository uses and adheres to the Anthropic
> brand guidelines and uses the same proprietary fonts that Anthropic
> has a license for but this software uses without permission as well
> as using assets from Anthropic such as the copyrighted Clawd mascot
> so even though the code in this repo is non-proprietary I will not
> license it myself under a copyleft license since this repo includes
> proprietary fonts and copyrighted assets. Please be aware of this if
> you fork or copy the code from this repo. **You have been warned!**

For this fork the font half of that warning no longer applies; the
copyrighted Clawd mascot assets are still included as-is and are not
relicensed.
