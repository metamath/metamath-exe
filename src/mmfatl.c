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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mmfatl.h"

/*!
 * string terminating character
 */
#define NUL '\x00'

/*!
 * format placeholder character
 */
#define PLACEHOLDER_CHAR '%'

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
#if TEST_ENABLE
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
enum TextType
{
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
static char const* appendText(char const* source, enum TextType type)
{
  char escape = type == FORMAT ? PLACEHOLDER_CHAR : NUL;
  while (buffer.begin != buffer.end
      && *source != NUL && *source != escape)
        *buffer.begin++ = *source++;
  return source;
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
  for (;i < BUFFERSIZE; ++i)
    ASSERT(buffer.text[i] == NUL);

  ASSERT(buffer.end == buffer.text + i)

  // ...and has the ELLIPSIS string at the end...
  char ellipsis[] = ELLIPSIS;
  unsigned j = 0;
  for (;ellipsis[j] != NUL; ++i, ++j)
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
                bufferCompare(match, from, lg, begin)
                : "format pointer not properly advanced";
}

// wrapper macro to get the function, line number right, and prevent
// further test cases
#define TESTCASE_appendText(format, adv, match, from, lg, begin)  \
  {                                                               \
    char const* errmsg =                                          \
        testcase_appendText(format, adv, match, from, lg, begin); \
    ASSERTF(errmsg == NULL, "%s\n", errmsg);                      \
  }

bool test_appendText()
{
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
  // corner case 3: checking for buffer overflow
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

void test_mmfatl(void) {
  RUN_TEST(test_initBuffer);
  RUN_TEST(test_appendText);
}

#endif // TEST_ENABLE
