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
/*!
 * \def MAX_COMMAND_FILE_NESTING
 * limits number of nested SUBMIT calls to 10.  A SUBMIT redirects the input
 * to file, which in turn may temporarily redirect input to another file, and
 * so on.
 */
#define MAX_COMMAND_FILE_NESTING 10
/*!
 * \var long g_commandFileNestingLevel
 * current level of nested SUBMIT commands.  0 is top level and refers to stdin
 * (usually the user controlled command line).  Any invocation of SUBMIT
 * increases this value by 1.  A return from a SUBMIT decrases it by 1 again.
 */
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
/*!
 * \var flag g_scrollMode
 * \brief controls whether output stops after a full page is printed.
 *
 * A value of 1 indicates the user wants prompted page wise output.
 * The command SET SCROLL controls this value.  If followed by CONTINUOUS, this
 * flag is reset to 0.
 */
extern flag g_scrollMode; /* Flag for continuous or prompted scroll */
/*!
 * \var flag g_quitPrint
 * The value 1 indicates the user entered a 'q' at the last scrolling prompt.
 */
extern flag g_quitPrint; /* Flag that user typed 'q' to last scrolling prompt */

/* printLongLine automatically puts a newline \n in the output line. */
void printLongLine(const char *line, const char *startNextLine, const char *breakMatch);
/*!
 * \brief requests a line of text from the __stream__.
 *
 * If not suppressed, displays a prompt text on the screen.  Then reads a
 * single line from the __stream__.  Returns this line as a \a vstring.
 *
 * A line in the __stream__ is terminated by a LF character (0x0D) character
 * alone.  It is read, but removed from the result.  Its maximum length is
 * given by CMD_BUFFER_SIZE - 1 (1999).  Reaching EOF (end of file) is
 * equivalent to reading LF, if at least 1 byte was read before.  Note that the
 * NUL character can be part of the line.  This will not lead to an error, but
 * truncate the line at that position if interpreted in the usual manner.
 *
 * Reading from an empty __stream__ (or one that is at EOF position) returns
 * NULL, not the empty string, and is formally signalled as an error.
 * Overflowing the buffer is also an error.  No truncated value is returned.
 *
 * If scrolling is enabled, the input is interpreted.  A line consisting of a
 * single character b or B indicates the user wants to scroll back through
 * saved pages of output.
 *
 * No timeout is applied when waiting for user input from the console.
 * \param[in] stream (not null) source to read the line from.  _stdin_ is
 *   common for user input from the console. 
 * \param[in] ask prompt text displayed on the screen before __stream__ is
 *   read.  This prompt can be suppressed by either a NULL value, or a
 *   setting of \a g_commandFileSilentFlag to 1.  It may be compared to
 *   \a g_commandPrompt.  If both match, it is inferred the user is not
 *   scrolling through a lengthy text any more.
 * \return a \a vstring containing the read (or interpreted) line.  The result
 *   needs to be deallocated by the caller, if not empty or  NULL.
 * \pre
 *   The following variables are honored during execution and should be properly
 *   set:
 *   - \a commandFileSilentFlag value 1 suppresses prompts altogether, not only
 *     those used for scrolling through long text;
 *   - \a g_commandFileNestingLevel a value > 0 indicates a SUBMIT call is
 *     executing, where scrolling prompts are suppressed;
 *   - \a backBuffer may contain text to display on scroll back operations;
 *   - \a g_scrollMode value 1 enables scrolling back through text held in
 *     \a backBuffer;
 *   - \a localScrollMode a value of 0 temporarily disables scrolling, despite
 *     the setting in \a g_scrollMode;
 *   - \a g_commandPrompt if its string matches ask, this can terminate a
 *     scroll loop.
 * \post
 *   \a db is updated and includes the length of the interpreted input.
 * \warning the calling program must deallocate the returned string (if not
 *   null or empty).  Note that the result can be NULL.  This is outside of the
 *   usual behavior of a \a vstring type.
 * \bug It is possible that the first character read from __stream__ is NUL,
 *   for example when a file is read.  Reading only stops at a LF or EOF or
 *   buffer end.  NUL will cause a print of an error message, but execution
 *   continues and in the wake may cause all kind of undefined behavior, like
 *   memory accesses beyond allocated buffers.
 */
vstring cmdInput(FILE *stream, const char *ask);
/*!
 * gets a line from either the terminal or the command file stream depending on
 * g_commandFileNestingLevel > 0.  It calls cmdInput().
 * \param ask text displayed before input prompt.  This can be located in
 *   \a tempAllocStack.  If this text contains more than \a g_screenWidth
 *   characters, it is wrapped preferably at space characters and split across
 *   multiple lines.  The final line leaves space for enough for a ten
 *   character user input
 * \return the entered input.
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
