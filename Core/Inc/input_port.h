#ifndef INPUT_PORT_H
#define INPUT_PORT_H
/* Button B1 (PA1) as an LVGL keypad:
 *   short press  -> LV_KEY_NEXT  (move focus to the next item)
 *   long press   -> LV_KEY_ENTER (click / toggle the focused item)
 * Call after lvgl_port_init(). Creates the default LVGL group. */
void input_port_init(void);
#endif
