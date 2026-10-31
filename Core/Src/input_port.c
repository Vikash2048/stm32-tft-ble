/* LVGL input: board button B1 (PA1, pressed = low) as a 1-button keypad. */
#include "main.h"
#include "stm32wbaxx_nucleo.h"
#include "input_port.h"
#include "lvgl.h"

#define LONG_PRESS_MS   500     /* hold at least this long = ENTER        */
#define DEBOUNCE_MS     30      /* shorter presses are ignored (bounce)   */

static lv_key_t  key_out   = LV_KEY_NEXT;
static uint8_t   release_pending;   /* 1 = report "released" on next read */
static uint8_t   was_down;
static uint8_t   long_fired;
static uint32_t  down_since;

static void read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->key = key_out;

    /* A key was reported as pressed last time: now report the release */
    if (release_pending) {
        release_pending = 0;
        data->state = LV_INDEV_STATE_RELEASED;
        return;
    }

    uint8_t  down = (uint8_t)BSP_PB_GetState(B1);
    uint32_t now  = HAL_GetTick();
    data->state = LV_INDEV_STATE_RELEASED;

    if (down && !was_down) {                         /* button just pressed */
        down_since = now;
        long_fired = 0;
    }
    if (down && !long_fired && (now - down_since) >= LONG_PRESS_MS) {
        long_fired = 1;                              /* held long: ENTER    */
        key_out = LV_KEY_ENTER;
        data->key = key_out;
        data->state = LV_INDEV_STATE_PRESSED;
        release_pending = 1;
    } else if (!down && was_down && !long_fired &&
               (now - down_since) >= DEBOUNCE_MS) {  /* short tap: NEXT     */
        key_out = LV_KEY_NEXT;
        data->key = key_out;
        data->state = LV_INDEV_STATE_PRESSED;
        release_pending = 1;
    }
    was_down = down;
}

void input_port_init(void)
{
    /* B1 was set up by BSP_PB_Init() in main.c (input, pull-up, interrupt).
       We only poll it, so switch its interrupt off. */
    HAL_NVIC_DisableIRQ(EXTI1_IRQn);

    lv_group_t *g = lv_group_create();
    lv_group_set_default(g);           /* new buttons/switches join it */

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, read_cb);
    lv_indev_set_group(indev, g);
}
