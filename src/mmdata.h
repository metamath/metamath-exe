/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file mmdata.h - includes for some principal data structures and data-string
 * handling */

#ifndef METAMATH_MMDATA_H_
#define METAMATH_MMDATA_H_

#include "mmvstr.h"

/*E*/extern long db,db0,db1,db2,db3,db4,db5,db6,db7,db8,db9;/* debugging flags & variables */
/*!
 * \typedef flag
 * a char whoose range is restricted to 0 (equivalent to false/no) and 1
 * (equivalent to true/yes), the typical semantics of a bool (Boolean value).
 */
typedef char flag;

/*!
 * \deprecated
 * \var flag g_listMode.
 * Obsolete.  Now fixed to 0.  Historically the metamath sources were also used
 * for other purposes than maintaining Metamath files.  One such application, a
 * standalone text processor, was LIST.EXE.  The sources still query this
 * \a flag occasionally, but its value is in fact fixed to 0 in metamath,
 * meaning the LIST.EXE functionality is an integral part of metamath now.
 */
extern flag g_listMode; /* 0 = metamath, 1 = list utility */
/*!
 * \var flag g_toolsMode.
 * Metamath has two modes of operation: In its primary mode it handles
 * mathematical contents like proofs.  In this mode \a g_toolsMode  is set to
 * 0.  This is the value assigned on startup.  A second mode is enabled after
 * executing the 'tools' command.  In this mode text files are processed using
 * high level commands.  It is indicated by a 1 in \a g_toolsMode.
 */
extern flag g_toolsMode; /* In metamath mode:  0 = metamath, 1 = tools */

typedef long nmbrString; /* String of numbers */
/*!
 * \typedef pntrString
 * an array of untyped pointers (void*).
 * Often this array is organized like a stack: the number of elements in the
 * pntrString grows and shrinks during program flow, values are pushed and
 * popped at the end.  To maintain such a stack, a hidden structure prepends
 * the first element then.  This structure occupies three pointer elements,
 * addressed with offsets -1 to -3 relative to the first element of the stack.
 * This allows for easy embedding of a stack within an even larger memory
 * pool.  The fields of this hidden structure, although formally pointers,
 * are loaded with long int values describing properties of the stack:
 *
 * offset -1: is the current size of the stack (in bytes, not elements!). When
 *   interpreted as an offset into the stack, it references the first element
 *   past the top of the stack.
 *
 * offset -2: the allocated size of the array, in bytes.  When used with a
 *   stack, it marks the limit where the stack overflows.  Current size
 *   <= allocated size is an invariant of this structure.
 *
 * offset -3.  If this array is a subarray (or sub-stack) of a larger pool of
 *  pointers, then it marks the offset in bytes of the heading structure in the
 *  larger pool.
 */
typedef void* pntrString; /* String of pointers */

/* A nmbrString allocated in temporary storage. These strings will be deallocated
   after the next call to `nmbrLet`.
   See also `temp_vstring` for information on how temporaries are handled. */
typedef nmbrString temp_nmbrString;

/* A pntrString allocated in temporary storage. These strings will be deallocated
   after the next call to `pntrLet`.
   See also `temp_vstring` for information on how temporaries are handled. */
typedef pntrString temp_pntrString;

enum mTokenType { var_, con_ };
#define lb_ '{' /* ${ */
#define rb_ '}' /* $} */
#define v_  'v' /* $v */
#define c_  'c' /* $c */
#define a_  'a' /* $a */
#define d_  'd' /* $d */
#define e_  'e' /* $e */
#define f_  'f' /* $f */
#define p_  'p' /* $p */
#define eq_ '=' /* $= */
#define sc_ '.' /* $. (historically, used to be $; (semicolon) ) */
#define illegal_ '?' /* anything else */
/* Global variables related to current statement */
extern int g_currentScope;

