#pragma once
#include <stdbool.h>
#include <stdint.h>
bool bsp_lvgl_lock(int timeout);
void bsp_lvgl_unlock(void);
void bsp_display_backlight(uint8_t value);
