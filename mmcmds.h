/*****************************************************************************/
/*               Copyright (C) 1997, NORMAN D. MEGILL                        */
/*****************************************************************************/

/*34567890123456 (79-character line to adjust text window width) 678901234567*/

void typeExpandedProof(nmbrString *reason,
    pntrString *expansion,nmbrString *stepNumbers,
    char displayModeFlag);
void typeCompactProof(nmbrString *reason);
void typeEnumProof(nmbrString *reason);
void typeEnumProof2(nmbrString *reason);
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
  flag texFlag);
/* Show details of step */
void showDetailStep(long statemNum, long detailStep);
/* Summary of statements in proof */
void proofStmtSumm(long statemNum, flag essentialFlag, flag texFlag);
/* Traces back the statements used by a proof, recursively. */
void traceProof(long statemNum,
  flag essentialFlag,
  flag axiomFlag);
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
void traceUsage(long statemNum,
  flag recursiveFlag);
vstring getDescription(long statemNum);
void readInput(void);
void writeInput(void);
void writeDict(void);
void eraseSource(void);
void verifyProofs(vstring labelMatch, flag verifyFlag);

extern vstring mainFileName;

/* For HELP processing */
extern flag printHelp;
void H(vstring helpLine);

