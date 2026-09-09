#pragma once
#include "dagou_model.h"
#include <stdbool.h>
#include <stdint.h>
bool dagou_audio_start(dagou_config_t *config);
void dagou_audio_config(const dagou_config_t *config, bool save);
bool dagou_audio_press(int syllable, int midi, unsigned id);
void dagou_audio_release(unsigned id);
unsigned dagou_audio_hit(void);
uint32_t dagou_audio_clock(void);
int dagou_audio_battery(void);
int dagou_audio_status(void);
bool dagou_audio_saved(void);
