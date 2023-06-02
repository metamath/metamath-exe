/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMDATA_H_
#define METAMATH_MMDATA_H_

/*!
 * \file mmdata.h
 * \brief includes for some principal data structures and data-string handling
 */

#include "mmvstr.h"

// debugging flags & variables

/*!
 * \var long db
 * \brief bytes held by vstring instances outside of the stack of temporaries
 *
 * monitors the number of bytes (including the terminal NUL) currently used in
 * all \ref vstring pointer variables OUTSIDE of the \ref tempAllocStack.
 * Note: This is NOT the number of bytes allocated, but the portion actually
 * used!  A few memory allocations are also included:
 * - command line buffers used to hold user input from a console;
 * - buffers used to read file contents in.
 *
 * If the user has turned MEMORY_STATUS on, metamath will print out this value
 * after each command in a message like "Memory: string < db >".
 */
/*E*/extern long db;
/*E*/extern long db0;

/*!
 * \var long db1
 * \brief bytes held by vstring instances inside of the stack of temporaries
 *
 * monitors the number of bytes currently pointed to by \ref vstring pointers
 * INSIDE the \ref tempAllocStack.  Note: This is NOT their capacity, but the
 * portion actually used!
 *
 * Not updated if NDEBUG (usually deactivates asserts in C code) is defined.
 *
 * \bug Seems never be displayed.
 */
/*E*/extern long db1;

/*!
 * \var long db2
 * Bytes held in \ref pgBlock "blocks" managed in \ref tempAllocStack
 * "temporary pointer stacks".
 */
/*E*/extern long db2;

/*!
 * \var db3
 * \brief monitors the de/allocations of nmbrString and \ref pntrString outside
 * of temporary arrays.
 *
 * The number of bytes held in blocks dedicated to global data.  There exist
 * also temporary stacks, but they are not considered here.  Useful to see
 * whether a process looses memory due to imbalanced calls to allocation/free
 * functions.
 *
 * If the user has turned MEMORY_STATUS on, metamath will print out this value
 * after each command in a message like "Memory: ... xxxString < db3 >".
 */
/*E*/extern long db3;
/*E*/extern long db4,db5,db6,db7,db8;

/*!
 * \var db9
 * \brief log memory pool usage for debugging purpose.
 *
 * If set to a non-zero value, the state of the memory pool is
 * logged during execution of metamath.  This debugging feature tracks
 * de/allocation of memory in the memory pool.
 * This particular debug mode is controlled by the Metamath commands
 * - SET MEMORY_STATUS ON\n
 *   enables memory logging
 * - SET MEMORY_STATUS OFF\n
 *   disables memory debugging
 * - SET DEBUG FLAG 9\n
 *   (deprecated) enables memory logging
 * - SET DEBUG OFF\n
 *   disables memory logging in conjunction with other debugging aid.
 */
/*E*/extern long db9;

/*!
 * \typedef flag
 * a char whose range is restricted to 0 (equivalent to false/no) and 1
 * (equivalent to true/yes), the typical semantics of a bool (Boolean value).
 */
typedef char flag;

/*!
 * \var g_listMode
 * \deprecated
 * Obsolete.  Now fixed to 0.  Historically the metamath sources were also used
 * for other purposes than maintaining Metamath files.  One such application, a
 * standalone text processor, was LIST.EXE.  The sources still query this
 * \ref flag occasionally, but its value is in fact fixed to 0 in metamath,
 * meaning the LIST.EXE functionality is an integral part of metamath now.
 */
extern flag g_listMode; // 0 = metamath, 1 = list utility

/*!
 * \var g_toolsMode
 * Metamath has two modes of operation: In its primary mode it handles
 * mathematical contents like proofs.  In this mode \ref g_toolsMode  is set to
 * 0.  This is the value assigned on startup.  A second mode is enabled after
 * executing the 'tools' command.  In this mode text files are processed using
 * high level commands.  It is indicated by a 1 in \ref g_toolsMode.
 */
extern flag g_toolsMode; // In metamath mode:  0 = metamath, 1 = tools

typedef long nmbrString; // String of numbers

