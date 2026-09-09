#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "dagou_app.h"
#include "esp_log.h"

void app_main(void) {
    bsp_i2c_init();
    if(bsp_display_init()!=ESP_OK || !bsp_lvgl_init()) {
        ESP_LOGE("dagou","Display initialization failed");
        return;
    }
    if(bsp_audio_init()!=ESP_OK)ESP_LOGW("dagou","Audio unavailable");
    if(bsp_battery_init()!=ESP_OK)ESP_LOGW("dagou","Battery unavailable");
    dagou_app_start();
}
