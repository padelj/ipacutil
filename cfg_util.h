/*************************************************************************
 *                                                                       *
 * This is free software; you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by  *
 * the Free Software Foundation; either version 2 of the License, or     *
 * at your option) any later version.                                    *
 *                                                                       *
 * This program is distributed in the hope that it will be useful,       *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 * GNU General Public License for more details.                          *
 *                                                                       *
 * You should have received a copy of the GNU General Public License     *
 * along with ipacutil; if not, write to:                                  *
 *                   Free Software Foundation, Inc.                      *
 *                   59 Temple Place                                     *
 *                   Suite 330                                           *
 *                   Boston, MA  02111-1307                              *
 *                   USA                                                 *
 *************************************************************************/


#include <stdio.h>
#include <math.h>
#include "ipac_prog.h"
#include "cmd_line.h"

//struct cfg_struct {
//   int shift_code;
//   int *key_code;
//};

static const char *MISC_SECTION       = "[Misc]";
static const char *SHIFTED_SECTION    = "[Shifted]";
static const char *SHIFTED_SECTION2   = "[Shifted2]";  /* for IPAC4 */
static const char *UNSHIFTED_SECTION  = "[Unshifted]";
static const char *UNSHIFTED_SECTION2 = "[Unshifted2]";  /* for IPAC4 */
static const char *MACRO_SECTION      = "[Macro]";
static const char *MACRO_SECTION2     = "[Macro2]";
static const char *SECTION_CLOSE      = ".";

#define SHIFT_CODE_OFFSET 0

static const char *SHIFT_CODE_LABEL  = "ShiftCode";
static const char *BOARD_TYPE_LABEL  = "BoardType";

typedef struct key_struct {
   int def_code;
   char *comment;
   char *def_key;
} key_def;

static const char *DEFAULT_CONFIG_FILE = "keycfg.cfg";

/* TODO: These should probably go in ipac_prog.h */

/* This shows the names of the 28 I-PAC inputs           */
/* This can be used when generating a config file        */
/* There are 56 entries - the first 28 are for unshifted */
/* The last 28 are for shifted                           */
static const struct key_struct ipac2_defaults[] = {
/*The first 28 are unshifted inputs*/
   { 0xF5, "1 Up",     "up arrow" },
   { 0xF4, "1 Right",  "right arrow" },
   { 0x14, "1 Bt 1",   "left ctrl" },
   { 0xEB, "1 Left",   "left arrow" },
   { 0x29, "1 Bt 3",   "space" },
   { 0xF2, "1 Down",   "down arrow" },
   { 0x1A, "1 Bt 5",   "z" },
   { 0x11, "1 Bt 2",   "left alt" },
   { 0x34, "2 Right",  "g" },
   { 0x12, "1 Bt 4",   "left shift" },
   { 0x23, "2 Left",   "d" },
   { 0x22, "1 Bt 6",   "x" },
   { 0x2D, "2 Up",     "r" },
   { 0x2B, "2 Down",   "f" },
   { 0x1C, "2 Bt 1",   "a" },
   { 0x16, "Start 1",  "1" },
   { 0x1B, "2 Bt 2",   "s" },
   { 0x1E, "Start 2",  "2" },
   { 0x15, "2 Bt 3 ",  "q" },
   { 0x2E, "Coin 1 ",  "5" },
   { 0x1D, "2 Bt 4 ",  "w" },
   { 0x36, "Coin 2",   "6" },
   { 0x43, "2 Bt 5",   "i" },
   { 0x21, "1 Bt 7",   "c" },
   { 0x42, "2 Bt 6",   "k" },
   { 0x2A, "1 Bt 8",   "v" },
   { 0x3B, "2 Bt 7",   "j" },
   { 0x4B, "2 Bt 8",   "l" },
/*Begin shifted inputs*/
   { 0x0E, "1 Up",     "`" },
   { 0x0D, "1 Right",  "Tab" },
   { 0x2E, "1 Bt 1",   "5" },
   { 0x5A, "1 Left",   "Enter" },
   { 0x00, "1 Bt 3",   "" },
   { 0x4D, "1 Down",   "p" },
   { 0x00, "1 Bt 5",   "" },
   { 0x00, "1 Bt 2",   "" },
   { 0x00, "2 Right",  "" },
   { 0x00, "1 Bt 4",   "" },
   { 0x00, "2 Left",   "" },
   { 0x00, "1 Bt 6",   "" },
   { 0x00, "2 Up",     "" },
   { 0x00, "2 Down",   "" },
   { 0x00, "2 Bt 1",   "" },
   { 0x00, "Start 1",  "" },
   { 0x00, "2 Bt 2",   "" },
   { 0x76, "Start 2",  "esc" },
   { 0x00, "2 Bt 3",   "" },
   { 0x00, "Coin 1",   "" },
   { 0x00, "2 Bt 4",   "" },
   { 0x00, "Coin 2",   "" },
   { 0x00, "2 Bt 5",   "" },
   { 0x00, "1 Bt 7",   "" },
   { 0x00, "2 Bt 6",   "" },
   { 0x00, "1 Bt 8",   "" },
   { 0x00, "2 Bt 7",   "" },
   { 0x00, "2 Bt 8",   "" }
};

