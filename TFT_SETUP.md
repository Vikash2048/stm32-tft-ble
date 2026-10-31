# TFT Shield on NUCLEO-WBA25CE1 – Setup Notes

**Status (30 Oct 2026):** Working. LVGL v9.6.0 runs on the screen in landscape 320×240 with correct orientation and colours (high-contrast test screen).

| Item | Details |
|---|---|
| Dev board | ST **NUCLEO-WBA25CE1** = MCU RF board **MB2293B** (STM32WBA25CEU7) on base board **MB1801D** |
| MCU | STM32WBA25CEU7, UFQFPN48, 512 KB flash, 96 KB RAM |
| Display | SIMKEIM "tftlcd for arduino uno (spiflash)" shield, 2.4"/2.8", ILI9341-class controller |
| Display interface | **8-bit parallel, Intel 8080 (i80) bus** – not SPI. SD-card slot and flash chip on the shield use SPI (not used) |
| Tools | STM32CubeMX (project generation) + STM32CubeIDE (build / debug) |
| Project folder | `C:\work\stm_projects\tft_screen_impl` |
| Reference | Board user manual **UM3610 Rev 2** – Table 12 (Arduino pinout), Table 13 (ST morpho pinout) |

---

## 1. What we did, step by step

1. **Identified the hardware.** The shield is a parallel (i80) display that normally needs 13 MCU pins: D0–D7, RD, WR, RS, CS, RST.
2. **Created the project in STM32CubeMX.** Board Selector → NUCLEO-WBA25CE1 → default peripherals → Toolchain = STM32CubeIDE → Generate Code → opened in CubeIDE. (CubeIDE 2.x no longer contains CubeMX; its "Empty Project" has no HAL/main.c.)
3. **Fixed the debugger connection.** "No device found on target" was solved by seating the MB2293B RF board properly and connecting.
4. **Tried stacking the shield on top – it cannot work on this board.** The STM32WBA25 (48-pin) has only **26 GPIOs**, and most are already used on the board. Arduino pins A0–A5 and most of D0–D9 are **not connected** to the MCU.
5. **Wrote a pin-finder program** (`pin_finder.c`): all pins become inputs; touching a header pin with a GND or 3V3 wire shows the STM32 port/pin in Live Expressions (`found_port`, `found_pin`, `found_count`).
6. **Mapped the real pins** with the pin finder and the user manual. Several pins listed in ST's example readmes were wrong for this board revision (see section 4).
7. **Reduced the display to 10 signal wires** by tying CS → GND, RD → 3.3 V, RST → 3.3 V (the code resets the display with a software command instead).
8. **Wrote our own minimal driver** (`tft.c`, no external library, only ST HAL) and wired the shield with jumper wires.
9. **Ran the colour test – working.**

---

## 2. Final wiring (TFT shield → board)

Connect with **USB unplugged**. Board orientation: USB connectors at the bottom.

### Signal pins

| # | TFT shield pin | Function | STM32 pin | Board connector & pin | Where on the board |
|---|---|---|---|---|---|
| 1 | LCD_D0 | Data bit 0 | **PA9** | **CN6 pin 4** | Right upper inner header, 7th hole from top |
| 2 | LCD_D1 | Data bit 1 | **PA10** | **CN6 pin 5** | Right upper inner header, 6th hole from top |
| 3 | LCD_D2 | Data bit 2 | **PA8** | **CN6 pin 6** | Right upper inner header, 5th hole from top |
| 4 | LCD_D3 | Data bit 3 | **PA11** | **CN6 pin 9** | Right upper inner header, 2nd hole from top |
| 5 | LCD_D4 | Data bit 4 | **PB2** | **CN6 pin 10** | Right upper inner header, 1st hole from top |
| 6 | LCD_D5 | Data bit 5 | **PA7** (also LED LD1) | **CN4 pin 6** | Right outer pin row |
| 7 | LCD_D6 | Data bit 6 | **PB15** (also LED LD3) | **CN4 pin 38** | Right outer pin row |
| 8 | LCD_D7 | Data bit 7 | **PA6** | **CN3 pin 37** | Left outer pin row |
| 9 | LCD_WR | Write strobe | **PA5** | **CN6 pin 3** | Right upper inner header, 8th hole from top |
| 10 | LCD_RS | Command (0) / Data (1) | **PC13** (also button B2) | **CN4 pin 30** | Right outer pin row |

