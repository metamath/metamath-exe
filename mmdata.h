/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* mmdata.h - includes for some principal data structures and data-string
   handling */

#ifndef METAMATH_MMDATA_H_
#define METAMATH_MMDATA_H_

#include "mmvstr.h"

/*E*/extern long db,db0,db1,db2,db3,db4,db5,db6,db7,db8,db9;/* debugging flags & variables */
typedef char flag; /* A "flag" is simply a character intended for use as a
                      yes/no logical Boolean; 0 = no and 1 = yes */

extern flag g_listMode; /* 0 = metamath, 1 = list utility */
extern flag g_toolsMode; /* In metamath mode:  0 = metamath, 1 = tools */

typedef long nmbrString; /* String of numbers */
typedef void* pntrString; /* String of pointers */



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
void outOfMemory(vstring msg);

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

/* Null pntrString -- NULL flags the end of a pntrString */
struct nullPntrStruct {
    long poolLoc;
    long allocSize;
    long actualSize;
    pntrString nullElement; };
extern struct nullPntrStruct g_PntrNull;
#define NULL_PNTRSTRING &(g_PntrNull.nullElement)


/* This function returns a 1 if any entry in a comma-separated list
   matches using the matches() function. */
flag matchesList(vstring testString, vstring pattern, char wildCard,
    char oneCharWildCard);

/* This function returns a 1 if the first argument matches the pattern of
   the second argument.  The second argument may have 0-or-more and
   exactly-1 character match wildcards, typically '*' and '?'.*/
