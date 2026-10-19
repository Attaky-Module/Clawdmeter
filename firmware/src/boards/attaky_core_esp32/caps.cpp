#include "../../hal/board_caps.h"
#include "board.h"

static const BoardCaps caps = {
    .name = BOARD_NAME,
    .width = LCD_WIDTH,
    .height = LCD_HEIGHT,
    .round_panel = false,
    // R2 = PRIMARY (Space), L1 = SECONDARY (Shift+Tab); D-pad and SELECT
    // are extended InputButton values.
    .button_count = 2,
    .has_rotation = (bool)BOARD_HAS_ROTATION,
    .has_battery  = (bool)BOARD_HAS_BATTERY,
    .has_imu      = (bool)BOARD_HAS_IMU,
    .minimal_ui   = false,
    .has_pwr_button = true,
    .has_touch = true,
    .ble_name = "Attaky Claude Monitor",
    // POWER long-hold is a hardware power toggle on this board, so
    // hold-to-pair lives on BOOT (see power.cpp).
    .pair_on_boot = true,
};

const BoardCaps& board_caps(void) { return caps; }