/*!
 * \typedef pntrString
 * \brief an array (maybe of size 1) of untyped pointers (void*)
 *
 * In general this array is organized like a stack: the number of elements in
 * the pntrString array grows and shrinks during program flow, values are
 * pushed and popped at the end.  Such a stack is embedded in a \ref pgBlock
 * that contains administrative information about the stack.  The stack begins
 * with element 0, and the administrative information is accessed through
 * negative indices, but need reinterpretation then.  To allow iterating
 * through the tail of an array from a certain element on, an array terminates
 * with a null pointer.  This type of usage forbids null pointer as ordinary
 * elements, and the terminal null pointer is not part of the data in the
 * array.
 *
 * The length of a pntrString array is implicitely given by a terminal NULL
 * pointer.  If this array is held in a \ref pgBlock, its size can also be
 * determined from its header's administrative data.  Both values must be kept
 * synchronized.  In early phases of memory allocation, when data wasn't
 * assigned yet, this need not hold, though.
 *
 * To summarize the usages of this type:
 * - If you want to resize the array/stack you need a pointer to element 0.
 * - You can iterate from an arbitrary element to the end.
 * - Sometimes pntrString denotes an isolated element, not embedded in a
 *   greater array.
 *
 * \warning Simply copying elements around is dangerous, if the elements point
 *   to allocated memory.  There must be a guard that owns the instances,
 *   survives each of the copies and finally deallocates the instances.
 *   Otherwise you risk memory leaks, or even worse, undefined behavior, if a
 *   copied pointer uses an instance previously freed by the original.  One way
 *   to provide such a guard is a nested program structure.  All copied
 *   pointers are created in subroutines, and they vanish before the caller
 *   gains control again, and can safely deallocate the instances.  A similar
 *   strategy is followed by \ref pntrLet, where deallocations of original and
 *   dependent instances are deferred until an operation finally finishes.
 *   More modern concepts employ reference counting schemes, thus avoiding
 *   dedicated ownership and solving this problem more thoroughly, but Metamath
 *   is not up to that level (yet).
 */
typedef void* pntrString; // String of pointers

// A nmbrString allocated in temporary storage. These strings will be deallocated
// after the next call to `nmbrLet`.
// See also `temp_vstring` for information on how temporaries are handled.
typedef nmbrString temp_nmbrString;

// A pntrString allocated in temporary storage. These strings will be deallocated
// after the next call to `pntrLet`.
// See also `temp_vstring` for information on how temporaries are handled.

/*!
 * \typedef temp_pntrString
 * \brief a single \ref pntrString element for use in a \ref pgStack "stack".
 *
 * These elements are pushed onto and popped off a
 * \ref pgStack "stack of temporary data".  Pointers of this type should ONLY
 * refer to dynamically allocated memory on the heap.  Special commands support
 * dependency tracking and free all pointers on and after a particular one in
 * such a stack.
 */
typedef pntrString temp_pntrString;

enum mTokenType { var_, con_ };
#define lb_ '{' // ${
#define rb_ '}' // $}
#define v_  'v' // $v
#define c_  'c' // $c
#define a_  'a' // $a
#define d_  'd' // $d
#define e_  'e' // $e
#define f_  'f' // $f
#define p_  'p' // $p
#define eq_ '=' // $=
#define sc_ '.' // $. (historically, used to be $; (semicolon) )
#define illegal_ '?' // anything else
// Global variables related to current statement
extern int g_currentScope;

/*! The data associated with a statement in the file */
struct statement_struct { // Array index is statement number, starting at 1
  long lineNum; /*!< Line number in file; 0 means not yet determined */
  vstring fileName; /*!< File statement is in; "" means not yet determined */
  vstring labelName; /*!< Label of statement */
  flag uniqueLabel; /*!< Flag that label is unique (future implementations may
                      allow duplicate labels on hypotheses) */
  char type;    /*!< 2nd character of keyword, e.g. 'e' for $e */
  int scope;    /*!< Block scope level, increased by ${ and decreased by $};
       ${ has scope _before_ the increase; $} has scope _before_ the decrease */
  long beginScopeStatementNum;  /*!< statement of previous ${ ; 0 if we're in
                outermost block */
  long endScopeStatementNum;  /*!< statement of next $} (populated for opening
                                 ${ only, 0 otherwise); g_statements+1 if
                              we're in outermost block */
  vstring statementPtr; /*!< Pointer to end of (unmodified) label section used
             to determine file and line number for error or info messages about
             the statement */
  vstring labelSectionPtr; /*!< Source code before statement keyword
                 - will be updated if labelSection changed */
  long labelSectionLen;
  char labelSectionChanged; /*!< Default is 0; if 1, labelSectionPtr points to an
                               allocated vstring that must be freed by ERASE */
  vstring mathSectionPtr; /*!< Source code between keyword and $= or $. */
  long mathSectionLen;
  char mathSectionChanged; /*!< Default is 0; if 1, mathSectionPtr points to an
                               allocated vstring that must be freed by ERASE */
  vstring proofSectionPtr; /*!< Source code between $= and $. */
  long proofSectionLen;
  char proofSectionChanged; /*!< Default is 0; if 1, proofSectionPtr points to an
                               allocated vstring that must be freed by ERASE */
  nmbrString *mathString; /*!< Parsed mathSection */
  long mathStringLen;
  nmbrString *proofString; /*!< Parsed proofSection (used by $p's only */
  nmbrString *reqHypList; /*!< Required hypotheses (excluding $d's) */
  nmbrString *optHypList; /*!< Optional hypotheses (excluding $d's) */
  long numReqHyp; /*!< Number of required hypotheses */
  nmbrString *reqVarList; /*!< Required variables */
  nmbrString *optVarList; /*!< Optional variables */
  nmbrString *reqDisjVarsA; /*!< Required disjoint variables, 1st of pair */
  nmbrString *reqDisjVarsB; /*!< Required disjoint variables, 2nd of pair */
  nmbrString *reqDisjVarsStmt; /*!< Req disjoint variables, statem number */
  nmbrString *optDisjVarsA; /*!< Optional disjoint variables, 1st of pair */
  nmbrString *optDisjVarsB; /*!< Optional disjoint variables, 2nd of pair */
  nmbrString *optDisjVarsStmt; /*!< Opt disjoint variables, statem number */
  long pinkNumber; /*!< The $a/$p sequence number for web pages */
  long headerStartStmt; /*!< # of stmt following previous $a, $p */
};

