# **QXGRAPH v1.2**  
### **QuanTXT Graphing Utility — IBM PC/XT CGA Time‑Series Visualizer**

A stand‑alone DOS graphing tool for the IBM PC/XT that renders financial, macroeconomic, and general time‑series datasets in **CGA Mode 6 (640×200 monochrome)**. Designed as a companion utility to the **QuanTXT** research platform, QXGRAPH runs clean on a stock 8088 XT with no extended memory, no EGA/VGA, and no mouse.

This release restores correctness after the v1.2 optimization pass and re‑establishes a stable, authentic CGA build.

[**Latest Release Here**](https://github.com/ArchLabWorks/QXGRAPH/releases)

---

## **Overview**

QXGRAPH reads plain‑text scenario files and renders each parameter as a scrollable, scalable line graph directly on CGA hardware. Parameters can be browsed manually or cycled automatically via carousel mode.

**V1.2** fixes multiple regressions introduced during the optimization pass, including:

- broken inline assembly routines  
- missing headers  
- incorrect CGA memory math  
- under‑clearing of the framebuffer  
- unsafe line‑drawing optimizations  

The renderer is now fully correct and period‑authentic.

---

## **Requirements**

- IBM PC/XT or compatible (8088/8086)  
- CGA graphics card  
- 8087 math coprocessor (required — compiled with `-fpi87`)  
- DOS 3.x or later  
- Compiled with **Open Watcom 1.9** (`-ml` large model)

---

## **Building**

```bat
wmake -f QXGRAPH.mk -h -e QXGRAPH.exe
```

Compiler flags:

```
wcc *.C -i="C:\WATCOM/h" -i="<source_dir>" -w4 -e25 -zq -od -d2 -fpi87 -bt=dos -fo=.obj -ml
```

> **Note:** The include path must point to your local source directory so the correct `QXGRAPH.H` is found during compilation.

---

## **Usage**

1. Place one or more `.TXT` data files in the working directory  
2. Run `QXGRAPH.EXE`  
3. Select a file from the picker  
4. Browse parameters and adjust scaling  
5. After closing a graph, press **ENTER** to return to the file picker or **ESC** to exit (**new in V1.2**)

---

## **Controls**

| Key | Action |
|-----|--------|
| `LEFT` / `RIGHT` | Switch parameter |
| `UP` / `DOWN` | Scroll time window |
| `A` | Cycle scale mode |
| `C` | Toggle carousel auto‑advance |
| `+` / `=` | Carousel faster (minimum 1 second) |
| `-` | Carousel slower (maximum 60 seconds) |
| `ESC` | Exit graph view or exit program from menu |

---

## **Scale Modes**

Cycle with the `A` key. Current mode shown in title bar.

| Mode | Behavior |
|------|----------|
| **Tight** | Exact min/max of visible window |
| **Padded** | 10% padding above and below range |
| **Fixed** | User‑defined min/max |
| **Centered** | Symmetric range around the mean — ideal for narrow‑variance or slowly trending data |

---

## **Carousel Mode**

Press `C` to start automatic parameter cycling. The footer shows carousel status and current delay in seconds. Use `+` and `-` to adjust rotation speed. Press `C` again to stop.

Carousel timing uses the BIOS tick counter at `0000:046C` (~18.2 ticks/sec) — no DOS timer calls, fully accurate at 4.77MHz.

---

## **Data File Format**

Plain text, space‑delimited. Supports two optional header lines before the column name line. The first column of each data row is a row label (date, scenario name, etc.) and is skipped during parsing.

```
# ticker SPX
# name open high low close volume rsi macd
2026-05-01 5204.30 5228.60 5192.10 5218.90 3284000000 53.8 18.60
2026-05-02 5218.90 5245.20 5208.40 5231.60 3156000000 55.2 19.10
```

Rules:

- `# ticker` — optional, sets ticker symbol in title bar  
- `# name` — required for column labels; must contain `name` as first token  
- Other `#` lines treated as comments  
- Blank lines and DOS `\r\n` handled correctly  
- Up to **256 rows**, **19 parameters**  
- Files without headers fall back to `col_1`, `col_2`, etc.

### **QuanTXT Scenario Format**

```
#US_MARKET_FISCAL_30D REPORT 5/17/2026
#
# name int_rev debt_gdp usd_reserve_share cbo_deficit xdate sahm tail_risk liq_gap ofr hy_spread dxy_mom oil_price ai_capex lagged_ai geopolitical_risk investor_sentiment infl unemp gdp

day_01 22.10 70.10 65.0 3.05 90 0.20 1.25 1.02 0.96 0.0605 5.1 81 1.02 0.0 0.52 0.39 0.032 0.042 0.020
```

### **Stock / Technical Indicator Format**

```
# ticker AAPL
# name open high low close volume vwap rsi macd macd_sig macd_hist sma20 sma50 bb_upper bb_lower atr obv stoch_k stoch_d
2026-05-01 189.20 191.40 188.60 190.80 52400000 190.10 58.2 1.24 0.98 0.26 188.40 184.20 194.60 182.20 1.84 52400000 62.4 58.1
```

---

## **Parameter Descriptions**

(unchanged — retained from v1.1)

---

## **Y‑Axis Label Formatting**

Values are auto‑formatted based on magnitude:

| Range | Format | Example |
|-------|--------|---------|
| ≥ 1,000,000 | Millions suffix | `31393.0M` |
| ≥ 1,000 | Thousands suffix | `4.2K` |
| < 0.1 | 4 decimal places | `0.0605` |
| All others | 2 decimal places | `22.74` |

> `format_val()` currently uses `sprintf()`; integer‑math formatting planned for a future release.

---

## **Source Files**

| File | Purpose |
|------|---------|
| `MAIN.C` | Entry point, banner, file selection, return‑to‑menu loop |
| `QXGRAPH.C` | CGA renderer, Bresenham line, axes, scaling, label formatting |
| `QXGRAPH.H` | Graph structs, scale mode enum, carousel fields, API |
| `QXRUN.C` | Data loader, main graph loop, input dispatch, carousel timer |
| `QXRUN.H` | `run_graph()` prototype |
| `TXT_PICK.C` | Scrolling file picker |
| `TXT_PICK.H` | `pick_txt_file()` prototype |
| `COLORS.H` | CGA/EGA color definitions and semantic aliases |

---

## **CGA Implementation Notes (Updated for V1.2)**

- **Mode 6** (640×200, 1bpp monochrome) via `_HRESBW`  
- Direct framebuffer writes to `B800:0000`  
- Correct CGA interlaced memory layout:  
  - even rows → `B800:0000`  
  - odd rows → `B800:2000`  
- **Pixel plotting rewritten for correctness**  
  - Address math computed in C  
  - Inline asm only performs read‑modify‑write  
  - Fixes illegal 8086 shifts, invalid index registers, wrong bit masks  
- **Horizontal/vertical line drawing reverted to C**  
  - v1.2 `REP STOSW` versions were unsafe due to CGA memory layout  
- **Screen clear restored to full 16KB**  
  - v1.2 optimization under‑cleared only 4KB; V1.2 restores correct `0x2000` word count  
- Integer Bresenham line algorithm — no floating point in pixel path  
- Mixed‑mode text overlay via Watcom `_outtext` / `_settextposition`

---

## **Known Limitations**

- `WINDOW` size fixed at 30 rows per view; files up to 256 rows supported  
- `clip_to_graph` clamps endpoints rather than performing true segment clipping  
- Mixed‑mode text overlay may vary on clone CGA cards  
- Requires 8087 coprocessor unless recompiled with `-fpi`  

---

## **License**

© 2026 ArchLabWorks. All rights reserved.

