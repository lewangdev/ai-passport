#include "toolkit_model.h"
#include <stdio.h>
#include <string.h>

const char *const tk_names[4] = {"FoloToy", "小小探险家", "绿色闪电", "学习搭子"};
const char *const tk_tasks[12] = {"喝一杯水", "整理桌面", "背五个单词", "出门走走", "读十页书", "完成作业", "回复邮件", "准备会议", "整理会议纪要", "跟进下一步", "锻炼十分钟", "早点休息"};
const tk_word tk_words[TK_WORDS] = {
    {"apple", "苹果", "An apple a day."}, {"book", "书", "Read a good book."},
    {"cat", "猫", "The cat is sleeping."}, {"dog", "狗", "I like my dog."},
    {"water", "水", "Drink some water."}, {"sun", "太阳", "The sun is bright."},
    {"moon", "月亮", "Look at the moon."}, {"star", "星星", "A star in the sky."},
    {"tree", "树", "A tall green tree."}, {"flower", "花", "This flower is red."},
    {"house", "房子", "This is my house."}, {"school", "学校", "We walk to school."},
    {"friend", "朋友", "You are my friend."}, {"happy", "开心", "I feel happy today."},
    {"learn", "学习", "Learn something new."}, {"play", "玩耍", "Time to play!"},
    {"work", "工作", "We work together."}, {"rest", "休息", "Take a short rest."},
    {"green", "绿色", "The leaf is green."}, {"small", "小的", "A small gift for you."},
    {"dream", "梦想", "Follow your dream."}, {"music", "音乐", "I enjoy music."},
    {"coffee", "咖啡", "A cup of coffee."}, {"time", "时间", "Use your time well."}
};
static uint32_t rng(tk_model *m) {
    uint32_t x = m->random; x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return m->random = x ? x : 1;
}
static void reward(tk_model *m, unsigned xp, unsigned coins) {
    m->save.xp = m->save.xp > 999999 - xp ? 999999 : m->save.xp + xp;
    m->save.coins = m->save.coins > 9999 - coins ? 9999 : m->save.coins + coins;
    m->dirty = true;
}
void tk_message(tk_model *m, const char *text) {
    snprintf(m->notice, sizeof(m->notice), "%s", text); m->notice_ticks = 3;
}
void tk_init(tk_model *m, uint32_t seed) {
    memset(m, 0, sizeof(*m)); m->random = seed ? seed : 1;
    m->save.id = rng(m); m->save.coins = 30; m->save.volume = 40;
    m->save.brightness = 70; memset(m->save.needs, 80, 4);
    m->save.tasks[0] = 0; m->save.tasks[1] = 2; m->save.count = 2;
    snprintf(m->chat_reply, sizeof(m->chat_reply), "我是你的随身搭子\n选个话题聊聊吧");
}
/* Perfect maze: reciprocal wall bits N/E/S/W, bounded iterative DFS. */
static int neighbor(int cell, int direction, int n) {
    int x = cell % n, y = cell / n;
    if (direction == 0) return y ? cell - n : -1;
    if (direction == 1) return x + 1 < n ? cell + 1 : -1;
    if (direction == 2) return y + 1 < n ? cell + n : -1;
    return x ? cell - 1 : -1;
}
void tk_maze_start(tk_model *m, unsigned difficulty) {
    m->difficulty = difficulty > 2 ? 2 : difficulty; m->size = 5 + 2 * m->difficulty;
    uint8_t stack[81], visited[81] = {0}; int depth = 1;
    memset(m->maze, 15, sizeof(m->maze)); stack[0] = 0; visited[0] = 1;
    while (depth) {
        int c = stack[depth - 1], choices[4], count = 0;
        for (int d = 0; d < 4; ++d) {
            int next = neighbor(c, d, m->size);
            if (next >= 0 && !visited[next]) choices[count++] = d;
        }
        if (!count) { --depth; continue; }
        int d = choices[rng(m) % count], next = neighbor(c, d, m->size);
        m->maze[c] &= ~(1 << d); m->maze[next] &= ~(1 << ((d + 2) % 4));
        visited[next] = 1; stack[depth++] = next;
    }
    m->star = 1 + rng(m) % (m->size * m->size - 2);
    m->player = 0; m->direction = 1; m->steps = 0; m->collected = m->won = false;
    m->page = TK_MAZE; m->selection = 0;
}
bool tk_maze_move(tk_model *m) {
    if (!m->size || m->won || (m->maze[m->player] & (1 << m->direction))) return false;
    int next = neighbor(m->player, m->direction, m->size);
    if (next < 0) return false;
    m->player = next; if (m->steps < 65535) ++m->steps;
    if (m->player == m->star) m->collected = true;
    if (m->player == m->size * m->size - 1) {
        if (m->collected) { m->won = true; reward(m, 15 + 5 * m->difficulty, 8); tk_message(m, "过关啦！金币 +8"); }
        else tk_message(m, "先找到星星再来吧");
    }
    return true;
}
static void timer_start(tk_timer *t, unsigned minutes) {
    t->remaining = t->total = minutes * 60; t->running = true;
}
static bool timer_tick(tk_timer *t, uint32_t elapsed) {
    if (!t->running) return false;
    if (elapsed < t->remaining) { t->remaining -= elapsed; return false; }
    t->remaining = 0; t->running = false; return true;
}
void tk_tick(tk_model *m, uint32_t elapsed) {
    m->seconds += elapsed;
    if (timer_tick(&m->meeting, elapsed)) { m->alert |= 1; reward(m, 10, 5); }
    if (timer_tick(&m->countdown, elapsed)) m->alert |= 2;
    if (timer_tick(&m->reminder, elapsed)) m->alert |= 4;
    m->cooldown = elapsed >= m->cooldown ? 0 : m->cooldown - elapsed;
    m->notice_ticks = elapsed >= m->notice_ticks ? 0 : m->notice_ticks - elapsed;
    uint64_t ticks = (uint64_t)m->pet_ticks + elapsed;
    unsigned decay = ticks / 180 > 100 ? 100 : ticks / 180;
    m->pet_ticks = ticks % 180;
    if (decay) {
        for (int i = 0; i < 4; ++i) m->save.needs[i] = decay >= m->save.needs[i] ? 0 : m->save.needs[i] - decay;
        m->dirty = true;
    }
}
int tk_options(const tk_model *m) {
    switch (m->page) {
    case TK_HOME: return 6;
    case TK_ID: return 2;
    case TK_CHAT: case TK_PET: case TK_TOOLS: return 4;
    case TK_SETTINGS: return 2;
    case TK_TODOS: return m->save.count + 1;
    case TK_ADD: return 12;
    case TK_TASK: case TK_DELETE: return 2;
    case TK_MAZE_MENU: return m->size && !m->won ? 4 : 3;
    case TK_WORD_MENU: return 3;
    case TK_WORD: return m->flipped ? 2 : 1;
    case TK_QUIZ: return 3;
    case TK_MEETING: return m->meeting.total ? 3 : 3;
    case TK_COUNTDOWN: return m->countdown.total ? 2 : 4;
    case TK_REMINDER: return m->reminder.total ? 1 : 4;
    default: return 1;
    }
}
static void page(tk_model *m, tk_page p) { m->page = p; m->selection = 0; }
static bool next_word(tk_model *m) {
    for (int i = 0; i < TK_WORDS; ++i) {
        m->word = (m->word + 1) % TK_WORDS;
        if (m->word_mode != 2 || (m->save.wrong & (1u << m->word))) {
            m->flipped = m->answered = false; m->selection = 0;
            m->quiz_answer = rng(m) % 3; return true;
        }
    }
    page(m, TK_WORD_MENU); tk_message(m, "没有错词，真棒！"); return false;
}
static void learned(tk_model *m) {
    uint32_t bit = 1u << m->word;
    if (!(m->save.learned & bit)) reward(m, 5, 2);
    m->save.learned |= bit; m->save.wrong &= ~bit; m->dirty = true;
}
void tk_keypress(tk_model *m, tk_key key) {
    if (m->alert) { if (key == TK_OK || key == TK_BACK) m->alert = 0; return; }
    if (key == TK_BACK) {
        switch (m->page) {
        case TK_HOME: page(m, TK_SETTINGS); break;
        case TK_MEETING: case TK_COUNTDOWN: case TK_REMINDER: case TK_TODOS: page(m, TK_TOOLS); break;
        case TK_ADD: case TK_TASK: page(m, TK_TODOS); break;
        case TK_DELETE: page(m, TK_TASK); break;
        case TK_MAZE: page(m, TK_MAZE_MENU); break;
        case TK_WORD: case TK_QUIZ: page(m, TK_WORD_MENU); break;
        default: page(m, TK_HOME); m->selection = m->home_selection; break;
        }
        return;
    }
    if (m->page == TK_MAZE) {
        if (key == TK_OK) { if (m->won) page(m, TK_MAZE_MENU); else if (!tk_maze_move(m)) tk_message(m, "前面是墙，转个方向"); }
        else m->direction = (m->direction + (key == TK_UP ? 3 : 1)) % 4;
        return;
    }
    if (key != TK_OK) {
        m->selection = (m->selection + tk_options(m) + (key == TK_UP ? -1 : 1)) % tk_options(m);
        return;
    }
    int s = m->selection;
    switch (m->page) {
    case TK_HOME: {
        const tk_page targets[] = {TK_ID, TK_CHAT, TK_TOOLS, TK_MAZE_MENU, TK_WORD_MENU, TK_PET};
        m->home_selection = s; page(m, targets[s]); break;
    }
    case TK_ID:
        if (!s) m->save.name = (m->save.name + 1) % 4; else m->save.avatar = (m->save.avatar + 1) % 3;
        m->dirty = true; break;
    case TK_CHAT:
        if (!s) tk_message(m, "你好！一起探索今天吧");
        if (s == 1) {
            unsigned remaining = 0;
            for (int i = 0; i < m->save.count; ++i) if (!(m->save.done & (1 << i))) ++remaining;
            snprintf(m->notice, sizeof(m->notice), "还有 %u 项待办没完成", remaining);
        }
        if (s == 2) snprintf(m->notice, sizeof(m->notice), "宠物饱腹值 %u / 100", m->save.needs[0]), m->notice_ticks = 3;
        if (s == 3) tk_message(m, "走走、喝水，再学一个词");
        snprintf(m->chat_reply, sizeof(m->chat_reply), "%s", m->notice); m->notice_ticks = 0;
        break;
    case TK_TOOLS: { const tk_page targets[] = {TK_MEETING, TK_TODOS, TK_COUNTDOWN, TK_REMINDER}; page(m, targets[s]); break; }
    case TK_TODOS:
        if (s == m->save.count) {
            if (m->save.count == TK_TASKS) tk_message(m, "清单已满，先删除一项"); else page(m, TK_ADD);
        } else { m->task = s; page(m, TK_TASK); }
        break;
    case TK_ADD:
        if (m->save.count < TK_TASKS) { m->save.tasks[m->save.count++] = s; m->dirty = true; page(m, TK_TODOS); tk_message(m, "已添加待办"); }
        break;
    case TK_TASK:
        if (s) page(m, TK_DELETE);
        else {
            uint8_t bit = 1u << m->task; m->save.done ^= bit;
            if ((m->save.done & bit) && !(m->save.rewarded & bit)) { reward(m, 5, 2); m->save.rewarded |= bit; }
            m->dirty = true;
        }
        break;
    case TK_DELETE:
        if (s) {
            int i = m->task;
            memmove(&m->save.tasks[i], &m->save.tasks[i + 1], m->save.count - i - 1);
            unsigned mask = (1u << i) - 1;
            m->save.done = (m->save.done & mask) | ((m->save.done >> 1) & ~mask);
            m->save.rewarded = (m->save.rewarded & mask) | ((m->save.rewarded >> 1) & ~mask);
            m->save.tasks[--m->save.count] = 0; m->dirty = true;
        }
        page(m, TK_TODOS); break;
    case TK_MEETING:
        if (!m->meeting.total) { const unsigned mins[] = {15, 25, 45}; timer_start(&m->meeting, mins[s]); }
        else if (!s) { if (m->meeting.remaining) m->meeting.running = !m->meeting.running; }
        else if (s == 1) {
            if (m->save.count == TK_TASKS) tk_message(m, "待办清单已满");
            else { m->save.tasks[m->save.count++] = 9; m->dirty = true; tk_message(m, "跟进事项已加入待办"); }
        } else { memset(&m->meeting, 0, sizeof(m->meeting)); m->selection = 0; }
        break;
    case TK_COUNTDOWN:
        if (!m->countdown.total) { const unsigned mins[] = {5, 10, 25, 45}; timer_start(&m->countdown, mins[s]); m->selection = 0; }
        else if (!s) { if (m->countdown.remaining) m->countdown.running = !m->countdown.running; }
        else { memset(&m->countdown, 0, sizeof(m->countdown)); m->selection = 0; }
        break;
    case TK_REMINDER:
        if (!m->reminder.total) { const unsigned mins[] = {5, 15, 30, 60}; timer_start(&m->reminder, mins[s]); m->selection = 0; }
        else memset(&m->reminder, 0, sizeof(m->reminder));
        break;
    case TK_MAZE_MENU: if (s == 3) page(m, TK_MAZE); else tk_maze_start(m, s); break;
    case TK_WORD_MENU:
        m->word_mode = s; m->word = -1; page(m, s == 1 ? TK_QUIZ : TK_WORD); next_word(m); break;
    case TK_WORD:
        if (!m->flipped) { m->flipped = true; m->selection = 0; }
        else { if (!s) learned(m); else { m->save.wrong |= 1u << m->word; m->dirty = true; } next_word(m); }
        break;
    case TK_QUIZ:
        if (m->answered) next_word(m);
        else {
            m->answered = true; m->quiz_correct = s == m->quiz_answer;
            if (m->quiz_correct) { learned(m); tk_message(m, "答对啦！"); }
            else { m->save.wrong |= 1u << m->word; m->dirty = true; tk_message(m, "记住答案，再试一次"); }
        }
        break;
    case TK_PET:
        if (m->cooldown) { tk_message(m, "搭子还在享受，等一会"); break; }
        if (m->save.needs[s] >= 100) { tk_message(m, "这个状态已经满啦"); break; }
        if (!s && m->save.coins < 5) { tk_message(m, "金币不足，去学习或闯关"); break; }
        if (s == 1 && m->save.needs[2] < 10) { tk_message(m, "太困了，先休息一下"); break; }
        if (!s) m->save.coins -= 5;
        if (s == 1) m->save.needs[2] -= 10;
        m->save.needs[s] = m->save.needs[s] > 70 ? 100 : m->save.needs[s] + 30;
        reward(m, 2, 0); m->cooldown = 10; tk_message(m, "搭子很开心！成长 +2"); break;
    case TK_SETTINGS:
        if (!s) m->save.volume = (m->save.volume + 20) % 120;
        else m->save.brightness = m->save.brightness >= 100 ? 20 : m->save.brightness + 10;
        m->dirty = true; break;
    default: break;
    }
}
/* Canonical, fixed-size encoding: no compiler padding, pointers, or raw device identity. */
static void put32(uint8_t *p, uint32_t n) { for (int i = 0; i < 4; ++i) p[i] = n >> (8 * i); }
static uint32_t get32(const uint8_t *p) { uint32_t n = 0; for (int i = 0; i < 4; ++i) n |= (uint32_t)p[i] << (8 * i); return n; }
static uint32_t checksum(const uint8_t *p) { uint32_t h = 2166136261u; for (int i = 0; i < 60; ++i) h = (h ^ p[i]) * 16777619u; return h; }
void tk_encode(const tk_save *s, uint8_t b[TK_SAVE_BYTES]) {
    memset(b, 0, TK_SAVE_BYTES); memcpy(b, "TK01", 4);
    put32(b + 4, s->id); put32(b + 8, s->xp); put32(b + 12, s->learned); put32(b + 16, s->wrong);
    b[20] = s->coins; b[21] = s->coins >> 8; b[22] = s->name; b[23] = s->avatar;
    b[24] = s->volume; b[25] = s->brightness; memcpy(b + 26, s->needs, 4);
    memcpy(b + 30, s->tasks, TK_TASKS); b[38] = s->done; b[39] = s->rewarded; b[40] = s->count;
    put32(b + 60, checksum(b));
}
bool tk_decode(tk_save *s, const uint8_t b[TK_SAVE_BYTES]) {
    if (memcmp(b, "TK01", 4) || get32(b + 60) != checksum(b)) return false;
    tk_save t = {0}; t.id = get32(b + 4); t.xp = get32(b + 8); t.learned = get32(b + 12); t.wrong = get32(b + 16);
    t.coins = b[20] | ((uint16_t)b[21] << 8); t.name = b[22]; t.avatar = b[23];
    t.volume = b[24]; t.brightness = b[25]; memcpy(t.needs, b + 26, 4); memcpy(t.tasks, b + 30, TK_TASKS);
    t.done = b[38]; t.rewarded = b[39]; t.count = b[40];
    if (t.xp > 999999 || t.coins > 9999 || t.name >= 4 || t.avatar >= 3 || t.volume > 100 || t.brightness < 20 || t.brightness > 100 || t.count > TK_TASKS || (t.learned >> TK_WORDS) || (t.wrong >> TK_WORDS)) return false;
    for (int i = 0; i < 4; ++i) if (t.needs[i] > 100) return false;
    for (int i = 0; i < TK_TASKS; ++i) if (t.tasks[i] >= 12) return false;
    if ((t.done >> t.count) || (t.rewarded >> t.count)) return false;
    *s = t; return true;
}
