/*****************************************************************************/
/*        Copyright (C) 2022  Wolf Lammen                                        */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMERR_H_
#define METAMATH_MMERR_H_

#include <stdio.h>

/*! \page pgError "Simple Error Messaging"
 * When a fatal error occurs, the internal structures of a program may be 
 * corrupted to the point that recovery is impossible.  The program exits
 * immediately, but hopefully still displaying a diagnostic message.
 *
 * To display this final message, we restrict its code to very basic,
 * self-contained routines, independent on the rest of the program to the
 * extent possible, thus dodging corrupted data.
 *
 * In particular everything should be pre-allocated and initialized, so the
 * chance of a failure in a corrupted environment is minimized.  This is to the
 * detriment of flexibility, in particular, support for dynamic behavior is
 * limited.
 *
 * Often it is sensible to embed details in a diagnosis message.  Placeholders
 * in the format string mark insertion points for such values, much as in
 * \p printf. The variety and functionality is greatly reduced in our case,
 * though.  Only pieces of text can be embedded (%s placeholder).
 * 
 * Still, for this kind of expansion you need a buffer where the final message is
 * constructed.  In our context, this buffer is pre-allocated, and fixed in size,
 * truncation enforced.
 */

/*!
 * Basic parameters controlling pre-allocation of basic routines and data.
 * Pre-allocation helps to construct a diagnosis message, even in a
 * corrupt environment.
 * 
 * A pre-allocated buffer is provided to allow embedding data in a diagnosis
 * message.  
 */
struct ErrorPreAllocatedParams {
    /*! size of pre-allocated buffer, excluding the space needed for \p ellipsis.
     * Sensible values range between the size of a single line (around 80
     * characters) up to a few KBytes.  If you want to support UTF-8 then each
     * character can consume up to 6 bytes.  The buffer size should be
     * accomodated accordingly.
     */
    size_t bufferSize;
    /*!
     * Pre-allocated data can to some extent be secured against accidental
     * overwrites by embedding it in a frame of allocated, but unused memory.
     * The bigger the size the better the extra security, since 1. the target
     * size reduces in relation to all allocated memory, and 2. range
     * violations often trepass on memory close to regular accesses only.  The
     * value given here describes the extra bytes on one side only.  If you
     * think this is a paranoid idea, set this value to zero.
     */
    size_t safetyOffset;
    /*! not-null, NUL-terminated trailing character sequence indicating the
     * error message was truncated due to buffer limitations.  The submitted
     * text is copied internally so no extra reference to its memory is
     * generated.
´     */
    char const* ellipsis;
};

/*!
 * get the \ref ErrorPreAllocatedParams used to allocate the current data.
 * \returns a non-null pointer to the current settings intended for reading only.
 */
struct ErrorPreAllocatedParams const* getErrorPreAllocatedParams();
/*!
 * frees any currently in-use pre-allocated buffer and installs a new one
 * matching given requirements.
 * This may fail, because
 *  - there is not enough memory available for the new buffer size including
 *    two safety offsets.
 *  - if one of the pointers in \p settings is NULL
 * \param newSettings the requirements of a new pre-allocated message buffer,
 *   replacing the current one, if exists.
 * \returns whether the reallocation was successful (1).
 * \post If no new buffer could be allocated, the old one stays in place.
 */
int setErrorPreAllocatedParams(struct ErrorPreAllocatedParams const* settings);

/*!
 * Allows only %s as placeholders
 */
typedef char const* ErrorFormat;

/*! fill the internal buffer with submitted data, expanding placeholders
 * if available.
 * \param format a message with embedded %s placeholders, see \ref ErrorFormat
 *   followed by a list of string values to be inserted at placeholders in the given order.
 * \returns 0, if not even a truncated message could be created.
 */
int setErrorMessage(ErrorFormat format, ...);

void raiseFatalError(
    char const* message,
    char const* line,
    char const* file
);

#endif /* include guard */
