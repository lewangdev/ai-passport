/* Executes the production LVGL screen with host-only board/audio adapters. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "dagou_audio.h"
#include "bsp_button.h"
#include "freertos/queue.h"
#define PIXELS (132*152*4)
uint8_t image0[PIXELS] asm("_binary_dagou_close_bgra_start");
uint8_t image1[PIXELS] asm("_binary_dagou_open_bgra_start");
uint8_t image2[PIXELS] asm("_binary_cat_close_bgra_start");
uint8_t image3[PIXELS] asm("_binary_cat_open_bgra_start");
typedef struct {uint8_t data[4096];size_t size;int count,head,tail,n;} queue_t;
QueueHandle_t xQueueCreate(int count,size_t size){queue_t *q=calloc(1,sizeof(*q));q->count=count;q->size=size;assert(count*size<=sizeof(q->data));return q;}
int xQueueSend(QueueHandle_t handle,const void *data,int timeout){(void)timeout;queue_t*q=handle;if(q->n==q->count)return 0;memcpy(q->data+q->tail*q->size,data,q->size);q->tail=(q->tail+1)%q->count;q->n++;return 1;}
int xQueueReceive(QueueHandle_t handle,void *data,int timeout){(void)timeout;queue_t*q=handle;if(!q->n)return 0;memcpy(data,q->data+q->head*q->size,q->size);q->head=(q->head+1)%q->count;q->n--;return 1;}
bool bsp_lvgl_lock(int t){(void)t;return true;}
void bsp_lvgl_unlock(void){}
void bsp_display_backlight(uint8_t v){(void)v;}
esp_err_t bsp_button_init(bsp_btn_cb_t cb,void *user){(void)cb;(void)user;return 0;}
static unsigned hits;
bool dagou_audio_start(dagou_config_t*c){dagou_defaults(c);return true;}
void dagou_audio_config(const dagou_config_t*c,bool save){(void)c;(void)save;}
bool dagou_audio_press(int s,int m,unsigned id){(void)s;(void)m;(void)id;hits++;return true;}
void dagou_audio_release(unsigned id){(void)id;}
unsigned dagou_audio_hit(void){return hits;}
uint32_t dagou_audio_clock(void){return lv_tick_get()*16;}
int dagou_audio_battery(void){return 78;}
int dagou_audio_status(void){return 1;}
bool dagou_audio_saved(void){return true;}
#include "../../main/dagou_app.c"
static uint16_t frame[240*320],draw[240*20];
static void flush(lv_display_t*d,const lv_area_t*a,uint8_t*p){
    uint16_t *s=(uint16_t*)p;
    for(int y=a->y1;y<=a->y2;++y)for(int x=a->x1;x<=a->x2;++x)frame[y*240+x]=*s++;
    lv_display_flush_ready(d);
}
static void advance(int ms){for(int t=0;t<ms;t+=10){lv_tick_inc(10);lv_timer_handler();}}
static void snapshot(const char *name){
    lv_obj_invalidate(screen);lv_refr_now(NULL);
    char path[256];snprintf(path,sizeof(path),"/tmp/dagou-%s.ppm",name);
    FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 320\n255\n");
    for(int i=0;i<240*320;i++){uint16_t p=frame[i];uint8_t rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);
}
int main(void){
    const char*names[]={"dagou_close","dagou_open","cat_close","cat_open"};uint8_t*data[]={image0,image1,image2,image3};
    for(int i=0;i<4;i++){char path[256];snprintf(path,sizeof(path),"assets/dagou-tap/%s.bgra",names[i]);FILE*f=fopen(path,"rb");assert(f);assert(fread(data[i],1,PIXELS,f)==PIXELS);fclose(f);}
    lv_init();lv_display_t*d=lv_display_create(240,320);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,draw,NULL,sizeof(draw),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(d,flush);
    dagou_app_start();advance(100);snapshot("welcome");
    /* Assert every UTF-8 glyph in the production source exists in the UI font. */
    FILE *src=fopen("main/dagou_app.c","rb");assert(src);
    unsigned char text[32768];size_t length=fread(text,1,sizeof(text),src);fclose(src);
    for(size_t p=0;p<length;){
        unsigned ch=text[p++];
        if(ch<128)continue;
        int extra=(ch&0xf0)==0xe0?2:((ch&0xe0)==0xc0?1:3);
        ch&=extra==1?31:(extra==2?15:7);
        while(extra-- && p<length)ch=(ch<<6)|(text[p++]&63);
        lv_font_glyph_dsc_t glyph;
        assert(lv_font_get_glyph_dsc(&app_font,&glyph,ch,0));
    }
    key_cb(BSP_BTN_OK,BSP_BTN_PRESS,NULL);advance(50);
    key_cb(BSP_BTN_OK,BSP_BTN_RELEASE,NULL);advance(50);
    key_cb(BSP_BTN_OK,BSP_BTN_PRESS,NULL);advance(250);snapshot("play");
    advance(1400);snapshot("hold");key_cb(BSP_BTN_OK,BSP_BTN_RELEASE,NULL);advance(100);
    key_cb(BSP_BTN_DOWN,BSP_BTN_LONG,NULL);advance(50);snapshot("settings");
    for(int j=0;j<SETTINGS_COUNT;j++){setting=j;show_settings();advance(50);lv_obj_update_layout(settings);assert(lv_obj_get_y(setting_text)+lv_obj_get_height(setting_text)<185);}
    toggle_settings();cfg.piano=1;cfg.grid=1;advance(100);snapshot("piano");
    for(int j=0;j<48;j++){key_cb(BSP_BTN_OK,BSP_BTN_PRESS,NULL);advance(50);key_cb(BSP_BTN_OK,BSP_BTN_RELEASE,NULL);advance(80);}
    lv_mem_monitor_t mem;lv_mem_monitor(&mem);printf("UI heap: used %zu / %zu, peak %zu\n",mem.total_size-mem.free_size,mem.total_size,mem.max_used);
    assert(lv_mem_test()==LV_RESULT_OK);puts("Production UI render and interaction smoke test: PASS");
}
