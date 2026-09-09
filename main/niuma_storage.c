#include "niuma_storage.h"
#include "niuma_save.h"
#include "bsp_battery.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include <string.h>

static QueueHandle_t queue;
typedef struct {nm_state_t state;uint32_t ticket;} save_request_t;
static uint32_t latest_ticket;
static portMUX_TYPE guard=portMUX_INITIALIZER_UNLOCKED;
static nm_store_status_t status={.phase=NM_STORE_LOADING,.battery=-1};
static nm_state_t restored;
static bool have_restored;
static bool reload_requested;
static const char *const keys[2]={"pet_a","pet_b"};

static void phase(nm_store_phase_t p){portENTER_CRITICAL(&guard);status.phase=p;portEXIT_CRITICAL(&guard);}
static bool newer(uint32_t a,uint32_t b){uint32_t d=a-b;return d && d<UINT32_C(0x80000000);}
static void load_slots(nvs_handle_t handle,int *active,uint32_t *generation){
    nm_state_t candidates[2];bool valid[2]={0},existing[2]={0};
    *active=-1;*generation=0;
    for(unsigned i=0;i<2;++i){
        uint8_t bytes[NM_SAVE_CAPACITY];size_t n=sizeof(bytes);
        esp_err_t read=nvs_get_blob(handle,keys[i],bytes,&n);
        existing[i]=read!=ESP_ERR_NVS_NOT_FOUND;
        valid[i]=read==ESP_OK && nm_save_decode(&candidates[i],bytes,n);
    }
    if(valid[0])*active=0;
    if(valid[1] && (*active<0 || newer(candidates[1].revision,candidates[0].revision)))*active=1;
    portENTER_CRITICAL(&guard);
    have_restored=false;
    if(*active>=0){
        restored=candidates[*active];have_restored=true;*generation=restored.revision;
        status.saved_revision=*generation;status.phase=NM_STORE_SAVED;
        status.recovered=(existing[0]&&!valid[0]) || (existing[1]&&!valid[1]);
    }else status.phase=(existing[0]||existing[1])?NM_STORE_ERROR:NM_STORE_EMPTY;
    portEXIT_CRITICAL(&guard);
}
static void storage_task(void *arg){
    (void)arg;
    nvs_handle_t handle=0;
    esp_err_t err=nvs_flash_init(); /* Never erase unrelated NVS on failure. */
    if(err==ESP_OK)err=nvs_open("niuma",NVS_READWRITE,&handle);
    int active=-1;uint32_t generation=0;
    if(err==ESP_OK)load_slots(handle,&active,&generation);
    else {ESP_LOGE("niuma","Storage init failed: %s",esp_err_to_name(err));phase(NM_STORE_ERROR);}

    /* Gauge reads block and therefore stay in this service, outside LVGL. */
    bool battery_ready=bsp_battery_init()==ESP_OK;
    TickType_t last_battery=0;
    for(;;){
        portENTER_CRITICAL(&guard);bool reload=reload_requested;reload_requested=false;portEXIT_CRITICAL(&guard);
        if(reload){
            if(err!=ESP_OK){err=nvs_flash_init();if(err==ESP_OK)err=nvs_open("niuma",NVS_READWRITE,&handle);}
            if(err==ESP_OK)load_slots(handle,&active,&generation);else phase(NM_STORE_ERROR);
        }
        save_request_t request;
        if(xQueueReceive(queue,&request,pdMS_TO_TICKS(250))==pdTRUE){
            nm_state_t snapshot=request.state;
            uint32_t requested=snapshot.revision;
            if(err!=ESP_OK){
                err=nvs_flash_init();
                if(err==ESP_OK)err=nvs_open("niuma",NVS_READWRITE,&handle);
                /* An initialization retry must discover the active slot and
                   generation before writing; otherwise a low-revision new
                   snapshot can be silently superseded on the next boot. */
                if(err==ESP_OK)load_slots(handle,&active,&generation);
            }
            if(err==ESP_OK){
                if(!newer(snapshot.revision,generation))snapshot.revision=generation+1;
                uint8_t bytes[NM_SAVE_CAPACITY],verify[NM_SAVE_CAPACITY];
                size_t n=nm_save_encode(&snapshot,bytes,sizeof(bytes));
                int target=active==0?1:0;
                esp_err_t write=n?nvs_set_blob(handle,keys[target],bytes,n):ESP_ERR_INVALID_ARG;
                if(write==ESP_OK)write=nvs_commit(handle);
                size_t actual=sizeof(verify);
                if(write==ESP_OK)write=nvs_get_blob(handle,keys[target],verify,&actual);
                bool ok=write==ESP_OK && actual==n && !memcmp(bytes,verify,n);
                portENTER_CRITICAL(&guard);
                status.phase=ok?NM_STORE_SAVED:NM_STORE_ERROR;
                /* A newer submission may be announced but not yet published
                 * to the queue. Queue emptiness cannot acknowledge that work. */
                status.pending=request.ticket!=latest_ticket;
                if(ok)status.saved_revision=requested;
                portEXIT_CRITICAL(&guard);
                if(ok){generation=snapshot.revision;active=target;}
            }else {phase(NM_STORE_ERROR);portENTER_CRITICAL(&guard);status.pending=request.ticket!=latest_ticket;portEXIT_CRITICAL(&guard);}
        }
        TickType_t now=xTaskGetTickCount();
        if(!last_battery || now-last_battery>=pdMS_TO_TICKS(10000)){
            int battery=battery_ready?bsp_battery_soc():-1;
            portENTER_CRITICAL(&guard);status.battery=battery;portEXIT_CRITICAL(&guard);
            last_battery=now;
        }
    }
}
bool nm_storage_start(void){
    if(queue)return true;
    queue=xQueueCreate(1,sizeof(save_request_t));
    if(!queue){phase(NM_STORE_ERROR);return false;}
    if(xTaskCreate(storage_task,"nm_store",5120,NULL,2,NULL)!=pdPASS){
        vQueueDelete(queue);queue=NULL;phase(NM_STORE_ERROR);return false;
    }
    return true;
}
bool nm_storage_request(const nm_state_t *s){
    if(!queue || !nm_valid(s))return false;
    save_request_t request={.state=*s};
    portENTER_CRITICAL(&guard);request.ticket=++latest_ticket;status.pending=true;portEXIT_CRITICAL(&guard);
    bool accepted=xQueueOverwrite(queue,&request)==pdPASS;
    if(!accepted)phase(NM_STORE_ERROR); /* Retain unresolved status until retry. */
    return accepted;
}
nm_store_status_t nm_storage_status(void){
    portENTER_CRITICAL(&guard);nm_store_status_t copy=status;portEXIT_CRITICAL(&guard);return copy;
}
bool nm_storage_loaded(nm_state_t *s){
    portENTER_CRITICAL(&guard);bool ok=have_restored;if(ok)*s=restored;portEXIT_CRITICAL(&guard);return ok;
}
bool nm_storage_reload(void){
    if(!queue)return false;
    portENTER_CRITICAL(&guard);
    bool allowed=!status.pending;
    if(allowed){reload_requested=true;have_restored=false;status.phase=NM_STORE_LOADING;}
    portEXIT_CRITICAL(&guard);return allowed;
}
