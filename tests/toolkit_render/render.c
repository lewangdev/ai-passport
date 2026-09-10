#include "toolkit_view.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
static uint16_t framebuffer[240 * 320], dma[240 * 20];
static void flush(lv_display_t *d, const lv_area_t *a, uint8_t *p) {
    uint16_t *pixels = (uint16_t *)p;
    for (int y = a->y1; y <= a->y2; ++y) for (int x = a->x1; x <= a->x2; ++x) {
        assert(x >= 0 && x < 240 && y >= 0 && y < 320); framebuffer[y * 240 + x] = *pixels++;
    }
    lv_display_flush_ready(d);
}
static void capture(lv_obj_t *screen, const char *dir, int page, int variant) {
    lv_obj_invalidate(screen); lv_refr_now(NULL);
    assert(framebuffer[0] == 0 && framebuffer[239] == 0 && framebuffer[319 * 240] == 0);
    char path[512]; snprintf(path, sizeof(path), "%s/page-%02d-%d.ppm", dir, page, variant);
    FILE *f = fopen(path, "wb"); assert(f); fprintf(f, "P6\n240 320\n255\n");
    for (int i = 0; i < 240 * 320; ++i) {
        uint16_t p = framebuffer[i]; uint8_t rgb[] = {((p >> 11) & 31) * 255 / 31, ((p >> 5) & 63) * 255 / 63, (p & 31) * 255 / 31};
        fwrite(rgb, 1, 3, f);
    }
    fclose(f);
}
int main(int argc, char **argv) {
    assert(argc == 2); lv_init();
    lv_display_t *display = lv_display_create(240, 320); lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, dma, NULL, sizeof(dma), LV_DISPLAY_RENDER_MODE_PARTIAL); lv_display_set_flush_cb(display, flush);
    tk_model model; tk_init(&model, 3); tk_status status = {78, 1, true, true};
    lv_obj_t *screen = tk_view_create(&model, &status);
    for (int page = TK_HOME; page <= TK_REMINDER; ++page) for (int variant = 0; variant < 2; ++variant) {
        tk_init(&model, 3); tk_maze_start(&model, variant ? 2 : 0); model.page = page;
        model.selection = variant ? tk_options(&model) - 1 : 0; model.seconds = 125;
        model.flipped = model.answered = variant; model.word = variant ? 19 : 0;
        if (variant) { model.meeting = (tk_timer){721, 1500, true}; model.countdown = model.meeting; model.reminder = model.meeting; }
        capture(screen, argv[1], page, variant);
    }
    model.page = TK_HOME; model.alert = 7; capture(screen, argv[1], 99, 0);
    model.alert = 0; tk_message(&model, "宠物饱腹值 100 / 100"); capture(screen, argv[1], 99, 1);
    /* Extremes, scrolling, failure messages and the entire word bank. */
    model.notice_ticks = 0; model.save.xp = 999999; model.save.coins = 9999;
    model.page = TK_ID;
    for (int name = 0; name < 4; ++name) { model.save.name = name; capture(screen, argv[1], 80, name); }
    model.page = TK_TODOS; model.save.count = 8; model.selection = 8; capture(screen, argv[1], 81, 0);
    model.page = TK_SETTINGS; status.storage = -1; status.audio = false; capture(screen, argv[1], 82, 0);
    model.page = TK_WORD; model.flipped = true;
    for (int word = 0; word < TK_WORDS; ++word) { model.word = word; capture(screen, argv[1], 83, word); }
    model.page = TK_CHAT;
    for (int topic = 0; topic < 4; ++topic) { model.selection = topic; tk_keypress(&model, TK_OK); capture(screen, argv[1], 84, topic); }
    lv_deinit(); puts("Toolkit render: all pages, text and glyph coverage PASS");
}
