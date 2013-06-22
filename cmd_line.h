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
 * along with Foobar; if not, write to:                                  *
 *                   Free Software Foundation, Inc.                      *
 *                   59 Temple Place                                     *
 *                   Suite 330                                           *
 *                   Boston, MA  02111-1307                              *
 *                   USA                                                 *
 *************************************************************************/

#ifndef CMD_LINE_H
#define CMD_LINE_H

#include <stdio.h>


#define CFG_FILENAME_LEN 512


typedef struct cmdline_struct {
   int  verbose;
   char config_file[CFG_FILENAME_LEN];
   int  showhelp;
   int  generate_config_file;
   int  board_number;
   int  run_detection;
   int  no_program;
   int  board_type;
   int  reset_defaults;
   int  ram_only;
} cmdline_opt_s;

static const char *HELP_FLAG1    = "-h";
static const char *HELP_FLAG2    = "--help";

static const char *VERBOSE_FLAG1 = "-v";
static const char *VERBOSE_FLAG2 = "--verbose";

static const char *GEN_CFG_FLAG1 = "-w";
static const char *GEN_CFG_FLAG2 = "--write_cfg";

static const char *CONFIG_FLAG1  = "-c";
static const char *CONFIG_FLAG2  = "--config";

static const char *DETECT_FLAG1  = "-d";
static const char *DETECT_FLAG2  = "--detect";

static const char *BOARD_FLAG1  = "-b";
static const char *BOARD_FLAG2  = "--board";

static const char *NO_PGM_FLAG1  = "-n";
static const char *NO_PGM_FLAG2  = "--no_program";

static const char *QUIET_FLAG1 = "-q";
static const char *QUIET_FLAG2 = "--quiet";

static const char *BD_TYPE_FLAG1 = "-t";
static const char *BD_TYPE_FLAG2 = "--type";

static const char *DEFAULT_FLAG1 = "-r";
static const char *DEFAULT_FLAG2 = "-reset";

static const char *RAMONLY_FLAG1 = "-a";
static const char *RAMONLY_FLAG2 = "--ram";


static const char *BOARD_STR_IPAC2     = "ipac2";
static const char *BOARD_STR_IPAC2_32  = "ipac2_32";
static const char *BOARD_STR_IPAC4     = "ipac4";
static const char *BOARD_STR_IPACVE    = "ipacve";
static const char *BOARD_STR_MINIPACVE = "minipacve";


int parse_cmdline(int argc, char **argv, cmdline_opt_s *settings);
#endif
