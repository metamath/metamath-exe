/*****************************************************************************/
/*        Copyright (C) 2011  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMPARS_H_
#define METAMATH_MMPARS_H_

#include "mmvstr.h"
#include "mmdata.h"

char *readRawSource(vstring inputFn, long bufOffsetSoFar, long *size);
void parseKeywords(void);
void parseLabels(void);
void parseMathDecl(void);
void parseStatements(void);
char parseProof(long statemNum);
char parseCompressedProof(long statemNum);

void rawSourceError(char *startFile, char *ptr, long tokenLen, long lineNum,
    vstring fileName, vstring errMsg);
void sourceError(char *ptr, long tokenLen, long stmtNum, vstring errMsg);
void mathTokenError(long tokenNum /* 0 is 1st one */,
    nmbrString *tokenList, long stmtNum, vstring errMsg);
vstring shortDumpRPNStack(void);

/* Label comparison for qsort */
int labelSortCmp(const void *key1, const void *key2);

/* Label comparison for bsearch */
int labelSrchCmp(const void *key, const void *data);

/* Math token comparison for qsort */
int mathSortCmp(const void *key1, const void *key2);

/* Math token label comparison for bsearch */
int mathSrchCmp(const void *key, const void *data);

/* Hypothesis and local label comparison for qsort */
int hypAndLocSortCmp(const void *key1, const void *key2);

/* Hypothesis and local label comparison for bsearch */
int hypAndLocSrchCmp(const void *key, const void *data);

/* This function returns the length of the white space starting at ptr.
   Comments are considered white space.  ptr should point to the first character
   of the white space.  If ptr does not point to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned. */
long whiteSpaceLen(char *ptr);

/* This function returns the length of the token (non-white-space) starting at
   ptr.  Comments are considered white space.  ptr should point to the first
   character of the token.  If ptr points to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned.  If ptr
   points to a keyword, 0 is returned.  A keyword ends a token. */
long tokenLen(char *ptr);

/* This function returns the length of the proof token starting at
   ptr.  Comments are considered white space.  ptr should point to the first
   character of the token.  If ptr points to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned.  If ptr
   points to a keyword, 0 is returned.  A keyword ends a token.
   ":" is considered a token. */
long proofTokenLen(char *ptr);

/* Array with sort keys for all possible labels, including the ones for
   hypotheses (which may not always be active) */
/* This array is used to see if any label is used anywhere, and is used
   to make sure there are no conflicts when local labels inside of compact
   proofs are generated. */
extern long *allLabelKeyBase;
extern long numAllLabelKeys;

/* Working structure for parsing proofs */
/* This structure should be deallocated by the ERASE command. */
extern long wrkProofMaxSize; /* Maximum size so far - it may grow */
struct sortHypAndLoc {  /* Used for sorting hypAndLocLabel field */
  long labelTokenNum;
  void *labelName;
};
struct wrkProof_struct {
  long numTokens; /* Number of tokens in proof */
  long numSteps; /* Number of steps in proof */
  long numLocalLabels; /* Number of local labels */
  long numHypAndLoc; /* Number of active hypotheses and local labels */
  char *localLabelPoolPtr; /* Next free location in local label pool */
  long RPNStackPtr; /* Offset of end of RPNStack */
  long errorCount; /* Errors in proof - used to suppress to many error msgs */
  flag errorSeverity; /* 0 = OK, 1 = unk step, 2 = error, 3 = severe error,
                          4 = not a $p statement */

  /* The following pointers will always be allocated with wrkProofMaxSize
     entries.  If a function needs more than wrkProofMaxSize, it must
     reallocate all of these and increase wrkProofMaxSize. */
  nmbrString *tokenSrcPtrNmbr; /* Source parsed into tokens vs. token number
                                    - token size */
  pntrString *tokenSrcPtrPntr; /* Source parsed into tokens vs. token number
                                    - token src ptrs */
  nmbrString *stepSrcPtrNmbr; /* Pointer to label token in source file
                                   vs. step number - label size */
  pntrString *stepSrcPtrPntr; /* Pointer to label token in source file
                                   vs. step number - label src ptrs */
  flag *localLabelFlag; /* 1 means step has a local label declaration */
  struct sortHypAndLoc *hypAndLocLabel;
                        /* Sorted ptrs to hyp and local label names + token# */
  char *localLabelPool; /* String pool to hold local labels */
  nmbrString *proofString; /* The proof in RPN - statement # if > 0
                             or -(step # + 1000) of local label decl if < -1 */
  pntrString *mathStringPtrs; /* Ptr to math string vs. each step */
                      /* (Allocated in verifyProof() as needed by nmbrLet()) */
  nmbrString *RPNStack; /* Stack for RPN parsing */

  /* For compressed proof parsing */
  nmbrString *compressedPfLabelMap; /* Map from compressed label to actual */
  long compressedPfNumLabels; /* Number of compressed labels */

};
extern struct wrkProof_struct wrkProof;

/* Converts an ASCII string to a nmbrString of math symbols.  statemNum
   provides the context for the parse (to get correct active symbols) */
nmbrString *parseMathTokens(vstring userText, long statemNum);

/* 12-Jun-2011 nm Added reformatFlag */
vstring outputStatement(long stmt, flag cleanFlag, flag reformatFlag);
/* 12-Jun-2011 nm */
/* Caller must deallocate return string */
vstring rewrapComment(vstring comment);

/* 10/10/02 */
/* Lookup $a or $p label and return statement number.
   Return -1 if not found. */
long lookupLabel(vstring label);

#endif /* METAMATH_MMPARS_H_ */
