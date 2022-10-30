/*****************************************************************************/
/*            Copyright (C) 2022  Wolf Lammen                                */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMFATL_H_
#define METAMATH_MMFATL_H_

#include <stdbool.h>
#include "mmtest.h"

/*!
 * \file mmfatl.h
 * \brief supports generating of fatal error messages
 *
 * part of the application's infrastructure
 *
 * Rationale
 * =========
 *
 * When a fatal error occurs, the internal structures of a program may be
 * corrupted to the point that recovery is impossible.  The program exits
 * immediately, but hopefully still displays a diagnostic message.
 *
 * To display this final message, we restrict its code to very basic,
 * self-contained routines, independent of the rest of the program to the
 * extent possible, thus avoiding any corrupted data.
 *
 * In particular everything should be pre-allocated and initialized, so the
 * risk of a failure in a corrupted or memory-tight environment is minimized.
 * This is to the detriment of flexibility, in particular, support for dynamic
 * behavior is limited.  Many Standard C library functions like printf MUST NOT
 * be called when heap problems arise, since they use malloc internally.  GNU
 * tags such functions as 'AS-Unsafe heap' in their documentation (libc.pdf).
 *
 * Often it is sensible to embed details in a diagnosis message.  Placeholders
 * in the format string mark insertion points for such values, much as in
 * \p printf. The variety and functionality is greatly reduced in our case,
 * though.  Only pieces of text or unsigned integers can be embedded
 * (%s or %u placeholder).
 *
 * For this kind of expansion you still need a buffer where the final message
 * is constructed.  In our context, this buffer is pre-allocated, and fixed in
 * size, truncation of overflowing text enforced.
 */

/* the size a fatal error message including the terminating NUL character can
 * assume without truncation. Must be in the range of an int.
 */
enum {
  MMFATL_BUFFERSIZE = 1024,
};

/* the character sequence appended to a truncated fatal error message due to
 * a buffer overflow, so a reader is aware a displayed text is incomplete.
 */
#define MMFATL_ELLIPSIS "..."

/*!
 * \brief Preparing internal data structures for an error message.
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
 *   a message from a format string and parameter values.

 */

extern void fatalErrorInit(void);

/*!
 * supported value types of a two character placeholder token in a format
 * string.  The first character of a placeholder is always an escape
 * character \ref MMFATL_PH_PREFIX, followed by one of the type characters
 * mentioned here.  A valid placeholder in a format string is replaced with a
 * submitted value during a parse phase.  The values in the enumeration here
 * are all ASCII characters different from NUL, and distinct from each other.
 *
 * Two \ref MMFATL_PH_PREFIX in succession serve as a special token denoting
 * the character \ref MMFATL_PH_PREFIX itself, as an ordinary text character.
 * It is neither necessary nor allowed to provide a value for substitution
 * in this particular case.
 */
enum fatalErrorPlaceholderType {
  MMFATL_PH_PREFIX = '%', //!< escape character marking a placeholder
  MMFATL_PH_STRING = 's', //<! type character marking a placeholder for a string
  MMFATL_PH_UNSIGNED = 'u', //<! type character marking a placeholder for an unsigned
};

/*!
 * \brief grammar support: generates a placeholder for insertion into a format
 *   string.
 *
 * The placeholders in fatal errors are a subset of those used in the C library
 * function printf.  A pedantic implementation might want Metamath generate
 * placeholders to cover all exceptions, during an automatic message
 * generation, instead of hardcoding them directly in the format string.
 * \param type data type of the value replacing the placeholder in a format
 *   string.  \ref MMFATL_PH_PREFIX is allowed as a type, yielding a token
 *   standing for the character \ref MMFATL_PH_PREFIX itself.
 * \return a placeholder of a supported type, or NULL, if you somehow
 *   manage to dodge the C type checking.
 * \attention the result is stable only until the next call to this
 *   function.
 */
extern char const* fatalErrorPlaceholder(enum fatalErrorPlaceholderType aType);

