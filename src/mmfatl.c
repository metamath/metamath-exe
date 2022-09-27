/*****************************************************************************/
/*        Copyright (C) 2022  Wolf Lammen                                    */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file mmfatl.c a self-contained set of text printing routines using
 * dedicated pre-allocated memory, designed to likely run even under difficult
 * conditions (corrupt state, out of memory).
 */

#include "mmfatl.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef TEST_MMFATL
    // We intercept malloc/free to test proper
    // freeing after allocation of memory
    static void* MALLOC(size_t size);
    static void FREE(void* ptr);
#else
#   define FREE free
#   define MALLOC malloc
#endif

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

/*!
 * utility function:
 * In case of truncation due to a full buffer the last UTF-8 multi-byte
 * sequence may not be transferred completely.  Trace back to the last
 * character of a completely transferred sequence.  ASCII encoding is a single byte
 * subset encoding of UTF-8 that always yields the last transferred byte.
 * \param begin [not null] a pointer to the buffer start.
 * \param last [not null] a pointer to the last transferred byte in the buffer.
 * \returns a pointer to the last valid byte, or NULL if it does not exist.
 *   The latter can happen only if either your buffer is less than 4 bytes
 *   long, or if it contains invalid UTF-8 encoding.
 */

static char* findLastUTF8Byte(char const* begin, char* end)
{
    if (*end >= 0) // ASCII character
        return end;
    char* pt = end + 1;
    // skip backwards all UTF-8 continuation bytes
    while ((*--pt & 0xC0) == 0x80 && pt != begin);
    // should be a leading byte here, otherwise invalid UTF-8
    unsigned char leadByte = *pt;
    if ((leadByte & 0xC0) <= 0x80)
        return NULL;
    // number of stored continuaton bytes
    int buffered = end - pt;
    if (buffered > 3)
        return NULL;
    // up to 3 continuation bytes possible
    int expected = leadByte >= 0xF0? 3 : (leadByte >= 0xE0? 2 : 1);
    return buffered == expected? end : (pt == begin? NULL : pt - 1);
}

/*=============   Allocation of Memory for Fatal Error Messages   ===========*/

/*!
 * a \ref FatalErrorBufferDescriptor describing a pre-allocated
 * buffer for fatal error messages.  The initial settings indicate the absence
 * of such a buffer, which is allowed during program startup only.  Should be
 * modified by \ref allocFatalErrorBuffer
 */
static struct FatalErrorBufferDescriptor descriptor = {
    0,  /* size */
    0,  /* dmz */
    NULL, /* no ellipsis.  Null only accepted during startup */
};

/*!
 * pre-allocated memory block used for fatal error messages.  The size of the
 * block is derived from a \ref FatalErrorBufferDescriptor.  Only a single
 * global instance is maintained.  All values assigned to \p memBlock must be a
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
 * \returns 1 if acceptable, 0 else.
 */
static int isValidFatalErrorBufferDescriptor(
    struct FatalErrorBufferDescriptor const* aDescriptor)
{
    return aDescriptor != NULL
        && aDescriptor->size > 0
        && aDescriptor->ellipsis != NULL;
}

/*!
 * determine the size of a memory block requested by a
 * \ref FatalErrorBufferDescriptor.
 * \param aDescriptor [null] a pointer to a \ref FatalErrorBufferDescriptor
 *   under investigation.
 * \returns 0, if the descriptor is invalid, or the size calculation suffers
 *   from overflow, 1 else.
 */
