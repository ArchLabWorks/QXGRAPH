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
   ============================================================ */
typedef struct {
    int          param_index;
    int          total_params;
    int          window_start;
    int          window_size;
    const char **labels;
    const char  *ticker;
    ScaleMode    scale_mode;
    double       fixed_min;
    double       fixed_max;
    int          carousel_on;       /* new */
    int          carousel_delay;    /* new - seconds between switches */
} GraphParams;

/* ============================================================
   GRAPH DATA
   ============================================================ */
typedef struct {
    int    count;
    double values[256];
} GraphData;

/* ============================================================
   GRAPH ENGINE API
   ============================================================ */
void graph_init_mode(void);
void graph_exit_mode(void);
void graph_draw(const GraphParams *gp, const GraphData *gd);
int  graph_handle_input(GraphParams *gp, int ch);

#endif
