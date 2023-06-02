/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL                                  */
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

// Global variables used by print2()

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

/*!
 * \var vstring g_printString
 * If output is redirected to a string by \ref g_outputToString, this variable
 * receives the contents.
 */
extern vstring g_printString;

// Global variables used by cmdInput()

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
extern flag g_commandFileSilentFlag; // For SUBMIT ... /SILENT

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
 * The presence of the LF character in a line shorter than \ref g_screenWidth
 * controls whether the current screen line is closed after print, or output in
 * a later call to print2 is padded right.  Output of more characters than this
 * limit always closes the screen line.
 *
 * Although the output of a single line is the main goal of this function, it
 * does a lot on the side, each effect individually enabled or disabled by
 * various global variables that are honored, sometimes even updated.  We skim
 * through these effects here in short:
 *
 * - (a) __Data embedding__.  The \p fmt parameter can contain simple text,
 *   that is displayed as is.  Or embedded placeholders are replaced with data
 *   pointed to by the following parameters.  If necessary, data are converted
 *   to strings before insertion.
 * - (b) Supporting __page-wise display__.  Output on the virtual text display
 *   is stopped after showing \ref g_screenHeight lines, called a __page__
 *   here.  The user can read the text at rest, and then request resuming
 *   output with a short command.  In fact, this feature is enhanced by
 *   maintaining a \ref pgBackBuffer, allowing the user to recall previously
 *   shown pages, and navigate them freely forward and backwards.  This feature
 *   temporarily grabs user input and interprets short commands as navigating
 *   instructions.  For more see \ref pgBackBuffer.
 * - (c) __Line wrapping__.  When the line to display exceeds the limits in
 *   \ref g_screenWidth, line breaking is applied.  The wrapping is actually
 *   done in \ref printLongLine which in turn uses separate print2 calls to
 *   output each individual line.  This requires a minimum synchronization of
 *   both the outer and inner calls, so that relevant state is carried back to
 *   the outer, temporarily suspended call.
 * - (d) __Redirection__.  Instead of displaying output on the virtual text
 *   display, it may be stored in a NUL-terminated string variable.  Or even
 *   completely suppressed, for example when executing a SUBMIT.
 * - (e) __Logging__.  Output may be logged to a file for examination.
 *
 * These effects need not be carried out in this order, some may even be
 * omitted.  We show the order of steps and their respective conditions here.
 *
 * 1. The \ref backBuffer is almost private to this function, so its
 *     initialization is done here, right at the beginning.  The
 *     \ref backBufferPos is always at least 1, so a value of 0 indicates an
 *     outstanding \ref backBuffer memory allocation, and an empty string is
 *     pushed as a first (guard) page onto it (see \ref pgBackBuffer).
 *
 *     \ref g_pntrTempAllocStackTop may be reset to
 *     \ref g_pntrStartTempAllocStack.
 *
 * 2. If the current page is full and further output would overflow it, as
 *     indicated by \ref printedLines, output may be suspended as described in
 *     (b) and the user is prompted for scrolling actions.\n
 *     This step is unconditionally executed when \ref backFromCmdInput = 1
 *     (\ref cmdInput explicitly requested it).  The values in
 *     \ref g_quitPrint and \ref localScrollMode are retained then, regardless
 *     of user input.
 *
 *     Other conditions prevent this step:  Output is discarded on user request
 *     (\ref g_quitPrint = 1), a SUBMIT call is running
 *     (\ref g_commandFileNestingLevel > 0), scrolling is generally
 *     (\ref g_scrollMode = 0) or temporarily (\ref localScrollMode = 0)
 *     disabled, output is redirected (\ref g_outputToString = 1).
 *
 *     This step can set \ref g_quitPrint to 1 (if \ref backFromCmdInput = 0).
 *     It can modify \ref backBufferPos (by command _q_ or _Q_) and set
 *     \ref localScrollMode = 0 (by command _s_ or _S_, and
 *     \ref backFromCmdInput = 0).
 *
 * 3. If pending output would overflow the screen, \ref backBuffer is extended
 *     by a new page to receive pending output.
 *
 *     Several conditions can prevent this step:  Step (2) was not executed,
 *     output is discarded on user request (\ref g_quitPrint = 1),
 *     \ref backFromCmdInput = 1 (\ref cmdInput needs the scrolling loop
 *     only).
 *
 *     Sets \ref printedLines to 0, indicating that nothing has been added to
 *     the new page yet. Increments \ref backBufferPos so all pending output is
 *     copied to the new page.  \ref g_pntrTempAllocStackTop may be reset to
 *     \ref g_pntrStartTempAllocStack.
 *
 * 4. The function parameters are used to create a new line.  Placeholders in
 *     \p fmt (see its description for details) are replaced with their
 *     respective data values, converted to text  All data is first stored in
 *     an internal buffer.
 *
 *     Some contexts prevent this step: Output (if to screen) is discarded on
 *     user request (\ref g_quitPrint = 1 and \ref g_outputToString = 0),
 *     \ref backFromCmdInput = 1 (\ref cmdInput uses only the scrolling
 *     features)
 *
 * 5. Revert any \ref QUOTED_SPACE back to space characters in the string
 *     produced in step (4) (\ref printLongLine might have introduced these
 *     characters to prevent line breaks at certain space characters).  For
 *     this to work correctly, \ref QUOTED_SPACE must not be part of the
 *     regular output. (Doing so will trigger a bug check in
 *     \ref printLongLine .)
 *
 *     Some contexts prevent this step: Output (if to screen) is discarded on
 *     user request (\ref g_quitPrint = 1 and \ref g_outputToString = 0),
 *     \ref backFromCmdInput = 1 (\ref cmdInput uses only the scrolling
 *     features)
 *
 * 6. Perform line wrapping.  The wrapping is actually done in
 *     \ref printLongLine, which in turn calls this function to handle each
 *     broken down line separately.  The output generated in step (4) is copied
 *     onto \ref tempAllocStack, omitting a trailing LF.
 *
 *     Some contexts prevent this step: The output from step (4) (excluding a
 *     trailing LF) does not exceed \ref g_screenWidth, output is discarded on
 *     user request (\ref g_quitPrint = 1), output is redirected to a string
 *     (\ref g_outputToString = 1), \ref backFromCmdInput = 1
 *     (\ref cmdInput uses only the scrolling features)
 *
 *      __todo__ clarify variants/invariants
 *
 * 7. Print the prepared output onto the screen.
 *
 *     Some contexts prevent this step: Step (6) (line wrapping) was executed,
 *     output is discarded on user request (\ref g_quitPrint = 1)
 *     output is redirected to a string (\ref g_outputToString = 1), a SUBMIT
 *     is silently executing (\ref g_commandFileSilentFlag = 1),
 *     \ref backFromCmdInput = 1 (\ref cmdInput uses only the scrolling
 *     features)
 *
 *     \ref printedLines is increased if the prepared output terminates with
 *     LF.
 *
 * 8. Add a new history page in \ref backBuffer for the output generated in
 *     step (4) if the current page has overflowed.  Do this even when scrolling
 *     is disabled.
 *
 *     Some contexts prevent this step: Step (7) was not executed,
 *     \ref printedLines <= \ref g_screenHeight allows another line of output,
 *     the output printed in step (7) has no trailing LF and is appended to the
 *     last line of output, a SUBMIT is silently executing
 *     (\ref g_commandFileSilentFlag = 1), scrolling is enabled
 *     (\ref g_scrollMode = 1, step (c) handles this case).
 *
 *     \ref printedLines is set to 1.  \ref backBufferPos is increased.
 *     \ref g_pntrTempAllocStackTop may be reset to
 *     \ref g_pntrStartTempAllocStack.
 *
 * 9. Add the output to the \ref backBuffer.
 *
 *     Some contexts prevent this step: Step (7) was not executed.
 *
 *     \ref g_tempAllocStackTop is reset to \ref g_startTempAllocStack.
 *
 * 10. Log the output to a file given by \ref g_logFilePtr.
 *
 *     Some contexts prevent this step: Step (4) was not executed, a log file
 *     is not opened (\ref g_logFileOpenFlag = 0), output is redirected to a string
 *     (\ref g_outputToString = 1), \ref backFromCmdInput = 1 (\ref cmdInput
 *     uses only the scrolling features)
 *
 *     \ref g_tempAllocStackTop is reset to \ref g_startTempAllocStack.
 *
 * 11. Copy the prepared output in step (4) to \ref g_printString.
 *
 *     Some contexts prevent this step: Step (4) was not executed, output is
 *     not redirected to a string (\ref g_outputToString = 0),
 *     \ref backFromCmdInput = 1 (\ref cmdInput uses only the scrolling
 *     features)
 * .
 *
 * \param[in] fmt (not null) NUL-terminated text to display with embedded
 *   placeholders for insertion of data (which are converted into text if
 *   necessary) pointed to by the following parameters.  The format string uses
 *   the same syntax as
 *   <a href="https://en.cppreference.com/w/c/io/fprintf">[printf]</a>.  A LF
 *   character must only be in final position, and ETX (0x03) is not allowed.
 *   This parameter is ignored when \ref backFromCmdInput is 1.
 * \param[in] "..." The data these (possibly empty sequence of) pointers refer
 *   to are converted to string and then inserted at the respective placeholder
 *   position in \p fmt.  They should strictly match in number, type and order
 *   the placeholders.  The characters LF and ETX (0x03) must not be produced,
 *   with the exemption that an LF is possible if it appers at the end of the
 *   expanded text (see (a)).
 *   These parameters are ignored when \ref backFromCmdInput is 1.
 * \return \ref g_quitPrint 0: user has quit the printout.
 * \pre
 *   - \ref printedLines if indicating a full page of output was reached,
 *     activates __step 2__ if not inhibited by other variables.
 *   - \ref g_screenHeight number of lines to display (a page of output) to a
 *     user without need of  __step 2__.
 *   - \ref g_screenWidth if the expanded text exceeds this width, line
 *     breaking may be required.  Other settings can still prevent this;
 *   - \ref g_quitPrint value 1:  Do not enter __step 2__ and suppress output
 *     to the (virtual) text display;
 *   - \ref backFromCmdInput value 1: assume the last entry in \ref backBuffer
 *     was just printed, \ref backBufferPos points to the entry before the
 *     last.  All steps but __1__ and __2__ are disabled, and __step 2__ is
 *     enforced, regardless of other settings.  No output apart from replaying
 *     saved pages in the \ref backBuffer is generated.
 *     This flag is set by \ref cmdInput only;
 *   - \ref g_commandFileSilentFlag value 1 suppresses output on the screen;
 *   - \ref g_commandFileNestingLevel a value > 0 indicates a SUBMIT call is
 *     executing, where __step 2__ is disabled, unless
 *     \ref backFromCmdInput is 1;
 *   - \ref g_scrollMode value 0 disables __step 2__, unless
 *     \ref backFromCmdInput is 1;
 *   - \ref localScrollMode value 0 disables __step 2__, unless
 *     \ref backFromCmdInput is 1;
 *   - \ref g_outputToString value 1 output is redirected and __step 2__ is
 *     disabled, unless \ref backFromCmdInput is 1.
 * \post
 *   - \ref g_quitPrint is set to 1, if the user entered _q_ or _Q_ in
 *      __step 2__, and \ref backFromCmdInput is 0.
 *   - \ref backBuffer is allocated and not empty (at least filled with an
 *     empty string)
 *   - \ref backBufferPos > 0, updated
 *   - \ref localScrollMode = 0 if the user entered _s_ or _S_ in __step 2__.
 *   - \ref g_printString receives the output if \ref g_outputToString = 1.
 *   - \ref printedLines updated
 *   - \ref g_pntrTempAllocStackTop may be reset to
 *     \ref g_pntrStartTempAllocStack.
 *   - \ref g_tempAllocStackTop may be reset to \ref g_startTempAllocStack.
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
 * characters, minus 1.  The command SET WIDTH changes this value.
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
 * \brief print lines and perform line wrapping on each one.
 *
 * Apply a line wrapping algorithm to fit a text into the screen rectangle
 * defined by \ref g_screenWidth + 1 and \ref g_screenHeight + 1.
 * Submit each individual broken down line to \ref print2 for output.  All
 * flags and data controlling \ref print2 are in effect.
 *
 * \p printLongLine pads a LF character to the right of \p line, if missing,
 * and honors embedded LF characters.  Still, each individual line
 * might need further breakdown to fit into the dimensions of the virtual
 * text display.  The maximal width available for a line is not limited by
 * \p g_screenWidth + 1 alone, leading or trailing prefix or suffix text also
 * reduce the available space.
 *
 * The following prefixes or suffixes are possible, and are applied to lines
 * too wide for the virtual text display, and, thus, they are presented in a
 * multi-line fashion.  We distinguish the first line, optional follow-up
 * lines, except for the last line being a special case.  They are visually
 * indicated in following ways:
 * - (a) leave the first line as is, start follow-up lines and the last line
 *      with a prefix \p startNextLine.
 * - (b) The same as (a) but right justify displayed text (the prefix
 *      \p startNextLine is still left aligned!) on virtual text display.  The
 *      fill character is a SP (0x20).
 * - (c) reserve the last column for a trailing ~ (0x7E) padded to the right of
 *      the first and every follow-up line, but not to the last one.
 *      The last and each follow-up line is indented by a space (SP, 0x20).
 *      The padded ~ follows the line immediately, the last column is a
 *      last resort and left blank in case of no need.
 * - (d) support for LaTeX: the same as (c), but instead of the ~ character a
 *      trailing % (0x25) is used.  The last and each follow-up line is indented
 *      by a space (SP, 0x20).
 *
 * The following list shows methods of breaking up a long line not containing a
 * LF character into first, follow-up and last screen lines.  Although the
 * methods aim at keeping each individual screen line within the limit given by
 * \ref g_screenWidth, some allow exceptions if for example an embedded link
 * address would be destroyed.  This supports HTML output, where the
 * virtual text display is not really important.  On the other hand, it likely
 * does not affect usual human readable text, because that has enough spaces
 * as break locations for even a small sized text display.
 *
 * 1. __Break at given characters__.  The \p breakMatch contains a non-empty
 *    set of characters marking optional break positions.  If a break occurs at
 *    such a point, the character is zapped, the substring before it becomes a
 *    screen line, the part after it is subject to further line breaking again.
 *    If not used as a break point, the character is left as is.
 *    The last character from \p breakMatch within the allowed line size
 *    determines the break position.  If there is none, the width of the
 *    virtual text display is temporarily suspended and increased as long by
 *    one, until the breaking algorithm finds a fit into the more and more
 *    widened text rectangle.  This temporary increase of dimensions affects
 *    the current screen line only.
 * 2. __Keep quotes__.  This mode is similar to 1., but text between two
 *    pairing quote characters " (0x22) is never split.  This mode allows
 *    breaks only at spaces (SP, 0x20) not contained in a quote.  For technical
 *    reasons the ETX (0x03) character must not appear in \p line.
 *    The parsing algorithm is kept simple, there is no way to use a " within a
 *    quote, it always delimits a quote.  Apart from these differences, all
 *    rules in 1. apply.
 * 3. __Break at any character__.  The \p line is broken into equally sized
 *    pieces, except for the first and last line.  The first one is not reduced by
 *    \p startNextLine, and may receive more characters than the following
 *    ones, the last simply takes the rest.  This method is not UTF-8 safe.
 * 4. __Compressed proof__.  This mode targets proof lines in Metamath that are
 *    encoded in a compressed style.  An example of a compressed proof is:
 *    \code{.unparsed}
 *    ( wa wn wo wi pm3.4 pm2.24 adantr pm2.21 jaoi pm2.27 imdistani orcd
 *    adantrr jca olcd adantrl pm2.61ian impbii ) ABDZAEZCDZFZABGZUCCGZDZUBUHUD
 *    UBUFUGABHAUGBACIJQUDUFUGUCUFCABKJUCCHQLAUHUEAUFUEUGAUFDUBUDAUFBABMNOPUCUG
 *    UEUFUCUGDUDUBUCUGCUCCMNRSTUA $.
 *    \endcode
 *    Both the label section in braces and the following coded proof may exceed
 *    the virtual screen width as shown above, and need being broken down.
 *    The label section comprises of blank separated theorem names usually
 *    short enough to fit in a screen line.  The proof can be split at any
 *    position, except that the bounding __$=__ and __$.__ must be left intact.
 *    This method is not UTF-8 safe.
 *
 * \param[in] line (not null) NUL-terminated text to apply line wrapping to.
 *    If the text contains LF characters, line breaks are enforced at these
 *    positions.
 * \param[in] startNextLine (not null) NUL-terminated string to place before
 *   any follow-up and the last line.  If this prefix leaves not at least 4
 *   characters for regular output on a screen line (\ref g_screenWidth), it is
 *   truncated accordingly (not UTF-8 safe).
 *\n
 *   The following characters in first position trigger a special mode:\n
 *     ~ (0x7E) trailing ~ character, see (c). The rest of this parameter is
 *       ignored, a single space will be used as a prefix.
 * \param[in] breakMatch (not null) NUL-terminated list of characters at which
 *   the line can be broken.  If empty, a break is possible anywhere
 *   (method 3.).\n
 *   Special cases:
 *\n
 *   The following characters in first position allow line breaks at spaces (SP), but
 *   trigger in addition special modes:\n
 *     SOH (0x01) nested tree display (right justify continuation lines);\n
 *     " (0x22) activates method 3. (keep quotes).  For use in HTML code;\n
 *     \ (0x5C) trailing % character (LaTeX support), see (d);\n
 * \n
 *   An & (0x46) in first position activates method 4.  It should be followed
 *   by a SP (0x20) used to break down a list of theorem labels.  Besides
 *   activating method 4., an & in \p line also marks a potential break
 *   location, so this character must neither be used in label names, or
 *   in encoding compressed proofs.
 * \pre
 *   - method 4 requires the \p line containing a compressed proof to neither
 *     use an & or a multi-byte unicode character.
 *   - method 3 requires a \p line to not use a multi-byte UTF-8 character.
 *   - if a full page of lines is reached (\ref printedLines), a user prompt
 *     asking for continuation is issued, if not inhibited by other variables.
 *   - \ref g_screenHeight number of lines to display (a page of output) to a
 *     user without a prompt.
 *   - \ref g_screenWidth if the expanded text exceeds this width, usually line
 *     breaking occurs.  Method 1. and 2. can extend the width temporarily, if
 *     no break location is found;
 *   - \ref g_quitPrint value 1:  Do not prompt (any longer) and suppress
 *     output to the (virtual) text display;
 *   - \ref g_commandFileSilentFlag value 1 suppresses output on the screen;
 *   - \ref g_commandFileNestingLevel a value > 0 indicates a SUBMIT call is
 *     executing, where prompting is disabled;
 *   - \ref g_scrollMode value 0 disables prompting;
 *   - \ref g_outputToString value 1 output is redirected and prompting is
 *     disabled.
 * \post
 *   - \ref g_quitPrint is set to 1, if the user entered _q_ or _Q_ in
 *      __step 2__, and \ref backFromCmdInput is 0.
 *   - \ref backBuffer is allocated and not empty (at least filled with an
 *     empty string)
 *   - \ref g_printString receives the output if \ref g_outputToString = 1.
 *   - \ref printedLines updated
 *   - \ref g_pntrTempAllocStackTop may be reset to
 *     \ref g_pntrStartTempAllocStack.
 *   - \ref tempAllocStack is cleared down to \ref g_startTempAllocStack.
 * \warning not UTF-8 safe.
 * \bug
 *   the use of & in \p breakMatch is unnecessarily inconsistent with other
 *   special characters like SOH " or \ .
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

#endif // METAMATH_MMINOU_H_
