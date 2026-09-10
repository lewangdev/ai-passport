#include "toolkit_model.h"
#include "toolkit_view.h"
#include "bsp_i2c.h"
#include "bsp_display.h"
#include "bsp_button.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "esp_log.h"
#include "esp_random.h"
#include "esp_timer.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include <string.h>

static tk_model model;
static tk_status status = {.battery = -1, .storage = 0};
static lv_obj_t *screen;
/* All queues carry values, never UI pointers. Worker lives for the application lifetime. */
typedef struct { tk_save save; uint32_t generation; } save_job;
typedef struct { int battery, storage; bool audio, loaded, save_valid; tk_save save; uint32_t generation; } io_result;
typedef struct { uint8_t volume, kind; } sound_job;
static QueueHandle_t keys, saves, sounds, results;
static uint32_t generation, pending_generation;
static bool loaded, needs_save, io_available;
static int64_t last_tick, save_at;

static void on_button(bsp_btn_t button, bsp_btn_ev_t event, void *context) {
    (void)context;
    tk_key key;
    if (event == BSP_BTN_LONG && button == BSP_BTN_OK) key = TK_BACK;
    else if (button != BSP_BTN_OK && event == BSP_BTN_PRESS) key = button == BSP_BTN_UP ? TK_UP : TK_DOWN;
    else if (button == BSP_BTN_OK && (event == BSP_BTN_CLICK || event == BSP_BTN_DOUBLE)) key = TK_OK;
    else return;
    (void)xQueueSend(keys, &key, 0);
}
/* Short original triangle-wave chirps, including release and silence to avoid clicks. */
static bool play_sound(sound_job job) {
    if (!job.volume) return true;
    bsp_audio_set_volume(job.volume);
    int16_t pcm[160]; uint32_t phase = 0;
    for (int block = 0; block < (job.kind ? 16 : 7); ++block) {
        unsigned hz = job.kind ? (block < 6 ? 660 : 880) : 740;
        for (int i = 0; i < 160; ++i) {
            unsigned t = block * 160 + i, length = job.kind ? 1920 : 480;
            phase += hz * 65536 / 16000;
            int tri = (phase & 65535) < 32768 ? (int)(phase & 65535) - 16384 : 49152 - (int)(phase & 65535);
            unsigned env = t < 80 ? t : t < length ? (length - t < 160 ? (length - t) / 2 : 80) : 0;
            pcm[i] = tri * (int)env / 320;
        }
        if (bsp_audio_write(pcm, sizeof(pcm)) != ESP_OK) return false;
    }
    return true;
}
static void io_worker(void *argument) {
    (void)argument;
    io_result result = {.battery = -1, .storage = -1};
    nvs_handle_t handle = 0;
    bool nvs_ok = nvs_flash_init() == ESP_OK && nvs_open("toolkit", NVS_READWRITE, &handle) == ESP_OK;
    if (nvs_ok) {
        uint8_t bytes[TK_SAVE_BYTES]; size_t size = sizeof(bytes);
        esp_err_t err = nvs_get_blob(handle, "state", bytes, &size);
        result.save_valid = err == ESP_OK && size == sizeof(bytes) && tk_decode(&result.save, bytes);
        /* A missing save is normal. An invalid one is reported, not silently erased. */
        result.storage = result.save_valid || err == ESP_ERR_NVS_NOT_FOUND ? 1 : -1;
        if (result.storage < 0) nvs_ok = false; /* Preserve unreadable/future-version data. */
    }
    result.loaded = true; xQueueSend(results, &result, portMAX_DELAY);
    result.loaded = false;
    result.audio = bsp_audio_init() == ESP_OK && bsp_audio_set_format(16000, 16, 1) == ESP_OK;
    bool battery_ok = bsp_battery_init() == ESP_OK;
    if (battery_ok) result.battery = bsp_battery_soc();
    xQueueSend(results, &result, portMAX_DELAY);
    int64_t battery_at = esp_timer_get_time();
    for (;;) {
        save_job save;
        if (xQueueReceive(saves, &save, 0) == pdTRUE) {
            uint8_t bytes[TK_SAVE_BYTES]; tk_encode(&save.save, bytes);
            result.storage = nvs_ok && nvs_set_blob(handle, "state", bytes, sizeof(bytes)) == ESP_OK && nvs_commit(handle) == ESP_OK ? 1 : -1;
            result.generation = save.generation; xQueueSend(results, &result, portMAX_DELAY);
        }
        sound_job sound;
        if (xQueueReceive(sounds, &sound, pdMS_TO_TICKS(20)) == pdTRUE && result.audio) {
            if (!play_sound(sound)) { result.audio = false; xQueueSend(results, &result, portMAX_DELAY); }
        }
        if (esp_timer_get_time() - battery_at >= 5000000) {
            result.battery = battery_ok ? bsp_battery_soc() : -1;
            battery_at = esp_timer_get_time(); xQueueSend(results, &result, portMAX_DELAY);
        }
    }
}
/* This callback belongs to LVGL. It is the only owner of model and screen mutations. */
static void ui_tick(lv_timer_t *timer) {
    (void)timer;
    bool redraw = false;
    io_result result;
    while (xQueueReceive(results, &result, 0) == pdTRUE) {
        status.battery = result.battery; status.audio = result.audio;
        if (result.loaded) {
            if (result.save_valid) model.save = result.save;
            loaded = true; status.storage = result.storage;
            bsp_display_backlight(model.save.brightness);
            if (result.storage < 0) tk_message(&model, "无法读取存档，使用临时数据");
            else if (!result.save_valid) { model.dirty = true; }
        } else if (result.generation == pending_generation && !needs_save) status.storage = result.storage;
        redraw = true;
    }
    int64_t now = esp_timer_get_time();
    if (now - last_tick >= 1000000) {
        uint32_t elapsed = (now - last_tick) / 1000000;
        uint8_t previous_alert = model.alert;
        tk_tick(&model, elapsed); last_tick += (int64_t)elapsed * 1000000; redraw = true;
        if (model.alert != previous_alert) { sound_job sound = {model.save.volume, 1}; xQueueOverwrite(sounds, &sound); }
    }
    tk_key key;
    while (xQueueReceive(keys, &key, 0) == pdTRUE) {
        if (!loaded) continue;
        uint8_t brightness = model.save.brightness;
        tk_keypress(&model, key);
        if (brightness != model.save.brightness) bsp_display_backlight(model.save.brightness);
        sound_job sound = {model.save.volume, 0}; xQueueOverwrite(sounds, &sound); redraw = true;
    }
    if (model.dirty && loaded && io_available) {
        model.dirty = false;
        if (!needs_save) save_at = now + 2000000;
        needs_save = true; status.storage = 0;
    }
    if (needs_save && now >= save_at) {
        save_job job = {model.save, ++generation};
        pending_generation = generation; xQueueOverwrite(saves, &job); needs_save = false;
    }
    if (redraw) lv_obj_invalidate(screen);
}
void app_main(void) {
    keys = xQueueCreate(12, sizeof(tk_key)); saves = xQueueCreate(1, sizeof(save_job));
    sounds = xQueueCreate(1, sizeof(sound_job)); results = xQueueCreate(8, sizeof(io_result));
    if (!keys || !saves || !sounds || !results) { ESP_LOGE("toolkit", "Queue allocation failed"); return; }
    tk_init(&model, esp_random());
    if (bsp_display_init() != ESP_OK || !bsp_lvgl_init()) { ESP_LOGE("toolkit", "Display unavailable"); return; }
    status.buttons = bsp_button_init(on_button, NULL) == ESP_OK;
    last_tick = esp_timer_get_time();
    if (!bsp_lvgl_lock(1000)) { ESP_LOGE("toolkit", "UI lock unavailable"); return; }
    screen = tk_view_create(&model, &status); lv_timer_create(ui_tick, 50, NULL);
    bsp_lvgl_unlock(); bsp_display_backlight(model.save.brightness);
    if (bsp_lvgl_lock(1000)) {
        io_available = xTaskCreate(io_worker, "toolkit_io", 6144, NULL, 4, NULL) == pdPASS;
        bsp_lvgl_unlock();
    }
    if (!io_available) {
        if (bsp_lvgl_lock(1000)) { loaded = true; status.storage = -1; tk_message(&model, "存档和音效不可用"); bsp_lvgl_unlock(); }
    }
}
