/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMINOU_H_
#define METAMATH_MMINOU_H_

/*!
 * \file mminou.h
 * \brief Basic input and output interface.
 */

#include <stdio.h>

#include "mmvstr.h"
#include "mmdata.h"

extern int g_errorCount;     /*!< Total error count */

/* Global variables used by print2() */
/*!
 * \var flag g_logFileOpenFlag
 * If set to 1, logging of input is enabled.  Initially set to 0.
 */
extern flag g_logFileOpenFlag;
/*!
 * \var FILE *g_logFilePtr
 * The OPEN LOG command opens a log file.  Its file descriptor is stored here,
 * and not removed, when the log file is closed.  You should access this
 * descriptor only when \ref g_logFileOpenFlag is 1.
 */
extern FILE *g_logFilePtr;

extern FILE *g_listFile_fp;

/*!
 * \var g_outputToString
 *
 * Global variable redirecting the output of the function print2 from the
 * console ( = 0) to a string ( = 1).  Initialized to 0 on program start.
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
 * Limited by \ref MAX_COMMAND_FILE_NESTING.
 */
extern long g_commandFileNestingLevel;
/*!
 * \var FILE *g_commandFilePtr[MAX_COMMAND_FILE_NESTING + 1]
 * file descriptors pointing to files invoked by SUBMIT commands.  The 0-th
 * element is fixed to stdin, and neither used nor assigned to.  Other
 * elements, up to the current value of \ref g_commandFileNestingLevel, are
 * valid, and point to opened files.
 */
extern FILE *g_commandFilePtr[MAX_COMMAND_FILE_NESTING + 1];
/*!
 * \var vstring g_commandFileName[]
 * list of command file names in nested SUBMIT commands.  This name need not be
 * fully qualified (with all directories down from the root directory).  The
 * first element is reserved for stdin and never set.
 */
extern vstring g_commandFileName[MAX_COMMAND_FILE_NESTING + 1];

extern flag g_commandFileSilent[MAX_COMMAND_FILE_NESTING + 1];

/*!
 * \var g_commandFileSilentFlag
 * If set to 1, suppresses prompts on input.  Activated through
 * SUBMIT ... /SILENT commands.  Initialized to 0 on program start.
 */
extern flag g_commandFileSilentFlag; /* For SUBMIT ... /SILENT */


extern FILE *g_input_fp;  /*!< File pointers */
extern vstring g_input_fn, g_output_fn;  /*!< File names */

/*! print2 returns 0 if the user has quit the printout.
  \warning:  never call print2 with string longer than PRINTBUFFERSIZE - 1  */
flag print2(const char* fmt,...);
extern long g_screenHeight; /*!< Height of screen */
/*!
 * \var long g_screenWidth
 * \brief Width of screen
 *
 * The minimum width of the display, measured in fixed width characters.
 */
extern long g_screenWidth;
/*!
 * \def MAX_LEN
 * \brief Default width of screen
 *
 * Number of characters that can always be displayed in a single line.  This
 * notion is reminiscent of CRT tubes with a fixed width character set.
 * Graphical Displays on a notebook for example can display much more, but on
 * some mobile devices this may be reduced to 30-40 characters.
 */
#define MAX_LEN 79
/*! Lines on screen, minus 1 to account for prompt */
#define SCREEN_HEIGHT 23
/*!
 * \var flag g_scrollMode
 * \brief controls whether output stops after a full page is printed.
 *
 * A value of 1 indicates the user wants prompted page wise output.
 * The command SET SCROLL controls this value.  If followed by CONTINUOUS, this
 * flag is reset to 0.
 */
extern flag g_scrollMode;
/*!
 * \var flag g_quitPrint
 * \brief Flag that user typed 'q' to last scrolling prompt
 * The value 1 indicates the user entered a 'q' at the last scrolling prompt.
 */
extern flag g_quitPrint;

/*! printLongLine automatically puts a newline \n in the output line. */
void printLongLine(const char *line, const char *startNextLine, const char *breakMatch);

