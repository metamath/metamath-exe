/*****************************************************************************/
/*        Copyright (C) 2022  Wolf Lammen                                    */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file mmfatl.c a self-contained set of text printing routines using dedicated
 * pre-allocated memory, designed to likely run even under difficult conditions
 * (corrupt state, out of memory).
 */

#include "mmfatl.h"
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

/*!
 * C has no boolean type, instead an int is used.  To ease code reading we
 * encode one boolean value here.
 * int cond = FALSE; if (cond) { } skips the code block.
 */
#define FALSE 0
/*!
 * C has no boolean type, instead an int is used.  To ease code reading we
 * encode one boolean value here.
 * int cond = TRUE; if (cond) { } branches into the code block.
 */
#define TRUE 1

/*! terminating character of a text string never used elsewhere */ 
#define NUL '\x00'

/*!
 * utility function:
 * adds two non-zero values and tests for overflow.  Zeros are indicative of a
 * previous overflow and are propagated.
 * \param x addend, 0 indicates it is the result of a previous overflow
 * \param y addend, 0 indicates it is the result of a previous overflow
 * \returns the sum of \p x and \p y, 0 indicates an overflow.
 */
static size_t addCheckOverflow(size_t x, size_t y)
{
    size_t result = x + y;
    return x == 0 || result <= x? 0 : result;
}

//=============   Allocation of Memory for Fatal Error Messages   ===========

/*!
 * a \ref FatalErrorBufferDescriptor describing a pre-allocated
 * buffer for fatal error messages.  The initial settings indicate the absence
 * of such a buffer, which is allowed during program startup only.  Should be
 * modified by \ref allocFatalErrorBuffer
 */
static struct FatalErrorBufferDescriptor descriptor = {
    0,  // capacity
    0,  // dmz
    NULL, // no ellipsis.  Null only accepted during startup
};

/*!
 * pre-allocated memory block used for fatal error messages.  The size of the
 * block is derived from a \ref FatalErrorBufferDescriptor.  Only a single
 * global instance ismaintained.  All values assigned to \p memBlock must be a
 * result of a \p malloc call.
 * \invariant \ref descriptor describes the memory layout correctly if this
 *   pointer is different from NULL.
 */
static void* memBlock = NULL;

struct FatalErrorBufferDescriptor const* getFatalErrorBufferDescriptor()
{
    return &descriptor;
}

/*!
 * does the submitted \ref FatalErrorBufferDescriptor fulfil minimum
 * requirements for a valid pre-allocation?
 * \attention an affirmative result is not the final answer.  Allocation
 *   requires also information about available memory.
 * \param aDescriptor [null] a pointer to a \ref FatalErrorBufferDescriptor
 *   under investigation.
 * \returns 0 if not acceptable, 1 else.
 */
static int isValidFatalErrorBufferDescriptor(
    struct FatalErrorBufferDescriptor const* aDescriptor)
{
    return aDescriptor != NULL 
        && aDescriptor->capacity > 0
        && aDescriptor->ellipsis != NULL? TRUE : FALSE;
}

/*!
 * determine the size of a memory block requested by a
 * \ref FatalErrorBufferDescriptor.
 * \param aDescriptor [null] a pointer to a \ref FatalErrorBufferDescriptor
 *   under investigation.
 * \returns 0, if the descriptor is invalid, or the size calculation suffers from
 *   overflow, 1 else.
 */
static size_t evalRequestedMemSize(
    struct FatalErrorBufferDescriptor const* aDescriptor)
{
    size_t result = 0;
    if (isValidFatalErrorBufferDescriptor(aDescriptor))
    {
        result = aDescriptor->capacity;

        size_t dmz = aDescriptor->dmz;
        if (dmz > 0)
        {
            result = addCheckOverflow(result, dmz);
            result = addCheckOverflow(result, dmz);            
        }

        result = addCheckOverflow(
            result, strlen(aDescriptor->ellipsis) + 1 /* NUL */);        
    }
    return result;
}

/*!
 * returns a pointer to the first char of the memory where fatal error
 * messages are kept, or built up.  The result is independent of any contents
 * of the buffer.
 * \return a pointer to the first character.
 * \pre \ref descriptor is set up and \ref memBlock not NULL.
 */
