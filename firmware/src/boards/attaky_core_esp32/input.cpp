#include "../../hal/input_hal.h"
#include "board.h"
#include "io_expander.h"
#include <Arduino.h>

#define POLL_MS      15
#define DEBOUNCE_MS  30

struct ButtonState {
    uint8_t bit;
    bool stable;
    bool last_raw;
    uint32_t last_change_ms;
};

static ButtonState expander_buttons[INPUT_BTN_COUNT] = {
    [INPUT_BTN_PRIMARY]   = { 0xFF, false, false, 0 },
    [INPUT_BTN_SECONDARY] = { 0xFF, false, false, 0 },
    [INPUT_BTN_UP]        = { BTN_BIT_UP,     false, false, 0 },
    [INPUT_BTN_DOWN]      = { BTN_BIT_DOWN,   false, false, 0 },
    // LEFT/RIGHT intentionally cross the BTN_BIT_* names: the board-internal
    // pin labels are mirrored vs the physical key positions (physically-left
    // key = P01 = BTN_BIT_RIGHT, physically-right = P03 = BTN_BIT_LEFT),
    // verified on hardware. Same compensation as the pre-HAL port.
    [INPUT_BTN_LEFT]      = { BTN_BIT_RIGHT,  false, false, 0 },
    [INPUT_BTN_RIGHT]     = { BTN_BIT_LEFT,   false, false, 0 },
    [INPUT_BTN_SELECT]    = { BTN_BIT_SELECT, false, false, 0 },
    [INPUT_BTN_L1]        = { BTN_BIT_L1,     false, false, 0 },
    [INPUT_BTN_R2]        = { BTN_BIT_R2,     false, false, 0 },
    [INPUT_BTN_UI_NAV]    = { 0xFF, false, false, 0 },
};

static bool boot_stable = false;
static bool boot_last_raw = false;
static uint32_t boot_change_ms = 0;
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
    for (int i = INPUT_BTN_UP; i <= INPUT_BTN_R2; i++) {
        ButtonState* b = &expander_buttons[i];
        debounce(b, (p0 & (1 << b->bit)) == 0, now);
    }

    bool boot_raw = digitalRead(BOOT_BTN_PIN) == LOW;
    if (boot_raw != boot_last_raw) {
        boot_last_raw = boot_raw;
        boot_change_ms = now;
    } else if (boot_raw != boot_stable && (now - boot_change_ms) >= DEBOUNCE_MS) {
        boot_stable = boot_raw;
    }
}

void input_hal_init(void) {
    pinMode(BOOT_BTN_PIN, INPUT_PULLUP);
}

bool input_hal_is_held(InputButton btn) {
    poll_buttons();
    if (btn == INPUT_BTN_UI_NAV) return boot_stable;
    if (btn < 0 || btn >= INPUT_BTN_COUNT) return false;
    return expander_buttons[btn].stable;
}
