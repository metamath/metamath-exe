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
 * behavior is limited.
 *
 * A corrupt state is often caused by limit violations overwriting adjacent
 * memory.  To specifically guard against this, the pre-allocated memory area,
 * at your option, may include safety borders detaching necessary pre-set
 * administrative data from the rest.
 *
 * Often it is sensible to embed details in a diagnosis message.  Placeholders
 * in the format string mark insertion points for such values, much as in
 * \p printf. The variety and functionality is greatly reduced in our case,
 * though.  Only pieces of text or unsigned integers can be embedded
 * (%s or %u placeholder).
 *
 * For this kind of expansion you still need a buffer where the final message is
 * constructed.  In our context, this buffer is pre-allocated, and fixed in size,
 * truncation of overflowing text enforced.
 *
 * Documentation
 * =============
 * 
 * Most comments are written in <a href="https://doxygen.nl/index.html">
 * doxygen</a> (Qt variant) style.  If you have doxygen installed on your
 * computer, you may generate HTML documentation out of them with its root
 * placed in build/html/index.html by running build.sh with the -d option.
 *
 * Regression tests
 * ================
 *
 * If the macro **REGRESSION_TEST** is defined (option -t of build.sh) or
 * **TEST_MMFATL** is defined, then regression tests are implemented.  Invoke
 * in addition option -c on build.sh, should you switch between with/out
 * testing, but no intermediate source file change.
 *
 * In order to run implemented regression tests properly we suggest to add a
 * \code{.c}
 * mmfatl_test();
 * \endcode
 * line to main() close to its begin and **before** any function declared in
 * this header file is called.
 *
 * If tests are disabled this line evaluates to nothing.  In addition, the
 * compiler skips any test code, and the artifact size will not grow.  All in
 * all, a disabled test suite does not come with a linking or runtime penalty.
 *
 * If enabled, running tests document its progress to stdout.  The test stops
 * on the first regression found with a diagnostic message further detailing on
 * the context of the failure.  The tests, if called as described above, do not
 * interfere with Metamath program behaviour, even not when detecting a
 * regression, in any way other than generating extra output on program start.
 *
 * We recommend running the tests each time you modify mmfatl.h or mmfatl.c
 * to ensure it still executes as desired.
 */

#include <stddef.h>

/* REGRESSION_TEST overrides the setting of TEST_MMFATL */
#ifdef REGRESSION_TEST
#   define TEST_MMFATL
#endif

/* uncomment one of the following to enforce disabling/enabling of regression
 * tests unconditionally
 */
// #undef TEST_MMFATL
// #define TEST_MMFATL

#ifdef TEST_MMFATL
    /* regression tests are implemented and called by this function */
    extern void mmfatl_test(void);
#else
    /* an empty macro deletes any call to the regression test suite at
     * compile time */
#   define mmfatl_test(x)
#endif


/*---------------------   Allocation Of An Error Buffer   ---------------------*/
/*!
 * Basic parameters controlling pre-allocation of basic routines and data.
 * Pre-allocation helps to construct a diagnosis message, even in a
 * corrupt environment.
 * 
 * A pre-allocated buffer is provided to allow embedding data in a diagnosis
 * message.
 */
struct FatalErrorBufferDescriptor {
    /*! size of pre-allocated buffer, excluding the space needed for
     * \p ellipsis and any dmz space.  Sensible values range between the size
     * of a single line (around 80 characters) up to a few KBytes.  If you want
     * to support UTF-8 then each character can consume up to 6 bytes.  The
     * size should be accomodated accordingly.
     * Messages up to this size (excluding the terminating NUL character) can
     * be displayed unabbreviated.
     */
    size_t size;
    /*!
     * Pre-allocated data (in particular administrative data describing the
     * buffer size and the like) can to some extent be secured against
     * accidental overwrites by embedding it in a frame of allocated, but
     * unused memory.  We call this zone a DMZ, or de-militarized zone, a word
     * coined for such safety concepts.  The bigger the size the better the
     * extra security, since (1) the used size reduces in relation to all
     * allocated memory, and (2) range violations often trepass on memory close
     * to regular accesses only.  The value given here describes the extra
     * bytes on one side only.  If you think this is a paranoid idea, set this
     * value to zero.
     */
    size_t dmz;
    /*! not-null, NUL-terminated trailing character sequence indicating the
     * error message was truncated due to buffer limitations.  The submitted
     * text is copied internally so no reference to its memory is generated.
     * It can safely be deallocated again after a call to
     * \ref allocFatalErrorBuffer.
     */
    char const* ellipsis;
};

/*!
 * get the \ref FatalErrorBufferDescriptor used to allocate the current memory.
 * \returns a non-null pointer to the current descriptor intended for reading only.
 * \attention the pointer to the ellipsis must be used for immediate reading
 *   only.  It is not stable and the referenced memory may change after a
 *   reallocation.
 */
struct FatalErrorBufferDescriptor const* getFatalErrorBufferDescriptor();

/*!
 * get the current contents of the fatal error buffer, or NULL if no buffer is assigned.
 * \returns the current contents as a NUL terminated string.
 */
char const* getFatalErrorMessage();

/*!
 * frees any previously allocated buffer and installs a new one matching
 * the submitted given requirements.
 *
 * This may fail, because
 *  - there is not enough memory available for the new buffer size including
 *    two safety offsets.
 *  - if bufferSize is 0, or ellipsis in \p descriptor is NULL
 * \param descriptor [not null] the requirements of a new pre-allocated message
 *   buffer, replacing the current one, if exists.
 * \returns whether the (re-)allocation was successful (1), or not (0).
 * \post Any previously allocated error buffer is returned
 * \post the memory of the ellipsis pointed to in \p descriptor is not needed
 *   after the call.
 * \post the descriptor describes any (re-)allocated buffer, or is invalid on failure. 
 */
int allocFatalErrorBuffer(struct FatalErrorBufferDescriptor const* descriptor);

/*!
 * call this only during a normal program exit.  We don't need a clean exit
 * state in case of a fatal error.  Some debugging tools like Valgrind complain
 * when they detect allocated memory not freed again.
 * \post any pre-allocated memory for the error buffer is returned to the system.
 * \post the buffer descriptor is reset to invalid 
 */
void freeFatalErrorBuffer();


/*----------------   Filling the buffer with an error message   -------------*/

/*!
 * Allows only %s and %u placeholders.  %s for embedded strings, %u for
 * embedded unsigned integers.
 */
typedef char const* FatalErrorFormat;

/*! fill the internal buffer with submitted data, expanding placeholders
 * if any.
 * \param format a message with embedded placeholders, see \ref ErrorFormat,
 *   followed by a list of string values to be inserted at placeholders in the given order.
 * \returns 0, if not even a truncated message could be created, 1 else.
 */
int setFatalErrorMessage(FatalErrorFormat format, ...);

/*!
 * creates an error message, prints it to stderr and exits with exit code 1.
 * \param line the program line where the fatal error occurred.  This value is
 *   conveniently created by the __LINE__ macro.  Set to 0, if not available.
 * \param file [null] the program file containing the faulting program line.
 *   This value is conveniently created by the __FILE__ macro.  Set to NULL,
 *   if not available
 * \param messageFormat [not null] the message to be displayed with embedded
 *   format specifiers to be expanded by the following parameters.
 * \returns never
 */
void exitOnFatalError(
    unsigned line,
    char const* file,
    FatalErrorFormat messageFormat, ...
);

#endif /* include guard */
