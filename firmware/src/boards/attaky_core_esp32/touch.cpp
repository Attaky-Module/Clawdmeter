#include "../../hal/touch_hal.h"
#include "board.h"
#include "io_expander.h"
#include <Arduino.h>
#include <Wire.h>

#define FT_REG_TD_STATUS 0x02
#define FT_REG_P1_XH     0x03
#define FT_REG_CHIPID    0xA3

static bool touch_pressed = false;
static uint16_t touch_x = 0;
static uint16_t touch_y = 0;

static bool ft_read(uint8_t reg, uint8_t* buf, uint8_t len) {
    Wire.beginTransmission(FT6636_ADDR);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)FT6636_ADDR, (int)len) != len) return false;
    for (uint8_t i = 0; i < len; i++) buf[i] = Wire.read();
    return true;
}

static void map_touch(uint16_t nx, uint16_t ny, uint16_t* lx, uint16_t* ly) {
    uint16_t x = (uint16_t)(LCD_NATIVE_H - 1 - ny);
    uint16_t y = nx;
    if (x >= LCD_WIDTH) x = LCD_WIDTH - 1;
    if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;
    *lx = x;
    *ly = y;
}

static void read_touch_state(void) {
    uint8_t b[5];
    if (!ft_read(FT_REG_TD_STATUS, b, sizeof(b))) {
        touch_pressed = false;
        return;
    }

    uint8_t points = b[0] & 0x0F;
    if (points == 0 || points > 5) {
        touch_pressed = false;
        return;
    }

    uint16_t nx = ((uint16_t)(b[1] & 0x0F) << 8) | b[2];
    uint16_t ny = ((uint16_t)(b[3] & 0x0F) << 8) | b[4];
    if (nx >= LCD_NATIVE_W) nx = LCD_NATIVE_W - 1;
    if (ny >= LCD_NATIVE_H) ny = LCD_NATIVE_H - 1;
    map_touch(nx, ny, &touch_x, &touch_y);
    touch_pressed = true;
}

void touch_hal_init(void) {
    io_expander_ctp_reset(true);
    delay(10);
    io_expander_ctp_reset(false);
    delay(200);

    uint8_t id = 0;
    if (ft_read(FT_REG_CHIPID, &id, 1)) {
        Serial.printf("FT6636 CHIPID=0x%02X\n", id);
    } else {
        Serial.println("FT6636 not responding");
    }
}

void touch_hal_read(uint16_t* x, uint16_t* y, bool* pressed) {
    read_touch_state();
    *x = touch_x;
    *y = touch_y;
    *pressed = touch_pressed;
}
