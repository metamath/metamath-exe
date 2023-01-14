/*****************************************************************************/
/*            Copyright (C) 2022  Mario Carneiro                             */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMTEST_H_
#define METAMATH_MMTEST_H_

/*!
 * \file mmtest.h
 * \brief runs the regression tests
 *
 * part of the application's infrastructure
 *
 * Regression tests
 * ================
 *
 * If the macro **TEST_ENABLE** is defined (option -t of build.sh), then
 * regression tests are run. The setting can be overridden in this file.
 * Invoke in addition option -c (clean) on build.sh, should you switch
 * between with/out testing, but no intermediate source file change.
 *
 * If tests are disabled the \ref runTests line evaluates to nothing.
 * In addition, the compiler skips any test code, so the artifact size will not
 * grow.  All in all, a disabled test suite does not come with a linking or
 * runtime penalty.
 *
 * If enabled, running tests document their progress to stdout.  Testing exits
 * on the first regression found with a diagnostic message further detailing on
 * the context of the failure.
 *
 * We recommend running the tests each time you modify the code to ensure it
 * still executes as desired.  They are automatically invoked by Github checks
 * on each push request.
 */

/*!
 * \def TEST_ENABLE
 * macro, no value, just defined or not.
 *
 * Controls whether the regression tests for a
 * particular module is in/excluded.
 */

// Uncomment this to force-disable tests
// #undef TEST_ENABLE

// Uncomment this to force-enable tests
// #define TEST_ENABLE

#include <stdio.h>

/*!
 * \def TEST_SILENT
 * macro, either true or false.
 *
 * Controls the verbosity of a regression test.  If true, success messages are
 * mostly suppressed during a test run.  A failing test always produces output.
 */
#define TEST_SILENT false

#ifdef TEST_ENABLE

  extern void runTest(
      bool (*test)(), const char* funcName, const char* testName);
  #define RUN_TEST(testName) runTest(testName, __func__, #testName)

  /*
  * If bool_expr evaluates to false, print an error message and return to
  * caller.
  *
  * Use this extension of ASSERT if file and line number is not
  * sufficient to locate the failing test.
  * Accepts a format string for the assertion message.
  */
  #define ASSERTF(bool_expr, ...)              \
    if (!(bool_expr)) {                        \
      printf("\n%s: ", __func__);              \
      printf(__VA_ARGS__);                     \
      printf(" at %s:%u", __FILE__, __LINE__); \
      return false;                            \
    }

  /*
  * If bool_expr evaluates to false, print an error message and return to
  * caller.
  *
  * File, line and the function this macro is embedded in is sufficient to
  * identify the error position.
  */
  #define ASSERT(bool_expr) \
      ASSERTF(bool_expr, "assertion %s failed", #bool_expr)

  extern void runTests(void);
  #define RUN_TESTS() runTests()

#else // TEST_ENABLE
  #define RUN_TESTS()
#endif // TEST_ENABLE

#endif // METAMATH_MMTEST_H_