/*! Sort keys for statement labels (allocated by parseLabels) */
extern long *g_labelKey;

/*! This structure holds all information related to $[ $] (include) statements
   in the input source files, for error message processing. */
struct includeCall_struct {
  vstring source_fn;  /*!< Name of the file where the
       inclusion source is located (= parent file for $( Begin $[... etc.) */
  vstring included_fn;  /*!< Name of the file in the
       inclusion statement e.g. "$( Begin $[ included_fn..." */
  long current_offset;  /*!< This is the starting
      character position of the included file w.r.t entire source buffer */
  long current_line; /*!< The line number
      of the start of the included file (=1) or the continuation line of
      the parent file */
  flag pushOrPop; /*!< 0 means included file, 1 means continuation of parent */
  vstring current_includeSource; /*!< (Currently) assigned
      only if we may need it for a later Begin comparison */
  long current_includeLength; /*!< Length of the file
      to be included (0 if the file was previously included) */
};

/*! The data associated with a math token. */
struct mathToken_struct {
  vstring tokenName; /*!< may be used more than once at different scopes */
  long length; /*!< to speed up parsing scans */
  char tokenType; /*!< variable or constant - (char)var_ or (char)con_ */
  flag active;  /*!< 1 if token is recognized in current scope */
  int scope;    /*!< scope level token was declared at */
  long tmp;     /*!< Temporary field use to speed up certain functions */
  long statement; /*!< Statement declared in */
  long endStatement; /*!< Statement of end of scope it was declared in */
};

/*! Sort keys for math tokens (allocated by parseMathDecl) */
extern long *g_mathKey;

extern long g_MAX_STATEMENTS;
extern long g_MAX_MATHTOKENS;
extern struct statement_struct *g_Statement;
// Obs // extern struct label_struct *label;

/*! \warning `mathToken[i]` is 0-based, not 1-based! */
extern struct mathToken_struct *g_MathToken;
extern long g_statements, /*labels,*/ g_mathTokens;

extern long g_MAX_INCLUDECALLS;
extern struct includeCall_struct *g_IncludeCall;
extern long g_includeCalls;

extern char *g_sourcePtr; /*!< Pointer to buffer in memory with input source */
extern long g_sourceLen; /*!< Number of chars. in all inputs files combined (after includes)*/

// For use by getMarkupFlag()
#define PROOF_DISCOURAGED_MARKUP "(Proof modification is discouraged.)"
#define USAGE_DISCOURAGED_MARKUP "(New usage is discouraged.)"
// Mode argument for getMarkupFlag()
#define PROOF_DISCOURAGED 1
#define USAGE_DISCOURAGED 2
#define RESET 0
// Mode argument for getContrib()
#define GC_RESET 0
#define GC_RESET_STMT 10
#define CONTRIBUTOR 1
#define CONTRIB_DATE 2
#define REVISER 3
#define REVISE_DATE 4
#define SHORTENER 5
#define SHORTEN_DATE 6
#define MOST_RECENT_DATE 7
#define GC_ERROR_CHECK_SILENT 8
#define GC_ERROR_CHECK_PRINT 9

// 14-May-2017 nm - TODO: someday we should create structures to
// hold global vars, and clear their string components in eraseSource().
extern vstring g_contributorName;
#define DEFAULT_CONTRIBUTOR "?who?"

extern vstring g_proofDiscouragedMarkup;
extern vstring g_usageDiscouragedMarkup;
extern flag g_globalDiscouragement; // SET DISCOURAGEMENT

// Allocation and deallocation in memory pool
void *poolFixedMalloc(long size /* bytes */);

/*!
 * \fn void *poolMalloc(long size)
 * \brief allocates and initializes a new \ref pgBlock
 *
 * allocates a \ref pgBlock, first removing and using the last element of the
 * \ref memFreePool.  If this block exists, but has not sufficient size, it is
 * reallocated from the system.  If the pool is empty, a new \ref pgBlock of
 * the given size is allocated from the system.  In any case, the header of the
 * \ref pgBlock is properly initialized.  Exits program on out of memory
 * condition.
 * \param[in] size (in bytes) of the block, not including the block header.
 * \return a \ref pgBlock with enough capacity for \p size bytes of data.
 *  \post
 *    - The \ref pgBlock "block's" header denotes \p size bytes are occupied,
 *      but they yet contain random data.
 *    - Exit on out-of-memory.
 */