/* This shows the names of the 28 I-PAC4 inputs          */
/* This can be used when generating a config file        */
/* There are 56 entries - the first 28 are for unshifted */
/* The last 28 are for shifted                           */
/* These are for the 2nd "page" which exists only on IPAC4 */
static const struct key_struct ipac4_defaults[] = {
/*The first 28 are unshifted inputs*/
   { 0x43, "3 Up",     "i" },
   { 0x4B, "3 Right",  "l" },
   { 0x94, "3 Bt 1",   "right ctrl" },
   { 0x3B, "3 Left",   "j" },
   { 0x5A, "3 Bt 3",   "enter" },
   { 0x42, "3 Down",   "k" },
   { 0x00, "3 Bt 5",   "" },
   { 0x59, "3 Bt 2",   "right shift" },
   { 0x3C, "4 Right",  "u" },
   { 0x44, "3 Bt 4",   "o" },
   { 0x2A, "4 Left",   "v" },
   { 0x00, "3 Bt 6",   "" },
   { 0x35, "4 Up",     "y" },
   { 0x31, "4 Down",   "n" },
   { 0x32, "4 Bt 1",   "b" },
   { 0x26, "Start 3",  "3" },
   { 0x24, "4 Bt 2",   "e" },
   { 0x25, "Start 4",  "4" },
   { 0x33, "4 Bt 3 ",  "h" },
   { 0x3D, "Coin 3 ",  "7" },
   { 0x3A, "4 Bt 4 ",  "m" },
   { 0x3E, "Coin 4",   "8" },
   { 0x00, "4 Bt 5",   "" },
   { 0x00, "3 Bt 7",   "" },
   { 0x00, "4 Bt 6",   "" },
   { 0x00, "3 Bt 8",   "" },
   { 0x00, "4 Bt 7",   "" },
   { 0x00, "4 Bt 8",   "" },
/*Begin shifted inputs*/
   { 0x00, "3 Up",     "" },
   { 0x00, "3 Right",  "" },
   { 0x00, "3 Bt 1",   "" },
   { 0x00, "3 Left",   "" },
   { 0x00, "3 Bt 3",   "" },
   { 0x00, "3 Down",   "" },
   { 0x00, "3 Bt 5",   "" },
   { 0x00, "3 Bt 2",   "" },
   { 0x00, "4 Right",  "" },
   { 0x00, "3 Bt 4",   "" },
   { 0x00, "4 Left",   "" },
   { 0x00, "3 Bt 6",   "" },
   { 0x00, "4 Up",     "" },
   { 0x00, "4 Down",   "" },
   { 0x00, "4 Bt 1",   "" },
   { 0x00, "Start 3",  "" },
   { 0x00, "4 Bt 2",   "" },
   { 0x00, "Start 4",  "" },
   { 0x00, "4 Bt 3",   "" },
   { 0x00, "Coin 3",   "" },
   { 0x00, "4 Bt 4",   "" },
   { 0x00, "Coin 4",   "" },
   { 0x00, "4 Bt 5",   "" },
   { 0x00, "3 Bt 7",   "" },
   { 0x00, "4 Bt 6",   "" },
   { 0x00, "3 Bt 8",   "" },
   { 0x00, "4 Bt 7",   "" },
   { 0x00, "4 Bt 8",   "" }
};

