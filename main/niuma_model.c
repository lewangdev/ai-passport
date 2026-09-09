#include "niuma_model.h"

#include <limits.h>
#include <string.h>

const nm_action_info_t nm_actions[NM_ACTION_COUNT] = {
    [NM_WORK]       = {"处理任务", "Work",       {-8,-12,0,8,0}, 0,1},
    [NM_OVERTIME]   = {"自愿加班", "Overtime",  {-10,-20,-6,20,-3},0,1},
    [NM_REST]       = {"闭眼小憩", "Rest",       {-3,24,4,-14,2},0,1},
    [NM_SLACK]      = {"茶水间摸鱼", "Tea break",{-3,5,18,-10,0},0,1},
    [NM_STUDY_TECH] = {"专业技能", "Technical", {-5,-10,3,3,0},0,1},
    [NM_STUDY_TALK] = {"表达沟通", "Communication",{-5,-10,3,3,0},0,1},
    [NM_STUDY_LIFE] = {"生活技能", "Life skills",{-5,-8,6,-3,0},0,1},
    [NM_EXERCISE]   = {"出去散步", "Exercise", {-8,-8,10,-12,8},0,1},
    [NM_CLEAN]      = {"清理桌面", "Tidy up",  {-3,-5,8,-4,2},0,1},
    [NM_TREAT]      = {"预约治疗", "Treatment", {0,10,5,-10,30},20,1},
    [NM_RECOVER]    = {"免费休养", "Recovery", {0,18,5,-18,12},0,1},
    [NM_CHAT]       = {"聊聊今天", "Chat",     {-3,-4,12,-8,0},0,1},
    [NM_GIFT]       = {"送份点心", "Give snack",{-3,-3,14,-5,0},8,1},
    [NM_TOILET]     = {"带薪思考", "Quiet moment",{0,0,2,-3,0},0,0},
    [NM_WATER]      = {"免费白开水", "Water",   {0,3,0,0,1},0,0},
    [NM_BASIC_MEAL] = {"免费基础餐", "Basic meal",{24,4,0,0,0},0,1},
    [NM_BENTO]      = {"热乎便当", "Bento",   {35,6,5,0,2},12,1},
    [NM_NOODLES]    = {"应急泡面", "Noodles", {22,3,2,0,0},6,1},
    [NM_COFFEE]     = {"提神咖啡", "Coffee",  {0,18,2,0,0},8,1},
    [NM_MILK_TEA]   = {"开心奶茶", "Milk tea",{8,4,20,-3,0},15,1},
    [NM_COOK]       = {"自己做便当", "Cook",  {40,-5,12,-6,5},8,1},
};

