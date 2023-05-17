/*****************************************************************************/
/*            Copyright (C) 2022  Wolf Lammen                                */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMFATL_H_
#define METAMATH_MMFATL_H_

#include <stdbool.h>
#include "mmtest.h"

/* Documentation
 * =============
 *
 * Most comments are written in <a href="https://doxygen.nl/index.html">
 * doxygen</a> (Qt variant) style.  If you have doxygen installed on your
 * computer, you may generate a hyperlinked HTML documentation out of them with
 * its root page placed in build/html/index.html by running build.sh with the
 * -d option.
 *
 * Delete this once we have this description available in a central location.
 * Keep this as a simple comment, so it is outside of Doxygen documentation.
 */

/*!
 * \file mmfatl.h
 * \brief supports generating of fatal error messages
 *
 * In a sort of 3-tier architecture consisting of resource/configuration/data
 * management, operation logic and algorithms, and presentation layer (or user
 * interface), this code is interfacing the operating system on the
 * lowest infrastructure (resource) layer.
 *
 * Rationale
 * =========
 *
 * When a fatal error occurs, the internal structures of a program may be
 * corrupted to the point that recovery is impossible.  The program exits
 * immediately with a failure code, but hopefully still displays a diagnostic
 * message.
 *
 * To display this final message, we restrict its code to very basic,
 * self-contained routines.  It does not share state with the rest of the
 * program, and uses shared resources only to the bare minimum:
 * (a) needs a few hundred bytes of stack memory, i.e. the most basic execution
 *     environment capable of calling a library function;
 * (b) expects only a simple library function forwarding 1 KB of raw text to
 *     stderr being operational.
 * In particular memory corruption in the Metamath executable cannot affect the
 * handling of the fatal error.
 *
 * To achieve this everything should be pre-allocated, so the risk of a failure
 * in a corrupted or memory-tight environment is minimized.  This is to the
 * detriment of flexibility, in particular, support for dynamic behavior is
 * limited.  Many Standard C library functions like \p printf MUST NOT be
 * called when heap problems arise, since they rely on it internally.  GNU tags
 * such functions as 'AS-Unsafe heap' in their documentation (libc.pdf).
 *
 * Often it is sensible to embed details in a diagnosis message.  Placeholders
 * in the format string mark insertion points for such values, much as in
 * \p printf. The variety and functionality is greatly reduced in our case,
 * though.  Only pieces of text or unsigned integers can be embedded
 * (%s or %u placeholder).  This is sufficient to embed an error location
 * given by __FILE__ and __LINE__ into the message.
 *
 * For this kind of expansion you still need a buffer where the final message
 * is constructed.  In our context, this buffer is pre-allocated, fixed in
 * size, and truncation of overflowing text enforced.
 *
 * Implementation hint
 * -------------------
 *
 * In a memory tight situation we cannot reset the memory heap, or stack, to
 * have free space again for, say, \p printf, even though we are about to exit
 * program execution, for two reasons:
 *   - We want to gather diagnostic information, so some program structures
 *     need to be intact;
 *   - The fatal error routines need not be the last portion of the program
 *     executing.  If a function is registered with \p atexit, it is called
 *     after an exit is triggered, and this function may rely on allocated
 *     data.
 */

// ***   Export basic features of the fatal error message processing   ***/

/*!
 * \brief size of a text buffer used to construct a message
 *
 * the size a fatal error message including the terminating NUL character can
 * assume without truncation. Must be in the range of an int.
 */
enum {
  MMFATL_MAX_MSG_SIZE = 1024,
};

/*!
 * \brief ASCII text sequence indicating truncated text
 *
 * the character sequence appended to a truncated fatal error message due to a
 * buffer overflow, so its reader is aware a displayed text is incomplete.  The
 * ellipsis is followed by a line feed to ensure an overflown message is still
 * on the previous line of the command prompt following program exit.
 */
#define MMFATL_ELLIPSIS "...\n"

/*!
 * \brief ASCII characters used for placeholder tokens, printf style
 *
 * Supported value types of a two character placeholder token in a format
 * string.  The first character of a placeholder is always an escape
 * character \ref MMFATL_PH_PREFIX, followed by one of the type characters
 * mentioned here.  A valid placeholder in a format string is replaced with a
 * submitted value during a parse phase.  The values in the enumeration here
 * are all ASCII characters different from NUL, and mutually distinct.
 *
 * Two \ref MMFATL_PH_PREFIX in succession serve as a special token denoting
 * the character \ref MMFATL_PH_PREFIX itself, as an ordinary text character.
 * It is neither necessary nor allowed to provide a value for substitution
 * in this particular case.
 */
