#include "dagou_model.h"
#include <assert.h>
#include <stdio.h>

int main(void) {
    dagou_config_t c;dagou_defaults(&c);assert(dagou_config_valid(&c));
    assert(dagou_midi(&c,0)==72);assert(dagou_midi(&c,2)==64);
    c.voice=0;c.tier=0;assert(dagou_midi(&c,0)==79);assert(dagou_midi(&c,1)==72);
    c.piano=1;
    const int scale[]={0,2,4,5,7,9,11,12};
    for(int oct=3;oct<=6;++oct)for(int note=0;note<8;++note){
        c.octave_enabled=1;c.octave=oct;c.note=note;
        assert(dagou_midi(&c,2)==(oct+1)*12+scale[note]);
        c.octave_enabled=0;assert(dagou_midi(&c,2)==60+scale[note]);
    }
    c.volume=101;assert(!dagou_config_valid(&c));dagou_defaults(&c);
    c.brightness=0;assert(!dagou_config_valid(&c));
    assert(dagou_wrap(-1,8)==7);assert(dagou_wrap(8,8)==0);
    for(uint32_t t=0;t<100000;t+=13){
        uint32_t q=dagou_next_step(t,true);
        assert(q>=t && q-t<3750 && q%3750==0);
        assert(dagou_next_step(t,false)==t);
    }
    puts("Dagou model tests: PASS");
}