static char* fatalErrorBufferBegin()
{
    return (char*)memBlock + descriptor.dmz;
}

/*!
 * returns a pointer to the first char past the memory where fatal error
 * messages are kept, or built up.  The result is independent of any contents
 * of the buffer.
 * \return a pointer to the first character behind the buffer, where the
 *   ellipsis begins.
 * \pre \ref descriptor is set up and \ref memBlock not NULL.
 */
static char* fatalErrorBufferEnd()
{
    return fatalErrorBufferBegin() + descriptor.capacity;
}

/*!
 * reset the current contents of the fatal error buffer to the empty string.
 * \pre \ref descriptor is set up and \ref memBlock not NULL.
 * \post the contents of the buffer read as a NUL terminated string is
 *   the empty string.
 */
static void clearFatalErrorBuffer()
{
    *fatalErrorBufferBegin() = NUL;
}

/*!
 * initializes the buffer used for fatal error messages with a NUL terminated
 * empty string.  Copies the ellipsis in \ref descriptor to the end of the
 * buffer, so it is automatically appended in case of a message truncation.
 * \pre \ref descriptor is set up and \ref memBlock not NULL.
 * \post The ellipsis in \ref descriptor is copied to the \ref memBlock, and
 *   the \ref descriptor is updated accordingly.  So no reference to a user
 *   space ellipsis is kept in this code any more.
 */
static void initFatalErrorBuffer()
{
    clearFatalErrorBuffer();
    char* bufferEnd = fatalErrorBufferEnd();
    strcpy(bufferEnd, descriptor.ellipsis);
    // This decouples from any user space supplied memory, that can safely be
    // deallocated any time now
    descriptor.ellipsis = bufferEnd;
}

int allocFatalErrorBuffer(struct FatalErrorBufferDescriptor const* aDescriptor)
{
    int result = FALSE;
    size_t memSize = evalRequestedMemSize(aDescriptor);
    if (memSize > 0)
    {
        void* newBlock = malloc(memSize);
        if (newBlock)
        {
            free(memBlock);
            memBlock = newBlock;
            // the assignment of the ellipsis is temporary only.
            descriptor = *aDescriptor;
            initFatalErrorBuffer();
            result = TRUE;
        }
    }
    return  result;
}

void freeFatalErrorBuffer()
{
    if (memBlock)
    {
        free(memBlock);
        descriptor.capacity = 0;
        descriptor.dmz = 0;
        descriptor.ellipsis = NULL;
    }
}

char const* getFatalErrorMessage()
{
    return memBlock? fatalErrorBufferBegin() : NULL;
}

//===================    Parsing the Format String   =====================

#define PLACEHOLDER_PREFIX_CHAR '%'
#define PLACEHOLDER_TYPE_STRING 's'
#define PLACEHOLDER_TYPE_UNSIGNED 'u'

/*!
 * converts an unsigned int to a sequence of decimal digits representing its value
 * \param value an unsigned value that is to be converted to string of decimal
 *   digits.
 * \returns a pointer to a string converted from \p value.  Note that this
 *   result is stable only until the next call to this function.
 */
static char const* unsignedToString(unsigned value)
{
    // each byte of an unsigned covers log(256) < 2.5 decimal digits.  Add 1 to
    // round up, and 1 for the terminating NUL
    static char digits[(5 * sizeof(unsigned)) / 2 + 2];

    digits[0] = '0';
    int ofs = value == 0? 1 : 0;
    while (value)
    {
        digits[ofs++] = (value % 10) + '0';
        value /= 10;
    }
    digits[ofs] = NUL;
    return digits;
}

/*!
 * A simple grammar scheme allows inserting data in a prepared general message.
 * The scheme is a simple downgrade of the C format string.  Allowed are only
 * placeholders %s and %u that are replaced with given data.  The percent sign
 * % is available through duplication %%.  For this kind of simple grammar 5
 * separate process states are sufficient.  Besides the placeholder handling,
 * the end-of-text condition must be recognized - both at the final NUL
 * character, and when the buffer limits are exceeded.
 *
 * Note that the complete state needs position information, too.
 */
