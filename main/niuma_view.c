#include "niuma_view.h"
#include "lvgl.h"
#include "niuma_icons.h"
#include <stdio.h>
#include <string.h>

LV_FONT_DECLARE(niuma_font);
static lv_obj_t *screen,*heading,*page_label,*status_label,*battery_label,*body,*hint,*picture,*rows[5],*row_labels[5];
static uint16_t art_pixels[NM_ART_W*NM_ART_H];
static lv_image_dsc_t art_image;
static const uint32_t bg=0xcbd0b5,ink=0x252b24,paper=0xeeebd8;
static lv_obj_t *metrics[5],*wallet;
static nm_state_t chrome_state;
static nm_store_status_t chrome_store;
static nm_view_kind_t chrome_kind;
static nm_icon_t row_icons[5];
static unsigned chrome_selected,chrome_count;
static void fill(lv_layer_t *layer,int x,int y,int w,int h,uint32_t color){
    lv_draw_rect_dsc_t rect;lv_draw_rect_dsc_init(&rect);
    rect.bg_color=lv_color_hex(color);rect.bg_opa=LV_OPA_COVER;
    lv_area_t area={x,y,x+w-1,y+h-1};lv_draw_rect(layer,&rect,&area);
}
static void outline(lv_layer_t *layer,int x,int y,int w,int h,uint32_t color){
    fill(layer,x,y,w,1,color);fill(layer,x,y+h-1,w,1,color);
    fill(layer,x,y,1,h,color);fill(layer,x+w-1,y,1,h,color);
}
static void icon(lv_layer_t *layer,nm_icon_t id,int x,int y,int scale,uint32_t color){
    for(int row=0;row<12;++row)for(int col=0;col<12;){
        if(!(nm_icons[id][row]&(1u<<(11-col)))){++col;continue;}
        int start=col;while(col<12 && (nm_icons[id][row]&(1u<<(11-col))))++col;
        fill(layer,x+start*scale,y+row*scale,(col-start)*scale,scale,color);
    }
}
static nm_icon_t menu_icon(const char *text){
    static const struct {const char *words[6];nm_icon_t icon;} map[]={
        {{"声音","音量","音乐","Sound","Volume","Music"},NM_ICON_SOUND},
        {{"亮度","屏幕","Brightness","brightness","Screen","screen"},NM_ICON_SUN},
        {{"待机","息屏","Idle","idle","秒","seconds"},NM_ICON_CLOCK},
        {{"语言","中文","英文","Language","Chinese","English"},NM_ICON_LANGUAGE},
        {{"帮助","指南","Help","help","Guide","guide"},NM_ICON_HELP},
        {{"背包","食品","Bag","Inventory","inventory","Stock"},NM_ICON_BAG},
        {{"小游戏","游戏","Minigame","minigame","game","Game"},NM_ICON_GAME},
        {{"事件","信","event","Event","回应","Response"},NM_ICON_MAIL},
        {{"工作","任务","Work","Typing","会议","Meeting"},NM_ICON_WORK},
        {{"饭","食","餐","Food","Meal","meal"},NM_ICON_FOOD},
        {{"休","睡","Sleep","Rest","nap","恢复"},NM_ICON_REST},
        {{"朋友","社交","聊","Friend","Chat","Talk"},NM_ICON_CHAT},
        {{"学习","笔记","Study","Learn","技能","资料"},NM_ICON_BOOK},
        {{"治疗","健康","运动","Care","Health","Exercise"},NM_ICON_HEALTH},
        {{"纪念","职业","成长","Career","Memor","Route"},NM_ICON_TROPHY},
        {{"设置","声音","亮度","Settings","Sound","Brightness"},NM_ICON_SETTINGS},
        {{"存档","保存","Save","save","读取","Reload"},NM_ICON_SAVE},
        {{"金币","工资","商店","Shop","pay","coin"},NM_ICON_COIN}
    };
    for(unsigned i=0;i<sizeof(map)/sizeof(map[0]);++i)for(unsigned j=0;j<6;++j)
        if(strstr(text,map[i].words[j]))return map[i].icon;
    return NM_ICON_MORE;
}
static void draw_chrome(lv_event_t *event){
    lv_layer_t *layer=lv_event_get_layer(event);
    icon(layer,NM_ICON_CALENDAR,16,9,1,ink);
    icon(layer,NM_ICON_SOUND,128,9,1,ink);
    for(unsigned i=0;i<4;++i){
        int h=3+(int)i*2;
        if(chrome_state.volume>i*25)fill(layer,143+i*4,21-h,2,h,ink);
        else fill(layer,143+i*4,20,2,1,ink);
    }
    if(!chrome_state.volume)for(int i=0;i<6;++i){fill(layer,144+i,10+i,1,1,ink);fill(layer,149-i,10+i,1,1,ink);}
    if(chrome_store.phase==NM_STORE_ERROR || chrome_store.pending){
        icon(layer,NM_ICON_SAVE,170,9,1,ink);
        if(chrome_store.phase==NM_STORE_ERROR)fill(layer,183,9,2,7,ink);
    }
    outline(layer,195,9,26,12,ink);fill(layer,221,12,3,6,ink);
    if(chrome_store.battery<0)fill(layer,204,14,7,2,ink);
    else{
        unsigned cells=chrome_store.battery?((unsigned)chrome_store.battery+24)/25:0;
        for(unsigned i=0;i<cells && i<4;++i)fill(layer,198+i*5,12,3,6,ink);
        if(chrome_store.battery<=15){fill(layer,189,9,2,7,ink);fill(layer,189,19,2,2,ink);}
    }
    fill(layer,12,chrome_kind==NM_VIEW_HOME?83:55,216,1,0x7b846b);
    if(chrome_kind==NM_VIEW_HOME){
        static const nm_icon_t stats[]={NM_ICON_FOOD,NM_ICON_ENERGY,NM_ICON_MOOD,NM_ICON_STRESS,NM_ICON_HEALTH};
        for(unsigned i=0;i<5;++i){
            icon(layer,row_icons[i],12+i*45,39,2,i==chrome_selected?paper:ink);
            icon(layer,stats[i],22+i*44,234,1,ink);
            outline(layer,11+i*44,267,33,3,0x7b846b);
            unsigned value=chrome_state.stat[i];
            fill(layer,12+i*44,268,(int)(value*31/100),1,ink);
        }
        icon(layer,NM_ICON_COIN,16,279,1,ink);
        for(unsigned i=0;i<6;++i){
            outline(layer,148+i*12,281,8,7,ink);
            if(i<chrome_state.actions)fill(layer,150+i*12,283,4,3,ink);
        }
    }else if(chrome_kind==NM_VIEW_MENU){
        for(unsigned i=0;i<chrome_count && i<5;++i)
            icon(layer,row_icons[i],18,80+i*45,1,i==chrome_selected%5?paper:ink);
    }else if(chrome_kind==NM_VIEW_STATS){
        static const nm_icon_t stats[]={NM_ICON_FOOD,NM_ICON_ENERGY,NM_ICON_MOOD,NM_ICON_STRESS,NM_ICON_HEALTH};
        for(unsigned i=0;i<5;++i)icon(layer,stats[i],18,63+i*28,2,ink);
    }
}
enum { NM_CORNER_RADIUS=16 };
/* Pixel-aligned casing mask. Draw after children without an offscreen layer. */
static void draw_corners(lv_event_t *event){
    lv_layer_t *layer=lv_event_get_layer(event);
    lv_area_t bounds;lv_obj_get_coords(screen,&bounds);
    lv_draw_rect_dsc_t rect;lv_draw_rect_dsc_init(&rect);
    rect.bg_color=lv_color_black();rect.bg_opa=LV_OPA_COVER;
    for(int y=0;y<NM_CORNER_RADIUS;++y){
        int cut=0,dy=2*(NM_CORNER_RADIUS-y)-1;
        while(cut<NM_CORNER_RADIUS){
            int dx=2*(NM_CORNER_RADIUS-cut)-1;
            if(dx*dx+dy*dy<=4*NM_CORNER_RADIUS*NM_CORNER_RADIUS)break;
            ++cut;
        }
        if(!cut)continue;
        for(int bottom=0;bottom<2;++bottom)for(int right=0;right<2;++right){
            int row=bottom?bounds.y2-y:bounds.y1+y;
            lv_area_t area={.x1=right?bounds.x2-cut+1:bounds.x1,.y1=row,
                .x2=right?bounds.x2:bounds.x1+cut-1,.y2=row};
            lv_draw_rect(layer,&rect,&area);
        }
    }
}
static lv_obj_t *label(lv_obj_t *parent,int x,int y,int w,int h){
    lv_obj_t *obj=lv_label_create(parent);
    lv_obj_set_pos(obj,x,y);lv_obj_set_size(obj,w,h);
    lv_obj_set_style_text_font(obj,&niuma_font,0);lv_obj_set_style_text_color(obj,lv_color_hex(ink),0);
    lv_label_set_long_mode(obj,LV_LABEL_LONG_WRAP);return obj;
}
bool nm_view_init(void){
    screen=lv_obj_create(NULL);if(!screen)return false;
    lv_obj_set_style_bg_color(screen,lv_color_hex(bg),0);
    lv_obj_set_style_pad_all(screen,0,0);lv_obj_remove_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(screen,draw_chrome,LV_EVENT_DRAW_POST_END,NULL);
    lv_obj_add_event_cb(screen,draw_corners,LV_EVENT_DRAW_POST_END,NULL);
    status_label=label(screen,34,6,90,22);battery_label=label(screen,195,6,29,22);
    lv_obj_add_flag(battery_label,LV_OBJ_FLAG_HIDDEN);
    lv_obj_set_style_text_align(battery_label,LV_TEXT_ALIGN_RIGHT,0);
    heading=label(screen,8,29,224,23);
    page_label=label(screen,184,29,48,23);
    lv_obj_set_style_text_align(page_label,LV_TEXT_ALIGN_RIGHT,0);
    body=label(screen,12,65,216,170);hint=label(screen,4,294,232,21);
    lv_obj_set_style_bg_color(hint,lv_color_hex(ink),0);lv_obj_set_style_bg_opa(hint,LV_OPA_COVER,0);
    lv_obj_set_style_text_color(hint,lv_color_hex(paper),0);
    lv_obj_set_style_text_align(hint,LV_TEXT_ALIGN_CENTER,0);
    picture=lv_image_create(screen);
    art_image.header.magic=LV_IMAGE_HEADER_MAGIC;art_image.header.cf=LV_COLOR_FORMAT_RGB565;
    art_image.header.w=NM_ART_W;art_image.header.h=NM_ART_H;art_image.header.stride=NM_ART_W*2;
    art_image.data_size=sizeof(art_pixels);art_image.data=(const uint8_t *)art_pixels;
    lv_image_set_src(picture,&art_image);lv_image_set_pivot(picture,0,0);
    lv_image_set_antialias(picture,false);
    for(unsigned i=0;i<5;++i){
        metrics[i]=label(screen,6+i*44,248,44,19);
        lv_obj_set_style_text_align(metrics[i],LV_TEXT_ALIGN_CENTER,0);
        lv_obj_add_flag(metrics[i],LV_OBJ_FLAG_HIDDEN);
        rows[i]=lv_obj_create(screen);lv_obj_remove_style_all(rows[i]);
        lv_obj_set_style_bg_opa(rows[i],LV_OPA_COVER,0);lv_obj_remove_flag(rows[i],LV_OBJ_FLAG_SCROLLABLE);
        row_labels[i]=label(rows[i],5,2,212,39);
    }
    wallet=label(screen,33,275,110,20);lv_obj_add_flag(wallet,LV_OBJ_FLAG_HIDDEN);
    lv_screen_load(screen);return true;
}
void nm_view_draw(const nm_view_t *v,const nm_state_t *s,const nm_game_t *g,nm_store_status_t store,unsigned frame){
    bool chrome_changed=chrome_kind!=v->kind || chrome_selected!=v->selected ||
        memcmp(&chrome_state,s,sizeof(*s)) || chrome_store.battery!=store.battery ||
        chrome_store.phase!=store.phase || chrome_store.pending!=store.pending;
    chrome_state=*s;chrome_store=store;chrome_kind=v->kind;
    chrome_selected=v->selected;
    char top[100],battery[24];
    bool low=store.battery>=0 && store.battery<=15;
    if(store.battery<0)snprintf(battery,sizeof(battery),s->english?"BAT --":"电量 --");
    else snprintf(battery,sizeof(battery),s->english?"%d%%%s":"电%d%%%s",store.battery,low?"!":"");
    snprintf(top,sizeof(top),"%lu",(unsigned long)s->day);
    lv_label_set_text(status_label,top);lv_label_set_text(battery_label,battery);
    lv_obj_set_style_bg_opa(battery_label,low?LV_OPA_COVER:LV_OPA_TRANSP,0);
    lv_obj_set_style_bg_color(battery_label,lv_color_hex(ink),0);
    lv_obj_set_style_text_color(battery_label,lv_color_hex(low?paper:ink),0);
    lv_label_set_text(heading,v->title);lv_label_set_text(body,v->body);
    bool is_home=v->kind==NM_VIEW_HOME;
    lv_obj_set_pos(heading,is_home?12:8,is_home?65:29);
    lv_obj_set_style_text_align(heading,is_home?LV_TEXT_ALIGN_CENTER:LV_TEXT_ALIGN_LEFT,0);
    lv_obj_set_style_text_line_space(body,v->kind==NM_VIEW_TEXT?2:0,0);
    for(unsigned i=0;i<5;++i){
        if(is_home || v->kind==NM_VIEW_STATS){
            static const char *zh[]={"饱腹","精力","心情","压力","健康"};
            static const char *en[]={"Food","Energy","Mood","Stress","Health"};
            char value[50];
            if(is_home)snprintf(value,sizeof(value),"%u",s->stat[i]);
            else snprintf(value,sizeof(value),"%s   %u / 100",s->english?en[i]:zh[i],s->stat[i]);
            lv_obj_set_pos(metrics[i],is_home?6+i*44:54,is_home?248:66+i*28);
            lv_obj_set_size(metrics[i],is_home?44:174,20);
            lv_obj_set_style_text_align(metrics[i],is_home?LV_TEXT_ALIGN_CENTER:LV_TEXT_ALIGN_LEFT,0);
            lv_label_set_text(metrics[i],value);lv_obj_remove_flag(metrics[i],LV_OBJ_FLAG_HIDDEN);
        }
        else lv_obj_add_flag(metrics[i],LV_OBJ_FLAG_HIDDEN);
    }
    if(is_home){
        char coins[20];snprintf(coins,sizeof(coins),"%lu",(unsigned long)s->coins);
        lv_label_set_text(wallet,coins);lv_obj_remove_flag(wallet,LV_OBJ_FLAG_HIDDEN);
        lv_label_set_text(heading,v->options[v->selected]);
    }else lv_obj_add_flag(wallet,LV_OBJ_FLAG_HIDDEN);
    bool paged=v->kind==NM_VIEW_MENU && v->count>5;
    lv_obj_set_width(heading,paged?172:224);
    if(paged){
        char pages[16];
        snprintf(pages,sizeof(pages),"%u/%u",v->selected/5+1,(v->count+4)/5);
        lv_label_set_text(page_label,pages);
        lv_obj_remove_flag(page_label,LV_OBJ_FLAG_HIDDEN);
    }else lv_obj_add_flag(page_label,LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(hint,v->hint[0]?v->hint:s->english?"↑↓ Select  ○ OK  Hold: back":"↑↓ 选择  ○ 确认  长按返回");
    bool scene=v->kind==NM_VIEW_SCENE || v->kind==NM_VIEW_HOME || v->kind==NM_VIEW_GAME || v->kind==NM_VIEW_MEETING || v->kind==NM_VIEW_ESCAPE;
    if(scene){
        if(v->kind==NM_VIEW_GAME || v->kind==NM_VIEW_MEETING || v->kind==NM_VIEW_ESCAPE)nm_art_game(art_pixels,s,g);
        else if(v->preview && v->preview_item<12){
            nm_state_t preview=*s;preview.equipped[v->preview_item/4]=v->preview_item;
            nm_art_render(art_pixels,&preview,v->scene,frame);
        }else nm_art_render(art_pixels,s,v->scene,frame);
        lv_obj_remove_flag(picture,LV_OBJ_FLAG_HIDDEN);
        bool big=v->kind==NM_VIEW_HOME || v->kind==NM_VIEW_GAME;
        unsigned crop_y=(v->scene==NM_SCENE_HOME || v->scene==NM_SCENE_SLEEP)?12:20;
        /* Remove the cropped edge of the distant window from the close-up. */
        if(is_home && (v->scene==NM_SCENE_OFFICE || v->scene==NM_SCENE_HOME))
            for(unsigned yy=crop_y;yy<30;++yy)for(unsigned xx=20;xx<32;++xx)art_pixels[yy*NM_ART_W+xx]=0xce96;
        /* Integer 3x close-up: retain the original pixel aspect and head/body ratio. */
        art_image.header.w=is_home?80:NM_ART_W;art_image.header.h=is_home?50:NM_ART_H;
        art_image.data=(const uint8_t *)(art_pixels+(is_home?crop_y*NM_ART_W+20:0));
        art_image.data_size=sizeof(art_pixels)-(is_home?(crop_y*NM_ART_W+20)*2:0);
        lv_image_set_src(picture,&art_image);
        lv_image_set_scale(picture,is_home?768:big?512:256);lv_obj_set_pos(picture,is_home?0:big?0:60,is_home?84:54);
        lv_obj_invalidate(picture);
        lv_obj_set_pos(body,12,big?217:136);lv_obj_set_size(body,216,big?75:86);
    }else{lv_obj_add_flag(picture,LV_OBJ_FLAG_HIDDEN);lv_obj_set_pos(body,12,65);lv_obj_set_size(body,216,155);}
    unsigned offset=v->kind==NM_VIEW_MENU?(v->selected/5)*5:0;
    chrome_count=v->count>offset?v->count-offset:0;
    for(unsigned i=0;i<5;++i){
        if(i+offset>=v->count){lv_obj_add_flag(rows[i],LV_OBJ_FLAG_HIDDEN);continue;}
        lv_obj_remove_flag(rows[i],LV_OBJ_FLAG_HIDDEN);
        bool home=v->kind==NM_VIEW_HOME,menu=v->kind==NM_VIEW_MENU;
        static const nm_icon_t nav[]={NM_ICON_WORK,NM_ICON_FOOD,NM_ICON_REST,NM_ICON_CHAT,NM_ICON_MORE};
        row_icons[i]=home?nav[i]:menu_icon(v->options[i+offset]);
        if(home && i==0)row_icons[i]=s->evening?NM_ICON_HOME:nm_weekend(s)?NM_ICON_HEALTH:NM_ICON_WORK;
        lv_obj_set_pos(rows[i],home?(int)i*45+8:8,home?34:menu?65+i*45:224+i*24);
        lv_obj_set_size(rows[i],home?33:224,home?31:menu?43:23);
        lv_obj_set_size(row_labels[i],menu?187:212,menu?41:22);lv_obj_set_pos(row_labels[i],menu?29:5,menu?3:0);
        lv_obj_set_style_text_line_space(row_labels[i],menu?1:0,0);
        bool active=i+offset==v->selected;
        lv_obj_set_style_bg_color(rows[i],lv_color_hex(active?ink:bg),0);
        lv_obj_set_style_text_color(row_labels[i],lv_color_hex(active?paper:ink),0);
        lv_label_set_text(row_labels[i],v->options[i+offset]);
        if(menu){
            lv_point_t text_size;
            lv_text_get_size(&text_size,v->options[i+offset],&niuma_font,0,1,187,LV_TEXT_FLAG_NONE);
            lv_obj_set_height(row_labels[i],text_size.y);
            lv_obj_set_y(row_labels[i],(43-text_size.y)/2);
        }
        if(home)lv_obj_add_flag(row_labels[i],LV_OBJ_FLAG_HIDDEN);
        else lv_obj_remove_flag(row_labels[i],LV_OBJ_FLAG_HIDDEN);
    }
    if(v->kind==NM_VIEW_MENU || is_home)lv_label_set_text(body,"");
    if(v->kind==NM_VIEW_STATS){
        lv_obj_set_pos(body,12,202);lv_obj_set_size(body,216,22);
        lv_label_set_text(body,s->burnout?(s->english?"Burnout: take recovery":"倦怠中：请休养"):(s->english?"Lower stress is better":"压力越低越好"));
    }
    if(chrome_changed)lv_obj_invalidate(screen);
}
