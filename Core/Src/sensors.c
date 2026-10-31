/* Step 2 – read the chip's internal temperature sensor and VREFINT with ADC4. */
#include "main.h"
#include "sensors.h"
#include "lvgl.h"

extern ADC_HandleTypeDef hadc4;        /* created by CubeMX in main.c */

/* Typical temperature sensor slope, used when the 130 C factory value is missing
 * (our engineering-sample chip has TS_CAL2 = 0xFFFF). Value in microvolts per deg C.
 * Typical for this ST sensor type; check the STM32WBA2x datasheet ("Avg_Slope") and
 * adjust if needed. Accuracy with single-point calibration: roughly +-2..3 C. */
#define TS_AVG_SLOPE_UV_PER_C   2500.0f

/* Smoothing of the displayed temperature (0..1, smaller = smoother) */
#define TEMP_SMOOTHING          0.3f

int sensors_cal_ok(void);

static float    temp_c, vdda_v, tmin = 1000.0f, tmax = -1000.0f;
static uint32_t samples;
static uint32_t last_raw_temp, last_raw_vref;
static int16_t  hist[SENSORS_HISTORY]; /* ring buffer, temperature x10 */
static uint32_t hist_head, hist_len;

/* Re-configure ADC4 for a 2-channel sequence: rank 1 = VREFINT, rank 2 = temperature.
 * Discontinuous mode: every HAL_ADC_Start() converts the next rank. */
static void adc_setup(void)
{
    ADC_ChannelConfTypeDef ch = {0};

    HAL_ADC_DeInit(&hadc4);
    hadc4.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV4;
    hadc4.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc4.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc4.Init.ScanConvMode          = ADC_SCAN_ENABLE;
    hadc4.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc4.Init.LowPowerAutoPowerOff  = DISABLE;
    hadc4.Init.LowPowerAutonomousDPD = ADC_LP_AUTONOMOUS_DPD_DISABLE;
    hadc4.Init.LowPowerAutoWait      = DISABLE;
    hadc4.Init.ContinuousConvMode    = DISABLE;
    hadc4.Init.NbrOfConversion       = 2;
    hadc4.Init.DiscontinuousConvMode = ENABLE;
    hadc4.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc4.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc4.Init.DMAContinuousRequests = DISABLE;
    hadc4.Init.TriggerFrequencyMode  = ADC_TRIGGER_FREQ_LOW;
    hadc4.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc4.Init.SamplingTimeCommon1   = ADC_SAMPLETIME_814CYCLES_5;  /* long: sensor needs time */
    hadc4.Init.SamplingTimeCommon2   = ADC_SAMPLETIME_814CYCLES_5;
    hadc4.Init.OversamplingMode      = DISABLE;
    if (HAL_ADC_Init(&hadc4) != HAL_OK) Error_Handler();

    ch.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
    ch.Channel = ADC_CHANNEL_VREFINT;     ch.Rank = ADC_REGULAR_RANK_1;
    if (HAL_ADC_ConfigChannel(&hadc4, &ch) != HAL_OK) Error_Handler();
    ch.Channel = ADC_CHANNEL_TEMPSENSOR;  ch.Rank = ADC_REGULAR_RANK_2;
    if (HAL_ADC_ConfigChannel(&hadc4, &ch) != HAL_OK) Error_Handler();

    if (HAL_ADCEx_Calibration_Start(&hadc4) != HAL_OK) Error_Handler();
    HAL_Delay(1);                          /* let the sensor / reference settle */
}

static uint32_t adc_read_next(void)
{
    uint32_t v = 0;
    HAL_ADC_Start(&hadc4);
    if (HAL_ADC_PollForConversion(&hadc4, 10) == HAL_OK)
        v = HAL_ADC_GetValue(&hadc4);
    return v;
}