void *poolMalloc(long size /* bytes */);

/*!
 * \fn poolFree(void *ptr)
 *
 * Removes \p ptr from the \ref memUsedPool, if it is listed there.  Then tries
 * adding it to the \ref memFreePool.  If this pool is full, it is increased by
 * \ref MEM_POOL_GROW.  If this fails, the program is exited, else \p ptr is
 * added.
 * \param[in] ptr pointer to a \ref pgBlock.
 * \pre
 *   - \p ptr refers to dynamically allocated memory on the heap.
 *   - all memory pointed to by \p ptr is considered free.  This holds even if it
 *     it is kept in \ref memUsedPool.
 * \post
 *   - \ref poolTotalFree is updated
 *   - Exit on out-of-memory (the \ref memFreePool overflows)
 * \attention never submit a \p ptr referring to memory not on the heap, like
 *   NULL_PTRSTRING.
 */
void poolFree(void *ptr);

/*!
 * \fn addToUsedPool(void *ptr)
 * \brief announces a block with free capacity for further allocation
 *
 * This function temporarily freezes the usage of a block for the current user,
 * and allows temporary reallocation of the free capacity to a new client.
 *
 * The program maintains pools of memory blocks with free capacity.  In case of
 * demand such a \ref pgBlock can temporarily allocate this capacity for new
 * usage.  Of course two (or more) clients share different parts of the same
 * \ref pgBlock then, so a newer client must complete its usage before the old
 * one resumes operation and may want to extend its usage of the \ref pgBlock.
 *
 * Before \p ptr is added to \ref memUsedPool, the pool size is checked and
 * increased by \ref MEM_POOL_GROW if full.  This may lead to out-of-memory
 * \ref outOfMemory "exit".  But if \p ptr is added to the end of the \ref memUsedPool,
 * \ref poolTotalFree is updated.
 * \param[in] ptr pointer to a \ref pgBlock.
 * \pre
 *   the block is \ref pgFragmentation "fragmented" (contains unused memory)
 *   If it is full, \ref bugfn "bug" is called and the function returns without
 *   further action.
 * \post
 *   - \ref poolTotalFree is the current free space in bytes in both pools.
 *   - A full \ref pgBlock is not added to \ref memUsedPool by this function.
 *   - Exit on out-of-memory (\ref memUsedPool overflows)
 */
void addToUsedPool(void *ptr);
/*! Purges reset memory pool usage */
void memFreePoolPurge(flag untilOK);
// Statistics

/*!
 * \fn getPoolStats(long *freeAlloc, long *usedAlloc, long *usedActual)
 * \brief Provide information about memory in pools at the instant of call.
 *
 * Return the overall statistics about the pools \ref memFreePool
 * "free block array" and the \ref memUsedPool "used block array".  In MEMORY
 * STATUS mode ON, a diagnostic message compares the contents of
 * \ref poolTotalFree to the values found in this statistics.  They should not
 * differ!
 *
 * \attention This is NOT full memory usage, because completely used
 * \ref pgBlock "blocks" are not tracked!
 *
 * \param[out] freeAlloc (not-null) address of a long variable receiving the
 * accumulated number of bytes in the free list.  Sizes do not include the
 * hidden header present in each block.
 * \param[out] usedAlloc (not-null) address of a long variable receiving the
 * accumulated number of free bytes in partially used blocks.
 * \param[out] usedActual (not-null) address of a long variable receiving the
 * accumulated bytes consumed by usage so far.  This value includes the hidden
 * header of the block.
 * \pre Do not call within \ref bugfn "bug".\n
 *   Submit only non-null pointers, even if not all information is needed.\n
 *   Pointers to irrelevant information may be the same.
 * \post Statistic data is copied to the locations the parameters point to.
 */
void getPoolStats(long *freeAlloc, long *usedAlloc, long *usedActual);

/*! Initial memory allocation */
void initBigArrays(void);

/*! Find the number of free memory bytes */
long getFreeSpace(long max);

/*!
 * \fn void outOfMemory(const char *msg)
 * \brief exit after fatal memory allocation error.
 *
 * called when memory cannot be allocated, either because memory/address space
 * is physically exhausted, or because administrative structures would overflow.
 * A non-recoverable condition, so stops execution with an error condition
 *
 * \param msg a short error message displayed to the user.
 * \return never, but exits the program instead.
 */
void outOfMemory(const char *msg);

/*!
 * \anchor bugfn
 * \fn void bug(int bugNum)
 * \param[in] bugNum
 */
void bug(int bugNum);

/*! Null nmbrString -- -1 flags the end of a nmbrString */
struct nullNmbrStruct {
    long poolLoc;
    long allocSize;
    long actualSize;
    nmbrString nullElement; };
