/*************************************************************************
 * Travis Veldkamp 02/07/2012                                            *
 * ipacutil@zumbrovalley.net                                             *
 * Copyright (C) 2012  Travis Veldkamp                                   *
 *                                                                       *
 * VERY simple routine for parsing the command line                      *
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


#include "cmd_line.h"
#include "ipac_prog.h"
#include <string.h>
#include "util.h"

/****************************************************
 * Do a simple-minded parse of the command line     *
 *                                                  *
 * Returns: 0 if completed w/o error, -1 on error   *
 ****************************************************/
int parse_cmdline(int argc, char **argv, cmdline_opt_s *settings) {

   int i;
   FILE *temp_stream;

   /* Setup defaults... try to pick reasonable values */
   settings->showhelp = FALSE;
   settings->verbose = VERBOSE_NORMAL;
   settings->generate_config_file = FALSE;
   settings->config_file[0] = '\0';
   settings->board_number= 0;             /* default to 1st board          */
   settings->run_detection = FALSE;       /* default is no board detection */
   settings->no_program = FALSE;          /* default is to try to program  */
   settings->board_type = BOARD_TYPE_INVALID;
   settings->reset_defaults = 0;          /* Default is to program user data */
   settings->ram_only = 0;                /* default is to update flash      */

   for (i=1; i<argc;i++) {
      if ((strcmp(argv[i], HELP_FLAG1) == 0) ||
          (strcmp(argv[i], HELP_FLAG2) == 0)) {

         settings->showhelp = 1;
         continue;
      }

      if ((strcmp(argv[i], VERBOSE_FLAG1) == 0) ||
          (strcmp(argv[i], VERBOSE_FLAG2) == 0)) {

         settings->verbose = VERBOSE_VERBOSE;
         continue;
      }

      if ((strcmp(argv[i], QUIET_FLAG1) == 0) ||
          (strcmp(argv[i], QUIET_FLAG2) == 0)) {
         settings->verbose = VERBOSE_QUIET;
         continue;
      }

      if ((strcmp(argv[i], DEFAULT_FLAG1) == 0) ||
          (strcmp(argv[i], DEFAULT_FLAG2) == 0)) {
         settings->reset_defaults = 1;  /* Won't use config file */
         continue;
      }

      if ((strcmp(argv[i], RAMONLY_FLAG1) == 0) ||
          (strcmp(argv[i], RAMONLY_FLAG2) == 0)) {
         settings->ram_only = 1; /* settings not saved across power cycle */
         continue;
      }

      if ((strcmp(argv[i], BOARD_FLAG1) == 0) ||
          (strcmp(argv[i], BOARD_FLAG2) == 0)) {

         /*The user is specifying which board to use*/
         i++;

         if (argc <= i) {
             printf("No board specified for %s\n", argv[i-1]);
             return -1;
         } else {
             settings->board_number = atoi(argv[i]);
             if ((settings->board_number < 0) || (settings->board_number > MAX_IPACS)) {
                 printf("**Invalid board specified: %s. **\n",
                        argv[i]);
                 return -1;
             }
         }

         continue;
      }

      if ((strcmp(argv[i], BD_TYPE_FLAG1) == 0) ||
          (strcmp(argv[i], BD_TYPE_FLAG2) == 0)) {

         char board_str[255];
         //printf("Found board type flag\n");

         i++;

         /*The next entry should be the specified board type*/
         if (argc > i) {
            strncpy(board_str, argv[i], 254);
            printf("Board type specified: %s\n", board_str);
            if (strncmp(board_str, BOARD_STR_IPAC2, 254)            == 0) {
                settings->board_type = BOARD_TYPE_IPAC2_28;

            } else if (strncmp(board_str, BOARD_STR_IPAC2_32, 254)   == 0) {
                settings->board_type = BOARD_TYPE_IPAC2_32;

            } else if (strncmp(board_str, BOARD_STR_IPAC4, 254)     == 0) {
                settings->board_type = BOARD_TYPE_IPAC4_56;

            } else if (strncmp(board_str, BOARD_STR_IPACVE, 254)    == 0) {
                settings->board_type = BOARD_TYPE_IPACVE;

            } else if (strncmp(board_str, BOARD_STR_MINIPACVE, 254) == 0) {
                settings->board_type = BOARD_TYPE_MINIPACVE;

            } else {
                printf("**Error: Invalid board type specified: %s**\n", board_str);
                return -1;
            }
         } else {
            printf("**Error: No board type specified for %s**\n",
                   argv[i-1]);
            return -1;
         }
         continue;
      }

      if ((strcmp(argv[i], NO_PGM_FLAG1) == 0) ||
          (strcmp(argv[i], NO_PGM_FLAG2) == 0)) {

         /*User is specifying no programming cycle*/
         /*This is probably only useful when combined*/
         /*with the detect flag*/
         settings->no_program = 1;
         continue;
      }

      if ((strcmp(argv[i], GEN_CFG_FLAG1) == 0) ||
          (strcmp(argv[i], GEN_CFG_FLAG2) == 0)) {

         /*User has asked for a default config file to be built*/
         settings->generate_config_file = 1;

         i++;

         /*The next entry should be the specified file*/
         if (argc > i) {
            /*Maybe we should check if file already exists so we don't
              clobber anything*/
            strncpy(settings->config_file, argv[i], CFG_FILENAME_LEN);
            //i++;
         } else {
            printf("**Error: No output file specified for %s**\n",
                   argv[i-1]);
            return -1;
         }
         continue;
      }

      if ((strcmp(argv[i], CONFIG_FLAG1) == 0) ||
          (strcmp(argv[i], CONFIG_FLAG2) == 0)) {

         /*The next entry should be the specified file*/
         i++;

         if (argc > i) {
            /*verify that the file exists*/
            if ((temp_stream = fopen(argv[i], "r")) == NULL) {
               printf("**Error: config file doesn't exist: %s**\n",
                      argv[i]);
               return -1;
            } else {
               fclose(temp_stream);
               strncpy(settings->config_file, argv[i], CFG_FILENAME_LEN);
            }
         } else {
            printf("**Error: No output file specified for %s**\n", argv[i-1]);
            return -1;
         }

         continue;
      }

      if ((strcmp(argv[i], DETECT_FLAG1) == 0) ||
          (strcmp(argv[i], DETECT_FLAG2) == 0)) {
         /* Warning: new I-PACs no longer support detection */
         settings->run_detection = 1;
         continue;
      }

      /*If we get to this point, we didn't match the argument with anything*/
      printf("**Error: unknown command line argument found: %s**\n", argv[i]);
      return -1;
   }
   return 0;
}

