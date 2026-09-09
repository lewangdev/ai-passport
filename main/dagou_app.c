#include "dagou_app.h"
#include "dagou_audio.h"
#include "dagou_model.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <math.h>
#include <stdio.h>
#include <stdatomic.h>

#define CREAM 0xfff2dc
#define AMBER 0xffb400
#define GRAY 0x87837e
LV_FONT_DECLARE(dagou_ui_font);
#define IMAGE(name) extern const uint8_t name##_data[] asm("_binary_" #name "_bgra_start")
IMAGE(dagou_close); IMAGE(dagou_open); IMAGE(cat_close); IMAGE(cat_open);
static lv_image_dsc_t images[4];
static lv_obj_t *screen,*character,*heading,*status,*hint,*word,*notes,*settings,*setting_text,*save_text;
static lv_obj_t *particles[12],*grid[24],*beatdots[4];
static dagou_config_t cfg;
static lv_font_t app_font;
typedef struct{bsp_btn_t key; bsp_btn_ev_t event;} input_t;
static QueueHandle_t inputs;
static atomic_uint dropped_releases;
static bool started, in_settings, held, editing;
static int setting, held_key=-1, fx;
static unsigned serial, seen_hit;
static uint32_t pressed_at, burst_at, last_action;
static const char *const effect_names[]={"RINGS","BLOOM","SPIRAL","RAYS","CONFETTI","ZIGZAG","POP","CROSS","ORBIT","WAVE","STARS","GRID"};
static const char *const setting_names[]={"音色选择","钢琴模式","八度切换","起始八度","强化节奏","显示网格","背景音乐","音效开关","音量大小","屏幕亮度","音高档位","钢琴音节","叮咚鸡","帝皇形象","关于原作","恢复设置","返回演奏"};
#define SETTINGS_COUNT 17

