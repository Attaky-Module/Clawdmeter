#include "../../hal/display_hal.h"
#include "board.h"
#include <Arduino.h>
#include <TFT_eSPI.h>

static TFT_eSPI tft;

void display_hal_init(void) {}

// The panel has no brightness command; brightness is a LEDC PWM duty on
// the backlight GPIO, so the brightness levels and the idle fade work.
void display_hal_begin(void) {
    pinMode(LCD_BL, OUTPUT);  // TFT_eSPI writes TFT_BL before configuring it
    tft.init();
    tft.setRotation(LCD_ROTATION);
    tft.fillScreen(TFT_BLACK);
    ledcAttach(LCD_BL, 5000 /* Hz */, 8 /* bits */);
    ledcWrite(LCD_BL, 200);
}

void display_hal_set_brightness(uint8_t level) {
    ledcWrite(LCD_BL, level);
}

void display_hal_draw_bitmap(int32_t x, int32_t y, int32_t w, int32_t h,
                             const uint16_t* pixels) {
    tft.startWrite();
    tft.setAddrWindow(x, y, w, h);
    tft.pushColors((uint16_t*)pixels, w * h, true);
    tft.endWrite();
}

void display_hal_tick(void) {}

void display_hal_round_area(int32_t* x1, int32_t* y1, int32_t* x2, int32_t* y2) {
    (void)x1;
    (void)y1;
    (void)x2;
    (void)y2;
}
