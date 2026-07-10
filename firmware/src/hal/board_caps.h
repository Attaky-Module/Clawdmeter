#pragma once
#include <stdint.h>

// Runtime board description consumed by board-agnostic code (UI, main loop).
// Each board provides a single BoardCaps instance via board_caps().
//
// Compile-time-only facts (pin numbers, library choice) belong in
// boards/<name>/board.h and never leak into shared code. Anything the UI or
// main loop needs at runtime — display size, optional-feature presence —
// goes here so shared code stays free of #ifdef BOARD_*.
struct BoardCaps {
    const char* name;        // human-readable, e.g. "Waveshare AMOLED 2.16"
    const char* ble_name;    // advertised BLE name, e.g. "Clawdmeter"
    // Button named by the on-screen pairing hint. NULL = "the power button"
    // (upstream boards). Boards whose power button is a hardware-level power
    // toggle route hold-to-pair to another button and set this label.
    const char* pair_button_label;

    int16_t width;           // active display width in pixels
    int16_t height;          // active display height in pixels

    uint8_t button_count;    // 1 = primary (BOOT) only; 2 = primary + secondary
    bool    has_rotation;    // IMU-driven CPU rotation in the flush callback
    bool    has_battery;     // AXP2101 battery measurement is meaningful
    bool    has_imu;         // QMI8658 (or compatible) is populated
};

const BoardCaps& board_caps(void);
