#ifndef SENSORS_H
#define SENSORS_H
#include <stdint.h>

/* Internal sensors read with ADC4 once per second (LVGL timer, always running):
 *   - chip temperature (internal temperature sensor, factory calibrated)
 *   - supply voltage VDDA (from the internal reference VREFINT)
 * Call after lvgl_port_init(). Uses hadc4 set up by CubeMX (MX_ADC4_Init). */

#define SENSORS_HISTORY  60      /* seconds of temperature history kept */

void     sensors_init(void);
float    sensors_temp_c(void);          /* latest temperature, deg C   */
float    sensors_vdda_v(void);          /* latest supply voltage, V    */
float    sensors_temp_min(void);        /* min / max since start       */
float    sensors_temp_max(void);
uint32_t sensors_sample_count(void);    /* number of samples taken     */
/* Diagnostics: raw ADC values and the factory calibration values */
uint32_t sensors_raw_temp(void);
uint32_t sensors_raw_vref(void);
uint16_t sensors_ts_cal1(void);         /* raw temp sensor value at 30 C  */
uint16_t sensors_ts_cal2(void);         /* raw temp sensor value at 130 C */
uint16_t sensors_vrefint_cal(void);
int      sensors_cal_ok(void);          /* 1 = factory calibration looks valid */
/* Copy history (oldest first) as temperature x10, returns number of values. */
uint32_t sensors_history_x10(int32_t *out, uint32_t max);

#endif