flag matches(vstring testString, vstring pattern, char wildCard,
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
void nmbrMakeTempAlloc(nmbrString *s);
                                    /* Make string have temporary allocation to be
                                    released by next nmbrLet() */

/**************************************************/


/* String assignment - MUST be used to assign vstrings */
void nmbrLet(nmbrString **target,nmbrString *source);

/* String concatenation - last argument MUST be NULL */
nmbrString *nmbrCat(nmbrString *string1,...);

/* Emulation of nmbrString functions similar to BASIC string functions */
nmbrString *nmbrSeg(nmbrString *sin, long p1, long p2);
nmbrString *nmbrMid(nmbrString *sin, long p, long l);
nmbrString *nmbrLeft(nmbrString *sin, long n);
nmbrString *nmbrRight(nmbrString *sin, long n);

/* Allocate and return an "empty" string n "characters" long */
nmbrString *nmbrSpace(long n);

long nmbrLen(nmbrString *s);
long nmbrAllocLen(nmbrString *s);
void nmbrZapLen(nmbrString *s, long length);

/* Search for string2 in string 1 starting at start_position */
long nmbrInstr(long start, nmbrString *sin, nmbrString *s);

/* Search for string2 in string 1 in reverse starting at start_position */
/* (Reverse nmbrInstr) */
long nmbrRevInstr(long start_position,nmbrString *string1,
    nmbrString *string2);

/* 1 if strings are equal, 0 otherwise */
int nmbrEq(nmbrString *sout,nmbrString *sin);

/* Converts mString to a vstring with one space between tokens */
vstring nmbrCvtMToVString(nmbrString *s);

/* Converts rString to a vstring with one space between tokens */
vstring nmbrCvtRToVString(nmbrString *s,
    flag explicitTargets,
    long statemNum);

/* Get step numbers in an rString - needed by cvtRToVString & elsewhere */
nmbrString *nmbrGetProofStepNumbs(nmbrString *reason);

/* Converts any nmbrString to an ASCII string of numbers corresponding
   to the .tokenNum field -- used for debugging only. */
vstring nmbrCvtAnyToVString(nmbrString *s);

/* Extract variables from a math token string */
nmbrString *nmbrExtractVars(nmbrString *m);

/* Determine if an element is in a nmbrString; return position if it is */
long nmbrElementIn(long start, nmbrString *g, long element);

/* Add a single number to end of a nmbrString - faster than nmbrCat */
nmbrString *nmbrAddElement(nmbrString *g, long element);

/* Get the set union of two math token strings (presumably
   variable lists) */
nmbrString *nmbrUnion(nmbrString *m1,nmbrString *m2);

/* Get the set intersection of two math token strings (presumably
   variable lists) */
nmbrString *nmbrIntersection(nmbrString *m1,nmbrString *m2);

/* Get the set difference m1-m2 of two math token strings (presumably
   variable lists) */
nmbrString *nmbrSetMinus(nmbrString *m1,nmbrString *m2);



/* This is a utility function that returns the length of a subproof that
   ends at step */
long nmbrGetSubproofLen(nmbrString *proof, long step);

/* This function returns a "squished" proof, putting in {} references
   to previous subproofs. */
nmbrString *nmbrSquishProof(nmbrString *proof);

/* This function unsquishes a "squished" proof, replacing {} references
   to previous subproofs by the subproofs themselvs.  The returned nmbrString
   must be deallocated by the caller. */
nmbrString *nmbrUnsquishProof(nmbrString *proof);

/* This function returns the indentation level vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0.  The caller is responsible for deallocating the
   result. */
nmbrString *nmbrGetIndentation(nmbrString *proof,
  long startingLevel);

/* This function returns essential (1) or floating (0) vs. step number of a
   proof string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0.  The caller is responsible for deallocating the
   result. */
nmbrString *nmbrGetEssential(nmbrString *proof);

/* This function returns the target hypothesis vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively.  The caller is responsible for
   deallocating the result.  statemNum is the statement being proved. */
nmbrString *nmbrGetTargetHyp(nmbrString *proof, long statemNum);

/* Converts a proof string to a compressed-proof-format ASCII string.
   Normally, the proof string would be compacted with squishProof first,
   although it's not a requirement. */
/* The statement number is needed because required hypotheses are
   implicit in the compressed proof. */
vstring compressProof(nmbrString *proof, long statemNum,
    flag oldCompressionAlgorithm);

/* Gets length of the ASCII form of a compressed proof */
long compressedProofSize(nmbrString *proof, long statemNum);


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
void pntrMakeTempAlloc(pntrString *s);
                                    /* Make string have temporary allocation to be
                                    released by next pntrLet() */

/**************************************************/


/* String assignment - MUST be used to assign vstrings */
void pntrLet(pntrString **target,pntrString *source);

/* String concatenation - last argument MUST be NULL */
pntrString *pntrCat(pntrString *string1,...);

/* Emulation of pntrString functions similar to BASIC string functions */
pntrString *pntrSeg(pntrString *sin, long p1, long p2);
pntrString *pntrMid(pntrString *sin, long p, long length);
pntrString *pntrLeft(pntrString *sin, long n);
pntrString *pntrRight(pntrString *sin, long n);

/* Allocate and return an "empty" string n "characters" long */
pntrString *pntrSpace(long n);

/* Allocate and return an "empty" string n "characters" long
   initialized to nmbrStrings instead of vStrings */
pntrString *pntrNSpace(long n);

/* Allocate and return an "empty" string n "characters" long
   initialized to pntrStrings instead of vStrings */
pntrString *pntrPSpace(long n);

long pntrLen(pntrString *s);
long pntrAllocLen(pntrString *s);
void pntrZapLen(pntrString *s, long length);

/* Search for string2 in string 1 starting at start_position */
long pntrInstr(long start, pntrString *sin, pntrString *s);

/* Search for string2 in string 1 in reverse starting at start_position */
/* (Reverse pntrInstr) */
long pntrRevInstr(long start_position,pntrString *string1,
    pntrString *string2);

/* 1 if strings are equal, 0 otherwise */
int pntrEq(pntrString *sout,pntrString *sin);

/* Add a single null string element to a pntrString - faster than pntrCat */
pntrString *pntrAddElement(pntrString *g);

/* Add a single null pntrString element to a pntrString - faster than pntrCat */
pntrString *pntrAddGElement(pntrString *g);

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
