#include <graph.h>
#include <conio.h>
#include <stdio.h>      /* FILE, fopen, fgets, fclose, sprintf, printf */
#include <stdlib.h>     /* atof */
#include <string.h>     /* strncpy, strtok, strcmp, memcpy, memset, size_t */
#include "qxgraph.h"
#include "qxrun.h"

#define MAX_ROWS 256
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
        tok = strtok(tmp, " \t\r\n");

    if (tok && strcmp(tok, "ticker") == 0) {
        tok = strtok(NULL, " \t\r\n");
        if (tok) {
            strncpy(ticker_buf, tok, 15);
            ticker_buf[15] = '\0';
        }
        continue;
    }

    if (tok && strcmp(tok, "name") == 0) {
        tok = strtok(NULL, " \t\r\n");
        c = 0;
        while (tok && c < MAX_COLS - 1) {
            strncpy(label_buf[c], tok, 31);
            label_buf[c][31] = '\0';
            tok = strtok(NULL, " \t\r\n");
            c++;
        }
        labels_loaded = 1;
    }
    continue;
}

        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;

        c   = 0;
        tok = strtok(line, " \t\r\n");   /* skip row label */
        tok = strtok(NULL, " \t\r\n");   /* first value */

        while (tok && c < MAX_COLS) {
            data[c][rows] = atof(tok);
            tok = strtok(NULL, " \t\r\n");
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
    memcpy(gp.labels, label_buf, (size_t)cols * sizeof(label_buf[0]));
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
            memset(gd.values, 0, sizeof(gd.values));
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