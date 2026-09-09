#include "niuma_art.h"
#include "niuma_sprites.h"
#include <stddef.h>
#include <string.h>

nm_scene_t nm_art_action_scene(nm_action_t action){
    static const nm_scene_t scenes[NM_ACTION_COUNT]={
        [NM_WORK]=NM_SCENE_OFFICE,[NM_OVERTIME]=NM_SCENE_OFFICE,
        [NM_REST]=NM_SCENE_SLEEP,[NM_SLACK]=NM_SCENE_HOME,
        [NM_STUDY_TECH]=NM_SCENE_STUDY,[NM_STUDY_TALK]=NM_SCENE_STUDY,[NM_STUDY_LIFE]=NM_SCENE_STUDY,
        [NM_EXERCISE]=NM_SCENE_PARK,[NM_CLEAN]=NM_SCENE_CLEAN,[NM_TREAT]=NM_SCENE_TREAT,
        [NM_RECOVER]=NM_SCENE_SLEEP,[NM_CHAT]=NM_SCENE_FRIEND,[NM_GIFT]=NM_SCENE_FRIEND,
        [NM_TOILET]=NM_SCENE_TOILET,[NM_WATER]=NM_SCENE_WATER,[NM_BASIC_MEAL]=NM_SCENE_RICE,
        [NM_BENTO]=NM_SCENE_EAT,[NM_NOODLES]=NM_SCENE_NOODLES,[NM_COFFEE]=NM_SCENE_DRINK,
        [NM_MILK_TEA]=NM_SCENE_MILK_TEA,[NM_COOK]=NM_SCENE_COOK
    };
    return (unsigned)action<NM_ACTION_COUNT?scenes[action]:NM_SCENE_STAND;
}

