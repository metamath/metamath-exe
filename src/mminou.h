/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file
 * Basic input and output interface.
 */

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
/*!
 * \var g_outputToString
 * 
 * Global variable redirecting the output of the function print2 from the
 * console ( = 0) to a string ( = 1).
 */
extern flag g_outputToString;
extern vstring g_printString;
/* Global variables used by cmdInput() */
#define MAX_COMMAND_FILE_NESTING 10
extern long g_commandFileNestingLevel;
extern FILE *g_commandFilePtr[MAX_COMMAND_FILE_NESTING + 1];
extern vstring g_commandFileName[MAX_COMMAND_FILE_NESTING + 1];
extern flag g_commandFileSilent[MAX_COMMAND_FILE_NESTING + 1];
/*!
 * \var g_commandFileSilentFlag
 * If set to 1, suppresses prompts on input.
 */
extern flag g_commandFileSilentFlag; /* For SUBMIT ... /SILENT */

extern FILE *g_input_fp;  /* File pointers */
extern vstring g_input_fn, g_output_fn;  /* File names */

/* Warning:  never call print2 with string longer than PRINTBUFFERSIZE - 1 */
/* print2 returns 0 if the user has quit the printout. */
flag print2(const char* fmt,...);
extern long g_screenHeight; /* Height of screen */
/*!
 * \var long g_screenWidth
 * The minimum width of the display, measured in fixed width characters.
 */
extern long g_screenWidth; /* Width of screen */
/*!
 * \def MAX_LEN
 * \brief Default width of screen
 *
 * Number of characters that can always be displayed in a single line.  This
 * notion is reminiscent of CRT tubes with a fixed width character set.
 * Graphical Displays on a notebook for example can display much more, but on
 * some mobile devices this may be reduced to 30-40 characters.
 */
#define MAX_LEN 79 /* Default width of screen */
#define SCREEN_HEIGHT 23 /* Lines on screen, minus 1 to account for prompt */
extern flag g_scrollMode; /* Flag for continuous or prompted scroll */
extern flag g_quitPrint; /* Flag that user typed 'q' to last scrolling prompt */

/* printLongLine automatically puts a newline \n in the output line. */
void printLongLine(const char *line, const char *startNextLine, const char *breakMatch);
vstring cmdInput(FILE *stream, const char *ask);
/*!
 * gets a line from either the terminal or the command file stream depending on
 * g_commandFileNestingLevel > 0.  It calls cmdInput().
 * \param ask text displayed before input prompt.  This can be located in
 *   \a tempAllocStack.  If this text contains more than \a g_screenWidth
 *   characters, it is wrapped preferably at space characters and split across
 *   multiple lines.  The final line leaves space for enough for a ten
 *   character user input
 * \returns the entered input.
 * \warning the calling program must deallocate the returned string.
 */
vstring cmdInput1(const char *ask);
flag cmdInputIsY(const char *ask);

enum severity {notice_,warning_,error_,fatal_};
void errorMessage(vstring line, long lineNum, long column, long tokenLength,
  vstring error, vstring fileName, long statementNum, flag warnFlag);

/* Opens files with error message; opens output files with
   backup of previous version.   Mode must be "r" or "w". */
FILE *fSafeOpen(const char *fileName, const char *mode, flag noVersioningFlag);

/* Renames a file with backup of previous version.  If non-zero
   is returned, there was an error. */
int fSafeRename(const char *oldFileName, const char *newFileName);

/* Finds the name of the first file of the form filePrefix +
   nnn + ".tmp" that does not exist.  THE CALLER MUST DEALLOCATE
   THE RETURNED STRING. */
vstring fGetTmpName(const char *filePrefix);

/* This function returns a character string containing the entire contents of
   an ASCII file, or Unicode file with only ASCII characters.   On some
   systems it is faster than reading the file line by line.  THE CALLER
   MUST DEALLOCATE THE RETURNED STRING.  If a NULL is returned, the file
   could not be opened or had a non-ASCII Unicode character or some other
   problem.   If verbose is 0, error and warning messages are suppressed. */
vstring readFileToString(const char *fileName, char verbose, long *charCount);

/* Returns total elapsed time in seconds since starting session (for the
   lcc compiler) or the CPU time used (for the gcc compiler).  The
   argument is assigned the time since the last call to this function. */
double getRunTime(double *timeSinceLastCall);

/* Call before exiting to free memory allocated by this module */
void freeInOu(void);

#endif /* METAMATH_MMINOU_H_*/
