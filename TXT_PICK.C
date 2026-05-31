#include <stdio.h>
#include <dos.h>
#include <conio.h>
#include <string.h>
#include <graph.h>
#include "txt_pick.h"

#define MAX_FILES 128

static char files[MAX_FILES][13];
static int file_count = 0;

int pick_txt_file(char *out)
{
    struct find_t f;
    int index = 0;
    int top = 0;
    int i;
    int needs_redraw = 1;   /* NEW: only redraw when needed */

    /* Build file list */
    file_count = 0;

    if (_dos_findfirst("*.TXT", _A_NORMAL, &f) == 0) {
        do {
            if (file_count < MAX_FILES) {
                strncpy(files[file_count], f.name, 12);
                files[file_count][12] = '\0';
                file_count++;
            }
        } while (_dos_findnext(&f) == 0);
    }

    if (file_count == 0) {
        return 0;
    }

    for (;;) {

        /* Only redraw when something changed */
        if (needs_redraw) {
            
            _clearscreen(_GCLEARSCREEN);
            _settextposition(1, 1);

            printf("Select TXT File:\n");
            printf("----------------\n");

            for (i = 0; i < 20 && (top + i) < file_count; i++) {
                if ((top + i) == index)
                    printf(" > %s\n", files[top + i]);
                else
                    printf("   %s\n", files[top + i]);
            }

            printf("\nUP/DOWN to scroll, ENTER to select, ESC to cancel\n");

            needs_redraw = 0;
        }

        /* Wait for key */
        switch (getch()) {

        case 0:
        case 224:
            switch (getch()) {

            case 72: /* up */
                if (index > 0) {
                    index--;
                    if (index < top)
                        top = index;
                    needs_redraw = 1;   /* NEW */
                }
                break;

            case 80: /* down */
                if (index < file_count - 1) {
                    index++;
                    if (index >= top + 20)
                        top = index - 19;
                    needs_redraw = 1;   /* NEW */
                }
                break;
            }
            break;

        case 13: /* ENTER */
            strcpy(out, files[index]);
            return 1;   /* No redraw before returning */

        case 27: /* ESC */
            return 0;   /* No redraw before returning */
        }
    }
}
