#include "niuma_save.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(void){
    nm_state_t a,b,before;uint8_t wire[NM_SAVE_CAPACITY],again[NM_SAVE_CAPACITY];
    nm_init(&a,32,1,3);nm_act(&a,NM_WORK,0,0);nm_act(&a,NM_CHAT,2,0);
    a.music=1;a.english=1;a.volume=70;
    a.route=NM_ROUTE_COMFORT;a.route_day[NM_ROUTE_COMFORT-1]=1;
    nm_close_work(&a,0);
    size_t n=nm_save_encode(&a,wire,sizeof(wire));assert(n>100 && n<=sizeof(wire));
    memset(&b,0,sizeof(b));assert(nm_save_decode(&b,wire,n));
    assert(nm_save_encode(&b,again,sizeof(again))==n && memcmp(wire,again,n)==0);
    assert(b.xp==a.xp && b.gender==1 && b.music==1 && b.relations[2]==a.relations[2]);
    assert(b.evening && b.paid_salary==20 && b.day==1);
    assert(b.route==NM_ROUTE_COMFORT && b.route_day[NM_ROUTE_COMFORT-1]==1);
    nm_state_t restored=b; nm_close_work(&restored,0);
    assert(memcmp(&restored,&b,sizeof(b))==0);
    before=b;
    for(size_t i=0;i<n;++i){
        wire[i]^=1;assert(!nm_save_decode(&b,wire,n));
        assert(memcmp(&b,&before,sizeof(b))==0);wire[i]^=1;
    }
    for(size_t length=0;length<n;++length)assert(!nm_save_decode(&b,wire,length));
    assert(!nm_save_decode(&b,wire,n+1));
    assert(nm_save_encode(&a,again,n-1)==0);
    a.actions=7;assert(nm_save_encode(&a,again,sizeof(again))==0);
    printf("NiuMa save: PASS (%zu bytes, CRC, truncation, roundtrip)\n",n);
}
