/*****************************************************************************/
/*        Copyright (C) 2022  Wolf Lammen                                    */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file mmfatl.c a self-contained set of text printing routines using
 * dedicated pre-allocated memory, designed to likely run even under difficult
 * conditions (corrupt state, out of memory).
 *
 * C99 (https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf)
 * compatible.  For a check enter the directory where build.sh is located,
 * create a subdirectory named 'build', and run:
 *
 * cd build && gcc -I. -I../src -I.. -std=c99 -pedantic \
 *   -DINLINE=inline -DTEST_ENABLE -c -o mmfatl.o ../src/mmfatl.c
 *
 * This should not produce an error or a warning.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "mmfatl.h"

/*!
 * some ASCII control characters
 */
enum {
  NUL = '\x00',
  LF = '\n',
};


//***     utility code used in the implementation of the interface    ***


/*!
 * \brief declares a buffer used to generate a text message through a
 * formatting procedure.
 *
 * This buffer type is used to send a final diagnostic message to the user,
 * before the program dies because of insufficient, or even corrupt memory.
 * Under such severe conditions support from the C library is limited.  In
 * particular the memory heap is not available, thus forbidding dynamic program
 * features.  That is why a buffer of this type is usally preallocated.
 *
 * The main buffer operation is appending to already available contents, much
 * in the way of a stream.  Occasionally we want to tentatively append
 * characters, so the semantics of this buffer does not match exactly that of a
 * stream.
 *
 * Apart from the writeable portion of the buffer, a fixed text \ref ELLIPSIS
 * is padded to the right, so in case of a buffer overflow both concatenated
 * portions automatically indicate truncated text.
 *
 * The available unallocated buffer space is delimited by member *end*  on the
 * right.  Write operations must never write to its address, or even trespass
 * it.  The current implementation sees its value as constant once it got
 * initialized.
 *
 * The buffer is filled by a formatting procedure, that usually appends to
 * previously generated text.  The current insertion point on the left is
 * marked by the member *begin* pointer.  Each insertion forwards it in
 * the direction of *end*.
 */

struct Buffer {
  /*! points to first unallocated character */
  char* begin;
  /*!
   * marks the end of the writeable portion, where the \ref MMFATL_ELLIPSIS
   * begins. Logically constant once it got initialized.  Never overwrite this
   * value.
   */
  char* end;
  /*!
   * the writeable buffer, followed by fixed text indicating truncation if
   * necessary.
   */
  char text[MMFATL_MAX_MSG_SIZE + sizeof(MMFATL_ELLIPSIS)];
};

/*!
 * a preallocated buffer supporting formatting.
 */
static struct Buffer buffer;

/*!
 * We do not rely on any initialization during program start.  Instead we
 * assume the worst case, a corrupted pointer overwrote the buffer.  So we
 * initialize it again immediately before use.
 * \pre the ellipsis appended to the writeable portion of the buffer must
 *   terminate with a LF, so printing a buffer in overflow state will keep
 *   a command prompt in a new line after program exit.
 * \post \ref buffer is initialized with all NUL characters, so NUL need
 *   not be copied
 * \param buffer [not null] the output buffer to empty and initialize
 */
static void initBuffer(struct Buffer* buffer) {
  char ellipsis[] = MMFATL_ELLIPSIS;

  buffer->begin = buffer->text;
  buffer->end = buffer->text + MMFATL_MAX_MSG_SIZE;
  memset(buffer->begin, NUL, MMFATL_MAX_MSG_SIZE);
  memcpy(buffer->end, ellipsis, sizeof(ellipsis));
}

/*!
 * \param buffer [const, not null] the buffer to check for emptiness.
 * \return true, iff the \ref buffer is in its initial state.
 * \pre \ref initBuffer was called
 */
inline static bool isBufferEmpty(struct Buffer const* buffer) {
  return buffer->begin == buffer->text;
}

/*!
 * Get the last stored character in the buffer.  Discarded characters due to
 * overflow are ignored.  A returned NUL indicates the buffer is empty.
 * \param buffer [const, not null] the buffer to investigate.
 * \return the last character stored in buffer, or NUL iff empty.
 * \pre \ref initBuffer was called
 */
static char getLastBufferedChar(struct Buffer const* buffer) {
  return isBufferEmpty(buffer)? NUL : *(buffer->begin - 1);
}

