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

/*!
 * character constant
 */
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

static char buffer[BUFFERSIZE + sizeof(ELLIPSIS)];

/*
 * We do not rely on any initialization during program start.  Instead we
 * assume the worst case, a corrupted pointer overwrote the buffer.  So we
 * initialize it again immediately before use.
 */
static void initBuffer(void) {
  char ellipsis[] = ELLIPSIS;

  memset(buffer, NUL, BUFFERSIZE);
  memcpy(buffer + BUFFERSIZE, ellipsis, sizeof(ellipsis));
}

#endif // UNDER_DEVELOPMENT


//=================   Regression tests   =====================

#ifdef TEST_ENABLE

bool test_initBuffer(void) {
  // emulate memory corruption
  memset(buffer, 'x', sizeof(buffer));

  initBuffer();

  unsigned i = 0;

  // check the buffer is filled with NUL...
  for (;i < BUFFERSIZE; ++i)
    ASSERT(buffer[i] == NUL);

  // ...and has the ELLIPSIS string at the end...
  char ellipsis[] = ELLIPSIS;
  unsigned j = 0;
  for (;ellipsis[j] != NUL; ++i, ++j)
    ASSERT(buffer[i] == ellipsis[j]);

  // ... and a terminating NUL character
  ASSERT(buffer[i] == NUL);
  return true;
}

void test_mmfatl(void) {
  RUN_TEST(test_initBuffer);
}

#endif // TEST_ENABLE
