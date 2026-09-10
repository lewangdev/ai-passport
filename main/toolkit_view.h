#pragma once
#include "lvgl.h"
#include "toolkit_model.h"
/* UI reads model only in the LVGL task. Worker status arrives via queues. */
typedef struct { int battery, storage; bool audio, buttons; } tk_status;
lv_obj_t *tk_view_create(const tk_model *model, const tk_status *status);
