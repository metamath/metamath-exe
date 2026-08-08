/*****************************************************************************/
/*            Copyright (C) 2022  Mario Carneiro                             */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMTEST_H_
#define METAMATH_MMTEST_H_

/*!
 * \file mmtest.h
 * \brief Framework for regression tests.
 *
 * This file is part of the application's test infrastructure.
 *
 * \section regression_tests Regression tests
 * \subsection running_tests Running the tests
 *
 * Regression tests can be run as follows:
 *
 * 1. Open a shell for running Bash commands.
 *
 * 2. Change to the directory containing Metamath's build script \c build.sh,
 * typically located at the top level of the \c metamath-exe project.
 *
 * 3. Execute the following commands:
 * \verbatim
   ./build.sh -ct
   ./metamath_test
   \endverbatim
 *
 * 4. The macro \ref TEST_SILENT can suppress progress and success information.
 * When left in its default state, messages of the following kind are issued:
 * \verbatim
   running test_mmfatl:test_fatalErrorInit... ok
   \endverbatim
 *
 * If every displayed test ends with \c ok, all regression tests have passed.
 * Otherwise, failing tests always display diagnostic information identifying
 * the test and the kind of failure.
 *
 * \subsection test_details Details and implementation
 * If the macro \c TEST_ENABLE is defined by the \c -t option of \c build.sh,
 * the script compiles the regression tests into an executable named
 * \c metamath_test. The value of \c TEST_ENABLE is irrelevant; only whether
 * the macro is defined matters. Defining the macro directly in this file does
 * not affect the executable's name.
 *
 * After enabling or disabling testing, even without modifying any source
 * files, all intermediate artifacts must be rebuilt. Pass the \c -c (clean)
 * option to \c build.sh to enforce this.
 *
 * If testing is disabled, the \c RUN_TESTS_AND_EXIT_IF_ENABLED macro expands
 * to nothing. The compiler also excludes all test code, so the resulting
 * executable does not increase in size. Thus, a disabled test suite incurs
 * neither a linking nor a runtime penalty.
 *
 * If testing is enabled, the progress of tests is reported to \c stdout,
 * subject to \ref TEST_SILENT. The first failed assertion aborts the test
 * suite to which it belongs, and a diagnostic message identifying the failing
 * assertion and its source location is always written. Remaining test suites
 * still run, and the program exits with a nonzero status if any test failed.
 *
 * We recommend running the regression tests whenever the code is modified
 * to ensure that it continues to behave as intended. The tests are also
 * run automatically by GitHub's checks whenever changes are pushed.
 *
 * The macro \c RUN_TESTS_AND_EXIT_IF_ENABLED should be the first instruction
 * in \c main. It expands to nothing when testing is disabled. When testing
 * is enabled, it runs the regression tests and terminates the program,
 * so the normal program execution does not take place:
 * \code
 * int main(int argc, char *argv[]) {
 *
 *  // Expands to nothing if tests are not enabled.
 *  RUN_TESTS_AND_EXIT_IF_ENABLED();
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
  #define RUN_TESTS_AND_EXIT_IF_ENABLED() runTests()

#else // TEST_ENABLE
  #define RUN_TESTS_AND_EXIT_IF_ENABLED()
#endif // TEST_ENABLE

#endif // METAMATH_MMTEST_H_