static const uint16_t palette[4]={0xce96,0x2144,0x7c2d,0xef5b};
typedef struct {uint16_t *p;} paint_t;
static void rect(paint_t p,int x,int y,int w,int h,unsigned color){
    for(int yy=y;yy<y+h;++yy)for(int xx=x;xx<x+w;++xx)
        if(xx>=0 && xx<NM_ART_W && yy>=0 && yy<NM_ART_H)p.p[yy*NM_ART_W+xx]=palette[color%4];
}
static void box(paint_t p,int x,int y,int w,int h,unsigned color){
    rect(p,x,y,w,1,color);rect(p,x,y+h-1,w,1,color);rect(p,x,y,1,h,color);rect(p,x+w-1,y,1,h,color);
}
static void sprite(paint_t p,const char *const *map,unsigned rows,int x,int y){
    for(unsigned yy=0;yy<rows;++yy)for(unsigned xx=0;map[yy][xx];++xx)
        if(map[yy][xx]!='.')rect(p,x+(int)xx,y+(int)yy,1,1,map[yy][xx]-'0');
}
static void walking_legs(paint_t p,int x,int y,unsigned frame){
    for(unsigned yy=0;yy<6;++yy){
        unsigned width=strlen(nm_sprite_walk[yy]);
        for(unsigned xx=0;xx<width;++xx){char c=nm_sprite_walk[yy][xx];
            if(c!='.')rect(p,x+(int)(frame%2?width-1-xx:xx),y+(int)yy,1,1,c-'0');
        }
    }
}
static void person(paint_t p,const nm_state_t *s,int x,int y,bool seated,bool walking,unsigned frame){
    bool tired=s->burnout || s->stat[NM_ENERGY]<20;
    if(tired)y+=2;
    if(s->gender)sprite(p,nm_sprite_ponytail,15,x-5+(walking?(int)(frame%2):0),y+4);
    sprite(p,s->gender?nm_sprite_femaleHead:nm_sprite_head,18,x,y);
    sprite(p,s->gender?nm_sprite_femaleBody:nm_sprite_body,11,x,y+18);
    if(walking)walking_legs(p,x,y+29,frame);
    else sprite(p,seated?nm_sprite_seated:nm_sprite_standing,seated?6:9,x,y+29);
    if(tired || frame%24==0){
        unsigned left=s->gender?8:9,right=s->gender?14:15;
        rect(p,x+left,y+9,1,2,3);rect(p,x+right,y+9,1,2,3);
        rect(p,x+left-1,y+9,2,1,1);rect(p,x+right-1,y+9,2,1,1);
    }
    if(s->equipped[2]!=255){
        if(s->equipped[2]%4==0){rect(p,x+4,y+33,4,2,3);rect(p,x+12,y+33,4,2,3);}
        if(s->equipped[2]%4==1){box(p,x+1,y+4,18,7,2);}
        if(s->equipped[2]%4==2){rect(p,x+15,y+21,3,10,3);}
        if(s->equipped[2]%4==3){rect(p,x+6,y+19,8,2,3);}
    }
}
static void plant(paint_t p,int x,int y){
    rect(p,x+4,y,3,10,1);rect(p,x,y+2,6,3,1);rect(p,x+6,y+4,6,3,1);
    rect(p,x+1,y+11,10,2,1);rect(p,x+2,y+13,8,6,2);box(p,x+2,y+13,8,6,1);
}
static void bento(paint_t p,int x,int y){
    rect(p,x,y,24,15,3);box(p,x,y,24,15,1);rect(p,x+12,y,1,15,1);
    rect(p,x+2,y+3,8,8,2);rect(p,x+15,y+3,6,4,1);rect(p,x+15,y+9,6,3,2);
}
void nm_art_render(uint16_t *pixels,const nm_state_t *s,nm_scene_t scene,unsigned frame){
    paint_t p={pixels};rect(p,0,0,120,80,0);rect(p,0,71,120,1,2);
    for(int x=0;x<120;x+=15)rect(p,x,77,6,1,2);
    bool meal=scene==NM_SCENE_EAT || scene==NM_SCENE_RICE || scene==NM_SCENE_NOODLES;
    if(scene==NM_SCENE_OFFICE || scene==NM_SCENE_STUDY || meal){
        box(p,4,7,25,22,2);rect(p,16,7,1,22,2);rect(p,4,18,25,1,2);
        box(p,99,6,13,13,1);rect(p,105,9,1,5,1);rect(p,105,13,4,1,1);
        box(p,77,42,8,23,1);rect(p,78,43,6,21,2);
        /* Hands meet the keyboard; seated legs remain below the desktop. */
        person(p,s,53,27,true,false,frame);
        rect(p,22,53,80,3,1);rect(p,25,56,3,16,1);rect(p,96,56,3,16,1);
        box(p,25,57,29,14,2);
        if(!meal){box(p,26,33,19,16,1);rect(p,28,35,15,12,2);rect(p,34,49,3,4,1);rect(p,59,51,15,2,2);}
        rect(p,61+(int)(frame%2)*2,49,5,2,3);
        if(scene==NM_SCENE_STUDY){box(p,48,45,13,8,1);rect(p,54,45,1,8,1);}
        if(scene==NM_SCENE_EAT)bento(p,27,38);
        if(scene==NM_SCENE_RICE){
            rect(p,32,40,12,3,3);rect(p,29,43,18,3,3);
            rect(p,28,46,20,2,1);rect(p,30,48,16,3,2);rect(p,33,51,10,2,1);
            rect(p,34,41,1,1,2);rect(p,40,44,1,1,2);
        }
        if(scene==NM_SCENE_NOODLES){
            rect(p,30,36,17,15,3);box(p,29,35,19,3,1);box(p,31,38,15,13,1);
            rect(p,33,50,11,3,1);rect(p,33,43,11,2,2);
            for(unsigned i=0;i<3;++i){rect(p,33+i*4,32+(int)((frame+i)%2),2,1,1);rect(p,35+i*4,33,1,2,1);}
            rect(p,40,28,14,1,1);rect(p,41,30,14,1,1);
        }
        if(s->burnout){
            for(int i=0;i<5;++i){rect(p,9+i%2,50-i*4,13,3,1);rect(p,10+i%2,51-i*4,11,1,3);}
            rect(p,56,18,1,2,1);rect(p,60,15,1,2,1);
            rect(p,64,18,1,2,1);rect(p,58,22,1,1,1);rect(p,62,22,1,1,1);
        }
        else for(unsigned i=0;i<s->mess;++i)box(p,8+i*5,62-i*2,4,5,1);
        if(s->equipped[0]!=255){
            if(s->equipped[0]%4==0)plant(p,85,34);
            if(s->equipped[0]%4==1)box(p,88,40,8,12,1);
            if(s->equipped[0]%4==2){box(p,85,37,11,10,1);rect(p,90,47,2,6,1);rect(p,87,41,7,2,2);}
            if(s->equipped[0]%4==3){rect(p,46,50,12,2,3);}
        }
    }else if(scene==NM_SCENE_HOME || scene==NM_SCENE_SLEEP){
        box(p,8,7,23,21,2);rect(p,18,7,1,21,2);plant(p,103,51);
        rect(p,21,52,70,17,2);box(p,18,47,76,23,1);rect(p,22,49,12,6,3);
        person(p,s,50,19,true,false,scene==NM_SCENE_SLEEP?0:frame);
        if(scene==NM_SCENE_SLEEP){box(p,78,18,4,4,2);box(p,85,11,6,6,2);}
        switch(s->equipped[1]){
        case 4: /* Woven rug. */
            box(p,23,73,75,6,1);for(int i=0;i<5;++i)rect(p,27+i*14,74,9,3,2);break;
        case 5: /* Padded arms and quilted sofa back. */
            rect(p,17,45,8,24,3);box(p,17,45,8,24,1);
            rect(p,87,45,9,24,3);box(p,87,45,9,24,1);
            for(int i=0;i<4;++i)box(p,30+i*13,58,11,8,1);
            break;
        case 6: /* Game console and controller. */
            rect(p,2,31,27,17,1);rect(p,4,33,23,12,3);
            rect(p,12,36,4,6,2);rect(p,7,42,15,2,2);rect(p,13,48,3,7,1);
            box(p,5,60,17,7,1);rect(p,8,62,5,2,1);rect(p,10,61,1,4,1);rect(p,18,62,2,2,1);break;
        case 7: /* Kitchen sink, hob and cupboard. */
            rect(p,2,44,29,26,3);box(p,2,44,29,26,1);rect(p,16,49,1,21,1);
            box(p,5,46,9,4,2);box(p,20,45,7,6,1);rect(p,8,39,7,2,1);rect(p,14,40,2,6,1);
            rect(p,12,55,2,4,1);rect(p,19,55,2,4,1);break;
        default:break;
        }
    }else if(scene==NM_SCENE_PARK){
        for(int x=10;x<120;x+=84){rect(p,x+5,19,4,45,2);rect(p,x-3,10,21,18,2);rect(p,x+1,5,13,26,2);}
        person(p,s,51,33,false,true,frame);
    }else if(scene==NM_SCENE_FRIEND){person(p,s,25,31,false,false,frame);nm_state_t friend=*s;friend.gender=!s->gender;person(p,&friend,78,31,false,false,frame+3);box(p,48,12,27,10,1);}
    else if(scene==NM_SCENE_TRAIN){for(int i=0;i<3;++i){box(p,7+i*38,7,29,23,2);rect(p,7+i*38,53,29,12,2);}person(p,s,51,28,true,false,frame);}
    else if(scene==NM_SCENE_LEAVE){
        box(p,84,6,29,65,1);box(p,90,12,17,12,2);rect(p,106,42,3,3,1);person(p,s,36,33,false,true,frame);
        box(p,54,57,12,10,1);rect(p,55,58,10,8,3);rect(p,30,65,8,6,1);
    }
    else if(scene==NM_SCENE_CLEAN){
        box(p,8,12,26,21,2);rect(p,21,12,1,21,2);plant(p,99,47);
        person(p,s,43,30,false,false,frame);
        rect(p,60,56,36,3,1);rect(p,63,59,3,12,1);rect(p,91,59,3,12,1);
        int cloth=69+(int)(frame%4)*2;
        rect(p,59,50,cloth-59,3,2);rect(p,cloth-2,50,4,3,3);
        rect(p,cloth-3,53,10,3,3);box(p,cloth-3,53,10,3,1);
        for(unsigned dust=frame%4;dust<4;++dust)box(p,78+dust*4,52-(int)(dust%2)*3,2,2,1);
        if(frame%4==3){rect(p,88,44,1,7,1);rect(p,85,47,7,1,1);}
    }
    else if(scene==NM_SCENE_TREAT){
        box(p,9,10,25,25,1);rect(p,19,15,5,15,2);rect(p,14,20,15,5,2);
        rect(p,39,57,48,5,2);rect(p,42,62,3,9,1);rect(p,81,62,3,9,1);
        person(p,s,54,24,true,false,frame);
        rect(p,62,30,9,3,3);rect(p,66,30,1,3,2);
        box(p,95,41,17,27,1);box(p,98,44,11,8,2);rect(p,101,55,5,8,3);
        if(frame%2){rect(p,42,18,7,2,1);rect(p,44,16,3,6,1);}
    }
    else if(scene==NM_SCENE_TOILET){
        /* Closed cubicle provides privacy; the character waits/washes outside. */
        box(p,8,9,44,61,1);rect(p,10,11,40,57,2);box(p,23,16,15,14,1);
        rect(p,26,19,9,2,3);rect(p,28,21,7,5,3);rect(p,30,26,3,2,3);
        rect(p,43,40,4,2,1);rect(p,40,36,1,5,frame%2?3:1);
        person(p,s,64,31,false,false,frame);
        box(p,91,49,25,7,1);rect(p,94,56,18,14,2);rect(p,103,40,2,9,1);rect(p,98,40,7,2,1);
        rect(p,99,44+(int)(frame%3),1,3,2);rect(p,80,52,17,3,2);
    }
    else if(scene==NM_SCENE_DRINK || scene==NM_SCENE_WATER || scene==NM_SCENE_MILK_TEA){
        box(p,10,16,23,49,1);rect(p,14,8,15,18,3);box(p,14,8,15,18,1);
        rect(p,17,32,3,3,1);rect(p,24,32,3,3,2);box(p,14,43,15,15,2);
        person(p,s,57,30,false,false,frame);
        int cup_y=frame%4<2?43:52;
        rect(p,71,cup_y,10,10,scene==NM_SCENE_WATER?0:3);box(p,71,cup_y,10,10,1);
        if(scene==NM_SCENE_DRINK){box(p,81,cup_y+2,4,5,1);rect(p,72,cup_y-5+(int)(frame%2),1,3,2);}
        else if(scene==NM_SCENE_WATER){rect(p,72,cup_y+5,8,1,2);rect(p,73,cup_y+2,1,2,3);}
        else{rect(p,70,cup_y-1,12,2,1);rect(p,76,cup_y-6,2,6,1);rect(p,77,cup_y-7,4,1,1);for(unsigned i=0;i<3;++i)rect(p,73+i*2,cup_y+7,1,1,1);}
        rect(p,68,cup_y+7,5,3,3);
    }
    else if(scene==NM_SCENE_COOK){
        box(p,7,8,28,21,2);rect(p,20,8,1,21,2);person(p,s,49,24,false,false,frame);
        rect(p,27,57,76,3,1);box(p,29,60,72,11,2);rect(p,64,60,1,11,2);
        rect(p,70,52,19,4,1);rect(p,68,48,23,5,2);rect(p,88,49,13,2,1);
        rect(p,63,44,15,2,1);rect(p,75,44,2,8,1);
        for(unsigned steam=0;steam<3;++steam)rect(p,73+steam*6,37+(int)((frame+steam)%3)*2,1,4,2);
        box(p,33,50,11,6,1);rect(p,36,47,4,3,2);
    }
    else if(scene==NM_SCENE_PLANT)plant(p,54,38);
    else if(scene==NM_SCENE_BENTO)bento(p,48,43);
    else person(p,s,51,30,false,false,frame);
}
void nm_art_game(uint16_t *pixels,const nm_state_t *s,const nm_game_t *g){
    nm_art_render(pixels,s,g->kind==NM_GAME_COMMUTE?NM_SCENE_TRAIN:g->kind==NM_GAME_ESCAPE?NM_SCENE_LEAVE:NM_SCENE_OFFICE,g->elapsed/250);
    paint_t p={pixels};
    if(g->kind==NM_GAME_TYPING){
        for(unsigned i=0;i<3;++i){rect(p,8,9+i*19,104,15,0);box(p,8,9+i*19,104,15,i==g->lane?1:2);rect(p,49,11+i*19,25,11,2);}
        rect(p,10+g->round_ms/20,12+g->lane*19,3,9,1);box(p,2,13+g->target*19,4,7,1);
    }else if(g->kind==NM_GAME_BENTO){
        rect(p,0,0,120,70,0);bento(p,9+g->target*38,g->round_ms*48/2000);
        rect(p,6+g->lane*38,62,31,4,1);
    }else if(g->kind==NM_GAME_COMMUTE){
        if(g->round_ms>=900)rect(p,7+g->target*38,53,29,12,3);
        box(p,5+g->lane*38,51,33,16,1);
    }else if(g->kind==NM_GAME_TEA){
        box(p,2,34,17,36,1);if(nm_game_danger(g)){rect(p,6,43,8,16,1);rect(p,8,39,5,5,1);}
        if(nm_game_warning(g)){rect(p,8,17,3,9,1);rect(p,8,29,3,3,1);}
        if(!g->busy)box(p,72,39,5,8,1);
        rect(p,7,75,g->tea_ms/14,3,1);
    }else if(g->kind==NM_GAME_MEETING){
        for(unsigned i=0;i<3;++i)box(p,8+i*38,4,29,10,i==g->lane?1:2);
        /* A speech bubble, notebook, and question mark; no marked answer. */
        box(p,17,6,10,5,1);rect(p,19,11,2,2,1);
        box(p,55,5,9,8,1);rect(p,57,7,5,1,1);rect(p,57,10,5,1,1);
        rect(p,93,5,5,1,1);rect(p,97,6,1,3,1);rect(p,95,8,2,2,1);rect(p,95,12,1,1,1);
    }else if(g->kind==NM_GAME_ESCAPE){
        rect(p,0,0,120,80,0);
        box(p,5,8,42,59,g->lane==0?1:2);
        if(nm_game_lift_ready(g)){rect(p,12,17,27,46,1);rect(p,16,20,19,43,3);}
        else{box(p,12,17,27,46,2);rect(p,25,17,1,46,1);}
        for(unsigned step=0;step<4;++step)rect(p,64+step*11,54-step*10,11,13+step*10,2);
        box(p,59,8,56,59,g->lane==1?1:2);
        if(g->lift_meeting){ /* A supervisor by the lift; stairs bypass the meeting. */
            rect(p,41,27,7,7,1);rect(p,39,34,11,17,1);box(p,43,13,12,9,1);rect(p,47,15,2,4,1);
        }
        person(p,s,g->lane?78:16,26,false,g->used && g->feedback==1,g->elapsed/250);
        for(unsigned floor=0;floor<3;++floor)box(p,41+floor*13,72,9,5,floor<g->floors?1:2);
    }
}
