#include "niuma_art.h"
#include "niuma_icons.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static struct {uint16_t before[16],pixels[NM_ART_W*NM_ART_H],after[16];} guarded;
static uint16_t repeat[NM_ART_W*NM_ART_H];
static void check_pixels(void){
    for(unsigned i=0;i<16;++i)assert(guarded.before[i]==0xa55a && guarded.after[i]==0xa55a);
    for(unsigned i=0;i<NM_ART_W*NM_ART_H;++i){
        uint16_t p=guarded.pixels[i];assert(p==0xce96 || p==0x2144 || p==0x7c2d || p==0xef5b);
    }
}
int main(void){
    for(unsigned i=0;i<NM_ICON_COUNT;++i){
        unsigned pixels=0;for(unsigned y=0;y<12;++y){assert(!(nm_icons[i][y]&0xf000));pixels|=nm_icons[i][y];}
        assert(pixels);
        for(unsigned j=i+1;j<NM_ICON_COUNT;++j)assert(memcmp(nm_icons[i],nm_icons[j],sizeof(nm_icons[i])));
    }
    for(unsigned i=0;i<16;++i)guarded.before[i]=guarded.after[i]=0xa55a;
    for(unsigned gender=0;gender<2;++gender)for(unsigned tired=0;tired<2;++tired){
        nm_state_t s;nm_init(&s,1,gender,0);s.burnout=tired;s.mess=4;
        for(unsigned item=0;item<12;++item){s.equipped[item/4]=item;
            nm_state_t before=s;
            for(unsigned scene=0;scene<NM_SCENE_COUNT;++scene)for(unsigned frame=0;frame<24;++frame){
                nm_art_render(guarded.pixels,&s,(nm_scene_t)scene,frame);check_pixels();
                nm_art_render(repeat,&s,(nm_scene_t)scene,frame);assert(!memcmp(repeat,guarded.pixels,sizeof(repeat)));
                assert(!memcmp(&before,&s,sizeof(s)));
            }
        }
        for(unsigned kind=0;kind<NM_GAME_COUNT;++kind){nm_game_t g;nm_game_start(&g,(nm_game_kind_t)kind,1);
            for(unsigned tick=0;tick<64;++tick){nm_art_game(guarded.pixels,&s,&g);check_pixels();nm_game_tick(&g,1000);}
        }
    }
    const nm_scene_t expected[]={NM_SCENE_OFFICE,NM_SCENE_OFFICE,NM_SCENE_SLEEP,NM_SCENE_HOME,
        NM_SCENE_STUDY,NM_SCENE_STUDY,NM_SCENE_STUDY,NM_SCENE_PARK,NM_SCENE_CLEAN,NM_SCENE_TREAT,
        NM_SCENE_SLEEP,NM_SCENE_FRIEND,NM_SCENE_FRIEND,NM_SCENE_TOILET,NM_SCENE_WATER,NM_SCENE_RICE,
        NM_SCENE_EAT,NM_SCENE_NOODLES,NM_SCENE_DRINK,NM_SCENE_MILK_TEA,NM_SCENE_COOK};
    _Static_assert(sizeof(expected)/sizeof(expected[0])==NM_ACTION_COUNT,"Cover every action scene");
    for(unsigned a=0;a<NM_ACTION_COUNT;++a)assert(nm_art_action_scene((nm_action_t)a)==expected[a]);
    assert(nm_art_action_scene((nm_action_t)-1)==NM_SCENE_STAND);
    nm_state_t s;nm_init(&s,1,0,0);
    assert(nm_art_expression(&s,NM_SCENE_SLEEP)==NM_FACE_SLEEP);
    assert(nm_art_expression(&s,NM_SCENE_OFFICE)==NM_FACE_FOCUSED);
    assert(nm_art_expression(&s,NM_SCENE_FRIEND)==NM_FACE_HAPPY);
    assert(nm_art_expression(&s,NM_SCENE_LEAVE)==NM_FACE_SURPRISED);
    s.stat[NM_MOOD]=20;assert(nm_art_expression(&s,NM_SCENE_STAND)==NM_FACE_SAD);
    s.stat[NM_STRESS]=80;assert(nm_art_expression(&s,NM_SCENE_STAND)==NM_FACE_STRESSED);
    s.burnout=1;assert(nm_art_expression(&s,NM_SCENE_OFFICE)==NM_FACE_TIRED);
    nm_init(&s,1,true,0);nm_art_render(guarded.pixels,&s,NM_SCENE_STAND,1);
    assert(guarded.pixels[27*NM_ART_W+59]==0x7c2d); /* Shaded, broad fringe. */
    assert(guarded.pixels[39*NM_ART_W+57]==0x2144); /* Oval pupil. */
    assert(guarded.pixels[40*NM_ART_W+56]==0x2144);
    assert(guarded.pixels[39*NM_ART_W+56]==0xef5b); /* Curved pupil corner. */
    assert(guarded.pixels[44*NM_ART_W+61]==0x2144); /* Separated smile. */
    nm_art_render(repeat,&s,NM_SCENE_STAND,24);
    assert(repeat[39*NM_ART_W+57]==0xef5b); /* Blink preserves the face. */
    assert(repeat[43*NM_ART_W+60]==0x2144); /* Smile survives a blink. */
    for(unsigned gender=0;gender<2;++gender){
        nm_init(&s,1,gender,0);
        nm_art_render(guarded.pixels,&s,NM_SCENE_STAND,1);
        assert(guarded.pixels[26*NM_ART_W+59]==0x2144); /* Same head crown height. */
        assert(guarded.pixels[38*NM_ART_W+49]==0x2144); /* Same head left/right span. */
        assert(guarded.pixels[38*NM_ART_W+70]==0x2144);
        nm_art_render(guarded.pixels,&s,NM_SCENE_OFFICE,1);
        assert(guarded.pixels[61*NM_ART_W+60]==0x2144); /* Shoes below y=53 desktop. */
        s.burnout=1;nm_art_render(guarded.pixels,&s,NM_SCENE_OFFICE,1);
        assert(guarded.pixels[63*NM_ART_W+60]==0x2144);
        assert(guarded.pixels[15*NM_ART_W+60]==0x2144); /* Fatigue dots above the head. */
    }
    for(unsigned scene=NM_SCENE_CLEAN;scene<NM_SCENE_COUNT;++scene){
        nm_art_render(guarded.pixels,&s,(nm_scene_t)scene,1);nm_art_render(repeat,&s,(nm_scene_t)scene,2);
        assert(memcmp(repeat,guarded.pixels,sizeof(repeat))); /* Each new action has moving feedback. */
    }
    const nm_scene_t food[]={NM_SCENE_RICE,NM_SCENE_EAT,NM_SCENE_NOODLES,NM_SCENE_WATER,NM_SCENE_DRINK,NM_SCENE_MILK_TEA};
    for(unsigned i=0;i<6;++i)for(unsigned j=i+1;j<6;++j){
        nm_art_render(guarded.pixels,&s,food[i],1);nm_art_render(repeat,&s,food[j],1);
        assert(memcmp(guarded.pixels,repeat,sizeof(repeat)));
    }
    puts("NiuMa pixel art: PASS (all scenes/actions, four colors, bounds, purity, animation)");
}
