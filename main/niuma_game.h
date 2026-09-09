#pragma once
#include "niuma_model.h"

typedef enum { NM_GAME_TYPING, NM_GAME_MEETING, NM_GAME_COMMUTE,
    NM_GAME_TEA, NM_GAME_BENTO, NM_GAME_ESCAPE, NM_GAME_COUNT } nm_game_kind_t;
typedef struct {
    nm_game_kind_t kind;
    uint32_t rng, elapsed, round_ms, tea_ms;
    uint8_t lane, target, round, score, attempts, feedback, topic;
    uint8_t meeting_answers[3]; /* Successful nod / notes / question responses. */
    uint16_t lift_arrival_ms;
    uint8_t floors; /* Escape starts three floors above the exit. */
    bool lift_meeting;
    bool done, settled, busy, used;
} nm_game_t;

/* Caller pauses by not ticking: no clock, UI, sound or state persistence here. */
void nm_game_start(nm_game_t *g, nm_game_kind_t kind, uint32_t seed);
void nm_game_move(nm_game_t *g, int direction);
bool nm_game_press(nm_game_t *g);
void nm_game_tick(nm_game_t *g, uint32_t delta_ms);
bool nm_game_danger(const nm_game_t *g);
bool nm_game_warning(const nm_game_t *g);
bool nm_game_window(const nm_game_t *g);
uint32_t nm_game_round_duration(const nm_game_t *g);
bool nm_game_lift_ready(const nm_game_t *g);
/* Skip has zero extra reward but does not undo a paid base action. */
void nm_game_skip(nm_game_t *g);
bool nm_game_settle(nm_game_t *g, nm_state_t *s);
