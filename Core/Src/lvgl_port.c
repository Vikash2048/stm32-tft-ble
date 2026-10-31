/* LVGL v9 port for the jumper-wired 8-bit parallel TFT (see tft.c / TFT_SETUP.md). */
#include "main.h"
#include "tft.h"
#include "lvgl_port.h"
#include "lvgl.h"

/* Partial render buffer: 320 x 24 lines x 2 bytes = 15 KB.
 * LVGL draws into this, then flush_cb sends it to the screen. */
#define DRAW_BUF_LINES  24
static uint8_t draw_buf[TFT_WIDTH * DRAW_BUF_LINES * 2] __attribute__((aligned(4)));

static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    tft_push_pixels((uint16_t)area->x1, (uint16_t)area->y1,
                    (uint16_t)area->x2, (uint16_t)area->y2,
                    (const uint16_t *)px_map);
    lv_display_flush_ready(disp);          /* tell LVGL the buffer is free again */
}

void lvgl_port_init(void)
{
    lv_init();
    lv_tick_set_cb(HAL_GetTick);           /* LVGL time base = HAL 1 ms tick */

    lv_display_t *disp = lv_display_create(TFT_WIDTH, TFT_HEIGHT);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(disp, draw_buf, NULL, sizeof(draw_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, flush_cb);

    /* Dark default theme: readable on this panel, shows a focus outline for the button input */
    lv_display_set_theme(disp, lv_theme_default_init(disp,
                         lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_CYAN),
                         true, &lv_font_montserrat_20));
}
