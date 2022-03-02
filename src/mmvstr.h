/*****************************************************************************/
/*        Copyright (C) 2011  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
\file mmvstr.h - VMS-BASIC variable length string library routines header
This is an emulation of the string functions available in VMS BASIC.
[VAX Basic](http://bitsavers.org/pdf/dec/vax/lang/basic/AA-HY16B-TE_VAX_BASIC_Reference_Manual_Feb90.pdf)
*/

/******************************************************************************

Variable-length string handler
------------------------------

     This collection of string-handling functions emulate most of the
string functions of VMS BASIC.  The objects manipulated by these
functions are strings of a special type called 'vstring' which have no
pre-defined upper length limit but are dynamically allocated and
deallocated as needed.  To use the vstring functions within a program,
all vstrings must be initially set to the null string when declared or
before first used, for example:

        vstring_def(string1);
        vstring stringArray[] = {"", "", ""};

        vstring bigArray[100][10]; /- Must be initialized before using -/
        int i, j;
        for (i = 0; i < 100; i++)
          for (j = 0; j < 10; j++)
            bigArray[i][j] = ""; /- Initialize -/


     After initialization, vstrings should be assigned with the 'let(&'
function only; for example the statements

        let(&string1, "abc");
        let(&string1, string2);
        let(&string1, left(string2, 3));

all assign the second argument to 'string1'.  The 'let(&' function must
_not_ be used to initialize a vstring the first time.

     Any local vstrings in a function must be deallocated before returning
from the function, otherwise there will be memory leakage and eventual
memory overflow.  To deallocate, assign the vstring to "" with 'free_vstring':

        void abc(void) {
          vstring_def(xyz);
          ...
          free_vstring(xyz);
        }

     The 'cat' function emulates the '+' concatenation operator in BASIC.
It has a variable number of arguments, and the last argument should always
be NULL.  For example,

        let(&string1,cat("abc","def",NULL));

assigns "abcdef" to 'string1'.  Warning: 0 will work instead of NULL on the
VAX but not on the Macintosh, so always use NULL.

     All other functions are generally used exactly like their BASIC
equivalents.  For example, the BASIC statement

        let string1$=left$("def",len(right$("xxx",2)))+"ghi"+string2$

is emulated in c as

        let(&string1,cat(left("def",len(right("xxx",2))),"ghi",string2,NULL));

Note that ANSI c does not allow "$" as part of an identifier
name, so the names in c have had the "$" suffix removed.

     The string arguments of the vstring functions may be either standard c
strings or vstrings (except that the first argument of the 'let(&' function
must be a vstring).  The standard c string functions may use vstrings or
vstring functions as their string arguments, as long as the vstring variable
itself (which is a char * pointer) is not modified and no attempt is made to
increase the length of a vstring.  Caution must be exercised when
assigning standard c string pointers to vstrings or the results of
vstring functions, as the memory space may be deallocated when the
'let(&' function is next executed.  For example,

        char *stdstr; /- A standard c string pointer -/
         ...
        stdstr=left("abc",2);

will assign "ab" to 'stdstr', but this assignment will be lost when the
next 'let(&' function is executed.  To be safe, use 'strcpy':

        char stdstr1[80]; /- A fixed length standard c string -/
         ...
        strcpy(stdstr1,left("abc",2));

Here, of course, the user must ensure that the string copied to 'stdstr1'
does not exceed 79 characters in length.

     The vstring functions allocate temporary memory whenever they are called.
This temporary memory is deallocated whenever a 'let(&' assignment is
made.  The user should be aware of this when using vstring functions
outside of 'let(&' assignments; for example

        for (i=0; i<10000; i++)
          print2("%s\n",left(string1,70));

will allocate another 70 bytes or so of memory each pass through the loop.
If necessary, 'freeTempAlloc' can be used periodically to clear
this temporary memory:

        for (i=0; i<10000; i++) {
          print2("%s\n",left(string1,70));
          freeTempAlloc();
        }

It should be noted that the 'linput' function assigns its target string
with 'let(&' and thus has the same effect as 'let(&'.

******************************************************************************/


