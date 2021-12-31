/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMINOU_H_
#define METAMATH_MMINOU_H_

#include <stdio.h>

#include "mmvstr.h"
#include "mmdata.h"

extern int g_errorCount;     /* Total error count */

/* Global variables used by print2() */
extern flag g_logFileOpenFlag;
extern FILE *g_logFilePtr;
extern FILE *g_listFile_fp;
/* Global variables used by print2() */
extern flag g_outputToString;
extern vstring g_printString;
/* Global variables used by cmdInput() */
#define MAX_COMMAND_FILE_NESTING 10
extern long g_commandFileNestingLevel;
extern FILE *g_commandFilePtr[MAX_COMMAND_FILE_NESTING + 1];
extern vstring g_commandFileName[MAX_COMMAND_FILE_NESTING + 1];
extern flag g_commandFileSilent[MAX_COMMAND_FILE_NESTING + 1];
extern flag g_commandFileSilentFlag;
                                    /* 23-Oct-2006 nm For SUBMIT ... /SILENT */

extern FILE *g_input_fp;  /* File pointers */
extern vstring g_input_fn, g_output_fn;  /* File names */

/* Warning:  never call print2 with string longer than PRINTBUFFERSIZE - 1 */
/* print2 returns 0 if the user has quit the printout. */
flag print2(char* fmt,...);
extern long g_screenHeight; /* Height of screen */
extern long g_screenWidth; /* Width of screen */
#define MAX_LEN 79 /* Default width of screen */
#define SCREEN_HEIGHT 23 /* Lines on screen, minus 1 to account for prompt */
extern flag g_scrollMode; /* Flag for continuous or prompted scroll */
extern flag g_quitPrint; /* Flag that user typed 'q' to last scrolling prompt */

/* printLongLine automatically puts a newline \n in the output line. */
void printLongLine(vstring line, vstring startNextLine, vstring breakMatch);
vstring cmdInput(FILE *stream,vstring ask);
vstring cmdInput1(vstring ask);

enum severity {notice_,warning_,error_,fatal_};
void errorMessage(vstring line, long lineNum, long column, long tokenLength,
  vstring error, vstring fileName, long statementNum, flag warnFlag);

/* Opens files with error message; opens output files with
   backup of previous version.   Mode must be "r" or "w". */
FILE *fSafeOpen(vstring fileName, vstring mode, flag noVersioningFlag);

/* Renames a file with backup of previous version.  If non-zero
   is returned, there was an error. */
int fSafeRename(vstring oldFileName, vstring newFileName);

/* Finds the name of the first file of the form filePrefix +
   nnn + ".tmp" that does not exist.  THE CALLER MUST DEALLOCATE
   THE RETURNED STRING. */
vstring fGetTmpName(vstring filePrefix);

/* This function returns a character string containing the entire contents of
   an ASCII file, or Unicode file with only ASCII characters.   On some
   systems it is faster than reading the file line by line.  THE CALLER
   MUST DEALLOCATE THE RETURNED STRING.  If a NULL is returned, the file
   could not be opened or had a non-ASCII Unicode character or some other
   problem.   If verbose is 0, error and warning messages are suppressed. */
/* 31-Dec-2017 nm Add charCount return argument */
vstring readFileToString(vstring fileName, char verbose, long *charCount);

/* Returns total elapsed time in seconds since starting session (for the
   lcc compiler) or the CPU time used (for the gcc compiler).  The
   argument is assigned the time since the last call to this function. */
double getRunTime(double *timeSinceLastCall);

/* Call before exiting to free memory allocated by this module */
void freeInOu(void);

#endif /* METAMATH_MMINOU_H_*/