static size_t evalRequestedMemSize(
    struct FatalErrorBufferDescriptor const* aDescriptor)
{
    size_t result = 0;
    if (isValidFatalErrorBufferDescriptor(aDescriptor))
    {
        result = aDescriptor->size;

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
    return fatalErrorBufferBegin() + descriptor.size;
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
    /* This decouples from any user space memory, that can safely be changed or
     * deallocated any time now */
    descriptor.ellipsis = strcpy(fatalErrorBufferEnd(), descriptor.ellipsis);
}

void freeFatalErrorBuffer()
{
    FREE(memBlock);
    memBlock = NULL;
    descriptor.size = 0;
    descriptor.dmz = 0;
    descriptor.ellipsis = NULL;
}

bool allocFatalErrorBuffer(struct FatalErrorBufferDescriptor const* aDescriptor)
{
    bool result = false;
    freeFatalErrorBuffer();
    size_t memSize = isValidFatalErrorBufferDescriptor(aDescriptor)?
            evalRequestedMemSize(aDescriptor) : 0;
    if (memSize > 0)
    {
        memBlock = MALLOC(memSize);
        if (memBlock)
        {
            /* the assignment of the ellipsis is temporary only. */
            descriptor = *aDescriptor;
            initFatalErrorBuffer();
            result = true;
        }
    }
    return result;
}

char const* getFatalErrorMessage()
{
    return memBlock? fatalErrorBufferBegin() : NULL;
}

/*===================    Parsing the Format String   =====================*/

#define PLACEHOLDER_PREFIX_CHAR '%'
#define PLACEHOLDER_CHAR_STRING 's'
#define PLACEHOLDER_CHAR_UNSIGNED 'u'

/*!
 * converts an unsigned int to a sequence of decimal digits representing its
 * value.
 * \param value an unsigned value that is to be converted to string of decimal
 *   digits.
 * \returns a pointer to a string converted from \p value.
 * \attention  The result is stable only until the next call to this function.
 */
static char const* unsignedToString(unsigned value)
{
    /* each byte of an unsigned covers log(256) < 2.5 decimal digits.  Add 1 to
     * round up, and 1 for the terminating NUL */
    static char digits[(5 * sizeof(unsigned)) / 2 + 2];

    if (value == 0)
    {
        digits[0] = '0';
        digits[1] = NUL;
    }
    else
    {
        int ofs = sizeof(digits) - 1;
        digits[ofs] = NUL;
        while (value)
        {
            digits[--ofs] = (value % 10) + '0';
            value /= 10;
        }
        if (ofs > 0)
            memmove(digits, digits + ofs, sizeof(digits) - ofs);
    }
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
 * \post On success, all pointers in \p state are setup for starting a parsing
 *   loop, except the list of parameters (field args).
 */
static bool resetParserState(
    struct ParserState* state,
    FatalErrorFormat format)
{
    bool result = state != NULL && format != NULL && memBlock != NULL;
    if (result)
    {
        state->processState = TEXT;
        state->formatPos = format;
        state->buffer = fatalErrorBufferBegin();
        state->bufferEnd = fatalErrorBufferEnd();
        state->arg = NULL;
    }
    else if (state)
    {
        state->processState = END_OF_TEXT;
        state->formatPos = NULL;
        state->buffer = NULL;
        state->bufferEnd = NULL;
        state->arg = NULL;
    }
    return result;
}

static size_t freeSpaceInBuffer(const struct ParserState* state)
{
    return
        state
        && state->buffer
        && state->buffer < state->bufferEnd?
            state->bufferEnd - state->buffer : 0;
}

/*!
 * assume the last character read from the format string was
 * no placeholder and is simply to be copied as is to the buffer.
 *
 * \param state not null, holds the parsing state
 *
 * \pre there is still a byte left in the buffer for any copy action
 * \pre the grammar state is TEXT or END_OF_TEXT
 */
static void handleTextState(struct ParserState* state)
{
     char formatChar = *state->formatPos;
     switch (formatChar)
     {
         case PLACEHOLDER_PREFIX_CHAR:
             state->processState = CHECK_PLACEHOLDER;
             break;
         case NUL:
             state->processState = END_OF_TEXT;
             break;
         default:
             /* keep the TEXT state */
             state->formatPos++;
             *(state->buffer++) = formatChar;
     }
}

/*!
 * assume the current character in the format string is a % and we now
 * need to check it introduced a placeholder.  If not, the % is copied verbatim
 * (not messing with UTF-8).  In particular, a sequence %% is reduced to a
 * single %.
 *
 * A NULL parameter is allowed, and has the same effect as an empty string.
 *
 * \param state not null, holds the parsing state
 *
 * \pre there is still a byte left in the buffer
 * \pre the grammar state in state.state is PLACEHOLDER_PREFIX, so a % is
 *   the current character in the format string.
 * \post the % is handled, either copied, or introducing a parameter.
 */
static void handlePlaceholderPrefixState(struct ParserState* state)
{
    char formatChar = *(++state->formatPos);
    switch (formatChar)
   {
        case PLACEHOLDER_CHAR_STRING:
            /* a %s sequence is recognized as a placeholder.  Don't copy
             * anything, if the parameter is NULL. */
            state->arg = va_arg(state->args, char const*);
            if (state->arg == NULL)
            {
                /* skip the placeholder, we are done */
                state->formatPos++;
                state->processState = TEXT;
            }
            else
                state->processState = PARAMETER_COPY;
            break;
        case PLACEHOLDER_CHAR_UNSIGNED:
            state->arg = unsignedToString(va_arg(state->args, unsigned));
            state->processState = PARAMETER_COPY;
            break;
        case PLACEHOLDER_PREFIX_CHAR:
            /* two consecutive %, ignore the first, copy the second */
            state->formatPos++;
            /* fall through */
        default:
            /* copy the % to the buffer. */
            *(state->buffer++) = PLACEHOLDER_PREFIX_CHAR;
            state->processState = TEXT;
    }
}

/*!
 * assume we encountered a placeholder %s or %u, and now copy from
 * the parameter, given as astring.  It is always copied verbatim, no
 * recursive search for placeholders takes place. The terminating NUL is not
 * copied.
 *
 * \param state not null, holds the parsing state
 *
 * \pre the next format character is not NUL and there is still
 *   a byte left in the buffer
 * \pre state->arg contains a pointer to the parameter to insert verbatim.
 * \pre the grammar state in state->state is PARAMETER_COPY, after e.g. a %s
 *   was encountered in the format string.
 */
static void handleParameterCopyState(struct ParserState* state)
{
    char paramChar = *(state->arg++);
    switch (paramChar)
    {
        case NUL:
            state->arg = NULL;
            state->processState = TEXT;
            state->formatPos++;
            break;
        default:
            *(state->buffer++) = paramChar;
    }
}

/*! evaluate the next character in the format string, append the appropriate
 * text in the message buffer and update the state in \p state.
 * \param state holds the parsing state
 * \returns message completed (possibly after a truncation)
 * \pre \p state is initialized.
 * \post \p state is updated to handle the next character in the
 *   format string
 * \post depending on the format character, text is appended to
 *   the message in the buffer.
 */
static bool parseAndCopy(struct ParserState* state)
{
    bool result = false;
    if (freeSpaceInBuffer(state) == 0)
        /* cannot even copy the terminating NUL any more... */
        state->processState = BUFFER_OVERFLOW;
    else if (*state->formatPos == NUL)
    {
        state->processState = END_OF_TEXT;
        *state->buffer = NUL;
    }
    else
    {
        result = true;
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
 * to \ref memBlock. Truncate if the destination has not enough memory
 * available.  Replace truncated UTF-8 multibyte sequences with space
 * characters.
 * \param state [ParserState, not null] an already initialized
 *   \ref ParserState containing parameters, a destination memory block, and a
 *   format string to interpret.
 * \pre \p state is initialized.
 * \post \p state is in an end state.
 */
static void appendMessage(struct ParserState* state)
{
    while (parseAndCopy(state));
    if (state->processState == BUFFER_OVERFLOW)
    {
        // overwrite truncated UTF-8 character sequences with spaces
        // will not affect ASCII only text
        char* end = fatalErrorBufferEnd();
        char* last = findLastUTF8Byte(fatalErrorBufferBegin(), end - 1);
        if (last != NULL)
            while (++last != end)
                *last = ' ';
    }
}

bool setFatalErrorMessage(FatalErrorFormat format, ...)
{
    struct ParserState state;
    if (getFatalErrorMessage() != NULL)
        clearFatalErrorBuffer();
    bool ok = resetParserState(&state, format);
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
 * \pre a buffer has been set up.
 * \returns a pointer to the end of the buffer contents.
 */
static char* setLocationData(char const* file, unsigned line)
{
    char* result = fatalErrorBufferBegin();
    int hasFileInfo = file != NULL && *file != NUL;
    int hasPosInfo = line != 0 || hasFileInfo;
    if (hasPosInfo)
    {
        char const* posFormat = line == 0?
                "%s:" : (hasFileInfo?
                        "%s:%u:" : "%s%u:");
        hasPosInfo = setFatalErrorMessage(posFormat, file, line);
    }
    if (hasPosInfo)
        result += strlen(result);
    else
       clearFatalErrorBuffer();

    return result;
}

void exitOnFatalError(
    char const* file,
    unsigned line,
    FatalErrorFormat messageFormat, ...)
{
    struct ParserState state;
    resetParserState(&state, messageFormat);

    /* marks the current end position in the buffer */
    state.buffer = setLocationData(file, line);

    /* now process the message */
    va_start(state.args, messageFormat);
    appendMessage(&state);
    va_end(state.args);
    fputs(getFatalErrorMessage(), stderr);
    exit(1);
}

#ifdef TEST_MMFATL

/*  automatic testing to prevent regression   */

static void* memAllocated = NULL;
static int missingFree = 0;
static int freeOutOfOrder = 0;

/* intercept malloc/free for checking proper pairing of both
 * There is only one buffer in use, so we can assume
 * a sequence alloc -> free -> alloc -> free and so on.
 */

static void* MALLOC(size_t size)
{
    void* result = malloc(size);
    if (memAllocated)
        missingFree = 1;
    memAllocated = result;
    return result;
}

static void FREE(void* ptr)
{
    free(ptr);
    if (ptr == memAllocated)
        memAllocated = NULL;
    else if (ptr != NULL)
        freeOutOfOrder = 1;
}

static bool testcase_addCheckOverflow(size_t x, size_t y, size_t expected)
{
    bool result = addCheckOverflow(x, y) == expected;
    if (!result)
        printf("addCheckOverflow(%zu, %zu) failed\n", x, y);
    return result;
}

static bool testall_addCheckOverflow()
{
    printf("testing addCheckOverflow...\n");
    bool result = true;
    result &= testcase_addCheckOverflow(0, 0, 0); // both summands are invalid
    result &= testcase_addCheckOverflow(0, 1, 0); // the first is invalid
    result &= testcase_addCheckOverflow(0, ~ (size_t) 0, 0); // border case
    result &= testcase_addCheckOverflow(1, 0, 0); // the second is invalid
    result &= testcase_addCheckOverflow(~ (size_t) 0, 0, 0); // border case
    result &= testcase_addCheckOverflow(1, 1, 2); // summation possible
    result &= testcase_addCheckOverflow(1, ~ (size_t) 0, 0); // overflow
    result &= testcase_addCheckOverflow(~ (size_t) 0, 1, 0);  // overflow
      // huge overflow
    result &= testcase_addCheckOverflow(~ (size_t) 0, ~ (size_t) 0, 0);
    return result;
}

static bool testall_findLastUTF8Byte()
{
    printf("testing findLastUTF8Byte...\n");
    char buffer[6] = { 0, 0, 0, 0, 0, 0 };
    bool ok = true;
    if (findLastUTF8Byte(buffer, buffer) != buffer)
    {
        printf("test 1 ASCII NUL failed\n");
        ok = false;
    }
    buffer[0] = 0x7F;
    if (ok && findLastUTF8Byte(buffer, buffer) != buffer)
    {
        printf("test 2 ASCII 7F failed\n");
        ok = false;
    }
    if (ok && findLastUTF8Byte(buffer, buffer + 1) != buffer + 1)
    {
        printf("test 3 trailing NUL failed\n");
        ok = false;
    }
    if (ok && findLastUTF8Byte(buffer, buffer + 2) != buffer + 2)
    {
        printf("test 4 trailing NUL failed\n");
        ok = false;
    }
    buffer[0] = 0x87;
    if (ok && findLastUTF8Byte(buffer, buffer) != NULL)
    {
        printf("test 5 continuation byte failed\n");
        ok = false;
    }
    buffer[1] = 0x84;
    if (ok && findLastUTF8Byte(buffer, buffer + 1) != NULL)
    {
        printf("test 6 continuation byte failed\n");
        ok = false;
    }
    buffer[0] = 0x00;
    if (ok && findLastUTF8Byte(buffer, buffer + 1) != NULL)
    {
        printf("test 7 leading ASCII followed by continuation byte failed\n");
        ok = false;
    }
    buffer[1] = 0xC4;
    if (ok && findLastUTF8Byte(buffer + 1, buffer + 1) != NULL)
    {
        printf("test 8 truncated 2 byte character failed\n");
        ok = false;
    }
    if (ok && findLastUTF8Byte(buffer, buffer + 1) != buffer)
    {
        printf("test 9 truncated 2 byte character failed\n");
        ok = false;
    }
    buffer[2] = 0x89;
    if (ok && findLastUTF8Byte(buffer, buffer + 2) != buffer + 2)
    {
        printf("test 10 2 byte character at end failed\n");
        ok = false;
    }
    buffer[1] = 0xE9;
    if (ok && findLastUTF8Byte(buffer + 1, buffer + 1) != NULL)
    {
        printf("test 11 truncated 3 byte character at end failed\n");
        ok = false;
    }
    if (ok && findLastUTF8Byte(buffer + 1, buffer + 2) != NULL)
    {
        printf("test 12 truncated 3 byte character at end failed\n");
        ok = false;
    }
    if (ok && findLastUTF8Byte(buffer, buffer + 2) != buffer)
    {
        printf("test 13 truncated 3 byte character at end failed\n");
        ok = false;
    }
    buffer[3] = 0x82;
    if (ok && findLastUTF8Byte(buffer, buffer + 3) != buffer + 3)
    {
        printf("test 14 3 byte character at end failed\n");
        ok = false;
    }
    buffer[1] = 0xF2;
    if (ok && findLastUTF8Byte(buffer + 1, buffer + 1) != NULL)
    {
        printf("test 15 truncated 4 byte character at end failed\n");
        ok = false;
    }
    if (ok && findLastUTF8Byte(buffer + 1, buffer + 2) != NULL)
    {
        printf("test 16 truncated 4 byte character at end failed\n");
        ok = false;
    }
    if (ok && findLastUTF8Byte(buffer + 1, buffer + 3) != NULL)
    {
        printf("test 17 truncated 4 byte character at end failed\n");
        ok = false;
    }
    if (ok && findLastUTF8Byte(buffer, buffer + 3) != buffer)
    {
        printf("test 18 truncated 4 byte character at end failed\n");
        ok = false;
    }
    buffer[4] = 0x82;
    if (ok && findLastUTF8Byte(buffer + 1, buffer + 4) != buffer + 4)
    {
        printf("test 19 4 byte character at end failed\n");
        ok = false;
    }
    buffer[5] = 0x85;
    if (ok && findLastUTF8Byte(buffer, buffer + 5) != NULL)
    {
        printf("test 20 excess continuation bytes failed\n");
        ok = false;
    }
    return ok;
}

static bool test_getFatalErrorDescriptor()
{
    printf("testing getFatalErrorBufferDescriptor...\n");
    bool result = getFatalErrorBufferDescriptor() == &descriptor;
    if (!result)
        printf("getFatalErrorBufferDescriptor() failed\n");
    return result;
}

static bool testall_isValidFatalErrorBufferDescriptor()
{
    printf("testing isValidFatalErrorBufferDescriptor...\n");
    struct TestCase {
        struct FatalErrorBufferDescriptor descriptor;
        int result; };
    struct TestCase tests[] =
    {
        {{ 0, 0, NULL }, 0 },  // case 0, size and ellipsis invalid
          // case 1, size and ellipsis invalid, dmz requested
        {{ 0, 100, NULL }, 0 },
        {{ 1000, 0, NULL }, 0 },  // case 2, ellipsis invalid
          // case 3, ellipsis invalid, dmz requested
        {{ 1000, 100, NULL }, 0 },
        {{ 0, 0, "" }, 0 },  // case 4, size invalid
          // case 5, size invalid, single character ellipsis
        {{ 0, 0, "?" }, 0 },
          // case 6, size invalid, no ellipsis, dmz requested
        {{ 0, 100, "" }, 0 },
          // case 7, size invalid, single character ellipsis, dmz requested
        {{ 0, 100, "?" }, 0 },
        {{ 1000, 0, "" }, 1 },  // case 8, no ellipsis, no dmz
        {{ 1000, 100, "" }, 1 },  // case 9, no ellipsis, dmz requested
        {{ 1000, 0, "?" }, 1 },  // case 10, single character ellipsis, no dmz
          // case 11, single character ellipsis, dmz requested
        {{ 1000, 100, "?" }, 1 },
        {{ 1000, 0, "..." }, 1 },  // case 12, complex ellipsis, no dmz
        {{ 1000, 100, "..." }, 1 }, // case 13, complex ellipsis, dmz requested
    };
    bool result = isValidFatalErrorBufferDescriptor(0) == 0;
    if (!result)
        printf ("submitting NULL failed\n");

    for (unsigned i = 0; i < sizeof(tests) / sizeof(struct TestCase); ++i)
        if (isValidFatalErrorBufferDescriptor(&tests[i].descriptor)
                != tests[i].result)
        {
            printf ("case %i failed\n", i);
            result = false;
        }
    return result;
}

static bool testall_evalRequestedMemSize()
{
    printf("testing evalRequestedMemSize...\n");
    bool result = evalRequestedMemSize(0) == 0;
    if (!result)
        printf("submitting NULL failed\n");

    struct TestCase {
        struct FatalErrorBufferDescriptor descriptor;
        size_t result; };
    struct TestCase tests[] =
    {
        {{ 0, 0, NULL }, 0 },  // case 0
        {{ 1000, 0, "" }, 1001u },  // case 1
        {{ 1000, 100, "" }, 1201u },  // case 2
        {{ ~ (size_t) 0, 0, "" }, 0 },  // case 3
        {{ (~ (size_t) 0) - 1u , 0, "" }, ~ (size_t) 0 },  // case 4
        {{ (~ (size_t) 0) - 50u , 100, "" }, 0 },  // case 5
        {{ (~ (size_t) 0) - 101u , 100, "" }, 0 },  // case 6
        {{ (~ (size_t) 0) - 204u , 100, "..." }, ~ (size_t) 0 },  // case 7
        {{ (~ (size_t) 0) - 1u , 0, "..." }, 0 },  // case 8
    };
    for (unsigned i = 0; i < sizeof(tests) / sizeof(struct TestCase); ++i)
        if (evalRequestedMemSize(&tests[i].descriptor) != tests[i].result)
        {
            printf ("case %i failed\n", i);
            result = false;
        }
    return result;
}

static bool test_swapMemBlock(struct FatalErrorBufferDescriptor* data,
                             void** buffer, size_t bufferSize)
{
    bool result = bufferSize == 0
            || evalRequestedMemSize(data) == bufferSize;
    if (result)
    {
        void* backupBuffer = memBlock;
        struct FatalErrorBufferDescriptor backupDescriptor = descriptor;
        memBlock = *buffer;
        descriptor = *data;
        *buffer = backupBuffer;
        *data = backupDescriptor;
    }
    return result;
}

static bool test_fatalErrorBufferBegin()
{
    printf("testing fatalErrorBufferBegin...\n");

    unsigned const dmz = 100;
    unsigned const size = 1000;
    char buffer[size + 2 * dmz + 3 /* ellipsis */ + 1 /* NUL */];
    struct FatalErrorBufferDescriptor memDescriptor = { size, dmz, "..." };
    void* bufferPt = buffer;
    bool result = test_swapMemBlock(&memDescriptor, &bufferPt, sizeof(buffer));
    if (result)
    {
        result = fatalErrorBufferBegin() == (buffer + dmz);
        if (!result)
            printf ("failed to return the pointer to the begin of the "
                    "fatal error buffer\n");
        test_swapMemBlock(&memDescriptor, &bufferPt, 0);
    }
    else
        printf ("test setup is incorrect, descriptor does not describe the"
                " given buffer\n");
    return result;
}

static bool test_fatalErrorBufferEnd()
{
    printf("testing fatalErrorBufferEnd...\n");

    unsigned const dmz = 100;
    unsigned const size = 1000;
    char buffer[size + 2 * dmz + 3 /* ellipsis */ + 1 /* NUL */];
    struct FatalErrorBufferDescriptor memDescriptor = { size, dmz, "..." };
    void* bufferPt = buffer;
    bool result = test_swapMemBlock(&memDescriptor, &bufferPt, sizeof(buffer));
    if (result)
    {
        result = fatalErrorBufferEnd() - fatalErrorBufferBegin() == size;
        if (!result)
            printf ("failed to return the pointer to the end of the fatal "
                    "error buffer\n");
        test_swapMemBlock(&memDescriptor, &bufferPt, 0);
    }
    else
        printf ("test setup is incorrect, descriptor does not describe the "
                "given buffer\n");
    return result;
}

static bool test_clearFatalErrorBuffer()
{
    printf("testing clearFatalErrorBuffer...\n");

    unsigned const dmz = 100;
    unsigned const size = 1000;
    char buffer[size + 2 * dmz + 3 /* ellipsis */ + 1 /* NUL */];
    struct FatalErrorBufferDescriptor memDescriptor = { size, dmz, "..." };
    void* bufferPt = buffer;
    bool result = test_swapMemBlock(&memDescriptor, &bufferPt, sizeof(buffer));
    if (result)
    {
        clearFatalErrorBuffer();
        result = strlen(fatalErrorBufferBegin()) == 0? 1 : 0;
        if (!result)
            printf ("failed to clear the fatal error buffer\n");
        test_swapMemBlock(&memDescriptor, &bufferPt, 0);
    }
    else
        printf ("test setup is incorrect, descriptor does not describe the "
                "given buffer\n");
    return result;
}

static bool test_initFatalErrorBuffer()
{
    printf("testing clearFatalErrorBuffer...\n");

    unsigned const dmz = 100;
    unsigned const size = 1000;
    char const* ellipsis = "...";
    char buffer[size + 2 * dmz + 3 /* ellipsis */ + 1 /* NUL */];
    struct FatalErrorBufferDescriptor memDescriptor = { size, dmz, ellipsis };
    void* bufferPt = buffer;
    bool result = test_swapMemBlock(&memDescriptor, &bufferPt, sizeof(buffer));
    if (result)
    {
        initFatalErrorBuffer();
        ellipsis = NULL;
        result = strlen(fatalErrorBufferBegin()) == 0;
        if (!result)
            printf ("failed to clear the fatal error buffer\n");
        if (result && strlen(fatalErrorBufferEnd()) != 3)
        {
            printf ("failed to copy the ellipsis to the fatal error buffer\n");
            result = false;
        }
        if (result && strcmp(fatalErrorBufferEnd(), descriptor.ellipsis) != 0)
        {
            printf ("failed to secure the pointer to ellipsis in the "
                    "descriptor\n");
            result = false;
        }
        test_swapMemBlock(&memDescriptor, &bufferPt, 0);
    }
    else
        printf ("test setup is incorrect, descriptor does not describe the "
                "given buffer\n");
    return result;
}

static bool test_freeFatalErrorBuffer()
{
    freeFatalErrorBuffer();
    bool result =
        !isValidFatalErrorBufferDescriptor(getFatalErrorBufferDescriptor());
    if (!result)
        printf ("failed to reset the descriptor after free\n");
    if (memBlock)
    {
        result = false;
        printf ("failed to reset the pointer to buffer after free\n");
    }
    if (freeOutOfOrder)
    {
        result = false;
        printf ("freeing memory not allocated immediately before\n");
    }
    return result;
}

static bool test_allocFatalErrorBuffer(struct FatalErrorBufferDescriptor* d,
                                      bool expectResult, int testCase)
{
    bool result = allocFatalErrorBuffer(d) == expectResult;
    if (!result)
        printf("allocation in case %i failed: expected result %i missed\n",
               testCase, expectResult);
    if (result && expectResult != isValidFatalErrorBufferDescriptor(&descriptor))
    {
        result = false;
        printf ("failed to set the descriptor after allocation in case %i\n",
                testCase);
    }
    if (result && expectResult != (memBlock != NULL))
    {
        result = false;
        printf ("failed to set the buffer pointer after allocation in "
                "case %i\n", testCase);
    }
    if (result && freeOutOfOrder)
    {
        result = false;
        printf ("freeing memory not allocated immediately before in case %i\n",
                testCase);
    }
    if (result && missingFree)
    {
        result = false;
        printf ("memory not freed before allocation in case %i\n", testCase);
    }
    return result;
}

static bool testall_Allocation()
{
    printf("testing allocation of memory...\n");

    // free without allocation
    bool result = test_freeFatalErrorBuffer();
    struct FatalErrorBufferDescriptor d[] =
    {
        { 1000, 0, "" },
        { 2000, 0, "" },
        { 0, 0, NULL },
    };
    if (result)
        result = test_allocFatalErrorBuffer(d + 0, true, 0);
    // reallocation
    if (result)
        result = test_allocFatalErrorBuffer(d + 1, true, 1);
    // invalid allocation
    if (result)
        result = test_allocFatalErrorBuffer(d + 2, false, 2);
    return result;
}

static bool test_unsignedToString(unsigned value, char const* digits)
{
    char const* computed = unsignedToString(value);
    bool result = strcmp(computed, digits) == 0;
    if (!result)
        printf ("conversion of %i yielded \"%s\"\n", value, computed);
    return result;
}

static bool testall_unsignedToString()
{
    printf("testing unsignedToInt...\n");
    bool result =
        test_unsignedToString(0u, "0")
    && test_unsignedToString(1u, "1")
    && test_unsignedToString(2u, "2")
    && test_unsignedToString(3u, "3")
    && test_unsignedToString(4u, "4")
    && test_unsignedToString(5u, "5")
    && test_unsignedToString(6u, "6")
    && test_unsignedToString(7u, "7")
    && test_unsignedToString(8u, "8")
    && test_unsignedToString(9u, "9")
    && test_unsignedToString(10u, "10")
    && test_unsignedToString(99u, "99")
    && test_unsignedToString(100u, "100")
    && test_unsignedToString(65535u, "65535");
# if UINT_MAX >= 4294967295UL
    result =
    result
    && test_unsignedToString(65536ul, "65536")
    && test_unsignedToString(4294967295ul, "4294967295")? 1 : 0;
#   endif
#   if UINT_MAX >= 18446744073709551615ULL
    result =
    result
    && test_unsignedToString(4294967296ull, "4294967296")
    && test_unsignedToString(18446744073709551615ull, "18446744073709551615");
#   endif
    return result;
}

static void test_allocTestErrorBuffer(unsigned size)
{
    struct FatalErrorBufferDescriptor d;
    d.size = size;
    d.dmz = 0;
    d.ellipsis = "?";
    allocFatalErrorBuffer(&d);
}

static bool test_resetParserState(int index, struct  ParserState* state,
                          char const* format, bool expectedResult)
{
    bool result = resetParserState(state, format) == expectedResult;
    if (!result)
        printf ("case %i: expected %s\n", index,
                !expectedResult? "failure" : "success");
    return result;
}

static bool compareParserState(int testCase, const struct ParserState* state,
            enum ParserProcessState processState, unsigned freeSpace,
            char last, char nextChar)
{
    bool result = state->processState == processState;
    if (!result)
        printf("case %i: incorrect parser process state, expected %i, "
               "got %i\n", testCase, processState, state->processState);
    else
    {
        unsigned space = freeSpaceInBuffer(state);
        if (space != freeSpace)
        {
            printf("case %i: incorrect buffer contents, expected free "
                   "space %i, got %i\n", testCase, freeSpace, space);
            result = false;
        }
    }
    if (result && last != NUL)
    {
        char c = *(state->buffer - 1);
        if (c != last)
        {
            printf("case %i: incorrect buffer contents, expected char %c, "
                   "got %c\n", testCase, last, c);
            result = false;
        }
    }
    if (result)
    {
        char c = *(state->formatPos);
        if (c != nextChar)
        {
            if (c == NUL)
                printf("case %i: incorrect format position, at the end "
                       "prematurely\n", testCase);
            else if (nextChar == NUL)
                printf("case %i: incorrect format position, not at end\n",
                       testCase);
            else
                printf("case %i: incorrect format position, expected '%c', "
                       "got '%c'\n", testCase, nextChar, c);
            result = false;
        }
    }
    return result;
}

static bool testall_resetParserState()
{
    printf("testing resetParserState...\n");
    struct ParserState state;
    bool result =
        test_resetParserState(0, 0, 0, false)
        && test_resetParserState(1, 0, "x", false)
        && test_resetParserState(2, &state, 0, false)
        && test_resetParserState(3, &state, "", 0);
    if (result)
    {
        test_allocTestErrorBuffer(20);
        result =
            test_resetParserState(4, 0, 0, false)
            && test_resetParserState(5, 0, "x", false)
            && test_resetParserState(6, &state, 0, false)
            && test_resetParserState(7, &state, "x", true);
        if (result)
        {
            result =
                compareParserState(8, &state, TEXT, 20, NUL, 'x');
            if (result)
            {
                result =
                    state.arg == NULL
                    && *state.bufferEnd == '?';
                if (!result)
                    printf("case 8: incorrect initialization of "
                           "struct ParserState");
            }
        }
    }
    freeFatalErrorBuffer();
    return result;
}

static bool test_freeSpaceInBuffer(int index, struct ParserState* state,
                                  size_t expectedResult)
{
    size_t free = freeSpaceInBuffer(state);
    bool result = free == expectedResult;
    if (!result)
        printf ("case %i: expected %zu, got %zu\n", index,
                expectedResult, free);
    return result;
}

static bool testall_freeSpaceInBuffer()
{
    printf("testing freeSpaceInBuffer...\n");
    struct ParserState state;
    resetParserState(&state, NULL);
    bool result =
        test_freeSpaceInBuffer(0, 0, 0);
    if (result)
    {
        test_allocTestErrorBuffer(20);
        resetParserState(&state,"");
        result = test_freeSpaceInBuffer(1, &state, 20);
    }
    if (result)
    {
        state.buffer += state.bufferEnd - state.buffer - 1;
        result = test_freeSpaceInBuffer(2, &state, 1);
    }
    if (result)
    {
        state.buffer++;
        result = test_freeSpaceInBuffer(3, &state, 0);
    }
    if (result)
    {
        state.buffer++;
        result = test_freeSpaceInBuffer(4, &state, 0);
    }
    freeFatalErrorBuffer();
    return result;
}

static bool testall_handleTextState()
{
    printf("testing handleTextState...\n");
    struct ParserState state;
    test_allocTestErrorBuffer(20);
    resetParserState(&state, "a%");
    handleTextState(&state);
    bool result = compareParserState(0, &state, TEXT, 19, 'a', '%');
    if (result)
    {
        handleTextState(&state);
        result = compareParserState(1, &state, CHECK_PLACEHOLDER, 19, 'a',
                                    '%');
    }
    if (result)
    {
        resetParserState(&state, "");
        handleTextState(&state);
        result = compareParserState(2, &state, END_OF_TEXT, 20, NUL, NUL);
    }
    freeFatalErrorBuffer();
    return result;
}

static bool test_handlePlaceholderPrefix(int testCase, char const* match,
                                        struct ParserState* state, ...)
{
    va_start(state->args, state);
    handleTextState(state);
    char const* format = state->formatPos + (match? 1 : 2);
    enum ParserProcessState expect = match? PARAMETER_COPY : TEXT;
    handlePlaceholderPrefixState(state);
    char const* current = state->arg;
    bool ok = compareParserState(testCase, state, expect, 20, NUL, *format);
    if (ok && match)
    {
        int pos = -1;
        while(ok && *match)
        {
            ok = *match++ == *current++? 1 : 0;
            ++pos;
        }
        if (!ok)
        printf("arg differs from parameter at pos %i in test case %i\n", pos,
               testCase);
    }
    if (ok && state->formatPos != format)
        printf("format pointer not in correct position in test case %i\n",
               testCase);
    va_end(state->args);
    return ok;
}

static bool testall_handlePlaceholderPrefixState()
{
    printf("testing handlePlaceholderPrefixState...\n");
    test_allocTestErrorBuffer(20);
    struct ParserState state;
    // case 1: single percent not followed by a legal format character
    resetParserState(&state, "%");
    handleTextState(&state);
    handlePlaceholderPrefixState(&state);
    bool result = compareParserState(1, &state, TEXT, 19, '%', NUL);
    if (result)
    {
        // case 2 escaped percent, representing a single percent
        resetParserState(&state, "%%");
        handleTextState(&state);
        handlePlaceholderPrefixState(&state);
        result = compareParserState(2, &state, TEXT, 19, '%', NUL);
    }
    if (result)
    {
        // case 3: placeholder for text
        resetParserState(&state, "%s");
        test_handlePlaceholderPrefix(3, "xx", &state, "xx");
    }
    if (result)
    {
        // case 4: placeholder for unsigned integer
        resetParserState(&state, "%u");
        test_handlePlaceholderPrefix(4, "1234", &state, 1234);
    }
    if (result)
    {
        // case 5: placeholder for unsigned integer
        resetParserState(&state, "%s");
        test_handlePlaceholderPrefix(5, NULL, &state, (char const*)NULL);
    }

    freeFatalErrorBuffer();
    return result;
}

static bool testall_handleParameterCopyState()
{
    printf("testing handleParameterCopyState...\n");
    test_allocTestErrorBuffer(20);
    // case 1: copying a character from the parameter
    struct ParserState state;
    resetParserState(&state, "%s");
    test_handlePlaceholderPrefix(0, "1", &state, "1");
    handleParameterCopyState(&state);
    bool ok = compareParserState(1, &state, PARAMETER_COPY, 19, '1', 's');
    if (ok)
    {
        // case 2: reacting on terminating NUL of parameter
        handleParameterCopyState(&state);
        ok = compareParserState(2, &state, TEXT, 19, NUL, NUL);
    }
    freeFatalErrorBuffer();
    return ok;
}

static void test_parseAndCopy(struct ParserState* state, ...)
{
    va_start(state->args, state);
    while (parseAndCopy(state));
    va_end(state->args);
}

static bool testall_parseAndCopy()
{
    printf("testing parseAndCopy...\n");
    test_allocTestErrorBuffer(3);
    struct ParserState state;
    resetParserState(&state, "x%s%u%s");
    char const* buffer = state.buffer;
    test_parseAndCopy(&state, "y", 1, NULL);
    bool ok = compareParserState(1, &state, BUFFER_OVERFLOW, 0, '1', 'u');
    if (ok && strcmp(buffer, "xy1?") != 0)
        printf("test 1: expected buffer contents 'xy1?', got %s\n", buffer);

    if (ok)
    {
        resetParserState(&state, "%u");
        test_parseAndCopy(&state, 12);
        ok = compareParserState(2, &state, END_OF_TEXT, 1, '2', NUL);
    }
    if (ok && strcmp(buffer, "12") != 0)
        printf("test 2: expected buffer contents '12', got %s\n", buffer);

    freeFatalErrorBuffer();
    return ok;
}

static bool testall_appendMessage()
{
    printf("testing appendMessage...\n");
    test_allocTestErrorBuffer(10);
    struct ParserState state;
    resetParserState(&state, "abc%%def");
    char const* buffer = state.buffer;
    appendMessage(&state);
    bool ok = compareParserState(1, &state, END_OF_TEXT, 3, 'f', NUL);
    if (ok && strcmp(buffer, "abc%def") != 0)
        printf("test 1: expected buffer contents 'abc%%def', got %s\n",
               buffer);
    if(ok)
    {
        state.formatPos = "1234";
        state.processState = TEXT;
        appendMessage(&state);
        ok = compareParserState(2, &state, BUFFER_OVERFLOW, 0, '3', '4');
        if (ok && strcmp(buffer, "abc%def123?") != 0)
            printf("test 2: expected buffer contents 'abc%%def123?', "
                   "got %s\n", buffer);
    }
    if(ok)
    {
        // Unicode
        resetParserState(&state, "123456789\xC2\xA2");
        appendMessage(&state);
        ok = compareParserState(3, &state, BUFFER_OVERFLOW, 0, ' ', 0xA2);
        if (ok && strcmp(buffer, "123456789 ?") != 0)
            printf("test 2: expected buffer contents '123456789 ?', "
                   "got %s\n", buffer);
    }
    freeFatalErrorBuffer();
    return ok;
}

static bool testall_setFatalErrorMessage()
{
    printf("testing setFatalErrorMessage...\n");
    bool ok = setFatalErrorMessage("xy") == 0;
    if(!ok)
        printf("test 1: expected failure due to missing buffer\n");
    if(ok)
    {
        ok = setFatalErrorMessage(NULL) == 0;
        if(!ok)
            printf("test 2: expected failure due to missing buffer, format\n");
    }
    test_allocTestErrorBuffer(10);
    if(ok)
    {
        ok = setFatalErrorMessage(NULL) == 0;
        if(!ok)
            printf("test 3: expected failure due to missing format\n");
    }
    if(ok)
    {
        ok = setFatalErrorMessage("") && strcmp(getFatalErrorMessage(), "") == 0;
        if(!ok)
            printf("test 4: expected message '', got '%s'\n",
                   (char const*)memBlock);
    }
    if(ok)
    {
        ok = setFatalErrorMessage("abc") && strcmp(getFatalErrorMessage(),
                                                   "abc") == 0;
        if(!ok)
            printf("test 5: expected message 'abc', got '%s'\n",
                   (char const*)memBlock);
    }
    if(ok)
    {
        ok = setFatalErrorMessage("%s", NULL) && strcmp(getFatalErrorMessage(),
                                                        "") == 0;
        if(!ok)
            printf("test 6: expected message '', got '%s'\n",
                   (char const*)memBlock);
    }
    if(ok)
    {
        ok = setFatalErrorMessage("%s", "abc")
                && strcmp(getFatalErrorMessage(), "abc") == 0;
        if(!ok)
            printf("test 7: expected message 'abc', got '%s'\n",
                   (char const*)memBlock);
    }
    if(ok)
    {
        ok = setFatalErrorMessage("%u", 1234) && strcmp(getFatalErrorMessage(),
                                                        "1234") == 0;
        if(!ok)
            printf("test 8: expected message '1234', got '%s'\n",
                   (char const*)memBlock);
    }
    if(ok)
    {
        ok = setFatalErrorMessage("%u%s", 1234, "5678")
                && strcmp(getFatalErrorMessage(), "12345678") == 0;
        if(!ok)
            printf("test 9: expected message '12345678', got '%s'\n",
                   (char const*)memBlock);
    }
    if(ok)
    {
        ok = setFatalErrorMessage("%u%s", 1234, "5678901")
                && strcmp(getFatalErrorMessage(), "1234567890?") == 0;
        if(!ok)
            printf("test 10: expected message '1234567890?', got '%s'\n", (char const*)memBlock);
    }
    freeFatalErrorBuffer();
    return ok;
}

static bool test_setLocationData(int testCase, char const* file, unsigned line,
                                char const* expected)
{
    char const* begin = fatalErrorBufferBegin();
    char const* end = setLocationData(file, line);
    bool ok = strcmp(begin, expected) == 0;
    if (!ok)
        printf("test %u: expected '%s', got '%s'\n", testCase, expected, begin);
    else if(strlen(expected) != (size_t)(end - begin))
        printf("test %u: returned pointer does not point to the current "
               "end of buffer", testCase);
    return ok;
}

static bool testall_setLocationData()
{
    printf("testing setLocationData...\n");
    test_allocTestErrorBuffer(20);
    bool ok = test_setLocationData(1, NULL, 0, "")
    && test_setLocationData(2, "", 0, "")
    && test_setLocationData(3, "", 123, "123:")
    && test_setLocationData(4, "metamath.c", 0, "metamath.c:")
    && test_setLocationData(5, "metamath.c", 123, "metamath.c:123:");
    freeFatalErrorBuffer();
    return ok;
}

bool testSuccessMessage()
{
    printf("Regression tests in " __FILE__ " indicate no error\n\n");
    return true;
}

void mmfatl_test()
{
    if (testall_addCheckOverflow()
        && testall_findLastUTF8Byte()
        && test_getFatalErrorDescriptor()
        && testall_isValidFatalErrorBufferDescriptor()
        && testall_evalRequestedMemSize()
        && test_fatalErrorBufferBegin()
        && test_fatalErrorBufferEnd()
        && test_clearFatalErrorBuffer()
        && test_initFatalErrorBuffer()
        && testall_Allocation()
        && testall_unsignedToString()
        && testall_resetParserState()
        && testall_freeSpaceInBuffer()
        && testall_handleTextState()
        && testall_handlePlaceholderPrefixState()
        && testall_handleParameterCopyState()
        && testall_parseAndCopy()
        && testall_appendMessage()
        && testall_setFatalErrorMessage()
        && testall_setLocationData()
        && testSuccessMessage()
    ) { }
}

#endif