/*!
 * \brief requests a line of text from the \p stream.
 *
 * If not suppressed, displays a prompt text on the screen.  Then reads a
 * line from the \p stream.  Some lines are interpreted as described further
 * below, in which case the prompt is reprinted and the next line is read.
 * Returns the first not interpreted line as a \ref vstring.
 *
 * A line in the \p stream is terminated by a LF character (0x0D) character
 * alone.  It is read, but removed from the result.  The maximum line length
 * without the LF is \ref CMD_BUFFER_SIZE - 1.  Reaching EOF (end of file,
 * CTRL-D) is equivalent to reading LF, if at least 1 byte was read before.
 * Note that the NUL character can be part of the line.  Reading a NUL is not
 * sufficiently supported in the current implementation and may or may not
 * cause an error message or even undefined behavior.
 *
 * Reading from an empty \p stream (or one that is at EOF position, on the
 * console CTRL-D) returns NULL, not the empty string, and is formally
 * signalled as an error.  Overflowing the buffer is also an error.  No
 * truncated value is returned.
 *
 * This routine interprets some input without returning it to the caller under
 * following two conditions:
 *
 * 1. If scrolling is enabled, the input is interpreted.  A line consisting of
 * a single character b or B indicates the user wants to scroll back through
 * saved pages of output.  This is handled within this routine, as often as
 * requested and possible.
 *
 * 2. The user hits ENTER only while prompted in top level context.  The empty
 * line is not returned.
 * 
 * No timeout is applied while waiting for user input from the console.
 *
 * A bug message need not result in an execution stop.  It is not directed to
 * the metamath bug function to avoid stacking up calls (bug calling cmdInput
 * again for scrolling etc.).
 *
 * \todo clarify recursive call to print2 and the role of backFromCmdInput. 
 * \param[in] stream (not null) source to read the line from.  _stdin_ is
 *   common for user input from the console. 
 * \param[in] ask prompt text displayed on the screen before \p stream is
 *   read.  This prompt is suppressed by either a NULL value, or setting 
 *   \ref g_commandFileSilentFlag to 1.  This prompt must be not NULL (empty is
 *   fine!) outside of a SUBMIT call, where user is expected to enter input.
 *   \n
 *   It may be compared to \ref g_commandPrompt.  If both match, it is inferred
 *   the user is in top level command mode, where empty input is not returned
 *   to the caller.
 * \return a \ref vstring containing the first read and not interpreted line.
 *   NULL indicates an error condition.  The result needs to be deallocated by
 *   the caller, if not empty or NULL.
 * \pre
 *   The following variables are honored during execution and should be properly
 *   set:
 *   - \ref g_commandFileSilentFlag value 1 suppresses all prompts, not only
 *     those used for scrolling through long text.  It does not suppress error
 *     messages;
 *   - \ref g_commandFileNestingLevel a value > 0 indicates a SUBMIT call is
 *     executing, a read line is returned as is.  0 is seen as an interactive
 *     mode, where read lines can be interpreted;
 *   - \ref g_outputToString value 1 renders scrolling as pointless and
 *     disables it;
 *   - \ref backBuffer may contain text to display on scroll back operations;
 *   - \ref g_scrollMode value 1 enables scrolling back through text held in
 *     \ref backBuffer;
 *   - \ref localScrollMode a value of 0 temporarily disables scrolling,
 *     overriding the setting in \ref g_scrollMode;
 *   - \ref g_commandPrompt if this string matches ask, top level user input is
 *     assumed, where empty lines are discarded.
 * \post
 *   \ref db is updated.
 * \warning the calling program must deallocate the returned string (if not
 *   null or empty).  Note that the result can be NULL.  This is outside of the
 *   usual behavior of a \ref vstring type.
 * \warning the returned string need not be valid ASCII or UTF-8.
 * \bug If a character read from \p stream is NUL, this may sometimes cause a
 *   print of an error message, but execution continues and in the wake may
 *   cause all kind of undefined behavior, like memory accesses beyond
 *   allocated buffers.
 */
vstring cmdInput(FILE *stream, const char *ask);

