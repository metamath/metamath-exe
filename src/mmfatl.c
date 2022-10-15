/*****************************************************************************/
/*        Copyright (C) 2022  Wolf Lammen                                    */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file mmfatl.c a self-contained set of text printing routines using
 * dedicated pre-allocated memory, designed to likely run even under difficult
 * conditions (corrupt state, out of memory).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mmfatl.h"

#define NUL '\x00'

//----------

/*
 * During development you may not want to expose preliminary results to the
 * normal compile, as this would trigger 'unused' warnings, for example.  In
 * the regression test environment your code may be referenced by testing code,
 * though.
 *
 * This section should be empty, or even removed, once your development is
 * finished.
 */
#if TEST_ENABLE
#   define UNDER_DEVELOPMENT
#endif

#ifdef UNDER_DEVELOPMENT
/* the size a fatal error message including the terminating NUL character can
 * assume without truncation.
 */
#define BUFFERSIZE 1024

/* the character sequence appended to a truncated fatal error message, so a
 * user is aware a displayed message is incomplete.
 */
#define ELLIPSIS "..."

char buffer[BUFFERSIZE + sizeof(ELLIPSIS)];

/*
 * We do not rely on any initialization during program start.  Instead we
 * assume the worst case, a corrupted pointer overwrote the buffer.  So we
 * initialize it again immediately before use.
 */
static void initBuffer()
{
    char ellipsis[] = ELLIPSIS;

    memset(buffer, NUL, BUFFERSIZE);
    memcpy(buffer + BUFFERSIZE, ellipsis, sizeof(ellipsis));
}

#endif // UNDER_DEVELOPMENT

//=================   Regression tests   =====================

#if TEST_ENABLE

/* -----   copy & paste code, the same in all modules -----  */

/*
 * If bool_expr evaluates to false, print the message and return to caller
 */
#define CHECK_TRUE(bool_expr)                      \
    if(!(bool_expr)) {                             \
        printf("assertion %s failed", #bool_expr); \
        printf(" at %u:%s\n", __LINE__, __FILE__); \
        exit(EXIT_FAILURE);                              \
    }
    
#define REGRESSION_TEST_SUCCESS            \
    printf("Regression tests in " __FILE__ \
        " completed with success\n");

#define SINGLE_TEST_SUCCESS(name) \
    if (! TEST_SILENT )                          \
      printf(#name " OK\n");

/* -----   end of copy & paste code   ----- */

void test_initBuffer()
{
    // emulate memory corruption
    memset(buffer, 'x', sizeof(buffer));

    initBuffer();

    unsigned i = 0;

    // check the buffer is filled with NUL...
    for (;i < BUFFERSIZE; ++i)
        CHECK_TRUE(buffer[i] == NUL)

    // ...and has the ELLIPSIS string at the end...
    char ellipsis[] = ELLIPSIS;
    unsigned j = 0;
    for (;ellipsis[j] != NUL; ++i, ++j)
        CHECK_TRUE(buffer[i] == ellipsis[j])

    // ... and a terminating NUL character
    CHECK_TRUE(buffer[i] == NUL)

    SINGLE_TEST_SUCCESS(initBuffer)
}

void test_mmfatl()
{
    test_initBuffer();
    REGRESSION_TEST_SUCCESS
}

#endif // TEST_ENABLE