### Tied pins and power

| TFT shield pin | Connect to | Board connector & pin | Why |
|---|---|---|---|
| LCD_CS | GND | **CN5 pin 7** | Display always selected |
| LCD_RD | 3.3 V (IOREF) | **CN5 pin 2** | We never read from the display |
| LCD_RST | 3.3 V | **CN3 pin 16** | Reset done in software (command 0x01) |
| 3V3 | 3.3 V | **CN5 pin 4** | Shield logic power |
| 5V | 5 V | **CN5 pin 5** | Backlight / shield power |
| GND | GND | **CN5 pin 6** | Ground |
| SD_SS, SD_DI, SD_DO, SD_SCK | not connected | – | SD card not used |

### Board header reference (USB at the bottom)

| Header | Position | Pins (top → bottom) |
|---|---|---|
| **CN5** (8 holes) | Left, upper inner | 1 NC, 2 IOREF (3.3 V), 3 NRST, 4 3V3, 5 5V, 6 GND, 7 GND, 8 VIN |
| **CN7** (6 holes) | Left, lower inner | A0 … A5 (not connected to MCU by default) |
| **CN6** (10 holes) | Right, upper inner | pin 10 at top → pin 1 at bottom (D15 … D8) |
| **CN8** (8 holes) | Right, lower inner | pin 8 at top → pin 1 at bottom (D7 … D0) |
| **CN3** | Left outer long pin row (ST morpho) | numbers printed beside the pins |
| **CN4** | Right outer long pin row (ST morpho) | numbers printed beside the pins |

> **Caution:** the outer rows (CN3/CN4) also carry 5V, 3V3, VIN and GND. Connect exactly to the numbered pin; never slide a wire along them while powered.

---

## 3. Pin budget – why only 10 wires

STM32WBA25CEU7 GPIOs (26 total): PA1, PA2, PA5–PA15, PB2–PB6, PB8, PB9, PB12, PB15, PC13–PC15, PH3.

| Already used by the board | Pins |
|---|---|
| Debugger (SWD) | PA13, PA14 |
| On-board QSPI flash | PA2, PA15, PB3, PB4, PB5, PB6 |
| USB | PB8, PB9 |
| ST-LINK virtual COM (VCP) RX | PA12 |
| 32 kHz crystal, BOOT0 | PC14, PC15, PH3 |

Used for the display: PA5, PA6, PA7, PA8, PA9, PA10, PA11, PB2, PB15, PC13.
Still free: **PA1** (button B1, CN4 pin 36 / CN3 pin 30) and **PB12** (LED LD2).

**Side effects of this wiring**
- LEDs LD1 (PA7) and LD3 (PB15) may flicker while the screen draws.
- Do not press **B2** while the display runs (shares PC13 with LCD_RS).
- **PA6 is the VCP TX line**, so UART printf to the PC no longer works.

---

## 4. Pin-finder results (measured on the real board)

| Header pin | Result | Note |
|---|---|---|
| CN6 pin 3 | PA5 | ✓ |
| CN6 pin 2 (D9) | nothing | Readme says PA6 – not connected on this board |
| CN3 pin 37 | PA6 | ✓ |
| CN4 pin 30 | PC13 | Readme said CN4 pin 36 |
| CN4 pin 6 | PA7 | Detect with 3V3 (LED pin rests low) |
| CN4 pin 38 | PB15 | Detect with 3V3 (LED pin rests low) |
| CN4 pin 36 | PA1 | ✓ |
| CN8 pin 4 / pin 7 | nothing | Readme says PB12 / PA7 – need solder bridges |
| CN7 holes 1–6, CN8 holes 1–8 | nothing | Arduino nets not connected by default |

