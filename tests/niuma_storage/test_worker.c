#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <string.h>
#include "niuma_save.h"
#include "niuma_storage.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"

typedef struct {uint8_t bytes[NM_SAVE_CAPACITY];size_t size;} slot_t;
static slot_t disk[2],staging;
static unsigned target,writes,commits,init_calls,open_calls;
static unsigned fail_init,fail_open,fail_write,fail_commit,torn_write,bad_readback;
static uint8_t queued[NM_SAVE_CAPACITY+32];
static size_t queue_item_size;
static bool waiting;
static bool finish_before_publish;
static bool publish_during_commit;
static void run(void);
static void (*worker)(void *);
static jmp_buf finished;
static int key_index(const char *key){assert(!strcmp(key,"pet_a")||!strcmp(key,"pet_b"));return !strcmp(key,"pet_b");}
QueueHandle_t xQueueCreate(unsigned count,size_t size){assert(count==1 && size<=sizeof(queued));queue_item_size=size;return &queued;}
void vQueueDelete(QueueHandle_t q){(void)q;}
int xQueueOverwrite(QueueHandle_t q,const void *in){
    assert(q==&queued);
    if(finish_before_publish){finish_before_publish=false;run();}
    memcpy(&queued,in,queue_item_size);waiting=true;return pdPASS;
}
unsigned uxQueueMessagesWaiting(QueueHandle_t q){assert(q==&queued);return waiting;}
int xQueueReceive(QueueHandle_t q,void *out,unsigned timeout){
    (void)timeout;assert(q==&queued);
    if(!waiting)longjmp(finished,1); /* Stop only at the worker's idle boundary. */
    memcpy(out,&queued,queue_item_size);waiting=false;return pdTRUE;
}
int xTaskCreate(void (*fn)(void *),const char *name,unsigned stack,void *arg,unsigned priority,void *handle){
    (void)name;(void)stack;(void)arg;(void)priority;(void)handle;worker=fn;return pdPASS;
}
TickType_t xTaskGetTickCount(void){return 10000;}
esp_err_t bsp_battery_init(void){return ESP_OK;}
int bsp_battery_soc(void){return 63;}
esp_err_t nvs_flash_init(void){++init_calls;return fail_init?--fail_init,9:ESP_OK;}
esp_err_t nvs_open(const char *name,int mode,nvs_handle_t *h){
    assert(!strcmp(name,"niuma") && mode==NVS_READWRITE);++open_calls;
    if(fail_open){--fail_open;return 9;}*h=1;return ESP_OK;
}
esp_err_t nvs_get_blob(nvs_handle_t h,const char *key,void *out,size_t *size){
    assert(h==1);slot_t *s=&disk[key_index(key)];if(!s->size)return ESP_ERR_NVS_NOT_FOUND;
    if(*size<s->size)return 9;
    memcpy(out,s->bytes,s->size);*size=s->size;
    if(bad_readback && writes){--bad_readback;((uint8_t *)out)[8]^=1;}
    return ESP_OK;
}
esp_err_t nvs_set_blob(nvs_handle_t h,const char *key,const void *in,size_t size){
    assert(h==1 && size<=NM_SAVE_CAPACITY);++writes;target=key_index(key);
    if(fail_write){--fail_write;return 9;}
    if(torn_write){--torn_write;memset(&disk[target],0,sizeof(slot_t));memcpy(disk[target].bytes,in,size/2);disk[target].size=size;return 9;}
    memcpy(staging.bytes,in,size);staging.size=size;return ESP_OK;
}
esp_err_t nvs_commit(nvs_handle_t h){
    assert(h==1);++commits;if(fail_commit){--fail_commit;return 9;}
    if(publish_during_commit){
        publish_during_commit=false;nm_state_t next;nm_init(&next,1,true,0);
        next.revision=2;next.coins=66;assert(nm_storage_request(&next));
        next.revision=3;next.coins=88;assert(nm_storage_request(&next));
    }
    disk[target]=staging;return ESP_OK;
}

/* Exercise the production worker, not a second implementation of its logic. */
#include "../../main/niuma_storage.c"