#ifndef METAMATH_MMVSTR_H_
#define METAMATH_MMVSTR_H_

#include <stdio.h>

/* A vstring is like a C string, but it contains a control block allowing
for memory allocation. New vstrings should always be constructed from the
`vstring_def` macro. */
/*!
 * \typedef vstring
 * \brief contains NUL terminated,  character oriented data
 *
 * - A vstring is never NULL;
 * - If the text is empty, i.e. the pointer points to the terminating 0x00
 *   character, then its contents is not allocated memory and not mutable
 *   instead;
 * - If not empty, i.e. the pointer points to a character different from
 *   0x00, then this is never a true left portion of another \a vstring.
 * - Although not required under all circumstances, it is highly recommended to
 *   uniquely point to some allocated memory only.
 *
 * You can use a vstring to read the associated text, but you must never write
 * to memory pointed to by a vstring directly, nor may you change the pointer's
 * value.  Declaration, definition and write access to a vstring, or the text it
 * points to, is exclusively done through dedicated functions.  Although the
 * encoding of the text (or whatever data it is) requires only the existence of
 * exactly one 0x00 at the end, using ASCII, or at least UTF-8, is recommended
 * to allow various print instructions.
 */
typedef char* vstring;

/* A vstring is allocated in temporary storage. These strings will be deallocated
  after the next call to `let`.

  A `temp_vstring` should never be used as the first argument of a `let`.
  This code is INCORRECT:

    temp_vstring foo = left("foobar", 3);
    let(&foo, "bar"); // this will cause a double free

  It is okay (and quite common) to use a temp_vstring as the second argument,
  however. It is best not to hold on to the value, though, because the `let`
  will free it. This code is INCORRECT:

    vstring_def(x);
    temp_vstring foobar = cat("foo", "bar");
    let(&x, foobar); // frees foobar
    let(&x, foobar); // dangling reference

  There is a converse problem when `temp_vstring`s are used without `let`:

    for (int i = 0; i < 100000; i++) {
      vstring_def(x);
      if (strlen(space(i)) == 99999) break;
    }

  We don't need to deallocate the string returned by `space()` directly,
  because it returns a `temp_vstring`, but because there is no `let` in
  this function, we end up allocating a bunch of temporaries and
  effectively get a memory leak. (There is space for about 100
  temporaries so this loop will cause a crash.) To solve this problem,
  we can either use a dummy `let()` statement in the loop, or call
  `freeTempAlloc` directly:

    for (int i = 0; i < 100000; i++) {
      vstring_def(x);
      if (strlen(space(i)) == 99999) break;
      freeTempAlloc();
    }

*/
/*!
 * \typedef temp_vstring
 *
 * This alias for \a vstring is used to mark an entry in the \a tempAllocStack.
 * Entries in this stack are subject to automatic deallocation by \a let or
 * calling \a freeTempAlloc.
 *
 * Unlike \a vstring this type knows no exceptional handling of empty strings.
 * If an empty string is generated as a temporary in the course of a
 * construction of a final \a vstring, it is allocated on the heap as usual.
 *
 * If returned by a function, it is already pushed on the \a tempAllocStack.
 */
typedef vstring temp_vstring;

/*!
 * \def vstring_def
 * \brief creates a new \a vstring variable.
 *
 * declares a \a vstring variable and initiates it with empty text ("").
 * If it remains unmodified, freeing of __x__ is possible, but not required.
 *
 * \param[in] x plain C variable name without quote characters.
 * \pre
 *   the variable has not been declared before in the current scope.
 * \post
 *   initialized with empty text.  No administrative data is added, in
 *   conformance with the semantics of a \a vstring.
 */
