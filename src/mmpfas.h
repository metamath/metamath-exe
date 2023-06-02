/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMPFAS_H_
#define METAMATH_MMPFAS_H_

/*! \file */

#include "mmvstr.h"
#include "mmdata.h"

extern long g_proveStatement; /*!< The statement to be proved */
extern flag g_proofChangedFlag; /*!< Flag to push 'undo' stack */

extern long g_userMaxProveFloat; /*!< Upper limit for proveFloating */

extern long g_dummyVars; /*!< The number of dummy variables currently declared */
extern long g_pipDummyVars; /*!< Number of dummy vars used by proof in progress */

/*!< Structure for holding a proof in progress.
  \note This structure should be deallocated after use. */
struct pip_struct {
  nmbrString *proof; /*!< The proof itself */
  pntrString *target; /*!< Left hand side of = in display */
  pntrString *source; /*!< Right hand side of = in display */
  pntrString *user; /*!< User-specified math string assignments to step */
};
extern struct pip_struct g_ProofInProgress;

/*! Interactively select statement assignments that match
  \param maxEssential the maximum number of essential hypotheses that a
    statement may have in order to be included in the matched list. */
void interactiveMatch(long step, long maxEssential);

/*! Assign a statement to an unknown proof step */
void assignStatement(long statemNum, long step);

/*! Find proof of formula by using the replaceStatement() algorithm i.e.
   see if any statement matches current step AND each of its hypotheses
   matches a proof in progress hypothesis or some step already in the proof.
   If a proof is found, it is returned, otherwise an empty (length 0) proof is
   returned.
  \note The caller must deallocate the returned nmbrString. */
nmbrString *proveByReplacement(long prfStmt,
    long prfStep, /*!< 0 means step 1 */
    flag noDistinct, /*!< 1 means don't try statements with $d's */
    flag dummyVarFlag, /*!< 0 means no dummy vars are in prfStmt */
    flag searchMethod, /*!< 1 means to try proveFloating on $e's also */
    long improveDepth,
    flag overrideFlag, /*!< 1 means to override usage locks */
    flag mathboxFlag /*!< 1 means allow mathboxes */
    );

nmbrString *replaceStatement(long replStatemNum,
    long prfStep,
    long provStmtNum,
    flag subProofFlag, /*!< If 1, then scan only subproof at prfStep to look for
   matches, instead of whole proof, for faster speed (used by MINIMIZE_WITH) */
    flag noDistinct, /*!< 1 means don't try statements with $d's */
    flag searchMethod, /*!< 1 means to try proveFloating on $e's also */
    long improveDepth,
    flag overrideFlag, /*!< 1 means to override usage locks */
    flag mathboxFlag /*!< 1 means allow mathboxes */
    );

/*! This function identifies all steps in the proof in progress that (1) are
   independent of step refStep, (2) have no dummy variables, (3) are
   not $f's or $e's, and (4) have subproofs that are complete
   (no unassigned steps).  A "Y" is returned for each such step,
   and "N" is returned for all other steps.  The "Y" steps can be used
   for scanning for useful subproofs outside of the subProof of refStep.
   \note The caller must deallocate the returned vstring. */
vstring getIndepKnownSteps(long proofStmt, long refStep);

/*! This function classifies each proof step in g_ProofInProgress.proof
   as known or unknown ('K' or 'U' in the returned string) depending
   on whether the step has a completely known subproof.
   \note The caller must deallocate the returned vstring. */
vstring getKnownSubProofs(void);

/*! Add a subproof in place of an unknown step to g_ProofInProgress.  The
   .target, .source, and .user fields are initialized to empty (except
   .target of the deleted unknown step is retained). */
void addSubProof(const nmbrString *subProof, long step);

/*! This function eliminates any occurrences of statement sourceStmtNum in the
   targetProof by substituting it with the proof of sourceStmtNum.  An empty
   nmbrString is returned if there was an error.

   Normally, targetProof is the global g_ProofInProgress.proof.  However,
   we make it an argument in case in the future we'd like to do this
   outside of the proof assistant.
  \note The caller must deallocate the returned nmbrString. */
nmbrString *expandProof(const nmbrString *targetProof, long sourceStmtNum);

/*! Delete a subproof starting (in reverse from) step.  The step is replaced
   with an unknown step, and its .target field is retained. */
void deleteSubProof(long step);

/*! Check to see if a statement will match the g_ProofInProgress.target (or .user)
   of an unknown step.  Returns 1 if match, 0 if not, 2 if unification
   timed out. */
char checkStmtMatch(long statemNum, long step);

