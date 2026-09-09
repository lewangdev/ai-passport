#include "niuma_sound.h"
#include "bsp_audio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static QueueHandle_t effects;
static portMUX_TYPE audio_guard=portMUX_INITIALIZER_UNLOCKED;
static uint8_t requested_volume=40;
static bool requested_music,requested_sleep,ready;
static void audio_task(void *arg){
    (void)arg;
    bool ok=bsp_audio_init()==ESP_OK && bsp_audio_set_format(16000,16,1)==ESP_OK;
    portENTER_CRITICAL(&audio_guard);ready=ok;portEXIT_CRITICAL(&audio_guard);
    if(!ok){vTaskDelete(NULL);return;}
    nm_synth_t synth={0};uint8_t last_volume=255;int16_t pcm[256];
    for(;;){
        portENTER_CRITICAL(&audio_guard);
        uint8_t volume=requested_volume;bool music=requested_music,sleeping=requested_sleep;
        portEXIT_CRITICAL(&audio_guard);
        if(volume!=last_volume){bsp_audio_set_volume(volume);last_volume=volume;}
        nm_sfx_t effect;
        TickType_t wait=(!synth.active && (!music || sleeping || !volume))?pdMS_TO_TICKS(50):0;
        if(xQueueReceive(effects,&effect,wait)==pdTRUE && !sleeping && volume)nm_synth_trigger(&synth,effect);
        if(!synth.active && (!music || sleeping || !volume))continue;
        nm_synth_render(&synth,pcm,256,music,sleeping || !volume);
        if(bsp_audio_write(pcm,sizeof(pcm))!=ESP_OK){
            portENTER_CRITICAL(&audio_guard);ready=false;portEXIT_CRITICAL(&audio_guard);
            vTaskDelete(NULL);return;
        }
        vTaskDelay(1); /* Yield even if DMA accepts a chunk immediately. */
    }
}
bool nm_audio_start(void){
    if(effects)return true;
    effects=xQueueCreate(8,sizeof(nm_sfx_t));
    if(!effects)return false;
    if(xTaskCreate(audio_task,"nm_audio",4096,NULL,3,NULL)!=pdPASS){vQueueDelete(effects);effects=NULL;return false;}
    return true;
}
bool nm_audio_ready(void){portENTER_CRITICAL(&audio_guard);bool value=ready;portEXIT_CRITICAL(&audio_guard);return value;}
void nm_audio_config(uint8_t volume,bool music,bool sleeping){
    portENTER_CRITICAL(&audio_guard);requested_volume=volume>100?100:volume;requested_music=music;requested_sleep=sleeping;portEXIT_CRITICAL(&audio_guard);
}
void nm_audio_play(nm_sfx_t effect){if(effects && (unsigned)effect<NM_SFX_COUNT)xQueueSend(effects,&effect,0);}
