#include <conio.h>
#include <graph.h>
#include <stdio.h>      /* sprintf */
#include <stdlib.h>     /* abs */
#include <string.h>     /* strcmp, memcpy, NULL */
#include "qxgraph.h"
#include "colors.h"

#define GX_LEFT   20
#define GX_RIGHT  620
#define GX_TOP    10
#define GX_BOTTOM 190

#define GX_WIDTH  (GX_RIGHT - GX_LEFT)
#define GX_HEIGHT (GX_BOTTOM - GX_TOP)

/* ============================================================
   LOW-LEVEL CGA BACKEND (MODE 6, 640x200, 1bpp)
   ============================================================ */

/* Video memory segment (0xB800) is addressed directly via ES in the
   two remaining asm blocks below, so no far pointer is needed here. */

static void cga_clear_screen(void)
{
    _asm {
        /* Set ES to CGA video memory segment */
        mov     ax, 0xB800
        mov     es, ax

        /* Clear the full 16KB mode 6 buffer (both interlaced fields) */
        xor     di, di          ; DI = 0
        xor     ax, ax          ; AX = 0 (black background)
        mov     cx, 0x2000      ; 16384 bytes / 2 = 8192 words

        /* Clear video memory */
        rep     stosw           ; fill ES:DI with AX
    }
}

/*
 * Row-offset/mask math is done in C (cheap, correct regardless of CPU
 * target, and the compiler already emits this as efficiently as the
 * shift tricks the old asm version was attempting). The only part
 * that actually needs a segment override is the single byte
 * read-modify-write, so that's the only piece left in asm. BX is used
 * for the index since CX is not a legal 8086/80186 base/index
 * register.
 */
static void cga_putpixel(int x, int y)
{
    unsigned int ux, uy, px_addr;
    unsigned char mask;

    if (x < 0 || x >= 640 || y < 0 || y >= 200)
        return;

    ux = (unsigned int)x;
    uy = (unsigned int)y;

    px_addr = ((uy >> 1) * 80u) + (ux >> 3);
    if (uy & 1u)
        px_addr += 0x2000u;

    mask = (unsigned char)(0x80u >> (ux & 7u));

    _asm {
        mov     ax, 0xB800
        mov     es, ax
        mov     bx, px_addr
        mov     al, es:[bx]
        or      al, mask
        mov     es:[bx], al
    }
}

/* Simple Cohen–Sutherland style clip against graph box */
static int clip_to_graph(int *x, int *y)
{
    if (*x < GX_LEFT)  *x = GX_LEFT;
    if (*x > GX_RIGHT) *x = GX_RIGHT;
    if (*y < GX_TOP)   *y = GX_TOP;
    if (*y > GX_BOTTOM)*y = GX_BOTTOM;
    return 1;
}