extern struct nullNmbrStruct g_NmbrNull;
#define NULL_NMBRSTRING &(g_NmbrNull.nullElement)
#define nmbrString_def(x) nmbrString *x = NULL_NMBRSTRING
#define free_nmbrString(x) nmbrLet(&x, NULL_NMBRSTRING)

/*!
 * \struct nullPntrStruct
 * \brief Null pntrString -- NULL flags the end of a pntrString
 *
 * Describes a \ref pgBlock of \ref pntrString containing only the null
 * pointer.  Besides this pointer it is accompanied with a header containing
 * the hidden administrative values of such \ref pgBlock "block".
 *
 * The values in this administrative header are such that it is never subject to
 * memory allocation or deallocation.
 *
 * \bug The C standard does not require a long having the same size as a
 * void*.  In fact there might be **no** integer type matching a pointer in size.
 */
struct nullPntrStruct {
  /*!
   * An instance of a nullPntrStruct is always standalone and never part of a
   * larger pool.  Indicated by the fixed value -1.
   */
  long poolLoc;
  /*!
   * allocated size of the memory block containing the \ref pntrString,
   * excluding any hidden administrative data.
   * Note: this is the number of bytes, not elements!  Fixed to the size of a
   * single void* instance.
   */
  long allocSize;
  /*!
   * currently used size of the memory block containing the \ref pntrString,
   * excluding any hidden administrative data.
   * Note: this is the number of bytes, not elements!  Fixed to the size of a
   * single pointer element.
   */
  long actualSize;
  /*!
   * memory for a single void* instance, set and fixed to the null pointer.
   * A null marks the end of the array.
   */
  pntrString nullElement;
};

/*!
 * \var g_PntrNull
 * Global instance of a memory block structured like a
 * \ref pntrString, fixed in size and containing always exactly one null pointer
 * element, the terminating NULL.  This setup is recognized as an empty
 * \ref pntrString.
 *
 * \attention mark as const
 */
extern struct nullPntrStruct g_PntrNull;

/*!
 * \def NULL_PNTRSTRING
 * The address of a \ref pgBlock "block" containing an empty, not resizable
 * \ref pntrString
 * stack.  Used to initialize \ref pntrString variables .
 */
#define NULL_PNTRSTRING &(g_PntrNull.nullElement)

/*!
 * \def pntrString_def
 *
 * declare a new \ref pntrString variable and initialize it to point to a block
 * with an empty, not resizable \ref pntrString.
 *
 * \param[in] x variable name
 * \pre The variable does not exist in the current scope.
 * \post The variable is initialized.
 */
#define pntrString_def(x) pntrString *x = NULL_PNTRSTRING

/*!
 * \def free_pntrString
 * \param[in,out] x variable name
 * Assigns \ref NULL_PNTRSTRING to a variable \ref pntrString \p x.  Frees all
 * \ref pntrTempAllocStack, beginning with index
 * \ref g_pntrStartTempAllocStack.  See \ref pntrLet.
 * \pre
 *   - the \ref pgBlock assigned to \p x does not contain any valuable data.
 *   - all \ref pntrString elements freed in \ref pntrTempAllocStack can be
 *     discarded without losing relevant references.
 * \post
 *   - \p x is assigned NULL_PNTRSTRING.
 *   - The stack pointer of \ref pntrTempAllocStack is reset to
 *     \ref g_pntrStartTempAllocStack and all referenced
 *     \ref pgBlock "blocks" on and beyond that are returned to the
 *     \ref memFreePool.
 *   - updates \ref db3 and \ref poolTotalFree.
 *   - Exit on out-of-memory
 */
#define free_pntrString(x) pntrLet(&x, NULL_PNTRSTRING)

/*! This function returns a 1 if any entry in a comma-separated list
   matches using the matches() function. */
flag matchesList(const char *testString, const char *pattern, char wildCard,
    char oneCharWildCard);

/*! This function returns a 1 if the first argument matches the pattern of
   the second argument.  The second argument may have 0-or-more and
   exactly-1 character match wildcards, typically '*' and '?'.*/
flag matches(const char *testString, const char *pattern, char wildCard,
    char oneCharWildCard);

/*******************************************************************/
/*********** Number string functions *******************************/
/*******************************************************************/

/******* Special purpose routines for better
      memory allocation (use with caution) *******/

extern long g_nmbrTempAllocStackTop; // Top of stack for nmbrTempAlloc function
 // Where to start freeing temporary allocation when nmbrLet() is called 
 // (normally 0, except for nested nmbrString functions).
extern long g_nmbrStartTempAllocStack;

/*!
  When "size" is >0, "size" instances of nmbrString are allocated.
  When "size" is 0, all memory previously allocated with this function
  is deallocated, down to g_nmbrStartTempAllocStack. */
temp_nmbrString *nmbrTempAlloc(long size);

/*! \brief Make string have temporary allocation to be released by next nmbrLet()
  \warning after nmbrMakeTempAlloc() is called, the nmbrString may NOT be
    assigned again with nmbrLet() */
