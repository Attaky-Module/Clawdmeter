# Attaky Core — `attaky_core_esp32`

Hardware notes for this board. Shared firmware rules are in `firmware/CLAUDE.md`.

Attaky Core ESP32 (ESP32-S3 with octal PSRAM, 320×240 landscape ST7789 over SPI, FT6636 touch @ 0x38) with the Power cell (MAX17048 fuel gauge @ 0x36). Build env: `attaky_core_esp32`. Display driver is TFT_eSPI, configured entirely by the env's build flags (no `User_Setup.h`).

- **USB is a CH340 bridge**, not native USB: no `ARDUINO_USB_CDC_ON_BOOT`, ports enumerate as `/dev/cu.usbserial-*` / `/dev/ttyUSB*`, and opening the port resets the board. The desktop app's flasher lists only Espressif-VID native-USB ports, so flash with `pio run -d firmware -e attaky_core_esp32 -t upload --upload-port <port>`.
- I2C bus SDA 2 / SCL 1 carries touch, two AW9523 IO expanders and the fuel gauge. `board_init()` configures the expanders; `touch_hal_init()` pulses the touch reset through AW9523_B P1.3.
- Keys sit on AW9523_B (0x59) Port0, active low, polled and debounced in `input.cpp`: R2 = PRIMARY (Space), L1 = SECONDARY (Shift+Tab), D-pad + SELECT = extended keys (arrows / Enter). The board's LEFT/RIGHT pin labels are mirrored against the physical keys; `input.cpp` compensates.
- **POWER long-hold is a hardware power toggle** the firmware cannot see the end of, so no long-press gesture may sit on POWER. POWER short press = brightness. Hold-to-pair comes from BOOT (GPIO 38): `power.cpp` reports the BOOT long-press / release edges as `pwr_long_pressed` / `pwr_released`, and `pair_on_boot` makes the pairing hint name BOOT. A BOOT press sends no key.
- Battery: MAX17048 state of charge; "charging" is a positive C-RATE. No VBUS or charger-status signal reaches the MCU, so `power_hal_is_vbus_in()` is "charging, or no fuel gauge" (no Power cell means USB power). On USB with a full battery it reads false and the screen may idle-sleep as on battery.
- Backlight is LEDC PWM on GPIO 14; the panel has no brightness command. No audio, no IMU.
- BLE name is "Attaky Claude Monitor" (`BoardCaps.ble_name`); the advert drops the optional appearance so the name fits.
