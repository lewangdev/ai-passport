#include "niuma_game.h"
#include <string.h>

static uint32_t random_next(nm_game_t *g) {
    uint32_t x=g->rng; x^=x<<13; x^=x>>17; x^=x<<5; return g->rng=x;
}
static void new_round(nm_game_t *g) {
    g->target=random_next(g)%3; g->used=false; g->round_ms=0; g->tea_ms=0;
    if(g->kind==NM_GAME_MEETING){g->topic=random_next(g)%6;g->target=g->topic%3;}
    if(g->kind==NM_GAME_ESCAPE){g->lift_arrival_ms=(1+random_next(g)%3)*1000;g->lift_meeting=random_next(g)%3==0;g->target=1;}
}
uint32_t nm_game_round_duration(const nm_game_t *g){return g->kind==NM_GAME_MEETING?8000:g->kind==NM_GAME_ESCAPE?6000:2000;}
bool nm_game_lift_ready(const nm_game_t *g){
    return g->kind==NM_GAME_ESCAPE && !g->lift_meeting && g->round_ms>=g->lift_arrival_ms && g->round_ms<g->lift_arrival_ms+1500u;
}
void nm_game_start(nm_game_t *g, nm_game_kind_t kind, uint32_t seed) {
    memset(g,0,sizeof(*g)); g->kind=kind; g->rng=seed?seed:1;
    if((unsigned)kind>=NM_GAME_COUNT) {g->done=true;return;}
    if(kind==NM_GAME_ESCAPE)g->floors=3;
    new_round(g);
}
void nm_game_move(nm_game_t *g, int direction) {
    if(g->done || (g->used && (g->kind==NM_GAME_MEETING || g->kind==NM_GAME_ESCAPE)))return;
    unsigned choices=g->kind==NM_GAME_ESCAPE?2:3;
    g->lane=(g->lane+(direction<0?choices-1:1))%choices;
}
bool nm_game_danger(const nm_game_t *g) {
    return g->kind==NM_GAME_TEA && g->round_ms>=1400;
}
bool nm_game_warning(const nm_game_t *g) {
    return g->kind==NM_GAME_TEA && g->round_ms>=1100 && g->round_ms<1400;
}
bool nm_game_window(const nm_game_t *g) {
    switch(g->kind) {
    case NM_GAME_TYPING: return g->round_ms>=800 && g->round_ms<1300;
    case NM_GAME_COMMUTE: return g->round_ms>=900 && g->round_ms<1600;
    case NM_GAME_BENTO: return g->round_ms>=1400 && g->round_ms<1900;
    case NM_GAME_MEETING: return true; /* Read and respond, not a reflex test. */
    case NM_GAME_ESCAPE: return g->lane==1 || nm_game_lift_ready(g);
    default: return false;
    }
}
bool nm_game_press(nm_game_t *g) {
    if(g->done) return false;
    if(g->kind==NM_GAME_TEA){g->busy=!g->busy;return true;}
    if(g->used) return false;
    g->used=true; ++g->attempts;
    if(g->kind==NM_GAME_ESCAPE){
        bool hit=nm_game_window(g);
        if(hit){
            unsigned descent=g->lane==1?1:2;
            if(descent>g->floors)descent=g->floors;
            g->floors-=descent;g->score+=descent*2;
            if(!g->floors){if(g->round<2)g->score+=2;g->done=true;}
        }
        g->feedback=hit?1:2;return hit;
    }
    bool hit=nm_game_window(g) && g->lane==g->target;
    if(hit){++g->score;if(g->kind==NM_GAME_MEETING)++g->meeting_answers[g->lane];}
    g->feedback=hit?1:2;
    return hit;
}
void nm_game_tick(nm_game_t *g, uint32_t delta_ms) {
    /* Slice at phase boundaries: large and small ticks produce equal results. */
    while(delta_ms && !g->done) {
        uint32_t duration=nm_game_round_duration(g);
        uint32_t boundary=g->kind==NM_GAME_TEA && g->round_ms<1400?1400:duration;
        uint32_t amount=boundary-g->round_ms;
        if(amount>delta_ms) amount=delta_ms;
        if(g->kind==NM_GAME_TEA && !g->busy) {
            if(g->round_ms<1400) g->tea_ms+=amount;
            else g->used=true; /* Caught: only lose this round's bonus. */
        }
        g->round_ms+=amount; g->elapsed+=amount; delta_ms-=amount;
        if(g->round_ms==duration) {
            if(g->kind==NM_GAME_TEA) {
                ++g->attempts;
                bool hit=g->tea_ms>=600 && !g->used;
                if(hit) ++g->score;
                g->feedback=hit?1:2;
            }
            ++g->round;
            if(g->round==8) g->done=true;
            else new_round(g);
        }
    }
}
void nm_game_skip(nm_game_t *g) {if(!g->settled){g->done=true;g->score=0;memset(g->meeting_answers,0,sizeof(g->meeting_answers));}}
bool nm_game_settle(nm_game_t *g, nm_state_t *s) {
    if(!g->done || g->settled) return false;
    if(g->kind==NM_GAME_MEETING){
        unsigned bond=s->relations[0]+g->meeting_answers[0]*2;
        unsigned xp=s->xp+g->meeting_answers[1]*3;
        unsigned talk=s->skill[1]+g->meeting_answers[2]*2;
        s->relations[0]=bond>100?100:bond;
        s->xp=xp>UINT16_MAX?UINT16_MAX:xp;
        s->skill[1]=talk>UINT16_MAX?UINT16_MAX:talk;
        s->stat[NM_ENERGY]=s->stat[NM_ENERGY]>g->meeting_answers[2]?s->stat[NM_ENERGY]-g->meeting_answers[2]:0;
        nm_check_friendship(s);
    }
    nm_game_reward(s,g->score,8);g->settled=true;return true;
}
