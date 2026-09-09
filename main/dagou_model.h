#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t version, voice, piano, octave_enabled, octave, snap, grid;
    uint8_t music, sound, volume, brightness, tier, note, syllable;
} dagou_config_t;
void dagou_defaults(dagou_config_t *c);
bool dagou_config_valid(const dagou_config_t *c);
int dagou_midi(const dagou_config_t *c, int syllable);
uint32_t dagou_next_step(uint32_t sample_clock, bool snap);
int dagou_wrap(int value, int count);
