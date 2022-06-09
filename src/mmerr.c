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
 * against accidental overwrites near the buffer boundaries.
 *
 * A frequent memory violation is triggered by an off-by-a small-number index.
 * Adjacent memory blocks could suffer an overwrite then, usually only near their
 * boundaries.  Keeping other memory blocks away by a safety region can guard
 * against accidental overwrite.
 */
#define INIT_SAFETY_OFFSET 200

/*! \def INIT_ELLIPSIS used on startup */
#define INIT_ELLIPSIS "..."

static char NUL = 0;
static char initEllipsis[] = INIT_ELLIPSIS;

/*!
 * set of \ref ErrorPreAllocParams used to pre-allocate data.  Can be modified
 * by \ref setErrorPreAllocParams
 */
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

/*! pointer to the pre-allocated buffer receiving the error message */
static Buffer* buffer = NUL;

/*!
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

/*!
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

/*!
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

/*!
 * A simple parsing scheme allows inserting data in a prepared general message.
 * The scheme is a simple downgrade of the C format string.  Allowed are only
 * placeholders %s that are replaced with given text data.  The percent sign % is
 * available through duplication %%.  For this kind of simple grammar 5 separate
 * process states are sufficient.  Besides the placeholder handling, the end-of-text
 * condition must be recognized - both at the final NUL character, and when the
 * buffer boundary is reached.
 *
 * Note that the complete state needs position  information, too.
 */
enum ParserState {
    /*! copy directly from format parameter */
    TEXT,
    /*! a percent sign was encountered.  This may either be a placeholder
     * or a duplicted % representing a percent proper. */
    PLACEHOLDER_PREFIX,
    /*! copying a parameter replacing a placeholder in the format string. */
    PARAMETER_COPY,
    /*! the buffer end was reached.  Add an ellipsis to indicate truncation. */
    BUFFER_OVERFLOW,
    /*! terminating NUL in format string encountered */
    END_OF_TEXT
}

/*!
 * This structure is used to represent the full state during the placeholder
 * replacement in the format string.
 */
struct ParserState {
    /*! the principal state selecting the proper operation type. */
    ParserState state;
    /*! the next reading position in the format string */
    char const* formatPos;
    /*! the next writing position in the buffer */
    char* buffer;
    /* the list of parameters inserted at placeholders.  This structure allows
     * traversing all entries. */
    va_list args;
    /*! cached location of the buffer end to determine truncation */
    char const* bufferEnd;
}

/*!
 * initialize a \ref ParserState with a startup state.  The \ref buffer is a
 * pre-allocated chunk of memory.
 *
 * \attention the list of parameters \p args is not initialized.
 *
 * \param state structure to be initialized
 * \param format a pointer to a format string with %s placeholders representing
 *   the error message
 * \returns false if either the format string or the \ref buffer is NULL, else true
 *
 * \post all pointers in \p data are setup for starting a parsing loop.
 */ 
static bool initParserState(
    ParserData& state,
    char const* format)
{
    bool result = format != 0 && buffer != 0 && buffer->length > 0;
    if (result)
    {
        state.state = TEXT;
        state.formatPos = format;
        state.buffer = buffer->message;
        state.bufferEnd = state.buffer + buffer->length;
    }
}

/*!
 * assume the last character read from the format string was
 * no placeholder and is simply to be copied as is to the buffer.
 *
 * \pre the next format character is not NUL and there is still
 *   a byte left in the buffer
 * \pre the grammar state is TEXT
 */
static void handleTextState(ParserState& state)
{
    char formatChar = *(state.formatPos++);
    switch (formatChar)
    {
        case '%':
            state.state = PLACEHOLDER_PREFIX;
            break;
        default:
            *(state.buffer++) = formatChar;
            break;
    }
}

/*!
 * assume the last character read from the format string was
 * a % and we now need to check it is a placeholder.
 *
 * \pre the next format character is not NUL and there is still
 *   a byte left in the buffer
 * \pre the grammar state is PLACEHOLDER_PREFIX, a % was
 *   encountered in the format string.
 */
static void handlePlaceholderPrefixState(ParserState& state)
{
    char formatChar = *(state.formatPos++);
    switch (formatChar)
   {
        case '%':
            // two successive % are replaced with a single %
            state.state = TEXT;
            *(state.buffer++) = formatChar;
            break;
        case 's':
            // a %s sequence is recognized as a placeholder.
            state.state = PARAMETER_COPY;
            break;
        default:
            // don't recognize as a placeholder and copy verbatim.
            *(state.buffer++) = '%';
            *(state.buffer++) = formatChar;
            state.state = TEXT;            
            break;
    }
    return result;
}

/*! evaluate the next character in the format string, append the
 * appropriate text in the message buffer and update the state in
 * \p state.
 *
 * \param state holds the parsing state 
 * 
 * \returns false iff the state END_OF_TEXT or
 *  BUFFER_OVERFLOW is reached.
 *
 * \pre \p state is initialized.
 * \post \p state is updated to handle the next character in the
 *   format string
 * \post depending on the format character text is appended to
 *   the message in the buffer.
 */
static bool parseAndCopy(ParserState& state)
{
    bool result = false;
    if (*state.formatPos == NUL)
        state.state = END_OF_TEXT;
    else if (state.buffer != state.bufferEnd)
        // cannot even copy the terminating NUL any more...
        state.state = BUFFER_OVERFLOW;
    else
    {
        result = true;
        switch (state.state)
        {
        TEXT:
            handleTextState(state);
            break;
        PLACEHOLDER_PREFIX:
            handlePlaceholderPrefixState(state);
            break;
        PLACEHOLDER_COPY:
        };
    }
    return result;
}

bool setErrorMessage(char const* format, ...)
{
    ParserState state;
    // everything but the args traversal...
    bool ok = initParserState(state, format);
    if (ok)
    {
        // init args traversal
        va_start(state.args, format);
        while (parseAndCopy(state));
        va_end(state.args);
    }
    return ok;
}
