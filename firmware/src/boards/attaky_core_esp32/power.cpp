#include "../../hal/power_hal.h"
#include "../../hal/input_hal.h"
#include "board.h"
#include "io_expander.h"
#include <Arduino.h>
#include <Wire.h>

#define MAX_REG_SOC     0x04
#define MAX_REG_VERSION 0x08
#define MAX_REG_CRATE   0x16

#define BATTERY_POLL_MS  2000
#define CHARGING_POLL_MS 1000
#define PWR_POLL_MS      50
#define PWR_LONG_MS      1500
#define CRATE_CHARGE_THRESHOLD_PCT_PER_HR 0.5f

static bool gauge_ok = false;
static int cached_pct = -1;
static bool cached_charging = false;
static bool pwr_pressed_flag = false;
static bool pwr_long_flag = false;
static bool pwr_released_flag = false;
static bool last_pwr_state = false;
static bool pwr_long_fired = false;
static uint32_t pwr_press_started_ms = 0;
static bool last_boot_state = false;
static bool boot_long_fired = false;
static uint32_t boot_press_started_ms = 0;
static uint32_t last_battery_ms = 0;
static uint32_t last_charging_ms = 0;
static uint32_t last_pwr_ms = 0;

static bool max_read16(uint8_t reg, uint16_t* out) {
    Wire.beginTransmission(MAX17048_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)MAX17048_ADDR, 2) != 2) return false;
    uint8_t hi = Wire.read();
    uint8_t lo = Wire.read();
    *out = ((uint16_t)hi << 8) | lo;
    return true;
}

static int read_soc_pct(void) {
    uint16_t raw = 0;
    if (!max_read16(MAX_REG_SOC, &raw)) return -1;
    int pct = (int)(raw / 256);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

static bool read_charging(void) {
    uint16_t raw = 0;
    if (!max_read16(MAX_REG_CRATE, &raw)) return false;
    float rate = (int16_t)raw * 0.208f;
    return rate > CRATE_CHARGE_THRESHOLD_PCT_PER_HR;
}

void power_hal_init(void) {
    uint16_t ver = 0;
    gauge_ok = max_read16(MAX_REG_VERSION, &ver);
    if (gauge_ok) {
        Serial.printf("MAX17048 VERSION=0x%04X\n", ver);
        cached_pct = read_soc_pct();
        cached_charging = read_charging();
    } else {
        Serial.println("MAX17048 not responding (battery module absent?)");
    }
}

void power_hal_tick(void) {
    uint32_t now = millis();

    if (gauge_ok && now - last_charging_ms >= CHARGING_POLL_MS) {
        last_charging_ms = now;
        cached_charging = read_charging();
    }
    if (gauge_ok && now - last_battery_ms >= BATTERY_POLL_MS) {
        last_battery_ms = now;
        cached_pct = read_soc_pct();
    }
    if (now - last_pwr_ms >= PWR_POLL_MS) {
        last_pwr_ms = now;

        // POWER short-press only. A long hold on this button is a
        // hardware-level power toggle executed by the power path at ~2s;
        // firmware never sees the end of the hold, so it must not carry
        // any long-press gesture. Short presses are still delivered.
        bool pwr_now = (io_expander_buttons_raw() & (1 << BTN_BIT_POWER)) == 0;
        if (pwr_now && !last_pwr_state) {
            pwr_press_started_ms = now;
            pwr_long_fired = false;
        } else if (pwr_now && last_pwr_state) {
            if (!pwr_long_fired && (now - pwr_press_started_ms >= PWR_LONG_MS)) {
                pwr_long_fired = true;  // suppress the short flag; no long gesture
            }
        } else if (!pwr_now && last_pwr_state) {
            if (!pwr_long_fired) pwr_pressed_flag = true;
        }
        last_pwr_state = pwr_now;

        // Hold-to-pair gesture lives on BOOT instead (safe to hold).
        // Long edge + release edge feed pair_tick() via the pwr_long /
        // pwr_released HAL flags; the short-release UI-nav action is
        // handled separately in shared code off INPUT_BTN_UI_NAV.
        bool boot_now = input_hal_is_held(INPUT_BTN_UI_NAV);
        if (boot_now && !last_boot_state) {
            boot_press_started_ms = now;
            boot_long_fired = false;
        } else if (boot_now && last_boot_state) {
            if (!boot_long_fired && (now - boot_press_started_ms >= PWR_LONG_MS)) {
                pwr_long_flag = true;
                boot_long_fired = true;
            }
        } else if (!boot_now && last_boot_state) {
            if (boot_long_fired) pwr_released_flag = true;
        }
        last_boot_state = boot_now;
    }
}

int power_hal_battery_pct(void) { return cached_pct; }
bool power_hal_is_charging(void) { return cached_charging; }
bool power_hal_is_vbus_in(void) { return cached_charging; }

bool power_hal_pwr_pressed(void) {
    if (pwr_pressed_flag) { pwr_pressed_flag = false; return true; }
    return false;
}

bool power_hal_pwr_long_pressed(void) {
    if (pwr_long_flag) { pwr_long_flag = false; return true; }
    return false;
}

bool power_hal_pwr_released(void) {
    if (pwr_released_flag) { pwr_released_flag = false; return true; }
    return false;
}
