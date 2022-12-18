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
 * In particular everything should be pre-allocated, so the risk of a failure
 * in a corrupted or memory-tight environment is minimized.  This is to the
 * detriment of flexibility, in particular, support for dynamic behavior is
 * limited.  Many Standard C library functions like printf MUST NOT be called
 * when heap problems arise, since they use it internally.  GNU tags such
 * functions as 'AS-Unsafe heap' in their documentation (libc.pdf).
 *
 * Often it is sensible to embed details in a diagnosis message.  Placeholders
 * in the format string mark insertion points for such values, much as in
 * \p printf. The variety and functionality is greatly reduced in our case,
 * though.  Only pieces of text or unsigned integers can be embedded
 * (%s or %u placeholder).  This is sufficient to embed an error location
 * given by __FILE__ and __LINE__ into the message.
 *
 * For this kind of expansion you still need a buffer where the final message
 * is constructed.  In our context, this buffer is pre-allocated, and fixed in
 * size, truncation of overflowing text enforced.
 */


/***   Export basic features of the fatal error message processing   ***/


/*! the size a fatal error message including the terminating NUL character can
 * assume without truncation. Must be in the range of an int.
 */
enum {
  MMFATL_MAX_MSG_SIZE = 1024,
};

/*! the character sequence appended to a truncated fatal error message due to
 * a buffer overflow, so its reader is aware a displayed text is incomplete.
 */
#define MMFATL_ELLIPSIS "..."

/*!
 * supported value types of a two character placeholder token in a format
 * string.  The first character of a placeholder is always an escape
 * character \ref MMFATL_PH_PREFIX, followed by one of the type characters
 * mentioned here.  A valid placeholder in a format string is replaced with a
 * submitted value during a parse phase.  The values in the enumeration here
 * are all ASCII characters different from NUL, and mutally distinct.
 *
 * Two \ref MMFATL_PH_PREFIX in succession serve as a special token denoting
 * the character \ref MMFATL_PH_PREFIX itself, as an ordinary text character.
 * It is neither necessary nor allowed to provide a value for substitution
 * in this particular case.
 */
enum fatalErrorPlaceholderType {
   //! escape character marking a placeholder
  MMFATL_PH_PREFIX = '%',
  //! type character marking a placeholder for a string
  MMFATL_PH_STRING = 's',
  //! type character marking a placeholder for an unsigned
  MMFATL_PH_UNSIGNED = 'u',
};


#ifdef TEST_ENABLE

extern void test_mmfatl(void);

#endif

#endif /* include guard */
