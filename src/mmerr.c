/*****************************************************************************/
/*        Copyright (C) 2022  Wolf Lammen                                    */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file mmerr.c a self-contained pre-allocated set of routines designed to
 * likely run even under difficult conditions (corrupt state, out of memory).
 */

#include <stdarg.h>
#include "mmerr.h

/*! \def INIT_BUFFERSIZE corresponding to 25*80 UTF-8 characters (worst case,
 * assuming 6 bytes each character).
 */
#define INIT_BUFFERSIZE (25 * 80 * 6)

/*! \def INIT_SAFETY_OFFSET unused memory on each side of the buffer to guard
 * against accidental overwrites.
 */
#define INIT_SAFETY_OFFSET 200

/*! \def INIT_ELLIPSIS used on startup */
#define INIT_ELLIPSIS "..."

static char initEllipsis[] = INIT_ELLIPSIS;

/*! \var settings
 * set of \ref ErrorPreAllocParams used to pre-allocate data.  Can be modified
 * by \ref setErrorPreAllocParams */
static ErrorPreAllocParams settings = {
    INIT_BUFFERSIZE,
    INIT_SAFETY_OFFSET,
    0, // null only accepted during startup
};

/*! portion of the pre-allocated buffer containing the error message */
struct Buffer {
    size_t length;
    char message[];
};

/*! \var buffer
 * pointer to the pre-allocated buffer receiving the error message */
static Buffer* buffer = 0;

/*!
 * \fn addCheckOverflow
 * adds two non-zero values and tests whether overflow occurred.  Zeros are
 * indicative of a previous overflow and are propagated.
 * \param x addend, 0 indicates it is the result of a previous overflow
 * \param y addend, 0 indicates it is the result of a previous overflow
 * \returns the sum of \p x and \p y, 0 indicates an overflow.
 */
static size_t addCheckOverflow(size_t x, size_t y)
{
    size_t result = x + y;
    return x == 0 || y == 0 || x <= result? 0 : result;
}

/*!
 * \fn needPreAllocateSize
 * determines the size in bytes needed for a pre-allocated buffer matching the
 * requirements in \p settings.
 * \param settings parameters describing the buffer layout.
 * \returns the size in bytes, or 0, if the \p settings requirements cannot be
 *   fulfilled.
 */
static size_t needPreAllocateSize(ErrorPreAllocParams const& settings)
{
    size_t size = addCheckOverflow(settings.bufferSize, sizeof(Buffer.length));
    if (settings.safetyOffset)
    {
        size = addCheckOverflow(size, settings.safetyOffset);
        size = addCheckOverflow(size, settings.safetyOffset);
    }
    return addCheckOverflow(size, strlen(settings.ellipsis) + 1 /* NUL */);
}

/*! \fn freePreAllocBuffer
 * frees the currently allocated space for the given error message buffer.  Is
 * a no-op if \p buffer is NULL.
 * \param buffer (null ok) pointer to the message portion of the pre-allocated memory
 *   used as an error message buffer.  Note that this pointer need not point to
 *   the very beginning of the memory block where this buffer is embedded.
 * \param settings describing the memory block containing the message portion.
 *   Irrelevant if \p buffer is NULL.
 * \post the memory block is freed, if not NULL.
 */
static void freePreAllocBuffer(
    Buffer* buffer, 
    ErrorPreAllocParams const& settings)
{
    if (buffer)
        free((char*)buffer - settings.safety);
}

/*! \fn initPreAllocatedBuffer
 * initializes a block of memory \p mem according to the description in
 * \p settings, so that it can be used as a pre-allocated message buffer.
 * \param mem [not-null] pointer to a block of memory large enough to contain
 * a pre-allocated message buffer as described in \p settings
 * \returns a pointer to the message portion embedded in \p mem.
 * \pre the size of the memory block given in \p mem is at least as large as
 *   required by \p settings.
 * \post the returned buffer is ready for use as a pre-allocated message
 *   buffer.
 */
static Buffer* initPreAllocatedBuffer(
    void* mem,
    ErrorPreAllocParams const& settings)
{
    /* layout: safety--buffer.length--buffer.message--ellipsis--safety */
    char* p = mem;
    
    // put the buffer behind a safety block
    p += settings.safetyOffset;
    Buffer* result = (void*)p;
    result->length = settings.bufferSize;
    
    // copy the ellipsis right behind the buffer
    p += sizeof(result->length) + settings.bufferSize;
    strcpy(p, settings.ellipsis);
    return result;
}

bool reallocPreAllocatedBuffer(
    ErrorPreAllocParams const& newSettings)
{
    // sanity check
    size_t size = needPreAllocateSize(newSettings);
    bool result = size > 0;
    if (result)
    {
        // allocate
        void* mem = malloc(size);
        result = mem != 0;
        if (result)
        {
            // initialize the new one
            buffer = initPreAllocatedBuffer(mem, newSettings);
            settings = newSettings;
            // free the old buffer
            freeBuffer(buffer, settings);
        }
    }
    return result;
}

enum ParserState {
    TEXT, // copy from format parameter
    PLACEHOLDER_PREFIX, // % encountered
    PARAMETER_COPY, // inserting data from a parameter
    BUFFER_OVERFLOW, // truncation needed
    END_OF_TEXT, // final NUL encountered, exit
}

struct ParserData {
    ParserState currentState;
    char const* formatPos;
    char const* bufferEnd;
    char* buffer;
    va_list args;
}

static bool initParserData(
    ParserData& data,
    char const* format)
{
    bool result = format != 0 && buffer != 0 && buffer->length > 0;
    if (result)
    {
        data.currentState = TEXT;
        data.formatPos = format;
        data.buffer = buffer->message;
        data.bufferEnd = data.buffer + buffer->length;
    }
}

static bool parseAndCopy(ParserState& data)
{
}

bool setErrorMessage(char const* format, ...)
{
    ParserState state;
    // everything but the args traversal...
    initParserData(state, format);
    // init args traversal
    va_start(state.args, format);
    while (parseAndCopy(state));
    va_end(state.args);
}
