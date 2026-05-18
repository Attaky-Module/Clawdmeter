#include "aw9523.h"
#include "display_cfg.h"
#include <Arduino.h>
#include <Wire.h>

// AW9523B register map
#define AW_REG_INPUT0    0x00
#define AW_REG_INPUT1    0x01
#define AW_REG_OUTPUT0   0x02
#define AW_REG_OUTPUT1   0x03
#define AW_REG_CONFIG0   0x04   // 1 = input, 0 = output
#define AW_REG_CONFIG1   0x05
#define AW_REG_INT0      0x06   // 1 = interrupt disabled
#define AW_REG_INT1      0x07
#define AW_REG_ID        0x10
#define AW_REG_GCR       0x11   // bit4 GPOMD: 1 = Port0 push-pull
#define AW_REG_LEDMODE0  0x12   // 1 = GPIO mode, 0 = LED current mode
#define AW_REG_LEDMODE1  0x13

// Shadow of AW9523_B Port1 output (RGB LED + CTP_RESET). Reset released and
// LEDs off by default: P13 (CTP_RESET) HIGH = run, P10/P11/P12 HIGH = LED off.
static uint8_t awb_p1_out = 0xFF;

static bool aw_write(uint8_t addr, uint8_t reg, uint8_t val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(val);
    return Wire.endTransmission() == 0;
}

static bool aw_read(uint8_t addr, uint8_t reg, uint8_t *val) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    if (Wire.requestFrom((int)addr, 1) != 1) return false;
    *val = Wire.read();
    return true;
}

void aw9523_init(void) {
    uint8_t id = 0;

    // ---- AW9523_A @0x58: only Port0 bit4 (touch INT) is read here ----
    if (aw_read(AW9523_A_ADDR, AW_REG_ID, &id)) {
        Serial.printf("AW9523_A(0x58) ID=0x%02X\n", id);
        aw_write(AW9523_A_ADDR, AW_REG_CONFIG0, 0xFF);  // Port0 all input
        aw_write(AW9523_A_ADDR, AW_REG_CONFIG1, 0xFF);  // Port1 all input
                                                        // (QMI8658/chained INT)
        aw_write(AW9523_A_ADDR, AW_REG_LEDMODE0, 0xFF);  // GPIO mode
        aw_write(AW9523_A_ADDR, AW_REG_LEDMODE1, 0xFF);
    } else {
        Serial.println("AW9523_A(0x58) not responding");
    }

    // ---- AW9523_B @0x59: buttons + RGB LED + CTP_RESET ----
    if (aw_read(AW9523_B_ADDR, AW_REG_ID, &id)) {
        Serial.printf("AW9523_B(0x59) ID=0x%02X\n", id);
    } else {
        Serial.println("AW9523_B(0x59) not responding");
    }
    aw_write(AW9523_B_ADDR, AW_REG_LEDMODE0, 0xFF);   // all GPIO, not LED-mode
    aw_write(AW9523_B_ADDR, AW_REG_LEDMODE1, 0xFF);
    aw_write(AW9523_B_ADDR, AW_REG_GCR, 0x10);        // Port0 push-pull
    aw_write(AW9523_B_ADDR, AW_REG_CONFIG0, 0xFF);    // Port0 all input (buttons)
    // Port1: P10/P11/P12/P13 output (LED + CTP_RESET), P14-P17 input.
    aw_write(AW9523_B_ADDR, AW_REG_CONFIG1, 0xF0);
    aw_write(AW9523_B_ADDR, AW_REG_OUTPUT1, awb_p1_out);
}

uint8_t aw9523_buttons_raw(void) {
    uint8_t v = 0xFF;  // default = nothing pressed (active low)
    aw_read(AW9523_B_ADDR, AW_REG_INPUT0, &v);
    return v;
}

bool aw9523_touch_int_asserted(void) {
    uint8_t v = 0xFF;
    if (!aw_read(AW9523_A_ADDR, AW_REG_INPUT0, &v)) return false;
    return (v & (1 << AWA_P0_TOUCH_INT)) == 0;  // active low
}

void aw9523_ctp_reset(bool asserted) {
    if (asserted) awb_p1_out &= ~(1 << AWB_P1_CTP_RESET);  // LOW = in reset
    else          awb_p1_out |=  (1 << AWB_P1_CTP_RESET);  // HIGH = run
    aw_write(AW9523_B_ADDR, AW_REG_OUTPUT1, awb_p1_out);
}

void aw9523_led(bool r, bool g, bool b) {
    // LOW = on, so a true channel clears its bit.
    if (r) awb_p1_out &= ~(1 << AWB_P1_LED_R); else awb_p1_out |= (1 << AWB_P1_LED_R);
    if (g) awb_p1_out &= ~(1 << AWB_P1_LED_G); else awb_p1_out |= (1 << AWB_P1_LED_G);
    if (b) awb_p1_out &= ~(1 << AWB_P1_LED_B); else awb_p1_out |= (1 << AWB_P1_LED_B);
    aw_write(AW9523_B_ADDR, AW_REG_OUTPUT1, awb_p1_out);
}
