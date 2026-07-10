#include "../../hal/board_caps.h"
#include "board.h"

static const BoardCaps caps = {
    .name = BOARD_NAME,
    .ble_name = "Attaky Claude Monitor",
    // POWER long-hold is a hardware power toggle on this board, so
    // hold-to-pair lives on BOOT (see boards/attaky_core_esp32/power.cpp).
    .pair_button_label = "the BOOT button",
    .width = LCD_WIDTH,
    .height = LCD_HEIGHT,
    .button_count = 7,
    .has_rotation = false,
    .has_battery = true,
    .has_imu = false,
};

const BoardCaps& board_caps(void) { return caps; }