/* This shows the names of the I-PAC VE inputs           */
/* This can be used when generating a config file        */
/* There are 60 entries - the first 32 are for unshifted */
/* The last 28 are for shifted                           */
static const struct key_struct ipacve_defaults[] = {
/*The first 32 are unshifted inputs*/
   { 0xF5, "1 Up",     "up arrow" },
   { 0xF4, "1 Right",  "right arrow" },
   { 0x14, "1 Bt 1",   "left ctrl" },
   { 0xEB, "1 Left",   "left arrow" },
   { 0x29, "1 Bt 3",   "space" },
   { 0xF2, "1 Down",   "down arrow" },
   { 0x1A, "1 Bt 5",   "z" },
   { 0x11, "1 Bt 2",   "left alt" },
   { 0x34, "2 Right",  "g" },
   { 0x12, "1 Bt 4",   "left shift" },
   { 0x23, "2 Left",   "d" },
   { 0x22, "1 Bt 6",   "x" },
   { 0x2D, "2 Up",     "r" },
   { 0x2B, "2 Down",   "f" },
   { 0x1C, "2 Bt 1",   "a" },
   { 0x16, "Start 1",  "1" },
   { 0x1B, "2 Bt 2",   "s" },
   { 0x1E, "Start 2",  "2" },
   { 0x15, "2 Bt 3 ",  "q" },
   { 0x2E, "Coin 1 ",  "5" },
   { 0x1D, "2 Bt 4 ",  "w" },
   { 0x36, "Coin 2",   "6" },
   { 0x43, "2 Bt 5",   "i" },
   { 0x21, "1 Bt 7",   "c" },
   { 0x42, "2 Bt 6",   "k" },
   { 0x2A, "1 Bt 8",   "v" },
   { 0x3B, "2 Bt 7",   "j" },
   { 0x4B, "2 Bt 8",   "l" },
   { 0x0E, "1A",       "`" },
   { 0x0D, "1B",       "Tab" },
   { 0x2E, "2A",       "5" },
   { 0x5A, "2B",       "Enter" },
/*Begin shifted inputs*/
   { 0x00, "1 Up",     "" },
   { 0x4D, "1 Right",  "p" },
   { 0x00, "1 Bt 1",   "" },
   { 0x00, "1 Left",   "" },
   { 0x00, "1 Bt 3",   "" },
   { 0x00, "1 Down",   "" },
   { 0x00, "1 Bt 5",   "" },
   { 0x00, "1 Bt 2",   "" },
   { 0x00, "2 Right",  "" },
   { 0x00, "1 Bt 4",   "" },
   { 0x00, "2 Left",   "" },
   { 0x00, "1 Bt 6",   "" },
   { 0x00, "2 Up",     "" },
   { 0x76, "2 Down",   "esc" },
   { 0x00, "2 Bt 1",   "" },
   { 0x00, "Start 1",  "" },
   { 0x00, "2 Bt 2",   "" },
   { 0x00, "Start 2",  "" },
   { 0x00, "2 Bt 3",   "" },
   { 0x00, "Coin 1",   "" },
   { 0x00, "2 Bt 4",   "" },
   { 0x00, "Coin 2",   "" },
   { 0x00, "2 Bt 5",   "" },
   { 0x00, "1A",       "" },
   { 0x00, "2 Bt 6",   "" },
   { 0x00, "1B",       "" },
   { 0x00, "2A",       "" },
   { 0x00, "2B",       "" }
};

