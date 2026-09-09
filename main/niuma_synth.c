#include "niuma_sound.h"

static int triangle(uint32_t *phase,unsigned hz){
    *phase+=hz;
    if(*phase>=16000)*phase-=16000;
    int v=*phase<8000?(int)*phase:16000-(int)*phase;
    return v-4000;
}
void nm_synth_trigger(nm_synth_t *s,nm_sfx_t effect){
    if((unsigned)effect>=NM_SFX_COUNT)return;
    s->effect=effect;s->effect_phase=0;s->effect_sample=0;s->active=true;
}
void nm_synth_render(nm_synth_t *s,int16_t *out,size_t count,bool music,bool muted){
    /* Original 16-step melody, 400 ms per step; not sampled commercial music. */
    static const uint16_t notes[16]={262,330,392,330,294,220,294,349,262,330,440,392,349,294,262,0};
    static const uint16_t frequencies[NM_SFX_COUNT]={880,1047,330,147,262,659};
    static const uint16_t lengths[NM_SFX_COUNT]={640,3840,2240,2880,4800,960};
    for(size_t i=0;i<count;++i){
        int value=0;
        if(music && !muted){
            unsigned note=notes[(s->clock/6400)%16];
            unsigned t=s->clock%6400;
            int env=t<320?(int)t:t>4800?(6400-(int)t)/5:320;
            if(note)value=triangle(&s->phase,note)*env/1600;
            s->clock=(s->clock+1)%(6400*16);
        }
        if(s->active){
            unsigned length=lengths[s->effect],t=s->effect_sample;
            unsigned frequency=frequencies[s->effect];
            if(s->effect==NM_SFX_COIN && t>1600)frequency=1568;
            if(s->effect==NM_SFX_EAT)frequency+=(t/400)%2*110;
            if(s->effect==NM_SFX_REST)frequency=392-t*130/length;
            unsigned env=t<80?t*100/80:(length-t)*100/length;
            if(!muted)value+=triangle(&s->effect_phase,frequency)*(int)env/100;
            if(++s->effect_sample>=length)s->active=false;
        }
        out[i]=(int16_t)value; /* Maximum mix is below 8000, no clipping. */
    }
}
