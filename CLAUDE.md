# Project context

ESP32-S3 firmware for a desk-side Claude Code usage monitor. The board
is **Attaky Core_ESP32_1.0** (+ **Power_Standard-Cell_1.0** battery),
driving an **ST7789 240x320 SPI** display used landscape via
`setRotation(3)` (logical 320x240). It connects to a host daemon over
BLE; the daemon polls the Anthropic API for usage data.

This file is for future Claude Code sessions to bootstrap quickly. Read
this first.

> **Hardware authority.** This firmware was ported from an earlier
> Waveshare ESP32-S3-Touch-AMOLED-2.16 prototype to the Attaky Core
> board, so older AMOLED/CO5300-era assumptions no longer hold. The
> authoritative, current hardware truth lives in the repo itself:
> - Pin map / peripherals / display + touch parts:
>   `firmware/src/display_cfg.h` (the values the code compiles against —
>   never transcribe pin numbers into this file; they drift).
> - Board / platform / build flags: `firmware/platformio.ini`.
>
> Any hardware/pin/flash detail below is intentionally generic — defer
> to those two in-repo files for specifics, and read the relevant
> `firmware/src/` source before changing display, touch, power, or
> input code.

## Architecture

```text
main.cpp        — setup(), loop(), button polling, display flush
display_cfg.h   — authoritative pin defines + extern object decls
ui.{h,cpp}      — 3-screen UI (splash, usage, bluetooth); splash is touch-toggled, usage↔bluetooth via mid button
splash.{h,cpp}  — 20×20 pixel-art animation engine, upscaled to the panel
buttons.cpp     — physical button handling (pins in display_cfg.h)
power.{h,cpp}   — battery fuel-gauge wrapper (gauge part / address in display_cfg.h)
touch.{h,cpp}   — minimal tap detector → ui_toggle_splash() (Usage/Splash) or ble_clear_bonds() (BT reset zone)
ble.{h,cpp}     — NimBLE peripheral: custom data service + HID keyboard
data.h          — UsageData struct
icons.h         — icon arrays. Battery (5×) are RGB565A8 with alpha; rest are raw RGB565.
logo.h          — RGB565 logo
font_*.c        — pre-compiled LVGL 9 bitmap fonts (Serif 34 = Source Serif 4, Sans 28/16/14/12 = Archivo, Mono 18 = DejaVu; all freely licensed)
splash_animations.h — generated, do not hand-edit
```

Hardware specifics for any of the above (display controller, fuel
gauge, touch part, button pins, panel size/orientation) live in
`firmware/src/display_cfg.h` and `firmware/platformio.ini` — do not
restate them here.

## Build / flash

```bash
pio run -d firmware                 # build
pio run -d firmware -t upload       # flash (pio auto-detects the serial port)
```

`/home/hermann/.platformio/penv/bin/pio` if `pio` isn't on PATH.

No fixed upload port is pinned in `firmware/platformio.ini` — pio
auto-detects the device. The board exposes serial over an external
USB-UART (no native USB); confirm the port against the connected
device if auto-detection picks the wrong one.

## QA your own UI changes — don't ask the user

The firmware ships a `screenshot` serial command that dumps the LVGL
framebuffer; `./screenshot.sh out.png <serial-port>` captures a PNG at
the panel's native resolution. **Use this on every UI iteration** —
Read the PNG with the Read tool, verify the change visually, iterate.

The boot screen is `SCREEN_SPLASH` and only advances on a physical
button press, so a fresh flash sits on the splash. To screenshot the
screen you're editing without asking the user to press a button,
**temporarily change the default boot screen** in `main.cpp` (search
for `ui_show_screen(SCREEN_SPLASH);`) to the target screen, iterate,
then revert before committing.

## Porting note (replaces the old board-specific gotcha list)

Several constraints from the original Waveshare AMOLED prototype no
longer apply — this port uses **TFT_eSPI + ST7789**, not the old
AMOLED/GFX stack, so the previous CO5300/CST9220/OPI-PSRAM gotchas are
removed rather than carried forward stale. Before changing display,
touch, PSRAM, or font code, read the relevant source under
`firmware/src/` and the build flags in `firmware/platformio.ini`;
several non-obvious port constraints are encoded there (e.g. TFT_eSPI
pin config is compiled in via `platformio.ini` build flags, not a
`User_Setup.h`).

## Icons

`tools/png_to_lvgl.js <input.png> <symbol> [W_MACRO] [H_MACRO] [--tint=RRGGBB | --no-tint]` converts an alpha PNG to RGB565A8. Default tint is white (`0xFFFFFF`) — necessary for Lucide PNGs. Splice output into `firmware/src/icons.h` and use `init_icon_dsc_rgb565a8()` in ui.cpp. Currently only the 5 battery icons use this format; the rest are still raw RGB565 baked over the panel background, fine because they live inside opaque zones.

## Splash animations

13 × 20×20 pixel-art creature animations sourced from
[claudepix.vercel.app](https://claudepix.vercel.app). Pipeline:

```bash
node tools/scrape_claudepix.js  # → tools/claudepix_data/*.json
node tools/convert_to_c.js      # → firmware/src/splash_animations.h
```

Each animation has a per-animation 10-color RGB565 palette. Cell values 0..9 index it. Default boot screen.

## User profile / preferences

See `~/.claude/projects/.../memory/` files for persistent context (user is an embedded-beginner senior dev, brand-conscious, prefers iterative UI refinement, dislikes me authoring my own art when third-party assets are intended). Always read those memory files at session start.

## Daemon / host side

Bash daemon (`daemon/claude-usage-daemon.sh`) reads OAuth token, polls Anthropic API, sends JSON over BLE GATT. Run with `systemctl --user start claude-usage-daemon`. The unit file's `ExecStart` is the absolute path to the script — repoint it when switching between the worktree and the main checkout.

**Discovery & resilience:**

- Connects by name (`"Claude Controller"`), then caches the resolved address at `~/.config/claude-usage-monitor/ble-address`. The device's BLE address is NOT a stable factory MAC — NimBLE uses a random address that changes across boots, so any cached address invalidates on reboot or board swap. Name-based connect still works; the cache is only a fast-path.
- On connect failure: cache is dropped AND device is removed from bluez (`bluetoothctl remove`) so the next scan won't re-pick a dead address. Multi-candidate scans pick `head -1` and let the failure cycle converge.
- `POLL_INTERVAL=60`, `TICK=5`. Inner loop wakes every 5s to detect disconnects fast; polls Anthropic when 60s elapsed OR when ESP fires a refresh request.

**GATT characteristics on service `4c41555a-...0001`:**

- `...0002` RX — daemon writes JSON usage payload here.
- `...0003` TX — firmware notifies ack/nack (daemon doesn't subscribe).
- `...0004` REQ — firmware fires `0x01` notify in `onSubscribe` if `has_received_data` is false. Daemon subscribes via `setsid bash -c "stdbuf -oL dbus-monitor … | awk …"`; awk drops a flag file the inner loop picks up. See the `feedback_dbus_monitor_pipe` memory for the three subtle gotchas (pipe buffering, busctl-exits race, `wait` blocking on pipeline jobs).
