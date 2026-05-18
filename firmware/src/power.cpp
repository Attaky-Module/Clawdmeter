#include "power.h"
#include "display_cfg.h"
#include <Arduino.h>
#include <Wire.h>

// MAX17048G+T10 fuel gauge (Power_Standard-Cell_1.0 @ 0x36).
// There is no PMIC / charger-status / VBUS line on Core — charge state and
// "USB present" are approximated from the gauge's signed charge-rate
// register (CRATE).
#define MAX_REG_VCELL    0x02   // 78.125 uV/LSB
#define MAX_REG_SOC      0x04   // %/256
#define MAX_REG_VERSION  0x08
#define MAX_REG_CRATE    0x16   // signed, 0.208 %/hr per LSB

#define BATTERY_POLL_MS  2000
#define CHARGING_POLL_MS 1000
// CRATE noise band: treat anything above this as "charging / on USB".
#define CRATE_CHARGE_THRESHOLD_PCT_PER_HR  0.5f

static bool     gauge_ok        = false;
static int      cached_pct      = -1;
static bool     cached_charging = false;
static uint32_t last_battery_ms  = 0;
static uint32_t last_charging_ms = 0;

static bool max_read16(uint8_t reg, uint16_t *out) {
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
    uint16_t raw;
    if (!max_read16(MAX_REG_SOC, &raw)) return -1;
    int pct = (int)(raw / 256);          // integer part of %/256
    if (pct < 0)   pct = 0;
    if (pct > 100) pct = 100;
    return pct;
}

static bool read_charging(void) {
    uint16_t raw;
    if (!max_read16(MAX_REG_CRATE, &raw)) return false;
    float rate = (int16_t)raw * 0.208f;  // signed %/hr
    return rate > CRATE_CHARGE_THRESHOLD_PCT_PER_HR;
}

void power_init(void) {
    uint16_t ver;
    gauge_ok = max_read16(MAX_REG_VERSION, &ver);
    if (gauge_ok) {
        Serial.printf("MAX17048 VERSION=0x%04X\n", ver);
        cached_pct      = read_soc_pct();
        cached_charging = read_charging();
    } else {
        Serial.println("MAX17048 not responding (battery module absent?)");
    }
}

void power_tick(void) {
    if (!gauge_ok) return;
    uint32_t now = millis();

    if (now - last_charging_ms >= CHARGING_POLL_MS) {
        last_charging_ms = now;
        cached_charging = read_charging();
    }
    if (now - last_battery_ms >= BATTERY_POLL_MS) {
        last_battery_ms = now;
        cached_pct = read_soc_pct();
    }
}

int  power_battery_pct(void) { return cached_pct; }
bool power_is_charging(void) { return cached_charging; }
bool power_pwr_pressed(void) { return false; }
