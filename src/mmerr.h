/*****************************************************************************/
/*        Copyright (C) 2022  Wolf Lammen                                        */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMERR_H_
#define METAMATH_MMERR_H_

#include <stdio.h>

/*! \page pgError "Simple Error Messaging"
 * When a fatal error occurs, the internal structures of a program are usually 
 * corrupted.  It is therfore not safe to assume resources are still available.
 * Instead the program exits immediately after displaying some diagnostic
 * message.
 *
 * In order to display this final message, the program is restricted to very
 * basic self-contained routines, not dependent on any state of the program.
 * In particular everything should be pre-allocated and initialized, so the
 * chance of a failure is minimized, even if the program state is corrupted.
 * This goes at the expense of flexibility, in particular, support for dynamic
 * behavior is limited.  For example, the whole diagnostic message is fitted
 * into a buffer of a given size, truncation enforced.  Internal structures used
 * are isolated from the rest of the program.  The possible formats with
 * placeholders are restricted to those of %s type.
 */

/*! \struct ErrorPreAllocParams
 * Basic parameters controlling pre-allocation of basic routines and data.
 */
struct ErrorPreAllocParams {
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

/*! \fn getIError
 * get the \ref ErrorPreAllocParams used to allocate the current data. */
ErrorPreAllocParams const& getErrorPreAllocParams();
/*! set the limitations of the internal buffer.
 * This may fail, becausebool setErrorPreAllocParams(ErrorPreAllocParams const& settings
 *  - there is not enough memory available for the new buffer size including
 *    two safety offsets.
 *  - if one of the pointers in \p settings is NULL
 *  - if the size of the default Message exceeds the buffer size
 *  - if the ellipsis or default message is not a UTF-8 (superset of ASCII)
 *    encoded text.
 * \return true, if the new settings are installed.
 */
);

/*! \fn reallocPreAllocatedBuffer
 * frees any currently in-use pre-allocated buffer and installs a new one
 * matching given requirements.
 * \param newSettings the requirements of a new pre-allocated message buffer,
 *   replacing the current one, if exists.
 * \returns whether the reallocation was successful.
 * \post If no new buffer could be allocated, the old one stays in place.
 */
bool reallocPreAllocatedBuffer(
    ErrorPreAllocParams const& newSettings);

/*! \typedef ErrorFormat
 * Allows only %s as placeholders
 */
typedef ErrorFormat char const*;

/*! fill the internal buffer with submitted data, expanding placeholders
 * if available.
 * \ref ErrorFormat.
 */
bool setErrorMessage(char const* format, ...);

void raiseFatalError(
    char const* message,
    char const* line,
    char const* file
);

#endif /* include guard */
