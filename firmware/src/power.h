#pragma once

void power_init(void);
void power_tick(void);
int  power_battery_pct(void);    // 0-100, or -1 if fuel gauge absent
bool power_is_charging(void);    // MAX17048 charge-rate > 0 (also USB proxy)
bool power_pwr_pressed(void);    // always false on Core (no PMU PWR IRQ);
                                 // screen-cycle moved to the SELECT button
