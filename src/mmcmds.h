/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMCMDS_H_
#define METAMATH_MMCMDS_H_

/*! \file */

#include "mmvstr.h"
#include "mmdata.h"

/*! Type (i.e. print) a statement */
void typeStatement(long statemNum,
  flag briefFlag,
  flag commentOnlyFlag,
  flag texFlag,
  flag htmlFlag);
/*! Type (i.e. print) a proof */
void typeProof(long statemNum,
  flag pipFlag, /*!< Type proofInProgress instead of source file proof */
  long startStep, long endStep,
  long endIndent,
  flag essentialFlag,
  flag renumberFlag,
  flag unknownFlag,
  flag notUnifiedFlag,
  flag reverseFlag,
  flag noIndentFlag,
  long startColumn,
  flag skipRepeatedSteps,
  flag texFlag,
  flag htmlFlag);
/*! Show details of step */
void showDetailStep(long statemNum, long detailStep);
/*! Summary of statements in proof */
void proofStmtSumm(long statemNum, flag essentialFlag, flag texFlag);
/*! Traces back the statements used by a proof, recursively. */
char traceProof(long statemNum,
  flag essentialFlag,
  flag axiomFlag,
  vstring matchList,
  vstring traceToList,
  flag testOnlyFlag,
  flag abortOnQuit);
void traceProofWork(long statemNum,
  flag essentialFlag,
  vstring traceToList,
  vstring *statementUsedFlagsP, /*!< 'y'/'n' flag that statement is used */
  nmbrString **unprovedListP);
/*! Traces back the statements used by a proof, recursively, with tree display.*/
void traceProofTree(long statemNum,
  flag essentialFlag, long endIndent);
void traceProofTreeRec(long statemNum,
  flag essentialFlag, long endIndent, long recursDepth);
/*! Counts the number of steps a completely exploded proof would require
  (Recursive)
  0 is returned if some assertions have incomplete proofs. */
double countSteps(long statemNum, flag essentialFlag);
/*! Traces what statements require the use of a given statement */
vstring traceUsage(long statemNum,
  flag recursiveFlag,
  long cutoffStmt /* if nonzero, stop scan there */);
vstring htmlDummyVars(long showStmt);
vstring htmlAllowedSubst(long showStmt);

void readInput(void);
/*! WRITE SOURCE command */
void writeSource(
  flag reformatFlag /* 1 = "/ FORMAT", 2 = "/REWRAP" */,
  flag splitFlag, // /SPLIT - write out separate $[ $] includes
  flag noVersioningFlag, // /NO_VERSIONING - no ~1 backup
  // /KEEP_SPLITS - don't delete included files when /SPIT is not specified
  flag keepSplitsFlag, 
  vstring extractLabels); // "" means write everything

/*! Get info for WRITE SOURCE ... / EXTRACT */
void writeExtractedSource(vstring extractLabels, // EXTRACT argument provided by user
  vstring fullOutput_fn, flag noVersioningFlag);

void fixUndefinedLabels(vstring extractNeeded, vstring *buf);

void writeDict(void);
void eraseSource(void);
void verifyProofs(vstring labelMatch, flag verifyFlag);

/*! If checkFiles = 0, do not open external files.
   If checkFiles = 1, check for presence of gifs and biblio file */
void verifyMarkup(vstring labelMatch, flag dateCheck, flag topDateCheck,
    flag fileCheck,
    flag underscoreCheck,
    flag mathboxCheck,
    flag verboseMode);

void processMarkup(vstring inputFileName, vstring outputFileName,
    flag processCss, long actionBits);

/*! List "discouraged" statements with "(Proof modification is discouraged."
   and "(New usage is discouraged.)" comment markup tags. */
void showDiscouraged(void);

/*! Take a relative step FIRST, LAST, +nn, -nn (relative to the unknown
   essential steps) or ALL, and return the actual step for use by ASSIGN,
   IMPROVE, REPLACE, LET (or 0 in case of ALL, used by IMPROVE).  In case
   stepStr is an unsigned integer nn, it is assumed to already be an actual
   step and is returned as is.  If format is illegal, -1 is returned.  */
long getStepNum(vstring relStep, /*!< User's argument */
   nmbrString *pfInProgress, /*!< proofInProgress.proof */
   flag allFlag /*!< 1 = "ALL" is permissible */);

/*! Convert the actual step numbers of an unassigned step to the relative
   -1, -2, etc. offset for SHOW NEW_PROOF ...  /UNKNOWN, to make it easier
   for the user to ASSIGN the relative step number. A 0 is returned
   for the last unknown step.  The step numbers of known steps are
   unchanged.
   The caller must deallocate the returned nmbrString. */
nmbrString *getRelStepNums(nmbrString *pfInProgress);

/*! This procedure finds the next statement number whose label matches
   stmtName.  Wildcards are allowed.  If uniqueFlag is 1,
   there must be exactly one match, otherwise an error message is printed,
   and -1 is returned.  If uniqueFlag is 0, the next match is
   returned, or -1 if there are no more matches.  No error messages are
   printed when uniqueFlag is 0, except for the special case of
   startStmt=1.  For use by PROVE, REPLACE, ASSIGN. */
long getStatementNum(
  vstring stmtName, /*!< Possibly with wildcards */
  long startStmt, /*!< Starting statement number (1 for full scan) */
  long maxStmt, /*!< Must be LESS THAN this statement number */
  flag aAllowed, /*!< 1 means $a is allowed */
  flag pAllowed, /*!< 1 means $p is allowed */
  flag eAllowed, /*!< 1 means $e is allowed */
  flag fAllowed, /*!< 1 means $f is allowed */
  flag efOnlyForMaxStmt, /*!< If 1, $e and $f must belong to maxStmt */
  flag uniqueFlag /*!< If 1, match must be unique */
);

/*! For HELP processing */
void H(vstring helpLine);

/*! For MIDI files */
extern flag g_midiFlag; /*!< Set to 1 if typeProof() is to output MIDI file */
extern vstring g_midiParam; /*!< Parameter string for MIDI file */
void outputMidi(long plen, nmbrString *indentationLevels,
  nmbrString *logicalFlags, vstring g_midiParameter, vstring statementLabel);

#endif // METAMATH_MMCMDS_H_