// Make string have temporary allocation to be released by next nmbrLet()
temp_nmbrString *nmbrMakeTempAlloc(nmbrString *s);

/**************************************************/

/*! String assignment - MUST be used to assign vstrings */
void nmbrLet(nmbrString **target, const nmbrString *source);

/*! String concatenation - last argument MUST be NULL */
temp_nmbrString *nmbrCat(const nmbrString *string1,...);

/*! Emulation of nmbrString functions similar to BASIC string functions */
temp_nmbrString *nmbrSeg(const nmbrString *sin, long p1, long p2);
temp_nmbrString *nmbrMid(const nmbrString *sin, long p, long l);
temp_nmbrString *nmbrLeft(const nmbrString *sin, long n);
temp_nmbrString *nmbrRight(const nmbrString *sin, long n);

/*! Allocate and return an "empty" string n "characters" long */
temp_nmbrString *nmbrSpace(long n);

long nmbrLen(const nmbrString *s);
long nmbrAllocLen(const nmbrString *s);
void nmbrZapLen(nmbrString *s, long length);

/*! Search for string2 in string 1 starting at start_position */
long nmbrInstr(long start, const nmbrString *sin, const nmbrString *s);

/*! Search for string2 in string 1 in reverse starting at start_position
 * (Reverse nmbrInstr)
 */
long nmbrRevInstr(long start_position, const nmbrString *string1,
   const nmbrString *string2);

/*! 1 if strings are equal, 0 otherwise */
flag nmbrEq(const nmbrString *s, const nmbrString *t);

/*! Converts mString to a vstring with one space between tokens */
temp_vstring nmbrCvtMToVString(const nmbrString *s);

/*! Converts rString to a vstring with one space between tokens */
temp_vstring nmbrCvtRToVString(const nmbrString *s,
    flag explicitTargets,
    long statemNum);

/*! Get step numbers in an rString - needed by cvtRToVString & elsewhere */
nmbrString *nmbrGetProofStepNumbs(const nmbrString *reason);

/*! Converts any nmbrString to an ASCII string of numbers corresponding
   to the .tokenNum field -- used for debugging only. */
temp_vstring nmbrCvtAnyToVString(const nmbrString *s);

/*! Extract variables from a math token string */
temp_nmbrString *nmbrExtractVars(const nmbrString *m);

/*! Determine if an element is in a nmbrString; return position if it is */
long nmbrElementIn(long start, const nmbrString *g, long element);

/*! Add a single number to end of a nmbrString - faster than nmbrCat */
temp_nmbrString *nmbrAddElement(const nmbrString *g, long element);

/*! Get the set union of two math token strings (presumably
   variable lists) */
temp_nmbrString *nmbrUnion(const nmbrString *m1, const nmbrString *m2);

/*! Get the set intersection of two math token strings (presumably
   variable lists) */
temp_nmbrString *nmbrIntersection(const nmbrString *m1, const nmbrString *m2);

/*! Get the set difference m1-m2 of two math token strings (presumably
   variable lists) */
temp_nmbrString *nmbrSetMinus(const nmbrString *m1,const nmbrString *m2);

/*! This is a utility function that returns the length of a subproof that
   ends at step */
long nmbrGetSubproofLen(const nmbrString *proof, long step);

/*! This function returns a "squished" proof, putting in {} references
   to previous subproofs. */
temp_nmbrString *nmbrSquishProof(const nmbrString *proof);

/*! This function un-squishes a "squished" proof, replacing {} references
   to previous subproofs by the subproofs themselves. */
temp_nmbrString *nmbrUnsquishProof(const nmbrString *proof);

/*! This function returns the indentation level vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0. */
temp_nmbrString *nmbrGetIndentation(const nmbrString *proof,
  long startingLevel);

/*! This function returns essential (1) or floating (0) vs. step number of a
   proof string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0. */
temp_nmbrString *nmbrGetEssential(const nmbrString *proof);

/*! This function returns the target hypothesis vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively.
   statemNum is the statement being proved. */
temp_nmbrString *nmbrGetTargetHyp(const nmbrString *proof, long statemNum);

/*!
 * Converts a proof string to a compressed-proof-format ASCII string.
 * Normally, the proof string would be compacted with squishProof first,
 * although it's not a requirement.
 *
 * The statement number is needed because required hypotheses are
 * implicit in the compressed proof.
 */
temp_vstring compressProof(const nmbrString *proof, long statemNum,
    flag oldCompressionAlgorithm);

/*! Gets length of the ASCII form of a compressed proof */
long compressedProofSize(const nmbrString *proof, long statemNum);

/*******************************************************************/
/*********** Pointer string functions ******************************/
/*******************************************************************/

/******* Special purpose routines for better
      memory allocation (use with caution) *******/

/*!
 * \var long g_pntrTempAllocStackTop
 *
 * Index of the current top of the \ref pgStack "stack" \ref pntrTempAlloc.
 * New data is pushed from this location on if space available.
 *
 * \invariant always refers the null pointer element behind the valid data.
 */
