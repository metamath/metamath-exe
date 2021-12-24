/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMUNIF_H_
#define METAMATH_MMUNIF_H_

#include "mmdata.h"

extern long g_minSubstLen; /* User-settable value - 0 or 1 */
extern long g_userMaxUnifTrials;
            /* User-defined upper limit (# backtracks) for unification trials */
extern long g_unifTrialCount;
                     /* 0 means don't time out; 1 means start counting trials */
extern long g_unifTimeouts; /* Number of timeouts so far for this command */
extern flag g_hentyFilter; /* Turns Henty filter on or off */

/* 26-Sep-2010 nm */
extern flag g_bracketMatchInit; /* So eraseSource() (mmcmds.c) can clr it */

/* 1-Oct-2017 nm Made this global so eraseSource() (mmcmds.c) can clr it */
extern nmbrString *g_firstConst;
/* 2-Oct-2017 nm Made these global so eraseSource() (mmcmds.c) can clr them */
extern nmbrString *g_lastConst;
extern nmbrString *g_oneConst;


nmbrString *makeSubstUnif(flag *newVarFlag,
    nmbrString *trialScheme, pntrString *stateVector);


char unify(
    nmbrString *schemeA,
    nmbrString *schemeB,
    /* nmbrString **unifiedScheme, */ /* stateVector[8] holds this */
    pntrString **stateVector,
    long reEntryFlag);
/* This function unifies two math token strings, schemeA and
   schemeB.  The result is contained in unifiedScheme.
   0 is returned if no assignment is possible.
   If reEntryFlag is 1, the next possible set of assignments, if any,
   is returned.  2 is returned if the unification times out.
   (*stateVector) contains the state of the previous
   call.  It is the caller's responsibility to deallocate the
   contents of (*stateVector) when done, UNLESS a 0 is returned.
   The caller must assign (*stateVector) to a legal pntrString
   (e.g. NULL_PNTRSTRING) before calling.

   All variables with a tokenNum > saveMathTokens are assumed
   to be "unknown" variables that can be assigned; all other
   variables are treated like constants in the unification
   algorithm.

   The "unknown" variable assignments are contained in (*stateVector)
   (which is a complex structure, described below).  Some "unknown"
   variables may have no assignment, in which case they will
   remain "unknown", and others may have assignments which include
   "unknown" variables.
*/


/* oneDirUnif() is like unify(), except that when reEntryFlag is 1,
   a new unification is returned ONLY if the assignments to the
   variables in schemeA have changed.  This is used to speed up the
   program. */
flag oneDirUnif(
    nmbrString *schemeA,
    nmbrString *schemeB,
    pntrString **stateVector,
    long reEntryFlag);


/* uniqueUnif() is like unify(), but there is no reEntryFlag, and 3 possible
   values are returned:
     0: no unification was possible.
     1: exactly one unification was possible, and stateVector is valid.
     2: unification timed out.
     3: more than one unification was possible. */
char uniqueUnif(
    nmbrString *schemeA,
    nmbrString *schemeB,
    pntrString **stateVector);

/* unifyH() is like unify(), except that when reEntryFlag is 1,
   a new unification is returned ONLY if the normalized unification
   does not previously exist in the "Henty filter".  This reduces
   ambiguous unifications.  The values returned are the same as
   those returned by unify().  (The elimination of equivalent
   unifications was suggested by Jeremy Henty.) */
char unifyH(
    nmbrString *schemeA,
    nmbrString *schemeB,
    pntrString **stateVector,
    long reEntryFlag);

/* Cleans out a stateVector if not empty */
void purgeStateVector(pntrString **stateVector);

/* Prints the substitutions determined by unify for debugging purposes */
void printSubst(pntrString *stateVector);

#endif /* METAMATH_MMUNIF_H_ */