struct statement_struct { /* Array index is statement number, starting at 1 */
  long lineNum; /* Line number in file; 0 means not yet determined */
  vstring fileName; /* File statement is in; "" means not yet determined */
  vstring labelName; /* Label of statement */
  flag uniqueLabel; /* Flag that label is unique (future implementations may
                      allow duplicate labels on hypotheses) */
  char type;    /* 2nd character of keyword, e.g. 'e' for $e */
  int scope;    /* Block scope level, increased by ${ and decreased by $};
       ${ has scope _before_ the increase; $} has scope _before_ the decrease */
  long beginScopeStatementNum;  /* statement of previous ${ ; 0 if we're in
                outermost block */
  long endScopeStatementNum;  /* statement of next $} (populated for opening
                                 ${ only, 0 otherwise); g_statements+1 if
                              we're in outermost block */
  vstring statementPtr; /* Pointer to end of (unmodified) label section used
             to determine file and line number for error or info messages about
             the statement */
  vstring labelSectionPtr; /* Source code before statement keyword
                 - will be updated if labelSection changed */
  long labelSectionLen;
  char labelSectionChanged; /* Default is 0; if 1, labelSectionPtr points to an
                               allocated vstring that must be freed by ERASE */
  vstring mathSectionPtr; /* Source code between keyword and $= or $. */
  long mathSectionLen;
  char mathSectionChanged; /* Default is 0; if 1, mathSectionPtr points to an
                               allocated vstring that must be freed by ERASE */
  vstring proofSectionPtr; /* Source code between $= and $. */
  long proofSectionLen;
  char proofSectionChanged; /* Default is 0; if 1, proofSectionPtr points to an
                               allocated vstring that must be freed by ERASE */
  nmbrString *mathString; /* Parsed mathSection */
  long mathStringLen;
  nmbrString *proofString; /* Parsed proofSection (used by $p's only */
  nmbrString *reqHypList; /* Required hypotheses (excluding $d's) */
  nmbrString *optHypList; /* Optional hypotheses (excluding $d's) */
  long numReqHyp; /* Number of required hypotheses */
  nmbrString *reqVarList; /* Required variables */
  nmbrString *optVarList; /* Optional variables */
  nmbrString *reqDisjVarsA; /* Required disjoint variables, 1st of pair */
  nmbrString *reqDisjVarsB; /* Required disjoint variables, 2nd of pair */
  nmbrString *reqDisjVarsStmt; /* Req disjoint variables, statem number */
  nmbrString *optDisjVarsA; /* Optional disjoint variables, 1st of pair */
  nmbrString *optDisjVarsB; /* Optional disjoint variables, 2nd of pair */
  nmbrString *optDisjVarsStmt; /* Opt disjoint variables, statem number */
  long pinkNumber; /* The $a/$p sequence number for web pages */
  long headerStartStmt; /* # of stmt following previous $a, $p */
  };

/* Sort keys for statement labels (allocated by parseLabels) */
extern long *g_labelKey;

struct includeCall_struct {
  /* This structure holds all information related to $[ $] (include) statements
     in the input source files, for error message processing. */
  vstring source_fn;  /* Name of the file where the
       inclusion source is located (= parent file for $( Begin $[... etc.) */
  vstring included_fn;  /* Name of the file in the
       inclusion statement e.g. "$( Begin $[ included_fn..." */
  long current_offset;  /* This is the starting
      character position of the included file w.r.t entire source buffer */
  long current_line; /* The line number
      of the start of the included file (=1) or the continuation line of
      the parent file */
  flag pushOrPop; /* 0 means included file, 1 means continuation of parent */
  vstring current_includeSource; /* (Currently) assigned
      only if we may need it for a later Begin comparison */
  long current_includeLength; /* Length of the file
      to be included (0 if the file was previously included) */
  };

struct mathToken_struct {
  vstring tokenName; /* may be used more than once at different scopes */
  long length; /* to speed up parsing scans */
  char tokenType; /* variable or constant - (char)var_ or (char)con_ */
  flag active;  /* 1 if token is recognized in current scope */
  int scope;    /* scope level token was declared at */
  long tmp;     /* Temporary field use to speed up certain functions */
  long statement; /* Statement declared in */
  long endStatement; /* Statement of end of scope it was declared in */
  };

/* Sort keys for math tokens (allocated by parseMathDecl) */
extern long *g_mathKey;

extern long g_MAX_STATEMENTS;
extern long g_MAX_MATHTOKENS;
extern struct statement_struct *g_Statement;
/*Obs*/ /*extern struct label_struct *label;*/

/* Warning: mathToken[i] is 0-based, not 1-based! */
extern struct mathToken_struct *g_MathToken;
extern long g_statements, /*labels,*/ g_mathTokens;

extern long g_MAX_INCLUDECALLS;
extern struct includeCall_struct *g_IncludeCall;
extern long g_includeCalls;

extern char *g_sourcePtr; /* Pointer to buffer in memory with input source */
extern long g_sourceLen; /* Number of chars. in all inputs files combined (after includes)*/

/* For use by getMarkupFlag() */
#define PROOF_DISCOURAGED_MARKUP "(Proof modification is discouraged.)"
#define USAGE_DISCOURAGED_MARKUP "(New usage is discouraged.)"
/* Mode argument for getMarkupFlag() */
#define PROOF_DISCOURAGED 1
#define USAGE_DISCOURAGED 2
#define RESET 0
/* Mode argument for getContrib() */
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