enum ParserProcessState {
    /*! copy directly from format parameter */
    TEXT,
    /*! a percent sign was encountered.  This may either be a placeholder
     * or a duplicted % representing a percent proper. */
    CHECK_PLACEHOLDER,
    /*! copying a parameter replacing a placeholder in the format string. */
    PARAMETER_COPY,
    /*! the buffer end was reached.  Add an ellipsis to indicate truncation. */
    BUFFER_OVERFLOW,
    /*! terminating NUL in format string encountered */
    END_OF_TEXT
};

/*!
 * This structure is used to represent the full state during the placeholder
 * replacement in the format string.  The current position is tracked as well.
 * Parsing depends on an available fatal error message buffer in \ref memBlock
 * and its descriptor \ref descriptor.
 */
struct ParserState {
    /*! the principal state selecting the proper operation type. */
    enum ParserProcessState processState;
    /*! the next reading position in the format string */
    char const* formatPos;
    /*! the next writing position in the buffer */
    char* buffer;
    /*! cached buffer end to determine truncation */
    char const* bufferEnd;
    /* the list of parameters inserted at placeholders.  This structure allows
     * traversing all entries. */
    va_list args;
    /*! holds the current parameter in \p args for a %s placeholder */
    char const* arg;
};

/*!
 * initialize a \ref ParserState with a startup state.  The \ref memBlock is a
 * pre-allocated chunk of memory.
 *
 * \attention the list of parameters \p args is not initialized.
 *
 * \param state [not null] structure to be initialized
 * \param format a pointer to a format string with %s, %u  placeholders
 *   representing the error message.
 * \returns 0 if either the format string or the \ref memBlock is NULL, else 1
 *
 * \post On success, all pointers in \p state are setup for starting a parsing loop, except the
 *   list of parameters (field args).
 */ 
static int resetParserState(
    struct ParserState* state,
    FatalErrorFormat format)
{
    int result = format != NULL && memBlock != NULL? TRUE : FALSE;
    if (result)
    {
        state->processState = TEXT;
        state->formatPos = format;
        state->buffer = fatalErrorBufferBegin();
        state->bufferEnd = fatalErrorBufferEnd();
        state->arg = NULL;
    }
    return result;
}

static int isBufferFull(struct ParserState* state)
{
    return state->buffer >= state->bufferEnd;
}

/*!
 * assume the last character read from the format string was
 * no placeholder and is simply to be copied as is to the buffer.
 *
 * \param state not null, holds the parsing state
 *
 * \pre the next format character is not NUL and there is still
 *   a byte left in the buffer
 * \pre the grammar state is TEXT
 */
static void handleTextState(struct ParserState* state)
{
    char formatChar = *(state->formatPos++);
    switch (formatChar)
    {
        case PLACEHOLDER_PREFIX_CHAR:
            // skip the % in output
            state->processState = CHECK_PLACEHOLDER;
            break;
        default:
            // keep the TEXT state
            *(state->buffer++) = formatChar;
    }
}

/*!
 * assume the last character read from the format string was a % and we now
 * need to check it introduced a placeholder.  If not, the % only is ignored 
 * (not messing with UTF-8).  In particular, a sequence %% is reduced to a
 * single %.
 *
 * A NULL parameter is allowed, and has the same effect as an empty string.
 *
 * \param state not null, holds the parsing state
 *
 * \pre the next format character is not NUL and there is still
 *   a byte left in the buffer
 * \pre the grammar state in state.state is PLACEHOLDER_PREFIX, so a % was
 *   previously encountered in the format string.
 */
static void handlePlaceholderPrefixState(struct ParserState* state)
{
    char formatChar = *(state->formatPos++);
    switch (formatChar)
   {
        case PLACEHOLDER_TYPE_STRING:
            // a %s sequence is recognized as a placeholder.  Don't copy
            // anything, if the parameter is NULL.
            state->arg = va_arg(state->args, char const*);
            state->processState = state->arg == NULL? TEXT : PARAMETER_COPY;
            break;
        case PLACEHOLDER_TYPE_UNSIGNED:
            state->arg = unsignedToString(va_arg(state->args, unsigned));
            state->processState = PARAMETER_COPY;
            break;
        default:
            // ignore the leading %, but copy the following character
            // to the buffer.  
            *(state->buffer++) = formatChar;
            state->processState = TEXT;
    }
}

