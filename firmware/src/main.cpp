#include <Arduino.h>
#include <lvgl.h>
#include <ArduinoJson.h>
#include "display_cfg.h"
#include "data.h"
#include "ui.h"
#include "ble.h"
#include "power.h"
#include "buttons.h"
#include "ft6636.h"
#include "aw9523.h"
#include "splash.h"
#include "usage_rate.h"

// ---- Hardware object ----
TFT_eSPI tft = TFT_eSPI();

static UsageData usage = {};

// ---- Touch shared state (polled FT6636; INT line is behind an I2C
//      expander so there is no GPIO ISR) ----
static bool     touch_pressed = false;
static uint16_t touch_x = 0;
static uint16_t touch_y = 0;

// Map FT6636 panel-native coords (nx:0..239, ny:0..319) to LVGL landscape
// space (0..LCD_WIDTH-1, 0..LCD_HEIGHT-1) for setRotation(3) + the panel's
// 180-degree physical mount.
// TUNE: verify on real hardware (tap the 4 corners, watch serial) and flip
// the axes/inversions here if a corner lands wrong.
static void map_touch(uint16_t nx, uint16_t ny, uint16_t *lx, uint16_t *ly) {
    uint16_t x = (uint16_t)(LCD_NATIVE_H - 1 - ny);  // 0..319 -> 0..LCD_WIDTH-1
    uint16_t y = nx;                                 // 0..239 -> 0..LCD_HEIGHT-1
    if (x >= LCD_WIDTH)  x = LCD_WIDTH - 1;
    if (y >= LCD_HEIGHT) y = LCD_HEIGHT - 1;
    *lx = x;
    *ly = y;
}

static void touch_read(void) {
    uint16_t nx, ny;
    if (ft6636_read(&nx, &ny)) {
        map_touch(nx, ny, &touch_x, &touch_y);
        touch_pressed = true;
    } else {
        touch_pressed = false;
    }
}

// ---- LVGL draw buffers (PSRAM-backed, partial render) ----
#define BUF_LINES 40
static uint16_t *buf1 = nullptr;
static uint16_t *buf2 = nullptr;

static uint32_t my_tick(void) { return millis(); }

// ST7789 supports native rotation, so the flush is a straight blit (no CPU
// strip-rotation / even-align rounding the CO5300 needed).
static void my_flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map) {
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t*)px_map, w * h, true);  // swap=true: LVGL LE -> ST7789 BE
    tft.endWrite();
    lv_display_flush_ready(disp);
}

static void my_touch_cb(lv_indev_t* indev, lv_indev_data_t* data) {
    if (touch_pressed) {
        data->point.x = touch_x;
        data->point.y = touch_y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static bool parse_json(const char* json, UsageData* out) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.printf("JSON parse error: %s\n", err.c_str());
        return false;
    }
    out->session_pct = doc["s"] | 0.0f;
    out->session_reset_mins = doc["sr"] | -1;
    out->weekly_pct = doc["w"] | 0.0f;
    out->weekly_reset_mins = doc["wr"] | -1;
    strlcpy(out->status, doc["st"] | "unknown", sizeof(out->status));
    out->ok = doc["ok"] | false;
    out->valid = true;
    return true;
}

// ---- Serial command buffer (UART0 via CH340X) ----
#define CMD_BUF_SIZE 64
static char cmd_buf[CMD_BUF_SIZE];
static int cmd_pos = 0;

static void send_screenshot() {
    const uint32_t w = LCD_WIDTH, h = LCD_HEIGHT;
    const uint32_t row_bytes = w * 2;
    const uint32_t buf_size = row_bytes * h;
    uint8_t* sbuf = (uint8_t*)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);
    if (!sbuf) {
        Serial.println("SCREENSHOT_ERR");
        return;
    }

    lv_draw_buf_t draw_buf;
    lv_draw_buf_init(&draw_buf, w, h, LV_COLOR_FORMAT_RGB565, row_bytes, sbuf, buf_size);

    lv_result_t res = lv_snapshot_take_to_draw_buf(lv_screen_active(), LV_COLOR_FORMAT_RGB565, &draw_buf);
    if (res != LV_RESULT_OK) {
        heap_caps_free(sbuf);
        Serial.println("SCREENSHOT_ERR");
        return;
    }

    Serial.printf("SCREENSHOT_START %lu %lu %lu\n", (unsigned long)w, (unsigned long)h, (unsigned long)buf_size);
    Serial.flush();
    Serial.write(sbuf, buf_size);
    Serial.flush();
    Serial.println();
    Serial.println("SCREENSHOT_END");

    heap_caps_free(sbuf);
}