/*!
 * \brief appends text to the current contents in the message buffer.
 * 
 * appends new text to the message buffer.  The submitted extra parameters
 * following \p format must match the placeholders in the \p format string
 * in type.  It is possible to add more parameters than necessary (they are
 * simply ignored then), but never fewer.  The caller is responsible for
 * this pre-condition, no runtime check is performed.
 * 
 * \param format [null] a format string usually containing NUL terminated
 *   UTF-8 (superset of ASCII) encoded text, along with embedded placeholders,
 *   that are replaced with parameters following \p format in the call.  The
 *   replacements should not violate UTF-8, after being embedded in
 *   surrounding text.  No runtime check is performed to ensure this rule.
 *
 *   A placeholder begins with an escape character \ref MMFATL_PH_PREFIX,
 *   immediately followed by a type character.  Currently two types are
 *   implemented \ref MMFATL_PH_STRING and \ref MMFATL_PH_UNSIGNED.
 *
 *   If you need a \ref MMFATL_PH_PREFIX verbatim in the error message, use
 *   two \ref MMFATL_PH_PREFIX in succession.  They will automatically be
 *   replaced with a single one.
 *
 *   NULL is equivalent to an empty format string, and supported both as a
 *   \ref format string and as a parameter for a string placeholder, to
 *   enhance robustness.
 * 
 * The \p format is followed by a possibly empty list of paramaters substituted
 *   for placeholders.  Currently unsigned int values may replace a
 *   \ref MMFATL_PH_UNSIGNED type placeholder, and a char const* pointer a
 *   \ref MMFATL_PH_STRING type placeholder.  If the latter pointer is NULL,
 *   the placeholder is replaced with an empty string, else it must point to a
 *   UTF-8 encoded text.  No value is required for the \ref MMFATL_PH_PREFIX
 *   type tokens.
 * 
 * \pre \p format if not NULL, contains NUL terminated UTF-8 text.
 * \pre string parameters following
 * \pre \ref fatalErrorInit was called before.
 * \pre the submitted parameters following \p format must match in type the
 *   placeholders in \p format.  Their count may exceed that of the
 *   placeholders, but must never be less.  String parameters 
 * \post the message is appended to the current buffer contents.  It is
 *   truncated if there is insufficient space for it, including the
 *   terminating NUL.
 * \return false iff the message buffer is in overflow state.
 */
extern bool fatalErrorPush(char const* format, ...);

/*!
 * \brief display buffer contents and exit program with code EXIT_FAILURE.
 * 
 * This function does not return.
 * 
 * \pre \ref fatalErrorInit has initialized the internal error message
 *   buffer possibly followed by a sequence of \ref fatalErrorPush
 *   filling it with a message.
 * \post [noreturn] the program terminates with an error code, after
 *   writing the buffer contents to stderr.
 */
extern void fatalErrorPrintAndExit();

/*!
 * convenience function, covering a sequence of \ref fatalErrorInit,
 * \ref fatalErrorPush and \ref fatalErrorPrintAndExit in succession.  This
 * function does not return.
 *
 * \param file [null] filename of code responsible for calling this function,
 *   suitable for macro __FILE__.  Part of an error location.
 * \param line [unsigned] if greater 0, interpreted as a line number, where
 *   a call to this function is initiated, suitable for macro __LINE__.
 *   Part of an error location.
 * \param msgWithPlaceholders the error message to display.  This message
 *   may include placeholders, in which case it must be followed by more
 *   parameters, corresponding to the values to replace the placeholders.
 *   These values must match in type that of the placeholders, and there
 *   must be enough to cover all placeholders.
 * \post the program exits with failure code, after writing the error
 *   location and message to stderr.
 */
extern void fatalErrorExitAt(char const* file, unsigned line,
                             char const* msgWithPlaceholders, ...);

#ifdef TEST_ENABLE

extern void test_mmfatl(void);

#endif

#endif /* include guard */