static lv_obj_t *box(lv_obj_t *p,int x,int y,int w,int h,uint32_t color,int radius) {
    lv_obj_t *o=lv_obj_create(p);lv_obj_remove_style_all(o);
    lv_obj_set_pos(o,x,y);lv_obj_set_size(o,w,h);
    lv_obj_set_style_bg_color(o,lv_color_hex(color),0);
    lv_obj_set_style_bg_opa(o,LV_OPA_COVER,0);lv_obj_set_style_radius(o,radius,0);
    lv_obj_remove_flag(o,LV_OBJ_FLAG_SCROLLABLE);return o;
}
static lv_obj_t *label(lv_obj_t *p,int x,int y,int w,const char *text) {
    lv_obj_t *o=lv_label_create(p);lv_obj_set_pos(o,x,y);lv_obj_set_width(o,w);
    lv_obj_set_style_text_align(o,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_set_style_text_color(o,lv_color_hex(GRAY),0);lv_label_set_text(o,text);return o;
}
static void show_settings(void) {
    char value[100]="";
    const char *onoff[]={"关闭","开启"};
    switch(setting){
    case 0:snprintf(value,sizeof(value),"%s",cfg.voice?"哈基米 / HA JI MI":"大狗叫 / DA GOU JIAO");break;
    case 1:snprintf(value,sizeof(value),"%s",onoff[cfg.piano]);break;
    case 2:snprintf(value,sizeof(value),"%s",onoff[cfg.octave_enabled]);break;
    case 3:snprintf(value,sizeof(value),"C%d - C%d",cfg.octave,cfg.octave+1);break;
    case 4:snprintf(value,sizeof(value),"%s  (128 BPM 八分拍)",onoff[cfg.snap]);break;
    case 5:snprintf(value,sizeof(value),"%s",onoff[cfg.grid]);break;
    case 6:snprintf(value,sizeof(value),"%s",onoff[cfg.music]);break;
    case 7:snprintf(value,sizeof(value),"%s",onoff[cfg.sound]);break;
    case 8:snprintf(value,sizeof(value),"%d%%",cfg.volume);break;
    case 9:snprintf(value,sizeof(value),"%d%%",cfg.brightness);break;
    case 10:snprintf(value,sizeof(value),"%d / 4",cfg.tier+1);break;
    case 11:snprintf(value,sizeof(value),"%s",(cfg.voice?(const char*[]){"HA","JI","MI"}:(const char*[]){"DA","GOU","JIAO"})[cfg.syllable]);break;
    case 12:case 13:snprintf(value,sizeof(value),"原网页的投币解锁内容\n请在哔哩哔哩支持原作\n本设备未包含此素材");break;
    case 14:snprintf(value,sizeof(value),"原作：马克杯 MarkCup\n哔哩哔哩 / 大狗 Tap\n口袋里的快乐\n愿你每天都开心！");break;
    case 15:snprintf(value,sizeof(value),"按确定：恢复默认设置");break;
    default:snprintf(value,sizeof(value),"按确定：继续快乐演奏");break;
    }
    lv_label_set_text_fmt(setting_text,"%02d / %02d\n\n%s\n\n%s",setting+1,SETTINGS_COUNT,setting_names[setting],value);
    lv_label_set_text(save_text,editing?"上 / 下：调整\n确定：完成":"上 / 下：选择\n确定：进入\n长按下键：返回");
}
static void settings_change(int delta) {
    switch(setting){
    case 0:cfg.voice^=1;break;
    case 1:cfg.piano^=1;break;
    case 2:cfg.octave_enabled^=1;break;
    case 3:cfg.octave=3+dagou_wrap(cfg.octave-3+delta,4);break;
    case 4:cfg.snap^=1;break;
    case 5:cfg.grid^=1;break;
    case 6:cfg.music^=1;break;
    case 7:cfg.sound^=1;break;
    case 8:cfg.volume=(uint8_t)dagou_wrap(cfg.volume+delta*5,105);break;
    case 9:cfg.brightness=10+dagou_wrap(cfg.brightness-10+delta*10,100);break;
    case 10:cfg.tier=dagou_wrap(cfg.tier+delta,4);break;
    case 11:cfg.syllable=dagou_wrap(cfg.syllable+delta,3);break;
    default:break;
    }
    dagou_audio_config(&cfg,true);show_settings();
}
static void toggle_settings(void){
    in_settings=!in_settings;editing=false;held=false;dagou_audio_release(serial);
    if(in_settings){lv_obj_remove_flag(settings,LV_OBJ_FLAG_HIDDEN);show_settings();}
    else lv_obj_add_flag(settings,LV_OBJ_FLAG_HIDDEN);
}
static void key_cb(bsp_btn_t key,bsp_btn_ev_t event,void *user){
    (void)user;input_t i={key,event};
    /* Bounded queue keeps the button callback non-blocking. */
    if(inputs && xQueueSend(inputs,&i,0)!=pdTRUE && event==BSP_BTN_RELEASE)
        atomic_fetch_or(&dropped_releases,1u<<key);
}
static void input(input_t i,uint32_t now){
    if(i.event==BSP_BTN_RELEASE){if(held_key==(int)i.key){held=false;held_key=-1;dagou_audio_release(serial);}return;}
    if(i.event==BSP_BTN_LONG){
        if(i.key==BSP_BTN_DOWN){toggle_settings();return;}
        if(i.key==BSP_BTN_UP && !in_settings){
            if(cfg.piano && cfg.octave_enabled)cfg.octave=3+dagou_wrap(cfg.octave-2,4);
            else if(!cfg.piano)cfg.tier=(cfg.tier+1)%4;
            dagou_audio_config(&cfg,true);
        }
        return;
    }
    if(i.event!=BSP_BTN_PRESS)return;
    last_action=now;
    if(!started){started=true;lv_label_set_text(heading,"大狗 Tap");return;}
    if(in_settings){
        if(i.key==BSP_BTN_OK){
            if(setting==16)toggle_settings();
            else if(setting==15){dagou_defaults(&cfg);dagou_audio_config(&cfg,true);show_settings();}
            else if(setting<12){editing=!editing;show_settings();}
        }else if(editing)settings_change(i.key==BSP_BTN_UP?-1:1);
        else{setting=dagou_wrap(setting+(i.key==BSP_BTN_UP?-1:1),SETTINGS_COUNT);show_settings();}
        return;
    }
    if(cfg.piano && i.key!=BSP_BTN_OK){cfg.note=dagou_wrap(cfg.note+(i.key==BSP_BTN_UP?1:-1),8);return;}
    int syllable=cfg.piano?cfg.syllable:(int)i.key;
    held=true;held_key=i.key;pressed_at=now;++serial;
    dagou_audio_release(serial-1);
    if(!dagou_audio_press(syllable,dagou_midi(&cfg,syllable),serial)){held=false;lv_label_set_text(word,"再按一次试试！");}
    else lv_label_set_text(word,(cfg.voice?(const char*[]){"HA!","JI!","MIIII!"}:(const char*[]){"DA!","GOU!","JIAOOO!"})[syllable]);
}
static void tick(lv_timer_t *timer){
    (void)timer;uint32_t now=lv_tick_get();input_t i;
    while(xQueueReceive(inputs,&i,0)==pdTRUE)input(i,now);
    unsigned releases=atomic_exchange(&dropped_releases,0);
    if(held_key>=0 && (releases&(1u<<held_key))){held=false;held_key=-1;dagou_audio_release(serial);}
    /* Bound sustain if a release was dropped; normal release remains immediate. */
    if(held && now-pressed_at>15000){held=false;dagou_audio_release(serial);}
    unsigned h=dagou_audio_hit();
    if(h!=seen_hit){seen_hit=h;burst_at=now;fx=(int)((h-1)%12);}
    uint32_t audio_clock=dagou_audio_clock();
    float age=(now-burst_at)/1000.f, beat=(audio_clock%7500)/7500.f;
    float hold=held?fminf((now-pressed_at)/2000.f,1.f):0;
    bool mouth=started && ((age<.20f && h) || (held && hold>.1f));
    lv_image_set_src(character,&images[cfg.voice*2+(mouth?1:0)]);
    lv_image_set_scale(character,256+(int)(hold*40));
    lv_obj_set_pos(character,54+(int)(hold*3*sinf(now*.07f)),100-(int)(sinf(beat*3.14159f)*7)-(int)(hold*12));
    lv_obj_set_style_image_recolor(character,lv_color_hex(0xff5a5f),0);
    lv_obj_set_style_image_recolor_opa(character,(int)(hold*85),0);
    int cells=cfg.piano?24:12, rows=cells/3;
    for(int j=0;j<24;++j){
        if(cfg.grid && j<cells && !in_settings){
            lv_obj_remove_flag(grid[j],LV_OBJ_FLAG_HIDDEN);lv_obj_set_pos(grid[j],(j%3)*80,42+(j/3)*224/rows);
            lv_obj_set_size(grid[j],80,224/rows);
        }else lv_obj_add_flag(grid[j],LV_OBJ_FLAG_HIDDEN);
    }
    static const uint32_t colors[]={AMBER,GRAY,0xff5a5f,0x16c2a3,0x3e7bfa};
    for(int j=0;j<12;++j){
        lv_obj_t *p=particles[j];
        if(age>1.1f || !h || in_settings){lv_obj_set_style_opa(p,0,0);continue;}
        float angle=j*.523599f+age*(fx%2?2:-2),r=25+age*110;
        int x=120+(int)(cosf(angle)*r),y=165+(int)(sinf(angle)*r),w=10+j%3*5,hh=w;
        int radius=(fx==0 || fx==2 || fx==8)?LV_RADIUS_CIRCLE:2;
        if(fx==0){w=hh=25+j*12+(int)(age*70);x=120;y=165;}
        if(fx==1){w=hh=18+j*3;angle+=age*2;x=120+(int)(cosf(angle)*r);}
        if(fx==3){w=5;hh=30+j*2;}
        if(fx==4 || fx==6 || fx==10){x=10+j*20;y=45+(int)(age*200)+((j*37)%75);radius=fx==6?LV_RADIUS_CIRCLE:0;}
        if(fx==5 || fx==9){x=10+j*20;y=165+(int)(sinf(j*.8f+age*8)*65);w=14;hh=fx==9?7:22;}
        if(fx==7){w=j%2?8:65;hh=j%2?65:8;}
        if(fx==11){x=(j%4)*65;y=65+(j/4)*70;w=54;hh=54;}
        lv_obj_set_pos(p,x-w/2,y-hh/2);lv_obj_set_size(p,w,hh);
        lv_obj_set_style_radius(p,radius,0);lv_obj_set_style_bg_color(p,lv_color_hex(colors[j%5]),0);
        lv_obj_set_style_bg_opa(p,fx==0?0:180,0);lv_obj_set_style_border_width(p,fx==0?3:0,0);
        lv_obj_set_style_border_color(p,lv_color_hex(AMBER),0);lv_obj_set_style_opa(p,(int)((1-age/1.1f)*220),0);
    }
    int b=(int)(audio_clock/7500)%4;
    for(int j=0;j<4;++j)lv_obj_set_style_bg_color(beatdots[j],lv_color_hex(j==b?AMBER:0xe6dbc6),0);
    char buf[100];int bat=dagou_audio_battery();
    if(bat>=0)snprintf(buf,sizeof(buf),"音量 %d%%   电量 %d%%",cfg.volume,bat);
    else snprintf(buf,sizeof(buf),"音量 %d%%   电量 --",cfg.volume);
    lv_label_set_text(status,buf);
    if(!started){lv_label_set_text(word,"送你一点小快乐");lv_label_set_text(hint,"按任意键，快乐开始");}
    else{
        if(cfg.piano){
            static const char* const ns[]={"C","D","E","F","G","A","B","C"};
            snprintf(buf,sizeof(buf),"%s%d    %s",ns[cfg.note],(cfg.octave_enabled?cfg.octave:4)+(cfg.note==7),cfg.voice?"HAJIMI":"DAGOU");
            lv_label_set_text(notes,buf);
            lv_label_set_text(hint,"上 / 下选音，确定演奏\n长按下键：设置");
        }else{
            snprintf(buf,sizeof(buf),"%s  |  音高 %d/4",cfg.voice?"HA  JI  MI":"DA  GOU  JIAO",cfg.tier+1);lv_label_set_text(notes,buf);
            lv_label_set_text(hint,"三键敲击，长按确定延音\n长按上调音高，下进设置");
        }
        if(!held && age>1.1f)lv_label_set_text(word,"按一下，就开心一下！");
    }
    if(dagou_audio_status()<0)lv_label_set_text(word,"声音初始化失败");
    if(in_settings)lv_label_set_text(heading,dagou_audio_saved()?"设置 / 已保存":"设置 / 未保存");
    else lv_label_set_text(heading,started?"大狗 Tap":"你好呀，我的朋友！");
    (void)effect_names; (void)last_action;
}
void dagou_app_start(void){
    inputs=xQueueCreate(32,sizeof(input_t));if(!inputs)return;
    dagou_audio_start(&cfg);
    if(!bsp_lvgl_lock(1000))return;
    screen=lv_obj_create(NULL);lv_obj_remove_style_all(screen);
    lv_obj_set_style_bg_color(screen,lv_color_hex(CREAM),0);lv_obj_set_style_bg_opa(screen,255,0);
    app_font=lv_font_montserrat_14;
    app_font.fallback=&dagou_ui_font;
    lv_obj_set_style_text_font(screen,&app_font,0);
    lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    for(int j=0;j<24;++j){grid[j]=box(screen,0,0,80,56,CREAM,0);lv_obj_set_style_bg_opa(grid[j],0,0);lv_obj_set_style_border_width(grid[j],1,0);lv_obj_set_style_border_color(grid[j],lv_color_hex(0xe6dbc6),0);}
    for(int j=0;j<12;++j)particles[j]=box(screen,0,0,10,10,AMBER,0);
    const uint8_t *data[]={dagou_close_data,dagou_open_data,cat_close_data,cat_open_data};
    for(int j=0;j<4;++j)images[j]=(lv_image_dsc_t){.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_COLOR_FORMAT_ARGB8888,.w=132,.h=152,.stride=528},.data_size=132*152*4,.data=data[j]};
    character=lv_image_create(screen);lv_image_set_src(character,&images[2]);
    heading=label(screen,0,7,240,"你好呀，我的朋友！");
    status=label(screen,0,28,240,"");
    word=label(screen,5,65,230,"送你一点小快乐");
    for(int j=0;j<4;++j)beatdots[j]=box(screen,90+j*16,88,8,5,AMBER,3);
    notes=label(screen,5,253,230,"128 BPM / MarkCup");
    hint=label(screen,2,278,236,"按任意键，快乐开始");
    settings=box(screen,9,52,222,259,0xfffaf0,14);
    setting_text=label(settings,8,18,206,"");save_text=label(settings,5,187,212,"");
    lv_obj_add_flag(settings,LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(screen);lv_timer_create(tick,33,NULL);
    bsp_lvgl_unlock();
    bsp_display_backlight(cfg.brightness);
    bsp_button_init(key_cb,NULL);
}
