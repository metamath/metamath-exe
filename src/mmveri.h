/*****************************************************************************/
/*        Copyright (C) 2005  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMVERI_H_
#define METAMATH_MMVERI_H_

/*! \file */

#include "mmdata.h"

char verifyProof(long statemNum);

/*! assignVar() finds an assignment to substScheme variables that match
   the assumptions specified in the reason string */
nmbrString *assignVar(nmbrString *bigSubstSchemeAss,
nmbrString *bigSubstInstAss, long substScheme,
// For error messages:
long statementNum, long step, flag unkHypFlag);

/*! Deallocate the math symbol strings assigned in g_WrkProof structure during
   proof verification.  This should be called after verifyProof() and after the
   math symbol strings have been used for proof printouts, etc.
  \note this does NOT free the other allocations in g_WrkProof.  The
   ERASE command will do this. */
void cleanWrkProof(void);

/*! \brief Structure for getting info about a step for SHOW PROOF/STEP command

  If getStep.stepNum is nonzero, we should get info about that step.
  \note This structure should be deallocated after use. */
struct getStep_struct {
  long stepNum; // Step # to get info about
  long sourceStmt; // Right side of = in proof display
  long targetStmt; // Left side of = in proof display
  long targetParentStep; // Step # of target's parent
  long targetParentStmt; // Statement # of target's parent
  nmbrString *sourceHyps; // List of step #'s
  nmbrString *sourceSubstsNmbr; // List of vars w/ ptr to subst math tokens
  pntrString *sourceSubstsPntr; // List of vars w/ ptr to subst math tokens
  nmbrString *targetSubstsNmbr; // List of vars w/ ptr to subst math tokens
  pntrString *targetSubstsPntr; // List of vars w/ ptr to subst math tokens
};
extern struct getStep_struct getStep;

#endif // METAMATH_MMVERI_H_
