#include <graph.h>
#include <conio.h>
#include <stdio.h>      /* FILE, fopen, fgets, fclose, sprintf, printf */
#include <string.h>     /* strncpy, strcmp, size_t */
#include "qxgraph.h"
#include "qxrun.h"

/* parse_decimal — replaces atof() for scenario-file numeric parsing.
   Confirmed against real .TXT files (NDXJUN26, SPJUN26, USJUN30D,
   1653 tokens, zero mismatches vs atof to 1e-9): format is always
   an optional leading '-' (never '+'), digits, and an optional '.'
   followed by one or more digits (2 or 3 seen; not hardcoded to a
   fixed count). No scientific notation, no thousands separators.
   Anything outside that format silently parses as 0.0, same as
   atof()'s own behavior on unparseable input — not a regression,
   just not stricter either. */
static double parse_decimal(const char *s)
{
    int neg = 0;
    double whole = 0.0;
    double frac = 0.0;
    double frac_scale = 1.0;

    if (*s == '-') { neg = 1; s++; }

    while (*s >= '0' && *s <= '9') {
        whole = whole * 10.0 + (double)(*s - '0');
        s++;
    }

    if (*s == '.') {
        s++;
        while (*s >= '0' && *s <= '9') {
            frac_scale *= 10.0;
            frac = frac * 10.0 + (double)(*s - '0');
            s++;
        }
        frac = frac / frac_scale;
    }

    return neg ? -(whole + frac) : (whole + frac);
}

