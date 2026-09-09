#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { NM_SFX_KEY, NM_SFX_COIN, NM_SFX_EAT, NM_SFX_ERROR,
    NM_SFX_REST, NM_SFX_WORK, NM_SFX_COUNT } nm_sfx_t;
typedef struct { uint32_t clock, phase, effect_phase, effect_sample; nm_sfx_t effect; bool active; } nm_synth_t;
void nm_synth_trigger(nm_synth_t *s,nm_sfx_t effect);
void nm_synth_render(nm_synth_t *s,int16_t *out,size_t count,bool music,bool muted);

/* Device service: requests never perform codec I/O in their caller. */
bool nm_audio_start(void);
bool nm_audio_ready(void);
void nm_audio_config(uint8_t volume,bool music,bool sleeping);
void nm_audio_play(nm_sfx_t effect);
