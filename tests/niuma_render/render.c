#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lvgl.h"
#include "niuma_storage.h"
#include "niuma_sound.h"
#include "freertos/queue.h"

typedef struct {uint8_t data[4096];size_t size;int capacity,head,tail,n;} queue_t;
QueueHandle_t xQueueCreate(int count,size_t size){queue_t*q=calloc(1,sizeof(*q));q->capacity=count;q->size=size;assert(count*size<=sizeof(q->data));return q;}
int xQueueSend(QueueHandle_t h,const void *d,int t){(void)t;queue_t*q=h;if(q->n==q->capacity)return 0;memcpy(q->data+q->tail*q->size,d,q->size);q->tail=(q->tail+1)%q->capacity;q->n++;return 1;}
int xQueueReceive(QueueHandle_t h,void *d,int t){(void)t;queue_t*q=h;if(!q->n)return 0;memcpy(d,q->data+q->head*q->size,q->size);q->head=(q->head+1)%q->capacity;q->n--;return 1;}
void vQueueDelete(QueueHandle_t q){free(q);}
static nm_state_t saved_state;
static unsigned saves,light=60;
static bool reject_save,override_store;
static nm_store_status_t fake_store;
static bool mock_loaded;
static nm_state_t mock_restored;
static unsigned reload_calls;
void bsp_display_backlight(uint8_t v){light=v;}
bool nm_storage_request(const nm_state_t *s){assert(nm_valid(s));if(reject_save)return false;saved_state=*s;saves++;return true;}
nm_store_status_t nm_storage_status(void){if(override_store)return fake_store;return (nm_store_status_t){.phase=saves?NM_STORE_SAVED:NM_STORE_EMPTY,.battery=78,.saved_revision=saved_state.revision};}
bool nm_storage_loaded(nm_state_t *s){if(mock_loaded)*s=mock_restored;return mock_loaded;}
bool nm_storage_reload(void){reload_calls++;fake_store.phase=NM_STORE_LOADING;return true;}
static uint8_t audio_volume;
static bool audio_music,audio_sleep;
void nm_audio_config(uint8_t v,bool m,bool sleep){audio_volume=v;audio_music=m;audio_sleep=sleep;}
void nm_audio_play(nm_sfx_t s){assert((unsigned)s<NM_SFX_COUNT);}
static bool audio_available=true;
bool nm_audio_ready(void){return audio_available;}
#include "../../main/niuma_view.c"
#include "../../main/niuma_app.c"

