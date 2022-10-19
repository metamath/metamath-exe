/*****************************************************************************/
/*            Copyright (C) 2022  Mario Carneiro                             */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#include <stdlib.h>
#include <stdbool.h>
#include "mmtest.h"
#include "mmfatl.h"

/*!
 * \file mmtest.c an implementation of a simple regression test framework
 */

#ifdef TEST_ENABLE

bool g_testFailed = false;

void runTest(bool (*test)(), const char* funcName, const char* testName) {
  if (!TEST_SILENT) printf("running %s:%s...", funcName, testName);
  if (test()) {
    if (!TEST_SILENT) printf(" ok\n");
  } else {
    printf("\nTEST %s:%s FAILED\n", funcName, testName);
    g_testFailed = true;
  }
}

void runTests(void) {
  test_mmfatl();
  exit(g_testFailed ? EXIT_FAILURE : EXIT_SUCCESS);
}

#endif // TEST_ENABLE
