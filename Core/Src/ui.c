/* UI helpers shared by all screens. */
#include "ui.h"

void ui_create(void)
{
    ui_show_home();
}

lv_obj_t *ui_new_screen(const char *title)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(scr, lv_color_white(), 0);

    if (title) {
        lv_obj_t *t = lv_label_create(scr);
        lv_obj_set_style_text_font(t, &lv_font_montserrat_28, 0);
        lv_label_set_text(t, title);
        lv_obj_align(t, LV_ALIGN_TOP_MID, 0, 8);
    }
    return scr;
}

/* Switching screens from inside a button event would delete the button while
 * LVGL is still using it, so the switch is done a moment later (async). */
static void go_async(void *fn)
{
    ((void (*)(void))fn)();
}

void ui_go(void (*show_fn)(void))
{
    lv_async_call(go_async, (void *)show_fn);
}

static void back_cb(lv_event_t *e)
{
    (void)e;
    ui_go(ui_show_home);
}

lv_obj_t *ui_add_back_button(lv_obj_t *scr)
{
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 84, 36);
    lv_obj_align(btn, LV_ALIGN_TOP_LEFT, 6, 6);
    lv_obj_add_event_cb(btn, back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *l = lv_label_create(btn);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_14, 0);
    lv_label_set_text(l, LV_SYMBOL_LEFT " Menu");
    lv_obj_center(l);
    return btn;
}

void ui_load(lv_obj_t *scr, lv_obj_t *focus)
{
    lv_screen_load_anim(scr, LV_SCREEN_LOAD_ANIM_NONE, 0, 0, true);  /* true = delete old screen */
    if (focus) lv_group_focus_obj(focus);
}
