#pragma once
#include <stdint.h>

// IMU auto-rotation was dropped for the Attaky Core port (fixed landscape,
// setRotation(3)). QMI8658A is present on the bus but unused here. These
// remain as no-ops so callers/includes stay valid.
void    imu_init(void);
void    imu_tick(void);
uint8_t imu_get_rotation(void);  // always 0 (fixed orientation)
