/*****************************************************************************/
/*        Copyright (C) 2019  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMCMDS_H_
#define METAMATH_MMCMDS_H_

#include "mmvstr.h"
#include "mmdata.h"

/* Type (i.e. print) a statement */
void typeStatement(long statemNum,
  flag briefFlag,
  flag commentOnlyFlag,
  flag texFlag,
  flag htmlFlg);
/* Type (i.e. print) a proof */
void typeProof(long statemNum,
  flag pipFlag, /* Type proofInProgress instead of source file proof */
  long startStep, long endStep,
  long endIndent,
  flag essentialFlag,
  flag renumberFlag,
  flag unknownFlag,
  flag notUnifiedFlag,
  flag reverseFlag,
  flag noIndentFlag,
  long startColumn,
  flag skipRepeatedSteps, /* 28-Jun-2013 nm Added */
  flag texFlag,
  flag htmlFlg);
/* Show details of step */
void showDetailStep(long statemNum, long detailStep);
/* Summary of statements in proof */
void proofStmtSumm(long statemNum, flag essentialFlag, flag texFlag);
/* Traces back the statements used by a proof, recursively. */
flag traceProof(long statemNum,
  flag essentialFlag,
  flag axiomFlag,
  vstring matchList, /* 19-May-2013 nm */
  vstring traceToList, /* 18-Jul-2015 nm */
  flag testOnlyFlag /* 20-May-2013 nm */);
void traceProofWork(long statemNum,
  flag essentialFlag,
  vstring traceToList, /* 18-Jul-2015 nm */
  vstring *statementUsedFlagsP, /* 'y'/'n' flag that statement is used */
  nmbrString **unprovedListP);
/* Traces back the statements used by a proof, recursively, with tree display.*/
void traceProofTree(long statemNum,
  flag essentialFlag, long endIndent);
void traceProofTreeRec(long statemNum,
  flag essentialFlag, long endIndent, long recursDepth);
/* Counts the number of steps a completely exploded proof would require */
/* (Recursive) */
/* 0 is returned if some assertions have incomplete proofs. */
double countSteps(long statemNum, flag essentialFlag);
/* Traces what statements require the use of a given statement */
vstring traceUsage(long statemNum,
  flag recursiveFlag,
  long cutoffStmt /* if nonzero, stop scan there */ /* 18-Jul-2015 nm */);
vstring htmlDummyVars(long showStmt);  /* 12-Aug-2017 nm */
vstring htmlAllowedSubst(long showStmt);  /* 4-Jan-2014 nm */

void readInput(void);
void writeInput(/* flag cleanFlag, 3-May-2017 */ /* 1 = "/ CLEAN" qualifier was chosen */
                flag reformatFlag /* 1 = "/ FORMAT", 2 = "/REWRAP" */,
                /* 31-Dec-2017 nm */
                flag splitFlag,  /* /SPLIT - write out separate $[ $] includes */
                flag noVersioningFlag, /* /NO_VERSIONING - no ~1 backup */
                flag keepSplitsFlag /* /KEEP_SPLITS - don't delete included files
                                      when /SPIT is not specified */
                );
void writeDict(void);
void eraseSource(void);
void verifyProofs(vstring labelMatch, flag verifyFlag);


/* 7-Nov-2015 nm Added this function for date consistency */
/* If checkFiles = 0, do not open external files.
   If checkFiles = 1, check mm*.html, presence of gifs, etc. */
void verifyMarkup(vstring labelMatch, flag dateSkip, flag topDateSkip,
    flag fileSkip, flag verboseMode); /* 26-Dec-2016 nm */

/* 10-Dec-2018 nm Added */
void processMarkup(vstring inputFileName, vstring outputFileName,
    flag processCss, long actionBits);

/* 3-May-2016 nm */
/* List "discouraged" statements with "(Proof modification is discouraged."
   and "(New usage is discourged.)" comment markup tags. */
void showDiscouraged(void);

/* 14-Sep-2012 nm */
/* Take a relative step FIRST, LAST, +nn, -nn (relative to the unknown
   essential steps) or ALL, and return the actual step for use by ASSIGN,
   IMPROVE, REPLACE, LET (or 0 in case of ALL, used by IMPROVE).  In case
   stepStr is an unsigned integer nn, it is assumed to already be an actual
   step and is returned as is.  If format is illegal, -1 is returned.  */
long getStepNum(vstring relStep, /* User's argument */
   nmbrString *pfInProgress, /* proofInProgress.proof */
   flag allFlag /* 1 = "ALL" is permissable */);

/* 22-Apr-2015 nm */
/* Convert the actual step numbers of an unassigned step to the relative
   -1, -2, etc. offset for SHOW NEW_PROOF ...  /UNKNOWN, to make it easier
   for the user to ASSIGN the relative step number. A 0 is returned
   for the last unknown step.  The step numbers of known steps are
   unchanged.  */
/* The caller must deallocate the returned nmbrString. */
nmbrString *getRelStepNums(nmbrString *pfInProgress);

/* 19-Sep-2012 nm */
/* This procedure finds the next statement number whose label matches
   stmtName.  Wildcards are allowed.  If uniqueFlag is 1,
   there must be exactly one match, otherwise an error message is printed,
   and -1 is returned.  If uniqueFlag is 0, the next match is
   returned, or -1 if there are no more matches.  No error messages are
   printed when uniqueFlag is 0, except for the special case of
   startStmt=1.  For use by PROVE, REPLACE, ASSIGN. */
long getStatementNum(vstring stmtName, /* Possibly with wildcards */
    long startStmt, /* Starting statement number (1 for full scan) */
    long maxStmt, /* Must be LESS THAN this statement number */
    flag aAllowed, /* 1 means $a is allowed */
    flag pAllowed, /* 1 means $p is allowed */
    flag eAllowed, /* 1 means $e is allowed */
    flag fAllowed, /* 1 means $f is allowed */
    flag efOnlyForMaxStmt, /* If 1, $e and $f must belong to maxStmt */
    flag uniqueFlag); /* If 1, match must be unique */

extern vstring mainFileName;

/* For HELP processing */
extern flag printHelp;
void H(vstring helpLine);

/* For MIDI files - added 8/28/00 */
extern flag midiFlag; /* Set to 1 if typeProof() is to output MIDI file */
extern vstring midiParam; /* Parameter string for MIDI file */
void outputMidi(long plen, nmbrString *indentationLevels,
  nmbrString *logicalFlags, vstring midiParameter, vstring statementLabel);


#endif /* METAMATH_MMCMDS_H_ */
