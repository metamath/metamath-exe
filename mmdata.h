/*****************************************************************************/
/*               Copyright (C) 1997, NORMAN D. MEGILL                        */
/*****************************************************************************/

/*34567890123456 (79-character line to adjust text window width) 678901234567*/

/* mmdata.h - includes for some principle data structures and data-string handling */

/*E*/extern long db,db0,db1,db2,db3,db4,db5,db6,db7,db8,db9;/* debugging flags & variables */
/*??? To work on:
   (20) Add a command line editor (compiler-specific - some may not have it)
        - handle arrows & recall buffer
   (21) Add selected DO LIST functions - PROCESS LINES or LINE PROCESSOR?
        Commands:  ADD, CLEAN?, DELETE, SUBSTITUTE, SWAP, COUNT, DIFFERENCE
        (general DIFF), DUPLICATE, PARALLEL, SPLIT, COPY, TYPE, RENAME?
   (23) When near completion:  Verify that each of the error messages
        works with an actual error
*/
typedef char flag;

typedef long nmbrString; /* String of numbers */
typedef void* pntrString; /* String of pointers */

/* Structure for generalized string handling
  - treats structure strings similarly to vstrings in vstring.h */
struct genString {
  long tokenNum; /* A -1 here denotes end-of-string */
  void *whiteSpace; /* White space and comments following token */
        /* whiteSpace is non-null only for mStrings and rStrings */
        /* that are read from the user's input file */
        /* (except, it is used as a genString pointer in mmveri.c;
            this is why it is void instead of vstring */
};
/* Each genString "character" is a structure */



enum mTokenType { var__, con__ };
#define lb_ '{' /* ${ */
#define rb_ '}' /* $} */
#define v__ 'v' /* $v */
#define c__ 'c' /* $c */
#define a__ 'a' /* $a */
#define d__ 'd' /* $e */
#define e__ 'e' /* $e */
#define f__ 'f' /* $e */
#define p__ 'p' /* $p */
#define eq_ '=' /* $= */
#define sc_ '.' /* $. (historically, used to be $; (semicolon) ) */
#define illegal_ '?' /* anything else */
/* Global variables related to current statement */
extern int currentScope;
extern long beginScopeStatementNum;

struct statement_struct { /* Array index is statement number, starting at 1 */
  long lineNum; /* Line number in file; 0 means not yet determined */
  vstring fileName; /* File statement is in; "" means not yet determined */
  vstring labelName; /* Label of statement */
  flag uniqueLabel; /* Flag that label is unique (future implementations may
                      allow duplicate labels on hypotheses) */
  char type;    /* 2nd character of keyword, e.g. 'e' for $e */
  int scope;    /* Block scope level, increased by ${ and decreased by ${ */
  long beginScopeStatementNum;  /* statement of last ${ */
  vstring labelSectionPtr; /* Source code before statement keyword */
  long labelSectionLen;
  vstring mathSectionPtr; /* Source code between keyword and $= or $. */
  long mathSectionLen;
  vstring proofSectionPtr; /* Source code between $= and $. */
  long proofSectionLen;
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
  };
  
/* Sort keys for statement labels (allocated by parseLabels) */
extern long *labelKey;

struct includeCall_struct {
  /* This structure holds all information related to $[ $] (include) statements
     in the input source files, for error message processing and for
     writing out the source files. */
  long bufOffset; /* Character offset in source buffer that the include
                      applies to */
  vstring current_fn;
  long current_line;
  vstring calledBy_fn;
  long calledBy_line;
  vstring includeSource; /* Source code with the include statement (the include
                         statement is replaced by the included text; this
                         is where the include statement itself is stored). */
  long length; /* length of included file */
  flag pushOrPop; /* 1 = start of include statement, 0 = end of it */
  flag alreadyIncluded; /* 1 = this file has already been included; don't
                           include it again. */
  };

struct mathToken_struct {
  vstring tokenName; /* may be used more than once at different scopes */
  long length; /* to speed up parsing scans */
  char tokenType; /* variable or constant */
  flag active;  /* 1 if token is recognized in current scope */
  int scope;    /* scope level token was declared at */
  long tmp;     /* Temporary field use to speed up certain functions */
  long statement; /* Statement declared in */
  long endStatement; /* Statement of end of scope it was declared in */
  };

/* Sort keys for math tokens (allocated by parseMathDecl) */
extern long *mathKey;