/*! Check to see if a (user-specified) math string will match the
   g_ProofInProgress.target (or .user) of an step.  Returns 1 if match, 0 if
   not, 2 if unification timed out. */
char checkMStringMatch(const nmbrString *mString, long step);

/*! Find proof of formula or simple theorem (no new vars in $e's)

  \param maxEDepth the maximum depth at which statements with $e hypotheses are
    considered.  A value of 0 means none are considered. */
nmbrString *proveFloating(const nmbrString *mString, long statemNum, long maxEDepth,
    long step, flag noDistinct,
    flag overrideFlag, /*!< 0 means respect usage locks
                         1 means to override usage locks
                         2 means override silently */
    flag mathboxFlag /*!< 1 means allow mathboxes */
);

/*! This function does quick check for some common conditions that prevent
   a trial statement (scheme) from being unified with a given instance.
   Return value 0 means it can't be unified, 1 means it might be unifiable. */
char quickMatchFilter(long trialStmt, const nmbrString *mString,
    long dummyVarFlag /*!< 0 if no dummy vars in mString */);

/*! Shorten proof by using specified statement. */
void minimizeProof(long repStatemNum, long prvStatemNum, flag allowGrowthFlag);

/*! Initialize g_ProofInProgress.source of the step, and .target of all
   hypotheses, to schemes using new dummy variables. */
void initStep(long step);

/*! Look for completely known subproofs in g_ProofInProgress.proof and
   assign g_ProofInProgress.target and .source.  Calls assignKnownSteps(). */
void assignKnownSubProofs(void);

/*! This function assigns math strings to all steps (g_ProofInProgress.target and
   .source fields) in a subproof with all known steps. */
void assignKnownSteps(long startStep, long sbProofLen);

/*! \brief Interactive unify a step.  Calls interactiveUnify().
   If messageFlag is 1, a message will be issued if the
   step is already unified.   If messageFlag is 0, show the step #
   being unified.  If messageFlag is 2, don't print step #, and do nothing
   if step is already unified. */
void interactiveUnifyStep(long step, char messageFlag);

/*! Interactively select one of several possible unifications
  \returns * 0 = no unification possible
           * 1 = unification was selected; held in stateVector
           * 2 = unification timed out
           * 3 = no unification was selected */
char interactiveUnify(const nmbrString *schemeA, const nmbrString *schemeB,
    pntrString **stateVector);

/*! Automatically unify steps with unique unification */
void autoUnify(flag congrats);

/*! Make stateVector substitutions in all steps.  The stateVector must
   contain the result of a valid unification. */
void makeSubstAll(pntrString *stateVector);

/*! Replace a dummy variable with a user-specified math string */
void replaceDummyVar(long dummyVar, const nmbrString *mString);

/*! Get subproof length of a proof, starting at endStep and going backwards */
long subproofLen(const nmbrString *proof, long endStep);

/*! If testStep has no dummy variables, return 0;
   if testStep has isolated dummy variables (that don't affect rest of
   proof), return 1;
   if testStep has dummy variables used elsewhere in proof, return 2 */
char checkDummyVarIsolation(long testStep /*!< 0=1st step, 1=2nd, etc. */);

/*! Given a starting step, find its parent (the step it is a hypothesis of)
   If the starting step is the last proof step, just return it */
long getParentStep(long startStep /*! < 0=1st step, 1=2nd, etc. */);

/*! Adds a dummy variable to end of mathToken array
  \note it now grows forever, but purging it might worsen fragmentation */
void declareDummyVars(long numNewVars);

/*! Copies the Proof Assistant proof state */
void copyProofStruct(struct pip_struct *outProofStruct,
    struct pip_struct inProofStruct);

/*! Initializes the Proof Assistant proof state */
void initProofStruct(struct pip_struct *proofStruct, const nmbrString *proof,
    long proveStatement);

/*! Clears the Proof Assistant proof state */
void deallocProofStruct(struct pip_struct *proofStruct);

// Actions for processUndoStack()
#define PUS_INIT 1
#define PUS_PUSH 2
#define PUS_UNDO 3
#define PUS_REDO 4
#define PUS_NEW_SIZE 5
#define PUS_GET_SIZE 6
#define PUS_GET_STATUS 7

/*! Handle the PUSH, UNDO, and REDO commands */
long processUndoStack(struct pip_struct *proofStruct,
    char action,
    vstring info, /*!< Info to print upon UNDO or REDO */
    long newStackSize /*!< Used only by NEW_SIZE */);

#endif // METAMATH_MMPFAS_H_
