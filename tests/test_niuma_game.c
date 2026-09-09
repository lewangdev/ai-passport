#include "niuma_game.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void escape_rules(void){
    nm_game_t g;nm_state_t s;nm_init(&s,1,true,0);
    nm_game_start(&g,NM_GAME_ESCAPE,19);assert(g.floors==3 && nm_game_round_duration(&g)==6000);
    nm_game_move(&g,-1);assert(g.lane==1);nm_game_move(&g,1);assert(g.lane==0);
    g.lift_meeting=false;g.lift_arrival_ms=2000;
    nm_game_tick(&g,1999);assert(!nm_game_lift_ready(&g));
    nm_game_tick(&g,1);assert(nm_game_lift_ready(&g));
    assert(nm_game_press(&g) && g.floors==1 && g.score==4 && !g.done);
    nm_game_move(&g,1);assert(g.lane==0);assert(!nm_game_press(&g));
    nm_game_tick(&g,4000);assert(g.round==1 && !g.used);
    g.lift_meeting=true;g.lane=1;assert(nm_game_press(&g) && g.done && !g.floors && g.score==8);
    assert(nm_game_settle(&g,&s) && s.coins==38);assert(!nm_game_settle(&g,&s));
    for(unsigned reason=0;reason<3;++reason){
        nm_game_start(&g,NM_GAME_ESCAPE,19);g.lift_meeting=reason==0;g.lift_arrival_ms=2000;
        nm_game_tick(&g,reason==1?1999:3500);assert(!nm_game_lift_ready(&g));
        assert(!nm_game_press(&g) && g.used && g.floors==3 && !g.score);
        nm_game_tick(&g,6000-g.round_ms);g.lane=1;assert(nm_game_press(&g) && g.floors==2);
        nm_game_skip(&g);unsigned coins=s.coins;assert(nm_game_settle(&g,&s) && s.coins==coins);
    }
    nm_game_start(&g,NM_GAME_ESCAPE,19);
    for(unsigned i=0;i<3;++i){g.lane=1;assert(nm_game_press(&g));nm_game_tick(&g,6000);}
    assert(g.done && !g.floors && g.score==6); /* Safe stairs lack the two-round bonus. */
    nm_game_start(&g,NM_GAME_ESCAPE,19);g.lane=1;assert(nm_game_press(&g));nm_game_tick(&g,UINT32_MAX);
    assert(g.done && g.floors==2 && g.score==2 && g.round==8); /* Partial progress still counts. */
}
static void meeting_rules(void){
    for(unsigned answer=0;answer<3;++answer){
        nm_game_t g;nm_state_t s;nm_init(&s,1,true,0);s.relations[0]=59;
        nm_game_start(&g,NM_GAME_MEETING,22);assert(g.topic<6 && g.target==g.topic%3);
        g.target=answer;g.lane=answer;
        assert(nm_game_press(&g));assert(!nm_game_press(&g));assert(g.meeting_answers[answer]==1);
        nm_game_tick(&g,UINT32_MAX);assert(g.done && g.score==1);
        assert(nm_game_settle(&g,&s));
        assert(s.relations[0]==(answer==0?61:59));
        assert(s.xp==(answer==1?3:0));
        assert(s.skill[1]==(answer==2?2:0));
        assert(s.stat[NM_ENERGY]==(answer==2?84:85));
        if(answer==0)assert(s.memory_day[NM_NEW_FRIEND]==1);
        nm_state_t settled=s;assert(!nm_game_settle(&g,&s));assert(!memcmp(&s,&settled,sizeof(s)));
        nm_game_start(&g,NM_GAME_MEETING,22);g.lane=g.target;assert(nm_game_press(&g));nm_game_skip(&g);
        assert(nm_game_settle(&g,&s));assert(s.coins==settled.coins && s.xp==settled.xp && s.skill[1]==settled.skill[1] && s.stat[NM_ENERGY]==settled.stat[NM_ENERGY]);
    }
    nm_game_t g;nm_state_t s;nm_init(&s,1,true,0);s.xp=UINT16_MAX-1;s.skill[1]=UINT16_MAX-1;s.relations[0]=99;s.stat[NM_ENERGY]=1;
    nm_game_start(&g,NM_GAME_MEETING,2);g.done=true;g.score=8;
    g.meeting_answers[0]=2;g.meeting_answers[1]=3;g.meeting_answers[2]=3;
    assert(nm_game_settle(&g,&s) && nm_valid(&s));
    assert(s.xp==UINT16_MAX && s.skill[1]==UINT16_MAX && s.relations[0]==100 && s.stat[NM_ENERGY]==0);
    nm_game_start(&g,NM_GAME_MEETING,2);g.lane=(g.target+1)%3;assert(!nm_game_press(&g));
    assert(g.score==0 && g.feedback==2 && g.meeting_answers[0]+g.meeting_answers[1]+g.meeting_answers[2]==0);
}

int main(void) {
    meeting_rules();
    escape_rules();
    for(unsigned k=0;k<NM_GAME_COUNT;++k) {
        nm_game_t g; nm_state_t s; nm_init(&s,1,0,0);
        nm_game_start(&g,(nm_game_kind_t)k,42);
        for(unsigned round=0;round<8 && !g.done;++round){
            if(k==NM_GAME_TEA) {
                if(g.busy) nm_game_press(&g);
                nm_game_tick(&g,1099); assert(!nm_game_warning(&g));
                nm_game_tick(&g,1); assert(nm_game_warning(&g) && !nm_game_danger(&g));
                nm_game_tick(&g,200);nm_game_press(&g); /* Human reaction during warning. */
                nm_game_tick(&g,100);assert(nm_game_danger(&g) && !nm_game_warning(&g));
                nm_game_tick(&g,600);
            } else if(k==NM_GAME_ESCAPE){
                g.lane=0;g.lift_meeting=false;g.lift_arrival_ms=1000;
                nm_game_tick(&g,1000);assert(nm_game_press(&g));assert(!nm_game_press(&g));nm_game_tick(&g,5000);
            } else {
                while(g.lane!=g.target) nm_game_move(&g,1);
                uint32_t t=k==NM_GAME_BENTO?1500:1000;
                nm_game_tick(&g,t); assert(nm_game_press(&g));
                assert(!nm_game_press(&g)); nm_game_tick(&g,nm_game_round_duration(&g)-t);
            }
        }
        assert(g.done && g.score==8);
        assert(nm_game_settle(&g,&s) && s.coins==38);
        assert(!nm_game_settle(&g,&s) && s.coins==38);
        nm_game_skip(&g); assert(g.score==8);

        nm_game_start(&g,(nm_game_kind_t)k,17);
        nm_game_skip(&g); assert(nm_game_settle(&g,&s) && s.coins==38);

        nm_game_t a,b; nm_game_start(&a,(nm_game_kind_t)k,31); b=a;
        nm_game_tick(&a,UINT32_MAX);
        unsigned duration=8*nm_game_round_duration(&b);
        for(unsigned i=0;i<duration;++i) nm_game_tick(&b,1);
        assert(a.done && b.done && a.score==b.score && a.rng==b.rng);
        assert(a.elapsed==duration && a.round==8);
    }
    puts("NiuMa minigames: PASS (six games, timing, skip, single settlement)");
}
