#include "niuma_sound.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(void){
    for(unsigned e=0;e<NM_SFX_COUNT;++e){
        nm_synth_t s={0};int16_t out[256];bool audible=false;
        nm_synth_trigger(&s,(nm_sfx_t)e);
        for(unsigned b=0;b<32;++b){nm_synth_render(&s,out,256,true,false);for(unsigned i=0;i<256;++i){if(out[i])audible=true;assert(abs(out[i])<8000);}}
        assert(audible && !s.active);
        nm_synth_trigger(&s,(nm_sfx_t)e);nm_synth_render(&s,out,256,true,true);
        for(unsigned i=0;i<256;++i)assert(!out[i]);
    }
    nm_synth_t a={0},b={0};int16_t full[1024],part[1024];
    nm_synth_trigger(&a,NM_SFX_COIN);b=a;
    nm_synth_render(&a,full,1024,true,false);
    for(unsigned i=0;i<4;++i)nm_synth_render(&b,part+256*i,256,true,false);
    assert(!memcmp(full,part,sizeof(full)));
    puts("NiuMa sound: PASS (six effects, silent mode, mix bounds, chunk invariance)");
}
