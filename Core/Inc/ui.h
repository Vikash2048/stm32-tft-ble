#ifndef UI_H
#define UI_H
#include "lvgl.h"

/* Entry point: shows the home menu. Call after lvgl_port_init() and input_port_init(). */
void ui_create(void);

/* Screens (one file each). Every feature we add gets its own screen + menu entry. */
void ui_show_home(void);      /* ui_home.c    - menu                       */
void ui_show_hello(void);     /* ui_hello.c   - step 0: Hello LVGL         */
void ui_show_buttons(void);   /* ui_buttons.c - step 1: button control     */
void ui_show_dashboard(void); /* ui_dashboard.c - step 2: live sensor dashboard */
void ui_show_speed(void);     /* ui_speed.c   - step 3: redraw speed test       */

/* Helpers (ui.c) */
lv_obj_t *ui_new_screen(const char *title);          /* black screen + title       */
lv_obj_t *ui_add_back_button(lv_obj_t *scr);         /* "< Menu" button, top-left  */
void      ui_load(lv_obj_t *scr, lv_obj_t *focus);   /* show screen, delete old    */
void      ui_go(void (*show_fn)(void));              /* switch screen from an event */

#endif