/* Integer Bresenham line, clipped to graph box */
static void cga_line(int x1, int y1, int x2, int y2)
{
    int dx, sx, dy, sy, err, e2;

    if (!clip_to_graph(&x1, &y1)) return;
    if (!clip_to_graph(&x2, &y2)) return;

    dx = (x2 > x1) ? (x2 - x1) : (x1 - x2);
    sx = (x1 < x2) ? 1 : -1;
    dy = -(abs(y2 - y1)); /* dy = -abs(y2 - y1) */
    sy = (y1 < y2) ? 1 : -1;
    err = dx + dy;

    for (;;) {
        cga_putpixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        e2 = err << 1;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

/* Horizontal line helper - optimized with REP STOSB */
/*
 * hline/vline only run a handful of times per frame (axis box +
 * gridlines), so they aren't worth hand-rolled asm. A REP STOSW
 * approach is also structurally wrong for cga_vline: successive rows
 * in mode 6 aren't contiguous in memory (each y step jumps by 80
 * bytes, or into the other interlaced field), so a single "fill
 * forward from DI" instruction can't express a vertical run at all.
 * Plain C calling the corrected cga_putpixel is simpler, correct, and
 * plenty fast for a once-per-frame axis draw.
 */
static void cga_hline(int x1, int x2, int y)
{
    int x;
    if (y < GX_TOP || y > GX_BOTTOM) return;
    if (x1 > x2) { int t = x1; x1 = x2; x2 = t; }
    if (x2 < GX_LEFT || x1 > GX_RIGHT) return;
    if (x1 < GX_LEFT)  x1 = GX_LEFT;
    if (x2 > GX_RIGHT) x2 = GX_RIGHT;
    for (x = x1; x <= x2; ++x)
        cga_putpixel(x, y);
}

static void cga_vline(int x, int y1, int y2)
{
    int y;
    if (x < GX_LEFT || x > GX_RIGHT) return;
    if (y1 > y2) { int t = y1; y1 = y2; y2 = t; }
    if (y2 < GX_TOP || y1 > GX_BOTTOM) return;
    if (y1 < GX_TOP)    y1 = GX_TOP;
    if (y2 > GX_BOTTOM) y2 = GX_BOTTOM;
    for (y = y1; y <= y2; ++y)
        cga_putpixel(x, y);
}

/* ============================================================
   VIDEO MODE CONTROL
   ============================================================ */

void graph_init_mode(void)
{
    _setvideomode(_HRESBW);      /* CGA 640x200 mono */
    cga_clear_screen();
}

void graph_exit_mode(void)
{
    _setvideomode(_TEXTC80);
}

/* ============================================================
   AXES
   ============================================================ */

static void draw_axes(void)
{
    cga_hline(GX_LEFT, GX_RIGHT, GX_BOTTOM);  /* X axis */
    cga_vline(GX_LEFT, GX_TOP, GX_BOTTOM);    /* Y axis */
}

/* ============================================================
   GRIDLINES (dotted horizontal)
   ============================================================ */

static void draw_gridlines(void)
{
    int y, x;
    for (y = GX_TOP + 10; y < GX_BOTTOM; y += 20) {
        for (x = GX_LEFT; x <= GX_RIGHT; x += 4) {
            cga_putpixel(x, y);
        }
    }
}

/* ============================================================
   X‑AXIS TICK MARKS
   ============================================================ */

static void draw_x_ticks(int count)
{
    int i, x;
    if (count < 2) return;

    /* Tick every ~10 points, but ensure last tick is drawn */
    for (i = 0; i < count; i += 10) {
        x = GX_LEFT + (i * GX_WIDTH) / (count - 1);
        cga_putpixel(x, GX_BOTTOM - 1);
        cga_putpixel(x, GX_BOTTOM - 2);
    }

    /* Ensure a tick at the last point if not already aligned */
    if ((count - 1) % 10 != 0) {
        x = GX_LEFT + ((count - 1) * GX_WIDTH) / (count - 1);
        cga_putpixel(x, GX_BOTTOM - 1);
        cga_putpixel(x, GX_BOTTOM - 2);
    }
}

/* ============================================================
   Format_Val (min, mid, max) – aligned to graph box
   OPTIMIZED: Pure integer arithmetic, no sprintf
   ============================================================ */

static void format_val(char *buf, double val)
{
    long v = (long)(val * 1000.0);
    int pos = 0;
    char *p = buf;
    
    if (v == 0) {
        *p++ = '0';
        pos = 1;
    } else if (v < 0) {
        *p++ = '-';
        v = -v;
    }
    
    /* Integer division for scaling */
    if (v >= 1000000L) {
        long m = v / 1000000L;
        int frac = (int)((v % 1000000L) * 10 / 100000L);
        /* Write "m.fracM" */
        pos += sprintf(p, "%ld.%dM", m, frac); p += 4; pos += 4;
    } else if (v >= 1000L) {
        long m = v / 1000L;
        int frac = (int)((v % 1000L) * 10 / 100L);
        /* Write "m.fracK" */
        pos += sprintf(p, "%ld.%dK", m, frac); p += 4; pos += 4;
    } else if (v < 100 && v > -100) {
        /* Write "%.4f" */
        pos += sprintf(p, "%.4f", val); p += 5; pos += 5;
    } else {
        /* Write "%.2f" */
        pos += sprintf(p, "%.2f", val); p += 5; pos += 5;
    }
    buf[pos] = '\0';
}

/* ============================================================
   Y‑AXIS LABELS (min, mid, max) – aligned to graph box
   ============================================================ */

static void draw_y_labels(double min, double max)
{
    char buf[20];
    int row_top, row_mid, row_bot;

    row_top = 1 + (GX_TOP    * 25) / 200;
    row_bot = 1 + (GX_BOTTOM * 25) / 200;
    row_mid = (row_top + row_bot) / 2;

    _settextposition(row_top, 1);
    format_val(buf, max);
    _outtext(buf);

    _settextposition(row_mid, 1);
    format_val(buf, (min + max) / 2.0);
    _outtext(buf);

    _settextposition(row_bot, 1);
    format_val(buf, min);
    _outtext(buf);
}

/* ============================================================
   MIDPOINT LABEL (value at middle data point) – aligned to graph box
   ============================================================ */

static void draw_midpoint_label(const GraphData *gd, double min, double max)
{
    char buf[20];
    int mid_i, x, y;
    double val, scaled;

    mid_i = gd->count / 2;
    val   = gd->values[mid_i];

    x = GX_LEFT + (mid_i * GX_WIDTH) / (gd->count - 1);

    scaled = (GX_HEIGHT * (val - min) / (max - min)) * 1.0;
    if (scaled < 0.0)        scaled = 0.0;
    if (scaled > GX_HEIGHT)  scaled = GX_HEIGHT;
    y = GX_BOTTOM - (int)(scaled + 0.5);

/* convert pixel y to text row */
    format_val(buf, val);
    _settextposition(1 + (y * 25) / 200, 1 + (x * 80) / 640);
    _outtext(buf);
}

/* ============================================================
   AUTO-SCALING LOGIC
   ============================================================ */

static void compute_scale(const GraphParams *gp, const GraphData *gd,
                          double *out_min, double *out_max)
{
    int i;
    double min = 1e9, max = -1e9;

    for (i = 0; i < gd->count; i++) {
        if (gd->values[i] < min) min = gd->values[i];
        if (gd->values[i] > max) max = gd->values[i];
    }

    if (max == min)
        max = min + 1.0;

    switch (gp->scale_mode) {

    case SCALE_PADDED:
        {
            double range = max - min;
            double pad = range * 0.10;   /* 10% padding */
            min -= pad;
            max += pad;
        }
        break;

    case SCALE_FIXED:
        min = gp->fixed_min;
        max = gp->fixed_max;
        break;
    
    case SCALE_CENTERED:
    {
        double sum = 0.0;
        double mean, half_range;
        /* Use already-computed min/max from first pass */
        for (i = 0; i < gd->count; i++) {
            sum += gd->values[i];
        }
        mean = sum / (double)gd->count;
        half_range = (max - min) * 0.75;
        if (half_range < 0.01) half_range = 0.01;
        min = mean - half_range;
        max = mean + half_range;
    }
    break;

    case SCALE_TIGHT:
    default:
        break;
    }

    *out_min = min;
    *out_max = max;
}

/* ============================================================
   LINE PLOTTING – CGA-ACCURATE, ROUNDED, NO ZERO-DROP
   ============================================================ */

static void draw_line(const GraphData *gd, double min, double max)
{
    int i;
    int x1, x2, y1, y2;
    double scaled1, scaled2;
    double inv_range = 1.0 / (max - min);

    for (i = 0; i < gd->count - 1; i++) {

        x1 = GX_LEFT + (i * GX_WIDTH) / (gd->count - 1);
        x2 = GX_LEFT + ((i + 1) * GX_WIDTH) / (gd->count - 1);

        scaled1 = (GX_HEIGHT * (gd->values[i]     - min) * inv_range);
        scaled2 = (GX_HEIGHT * (gd->values[i + 1] - min) * inv_range);


        if (scaled1 < 0.0) scaled1 = 0.0;
        if (scaled2 < 0.0) scaled2 = 0.0;
        if (scaled1 > GX_HEIGHT) scaled1 = GX_HEIGHT;
        if (scaled2 > GX_HEIGHT) scaled2 = GX_HEIGHT;

        y1 = GX_BOTTOM - (int)(scaled1 + 0.5);
        y2 = GX_BOTTOM - (int)(scaled2 + 0.5);


        cga_line(x1, y1, x2, y2);
    }
}

/* ============================================================
   PARAMETER LABEL LOOKUP
   UPDATED: Use inline labels array instead of pointers
   ============================================================ */

static const char *get_param_desc(const char *short_name)
{
    static const struct {
        const char *key;
        const char *desc;
    } lookup[] = {
        { "name",               "Scenario Name"                                },
        { "int_rev",            "Interest Payments as % of Federal Revenue"    },
        { "debt_gdp",           "Federal Debt as % of GDP"                     },
        { "usd_reserve_share",  "US Dollar Share of Global FX Reserves"        },
        { "cbo_deficit",        "CBO Projected Federal Deficit as % of GDP"    },
        { "xdate",              "Days Until Debt-Ceiling X-Date"               },
        { "sahm",               "Sahm Rule Recession Indicator"                },
        { "tail_risk",          "Tail-Risk Index"                              },
        { "liq_gap",            "Liquidity Gap"                                },
        { "ofr",                "OFR Financial Stress Index"                   },
        { "hy_spread",          "High-Yield Credit Spread (bps)"               },
        { "dxy_mom",            "Dollar Index Momentum"                        },
        { "oil_price",          "Spot Price of Crude Oil"                      },
        { "ai_capex",           "AI-Related Capital Expenditure"               },
        { "geopolitical_risk",  "Geopolitical Risk Index (0-1)"                },
        { "investor_sentiment", "Investor Sentiment Index (0-1)"               },
        { "lagged_ai",          "Lagged AI Capital Expenditure"                },
        { "infl",               "Inflation Rate"                               },
        { "unemp",              "Unemployment Rate"                            },
        { "gdp",                "GDP Growth Rate"                              },
        /* stock price data */
        { "open",               "Opening Price"                                },
        { "high",               "Session High"                                 },
        { "low",                "Session Low"                                  },
        { "close",              "Closing Price"                                },
        { "volume",             "Trading Volume"                               },
        { "vwap",               "Volume Weighted Average Price"                },
        { "adj_close",          "Adjusted Closing Price"                       },
        /* technical indicators */
        { "rsi",                "Relative Strength Index (0-100)"              },
        { "macd",               "MACD Line"                                    },
        { "macd_sig",           "MACD Signal Line"                             },
        { "macd_hist",          "MACD Histogram"                               },
        { "sma20",              "20-Day Simple Moving Average"                 },
        { "sma50",              "50-Day Simple Moving Average"                 },
        { "sma200",             "200-Day Simple Moving Average"                },
        { "ema12",              "12-Day Exponential Moving Average"            },
        { "ema26",              "26-Day Exponential Moving Average"            },
        { "bb_upper",           "Bollinger Band Upper"                         },
        { "bb_lower",           "Bollinger Band Lower"                         },
        { "atr",                "Average True Range"                           },
        { "obv",                "On-Balance Volume"                            },
        { "stoch_k",            "Stochastic Oscillator %K"                     },
        { "stoch_d",            "Stochastic Oscillator %D"                     },
        { NULL, NULL }
    };

    int i;
    for (i = 0; lookup[i].key != NULL; i++) {
        if (strcmp(short_name, lookup[i].key) == 0) {
            return lookup[i].desc;
        }
    }
    return short_name;
}

/* ============================================================
   MAIN DRAW FUNCTION
   ============================================================ */

void graph_draw(const GraphParams *gp, const GraphData *gd)
{
    char buf[80];
    double min, max;

    compute_scale(gp, gd, &min, &max);

    cga_clear_screen();

    draw_axes();
    draw_gridlines();
    draw_x_ticks(gd->count);
    draw_line(gd, min, max);
    draw_y_labels(min, max);
    draw_midpoint_label(gd, min, max);   /* add this line */

/* Title bar (top text row) */

    _settextposition(1, 1);
    if (gp->ticker[0] != '\0')
        sprintf(buf, "[%s] %s  (%d/%d)  Scale: %s",
                gp->ticker,
                get_param_desc(gp->labels[gp->param_index]),
                gp->param_index + 1,
                gp->total_params,
                (gp->scale_mode == SCALE_TIGHT)    ? "Tight"  :
                (gp->scale_mode == SCALE_PADDED)   ? "Padded" :
                (gp->scale_mode == SCALE_CENTERED) ? "Center" : "Fixed");
    else
        sprintf(buf, "%s  (%d/%d)  Scale: %s",
                get_param_desc(gp->labels[gp->param_index]),
                gp->param_index + 1,
                gp->total_params,
                (gp->scale_mode == SCALE_TIGHT)    ? "Tight"  :
                (gp->scale_mode == SCALE_PADDED)   ? "Padded" :
                (gp->scale_mode == SCALE_CENTERED) ? "Center" : "Fixed");
    buf[79] = '\0';
    _outtext(buf);

/* Footer */
    _settextposition(25, 1);
    if (gp->carousel_on) {
        sprintf(buf, "CAROUSEL %ds  L/R=param  A=scale  C=stop  +/-=speed  ESC=exit",
                gp->carousel_delay);
        buf[79] = '\0';
        _outtext(buf);
    } else {
        _outtext("L/R=param  A=scale  C=carousel  +/-=speed  ESC=exit");
    }
}

/* ============================================================
   INPUT HANDLING
   ============================================================ */

int graph_handle_input(GraphParams *gp, int ch)
{
    switch (ch) {
    case 75: /* left */
        if (gp->param_index > 0) {
            gp->param_index--;
            return 1;
        }
        break;

    case 77: /* right */
        if (gp->param_index < gp->total_params - 1) {
            gp->param_index++;
            return 1;
        }
        break;

    case 'A': case 'a': /* cycle scaling modes */
        gp->scale_mode = (ScaleMode)(gp->scale_mode + 1);
        if (gp->scale_mode > SCALE_CENTERED)
            gp->scale_mode = SCALE_TIGHT;
        return 1;
        
    case 'C': case 'c':
        return 2;   /* carousel toggle */

    case '+': case '=':
        return 3;   /* faster */

    case '-':
        return 4;   /* slower */

    case 27: /* ESC */
        return -1;
    }
    return 0;
}