static uint16_t fb[240*320],dma[240*20];
static void flush(lv_display_t*d,const lv_area_t*a,uint8_t*p){
    uint16_t *src=(uint16_t*)p;
    for(int y=a->y1;y<=a->y2;++y)for(int x=a->x1;x<=a->x2;++x){assert(x>=0&&x<240&&y>=0&&y<320);fb[y*240+x]=*src++;}
    lv_display_flush_ready(d);
}
static void advance(int ms){for(int t=0;t<ms;t+=10){lv_tick_inc(10);lv_timer_handler();}}
static void check_corners(void){
    for(int y=0;y<16;++y)for(int x=0;x<16;++x){
        int dx=31-2*x,dy=31-2*y;
        if(dx*dx+dy*dy<=32*32)continue;
        assert(fb[y*240+x]==0 && fb[y*240+239-x]==0);
        assert(fb[(319-y)*240+x]==0 && fb[(319-y)*240+239-x]==0);
    }
    assert(fb[120]!=0 && fb[319*240+120]!=0);
}
static void capture_frame(const char *name,unsigned frame){
    describe();nm_view_draw(&view,&state,&game,nm_storage_status(),frame);lv_obj_invalidate(screen);lv_refr_now(NULL);
    if(view.kind==NM_VIEW_HOME){
        unsigned crop_y=view.scene==NM_SCENE_HOME?12:20;
        assert(art_image.data==(const uint8_t *)(art_pixels+crop_y*NM_ART_W+20));
    }
    check_corners();
    char path[256];snprintf(path,sizeof(path),"/tmp/niuma-%s.ppm",name);
    FILE*f=fopen(path,"wb");assert(f);fprintf(f,"P6\n240 320\n255\n");
    for(int i=0;i<240*320;++i){uint16_t p=fb[i];uint8_t rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,f);}fclose(f);
}
static void capture(const char *name){capture_frame(name,1);}
static void press(bsp_btn_t key,bsp_btn_ev_t event){niuma_app_key(key,event,NULL);advance(100);}
static unsigned layout_errors;
static void check_label(lv_obj_t *obj,const char *name){
    if(lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN))return;
    if(view.kind==NM_VIEW_MENU)for(unsigned i=0;i<5;++i)if(obj==row_labels[i]){
        assert(abs(2*lv_obj_get_y(obj)+lv_obj_get_height(obj)-43)<=1);
        assert(lv_obj_get_y(obj)>=0 && lv_obj_get_y(obj)+lv_obj_get_height(obj)<=43);
    }
    if(obj==status_label || obj==battery_label || obj==hint){
        lv_area_t area;lv_obj_get_coords(obj,&area);
        for(int side=0;side<2;++side)for(int bottom=0;bottom<2;++bottom){
            int x=side?area.x2:area.x1,y=bottom?area.y2:area.y1;
            assert(x>=0 && x<240 && y>=0 && y<320);
            int edge_x=x<120?x:239-x,edge_y=y<160?y:319-y;
            if(edge_x<16 && edge_y<16){
                int dx=31-2*edge_x,dy=31-2*edge_y;
                assert(dx*dx+dy*dy<=32*32);
            }
        }
    }
    const unsigned char *text=(const unsigned char *)lv_label_get_text(obj);
    while(*text){
        uint32_t cp=*text++;unsigned continuation=0;
        if(cp>=0xf0){cp&=7;continuation=3;}else if(cp>=0xe0){cp&=15;continuation=2;}else if(cp>=0xc0){cp&=31;continuation=1;}
        while(continuation--){assert((*text&0xc0)==0x80);cp=(cp<<6)|(*text++&63);}
        if(cp<32)continue;
        lv_font_glyph_dsc_t glyph;
        assert(lv_font_get_glyph_dsc(&niuma_font,&glyph,cp,0) && !glyph.is_placeholder);
    }
    lv_point_t size;
    lv_text_get_size(&size,lv_label_get_text(obj),&niuma_font,0,0,lv_obj_get_width(obj),LV_TEXT_FLAG_NONE);
    if(size.y>lv_obj_get_height(obj)){
        printf("LAYOUT page=%d lang=%u %s needs %ld has %ld: %s\n",page,state.english,name,(long)size.y,(long)lv_obj_get_height(obj),lv_label_get_text(obj));layout_errors++;
    }
}
static void game_visuals(void){
    for(unsigned lang=0;lang<2;++lang)for(unsigned kind=0;kind<NM_GAME_COUNT;++kind){
        nm_init(&state,123,true,0);state.english=lang;loaded=true;home();
        start_game((nm_game_kind_t)kind);assert(page==P_GAME);nm_game_tick(&game,1200);
        if(kind==NM_GAME_TEA){
            describe();assert(strstr(view.body,lang?"Sipping tea":"正在喝茶") && !strstr(view.body,lang?"Target":"目标"));
            nm_game_press(&game);describe();assert(strstr(view.body,lang?"Acting busy":"正在假装忙碌"));
            nm_game_press(&game);
        }
        char name[60];snprintf(name,sizeof(name),"game-%u-%s",kind,lang?"en":"zh");capture(name);
    }
    home();game_paid=false;
}
static void scene_atlas(void){
    enum {columns=5,rows=(NM_SCENE_COUNT+columns-1)/columns,width=columns*(NM_ART_W+4),height=2*rows*(NM_ART_H+4)};
    static uint16_t atlas[width*height],tile[NM_ART_W*NM_ART_H];
    for(unsigned i=0;i<width*height;++i)atlas[i]=0xef5b;
    for(unsigned gender=0;gender<2;++gender)for(unsigned scene=0;scene<NM_SCENE_COUNT;++scene){
        nm_state_t sample;nm_init(&sample,1,gender,0);nm_art_render(tile,&sample,(nm_scene_t)scene,1);
        unsigned x=(scene%columns)*(NM_ART_W+4),y=(gender*rows+scene/columns)*(NM_ART_H+4);
        for(unsigned line=0;line<NM_ART_H;++line)memcpy(atlas+(y+line)*width+x,tile+line*NM_ART_W,NM_ART_W*sizeof(uint16_t));
    }
    FILE *out=fopen("/tmp/niuma-scene-atlas.ppm","wb");assert(out);fprintf(out,"P6\n%d %d\n255\n",width,height);
    for(unsigned i=0;i<width*height;++i){uint16_t p=atlas[i];uint8_t rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};assert(fwrite(rgb,1,3,out)==3);}
    assert(fclose(out)==0);
}
static void lifecycle_pages(void){
    for(unsigned gender=0;gender<2;++gender){
        nm_init(&state,1,gender,0);state.english=1;loaded=true;home();
        const char *poses[]={"new","work","tired","leave","return"};
        for(unsigned pose=0;pose<5;++pose){
            state.burnout=pose==2;
            if(pose==0)page=P_CREATE;
            else if(pose==1 || pose==2)page=P_HOME;
            else if(pose==3)page=P_LEAVING;
            else{page=P_RESULT;copy(result_title,sizeof(result_title),"Rehired");copy(result_body,sizeof(result_body),"Your story continues.");result_scene=NM_SCENE_OFFICE;}
            selection=0;char name[80];snprintf(name,sizeof(name),"lifecycle-%s-%s",gender?"female":"male",poses[pose]);capture(name);
        }
    }
    home();
}
static void morning_pages(void){
    for(unsigned lang=0;lang<2;++lang){
        nm_init(&state,311,true,0);state.english=lang;loaded=true;home();
        for(unsigned day=1;day<=8;++day){
            prepare_action(NM_BASIC_MEAL);selection=0;activate();home();finish_day();activate();
            unsigned coins=state.coins;selection=0;activate();assert(page==P_MORNING && state.day==day+1 && state.actions==6);
            nm_state_t before=state;unsigned writes=saves;
            describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"morning summary");
            assert(view.scene==(nm_weekend(&state)?NM_SCENE_PARK:NM_SCENE_HOME));
            if(state.day==6)capture(lang?"morning-weekend-en":"morning-weekend-zh");
            selection=1;activate();assert(page==P_STATS);press(BSP_BTN_OK,BSP_BTN_LONG);assert(page==P_MORNING);
            press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HOME && !memcmp(&state,&before,sizeof(state)) && writes==saves && state.coins==coins);
        }
    }
}
static void memory_pages(void){
    for(unsigned lang=0;lang<2;++lang)for(unsigned unlocked=0;unlocked<2;++unlocked){
        nm_init(&state,43,true,0);state.english=lang;state.day=28;loaded=true;
        if(unlocked){state.achievements=(1u<<NM_ACHIEVEMENT_COUNT)-1;for(unsigned i=0;i<NM_ACHIEVEMENT_COUNT;++i)state.memory_day[i]=i+1;}
        home();go(P_ALBUM);nm_state_t before=state;unsigned writes=saves;
        for(unsigned i=0;i<NM_ACHIEVEMENT_COUNT;++i){
            describe();assert(strstr(view.options[i],unlocked?"[+]":"[-]"));selection=i;activate();describe();
            assert(page==P_MEMORY && !strcmp(view.title,tr(memory_zh[i],memory_en[i])));
            if(unlocked){char day[40];snprintf(day,sizeof(day),lang?"Day %u":"第 %u 天",i+1);assert(strstr(view.body,day));}
            else assert(strstr(view.body,lang?"Not unlocked":"尚未解锁"));
            nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"memory story or requirements");check_label(heading,"memory title");
            if(i==NM_LIFE_MASTER){assert(view.count==(unlocked?2:1));capture(lang?"milestone-en":"milestone-zh");}
            press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_ALBUM);
            assert(!memcmp(&before,&state,sizeof(state)) && saves==writes);
        }
        if(unlocked){selection=NM_LIFE_MASTER;activate();selection=1;activate();assert(page==P_HOME && !memcmp(&before,&state,sizeof(state)));}
    }
}
static void resource_fallbacks(void){
    for(unsigned lang=0;lang<2;++lang){
        nm_init(&state,876,true,0);state.english=lang;state.coins=0;loaded=true;home();
        for(unsigned choice=0;choice<2;++choice){
            prepare_action(NM_BENTO);nm_state_t before=state;unsigned writes=saves;
            press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_POOR && !memcmp(&state,&before,sizeof(state)) && saves==writes);
            describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"no money");capture(lang?"poor-en":"poor-zh");
            selection=2;activate();assert(page==P_ACTION && !memcmp(&state,&before,sizeof(state)));
            selection=0;activate();assert(page==P_POOR);selection=choice;activate();
            assert(page==P_ACTION && pending_action==(choice?NM_RECOVER:NM_BASIC_MEAL));
            assert(!memcmp(&state,&before,sizeof(state)) && saves==writes);
            nm_effect_t effect;nm_state_t expected=state;assert(nm_act(&expected,pending_action,0,&effect)==NM_OK);
            selection=0;activate();assert(page==P_RESULT && !memcmp(&state,&expected,sizeof(state)) && state.coins==0);
            assert(!memcmp(&saved_state,&state,sizeof(state)));home();
        }
        while(state.actions){prepare_action(NM_RECOVER);selection=0;activate();home();}
        nm_state_t before=state;unsigned writes=saves;
        prepare_action(NM_BENTO);activate();assert(page==P_EXHAUSTED && saves==writes && !memcmp(&state,&before,sizeof(state)));
        describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"no actions");capture(lang?"exhausted-en":"exhausted-zh");
        selection=1;activate();assert(page==P_STATS && !memcmp(&state,&before,sizeof(state)));
        press(BSP_BTN_OK,BSP_BTN_LONG);assert(page==P_EXHAUSTED);
        press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_PAY && state.evening && state.day==before.day);
        unsigned coins=state.coins;press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HOME);
        prepare_action(NM_RECOVER);selection=0;activate();assert(page==P_EXHAUSTED);
        describe();assert(!strcmp(view.options[0],lang?"Sleep until tomorrow":"睡觉，开始明天"));
        selection=0;activate();assert(page==P_MORNING && state.day==before.day+1 && state.actions==6 && state.coins==coins);
        press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HOME);
        assert(!memcmp(&saved_state,&state,sizeof(state)));
        /* Only the designated hold event resets; press/double-click do not. */
        go(P_RESET);before=state;writes=saves;
        press(BSP_BTN_OK,BSP_BTN_PRESS);press(BSP_BTN_OK,BSP_BTN_DOUBLE);
        assert(page==P_RESET && !memcmp(&state,&before,sizeof(state)) && saves==writes);
        press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HOME && saves==writes);
    }
}
static void rest_and_notebook(void){
    for(unsigned lang=0;lang<2;++lang){
        nm_init(&state,714,true,0);state.english=lang;loaded=true;home();
        prepare_action(NM_OVERTIME);assert(selection==1);
        nm_state_t before=state;unsigned writes=saves;
        press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HOME && saves==writes && !memcmp(&state,&before,sizeof(state)));
        go(P_STUDY);selection=0;activate();selection=0;activate();assert(page==P_RESULT);
        home();go(P_STUDY);selection=3;activate();assert(page==P_NOTEBOOK);
        before=state;writes=saves;describe();
        assert(strstr(view.body,lang?"Tech 12":"专业 12") && strstr(view.body,lang?"Career XP 6":"职业经验 6"));
        nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"learning notebook");capture(lang?"notebook-en":"notebook-zh");
        press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_STUDY && saves==writes && !memcmp(&state,&before,sizeof(state)));
        for(unsigned mode=0;mode<3;++mode){
            state.volume=mode==1?0:40;audio_available=mode!=2;home();go(P_REST);selection=5;activate();assert(page==P_MUSIC);
            before=state;writes=saves;advance(100);assert(!audio_music && saves==writes && !memcmp(&state,&before,sizeof(state)));
            selection=1;activate();assert(page==P_REST && !memcmp(&state,&before,sizeof(state)));
            selection=5;activate();describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"music preview");
            nm_state_t expected=state;nm_effect_t e;assert(nm_act(&expected,NM_REST,0,&e)==NM_OK);
            press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_MUSIC_RESULT && audio_music && !audio_sleep && audio_volume==state.volume);
            assert(!memcmp(&state,&expected,sizeof(state)) && !memcmp(&state,&saved_state,sizeof(state)) && !state.music);
            describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"music result");capture(lang?"music-rest-en":"music-rest-zh");
            if(mode==0){state.idle_seconds=30;advance(31000);assert(sleeping && audio_sleep);press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_MUSIC_RESULT && !audio_sleep);}
            press(BSP_BTN_OK,BSP_BTN_LONG);assert(page==P_MUSIC && !audio_music);
            home();advance(100);assert(!audio_music);
        }
        /* Leaving listening mode retains an explicitly enabled global melody. */
        state.music=true;home();advance(100);assert(audio_music);state.music=false;audio_available=true;
        state.actions=0;go(P_REST);selection=5;activate();selection=0;activate();advance(100);
        assert(page==P_EXHAUSTED && !audio_music);
    }
    audio_available=true;home();
}
static void career_gallery_and_transitions(void){
    for(unsigned lang=0;lang<2;++lang)for(unsigned gender=0;gender<2;++gender){
        nm_init(&state,431+gender,gender,0);state.english=lang;loaded=true;
        nm_effect_t effect;assert(nm_act(&state,NM_CHAT,0,&effect)==NM_OK);
        assert(nm_style(&state)==NM_SOCIAL);
        home();go(P_CAREER);selection=1;activate();assert(page==P_STYLE && style_choice==NM_SOCIAL);
        nm_state_t before=state;unsigned writes=saves;
        unsigned seen=0;
        for(unsigned i=0;i<6;++i){
            unsigned choice=(NM_SOCIAL+i)%6;assert(style_choice==choice);seen|=1u<<choice;
            describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);
            assert(strstr(view.body,choice==NM_SOCIAL?tr("这是你的当前风格","Your current style"):tr("其他可养成的风格","Another possible style")));
            check_label(body,"style description");check_label(heading,"style title");
            if(i==0 && gender==1)capture(lang?"style-current-en":"style-current-zh");
            press(BSP_BTN_OK,BSP_BTN_CLICK);
            assert(!memcmp(&state,&before,sizeof(state)) && saves==writes);
        }
        assert(seen==63 && style_choice==NM_SOCIAL);
        press(BSP_BTN_DOWN,BSP_BTN_CLICK);press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_CAREER);
        for(unsigned employer=0;employer<3;++employer){
            go(P_RESIGN);describe();before=state;writes=saves;
            press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_CAREER && saves==writes && !memcmp(&state,&before,sizeof(state)));
            go(P_RESIGN);selection=1;activate();describe();
            assert(page==P_LEAVING && view.scene==NM_SCENE_LEAVE && state.company==NM_UNEMPLOYED);
            assert(saves==writes+1 && !memcmp(&state,&saved_state,sizeof(state)));
            assert(state.coins==before.coins && state.xp==before.xp && state.actions==before.actions);
            assert(!memcmp(state.skill,before.skill,sizeof(state.skill)) && !memcmp(state.relations,before.relations,sizeof(state.relations)));
            assert(state.owned==before.owned && state.gender==before.gender);
            assert(state.achievements&(1u<<NM_RESIGNED));
            nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"leaving message");
            if(employer==0)capture(lang?"leaving-en":"leaving-zh");
            /* Rest is safe, then employers remain accessible from career. */
            if(employer==0){selection=1;activate();assert(page==P_HOME);go(P_CAREER);selection=4;activate();}
            else activate();
            assert(page==P_COMPANY);selection=employer;activate();describe();
            assert(page==P_RESULT && view.scene==NM_SCENE_OFFICE && state.company==employer);
            assert(!memcmp(&state,&saved_state,sizeof(state)) && state.coins==before.coins);
            capture(lang?"rehired-en":"rehired-zh");
            /* Returning must not reopen a stale resignation / employer dialog. */
            press(BSP_BTN_OK,BSP_BTN_LONG);assert(page==P_HOME);
            go(P_CAREER);
        }
    }
}
static void menu_pagination(void){
    for(unsigned lang=0;lang<2;++lang){
        state.english=lang;home();go(P_CAREER);describe();
        assert(view.count==6);
        for(unsigned n=0;n<6;++n){
            describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);
            lv_obj_update_layout(screen);
            assert(selection==n);
            assert(!strcmp(lv_label_get_text(page_label),n<5?"1/2":"2/2"));
            assert(!lv_obj_has_flag(page_label,LV_OBJ_FLAG_HIDDEN));
            unsigned offset=(n/5)*5;
            for(unsigned i=0;i<5;++i){
                assert(lv_obj_has_flag(rows[i],LV_OBJ_FLAG_HIDDEN)==(offset+i>=view.count));
                if(offset+i<view.count)assert(!strcmp(lv_label_get_text(row_labels[i]),view.options[offset+i]));
            }
            check_label(heading,"paged title");check_label(page_label,"page counter");
            if(n==5)capture(lang?"menu-page-en":"menu-page-zh");
            press(BSP_BTN_DOWN,BSP_BTN_CLICK);
        }
        assert(selection==0);
        press(BSP_BTN_UP,BSP_BTN_CLICK);assert(selection==5);
        press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_ROUTES);
        press(BSP_BTN_OK,BSP_BTN_LONG);assert(page==P_CAREER && selection==0);
        home();describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);
        lv_obj_update_layout(screen);
        assert(lv_obj_has_flag(page_label,LV_OBJ_FLAG_HIDDEN));
        assert(lv_obj_get_width(heading)==224);
    }
}
static void settings_and_save(void){
    home();go(P_SETTINGS);go(P_BRIGHT);
    unsigned original=state.brightness,rev=state.revision;
    press(BSP_BTN_DOWN,BSP_BTN_CLICK);assert(state.brightness==original+10 && light==original+10);
    save();assert(saved_state.brightness==original);
    press(BSP_BTN_OK,BSP_BTN_LONG);assert(state.brightness==original && light==original && state.revision==rev);
    go(P_BRIGHT);press(BSP_BTN_DOWN,BSP_BTN_CLICK);press(BSP_BTN_OK,BSP_BTN_CLICK);
    assert(state.brightness==original+10 && saved_state.brightness==state.brightness);
    go(P_SOUND);original=state.volume;rev=state.revision;unsigned calls=saves;
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(adjusting && saves==calls);
    press(BSP_BTN_DOWN,BSP_BTN_CLICK);assert(state.volume==original+10);
    save();assert(saved_state.volume==original);
    press(BSP_BTN_OK,BSP_BTN_LONG);assert(state.volume==original && state.revision==rev);
    go(P_SOUND);press(BSP_BTN_OK,BSP_BTN_CLICK);press(BSP_BTN_DOWN,BSP_BTN_CLICK);press(BSP_BTN_OK,BSP_BTN_CLICK);
    assert(!adjusting && state.volume==original+10 && saved_state.volume==state.volume);
    go(P_SAVE_STATUS);state.english=1;
    override_store=true;fake_store=(nm_store_status_t){.phase=NM_STORE_SAVED,.pending=true,.battery=-1};
    describe();assert(strstr(view.body,"Saving"));
    fake_store.pending=false;fake_store.saved_revision=state.revision;describe();assert(strstr(view.body,"Safe to exit"));
    fake_store.phase=NM_STORE_ERROR;describe();assert(strstr(view.body,"failed"));
    reject_save=true;activate();assert(save_request_failed);describe();assert(strstr(view.body,"failed"));
    reject_save=false;override_store=false;activate();describe();assert(!save_request_failed && strstr(view.body,"Safe to exit"));
    state.english=0;home();
}
static void game_guides(void){
    nm_state_t original=state;
    for(unsigned k=0;k<NM_GAME_COUNT;++k){
        nm_init(&state,17,true,0);home();go(P_GAMES);selection=k;activate();
        assert(page==P_GAME_GUIDE && state.actions==6 && state.coins==30);
        nm_state_t before=state;selection=2;activate();assert(page==P_GAMES && !memcmp(&before,&state,sizeof(state)));
        prepare_game((nm_game_kind_t)k);selection=1;activate();
        nm_state_t expected=before;assert(nm_act(&expected,game_action((nm_game_kind_t)k),friend_id,NULL)==NM_OK);
        assert(page==P_RESULT && !memcmp(&expected,&state,sizeof(state)));
        nm_init(&state,17,true,0);prepare_game((nm_game_kind_t)k);selection=0;activate();
        assert(page==P_GAME && state.actions==5);unsigned cash=state.coins;
        go(P_GAME_PAUSE);selection=1;activate();assert(page==P_HOME && state.actions==5 && state.coins==cash);
        for(unsigned lang=0;lang<2;++lang){
            state.english=lang;prepare_game((nm_game_kind_t)k);describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);
            lv_obj_update_layout(screen);check_label(body,"game guide");
            for(unsigned i=0;i<3;++i)check_label(row_labels[i],"guide button");
            if(k==NM_GAME_TEA && lang==0)capture("tea-guide");
        }
    }
    state=original;home();save();
}
static void action_previews(void){
    nm_state_t original=state;
    nm_init(&state,1,true,0);state.english=1;state.coins=100;state.coffees=1;state.stat[NM_ENERGY]=90;
    prepare_action(NM_COFFEE);nm_state_t before=state;describe();
    assert(!memcmp(&state,&before,sizeof(state)));
    assert(strstr(view.body,"Energy +10") && strstr(view.body,"Stress +10"));
    prepare_action(NM_COOK);describe();assert(strstr(view.body,"Requires Life skill 24"));
    for(unsigned lang=0;lang<2;++lang){state.english=lang;
        for(unsigned a=0;a<NM_ACTION_COUNT;++a){prepare_action((nm_action_t)a);describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"action preview");}
    }
    state=original;home();save();
}
static void startup_protection(void){
    nm_state_t original=state;unsigned calls=saves;
    loaded=false;started=false;sleeping=false;page=P_BOOT;depth=0;
    override_store=true;mock_loaded=false;fake_store=(nm_store_status_t){.phase=NM_STORE_ERROR,.battery=-1};
    advance(100);assert(page==P_START_ERROR && !loaded);
    advance(61000);assert(sleeping && saves==calls);
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(!sleeping && page==P_START_ERROR);
    press(BSP_BTN_OK,BSP_BTN_LONG);assert(page==P_BOOT);
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_START_ERROR && saves==calls);
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(reload_calls && !started && saves==calls);
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(!started && page==P_BOOT);
    nm_init(&mock_restored,4,true,2);mock_restored.coins=777;mock_restored.revision=44;
    mock_loaded=true;fake_store.phase=NM_STORE_SAVED;fake_store.saved_revision=44;
    advance(100);assert(loaded && state.coins==777 && saves==calls);
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HOME);
    go(P_RESET);press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HOME && state.coins==777 && saves==calls);
    go(P_RESET);press(BSP_BTN_OK,BSP_BTN_LONG);assert(page==P_CREATE && !loaded && saves==calls+1);
    assert(saved_state.coins==30 && saved_state.revision==45);
    press(BSP_BTN_OK,BSP_BTN_LONG);assert(page==P_BOOT);advance(61000);assert(saves==calls+1);
    state=original;loaded=true;started=true;sleeping=false;override_store=false;mock_loaded=false;
    last_input=lv_tick_get();home();save();
}
static void device_states(void){
    nm_state_t original=state;home();go(P_SETTINGS);selection=6;activate();assert(page==P_DEVICE);
    override_store=true;fake_store=(nm_store_status_t){.phase=NM_STORE_SAVED,.saved_revision=state.revision};
    const int battery_values[]={-1,0,15,16,100};
    for(unsigned lang=0;lang<2;++lang){state.english=lang;
        for(unsigned i=0;i<5;++i){fake_store.battery=battery_values[i];audio_available=i%2;
            describe();nm_view_draw(&view,&state,&game,fake_store,1);lv_obj_update_layout(screen);
            check_label(body,"device status");check_label(status_label,"status bar");check_label(battery_label,"battery bar");
            const char *battery=lv_label_get_text(battery_label);
            if(i==0)assert(strstr(battery,"--"));
            if(i==1 || i==2)assert(strchr(battery,'!'));
            if(i>=3)assert(!strchr(battery,'!'));
            assert(lv_obj_get_x(status_label)+lv_obj_get_width(status_label)<=lv_obj_get_x(battery_label));
            lv_refr_now(NULL);
            /* Battery cells, unavailable dash and low-charge warning are drawn,
             * not merely represented by hidden text. */
            assert(fb[9*240+195]==lv_color_to_u16(lv_color_hex(ink)));
            if(fake_store.battery>0)assert(fb[13*240+198]==lv_color_to_u16(lv_color_hex(ink)));
            if(fake_store.battery<0)assert(fb[14*240+207]==lv_color_to_u16(lv_color_hex(ink)));
            if(lang==0 && i==1)capture("device-low-battery");
        }
    }
    state=original;override_store=false;audio_available=true;home();
}
static void review_pages(void){
    nm_state_t original=state;
    for(unsigned lang=0;lang<2;++lang){
        nm_init(&state,1,true,0);state.english=lang;home();go(P_REVIEW);describe();assert(strstr(view.body,"27"));
        nm_state_t before=state;unsigned calls=saves;activate();assert(page==P_RESULT && !memcmp(&before,&state,sizeof(state)) && calls==saves);home();
        while(state.day<=56){
            const nm_action_t actions[]={NM_BASIC_MEAL,NM_STUDY_TALK,NM_STUDY_TALK,NM_RECOVER};
            for(unsigned a=0;a<4;++a){prepare_action(actions[a]);selection=0;activate();assert(page==P_RESULT);home();}
            if(state.day==28 || state.day==56){
                unsigned day=state.day;go(P_REVIEW);selection=0;activate();
                assert(page==P_RESULT && state.salary==(day==28?20:24));
                assert(strstr(result_body,day==28?"28":"20 -> 24"));
                describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"salary result");
                if(day==56)capture(lang?"raise-en":"raise-zh");
                assert(!memcmp(&saved_state,&state,sizeof(state)));home();
            }
            finish_day();activate();finish_day();home();
        }
        state.salary=100;go(P_REVIEW);describe();assert(strstr(view.body,"100"));
        before=state;calls=saves;selection=0;activate();assert(page==P_RESULT && !memcmp(&before,&state,sizeof(state)) && calls==saves);
        state.company=NM_UNEMPLOYED;home();go(P_REVIEW);describe();assert(strstr(view.body,lang?"employer":"公司"));
    }
    state=original;home();save();
}
static void home_contexts(void){
    nm_state_t original=state;
    for(unsigned lang=0;lang<2;++lang){
        nm_init(&state,1,true,0);state.english=lang;home();
        while(state.day<6){finish_day();activate();finish_day();activate();}
        describe();assert(!strcmp(view.options[0],lang?"Walk":"散步"));
        assert(strstr(view.body,lang?"Food":"饱腹") && strstr(view.body,lang?"HP":"健康"));
        nm_state_t before=state;selection=0;activate();assert(page==P_ACTION && pending_action==NM_EXERCISE && !memcmp(&state,&before,sizeof(state)));
        selection=1;activate();assert(page==P_HOME);capture(lang?"weekend-en":"weekend-zh");
        while(state.day<8){finish_day();activate();finish_day();}
        assert(nm_resign(&state)==NM_OK);home();describe();assert(!strcmp(view.options[0],lang?"Paths":"去向"));
        selection=0;activate();assert(page==P_CAREER);
        state.skill[0]=96;assert(nm_choose_route(&state,NM_ROUTE_FREELANCE)==NM_OK);home();describe();
        assert(!strcmp(view.options[0],lang?"Work":"接单"));selection=0;activate();assert(page==P_WORK);
        home();finish_day();activate();describe();assert(!strcmp(view.options[0],lang?"Sleep":"睡觉"));
    }
    state=original;home();save();
}
static void burnout_pages(void){
    nm_state_t original=state;
    for(unsigned lang=0;lang<2;++lang){
        nm_init(&state,1,true,0);state.english=lang;home();
        for(unsigned day=0;day<4;++day){
            const nm_action_t actions[]={NM_OVERTIME,NM_COFFEE,NM_COFFEE,NM_NOODLES,NM_WORK,NM_WORK};
            for(unsigned a=0;a<6;++a){prepare_action(actions[a]);selection=0;activate();assert(page==P_RESULT);home();}
            finish_day();assert(page==P_PAY);activate();finish_day();
        }
        assert(page==P_BURNOUT && state.burnout && state.day==5 && state.actions==6);
        assert(!memcmp(&state,&saved_state,sizeof(state)));
        describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"burnout notice");
        capture(lang?"burnout-en":"burnout-zh");
        unsigned slots=state.actions;activate();assert(page==P_ACTION && pending_action==NM_RECOVER && state.actions==slots);
        selection=1;activate();assert(page==P_BURNOUT);selection=1;activate();assert(page==P_SOCIAL);
        home();page=P_BOOT;selection=0;activate();assert(page==P_BURNOUT);selection=2;activate();assert(page==P_HOME);
        for(unsigned day=0;day<2;++day){
            prepare_action(NM_BASIC_MEAL);selection=0;activate();home();
            while(state.actions){prepare_action(NM_RECOVER);selection=0;activate();home();}
            finish_day();activate();finish_day();
        }
        assert(page==P_BURNOUT && !state.burnout && state.day==7);
        describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"recovery notice");
        capture(lang?"recovered-en":"recovered-zh");activate();assert(page==P_HOME);
    }
    state=original;home();save();
}
static void action_scenes(void){
    nm_state_t original=state;
    for(unsigned lang=0;lang<2;++lang)for(unsigned a=0;a<NM_ACTION_COUNT;++a){
        nm_init(&state,1,true,0);state.english=lang;state.coins=200;state.skill[2]=24;
        nm_state_t before=state;
        home();prepare_action((nm_action_t)a);selection=0;activate();
        assert(page==P_RESULT && result_scene==nm_art_action_scene((nm_action_t)a));
        assert(last_effect.experience==state.xp-before.xp);
        for(unsigned i=0;i<3;++i){assert(last_effect.skill[i]==state.skill[i]-before.skill[i]);assert(last_effect.relations[i]==state.relations[i]-before.relations[i]);}
        describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);
        check_label(body,"action receipt");check_label(heading,"action title");
        if(a==NM_CLEAN || a==NM_TREAT || a==NM_TOILET || a==NM_WATER || a==NM_COOK || a==NM_STUDY_TECH || a==NM_CHAT){
            char filename[80];snprintf(filename,sizeof(filename),"action-%u-%s",a,lang?"en":"zh");capture(filename);
        }
    }
    state=original;home();save();
}
static void escape_pages(void){
    nm_state_t original=state;
    for(unsigned lang=0;lang<2;++lang){
        nm_init(&state,1,true,0);state.english=lang;home();start_game(NM_GAME_ESCAPE);
        for(unsigned phase=0;phase<5;++phase)for(unsigned route=0;route<2;++route){
            game.lift_arrival_ms=2000;game.lift_meeting=phase==0;game.lane=route;game.used=false;
            game.round_ms=phase==1?1000:phase==2?2000:4000;
            if(phase==4){game.used=true;game.feedback=2;}
            describe();assert(view.kind==NM_VIEW_ESCAPE && view.count==2 && view.selected==route);
            nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);
            check_label(body,"escape status");check_label(hint,"escape controls");
            for(unsigned i=0;i<2;++i)check_label(row_labels[i],"escape route");
            if(phase==0 && route==1)capture(lang?"escape-meeting-en":"escape-meeting-zh");
            if(phase==2 && route==0)capture(lang?"escape-lift-en":"escape-lift-zh");
        }
        home();start_game(NM_GAME_ESCAPE);game.lane=1;unsigned cash=state.coins;
        for(unsigned i=0;i<3;++i){
            press(BSP_BTN_OK,BSP_BTN_PRESS);press(BSP_BTN_OK,BSP_BTN_CLICK);
            if(i<2){assert(page==P_GAME && game.floors==2-i);advance(6100);}
        }
        assert(page==P_EVENT_RESULT && state.coins==cash+6);
        assert(!memcmp(&saved_state,&state,sizeof(state)));
        describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"escape result");
    }
    state=original;home();save();
}
static void meeting_pages(void){
    nm_state_t original=state;
    for(unsigned lang=0;lang<2;++lang){
        nm_init(&state,1,true,0);state.english=lang;home();start_game(NM_GAME_MEETING);
        for(unsigned topic=0;topic<6;++topic)for(unsigned response=0;response<3;++response){
            game.topic=topic;game.target=topic%3;game.lane=response;game.used=false;game.round_ms=0;
            for(unsigned answered=0;answered<2;++answered){
                if(answered)nm_game_press(&game);
                describe();assert(view.kind==NM_VIEW_MEETING && view.count==3 && view.selected==response);
                nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);
                check_label(body,"meeting topic");check_label(hint,"meeting hint");
                for(unsigned i=0;i<3;++i)check_label(row_labels[i],"meeting response");
            }
        }
        home();start_game(NM_GAME_MEETING);game.topic=2;game.target=2;game.lane=2;
        capture(lang?"meeting-en":"meeting-zh");
        nm_state_t before=state;
        press(BSP_BTN_OK,BSP_BTN_PRESS);press(BSP_BTN_OK,BSP_BTN_CLICK);
        assert(game.score==1 && game.meeting_answers[2]==1);
        state.idle_seconds=0;advance(65000);assert(page==P_EVENT_RESULT);
        assert(state.skill[1]==before.skill[1]+2 && state.stat[NM_ENERGY]==before.stat[NM_ENERGY]-1);
        describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"meeting reward");
        assert(!memcmp(&state,&saved_state,sizeof(state)));
    }
    state=original;home();save();
}
static void daily_events(void){
    nm_state_t original=state;
    for(unsigned lang=0;lang<2;++lang)for(unsigned event=0;event<8;++event)for(unsigned choice=0;choice<3;++choice){
        nm_init(&state,1,true,0);state.english=lang;state.event=event;state.relations[0]=50;
        nm_state_t expected=state;nm_effect_t effect;
        assert(nm_event_choose(&expected,choice,&effect)==NM_OK);
        home();go(P_EVENT);describe();assert(view.count==3);
        selection=choice;activate();assert(page==P_EVENT_RESULT && !memcmp(&state,&expected,sizeof(state)));
        assert(!memcmp(&saved_state,&state,sizeof(state)));
        describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);
        check_label(body,"event result");check_label(heading,"event title");check_label(row_labels[0],"event continue");
        char delta[80];snprintf(delta,sizeof(delta),lang?"Tech %+d Talk %+d Life %+d":"专业 %+d 沟通 %+d 生活 %+d",
            event==2&&choice==1?6:event==5&&choice==0?10:event==7&&choice==2?6:0,
            event==0&&choice==1?5:(event==1&&choice==0)||(event==4&&choice==2)?6:event==5&&choice==1?10:0,
            event==6&&choice==2?10:0);
        assert(strstr(view.body,delta));
        if(event==3 && choice==1){assert(state.memory_day[NM_NEW_FRIEND]==1);capture(lang?"event-share-en":"event-share-zh");}
        activate();assert(page==P_HOME);go(P_EVENT);describe();assert(view.count==1);
        nm_state_t before=state;unsigned calls=saves;activate();assert(page==P_HOME && saves==calls && !memcmp(&before,&state,sizeof(state)));
    }
    for(unsigned lang=0;lang<2;++lang){
        nm_init(&state,1,true,0);state.english=lang;home();go(P_EVENT);selection=2;
        nm_state_t before=state;unsigned calls=saves;activate();describe();assert(page==P_RESULT);
        assert(strstr(view.body,"40") && saves==calls && !memcmp(&before,&state,sizeof(state)));
        state.day=6;home();go(P_EVENT);selection=0;before=state;activate();describe();
        assert(strstr(view.body,lang?"weekends":"周末") && !memcmp(&before,&state,sizeof(state)));
    }
    state=original;home();save();
}
static void game_gestures(void){
    nm_state_t original=state;
    for(unsigned kind=0;kind<NM_GAME_COUNT;++kind){
        nm_init(&state,41,true,0);home();start_game((nm_game_kind_t)kind);
        game.round_ms=kind==NM_GAME_BENTO?1500:1000;game.elapsed=game.round_ms;game.lane=game.target;
        nm_game_t before=game;unsigned coins=state.coins;
        key_event((key_t){BSP_BTN_OK,BSP_BTN_PRESS});advance(500);
        assert(game_press_pending && !memcmp(&before,&game,sizeof(game)));
        key_event((key_t){BSP_BTN_OK,BSP_BTN_LONG});assert(page==P_GAME_PAUSE && !game_press_pending);
        advance(2000);assert(!memcmp(&before,&game,sizeof(game)));
        selection=0;activate();assert(page==P_GAME);
        key_event((key_t){BSP_BTN_OK,BSP_BTN_CLICK});assert(!memcmp(&before,&game,sizeof(game)));
        key_event((key_t){BSP_BTN_OK,BSP_BTN_PRESS});advance(200);
        key_event((key_t){BSP_BTN_OK,BSP_BTN_CLICK});
        if(kind==NM_GAME_TEA)assert(game.busy);else assert(game.score==(kind==NM_GAME_ESCAPE?2:1) && game.attempts==1);
        before=game;key_event((key_t){BSP_BTN_OK,BSP_BTN_DOUBLE});assert(!memcmp(&before,&game,sizeof(game)));
        key_event((key_t){BSP_BTN_OK,BSP_BTN_PRESS});advance(3100);assert(!game_press_pending);
        key_event((key_t){BSP_BTN_OK,BSP_BTN_LONG});selection=1;activate();
        assert(page==P_HOME && state.actions==5 && state.coins==coins && !game_paid);
    }
    state=original;home();save();
}
#include "career_journey.inc"

