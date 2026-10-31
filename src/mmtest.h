/*****************************************************************************/
/*            Copyright (C) 2022  Mario Carneiro                             */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMTEST_H_
#define METAMATH_MMTEST_H_

/* Documentation
 * =============
 *
 * Most comments are written in <a href="https://doxygen.nl/index.html">
 * doxygen</a> (Qt variant) style.  If you have doxygen installed on your
 * computer, you may generate a hyperlinked HTML documentation out of them with
 * its root page placed in build/html/index.html by running build.sh with the
 * -d flag.
 *
 * Delete this once we have this description available in a central location.
 * Keep this as a simple comment, so it is outside of Doxygen documentation.
 */

/* Compilation of this code requires at least C99. This should be established
 *   somewhere during configuration, but is explicitly required here (again).
 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
# error "C99 or later is required."
#endif

/*!
 * \file mmtest.h
 * \brief Framework for regression tests.
 *
 * This file is part of the application's test infrastructure.
 *
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
 * 2. Change to the directory containing Metamath's build script \c build.sh,
 * typically located at the top level of the \c metamath-exe project.
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
 * If every displayed test ends with \c ok, all regression tests have passed.
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

/*!
 * \def TEST_ENABLE
 * \brief If defined, enables regression tests.
 *
 * This macro controls the conditional inclusion of regression tests. Its value
 * is irrelevant; only whether it is defined matters.
 *
 * Metamath provides three ways to define \p TEST_ENABLE:
 * 1. Set the \c -t option when running \c build.sh. This is the recommended
 *   and easiest method, see \ref running-tests;
 * 2. If you use your own build script and compile the sources with \c gcc
 *   specify the \c -DTEST_ENABLE option to the GNU compiler;
 * 3. Modify this file and uncomment one of the \c \#define or \c \#undef lines
 *   related to \p TEST_ENABLE. These lines take precedence over other methods
 *   of controlling testing. Scripts that operate directly on the source code
 *   may prefer this method.
 */

// Uncomment this to force-disable tests
// #undef TEST_ENABLE

// Uncomment this to force-enable tests
// #define TEST_ENABLE

/* This macro is nowhere defined in the Metamath source code; it is used only
 * for Doxygen by a PREDEFINED entry in Doxyfile.diff. Therefore, code
 * conditional on this macro being defined is ignored by the compiler, but
 * processed by Doxygen.
 */
#ifdef DOXYGEN_BUILD

/* Fake definition visible to Doxygen only. Without defining TEST_ENABLE,
 * Doxygen issues a warning that the documentation item is not related to
 * anything. One could have used PREDEFINED for this as well, but we avoid
 * including knowledge about Metamath details in the configuration of an
 * external, non-central tool.
 */
  #define TEST_ENABLE
#endif // DOXYGEN_BUILD

#include <stdbool.h>
#include <stdio.h>