/* This shows the names of the I-PAC VE inputs           */
/* This can be used when generating a config file        */
/* There are 64 entries - the first 36 are for unshifted */
/* The last 28 are for shifted                           */
static const struct key_struct minipacve_defaults[] = {
/*The first 36 are unshifted inputs*/
   { 0xF5, "1 Up",     "up arrow" },
   { 0xF4, "1 Right",  "right arrow" },
   { 0x14, "1 Bt 1",   "left ctrl" },
   { 0xEB, "1 Left",   "left arrow" },
   { 0x29, "1 Bt 3",   "space" },
   { 0xF2, "1 Down",   "down arrow" },
   { 0x1A, "1 Bt 5",   "z" },
   { 0x11, "1 Bt 2",   "left alt" },
   { 0x34, "2 Right",  "g" },
   { 0x12, "1 Bt 4",   "left shift" },
   { 0x23, "2 Left",   "d" },
   { 0x22, "1 Bt 6",   "x" },
   { 0x2D, "2 Up",     "r" },
   { 0x2B, "2 Down",   "f" },
   { 0x1C, "2 Bt 1",   "a" },
   { 0x16, "Start 1",  "1" },
   { 0x1B, "2 Bt 2",   "s" },
   { 0x1E, "Start 2",  "2" },
   { 0x15, "2 Bt 3 ",  "q" },
   { 0x2E, "Coin 1 ",  "5" },
   { 0x1D, "2 Bt 4 ",  "w" },
   { 0x36, "Coin 2",   "6" },
   { 0x43, "2 Bt 5",   "i" },
   { 0x21, "1 Bt 7",   "c" },
   { 0x42, "2 Bt 6",   "k" },
   { 0x2A, "1 Bt 8",   "v" },
   { 0x3B, "2 Bt 7",   "j" },
   { 0x4B, "2 Bt 8",   "l" },
   { 0x0E, "1A",       "~ (actually just `)" },
   { 0x0D, "1B",       "Tab" },
   { 0x2E, "1C",       "5" },
   { 0x5A, "1E",       "Enter" },
   { 0x00, "2A",       "" },
   { 0x4D, "2B",       "p" },
   { 0x00, "2C",       "" },
   { 0x00, "2D",       "" },
/*Begin shifted inputs*/
   { 0x00, "1 Up",     "" },
   { 0x4D, "1 Right",  "" },
   { 0x00, "1 Bt 1",   "" },
   { 0x00, "1 Left",   "" },
   { 0x00, "1 Bt 3",   "" },
   { 0x00, "1 Down",   "" },
   { 0x00, "1 Bt 5",   "" },
   { 0x00, "1 Bt 2",   "" },
   { 0x00, "2 Right",  "" },
   { 0x76, "1 Bt 4",   "esc" },
   { 0x00, "2 Left",   "" },
   { 0x00, "1 Bt 6",   "" },
   { 0x00, "2 Up",     "" },
   { 0x00, "2 Down",   "" },
   { 0x00, "2 Bt 1",   "" },
   { 0x00, "Start 1",  "" },
   { 0x00, "2 Bt 2",   "" },
   { 0x00, "Start 2",  "" },
   { 0x00, "2 Bt 3",   "" },
   { 0x00, "Coin 1",   "" },
   { 0x00, "2 Bt 4",   "" },
   { 0x00, "Coin 2",   "" },
   { 0x00, "2 Bt 5",   "" },
   { 0x00, "1A",       "" },
   { 0x00, "2 Bt 6",   "" },
   { 0x00, "1B",       "" },
   { 0x00, "2A",       "" },
   { 0x00, "2B",       "" }
};

static const struct key_struct DEF_SHIFT = {
//   0x12, "Start 1", ""  };
   0x10, "", ""  };


#define FULL_KEY_COUNT  104