static void expression_gallery(void){
    static uint16_t atlas[480*320],tile[NM_ART_W*NM_ART_H];
    for(unsigned gender=0;gender<2;++gender)for(unsigned face=0;face<NM_FACE_COUNT;++face){
        nm_state_t sample;nm_init(&sample,1,gender,0);nm_scene_t scene=NM_SCENE_STAND;
        if(face==NM_FACE_HAPPY)sample.stat[NM_MOOD]=90;
        if(face==NM_FACE_FOCUSED)scene=NM_SCENE_OFFICE;
        if(face==NM_FACE_TIRED)sample.burnout=1;
        if(face==NM_FACE_STRESSED)sample.stat[NM_STRESS]=85;
        if(face==NM_FACE_SAD)sample.stat[NM_MOOD]=20;
        if(face==NM_FACE_SLEEP)scene=NM_SCENE_SLEEP;
        if(face==NM_FACE_SURPRISED)scene=NM_SCENE_LEAVE;
        assert(nm_art_expression(&sample,scene)==(nm_face_t)face);
        nm_art_render(tile,&sample,scene,1);
        for(unsigned y=0;y<80;++y)for(unsigned x=0;x<120;++x)
            atlas[(gender*160+face/4*80+y)*480+face%4*120+x]=tile[y*120+x];
    }
    FILE *file=fopen("/tmp/niuma-expressions.ppm","wb");assert(file);fprintf(file,"P6\n480 320\n255\n");
    for(unsigned i=0;i<480*320;++i){uint16_t p=atlas[i];uint8_t rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,file);}fclose(file);
}
static void character_card(void){
    nm_state_t sample;nm_init(&sample,1,true,0);
    uint16_t tile[NM_ART_W*NM_ART_H];nm_art_render(tile,&sample,NM_SCENE_STAND,1);
    FILE *file=fopen("/tmp/niuma-female-reference-card.ppm","wb");assert(file);
    fprintf(file,"P6\n240 320\n255\n");
    for(int y=0;y<320;++y)for(int x=0;x<240;++x){
        uint16_t p=0xef5b;
        if(x>=24 && x<216 && y>=22 && y<298){
            p=tile[(23+(y-22)/6)*NM_ART_W+42+(x-24)/6];
            if(p==0xce96)p=0xef5b;
        }
        uint8_t rgb[]={((p>>11)&31)*255/31,((p>>5)&63)*255/63,(p&31)*255/31};fwrite(rgb,1,3,file);
    }
    fclose(file);
}
int main(void){
    lv_init();lv_display_t*d=lv_display_create(240,320);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,dma,NULL,sizeof(dma),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(d,flush);
    assert(niuma_app_start());advance(100);capture("splash");
    assert(splash && view.kind==NM_VIEW_SPLASH && !saves);
    lv_tick_inc(61000);advance(100);
    assert(splash && !sleeping && !saves);
    for(unsigned k=0;k<3;++k){
        splash=true;page=P_BOOT;selection=0;describe();
        nm_state_t before=state;unsigned writes=saves;
        press((bsp_btn_t)k,BSP_BTN_PRESS);assert(splash);
        press((bsp_btn_t)k,BSP_BTN_CLICK);
        assert(!splash && page==P_BOOT && selection==0 && saves==writes);
        assert(!memcmp(&state,&before,sizeof(state)));
    }
    splash=true;press(BSP_BTN_OK,BSP_BTN_LONG);assert(!splash && page==P_BOOT);
    splash=true;press(BSP_BTN_UP,BSP_BTN_DOUBLE);assert(!splash && page==P_BOOT && !saves);
    /* Loading ignores queued gestures; failed loads still require the same
       explicit retry/reset flow after the cover has been dismissed. */
    override_store=true;fake_store=(nm_store_status_t){.phase=NM_STORE_LOADING,.battery=78};
    started=false;splash=true;
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(!started && splash && !saves);
    fake_store.phase=NM_STORE_ERROR;advance(100);
    assert(started && splash && page==P_START_ERROR && !saves);
    press(BSP_BTN_DOWN,BSP_BTN_CLICK);assert(!splash && page==P_START_ERROR && selection==0 && !saves);
    override_store=false;page=P_BOOT;describe();
    /* A loaded game is not resumed or altered by the entry gesture either. */
    loaded=true;splash=true;state.english=true;describe();
    nm_state_t restored=state;
    press(BSP_BTN_OK,BSP_BTN_CLICK);
    assert(!splash && page==P_BOOT && !memcmp(&restored,&state,sizeof(state)) && !saves);
    loaded=false;state.english=false;
    capture("boot");
    expression_gallery();
    character_card();
    if(getenv("NM_VISUAL_ONLY")){
        for(unsigned gender=0;gender<2;++gender){
            nm_init(&state,1,gender,0);loaded=true;home();capture(gender?"polish-female":"polish-male");
        }
        nm_close_work(&state,&receipt);home();capture("polish-evening");
        go(P_SETTINGS);capture("polish-settings");go(P_STATS);capture("polish-stats");
        return 0;
    }
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_CREATE);
    press(BSP_BTN_DOWN,BSP_BTN_CLICK);press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_NAME);
    press(BSP_BTN_DOWN,BSP_BTN_CLICK);press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_BADGE && saves);
    press(BSP_BTN_DOWN,BSP_BTN_CLICK);press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HELP);
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HOME);capture("home");
    unsigned actions=state.actions;prepare_action(NM_BENTO);describe();activate();assert(state.actions==actions-1 && state.coins==18);capture("eat");
    home();start_game(NM_GAME_TYPING);assert(page==P_GAME);capture("typing");advance(17000);assert(page==P_RESULT);capture("game-result");
    go(P_SETTINGS);capture("settings");
    home();unsigned day=state.day;
    finish_day();assert(page==P_PAY && state.evening && state.day==day);
    unsigned coins=state.coins;activate();assert(page==P_HOME);
    capture("evening");
    selection=1;activate();assert(page==P_FOOD);
    home();selection=0;activate();
    assert(page==P_MORNING && !state.evening && state.day==day+1 && state.coins==coins);
    press(BSP_BTN_OK,BSP_BTN_CLICK);assert(page==P_HOME);
    go(P_CAREER);selection=5;activate();assert(page==P_ROUTES);
    selection=NM_ROUTE_EXPERT;activate();assert(page==P_ROUTE);
    state.skill[0]=120;state.xp=240;selection=0;activate();
    assert(state.route==NM_ROUTE_EXPERT && page==P_HOME);
    go(P_BADGE);capture("badge");
    item=6;category=1;go(P_ITEM);capture("console-preview");
    nm_state_t unpurchased=state;selection=1;activate();
    assert(!memcmp(&unpurchased,&state,sizeof(state)));
    route_choice=NM_ROUTE_FREELANCE;go(P_ROUTE);capture("freelance");
    settings_and_save();
    game_guides();
    action_previews();
    startup_protection();
    device_states();
    game_gestures();
    daily_events();
    meeting_pages();
    escape_pages();
    action_scenes();
    burnout_pages();
    home_contexts();
    review_pages();
    morning_pages();
    memory_pages();
    resource_fallbacks();
    rest_and_notebook();
    career_gallery_and_transitions();
    menu_pagination();
    full_controller_career();
    lifecycle_pages();
    scene_atlas();
    game_visuals();
    for(unsigned lang=0;lang<2;++lang){
        state.english=lang;item=6;category=1;
        /* This synthetic page sweep must not reuse a text-only meeting receipt
         * as an action-scene payload. Real receipts are checked in their flows. */
        copy(result_body,sizeof(result_body),"Food +1\nEnergy +2\nCoins +0");
        for(int p=P_BOOT;p<=P_MORNING;++p){
            page=(page_t)p;selection=0;describe();unsigned count=view.count?view.count:1;
            for(unsigned selected=0;selected<count;++selected){
                selection=selected;describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);
                assert(nm_valid(&state));lv_obj_update_layout(screen);
                lv_refr_now(NULL);check_corners();
                check_label(body,"body");check_label(heading,"title");check_label(hint,"hint");
                check_label(status_label,"status");check_label(battery_label,"battery");
                for(unsigned m=0;m<5;++m)check_label(metrics[m],"need metric");
                check_label(wallet,"coins");
                check_label(page_label,"page counter");
                for(unsigned i=0;i<5;++i)if(!lv_obj_has_flag(rows[i],LV_OBJ_FLAG_HIDDEN))check_label(row_labels[i],"option");
            }
        }
        for(unsigned route=0;route<NM_ROUTE_COUNT;++route){page=P_ROUTE;route_choice=(nm_route_t)route;describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"route details");}
        for(item=0;item<12;++item){page=P_ITEM;category=item/4;selection=0;nm_state_t before=state;describe();nm_view_draw(&view,&state,&game,nm_storage_status(),1);lv_obj_update_layout(screen);check_label(body,"item preview");assert(!memcmp(&state,&before,sizeof(state)));}
    }
    home();state.idle_seconds=30;advance(31000);assert(sleeping && light==0);
    press(BSP_BTN_OK,BSP_BTN_PRESS);press(BSP_BTN_OK,BSP_BTN_CLICK);assert(!sleeping && page==P_HOME);
    lv_mem_monitor_t mem;lv_mem_monitor(&mem);
    printf("UI heap used %zu / %zu, peak %zu\n",mem.total_size-mem.free_size,mem.total_size,mem.max_used);
    assert(lv_mem_test()==LV_RESULT_OK);
    assert(layout_errors==0);
    puts("NiuMa production UI: PASS (create/feed/game/settings/bilingual pages/idle wake)");
}