extern long MAX_STATEMENTS;
extern long MAX_MATHTOKENS;
extern struct statement_struct *statement;
/*0bs*/ /*extern struct label_struct *label;*/
extern struct mathToken_struct *mathToken;
extern long statements, /*labels, */mathTokens;
extern long maxMathTokenLength;

extern long MAX_INCLUDECALLS;
extern struct includeCall_struct *includeCall;
extern long includeCalls;

extern char *sourcePtr; /* Pointer to buffer in memory with input source */
extern long sourceLen; /* Number of chars. in all inputs files combined (after includes)*/

/* Allocation and deallocation in memory pool */
void *poolFixedMalloc(long size /* bytes */);
void *poolMalloc(long size /* bytes */);
void poolFree(void *ptr);
void addToUsedPool(void *ptr);
/* Purges reset memory pool usage */
void memFreePoolPurge(flag untilOK);
void memUsedPoolPurge(flag untilOK);
void memUsedPoolHalfPurge(flag untilOK);
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


/* Null genString -- -1 flags the end of an genString */
struct nullGenStruct {
    long poolLoc;
    long allocSize;
    long actualSize;
    struct genString nullElement; };
extern struct nullGenStruct genNull;
#define NULL_GENSTRING &(genNull.nullElement)
/*extern struct genString genNullString;*/ /* old */

/* Null nmbrString -- -1 flags the end of a nmbrString */
struct nullNmbrStruct {
    long poolLoc;
    long allocSize;
    long actualSize;
    nmbrString nullElement; };
extern struct nullNmbrStruct nmbrNull;
#define NULL_NMBRSTRING &(nmbrNull.nullElement)

/* Null pntrString -- NULL flags the end of a pntrString */
struct nullPntrStruct {
    long poolLoc;
    long allocSize;
    long actualSize;
    pntrString nullElement; };
extern struct nullPntrStruct pntrNull;
#define NULL_PNTRSTRING &(pntrNull.nullElement)

/* Make string have temporary allocation to be released by next genLet() */
/* Warning:  after genMakeTempAlloc() is called, the genString may NOT be
   assigned again with genLet() */
void genMakeTempAlloc(struct genString *s);


/******* Special pupose routines for better
      memory allocation (use with caution) *******/

extern genTempAllocStackTop;        /* Top of stack for genTempAlloc function */
extern genStartTempAllocStack;      /* Where to start freeing temporary allocation
                                    when genLet() is called (normally 0, except for
                                    nested genString functions) */

/* Make string have temporary allocation to be released by next genLet() */
/* Warning:  after genMakeTempAlloc() is called, the genString may NOT be
   assigned again with genLet() */
void genMakeTempAlloc(struct genString *s);
                                    /* Make string have temporary allocation to be
                                    released by next genLet() */

/**************************************************/


/* String assignment - MUST be used to assign vstrings */
void genLet(struct genString **target,struct genString *source);

/* String concatenation - last argument MUST be NULL */
struct genString *genCat(struct genString *string1,...);

/* Emulation of genString functions similar to BASIC string functions */
struct genString *genSeg(struct genString *sin, long p1, long p2);
struct genString *genMid(struct genString *sin, long p, long l);
struct genString *genLeft(struct genString *sin, long n);
struct genString *genRight(struct genString *sin, long n);

/* Allocate and return an "empty" string n "characters" long */
struct genString *genSpace(long n);

/* Allocate and return an "empty" string n "characters" long 
   with whiteSpace initialized to genStrings instead of vStrings */
struct genString *genGSpace(long n);

long genLen(struct genString *s);
long genAllocLen(struct genString *s);
void genZapLen(struct genString *s, long len);

/* Search for string2 in string 1 starting at start_position */
long genInstr(long start, struct genString *sin, struct genString *s);

/* Search for string2 in string 1 in reverse starting at start_position */
/* (Reverse genInstr) */
long genRevInstr(long start_position,struct genString *string1,
    struct genString *string2);

/* 1 if strings are equal, 0 otherwise */
int genEq(struct genString *sout,struct genString *sin);

/* Converts mString to a vstring with correct white space between tokens */
/* If whiteSpaceFlag is 1, put actual white space between tokens, otherwise
   put a single space (for error messages) */
vstring cvtMToVString(struct genString *s, flag whiteSpaceFlag);

/* Converts rString to a vstring with correct white space between tokens */
/* If whiteSpaceFlag is 1, put actual white space between tokens, otherwise
   put a single space (for error messages) */
vstring cvtRToVString(struct genString *s, flag whiteSpaceFlag);