enum fatalErrorPlaceholderType {
  //! escape character marking a placeholder, followed by a type character
  MMFATL_PH_PREFIX = '%',
  //! type character marking a placeholder for a string
  MMFATL_PH_STRING = 's',
  //! type character marking a placeholder for an unsigned
  MMFATL_PH_UNSIGNED = 'u',
};

// ***   Interface of fatal error message processing   ***/

/*!
 * \brief Prepare internal data structures for an error message.
 *
 * Empties the message buffer used to construct error messages by
 * \ref fatalErrorPush.
 *
 * Prior to generating an error message some internal data structures need to
 * be initialized.  Usually such initialization is automatically executed on
 * program startup.  Since we are in a fatal error situation, we do not rely
 * on this.  Instead we assume memory corruption has affected this module's
 * data and renders its state useless.  So we initialize it immediately before
 * the error message is generated.  Note that we still rely on part of the
 * system be running.  We cannot overcome a fully clobbered system, we only
 * can increase our chances of bypassing some degree of memory corruption.
 * \post internal data structures are initialized and ready for constructing
 *   a message from a format string and parameter values.  Any previous
 *   contents is discarded.
 * \invariant the memory state of the rest of the program is not changed (in
 *   case there is still a function in the atexit queue).
 */
extern void fatalErrorInit(void);

/*!
 * \brief append text to the current contents in the message buffer.
 *
 * Appends new text to the message buffer.  The submitted extra parameters
 * following \p format must match the placeholders in the \p format string
 * in type.  It is possible to add more parameters than necessary (they are
 * simply ignored then), but never fewer.  The caller is responsible for
 * this pre-condition, no runtime check is performed.
 *
 * \param[in] format [null] a format string containing NUL terminated ASCII
 *   encoded text, along with embedded placeholders, that are replaced with
 *   parameters following \p format in the call.
 *
 *   A placeholder begins with an escape character \ref MMFATL_PH_PREFIX,
 *   immediately followed by a type character.  Currently two types are
 *   implemented \ref MMFATL_PH_STRING and \ref MMFATL_PH_UNSIGNED.
 *
 *   If you need a \ref MMFATL_PH_PREFIX verbatim in the error message, use
 *   two \ref MMFATL_PH_PREFIX in succession.  They will automatically be
 *   replaced with a single one.
 *
 *   For convenience \ref getFatalErrorPlaceholderToken may provide the
 *   correct placeholder token.
 *
 *   NULL is equivalent to an empty format string, and supported both as a
 *   \p format string and as a parameter for a string placeholder, to
 *   enhance robustness.
 *
 *   It is recommended to let the message end with a LF character, so a command
 *   prompt following it is displayed on a new line.  If it is missing
 *   \ref fatalErrorPrintAndExit will supply one, but relying on this adds an
 *   unnecessary correction under severe conditions.
 *
 * The \p format is followed by a possibly empty list of parameters substituted
 *   for placeholders.  Currently unsigned int values may replace a
 *   \ref MMFATL_PH_UNSIGNED type placeholder (%u), and a char const* pointer a
 *   \ref MMFATL_PH_STRING type placeholder (%s).  If the latter pointer is
 *   NULL, the placeholder is replaced with an empty string, else it must point
 *   to ASCII encoded NUL terminated text.  No value is required for the
 *   \ref MMFATL_PH_PREFIX type tokens.
 * \return false iff the message buffer is in overflow state.
 *
 * \pre the buffer is initialized, by calling \ref fatalErrorInit prior
 * \pre the submitted parameters following \p format must match in type and
 *   order the placeholders in \p format.  Their count may exceed that of the
 *   placeholders, but must never be less.
 * \post the message is appended to the current buffer contents.  It is
 *   truncated if there is insufficient space for it, including the
 *   terminating NUL.
 * \invariant the memory state of the rest of the program is not changed (in
 *   case there is still a function in the atexit queue).
 * \warning This function does not automatically wrap messages and insert LF
 *   characters at the end of the declared screen width (g_screenWidth in
 *   mminou.h) as print2 does.  Tests show that usual terminal emulators break
 *   up text at the last column, but that may depend on the used
 *   hard-/software.
 */