// TODO: sort this by keycode??
static const struct key_struct FULL_KEY_LIST[ FULL_KEY_COUNT ] = {
	{ 0x1C, "", "A" },
	{ 0x32, "", "B" },
	{ 0x21, "", "C" },
	{ 0x23, "", "D" },
	{ 0x24, "", "E" },
	{ 0x2B, "", "F" },
	{ 0x34, "", "G" },
	{ 0x33, "", "H" },
	{ 0x43, "", "I" },
	{ 0x3B, "", "J" },
	{ 0x42, "", "K" },
	{ 0x4B, "", "L" },
	{ 0x3A, "", "M" },
	{ 0x31, "", "N" },
	{ 0x44, "", "O" },
	{ 0x4D, "", "P" },
	{ 0x15, "", "Q" },
	{ 0x2D, "", "R" },
	{ 0x1B, "", "S" },
	{ 0x2C, "", "T" },
	{ 0x3C, "", "U" },
	{ 0x2A, "", "V" },
	{ 0x1D, "", "W" },
	{ 0x22, "", "X" },
	{ 0x35, "", "Y" },
	{ 0x1A, "", "Z" },
	{ 0x45, "", "0" },
	{ 0x16, "", "1" },
	{ 0x1E, "", "2" },
	{ 0x26, "", "3" },
	{ 0x25, "", "4" },
	{ 0x2E, "", "5" },
	{ 0x36, "", "6" },
	{ 0x3D, "", "7" },
	{ 0x3E, "", "8" },
	{ 0x46, "", "9" },
	{ 0x0E, "", "`" },
	{ 0x4E, "", "-" },
	{ 0x55, "", "=" },
	{ 0x41, "", "," },
	{ 0x5D, "", "\\" },
	{ 0x66, "", "BKSP" },
	{ 0x29, "", "SPACE" },
	{ 0x0D, "", "TAB" },
	{ 0x58, "", "CAPS" },
	{ 0x12, "", "L SHFT" },
	{ 0x14, "", "L CTRL" },
	{ 0xE0, "", "L GUI" },
	{ 0x11, "", "L ALT" },
	{ 0x59, "", "R SHFT" },
	{ 0xE0, "", "R CTRL" },
	{ 0xE0, "", "R GUI" },
	{ 0xE0, "", "R ALT" },
	{ 0xE0, "", "APPS" },
	{ 0x5A, "", "ENTER" },
	{ 0x76, "", "ESC" },
	{ 0x05, "", "F1" },
	{ 0x06, "", "F2" },
	{ 0x04, "", "F3" },
	{ 0x0C, "", "F4" },
	{ 0x03, "", "F5" },
	{ 0x0B, "", "F6" },
	{ 0x83, "", "F7" },
	{ 0x0A, "", "F8" },
	{ 0x01, "", "F9" },
	{ 0x09, "", "F10" },
	{ 0x78, "", "F11" },
	{ 0x07, "", "F12" },
	{ 0xE0, "", "PRNT  SCRN" },
	{ 0x7E, "", "SCROLL" },
	{ 0xE1, "", "PAUSE" },
	{ 0x54, "", "[" },
	{ 0xE0, "", "INSERT" },
	{ 0xE0, "", "HOME" },
	{ 0xE0, "", "PG UP" },
	{ 0xE0, "", "DELETE" },
	{ 0xE0, "", "END" },
	{ 0xE0, "", "PG DN" },
	{ 0xE0, "", "U ARROW" },
	{ 0xE0, "", "L ARROW" },
	{ 0xE0, "", "D ARROW" },
	{ 0xE0, "", "R ARROW" },
	{ 0x77, "", "NUM" },
	{ 0xE0, "", "KP /" },
	{ 0x7C, "", "KP *" },
	{ 0x7B, "", "KP -" },
	{ 0x79, "", "KP +" },
	{ 0xE0, "", "KP EN" },
	{ 0x71, "", "KP ." },
	{ 0x70, "", "KP 0" },
	{ 0x69, "", "KP 1" },
	{ 0x72, "", "KP 2" },
	{ 0x7A, "", "KP 3" },
	{ 0x6B, "", "KP 4" },
	{ 0x73, "", "KP 5" },
	{ 0x74, "", "KP 6" },
	{ 0x6C, "", "KP 7" },
	{ 0x75, "", "KP 8" },
	{ 0x7D, "", "KP 9" },
	{ 0x5B, "", "]" },
	{ 0x4C, "", ";" },
	{ 0x52, "", "'" },
	{ 0x49, "", "." },
	{ 0x4A, "", "/" }
};

static const struct key_struct macro_defaults[NUM_MACRO_KEYCODES] = {
	{ 0x00, "macro 1(E0), key 1", "" },
	{ 0x00, "macro 1(E0), key 2", "" },
	{ 0x00, "macro 1(E0), key 3", "" },
	{ 0x00, "macro 1(E0), key 4", "" },
	{ 0x00, "macro 2(E1), key 1", "" },
	{ 0x00, "macro 2(E1), key 2", "" },
	{ 0x00, "macro 2(E1), key 3", "" },
	{ 0x00, "macro 2(E1), key 4", "" },
	{ 0x00, "macro 3(E2), key 1", "" },
	{ 0x00, "macro 3(E2), key 2", "" },
	{ 0x00, "macro 3(E2), key 3", "" },
	{ 0x00, "macro 3(E2), key 4", "" },
	{ 0x00, "macro 4(E3), key 1", "" },
	{ 0x00, "macro 4(E3), key 2", "" },
	{ 0x00, "macro 4(E3), key 3", "" },
	{ 0x00, "macro 4(E3), key 4", "" },
};


int htoi(char *hexstr);
  
int ReadCfg(cmdline_opt_s *settings, char *cfg_data);

int GotoSection(FILE *stream, char *section);

int ReadNextLine(FILE *stream, char *buf_line);

int WriteDefaultCfg(char *filename, int board_type);

/*
int WriteCfgFile(char *filename, struct key_struct *data_def, 
                 struct key_struct shift_def, struct key_struct *macro_def,
		 int board_type);
*/

