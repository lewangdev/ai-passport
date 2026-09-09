#pragma once
#include "niuma_model.h"

typedef enum { NM_STORE_LOADING, NM_STORE_EMPTY, NM_STORE_SAVED, NM_STORE_ERROR } nm_store_phase_t;
typedef struct {
    nm_store_phase_t phase;
    uint32_t saved_revision;
    int battery;
    bool pending, recovered;
} nm_store_status_t;

/* Application-lifetime worker. Nonblocking requests copy the full snapshot.
   Start before UI; no worker owns or accesses any LVGL object.
   Submit/reload from the single UI owner. Request tickets distinguish pending
   work from an empty queue, including the announce-before-publish interval. */
bool nm_storage_start(void);
bool nm_storage_request(const nm_state_t *s);
nm_store_status_t nm_storage_status(void);
bool nm_storage_loaded(nm_state_t *s);
/* Read-only asynchronous startup retry. Never writes a default character. */
bool nm_storage_reload(void);