extern bool fatalErrorPush(char const* format, ...);

/*!
 * \brief display buffer contents and exit program with code EXIT_FAILURE.
 *
 * This function does not return.
 *
 * A NUL terminated message has been prepared in an internal buffer using a
 * sequence of \ref fatalErrorPush.  This function writes this message to
 * stderr (by default tied to the terminal screen like stdout), and exits the
 * program afterwards, indicating a failure to the operating system.
 *
 * A line feed is appended to a prepared non-empty message if it is not its
 * last character.  This keeps the message and a command prompt following the
 * exit on separare lines.  It is recommended to avoid this corrective
 * measure and supply a terminating line feed as part of the message.
 *
 * It is possible to call this function without preparing a message.  In this
 * case nothing is written to stderr, and the program just exits with a failure
 * code.
 *
 * \pre the buffer is initialized, by calling \ref fatalErrorInit prior,
 *   possibly followed by a sequence of \ref fatalErrorPush filling it with a
 *   message.
 * \post [noreturn] the program terminates with error code EXIT_FAILURE, after
 *   writing the buffer contents to stderr.
 * \post a line feed is appended to any non-empty message, if it is not
 *   provided
 * \invariant the memory state of the rest of the program is not changed (in
 *   case there is still a function in the atexit queue).
 * \attention Although this function does not return to the caller, we must not
 *   assume it is the last piece of program code executing.  A function
 *   registered with \p atexit executes after \p exit is triggered.  That is
 *   why the above invariant is important to keep.
 * \warning the output is to stderr, not to stdout.  As long as you do not
 *   redirect stderr to, say, a log file, the error message is displayed to the
 *   user on the terminal.
 * \warning previous versions of Metamath returned the exit code 1.  Many
 *   systems define EXIT_FAILURE to this very value, but that is not mandated
 *   by the C11 standard.  In fact, some systems may interpret 1 as a success
 *   code, so EXIT_FAILURE is more appropriate.
 */
extern void fatalErrorPrintAndExit(void);

/*!
 * \brief standard error reporting and program exit with failure code.
 *
 * Convenience function, covering a sequence of \ref fatalErrorInit,
 * \ref fatalErrorPush and \ref fatalErrorPrintAndExit in succession.  This
 * function does not return.
 *
 * If an error location is given, it is printed first, followed by any
 * non-empty message.  A line feed is padded to the right of the message if it
 * is missing.  This is to keep a following command prompt on a new line.  It
 * is recommended to avoid this corrective measure and supply a line feed as
 * part of the message.
 *
 * If all parameters are NULL or 0, no message is printed, not even the
 * automatically supplied line feed, and this function just exits the program
 * with an error code.  If possible use \ref fatalErrorPrintAndExit directly
 * instead.
 *
 * If you need to concat two or more pieces to form the error message, use
 * a sequence of \ref fatalErrorInit, multiple \ref fatalErrorPush and finally
 * \ref fatalErrorPrintAndExit instead of this function.
 *
 * \param[in] file [null] filename of code responsible for calling this
 *   function, suitable for macro __FILE__.  Part of an error location.
 *   Ignored in case of NULL.
 * \param[in] line [unsigned] if greater 0, interpreted as a line number, where
 *   a call to this function is initiated, suitable for macro __LINE__.  Part
 *   of an error location.
 * \param[in] msgWithPlaceholders [null] the error message to display.  This
 *   message may include placeholders in printf style, in which case it must be
 *   followed by more parameters, corresponding to the values replacing
 *   placeholders.  These values must match in type that of the placeholders,
 *   and their number must be enough (can be more) to cover all placeholders.
 *   The details of this process is explained in \ref fatalErrorPush.  Ignored
 *   if NULL.
 * \post the program exits with EXIT_FAILURE return code, after writing the
 *   error location and message to stderr.
 * \invariant the memory state of the rest of the program is not changed (in
 *   case there is still a function in the atexit queue).
 */
extern void fatalErrorExitAt(char const* file, unsigned line,
                             char const* msgWithPlaceholders, ...);

#ifdef TEST_ENABLE

extern void test_mmfatl(void);

#endif // TEST_ENABLE

#endif // METAMATH_MMFATL_H_
