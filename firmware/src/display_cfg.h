#pragma once

#include <TFT_eSPI.h>
#include <Wire.h>

// ============================================================================
// Attaky Core_ESP32_1.0 (+ Power_Standard-Cell_1.0) hardware map.
// Pin assignments verified against the authoritative Attaky hardware spec.
// ============================================================================

// ---- Display: ST7789 240x320 over SPI, used landscape via setRotation(3) ----
// Panel is physically mounted 180-rotated in the Core enclosure; rotation 3
// is the user-correct landscape orientation (hardware-verified).
#define LCD_NATIVE_W   240
#define LCD_NATIVE_H   320
#define LCD_WIDTH      320    // logical, after setRotation(3)
#define LCD_HEIGHT     240
#define LCD_ROTATION   3
#define LCD_BL         14   // backlight (IO14); must be driven OUTPUT HIGH
// Other pin numbers are compiled into TFT_eSPI via platformio.ini build
// flags (TFT_MOSI=13, TFT_SCLK=11, TFT_CS=45, TFT_DC=12, TFT_RST=10).

// ---- Single shared I2C bus (SCL=IO1, SDA=IO2) ----
#define IIC_SDA        2
#define IIC_SCL        1
#define I2C_FREQ_HZ    400000

// ---- I2C device addresses (7-bit) ----
#define FT6636_ADDR    0x38   // capacitive touch
#define AW9523_A_ADDR  0x58   // IO expander: touch INT (P04) + passthrough
#define AW9523_B_ADDR  0x59   // IO expander: buttons (P00-P07), CTP_RESET,
                              //              RGB LED (P10/P11/P12), P13=CTP_RST
#define MAX17048_ADDR  0x36   // fuel gauge (Power_Standard-Cell_1.0)

// ---- AW9523_B (@0x59) Port0 button bits (active LOW) ----
#define BTN_BIT_DOWN       0
#define BTN_BIT_RIGHT      1
#define BTN_BIT_SELECT     2
#define BTN_BIT_LEFT       3
#define BTN_BIT_UP         4
#define BTN_BIT_L1         5
#define BTN_BIT_R2         6
#define BTN_BIT_POWER      7   // hardware power toggle — do NOT use as UI key

// ---- AW9523_B (@0x59) Port1 output bits ----
#define AWB_P1_LED_R       0   // P10 (LOW = on)
#define AWB_P1_LED_G       1   // P11
#define AWB_P1_LED_B       2   // P12
#define AWB_P1_CTP_RESET   3   // P13 (LOW = touch IC held in reset)

// ---- AW9523_A (@0x58) Port0: touch INT on bit 4 ----
#define AWA_P0_TOUCH_INT   4

// ---- BOOT_BTN: direct ESP32 GPIO (IO38), external pullup, LOW = pressed.
//      Used as the UI-nav key (cycle screen / next splash animation).
#define BOOT_BTN_PIN       38

// ---- Global hardware object (defined in main.cpp) ----
extern TFT_eSPI tft;