#define vstring_def(x) vstring x = ""
/*!
 * \def free_vstring
 * \brief deallocates a \a vstring variable and sets it to the empty string.
 *
 * Multiple invocations on the same variable is possible.  Can be reused again
 * without a call to \a vstring_def.
 *
 * Side effect: Frees and pops off entries on and beyond index
 * \a g_startTempAllocStack from the \a tempAllocStack.
 *
 * \param[in,out] x (not null) an initialized \a vstring variable.  According to the
 * semantics of a \a vstring, __x__ is not deallocated, if it points to an
 * empty string. 
 * \pre
 *   __x__ was declared and initialized before.
 * \post
 *   __x__ initialized with empty text.  Entries on and beyond index
 * \a g_startTempAllocStack are freed and popped off the \a tempAllocStack.
 */
#define free_vstring(x) let(&x, "")

/* Free space allocated for temporaries. This is normally called automatically
  by let(), but it can also be called directly to avoid buildup of temporary
  strings. */
/*!
 * \fn freeTempAlloc
 * \brief Free space allocated for temporary vstring instances.
 *
 * Temporary \a vstring in \a tempAllocStack are used for example to construct
 * final text from patterns, boilerplate etc. along with data to be filled in.
 *
 * This function frees all entries beginning with \a g_startTempAllocStack.
 * It is usually called automatically by let(), but can also be invoked
 * directly to avoid buildup of temporary strings.
 *
 * \pre
 *   \a g_startTempAllocStack is the index in \a tempAllocStack from which
 *   on enties are freed.
 * \post
 * - Entries in \a tempAllocStack from index \a g_startTempAllocStack on
 *   are freed.  The top of stack \a g_tempAllocStackTop is back to
 *   \a g_startTempAllocStack again, so the current scope of temporaries is
 *   empty;
 * - db1 is updated, if NDEBUG is not defined.
 */
void freeTempAlloc(void);

/* `source` must not point into `target` (but this is unlikely to arise if
   `source` is calculated using `temp_vstring` operations from `target`). */
/*!
 * \fn let(vstring *target, const char *source)
 * \brief emulation of BASIC string assignment
 *
 * assigns to text to a \a vstring pointer.  This includes a bit of memory
 * management.  Not only is the space of the destination of the assignment
 * reallocated if its previous size was too small.  But in addition the
 * \ref stack "stack" \a tempAllocStack is freed of intermediate values again.
 * Every entry on and beyond \a g_startTempAllocStack is considered to be
 * consumed and subject to deallocation.
 *
 * This deallocation procedure is embedded in this operation, since frequently
 * the final string was composed of some fragments, that now can be disposed
 * of.  In fact, this function must ALWAYS be called to assign to a vstring in
 * order for the memory cleanup routines, etc. to work properly.  A new vstring
 * should be initialized to "" (the empty string), and the 'vstring_def' macro
 * handles creation of such variables.
 *
 * Possible failures: Out of memory condition.
 *
 * \param[out] target (not null) address of a \a vstring receiving a copy of
 *   the source string.  Its current value, if not empty, must never point to a
 *   true portion of another \a vstring.  It must not coincide with any of the
 *   temporary strings in \a tempAllocStack, from index
 *   \a g_startTempAllocStack on.  You can assign to an entry with index below
 *   this value, though.
 * \param[in] source (not null) NUL terminated string to be copied from.
 *
 * \pre
 * - \a g_startTempAllocStack contains the starting index of entries in
 *   \a tempAllocStack, that is going to be deallocated.
 * - both parameters are not null and point to NUL terminated strings.
 * - The destination of this function must either be empty, or uniquely point
 *   to a \a vstring, but not any of the temp_vstring;
 * - The destination need not provide enough space for the source.  If
 *   necessary, it is reallocated to point to a larger chunk of memory;
 * \post
 * - Entries in \a tempAllocStack from \a g_startTempAllocStack (on entry to the
 *   function) are deallocated;
 * - The stack pointer in \a g_tempAllocStackTop is set to
 *   \a g_startTempAllocStack (on entry to the function);
 * - If the assigned value is the empty string, but the destination not, it is
 *   freed and assigned to a constant "";
 * - \a db is updated.
 */
void let(vstring *target, const char *source);

