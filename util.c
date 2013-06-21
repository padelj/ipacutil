/*************************************************************************
 * TLV 02/18/2002                                                        *
 *                                                                       *
 *************************************************************************/

#include "util.h"


/**********************************************
 * Print verbose - take a verbosity level and *
 * print if we exceed some specified limit    *
 * The code for this routine is based on:     *
 * http://www.eskimo.com/~scs/C-faq/q15-5.html*
 **********************************************/
void printv(int verbosity, int trigger, char *fmt, ...) {

     va_list argp;

     /*Currently, there are only 2 settings - verbose or not verbose*/
     if (verbosity >= trigger) {
        va_start(argp, fmt);
        vprintf(fmt, argp);
        va_end(argp);
     }

}

/**********************************************
 * Print to log -  take a verbosity level and *
 * print if we exceed some specified limit    *
 * The code for this routine is based on:     *
 * http://www.eskimo.com/~scs/C-faq/q15-5.html*
 **********************************************/
void printl(int verbosity, int trigger, FILE *loghdl, char *fmt, ...) {

     va_list argp;

     /*Currently, there are only 2 settings - verbose or not verbose*/
     if (verbosity >= trigger) {
        va_start(argp, fmt);
        vfprintf(loghdl, fmt, argp);
        va_end(argp);
     }
}

/**********************************************
 * Print error - print an error message in a  *
 * standard format                            *
 * The code for this routine is from:         *
 * http://www.eskimo.com/~scs/C-faq/q15-5.html*
 **********************************************/
void printe(char *fmt, ...) {

   va_list argp;
   fprintf(stderr, "Error: ");
   va_start(argp, fmt);
   vfprintf(stderr, fmt, argp);
   va_end(argp);
}

FILE *openlog(char *logfile) {
   return fopen(logfile, "w");
}

void closelog(FILE *loghdl) {
   fclose(loghdl);
}
