#include "niuma_model.h"
#include "niuma_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Progression acceptance: never assign coins, skills, relationships, date or
 * achievements. Every milestone must be reached through public player actions. */
static void act(nm_state_t *s,nm_action_t action,unsigned friend_id){
    assert(nm_act(s,action,friend_id,NULL)==NM_OK);assert(nm_valid(s));
}
static void restore(nm_state_t *s){
    uint8_t bytes[NM_SAVE_CAPACITY],again[NM_SAVE_CAPACITY];nm_state_t loaded;
    size_t n=nm_save_encode(s,bytes,sizeof(bytes));assert(n && nm_save_decode(&loaded,bytes,n));
    assert(nm_save_encode(&loaded,again,sizeof(again))==n && !memcmp(bytes,again,n));
    *s=loaded;assert(nm_valid(s));
}
static void finish(nm_state_t *s){
    nm_close_work(s,NULL);restore(s);nm_end_day(s,NULL);restore(s);
}
static void career_journey(unsigned gender){
    nm_state_t s;nm_init(&s,41,gender,0);
    bool cooked=false,raised=true;
    while(s.day<=56){
        if(s.day==28 || s.day==56){
            assert(nm_negotiate(&s,&raised)==NM_OK);
            assert(raised==(s.day==56));restore(&s);
        }
        act(&s,NM_BASIC_MEAL,0);
        if(!cooked && s.skill[2]>=24){act(&s,NM_COOK,0);cooked=true;}
        else act(&s,NM_BASIC_MEAL,0);
        act(&s,nm_weekend(&s)?NM_CHAT:NM_WORK,1);
        act(&s,(nm_action_t)(NM_STUDY_TECH+(s.day-1)%3),0);
        act(&s,NM_RECOVER,0);
        act(&s,s.skill[2]<96?NM_STUDY_LIFE:s.relations[1]<60?NM_CHAT:s.skill[0]<120?NM_STUDY_TECH:NM_STUDY_TALK,1);
        assert(s.actions==0);finish(&s);
    }
    assert(s.salary==24 && s.skill[0]>=120 && s.skill[1]>=120 && s.skill[2]>=96);
    assert(s.relations[1]>=60 && s.xp>=240 && s.ontime_days>=20 && s.stat[NM_HEALTH]>=70);
    for(unsigned item=0;item<12;++item){assert(nm_buy(&s,item)==NM_OK);assert(nm_equip(&s,item)==NM_OK);restore(&s);}
    assert(s.owned==0xfff);
    assert(nm_choose_route(&s,NM_ROUTE_EXPERT)==NM_OK);nm_effect_t effect;
    assert(nm_act(&s,NM_WORK,0,&effect)==NM_OK && effect.experience==18);finish(&s);
    assert(nm_choose_route(&s,NM_ROUTE_MANAGER)==NM_OK);unsigned bond=s.relations[0];
    act(&s,NM_WORK,0);assert(s.relations[0]==bond+1);finish(&s);
    assert(nm_resign(&s)==NM_OK);restore(&s);
    for(unsigned route=NM_ROUTE_FREELANCE;route<=NM_ROUTE_SHOP;++route){
        assert(nm_choose_route(&s,(nm_route_t)route)==NM_OK);uint32_t cash=s.coins;
        act(&s,NM_WORK,0);nm_close_work(&s,NULL);assert(s.coins==cash+(route==NM_ROUTE_FREELANCE?12:10));finish(&s);
    }
    assert(nm_choose_route(&s,NM_ROUTE_COMFORT)==NM_OK);act(&s,NM_REST,0);finish(&s);
    for(unsigned company=0;company<NM_UNEMPLOYED;++company){
        if(s.company!=NM_UNEMPLOYED)assert(nm_resign(&s)==NM_OK);
        uint32_t cash=s.coins;uint16_t xp=s.xp;
        assert(nm_join(&s,(nm_company_t)company)==NM_OK && s.coins==cash && s.xp==xp && s.owned==0xfff);
        restore(&s);
    }
    assert(s.achievements==((1u<<NM_ACHIEVEMENT_COUNT)-1));
    for(unsigned i=0;i<NM_ACHIEVEMENT_COUNT;++i)assert(s.memory_day[i] && s.memory_day[i]<=s.day);
    for(unsigned i=0;i<NM_ROUTE_COUNT-1;++i)assert(s.route_day[i] && s.route_day[i]<=s.day);
    printf("Natural career journey: gender %u, day %lu, all routes/items/achievements\n",gender,(unsigned long)s.day);
}
static void burnout_journey(void){
    nm_state_t s;nm_init(&s,1,0,0);nm_receipt_t receipt;
    for(unsigned day=0;day<4;++day){
        act(&s,NM_OVERTIME,0);act(&s,NM_COFFEE,0);act(&s,NM_COFFEE,0);
        act(&s,NM_NOODLES,0);act(&s,NM_WORK,0);act(&s,NM_WORK,0);
        nm_end_day(&s,&receipt);restore(&s);
    }
    assert(s.burnout && receipt.burnout_started);
    assert(nm_can_act(&s,NM_OVERTIME,0)==NM_TOO_TIRED);
    for(unsigned day=0;day<2;++day){
        act(&s,NM_BASIC_MEAL,0);while(s.actions)act(&s,NM_RECOVER,0);
        nm_end_day(&s,&receipt);restore(&s);
    }
    assert(!s.burnout && receipt.burnout_ended);
}
static void style_journey(void){
    nm_state_t s;nm_init(&s,1,0,0);
    for(unsigned style=0;style<6;++style){
        /* A new policy replaces the old style within its rolling window. */
        for(unsigned day=0;day<21;++day){
            if(style==NM_STRIVER && !nm_weekend(&s)){
                if(nm_can_act(&s,NM_OVERTIME,0)==NM_OK){
                    act(&s,NM_OVERTIME,0);act(&s,NM_BASIC_MEAL,0);act(&s,NM_REST,0);
                    act(&s,NM_MILK_TEA,0);act(&s,NM_MILK_TEA,0);act(&s,NM_WATER,0);
                }else{act(&s,NM_BASIC_MEAL,0);while(s.actions)act(&s,NM_RECOVER,0);}
            }else if(style!=NM_STRIVER){
                nm_action_t a=style==NM_EXPERT?NM_STUDY_TECH:style==NM_SOCIAL?NM_CHAT:style==NM_SLACKER?NM_SLACK:NM_RECOVER;
                if(style!=NM_ONTIME)for(unsigned i=0;i<3;++i)act(&s,a,0);
                act(&s,NM_BASIC_MEAL,0);
                if(style!=NM_ONTIME){act(&s,NM_BASIC_MEAL,0);act(&s,NM_RECOVER,0);}
            }
            finish(&s);
        }
        printf("Style policy %u -> %u at day %lu\n",style,nm_style(&s),(unsigned long)s.day);fflush(stdout);
        assert(nm_style(&s)==(nm_style_t)style);
    }
}
int main(void){career_journey(0);career_journey(1);burnout_journey();style_journey();puts("NiuMa natural progression: PASS");}
