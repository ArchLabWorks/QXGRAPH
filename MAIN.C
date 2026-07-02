#include <stdio.h>
#include <conio.h>
#include <graph.h>
#include <string.h>
#include "qxgraph.h"
#include "qxrun.h"
#include "txt_pick.h"

static void qxgraph_banner(void)
{
    static const char *lines[] = {
        "============================================================",
        "                        QXGRAPH  v1.2                       ",
        "                   QUANTXT Graphing Utility                 ",
        "------------------------------------------------------------",
        "   Stand-alone CGA Mode 6 Time-Series Graphing Tool         ",
        "   (C) 2026 ArchLabWorks                           ",
        "                                                            ",
        "   LEFT / RIGHT  : Switch parameter                         ",
        "   A             : Cycle scale mode                         ",
        "   ESC           : Exit                                     ",
        "------------------------------------------------------------"
    };
    const int line_count = sizeof(lines) / sizeof(lines[0]);

    int i, max_width = 0;
    int start_row, start_col;

    _clearscreen(_GCLEARSCREEN);

    /* compute max line width */
    for (i = 0; i < line_count; i++) {
        int len = (int)strlen(lines[i]);
        if (len > max_width) max_width = len;
    }

    /* center block in 80x25 text mode */
    start_row = (25 - line_count) / 2;
    if (start_row < 1) start_row = 1;
    start_col = (80 - max_width) / 2;
    if (start_col < 1) start_col = 1;

    for (i = 0; i < line_count; i++) {
        _settextposition(start_row + i, start_col);
        _outtext((char far *)lines[i]);
    }

    /* centered prompt below banner */
    {
        const char *prompt = "Press any key to begin...";
        int prompt_col = (80 - (int)strlen(prompt)) / 2;
        if (prompt_col < 1) prompt_col = 1;
        _settextposition(start_row + line_count + 2, prompt_col);
        _outtext((char far *)prompt);
    }

    getch();
}

int main(void)
{
    char fname[13];
    int  ch;

    qxgraph_banner();

    for (;;) {
        _clearscreen(_GCLEARSCREEN);
        _settextposition(1, 1);

        printf("Select a scenario file (.TXT):\n\n");

        if (!pick_txt_file(fname)) {
            printf("\nNo file selected. Press any key to exit.\n");
            getch();
            break;
        }

        printf("\nSelected file: %s\n", fname);
        printf("Press any key to load graph...\n");
        getch();

        /* flush any extra keystrokes before entering graph loop */
        while (kbhit()) getch();

        run_graph(fname);

        /* run_graph() already returned to text mode; offer a way back
           to the file picker instead of exiting unconditionally */
        _clearscreen(_GCLEARSCREEN);
        _settextposition(1, 1);
        printf("Graph closed.\n\n");
        printf("ENTER : Select another scenario\n");
        printf("ESC   : Exit\n");

        for (;;) {
            ch = getch();
            if (ch == 13) break;   /* back to top of outer loop */
            if (ch == 27) return 0;
        }
    }

    return 0;
}