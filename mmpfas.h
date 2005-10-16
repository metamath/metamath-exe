/*****************************************************************************/
/*        Copyright (C) 2005  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

extern long proveStatement; /* The statement to be proved */
extern flag proofChangedFlag; /* Flag to push 'undo' stack */

extern long userMaxProveFloat; /* Upper limit for proveFloating */

extern long dummyVars; /* The number of dummy variables currently declared */
extern long pipDummyVars; /* Number of dummy vars used by proof in progress */

/* Structure for holding a proof in progress. */
/* This structure should be deallocated after use. */
struct pip_struct {
  nmbrString *proof; /* The proof itself */
  pntrString *target; /* Left hand side of = in display */
  pntrString *source; /* Right hand side of = in display */
  pntrString *user; /* User-specified math string assignments to step */
};
extern struct pip_struct proofInProgress;

/* Interactively select statement assignments that match */
/* maxEssential is the maximum number of essential hypotheses that a
   statement may have in order to be included in the matched list. */
void interactiveMatch(long step, long maxEssential);

/* Assign a statement to an unknown proof step */
void assignStatement(long statemNum, long step);

/* Replace a known step with another statement.  Returns 0-length proof
   if an exact hypothesis match wasn't found, otherwise returns a subproof
   starting at the replaced step, with the step replaced by
   replStatemNum. */
nmbrString *replaceStatement(long replStatemNum, long step, long provStmtNum);

/* Add a subproof in place of an unknown step to proofInProgress.  The
   .target, .source, and .user fields are initialized to empty (except
   .target of the deleted unknown step is retained). */
void addSubProof(nmbrString *subProof, long step);

/* Delete a subproof starting (in reverse from) step.  The step is replaced
   with an unknown step, and its .target field is retained. */
void deleteSubProof(long step);

/* Check to see if a statement will match the proofInProgress.target (or .user)
   of an unknown step.  Returns 1 if match, 0 if not, 2 if unification
   timed out. */
char checkStmtMatch(long statemNum, long step);

/* Check to see if a (user-specified) math string will match the
   proofInProgress.target (or .user) of an step.  Returns 1 if match, 0 if
   not, 2 if unification timed out. */
char checkMStringMatch(nmbrString *mString, long step);

/* Find proof of formula or simple theorem (no new vars in $e's) */
/* maxEDepth is the maximum depth at which statements with $e hypotheses are
   considered.  A value of 0 means none are considered. */
nmbrString *proveFloating(nmbrString *mString, long statemNum, long maxEDepth,
    long step, flag noDistinct);

/* Shorten proof by using specified statement. */
void minimizeProof(long repStatemNum, long prvStatemNum, flag
    allowGrowthFlag);

/* Initialize proofInProgress.source of the step, and .target of all
   hypotheses, to schemes using new dummy variables. */
void initStep(long step);

/* Look for completely known subproofs in proofInProgress.proof and
   assign proofInProgress.target and .source.  Calls assignKnownSteps(). */
void assignKnownSubProofs(void);

/* This function assigns math strings to all steps (proofInProgress.target and
   .source fields) in a subproof with all known steps. */
void assignKnownSteps(long startStep, long sbProofLen);

/* Interactive unify a step.  Calls interactiveUnify(). */
/* If messageFlag is 1, a message will be issued if the
   step is already unified.   If messageFlag is 0, show the step #
   being unified.  If messageFlag is 2, don't print step #, and do nothing
   if step is already unified. */
void interactiveUnifyStep(long step, char messageFlag);

/* Interactively select one of several possible unifications */
/* Returns:  0 = no unification possible
             1 = unification was selected; held in stateVector
             2 = unification timed out
             3 = no unification was selected */
char interactiveUnify(nmbrString *schemeA, nmbrString *schemeB,
    pntrString **stateVector);

/* Automatically unify steps with unique unification */
void autoUnify(flag congrats);

/* Make stateVector substitutions in all steps.  The stateVector must
   contain the result of a valid unification. */
void makeSubstAll(pntrString *stateVector);

/* Replace a dummy variable with a user-specified math string */
void replaceDummyVar(long dummyVar, nmbrString *mString);

/* Get subproof length of a proof, starting at endStep and going backwards */
long subProofLen(nmbrString *proof, long endStep);

/* Adds a dummy variable to end of mathToken array */
/* (Note:  it now grows forever, but purging it might worsen fragmentation) */
void declareDummyVars(long numNewVars);

