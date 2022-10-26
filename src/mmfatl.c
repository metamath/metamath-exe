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
 * compatible.
 */

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "mmfatl.h"

/*!
 * string terminating character
 */
#define NUL '\x00'

/*!
 * format placeholder character, not NUL
 */
#define PLACEHOLDER_CHAR '%'
#define PLACEHOLDER_STRING "%"
/*!
 * marks a string type in a placeholder substitution
 */
#define PLACEHOLDER_TYPE_STRING 's'

/*!
 * marks an unsigned type in a placeholder substitution
 */
#define PLACEHOLDER_TYPE_UNSIGNED 'u'


//----------

/*
 * During development you may not want to expose preliminary results to the
 * normal compile, as this would trigger 'unused' warnings, for example.  In
 * the regression test environment your code may be referenced by testing code,
 * though.
 *
 * This section should be empty, or even removed, once your development is
 * finished.
 */
#ifdef TEST_ENABLE
#   define UNDER_DEVELOPMENT
#endif

#ifdef UNDER_DEVELOPMENT

/* the size a fatal error message including the terminating NUL character can
 * assume without truncation.
 */
#define BUFFERSIZE 1024

/* the character sequence appended to a truncated fatal error message, so a
 * user is aware a displayed message is incomplete.
 */
#define ELLIPSIS "..."

/*!
 * converts an unsigned to a sequence of decimal digits representing its value.
 * The value range is known to be at least 2**32 on contemporary hardware.
 * We support unsigned in formatted error output to allow for macros like
 * __LINE__ denoting error positions in text files.
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
  char* begin; /*!< points to first unallocated character */
  /*!
   * marks the end of the writeable portion, where the \ref ELLIPSIS begins.
   * Logically constant once it got initialized.  Never overwrite this value.
   */
  char* end;
  /*!
   * the writeable buffer, followed by fixed text indicating truncation if
   * necessary.
   */
  char text[BUFFERSIZE + sizeof(ELLIPSIS)];
};

/*!
 * a preallocated buffer supporting formatting.
 */
static struct Buffer buffer;

/*
 * We do not rely on any initialization during program start.  Instead we
 * assume the worst case, a corrupted pointer overwrote the buffer.  So we
 * initialize it again immediately before use.
 */
static void initBuffer(void) {
  char ellipsis[] = ELLIPSIS;

  buffer.begin = buffer.text;
  buffer.end = buffer.text + BUFFERSIZE;
  memset(buffer.begin, NUL, BUFFERSIZE);
  memcpy(buffer.end, ellipsis, sizeof(ellipsis));
}

/*!
 * used to indicate whether \ref PLACEHOLDER_CHAR is a normal character, or
 * is an escape character in a format string.
 */
enum TextType {
  STRING, //<! NUL terminated text
  FORMAT  //<! NUL terminated format containing placeholders
};

/*!
 * append characters to the current end of the buffer from a format string
 * until a terminating NUL or PLACEHOLDER_CHAR is encountered, or the buffer
 * overflows.  A terminating character is not copied.
 * \param source [not null] the source from which bytes are copied.
 * \param type if \ref FORMAT, a PLACEHOLDER_CHAR is interpreted as a possible
 *   insertion point of other data, and stops the copy process.
 * \return a pointer to the character following the last one copied.
 */
static char const* appendText(char const* source, enum TextType type) {
  char escape = type == FORMAT ? PLACEHOLDER_CHAR : NUL;
  while (buffer.begin != buffer.end && *source != NUL && *source != escape)
    *buffer.begin++ = *source++;
  return source;
}

/*!
 * A simple grammar scheme allows inserting data in a prepared general message.
 * The scheme is a downgrade of the C format string.  Allowed are only
 * placeholders %s and %u that are replaced with given data.  The percent sign
 * % is available through duplication %%.  For this kind of grammar 4
 * separate process states are sufficient.
 */

