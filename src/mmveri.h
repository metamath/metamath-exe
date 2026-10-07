/*****************************************************************************/
/*        Copyright (C) 2005  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMVERI_H_
#define METAMATH_MMVERI_H_

/*! \file */

#include "mmdata.h"

/* Flags for verifyProof */
/*! Check that non-dummy distinct variable conditions ($d) are used. */
#define UNUSED_DVS        1
/*! Check that all distinct variable conditions ($d) are used. */
#define UNUSED_DUMMY_DVS  2
/*! Check that all essential hypotheses ($e) are used. */
#define UNUSED_ESSENTIAL  4

/*! Verify proof of one statement in a source file.
 * \param[in] statemNum number of the statement to verify
 * \param[in] unusedFlag bitwise OR of \c UNUSED_* flags.  If \c unusedFlag
 *   includes:
 *   - \c UNUSED_DVS: check for unused \c $d conditions on nondummy variables
 *   - \c UNUSED_DUMMY_DVS: check for all unused \c $d conditions
 *   - \c UNUSED_ESSENTIAL: check for unused \c $e statements
 * \return
 *   - 0 if proof is OK
 *   - 1 if proof is incomplete (has '?' tokens)
 *   - 2 if error found
 *   - 3 if severe error found
 *   - 4 if not a $p statement
 * \pre parseProof() must have already been called for the statement
 */
char verifyProof(long statemNum, flag unusedFlag);

/*! Find an assignment to substScheme variables that match the assumptions
    specified in the reason string
 * \param[in] bigSubstSchemAss the hypotheses the scheme
 * \param[in] bigSubstInstAss the instance hypotheses
 * \param[in] substScheme number of the statement to use for substitution
 * \param[in] statementNum number of the statement being verified
 * \param[in] step number of the proof step being verified
 * \param[in] unkHypFlag nonzero if there are unknown hypotheses, zero otherwise
 * \param[in] unusedFlag bitwise OR of \c UNUSED_* flags; e.g.,
 *            <tt>UNUSED_DVS|UNUSED_DUMMY_DVS</tt>
 * \param[out] djRVar array with one element per non-dummy distinct variable
 *            condition, used to mark those that are used
 * \param[out] djOVar array with one element per dummy distinct variable
 *            condition, used to mark those that are used
 * \pre if \c UNUSED_DVS is set in \c unusedFlag, then the length of \c djRVar
 *      is at least as great as the length of
 *      \c g_Statement[statemNum].reqDisjVarsA and contains all zeroes
 * \pre if \c UNUSED_DUMMY_DVS is set in \c unusedFlag, then the length of
 *      \c djOVar is at least as great as the length of
 *      \c g_Statement[statemNum].optDisjVarsA and contains all zeroes
 * \post if \c UNUSED_DVS is set in \c unusedFlag, then for every integer \c i
 *      such that the variables \c g_Statement[statementNum].reqDisjVarsA[i]
 *      and \c g_Statement[statementNum].reqDisjVarsB[i] were checked for
 *      distinctness, \c djRVar[i] is nonzero
 * \post if \c UNUSED_DUMMY_DVS is set in \c unusedFlag, then for every integer
 *      \c i such that the variables
 *      \c g_Statement[statementNum].optDisjVarsA[i] and
 *      \c g_Statement[statementNum].optDisjVarsB[i] were checked for
 *      distinctness, \c djOVar[i] is nonzero
 */
nmbrString *assignVar(nmbrString *bigSubstSchemeAss,
nmbrString *bigSubstInstAss, long substScheme,
// For error messages:
long statementNum, long step, flag unkHypFlag,
// For checking for unused $d conditions:
flag unusedFlag, nmbrString *djRVar, nmbrString *djOVar);

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