**Lesson:** on this board, the "Arduino" positions often need solder bridges; the ST morpho rows (CN3/CN4) are the reliable place to reach MCU pins. Trust the measured results over example readmes.

---

## 5. Software

### Files in the project

| File | Purpose |
|---|---|
| `Core/Src/tft.c`, `Core/Inc/tft.h` | Our display driver (no library, ST HAL only) |
| `Core/Src/pin_finder.c`, `Core/Inc/pin_finder.h` | Pin-finder tool |
| `Core/Src/main.c` | Calls the driver; switch `APP_PIN_FINDER` |

### Mode switch (`main.c`, USER CODE PD section)

```c
#define APP_MODE  APP_LVGL   /* APP_PIN_FINDER (shield removed), APP_COLOR_TEST or APP_LVGL */
```

### Driver API (`tft.h`)

```c
void tft_init(void);                                      /* configure pins + init display */
void tft_fill_rect(uint16_t x, uint16_t y,
                   uint16_t w, uint16_t h, uint16_t color);
void tft_fill_screen(uint16_t color);
/* colours (RGB565): BLACK WHITE RED GREEN BLUE YELLOW CYAN MAGENTA */
/* size/orientation: TFT_WIDTH 320, TFT_HEIGHT 240, TFT_MADCTL 0x28 (landscape) */
void tft_push_pixels(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, const uint16_t *px);
```

### How the driver works

- **Bus:** for every byte – set RS (0 = command, 1 = data) → put the byte on D0–D7 → pulse WR low → high (display latches on the rising edge).
- **Fast writes:** data pins are grouped per port, so a byte is written with one `BSRR` write to GPIOA and one to GPIOB.
- **RS on PC13:** PC13 is a low-speed pin shared with button B2, so a short delay follows every RS change (`short_delay()`, raise 100 → 1000 if the picture is garbled).
- **Init sequence (MIPI DCS commands):** `0x01` software reset → `0x11` sleep out → `0x3A 0x55` 16-bit RGB565 → `0x36 0x48` portrait/BGR → `0x29` display on.
- **Drawing:** `0x2A` column range, `0x2B` row range, `0x2C` then 2 bytes per pixel.

### Demo loop (`main.c`, USER CODE 3)

```c
tft_fill_screen(RED);   HAL_Delay(500);
tft_fill_screen(GREEN); HAL_Delay(500);
tft_fill_screen(BLUE);  HAL_Delay(500);
tft_fill_screen(BLACK);
tft_fill_rect(20, 20, 100, 60, YELLOW);
tft_fill_rect(60, 120, 120, 120, CYAN);
tft_fill_rect(0, 280, 240, 40, WHITE);
HAL_Delay(2000);
```

All user code is inside CubeMX `USER CODE` blocks, so regenerating from the `.ioc` keeps it.

---

## 6. How to build and run

1. STM32CubeIDE → select project → **F5** (refresh).
2. **Build** (hammer) → `0 errors`.
3. **Run** (green ▶). For Live Expressions you need **Debug** (bug icon) + **F8** (resume).

## 7. Troubleshooting

| Symptom | Check |
|---|---|
| No backlight | 5V and GND wires |
| White screen only | WR (CN6-3), RS (CN4-30), CS→GND, RD and RST on 3.3 V |
| Wrong / noisy colours | D0–D7 wiring (swapped or loose wire); pixel byte order (low byte first on this panel) |
| Garbled picture | Increase `short_delay()` loop in `tft.c` (100 → 1000) |
| Image covers only part of the screen | 3.5" display: set `TFT_WIDTH 320`, `TFT_HEIGHT 480` |
| HardFault right after LVGL starts | Stack too small: `_Min_Stack_Size` must be ≥ 0x2000 |
| Debugger: "No device found on target" | Re-seat MB2293B, use ST-LINK USB-C, JP1 on 5V_STLK, Connect under reset, update ST-LINK firmware |

