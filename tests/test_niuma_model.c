#include "niuma_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void basics(void) {
    nm_state_t s; nm_effect_t e; nm_receipt_t r;
    nm_init(&s,123,true,0); assert(nm_valid(&s));
    assert(nm_act(&s,NM_WORK,0,&e)==NM_OK);
    assert(e.delta[NM_ENERGY]==-12 && e.experience==12 && s.actions==5);
    assert(nm_act(&s,NM_WORK,0,0)==NM_OK);
    assert(nm_act(&s,NM_BENTO,0,&e)==NM_OK && e.coins==-12);
    assert(nm_act(&s,NM_REST,0,0)==NM_OK && s.actions==2);
    nm_end_day(&s,&r);
    assert(r.salary==20 && r.target_bonus==10 && r.expenses==12 && r.net==18);
    assert(s.coins==48 && s.performance==0 && s.xp==24 && s.actions==6);
    assert(s.achievements&(1u<<NM_FIRST_PAY));
}
static void rejects(void) {
    nm_state_t s,before;
    nm_init(&s,1,false,2); s.coins=0; before=s;
    assert(nm_act(&s,NM_BENTO,0,0)==NM_NO_MONEY && memcmp(&s,&before,sizeof(s))==0);
    assert(nm_act(&s,NM_BASIC_MEAL,0,0)==NM_OK);
    assert(nm_act(&s,NM_RECOVER,0,0)==NM_OK);
    assert(nm_act(&s,NM_WATER,0,0)==NM_OK); before=s;
    assert(nm_act(&s,NM_WATER,0,0)==NM_DAILY_LIMIT && memcmp(&s,&before,sizeof(s))==0);
    s.actions=0; before=s;
    assert(nm_act(&s,NM_WORK,0,0)==NM_NO_ACTIONS && memcmp(&s,&before,sizeof(s))==0);
    assert(nm_act(&s,(nm_action_t)-1,0,0)==NM_BAD_INPUT);
    s.actions=6; s.day=6;
    assert(nm_act(&s,NM_WORK,0,0)==NM_LOCKED);
    s.day=1; s.burnout=1;
    assert(nm_act(&s,NM_OVERTIME,0,0)==NM_TOO_TIRED);
}
static void evening(void) {
    nm_state_t s,before; nm_receipt_t r;
    nm_init(&s,123,false,0);
    assert(nm_act(&s,NM_WORK,0,0)==NM_OK);
    assert(nm_act(&s,NM_WORK,0,0)==NM_OK);
    nm_close_work(&s,&r);
    assert(s.evening && s.day==1 && s.actions==4 && s.coins==60);
    assert(r.salary==20 && r.target_bonus==10 && r.net==30);
    before=s; nm_close_work(&s,&r);
    assert(memcmp(&s,&before,sizeof(s))==0);
    assert(nm_act(&s,NM_WORK,0,0)==NM_LOCKED);
    assert(nm_act(&s,NM_BENTO,0,0)==NM_OK);
    nm_end_day(&s,&r);
    assert(r.net==18 && s.coins==48 && s.day==2 && s.actions==6 && !s.evening);
    assert(!s.paid_salary && !s.paid_target_bonus && nm_valid(&s));
    s.day=6; nm_close_work(&s,&r);
    assert(r.weekend && !r.salary && !r.target_bonus && s.coins==48);
    nm_end_day(&s,0); assert(s.day==7 && s.coins==48);
}
static void growth(void) {
    nm_state_t s; nm_receipt_t r; bool raised=true;
    nm_init(&s,7,true,1);
    for(unsigned i=0;i<3;++i){s.stat[NM_STRESS]=90;nm_end_day(&s,&r);}
    assert(s.burnout && r.burnout_started);
    s.stat[NM_STRESS]=20; nm_end_day(&s,0); nm_end_day(&s,&r);
    assert(!s.burnout && r.burnout_ended);
    s.day=28; s.skill[1]=24; s.xp=120;
    assert(nm_negotiate(&s,&raised)==NM_OK && !raised);
    assert(nm_negotiate(&s,&raised)==NM_LOCKED);
    s.day=56; assert(nm_negotiate(&s,&raised)==NM_OK && raised && s.salary==24);
    s.coins=200; s.skill[0]=40;
    assert(nm_buy(&s,0)==NM_OK && s.coins==176);
    assert(nm_equip(&s,0)==NM_OK);
    assert(nm_resign(&s)==NM_OK && s.skill[0]==40 && s.owned==1);
    assert(nm_join(&s,NM_FLEX)==NM_OK && s.coins==176 && s.equipped[0]==0);
    assert(nm_valid(&s));
}
static void growth_effects(void){
    nm_state_t s;nm_effect_t e;nm_init(&s,1,true,0);
    s.skill[0]=UINT16_MAX-1;s.xp=UINT16_MAX-1;
    assert(nm_act(&s,NM_STUDY_TECH,0,&e)==NM_OK && e.skill[0]==1 && e.experience==1);
    assert(nm_act(&s,NM_STUDY_TECH,0,&e)==NM_OK && e.skill[0]==0 && e.experience==0);
    s.relations[2]=99;assert(nm_act(&s,NM_CHAT,2,&e)==NM_OK && e.relations[2]==1);
    assert(nm_act(&s,NM_GIFT,2,&e)==NM_OK && e.relations[2]==0);
}
static void review_boundaries(void){
    nm_state_t s;nm_init(&s,1,true,0);bool raised=true;
    assert(nm_review_wait_days(&s)==27);
    s.day=28;assert(nm_review_wait_days(&s)==0);
    assert(nm_negotiate(&s,&raised)==NM_OK && !raised && nm_review_wait_days(&s)==28);
    s.day=55;assert(nm_review_wait_days(&s)==1 && !nm_review_available(&s));
    s.day=56;s.xp=120;s.skill[1]=24;s.salary=99;
    assert(nm_negotiate(&s,&raised)==NM_OK && raised && s.salary==100);
    s.day=84;nm_state_t before=s;
    assert(!nm_review_available(&s));assert(nm_negotiate(&s,&raised)==NM_LOCKED && !raised);
    assert(!memcmp(&s,&before,sizeof(s)));
    s.day=UINT32_MAX;s.last_review_day=UINT32_MAX-10;assert(nm_review_wait_days(&s)==18);
    nm_init(&s,31,true,0);s.company=NM_FLEX;s.rank=4;s.salary=40;
    nm_state_t higher=s;higher.salary=44;nm_receipt_t low_pay,high_pay;
    nm_close_work(&s,&low_pay);nm_close_work(&higher,&high_pay);
    assert(high_pay.salary==low_pay.salary+4); /* Raises still matter at maximum rank. */
}
static void simulate(void) {
    /* Many mixed-care playthroughs, including poor, exhausted, retired and
       newly employed states; validation is checked after every operation. */
    for(unsigned seed=1;seed<=30;++seed){
        nm_state_t s; nm_init(&s,seed,seed%2,seed%4);
        for(unsigned day=0;day<365;++day){
            for(unsigned i=0;i<12;++i){
                unsigned pick=nm_random(&s)%NM_ACTION_COUNT;
                nm_state_t before=s;
                nm_error_t error=nm_act(&s,(nm_action_t)pick,seed%3,0);
                if(error!=NM_OK) assert(memcmp(&s,&before,sizeof(s))==0);
                assert(nm_valid(&s));
            }
            nm_end_day(&s,0); assert(nm_valid(&s));
        }
    }
}
static void directions(void){
    nm_state_t s,before;nm_effect_t e;nm_receipt_t r;
    nm_init(&s,12,true,1);before=s;
    assert(nm_choose_route(&s,NM_ROUTE_EXPERT)==NM_LOCKED && !memcmp(&s,&before,sizeof(s)));
    s.skill[0]=120;s.skill[1]=120;s.skill[2]=96;s.xp=240;s.relations[1]=60;s.ontime_days=10;s.coins=200;
    assert(nm_choose_route(&s,NM_ROUTE_EXPERT)==NM_OK);
    assert(nm_act(&s,NM_WORK,0,&e)==NM_OK && e.experience==18);
    before=s;assert(nm_choose_route(&s,NM_ROUTE_MANAGER)==NM_DAILY_LIMIT && !memcmp(&s,&before,sizeof(s)));
    nm_end_day(&s,0);assert(nm_choose_route(&s,NM_ROUTE_MANAGER)==NM_OK);
    unsigned bond=s.relations[0];assert(nm_act(&s,NM_WORK,0,0)==NM_OK && s.relations[0]==bond+1);
    nm_end_day(&s,0);assert(nm_resign(&s)==NM_OK);
    for(unsigned route=NM_ROUTE_FREELANCE;route<=NM_ROUTE_SHOP;++route){
        assert(nm_choose_route(&s,(nm_route_t)route)==NM_OK);
        uint32_t cash=s.coins;
        assert(nm_act(&s,NM_WORK,0,0)==NM_OK);
        assert(nm_act(&s,NM_OVERTIME,0,0)==NM_LOCKED);
        nm_close_work(&s,&r);assert(r.salary==(route==NM_ROUTE_FREELANCE?12:10));
        assert(s.coins==cash+r.salary);before=s;nm_close_work(&s,0);assert(!memcmp(&s,&before,sizeof(s)));
        nm_end_day(&s,0);
    }
    assert(nm_choose_route(&s,NM_ROUTE_COMFORT)==NM_OK);
    s.stat[NM_MOOD]=20;s.stat[NM_STRESS]=60;
    assert(nm_act(&s,NM_REST,0,&e)==NM_OK && e.delta[NM_MOOD]==9 && e.delta[NM_STRESS]==-19);
    assert(nm_join(&s,NM_STABLE)==NM_OK && s.route==NM_ROUTE_EMPLOYEE);
    for(unsigned i=0;i<NM_ROUTE_COUNT-1;++i)assert(s.route_day[i]);
    assert(nm_valid(&s));
    for(unsigned i=0;i<12;++i)assert(nm_item_price(i)>0);
    assert(nm_item_price(12)==0);
}
int main(void) {
    basics(); rejects(); evening(); growth(); growth_effects(); review_boundaries(); directions(); simulate();
    puts("NiuMa model: PASS (30 x 365 simulated days)");
    return 0;
}
