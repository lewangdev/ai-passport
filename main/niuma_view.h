#pragma once
#include "niuma_art.h"
#include "niuma_storage.h"

typedef enum { NM_VIEW_MENU, NM_VIEW_SCENE, NM_VIEW_HOME, NM_VIEW_GAME, NM_VIEW_TEXT, NM_VIEW_MEETING, NM_VIEW_ESCAPE } nm_view_kind_t;
typedef struct {
    nm_view_kind_t kind;
    nm_scene_t scene;
    char title[80],body[512],options[12][100],hint[100];
    uint8_t count,selected;
    bool preview;
    uint8_t preview_item;
} nm_view_t;
bool nm_view_init(void);
/* All calls must run in LVGL context or under the board's LVGL lock. */
void nm_view_draw(const nm_view_t *v,const nm_state_t *s,const nm_game_t *g,nm_store_status_t store,unsigned frame);