static uint8_t clamp_stat(int n) { return n < 0 ? 0 : n > 100 ? 100 : (uint8_t)n; }
static uint16_t add16(uint16_t a, unsigned b) { return b > UINT16_MAX-a ? UINT16_MAX : (uint16_t)(a+b); }
static uint32_t add32(uint32_t a, uint32_t b) { return b > UINT32_MAX-a ? UINT32_MAX : a+b; }
static void award(nm_state_t *s, nm_achievement_t a) {
    if(!(s->achievements & (UINT32_C(1)<<a))) s->memory_day[a]=s->day;
    s->achievements |= UINT32_C(1) << a;
}
static void earn(nm_state_t *s, unsigned coins) {
    s->coins = add32(s->coins, coins);
    s->earned_total = add32(s->earned_total, coins);
}
void nm_check_friendship(nm_state_t *s){
    for(unsigned i=0;i<3;++i)if(s->relations[i]>=60)award(s,NM_NEW_FRIEND);
}
static void style_add(nm_state_t *s, nm_style_t style) {
    uint8_t *v = &s->style_history[s->history_cursor][style];
    if (*v < 255) ++*v;
}
uint32_t nm_random(nm_state_t *s) {
    uint32_t x = s->rng ? s->rng : UINT32_C(0x4e69754d);
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return s->rng = x;
}
void nm_init(nm_state_t *s, uint32_t seed, bool female, uint8_t name) {
    memset(s, 0, sizeof(*s));
    s->day=1; s->rng=seed ? seed : 1; s->coins=30; s->salary=20;
    s->actions=6; s->stat[NM_SATIETY]=70; s->stat[NM_ENERGY]=85;
    s->stat[NM_MOOD]=75; s->stat[NM_STRESS]=15; s->stat[NM_HEALTH]=90;
    s->gender=female; s->name=name%4; s->volume=40; s->brightness=60;
    s->idle_seconds=60; s->key_sound=1;
    memset(s->equipped, 255, sizeof(s->equipped));
    s->relations[0]=20; s->relations[1]=30; s->relations[2]=40;
}
bool nm_weekend(const nm_state_t *s) { return (s->day-1)%7 >= 5; }
bool nm_valid(const nm_state_t *s) {
    if (!s || !s->day || s->company>NM_UNEMPLOYED || s->gender>1 || s->name>3 ||
        s->actions>6 || s->history_cursor>=14 || s->volume>100 || s->brightness<10 ||
        s->brightness>100 || s->music>1 || s->key_sound>1 || s->english>1 ||
        s->salary<20 || s->salary>100 || s->rank>4 || s->burnout>1 || s->mess>3 ||
        s->waters>1 || s->toilets>1 || s->overtime>1 || s->review_done>1 ||
        s->event_done>1 || s->event>7 || s->coffees>6 || s->meals>6 ||
        s->performance>100 || s->last_review_day>s->day || s->evening>1 ||
        s->paid_salary>120 || s->paid_target_bonus>18 || s->route>=NM_ROUTE_COUNT ||
        (!s->evening && (s->paid_salary || s->paid_target_bonus)) ||
        (s->idle_seconds!=0 && s->idle_seconds!=30 && s->idle_seconds!=60 && s->idle_seconds!=120)) return false;
    for (unsigned i=0;i<NM_STAT_COUNT;++i) if(s->stat[i]>100) return false;
    for(unsigned i=0;i<NM_ACHIEVEMENT_COUNT;++i) if(s->memory_day[i]>s->day) return false;
    for(unsigned i=0;i<NM_ROUTE_COUNT-1;++i) if(s->route_day[i]>s->day) return false;
    if(s->route && !s->route_day[s->route-1]) return false;
    if((s->route==NM_ROUTE_FREELANCE || s->route==NM_ROUTE_SHOP) && s->company!=NM_UNEMPLOYED) return false;
    for(unsigned i=0;i<6;++i) if(s->food_stock[i]>9) return false;
    for (unsigned i=0;i<3;++i) {
        if(s->relations[i]>100) return false;
        if(s->equipped[i]!=255 && (s->equipped[i]>=12 || s->equipped[i]/4!=i ||
            !(s->owned & (1u<<s->equipped[i])))) return false;
    }
    return true;
}
nm_style_t nm_style(const nm_state_t *s) {
    unsigned totals[6]={0};
    for (unsigned d=0;d<14;++d) for(unsigned i=0;i<6;++i) totals[i]+=s->style_history[d][i];
    unsigned best=0;
    for(unsigned i=1;i<6;++i) if(totals[i]>totals[best]) best=i;
    return (nm_style_t)best;
}
nm_error_t nm_can_act(const nm_state_t *s, nm_action_t a, unsigned friend_index) {
    if((unsigned)a>=NM_ACTION_COUNT || friend_index>=3) return NM_BAD_INPUT;
    const nm_action_info_t *info=&nm_actions[a];
    if(s->actions<info->slots) return NM_NO_ACTIONS;
    if(s->coins<info->cost) return NM_NO_MONEY;
    if((a==NM_WATER && s->waters) || (a==NM_TOILET && s->toilets) ||
        (a==NM_OVERTIME && s->overtime)) return NM_DAILY_LIMIT;
    bool independent=s->route==NM_ROUTE_FREELANCE || s->route==NM_ROUTE_SHOP;
    if((a==NM_WORK || a==NM_OVERTIME) && ((!independent && s->company==NM_UNEMPLOYED) || nm_weekend(s) || s->evening)) return NM_LOCKED;
    if(a==NM_OVERTIME && independent) return NM_LOCKED;
    if(a==NM_OVERTIME && (s->burnout || s->stat[NM_ENERGY]<25 || s->stat[NM_HEALTH]<30)) return NM_TOO_TIRED;
    if(a==NM_WORK && s->stat[NM_ENERGY]<12) return NM_TOO_TIRED;
    if(a==NM_COOK && s->skill[2]<24) return NM_LOCKED;
    return NM_OK;
}
nm_error_t nm_act(nm_state_t *s, nm_action_t a, unsigned friend_index, nm_effect_t *effect) {
    nm_error_t err=nm_can_act(s,a,friend_index);
    if(effect) memset(effect,0,sizeof(*effect));
    if(err!=NM_OK) return err; /* Failed actions are byte-for-byte non-mutating. */
    nm_state_t before=*s;
    const nm_action_info_t *info=&nm_actions[a];
    s->coins-=info->cost; s->today_spent=add16(s->today_spent,info->cost); s->actions-=info->slots;
    for(unsigned i=0;i<NM_STAT_COUNT;++i) s->stat[i]=clamp_stat(s->stat[i]+info->delta[i]);
    switch(a) {
    case NM_WORK:
        s->performance=clamp_stat(s->performance+10); s->xp=add16(s->xp,12);
        s->stat[NM_STRESS]=clamp_stat(s->stat[NM_STRESS]+(s->company==NM_PROJECT?3:0));
        style_add(s,NM_EXPERT); break;
    case NM_OVERTIME:
        s->overtime=1; s->performance=clamp_stat(s->performance+5); earn(s,16);
        s->today_bonus=add16(s->today_bonus,16); style_add(s,NM_STRIVER); break;
    case NM_REST: case NM_RECOVER: case NM_TREAT: style_add(s,NM_LIFESTYLE); break;
    case NM_SLACK: style_add(s,NM_SLACKER); break;
    case NM_STUDY_TECH: case NM_STUDY_TALK: case NM_STUDY_LIFE:
        s->skill[a-NM_STUDY_TECH]=add16(s->skill[a-NM_STUDY_TECH],12);
        s->xp=add16(s->xp,6); style_add(s,a==NM_STUDY_TECH?NM_EXPERT:NM_LIFESTYLE); break;
    case NM_CLEAN: s->mess=0; style_add(s,NM_LIFESTYLE); break;
    case NM_EXERCISE: style_add(s,NM_LIFESTYLE); break;
    case NM_CHAT: case NM_GIFT:
        s->relations[friend_index]=clamp_stat(s->relations[friend_index]+(a==NM_CHAT?8:15));
        if(s->relations[friend_index]>=60) award(s,NM_NEW_FRIEND);
        style_add(s,NM_SOCIAL); break;
    case NM_TOILET: s->toilets=1; break;
    case NM_WATER: s->waters=1; break;
    case NM_COFFEE:
        if(s->coffees++) s->stat[NM_STRESS]=clamp_stat(s->stat[NM_STRESS]+10);
        if(s->mess<3) ++s->mess;
        break;
    case NM_NOODLES:
        if(s->noodle_days<255) ++s->noodle_days;
        if(s->noodle_days>=3) s->stat[NM_HEALTH]=clamp_stat(s->stat[NM_HEALTH]-5);
        if(s->mess<3) ++s->mess;
        ++s->meals; break;
    case NM_COOK: award(s,NM_FIRST_COOK); style_add(s,NM_LIFESTYLE); /* fall through */
    case NM_BENTO: case NM_BASIC_MEAL:
        /* Essential feeding is style-neutral; deliberate cooking/self-care
         * contributes to lifestyle without eclipsing every other play style. */
        s->noodle_days=0; ++s->meals; break;
    default: break;
    }
    if(a==NM_WORK && s->route==NM_ROUTE_EXPERT)s->xp=add16(s->xp,6);
    if(a==NM_WORK && s->route==NM_ROUTE_MANAGER){
        for(unsigned i=0;i<3;++i){s->relations[i]=clamp_stat(s->relations[i]+1);if(s->relations[i]>=60)award(s,NM_NEW_FRIEND);}
    }
    if((a==NM_REST || a==NM_EXERCISE) && s->route==NM_ROUTE_COMFORT){
        s->stat[NM_MOOD]=clamp_stat(s->stat[NM_MOOD]+5);
        s->stat[NM_STRESS]=clamp_stat(s->stat[NM_STRESS]-5);
    }
    s->revision++;
    if(effect) {
        for(unsigned i=0;i<NM_STAT_COUNT;++i) effect->delta[i]=(int16_t)s->stat[i]-before.stat[i];
        effect->coins=(int32_t)((int64_t)s->coins-before.coins);
        effect->experience=s->xp-before.xp; effect->slots=info->slots;
        for(unsigned i=0;i<3;++i){effect->skill[i]=s->skill[i]-before.skill[i];effect->relations[i]=s->relations[i]-before.relations[i];}
    }
    return NM_OK;
}
void nm_close_work(nm_state_t *s, nm_receipt_t *receipt) {
    nm_receipt_t r={0};
    r.weekend=nm_weekend(s); r.expenses=s->today_spent; r.game_bonus=s->today_bonus;
    if(!s->evening && !r.weekend && s->company!=NM_UNEMPLOYED) {
        r.salary=s->salary;
        r.target_bonus=s->performance>=20?10:0;
        if(s->company==NM_PROJECT && r.target_bonus) r.target_bonus+=nm_random(s)%9;
        if(s->company==NM_FLEX) r.salary=16+nm_random(s)%13+(s->salary-20);
        earn(s,r.salary+r.target_bonus); s->working_days=add16(s->working_days,1); award(s,NM_FIRST_PAY);
        if(!s->overtime) {
            s->ontime_days=add16(s->ontime_days,1); style_add(s,NM_ONTIME);
            if(s->ontime_days>=10) award(s,NM_TEN_ONTIME);
        }
    }
    if(!s->evening) {
        if(!r.weekend && (s->route==NM_ROUTE_FREELANCE || s->route==NM_ROUTE_SHOP)) {
            unsigned rate=s->route==NM_ROUTE_FREELANCE?12:10;
            r.salary=(s->performance/10)*rate;
            if(r.salary){earn(s,r.salary);s->working_days=add16(s->working_days,1);award(s,NM_FIRST_PAY);}
        }
        s->paid_salary=r.salary; s->paid_target_bonus=r.target_bonus;
        s->evening=1; ++s->revision;
    }
    r.salary=s->paid_salary; r.target_bonus=s->paid_target_bonus;
    r.net=(int32_t)r.salary+r.target_bonus+r.game_bonus-r.expenses;
    if(receipt) *receipt=r;
}
void nm_end_day(nm_state_t *s, nm_receipt_t *receipt) {
    nm_receipt_t r;
    nm_close_work(s,&r);
    if(s->stat[NM_STRESS]>=75) { if(s->bad_days<3) ++s->bad_days; } else s->bad_days=0;
    if(s->stat[NM_STRESS]<=35) { if(s->good_days<2) ++s->good_days; } else s->good_days=0;
    if(s->bad_days>=3 && !s->burnout) { s->burnout=1; r.burnout_started=true; }
    if(s->good_days>=2 && s->burnout) { s->burnout=0; r.burnout_ended=true; }
    if(!s->meals) s->stat[NM_HEALTH]=clamp_stat(s->stat[NM_HEALTH]-3);
    s->stat[NM_ENERGY]=clamp_stat(s->stat[NM_ENERGY]+35);
    s->stat[NM_SATIETY]=clamp_stat(s->stat[NM_SATIETY]-15);
    if(s->coins>=500 && s->ontime_days>=20 && s->stat[NM_HEALTH]>=70) award(s,NM_LIFE_MASTER);
    s->day=add32(s->day,1); s->history_cursor=(s->history_cursor+1)%14;
    memset(s->style_history[s->history_cursor],0,6);
    s->actions=6; s->performance=0; s->overtime=0; s->coffees=0; s->waters=0;
    s->toilets=0; s->meals=0; s->event_done=0; s->event=nm_random(s)%8;
    s->today_spent=0; s->today_bonus=0; s->evening=0;
    s->paid_salary=0; s->paid_target_bonus=0; s->revision++;
    if(receipt) *receipt=r;
}
void nm_game_reward(nm_state_t *s, unsigned score, unsigned maximum) {
    /* Called exactly once by the completed minigame session, never by UI redraw. */
    if(!maximum) return;
    if(score>maximum) score=maximum;
    unsigned bonus=(unsigned)((uint64_t)score*8/maximum);
    earn(s,bonus); s->today_bonus=add16(s->today_bonus,bonus); s->revision++;
}
static const uint8_t prices[12]={24,32,40,48,40,60,80,96,20,28,36,44};
unsigned nm_item_price(unsigned item){return item<12?prices[item]:0;}
nm_error_t nm_buy(nm_state_t *s, unsigned item) {
    if(item>=12) return NM_BAD_INPUT;
    if(s->owned&(1u<<item)) return NM_DAILY_LIMIT;
    if(s->coins<prices[item]) return NM_NO_MONEY;
    s->coins-=prices[item]; s->today_spent=add16(s->today_spent,prices[item]);
    s->owned|=1u<<item; ++s->revision; return NM_OK;
}
nm_error_t nm_equip(nm_state_t *s, unsigned item) {
    if(item>=12) return NM_BAD_INPUT;
    if(!(s->owned&(1u<<item))) return NM_LOCKED;
    s->equipped[item/4]=item; ++s->revision; return NM_OK;
}
nm_error_t nm_resign(nm_state_t *s) {
    if(s->company==NM_UNEMPLOYED) return NM_LOCKED;
    s->company=NM_UNEMPLOYED; s->performance=0; s->route=NM_ROUTE_EMPLOYEE; award(s,NM_RESIGNED);
    ++s->revision; return NM_OK;
}
nm_error_t nm_join(nm_state_t *s, nm_company_t company) {
    if((unsigned)company>=NM_UNEMPLOYED) return NM_BAD_INPUT;
    if(s->company!=NM_UNEMPLOYED) return NM_LOCKED;
    s->company=company; s->rank=0; s->salary=20; s->performance=0; s->route=NM_ROUTE_EMPLOYEE;
    ++s->revision; return NM_OK;
}
unsigned nm_review_wait_days(const nm_state_t *s){
    if(s->day<28)return 28-s->day;
    uint32_t elapsed=s->day-s->last_review_day;
    return elapsed>=28?0:28-elapsed;
}
bool nm_review_available(const nm_state_t *s) {
    return s->company!=NM_UNEMPLOYED && s->salary<100 && nm_review_wait_days(s)==0;
}
nm_error_t nm_negotiate(nm_state_t *s, bool *raised) {
    if(raised) *raised=false;
    if(!nm_review_available(s)) return NM_LOCKED;
    s->last_review_day=s->day;
    bool success=s->negotiation>=10 && s->skill[1]>=24 && s->xp>=120;
    s->negotiation=add16(s->negotiation,10);
    if(success) {
        if(s->rank<4) ++s->rank;
        s->salary=s->salary>96?100:s->salary+4;
        award(s,NM_FIRST_RAISE);
    } else award(s,NM_PIE);
    if(raised) *raised=success;
    ++s->revision; return NM_OK;
}