## 8. LVGL (added 30 Oct 2026)

| Item | Details |
|---|---|
| Version | **LVGL v9.6.0** (from GitHub release tarball) |
| Location | **`Core/lvgl/`** – only `src/`, `include/`, headers and licence (no demos/examples/docs, no C++/asm parts: `src/libs/thorvg`, `src/libs/gltf`, `src/debugging/vg_lite_tvg`, `src/draw/sw/blend/helium`, `*.cpp` left out) |
| Config | `Core/Inc/lv_conf.h` (copied from `lv_conf_template.h`) |
| Port | `Core/Src/lvgl_port.c` – display driver glue |
| First screen | `Core/Src/ui.c` |
| Orientation | Landscape **320 × 240** (`TFT_MADCTL 0x28` in `tft.h`) |

### CubeIDE build settings changed (`.cproject`, Debug and Release)
- Include path **`../Core/lvgl`**.
- Define `LV_CONF_INCLUDE_SIMPLE`.
- Debug optimisation changed from `-O0` to **`-Og`** (LVGL is much smaller/faster; still debuggable).
- No extra source folder needed: `Core` is already compiled, and the unused LVGL parts are simply not in the project.

> **Lesson learned (30 Oct 2026):** LVGL was first placed in `Middlewares/Third_Party/lvgl`. When CubeMX regenerated code (adding ADC4), it **deleted the whole `Middlewares` folder** and removed its source entry from `.cproject`. CubeMX owns `Middlewares/` and the list of source folders, but leaves `Core/` user files and the include paths/defines alone. So third-party libraries go in **`Core/`**. After any CubeMX "Generate Code", check that `Core/lvgl` still exists and the include path `../Core/lvgl` is still set.

### `lv_conf.h` changes from the template
| Setting | Value | Why |
|---|---|---|
| `LV_MEM_SIZE` | 40 KB | LVGL heap, fits in 96 KB RAM |
| `LV_COLOR_FORMAT_DEFAULT` | RGB565 | Matches the display (16-bit) |
| `LV_DEF_REFR_PERIOD` | 20 ms | Refresh up to ~50 Hz (limited by the bus) |
| Fonts | Montserrat 14, 20, 28 | Small / normal / title |
| Draw formats disabled | RGB888, XRGB8888, ARGB8888 premultiplied, L8, AL88, I1 | Save flash |
| Widgets disabled | calendar, keyboard, menu, msgbox, tabview, tileview, win, spinbox, imagebutton, roller, table, dropdown, list, arclabel, animimg, canvas, textarea, span | Save flash/RAM |
| `LV_BUILD_EXAMPLES`, `LV_BUILD_DEMOS` | 0 | Not needed |

### How the port works (`lvgl_port.c`)
- `lv_init()`, tick from `HAL_GetTick()` via `lv_tick_set_cb()`.
- `lv_display_create(320, 240)`, RGB565, **partial render** buffer 320 × 24 lines (15 KB).
- `flush_cb` → `tft_push_pixels()` (sets the window, sends 2 bytes per pixel) → `lv_display_flush_ready()`.
- Main loop: `lv_timer_handler(); HAL_Delay(5);`

### Driver changes for LVGL (`tft.c` / `tft.h`)
- New `tft_push_pixels(x0, y0, x1, y1, pixels)`.
- **Lookup tables** (`lutA`/`lutB`, 2 KB RAM): each byte is now 2 register writes (+1 for WR) instead of a loop → much faster pixel output.
- Orientation via `TFT_MADCTL` (0x28 landscape). If the image is mirrored/upside down try `0xE8`, `0x68` or `0xA8`.

### Mode switch (`main.c`)
```c
#define APP_MODE  APP_LVGL      /* APP_PIN_FINDER, APP_COLOR_TEST or APP_LVGL */
```