struct ParserInput {
    /*! the next reading position in the format string.  Will be consumed, i.e.
     scanned only once from begin to end. */
    char const* format;
    /* the list of parameters substituting placeholders.  This structure allows
     * traversing all entries. */
    va_list args;
};

static struct ParserInput state;

/*!
 * A format specifier is a two character combination, where a PLACEHOLDER_CHAR
 * is followed by an alphabetic character designating a type.  A placeholder
 * is substituted by the next argument in member *args* of \ref state.  This
 * function handles this substitution when member *format* of \ref state points
 * to a placeholder.
 *
 * This function also handles the case where the type character is missing or
 * invalid.  Usually we want to log the presence of stray PLACEHOLDER_CHAR
 * characters in a format string to some file, or at least display a warning.
 * But we are in the process of a fatal error already, so we gracefully accept
 * garbage and simply copy that character to the output, in the hope, misplaced
 * characters are somehow noticed there.
 * Note that a duplicated PLACEHOLDER_CHAR is the accepted way to embed such a
 * character in a format string.  This is correctly handled in this function.
 * \pre the format member in \ref state points to a PLACEHOLDER_CHAR.
 * \post the substituted value is written to \ref buffer.
 * \post the format member in \ref state is skipped
 */
static void handleSubstitution(void) {
  char const* arg = PLACEHOLDER_STRING;  // default: no substitution case
  int advFormatPtr = 2; // advance by this many characters
  switch (*(state.format + 1)) {
    case PLACEHOLDER_TYPE_STRING:
      arg = va_arg(state.args, char const*);
      break;
    case PLACEHOLDER_TYPE_UNSIGNED:
      // a %u format specifier is recognized. 
      arg = unsignedToString(va_arg(state.args, unsigned));
      break;
    case PLACEHOLDER_CHAR:
      // %%
      break;
    default:;
      // stray %
      advFormatPtr = 1;
  }
  if (arg != NULL)
    appendText(arg, STRING);
  state.format += advFormatPtr;
}

#endif // UNDER_DEVELOPMENT


//=================   Regression tests   =====================

#ifdef TEST_ENABLE

bool test_initBuffer(void) {
  // emulate memory corruption
  memset(&buffer, 'x', sizeof(buffer));

  initBuffer();

  ASSERT(buffer.begin == buffer.text);
  unsigned i = 0;

  // check the buffer is filled with NUL...
  for (; i < BUFFERSIZE; ++i)
    ASSERT(buffer.text[i] == NUL);

  ASSERT(buffer.end == buffer.text + i);

  // ...and has the ELLIPSIS string at the end...
  char ellipsis[] = ELLIPSIS;
  unsigned j = 0;
  for (; ellipsis[j] != NUL; ++i, ++j)
    ASSERT(buffer.text[i] == ellipsis[j]);

  // ... and a terminating NUL character
  ASSERT(buffer.text[i] == NUL);
  return true;
}

char const* bufferCompare(char const* match, int from, unsigned lg,
    unsigned begin)
{
  if (memcmp(buffer.begin + from, match, lg) != 0)
    return "unexpected buffer contents";
  return buffer.begin == buffer.text + begin ?
    NULL : "unexpected buffer begin";
}

/*
 * \param text source text, first character is skipped and indicates its type:
 *   % a format string with special treatment of the PLACEHOLDER_CHAR, else
 *   normal NUL terminated string
 * \param adv that many characters are expected to be copied
 * \param match memory dump of buffer after copy...
 * \param from ... counting from this offset from buffer.begin after copy...
 * \param lg ...and this many characters.
 * \param begin offset of buffer.begin from buffer.text after copy.
 * \return NULL on success, otherwise a message describing a failure
 */
char const* testcase_appendText(char const* text, unsigned adv,
    char const* match, int from, int lg, unsigned begin)
{
  enum TextType type = *text == PLACEHOLDER_CHAR ? FORMAT : STRING;
  return appendText(text + 1, type) == text + adv + 1 ?
    bufferCompare(match, from, lg, begin) :
    "format pointer not properly advanced";
}

