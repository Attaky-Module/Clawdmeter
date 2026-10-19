#include "io_expander.h"
#include "board.h"
#include <Arduino.h>
#include <Wire.h>

#define AW_REG_INPUT0    0x00
#define AW_REG_OUTPUT1   0x03
#define AW_REG_CONFIG0   0x04
#define AW_REG_CONFIG1   0x05
#define AW_REG_ID        0x10
#define AW_REG_GCR       0x11
#define AW_REG_LEDMODE0  0x12
#define AW_REG_LEDMODE1  0x13

static uint8_t awb_p1_out = 0xFF;

static bool aw_write(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool aw_read(uint8_t addr, uint8_t reg, uint8_t* val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)addr, 1) != 1) return false;
    *val = Wire.read();
    return true;
}

void io_expander_init(void) {
    uint8_t id = 0;

    if (aw_read(AW9523_A_ADDR, AW_REG_ID, &id)) {
        Serial.printf("AW9523_A(0x58) ID=0x%02X\n", id);
        aw_write(AW9523_A_ADDR, AW_REG_CONFIG0, 0xFF);
        aw_write(AW9523_A_ADDR, AW_REG_CONFIG1, 0xFF);
        aw_write(AW9523_A_ADDR, AW_REG_LEDMODE0, 0xFF);
        aw_write(AW9523_A_ADDR, AW_REG_LEDMODE1, 0xFF);
    } else {
        Serial.println("AW9523_A(0x58) not responding");
    }

    if (aw_read(AW9523_B_ADDR, AW_REG_ID, &id)) {
        Serial.printf("AW9523_B(0x59) ID=0x%02X\n", id);
    } else {
        Serial.println("AW9523_B(0x59) not responding");
    }

    aw_write(AW9523_B_ADDR, AW_REG_LEDMODE0, 0xFF);
    aw_write(AW9523_B_ADDR, AW_REG_LEDMODE1, 0xFF);
    aw_write(AW9523_B_ADDR, AW_REG_GCR, 0x10);
    aw_write(AW9523_B_ADDR, AW_REG_CONFIG0, 0xFF);
    aw_write(AW9523_B_ADDR, AW_REG_CONFIG1, 0xF0);
    aw_write(AW9523_B_ADDR, AW_REG_OUTPUT1, awb_p1_out);
}

uint8_t io_expander_buttons_raw(void) {
    uint8_t v = 0xFF;
    aw_read(AW9523_B_ADDR, AW_REG_INPUT0, &v);
    return v;
}

bool io_expander_touch_int_asserted(void) {
    uint8_t v = 0xFF;
    if (!aw_read(AW9523_A_ADDR, AW_REG_INPUT0, &v)) return false;
    return (v & (1 << AWA_P0_TOUCH_INT)) == 0;
}

void io_expander_ctp_reset(bool asserted) {
    if (asserted) awb_p1_out &= ~(1 << AWB_P1_CTP_RESET);
    else          awb_p1_out |=  (1 << AWB_P1_CTP_RESET);
    aw_write(AW9523_B_ADDR, AW_REG_OUTPUT1, awb_p1_out);
}

void io_expander_led(bool r, bool g, bool b) {
    if (r) awb_p1_out &= ~(1 << AWB_P1_LED_R); else awb_p1_out |= (1 << AWB_P1_LED_R);
    if (g) awb_p1_out &= ~(1 << AWB_P1_LED_G); else awb_p1_out |= (1 << AWB_P1_LED_G);
    if (b) awb_p1_out &= ~(1 << AWB_P1_LED_B); else awb_p1_out |= (1 << AWB_P1_LED_B);
    aw_write(AW9523_B_ADDR, AW_REG_OUTPUT1, awb_p1_out);
}
