#include "niuma_view.h"
#include "lvgl.h"
#include <stdio.h>

LV_FONT_DECLARE(niuma_font);
static lv_obj_t *screen,*heading,*page_label,*status_label,*battery_label,*body,*hint,*picture,*rows[5],*row_labels[5];
static uint16_t art_pixels[NM_ART_W*NM_ART_H];
static lv_image_dsc_t art_image;
static const uint32_t bg=0xcbd0b5,ink=0x252b24,paper=0xeeebd8;
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
    status_label=label(screen,8,3,148,22);battery_label=label(screen,160,3,72,22);
    lv_obj_set_style_text_align(battery_label,LV_TEXT_ALIGN_RIGHT,0);
    heading=label(screen,8,29,224,23);
    page_label=label(screen,184,29,48,23);
    lv_obj_set_style_text_align(page_label,LV_TEXT_ALIGN_RIGHT,0);
    body=label(screen,12,65,216,170);hint=label(screen,4,299,232,21);
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
        rows[i]=lv_obj_create(screen);lv_obj_remove_style_all(rows[i]);
        lv_obj_set_style_bg_opa(rows[i],LV_OPA_COVER,0);lv_obj_remove_flag(rows[i],LV_OBJ_FLAG_SCROLLABLE);
        row_labels[i]=label(rows[i],5,2,212,39);
    }
    lv_screen_load(screen);return true;
}
void nm_view_draw(const nm_view_t *v,const nm_state_t *s,const nm_game_t *g,nm_store_status_t store,unsigned frame){
    char top[100],battery[24];
    bool low=store.battery>=0 && store.battery<=15;
    if(store.battery<0)snprintf(battery,sizeof(battery),s->english?"BAT --":"电量 --");
    else snprintf(battery,sizeof(battery),s->english?"%d%%%s":"电%d%%%s",store.battery,low?"!":"");
    snprintf(top,sizeof(top),s->english?"D%lu V%u%s":"D%lu 音%u%s",(unsigned long)s->day,s->volume,store.phase==NM_STORE_ERROR?" !":store.pending?" *":"");
    lv_label_set_text(status_label,top);lv_label_set_text(battery_label,battery);
    lv_obj_set_style_bg_opa(battery_label,low?LV_OPA_COVER:LV_OPA_TRANSP,0);
    lv_obj_set_style_bg_color(battery_label,lv_color_hex(ink),0);
    lv_obj_set_style_text_color(battery_label,lv_color_hex(low?paper:ink),0);
    lv_label_set_text(heading,v->title);lv_label_set_text(body,v->body);
    bool paged=v->kind==NM_VIEW_MENU && v->count>5;
    lv_obj_set_width(heading,paged?172:224);
    if(paged){
        char pages[16];
        snprintf(pages,sizeof(pages),"%u/%u",v->selected/5+1,(v->count+4)/5);
        lv_label_set_text(page_label,pages);
        lv_obj_remove_flag(page_label,LV_OBJ_FLAG_HIDDEN);
    }else lv_obj_add_flag(page_label,LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(hint,v->hint[0]?v->hint:s->english?"UP/DN Select  OK Enter":"上下选择 确定进入 长按返回");
    bool scene=v->kind==NM_VIEW_SCENE || v->kind==NM_VIEW_HOME || v->kind==NM_VIEW_GAME || v->kind==NM_VIEW_MEETING || v->kind==NM_VIEW_ESCAPE;
    if(scene){
        if(v->kind==NM_VIEW_GAME || v->kind==NM_VIEW_MEETING || v->kind==NM_VIEW_ESCAPE)nm_art_game(art_pixels,s,g);
        else if(v->preview && v->preview_item<12){
            nm_state_t preview=*s;preview.equipped[v->preview_item/4]=v->preview_item;
            nm_art_render(art_pixels,&preview,v->scene,frame);
        }else nm_art_render(art_pixels,s,v->scene,frame);
        lv_obj_remove_flag(picture,LV_OBJ_FLAG_HIDDEN);
        bool big=v->kind==NM_VIEW_HOME || v->kind==NM_VIEW_GAME;
        lv_image_set_scale(picture,big?512:256);lv_obj_set_pos(picture,big?0:60,54);
        lv_obj_invalidate(picture);
        lv_obj_set_pos(body,12,big?217:136);lv_obj_set_size(body,216,big?75:86);
    }else{lv_obj_add_flag(picture,LV_OBJ_FLAG_HIDDEN);lv_obj_set_pos(body,12,65);lv_obj_set_size(body,216,155);}
    unsigned offset=v->kind==NM_VIEW_MENU?(v->selected/5)*5:0;
    for(unsigned i=0;i<5;++i){
        if(i+offset>=v->count){lv_obj_add_flag(rows[i],LV_OBJ_FLAG_HIDDEN);continue;}
        lv_obj_remove_flag(rows[i],LV_OBJ_FLAG_HIDDEN);
        bool home=v->kind==NM_VIEW_HOME,menu=v->kind==NM_VIEW_MENU;
        lv_obj_set_pos(rows[i],home?(int)i*48+1:8,home?270:menu?65+i*45:224+i*24);
        lv_obj_set_size(rows[i],home?46:224,menu?43:23);
        lv_obj_set_size(row_labels[i],home?44:212,menu?41:22);lv_obj_set_pos(row_labels[i],home?1:5,0);
        bool active=i+offset==v->selected;
        lv_obj_set_style_bg_color(rows[i],lv_color_hex(active?ink:bg),0);
        lv_obj_set_style_text_color(row_labels[i],lv_color_hex(active?paper:ink),0);
        lv_label_set_text(row_labels[i],v->options[i+offset]);
    }
    if(v->kind==NM_VIEW_MENU)lv_label_set_text(body,"");
}
