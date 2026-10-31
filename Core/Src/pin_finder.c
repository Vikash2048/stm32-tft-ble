/* PIN FINDER - shows which STM32 pin sits behind each Arduino header pin.
 *
 * Use with the TFT shield REMOVED:
 *   Debug -> Resume -> Live Expressions: add  found_port  found_pin  found_count
 *   Touch a jumper wire from a GND pin to an Arduino pin (A0..A5, D0..D15).
 *   found_port='A', found_pin=7  ->  PA7.  found_count goes up on every new hit.
 *   If nothing changes for a header pin, that pin has no GPIO behind it.
 *
 * Only ever touch pins with GND, never with 5V or 3V3.
 */
#include "main.h"
#include "pin_finder.h"

volatile char     found_port  = '-';
volatile int      found_pin   = -1;
volatile uint32_t found_count = 0;

typedef struct { GPIO_TypeDef *p; char name; uint16_t skip; } port_t;

static const port_t PORTS[] = {
    {GPIOA, 'A', GPIO_PIN_13 | GPIO_PIN_14},   /* SWD debugger - must not touch */
    {GPIOB, 'B', 0},
    {GPIOC, 'C', GPIO_PIN_14 | GPIO_PIN_15},   /* 32 kHz crystal */
    {GPIOH, 'H', 0},
};
#define NPORTS (sizeof(PORTS) / sizeof(PORTS[0]))

static uint16_t watch[NPORTS];
static uint16_t base[NPORTS];

/* raw input registers - add these to Live Expressions too (shown in hex) */
volatile uint16_t idr_a, idr_b, idr_c, idr_h;

void finder_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    /* stop button interrupts from firing while we probe */
    HAL_NVIC_DisableIRQ(EXTI1_IRQn);
    HAL_NVIC_DisableIRQ(EXTI2_IRQn);
    HAL_NVIC_DisableIRQ(EXTI13_IRQn);

    GPIO_InitTypeDef g = {0};
    g.Mode  = GPIO_MODE_INPUT;
    g.Pull  = GPIO_PULLUP;
    g.Speed = GPIO_SPEED_FREQ_LOW;

    for (unsigned i = 0; i < NPORTS; i++) {
        uint16_t pins = (uint16_t)(0xFFFFu & ~PORTS[i].skip);
        g.Pin = pins;
        HAL_GPIO_Init(PORTS[i].p, &g);
        watch[i] = pins;
    }
    HAL_Delay(50);
    for (unsigned i = 0; i < NPORTS; i++)
        base[i] = (uint16_t)PORTS[i].p->IDR;   /* resting state */
}

void finder_poll(void)
{
    idr_a = (uint16_t)GPIOA->IDR;
    idr_b = (uint16_t)GPIOB->IDR;
    idr_c = (uint16_t)GPIOC->IDR;
    idr_h = (uint16_t)GPIOH->IDR;

    for (unsigned i = 0; i < NPORTS; i++) {
        /* any pin that differs from its resting state */
        uint16_t diff = (uint16_t)(PORTS[i].p->IDR ^ base[i]) & watch[i];
        if (diff) {
            int pin = __builtin_ctz(diff);
            if (found_port != PORTS[i].name || found_pin != pin) {
                found_port = PORTS[i].name;
                found_pin  = pin;
                found_count++;
            }
        }
    }
    HAL_Delay(10);
}