/*!
 * \param buffer [const, not null] the buffer to check for overflow.
 * \return true, iff the current contents exceeds the capacity of the
 *   \ref buffer, so at least the terminating \ref NUL is cut off, maybe more.
 * \pre \ref initBuffer was called
 */
inline static bool isBufferOverflow(struct Buffer const* buffer) {
  return buffer->begin == buffer->end;
}

/*!
 * used to indicate whether \ref MMFATL_PH_PREFIX is a normal character, or
 * an escape character in a format string.
 */
enum SourceType {
  STRING, //<! NUL terminated text
  FORMAT  //<! NUL terminated format containing placeholders
};

/*!
 * append characters to the current end of the buffer from a string until a
 * terminating \ref NUL, or optionally a placeholder is encountered, or the
 * buffer overflows.
 * \param source [not null] the source from which bytes are copied.
 * \param type If \ref FORMAT, \ref MMFATL_PH_PREFIX besides \ref NUL stops
 *   copying.
 * \param buffer [not null] the output buffer where to append the source text.
 * \return the number of characters copied.
 * \pre \ref initBuffer was called
 */
static unsigned appendText(char const* source, enum SourceType type,
                           struct Buffer* buffer) {
  char escape = type == FORMAT? MMFATL_PH_PREFIX : NUL;
  char const* start = buffer->begin;
  while (*source != NUL && *source != escape && !isBufferOverflow(buffer))
    *buffer->begin++ = *source++;
  return buffer->begin - start;
}

/*!
 * A simple grammar scheme allows inserting data in a prepared general message.
 * The scheme is a downgrade of the C format string.  Allowed are only
 * placeholders %s and %u that are replaced with given data.  The percent sign
 * % is available through duplication %%.  For this kind of grammar 4
 * separate process states are sufficient, encoded in the \ref format pointer:
 *
 * 1. the current format pointer points to \ref NUL (end of parse);
 * 2. the current format pointer points to a placeholder (\ref MMFATL_PH_PREFIX);
 * 3. the current format pointer points to any other character;
 *
 * During a parse, the state alternates between 2 and 3, until the terminating
 * state 1 is reached.
 */
struct ParserState {
  /*!
   * The buffer the expanded format string is written to.  The buffer updates
   * its own state if accessed only through \ref initBuffer and \ref appendText.
   * \invariant never NULL
   */
  struct Buffer* out;
  /*!
   * the next reading position in a NUL-terminated format string.  Will be
   * consumed, i.e. scanned only once from begin to end.
   * \invariant never NULL.
   */
  char const* format;
  /*!
   * the list of parameters substituting placeholders.  This structure allows
   * traversing all entries.
   *
   * \invariant The parameters match the placeholders in \ref format in
   * type, and their number is not less than that of the placeholders.
   * This invariant cannot be verified at runtime in this module, but must be
   * guaranteed on invocation by the caller.
   */
  va_list args;
};

static struct ParserState state;

/*!
 * initializes \ref state.
 * \post establish the invariant in state
 * \param state [not null] the struct \ref ParserState to initialize.
 * \param buffer [not null] the buffer to use for output 
 */
static void initState(struct ParserState* state, struct Buffer* buffer) {
  // The invariants in state are established.
  static char empty[] = "";
  state->out = buffer;
  state->format = empty;
}

/*!
 * converts an unsigned to a sequence of decimal digits representing its value.
 * The value range is known to be at least 2**32 on contemporary hardware, but
 * C99 guarantees just 2**16.  We support unsigned in formatted error output
 * to allow for macros like __LINE__ denoting error positions in text files.
 *
 * There exist no utoa in the C99 standard library, that could be used instead,
 * and sprintf must not be used in a memory-tight situation (AS Unsafe heap,
 * https://www.gnu.org/software/libc/manual/html_node/Formatted-Output-Functions.html).
 * \param value an unsigned value to convert.
 * \returns a pointer to a string converted from \p value.  Except for zero,
 *   the result has a leading non-zero digit.
 * \attention  The result is stable only until the next call to this function.
 */