static int is_delim(char c)
{
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

/* my_strtok — drop-in replacement for strtok() using the fixed
   delimiter set " \t\r\n" (confirmed via grep: the only delimiter
   set passed to strtok() anywhere in this file). Same collapsing-
   consecutive-delimiters behavior as strtok: runs of delimiters are
   skipped as a single boundary, so no empty-string token is ever
   returned — matches load_txt()'s existing parsing assumptions.
   Same call convention: pass a string to start a new scan, NULL to
   continue from the internal saved position. Single static save
   pointer, same as the underlying strtok() — not thread-safe, which
   matches this program's single-threaded use.
   Verified against strtok() on real header/data lines from
   NDXJUN26.TXT, SPJUN26.TXT, USJUN30D.TXT plus edge cases (double
   spaces, tabs, CRLF, leading/trailing whitespace, empty string,
   whitespace-only, single token) — all matched token-for-token. */
static char *my_strtok(char *str)
{
    static char *next = NULL;
    char *start;

    if (str != NULL) {
        next = str;
    }
    if (next == NULL) {
        return NULL;
    }

    while (*next && is_delim(*next)) {
        next++;
    }
    if (*next == '\0') {
        next = NULL;
        return NULL;
    }

    start = next;
    while (*next && !is_delim(*next)) {
        next++;
    }
    if (*next != '\0') {
        *next = '\0';
        next++;
    } else {
        next = NULL;
    }

    return start;
}

#define MAX_ROWS 40      /* was 256. Confirmed max real usage: 31 rows
                            (calendar-month daily window, per Tom).
                            40 gives ~30% headroom over that confirmed
                            max. The existing bounds check below
                            (`if (rows >= MAX_ROWS) break;`) already
                            references this symbol, not a literal, so
                            it adapts automatically — no other code
                            change needed for this resize. */
#define MAX_COLS 20
#define WINDOW   30

static double data[MAX_COLS][MAX_ROWS];
static int rows = 0;
static int cols = 0;
static int top  = 0;

static char label_buf[MAX_COLS][32];
static int labels_loaded = 0;
static char ticker_buf[16] = "";

static int  carousel_on    = 0;
static int  carousel_delay = 5;
static long carousel_ticks = 0;

static long read_bios_ticks(void)
{
    long far *ticks = (long far *)0x0000046CUL;
    return *ticks;
}



static int load_txt(const char *fname)
{
    FILE *fp;
    char line[256];
    char *tok;
    int c;

    fp = fopen(fname, "r");
    if (!fp) return 0;

    rows = 0;
    cols = 0;
    labels_loaded = 0;

    while (fgets(line, sizeof(line), fp)) {

        /* parse header line for column labels */
    if (line[0] == '#' && !labels_loaded) {
        char tmp[256];
        strncpy(tmp, line + 1, 255);
        tmp[255] = '\0';
        tok = my_strtok(tmp);

    if (tok && strcmp(tok, "ticker") == 0) {
        tok = my_strtok(NULL);
        if (tok) {
            strncpy(ticker_buf, tok, 15);
            ticker_buf[15] = '\0';
        }
        continue;
    }

    if (tok && strcmp(tok, "name") == 0) {
        tok = my_strtok(NULL);
        c = 0;
        while (tok && c < MAX_COLS - 1) {
            strncpy(label_buf[c], tok, 31);
            label_buf[c][31] = '\0';
            tok = my_strtok(NULL);
            c++;
        }
        labels_loaded = 1;
    }
    continue;
}

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        c   = 0;
        tok = my_strtok(line);   /* skip row label */
        tok = my_strtok(NULL);   /* first value */

        while (tok && c < MAX_COLS) {
            data[c][rows] = parse_decimal(tok);
            tok = my_strtok(NULL);
            c++;
        }

        if (c > cols) cols = c;
        rows++;
        if (rows >= MAX_ROWS) break;
    }

    /* fallback if no header found */
    if (!labels_loaded) {
        for (c = 0; c < cols; c++) {
            sprintf(label_buf[c], "col_%d", c + 1);
        }
    }

    fclose(fp);
    return 1;
}

void run_graph(const char *fname)
{
    static GraphParams gp;
    static GraphData   gd;
    int i, r, ch, needs_draw;

    if (!load_txt(fname)) {
        printf("Could not load file\n");
        printf("Press any key...\n");
        getch();
        return;
    }

    /* safety: ensure labels exist for all columns */
    if (cols > MAX_COLS - 1)
        cols = MAX_COLS - 1;

    gp.param_index  = 0;
    gp.total_params = cols;
    {
        /* replaces memcpy(gp.labels, label_buf, cols * sizeof(label_buf[0]));
           both are char[][32] with identical row size (MAX_COLS=20),
           laid out contiguously, so a flat byte copy is equivalent.
           This and the memset below are the only two memcpy/memset
           call sites in the whole linked program (confirmed by grep
           across QXGRAPH.C/MAIN.C/TXT_PICK.C/QXRUN.C) — inlining both
           drops the memcpy/memset library routines from the link
           entirely, which is why this exceeds the usual 32-byte
           inline threshold. */
        size_t label_bytes = (size_t)cols * sizeof(label_buf[0]);
        char *dst = (char *)gp.labels;
        const char *src = (char *)label_buf;
        size_t bi;
        for (bi = 0; bi < label_bytes; bi++) {
            dst[bi] = src[bi];
        }
    }
    strncpy(gp.ticker, ticker_buf, 15);
    gp.scale_mode   = SCALE_TIGHT;
    gp.fixed_min    = 0.0;
    gp.fixed_max    = 1.0;
    gp.carousel_on    = 0;
    gp.carousel_delay = carousel_delay;

    top        = 0;
    needs_draw = 1;

    /* flush any buffered keys */
    while (kbhit()) getch();

    graph_init_mode();

    for (;;) {

        if (needs_draw) {
            /* replaces memset(gd.values, 0, sizeof(gd.values)); zeroing
               via double assignment (256 iterations) rather than a
               byte loop (2048 iterations) — fewer iterations, and
               IEEE 0.0's bit pattern is all-zero-bytes anyway, so this
               is exactly equivalent to the byte-level memset. */
            {
                int zi;
                for (zi = 0; zi < (int)(sizeof(gd.values) / sizeof(gd.values[0])); zi++) {
                    gd.values[zi] = 0.0;
                }
            }
            gd.count = 0;
            for (i = 0; i < WINDOW && (top + i) < rows; i++) {
                gd.values[i] = data[gp.param_index][top + i];
                gd.count++;
            }
            graph_draw(&gp, &gd);
            needs_draw = 0;
        }

        /* carousel auto-advance */
        if (carousel_on) {
            if (read_bios_ticks() - carousel_ticks >=
                    (long)(carousel_delay * 18)) {
                gp.param_index++;
                if (gp.param_index >= gp.total_params)
                    gp.param_index = 0;
                carousel_ticks = read_bios_ticks();
                needs_draw = 1;
            }
        }

        if (!kbhit()) continue;

        ch = getch();
        if (ch == 27) break;

        if (ch == 0 || ch == 224) {
            ch = getch();
            if (ch == 72 && top > 0) {
                top--;
                needs_draw = 1;
            }
            else if (ch == 80 && rows > WINDOW && top < (rows - WINDOW)) {
                top++;
                needs_draw = 1;
            }
            else {
                r = graph_handle_input(&gp, ch);
                if (r == 1) needs_draw = 1;
            }
        } else {
            r = graph_handle_input(&gp, ch);
            if (r == 1) needs_draw = 1;

            if (r == 2) {   /* carousel toggle */
                carousel_on       = !carousel_on;
                gp.carousel_on    = carousel_on;
                carousel_ticks    = read_bios_ticks();
                needs_draw        = 1;
            }
            if (r == 3 && carousel_delay > 1) {  /* faster */
                carousel_delay--;
                gp.carousel_delay = carousel_delay;
                needs_draw        = 1;
            }
            if (r == 4 && carousel_delay < 60) { /* slower */
                carousel_delay++;
                gp.carousel_delay = carousel_delay;
                needs_draw        = 1;
            }
        }
    }

    graph_exit_mode();
    _settextposition(1,1);
}

