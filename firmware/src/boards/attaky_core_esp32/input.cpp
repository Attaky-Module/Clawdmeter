#include "../../hal/input_hal.h"
#include "board.h"
#include "io_expander.h"
#include <Arduino.h>

#define POLL_MS      15
#define DEBOUNCE_MS  30
#define NO_BIT       0xFF

struct ButtonState {
    uint8_t bit;          // AW9523_B Port0 bit, or NO_BIT for BOOT (GPIO)
    bool stable;
    bool last_raw;
    uint32_t last_change_ms;
};

static ButtonState buttons[INPUT_BTN_COUNT] = {
    [INPUT_BTN_PRIMARY]   = { BTN_BIT_R2,     false, false, 0 },
    [INPUT_BTN_SECONDARY] = { BTN_BIT_L1,     false, false, 0 },
    [INPUT_BTN_UP]        = { BTN_BIT_UP,     false, false, 0 },
    [INPUT_BTN_DOWN]      = { BTN_BIT_DOWN,   false, false, 0 },
    // LEFT/RIGHT intentionally cross the BTN_BIT_* names: the board-internal
    // pin labels are mirrored vs the physical key positions (physically-left
    // key = P01 = BTN_BIT_RIGHT, physically-right = P03 = BTN_BIT_LEFT),
    // verified on hardware.
    [INPUT_BTN_LEFT]      = { BTN_BIT_RIGHT,  false, false, 0 },
    [INPUT_BTN_RIGHT]     = { BTN_BIT_LEFT,   false, false, 0 },
    [INPUT_BTN_SELECT]    = { BTN_BIT_SELECT, false, false, 0 },
    [INPUT_BTN_UI_NAV]    = { NO_BIT,         false, false, 0 },
};

static uint32_t last_poll_ms = 0;

static void debounce(ButtonState* b, bool raw, uint32_t now) {
    if (raw != b->last_raw) {
        b->last_raw = raw;
        b->last_change_ms = now;
        return;
    }
    if (raw != b->stable && (now - b->last_change_ms) >= DEBOUNCE_MS) {
        b->stable = raw;
    }
}

static void poll_buttons(void) {
    uint32_t now = millis();
    if (now - last_poll_ms < POLL_MS) return;
    last_poll_ms = now;

    uint8_t p0 = io_expander_buttons_raw();
    for (int i = 0; i < INPUT_BTN_COUNT; i++) {
        ButtonState* b = &buttons[i];
        bool raw = (b->bit == NO_BIT) ? digitalRead(BOOT_BTN_PIN) == LOW
                                      : (p0 & (1 << b->bit)) == 0;
        debounce(b, raw, now);
    }
}

void input_hal_init(void) {
    pinMode(BOOT_BTN_PIN, INPUT_PULLUP);
}

bool input_hal_is_held(InputButton btn) {
    if (btn < 0 || btn >= INPUT_BTN_COUNT) return false;
    poll_buttons();
    return buttons[btn].stable;
}