static char const* unsignedToString(unsigned value) {
  /*
   * sizeof(value) * CHAR_BIT are the bits encodable in value, the factor 146/485,
   * derived from a chained fraction, is about 0.3010309, slightly greater than
   * log 2.  So the number within the brackets is the count of decimal digits
   * encodable in value.  Two extra bytes compensate for the truncation error of
   * the division and allow for a terminating NUL.
   */
  static char digits[(sizeof(value) * CHAR_BIT * 146) / 485 + 2];

  unsigned ofs = sizeof(digits) - 1;
  digits[ofs] = NUL;
  if (value == 0)
    digits[--ofs] = '0';
  else {
    while (value) {
      digits[--ofs] = (value % 10) + '0';
      value /= 10;
    }
  }
  return digits + ofs;
}

/*! reflect a possible buffer overflow in the parser state
 * \param state [not null] ParserState object being updated in case of
 *   overflow
 * \return false in case of overflow
 * \pre \ref initState was called
 * \post in case of overflow the current format position is moved to the end
 */
static bool checkOverflow(struct ParserState* state) {
  bool isOverflow = isBufferOverflow(state->out);
  if (isOverflow)
    state->format += strlen(state->format);
  return !isOverflow;
}

/*!
 * copy text verbatim from a format string to the message buffer, until either
 * the format ends, or a placeholder is encountered.
 * \param state struct ParserState* parser state going to be handled and updated
 * \pre \ref initState was called
 * \post member format of \ref state is advanced over the copied text, if no
 *   overflow.
 * \post member format of \ref state points to the terminating \ref NUL on
 *   overflow.
 */
static void handleText(struct ParserState* state) {
  state->format += appendText(state->format, FORMAT, &buffer);
  checkOverflow(state);
}

/*!
 * A format specifier is a two character combination, where a placeholder
 * character \ref MMFATL_PH_PREFIX is followed by an alphabetic character
 * designating a type.  A placeholder is substituted by the next argument in
 * member *args* of \ref state.  This function handles this substitution when
 * member *format* of \ref state points to a placeholder.
 *
 * This function also handles the case where the type character is missing or
 * invalid.  Usually we want to log the presence of stray MMFATL_PH_PREFIX
 * characters in a format string to some file, or at least display a warning.
 * But we are in the process of a fatal error already, so we gracefully accept
 * garbage and simply copy that character to the output, in the hope, misplaced
 * characters are somehow noticed there.
 *
 * Note that a duplicated MMFATL_PH_PREFIX is the accepted way to embed such a
 * character in a format string.  This is correctly handled in this function.
 * \pre the format member in \ref state points to a MMFATL_PH_PREFIX.
 * \pre \ref initState was called
 * \post the substituted value is written to \ref buffer.
 * \post the format member in \ref state is skipped
 * \param state struct ParserState* parser state going to be handled and updated
 */
static void handleSubstitution(struct ParserState* state) {
  // replacement value for a token representing MMFATL_PH_PREFIX itself
  static char const defaultArg[2] = { MMFATL_PH_PREFIX, NUL };
  char const* arg;
  int placeholderSize = 2; // advance state.format by this many characters
  switch (*(state->format + 1)) {
    case MMFATL_PH_STRING:
      arg = va_arg(state->args, char const*);
      break;
    case MMFATL_PH_UNSIGNED:
      // a %u format specifier is recognized. 
      arg = unsignedToString(va_arg(state->args, unsigned));
      break;
    case MMFATL_PH_PREFIX:
      // %%
      arg = defaultArg;
      break;
    default:
      // stray %
      arg = defaultArg;
      placeholderSize = 1;
  }
  state->format += placeholderSize;
  if (arg) {
    // parameter was not NULL
    appendText(arg, STRING, state->out);
    checkOverflow(state);
  }
}

/*!
 * parses the submitted format string, replacing each placeholder with one of
 * the values in member args of \ref state, and appends the result to the
 * current contents of \ref buffer.
 * \param state struct ParserState* parser state going to be handled and updated
 * \pre \ref initState was called
 */
static void parse(struct ParserState* state) {
  do {
    if (*state->format == MMFATL_PH_PREFIX)
      handleSubstitution(state);
    else
      handleText(state);
  } while (*state->format != NUL);
}

/****    Implementation of the interface in the header file   ****/

/* See the header file for descriptions
 * Global instances buffer and state are known to the following functions.
 * They must not be passed as parameters, as they are implementation details
 * not to be exposed through the interface.
 */