/* 14-May-2017 nm - TODO: someday we should create structures to
   hold global vars, and clear their string components in eraseSource() */
extern vstring g_contributorName;
#define DEFAULT_CONTRIBUTOR "?who?"

extern vstring g_proofDiscouragedMarkup;
extern vstring g_usageDiscouragedMarkup;
extern flag g_globalDiscouragement; /* SET DISCOURAGEMENT */

/* Allocation and deallocation in memory pool */
void *poolFixedMalloc(long size /* bytes */);
void *poolMalloc(long size /* bytes */);
void poolFree(void *ptr);
void addToUsedPool(void *ptr);
/* Purges reset memory pool usage */
void memFreePoolPurge(flag untilOK);
/* Statistics */
void getPoolStats(long *freeAlloc, long *usedAlloc, long *usedActual);

/* Initial memory allocation */
void initBigArrays(void);

/* Find the number of free memory bytes */
long getFreeSpace(long max);

/* Fatal memory allocation error */
void outOfMemory(const char *msg);

/* Bug check error */
void bug(int bugNum);


/* Null nmbrString -- -1 flags the end of a nmbrString */
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
 * describing a block of memory of pntrString containing only the
 * null pointer.  Besides the pointer it is accompanied with a header matching
 * the hidden administrative values of a usual pntrString managed as a stack.
 * 
 * The values in this administrative header are such that it is never subject to
 * memory allocation or deallocation.
 * 
 * \bug The C standard does not require a long having the same size as a
 * void*.  In fact there might be **no** integer type matching a pointer in size.
 */
/* Null pntrString -- NULL flags the end of a pntrString */
struct nullPntrStruct {
    /*!
     * An instance of a nullPntrStruct is always standalone and never part of a
     * larger pool.  Indicated by the fixed value -1.
     */
    long poolLoc;
    /*! 
     * allocated size of the memory block containing the \a pntrString,
     * excluding any hidden administrative data.
     * Note: this is the number of bytes, not elements!  Fixed to the size of a
     * single void* instance. 
     */
    long allocSize;
    /*! 
     * currently used size of the memory block containing the \a pntrString,
     * excluding any hidden administrative data.
     * Note: this is the number of bytes, not elements!  Fixed to the size of a
     * single pointer element.
     */
    long actualSize;
    /*! 
     * memory for a single void* instance, set and fixed to the null pointer.
     * A null marks the end of the array.
     */
    pntrString nullElement; };
/*!
 * \var g_PntrNull. Global instance of a memory block structured like a
 * \a pntrString, but fixed in size and containing always exactly one null
 * pointer element.
 * 
 * \attention This structure must not be modified.  Consider enforcing this
 * condition by for example adding a const attribute.
 */
extern struct nullPntrStruct g_PntrNull;
/*!
 * \def NULL_PNTRSTRING
 * yields the address of a global null pointer element.
 */
#define NULL_PNTRSTRING &(g_PntrNull.nullElement)
#define pntrString_def(x) pntrString *x = NULL_PNTRSTRING
#define free_pntrString(x) pntrLet(&x, NULL_PNTRSTRING)


/* This function returns a 1 if any entry in a comma-separated list
   matches using the matches() function. */
flag matchesList(const char *testString, const char *pattern, char wildCard,
    char oneCharWildCard);

/* This function returns a 1 if the first argument matches the pattern of
   the second argument.  The second argument may have 0-or-more and
   exactly-1 character match wildcards, typically '*' and '?'.*/
flag matches(const char *testString, const char *pattern, char wildCard,
    char oneCharWildCard);



/*******************************************************************/
/*********** Number string functions *******************************/
/*******************************************************************/

/******* Special pupose routines for better
      memory allocation (use with caution) *******/

extern long g_nmbrTempAllocStackTop;   /* Top of stack for nmbrTempAlloc funct */
extern long g_nmbrStartTempAllocStack; /* Where to start freeing temporary
    allocation when nmbrLet() is called (normally 0, except for nested
    nmbrString functions) */

/* Make string have temporary allocation to be released by next nmbrLet() */
/* Warning:  after nmbrMakeTempAlloc() is called, the nmbrString may NOT be
   assigned again with nmbrLet() */
temp_nmbrString *nmbrMakeTempAlloc(nmbrString *s);
                                    /* Make string have temporary allocation to be
                                    released by next nmbrLet() */

/**************************************************/


/* String assignment - MUST be used to assign vstrings */
void nmbrLet(nmbrString **target, const nmbrString *source);

/* String concatenation - last argument MUST be NULL */
temp_nmbrString *nmbrCat(const nmbrString *string1,...);