static void sample(void)
{
    uint32_t raw_vref = adc_read_next();   /* rank 1 */
    uint32_t raw_temp = adc_read_next();   /* rank 2 */
    HAL_ADC_Stop(&hadc4);                  /* restart the sequence at rank 1 next time */
    last_raw_temp = raw_temp;
    last_raw_vref = raw_vref;
    if (raw_vref == 0) return;

    /* VDDA from the factory-calibrated internal reference */
    uint32_t vdda_mv = __LL_ADC_CALC_VREFANALOG_VOLTAGE(raw_vref, LL_ADC_RESOLUTION_12B);

    float cal1 = (float)*TEMPSENSOR_CAL1_ADDR;
    float cal2 = (float)*TEMPSENSOR_CAL2_ADDR;
    float t;

    if (sensors_cal_ok()) {
        /* Two-point: factory values at 30 C and 130 C
           (same formula as __LL_ADC_CALC_TEMPERATURE, but 0.1 C resolution) */
        float ts = (float)raw_temp * (float)vdda_mv / (float)TEMPSENSOR_CAL_VREFANALOG;
        t = (ts - cal1) * (float)(TEMPSENSOR_CAL2_TEMP - TEMPSENSOR_CAL1_TEMP) / (cal2 - cal1)
            + (float)TEMPSENSOR_CAL1_TEMP;
    } else {
        /* Single-point: factory value at 30 C + typical slope
           sensor voltage now   = raw  * VDDA / 4095
           sensor voltage at 30 = cal1 * 3.0 V / 4095 (calibrated with Vref+ = 3.0 V) */
        float v_now_uv = (float)raw_temp * (float)vdda_mv * 1000.0f / 4095.0f;
        float v_30_uv  = cal1 * (float)TEMPSENSOR_CAL_VREFANALOG * 1000.0f / 4095.0f;
        t = (float)TEMPSENSOR_CAL1_TEMP + (v_now_uv - v_30_uv) / TS_AVG_SLOPE_UV_PER_C;
    }

    /* light smoothing so the value and chart don't jitter by one ADC step */
    temp_c = (samples == 0) ? t : temp_c + TEMP_SMOOTHING * (t - temp_c);
    t = temp_c;
    vdda_v = (float)vdda_mv / 1000.0f;
    if (t < tmin) tmin = t;
    if (t > tmax) tmax = t;
    samples++;

    hist[hist_head] = (int16_t)(t * 10.0f);
    hist_head = (hist_head + 1) % SENSORS_HISTORY;
    if (hist_len < SENSORS_HISTORY) hist_len++;
}

static void sample_timer_cb(lv_timer_t *t)
{
    (void)t;
    sample();
}

void sensors_init(void)
{
    adc_setup();
    sample();                               /* first value right away */
    lv_timer_create(sample_timer_cb, 1000, NULL);
}

float    sensors_temp_c(void)       { return temp_c; }
float    sensors_vdda_v(void)       { return vdda_v; }
float    sensors_temp_min(void)     { return tmin; }
float    sensors_temp_max(void)     { return tmax; }
uint32_t sensors_sample_count(void) { return samples; }
uint32_t sensors_raw_temp(void)     { return last_raw_temp; }
uint32_t sensors_raw_vref(void)     { return last_raw_vref; }
uint16_t sensors_ts_cal1(void)      { return *TEMPSENSOR_CAL1_ADDR; }
uint16_t sensors_ts_cal2(void)      { return *TEMPSENSOR_CAL2_ADDR; }
uint16_t sensors_vrefint_cal(void)  { return *VREFINT_CAL_ADDR; }

/* Blank flash reads 0xFFFF; a real sensor gives a higher value at 130 C than at 30 C */
int sensors_cal_ok(void)
{
    uint16_t c1 = sensors_ts_cal1(), c2 = sensors_ts_cal2();
    return c1 != 0 && c1 != 0xFFFF && c2 != 0 && c2 != 0xFFFF && c2 > c1 + 100;
}

uint32_t sensors_history_x10(int32_t *out, uint32_t max)
{
    uint32_t n = hist_len < max ? hist_len : max;
    uint32_t start = (hist_head + SENSORS_HISTORY - n) % SENSORS_HISTORY;
    for (uint32_t i = 0; i < n; i++)
        out[i] = hist[(start + i) % SENSORS_HISTORY];
    return n;
}