char const* getFatalErrorPlaceholderToken(
                    enum fatalErrorPlaceholderType type) {

  static char result[3];

  switch (type)
  {
    case MMFATL_PH_PREFIX:
    case MMFATL_PH_STRING:
    case MMFATL_PH_UNSIGNED:
      result[0] = MMFATL_PH_PREFIX;
      result[1] = type;
      result[2] = NUL;
      break;
    default:
      return NULL;
  }
  return result;
}

/*!
 * gets the instance of Buffer to use with this interface (currently a global
 * singleton).  The returned instance is not guaranteed to be initialized.
 * \return [not null] a pointer to the Buffer instance
 */
inline static struct Buffer* getBufferInstance(void) {
  return &buffer;
}

/*!
 * gets the instance of ParserState to use with this interface (currently a
 * global singleton).  The returned instance is not guaranteed to be
 * initialized.
 * \return [not null] a pointer to the ParserState instance 
 */
inline static struct ParserState* getParserStateInstance(void) {
  return &state;
}

void fatalErrorInit(void) {
  struct Buffer* buffer = getBufferInstance();

  initBuffer(buffer);
  initState(getParserStateInstance(), buffer);
}

bool fatalErrorPush(char const* format, ...) {
  struct ParserState* state = getParserStateInstance();

  bool overflow = isBufferOverflow(state->out);
  if (!overflow && format != NULL) {
    // initialize the parser state
    state->format = format;
    va_start(state->args, format);
    parse(state);
    overflow = isBufferOverflow(state->out);
    va_end(state->args);
  }

  return !overflow;
}

void fatalErrorPrintAndExit(void) {
  struct Buffer* buffer = getBufferInstance();

  if (!isBufferOverflow(buffer)
      && !isBufferEmpty(buffer)
      && getLastBufferedChar(buffer) != LF)
    fatalErrorPush("\n");
#ifndef TEST_ENABLE // we don't want a test program terminate here
  fputs(buffer->text, stderr);
  exit(EXIT_FAILURE);
#endif
}

void fatalErrorExitAt(char const* file, unsigned line,
                      char const* msgWithPlaceholders, ...) {
  fatalErrorInit();

  // a format for the error location, only showing relevant data
  char const* format = NULL;
  if (file && *file)
  {
    if (line > 0)
      format = "At %s:%u\n";
    else
      format = "In file %s:\n";
  }
  else if (line > 0)
    format = "%sIn line %u:\n";

  if (fatalErrorPush(format, file, line) && msgWithPlaceholders) {
    struct ParserState* state = getParserStateInstance();

    state->format = msgWithPlaceholders;
    va_start(state->args, msgWithPlaceholders);
    parse(state);
    va_end(state->args);
  }

  fatalErrorPrintAndExit();
}


//=================   Regression tests   =====================

#ifdef TEST_ENABLE

static bool test_fatalErrorInit(void) {
  // emulate memory corruption
  memset(&buffer, 'x', sizeof(buffer));
  state.format = NULL;

  fatalErrorInit();

  ASSERT(*state.format == NUL);
  ASSERT(buffer.begin == buffer.text);

  unsigned i = 0;
  // check the buffer is filled with NUL...
  for (; i < MMFATL_MAX_MSG_SIZE; ++i)
    ASSERT(buffer.text[i] == NUL);

  ASSERT(buffer.end == buffer.text + i);

  // ...and has the ELLIPSIS string at the end...
  char ellipsis[] = MMFATL_ELLIPSIS;
  for (unsigned j = 0; ellipsis[j] != NUL; ++i, ++j)
    ASSERT(buffer.text[i] == ellipsis[j]);

  // ... and a terminating NUL character
  ASSERT(buffer.text[i] == NUL);

  return true;
}

// for buffer overflow tests, free space is surrounded by $, so
// limit violations can be detected.
static void limitFreeBuffer(unsigned size) {
  initBuffer(&buffer);
  *buffer.begin++ = '$';
  buffer.end = buffer.begin + size;
  *buffer.end = '$';
  *(buffer.end + 1) = NUL;
}

