#pragma once
#include <stdint.h>

// Physical button abstraction. Boards report up to two screen-independent
// buttons:
//   PRIMARY   — left button on this project's boards (BOOT / GPIO 0).
//               Drives the Claude Code voice-mode PTT (HID Space).
//   SECONDARY — right button on boards that have one (e.g. GPIO 18 on
//               AMOLED-2.16). Drives mode-toggle (HID Shift+Tab). Boards
//               without it report held=false forever and shared code
//               handles that gracefully via BoardCaps.button_count.
//
// The PWR button is owned by power_hal (it's tied to the PMU on some boards
// and to an IO expander on others — see power_hal_pwr_pressed()).

enum InputButton {
    INPUT_BTN_PRIMARY = 0,
    INPUT_BTN_SECONDARY = 1,
    INPUT_BTN_UP,
    INPUT_BTN_DOWN,
    INPUT_BTN_LEFT,
    INPUT_BTN_RIGHT,
    INPUT_BTN_SELECT,
    INPUT_BTN_L1,
    INPUT_BTN_R2,
    INPUT_BTN_UI_NAV,
    INPUT_BTN_COUNT,
};

void input_hal_init(void);

// True while the button is physically held (active-low GPIOs are
// de-bounced at the caller's expense — the existing code polls every
// loop iteration). Boards lacking a button always return false.
bool input_hal_is_held(InputButton btn);
