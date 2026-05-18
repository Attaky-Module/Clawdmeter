#include "ft6636.h"
#include "display_cfg.h"
#include "aw9523.h"
#include <Arduino.h>
#include <Wire.h>

// FT6x36 registers
#define FT_REG_TD_STATUS  0x02   // low nibble = number of touch points
#define FT_REG_P1_XH      0x03   // [3:0] X high, [7:6] event flag
#define FT_REG_P1_XL      0x04
#define FT_REG_P1_YH      0x05   // [3:0] Y high
#define FT_REG_P1_YL      0x06
#define FT_REG_CHIPID     0xA3

static bool ft_read(uint8_t reg, uint8_t *buf, uint8_t len) {
    Wire.beginTransmission(FT6636_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)FT6636_ADDR, (int)len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

void ft6636_init(void) {
    // Hardware reset via AW9523_B P13: hold low, release, settle.
    aw9523_ctp_reset(true);
    delay(10);
    aw9523_ctp_reset(false);
    delay(200);

    uint8_t id = 0;
    if (ft_read(FT_REG_CHIPID, &id, 1)) {
        Serial.printf("FT6636 CHIPID=0x%02X\n", id);
    } else {
        Serial.println("FT6636 not responding");
    }
}

bool ft6636_read(uint16_t *x, uint16_t *y) {
    uint8_t b[7];
    if (!ft_read(FT_REG_TD_STATUS, b, 5)) return false;

    uint8_t points = b[0] & 0x0F;
    if (points == 0 || points > 5) return false;

    uint16_t rx = ((uint16_t)(b[1] & 0x0F) << 8) | b[2];
    uint16_t ry = ((uint16_t)(b[3] & 0x0F) << 8) | b[4];

    if (rx >= LCD_NATIVE_W) rx = LCD_NATIVE_W - 1;
    if (ry >= LCD_NATIVE_H) ry = LCD_NATIVE_H - 1;

    *x = rx;
    *y = ry;
    return true;
}