// wrapper macro to get the function, line number right, and prevent
// further test cases on error
#define TESTCASE_appendText(format, adv, match, from, lg, begin) { \
  char const* errmsg =                                             \
    testcase_appendText(format, adv, match, from, lg, begin);      \
  ASSERTF(errmsg == NULL, "%s\n", errmsg);                         \
}

bool test_appendText(void) {
  initBuffer();

  // uncomment to deliberately trigger an error message
  // TESTCASE_appendText("_$", 1, "x$", -1, 2, 1);


  // The first character of the source is a type character and skipped:
  // _ STRING, % FORMAT
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
  buffer.begin = buffer.end;
  *(buffer.end - 1) = '$';
  *(buffer.end) = '$';
  TESTCASE_appendText("%", 0, "$$", -1, 2, BUFFERSIZE);
  TESTCASE_appendText("%%", 0, "$$", -1, 2, BUFFERSIZE);
  TESTCASE_appendText("%abc", 0, "$$", -1, 2, BUFFERSIZE);
  // corner case 4: truncation in the middle of the text due to overflow
  *(buffer.end - 2) = '$';
  buffer.begin = buffer.end - 1;
  TESTCASE_appendText("_a", 1, "$a$", -2, 3, BUFFERSIZE);
  buffer.begin = buffer.end - 1;
  TESTCASE_appendText("%def", 1, "$d$", -2, 3, BUFFERSIZE);
  buffer.begin = buffer.end - 1;
  TESTCASE_appendText("%g%", 1, "$g$", -2, 3, BUFFERSIZE);

  return true;
}

bool test_unsignedToString(void)
{
  ASSERT(strcmp(unsignedToString(0), "0") == 0);
  ASSERT(strcmp(unsignedToString(123), "123") == 0);
  // test max unsigned by converting back and forth
  ASSERT(strtoul(unsignedToString(~0u), NULL, 10) == ~0u);
}

bool test_handleSubstitution1(int dummy, ...) {
  char const* format = state.format;
  va_start(state.args, dummy);
  
  // without initializing the buffer each test appends
  // to the result of the former test.

  // %s NULL
  initBuffer();
  handleSubstitution();
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "") == 0);

  // %s ""
  handleSubstitution();
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "") == 0);

  // %s "abc"
  handleSubstitution();
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "abc") == 0);

  // %s "%s"
  handleSubstitution();
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "abc%s") == 0);

  // %u 0
  handleSubstitution();
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "abc%s0") == 0);

  // %u 123
  handleSubstitution();
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "abc%s0123") == 0);

  // %u ~0u
  initBuffer();
  handleSubstitution();
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strtoul(buffer.text, NULL, 10) == ~0u);

  // case buffer overflow
  buffer.begin = buffer.end;
  *(buffer.end - 1) = '$';
  *buffer.end = '$';
  handleSubstitution();
  format += 2;
  ASSERT(format == state.format);
  char const* errmsg = bufferCompare("$$", -1, 2, BUFFERSIZE);
  ASSERTF(errmsg == NULL, "%s\n", errmsg);

  // %%
  initBuffer();
  handleSubstitution();
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "%") == 0);

  // %;
  handleSubstitution();
  ++state.format; // skip the ;
  format += 2;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "%%") == 0);

  // %<NUL>
  handleSubstitution();
  ++format;
  ASSERT(format == state.format);
  ASSERT(strcmp(buffer.text, "%%%") == 0);

  return true;
}

bool test_handleSubstitution(void) {
  state.format = "%s%s%s%s%u%u%u%s%%%;%";
  bool result = test_handleSubstitution1(0,
    NULL, "", "abc", "%s", 0, 123, ~0u, "overflow");
  va_end(state.args);
  return result;
}

void test_mmfatl(void) {
  RUN_TEST(test_initBuffer);
  RUN_TEST(test_appendText);
  RUN_TEST(test_unsignedToString);
  RUN_TEST(test_handleSubstitution);
}

#endif // TEST_ENABLE