/* Get step numbers in an rString - needed by cvtRToVString & elsewhere */
struct genString *getProofStepNumbs(struct genString *reason);

/* Converts any genString to an ASCII string of numbers corresponding
   to the .tokenNum field -- used for debugging only. */
vstring cvtAnyToVString(struct genString *s);

/* Extract variables from a math token string */
struct genString *genExtractVars(struct genString *m);

/* Determine if an element is in a genString; return position if it is */
long genElementIn(long start, struct genString *g, long element);

/* Add a single string element to a genString - faster than genCat */
struct genString *genAddElement(struct genString *g, long element);

/* Add a single genString element to a genString - faster than genCat */
struct genString *genAddGElement(struct genString *g, long element);

/* Get the set union of two math token strings (presumably
   variable lists) */
struct genString *genUnion(struct genString *m1,struct genString *m2);

/* Get the set intersection of two math token strings (presumably
   variable lists) */
struct genString *genIntersection(struct genString *m1,struct genString *m2);

/* Get the set difference m1-m2 of two math token strings (presumably
   variable lists) */
struct genString *genSetMinus(struct genString *m1,struct genString *m2);



/* This is a utility function that returns the length of a subproof that
   ends at step */
long getSubProofLen(struct genString *proof, long step);

/* This function returns a "squished" proof, putting in {} references
   to previous subproofs. */
struct genString *squishProof(struct genString *proof);

/* This function unsquishes a "squished" proof, replacing {} references
   to previous subproofs by the subproofs themselvs.  The returned genString
   must be deallocated by the caller. */
struct genString *unsquishProof(struct genString *proof);

/* This function returns the indentation level vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0.  The caller is responsible for deallocating the
   result. */
struct genString *getIndentation(struct genString *proof,
  long startingLevel);

/* This function returns essential (1) or floating (0) vs. step number of a
   proof string.  This information is used for formatting proof displays.  The
   function calls itself recursively, but the first call should be with
   startingLevel = 0.  The caller is responsible for deallocating the
   result. */
struct genString *getEssential(struct genString *proof);

/* This function returns the target hypothesis vs. step number of a proof
   string.  This information is used for formatting proof displays.  The
   function calls itself recursively.  The caller is responsible for
   deallocating the result.  statemNum is the statement being proved. */
struct genString *getTargetHyp(struct genString *proof, long statemNum);

/* This function returns a 1 if the first argument matches the pattern of
   the second argument.  The second argument may have '*' wildcard characters.*/
flag matches(vstring testString, vstring pattern, char wildCard);



/*******************************************************************/
/*********** Number string functions *******************************/
/*******************************************************************/

/* Make string have temporary allocation to be released by next nmbrLet() */
/* Warning:  after nmbrMakeTempAlloc() is called, the nmbrString may NOT be
   assigned again with nmbrLet() */
void nmbrMakeTempAlloc(nmbrString *s);


/******* Special pupose routines for better
      memory allocation (use with caution) *******/

extern nmbrTempAllocStackTop;        /* Top of stack for nmbrTempAlloc function */
extern nmbrStartTempAllocStack;      /* Where to start freeing temporary allocation
                                    when nmbrLet() is called (normally 0, except for
                                    nested nmbrString functions) */

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
void nmbrZapLen(nmbrString *s, long len);

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
vstring nmbrCvtRToVString(nmbrString *s);

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
long nmbrGetSubProofLen(nmbrString *proof, long step);

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
vstring compressProof(nmbrString *proof, long statemNum);

/* Converts genString to nmbrString */
nmbrString *genToNmbr(struct genString *sin);

/* Converts genString to pntrString */
pntrString *genToPntr(struct genString *sin);

/* Converts nmbrString to genString */
struct genString *nmbrToGGen(nmbrString *sin);


/*******************************************************************/
/*********** Pointer string functions ******************************/
/*******************************************************************/

/* Make string have temporary allocation to be released by next pntrLet() */
/* Warning:  after pntrMakeTempAlloc() is called, the pntrString may NOT be
   assigned again with pntrLet() */
void pntrMakeTempAlloc(pntrString *s);


/******* Special pupose routines for better
      memory allocation (use with caution) *******/

extern pntrTempAllocStackTop;        /* Top of stack for pntrTempAlloc function */
extern pntrStartTempAllocStack;      /* Where to start freeing temporary allocation
                                    when pntrLet() is called (normally 0, except for
                                    nested pntrString functions) */

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
pntrString *pntrMid(pntrString *sin, long p, long l);
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
void pntrZapLen(pntrString *s, long len);

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

