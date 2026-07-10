#pragma once

#define BOARD_NAME "Attaky Core ESP32"

// ST7789 panel is 240x320 native, mounted for 320x240 landscape.
#define LCD_NATIVE_W 240
#define LCD_NATIVE_H 320
#define LCD_WIDTH    320
#define LCD_HEIGHT   240
#define LCD_ROTATION 3

// TFT_eSPI pin configuration is supplied by platformio.ini build flags.
#define LCD_BL 14

// Shared I2C bus: touch, IO expanders, fuel gauge.
#define IIC_SDA     2
#define IIC_SCL     1
#define I2C_FREQ_HZ 400000

// I2C addresses.
#define FT6636_ADDR    0x38
#define AW9523_A_ADDR  0x58
#define AW9523_B_ADDR  0x59
#define MAX17048_ADDR  0x36

// AW9523_B (@0x59) Port0 button bits, active low.
#define BTN_BIT_DOWN   0
#define BTN_BIT_RIGHT  1
#define BTN_BIT_SELECT 2
#define BTN_BIT_LEFT   3
#define BTN_BIT_UP     4
#define BTN_BIT_L1     5
#define BTN_BIT_R2     6
#define BTN_BIT_POWER  7

// AW9523_B (@0x59) Port1 output bits, active low for LEDs/reset.
#define AWB_P1_LED_R     0
#define AWB_P1_LED_G     1
#define AWB_P1_LED_B     2
#define AWB_P1_CTP_RESET 3

// AW9523_A (@0x58) Port0 touch interrupt bit.
#define AWA_P0_TOUCH_INT 4

// Direct BOOT button, active low. Used as UI navigation on Attaky Core.
#define BOOT_BTN_PIN 38

#define BOARD_HAS_SECONDARY_BUTTON 0
#define BOARD_HAS_ROTATION         0
#define BOARD_HAS_IMU              0
#define BOARD_HAS_BATTERY          1
#define BOARD_HAS_IO_EXPANDER      1
#define BOARD_HAS_SOUND            0
