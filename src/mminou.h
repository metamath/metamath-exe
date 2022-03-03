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
 * limits number of nested SUBMIT calls.  A SUBMIT redirects the input to a
 * file, which in turn may temporarily redirect input to another file, and so
 * on.
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
 * line from the __stream__.  This line may be interpreted as described further
 * below, in which case the prompt is reprinted and the next line is read.
 * Returns the first not interpreted line as a \a vstring.
 *
 * A line in the __stream__ is terminated by a LF character (0x0D) character
 * alone.  It is read, but removed from the result.  The maximum line length
 * without the LF is \a CMD_BUFFER_SIZE - 1.  Reaching EOF (end of file) is
 * equivalent to reading LF, if at least 1 byte was read before.  Note that the
 * NUL character can be part of the line.  Reading a NUL is not sufficiently
 * supported in the current implementation and may or may not cause an error
 * message or even undefined behavior.
 *
 * Reading from an empty __stream__ (or one that is at EOF position) returns
 * NULL, not the empty string, and is formally signalled as an error.
 * Overflowing the buffer is also an error.  No truncated value is returned.
 *
 * This routine interprets some input without returning it to the caller under
 * following two conditions:
 *
 * 1. If scrolling is enabled, a line consisting of a single character b or B
 * may indicate the user wants to scroll back through saved pages of output.
 * This is handled within this routine, if possible.  If it cannot be served,
 * the b or B is returned as common user input.
 *
 * 2. The user hits ENTER (only) while prompted in top level context.  The
 * empty line is not returned.
 * 
 * No timeout is applied while waiting for user input from the console.
 *
 * Detected format errors result in following bug messages:
 *   - 1507: The first read character is NUL
 *   - 1508: line overflow, the last character is not NUL
 *   - 1519: padding of LF failed, or first read character was NUL
 *   - 1521: a NUL in first and second position was read
 *   - 1523: NULL instead of a prompt text when user input is required
 *   - 1525: missing terminating LF, not caused by an EOF.
 *
 *   A bug message need not result in an execution stop.  It is not directed to
 *   the metamath bug function to avoid stacking up calls (bug calling cmdInput
 *   again for scrolling etc.).
 *
 * \todo clarify recursive call to print2 and the role of backFromCmdInput. 
 * \param[in] stream (not null) source to read the line from.  _stdin_ is
 *   common for user input from the console. 
 * \param[in] ask prompt text displayed on the screen before __stream__ is
 *   read.  This prompt is suppressed by either a NULL value, or setting 
 *   \a g_commandFileSilentFlag to 1.  This prompt must be not NULL (empty is
 *   fine!) outside of a SUBMIT call, where user is expected to enter input.
 *   \n
 *   It may be compared to \a g_commandPrompt.  If both match, it is inferred
 *   the user is in top level command mode, where empty input is not returned
 *   to the caller.
 * \return a \a vstring containing the first read and not interpreted line.
 *   NULL indicates an error condition.  The result needs to be deallocated by
 *   the caller, if not empty or NULL.
 * \pre
 *   The following variables are honored during execution and should be properly
 *   set:
 *   - \a commandFileSilentFlag value 1 suppresses prompts altogether, not only
 *     those used for scrolling through long text;
 *   - \a g_commandFileNestingLevel a value > 0 indicates a SUBMIT call is
 *     executing, where scrolling prompts are suppressed;
 *   - \a g_outputToString value 1 renders scrolling as pointless and disables it;
 *   - \a backBuffer may contain text to display on scroll back operations;
 *   - \a g_scrollMode value 1 enables scrolling back through text held in
 *     \a backBuffer;
 *   - \a localScrollMode a value of 0 temporarily disables scrolling, despite
 *     the setting in \a g_scrollMode;
 *   - \a g_commandPrompt if this string matches ask, top level user input is
 *     assumed.
 * \post
 *   \a db is updated.
 * \warning the calling program must deallocate the returned string (if not
 *   null or empty).  Note that the result can be NULL.  This is outside of the
 *   usual behavior of a \a vstring type.
 * \warning the returned string need not be valid ASCII or UTF-8.
 * \bug If a character read from __stream__ is NUL, this may sometimes cause a
 *   print of an error message, but execution continues and in the wake may
 *   cause all kind of undefined behavior, like memory accesses beyond
 *   allocated buffers.
 */
vstring cmdInput(FILE *stream, const char *ask);
/*!
 * gets a line from either stdin or the command file stream depending on
 * \a g_commandFileNestingLevel.  It uses \a cmdInput(), i.e some input
 * lines may be interpreted and not returned to the caller.  The conditions for
 * this are listed in \a cmdInput, except that \a localScrollMode is fixed to 1.
 * \param ask (not null) text displayed before input prompt.  This can be
 *   located in \a tempAllocStack.  If this text contains more than
 *   \a g_screenWidth characters, it is wrapped preferably at space characters
 *   and split across multiple lines.  If the final line contains spaces in the
 *   range from position 1 to \a g_screenWidth - 11, it is wrapped such, that
 *   it leaves enough space for ten character user input.
 *   \n
 *   If the prompt needs to be displayed again, only the last line after
 *   wrapping is reprinted.
 * \return not interpreted input.
 * \pre
 *   The following variables are honored during execution and should be properly
 *   set:
 *   - \a commandFileSilentFlag value 1 suppresses prompts altogether, not only
 *     those used for scrolling through long text;
 *   - \a g_commandFileNestingLevel a value > 0 indicates a SUBMIT call is
 *     executing, where scrolling prompts are suppressed;
 *   - \a g_outputToString value 1 renders scrolling as pointless and disables it;
 *   - \a backBuffer may contain text to display on scroll back operations;
 *   - \a g_scrollMode value 1 enables scrolling back through text held in
 *     \a backBuffer;
 *   - \a g_commandPrompt if this string matches ask, top level user input is
 *     assumed.
 * \post
 *   \a localScrollMode is set to 1
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
