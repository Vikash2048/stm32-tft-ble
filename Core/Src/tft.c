/* Minimal 8-bit parallel TFT driver (ILI9341 / ILI9486 class)
 * for NUCLEO-WBA25CE1 with the shield connected by JUMPER WIRES.
 *
 * Wiring (shield pin -> board). CN3 = left outer pin row, CN4 = right outer pin row
 * (pin positions from user manual UM3610, Table 13):
 *   LCD_D0  -> PA9   (CN6 pin 4)      LCD_WR  -> PA5   (CN6 pin 3)
 *   LCD_D1  -> PA10  (CN6 pin 5)      LCD_RS  -> PC13  (CN4 pin 30)
 *   LCD_D2  -> PA8   (CN6 pin 6)      LCD_CS  -> GND
 *   LCD_D3  -> PA11  (CN6 pin 9)      LCD_RD  -> 3V3
 *   LCD_D4  -> PB2   (CN6 pin 10)     LCD_RST -> 3V3
 *   LCD_D5  -> PA7   (CN4 pin 6)      5V  -> 5V   (CN5 pin 5)
 *   LCD_D6  -> PB15  (CN4 pin 38)     3V3 -> 3V3  (CN5 pin 4)
 *   LCD_D7  -> PA6   (CN3 pin 37)     GND -> GND  (CN5 pin 6)
 */
#include "tft.h"

typedef struct { GPIO_TypeDef *port; uint16_t pin; } pin_t;

static const pin_t DATA[8] = {
    {GPIOA, GPIO_PIN_9 },   /* D0 */
    {GPIOA, GPIO_PIN_10},   /* D1 */
    {GPIOA, GPIO_PIN_8 },   /* D2 */
    {GPIOA, GPIO_PIN_11},   /* D3 */
    {GPIOB, GPIO_PIN_2 },   /* D4 */
    {GPIOA, GPIO_PIN_7 },   /* D5 */
    {GPIOB, GPIO_PIN_15},   /* D6 */
    {GPIOA, GPIO_PIN_6 },   /* D7 */
};
#define LCD_WR   GPIOA, GPIO_PIN_5
#define LCD_RS   GPIOC, GPIO_PIN_13

/* data pins grouped per port, for faster writes */
#define DATA_MASK_A (GPIO_PIN_6 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11)
#define DATA_MASK_B (GPIO_PIN_2 | GPIO_PIN_15)

static inline void pin_w(GPIO_TypeDef *p, uint16_t n, int v)
{
    p->BSRR = v ? n : ((uint32_t)n << 16);
}

static void short_delay(void)            /* a few microseconds */
{
    for (volatile int i = 0; i < 300; i++);
}

static void tft_gpio_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    /* PC13 is also user button B2: stop its interrupt */
    HAL_NVIC_DisableIRQ(EXTI13_IRQn);

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_OUTPUT_PP;
    g.Pull  = GPIO_NOPULL;
    g.Speed = GPIO_SPEED_FREQ_HIGH;

    g.Pin = DATA_MASK_A | GPIO_PIN_5;            HAL_GPIO_Init(GPIOA, &g);
    g.Pin = DATA_MASK_B;                         HAL_GPIO_Init(GPIOB, &g);
    g.Speed = GPIO_SPEED_FREQ_LOW;               /* PC13 is a low-speed pin */
    g.Pin = GPIO_PIN_13;                         HAL_GPIO_Init(GPIOC, &g);

    pin_w(LCD_WR, 1);
    pin_w(LCD_RS, 1);
}

/* Lookup tables: for every byte value, the BSRR word for GPIOA and GPIOB.
 * Writing a byte is then just two register writes (built once in tft_init). */
static uint32_t lutA[256], lutB[256];

static void build_lut(void)
{
    for (int v = 0; v < 256; v++) {
        uint32_t setA = 0, setB = 0;
        for (int i = 0; i < 8; i++) {
            if (v & (1 << i)) {
                if (DATA[i].port == GPIOA) setA |= DATA[i].pin;
                else                       setB |= DATA[i].pin;
            }
        }
        lutA[v] = setA | ((uint32_t)(DATA_MASK_A & ~setA) << 16);
        lutB[v] = setB | ((uint32_t)(DATA_MASK_B & ~setB) << 16);
    }
}

#define WR_PIN   GPIO_PIN_5                      /* PA5 */

static inline void write8(uint8_t b)
{
    GPIOA->BSRR = lutA[b] | ((uint32_t)WR_PIN << 16);   /* data + WR low */
    GPIOB->BSRR = lutB[b];
    __NOP(); __NOP(); __NOP(); __NOP();          /* data setup time (level shifters) */
    __NOP(); __NOP(); __NOP(); __NOP();
    GPIOA->BSRR = WR_PIN;                        /* WR high: display latches */
    __NOP(); __NOP();                            /* hold time */
}