extern long g_pntrTempAllocStackTop; // Top of stack for pntrTempAlloc function

/*!
 * \var long g_pntrStartTempAllocStack
 *
 * Index of the first entry of the \ref pgStack "stack" \ref pntrTempAllocStack
 * eligible for deallocation on the next call to \ref pntrTempAlloc.  Entries
 * below this value are considered not dependent on the value at this index,
 * but entries above are.  So when this entry gets deallocated, dependent ones
 * should follow suit.  A function like \ref pntrTempAlloc or \ref pntrLet
 * manage this automatic deallocation.
 *
 * Nested functions using the \ref pntrTempAllocStack usually save the current
 * value and set it to \ref g_pntrTempAllocStackTop, so they can create their
 * local dependency chain.  On return the saved value is restored.
 * \invariant always less or equal to \ref g_pntrTempAllocStackTop.
 */
// Where to start freeing temporary allocation when pntrLet() is called
// (normally 0, except for nested pntrString functions).
extern long g_pntrStartTempAllocStack;

/*!
 * \brief Make string have temporary allocation to be released by next pntrLet()
 * \warning after pntrMakeTempAlloc() is called, the pntrString may NOT be
 *   assigned again with pntrLet()
 */
temp_pntrString *pntrMakeTempAlloc(pntrString *s);

/**************************************************/

/*!
 * \fn void pntrLet(pntrString **target, const pntrString *source)
 * String assignment - MUST be used to assign vstrings.
 *
 * Copies the \ref pntrString elements of \p source to the beginning of a
 * \ref pgBlock referenced by \p target.  If necessary, the \p target block is
 * reallocated, and if it is, it gets twice the needed size to account for
 * future growing.  If the \p target block is only partially used after copy it
 * is added to the \ref memUsedPool.  If \p source is empty, the \p target is
 * set to \ref NULL_PNTRSTRING.
 *
 * It is assumed that the value persisted in \p target is in fact computed from
 * temporary operands in \ref pntrTempAllocStack.  All blocks starting with
 * the element at \ref g_pntrStartTempAllocStack are returned to the
 * \ref memFreePool.
 * \attention freed \ref pgBlock "blocks" contain \ref pntrString instances.
 *   See \ref pntrTempAllocStack to learn how this free process can be
 *   dangerous if insufficient precautions are taken.
 * \param[in,out] target (not null) the address of a pointer pointing to the
 *   first byte of a \ref pgBlock receiving the copied elements of \p source.
 * \param[in] source (not null) a pointer to the first \ref pntrString element
 *   in a \ref pgBlock, to be copied from.
 * \pre
 *   - source does not contain NULL pointer elements , but is terminated by
 *     one.  This final NULL pointer is not part of the array, but must be present.
 *   - the target \ref pgBlock does not contain any valuable data.
 *   - all \ref pntrString elements freed in \ref pntrTempAllocStack can be
 *     discarded without losing relevant references.
 * \post
 *   - the \ref pgBlock \p target points to is filled with a copy of
 *     \ref pntrString elements \p source points to, padded with a terminal
 *     NULL.
 *   - due to a possible reallocation the pointer \p target points to may
 *     change.
 *   - The stack pointer \ref g_pntrTempAllocStackTop of
 *     \ref pntrTempAllocStack is reset to \ref g_pntrStartTempAllocStack and
 *     all referenced \ref pgBlock "blocks" on and beyond that are returned to
 *     the \ref memFreePool.
 *   - updates \ref db3 and \ref poolTotalFree.
 *   - Exit on out-of-memory
 * \bug If the \p target block is full after the copy operation, it is not
 *   necessarily removed from the \ref memUsedPool, although other
 *   functions like \ref addToUsedPool do not support this.
 */
void pntrLet(pntrString **target, const pntrString *source);

/*! String concatenation - last argument MUST be NULL */
temp_pntrString *pntrCat(const pntrString *string1,...);

// Emulation of pntrString functions similar to BASIC string functions
temp_pntrString *pntrSeg(const pntrString *sin, long p1, long p2);
temp_pntrString *pntrMid(const pntrString *sin, long p, long length);
temp_pntrString *pntrLeft(const pntrString *sin, long n);
temp_pntrString *pntrRight(const pntrString *sin, long n);

/*! Allocate and return an "empty" string n "characters" long */
temp_pntrString *pntrSpace(long n);

/*! Allocate and return an "empty" string n "characters" long
   initialized to nmbrStrings instead of vStrings */
temp_pntrString *pntrNSpace(long n);

/*! Allocate and return an "empty" string n "characters" long
   initialized to pntrStrings instead of vStrings */
temp_pntrString *pntrPSpace(long n);

