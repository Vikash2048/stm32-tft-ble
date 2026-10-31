/* Step 0 – Hello LVGL: title, subtitle, animated progress bar, uptime counter. */
#include "ui.h"

static lv_obj_t   *bar_label;
static lv_obj_t   *uptime_label;
static lv_timer_t *uptime_timer;
static uint32_t    seconds;

static void bar_anim_cb(void *obj, int32_t v)
{
    lv_bar_set_value((lv_obj_t *)obj, v, LV_ANIM_OFF);
    lv_label_set_text_fmt(bar_label, "%d %%", (int)v);
}

static void uptime_timer_cb(lv_timer_t *t)
{
    (void)t;
    seconds++;
    lv_label_set_text_fmt(uptime_label, "Uptime: %lu s", (unsigned long)seconds);
}

static void screen_deleted_cb(lv_event_t *e)
{
    (void)e;
    lv_timer_delete(uptime_timer);       /* stop the timer when leaving the screen */
    uptime_timer = NULL;
}

void ui_show_hello(void)
{
    lv_obj_t *scr = ui_new_screen(NULL);
    lv_obj_add_event_cb(scr, screen_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_obj_t *back = ui_add_back_button(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_label_set_text(title, "Hello LVGL");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 30, 10);

    lv_obj_t *sub = lv_label_create(scr);
    lv_obj_set_style_text_font(sub, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(sub, lv_color_hex(0xFFD000), 0);
    lv_label_set_text(sub, "NUCLEO-WBA25CE1");
    lv_obj_align(sub, LV_ALIGN_TOP_MID, 0, 50);

    lv_obj_t *bar = lv_bar_create(scr);
    lv_obj_set_size(bar, 260, 28);
    lv_obj_align(bar, LV_ALIGN_CENTER, 0, 6);
    lv_bar_set_range(bar, 0, 100);
    lv_obj_set_style_radius(bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(bar, 4, LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x404040), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, LV_PART_MAIN);
    lv_obj_set_style_bg_color(bar, lv_color_hex(0x00E040), LV_PART_INDICATOR);
    lv_obj_set_style_border_width(bar, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(bar, lv_color_white(), LV_PART_MAIN);

    bar_label = lv_label_create(scr);
    lv_obj_set_style_text_font(bar_label, &lv_font_montserrat_28, 0);
    lv_label_set_text(bar_label, "0 %");
    lv_obj_align_to(bar_label, bar, LV_ALIGN_OUT_BOTTOM_MID, 0, 6);

    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, bar);
    lv_anim_set_exec_cb(&a, bar_anim_cb);
    lv_anim_set_values(&a, 0, 100);
    lv_anim_set_duration(&a, 3000);
    lv_anim_set_reverse_duration(&a, 3000);
    lv_anim_set_repeat_count(&a, LV_ANIM_REPEAT_INFINITE);
    lv_anim_start(&a);                  /* deleted automatically with the bar */

    uptime_label = lv_label_create(scr);
    lv_obj_set_style_text_font(uptime_label, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(uptime_label, lv_color_hex(0x00D0FF), 0);
    lv_label_set_text_fmt(uptime_label, "Uptime: %lu s", (unsigned long)seconds);
    lv_obj_align(uptime_label, LV_ALIGN_BOTTOM_MID, 0, -12);
    uptime_timer = lv_timer_create(uptime_timer_cb, 1000, NULL);

    ui_load(scr, back);
}
