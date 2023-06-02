/*****************************************************************************/
/*        Copyright (C) 2018  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMPARS_H_
#define METAMATH_MMPARS_H_

/*! \file */

#include "mmvstr.h"
#include "mmdata.h"

char *readRawSource(vstring inputBuf, long *size);
void parseKeywords(void);
void parseLabels(void);
void parseMathDecl(void);
void parseStatements(void);
char parseProof(long statemNum);
char parseCompressedProof(long statemNum);
nmbrString *getProof(long statemNum, flag printFlag);

void rawSourceError(char *startFile, char *ptr, long tokenLen, vstring errMsg);
void sourceError(char *ptr, long tokenLen, long stmtNum, vstring errMsg);
void mathTokenError(long tokenNum /* 0 is 1st one */,
    nmbrString *tokenList, long stmtNum, vstring errMsg);
vstring shortDumpRPNStack(void);

/*! Label comparison for qsort */
int labelSortCmp(const void *key1, const void *key2);

/*! Label comparison for bsearch */
int labelSrchCmp(const void *key, const void *data);

/*! Math token comparison for qsort */
int mathSortCmp(const void *key1, const void *key2);

/*! Math token label comparison for bsearch */
int mathSrchCmp(const void *key, const void *data);

/*! Hypothesis and local label comparison for qsort */
int hypAndLocSortCmp(const void *key1, const void *key2);

/*! Hypothesis and local label comparison for bsearch */
int hypAndLocSrchCmp(const void *key, const void *data);

/*! This function returns the length of the white space starting at ptr.
   Comments are considered white space.  ptr should point to the first character
   of the white space.  If ptr does not point to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned. */
long whiteSpaceLen(char *ptr);

// For .mm file splitting
/*! Like whiteSpaceLen except comments are not whitespace */
long rawWhiteSpaceLen(char *ptr);

/*! This function returns the length of the token (non-white-space) starting at
   ptr.  Comments are considered white space.  ptr should point to the first
   character of the token.  If ptr points to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned.  If ptr
   points to a keyword, 0 is returned.  A keyword ends a token. */
long tokenLen(char *ptr);

/*! Unlike tokenLen(), keywords are not treated as special.  In particular:
   if ptr points to a keyword, 0 is NOT returned (instead, 2 is returned),
   and a keyword does NOT end a token (which is a relic of days before
   whitespace surrounding a token was part of the spec, but still serves
   a useful purpose in token() for friendlier error detection). */
long rawTokenLen(char *ptr);

/*! This function returns the length of the proof token starting at
   ptr.  Comments are considered white space.  ptr should point to the first
   character of the token.  If ptr points to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned.  If ptr
   points to a keyword, 0 is returned.  A keyword ends a token.
   ":" is considered a token. */
long proofTokenLen(char *ptr);

/*! Counts the number of \n between start for length chars.
   If length = -1, then use end-of-string 0 to stop.
   If length >= 0, then scan at most length chars, but stop
       if end-of-string 0 is found. */
long countLines(const char *start, long length);

/*! \brief Array with sort keys for all possible labels, including the ones for
   hypotheses (which may not always be active)

   This array is used to see if any label is used anywhere, and is used
   to make sure there are no conflicts when local labels inside of compact
   proofs are generated. */
extern long *g_allLabelKeyBase;
extern long g_numAllLabelKeys;

extern long g_wrkProofMaxSize; /*!< Maximum size so far - it may grow */
struct sortHypAndLoc { // Used for sorting hypAndLocLabel field
  long labelTokenNum;
  void *labelName;
};
/*! \brief Working structure for parsing proofs
  \note This structure should be deallocated by the ERASE command. */
struct wrkProof_struct {
  long numTokens; /*!< Number of tokens in proof */
  long numSteps; /*!< Number of steps in proof */
  long numLocalLabels; /*!< Number of local labels */
  long numHypAndLoc; /*!< Number of active hypotheses and local labels */
  char *localLabelPoolPtr; /*!< Next free location in local label pool */
  long RPNStackPtr; /*!< Offset of end of RPNStack */
  long errorCount; /*!< Errors in proof - used to suppress too many error msgs */
  flag errorSeverity; /*!< 0 = OK, 1 = unknown step, 2 = error, 3 = severe error,
                          4 = not a $p statement */

