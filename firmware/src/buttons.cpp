#include "buttons.h"
#include "aw9523.h"
#include "display_cfg.h"
#include <Arduino.h>

#define POLL_MS      15
#define DEBOUNCE_MS  30

struct Btn {
    uint8_t  bit;           // AW9523_B Port0 bit
    bool     stable;        // debounced level (true = pressed)
    bool     last_raw;
    uint32_t last_change_ms;
};

// User-facing button → AW9523_B (@0x59) Port0 bit.
// The host pinmap labels P01=RIGHT / P03=LEFT from the board-
// internal view, which is MIRRORED vs how the user faces the device.
// Hardware-verified: the physically-left key is
// P01 and the physically-right key is P03. UP/DOWN are not affected by a
// left/right mirror. the board pin labelling is left unchanged here.
static Btn btns[BTN_HID_COUNT] = {
    [BTN_UP]     = { BTN_BIT_UP,     false, false, 0 },  // P04
    [BTN_DOWN]   = { BTN_BIT_DOWN,   false, false, 0 },  // P00
    [BTN_LEFT]   = { BTN_BIT_RIGHT,  false, false, 0 },  // P01 (mirror)
    [BTN_RIGHT]  = { BTN_BIT_LEFT,   false, false, 0 },  // P03 (mirror)
    [BTN_SELECT] = { BTN_BIT_SELECT, false, false, 0 },  // P02
    [BTN_L1]     = { BTN_BIT_L1,     false, false, 0 },  // P05
    [BTN_R2]     = { BTN_BIT_R2,     false, false, 0 },  // P06
};

// BOOT_BTN (IO38, direct GPIO, active LOW) — debounced one-shot for UI nav.
static bool     boot_stable = false;
static bool     boot_last_raw = false;
static uint32_t boot_change_ms = 0;
static bool     boot_edge = false;

static uint32_t last_poll_ms = 0;

static void debounce(Btn *b, uint8_t port0, uint32_t now) {
    bool raw = (port0 & (1 << b->bit)) == 0;  // active low
    if (raw != b->last_raw) {
        b->last_raw = raw;
        b->last_change_ms = now;
        return;
    }
    if (raw != b->stable && (now - b->last_change_ms) >= DEBOUNCE_MS) {
        b->stable = raw;
    }
}

void buttons_init(void) {
    // AW9523 itself is brought up by aw9523_init() in main setup().
    pinMode(BOOT_BTN_PIN, INPUT_PULLUP);  // external pullup; LOW = pressed
}

void buttons_tick(void) {
    uint32_t now = millis();
    if (now - last_poll_ms < POLL_MS) return;
    last_poll_ms = now;

    uint8_t p0 = aw9523_buttons_raw();
    for (int i = 0; i < BTN_HID_COUNT; i++) debounce(&btns[i], p0, now);

    // BOOT_BTN (separate GPIO, not on the expander)
    bool braw = (digitalRead(BOOT_BTN_PIN) == LOW);
    if (braw != boot_last_raw) {
        boot_last_raw = braw;
        boot_change_ms = now;
    } else if (braw != boot_stable && (now - boot_change_ms) >= DEBOUNCE_MS) {
        boot_stable = braw;
        if (braw) boot_edge = true;  // press edge only
    }
}

bool buttons_down(btn_id_t b) {
    if (b < 0 || b >= BTN_HID_COUNT) return false;
    return btns[b].stable;
}

bool buttons_boot_pressed(void) {
    if (boot_edge) { boot_edge = false; return true; }
    return false;
}