/* Emulation of nmbrString functions similar to BASIC string functions */
temp_nmbrString *nmbrSeg(const nmbrString *sin, long p1, long p2);
temp_nmbrString *nmbrMid(const nmbrString *sin, long p, long l);
temp_nmbrString *nmbrLeft(const nmbrString *sin, long n);
temp_nmbrString *nmbrRight(const nmbrString *sin, long n);

/* Allocate and return an "empty" string n "characters" long */
temp_nmbrString *nmbrSpace(long n);

long nmbrLen(const nmbrString *s);
long nmbrAllocLen(const nmbrString *s);
void nmbrZapLen(nmbrString *s, long length);

/* Search for string2 in string 1 starting at start_position */
long nmbrInstr(long start, const nmbrString *sin, const nmbrString *s);

/* Search for string2 in string 1 in reverse starting at start_position */
/* (Reverse nmbrInstr) */
long nmbrRevInstr(long start_position, const nmbrString *string1,
   const nmbrString *string2);

/* 1 if strings are equal, 0 otherwise */
flag nmbrEq(const nmbrString *s, const nmbrString *t);

/* Converts mString to a vstring with one space between tokens */
temp_vstring nmbrCvtMToVString(const nmbrString *s);

/* Converts rString to a vstring with one space between tokens */
temp_vstring nmbrCvtRToVString(const nmbrString *s,
    flag explicitTargets,
    long statemNum);

/* Get step numbers in an rString - needed by cvtRToVString & elsewhere */
nmbrString *nmbrGetProofStepNumbs(const nmbrString *reason);

/* Converts any nmbrString to an ASCII string of numbers corresponding
   to the .tokenNum field -- used for debugging only. */
temp_vstring nmbrCvtAnyToVString(const nmbrString *s);

/* Extract variables from a math token string */
temp_nmbrString *nmbrExtractVars(const nmbrString *m);

/* Determine if an element is in a nmbrString; return position if it is */
long nmbrElementIn(long start, const nmbrString *g, long element);

/* Add a single number to end of a nmbrString - faster than nmbrCat */
temp_nmbrString *nmbrAddElement(const nmbrString *g, long element);

/* Get the set union of two math token strings (presumably
   variable lists) */
temp_nmbrString *nmbrUnion(const nmbrString *m1, const nmbrString *m2);

/* Get the set intersection of two math token strings (presumably
   variable lists) */
temp_nmbrString *nmbrIntersection(const nmbrString *m1, const nmbrString *m2);

/* Get the set difference m1-m2 of two math token strings (presumably
   variable lists) */
temp_nmbrString *nmbrSetMinus(const nmbrString *m1,const nmbrString *m2);



/* This is a utility function that returns the length of a subproof that
   ends at step */
long nmbrGetSubproofLen(const nmbrString *proof, long step);

/* This function returns a "squished" proof, putting in {} references
   to previous subproofs. */
temp_nmbrString *nmbrSquishProof(const nmbrString *proof);

/* This function unsquishes a "squished" proof, replacing {} references
   to previous subproofs by the subproofs themselves. */
temp_nmbrString *nmbrUnsquishProof(const nmbrString *proof);

/* This function returns the indentation level vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0. */
temp_nmbrString *nmbrGetIndentation(const nmbrString *proof,
  long startingLevel);

/* This function returns essential (1) or floating (0) vs. step number of a
   proof string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0. */
temp_nmbrString *nmbrGetEssential(const nmbrString *proof);

/* This function returns the target hypothesis vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively.
   statemNum is the statement being proved. */
temp_nmbrString *nmbrGetTargetHyp(const nmbrString *proof, long statemNum);

/* Converts a proof string to a compressed-proof-format ASCII string.
   Normally, the proof string would be compacted with squishProof first,
   although it's not a requirement. */
/* The statement number is needed because required hypotheses are
   implicit in the compressed proof. */
temp_vstring compressProof(const nmbrString *proof, long statemNum,
    flag oldCompressionAlgorithm);

/* Gets length of the ASCII form of a compressed proof */
long compressedProofSize(const nmbrString *proof, long statemNum);


/*******************************************************************/
/*********** Pointer string functions ******************************/
/*******************************************************************/

/******* Special pupose routines for better
      memory allocation (use with caution) *******/

extern long g_pntrTempAllocStackTop;   /* Top of stack for pntrTempAlloc funct */
extern long g_pntrStartTempAllocStack; /* Where to start freeing temporary
    allocation when pntrLet() is called (normally 0, except for nested
    pntrString functions) */

/* Make string have temporary allocation to be released by next pntrLet() */
/* Warning:  after pntrMakeTempAlloc() is called, the pntrString may NOT be
   assigned again with pntrLet() */