void cmdline_ShowUsage() {
   printf("Usage: ipacutil [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s] [%s board_num] [%s board_num] [%s config_file] [%s config_file] [%s config_file] [%s config_file] [%s type] [%s type]\n",
          HELP_FLAG1, HELP_FLAG2, VERBOSE_FLAG1, VERBOSE_FLAG2, QUIET_FLAG1, QUIET_FLAG2,
          DETECT_FLAG1, DETECT_FLAG2, NO_PGM_FLAG1, NO_PGM_FLAG2, 
          DEFAULT_FLAG1, DEFAULT_FLAG2, RAMONLY_FLAG1, RAMONLY_FLAG2,
          BOARD_FLAG1, BOARD_FLAG2, CONFIG_FLAG1, CONFIG_FLAG2, GEN_CFG_FLAG1, GEN_CFG_FLAG2,
          BD_TYPE_FLAG1, BD_TYPE_FLAG2);
   printf("%s | %s\t\t-- print this information\n", HELP_FLAG1, HELP_FLAG2);
   printf("%s | %s\t\t-- turn on verbose mode\n", VERBOSE_FLAG1, VERBOSE_FLAG2);
   printf("%s | %s\t\t-- don't print anything to screen except errors\n", QUIET_FLAG1, QUIET_FLAG2);
   printf("%s | %s\t\t-- detect connected I-PACs\n", DETECT_FLAG1, DETECT_FLAG2);
   printf("%s | %s\t\t-- reset to factory default program\n", DEFAULT_FLAG1, DEFAULT_FLAG2);
   printf("%s | %s\t\t-- update RAM only -- don't save programming across power cycle\n", RAMONLY_FLAG1, RAMONLY_FLAG2);
   printf("%s | %s\t-- don't program (probably not useful any more)\n", NO_PGM_FLAG1, NO_PGM_FLAG2);
   printf("%s board_num\t\t-- program board specified (from detection)\n", BOARD_FLAG1);
   printf("%s board_num\t-- program board specified (from detection)\n", BOARD_FLAG2);
   printf("%s config_file\t\t-- use config file to program the ipac\n", CONFIG_FLAG1);
   printf("%s cfg_file\t-- use config file to program the ipac\n", CONFIG_FLAG2);
   printf("%s config_file\t\t-- generate a default config file called config_file\n", GEN_CFG_FLAG1);
   printf("%s cfg_file\t-- generate a default config file called cfg_file\n", GEN_CFG_FLAG2);
   printf("%s type\t\t\t-- force type of board (only used for %s/%s)\n", BD_TYPE_FLAG1, GEN_CFG_FLAG1, GEN_CFG_FLAG2);
   printf("%s type\t\t-- force type of board (only used for %s/%s)\n", BD_TYPE_FLAG2, GEN_CFG_FLAG1, GEN_CFG_FLAG2);
   printf("\t\tValid board types: %s, %s, %s, %s, %s\n", BOARD_STR_IPAC2, BOARD_STR_IPAC2_32, BOARD_STR_IPAC4, BOARD_STR_IPACVE, BOARD_STR_MINIPACVE);
   printf("\nThis is free software; see the source for copying conditions.  There is NO\n");
   printf("warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n");

}