/* Emulation of BASIC string concatenation - last argument MUST be NULL */
/* vstring cat(vstring string1, ..., stringN, NULL); */
/* e.g. 'let(&abc, cat("Hello", " ", left("worldx", 5), "!", NULL);' */
/* Also the first string must not be `NULL`, i.e. `cat(NULL)` alone is invalid. */
/*!
 * \fn temp_vstring cat(const char * string1, ...)
 * \brief concatenates several NUL terminated strings to a single string.
 *
 * Up to MAX_CAT_ARGS - 1 (49) NUL terminated strings submitted as parameters
 * are concatenated to form a single NUL terminated string.  The parameters
 * terminate with a NULL pointer, a single NULL pointer is not allowed, though.
 * The resulting string is pushed on \a tempAllocStack.
 * \param[in] string1 (not null) a pointer to a NUL terminated string.
 *   The following parameters are pointers to NUL terminated strings as well,
 *   except for the last parameter that must be NULL.  It is allowed to
 *   duplicate parameters.
 * \return the concatenated string terminated by a NUL character.
 * \post the resulting string is pushed onto the \a tempAllocStack.
 *   \a db is updated.
 * \bug a stack overflow of \a tempAllocStack is not handled correctly.
 */
temp_vstring cat(const char * string1, ...);

/* Emulation of BASIC linput (line input) statement; returns NULL if EOF */
/* Note that linput assigns target string with let(&target,...) */
  /*
    BASIC:  linput "what";a$
    c:      linput(NULL,"what?",&a);

    BASIC:  linput #1,a$                        (error trap on EOF)
    c:      if (!linput(file1,NULL,&a)) break;  (break on EOF)

  */
/* returns whether a (possibly empty) line was successfully read */
int linput(FILE *stream, const char *ask, vstring *target);

/* Emulation of BASIC string functions */
/* Indices are 1-based */
/*!
 * \fn temp_vstring seg(const char *sin, long p1, long p2)
 * Extracts a substring from a source and pushes it on \a tempAllocStack.
 * Note: The bounding indices are 1-based and inclusive.
 *
 * \param[in] sin (not null) pointer to the NUL-terminated source text.
 * \param[in] p1 offset of the first byte of the substring, counted in bytes from
 *   the first one of __sin__, a 1-based index.  A value less than 1 is
 *   internally corrected to 1, but it must not point beyond the terminating
 *   NUL of __sin__, if __p1__ <= __p2__.
 * \param[in] p2 offset of the last byte of the substring, counted in bytes from
 *   the first one of __sin__, a 1-based index.  The natural bounds of this
 *   value are __p1__ - 1 and the length of __sin__.  Values outside of this
 *   range are internally corrected to the closer of these limits.  If __p2__ <
 *   __p1__ the empty string is returned.
 * \attention the indices are 1-based: seg("hello", 2, 3) == "el"!
 * \return a pointer to new allocated \a temp_vstring referencing the
 *   requested substring, that is also pushed onto the top of \a tempAllocStack
 * \pre
 *   __p1__ <= length(__sin__).
 * \post
 *   A pointer to the substring is pushed on \a tempAllocStack, even if it
 *   empty;
 * \warning not UTF-8 safe.
 * \bug a stack overflow of \a tempAllocStack is not handled correctly;
 */
temp_vstring seg(const char *sin, long p1, long p2);
/*!
 * \fn temp_vstring mid(const char *sin, long p, long l)
 * Extracts a substring from a source and pushes it on \a tempAllocStack
 *
 * \param[in] sin (not null) pointer to the NUL-terminated source text.
 * \param[in] p offset of the substring in bytes from the first byte of __sin__,
 *   1-based.  A value less than 1 is internally corrected to 1, but it must
 *   never point beyond the terminating NUL of __sin__.
 * \param[in] l length of substring in bytes.  Negative values are corrected to 0.
 *   If __p__ + __l__ exceeds the length of __sin__, then only the portion up
 *   to the terminating NUL is taken.
 * \attention the index __p__ is 1-based: mid("hello", 2, 1) == "e"!
 * \return a pointer to new allocated \a temp_vstring referencing the
 *   requested substring, that is also pushed onto the top of \a tempAllocStack
 * \pre
 *   __p__ <= length(__sin__).  This must hold even if the requested length is
 *   0, because its implementation in C requires the validity of the pointer,
 *   even if it is not dereferenced.
 * \post
 *   A pointer to the substring is pushed on \a tempAllocStack, even if it
 *   empty;
 * \warning not UTF-8 safe.
 * \bug a stack overflow of \a tempAllocStack is not handled correctly;
 */
