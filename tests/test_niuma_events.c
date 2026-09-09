#include "niuma_events.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
int main(void){
    for(unsigned e=0;e<8;++e)for(unsigned c=0;c<3;++c){
        nm_state_t s;nm_init(&s,1,1,0);s.event=e;s.relations[0]=50;
        nm_state_t initial=s;nm_effect_t event_effect;
        assert(nm_event_choose(&s,c,&event_effect)==NM_OK && s.event_done);
        assert(event_effect.experience==s.xp-initial.xp);
        for(unsigned i=0;i<3;++i){assert(event_effect.skill[i]==s.skill[i]-initial.skill[i]);assert(event_effect.relations[i]==s.relations[i]-initial.relations[i]);}
        nm_state_t before=s;
        assert(nm_event_choose(&s,c,0)==NM_DAILY_LIMIT && !memcmp(&s,&before,sizeof(s)));
        assert(nm_valid(&s));
    }
    nm_state_t s;nm_init(&s,2,0,1);nm_state_t before=s;
    assert(nm_event_choose(&s,2,0)==NM_LOCKED && !memcmp(&s,&before,sizeof(s)));
    assert(nm_stock_food(&s,1)==NM_OK && s.coins==18 && s.food_stock[1]==1);
    nm_effect_t effect;
    assert(nm_eat_stock(&s,1,&effect)==NM_OK && s.coins==18 && effect.coins==0);
    assert(!s.food_stock[1] && s.actions==5 && s.today_spent==12);
    assert(nm_eat_stock(&s,1,0)==NM_LOCKED);
    nm_end_day(&s,0);assert(s.memory_day[NM_FIRST_PAY]==1);
    nm_init(&s,1,true,0);s.day=12;s.event=3;s.relations[0]=50;
    assert(nm_event_choose(&s,1,0)==NM_OK && s.relations[0]==60);
    assert((s.achievements&(1u<<NM_NEW_FRIEND)) && s.memory_day[NM_NEW_FRIEND]==12);
    nm_end_day(&s,0);assert(s.memory_day[NM_FIRST_PAY]==12);
    s.event=3;assert(nm_event_choose(&s,1,0)==NM_OK);
    assert(s.memory_day[NM_NEW_FRIEND]==12);
    puts("NiuMa events/inventory: PASS (24 choices, idempotence, paid stock, memories)");
}
