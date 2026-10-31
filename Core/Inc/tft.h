#ifndef TFT_DRIVER_H
#define TFT_DRIVER_H

#include "main.h"   /* pulls in the STM32 HAL for your chip */

/* Orientation: landscape 320 x 240.
 * MADCTL (command 0x36) value: 0xE8 = landscape (correct for our setup), 0x28 = landscape rotated 180,
 * 0x48 = portrait (then use 240 x 320). Bit 3 (0x08) = BGR colour order. */
#define TFT_WIDTH   320
#define TFT_HEIGHT  240
#define TFT_MADCTL  0xE8

/* RGB565 colours */
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0
#define CYAN    0x07FF
#define MAGENTA 0xF81F


void     tft_init(void);
void     tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void     tft_fill_screen(uint16_t color);
/* Send a block of RGB565 pixels to the area (x0,y0)-(x1,y1), inclusive. Used by LVGL. */
void     tft_push_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                         const uint16_t *px);

/* Step 3: fast pixel output. 0 = normal (original loop), 1 = fast (optimised). */
void     tft_set_fast(int on);
int      tft_get_fast(void);

#endif
