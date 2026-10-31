/* Home menu: one entry per feature. Add new features to MENU[] below. */
#include "ui.h"

typedef struct { const char *text; void (*show)(void); } menu_item_t;

static const menu_item_t MENU[] = {
    { "1. Hello LVGL",    ui_show_hello   },
    { "2. Button demo",   ui_show_buttons },
    { "3. Dashboard",     ui_show_dashboard },
    { "4. Speed test",    ui_show_speed },
    /* next steps get added here */
};
#define MENU_COUNT (sizeof(MENU) / sizeof(MENU[0]))

static void item_cb(lv_event_t *e)
{
    const menu_item_t *item = (const menu_item_t *)lv_event_get_user_data(e);
    ui_go(item->show);
}

void ui_show_home(void)
{
    lv_obj_t *scr = ui_new_screen("Menu");

    lv_obj_t *hint = lv_label_create(scr);
    lv_obj_set_style_text_font(hint, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(hint, lv_color_hex(0xFFD000), 0);
    lv_label_set_text(hint, "B1: tap = next    hold = select");
    lv_obj_align(hint, LV_ALIGN_TOP_MID, 0, 44);

    /* scrolling column of menu buttons */
    lv_obj_t *list = lv_obj_create(scr);
    lv_obj_set_size(list, 300, 170);
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(list, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(list, 8, 0);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(list, 0, 0);

    lv_obj_t *first = NULL;
    for (unsigned i = 0; i < MENU_COUNT; i++) {
        lv_obj_t *btn = lv_button_create(list);
        lv_obj_set_size(btn, 270, 46);
        lv_obj_add_event_cb(btn, item_cb, LV_EVENT_CLICKED, (void *)&MENU[i]);
        lv_obj_t *l = lv_label_create(btn);
        lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
        lv_label_set_text(l, MENU[i].text);
        lv_obj_align(l, LV_ALIGN_LEFT_MID, 8, 0);
        if (!first) first = btn;
    }
    ui_load(scr, first);
}
