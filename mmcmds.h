/*****************************************************************************/
/*       Copyright (C) 2002  NORMAN D. MEGILL nm@alum.mit.edu                */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* Type (i.e. print) a statement */
void typeStatement(long statemNum,
  flag briefFlag,
  flag commentOnlyFlag,
  flag texFlag,
  flag htmlFlag);
/* Type (i.e. print) a proof */
void typeProof(long statemNum,
  flag pipFlag, /* Type proofInProgress instead of source file proof */
  long startStep, long endStep,
  long startIndent, long endIndent,
  flag essentialFlag,
  flag renumberFlag,
  flag unknownFlag,
  flag notUnifiedFlag,
  flag reverseFlag,
  flag noIndentFlag,
  long startColumn,
  flag texFlag,
  flag htmlFlag);
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
void writeInput(void);
void writeDict(void);
void eraseSource(void);
void verifyProofs(vstring labelMatch, flag verifyFlag);

extern vstring mainFileName;

/* For HELP processing */
extern flag printHelp;
void H(vstring helpLine);

/* Pink statement number for HTML pages */
long pinkNumber(long statemNum);

/* For MIDI files - added 8/28/00 */
extern flag midiFlag; /* Set to 1 if typeProof() is to output MIDI file */
extern vstring midiParam; /* Parameter string for MIDI file */
void outputMidi(long plen, nmbrString *indentationLevels,
  nmbrString *logicalFlags, vstring midiParam, vstring statementLabel);