/*!
 * \fn long pntrLen(const pntrString *s)
 * \brief Determine the length of a pntrString held in a \ref pgBlock "block"
 * dedicated to it.
 *
 * returns the number of **reserved** pointers in the array pointed to by \p s,
 * derived solely from administrative data in the surrounding \ref pgBlock.
 * Thus, the value is valid, even if data has not yet been transferred to the
 * reserved space, and the terminal NULL is not safely recognized. The returned
 * value excludes the space set aside for a terminal NULL.
 *
 * \attention This is not the capacity of the array.
 * \param[in] s points to a element 0 of a \ref pntrString  embedded in a block
 * \return the number of pointers currently in use in the array pointed to by \p s.
 * \pre the array pointed to by s is the sole user of a \ref pgBlock "block".
 */
long pntrLen(const pntrString *s);

/*!
 * \fn long pntrAllocLen(const pntrString *s)
 * \brief Determine the capacity of a pntrString embedded in a dedicated block
 *
 * returns the capacity of pointers in the array pointed to by \p s,
 * derived from administrative data in the surrounding block.  The result
 * excludes the terminal element reserved for a null pointer.
 *
 * \param[in] s points to element 0 of a \ref pntrString embedded in a block
 * \return the maximal number of pointers that can be used in the array pointed
 * to by \p s.
 * \pre the array pointed to by s is the sole user of a \ref pgBlock "block".
 */
long pntrAllocLen(const pntrString *s);
void pntrZapLen(pntrString *s, long length);

/*! Search for string2 in string 1 starting at start_position */
long pntrInstr(long start, const pntrString *sin, const pntrString *s);

/*! Search for string2 in string 1 in reverse starting at start_position
    (Reverse pntrInstr) */
long pntrRevInstr(long start_position, const pntrString *string1,
    const pntrString *string2);

/*! 1 if strings are equal, 0 otherwise */
flag pntrEq(const pntrString *sout, const pntrString *sin);

/*!
 * \fn temp_pntrString *pntrAddElement(const pntrString *g)
 * Add a single null string element to a pntrString - faster than pntrCat
 *
 * \param[in] g points to the first element of a NULL terminated array in a
 *   \ref pgBlock.  It is assumed it is an array of pointer to \ref vstring.
 * \return a copy of \p g, the terminal NULL replaced with a \ref vstring ""
 *   followed by NULL.
 * \attention
 *   the pointers in \p g are copied to the result.  If some of them
 *   reference allocated memory, check for possible double free, for example.
 * \pre
 *   Intended to be used with arrays of \ref vstring * only.
 * \post
 *   the elements of \p g are duplicated.
 */
temp_pntrString *pntrAddElement(const pntrString *g);

/*! Add a single null pntrString element to a pntrString - faster than pntrCat */
temp_pntrString *pntrAddGElement(const pntrString *g);

// Utility functions

/*! 0/1 knapsack algorithm */
long knapsack01(long items, long *size, long *worth, long maxSize,
       char *itemIncluded /* output: 1 = item included, 0 = not included */);

/*! 2D matrix allocation */
long **alloc2DMatrix(size_t xsize, size_t ysize);
/*! 2D matrix deallocation */
void free2DMatrix(long **matrix, size_t xsize /*, size_t ysize*/);

/*! Returns the amount of indentation of a statement label.  Used to
   determine how much to indent a saved proof. */
long getSourceIndentation(long statemNum);

/*! Returns any comment (description) that occurs just before a statement */
vstring getDescription(long statemNum);

/*! Returns the label section of a statement with all comments except the
   last removed. */
vstring getDescriptionAndLabel(long statemNum);

/*! Returns 1 if comment has an "is discouraged" markup tag */
flag getMarkupFlag(long statemNum, char mode);

/*! Extract any contributors and dates from statement description.
   If missing, the corresponding return string is blank.
   See GC_RESET etc. modes above.  Caller must deallocate returned
   string. */
vstring getContrib(long stmtNum, char mode);

/*! Extract up to 2 dates after a statement's proof.  If no date is present,
   date1 will be blank.  If no 2nd date is present, date2 will be blank. */
void getProofDate(long stmtNum, vstring *date1, vstring *date2);

/*! Get date, month, year fields from a dd-mmm-yyyy date string,
   where dd may be 1 or 2 digits, mmm is 1st 3 letters of month,
   and yyyy is 2 or 4 digits.  A 1 is returned if an error was detected. */
flag parseDate(vstring dateStr, long *dd, long *mmm, long *yyyy);

/*! Build date from numeric fields.  mmm should be a number from 1 to 12. */
void buildDate(long dd, long mmm, long yyyy, vstring *dateStr);

/*! Compare two dates in the form dd-mmm-yyyy.  -1 = date1 < date2,
   0 = date1 = date2,  1 = date1 > date2.  There is no error checking. */
flag compareDates(vstring date1, vstring date2);

extern vstring g_qsortKey;
/*!
 * \brief Comparison function for qsort
 * \note Used by qsortStringCmp; pointer only, do not deallocate
 */
int qsortStringCmp(const void *p1, const void *p2);

/*! Call on exit to free memory */
void freeData(void);

#endif // METAMATH_MMDATA_H_