static void reboot(void){
    queue=NULL;status=(nm_store_status_t){.phase=NM_STORE_LOADING,.battery=-1};
    have_restored=false;reload_requested=false;latest_ticket=0;memset(&restored,0,sizeof(restored));waiting=false;
    writes=commits=init_calls=open_calls=0;worker=NULL;
    assert(nm_storage_start());
}
static void run(void){if(!setjmp(finished))worker(NULL);}
static void seed(unsigned slot,uint32_t revision,unsigned coins){
    nm_state_t s;nm_init(&s,12,true,0);s.revision=revision;s.coins=coins;
    disk[slot].size=nm_save_encode(&s,disk[slot].bytes,sizeof(disk[slot].bytes));assert(disk[slot].size);
}
static void reset_disk(void){memset(disk,0,sizeof(disk));fail_init=fail_open=fail_write=fail_commit=torn_write=bad_readback=0;}
int main(void){
    nm_state_t s;reset_disk();reboot();run();assert(status.phase==NM_STORE_EMPTY && !nm_storage_loaded(&s));
    seed(0,10,40);seed(1,11,50);reboot();run();assert(nm_storage_loaded(&s) && s.coins==50 && !status.recovered);
    for(unsigned fault=0;fault<4;++fault){
        reset_disk();seed(0,10,40);seed(1,11,50);slot_t protected=disk[1];
        reboot();nm_init(&s,12,true,0);s.revision=12;s.coins=60;
        if(fault==0)fail_write=1;
        if(fault==1)fail_commit=1;
        if(fault==2)torn_write=1;
        if(fault==3)bad_readback=1;
        assert(nm_storage_request(&s));run();assert(status.phase==NM_STORE_ERROR);
        assert(!memcmp(&protected,&disk[1],sizeof(protected)) && !status.pending);
        reboot();run();assert(nm_storage_loaded(&s));
        /* A durable write with failed readback may recover the new value;
           every other failure must retain the previous valid value. */
        assert(s.coins==(fault==3?60:50));
        if(fault==2)assert(status.recovered);
    }
    reset_disk();seed(0,UINT32_MAX,70);seed(1,0,80);reboot();run();assert(nm_storage_loaded(&s) && s.coins==80);
    reset_disk();seed(0,10,40);seed(1,11,50);disk[1].bytes[20]^=1;
    reboot();run();assert(nm_storage_loaded(&s) && s.coins==40 && status.recovered);
    disk[0].bytes[20]^=1;reboot();run();assert(!nm_storage_loaded(&s) && status.phase==NM_STORE_ERROR);
    reset_disk();reboot();nm_init(&s,2,true,0);s.revision=1;s.coins=70;assert(nm_storage_request(&s));
    s.revision=2;s.coins=90;assert(nm_storage_request(&s));run();assert(status.phase==NM_STORE_SAVED && writes==1);
    reboot();run();assert(nm_storage_loaded(&s) && s.coins==90);
    reset_disk();fail_init=1;reboot();run();assert(status.phase==NM_STORE_ERROR && !writes);
    reset_disk();fail_open=1;reboot();run();assert(status.phase==NM_STORE_ERROR && !writes);
    reset_disk();seed(0,10,40);seed(1,11,50);fail_init=1;reboot();
    nm_init(&s,2,true,0);s.revision=1;s.coins=60;assert(nm_storage_request(&s));run();
    assert(status.phase==NM_STORE_SAVED);
    reboot();run();assert(nm_storage_loaded(&s) && s.coins==60);
    reset_disk();seed(0,10,40);seed(1,11,50);fail_init=1;reboot();
    assert(nm_storage_reload());run();assert(nm_storage_loaded(&s) && s.coins==50 && writes==0);
    reboot();nm_init(&s,1,true,0);assert(nm_storage_request(&s));assert(!nm_storage_reload());
    for(unsigned fault=0;fault<4;++fault)for(unsigned same_revision=0;same_revision<2;++same_revision)for(unsigned wrap=0;wrap<2;++wrap){
        reset_disk();reboot();if(wrap)latest_ticket=UINT32_MAX-1;
        nm_init(&s,1,true,0);s.revision=1;assert(nm_storage_request(&s));
        if(fault==1)fail_write=1;
        if(fault==2)fail_commit=1;
        if(fault==3)fail_init=2;
        finish_before_publish=true;s.revision=same_revision?1:2;s.coins=77;assert(nm_storage_request(&s));
        assert(status.pending && !nm_storage_reload());
        run();assert(!status.pending && status.saved_revision==s.revision);
        reboot();run();assert(nm_storage_loaded(&s) && s.coins==77);
    }
    reset_disk();reboot();nm_init(&s,1,true,0);s.revision=1;assert(nm_storage_request(&s));
    publish_during_commit=true;run();assert(writes==2 && !status.pending && status.saved_revision==3);
    reboot();run();assert(nm_storage_loaded(&s) && s.coins==88);
    puts("NiuMa storage worker: PASS (faults, restart, corruption, wrap, interleaved submissions)");
}
