#include "toolkit_model.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void maze_tests(void) {
    for (unsigned seed = 1; seed < 501; ++seed) for (unsigned difficulty = 0; difficulty < 3; ++difficulty) {
        tk_model m; tk_init(&m, seed); tk_maze_start(&m, difficulty);
        int n = m.size, queue[81], parent[81], head = 0, tail = 1, edges = 0;
        for (int i = 0; i < 81; ++i) parent[i] = -1;
        queue[0] = 0; parent[0] = 0;
        while (head < tail) {
            int c = queue[head++];
            for (int d = 0; d < 4; ++d) if (!(m.maze[c] & (1 << d))) {
                int next = c + (d == 0 ? -n : d == 1 ? 1 : d == 2 ? n : -1);
                assert(next >= 0 && next < n * n);
                assert(d != 1 || c % n < n - 1); assert(d != 3 || c % n > 0);
                assert(!(m.maze[next] & (1 << ((d + 2) % 4)))); ++edges;
                if (parent[next] < 0) { parent[next] = c; queue[tail++] = next; }
            }
        }
        assert(tail == n * n && edges == 2 * (n * n - 1));
        assert(m.star && m.star < n * n - 1);
        /* Walk every passage in both directions: movement cannot cross walls. */
        for (int c = 0; c < n * n; ++c) for (int d = 0; d < 4; ++d) {
            m.player = c; m.direction = d; m.won = m.collected = false;
            bool moved = tk_maze_move(&m); assert(moved == !(m.maze[c] & (1 << d)));
        }
        m.collected = true; m.won = false;
        int exit = n * n - 1;
        for (int d = 0; d < 4; ++d) if (!(m.maze[exit] & (1 << d))) {
            m.player = exit + (d == 0 ? -n : d == 1 ? 1 : d == 2 ? n : -1); m.direction = (d + 2) % 4;
            unsigned coins = m.save.coins; assert(tk_maze_move(&m) && m.won);
            assert(m.save.coins == coins + 8); assert(!tk_maze_move(&m)); break;
        }
    }
}
static void timers(void) {
    tk_model m; tk_init(&m, 1); m.page = TK_MEETING; tk_keypress(&m, TK_OK);
    assert(m.meeting.running && m.meeting.remaining == 900);
    tk_tick(&m, 100); tk_keypress(&m, TK_OK); tk_tick(&m, 500); assert(m.meeting.remaining == 800);
    tk_keypress(&m, TK_OK); tk_keypress(&m, TK_BACK); assert(m.page == TK_TOOLS);
    m.countdown = (tk_timer){800, 800, true}; m.reminder = m.countdown;
    tk_tick(&m, 801); assert(m.alert == 7 && !m.meeting.running);
    unsigned coins = m.save.coins; tk_tick(&m, 100000); assert(m.save.coins == coins);
    tk_keypress(&m, TK_OK); assert(!m.alert);
    for (int i = 0; i < 4; ++i) assert(m.save.needs[i] == 0);
}
static void persistence(void) {
    tk_model m; tk_init(&m, 45); uint8_t bytes[TK_SAVE_BYTES]; tk_save out;
    tk_encode(&m.save, bytes); assert(tk_decode(&out, bytes)); assert(out.id == m.save.id);
    for (int i = 0; i < TK_SAVE_BYTES; ++i) { bytes[i] ^= 1; assert(!tk_decode(&out, bytes)); bytes[i] ^= 1; }
    m.save.volume = 101; tk_encode(&m.save, bytes); assert(!tk_decode(&out, bytes));
    m.save.volume = 40; m.save.count = 9; tk_encode(&m.save, bytes); assert(!tk_decode(&out, bytes));
}
static void tasks_words_pet(void) {
    tk_model m; tk_init(&m, 4); m.page = TK_TASK; m.task = 0;
    tk_keypress(&m, TK_OK); unsigned coins = m.save.coins;
    tk_keypress(&m, TK_OK); tk_keypress(&m, TK_OK); assert(m.save.coins == coins);
    m.selection = 1; tk_keypress(&m, TK_OK); assert(m.page == TK_DELETE);
    m.selection = 1; tk_keypress(&m, TK_OK); assert(m.save.count == 1 && m.save.tasks[0] == 2 && m.save.done == 0);
    m.page = TK_ADD; m.selection = 5; for (int i = 0; i < 7; ++i) { m.page = TK_ADD; tk_keypress(&m, TK_OK); }
    assert(m.save.count == 8); m.page = TK_TODOS; m.selection = 8; tk_keypress(&m, TK_OK); assert(m.page == TK_TODOS);
    m.page = TK_WORD_MENU; m.selection = 2; tk_keypress(&m, TK_OK); assert(m.page == TK_WORD_MENU);
    m.selection = 1; tk_keypress(&m, TK_OK); assert(m.page == TK_QUIZ);
    m.selection = (m.quiz_answer + 1) % 3; tk_keypress(&m, TK_OK); assert(m.save.wrong & 1);
    m.page = TK_WORD_MENU; m.selection = 2; tk_keypress(&m, TK_OK); assert(m.word == 0 && m.page == TK_WORD);
    tk_keypress(&m, TK_OK); tk_keypress(&m, TK_OK); assert(m.page == TK_WORD_MENU && !m.save.wrong && m.save.learned == 1);
    m.page = TK_PET; m.selection = 0; coins = m.save.coins; tk_keypress(&m, TK_OK);
    assert(m.save.coins == coins - 5 && m.save.needs[0] == 100);
    tk_keypress(&m, TK_OK); assert(m.save.coins == coins - 5);
    /* Exercise navigation and check all persistent invariants across random input. */
    for (unsigned i = 0, random = 87; i < 100000; ++i) {
        random = random * 1664525u + 1013904223u; tk_keypress(&m, (tk_key)(random >> 29 & 3));
        if (i % 20 == 0) tk_tick(&m, 1);
        uint8_t bytes[TK_SAVE_BYTES]; tk_save out; tk_encode(&m.save, bytes); assert(tk_decode(&out, bytes));
        assert(m.selection >= 0 && m.selection < tk_options(&m));
    }
}
int main(void) { maze_tests(); timers(); persistence(); tasks_words_pet(); puts("Toolkit: maze, timers, storage, rewards and 100000 navigation events PASS"); }
