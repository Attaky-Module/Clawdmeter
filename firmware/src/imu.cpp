#include "imu.h"

// No-op: auto-rotation removed for the Attaky Core port. Orientation is
// fixed to landscape via TFT_eSPI setRotation(3) in main.cpp.
void    imu_init(void) {}
void    imu_tick(void) {}
uint8_t imu_get_rotation(void) { return 0; }
