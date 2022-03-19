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

/*!
 * \fn flag print2(const char* fmt,...)
 * \brief formatted output of a single line with optional page-wise display.
 *
 * It is important to understand that the submitted parameters result in a
 * NUL-terminated string that contains at most one LF character, only allowed
 * in final position.  A long line exceeding \ref g_screenWidth is broken down
 * into multiple lines using a built-in line wrap algorithm.  But this must
 * never be preempted by preparing parameters accordingly.
 *
 * Although the output of a single line is the main goal of this function, it
 * does a lot on the side, each effect individually enabled or disabled by
 * various global variables that are honored, sometimes even updated.  We skim
 * through these effects here in short:
 * 
 * 1. __Data embedding__.  The \p fmt parameter can contain simple text, that
 *     is displayed as is.  Or embedded placeholders are replaced with data
 *     pointed to by the following parameters.  If necessary, data are
 *     converted to strings before insertion.
 * 2. Supporting __page-wise display__.  Output on the virtual text display is
 *     stopped after showing \ref g_screenHeight lines, called a __page__ here.
 *     The user can read the text at rest, and then request resuming output
 *     with a short command.  In fact, this feature is enhanced by maintaining
 *     a \ref pgBackBuffer, allowing the user to recall previously shown pages,
 *     and navigate them freely forward and backwards.  This feature
 *     temporarily grabs user input and interprets short commands as navigating
 *     instructions.  For more see \ref pgBackBuffer.
 * 3. __Line wrapping__.  When the line to display exceeds the limits in
 *     \ref g_screenWidth, line breaking is applied.  The wrapping is actually
 *     done in \ref printLongLine which in turn uses separate print2 calls to
 *     output each individual line.  This requires a minimum synchronization of
 *     both the 'master' and 'slave' calls, so that relevant state is carried
 *     back to the outer, temporarily suspended call.
 * 4. __Redirection__.  Instead of displaying output on the virtual text
 *     display, it may be stored in a NUL-terminated string variable.  Or even
 *     completely suppressed, when executing a SUBMIT, for example.
 * 5. __Logging__.  Output may be logged to a file for examination.
 *
 * These effects need not be carried out in this order, some may even be
 * omitted.  We show the order of steps and their respective conditions here.
 * 
 * -# The \ref backBuffer is almost private to this function, so its
 *     initialization is done here, right at the beginning.  The
 *     \ref backBufferPos is always at least 1, so a value of 0 indicates an
 *     outstanding \ref backBuffer memory allocation, and an empty string is
 *     pushed as a first (guard) page onto it (see \ref pgBackBuffer).
 *
 * -# If the current page is full and further output would overflow it, as
 *     indicated by \ref pageLines, output may be suspended as described in
 *     (2) and the user is prompted for scrolling actions.\n
 *     This step is unconditionally executed when \ref backFromCmdInput = 1
 *     (\ref cmdInput explicitely requested it).  The values in
 *     \ref g_quitPrint and \ref localScrollMode are retained, regardless of
 *     user input.\n
 *   \n
 *     Other conditions prevent this step:  Output is discarded on user request
 *     (\ref g_quitPrint = 1), a SUBMIT call is running
 *     (\ref g_commandFileNestingLevel > 0), scrolling is generally
 *     (\ref g_scrollMode = 0) or temporarily (\ref localScrollMode = 0)
 *     disabled, output is redirected (\ref g_outputToString = 1).\n
 *   \n
 *     This step can set \ref g_quitPrint to 1 (if \ref backFromCmdInput = 0).
 *     It can modify \ref \backBufferPos (by command _q_ or _Q_) and set
 *     \ref localScrollMode = 0 (by command _s_ or _S_, and
 *     \ref backFromCmdInput = 0).
 *
 * -# If pending output would overflow the screen, \ref backBuffer is extended
 *     by a new page to receive pending output.\n
 *     Several conditions can prevent this step:  Step (2) was not executed,
 *     output is discarded on user request (\ref g_quitPrint = 1),
 *     \ref backFromCmdInput = 1 (\ref cmdInput needs the scrolling loop
 *     only).\n
 *   \n
 *     Sets \ref printedLines to 0, indicating that nothing has been added to
 *     the new page yet. Increments \ref backBufferPos so all pending output is
 *     copied to the new page.
 *
 * -# The submitted parameters are used to create a new line from their values.
 *     Placeholders in \p fmt are replaced with their respective and to text
 *     converted data values.  All stored in an internal buffer first.\n
 *  \n
 *     Some contexts prevent this step: Output (if to screen) is discarded on
 *     user request (\ref g_quitPrint = 1 and \ref g_outputToString = 0),
 *     \ref backFromCmdInput = 1 (\ref cmdInput uses only the scrolling
 *     features)
 *
 * -# Revert any \ref QUOTED_SPACE back to space characters in the string
 *     produced in step (4).  \ref printLongLine might have introduced these
 *     characters to prevent line breaks at certain space characters.  A
 *     \ref QUOTED_SPACE must not be part of the regular output to make this
 *     work flawlessly.\n
 *  \n
 *     Some contexts prevent this step: Output (if to screen) is discarded on
 *     user request (\ref g_quitPrint = 1 and \ref g_outputToString = 0),
 *     \ref backFromCmdInput = 1 (\ref cmdInput uses only the scrolling
 *     features)
 *
 * -# Perform line wrapping.  The wrapping is actually done in
 *     \ref printLongLine, which in turn calls this function to handle each
 *     broken down line separately.  The output generated in step (4) is copied
 *     onto \ref tempAllocStack, omitting a trailing LF.\n
 *  \n
 *     Some contexts prevent this step: The output from step (4) (excluding a
 *     trailing LF) does not exceed \ref g_screenWidth, output is discarded on
 *     user request (\ref g_quitPrint = 1), output is redirected to a string
 *     (\ref g_outputToString = 1), \ref backFromCmdInput = 1
 *     (\ref cmdInput uses only the scrolling features)\n
 *  \n
 *      __todo__ clarify variants/invariants
 * -# Print the prepared output onto the screen.\n
 *  \n
 *     Some contexts prevent this step: Step (6) (line wrapping) was executed,
 *     output is discarded on user request (\ref g_quitPrint = 1)
 *     output is redirected to a string (\ref g_outputToString = 1), a SUBMIT
 *     is silently executing (\ref g_commandFileSilentFlag = 1),
 *     \ref backFromCmdInput = 1 (\ref cmdInput uses only the scrolling
 *     features)\n
 *  \n
 *     \ref printedLines is increased if the prepared output terminates with
 *     LF.
 * -# Add a new history page in \ref backBuffer for the output generated in
 *     step (4) if the current page has overflown.  Do this even when scrolling
 *     is disabled.\n
 *  \n
 *     Some contexts prevent this step: Step (7) was not executed,
 *     \ref printedLines <= \ref g_screenHeight allows another line of output,
 *     the output printed in step (7) has no trailing LF and is appended to the
 *     last line of output, a SUBMIT is silently executing
 *     (\ref g_commandFileSilentFlag = 1), scrolling is enabled
 *     (\ref g_scrollMode = 1, step (3) handles this case).\n
 *  \n
 *     \ref printedLines is set to 1.  \ref backBufferPos is increased.
 * -# Add the output to the \ref backBuffer.
 *  \n
 *     Some contexts prevent this step: Step (7) was not executed.
 * -# Log the output to a file.\n
 *  \n
 *     Some contexts prevent this step: A log file is not opened
 *     (\ref g_logFileOpenFlag = 0), output is redirected to a string
 *     (\ref g_outputToString = 1), output is discarded on user request
 *     (\ref g_quitPrint = 1), \ref backFromCmdInput = 1 (\ref cmdInput uses
 *     only the scrolling features)\n
 *     
 *
 * \param[in] fmt NUL-terminated text to display with embedded placeholders
 *   for insertion of data (which are converted into text if necessary) pointed
 *   to by the following parameters.  The placeholders are encoded in a cryptic
 *   syntax explained
 *   <a href="https://en.wikipedia.org/wiki/Printf_format_string">here</a> or
 *   <a href="https://en.cppreference.com/w/c/io/fprintf">here</a>.
 *
 *   This parameter is ignored when \ref backFromCmdInput is 1.
 * \param[in] "..." The data these (possibly empty sequence of) pointers refer
 *   to are converted to string and then inserted at the respective placeholder
 *   position in \p fmt.  They should strictly match in number, type and order
 *   the placeholders.
 *
 *   These parameters are ignored when \ref backFromCmdInput is 1.
 * \return \ref g_quitPrint 0: user has quit the printout.
 * \pre
 *   - \ref printedLines if indicating a full page of output was reached,
 *     activates __scroll mode__ if not inhibited by other variables.
 *   - \ref g_screenHeight number of lines to display (a page of output) to a
 *     user without need of  __scroll mode__.
 *   - \ref g_screenWidth if the expanded text exceeds this width, line
 *     breaking may be required.  Other settings can still prevent this;
 *   - \ref g_quitPrint value 1:  Do not enter __scroll mode__ and suppress output to the
 *     (virtual) text display;
 *   - \ref backFromCmdInput value 1: assume the last entry in \ref backBuffer
 *     was just printed, \ref backBufferPos points to the entry before the
 *     last, and __scroll mode__ is requested, and nothing else.  No output
 *     apart from replaying saved pages in the \ref backBuffer is generated.
 *     This flag enables __scroll mode__ unconditionally, regardless of other
 *     settings.  This flag is set by \ref cmdInput only;
 *   - \ref g_commandFileSilentFlag value 1 suppresses output on the screen;
 *   - \ref g_commandFileNestingLevel a value > 0 indicates a SUBMIT call is
 *     executing, where __scroll mode__ is disabled, unless
 *     \ref backFromCmdInput is 1;
 *   - \ref g_scrollMode value 0 disables __scroll mode__, unless
 *     \ref backFromCmdInput is 1;
 *   - \ref localScrollMode value 0 disables __scroll mode__, unless
 *     \ref backFromCmdInput is 1;
 *   - \ref g_outputToString value 1 output is redirected and __scroll mode__
 *     is disabled, unless \ref backFromCmdInput is 1.
 * \post
 *   - \ref g_quitPrint is set to 1, if the user entered q or Q in
 *      __scroll mode__, and \ref backFromCmdInput is 0.
 *   - \ref backBuffer is allocated and not empty (at least filled with an
 *     empty string)
 *   - \ref backBufferPos > 0
 * \bug It is possible to produce lines exceeding \ref g_screenWidth by
 *   concatenating substrings smaller than this value, but having no LF at the
 *   end.
 * \warning never call print2 with string longer than PRINTBUFFERSIZE - 1
 */