temp_pntrString *pntrMakeTempAlloc(pntrString *s);
                                    /* Make string have temporary allocation to be
                                    released by next pntrLet() */

/**************************************************/


/* String assignment - MUST be used to assign vstrings */
void pntrLet(pntrString **target, const pntrString *source);

/* String concatenation - last argument MUST be NULL */
temp_pntrString *pntrCat(const pntrString *string1,...);

/* Emulation of pntrString functions similar to BASIC string functions */
temp_pntrString *pntrSeg(const pntrString *sin, long p1, long p2);
temp_pntrString *pntrMid(const pntrString *sin, long p, long length);
temp_pntrString *pntrLeft(const pntrString *sin, long n);
temp_pntrString *pntrRight(const pntrString *sin, long n);

/* Allocate and return an "empty" string n "characters" long */
temp_pntrString *pntrSpace(long n);

/* Allocate and return an "empty" string n "characters" long
   initialized to nmbrStrings instead of vStrings */
temp_pntrString *pntrNSpace(long n);

/* Allocate and return an "empty" string n "characters" long
   initialized to pntrStrings instead of vStrings */
temp_pntrString *pntrPSpace(long n);

/*!
 * \fn long pntrLen(const pntrString* s) Determine the length of a pntrString
 * \param s \a pntrString array of pointers from its hidden structure.
 * \pre s has a hidden structure, see \a pntrString 
 */
long pntrLen(const pntrString *s);
long pntrAllocLen(const pntrString *s);
void pntrZapLen(pntrString *s, long length);

/* Search for string2 in string 1 starting at start_position */
long pntrInstr(long start, const pntrString *sin, const pntrString *s);

/* Search for string2 in string 1 in reverse starting at start_position */
/* (Reverse pntrInstr) */
long pntrRevInstr(long start_position, const pntrString *string1,
    const pntrString *string2);

/* 1 if strings are equal, 0 otherwise */
flag pntrEq(const pntrString *sout, const pntrString *sin);

/* Add a single null string element to a pntrString - faster than pntrCat */
temp_pntrString *pntrAddElement(const pntrString *g);

/* Add a single null pntrString element to a pntrString - faster than pntrCat */
temp_pntrString *pntrAddGElement(const pntrString *g);

/* Utility functions */

/* 0/1 knapsack algorithm */
long knapsack01(long items, long *size, long *worth, long maxSize,
       char *itemIncluded /* output: 1 = item included, 0 = not included */);

/* 2D matrix allocation and deallocation */
long **alloc2DMatrix(size_t xsize, size_t ysize);
void free2DMatrix(long **matrix, size_t xsize /*, size_t ysize*/);

/* Returns the amount of indentation of a statement label.  Used to
   determine how much to indent a saved proof. */
long getSourceIndentation(long statemNum);

/* Returns any comment (description) that occurs just before a statement */
vstring getDescription(long statemNum);

/* Returns the label section of a statement with all comments except the
   last removed. */
vstring getDescriptionAndLabel(long statemNum);

/* Returns 1 if comment has an "is discouraged" markup tag */
flag getMarkupFlag(long statemNum, char mode);

/* Extract any contributors and dates from statement description.
   If missing, the corresponding return string is blank.
   See GC_RESET etc. modes above.  Caller must deallocate returned
   string. */
vstring getContrib(long stmtNum, char mode);


/* Extract up to 2 dates after a statement's proof.  If no date is present,
   date1 will be blank.  If no 2nd date is present, date2 will be blank. */
void getProofDate(long stmtNum, vstring *date1, vstring *date2);

/* Get date, month, year fields from a dd-mmm-yyyy date string,
   where dd may be 1 or 2 digits, mmm is 1st 3 letters of month,
   and yyyy is 2 or 4 digits.  A 1 is returned if an error was detected. */
flag parseDate(vstring dateStr, long *dd, long *mmm, long *yyyy);

/* Build date from numeric fields.  mmm should be a number from 1 to 12. */
void buildDate(long dd, long mmm, long yyyy, vstring *dateStr);

/* Compare two dates in the form dd-mmm-yyyy.  -1 = date1 < date2,
   0 = date1 = date2,  1 = date1 > date2.  There is no error checking. */
flag compareDates(vstring date1, vstring date2);

extern vstring g_qsortKey;
      /* Used by qsortStringCmp; pointer only, do not deallocate */
/* Comparison function for qsort */
int qsortStringCmp(const void *p1, const void *p2);

/* Call on exit to free memory */
void freeData(void);

#endif /* METAMATH_MMDATA_H_ */
