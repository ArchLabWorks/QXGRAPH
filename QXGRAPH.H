#ifndef QXGRAPH_H
#define QXGRAPH_H

/* ============================================================
   AUTO-SCALING MODES
   ============================================================ */
typedef enum {
    SCALE_TIGHT = 0,
    SCALE_PADDED,
    SCALE_FIXED,
    SCALE_CENTERED    /* symmetric around mean */
} ScaleMode;

/* ============================================================
   GRAPH PARAMETER CONTROL
   COMPACT: Use fixed-size arrays instead of pointers
   ============================================================ */
typedef struct {
    unsigned char param_index;    /* was int; confirmed safe: max ~19,
                                      C's default argument promotion
                                      makes %d/arithmetic sites behave
                                      identically to before (see task
                                      6 verification) */
    unsigned char total_params;   /* was int; same reasoning */
    int          window_start;
    int          window_size;
    char         labels[20][32];  /* inline labels, max 19 params */
    char         ticker[16];      /* inline ticker */
    unsigned char scale_mode;     /* byte instead of enum */
    double       fixed_min;
    double       fixed_max;
    unsigned char carousel_on;
    unsigned char carousel_delay;  /* 1-60 seconds */
} GraphParams;

/* ============================================================
   GRAPH DATA
   COMPACT: Use fixed-size array inline
   ============================================================ */
typedef struct {
    int    count;
    double values[256];  /* inline data, max 256 points */
} GraphData;

/* ============================================================
   GRAPH ENGINE API
   ============================================================ */
void graph_init_mode(void);
void graph_exit_mode(void);
void graph_draw(const GraphParams *gp, const GraphData *gd);
int  graph_handle_input(GraphParams *gp, int ch);

#endif
