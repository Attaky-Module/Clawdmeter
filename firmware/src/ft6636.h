#pragma once
#include <stdint.h>
#include <stdbool.h>

// Minimal FocalTech FT6636 (FT6x36 family) capacitive-touch driver.
// Reset line is on AW9523_B @0x59 P13 (driven via aw9523_ctp_reset()).
void ft6636_init(void);

// Reads point 1. Returns true if a finger is down; *x/*y are panel-native
// coordinates (x: 0..LCD_NATIVE_W-1, y: 0..LCD_NATIVE_H-1). Rotation /
// mirror to LVGL space is done by the caller.
bool ft6636_read(uint16_t *x, uint16_t *y);