### First LVGL screen (`ui.c`)
Black background, white title "Hello LVGL" (Montserrat 28), yellow subtitle (20), green progress bar with white border animating 0 → 100 → 0 with a percent label (28), and a cyan "Uptime: N s" counter (28) updated every second by an `lv_timer`.

### Stack size – important
The Cortex-M33 startup code sets the hardware stack limit (`MSPLIM`) to `_Min_Stack_Size`. CubeMX's default is **0x400 (1 KB)**, which LVGL overflows immediately → **HardFault**.
Changed to **0x2000 (8 KB)** in `STM32WBA25CEUX_FLASH.ld`, `STM32WBA25CEUX_RAM.ld` and in the `.ioc` (`ProjectManager.StackSize`, CubeMX → Project Manager → Linker Settings → Minimum Stack Size), so regenerating keeps it.

### Display quirks found on this panel
- **Orientation:** `TFT_MADCTL 0xE8` gives the correct landscape image for our mounting (0x28 was upside down).
- **Pixel byte order:** the panel expects the **low byte of each RGB565 pixel first**. Found with a 6-stripe test (sent R,G,B,W,K,navy → saw blue, magenta, green, white, dark, light). `tft_fill_rect` and `tft_push_pixels` send low byte, then high byte. Command parameters (window coordinates) stay high byte first.
- Black looks dark blue on this TN panel and contrast depends on viewing angle; the UI uses a black background, white/bright colours and 20–28 px fonts for readability.
- The 6-stripe colour test was used once for diagnosis and has been removed from `main.c`.

### Build check (ARM GCC 14.2, `-Og`)
433 C files compile with no errors or warnings. Size: **flash 259 KB of 512 KB**, **RAM 61 KB of 96 KB** static (rest is stack).

## 9. Feature steps (each one stays in the code, reachable from the menu)

The app starts on a **home menu**. Every feature is its own screen (`ui_*.c`) with a **"< Menu"** button. New features are added to `MENU[]` in `ui_home.c`; nothing is removed.

### Button input (B1)
| Action on B1 (PA1) | LVGL key | Effect |
|---|---|---|
| Tap (< 0.5 s) | `LV_KEY_NEXT` | Focus moves to the next item (outline shows focus) |
| Hold (≥ 0.5 s) | `LV_KEY_ENTER` | Click / toggle the focused item |

Implemented in `Core/Src/input_port.c` as an LVGL **keypad** input device, polled with `BSP_PB_GetState(B1)` (B1 interrupt disabled). All focusable widgets join the default group automatically.

### UI files
| File | Content |
|---|---|
| `ui.c` / `ui.h` | Helpers: `ui_new_screen()`, `ui_add_back_button()`, `ui_load()` (loads screen, deletes old one), `ui_go()` (screen switch from an event, done async) |
| `ui_home.c` | Menu – list of features |
| `ui_hello.c` | 1. Hello LVGL – title, animated bar, uptime counter |
| `ui_buttons.c` | 2. Button demo – push button + counter, switch → green LED **LD2 (PB12)**, checkbox, status line |
| `ui_dashboard.c` | 3. Dashboard – live chip temperature (big), supply voltage, min/max, 60 s line chart |
| `sensors.c` / `sensors.h` | ADC4 reading of the internal temperature sensor + VREFINT every second (always running, keeps 60 s history) |
| `ui_speed.c` | 4. Speed test – shows CPU clock + instruction-cache state, measures a full-screen redraw via LVGL and via the raw driver (5 frames each) |

Dark default theme (`lv_theme_default_init(..., dark = true, Montserrat 20)`) set in `lvgl_port.c`.
Screens that use timers delete them on `LV_EVENT_DELETE` (otherwise the timer would update a deleted label → crash).

