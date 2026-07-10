#pragma once

#include <stdint.h>

void io_expander_init(void);
uint8_t io_expander_buttons_raw(void);
bool io_expander_touch_int_asserted(void);
void io_expander_ctp_reset(bool asserted);
void io_expander_led(bool r, bool g, bool b);
