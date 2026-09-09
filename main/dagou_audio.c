#include "dagou_audio.h"
#include "bsp_audio.h"
#include "bsp_battery.h"
#include "bsp_display.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "nvs.h"
#include "nvs_flash.h"
#include <math.h>
#include <stdatomic.h>
#include <string.h>

#define SAMPLE(name) extern const uint8_t name##_start[] asm("_binary_" #name "_pcm_start"); \
    extern const uint8_t name##_end[] asm("_binary_" #name "_pcm_end")
SAMPLE(da); SAMPLE(gou); SAMPLE(jiao); SAMPLE(ha); SAMPLE(ji); SAMPLE(mi);
typedef struct { int kind, syllable, midi; unsigned id; bool save; dagou_config_t config; } command_t;
typedef struct { const int16_t *pcm; int count; float pos, rate, gain; unsigned id; bool active, held; int syllable; } voice_t;
static QueueHandle_t queue, configs, storage;
static atomic_uint hit;
static atomic_uint sample_clock;
static atomic_uint released;
static atomic_int battery=-1, status;
static atomic_bool saved;
static bool nvs_ready;
static dagou_config_t initial;

static void persist(const dagou_config_t *c) {
    nvs_handle_t h;
    bool ok=false;
    if (nvs_ready && nvs_open("dagou", NVS_READWRITE, &h)==ESP_OK) {
        ok=nvs_set_blob(h,"config",c,sizeof(*c))==ESP_OK && nvs_commit(h)==ESP_OK;
        nvs_close(h);
    }
    atomic_store(&saved,ok);
}
static float tri(float phase) { return 1.f-4.f*fabsf(phase-.5f); }
static void service(void *arg) {
    (void)arg;
    dagou_config_t c;
    bool dirty=false;
    TickType_t changed=0, last_battery=xTaskGetTickCount();
    for(;;) {
        if(xQueueReceive(storage,&c,pdMS_TO_TICKS(100))==pdTRUE) {
            dirty=true;changed=xTaskGetTickCount();
        }
        TickType_t now=xTaskGetTickCount();
        if(dirty && now-changed>pdMS_TO_TICKS(1500)){persist(&c);dirty=false;}
        if(now-last_battery>pdMS_TO_TICKS(10000)){atomic_store(&battery,bsp_battery_soc());last_battery=now;}
    }
}
static void worker(void *arg) {
    dagou_config_t c=*(dagou_config_t *)arg;
    voice_t v[4]={0};
    command_t pending[16]; int pn=0;
    uint32_t due[16], clock=0, noise=1234567;
    const uint8_t *starts[]={da_start,gou_start,jiao_start,ha_start,ji_start,mi_start};
    const uint8_t *ends[]={da_end,gou_end,jiao_end,ha_end,ji_end,mi_end};
    const float anchors[]={71.19508f,65.59509f,71.12261f,72.66529f,67.55506f,65.47641f};
    const float bass[]={65.41f,49.f,55.f,43.65f};
    const float chords[4][4]={{261.63f,329.63f,392.f,523.25f},{196.f,246.94f,293.66f,392.f},
        {220.f,261.63f,329.63f,440.f},{174.61f,220.f,261.63f,349.23f}};
    static int16_t kick[1800];
    for(int i=0;i<1800;++i)
        kick[i]=(int16_t)(sinf(6.2831853f*(55.f*i/16000.f+1.1f*(1.f-expf(-(float)i/300.f))))*(1.f-i/1800.f)*3200);
    float bass_phase=0,arp_phase=0;
    bool audio_ok=bsp_audio_set_format(16000,16,1)==ESP_OK;
    atomic_store(&status,audio_ok?1:-1);
    bsp_audio_set_volume(c.volume);
    int16_t out[160];
    for (;;) {
        command_t cmd;
        if (xQueueReceive(configs,&cmd,0)==pdTRUE) {
            c=cmd.config; bsp_audio_set_volume(c.volume); bsp_display_backlight(c.brightness);
        }
        while(pn<16 && xQueueReceive(queue,&cmd,0)==pdTRUE) {
            pending[pn]=cmd; due[pn]=dagou_next_step(clock,c.snap); ++pn;
        }
        for(int i=0;i<160;++i,++clock) {
            while(pn && (int32_t)(clock-due[0])>=0) {
                cmd=pending[0]; --pn;
                memmove(pending,pending+1,(size_t)pn*sizeof(*pending));
                memmove(due,due+1,(size_t)pn*sizeof(*due));
                int slot=0;
                for(int k=0;k<4;++k) { if(!v[k].active){slot=k;break;} if(v[k].id<v[slot].id)slot=k; }
                int s=c.voice*3+cmd.syllable;
                v[slot]=(voice_t){.pcm=(const int16_t*)starts[s],.count=(int)(ends[s]-starts[s])/2,
                    .rate=powf(2.f,(cmd.midi-anchors[s])/12.f),.gain=1.f,.id=cmd.id,
                    .active=true,.held=cmd.id>atomic_load(&released),.syllable=cmd.syllable};
                atomic_fetch_add(&hit,1);
            }
            float mix=0;
            for(int k=0;k<4;++k) if(v[k].active) {
                voice_t *p=&v[k];
                if(p->id<=atomic_load(&released))p->held=false;
                int end=c.voice?5520:4640, start=c.voice?3920:2000;
                if(end>=p->count)end=p->count-1;
                if(start>=end)start=end/2;
                if(p->held && p->syllable==2 && p->pos>=end) p->pos=start+fmodf(p->pos-end,(float)(end-start));
                if(p->pos>=p->count-1){p->active=false;continue;}
                int ix=(int)p->pos; float f=p->pos-ix;
                float sample=p->pcm[ix]*(1-f)+p->pcm[ix+1]*f;
                /* Short crossfade avoids a hard discontinuity in held vowels. */
                if(p->held && p->syllable==2 && p->pos>end-160) {
                    float a=(p->pos-(end-160))/160.f;
                    int j=start+(int)(a*160); if(j<p->count)sample=sample*(1-a)+p->pcm[j]*a;
                }
                if(!p->held && p->pos>start && p->syllable==2)p->gain*=.9985f;
                if(c.sound)mix+=sample*.60f*p->gain;
                p->pos+=p->rate;
                if(p->gain<.005f)p->active=false;
            }
            if(c.music) {
                uint32_t beat=clock%7500, step=(clock/1875)%64;
                int chord=(int)(step/16);
                if(beat<1800)mix+=kick[beat];
                noise^=noise<<13;noise^=noise>>17;noise^=noise<<5;
                float n=((int)(noise&65535)-32768)/32768.f;
                int sub=clock%1875;
                if(sub<400)mix+=n*(1.f-sub/400.f)*600;
                if((clock/7500)%2 && beat<1600)mix+=n*(1.f-beat/1600.f)*1600;
                bass_phase+=bass[chord]/16000.f;
                arp_phase+=chords[chord][step%4]/16000.f;
                if(bass_phase>=1)bass_phase-=1;
                if(arp_phase>=1)arp_phase-=1;
                mix+=tri(bass_phase)*700;
                mix+=tri(arp_phase)*(1.f-sub/1875.f)*800;
            }
            if(mix>28000)mix=28000;
            if(mix< -28000)mix=-28000;
            out[i]=(int16_t)mix;
        }
        if(audio_ok) {
            if(bsp_audio_write(out,sizeof(out))!=ESP_OK){audio_ok=false;atomic_store(&status,-1);}
        } else vTaskDelay(pdMS_TO_TICKS(10));
        atomic_store(&sample_clock,clock);
        /* Leave CPU time for the idle task even while I2S accepts queued data. */
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
bool dagou_audio_start(dagou_config_t *c) {
    dagou_defaults(c);
    nvs_ready=nvs_flash_init()==ESP_OK; /* Never erase factory/user data on init error. */
    nvs_handle_t h;
    if(nvs_ready && nvs_open("dagou",NVS_READONLY,&h)==ESP_OK) {
        dagou_config_t stored;size_t size=sizeof(stored);
        if(nvs_get_blob(h,"config",&stored,&size)==ESP_OK && size==sizeof(stored) && dagou_config_valid(&stored))*c=stored;
        nvs_close(h);
    }
    atomic_store(&saved,nvs_ready);
    atomic_store(&battery,bsp_battery_soc());
    queue=xQueueCreate(16,sizeof(command_t)); configs=xQueueCreate(1,sizeof(command_t));
    storage=xQueueCreate(1,sizeof(dagou_config_t));
    if(!queue || !configs || !storage){atomic_store(&status,-1);return false;}
    initial=*c;
    if(xTaskCreate(service,"dagou_storage",3072,NULL,2,NULL)!=pdPASS)atomic_store(&saved,false);
    bool ok=xTaskCreate(worker,"dagou_audio",6144,&initial,4,NULL)==pdPASS;
    if(!ok)atomic_store(&status,-1);
    return ok;
}
void dagou_audio_config(const dagou_config_t *c,bool save) {
    command_t cmd={.config=*c,.save=save}; if(configs)xQueueOverwrite(configs,&cmd);
    if(save && storage){atomic_store(&saved,false);xQueueOverwrite(storage,c);}
}
bool dagou_audio_press(int syllable,int midi,unsigned id) {
    command_t cmd={.syllable=syllable,.midi=midi,.id=id};
    return queue && xQueueSend(queue,&cmd,0)==pdTRUE;
}
void dagou_audio_release(unsigned id){atomic_store(&released,id);}
unsigned dagou_audio_hit(void){return atomic_load(&hit);}
uint32_t dagou_audio_clock(void){return atomic_load(&sample_clock);}
int dagou_audio_battery(void){return atomic_load(&battery);}
int dagou_audio_status(void){return atomic_load(&status);}
bool dagou_audio_saved(void){return atomic_load(&saved);}