static bool test_isBufferOverflow(void) {
  fatalErrorInit();

  limitFreeBuffer(0);
  ASSERT(isBufferOverflow(&buffer));
  limitFreeBuffer(1);
  ASSERT(!isBufferOverflow(&buffer));

  char const* format = "abc";
  state.format = format;
  limitFreeBuffer(1);
  ASSERT(checkOverflow(&state));
  ASSERT(state.format == format);
  limitFreeBuffer(0);
  ASSERT(!checkOverflow(&state));
  ASSERT(*state.format == NUL);

  return true;
}

static char const* bufferCompare(char const* match, int from, unsigned lg,
    unsigned begin)
{
  if (memcmp(buffer.begin + from, match, lg) != 0)
    return "unexpected buffer contents";
  return buffer.begin == buffer.text + begin ?
    NULL : "unexpected buffer begin";
}

/*!
 * \param text source text, first character is skipped and indicates its type:
 *   % a format string with special treatment of the MMFATL_PH_PREFIX,
 *   else normal NUL terminated string
 * \param adv that many characters are expected to be copied
 * \param match memory dump of buffer after copy...
 * \param from ... counting from this offset from buffer.begin after copy...
 * \param lg ...and this many characters.
 * \param begin offset of buffer.begin from buffer.text after copy.
 * \return NULL on success, otherwise a message describing a failure
 */
static char const* testcase_appendText(char const* text, unsigned adv,
    char const* match, int from, int lg, unsigned begin) {
  char escape = *text == '%' ? FORMAT : STRING;
  return appendText(text + 1, escape, &buffer) == adv ?
    bufferCompare(match, from, lg, begin) :
    "incorrect number of bytes copied";
}

// wrapper macro to get the function, line number right, and prevent
// further test cases on error
#define TESTCASE_appendText(format, adv, match, from, lg, begin) { \
  char const* errmsg =                                             \
    testcase_appendText(format, adv, match, from, lg, begin);      \
  ASSERTF(errmsg == NULL, "%s\n", errmsg);                         \
}

static bool test_appendText(void) {
  fatalErrorInit();

  // The first character of the source is a type character and skipped:
  // _ STRING, % FORMAT

  // uncomment to deliberately trigger an error message
  // TESTCASE_appendText("_$", 1, "x$", -1, 2, 1);


  // corner case 1: insertion at the very beginning of the buffer
  TESTCASE_appendText("_$", 1, "$", -1, 2, 1);
  // corner case 2: empty text, placeholder handling
  TESTCASE_appendText("%", 0, "$", -1, 2, 1)
  TESTCASE_appendText("%%", 0, "$", -1, 2, 1);
  // non-empty text, placeholder handling
  TESTCASE_appendText("%abc", 3, "$abc", -4, 5, 4);
  TESTCASE_appendText("%def%", 3, "$abcdef", -7, 8, 7);
  TESTCASE_appendText("_gh%i", 4, "$abcdefgh%i", -11, 12, 11);
  // corner case 3: no space left
  limitFreeBuffer(0);
  TESTCASE_appendText("%", 0, "$$", -1, 2, 1);
  TESTCASE_appendText("%%", 0, "$$", -1, 2, 1);
  TESTCASE_appendText("%abc", 0, "$$", -1, 2, 1);
  // corner case 4: truncation in the middle of the text due to overflow
  limitFreeBuffer(1);
  TESTCASE_appendText("_a", 1, "$a$", -2, 3, 2);
  limitFreeBuffer(1);
  TESTCASE_appendText("%def", 1, "$d$", -2, 3, 2);
  limitFreeBuffer(1);
  TESTCASE_appendText("%g%", 1, "$g$", -2, 3, 2);

  return true;
}

static bool test_isBufferEmpty(void) {
  fatalErrorInit();
  ASSERT(isBufferEmpty(&buffer));
  appendText("a", STRING, &buffer);
  ASSERT(!isBufferEmpty(&buffer));
  limitFreeBuffer(0);
  ASSERT(!isBufferEmpty(&buffer));

  return true;  
}

static bool test_getLastBufferedChar(void) {
  fatalErrorInit();
  ASSERT(getLastBufferedChar(&buffer) == NUL);
  appendText("a", STRING, &buffer);
  ASSERT(getLastBufferedChar(&buffer) == 'a');
  limitFreeBuffer(1);
  appendText("bc", STRING, &buffer);
  ASSERT(getLastBufferedChar(&buffer) == 'b');

  return true;  
}

