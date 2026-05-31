#ifndef TXT_PICK_H
#define TXT_PICK_H

/* 
 * pick_txt_file
 * ---------------------------------------------------------
 * Displays a simple UP/DOWN scrolling list of *.TXT files 
 * in the current directory. 
 *
 * out  = buffer for selected filename (must be at least 13 bytes)
 *
 * Returns:
 *   1 = file selected
 *   0 = user pressed ESC or no files found
 */
int pick_txt_file(char *out);

#endif
