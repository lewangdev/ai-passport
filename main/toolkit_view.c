#include "toolkit_view.h"
#include <stdio.h>
#ifdef TK_RENDER_TEST
#include <assert.h>
#include <stdlib.h>
#endif

LV_FONT_DECLARE(toolkit_font_16);
#define BG 0x080F17
#define PANEL 0x13212C
#define GREEN 0x55E599
#define MUTED 0x96ACB3
#define WHITE 0xEDF6EF
#define GOLD 0xE7C96B
static const tk_model *m;
static const tk_status *status;
static lv_layer_t *layer;
/* Original hand-authored 16 px silhouettes. Colors are supplied at draw time. */
static const uint16_t icons[11][16] = {
 {0,0x7ffe,0x4002,0x4c02,0x4cfa,0x4002,0x5efa,0x5e02,0x5efa,0x4002,0x4002,0x7ffe,0,0,0,0},
 {0,0x3ffc,0x4002,0x4002,0x4992,0x4992,0x4002,0x4002,0x3ff4,0x0018,0x0030,0,0,0,0,0},
 {0,0x0ff0,0x0810,0x7ffe,0x4002,0x4002,0x7ffe,0x4242,0x43c2,0x4002,0x4002,0x7ffe,0,0,0,0},
 {0,0x1ff8,0x2004,0x4422,0x4e12,0x4422,0x4002,0x4182,0x2004,0x27e4,0x1818,0,0,0,0,0},
 {0,0x7c3e,0x4242,0x4242,0x4242,0x4242,0x4242,0x4242,0x4242,0x4242,0x7c3e,0x03c0,0,0,0,0},
 {0,0x300c,0x381c,0x1ff8,0x3ffc,0x6666,0x6666,0x7ffe,0x3e7c,0x3ffc,0x1ff8,0x1818,0,0,0,0},
 {0,0x0180,0x03c0,0x03c0,0x1ff8,0x0ff0,0x07e0,0x0ff0,0x0c30,0x0810,0,0,0,0,0,0},
 {0,0x0c30,0x1e78,0x3ffc,0x3ffc,0x1ff8,0x0ff0,0x07e0,0x03c0,0x0180,0,0,0,0,0,0},
 {0,0x3ff8,0x2008,0x201e,0x2012,0x2012,0x201e,0x2008,0x1ff0,0,0x3ffc,0,0,0,0,0},
 {0,0x03c0,0x0810,0x1008,0x2184,0x2184,0x2004,0x201c,0x1008,0x0810,0x07e0,0,0,0,0,0},
 {0x0180,0x0380,0x0700,0x1ef0,0x3ff8,0x7ffc,0x7ffc,0x7ffc,0x7ffc,0x7ffc,0x3ff8,0x1ef0,0,0,0,0}
};
static const char *const avatar[20] = {
 ".....HHHHH......", "....HHHHHHHH....", "...HHHHHHHHHH...", "...HHSHHSHHHH...",
 "...HSSSSSSSSH...", "...SSWSSSWSSS...", "...SSISSSISSS...", "...SSSSSSSSSS...",
 "....SSSIISSS....", ".....SSSSSS.....", "....JJWWJJJ.....", "...JJJWIWJJJ....",
 "...SJJIWIJJS....", "...SJJJIJJJS....", "....JJJJJJJ.....", "....JJJJJJJ.....",
 "....JJJ.JJJ.....", "....JJJ.JJJ.....", "...IIII.IIII....", "................"
};
static void box(int x, int y, int w, int h, uint32_t color, int radius, uint32_t border) {
    lv_area_t a = {x, y, x + w - 1, y + h - 1};
    if (w <= 0 || h <= 0 || a.y2 < layer->_clip_area.y1 || a.y1 > layer->_clip_area.y2) return;
    lv_draw_rect_dsc_t d; lv_draw_rect_dsc_init(&d); d.bg_color = lv_color_hex(color);
    d.radius = radius; d.border_width = border ? 1 : 0; d.border_color = lv_color_hex(border);
    lv_draw_rect(layer, &d, &a);
}
static void text(int x, int y, int w, int h, const char *s, uint32_t color, bool big, bool center) {
    lv_area_t a = {x, y, x + w - 1, y + h - 1};
    if (a.y2 < layer->_clip_area.y1 || a.y1 > layer->_clip_area.y2) return;
    lv_draw_label_dsc_t d; lv_draw_label_dsc_init(&d); d.text = s; d.text_local = 1;
    d.color = lv_color_hex(color); d.font = big ? &lv_font_montserrat_20 : &toolkit_font_16;
    d.line_space = 4; d.align = center ? LV_TEXT_ALIGN_CENTER : LV_TEXT_ALIGN_LEFT;
#ifdef TK_RENDER_TEST
    const unsigned char *p = (const unsigned char *)s;
    while (*p) {
        uint32_t cp = *p++; unsigned extra = 0;
        if (cp >= 0xf0) { cp &= 7; extra = 3; }
        else if (cp >= 0xe0) { cp &= 15; extra = 2; }
        else if (cp >= 0xc0) { cp &= 31; extra = 1; }
        while (extra--) { assert((*p & 0xc0) == 0x80); cp = (cp << 6) | (*p++ & 63); }
        if (cp < 32) continue;
        lv_font_glyph_dsc_t glyph;
        assert(lv_font_get_glyph_dsc(d.font, &glyph, cp, 0) && !glyph.is_placeholder);
    }
    lv_point_t measured; lv_text_get_size(&measured, s, d.font, 0, 4, w, LV_TEXT_FLAG_NONE);
    if (measured.y > h) { fprintf(stderr, "Layout page %d: needs %ld, has %d: %s\n", m->page, (long)measured.y, h, s); abort(); }
#endif
    lv_draw_label(layer, &d, &a);
}
static void icon(int type, int x, int y, int scale, uint32_t color) {
    for (int r = 0; r < 16; ++r) {
        int c = 0;
        while (c < 16) {
            if (!(icons[type][r] & (1u << (15 - c)))) { ++c; continue; }
            int first = c; while (c < 16 && (icons[type][r] & (1u << (15 - c)))) ++c;
            box(x + first * scale, y + r * scale, (c - first) * scale, scale, color, 0, 0);
        }
    }
}
static void person(int x, int y, int scale) {
    for (int r = 0; r < 20; ++r) for (int c = 0; c < 16;) {
        char p = avatar[r][c]; int first = c; while (c < 16 && avatar[r][c] == p) ++c;
        if (p == '.') continue;
        uint32_t color = p == 'H' ? (m->save.avatar == 0 ? 0xDA614F : m->save.avatar == 1 ? 0xA893D7 : GREEN) : p == 'S' ? 0xF5D1A0 : p == 'W' ? WHITE : p == 'I' ? BG : 0x567383;
        if (r == 6 && p == 'I' && m->seconds % 7 == 0) {
            box(x + first * scale, y + r * scale, (c - first) * scale, 1, color, 0, 0);
        } else box(x + first * scale, y + r * scale, (c - first) * scale, scale, color, 0, 0);
    }
}
static void bar(int x, int y, int w, unsigned value, uint32_t color) {
    box(x, y, w, 6, 0x2A3A44, 3, 0); if (value) box(x, y, w * (value > 100 ? 100 : value) / 100, 6, color, 3, 0);
}
static void row(int index, int y, const char *label, int symbol) {
    bool selected = index == m->selection;
    box(12, y, 216, 35, selected ? 0x1C3B32 : PANEL, 8, selected ? GREEN : 0);
    if (symbol >= 0) icon(symbol, 23, y + 10, 1, selected ? GREEN : MUTED);
    text(symbol < 0 ? 25 : 49, y + 7, symbol < 0 ? 190 : 172, 24, label, selected ? WHITE : MUTED, false, false);
}
static void title(const char *s) { text(16, 35, 208, 28, s, WHITE, false, false); }
static void note(const char *s) { text(16, 260, 208, 34, s, MUTED, false, true); }
static unsigned bitcount(uint32_t n) { unsigned c = 0; for (; n; n &= n - 1) ++c; return c; }
static void header(void) {
    char b[48]; snprintf(b, sizeof(b), "运行 %02lu:%02lu", (unsigned long)(m->seconds / 3600 % 100), (unsigned long)(m->seconds / 60 % 60));
    text(16, 9, 112, 20, b, MUTED, false, false);
    /* Speaker bars and battery are actual runtime status, not decoration. */
    box(135, 13, 4, 8, MUTED, 0, 0); box(139, 10, 4, 14, MUTED, 0, 0);
    for (int i = 0; i < 5; ++i) box(146 + i * 4, 20 - i * 2, 2, 3 + i * 2, status->audio && m->save.volume > i * 20 ? GREEN : PANEL, 0, 0);
    box(182, 12, 29, 12, BG, 2, MUTED); box(212, 15, 2, 6, MUTED, 0, 0);
    if (status->battery >= 0) box(184, 14, status->battery * 25 / 100, 8, status->battery <= 15 ? GOLD : GREEN, 0, 0);
    else text(188, 7, 20, 22, "?", MUTED, false, false);
    box(12, 30, 216, 1, PANEL, 0, 0);
}
static void timer_page(const tk_timer *timer, bool meeting, bool reminder) {
    char b[64];
    if (!timer->total) {
        const char *const mins[] = {"5 分钟", "10 分钟", "25 分钟", "45 分钟"};
        const char *const meet[] = {"15 分钟 · 快速碰头", "25 分钟 · 专注讨论", "45 分钟 · 完整议程"};
        const char *const remind[] = {"5 分钟后", "15 分钟后", "30 分钟后", "60 分钟后"};
        for (int i = 0; i < tk_options(m); ++i) row(i, 81 + i * 42, meeting ? meet[i] : reminder ? remind[i] : mins[i], 9);
        note("关机后停止，不补计时"); return;
    }
    box(12, 72, 216, 85, PANEL, 12, 0);
    snprintf(b, sizeof(b), "%02lu : %02lu", (unsigned long)(timer->remaining / 60), (unsigned long)(timer->remaining % 60));
    text(24, 84, 192, 30, b, GREEN, true, true);
    const char *state = timer->running ? "计时中" : timer->remaining ? "已暂停" : "已完成";
    if (meeting && timer->running) {
        unsigned phase = 3 * (timer->total - timer->remaining) / timer->total;
        state = phase == 0 ? "议程 1 / 3 · 明确目标" : phase == 1 ? "议程 2 / 3 · 讨论方案" : "议程 3 / 3 · 确认行动";
    }
    text(16, 122, 208, 25, state, MUTED, false, true);
    bar(24, 151, 192, 100 * (timer->total - timer->remaining) / timer->total, GREEN);
    row(0, 169, reminder ? "取消提醒" : timer->running ? "暂停计时" : timer->remaining ? "继续计时" : "计时已完成", 9);
    if (!reminder) row(1, 209, meeting ? "添加跟进待办" : "结束 / 重新设置", 2);
    if (meeting) row(2, 249, "结束 / 重新设置", 0);
    if (reminder) note("离开页面仍会提醒");
}
static void draw(lv_event_t *e) {
    layer = lv_event_get_layer(e); box(0, 0, 240, 320, 0, 0, 0); box(0, 0, 240, 320, BG, 18, 0);
    header(); char b[160];
    switch (m->page) {
    case TK_HOME: {
        title("FoloToy · 随身小宇宙");
        const char *const names[] = {"电子身份卡", "AI 对话", "效率工具", "像素迷宫", "单词学习", "像素宠物"};
        for (int i = 0; i < 6; ++i) {
            int x = 12 + (i % 2) * 112, y = 72 + (i / 2) * 67;
            box(x, y, 104, 61, i == m->selection ? 0x1C3B32 : PANEL, 9, i == m->selection ? GREEN : 0);
            icon(i, x + 39, y + 7, 2, GREEN);
            text(x + 2, y + 36, 100, 23, names[i], WHITE, false, true);
        }
        snprintf(b, sizeof(b), "Lv.%lu   ·   %u 金币", (unsigned long)(1 + m->save.xp / 100), m->save.coins);
        text(16, 278, 208, 22, b, GREEN, false, true); break;
    }
    case TK_ID:
        title("电子身份卡"); box(12, 73, 216, 129, PANEL, 12, 0); person(23, 94, 4);
        text(98, 85, 127, 44, tk_names[m->save.name], WHITE, false, false);
        snprintf(b, sizeof(b), "ID %08lX", (unsigned long)m->save.id); text(98, 130, 124, 22, b, MUTED, false, false);
        snprintf(b, sizeof(b), "Lv.%lu", (unsigned long)(1 + m->save.xp / 100)); text(98, 156, 120, 28, b, GREEN, true, false);
        bar(99, 190, 112, m->save.xp % 100, GREEN);
        row(0, 213, "切换昵称", 0); row(1, 255, "切换像素头像", 5); break;
    case TK_CHAT:
        title("AI 对话 · 离线演示");
        box(12, 70, 216, 56, PANEL, 10, 0); icon(1, 22, 88, 2, GREEN);
        text(65, 80, 153, 44, m->chat_reply, WHITE, false, false);
        { const char *const topics[] = {"你好，搭子！", "看看我的待办", "宠物饿了吗？", "给我一个小建议"};
          for (int i = 0; i < 4; ++i) row(i, 135 + 39 * i, topics[i], 1); }
        break;
    case TK_TOOLS:
        title("效率工具");
        { const char *const names[] = {"会议助手", "待办清单", "专注倒计时", "稍后提醒"};
          for (int i = 0; i < 4; ++i) row(i, 80 + 43 * i, names[i], i == 1 ? 2 : 9); }
        note("三键也能安排好今天"); break;
    case TK_TODOS: case TK_ADD: {
        title(m->page == TK_ADD ? "添加预设待办" : "待办清单");
        int start = m->selection / 5 * 5;
        for (int i = start; i < tk_options(m) && i < start + 5; ++i) {
            if (m->page == TK_ADD) snprintf(b, sizeof(b), "%s", tk_tasks[i]);
            else if (i == m->save.count) snprintf(b, sizeof(b), "+ 添加待办");
            else snprintf(b, sizeof(b), "%s %s", m->save.done & (1 << i) ? "[x]" : "[ ]", tk_tasks[m->save.tasks[i]]);
            row(i, 74 + (i - start) * 39, b, -1);
        }
        snprintf(b, sizeof(b), "%d / %d", m->selection + 1, tk_options(m)); note(b); break;
    }
    case TK_TASK: case TK_DELETE:
        title(m->page == TK_DELETE ? "删除这条待办？" : "待办详情");
        icon(2, 97, 82, 3, GREEN); text(16, 144, 208, 42, tk_tasks[m->save.tasks[m->task]], WHITE, false, true);
        row(0, 199, m->page == TK_DELETE ? "保留" : m->save.done & (1 << m->task) ? "标为未完成" : "完成任务", 6);
        row(1, 241, m->page == TK_DELETE ? "确认删除" : "删除这条任务", 2); break;
    case TK_MEETING: title("会议助手"); timer_page(&m->meeting, true, false); break;
    case TK_COUNTDOWN: title("专注倒计时"); timer_page(&m->countdown, false, false); break;
    case TK_REMINDER: title("稍后提醒"); timer_page(&m->reminder, false, true); break;
    case TK_MAZE_MENU:
        title("像素迷宫");
        { const char *const names[] = {"轻松探索 · 5 x 5", "小小挑战 · 7 x 7", "迷宫高手 · 9 x 9", "继续刚才的迷宫"};
          for (int i = 0; i < tk_options(m); ++i) row(i, 79 + i * 42, names[i], 3); }
        note("收集星星，再走到出口"); break;
    case TK_MAZE: {
        title(m->won ? "迷宫完成！确认再来一局" : "左转 / 右转 / 前进");
        int n = m->size, cell = 189 / n, x0 = (240 - n * cell) / 2, y0 = 77;
        box(x0, y0, n * cell + 1, n * cell + 1, PANEL, 0, 0);
        for (int c = 0; c < n * n; ++c) {
            int x = x0 + c % n * cell, y = y0 + c / n * cell;
            if (m->maze[c] & 1) box(x, y, cell + 1, 1, MUTED, 0, 0);
            if (m->maze[c] & 8) box(x, y, 1, cell + 1, MUTED, 0, 0);
            if (c / n == n - 1) box(x, y + cell, cell + 1, 1, MUTED, 0, 0);
            if (c % n == n - 1) box(x + cell, y, 1, cell + 1, MUTED, 0, 0);
            if (c == m->star && !m->collected) icon(6, x + (cell - 16) / 2, y + (cell - 16) / 2 + 2, 1, GOLD);
            if (c == n * n - 1) icon(7, x + (cell - 16) / 2, y + (cell - 16) / 2 + 2, 1, GREEN);
            if (c == m->player) {
                int cx = x + cell / 2, cy = y + cell / 2;
                box(cx - 4, cy - 4, 9, 9, GREEN, 2, 0);
                const int dx[] = {0, 1, 0, -1}, dy[] = {-1, 0, 1, 0};
                box(cx + dx[m->direction] * 6 - 1, cy + dy[m->direction] * 6 - 1, 3, 3, WHITE, 0, 0);
            }
        }
        snprintf(b, sizeof(b), "%u 步  ·  星星 %s", m->steps, m->collected ? "1 / 1" : "0 / 1");
        text(16, 278, 208, 22, b, m->collected ? GREEN : MUTED, false, true); break;
    }
    case TK_WORD_MENU:
        title("单词学习");
        snprintf(b, sizeof(b), "已掌握 %u / 24", bitcount(m->save.learned)); text(16, 80, 208, 24, b, GREEN, false, true);
        row(0, 119, "翻卡记单词", 4); row(1, 162, "三选一小测验", 6); row(2, 205, "复习错词", 4);
        note("首次掌握获得成长与金币"); break;
    case TK_WORD: case TK_QUIZ: {
        const tk_word *word = &tk_words[m->word]; title(m->page == TK_QUIZ ? "选出正确的意思" : "单词卡");
        box(12, 72, 216, 106, PANEL, 12, 0);
        icon(m->word == 0 ? 10 : m->word == 7 ? 6 : m->word == 22 ? 8 : 4, 103, 82, 2, m->word == 0 ? 0xDA614F : GREEN);
        text(20, 111, 200, 30, word->en, GREEN, true, true);
        if (m->flipped || (m->page == TK_QUIZ && m->answered)) text(20, 148, 200, 24, word->meaning, WHITE, false, true);
        if (m->page == TK_WORD) {
            if (m->flipped) {
                text(20, 188, 200, 47, word->example, MUTED, false, true);
                row(0, 236, "记住了", 6); row(1, 274, "还不熟", 4);
            } else { row(0, 207, "翻面看解释", 4); note("先想想，再揭晓"); }
        } else {
            for (int i = 0; i < 3; ++i) {
                int index = i == m->quiz_answer ? m->word : (m->word + 1 + ((i - m->quiz_answer + 3) % 3)) % TK_WORDS;
                row(i, 189 + 36 * i, tk_words[index].meaning, m->answered && i == m->quiz_answer ? 6 : 4);
            }
        }
        break;
    }
    case TK_PET:
        title("像素搭子");
        box(12, 71, 216, 114, PANEL, 12, 0);
        icon(5, 29, 89 + (m->seconds % 2) * 2, 5, GREEN);
        if (m->save.needs[1] >= 60) icon(7, 100, 81, 1, GOLD);
        { const char *const needs[] = {"饱腹", "心情", "精力", "整洁"};
          for (int i = 0; i < 4; ++i) { text(120, 77 + i * 25, 40, 22, needs[i], MUTED, false, false); bar(158, 86 + i * 25, 55, m->save.needs[i], GREEN); }
          const char *const actions[] = {"喂食 -5", "玩耍", "休息", "清洁"};
          for (int i = 0; i < 4; ++i) {
              int x = 12 + i % 2 * 112, y = 196 + i / 2 * 44;
              box(x, y, 104, 38, i == m->selection ? 0x1C3B32 : PANEL, 8, i == m->selection ? GREEN : 0);
              text(x + 4, y + 9, 96, 24, actions[i], WHITE, false, true);
          } }
        snprintf(b, sizeof(b), "Lv.%lu  ·  %u 金币", (unsigned long)(1 + m->save.xp / 100), m->save.coins);
        text(16, 280, 208, 22, b, GREEN, false, true); break;
    case TK_SETTINGS:
        title("设置"); snprintf(b, sizeof(b), "音效音量    %u%%", m->save.volume); row(0, 82, b, 8);
        snprintf(b, sizeof(b), "屏幕亮度    %u%%", m->save.brightness); row(1, 127, b, 6);
        text(20, 184, 200, 67, status->storage < 0 ? "存档不可用\n本次更改尚未保存" : status->storage == 0 ? "正在保存…" : "本地存档已保存", status->storage < 0 ? GOLD : MUTED, false, true);
        if (!status->audio) note("音频不可用 · 其他功能可用");
        else note("确认循环调节 · 长按返回");
        break;
    }
    if (m->page != TK_WORD || !m->flipped)
        text(14, 298, 212, 21, m->page == TK_HOME ? "上下选 确认 长按设置" : m->page == TK_MAZE ? "转向 前进 长按返回" : m->page == TK_QUIZ && m->answered ? "确认下一题 长按返回" : "上下选 确认 长按返回", MUTED, false, true);
    if (m->notice_ticks) {
        box(12, 231, 216, 60, 0x214133, 10, GREEN);
        text(23, 241, 194, 48, m->notice, WHITE, false, true);
    }
    if (m->alert) {
        box(8, 74, 224, 210, PANEL, 14, GREEN); icon(9, 96, 91, 3, GREEN);
        snprintf(b, sizeof(b), "%s%s%s", (m->alert & 1) ? "会议时间到！\n" : "", (m->alert & 2) ? "倒计时结束！\n" : "", (m->alert & 4) ? "提醒时间到！" : "");
        text(20, 145, 200, 95, b, WHITE, false, true);
        text(20, 245, 200, 24, "确认关闭提醒", GREEN, false, true);
    }
    if (!status->buttons) { box(8, 264, 224, 39, PANEL, 8, GOLD); text(16, 270, 208, 25, "按键初始化失败，请重启", GOLD, false, true); }
}
lv_obj_t *tk_view_create(const tk_model *model, const tk_status *runtime) {
    m = model; status = runtime;
    lv_obj_t *screen = lv_obj_create(NULL); lv_obj_remove_style_all(screen);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE); lv_obj_set_size(screen, 240, 320);
    lv_obj_add_event_cb(screen, draw, LV_EVENT_DRAW_MAIN, NULL); lv_screen_load(screen); return screen;
}
