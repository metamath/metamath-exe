/*****************************************************************************/
/*        Copyright (C) 2011  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

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
  flag texFlag,
  flag htmlFlg);
/* Show details of step */
void showDetailStep(long statemNum, long detailStep);
/* Summary of statements in proof */
void proofStmtSumm(long statemNum, flag essentialFlag, flag texFlag);
/* Traces back the statements used by a proof, recursively. */
void traceProof(long statemNum,
  flag essentialFlag,
  flag axiomFlag);
void traceProofWork(long statemNum,
  flag essentialFlag,
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
  flag recursiveFlag);
/* Returns any comment that occurs just before a statement */
vstring getDescription(long statemNum);
long getSourceIndentation(long statemNum);
void readInput(void);
void writeInput(flag cleanFlag, flag reformatFlag);
void writeDict(void);
void eraseSource(void);
void verifyProofs(vstring labelMatch, flag verifyFlag);

/* 14-Sep-2012 nm */
/* Take a relative step FIRST, LAST, +nn, -nn (relative to the unknown
   essential steps) or ALL, and return the actual step for use by ASSIGN,
   IMPROVE, REPLACE, LET (or 0 in case of ALL, used by IMPROVE).  In case
   stepStr is an unsigned integer nn, it is assumed to already be an actual
   step and is returned as is.  If format is illegal, -1 is returned.  */
long getStepNum(vstring relStep, /* User's argument */
   nmbrString *pfInProgress, /* proofInProgress.proof */
   flag allFlag /* 1 = "ALL" is permissable */);

/* 14-Sep-2012 nm */
/* This procedure finds the (unique) statement number whose label matches
   stmtName.  Wildcards are allowed in stmtName for user convenience, but
   there must be exactly one match, otherwise an error message is printed,
   and -1 is returned.  For use by PROVE, REPLACE, ASSIGN. */
long getStatementNum(vstring stmtName, /* Possibly with wildcards */
    long maxStmt, /* Must be less than this statement number */
    flag aAllowed, /* 1 means $a is allowed */
    flag pAllowed, /* 1 means $p is allowed */
    flag eAllowed, /* 1 means $e is allowed */
    flag fAllowed, /* 1 means $f is allowed */
    flag efOnlyForMaxStmt); /* If 1, $e and $f must belong to maxStmt */

extern vstring mainFileName;

/* For HELP processing */
extern flag printHelp;
void H(vstring helpLine);

/* For MIDI files - added 8/28/00 */
extern flag midiFlag; /* Set to 1 if typeProof() is to output MIDI file */
extern vstring midiParam; /* Parameter string for MIDI file */
void outputMidi(long plen, nmbrString *indentationLevels,
  nmbrString *logicalFlags, vstring midiParameter, vstring statementLabel);
