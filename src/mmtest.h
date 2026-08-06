/*****************************************************************************/
/*            Copyright (C) 2022  Mario Carneiro                             */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMTEST_H_
#define METAMATH_MMTEST_H_


/*!
 *
 * \file mmtest.h
 * \brief Framework for regression tests.
 *
 * This file is part of the application's test infrastructure.
 *
 * \section regression_tests Regression tests
 *
 * Regression tests can be run as follows:
 *
 * 1. Open a shell for running Bash commands.
 *
 * 2. Change to the directory containing Metamath's build script
 * \c build.sh.
 *
 * 3. Execute the following commands:
 * \verbatim
   ./build.sh -ct
   ./metamath_test
   \endverbatim
 *
 * 4. The test program produces output similar to the following:
 *  \verbatim
   > running test_mmfatl:test_fatalErrorInit... ok
  \endverbatim
 *
 * If every test ends with \c ok, all regression tests have passed.
 * Otherwise, diagnostic information identifies the failed test.
 *
 * If the macro \c TEST_ENABLE is defined (the \c -t option of
 * \c build.sh), the regression tests are compiled into a separate
 * executable named \c metamath_test. The value of \c TEST_ENABLE can
 * also be overridden in this file. If testing is switched on or off
 * without modifying any source files in between, also pass the \c -c
 * (clean) option to \c build.sh to remove intermediate build artifacts.
 *
 * If testing is disabled, the \c RUN_TESTS_IF_ENABLED macro expands to
 * nothing. The compiler also excludes all test code, so the resulting
 * executable does not increase in size. Thus, a disabled test suite incurs
 * neither a linking nor a runtime penalty.
 *
 * If testing is enabled, the tests report their progress to \c stdout.
 * Execution stops at the first regression failure, and a diagnostic
 * message provides further details about the context of the failure.
 *
 * We recommend running the regression tests whenever the code is modified
 * to ensure that it continues to behave as intended. The tests are also
 * run automatically by GitHub checks whenever changes are pushed.
 *
 * The macro \c RUN_TESTS_IF_ENABLED should be the first instruction in
 * \c main. It expands to nothing when testing is disabled. When testing
 * is enabled, it runs the regression tests and terminates the program,
 * so the normal program code is not executed:
 * \code
 * int main(int argc, char *argv[]) {
 *
 *  // Expands to nothing if tests are not enabled.
 *  RUN_TESTS_IF_ENABLED();
 *
 *  // This code is not reached if tests are enabled.
 *
 *  // Code performing normal execution of Metamath goes here.
 * }
 * \endcode
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
  #define RUN_TESTS_IF_ENABLED() runTests()

#else // TEST_ENABLE
  #define RUN_TESTS_IF_ENABLED()
#endif // TEST_ENABLE

#endif // METAMATH_MMTEST_H_