#ifdef TEST_ENABLE
  /*!
   * \def TEST_SILENT
   * \brief A <tt>_Bool</tt>-compatible macro controlling whether successful
   * regression tests produce output.
   *
   * Has no effect when regression tests are disabled. The default is \c false,
   * allowing successful tests to report their progress.
   *
   * Defining \p TEST_SILENT on the compiler command line takes precedence over
   * the default. This is recommended for automated checks, where only test
   * failures need to be reported. The \c -q option to \c build.sh also sets
   * this macro to \c true, suppressing output from successful tests:
   * \code
   * ./build.sh -qt
   * \endcode
   *
   * Developers may prefer to see test progress when verifying that a change
   * has not introduced regressions, which is why output is enabled by default.
   *
   * Historically, \p TEST_SILENT was modified directly in this file to control
   * test output. This method remains supported for backward compatibility.
   */
  #ifndef TEST_SILENT
    // the default
    #define TEST_SILENT false
  #endif

  /*!
   * \brief Runs a test function and reports its result.
   *
   * Wraps a particular test named \p testName, which belongs to the test suite
   * \p suiteName, and writes a message to \c stdout according to the result.
   * If the test function \p test returns \c false, indicating failure, a
   * summary error message consisting of the suite name and test name is sent
   * to \c stdout, following any messages already issued by the \p test
   * function. Otherwise, a message indicating successful completion of the
   * test may be written, depending on the setting of \ref TEST_SILENT.
   *
   * A failing test sets an internal flag .
   *
   * \pre Macro \ref TEST_ENABLE is defined.
   * \pre Macro \ref TEST_SILENT is set. It determines whether successful tests
   *   are reported.
   * \param[in] test [not null] Pointer to a function taking no parameters and
   *   returning a \c _Bool value: \c true indicates success of the called
   *   individual test of the suite, \c false a failure.
   * \param[in] suiteName Name identifying the suite being run, often derived
   *   from the module whose functionality is being tested.
   * \param[in] testName Name identifying the test. Should be unique within a
   *   test suite.
   * \post If \p test fails, an internal status is updated to indicate at least
   *   one regression test failed.
   */
  void runTest(
      bool (*test)(void), const char* suiteName, const char* testName);

  /*!
   * \brief Runs a single test or a suite of tests.
   *
   * This macro expands to a call to \c runTest, passing the specified test
   * function, the name of the calling function as the suite name, and the test
   * function's name as the test name.
   *
   * Each test returns a Boolean value indicating success or failure. On
   * failure, the calling code may conditionally skip subsequent tests or
   * abort further testing altogether. Tests can be organized hierarchically
   * into suites, with tests of basic features residing at the leaf nodes.
   *
   * \pre Macro \ref TEST_ENABLE is defined.
   * \pre Macro \ref TEST_SILENT is set. It determines whether successful tests
   *   are reported.
   * \param [in] testName The name of a function executing one feature test, or
   *   a test suite consisting of multiple tests.
   * \post If \p test fails, an internal status is updated to indicate at least
   *   one regression test failed.
   */
  #define RUN_TEST(testName) runTest(testName, __func__, #testName)

  /*!
   * \brief Checks a boolean expression and if false, issues an error message
   *   using a format and terminates the current test function.
   *
   * If \p bool_expr evaluates to 0 or \c false, an error message is printed
   * and the currently running function is exited immediately. Consequently,
   * all tests following the one just run in the same function are skipped.
   * This is the behavior expected from a test suite.
   *
   * Use this extension of \ref ASSERT_OR_RETURN if file and line number alone
   * are not sufficient to identify the failing test. It accepts a format
   * string and optional arguments for an error message.
   *
   * \pre Macro \ref TEST_ENABLE is defined.
   * \pre The function expanding this macro returns a \c bool.  With a pointer
   *   return type, \c false converts to \c NULL without a diagnostic, and a
   *   failure may then be reported as success.
   * \param[in] bool_expr an integer expression representing the result of a
   *   test. A value 0 or \c false indicates failure; a non-zero or \c true
   *   value indicates success.
   * \param[in] ... A format string followed by any arguments used to provide
   *   additional information about the failing test. For example, these
   *   arguments can identify individual tests when a test is run repeatedly in
   *   a loop with different input values. The first argument must be a format
   *   string, and \c printf must be able to format each subsequent argument
   *   according to that string.
   * \post On failure, a message consisting of the location of this macro in
   *   the test code, the name of the function from which this macro is called,
   *   and optionally, additional values identifying the particular test is
   *   printed to \c stdout. The function in which this macro is expanded is
   *   exited prematurely with a return value of \c false, indicating failure.
   *   This skips all subsequent tests in that function.
   *   On success, no message is printed, and execution of the remainder of the
   *   current function continues.
   */
  #define ASSERT_OR_RETURN_FMSG(bool_expr, ...)                \
    do {                                                       \
      if (!(bool_expr)) {                                      \
        printf("\n%s:%d: %s: ", __FILE__, __LINE__, __func__); \
        printf(__VA_ARGS__);                                   \
        return false;                                          \
      }                                                        \
    } while(0)

  /*!
   * \brief Checks a Boolean expression and, if false, issues an error message
   *   and terminates the current test function.
   *
   * If \p bool_expr evaluates to 0 or \c false, an error message is printed
   * and the currently running function is exited immediately. Consequently,
   * all tests following the one just run in the same function are skipped.
   * This is the behavior expected from a test suite.
   *
   * On failure, a standard message is printed on \c stdout. It contains the
   * expression \p bool_expr and the location where this macro is expanded in
   * the test code.
   *
   * Use this macro if the file, line, and function in which this macro is
   * embedded are sufficient to identify the failing test. The macro
   * \ref ASSERT_OR_RETURN_FMSG accepts extra arguments if needed for
   * identification.
   *
   * \pre See the preconditions listed in \ref ASSERT_OR_RETURN_FMSG.
   * \param[in] bool_expr represents the result of a test. For details see
   *   \ref ASSERT_OR_RETURN_FMSG.
   * \post On failure, a message consisting of the checked \p bool_expr, the
   *   name of the function from which this macro is called, and the location
   *   of this macro in the test code, thereby identifying the particular test,
   *   is printed to \c stdout.  The current test function is then exited
   *   prematurely with a return value of \c false, indicating failure. This
   *   skips all subsequent tests in that function.
   *   On success, no message is printed, and execution of the remainder of the
   *   current function continues.
   */
  #define ASSERT_OR_RETURN(bool_expr) \
      ASSERT_OR_RETURN_FMSG(bool_expr, "assertion %s failed", #bool_expr)

  /*!
   * \brief Runs all regression tests and then terminates the program.
   *
   * Usually called from \c main(), at the earliest possible point, after only
   * the initializations necessary to run the tests.
   *
   * Called via macro \c RUN_TESTS_AND_EXIT_IF_ENABLED.
   *
   * \pre Macro \ref TEST_ENABLE is defined and \ref TEST_SILENT is set.
   * \post The program terminates with exit status \c EXIT_FAILURE if any
   *   regression test failed, and \c EXIT_SUCCESS otherwise.
   */
  void runTests(void);

  /*!
   * \def RUN_TESTS_AND_EXIT_IF_ENABLED
   *
   * \brief If enabled, runs all regression tests and then exits the program.
   *
   * If the macro \ref TEST_ENABLE is defined, this macro expands to a call to
   * \c runTests which executes all regression tests, and then exits the
   * program. Otherwise, it expands to nothing.
   *
   * The macro is function-like and expects no parameters. In ordinary C code,
   * this macro appears as a function call, imitating the signature of
   * \c runTests. A semicolon must follow this call.
   *
   * \post The program terminates with exit status \c EXIT_FAILURE if any
   *   regression test failed, and \c EXIT_SUCCESS otherwise.
   */
  #define RUN_TESTS_AND_EXIT_IF_ENABLED() runTests()

#else // TEST_ENABLE
  #define RUN_TESTS_AND_EXIT_IF_ENABLED()
#endif // TEST_ENABLE

#endif // METAMATH_MMTEST_H_