flag print2(const char* fmt,...);
/*!
 * \var long g_screenHeight
 * Number of lines the (virtual) text display can display to the user at the
 * same time, apart from an extra line set aside for a prompt and echo of user
 * input.  The command SET HEIGHT changes this value.
 *
 * This value equals the number of lines in a page of output.  After a page
 * output is interrupted to let the user read the contents and request
 * resuming. 
 */
extern long g_screenHeight;
/*!
 * \var long g_screenWidth
 * \brief Width of screen
 *
 * The minimum width of the (virtual) text display, measured in fixed width
 * characters.  The command SET WIDTH changes this value.
 */
extern long g_screenWidth;
/*!
 * \def MAX_LEN
 * \brief Default width of screen
 * The default setting of \ref g_screenWidth on program start.
 *
 * Number of characters that can always be displayed in a single line.  This
 * notion is reminiscent of CRT tubes with a fixed width character set.
 * Graphical Displays on a notebook for example can display much more, but on
 * some mobile devices this may be reduced to 30-40 characters.
 */
#define MAX_LEN 79
/*!
 * \def SCREEN_HEIGHT
 * \brief Default height of screen
 * The default setting of \ref g_screenHeight on program start.
 *
 * This notion is reminiscent of CRT tubes with a fixed width character set.
 * Graphical Displays on a notebook for example can display much more.
 */
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
 * The value 1 indicates the user requested immediately discarding pending
 * output, and return back to normal command input.
 *
 * This flag is set in \ref print2 when the user inputs q or Q.
 */
