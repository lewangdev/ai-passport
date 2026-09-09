#pragma once
#include <stdbool.h>
#include "bsp_button.h"
bool niuma_app_start(void);
void niuma_app_key(bsp_btn_t button,bsp_btn_ev_t event,void *user);
