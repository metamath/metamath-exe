´/*****************************************************************************/
/*            Copyright (C) 2022  Mario Carneiro                             */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file mmtest.c Regression test framework
 * \brief An implementation of a simple regression test framework.
 *
 * See \ref mmtest.h for the interface.
 */

/*!
 * \page usage-regression-tests Using regression tests
 * \section regression-tests Regression tests
 * \subsection running-tests Running the tests
 *
 * Regression tests are part of a development cycle. They are routinely run
 * whenever changes are pushed to GitHub's metamath-exe project.
 *
 * A developer may additionally want to run them during development to ensure
 * that intermediate changes remain functionally correct and do not deviate
 * from previous implementations that are known to be correct.
 *
 * Regression tests can, for example, be run as follows:
 *
 * 1. Open a shell for running Bash commands.
 *
 * 2. Change to the directory containing Metamath's build script \e build.sh,
 * typically located at the top level of the \e metamath-exe project.
 *
 * 3. Execute the following commands:
 * \verbatim
   ./build.sh -t
   ./metamath_test
   \endverbatim
 * You don't need to use flag \c -c, because the build script always cleans the
 * build directory.
 *
 * 4. The test program reports its progress to \c stdout:
 * \verbatim
   running test_mmfatl:test_fatalErrorInit... ok
   \endverbatim
 * If every displayed test ends with \e ok, all regression tests have passed.
 * Progress and success messages can be suppressed, see \ref TEST_SILENT.
 * Failing tests always display diagnostic information identifying the failing
 * assertion and its source location.
 *
 * \subsection test-details Details and implementation
 * The normal Metamath application distributed by GitHub has already passed
 * the regression tests. To avoid the overhead of loading a larger executable
 * and initializing regression-test code when the normal application starts,
 * that code should therefore be included only conditionally during
 * compilation. An executable compiled with regression tests executes the tests
 * and then exits, although all normal application code remains available
 * both to preserve the normal executable's behavior and to support testing.
 *
 * The macro \c TEST_ENABLE controls whether the regression tests are
 * compiled into an executable.
 *
 * If testing is disabled, the \c RUN_TESTS_AND_EXIT_IF_ENABLED macro expands
 * to nothing. The compiler also excludes all test code, so the resulting
 * executable does not increase in size. Thus, a disabled test suite incurs
 * neither a linking nor a runtime penalty.
 *
 * If testing is enabled, the expansion of the macro
 * \c RUN_TESTS_AND_EXIT_IF_ENABLED runs the regression tests, and exits
 * afterwards. The progress of tests is controlled by \ref TEST_SILENT. A
 * failed assertion issues a diagnostic message, and may abort the test suite
 * to which it belongs. Other independent test suites continue to run, and the
 * program exits afterwards with status 0 if all tests passed, and nonzero
 * otherwise.
 *
 * The macro \c RUN_TESTS_AND_EXIT_IF_ENABLED should be the first instruction
 * in \c main:
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

#include "mmtest.h"

#ifdef TEST_ENABLE

#include <stdlib.h> // for exit()

// modules supporting regression tests
#include "mmfatl.h"

/*!
 * \brief Summarizes the result of all regression tests executed so far.
 *
 * The \c bool variable \p g_testFailed remains unchanged when a test succeeds
 * and is set to \c true if any test fails.
 *
 * When the test program \e metamath_test terminates after executing the last
 * regression test, this variable determines its exit code: either
 * \c EXIT_SUCCESS or \c EXIT_FAILURE.
 *
 * \pre \c TEST_ENABLE is defined.
 */
static bool g_testFailed = false;

void runTest(bool (*test)(void), const char* suiteName, const char* testName) {
  if (!TEST_SILENT) printf("running %s:%s...", suiteName, testName);
  if (test()) {
    if (!TEST_SILENT) printf(" ok\n");
  } else {
    printf("\nTEST %s:%s FAILED\n", suiteName, testName);
    g_testFailed = true;
  }
}

void runTests(void) {
  test_mmfatl();
  exit(g_testFailed ? EXIT_FAILURE : EXIT_SUCCESS);
}

#endif // TEST_ENABLE
