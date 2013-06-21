/*************************************************************************
 * TLV 02/07/2012                                                        *
 * Copyright (C) 2012 Travis Veldkamp                                    *
 *                                                                       *
 * This program is for programming the I-PAC keyboard encoder            *
 * For more information, see http://www.ultimarc.com                     *
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


#include "cfg_util.h"
#include "cmd_line.h"
//#include "ipac_prog.h"
#include "string.h"
#include "util.h"


static const int MAJOR_VERSION = 0;
static const int MINOR_VERSION = 7;
static const int VERSION_DATE  = 20120216;

/*************************************************************************
 * GLOBAL VARIABLES                                                      *
 *************************************************************************/

/*This is set by the signal handler (sig_handle()) when sig_int is caught*
 *This allows us to clean up before falling out                          */
int want_out=0;

/*************************************************************************
 * END GLOBAL VARIABLES                                                  *
 *************************************************************************/



/***********************************************************
 * Simple signal handler to set global variable indicating *
 * that the user wants out, but allows us time to clean up *
 ***********************************************************/
void sig_handle(int signal) {

   if (signal == SIGINT) {
      printf("Exiting.... please wait while cleaning up\n");
      want_out = 1;
   }
}

void PrintWelcome() {
   printf("ipacutil v%d.%02d - %d\n", MAJOR_VERSION,
           MINOR_VERSION, VERSION_DATE);
   printf("I-PAC USB keyboard encoder programming utility for Linux\n");
   printf("Copyright (C) 2012 Travis Veldkamp\n\n");
}

void ShowUsage() {
   PrintWelcome();

   cmdline_ShowUsage();
}

int main(int argc, char **argv) {

   int error=0;
   char cfg_data[IPAC_DATA_LEN*2];  /* big enough for IPAC-4 */
   int i;
   int shift_code;
   /*The 96 key codes to program the I-PAC with*/
   char keycodes[IPAC_DATA_LEN];
   int rc;
   char *cfg_file;
   struct cmdline_struct settings;
   int control = 0;
   FILE *loghdl = openlog("ipacutil.log");
   int numipacs;

   if (loghdl == NULL) { 
       loghdl = stdout; 
       printf("Failed to open ipacutil.log.  Dumping garbage to screen\n");
   };

   if (parse_cmdline(argc, argv, &settings) < 0) {
      printf("\n");
      settings.showhelp = 1;
   }

   if (settings.showhelp) {
      ShowUsage();
      exit(1);
   }

   if ((strlen(settings.config_file) == 0) &&
       (settings.run_detection == FALSE) && (settings.reset_defaults == FALSE)) {
      printe("Config file not specified\n\n");
      ShowUsage();
      exit(1);
   } else {
      /*Config file specified... see if they want to write default or
        read specified*/
      if (settings.generate_config_file) {

         if (settings.board_type == BOARD_TYPE_INVALID) {
            printe("board type not specified -- can't write config\n");
            exit(1);
         }
         if (WriteDefaultCfg(settings.config_file, settings.board_type) >= 0) {
            printf("Created new config file %s for board type %s\n", settings.config_file, board_name_by_type[settings.board_type]);
         } else {
            printe("Failed to create config file: %s\n", settings.config_file);
         }
         exit(1);
      }
   }

   if (getuid()) {
      printe("This program can only be run as root\n");
      exit(1);
   }


   /*Register the signal handler for sig int (ctrl-c)*/
   if (signal(SIGINT, &sig_handle) == SIG_ERR) {
      printl(settings.verbose, VERBOSE_VERBOSE, loghdl,
             "Error registering new signal handler\n");
      error = 1;
      want_out = 1;
   }


   numipacs = ipac_count();

   if (settings.run_detection) {
      int idx;

      if (settings.verbose) PrintWelcome();

      printv(settings.verbose, VERBOSE_NORMAL, "Attempting to detect I-PACs\n");

      printv(settings.verbose, VERBOSE_NORMAL, "Detect: found %d I-PACs\n", numipacs);

      if (numipacs < 1)
      {
          printv(settings.verbose, VERBOSE_NORMAL, "Error: no I-PACs found\n");
      }
      
      for(idx=0; idx<numipacs; idx++) {
         rc = detect_ipac_model(idx, loghdl);
     	 printl(0, 0, loghdl, "detect returned: %d\n", rc);
         if (rc > -1) {
            printf("I-PAC Board Number %d: %s\n", idx, board_name_by_type[rc]);
         } else {
            printf("WARNING: Failed to detect I-PAC model for board %d\n", idx);
            exit(1);
         }
      }

      printv(settings.verbose, VERBOSE_NORMAL, "Detection complete\n");
      exit(0);
   }

   if (settings.no_program) {
      /* User has asked for no programming step*/
      /* This is most likely used when the user wants to detect their board*/
      printv(settings.verbose, VERBOSE_NORMAL,
             "No programming cycle requested\n");
      printv(settings.verbose, VERBOSE_VERBOSE, "Exiting.\n");
      want_out = 1;
   }


   printv(settings.verbose, VERBOSE_NORMAL,
          "Programming I-PAC using %s for configuration\n",
          settings.config_file);

   if ((!error) && (!want_out)) {
      if (settings.verbose) PrintWelcome();

      printv(settings.verbose, VERBOSE_VERBOSE,
             "Using %s for keycode data\n", settings.config_file);

      //cfg_data.key_code = keycodes;
      printv(settings.verbose, VERBOSE_VERBOSE,
             "Reading in keycode data\n");

      /* Read in the keycode data */
      if (ReadCfg(&settings, cfg_data) < 0) {
         printe("Error reading in configuration data\n");
         error = 1;
         want_out = 1;
      } else {
         /* Read in the shift code data */
         shift_code = cfg_data[SHIFT_CODE_OFFSET];
         printv(settings.verbose, VERBOSE_VERBOSE, "Shift code is 0x%02X\n", shift_code);
         printv(settings.verbose, VERBOSE_VERBOSE, "Board type from config file is 0x%02X\n", settings.board_type);
      }
   }

   if (settings.board_number >= numipacs) {
      printe("Board %d specified, but only found %d I-PAC(s).  Numbering starts at 0\n", settings.board_number, numipacs);
      exit(1);
   }

   /* Now, try to program the I-PAC */
   if ((!error) && (!want_out)) {
      printv(settings.verbose, VERBOSE_NORMAL, "Programming I-PAC... " );

   } else {
      exit(1);
   } 

   if (settings.board_type < 0) {
      printe("No board type is specified.  Either use the -t option or a config file\n");
      exit(1);
   }

   if (settings.reset_defaults) {
      control |= CONTROL_FACTORY_DEFAULTS;
   }
   if (settings.ram_only) {
      control |= CONTROL_RAM_ONLY;
   }
   error = program_ipac(settings.board_number, settings.board_type, 
                        cfg_data, TRUE, control, loghdl);
   printl(0, 0, loghdl, "program_ipac returned: %d\n", error);

   if (loghdl != stdout) {
      closelog(loghdl);
   }
   
   /* Try to get kernel driver to reclaim device */
   re_enumerate(0, loghdl);

   if (error) {
      printf("** Programming FAILED **\n");
      return 1;
   } else {
      printf("** I-PAC has been reprogrammed **\n");
      return 0;
   }

}