| Step | Feature | Status |
|---|---|---|
| 1 | Button control with B1 + home menu | Added 30 Oct 2026 – working |
| 2 | Dashboard: chip temperature + supply voltage + 60 s chart (ADC4) | Added 30 Oct 2026 – working |
| 3 | Faster drawing: speed-test screen, fast driver, instruction cache (64 MHz is the WBA25 maximum) | Added 30 Oct 2026 – working (5 → 11.6 fps) |
| 4 | Bluetooth (BLE) data on screen | Planned |

### Step 2 details – ADC4 and the internal sensors
1. **CubeMX:** Analog → ADC4 → tick *Temperature Sensor Channel* (and *Vrefint Channel*) → Generate Code. This adds the HAL ADC driver files, `HAL_ADC_MODULE_ENABLED`, `MX_ADC4_Init()` (in `main.c`) and the ADC clock in `stm32wbaxx_hal_msp.c` (`RCC_ADCCLKSOURCE_HCLK`).
2. **`sensors_init()`** re-initialises ADC4 our way: clock HCLK/4, 12-bit, sequence of 2 ranks (1 = VREFINT, 2 = temperature sensor), discontinuous mode (each `HAL_ADC_Start()` converts the next rank), sampling time 814.5 cycles (the sensor needs a long sample), then runs the ADC calibration.
3. **Every second** (`lv_timer`): read both ranks, then
   - VDDA (mV) = `__LL_ADC_CALC_VREFANALOG_VOLTAGE(raw_vrefint, 12 bit)` – uses the factory value `VREFINT_CAL`.
   - Temperature = `(raw_temp × VDDA / 3000 − TS_CAL1) × (130 − 30) / (TS_CAL2 − TS_CAL1) + 30` – the two factory calibration points at 30 °C and 130 °C, computed in float for 0.1 °C resolution.
4. History: 60 values in a ring buffer (temperature × 10). The dashboard redraws the chart when a new sample arrives and fits the Y range to the data ±1 °C.
- **Degree sign:** LVGL's built-in Montserrat fonts contain only ASCII + LVGL symbols, no `°`. We generated our own font with LVGL's converter:
  `npx lv_font_conv@1.5.3 --font Montserrat-Medium.ttf --size 28 --bpp 4 -r 0x20-0x7F,0xB0 --format lvgl --no-compress --lv-include lvgl.h --lv-font-name lv_font_montserrat_28_deg -o lv_font_montserrat_28_deg.c`
  (TTF from github.com/JulietaUla/Montserrat). The file is `Core/Src/lv_font_montserrat_28_deg.c` (~14 KB flash); use it with `LV_FONT_DECLARE(lv_font_montserrat_28_deg);` and write `°` as UTF-8 `"\xC2\xB0"`. Add more characters by adding them to `-r`.
- **Our ES chip has only half the factory calibration:** `cal30` (TS_CAL1) = 1021 is valid, but `cal130` (TS_CAL2) = **65535 (blank)**. With the two-point formula this divided by 64514 instead of ~400, so the result was stuck at 30.0 °C.
  **Fix:** `sensors.c` checks the values (`sensors_cal_ok()`); if TS_CAL2 is blank it uses **single-point calibration**: T = 30 + (V_now − V_30) / Avg_Slope, with V_now = raw × VDDA / 4095, V_30 = TS_CAL1 × 3.0 V / 4095 and typical **Avg_Slope = 2.5 mV/°C** (`TS_AVG_SLOPE_UV_PER_C`, check the WBA2x datasheet). Expected accuracy ±2–3 °C. A light smoothing filter (`TEMP_SMOOTHING` 0.3) removes 1-LSB jitter (1 LSB ≈ 0.3 °C).
- **Diagnostics line** on the dashboard: raw ADC value + `cal30` / `cal130` and the mode: "2-pt" (both factory values valid) or "1-pt cal" (single-point fallback).
- Try it: touch the chip (the metal-can module on the blue board) – the temperature rises within seconds.

### How the temperature measurement works (explained)

#### 1. The temperature sensor
The STM32 has a small circuit inside the chip whose **output voltage rises in a straight line with temperature**, about **2.5 mV per °C**:

