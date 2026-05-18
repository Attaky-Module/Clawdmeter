#include "buttons.h"
#include "aw9523.h"
#include "display_cfg.h"
#include <Arduino.h>

#define POLL_MS      15
#define DEBOUNCE_MS  30

struct Btn {
    uint8_t  bit;
    bool     stable;        // debounced level (true = pressed)
    bool     last_raw;
    uint32_t last_change_ms;
};

static Btn left   = { BTN_BIT_LEFT,   false, false, 0 };
static Btn right  = { BTN_BIT_RIGHT,  false, false, 0 };
static Btn sel    = { BTN_BIT_SELECT, false, false, 0 };

static bool     select_edge = false;
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
        if (b == &sel && raw) select_edge = true;  // press edge only
    }
}

void buttons_init(void) {
    // AW9523 is brought up by aw9523_init() in main setup().
}

void buttons_tick(void) {
    uint32_t now = millis();
    if (now - last_poll_ms < POLL_MS) return;
    last_poll_ms = now;

    uint8_t p0 = aw9523_buttons_raw();
    debounce(&left,   p0, now);
    debounce(&right,  p0, now);
    debounce(&sel,    p0, now);
}

bool buttons_left_down(void)  { return left.stable; }
bool buttons_right_down(void) { return right.stable; }

bool buttons_select_pressed(void) {
    if (select_edge) { select_edge = false; return true; }
    return false;
}
