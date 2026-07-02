# **QXGRAPH v1.2**  
### **QuanTXT Graphing Utility — IBM PC/XT CGA Time‑Series Visualizer**

A stand‑alone DOS graphing tool for the IBM PC/XT that renders financial and macroeconomic time‑series data in CGA Mode 6 (640×200 monochrome). Built as a companion utility to the **QuanTXT** research platform. Also supports general‑purpose columnar time‑series data including stock market and technical indicator datasets.

This release restores correctness after the v1.2 optimization pass and re‑establishes a clean, stable build.

---

## **Overview**

QXGRAPH reads plain‑text scenario files and renders each parameter as a scrollable, scalable line graph directly on CGA hardware. Parameters cycle manually or automatically via carousel mode. No mouse, no EGA, no extended memory — runs clean on a stock 8088 XT with CGA card.

v1.3.1 fixes multiple regressions introduced in v1.3, including broken assembly routines, missing headers, incorrect CGA memory math, and under‑clearing of the framebuffer.

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

Compiler flags used:

```
wcc *.C -i="C:\WATCOM/h" -i="<source_dir>" -w4 -e25 -zq -od -d2 -fpi87 -bt=dos -fo=.obj -ml
```

Note: include path must point to your local source directory so the correct `QXGRAPH.H` is found during compilation.

---

## **Usage**

1. Place one or more `.TXT` data files in the working directory  
2. Run `QXGRAPH.EXE`  
3. Select a file from the picker  
4. Browse parameters and adjust scaling  
5. After closing a graph, press **ENTER** to return to the file picker or **ESC** to exit (new in v1.3.1)

### **Controls**

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
| **Centered** | Symmetric range around the mean — best for slowly trending or narrow‑variance data |

---

## **Carousel Mode**

Press `C` to start automatic parameter cycling. The footer shows carousel status and current delay in seconds. Use `+` and `-` to adjust rotation speed. Press `C` again to stop.

Carousel uses the BIOS tick counter at `0000:046C` (~18.2 ticks/second) for timing — no DOS timer calls, accurate at 4.77MHz.

---

## **Data File Format**

Plain text, space‑delimited. Supports two optional header lines before the column name line. First column of each data row is a row label (date, scenario name, etc.) and is skipped during parsing.

(Examples unchanged — omitted here for brevity.)

---

## **Y‑Axis Label Formatting**

Values are auto‑formatted based on magnitude:

| Range | Format | Example |
|-------|--------|---------|
| >= 1,000,000 | Millions suffix | `31393.0M` |
| >= 1,000 | Thousands suffix | `4.2K` |
| < 0.1 | 4 decimal places | `0.0605` |
| All others | 2 decimal places | `22.74` |

*Note:* `format_val()` still uses `sprintf()` internally; integer‑math formatting planned but not yet implemented (V1.2).

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
- Direct framebuffer write to `B800:0000`
- Correct CGA interlaced memory layout:  
  - even rows at `B800:0000`  
  - odd rows at `B800:2000`
- **Pixel plotting rewritten for correctness**  
  - Address math now computed in C  
  - Inline asm only performs the read‑modify‑write  
  - Fixes illegal 8086 shifts, invalid index registers, wrong bit masks
- **Horizontal/vertical line drawing reverted to C**  
  - v1.2 `REP STOSW` versions were unsafe and incompatible with CGA memory layout
- **Screen clear restored to full 16KB**  
  - v1.2 under‑cleared only 4KB; V1.2 restores correct `0x2000` word count
- Integer Bresenham line algorithm — no floating point in pixel path
- Mixed‑mode text overlay via Watcom `_outtext` / `_settextposition`

---

## **Known Limitations**

(unchanged, except updated wording)

- `WINDOW` size fixed at 30 rows per view; files up to 256 rows supported  
- `clip_to_graph` clamps endpoints rather than performing true segment clipping  
- Mixed‑mode text overlay may vary on clone CGA cards  
- Requires 8087 coprocessor unless recompiled with `-fpi`  

---

## **Version Notes**

### **V1.2 — Correctness & Build Restoration**
- Restored required headers (`stdio.h`, `stdlib.h`, `string.h`)  
- Repaired `cga_putpixel()` (five independent bugs)  
- Reverted unsafe assembly in `cga_hline` / `cga_vline`  
- Restored full framebuffer clear size  
- Removed dead `label_ptrs[]` and unused `CGA` pointer  
- Added return‑to‑menu loop in `MAIN.C`  
- Build now compiles clean under Open Watcom 1.9

### **v1.2 — Optimization Pass (Partially Reverted)**
- Struct compaction, inline arrays, type narrowing retained  
- Header removal, asm line drawing, and partial screen‑clear optimization reverted  
- Pixel plotting asm rewritten for correctness rather than removed

---

## **License**

© 2026 ArchLabWorks. All rights reserved.