/* RS is on PC13 (slow pin + button circuit), so give it time to settle */
static void cmd(uint8_t c)
{
    pin_w(LCD_RS, 0); short_delay();
    write8(c);
    pin_w(LCD_RS, 1); short_delay();
}
static void data(uint8_t d) { write8(d); }

/* ---------------------------------------------------------------------------
 * FAST pixel output (step 3). Same bus protocol as write8(), but:
 *  - compiled with -O2 even in the Debug build,
 *  - no function call per byte, GPIO addresses kept in registers,
 *  - fill: the 4 register values for one pixel are computed once,
 *  - shorter waits: 3 NOPs data setup, 1 NOP hold (ILI9341 needs a write
 *    cycle of >= 66 ns; at 64 MHz one CPU cycle = 15.6 ns, and every GPIO
 *    write itself takes a few cycles on the bus).
 * The original slow path stays available: tft_set_fast(0).
 * ------------------------------------------------------------------------- */
static uint8_t fast_mode = 1;          /* 1 = fast (default, tested OK), 0 = normal */

void tft_set_fast(int on) { fast_mode = on ? 1 : 0; }
int  tft_get_fast(void)   { return fast_mode; }

#define BYTE_OUT(a, b)                                                     \
    do {                                                                   \
        A->BSRR = (a);              /* data on port A + WR low */          \
        B->BSRR = (b);              /* data on port B          */          \
        __NOP(); __NOP(); __NOP();  /* setup time              */          \
        A->BSRR = WR_PIN;           /* WR high: display latches */         \
        __NOP();                    /* hold time               */          \
    } while (0)

__attribute__((optimize("O2")))
static void push_fast(const uint16_t *px, uint32_t n)
{
    GPIO_TypeDef *A = GPIOA, *B = GPIOB;
    const uint32_t wr_lo = (uint32_t)WR_PIN << 16;
    while (n--) {
        uint16_t c = *px++;
        uint8_t lo = (uint8_t)c, hi = (uint8_t)(c >> 8);
        BYTE_OUT(lutA[lo] | wr_lo, lutB[lo]);   /* low byte first (this panel) */
        BYTE_OUT(lutA[hi] | wr_lo, lutB[hi]);
    }
}

__attribute__((optimize("O2")))
static void fill_fast(uint16_t color, uint32_t n)
{
    GPIO_TypeDef *A = GPIOA, *B = GPIOB;
    const uint32_t wr_lo = (uint32_t)WR_PIN << 16;
    const uint32_t a_lo = lutA[color & 0xFF] | wr_lo, b_lo = lutB[color & 0xFF];
    const uint32_t a_hi = lutA[color >> 8]   | wr_lo, b_hi = lutB[color >> 8];
    while (n--) {
        BYTE_OUT(a_lo, b_lo);
        BYTE_OUT(a_hi, b_hi);
    }
}

void tft_init(void)
{
    build_lut();
    tft_gpio_init();
    HAL_Delay(150);

    cmd(0x01); HAL_Delay(150);         /* software reset (RST is tied to 3V3) */
    cmd(0x11); HAL_Delay(150);         /* sleep out                 */
    cmd(0x3A); data(0x55);             /* 16-bit colour (RGB565)    */
    cmd(0x36); data(TFT_MADCTL);       /* orientation + BGR (tft.h) */
    cmd(0x29); HAL_Delay(20);          /* display ON                */
}

static void set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    cmd(0x2A); data(x0 >> 8); data(x0); data(x1 >> 8); data(x1);
    cmd(0x2B); data(y0 >> 8); data(y0); data(y1 >> 8); data(y1);
    cmd(0x2C);
}

void tft_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (x >= TFT_WIDTH || y >= TFT_HEIGHT) return;
    if (x + w > TFT_WIDTH)  w = TFT_WIDTH  - x;
    if (y + h > TFT_HEIGHT) h = TFT_HEIGHT - y;

    set_window(x, y, x + w - 1, y + h - 1);
    if (fast_mode) { fill_fast(color, (uint32_t)w * h); return; }

    uint8_t hi = color >> 8, lo = color & 0xFF;
    /* This panel expects the LOW byte of each pixel first (found with the stripe test) */
    for (uint32_t i = 0; i < (uint32_t)w * h; i++) { data(lo); data(hi); }
}

void tft_push_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1,
                     const uint16_t *px)
{
    set_window(x0, y0, x1, y1);
    uint32_t n = (uint32_t)(x1 - x0 + 1) * (uint32_t)(y1 - y0 + 1);
    if (fast_mode) { push_fast(px, n); return; }

    while (n--) {
        uint16_t c = *px++;
        data(c & 0xFF);                 /* low byte first: this panel's byte order */
        data(c >> 8);
    }
}

void tft_fill_screen(uint16_t color)
{
    tft_fill_rect(0, 0, TFT_WIDTH, TFT_HEIGHT, color);
}
