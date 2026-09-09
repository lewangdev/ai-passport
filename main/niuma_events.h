#pragma once
#include "niuma_model.h"

typedef struct {
    const char *zh, *en, *body_zh, *body_en;
    const char *choice_zh[3], *choice_en[3];
} nm_event_info_t;
extern const nm_event_info_t nm_events[8];
/* Choices are transactional and at most one event is resolved per day. */
nm_error_t nm_event_choose(nm_state_t *s,unsigned choice,nm_effect_t *effect);
