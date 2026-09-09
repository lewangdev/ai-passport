#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Pure game rules. No wall-clock decay: only accepted actions advance a day. */
typedef enum { NM_SATIETY, NM_ENERGY, NM_MOOD, NM_STRESS, NM_HEALTH, NM_STAT_COUNT } nm_stat_t;
typedef enum {
    NM_WORK, NM_OVERTIME, NM_REST, NM_SLACK, NM_STUDY_TECH,
    NM_STUDY_TALK, NM_STUDY_LIFE, NM_EXERCISE, NM_CLEAN, NM_TREAT,
    NM_RECOVER, NM_CHAT, NM_GIFT, NM_TOILET, NM_WATER, NM_BASIC_MEAL,
    NM_BENTO, NM_NOODLES, NM_COFFEE, NM_MILK_TEA, NM_COOK, NM_ACTION_COUNT
} nm_action_t;
typedef enum { NM_OK, NM_NO_ACTIONS, NM_NO_MONEY, NM_DAILY_LIMIT,
    NM_TOO_TIRED, NM_LOCKED, NM_BAD_INPUT } nm_error_t;
typedef enum { NM_ONTIME, NM_EXPERT, NM_SOCIAL, NM_SLACKER, NM_STRIVER, NM_LIFESTYLE } nm_style_t;
typedef enum { NM_STABLE, NM_PROJECT, NM_FLEX, NM_UNEMPLOYED } nm_company_t;
typedef enum { NM_ROUTE_EMPLOYEE, NM_ROUTE_EXPERT, NM_ROUTE_MANAGER,
    NM_ROUTE_FREELANCE, NM_ROUTE_SHOP, NM_ROUTE_COMFORT, NM_ROUTE_COUNT } nm_route_t;
typedef enum { NM_FIRST_PAY, NM_TEN_ONTIME, NM_FIRST_RAISE, NM_RESIGNED,
    NM_LIFE_MASTER, NM_PIE, NM_NEW_FRIEND, NM_FIRST_COOK, NM_ACHIEVEMENT_COUNT } nm_achievement_t;
typedef struct {
    const char *zh, *en;
    int8_t delta[NM_STAT_COUNT];
    uint8_t cost, slots;
} nm_action_info_t;
typedef struct {
    uint32_t day, rng, revision;
    uint32_t achievements;
    uint32_t coins, earned_total;
    uint16_t xp, skill[3], reputation, negotiation;
    uint16_t owned;                 /* Shop item bits, never random loot. */
    uint16_t ontime_days, working_days;
    uint8_t stat[NM_STAT_COUNT], relations[3];
    uint8_t gender, name, company, rank, salary;
    uint8_t actions, performance, overtime, coffees, waters, toilets;
    uint8_t mess, noodle_days, bad_days, good_days, burnout, review_done;
    uint8_t event_done, event, equipped[3], meals;
    uint8_t style_history[14][6], history_cursor;
    uint8_t volume, brightness, idle_seconds, key_sound, music, english;
    uint16_t today_spent, today_bonus;
    uint32_t last_review_day;
    uint32_t memory_day[NM_ACHIEVEMENT_COUNT];
    uint8_t food_stock[6];          /* Basic meal, bento, noodles, coffee, tea, cooked. */
    uint8_t evening;               /* Wages settled; home actions remain available. */
    uint16_t paid_salary, paid_target_bonus;
    uint8_t route;
    uint32_t route_day[NM_ROUTE_COUNT-1]; /* First entry, retained across careers. */
} nm_state_t;
typedef struct {
    int16_t delta[NM_STAT_COUNT];
    int32_t coins;
    uint16_t experience;
    uint16_t skill[3];
    uint8_t relations[3];
    uint8_t slots;
} nm_effect_t;
typedef struct {
    uint16_t salary, target_bonus, game_bonus, expenses;
    int32_t net;
    bool weekend, burnout_started, burnout_ended;
} nm_receipt_t;

extern const nm_action_info_t nm_actions[NM_ACTION_COUNT];
void nm_init(nm_state_t *s, uint32_t seed, bool female, uint8_t name);
bool nm_valid(const nm_state_t *s);
bool nm_weekend(const nm_state_t *s);
nm_style_t nm_style(const nm_state_t *s);
nm_error_t nm_can_act(const nm_state_t *s, nm_action_t action, unsigned friend_index);
nm_error_t nm_act(nm_state_t *s, nm_action_t action, unsigned friend_index, nm_effect_t *effect);
void nm_end_day(nm_state_t *s, nm_receipt_t *receipt);
void nm_close_work(nm_state_t *s, nm_receipt_t *receipt);
void nm_game_reward(nm_state_t *s, unsigned score, unsigned maximum);
nm_error_t nm_buy(nm_state_t *s, unsigned item);
nm_error_t nm_equip(nm_state_t *s, unsigned item);
nm_error_t nm_resign(nm_state_t *s);
nm_error_t nm_join(nm_state_t *s, nm_company_t company);
bool nm_review_available(const nm_state_t *s);
unsigned nm_review_wait_days(const nm_state_t *s);
nm_error_t nm_negotiate(nm_state_t *s, bool *raised);
uint32_t nm_random(nm_state_t *s);
nm_error_t nm_stock_food(nm_state_t *s, unsigned food);
nm_error_t nm_eat_stock(nm_state_t *s, unsigned food, nm_effect_t *effect);
nm_error_t nm_route_available(const nm_state_t *s, nm_route_t route);
nm_error_t nm_choose_route(nm_state_t *s, nm_route_t route);
unsigned nm_item_price(unsigned item);
/* Caller owns the transaction/revision; shared by actions and event choices. */
void nm_check_friendship(nm_state_t *s);
