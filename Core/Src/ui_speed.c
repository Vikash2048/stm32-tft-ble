/* Step 3 – Speed test.
 * Shows CPU clock + instruction-cache state and measures a full-screen redraw
 *   (a) through LVGL (render + send) and (b) raw driver fill (send only),
 * each with the NORMAL and the FAST driver (tft_set_fast). 5 frames each.
 * The "Fast" switch selects which driver the whole app uses afterwards. */
#include "main.h"
#include "ui.h"
#include "tft.h"

#define FRAMES  5

static lv_obj_t *info_label, *run_btn, *fast_sw;
static lv_obj_t *cell[3][2];           /* rows: LVGL ms, raw ms, LVGL fps; cols: normal, fast */

static void show_info(void)
{
    uint32_t mhz = HAL_RCC_GetHCLKFreq() / 1000000U;
    int cache_on = (ICACHE->CR & ICACHE_CR_EN) != 0;
    lv_label_set_text_fmt(info_label, "CPU %lu MHz   I-cache %s",
                          (unsigned long)mhz, cache_on ? "ON" : "OFF");
}

static uint32_t measure_lvgl(lv_obj_t *scr)
{
    static const uint32_t colors[FRAMES] = { 0x800000, 0x008000, 0x000080, 0x808000, 0x008080 };
    uint32_t t0 = HAL_GetTick();
    for (int i = 0; i < FRAMES; i++) {
        lv_obj_set_style_bg_color(scr, lv_color_hex(colors[i]), 0);  /* whole screen invalid */
        lv_refr_now(NULL);                                           /* render + flush now   */
    }
    return HAL_GetTick() - t0;
}

static uint32_t measure_raw(void)
{
    static const uint16_t colors[FRAMES] = { RED, GREEN, BLUE, YELLOW, CYAN };
    uint32_t t0 = HAL_GetTick();
    for (int i = 0; i < FRAMES; i++) tft_fill_screen(colors[i]);
    return HAL_GetTick() - t0;
}

static void run_test(void *unused)
{
    (void)unused;
    lv_obj_t *scr = lv_screen_active();
    int keep = tft_get_fast();

    for (int mode = 0; mode < 2; mode++) {           /* 0 = normal, 1 = fast */
        tft_set_fast(mode);
        uint32_t t_lvgl = measure_lvgl(scr);
        uint32_t t_raw  = measure_raw();
        if (t_lvgl == 0) t_lvgl = 1;
        uint32_t fps10 = (FRAMES * 10000U) / t_lvgl;
        lv_label_set_text_fmt(cell[0][mode], "%lu ms", (unsigned long)(t_lvgl / FRAMES));
        lv_label_set_text_fmt(cell[1][mode], "%lu ms", (unsigned long)(t_raw / FRAMES));
        lv_label_set_text_fmt(cell[2][mode], "%lu.%lu", (unsigned long)(fps10 / 10),
                              (unsigned long)(fps10 % 10));
    }
    tft_set_fast(keep);

    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);   /* restore normal screen */
    lv_obj_invalidate(scr);
    show_info();
}

static void run_cb(lv_event_t *e)
{
    (void)e;
    lv_async_call(run_test, NULL);       /* run after this event has finished */
}

static void fast_cb(lv_event_t *e)
{
    tft_set_fast(lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED));
    lv_obj_invalidate(lv_screen_active());       /* redraw with the chosen driver */
}

static lv_obj_t *text(lv_obj_t *parent, const lv_font_t *f, uint32_t color,
                      int32_t x, int32_t y, const char *t)
{
    lv_obj_t *l = lv_label_create(parent);
    lv_obj_set_style_text_font(l, f, 0);
    lv_obj_set_style_text_color(l, lv_color_hex(color), 0);
    lv_obj_set_pos(l, x, y);
    lv_label_set_text(l, t);
    return l;
}

void ui_show_speed(void)
{
    lv_obj_t *scr = ui_new_screen(NULL);
    lv_obj_t *back = ui_add_back_button(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_label_set_text(title, "Speed test");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 40, 10);

    info_label = text(scr, &lv_font_montserrat_20, 0xFFD000, 10, 50, "");
    show_info();

    /* Run button (left) and Fast switch (right) */
    run_btn = lv_button_create(scr);
    lv_obj_set_size(run_btn, 130, 38);
    lv_obj_set_pos(run_btn, 10, 80);
    lv_obj_add_event_cb(run_btn, run_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(run_btn);
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_20, 0);
    lv_label_set_text(bl, "Run test");
    lv_obj_center(bl);

    text(scr, &lv_font_montserrat_20, 0xFFFFFF, 178, 88, "Fast");
    fast_sw = lv_switch_create(scr);
    lv_obj_set_size(fast_sw, 64, 32);
    lv_obj_set_pos(fast_sw, 240, 83);
    if (tft_get_fast()) lv_obj_add_state(fast_sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(fast_sw, fast_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Results table:          normal   fast */
    const int32_t col_x[2] = { 150, 240 };
    const int32_t row_y[3] = { 156, 182, 208 };
    static const char *row_name[3] = { "LVGL frame", "Raw fill", "LVGL fps" };
    text(scr, &lv_font_montserrat_14, 0x909090, col_x[0], 134, "normal");
    text(scr, &lv_font_montserrat_14, 0x909090, col_x[1], 134, "fast");
    for (int r = 0; r < 3; r++) {
        text(scr, &lv_font_montserrat_20, 0xFFFFFF, 10, row_y[r], row_name[r]);
        for (int c = 0; c < 2; c++)
            cell[r][c] = text(scr, &lv_font_montserrat_20, 0x00D0FF, col_x[c], row_y[r], "-");
    }

    ui_load(scr, run_btn);
    (void)back;
}
