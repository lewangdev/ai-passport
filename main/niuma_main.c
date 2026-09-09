#include "niuma_app.h"
#include "niuma_sound.h"
#include "niuma_storage.h"
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "sdkconfig.h"

_Static_assert(CONFIG_BUTTON_LONG_PRESS_TIME_MS == 2000,
               "NiuMa requires a two-second hold; regenerate sdkconfig from defaults");

void app_main(void){
    ESP_LOGI("niuma","NiuMa office pet starting");
    if(bsp_i2c_init()!=ESP_OK)ESP_LOGW("niuma","Shared I2C initialization failed");
    if(bsp_display_init()!=ESP_OK || !bsp_lvgl_init()){
        ESP_LOGE("niuma","Display initialization failed");return;
    }
    nm_storage_start();nm_audio_start();
    bool ready=false;
    if(bsp_lvgl_lock(1000)){ready=niuma_app_start();bsp_lvgl_unlock();}
    if(!ready){ESP_LOGE("niuma","UI initialization failed");return;}
    if(bsp_button_init(niuma_app_key,NULL)!=ESP_OK)ESP_LOGE("niuma","Button initialization failed");
    bsp_display_backlight(60);
}