static const nm_action_t food_actions[6]={NM_BASIC_MEAL,NM_BENTO,NM_NOODLES,NM_COFFEE,NM_MILK_TEA,NM_COOK};
nm_error_t nm_stock_food(nm_state_t *s,unsigned food) {
    if(food>=6) return NM_BAD_INPUT;
    if(food==0 || food==5) return NM_LOCKED; /* Free meal is eaten, cooked meal requires an action. */
    if(s->food_stock[food]>=9) return NM_DAILY_LIMIT;
    unsigned price=nm_actions[food_actions[food]].cost;
    if(s->coins<price) return NM_NO_MONEY;
    s->coins-=price; s->today_spent=add16(s->today_spent,price);
    ++s->food_stock[food]; ++s->revision; return NM_OK;
}
nm_error_t nm_eat_stock(nm_state_t *s,unsigned food,nm_effect_t *effect) {
    if(food>=6) return NM_BAD_INPUT;
    if(!s->food_stock[food]) return NM_LOCKED;
    /* Use a copy so rejected consumption cannot change cash or inventory. */
    nm_state_t next=*s;
    unsigned price=nm_actions[food_actions[food]].cost;
    if(next.coins>UINT32_MAX-price) return NM_BAD_INPUT;
    next.coins+=price;
    nm_error_t err=nm_act(&next,food_actions[food],0,effect);
    if(err!=NM_OK) return err;
    next.today_spent=s->today_spent; --next.food_stock[food];
    if(effect) effect->coins=0;
    *s=next; return NM_OK;
}
nm_error_t nm_route_available(const nm_state_t *s,nm_route_t route) {
    if((unsigned)route>=NM_ROUTE_COUNT)return NM_BAD_INPUT;
    if(s->evening || s->actions!=6 || s->performance)return NM_DAILY_LIMIT;
    switch(route){
    case NM_ROUTE_EMPLOYEE:return NM_OK;
    case NM_ROUTE_EXPERT:return s->company!=NM_UNEMPLOYED && s->skill[0]>=120 && s->xp>=240?NM_OK:NM_LOCKED;
    case NM_ROUTE_MANAGER:return s->company!=NM_UNEMPLOYED && s->skill[1]>=120 && s->relations[1]>=60?NM_OK:NM_LOCKED;
    case NM_ROUTE_FREELANCE:return s->company==NM_UNEMPLOYED && s->skill[0]>=96 && s->coins>=100?NM_OK:NM_LOCKED;
    case NM_ROUTE_SHOP:return s->company==NM_UNEMPLOYED && s->skill[2]>=96 && s->coins>=200?NM_OK:NM_LOCKED;
    case NM_ROUTE_COMFORT:return s->ontime_days>=10 && s->skill[2]>=48?NM_OK:NM_LOCKED;
    default:return NM_BAD_INPUT;
    }
}
nm_error_t nm_choose_route(nm_state_t *s,nm_route_t route) {
    nm_error_t err=nm_route_available(s,route);if(err!=NM_OK)return err;
    if(s->route==route)return NM_OK;
    s->route=route;
    if(route && !s->route_day[route-1])s->route_day[route-1]=s->day;
    ++s->revision;return NM_OK;
}
