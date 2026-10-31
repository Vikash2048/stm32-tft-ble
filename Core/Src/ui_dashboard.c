/* Step 2 – Dashboard: live chip temperature, supply voltage, min/max and a 60 s chart. */
#include "ui.h"
#include "sensors.h"

LV_FONT_DECLARE(lv_font_montserrat_28_deg);   /* our generated font: ASCII + degree sign */

static lv_obj_t          *temp_label, *vdda_label, *minmax_label, *diag_label;
static lv_obj_t          *chart;
static lv_chart_series_t *series;
static lv_timer_t        *refresh_timer;
static uint32_t           shown_samples;

/* Print a float with one decimal (LVGL's printf has no %f) */
static void fmt1(char *buf, size_t n, const char *pre, float v, const char *post)
{
    int32_t x10 = (int32_t)(v * 10.0f + (v >= 0 ? 0.5f : -0.5f));
    int32_t a = x10 / 10, b = x10 % 10;
    if (b < 0) b = -b;
    lv_snprintf(buf, n, "%s%s%ld.%ld%s", pre, (x10 < 0 && a == 0) ? "-" : "",
                (long)a, (long)b, post);
}

static void fmt2(char *buf, size_t n, const char *pre, float v, const char *post)
{
    int32_t x100 = (int32_t)(v * 100.0f + 0.5f);
    lv_snprintf(buf, n, "%s%ld.%02ld%s", pre, (long)(x100 / 100), (long)(x100 % 100), post);
}

/* Fit the chart's Y range around the data (with 1 C margin) */
static void chart_reload(void)
{
    int32_t v[SENSORS_HISTORY];
    uint32_t n = sensors_history_x10(v, SENSORS_HISTORY);
    if (n == 0) return;

    int32_t lo = v[0], hi = v[0];
    for (uint32_t i = 0; i < n; i++) { if (v[i] < lo) lo = v[i]; if (v[i] > hi) hi = v[i]; }
    lv_chart_set_axis_range(chart, LV_CHART_AXIS_PRIMARY_Y, lo - 10, hi + 10);

    lv_chart_set_all_values(chart, series, LV_CHART_POINT_NONE);
    for (uint32_t i = 0; i < n; i++) lv_chart_set_next_value(chart, series, v[i]);
    lv_chart_refresh(chart);
}

static void refresh(void)
{
    char buf[40];
    fmt1(buf, sizeof buf, "", sensors_temp_c(), " \xC2\xB0" "C");   /* UTF-8 degree sign */
    lv_label_set_text(temp_label, buf);
    fmt2(buf, sizeof buf, "VDD ", sensors_vdda_v(), " V");
    lv_label_set_text(vdda_label, buf);
    char a[16], b[16];
    fmt1(a, sizeof a, "", sensors_temp_min(), "");
    fmt1(b, sizeof b, "", sensors_temp_max(), "");
    lv_label_set_text_fmt(minmax_label, "min %s  max %s", a, b);

    /* Diagnostics: raw ADC value of the sensor + factory calibration (30 C / 130 C) */
    lv_label_set_text_fmt(diag_label, "raw %lu   cal30 %u  cal130 %u%s",
                          (unsigned long)sensors_raw_temp(),
                          (unsigned)sensors_ts_cal1(), (unsigned)sensors_ts_cal2(),
                          sensors_cal_ok() ? "  2-pt" : "  1-pt cal");

    if (sensors_sample_count() != shown_samples) {   /* new sample -> redraw chart */
        shown_samples = sensors_sample_count();
        chart_reload();
    }
}

static void refresh_timer_cb(lv_timer_t *t) { (void)t; refresh(); }

static void screen_deleted_cb(lv_event_t *e)
{
    (void)e;
    lv_timer_delete(refresh_timer);
    refresh_timer = NULL;
}

void ui_show_dashboard(void)
{
    lv_obj_t *scr = ui_new_screen(NULL);
    lv_obj_add_event_cb(scr, screen_deleted_cb, LV_EVENT_DELETE, NULL);
    lv_obj_t *back = ui_add_back_button(scr);

    lv_obj_t *title = lv_label_create(scr);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFD000), 0);
    lv_label_set_text(title, "Chip temperature");
    lv_obj_align(title, LV_ALIGN_TOP_RIGHT, -8, 14);

    /* Big temperature value */
    temp_label = lv_label_create(scr);
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_28_deg, 0);
    lv_obj_set_style_text_color(temp_label, lv_color_hex(0xFF8040), 0);   /* orange */
    lv_obj_set_pos(temp_label, 10, 48);

    /* Supply voltage and min/max on the right */
    vdda_label = lv_label_create(scr);
    lv_obj_set_style_text_font(vdda_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(vdda_label, lv_color_hex(0x00D0FF), 0);   /* cyan */
    lv_obj_align(vdda_label, LV_ALIGN_TOP_RIGHT, -8, 46);

    minmax_label = lv_label_create(scr);
    lv_obj_set_style_text_font(minmax_label, &lv_font_montserrat_14, 0);
    lv_obj_align(minmax_label, LV_ALIGN_TOP_RIGHT, -8, 72);

    diag_label = lv_label_create(scr);
    lv_obj_set_style_text_font(diag_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(diag_label, lv_color_hex(0x909090), 0);
    lv_obj_set_pos(diag_label, 10, 82);

    /* Line chart: last 60 seconds */
    chart = lv_chart_create(scr);
    lv_obj_set_size(chart, 304, 132);
    lv_obj_align(chart, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    lv_chart_set_point_count(chart, SENSORS_HISTORY);
    lv_chart_set_update_mode(chart, LV_CHART_UPDATE_MODE_SHIFT);
    lv_chart_set_div_line_count(chart, 4, 6);
    lv_obj_set_style_bg_color(chart, lv_color_hex(0x101010), 0);
    lv_obj_set_style_line_color(chart, lv_color_hex(0x404040), LV_PART_MAIN);   /* grid */
    lv_obj_set_style_size(chart, 0, 0, LV_PART_INDICATOR);                      /* no dots */
    lv_obj_set_style_line_width(chart, 3, LV_PART_ITEMS);
    series = lv_chart_add_series(chart, lv_color_hex(0xFF8040), LV_CHART_AXIS_PRIMARY_Y);

    shown_samples = 0;
    refresh();
    refresh_timer = lv_timer_create(refresh_timer_cb, 500, NULL);

    ui_load(scr, back);
}
