/*************************************************************************
 * TLV 02/18/2002                                                        *
 *************************************************************************/


#include <stdio.h>
#include <stdarg.h>

#define PRINT_ALWAYS  2
#define PRINT_NORMAL  1
#define PRINT_VERBOSE 0

#define VERBOSE_VERBOSE 2
#define VERBOSE_NORMAL  1
#define VERBOSE_QUIET   0

#define TRUE 1
#define FALSE 0

void printv(int verbosity, int trigger, char *fmt, ...);

/* Print log */
void printl(int verbosity, int trigger, FILE *loghdl, char *fmt, ...);
FILE *openlog(char *filename);
void closelog(FILE *loghandle);

void printe(char *fmt, ...);
