/*****************************************************************************/
/*            Copyright (C) 2022  Wolf Lammen                                */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMFATL_H_
#define METAMATH_MMFATL_H_

#include <stdbool.h>

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
 *
 * Regression tests
 * ================
 *
 * If the macro **BUILD_REQUESTS_REGRESSION_TEST** is defined (option -t of
 * build.sh) or **TEST_MMFATL** is defined, then regression tests are
 * implemented.  Invoke in addition option -c on build.sh, should you switch
 * between with/out testing, but no intermediate source file change.
 *
 * In order to run implemented regression tests properly we suggest to add a
 * \code{.c}
 * test_mmfatl();
 * \endcode
 * line to main() close to its begin and **before** any function declared in
 * this header file is called.
 *
 * If tests are disabled this line evaluates to nothing.  In addition, the
 * compiler skips any test code, so the artifact size will not grow.  All in
 * all, a disabled test suite does not come with a linking or runtime penalty.
 *
 * If enabled, running tests document their progress to stdout.  Testing exits
 * on the first regression found with a diagnostic message further detailing on
 * the context of the failure.
 *
 * We recommend running the tests each time you modify mmfatl.h or mmfatl.c
 * to ensure it still executes as desired.  They are automatically invoked by
 * Github checks on each push request.
 */

// Setting TEST_MMFATL to en/disable regression tests in this module
//------------------------------------------------------------------

/* If BUILD_REQUESTS_REGRESSION_TEST is defined compilation of regression
 * tests is requested from somewhere outside of C files.  This can still be
 * overridden locally by setting TEST_MMFATL explicitely below.
 */
#ifdef BUILD_REQUESTS_REGRESSION_TEST
#   define TEST_MMFATL
/* optimized for continuous integration */
#   define TEST_MMFATL_SILENT true
#else
/* each test prints a confirmation on the screen */  
#   define TEST_MMFATL_SILENT false
#endif

/* uncomment one of the following to enforce disabling/enabling of regression
 * tests in this file unconditionally
 */
// #undef TEST_MMFATL
// #define TEST_MMFATL

#ifdef TEST_MMFATL

/* enable regression tests in main() in metamath.c */
#   define RUN_REGRESSION_TEST
    /* regression tests are implemented and called through this function */
    extern void test_mmfatl(bool*);
#else
    /* still necessary should another unit request regression tests */
#   define test_mmfatl(x)
#endif

#endif /* include guard */