extern flag g_quitPrint;

/*!
 * \fn void printLongLine(const char *line, const char *startNextLine, const char *breakMatch)
 * \brief perform line wrapping and print
 * apply a line wrapping algorithm to fit a text into the screen rectangle.
 * Submit each individual broken down line to \ref print2 for output.
 *
 * printLongLine automatically puts a newline in the output line.
 * \param[in] line (not null) NUL-terminated text (may contain LF) to apply
 *   line wrapping to.
 * \param[in] startNextLine (not null) NUL-terminated string to place before
 *   continuation lines.
 * \param[in] breakMatch (not null) NULL-terminated list of characters at which
 *   the line can be broken.  If empty, a break is possible anywhere.
 * \post
 *   \ref tempAllocStack is cleared down to \ref g_startTempAllocStack.
 */
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
 *   - \ref g_outputToString value 1 output is redirected and scrolling is
 *     disabled
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
 * \brief print prompt (or explanatory) text and then read a line.
 *
 * After a prompt text is printed, gets a line from either stdin or the
 * command file stream in \ref g_commandFilePtr, depending on the value of
 * \ref g_commandFileNestingLevel.  If this value is 0, interactive input via
 * stdin is assumed, else non interpreted lines are read from a file in submit
 * mode.  The line returned to the caller is more or less what \ref cmdInput()
 * yields, but some fine tuning is applied.
 *
 * \par Displaying the prompt text
 *
 * This function is prepared to display a longer text, before issuing a final
 * prompt line (unlike \ref cmdInput).  The text shown to the user is usually
 * wrapped around preferably at spaces to fit into a display of width
 * \ref g_screenWidth.  If possible, wrapping shortens the last line such that
 * space for 10 characters is left to the right for user input.
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
 * \ref g_commandFileNestingLevel is activated.  In this particular case the
 * read line is the empty string.  A message indicating the end of the command
 * file is printed.  The \ref g_commandFileSilentFlag controlling console
 * output is copied from the appropriate entry of \ref g_commandFileSilent,
 * unless the interactive mode is reached; here output is never suppressed
 * (value 0).
 *
 * 3. remove all CR (0x0D) characters, not only those in combination with LF.
 *
 * 4. prompt and command is printed, if not suppressed, then the read line is
 * returned.
 *
 * \return first not interpreted line as \ref vstring, or "EXIT" on error. 
 * \pre
 *   The following variables are honored during execution and should be properly
 *   set:
 *   - \ref g_quitPrint a 1 suppresses wrapping and user controlled paging of
 *     the prompt text.
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
 *     \ref g_startTempAllocStack.
 *   - interactive mode: \ref pntrTempAllocStack frees top elements down to
 *     \ref g_pntrStartTempAllocStack.
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