```
voltage
  │                      ╱
  │                   ╱      slope ≈ 2.5 mV per °C
0.75 V ─ ─ ─ ─ ─ ─ ●   ← at 30 °C (measured in the factory)
  │            ╱
  └──────────┼──────────── temperature
            30 °C
```

Knowing the voltage at **one known temperature** and the **change per °C** gives any temperature from the voltage.
It measures the **silicon die**, not the room – a running chip is usually a few degrees warmer than the air.

#### 2. The ADC
ADC4 converts a voltage into a number. It is **12-bit → 0 … 4095**:

```
ADC number = input voltage / VDDA × 4095
```

**VDDA (the supply voltage) is the ADC's ruler.** The same voltage gives a different number when VDDA changes, so VDDA must be known first.

#### 3. Measuring VDDA with VREFINT
The chip has an internal reference voltage **VREFINT** (~1.2 V, stable). ST measured it in the factory with exactly **3.0 V** supply and stored the result as **VREFINT_CAL**. Measuring it again now:

```
VDDA = VREFINT_CAL × 3000 mV / raw_vrefint
```

If today's reading is smaller than the factory one, the supply is higher than 3.0 V. Our board: **3.30 V**.

#### 4. The values on the dashboard

| On screen | What it is | Our chip |
|---|---|---|
| `raw` | ADC number from the temperature sensor now (ruler = 3.3 V) | ≈ 933 |
| `cal30` | Factory ADC number at **30 °C**, ruler **3.0 V** (TS_CAL1) | 1021 |
| `cal130` | Factory ADC number at **130 °C** (TS_CAL2) | 65535 = **blank** (ES chip) |
| `1-pt cal` | Only one valid factory point → typical slope used | ✓ |
| `2-pt` | Both factory points valid (normal production chips) | – |

**Why is raw (933) lower than cal30 (1021) although the chip is warmer than 30 °C?** Different rulers: raw is measured with 3.3 V, cal30 with 3.0 V. A bigger ruler gives a smaller number for the same voltage – so both are **converted to millivolts** before comparing.

#### 5. Single-point calculation ("1-pt cal") – worked example

```
Step 1  sensor voltage now:      V_now = raw   × VDDA    / 4095 = 933  × 3300 / 4095 = 751.9 mV
Step 2  sensor voltage at 30 °C: V_30  = cal30 × 3000 mV / 4095 = 1021 × 3000 / 4095 = 748.0 mV
Step 3  difference:              751.9 − 748.0 = 3.9 mV
        3.9 mV ÷ 2.5 mV per °C = 1.6 °C
        temperature = 30 + 1.6 = 31.6 °C
```

One factory point (30 °C) + typical slope (2.5 mV/°C, `TS_AVG_SLOPE_UV_PER_C` in `sensors.c`).

#### 6. Two-point calculation ("2-pt") – normal production chips
With the 130 °C point present the slope is not assumed, it comes from this exact chip:

```
ts = raw × VDDA / 3000                 (reading converted to the 3.0 V ruler)
T  = 30 + (ts − cal30) × (130 − 30) / (cal130 − cal30)
```

On our ES chip `cal130` = 65535 (erased memory), so the formula divided by 64514 instead of ~400 and the result stayed at 30.0 °C. The code now checks the values and picks 2-pt or 1-pt automatically.

