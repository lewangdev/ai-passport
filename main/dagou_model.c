#include "dagou_model.h"

void dagou_defaults(dagou_config_t *c) {
    *c = (dagou_config_t){.version=1, .voice=1, .octave=4, .snap=1,
        .music=1, .sound=1, .volume=55, .brightness=80, .tier=3};
}
bool dagou_config_valid(const dagou_config_t *c) {
    return c && c->version==1 && c->voice<2 && c->piano<2 &&
        c->octave_enabled<2 && c->octave>=3 && c->octave<=6 &&
        c->snap<2 && c->grid<2 && c->music<2 && c->sound<2 &&
        c->volume<=100 && c->brightness>=10 && c->brightness<=100 &&
        c->tier<4 && c->note<8 && c->syllable<3;
}
int dagou_wrap(int value, int count) {
    return count>0 ? ((value%count)+count)%count : 0;
}
int dagou_midi(const dagou_config_t *c, int syllable) {
    static const int scale[]={0,2,4,5,7,9,11,12};
    static const int pitches[2][3][4]={
        {{79,76,72,69},{72,69,67,64},{79,76,72,69}},
        {{81,79,76,72},{74,72,69,67},{72,69,67,64}}};
    if (!dagou_config_valid(c) || syllable<0 || syllable>2) return 60;
    if (c->piano) return ((c->octave_enabled?c->octave:4)+1)*12+scale[c->note];
    return pitches[c->voice][syllable][c->tier];
}
uint32_t dagou_next_step(uint32_t clock, bool snap) {
    /* 16 kHz, 128 BPM: exactly 3750 samples per eighth note. */
    return snap ? clock + ((3750-clock%3750)%3750) : clock;
}
