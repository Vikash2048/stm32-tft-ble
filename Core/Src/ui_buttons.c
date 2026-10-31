/* Step 1 – Button control with B1: push button + counter, switch -> green LED LD2, checkbox. */
#include "main.h"
#include "ui.h"

#define LD2_PORT  GPIOB
#define LD2_PIN   GPIO_PIN_12          /* LD2 (green), active low, free pin */

static lv_obj_t *count_label;
static lv_obj_t *status_label;
static uint32_t  press_count;

static void led_set(int on)
{
    HAL_GPIO_WritePin(LD2_PORT, LD2_PIN, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void push_cb(lv_event_t *e)
{
    (void)e;
    press_count++;
    lv_label_set_text_fmt(count_label, "Pressed: %lu", (unsigned long)press_count);
    lv_label_set_text(status_label, "Button clicked");
}

static void switch_cb(lv_event_t *e)
{
    lv_obj_t *sw = lv_event_get_target_obj(e);
    int on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    led_set(on);
    lv_label_set_text(status_label, on ? "LED LD2 ON" : "LED LD2 OFF");
}

static void check_cb(lv_event_t *e)
{
    lv_obj_t *cb = lv_event_get_target_obj(e);
    lv_label_set_text(status_label,
                      lv_obj_has_state(cb, LV_STATE_CHECKED) ? "Option checked" : "Option unchecked");
}

static void screen_deleted_cb(lv_event_t *e)
{
    (void)e;
    led_set(0);                        /* LED off when leaving this screen */
}

void ui_show_buttons(void)
{
    lv_obj_t *scr = ui_new_screen(NULL);
    lv_obj_add_event_cb(scr, screen_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_obj_t *back = ui_add_back_button(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_28, 0);
    lv_label_set_text(title, "Buttons");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 40, 10);

    /* Push button + counter (left column) */
    lv_obj_t *btn = lv_button_create(scr);
    lv_obj_set_size(btn, 140, 50);
    lv_obj_set_pos(btn, 14, 64);
    lv_obj_add_event_cb(btn, push_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_t *bl = lv_label_create(btn);
    lv_obj_set_style_text_font(bl, &lv_font_montserrat_20, 0);
    lv_label_set_text(bl, "Press me");
    lv_obj_center(bl);

    count_label = lv_label_create(scr);
    lv_obj_set_style_text_font(count_label, &lv_font_montserrat_20, 0);
    lv_label_set_text_fmt(count_label, "Pressed: %lu", (unsigned long)press_count);
    lv_obj_set_pos(count_label, 18, 124);

    /* Switch -> LED LD2 (right column) */
    lv_obj_t *swl = lv_label_create(scr);
    lv_obj_set_style_text_font(swl, &lv_font_montserrat_20, 0);
    lv_label_set_text(swl, "LED LD2");
    lv_obj_set_pos(swl, 180, 60);

    lv_obj_t *sw = lv_switch_create(scr);
    lv_obj_set_size(sw, 70, 34);
    lv_obj_set_pos(sw, 186, 88);
    lv_obj_add_event_cb(sw, switch_cb, LV_EVENT_VALUE_CHANGED, NULL);
    led_set(0);

    /* Checkbox */
    lv_obj_t *cb = lv_checkbox_create(scr);
    lv_obj_set_style_text_font(cb, &lv_font_montserrat_20, 0);
    lv_checkbox_set_text(cb, "Option");
    lv_obj_set_pos(cb, 180, 134);
    lv_obj_add_event_cb(cb, check_cb, LV_EVENT_VALUE_CHANGED, NULL);

    /* Status line */
    status_label = lv_label_create(scr);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x00D0FF), 0);
    lv_label_set_text(status_label, "Tap B1 to move, hold to press");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -14);

    ui_load(scr, btn);
    (void)back;
}
