#pragma once
#include <stdbool.h>

// Input map (hardware-verified):
//   D-pad UP/DOWN/LEFT/RIGHT → keyboard arrow keys (HID, level-tracked)
//   SELECT                   → Enter / confirm (HID)
//   BTN_L1                   → Shift+Tab (HID)
//   BTN_R2                   → Space (HID)
//   BOOT_BTN (IO38)          → UI nav: cycle screen / next splash anim
//   POWER_BTN                → hardware power toggle, not exposed
//
// D-pad + SELECT + L1 + R2 are on AW9523_B (@0x59) Port0, active LOW.
// Physical LEFT/RIGHT are mirrored vs the board-internal P01/P03 labels (board-
// internal view); the mirror is resolved inside buttons.cpp.

typedef enum {
    BTN_UP = 0,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_SELECT,
    BTN_L1,
    BTN_R2,
    BTN_HID_COUNT
} btn_id_t;

void buttons_init(void);              // sets up the BOOT_BTN GPIO
void buttons_tick(void);              // call once per loop()

bool buttons_down(btn_id_t b);        // debounced level for a HID button
bool buttons_boot_pressed(void);      // BOOT_BTN one-shot (consumes edge)