/*!
 * assume we encountered a placeholder %s or %u, and copy now from
 * the parameter, given as astring.  It is always copied verbatim, no
 * recursive search for placeholders takes place. The terminating NUL is not
 * copied.
 *
 * \param state not null, holds the parsing state
 *
 * \pre the next format character is not NUL and there is still
 *   a byte left in the buffer
 * \pre state->arg contains a pointer to the parameter to insert verbatim.
 * \pre the grammar state in state->state is PARAMETER_COPY, after a %s was
 *   encountered in the format string.
 */
static void handleParameterCopyState(struct ParserState* state)
{
    char paramChar = *(state->arg++);
    switch (paramChar)
    {
        case NUL:
            state->arg = NULL;
            state->processState = TEXT;
            break;
        default:
            *(state->buffer++) = paramChar;
    }
}

/*! evaluate the next character in the format string, append the appropriate
 * text in the message buffer and update the state in \p state.
 * \param state holds the parsing state
 * \returns 0: message completed (possibly after a truncation), 1 else
 * \pre \p state is initialized.
 * \post \p state is updated to handle the next character in the
 *   format string
 * \post depending on the format character, text is appended to
 *   the message in the buffer.
 */
static int parseAndCopy(struct ParserState* state)
{
    int result = FALSE;
    if (state->processState != PARAMETER_COPY && *state->formatPos == NUL)
        state->processState = END_OF_TEXT;
    else if (isBufferFull(state))
        // cannot even copy the terminating NUL any more...
        state->processState = BUFFER_OVERFLOW;
    else
    {
        result = TRUE;
        switch (state->processState)
        {
        case TEXT:
            handleTextState(state);
            break;
        case CHECK_PLACEHOLDER:
            handlePlaceholderPrefixState(state);
            break;
        case PARAMETER_COPY:
            handleParameterCopyState(state);
            break;
        default:;
        };
    }
    return result;
}

/*!
 * evaluate a message according to the information in \p state, and append it
 * to \ref memBlock. Truncate if the destination has not enough memory available.
 * \param state [ParserState, not null] an already initialized
 *   \ref ParserState containing parameters, a destination memory block, and a
 *   format string to interpret.
 * \pre \p state is initialized.
 * \post \p state is consumed.
 */
static void appendMessage(struct ParserState* state)
{
    while (parseAndCopy(state));
}

int setFatalErrorMessage(FatalErrorFormat format, ...)
{
    struct ParserState state;
    int ok = resetParserState(&state, format);
    clearFatalErrorBuffer();
    if (ok)
    {
        va_start(state.args, format);
        appendMessage(&state); 
        va_end(state.args);
    }
    return ok;
}

/*!
 * empties the error buffer and fills it with location information.
 * If not successful, the buffer remains in an empty state.
 * \param line program line causing the error, or 0 if not available.
 * \param file file containing the faulting line, or NULL if not available.
 * \returns a pointer to the end of the buffer contents.
 */
static char* setLocationData(unsigned line, char const* file)
{
    char* result = fatalErrorBufferBegin();
    int hasPosInfo = line != 0 || (file != NULL && *file != NUL);
    if (hasPosInfo)
    {
        char const* posFormat = line == 0? "%s:\n" : "%s@%u:\n";
        hasPosInfo = setFatalErrorMessage(posFormat, file, line);
    }
    if (hasPosInfo)
        result += strlen(result);
    else
       clearFatalErrorBuffer();
    
    return result;
}

void exitOnFatalError(
    unsigned line,
    char const* file,
    FatalErrorFormat messageFormat, ...)
{
    struct ParserState state;
    resetParserState(&state, messageFormat);

    // marks the current end position in the buffer
    state.buffer = setLocationData(line, file);

    // now process the message
    va_start(state.args, messageFormat);
    appendMessage(&state); 
    va_end(state.args);
    exit(1);
}
