/*****************************************************************************/
/*            Copyright (C) 2022  Wolf Lammen                                */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMFATL_H_
#define METAMATH_MMFATL_H_

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
 * be called when heap problems arise, since they use it internally.  GNU tags
 * such functions as 'AS-Unsafe heap' in their documentation (libc.pdf).
 *
 * A corrupt state is often caused by limit violations overwriting adjacent
 * memory.  To specifically guard against this, the pre-allocated memory area,
 * at your option, may include safety borders detaching necessary pre-set
 * administrative data from other memory.
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

#endif /* include guard */