static bool test_handleText(void) {
  fatalErrorInit();
  char const* format = state.format;
  // no format
  handleText(&state);
  ASSERT(strcmp(buffer.text, "") == 0);
  ASSERT(format == state.format);

  state.format = "abc";
  handleText(&state);
  ASSERT(strcmp(buffer.text, "abc") == 0);
  ASSERT(*state.format == NUL);

  state.format = "abc%s";
  handleText(&state);
  ASSERT(strcmp(buffer.text, "abcabc") == 0);
  ASSERT(*state.format == '%');

  limitFreeBuffer(1);
  state.format = "%s";
  handleText(&state);
  ASSERT(*state.format == '%');
  ASSERT(buffer.begin == buffer.text + 1);  
  
  state.format = "abc";
  handleText(&state);
  ASSERT(strcmp(buffer.text, "$a$") == 0);
  ASSERT(*state.format == NUL);

  return true;
}

bool test_unsignedToString(void)
{
  ASSERT(strcmp(unsignedToString(0), "0") == 0);
  ASSERT(strcmp(unsignedToString(123), "123") == 0);
  // test max unsigned by converting back and forth
  ASSERT(strtoul(unsignedToString(~0u), NULL, 10) == ~0u);

  return true;
}

static bool test_handleSubstitution1(char const* format, ...) {
  va_start(state.args, format);
  
  // without initializing the buffer each test appends
  // to the result of the former test.

  // %s NULL
  fatalErrorInit();
  state.format = format;
  handleSubstitution(&state);
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "") == 0);

  // %s ""
  handleSubstitution(&state);
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "") == 0);

  // %s "abc"
  handleSubstitution(&state);
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "abc") == 0);

  // %s "%s"
  handleSubstitution(&state);
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "abc%s") == 0);

  // %u 0
  handleSubstitution(&state);
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "abc%s0") == 0);

  // %u 123
  handleSubstitution(&state);
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "abc%s0123") == 0);

  // %u ~0u
  initBuffer(&buffer);
  handleSubstitution(&state);
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strtoul(buffer.text, NULL, 10) == ~0u);

  // case buffer overflow
  limitFreeBuffer(1);
  handleSubstitution(&state);
  format += 2;
  ASSERT(*state.format == NUL);
  char const* errmsg = bufferCompare("$o$", -2, 3, 2);
  ASSERTF(errmsg == NULL, "%s\n", errmsg);

  // %%
  initBuffer(&buffer);
  state.format = format;
  handleSubstitution(&state);
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "%") == 0);

  // %;
  handleSubstitution(&state);
  ++state.format; // skip the ;
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "%%") == 0);

  // %<NUL>
  handleSubstitution(&state);
  ++format;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "%%%") == 0);

  va_end(state.args);
  return true;
}

bool test_handleSubstitution(void) {
  return test_handleSubstitution1(
    "%s%s%s%s%u%u%u%s%%%;%",
    NULL, "", "abc", "%s", 0, 123, ~0u, "overflow");
}

static char const* testcase_parse(char const* expect, char const* format, ...) {
  fatalErrorInit();
  va_start(state.args, format);
  state.format = format;
  parse(&state);
  va_end(state.args);
  return strcmp(buffer.text, expect) == 0?
    NULL 
    : "text mismatch";
}

// wrapper macro to get the function, line number right, and prevent
// further test cases on error
#define TESTCASE_parse(expect, format, ...) {    \
  char const* errmsg =                           \
    testcase_parse(expect, format, __VA_ARGS__); \
  ASSERTF(errmsg == NULL, "%s\n", errmsg);       \
}

static bool test_parse(void) {
  ASSERT(testcase_parse("", "") == NULL);
  ASSERT(testcase_parse("abc", "abc") == NULL);

  TESTCASE_parse("abc", "%s", "abc");
  TESTCASE_parse("123", "%u", 123);
  TESTCASE_parse("123abc", "%u%s", 123, "abc");
  TESTCASE_parse("123%abc", "%u%%%s", 123, "abc");
  TESTCASE_parse("XY123ABabcST", "XY%uAB%sST", 123, "abc");
  // buffer overflow
  limitFreeBuffer(0);
  state.format = "";
  parse(&state);
  ASSERT(strcmp(buffer.text, "$$") == 0);
  limitFreeBuffer(1);
  state.format = "123";
  parse(&state);
  ASSERT(strcmp(buffer.text, "$1$") == 0);

  return true;
}

