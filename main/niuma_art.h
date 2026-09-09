#pragma once
#include "niuma_model.h"
#include "niuma_game.h"

#define NM_ART_W 120
#define NM_ART_H 80
typedef enum { NM_SCENE_STAND, NM_SCENE_OFFICE, NM_SCENE_HOME, NM_SCENE_PARK,
    NM_SCENE_SLEEP, NM_SCENE_STUDY, NM_SCENE_EAT, NM_SCENE_FRIEND,
    NM_SCENE_LEAVE, NM_SCENE_TRAIN, NM_SCENE_PLANT, NM_SCENE_BENTO,
    NM_SCENE_CLEAN, NM_SCENE_TREAT, NM_SCENE_TOILET, NM_SCENE_DRINK,
    NM_SCENE_COOK, NM_SCENE_RICE, NM_SCENE_NOODLES, NM_SCENE_WATER,
    NM_SCENE_MILK_TEA, NM_SCENE_COUNT } nm_scene_t;
nm_scene_t nm_art_action_scene(nm_action_t action);
/* RGB565, host-endian, logical 120 x 80 (display at integer 2x).
   Writes only the supplied fixed-size buffer; no allocation or LVGL calls. */
void nm_art_render(uint16_t *pixels,const nm_state_t *s,nm_scene_t scene,unsigned frame);
void nm_art_game(uint16_t *pixels,const nm_state_t *s,const nm_game_t *game);
