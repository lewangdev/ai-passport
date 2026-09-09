#include "niuma_save.h"
#include <string.h>

typedef struct { uint8_t *p; size_t offset, size; bool ok, read; } codec_t;
static uint32_t value(codec_t *c, uint32_t v, unsigned width) {
    if(c->offset+width>c->size){c->ok=false;return 0;}
    uint32_t result=0;
    for(unsigned i=0;i<width;++i){
        if(c->read) result|=(uint32_t)c->p[c->offset++]<<(8*i);
        else c->p[c->offset++]=(uint8_t)(v>>(8*i));
    }
    return c->read?result:v;
}
static uint32_t checksum(const uint8_t *p,size_t n){
    uint32_t crc=UINT32_MAX;
    for(size_t j=0;j<n;++j){crc^=p[j];for(unsigned i=0;i<8;++i)crc=(crc>>1)^((crc&1)?UINT32_C(0xedb88320):0);}
    return ~crc;
}
static void fields(codec_t *c,nm_state_t *s){
#define F(name,width) s->name=value(c,s->name,width)
    F(day,4); F(rng,4); F(revision,4); F(achievements,4); F(coins,4); F(earned_total,4);
    F(xp,2); for(unsigned i=0;i<3;++i){F(skill[i],2);} F(reputation,2); F(negotiation,2);
    F(owned,2); F(ontime_days,2); F(working_days,2);
    for(unsigned i=0;i<NM_STAT_COUNT;++i){F(stat[i],1);}
    for(unsigned i=0;i<3;++i){F(relations[i],1);}
    F(gender,1); F(name,1); F(company,1); F(rank,1); F(salary,1);
    F(actions,1); F(performance,1); F(overtime,1); F(coffees,1); F(waters,1); F(toilets,1);
    F(mess,1); F(noodle_days,1); F(bad_days,1); F(good_days,1); F(burnout,1); F(review_done,1);
    F(event_done,1); F(event,1); for(unsigned i=0;i<3;++i){F(equipped[i],1);} F(meals,1);
    for(unsigned d=0;d<14;++d) for(unsigned i=0;i<6;++i){F(style_history[d][i],1);}
    F(history_cursor,1); F(volume,1); F(brightness,1); F(idle_seconds,1);
    F(key_sound,1); F(music,1); F(english,1); F(today_spent,2); F(today_bonus,2); F(last_review_day,4);
    for(unsigned i=0;i<NM_ACHIEVEMENT_COUNT;++i){F(memory_day[i],4);}
    for(unsigned i=0;i<6;++i){F(food_stock[i],1);}
    F(evening,1); F(paid_salary,2); F(paid_target_bonus,2);
    F(route,1); for(unsigned i=0;i<NM_ROUTE_COUNT-1;++i){F(route_day[i],4);}
#undef F
}
size_t nm_save_encode(const nm_state_t *s,uint8_t *out,size_t capacity){
    if(!out || capacity<8 || !nm_valid(s)) return 0;
    nm_state_t copy=*s;
    codec_t c={out,0,capacity,true,false};
    value(&c,UINT32_C(0x34414d4e),4); /* NMA4: career directions and dated milestones. */
    fields(&c,&copy);
    if(!c.ok || c.offset+4>capacity) return 0;
    uint32_t crc=checksum(out,c.offset);value(&c,crc,4);
    return c.ok?c.offset:0;
}
bool nm_save_decode(nm_state_t *s,const uint8_t *data,size_t length){
    if(!s || !data || length<8 || length>NM_SAVE_CAPACITY) return false;
    /* Reader never modifies the source; value() writes only when read=false. */
    codec_t c={(uint8_t *)data,0,length,true,true};
    if(value(&c,0,4)!=UINT32_C(0x34414d4e)) return false;
    nm_state_t candidate={0};fields(&c,&candidate);
    if(!c.ok || c.offset+4!=length) return false;
    uint32_t expected=checksum(data,c.offset);
    if(value(&c,0,4)!=expected || !nm_valid(&candidate)) return false;
    *s=candidate;return true;
}
