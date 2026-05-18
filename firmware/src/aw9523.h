#pragma once
#include <stdint.h>
#include <stdbool.h>

// Minimal AW9523B driver for the two Core IO expanders on the shared I2C bus.
// @0x58 (AW9523_A): we only read Port0 bit4 (FT6636 touch INT).
// @0x59 (AW9523_B): Port0 = 8 buttons (input, active LOW);
//                   Port1 P10/P11/P12 = RGB LED (output, LOW=on),
//                   Port1 P13 = CTP_RESET (output, LOW=reset).

void    aw9523_init(void);

// Raw Port0 input byte of AW9523_B (@0x59). Bit N low = that button pressed.
uint8_t aw9523_buttons_raw(void);

// true if the FT6636 touch INT line (AW9523_A @0x58 Port0 bit4) is asserted
// (active low → returns true when the line is LOW = data ready).
bool    aw9523_touch_int_asserted(void);

// Drive the FT6636 reset line (AW9523_B @0x59 P13). asserted=true holds the
// touch IC in reset (line LOW); asserted=false releases it (line HIGH).
void    aw9523_ctp_reset(bool asserted);

// RGB status LED on AW9523_B @0x59 P10/P11/P12 (LOW = on).
void    aw9523_led(bool r, bool g, bool b);