/*!
 * \brief print explanatory text and then read a line.
 *
 * After some explanatory text is printed, gets a line from either stdin or the
 * command file stream in \ref g_commandFilePt, depending on the value of
 * \ref g_commandFileNestingLevel.  If this value is 0, interactive input via
 * stdin is assumed, else non interpreted lines are read from a file in submit
 * mode.  The line returned to the caller is more or less what \ref cmdInput()
 * yields, but some fine tuning is applied.
 *
 * \par Displaying the prompt text
 *
 * The text used to prompt the user is wrapped around preferably spaces to fit
 * into a display of \ref g_screenWidth.  If possible, wrapping shortens the
 * last line such that space for 10 characters is available to the right of the
 * prompt for user input.
 *
 * \par Interactive Mode
 *
 * 1. A long prompt text may be interrupted for convenient page wise display.
 * The user's scroll commands are interpreted internally and not seen by the
 * caller.  If a line is either discarded or interpreted, the user is prompted
 * again. The full prompt text is never repeated, only its last line after
 * wrapping was applied.
 *
 * 2. Empty lines are discarded, and a reprompt is triggered.
 * 
 * 3. A NULL resulting from an error (buffer overflow) or a premature EOF
 * (CTRL_D from keyboard) from \ref cmdInput is either returned as "EXIT".  Or
 * if the last line of the prompt starts with "Do", then it is assumed to
 * expand to "Do you want to EXIT anyway (Y, N)?" and a "Y" is returned. In any
 * case, the returned string is printed before it may finally trigger an
 * immediate stop on the caller's side.
 *
 * 4. If logging is enabled, prompt and returned input is logged.
 * 
 * \par Submit Mode
 *
 * 1. a non-interpreted line is read from the appropriate entry in
 * \ref g_commandFilePtr by \ref cmdInput.
 *
 * 2. If NULL is returned, reaching EOF is assumed, the file is closed, its
 * name in \ref g_commandFileName deallocated and the previous
 * \ref g_commandFileLevel is activated.  In this particular case the read line
 * is the empty string.  A message indicating the end of the command file is
 * printed.  The \ref g_commandFileSilentFlag controlling console output is
 * copied from the appropriate entry of \ref g_commandFileSilent, unless the
 * interactive mode is reached; here output is never suppressed (value 0).
 *
 * 3. remove all CR (0x0D) characters, not only those in compination with LF.
 *
 * 4. prompt and command is printed, if not suppressed, then the read line is
 * returned.
 *
 * \return first not interpreted line as \ref vstring, or "EXIT" on error. 
 * \pre
 *   The following variables are honored during execution and should be properly
 *   set:
 *   - \ref g_commandFileSilentFlag value 1 suppresses output and prompts, but
 *     not all error messages;
 *   - \ref g_commandFileNestingLevel a value > 0 indicates a SUBMIT call is
 *     executing, where a read line is returned as is. 0 is seen as interactive
 *     mode, where read lines can be interpreted;
 *   - \ref g_outputToString value 1 renders scrolling as pointless and
 *     disables it;
 *   - \ref backBuffer may contain text to display on scroll back operations;
 *   - \ref g_scrollMode value 1 enables scrolling back through text held in
 *     \ref backBuffer;
 *   - \ref localScrollMode a value of 0 temporarily disables scrolling,
 *     overriding the setting in \ref g_scrollMode;
 *   - \ref g_commandPrompt if this string matches ask, top level user input is
 *     assumed, and an empty line is usually discarded;
 *   - \ref g_logFileOpenFlag if set to 1, a not interpreted returned line is
 *     logged before it is passed on to the caller.
 * \post
 *   - \ref localScrollMode is set to 1
 *   - \ref printedLines is reset to 0
 *   - \ref g_quitPrint is reset to 0
 *   - interactive mode: \ref tempAllocStack frees top elements down to
 *     \ref g_startTempAlloc.
 *   - interactive mode: \ref pntrTempAllocStack frees top elements down to
 *     \ref g_pntrStartTempAlloc.
 *   - interactive mode: The \ref backBuffer is cleared, then filled with
 *     prompt (last line only) and input of the user.
 *   - submit mode: In case of EOF the previous \ref g_commandFileNestingLevel
 *     is activated, necessary cleanups performed, and 
 *     the \ref g_commandFileSilentFlag is updated appropriately.
 * \warning the calling program must deallocate the returned string.
 */
vstring cmdInput1(const char *ask);

flag cmdInputIsY(const char *ask);

enum severity {notice_,warning_,error_,fatal_};
void errorMessage(vstring line, long lineNum, long column, long tokenLength,
  vstring error, vstring fileName, long statementNum, flag warnFlag);

/*! Opens files with error message; opens output files with
   backup of previous version.   Mode must be "r" or "w". */
FILE *fSafeOpen(const char *fileName, const char *mode, flag noVersioningFlag);

/*! Renames a file with backup of previous version.  If non-zero
   is returned, there was an error. */
int fSafeRename(const char *oldFileName, const char *newFileName);

/*! Finds the name of the first file of the form filePrefix +
   nnn + ".tmp" that does not exist.  THE CALLER MUST DEALLOCATE
   THE RETURNED STRING. */
vstring fGetTmpName(const char *filePrefix);

/*! This function returns a character string containing the entire contents of
   an ASCII file, or Unicode file with only ASCII characters.   On some
   systems it is faster than reading the file line by line.  THE CALLER
   MUST DEALLOCATE THE RETURNED STRING.  If a NULL is returned, the file
   could not be opened or had a non-ASCII Unicode character or some other
   problem.   If verbose is 0, error and warning messages are suppressed. */
vstring readFileToString(const char *fileName, char verbose, long *charCount);

/*! Returns total elapsed time in seconds since starting session (for the
   lcc compiler) or the CPU time used (for the gcc compiler).  The
   argument is assigned the time since the last call to this function. */
double getRunTime(double *timeSinceLastCall);

/*! Call before exiting to free memory allocated by this module */
void freeInOu(void);

#endif /* METAMATH_MMINOU_H_*/