static bool test_getFatalErrorPlaceholderToken(void) {
  char const* result = getFatalErrorPlaceholderToken(MMFATL_PH_PREFIX);
  ASSERT(result && strcmp(result, "%%") == 0);
  result = getFatalErrorPlaceholderToken(MMFATL_PH_UNSIGNED);
  ASSERT(result && strcmp(result, "%u") == 0);
  ASSERT(getFatalErrorPlaceholderToken(
      (enum fatalErrorPlaceholderType)NUL) == NULL);
  return true;
}

static bool test_fatalErrorPush() {
  fatalErrorInit(); // pre-condition

  // case format NULL or empty (do nothing)
  ASSERT(fatalErrorPush(NULL));
  ASSERT(strcmp(buffer.text, "") == 0);
  ASSERT(fatalErrorPush(""));
  ASSERT(strcmp(buffer.text, "") == 0);

  // simple message, no placeholder
  ASSERT(fatalErrorPush("abc"));
  ASSERT(strcmp(buffer.text, "abc") == 0);

  // message with placeholders, appended
  ASSERT(fatalErrorPush("x%sy%uz", "--", 123));
  ASSERT(strcmp(buffer.text, "abcx--y123z") == 0);

  // overflow
  limitFreeBuffer(2);
  ASSERT(!fatalErrorPush("abc"));
  ASSERT(strcmp(buffer.text, "$ab$") == 0);

  ASSERT(!fatalErrorPush(NULL));
  ASSERT(!fatalErrorPush(""));
  ASSERT(strcmp(buffer.text, "$ab$") == 0);

  return true;
}

static bool test_fatalErrorPrintAndExit(void) {
  fatalErrorInit(); // pre-condition
  fatalErrorPrintAndExit();
  ASSERT(strcmp(buffer.text, "") == 0);
  
  fatalErrorPush("aaa");
  fatalErrorPrintAndExit();
  ASSERT(strcmp(buffer.text, "aaa\n") == 0);
  
  // no second \n is appended 
  fatalErrorPrintAndExit();
  ASSERT(strcmp(buffer.text, "aaa\n") == 0);
  
  // in overflow condition do not append a LF
  fatalErrorInit();
  while (!isBufferOverflow(&buffer)) // fill the buffer with "a"
    fatalErrorPush("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
  ASSERT(strcmp(buffer.text + MMFATL_MAX_MSG_SIZE - 3, "aaa" MMFATL_ELLIPSIS) == 0);

  return true;
}

static bool test_fatalErrorExitAt() {
  // note that in test mode the fatalErrorExitAt neither prints to stderr
  // nor exits.  The message is still in the buffer.

  fatalErrorExitAt("test.c", 1000, "%s failed!", "program");
  ASSERT(strcmp(buffer.text, "At test.c:1000\nprogram failed!\n") == 0);
  // ignoring line
  fatalErrorExitAt("x.c", 0, "test %u failed!", 5);
  ASSERT(strcmp(buffer.text, "In file x.c:\ntest 5 failed!\n") == 0);
  // ignoring file
  fatalErrorExitAt(NULL, 123, "%s", "need help!\n");
  ASSERT(strcmp(buffer.text, "In line 123:\nneed help!\n") == 0);

  // ignoring error location
  fatalErrorExitAt(NULL, 0, "take lessons, you fool!");
  ASSERT(strcmp(buffer.text, "take lessons, you fool!\n") == 0);

  return true;
}


void test_mmfatl(void) {
  RUN_TEST(test_unsignedToString);
  RUN_TEST(test_fatalErrorInit);
  RUN_TEST(test_isBufferOverflow);
  RUN_TEST(test_appendText);
  RUN_TEST(test_isBufferEmpty);
  RUN_TEST(test_getLastBufferedChar);
  RUN_TEST(test_handleText);
  RUN_TEST(test_parse);
  RUN_TEST(test_handleSubstitution);
  RUN_TEST(test_getFatalErrorPlaceholderToken);
  RUN_TEST(test_fatalErrorPush);
  RUN_TEST(test_fatalErrorPrintAndExit);
  RUN_TEST(test_fatalErrorExitAt);
}

#endif // TEST_ENABLE