temp_vstring mid(const char *sin, long p, long l);
/*!
 * \fn temp_vstring left(const char *sin, long n)
 * \brief Extract leftmost n characters.
 *
 * Copies the leftmost n bytes of a NUL terminated string to a temporary.
 * If the source contains UTF-8 encoded text, care has to be taken that a
 * multi-byte character is not split in this process.
 *
 * \param[in] sin (not null) pointer to a NUL terminated string to be copied from.
 * \param[in] n count of bytes to be copied from the source.  The natural bounds of
 *   this value is 0 and the length of __sin__ in bytes.  Any value outside of
 *   this range is corrected to the closer one of these limits.
 * \return a pointer to new allocated \a temp_vstring referencing the
 *   requested portion, that is also pushed onto the top of \a tempAllocStack
 * \post
 *   A pointer to the substring is pushed on \a tempAllocStack, even if it
 *   empty.
 * \warning not UTF-8 safe.
 * \bug a stack overflow of \a tempAllocStack is not handled correctly.
 */
temp_vstring left(const char *sin, long n);
/*!
 * \fn temp_vstring right(const char *sin, long n)
 * \brief Extract the substring starting with position n.
 *
 * Copies a NUL terminated string starting with the character at position __n__
 * to a temporary.  If the source contains UTF-8 encoded text, care has to be
 * taken that a multi-byte character is not split in this process.
 *
 * \param[in] sin (not null) pointer to a NUL terminated string to be copied
 *   from.
 * \param[in] n 1-based index of the first not skipped character at the
 *   beginning of the source.  A value less than 1 is internally corrected to 1.
 * \return a pointer to new allocated \a temp_vstring referencing the
 *   requested portion, that is also pushed onto the top of \a tempAllocStack
 * \attention the index __n__ is 1-based: right("hello", 2) == "ello"!
 * \pre
 *   __n__ <= length(__sin__)
 * \post
 *   A pointer to the substring is pushed on \a tempAllocStack, even if it
 *   empty.
 * \warning not UTF-8 safe.
 * \bug a stack overflow of \a tempAllocStack is not handled correctly.
 */
temp_vstring right(const char *sin, long n);
temp_vstring edit(const char *sin, long control);
/*!
 * \fn temp_vstring space(long n)
 * pushes a NUL terminated string of __n__ characters onto
 * \a tempAllocStack.
 * \param[in] n the count of spaces, one less than the memory to allocate in
 *   bytes.
 * \return a pointer to new allocated \a temp_vstring referencing the
 *   requested contents, also pushed onto the top of \a tempAllocStack
 * \post The returned string is NUL terminated
 * \bug a stack overflow of \a tempAllocStack is not handled correctly.
 */
temp_vstring space(long n);
/*!
 * \fn temp_vstring string(long n, char c)
 * pushes a NUL terminated string of __n__ characters __c__ onto
 * \a tempAllocStack.
 * \param[in] n one less than the memory to allocate in bytes.
 * \param[in] c character to fill the allocated memory with.  It is padded to
 *   the right with a NUL character.
 * \attention The choice of NUL for __c__ returns the empty string in a block
 *   of __n__ + 1 allocated bytes.
 * \post The returned string is NUL terminated
 * \return a pointer to new allocated \a temp_vstring referencing the
 *   requested contents, also pushed onto the top of \a tempAllocStack
 * \bug a stack overflow of \a tempAllocStack is not handled correctly.
 */
