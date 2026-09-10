#pragma once
#include <stdbool.h>
#include <stdint.h>

#define TK_WORDS 24
#define TK_TASKS 8
#define TK_SAVE_BYTES 64
typedef enum { TK_HOME, TK_ID, TK_CHAT, TK_TOOLS, TK_MAZE_MENU, TK_MAZE,
    TK_WORD_MENU, TK_WORD, TK_QUIZ, TK_PET, TK_SETTINGS, TK_TODOS, TK_ADD,
    TK_TASK, TK_DELETE, TK_MEETING, TK_COUNTDOWN, TK_REMINDER } tk_page;
typedef enum { TK_UP, TK_DOWN, TK_OK, TK_BACK } tk_key;
typedef struct { const char *en, *meaning, *example; } tk_word;
extern const tk_word tk_words[TK_WORDS];
extern const char *const tk_tasks[12];
extern const char *const tk_names[4];
typedef struct {
    uint32_t id, xp, learned, wrong;
    uint16_t coins;
    uint8_t name, avatar, volume, brightness, needs[4];
    uint8_t tasks[TK_TASKS], done, rewarded, count;
} tk_save;
typedef struct { uint32_t remaining, total; bool running; } tk_timer;
typedef struct {
    tk_save save;
    tk_page page;
    int selection, home_selection, task, word, word_mode, quiz_choice, quiz_answer;
    bool flipped, answered, quiz_correct, dirty;
    uint32_t random, seconds, pet_ticks, cooldown;
    tk_timer meeting, countdown, reminder;
    uint8_t maze[81], size, player, star, direction, difficulty;
    bool collected, won;
    uint16_t steps;
    uint8_t alert; /* Bitmask: meeting, countdown, reminder. Never overwrites another alert. */
    char notice[96];
    char chat_reply[96];
    uint8_t notice_ticks;
} tk_model;

void tk_init(tk_model *m, uint32_t seed);
void tk_keypress(tk_model *m, tk_key key);
void tk_tick(tk_model *m, uint32_t elapsed_seconds);
void tk_maze_start(tk_model *m, unsigned difficulty);
bool tk_maze_move(tk_model *m);
void tk_encode(const tk_save *s, uint8_t bytes[TK_SAVE_BYTES]);
bool tk_decode(tk_save *s, const uint8_t bytes[TK_SAVE_BYTES]);
int tk_options(const tk_model *m);
void tk_message(tk_model *m, const char *text);
