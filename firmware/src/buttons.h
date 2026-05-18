#pragma once
#include <stdbool.h>

// Physical buttons on AW9523_B (@0x59) Port0, active LOW, polled + debounced.
// Mapping for this port (hardware-verified):
//   LEFT   → Space      (Claude Code voice-mode push-to-talk; level-tracked)
//   RIGHT  → Shift+Tab  (Claude Code mode toggle; level-tracked)
//   SELECT → cycle screen / next splash animation (one-shot edge)
// POWER_BTN is a hardware power toggle and is intentionally not exposed.

void buttons_init(void);
void buttons_tick(void);          // call once per loop()

bool buttons_left_down(void);     // debounced level
bool buttons_right_down(void);    // debounced level
bool buttons_select_pressed(void);// true once per press (consumes the edge)