static void check_serial_cmd() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            cmd_buf[cmd_pos] = '\0';
            if (strcmp(cmd_buf, "screenshot") == 0) {
                send_screenshot();
            }
            cmd_pos = 0;
        } else if (cmd_pos < CMD_BUF_SIZE - 1) {
            cmd_buf[cmd_pos++] = c;
        }
    }
}

void setup() {
    Serial.begin(115200);   // UART0 → CH340X (no native USB; no USB CDC flag)
    delay(300);
    Serial.println("{\"ready\":true}");

    // Single shared I2C bus (SCL=IO1, SDA=IO2): touch, expanders, fuel gauge.
    Wire.begin(IIC_SDA, IIC_SCL);
    Wire.setClock(I2C_FREQ_HZ);

    // Backlight: drive IO14 as GPIO HIGH before TFT_eSPI touches it
    // (Arduino-3.x core errors if TFT_BL is written before pinMode).
    pinMode(LCD_BL, OUTPUT);
    digitalWrite(LCD_BL, HIGH);

    // Display: ST7789 via TFT_eSPI, fixed landscape.
    tft.init();
    tft.setRotation(LCD_ROTATION);
    tft.fillScreen(TFT_BLACK);

    // IO expanders first (touch reset + buttons live behind them).
    aw9523_init();

    // Battery fuel gauge (MAX17048 on the Power module).
    power_init();

    // Touch (FT6636; hardware-reset via AW9523_B P13 inside ft6636_init).
    ft6636_init();

    // LVGL
    lv_init();
    lv_tick_set_cb(my_tick);

    buf1 = (uint16_t*)heap_caps_malloc(LCD_WIDTH * BUF_LINES * 2, MALLOC_CAP_SPIRAM);
    buf2 = (uint16_t*)heap_caps_malloc(LCD_WIDTH * BUF_LINES * 2, MALLOC_CAP_SPIRAM);
    if (!buf1 || !buf2) {
        Serial.println("FATAL: LVGL draw buffer alloc failed (PSRAM?)");
        while (true) { delay(1000); }
    }

    lv_display_t* disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, my_flush_cb);
    lv_display_set_buffers(disp, buf1, buf2, LCD_WIDTH * BUF_LINES * 2,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touch_cb);

    ble_init();
    buttons_init();

    ui_init();
    ui_update_ble_status(ble_get_state(), ble_get_device_name(), ble_get_mac_address());
    ui_update_battery(power_battery_pct(), power_is_charging());
    ui_show_screen(SCREEN_SPLASH);

    Serial.println("Dashboard ready, waiting for data on BLE...");
}

static ble_state_t last_ble_state = BLE_STATE_INIT;

void loop() {
    touch_read();
    lv_timer_handler();
    ui_tick_anim();
    ble_tick();
    power_tick();
    buttons_tick();
    splash_tick();

    // Three-button input (global, screen-independent):
    //   LEFT   → Space (voice-mode push-to-talk; press & release tracked)
    //   RIGHT  → Shift+Tab (Claude Code mode toggle)
    //   SELECT → cycle screens; on splash, cycle animations
    {
        static bool left_was = false, right_was = false;
        bool left_now  = buttons_left_down();
        bool right_now = buttons_right_down();

        if (left_now != left_was) {
            if (left_now) ble_keyboard_press(0x2C, 0);     // HID Space, no mods
            else          ble_keyboard_release();
            left_was = left_now;
        }
        if (right_now != right_was) {
            if (right_now) ble_keyboard_press(0x2B, 0x02);  // HID Tab + LEFT_SHIFT
            else           ble_keyboard_release();
            right_was = right_now;
        }

        if (buttons_select_pressed()) {
            if (ui_get_current_screen() == SCREEN_SPLASH) splash_next();
            else                                          ui_cycle_screen();
        }
    }

    // Update BLE status on screen when state changes
    ble_state_t bs = ble_get_state();
    if (bs != last_ble_state) {
        last_ble_state = bs;
        ui_update_ble_status(bs, ble_get_device_name(), ble_get_mac_address());
    }

    // Update battery indicator
    static int last_pct = -2;
    static bool last_charging = false;
    int pct = power_battery_pct();
    bool charging = power_is_charging();
    if (pct != last_pct || charging != last_charging) {
        last_pct = pct;
        last_charging = charging;
        ui_update_battery(pct, charging);
    }

    check_serial_cmd();

    if (ble_has_data()) {
        if (parse_json(ble_get_data(), &usage)) {
            int g_before = usage_rate_group();
            usage_rate_sample(usage.session_pct);
            int g_after = usage_rate_group();
            if (g_after != g_before) {
                Serial.printf("usage rate: group %d -> %d (s=%.2f%%)\n",
                    g_before, g_after, usage.session_pct);
                if (splash_is_active()) splash_pick_for_current_rate();
            }
            ui_update(&usage);
            ble_send_ack();
        } else {
            ble_send_nack();
        }
    }

    delay(5);
}
