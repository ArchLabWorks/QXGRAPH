#ifndef COLORS_H
#define COLORS_H

/* ---------------------------------------------------------
 * CGA/EGA/VGA standard text‑mode colors (0–15)
 * --------------------------------------------------------- */
#define COL_BLACK        0
#define COL_BLUE         1
#define COL_GREEN        2
#define COL_CYAN         3
#define COL_RED          4
#define COL_MAGENTA      5
#define COL_BROWN        6
#define COL_LIGHTGRAY    7
#define COL_DARKGRAY     8
#define COL_LIGHTBLUE    9
#define COL_LIGHTGREEN   10
#define COL_LIGHTCYAN    11
#define COL_LIGHTRED     12
#define COL_LIGHTMAGENTA 13
#define COL_YELLOW       14
#define COL_WHITE        15

/* ---------------------------------------------------------
 * Semantic UI colors for QUANTXT
 * --------------------------------------------------------- */
#define COL_NORMAL       COL_LIGHTGRAY
#define COL_HEADER       COL_CYAN
#define COL_FOOTER       COL_DARKGRAY
#define COL_HIGHLIGHT    COL_YELLOW
#define COL_WARNING      COL_YELLOW
#define COL_ERROR        COL_LIGHTRED

/* ---------------------------------------------------------
 * Risk‑bar colors (new)
 * --------------------------------------------------------- */
#define COL_GOOD         COL_GREEN       /* low risk */
#define COL_CRITICAL     COL_LIGHTRED    /* high risk */

/* ---------------------------------------------------------
 * Additional semantic colors used in browser & preview
 * --------------------------------------------------------- */
#define COL_SELECTED     COL_YELLOW
#define COL_BORDER       COL_LIGHTGRAY
#define COL_LABEL        COL_CYAN

#endif