  // The following pointers will always be allocated with g_wrkProofMaxSize
  // entries.  If a function needs more than g_wrkProofMaxSize, it must
  // reallocate all of these and increase g_wrkProofMaxSize.
  nmbrString *tokenSrcPtrNmbr; /*!< Source parsed into tokens vs. token number
                                    - token size */
  pntrString *tokenSrcPtrPntr; /*!< Source parsed into tokens vs. token number
                                    - token src ptrs */
  nmbrString *stepSrcPtrNmbr; /*!< Pointer to label token in source file
                                   vs. step number - label size */
  pntrString *stepSrcPtrPntr; /*!< Pointer to label token in source file
                                   vs. step number - label src ptrs */
  flag *localLabelFlag; /*!< 1 means step has a local label declaration */
  /*! Sorted ptrs to hyp and local label names + token# */
  struct sortHypAndLoc *hypAndLocLabel;
  char *localLabelPool; /*!< String pool to hold local labels */
  nmbrString *proofString; /*!< The proof in RPN - statement # if > 0
                             or -(step # + 1000) of local label decl if < -1 */
  /*! Ptr to math string vs. each step (Allocated in verifyProof() as needed by nmbrLet()) */
  pntrString *mathStringPtrs;
  nmbrString *RPNStack; /*!< Stack for RPN parsing */

  // For compressed proof parsing
  nmbrString *compressedPfLabelMap; /*!< Map from compressed label to actual */
  long compressedPfNumLabels; /*!< Number of compressed labels */
};
extern struct wrkProof_struct g_WrkProof;

/*! Converts an ASCII string to a nmbrString of math symbols.  statemNum
   provides the context for the parse (to get correct active symbols) */
nmbrString *parseMathTokens(vstring userText, long statemNum);

vstring outputStatement(long stmt, /* flag cleanFlag, */ flag reformatFlag);
/*! Caller must deallocate return string */
vstring rewrapComment(const char *comment);

/*! Lookup $a or $p label and return statement number.
   Return -1 if not found. */
long lookupLabel(const char *label);

// For file splitting

/*! Get the next real $[...$] or virtual $( Begin $[... inclusion */
void getNextInclusion(char *fileBuf, long startOffset, /*!< inputs */
    /*! outputs: */
    long *cmdPos1, long *cmdPos2,
    long *endPos1, long *endPos2,
    char *cmdType, /*!< 'B' = "$( Begin [$..." through "$( End [$...",
                        'I' = "[$...",
                        'S' = "$( Skip [$...",
                        'E' = Start missing matched End
                        'N' = no include found */
    vstring *fileName /*!< name of included file */
    );

/*! This function transfers the content of the statement[] array
   to a linear buffer in preparation for creating the output file. */
vstring writeSourceToBuffer(void);

/*! This function creates split files containing $[ $] inclusions, from
   an unsplit source with $( Begin $[... etc. inclusions
  \note that *fileBuf is assigned to the empty string upon return, to
   conserve memory */
void writeSplitSource(vstring *fileBuf, const char *fileName,
    flag noVersioningFlag, flag noDeleteFlag);

/*! When "write source" does not have the "/split" qualifier, by default
   (i.e. without "/no_delete") the included modules are "deleted" (renamed
   to ~1) since their content will be in the main output file. */
void deleteSplits(vstring *fileBuf, flag noVersioningFlag);

/*! \brief Get file name and line number given a pointer into the read buffer
 * \note The user must deallocate the returned string (file name)
 * \note The globals includeCall structure and includeCalls are used
 */
vstring getFileAndLineNum(const char *buffPtr /* start of read buffer */,
    const char *currentPtr /* place at which to get file name and line no */,
    long *lineNum /* return argument */);

/*! statement[stmtNum].fileName and .lineNum are initialized to "" and 0.
   To save CPU time, they aren't normally assigned until needed, but once
   assigned they can be reused without looking them up again.  This function
   will assign them if they haven't been assigned yet.  It just returns if
   they have already been assigned.
  \note The globals statement[] and sourcePtr are used */
void assignStmtFileAndLineNum(long stmtNum);

/*! Initial read of source file */
vstring readSourceAndIncludes(const char *inputFn, long *size);

/*! Recursively expand the source of an included file */
vstring readInclude(const char *fileBuf, long fileBufOffset,
    /* vstring inclFileName, */ const char *sourceFileName,
    long *size, long parentLineNum, flag *errorFlag);

#endif // METAMATH_MMPARS_H_