**Accuracy:** 2-point ≈ ±1–2 °C; 1-point (ours) ≈ ±2–3 °C (each chip's real slope differs a little). Changes up/down are shown correctly. To fine-tune, compare with a thermometer and adjust `TS_AVG_SLOPE_UV_PER_C`.

#### 7. How the code runs
1. **Startup – `sensors_init()`:** ADC4 set to read 2 channels in order, one per start: rank 1 = VREFINT, rank 2 = temperature sensor. Sampling time **814.5 clock cycles** (the sensor output is weak and needs time to charge the ADC input). ADC self-calibration is run.
2. **Every second (LVGL timer):**
   1. read VREFINT → VDDA
   2. read temperature sensor → °C (2-pt or 1-pt)
   3. **smoothing:** new = old + 0.3 × (measured − old) (1 ADC step ≈ 0.3 °C would otherwise flicker)
   4. update min / max, store in the **60-value history** (last 60 s)
3. **Dashboard – `ui_dashboard.c`:** reads the values every 0.5 s, updates the labels, redraws the chart when a new sample arrived. Sampling continues in the background on other screens.

```
temperature sensor ─► ADC4 (raw) ─┐
                                   ├─► convert both to mV ─► compare with factory 30 °C point
VREFINT ─► ADC4 ─► VDDA ──────────┘         ─► ÷ 2.5 mV/°C ─► + 30 ─► smoothing ─► screen + chart
```

### Step 3 – speed test, instruction cache and faster driver
Starting point: **64 MHz** (HSI 16 MHz → PLL1 M=1, N=8, R=2), flash latency 1, **instruction cache (ICACHE) off**.
**64 MHz is the maximum HCLK of the STM32WBA25** (CubeMX Clock Configuration shows "64 MHz max"), so the clock cannot be raised – speed has to come from the cache and better code.

| Setup | LVGL frame normal | LVGL frame fast | Raw fill normal | Raw fill fast |
|---|---|---|---|---|
| 64 MHz, cache off | 190–196 ms (5.0–5.2 fps) | 91–94 ms (10.5–10.9 fps) | 137 ms | 16 ms |
| 64 MHz, cache on | 157–158 ms (6.3 fps) | 86–87 ms (11.4–11.6 fps) | 103 ms | **13 ms** |

**Result:** the fast driver sends a full screen **8–10× faster** (137 → 13 ms); a full LVGL redraw is about **2× faster** (190 → 86 ms, 5 → 11.6 fps). The cache helps the slow code most (it fetches many instructions); the fast loop is limited mostly by the GPIO bus.
**Where the remaining LVGL time goes:** ~86 ms per frame = ~15 ms sending + **~70 ms LVGL rendering** (software drawing into the buffer, built with `-Og`). Normal UI updates redraw only the changed area, so they are much quicker than a full-screen frame.
The fast driver is now the **default** (`fast_mode = 1` in `tft.c`); colours were clean in the test.

- **Speed test screen** (`ui_speed.c`, menu 4): runs each test with the **normal** and the **fast** driver: (a) LVGL – changes the screen colour and calls `lv_refr_now()` 5× (render 320×240 in 24-line strips + send); (b) raw – `tft_fill_screen()` 5× (send only). Time from `HAL_GetTick()`. The **Fast switch** selects the driver used by the whole app (default: normal).
- **Why the normal driver is slow:** 137 ms for 153 600 bytes = 0.89 µs ≈ **57 CPU cycles per byte** (function call per byte, table look-ups, 10 NOPs, Debug `-Og`, no cache).
- **Fast driver** (`tft.c`: `push_fast()`, `fill_fast()`, `tft_set_fast()`): same bus protocol, compiled with `__attribute__((optimize("O2")))` even in Debug, no call per byte, GPIO addresses in registers, fill pre-computes the 4 register values once, shorter waits (3 NOP setup, 1 NOP hold – ILI9341 write cycle ≥ 66 ns, 1 cycle = 15.6 ns). The fill loop is ~14 instructions per pixel.
- **Instruction cache:** CubeMX → System Core → **ICACHE** → Mode "1-way (direct mapped cache)" → Generate Code. The CPU keeps recently used code in fast memory instead of waiting for flash on every instruction.

## 10. Next steps (options)


1. **Extend the driver:** pixels, lines, circles, text with a built-in font.
2. **LVGL v9:** fits (≈150–250 KB flash, ≈40–60 KB RAM); partial redraws are fine, full-screen animations are slow on this bit-banged bus; no touch on this shield.