temp_vstring string(long n, char c);
/*!
 * \fn temp_vstring chr(long n)
 * \brief create a temporary string containing a single byte.
 *
 * create a NUL terminated string containing only the least significant byte of
 * __n__.  If this byte is 0x00, an empty string is the result.  If the most
 * significant bit of this byte is set, the returned string is not an ASCII
 * or UTF-8 string.
 * \param[in] n The eight least significant bits are converted into a
 *   character used to build a string from.
 * \post A string containing a single character different from NUL, or
 *   the empty string else, is pushed onto the \a tempAllocStack.
 * \return a pointer to new allocated \a temp_vstring referencing the
 *   requested contents, also pushed onto the top of \a tempAllocStack
 * \warning
 *   the resulting string need not contain exactly 1 character, and if it does,
 *   this character need not be ASCII or UTF-8.  If CHAR_BITS is not 8
 *   (extremely rare nowadays) there might be a portability issue.
 * \bug a stack overflow of \a tempAllocStack is not handled correctly.
 */
temp_vstring chr(long n);
temp_vstring xlate(const char *sin, const char *table);
temp_vstring date(void);
temp_vstring time_(void);
temp_vstring num(double x);
temp_vstring num1(double x);
temp_vstring str(double x);
long len(const char *s);
long instr(long start, const char *string1, const char *string2);
long rinstr(const char *string1, const char *string2);
long ascii_(const char *c);
double val(const char *s);

/* Emulation of Progress 4GL string functions */
temp_vstring entry(long element, const char *list);
long lookup(const char *expression, const char *list);
long numEntries(const char *list);
long entryPosition(long element, const char *list);

/* Routines may/may not be written (lowest priority):
vstring place$(vstring sout);
vstring prod$(vstring sout);
vstring quo$(vstring sout);
*/

/******* Special purpose routines for better
      memory allocation (use with caution) *******/

/*!
 * \var g_tempAllocStackTop
 * \brief Top of stack for temporary text.
 *
 * Refers to the \ref stack "stack" in \a tempAllocStack for temporary text.
 * The current top index referencing the next free entry is kept in this variable.
 *
 * This value is made public for setting up scopes of temporary memory for
 * nested functions.  Each such function allocates/frees scratch memory
 * independently.  Once a nested function is done, the caller's context must
 * be restored again, so it sees "its" temporaries untempered with.  To this
 * end the called nested function saves administrative stack data.  Upon finish
 * it restores those values.
 */
extern long g_tempAllocStackTop;   /* Top of stack for tempAlloc function */
/*!
 * \var g_startTempAllocStack
 * \brief references the first entry of the current scope of temporaries.
 *
 * Refers to the \ref stack "stack" in \a tempAllocStack for temporary text.
 * Nested functions maintain their own scope of temporary data.  The index
 * referencing the first index of the current scope is kept in this variable.
 *
 * This value is made public for setting up scopes of temporary memory for
 * nested functions.  Each such function allocates/frees scratch memory
 * independently.  Once a nested function is done, the caller's context must
 * be restored again, so it sees "its" temporaries untempered with.  To this
 * end the called nested function saves administrative stack data.  Upon finish
 * it restores those values.
 *
 * \invariant
 *   \a g_startTempAllocStack <= \a g_tempAllocStackTop.
 */
extern long g_startTempAllocStack; /* Where to start freeing temporary allocation
    when let() is called (normally 0, except for nested vstring functions) */

/* Make string have temporary allocation to be released by next let().
  This function effectively changes the type of `s`
  from `vstring` (an owned pointer) to `temp_vstring` (a temporary to be
  freed by the next `let()`). See `temp_vstring` for information on what
  you can do with temporary strings.
  In particular, after makeTempAlloc() is called, the vstring may NOT be
  assigned again with let(). */
temp_vstring makeTempAlloc(vstring s);    /* Make string have temporary allocation to be
                                    released by next let() */
#endif /* METAMATH_MMVSTR_H_ */
