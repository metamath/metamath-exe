/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* mmpfas.c - Proof assistant module */

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <setjmp.h>
#include <time.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h"
#include "mmunif.h"
#include "mmpfas.h"
#include "mmwtex.h"

/* Allow user to define INLINE as "inline".  lcc doesn't support inline. */
#ifndef INLINE
#define INLINE
#endif

long g_proveStatement = 0; /* The statement to be proved - global */
flag g_proofChangedFlag; /* Flag to push 'undo' stack - global */

/* 4-Aug-2011 nm Changed from 25000 to 50000 */
/* 11-Dec-2010 nm Changed from 10000 to 25000 to accomodate df-plig in set.mm
   (which needs >= 23884 to generate with 'show statement / html'). */
/* g_userMaxProveFloat can be overridden by user with SET SEARCH_LIMIT */
long g_userMaxProveFloat = 50000; /* Upper limit for proveFloating */

long g_dummyVars = 0; /* Total number of dummy variables declared */
long g_pipDummyVars = 0; /* Number of dummy variables used by proof in progress */

/* Structure for holding a proof in progress. */
/* This structure should be deallocated after use. */
struct pip_struct g_ProofInProgress = {
    NULL_NMBRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING };

/* Interactively select statement assignments that match */
/* maxEssential is the maximum number of essential hypotheses that a
   statement may have in order to be included in the matched list.
   If -1, there is no limit. */
void interactiveMatch(long step, long maxEssential)
{
  long matchCount = 0;
  long timeoutCount = 0;
  long essHypCount, hyp;
  vstring matchFlags = "";
  vstring timeoutFlags = "";
  char unifFlag;
  vstring tmpStr1 = "";
  vstring tmpStr4 = "";
  vstring tmpStr2 = "";
  vstring tmpStr3 = "";
  nmbrString *matchList = NULL_NMBRSTRING;
  nmbrString *timeoutList = NULL_NMBRSTRING;
  long stmt, matchListPos, timeoutListPos;

  printLongLine(cat("Step ", str((double)step + 1), ":  ", nmbrCvtMToVString(
      (g_ProofInProgress.target)[step]), NULL), "  ", " ");
  if (nmbrLen((g_ProofInProgress.user)[step])) {
    printLongLine(cat("Step ", str((double)step + 1), "(user):  ", nmbrCvtMToVString(
        (g_ProofInProgress.user)[step]), NULL), "  ", " ");
  }
  /* Allocate a flag for each step to be tested */
  /* 1 means no match, 2 means match */
  let(&matchFlags, string(g_proveStatement, 1));
  /* 1 means no timeout, 2 means timeout */
  let(&timeoutFlags, string(g_proveStatement, 1));
  for (stmt = 1; stmt < g_proveStatement; stmt++) {
    if (g_Statement[stmt].type != (char)e_ &&
        g_Statement[stmt].type != (char)f_ &&
        g_Statement[stmt].type != (char)a_ &&
        g_Statement[stmt].type != (char)p_) continue;

    /* See if the maximum number of requested essential hypotheses is
       exceeded */
    if (maxEssential != -1) {
      essHypCount = 0;
      for (hyp = 0; hyp < g_Statement[stmt].numReqHyp; hyp++) {
        if (g_Statement[g_Statement[stmt].reqHypList[hyp]].type == (char)e_) {
          essHypCount++;
          if (essHypCount > maxEssential) break;
        }
      }
      if (essHypCount > maxEssential) continue;
    }

    unifFlag = checkStmtMatch(stmt, step);
    if (unifFlag) {
      if (unifFlag == 1) {
        matchFlags[stmt] = 2;
        matchCount++;
      } else { /* unifFlag = 2 */
        timeoutFlags[stmt] = 2;
        timeoutCount++;
      }
    }
  }

  if (matchCount == 0 && timeoutCount == 0) {
    print2("No statements match step %ld.  The proof has an error.\n",
        (long)(step + 1));
    let(&matchFlags, "");
    let(&timeoutFlags, "");
    return;
  }

#define MATCH_LIMIT 100
  if (matchCount > MATCH_LIMIT) {
    let(&tmpStr1, cat("There are ", str((double)matchCount), " matches for step ",
      str((double)step + 1), ".  View them (Y, N) <N>? ", NULL));
    tmpStr2 = cmdInput1(tmpStr1);
    let(&tmpStr1, "");

    if (tmpStr2[0] != 'Y' && tmpStr2[0] != 'y') {
      let(&tmpStr2, "");
      let(&matchFlags, "");
      let(&timeoutFlags, "");
      return;
    }

  }

  nmbrLet(&matchList, nmbrSpace(matchCount));
  matchListPos = 0;
  for (stmt = 1; stmt < g_proveStatement; stmt++) {
    if (matchFlags[stmt] == 2) {
      matchList[matchListPos] = stmt;
      matchListPos++;
    }
  }

  nmbrLet(&timeoutList, nmbrSpace(timeoutCount));
  timeoutListPos = 0;
  for (stmt = 1; stmt < g_proveStatement; stmt++) {
    if (timeoutFlags[stmt] == 2) {
      timeoutList[timeoutListPos] = stmt;
      timeoutListPos++;
    }
  }

  let(&tmpStr1, nmbrCvtRToVString(matchList,
                0, /*explicitTargets*/
                0 /*statemNum, used only if explicitTargets*/));
  let(&tmpStr4, nmbrCvtRToVString(timeoutList,
                0, /*explicitTargets*/
                0 /*statemNum, used only if explicitTargets*/));

  printLongLine(cat("Step ", str((double)step + 1), " matches statements:  ", tmpStr1,
      NULL), "  ", " ");
  if (timeoutCount) {
    printLongLine(cat("In addition, there were unification timeouts with the",
        " following steps, which may or may not match:  ", tmpStr4, NULL),
        "  ", " ");
  }

  if (matchCount == 1 && timeoutCount == 0 && maxEssential == -1) {
    /* Assign it automatically */
    matchListPos = 0;
    stmt = matchList[matchListPos];
    print2("Step %ld was assigned statement %s since it is the only match.\n",
        (long)(step + 1),
        g_Statement[stmt].labelName);
  } else {

    while (1) {
      let(&tmpStr3, cat("What statement to select for step ", str((double)step + 1),
          " (<return> to bypass)? ", NULL));
      tmpStr2 = cmdInput1(tmpStr3);
      let(&tmpStr3, "");

      if (tmpStr2[0] == 0) {
        let(&tmpStr1, "");
        let(&tmpStr4, "");
        let(&tmpStr2, "");
        let(&matchFlags, "");
        let(&timeoutFlags, "");
        nmbrLet(&matchList, NULL_NMBRSTRING);
        nmbrLet(&timeoutList, NULL_NMBRSTRING);
        return;
      }
      if (!instr(1, cat(" ", tmpStr1, " ", tmpStr4, " ", NULL),
           cat(" ", tmpStr2, " ", NULL))) {
        print2("\"%s\" is not one of the choices.  Try again.\n", tmpStr2);
      } else {
        break;
      }
    }

    for (matchListPos = 0; matchListPos < matchCount; matchListPos++) {
      if (!strcmp(tmpStr2, g_Statement[matchList[matchListPos]].labelName)) break;
    }
    if (matchListPos < matchCount) {
      stmt = matchList[matchListPos];
    } else {
      for (timeoutListPos = 0; timeoutListPos < timeoutCount;
          timeoutListPos++) {
      if (!strcmp(tmpStr2, g_Statement[timeoutList[timeoutListPos]].labelName))
          break;
      } /* Next timeoutListPos */
      if (timeoutListPos == timeoutCount) bug(1801);
      stmt = timeoutList[timeoutListPos];
    }
    print2("Step %ld was assigned statement %s.\n",
        (long)(step + 1),
        g_Statement[stmt].labelName);

  } /* End if matchCount == 1 */

  /* Add to statement to the proof */
  assignStatement(matchList[matchListPos], step);
  g_proofChangedFlag = 1; /* Flag for 'undo' stack */

  let(&tmpStr1, "");
  let(&tmpStr4, "");
  let(&tmpStr2, "");
  let(&matchFlags, "");
  let(&timeoutFlags, "");
  nmbrLet(&matchList, NULL_NMBRSTRING);
  nmbrLet(&timeoutList, NULL_NMBRSTRING);
  return;

} /* interactiveMatch */



/* Assign a statement to an unknown proof step */
void assignStatement(long statemNum, long step)
{
  long hyp;
  nmbrString *hypList = NULL_NMBRSTRING;

  if ((g_ProofInProgress.proof)[step] != -(long)'?') bug(1802);

  /* Add the statement to the proof */
  nmbrLet(&hypList, nmbrSpace(g_Statement[statemNum].numReqHyp + 1));
  for (hyp = 0; hyp < g_Statement[statemNum].numReqHyp; hyp++) {
    /* A hypothesis of the added statement */
    hypList[hyp] = -(long)'?';
  }
  hypList[g_Statement[statemNum].numReqHyp] = statemNum; /* The added statement */
  addSubProof(hypList, step);
  initStep(step + g_Statement[statemNum].numReqHyp);
  nmbrLet(&hypList, NULL_NMBRSTRING);
  return;
} /* assignStatement */



/* Find proof of formula by using the replaceStatement() algorithm i.e.
   see if any statement matches current step AND each of its hypotheses
   matches a proof in progress hypothesis or some step already in the proof.
   If a proof is found, it is returned, otherwise an empty (length 0) proof is
   returned. */
/* The caller must deallocate the returned nmbrString. */
nmbrString *proveByReplacement(long prfStmt,
    long prfStep, /* 0 means step 1 */
    flag noDistinct, /* 1 means don't try statements with $d's */
    flag dummyVarFlag, /* 0 means no dummy vars are in prfStmt */
    flag searchMethod, /* 1 means to try proveFloating on $e's also */
    long improveDepth, /* depth for proveFloating() */
    flag overrideFlag, /* 1 means to override usage locks */
    flag mathboxFlag
    )
{

  long trialStmt;
  nmbrString *prfMath;
  nmbrString *trialPrf = NULL_NMBRSTRING;
  long prfMbox;

  prfMath = (g_ProofInProgress.target)[prfStep];
  prfMbox = getMathboxNum(prfStmt);
  for (trialStmt = 1; trialStmt < prfStmt; trialStmt++) {

    if (quickMatchFilter(trialStmt, prfMath, dummyVarFlag) == 0) continue;

    /* Skip statements with discouraged usage (the above skips non-$a,p) */
    if (overrideFlag == 0 && getMarkupFlag(trialStmt, USAGE_DISCOURAGED)) {
      continue;
    }

    /* Skip statements in other mathboxes unless /INCLUDE_MATHBOXES.  (We don't
       care about the first mathbox since there are no others above it.) */
    if (mathboxFlag == 0 && prfMbox >= 2) {
      /* Note that g_mathboxStart[] starts a 0 */
      if (trialStmt > g_mathboxStmt && trialStmt < g_mathboxStart[prfMbox - 1]) {
        continue;
      }
    }

    /* noDistinct is set by NO_DISTICT qualifier in IMPROVE */
    if (noDistinct) {
      /* Skip the statement if it has a $d requirement.  This option
         prevents illegal proofs that would violate $d requirements
         since the Proof Assistant does not check for $d violations. */
      if (nmbrLen(g_Statement[trialStmt].reqDisjVarsA)) {
        continue;
      }
    }

    trialPrf = replaceStatement(trialStmt, prfStep,
        prfStmt, 0,/*scan whole proof to maximize chance of a match*/
        noDistinct,
        searchMethod,
        improveDepth,
        overrideFlag,
        mathboxFlag /* 1 means allow mathboxes */
        );
    if (nmbrLen(trialPrf) > 0) {
      /* A proof for the step was found. */

      /* Inform user that we're using a statement with discouraged usage */
      if (overrideFlag == 1 && getMarkupFlag(trialStmt, USAGE_DISCOURAGED)) {
        /* print2("\n"); */ /* Enable for more emphasis */
        print2(
          ">>> ?Warning: Overriding discouraged usage of statement \"%s\".\n",
            g_Statement[trialStmt].labelName);
        /* print2("\n"); */ /* Enable for more emphasis */
      }

      return trialPrf;
    }
    /* Don't need to do this because it is already null */
    /* nmbrLet(&trialPrf, NULL_NMBRSTRING); */
  }
  return trialPrf;  /* Proof not found - return empty proof */
}


nmbrString *replaceStatement(long replStatemNum, long prfStep,
    long provStmtNum,
    flag subProofFlag, /* If 1, then scan only subproof at prfStep to look for
   matches, instead of whole proof, for faster speed (used by MINIMIZE_WITH) */
    flag noDistinct, /* 1 means proveFloating won't try statements with $d's */
    flag searchMethod, /* 1 means to try proveFloating on $e's also */
    long improveDepth, /* Depth for proveFloating */
    flag overrideFlag,  /* 1 means to override statement usage locks */
    flag mathboxFlag /* 1 means allow mathboxes */ /* 5-Aug-2020 nm */
    ) {
  nmbrString *prfMath; /* Pointer only */
  long reqHyps;
  long hyp, sym, var, i, j, k, trialStep;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *scheme = NULL_NMBRSTRING;
  pntrString *hypList = NULL_PNTRSTRING;
  nmbrString *hypSortMap = NULL_NMBRSTRING; /* Order remapping for speedup */
  pntrString *hypProofList = NULL_PNTRSTRING;
  pntrString *stateVector = NULL_PNTRSTRING;
  nmbrString *replStmtSchemePtr;
  nmbrString *hypSchemePtr;
  nmbrString *hypProofPtr;
  nmbrString *makeSubstPtr;
  pntrString *hypMakeSubstList = NULL_PNTRSTRING;
  pntrString *hypStateVectorList = NULL_PNTRSTRING;
  vstring hypReEntryFlagList = "";
  nmbrString *hypStepList = NULL_NMBRSTRING;
  flag reEntryFlag;
  flag tmpFlag;
  flag noHypMatch;
  flag foundTrialStepMatch;
  long replStmtSchemeLen, schemeVars, schReqHyps, hypLen, reqVars,
      schEHyps, subPfLen;
  long saveUnifTrialCount;
  flag reenterFFlag;
  flag dummyVarFlag; /* Flag that replacement hypothesis under consideration has
                        dummy variables */
  nmbrString *hypTestPtr; /* Points to what we are testing hyp. against */
  flag hypOrSubproofFlag; /* 0 means testing against hyp., 1 against proof*/
  vstring indepKnownSteps = ""; /* 'Y' = ok to try step; 'N' = not ok */
  long pfLen;
  long scanLen;
  long scanUpperBound;
  long scanLowerBound;
  vstring hasFloatingProof = "";  /* 'N' or 'Y' for $e hyps */
  vstring tryFloatingProofLater = "";  /* 'N' or 'Y' */
  flag hasDummyVar;     /* 4-Sep-2012 nm */

  /* If we are overriding discouraged usage, a warning has already been
     printed.  If we are not, then we should never get here. */
  if (overrideFlag == 0 && getMarkupFlag(replStatemNum, USAGE_DISCOURAGED)) {
    bug(1868);
  }

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  trialStep = 0;

  prfMath = (g_ProofInProgress.target)[prfStep];
  if (subProofFlag) {
    /* Get length of the existing subproof at the replacement step.  The
       existing subproof will be scanned to see if there is a match to
       the $e hypotheses of the replacement statement.  */
    subPfLen = subproofLen(g_ProofInProgress.proof, prfStep);
    scanLen = subPfLen;
    scanUpperBound = prfStep;
    scanLowerBound = scanUpperBound - scanLen + 1;
  } else {
    /* Treat the whole proof as a "subproof" and get its length.  The whole
       existing proof will be scanned to see if there is a match to
       the $e hypotheses of the replacement statement.  */
    pfLen = nmbrLen(g_ProofInProgress.proof);
    scanUpperBound = pfLen - 1;  /* Last proof step (0=1st step, 1=2nd, etc. */
    scanLowerBound = 0; /* scanUpperBound - scanLen + 1;  */
  }
  /* Note: the variables subPfLen, pfLen, and scanLen aren't
     used again.  They could be eliminated above if we wanted. */

  if (g_Statement[replStatemNum].type != (char)a_ &&
      g_Statement[replStatemNum].type != (char)p_)
    bug(1822); /* Not $a or $p */

  schReqHyps = g_Statement[replStatemNum].numReqHyp;
  reqVars = nmbrLen(g_Statement[replStatemNum].reqVarList);

  /* hasFloatingProof is used only when searchMethod=1 */
  let(&hasFloatingProof, string(schReqHyps, ' '));
  let(&tryFloatingProofLater, string(schReqHyps, ' '));
  replStmtSchemePtr = g_Statement[replStatemNum].mathString;
  replStmtSchemeLen = nmbrLen(replStmtSchemePtr);

  /* Change all variables in the statement to dummy vars for unification */
  nmbrLet(&scheme, replStmtSchemePtr);
  schemeVars = reqVars;
  if (schemeVars + g_pipDummyVars/*global*/ > g_dummyVars/*global*/) {
    /* Declare more dummy vars if necessary */
    declareDummyVars(schemeVars + g_pipDummyVars - g_dummyVars);
  }
  for (var = 0; var < schemeVars; var++) {
    /* Put dummy var mapping into g_MathToken[].tmp field */
    g_MathToken[g_Statement[replStatemNum].reqVarList[var]].tmp
        = g_mathTokens/*global*/ + 1 + g_pipDummyVars/*global*/ + var;
  }
  for (sym = 0; sym < replStmtSchemeLen; sym++) {
    if (g_MathToken[replStmtSchemePtr[sym]].tokenType != (char)var_) continue;
    /* Use dummy var mapping from g_MathToken[].tmp field */
    scheme[sym] = g_MathToken[replStmtSchemePtr[sym]].tmp;
  }

  /* Change all variables in the statement's hyps to dummy vars for subst. */
  pntrLet(&hypList, pntrNSpace(schReqHyps));
  nmbrLet(&hypSortMap, nmbrSpace(schReqHyps));
  pntrLet(&hypProofList, pntrNSpace(schReqHyps));

  for (hyp = 0; hyp < schReqHyps; hyp++) {
    hypSchemePtr = NULL_NMBRSTRING;
    nmbrLet(&hypSchemePtr,
      g_Statement[g_Statement[replStatemNum].reqHypList[hyp]].mathString);
    hypLen = nmbrLen(hypSchemePtr);
    for (sym = 0; sym < hypLen; sym++) {
      if (g_MathToken[hypSchemePtr[sym]].tokenType
          != (char)var_) continue;
      /* Use dummy var mapping from g_MathToken[].tmp field */
      hypSchemePtr[sym] = g_MathToken[hypSchemePtr[sym]].tmp;
    }
    hypList[hyp] = hypSchemePtr;
    hypSortMap[hyp] = hyp;
  }

  /* Move all $e's to front of hypothesis list */
  schEHyps = 0;
  for (hyp = 0; hyp < schReqHyps; hyp++) {
    if (g_Statement[g_Statement[replStatemNum].reqHypList[hypSortMap[hyp]]].type
         == (char)e_) {
      j = hypSortMap[hyp];
      hypSortMap[hyp] = hypSortMap[schEHyps];
      hypSortMap[schEHyps] = j;
      schEHyps++;
    }
  }

  /* Speedup - sort essential hyp's according to decreasing length to
     maximize the chance of early rejection */
  for (hyp = 0; hyp < schEHyps; hyp++) {
    /* Bubble sort - but should be OK for typically small # of hyp's */
    for (i = hyp + 1; i < schEHyps; i++) {
      if (nmbrLen(hypList[hypSortMap[i]]) > nmbrLen(hypList[hypSortMap[hyp]])) {
        j = hypSortMap[hyp];
        hypSortMap[hyp] = hypSortMap[i];
        hypSortMap[i] = j;
      }
    }
  }

  /* If we are just scanning the subproof, all subproof steps are independent,
     so the getIndepKnownSteps scan would be redundant. */
  if (!subProofFlag) {
    /* If subProofFlag is not set, we scan the whole proof,
       not just the subproof starting at prfStep, to find additional possible
       matches. */
    /* Get a list of the possible steps to look at that are not dependent
       on the prfStep.  A value of 'Y'  means we can try the step. */
    let(&indepKnownSteps, "");
    indepKnownSteps = getIndepKnownSteps(provStmtNum, prfStep);
  }

  /* Initialize state vector list for hypothesis unifications */
  /* (We will really only use up to schEHyp entries, but allocate all
     for possible future use) */
  pntrLet(&hypStateVectorList, pntrPSpace(schReqHyps));
  /* Initialize unification reentry flags for hypothesis unifications */
  /* (1 means 0, and 2 means 1, because 0 means end-of-character-string.) */
  /* (3 means previous proveFloating call found proof) */
  let(&hypReEntryFlagList, string(schReqHyps, 1));
  /* Initialize starting subproof step to scan for each hypothesis */
  nmbrLet(&hypStepList, nmbrSpace(schReqHyps));
  /* Initialize list of hypotheses after substitutions made */
  pntrLet(&hypMakeSubstList, pntrNSpace(schReqHyps));


  g_unifTrialCount = 1; /* Reset unification timeout */
  reEntryFlag = 0; /* For unifyH() */

  /* Number of required hypotheses of statement we're proving */
  reqHyps = g_Statement[provStmtNum].numReqHyp;

  while (1) { /* Try all possible unifications */
    tmpFlag = unifyH(scheme, prfMath, &stateVector, reEntryFlag);
    if (!tmpFlag) break; /* (Next) unification not possible */
    if (tmpFlag == 2) {
      print2("Unification timed out.  "
        "Larger SET UNIFICATION_TIMEOUT may improve results.\n");
      g_unifTrialCount = 1; /* Reset unification timeout */
      break; /* Treat timeout as if unification not possible */
    }

    reEntryFlag = 1; /* For next unifyH() */

    /* Make substitutions into each hypothesis, and try to prove that
       hypothesis */
    nmbrLet(&proof, NULL_NMBRSTRING);
    noHypMatch = 0;
    for (hyp = 0; hyp < schReqHyps; hyp++) {

      /* Make substitutions from replacement statement's stateVector */
      nmbrLet((nmbrString **)(&(hypMakeSubstList[hypSortMap[hyp]])),
          NULL_NMBRSTRING); /* Deallocate previous pass if any */
      hypMakeSubstList[hypSortMap[hyp]] =
          makeSubstUnif(&dummyVarFlag, hypList[hypSortMap[hyp]],
          stateVector);

      /* Initially, a $e has no proveFloating proof */
      hasFloatingProof[hyp] = 'N';  /* Init for this pass */
      tryFloatingProofLater[hyp] = 'N'; /* Init for this pass */

      /* Make substitutions from each earlier hypothesis unification */
      for (i = 0; i < hyp; i++) {
        /* Only do substitutions for $e's -- the $f's will have no
           dummy vars., and they have no stateVector
           since they were found with proveFloating below */
        if (i >= schEHyps) break;

        /* If it is an essential hypothesis with a proveFloating proof,
           we don't want to make substitutions since it has no
           stateVector.  (This is the only place we look
           at hasFloatingProof.) */
        if (hasFloatingProof[i] == 'Y') continue;

        makeSubstPtr = makeSubstUnif(&dummyVarFlag,
            hypMakeSubstList[hypSortMap[hyp]],
            hypStateVectorList[hypSortMap[i]]);
        nmbrLet((nmbrString **)(&(hypMakeSubstList[hypSortMap[hyp]])),
            NULL_NMBRSTRING);
        hypMakeSubstList[hypSortMap[hyp]] = makeSubstPtr;
      }

      if (hyp < schEHyps) {
        /* It's a $e hypothesis */
        if (g_Statement[g_Statement[replStatemNum].reqHypList[hypSortMap[hyp]]
            ].type != (char)e_) bug(1823);
      } else {
        /* It's a $f hypothesis */
        if (g_Statement[g_Statement[replStatemNum].reqHypList[hypSortMap[hyp]]
             ].type != (char)f_) bug(1824);
      }


      /* Scan all known steps of existing subproof to find a hypothesis
         match */
      foundTrialStepMatch = 0;
      reenterFFlag = 0;
      if (hypReEntryFlagList[hypSortMap[hyp]] == 2) {
        /* Reentry flag is set; we're continuing with a previously unified
           subproof step */
        trialStep = hypStepList[hypSortMap[hyp]];

        /* If we are re-entering the unification for a $f, it means we
           backtracked from a later failure, and there won't be another
           unification possible.  In this case we should bypass the
           proveFloating call to force a further backtrack.  (Otherwise
           we will have an infinite loop.)  Note that for $f's, all
           variables will be known so there will only be one unification
           anyway. */
        if (hyp >= schEHyps
            || hasFloatingProof[hyp] == 'Y' /* 5-Sep-2012 nm */
            ) {
          reenterFFlag = 1;
        }
      } else {
        if (hypReEntryFlagList[hypSortMap[hyp]] == 1) {
          /* Start at the beginning of the proof */
          /* trialStep = prfStep - subPfLen + 1; */ /* obsolete */
          trialStep = scanLowerBound;
          /* Later enhancement:  start at required hypotheses */
          /* (Here we use the trick of shifting down the starting
              trialStep to below the real subproof start) */
          trialStep = trialStep - reqHyps;
        } else {
          if (hypReEntryFlagList[hypSortMap[hyp]] == 3) {
            /* This is the case where proveFloating previously found a
               proof for the step, and we've backtracked.  In this case,
               we want to backtrack further - no scan or proveFloating
               call again. */
            hypReEntryFlagList[hypSortMap[hyp]] = 1;
            reenterFFlag = 1; /* Skip proveFloating call */
            trialStep = scanUpperBound; /* Skip loop */
          } else {
            bug(1826);
          }
        }
      }

      for (trialStep = trialStep + 0; trialStep < scanUpperBound;
          trialStep++) {
        /* Note that step scanUpperBound is not scanned since that is
           the statement we want to replace (subProofFlag = 1) or the
           last statement of the proof (subProofFlag = 0), neither of
           which would be useful for a replacement step subproof. */

        if (trialStep < scanLowerBound) {
          /* We're scanning required hypotheses */
          hypOrSubproofFlag = 0;
          /* Point to what we are testing hyp. against */
          /* (Note offset to trialStep needed to compensate for trick) */
          hypTestPtr =
              g_Statement[g_Statement[provStmtNum].reqHypList[
              trialStep - (scanLowerBound - reqHyps)]].mathString;
        } else {
          /* We're scanning the subproof */
          hypOrSubproofFlag = 1;
          /* Point to what we are testing hyp. against */
          hypTestPtr = (g_ProofInProgress.target)[trialStep];

          /* Subproof step has dummy var.; don't use it */
          if (!subProofFlag) {
            if (indepKnownSteps[trialStep] != 'Y') {
              if (indepKnownSteps[trialStep] != 'N') bug(1836);
              continue; /* Don't use the step */
            }
          }
        }

        /* Speedup - skip if no dummy vars in hyp and statements not equal */
        if (!dummyVarFlag) {
          if (!nmbrEq(hypTestPtr, hypMakeSubstList[hypSortMap[hyp]])) {
            continue;
          }
        }

        /* Speedup - skip if 1st symbols are constants and don't match */
        /* First symbol */
        i = hypTestPtr[0];
        j = ((nmbrString *)(hypMakeSubstList[hypSortMap[hyp]]))[0];
        if (g_MathToken[i].tokenType == (char)con_) {
          if (g_MathToken[j].tokenType == (char)con_) {
            if (i != j) {
              continue;
            }
          }
        }

        /* g_unifTrialCount = 1; */ /* ??Don't reset it here in order to
           detect exponential blowup in hypotheses trials */
        g_unifTrialCount = 1; /* Reset unification timeout counter */
        if (hypReEntryFlagList[hypSortMap[hyp]] < 1
            || hypReEntryFlagList[hypSortMap[hyp]] > 2)
          bug(1851);
        tmpFlag = unifyH(hypMakeSubstList[hypSortMap[hyp]],
            hypTestPtr,
            (pntrString **)(&(hypStateVectorList[hypSortMap[hyp]])),
            /* (Remember: 1 = false, 2 = true in hypReEntryFlagList) */
            hypReEntryFlagList[hypSortMap[hyp]] - 1);
        if (!tmpFlag || tmpFlag == 2) {
          /* (Next) unification not possible or timeout */
          if (tmpFlag == 2) {
            print2(
"Unification timed out.  SET UNIFICATION_TIMEOUT larger for better results.\n");
            g_unifTrialCount = 1; /* Reset unification timeout */
            /* Deallocate unification state vector */
            purgeStateVector(
                (pntrString **)(&(hypStateVectorList[hypSortMap[hyp]])));
          }

          /* If this is a reenter, and there are no dummy vars in replacement
             hypothesis, we have already backtracked from a unique exact
             match that didn't work.  There is no point in continuing to
             look for another exact match for this hypothesis, so we'll just
             skip the rest of the subproof scan. */
          /* (Note that we could in principle bypass the redundant unification
             above since we know it will fail, but it will clear out our
             stateVector for us.) */
          if (!dummyVarFlag) {
            if (hypReEntryFlagList[hypSortMap[hyp]] - 1 == 1) {
              /* There are no dummy variables, so previous match
                 was exact.  Force the trialStep loop to terminate as
                 if nothing further was found.  (If we don't do this,
                 there could be, say 50 more matches for "var x",
                 so we might need a factor of 50 greater runtime for each
                 replacement hypothesis having this situation.) */
              /* trialStep = prfStep - 1;  old version */
              trialStep = scanUpperBound - 1;
                       /* Make this the last loop pass */
            }
          }

          hypReEntryFlagList[hypSortMap[hyp]] = 1;
          continue;
        } else {

          /* tmpFlag = 1:  a unification was found */
          if (tmpFlag != 1) bug(1828);

          /* (Speedup) */
          /* If this subproof step has previously occurred in a hypothesis
             or an earlier subproof step, don't consider it since that
             would be redundant. */
          if (hypReEntryFlagList[hypSortMap[hyp]] == 1) {
            j = 0; /* Break flag */
            for (i = scanLowerBound - reqHyps; i < trialStep; i++) {
              if (i < scanLowerBound) {
                /* A required hypothesis */
                if (nmbrEq(hypTestPtr,
                    g_Statement[g_Statement[provStmtNum].reqHypList[
                    i - (scanLowerBound - reqHyps)]].mathString)) {
                  j = 1;
                  break;
                }
              } else {
                /* A subproof step */
                if (nmbrEq(hypTestPtr,
                    (g_ProofInProgress.target)[i])) {
                  j = 1;
                  break;
                }
              }
            } /* next i */
            if (j) {
              /* This subproof step was already considered earlier, so
                 we can skip considering it again. */
              /* Deallocate unification state vector */
              purgeStateVector(
                  (pntrString **)(&(hypStateVectorList[hypSortMap[hyp]])));
              continue;
            }
          } /* end if not reentry */
          /* (End speedup) */


          hypReEntryFlagList[hypSortMap[hyp]] = 2; /* For next unifyH() */
          hypStepList[hypSortMap[hyp]] = trialStep;

          if (!hypOrSubproofFlag) {
            /* We're scanning required hypotheses */
            nmbrLet((nmbrString **)(&hypProofList[hypSortMap[hyp]]),
                nmbrAddElement(NULL_NMBRSTRING,
                g_Statement[provStmtNum].reqHypList[
                trialStep - (scanLowerBound - reqHyps)]));
          } else {
            /* We're scanning the subproof */
            i = subproofLen(g_ProofInProgress.proof, trialStep);
            nmbrLet((nmbrString **)(&hypProofList[hypSortMap[hyp]]),
                nmbrSeg(g_ProofInProgress.proof, trialStep - i + 2,
                trialStep + 1));
          }

          foundTrialStepMatch = 1;
          break;
        } /* end if (!tmpFlag || tmpFlag = 2) */
      } /* next trialStep */

      if (!foundTrialStepMatch) {
        hasDummyVar = 0;
        hypLen = nmbrLen(hypMakeSubstList[hypSortMap[hyp]]);
        for (sym = 0; sym < hypLen; sym++) {
          k = ((nmbrString *)(hypMakeSubstList[hypSortMap[hyp]]))[sym];
          if (k > g_mathTokens/*global*/) {
            hasDummyVar = 1;
            break;
          }
        }
        /* There was no (completely known) step in the (sub)proof that
           matched the hypothesis.  If it's a $f hypothesis, we will try
           to prove it by itself. */
        /* (However, if this is 2nd pass of backtrack, i.e. reenterFFlag is
           set, we already got an exact $f match earlier and don't need this
           scan, and shouldn't do it to prevent inf. loop.) */
        if ((hyp >= schEHyps || searchMethod == 1) && !reenterFFlag) {
          /* It's a $f hypothesis, or any hypothesis when searchMethod=1 */
          if (hasDummyVar) {
            /* If it's a $f and we have dummy vars, that is bad so we leave
               foundTrialStepMatch = 0 to backtrack */
            if (hyp < schEHyps) {
              /* It's a $e with dummy vars, so we flag it to try later in
                 case further matches get rid of the dummy vars */
              tryFloatingProofLater[hyp] = 'Y';
              /* Unify the hypothesis with itself to initialize the
                 stateVector to allow further substitutions */
              tmpFlag = unifyH(hypMakeSubstList[hypSortMap[hyp]],
                  hypMakeSubstList[hypSortMap[hyp]],
                  (pntrString **)(&(hypStateVectorList[hypSortMap[hyp]])),
                  /* (Remember: 1 = false, 2 = true in hypReEntryFlagList) */
                  hypReEntryFlagList[hypSortMap[hyp]] - 1);
              if (tmpFlag != 1) bug (1849);  /* This should be a trivial
                      unification, so it should never fail */
              foundTrialStepMatch = 1; /* So we can continue */
            }
          } else {
            saveUnifTrialCount = g_unifTrialCount; /* Save unification timeout */
            hypProofPtr =
                proveFloating(hypMakeSubstList[hypSortMap[hyp]],
                    provStmtNum, improveDepth, prfStep, noDistinct,
                    overrideFlag, mathboxFlag);
            g_unifTrialCount = saveUnifTrialCount; /* Restore unif. timeout */
            if (nmbrLen(hypProofPtr)) { /* Proof was found */
              nmbrLet((nmbrString **)(&hypProofList[hypSortMap[hyp]]),
                  NULL_NMBRSTRING);
              hypProofList[hypSortMap[hyp]] = hypProofPtr;
              foundTrialStepMatch = 1;
              hypReEntryFlagList[hypSortMap[hyp]] = 3;
              /* Set flag so that we won't attempt subst. on $e w/ float prf */
              hasFloatingProof[hyp] = 'Y';
            }
          }
        } /* end if $f, or $e and searchMethod 1 */
      }

      if (hyp == schEHyps - 1 && foundTrialStepMatch) {
        /* Scan all the postponed $e hypotheses in case they are known now */
        for (i = 0; i < schEHyps; i++) {
          if (tryFloatingProofLater[i] == 'Y') {

            /* Incorporate substitutions of all later hypotheses
               into this one (only earlier ones were done in main scan) */
            for (j = i + 1; j < schEHyps; j++) {
              if (hasFloatingProof[j] == 'Y') continue;
              makeSubstPtr = makeSubstUnif(&dummyVarFlag,
                  hypMakeSubstList[hypSortMap[i]],
                  hypStateVectorList[hypSortMap[j]]);
              nmbrLet((nmbrString **)(&(hypMakeSubstList[hypSortMap[i]])),
                  NULL_NMBRSTRING);
              hypMakeSubstList[hypSortMap[i]] = makeSubstPtr;
            }

            hasDummyVar = 0;
            hypLen = nmbrLen(hypMakeSubstList[hypSortMap[i]]);
            for (sym = 0; sym < hypLen; sym++) {
              k = ((nmbrString *)(hypMakeSubstList[hypSortMap[i]]))[sym];
              if (k > g_mathTokens/*global*/) {
                hasDummyVar = 1;
                break;
              }
            }
            if (hasDummyVar) {
              foundTrialStepMatch = 0; /* Force backtrack */
              /* If we don't have a proof at this point, we didn't save
                 enough info to backtrack easily, so we'll break out to
                 the top-most next unification (if any). */
              hyp = 0; /* Force breakout below */
              break;
            }
            saveUnifTrialCount = g_unifTrialCount; /* Save unification timeout */
            hypProofPtr =
                proveFloating(hypMakeSubstList[hypSortMap[i]],
                    provStmtNum, improveDepth, prfStep, noDistinct,
                    overrideFlag, mathboxFlag);
            g_unifTrialCount = saveUnifTrialCount; /* Restore unif. timeout */
            if (nmbrLen(hypProofPtr)) { /* Proof was found */
              nmbrLet((nmbrString **)(&hypProofList[hypSortMap[i]]),
                  NULL_NMBRSTRING);
              hypProofList[hypSortMap[i]] = hypProofPtr;
              foundTrialStepMatch = 1;
              hypReEntryFlagList[hypSortMap[i]] = 3;
              /* Set flag so that we won't attempt subst. on $e w/ float prf */
              hasFloatingProof[i] = 'Y';
            } else {   /* Proof not found */
              foundTrialStepMatch = 0; /* Force backtrack */
              /* If we don't have a proof at this point, we didn't save
                 enough info to backtrack easily, so we'll break out to
                 the top-most next unification (if any). */
              hyp = 0; /* Force breakout below */
              break;
            } /* if (nmbrLen(hypProofPtr)) */
          } /* if (tryFloatingProofLater[i] == 'Y') */
        } /* for (i = 0; i < schEHyps; i++) */
      } /* if (hyp == schEHyps - 1 && foundTrialStepMatch) */

      if (!foundTrialStepMatch) {
        /* We must backtrack */

        /* Backtrack through all of postponed hypotheses with
           dummy variables whose proof wasn't found yet.  If we
           don't do this, we could end up with an infinite loop since we
           would just repeat the postponement and move forward again. */
        for (i = hyp - 1; i >=0; i--) {
          if (tryFloatingProofLater[i] == 'N') break;
          if (tryFloatingProofLater[i] != 'Y') bug(1853);
          hyp--;
        }

        if (hyp == 0) {
          /* No more possibilities to try */
          noHypMatch = 1;
          break;
        }
        hyp = hyp - 2; /* Go back one interation (subtract 2 to offset
                          end of loop increment) */
      } /* if (!foundTrialStepMatch) */

    } /* next hyp */

    if (noHypMatch) {
      /* Proof was not found for some hypothesis. */
      continue; /* Get next unification */
    } /* End if noHypMatch */

    /* Proofs were found for all hypotheses */

    /* Build the proof */
    for (hyp = 0; hyp < schReqHyps; hyp++) {
      if (nmbrLen(hypProofList[hyp]) == 0) bug(1852); /* Should have proof */
      nmbrLet(&proof, nmbrCat(proof, hypProofList[hyp], NULL));
    }
    nmbrLet(&proof, nmbrAddElement(proof, replStatemNum));
                                                     /* Complete the proof */

    /* Deallocate hypothesis schemes and proofs */
    /* 25-Jun-2014 This is now done after returnPoint (why was it incomplete?)
    for (hyp = 0; hyp < schReqHyps; hyp++) {
      nmbrLet((nmbrString **)(&hypList[hyp]), NULL_NMBRSTRING);
      nmbrLet((nmbrString **)(&hypProofList[hyp]), NULL_NMBRSTRING);
    }
    */
    goto returnPoint;

  } /* End while (next unifyH() call for main replacement statement) */

  nmbrLet(&proof, NULL_NMBRSTRING);  /* Proof not possible */

 returnPoint:

  /* Deallocate hypothesis schemes and proofs */
  for (hyp = 0; hyp < schReqHyps; hyp++) {
    nmbrLet((nmbrString **)(&(hypList[hyp])), NULL_NMBRSTRING);
    nmbrLet((nmbrString **)(&(hypProofList[hyp])), NULL_NMBRSTRING);
    nmbrLet((nmbrString **)(&(hypMakeSubstList[hyp])), NULL_NMBRSTRING);
    purgeStateVector((pntrString **)(&(hypStateVectorList[hyp])));
  }

  /* Deallocate unification state vector */
  purgeStateVector(&stateVector);

  nmbrLet(&scheme, NULL_NMBRSTRING);
  pntrLet(&hypList, NULL_PNTRSTRING);
  nmbrLet(&hypSortMap, NULL_NMBRSTRING);
  pntrLet(&hypProofList, NULL_PNTRSTRING);
  pntrLet(&hypMakeSubstList, NULL_PNTRSTRING);
  pntrLet(&hypStateVectorList, NULL_PNTRSTRING);
  let(&hypReEntryFlagList, "");
  nmbrLet(&hypStepList, NULL_NMBRSTRING);
  let(&indepKnownSteps, "");
  let(&hasFloatingProof, "");
  let(&tryFloatingProofLater, "");


/*E*/if(db8)print2("%s\n", cat("Returned: ",
/*E*/   nmbrCvtRToVString(proof,
/*E*/                0, /*explicitTargets*/
/*E*/                0 /*statemNum, used only if explicitTargets*/), NULL));
  return (proof); /* Caller must deallocate */
} /* replaceStatement */



/* This function identifies all steps in the proof in progress that (1) are
   independent of step refStep, (2) have no dummy variables, (3) are
   not $f's or $e's, and (4) have subproofs that are complete
   (no unassigned steps).  A "Y" is returned for each such step,
   and "N" is returned for all other steps.  The "Y" steps can be used
   for scanning for useful subproofs outside of the subProof of refStep.
   Note: The caller must deallocate the returned vstring. */
vstring getIndepKnownSteps(long proofStmt, long refStep)
{
  long proofLen, prfStep, step2;
  long wrkSubPfLen, mathLen;
  nmbrString *proofStepContent;
  vstring indepSteps = "";
  vstring unkSubPrfSteps = ""; /* 'K' if subproof is known, 'U' if unknown */

  /* g_ProofInProgress is global */
  proofLen = nmbrLen(g_ProofInProgress.proof);
  /* Preallocate the return argument */
  let(&indepSteps, string(proofLen, 'N'));

  /* Scan back from last step to get independent subproofs */
  for (prfStep = proofLen - 2 /*next to last step*/; prfStep >= 0;
      prfStep--) {
    wrkSubPfLen = subproofLen(g_ProofInProgress.proof, prfStep);
    if (prfStep >= refStep && prfStep - wrkSubPfLen + 1 <= refStep) {
      /* The subproof includes the refStep; reject it */
      continue;
    }
    /* Mark all steps in the subproof as independent */
    for (step2 = prfStep - wrkSubPfLen + 1; step2 <= prfStep; step2++) {
      if (indepSteps[step2] == 'Y') bug(1832); /* Should be 1st Y assignment */
      indepSteps[step2] = 'Y';
    }
    /* Speedup: skip over independent subproof to reduce subproofLen() calls */
    prfStep = prfStep - wrkSubPfLen + 1; /* Decrement loop counter */
        /* (Note that a 1-step subproof won't modify loop counter) */
  } /* next prfStep */

  /* Scan all of the 'Y' steps and mark them as 'N' if $e, $f, or the
     step has dummy variables */
  for (prfStep = 0; prfStep < proofLen; prfStep++) {
    if (indepSteps[prfStep] == 'N') continue;

    /* Flag $e, $f as 'N' */
    proofStmt = (g_ProofInProgress.proof)[prfStep];
    if (proofStmt < 0) {
      if (proofStmt == -(long)'?') {
        /* indepSteps[prfStep] = 'N' */ /* We can still use its mathstring */
      } else {
        bug(1833); /* Packed ("squished") proof not handled (yet?) */
      }
    } else {
      if (g_Statement[proofStmt].type == (char)e_
          || g_Statement[proofStmt].type == (char)f_) {
        /* $e or $f */
        indepSteps[prfStep] = 'N';
      } else if (g_Statement[proofStmt].type != (char)p_
            && g_Statement[proofStmt].type != (char)a_) {
        bug(1834);
      }
    }

    if (indepSteps[prfStep] == 'N') continue;

    /* Flag statements with dummy variables with 'N' */

    /* Get the math tokens in the proof step */
    proofStepContent = (g_ProofInProgress.target)[prfStep];

    /* Do not consider unknown subproof steps or those with
       unknown variables */
    mathLen = nmbrLen(proofStepContent);
    if (mathLen == 0) bug(1835); /* Shouldn't be empty */
    for (mathLen = mathLen - 1; mathLen >= 0; mathLen--) {
      if (((nmbrString *)proofStepContent)[mathLen] >
          g_mathTokens/*global*/) {
        /* The token is a dummy variable */
        indepSteps[prfStep] = 'N';
        break;
      }
    }
  } /* next prfStep */

  /* Identify subproofs that have unknown steps */
  unkSubPrfSteps = getKnownSubProofs();
  /* Propagate unknown subproofs to Y/N flags */
  for (prfStep = 0; prfStep < proofLen; prfStep++) {
    if (unkSubPrfSteps[prfStep] == 'U') indepSteps[prfStep] = 'N';
  }

  let(&unkSubPrfSteps, ""); /* Deallocate */

  return indepSteps; /* Caller must deallocate */

} /* getIndepKnownSteps */



/* This function classifies each proof step in g_ProofInProgress.proof
   as known or unknown ('K' or 'U' in the returned string) depending
   on whether the step has a completely known subproof.
   Note: The caller must deallocate the returned vstring. */
vstring getKnownSubProofs(void)
{
  long proofLen, hyp;
  vstring unkSubPrfSteps = ""; /* 'K' if subproof is known, 'U' if unknown */
  vstring unkSubPrfStack = ""; /* 'K' if subproof is known, 'U' if unknown */
  long stackPtr, prfStep, stmt;

  /* g_ProofInProgress is global */
  proofLen = nmbrLen(g_ProofInProgress.proof);

  /* Scan the proof and identify subproofs that have unknown steps */
  let(&unkSubPrfSteps, space(proofLen));
  let(&unkSubPrfStack, space(proofLen));
  stackPtr = -1;
  for (prfStep = 0; prfStep < proofLen; prfStep++) {
    stmt = (g_ProofInProgress.proof)[prfStep];
    if (stmt < 0) { /* Unknown step or local label */
      if (stmt != -(long)'?') bug(1837); /* We don't handle compact proofs */
      unkSubPrfSteps[prfStep] = 'U'; /* Subproof is unknown */
      stackPtr++;
      unkSubPrfStack[stackPtr] = 'U';
      continue;
    }
    if (g_Statement[stmt].type == (char)e_ ||
        g_Statement[stmt].type == (char)f_) { /* A hypothesis */
      unkSubPrfSteps[prfStep] = 'K'; /* Subproof is known */
      stackPtr++;
      unkSubPrfStack[stackPtr] = 'K';
      continue;
    }
    if (g_Statement[stmt].type != (char)a_ &&
        g_Statement[stmt].type != (char)p_) bug(1838);
    unkSubPrfSteps[prfStep] = 'K';  /* Start assuming subproof is known */
    for (hyp = 1; hyp <= g_Statement[stmt].numReqHyp; hyp++) {
      if (stackPtr < 0) bug(1839);
      if (unkSubPrfStack[stackPtr] == 'U') {
        /* If any hypothesis is unknown, the statement's subproof is unknown */
        unkSubPrfSteps[prfStep] = 'U';
      }
      stackPtr--;
    }
    stackPtr++;
    if (stackPtr < 0) bug(1840);
    unkSubPrfStack[stackPtr] = unkSubPrfSteps[prfStep];
  } /* next prfStep */
  if (stackPtr != 0) bug(1841);
  let(&unkSubPrfStack, ""); /* Deallocate */
  return unkSubPrfSteps; /* Caller must deallocate */

} /* getKnownSubProofs */



/* Add a subproof in place of an unknown step to g_ProofInProgress.  The
   .target, .source, and .user fields are initialized to empty (except
   .target and .user of the deleted unknown step are retained). */
void addSubProof(nmbrString *subProof, long step) {
  long sbPfLen;

  if ((g_ProofInProgress.proof)[step] != -(long)'?') bug(1803);
                     /* Only unknown steps should be allowed at cmd interface */
  sbPfLen = nmbrLen(subProof);
  nmbrLet(&g_ProofInProgress.proof, nmbrCat(nmbrLeft(g_ProofInProgress.proof, step),
      subProof, nmbrRight(g_ProofInProgress.proof, step + 2), NULL));
  pntrLet(&g_ProofInProgress.target, pntrCat(pntrLeft(g_ProofInProgress.target,
      step), pntrNSpace(sbPfLen - 1), pntrRight(g_ProofInProgress.target,
      step + 1), NULL));
  /* Deallocate .source in case not empty (if not, though, it's a bug) */
  if (nmbrLen((g_ProofInProgress.source)[step])) bug(1804);
 /*nmbrLet((nmbrString **)(&((g_ProofInProgress.source)[step])), NULL_NMBRSTRING);*/
  pntrLet(&g_ProofInProgress.source, pntrCat(pntrLeft(g_ProofInProgress.source,
      step), pntrNSpace(sbPfLen - 1), pntrRight(g_ProofInProgress.source,
      step + 1), NULL));
  pntrLet(&g_ProofInProgress.user, pntrCat(pntrLeft(g_ProofInProgress.user,
      step), pntrNSpace(sbPfLen - 1), pntrRight(g_ProofInProgress.user,
      step + 1), NULL));
} /* addSubProof */

/* This function eliminates any occurrences of statement sourceStmtNum in the
   targetProof by substituting it with the proof of sourceStmtNum.  The
   unchanged targetProof is returned if there was an error. */
/* Normally, targetProof is the global g_ProofInProgress.proof.  However,
   we make it an argument in case in the future we'd like to do this
   outside of the Proof Assistant. */
/* The rawTargetProof may be uncompressed or compressed. */
nmbrString *expandProof(
    nmbrString *rawTargetProof, /* May be compressed or uncompressed */
    long sourceStmtNum   /* The statement whose proof will be expanded */
    /* , long targetStmtNum */) { /* The statement begin proved */
  nmbrString *origTargetProof = NULL_NMBRSTRING;
  nmbrString *targetProof = NULL_NMBRSTRING;
  nmbrString *sourceProof = NULL_NMBRSTRING;
  nmbrString *expandedTargetProof = NULL_NMBRSTRING;
  pntrString *hypSubproofs = NULL_PNTRSTRING;
  nmbrString *expandedSubproof = NULL_NMBRSTRING;
  long targetStep, srcHyp, hypStep, totalSubpLen, subpLen, srcStep;
  long sourcePLen, sourceHyps, targetPLen, targetSubpLen;
  flag hasDummyVar = 0;
  flag hasUnknownStep = 0;
  char srcStepType;
  long srcHypNum;
  flag foundMatch;

  sourceProof = getProof(sourceStmtNum, 0); /* Retrieve from source file */
  nmbrLet(&sourceProof, nmbrUnsquishProof(sourceProof)); /* Uncompress */
  /* (The following nmbrUnsquishProof() is unnecessary when called from
     within the Proof Assistant.) */
  nmbrLet(&origTargetProof, nmbrUnsquishProof(rawTargetProof)); /* Uncompress */
  nmbrLet(&expandedTargetProof, origTargetProof);
  sourcePLen = nmbrLen(sourceProof);
  sourceHyps = nmbrLen(g_Statement[sourceStmtNum].reqHypList);
  pntrLet(&hypSubproofs, pntrNSpace(sourceHyps)); /* pntrNSpace initializes
        to null nmbrStrings */
  if (g_Statement[sourceStmtNum].type != (char)p_) {
    /* Caller should enforce $p statements only */
    bug(1871);
    nmbrLet(&expandedTargetProof, targetProof);
    goto RETURN_POINT;
  }

  while (1) { /* Restart after every expansion (to handle nested
                       references to sourceStmtNum correctly) */
    nmbrLet(&targetProof, expandedTargetProof);
    targetPLen = nmbrLen(targetProof);
    foundMatch = 0;
    for (targetStep = targetPLen - 1; targetStep >= 0; targetStep--) {
      if (targetProof[targetStep] != sourceStmtNum) continue;
      foundMatch = 1;
      /* Found a use of the source statement in the proof */
      targetSubpLen = subproofLen(targetProof, targetStep);
      /* Collect the proofs of the hypotheses */
      /*pntrLet(&hypSubproofs, pntrNSpace(sourceHyps));*/ /* done above */
      hypStep = targetStep - 1;
      totalSubpLen = 0;
      for (srcHyp = sourceHyps - 1; srcHyp >= 0; srcHyp--) {
        subpLen = subproofLen(targetProof, hypStep);
                                      /* Find length of proof of hypothesis */
        nmbrLet((nmbrString **)(&(hypSubproofs[srcHyp])),
          nmbrMid(targetProof, (hypStep + 1) - (subpLen - 1), subpLen));
                                    /* Note that nmbrStrings start at 1, not 0 */
        hypStep = hypStep - subpLen;
        totalSubpLen = totalSubpLen + subpLen;
      }
      if (totalSubpLen != targetSubpLen - 1) {
        /* Independent calculation of source statement subproof failed.
           Could be caused by corrupted proof also.  If this is confirmed,
           change the bug() to an error message (or depend on getProof() error
           messages) */
        bug(1872);
        nmbrLet(&expandedTargetProof, targetProof);
        goto RETURN_POINT;
      }

      /* Build the expanded subproof */
      nmbrLet(&expandedSubproof, NULL_NMBRSTRING);
      /* Scan the proof of the statement to be expanded */
      for (srcStep = 0; srcStep < sourcePLen; srcStep++) {
        /* 14-Sep-2016 nm */
        if (sourceProof[srcStep] < 0) {
          if (sourceProof[srcStep] == -(long)'?') {
            /* It's an unknown step in the source proof; make it an
               unknown step in the target proof */
            hasUnknownStep = 1;
          } else {
            /* It shouldn't be a compressed proof because we called
               unSquishProof() above */
            bug(1873);
          }
          /* Assign unknown to the target proof */
          nmbrLet(&expandedSubproof, nmbrAddElement(expandedSubproof,
              -(long)'?'));
          continue;
        }
        srcStepType = g_Statement[sourceProof[srcStep]].type;
        if (srcStepType == (char)e_ || srcStepType == (char)f_) {
          srcHypNum = -1;  /* Means the step is not a (required) hypothesis */
          for (srcHyp = 0; srcHyp < sourceHyps; srcHyp++) {
            /* Find out if the proof step references a required hyp */
            if ((g_Statement[sourceStmtNum].reqHypList)[srcHyp]
                == sourceProof[srcStep]) {
              srcHypNum = srcHyp;
              break;
            }
          }
          if (srcHypNum > -1) {
            /* It's a required hypothesis */
            nmbrLet(&expandedSubproof, nmbrCat(expandedSubproof,
                hypSubproofs[srcHypNum], NULL));
          } else if (srcStepType == (char)e_) {
            /* A non-required hypothesis cannot be $e */
            bug(1874);
          } else if (srcStepType == (char)f_) {
            /* It's an optional hypothesis (dummy variable), which we don't
               know what it will be in final proof, so make it an unknown
               step in final proof */
            hasDummyVar = 1;
            nmbrLet(&expandedSubproof, nmbrAddElement(expandedSubproof,
                -(long)'?'));
          }
        } else if (srcStepType != (char)a_ && srcStepType != (char)p_) {
          bug(1875);
        } else {
          /* It's a normal statement reference ($a, $p); use it as is */
          /* (This adds normal ref steps one by one, each requiring a new
             allocation in nmbrAddElement.  This should be OK if, as expected,
             only relatively short proofs are expanded.  If it becomes a problem,
             we can modify the code to do bigger chunks of $a, $p steps at a
             time.) */
          nmbrLet(&expandedSubproof, nmbrAddElement(expandedSubproof,
              sourceProof[srcStep]));
        } /* if srcStepType... *? */
      } /* next srcStep */
      /* Insert the expanded subproof into the final expanded proof */
      nmbrLet(&expandedTargetProof, nmbrCat(
          nmbrLeft(expandedTargetProof, (targetStep + 1) - targetSubpLen),
          expandedSubproof,
          nmbrRight(expandedTargetProof, (targetStep + 1) + 1), NULL));
      break; /* Start over after processing an expansion */
    } /* next targetStep */
    if (!foundMatch) break;
    /* A matching statement was expanded.  Start over so we can accurate
       process nested references to sourceStmt */
  } /* end while(1) */


 RETURN_POINT:
  if (nmbrEq(origTargetProof, expandedTargetProof)) {
    /*
    print2("No expansion occurred.  The proof was not changed.\n");
    */
  } else {
    /*
    printLongLine(cat("All references to theorem \"",
        g_Statement[sourceStmtNum].labelName,
        "\" were expanded in the proof of \"",
        g_Statement[targetStmtNum].labelName,
        "\", which increased from ",
        str((double)(nmbrLen(targetProof))), " to ",
        str((double)(nmbrLen(expandedTargetProof))), " steps (uncompressed).",
        NULL), " ", " ");
    */
    if (hasDummyVar == 1) {
      printLongLine(cat(
      "******* Note: The expansion of \"",
      g_Statement[sourceStmtNum].labelName,
      "\" has dummy variable(s) that need to be assigned.", NULL), " ", " ");
    }
    if (hasUnknownStep == 1) {
      printLongLine(cat(
      "******* Note: The expansion of \"",
      g_Statement[sourceStmtNum].labelName,
      "\" has unknown step(s) that need to be assigned.", NULL), " ", " ");
    }
  }
  /* Deallocate memory */
  nmbrLet(&sourceProof, NULL_NMBRSTRING);
  nmbrLet(&origTargetProof, NULL_NMBRSTRING);
  nmbrLet(&targetProof, NULL_NMBRSTRING);
  nmbrLet(&expandedSubproof, NULL_NMBRSTRING);
  /* Deallocate array entries */
  for (srcHyp = 0; srcHyp < sourceHyps; srcHyp++) {
    nmbrLet((nmbrString **)(&(hypSubproofs[srcHyp])), NULL_NMBRSTRING);
  }
  /* Deallocate array */
  pntrLet(&hypSubproofs, NULL_PNTRSTRING);
  return expandedTargetProof;
} /* expandProof */


/* Delete a subproof starting (in reverse from) step.  The step is replaced
   with an unknown step, and its .target and .user fields are retained. */
void deleteSubProof(long step) {
  long sbPfLen, pos;

  /* Unknown step should not be allowed at cmd interface */
  /* Don't do anything if step is unassigned. */
  if ((g_ProofInProgress.proof)[step] == -(long)'?') return;

  sbPfLen = subproofLen(g_ProofInProgress.proof, step);
  nmbrLet(&g_ProofInProgress.proof, nmbrCat(nmbrAddElement(
      nmbrLeft(g_ProofInProgress.proof, step - sbPfLen + 1), -(long)'?'),
      nmbrRight(g_ProofInProgress.proof, step + 2), NULL));
  for (pos = step - sbPfLen + 1; pos <= step; pos++) {
    if (pos < step) {
      /* Deallocate .target and .user */
      nmbrLet((nmbrString **)(&((g_ProofInProgress.target)[pos])), NULL_NMBRSTRING);
      nmbrLet((nmbrString **)(&((g_ProofInProgress.user)[pos])), NULL_NMBRSTRING);
    }
    /* Deallocate .source */
    nmbrLet((nmbrString **)(&((g_ProofInProgress.source)[pos])), NULL_NMBRSTRING);
  }
  pntrLet(&g_ProofInProgress.target, pntrCat(pntrLeft(g_ProofInProgress.target,
      step - sbPfLen + 1), pntrRight(g_ProofInProgress.target,
      step + 1), NULL));
  pntrLet(&g_ProofInProgress.source, pntrCat(pntrLeft(g_ProofInProgress.source,
      step - sbPfLen + 1), pntrRight(g_ProofInProgress.source,
      step + 1), NULL));
  pntrLet(&g_ProofInProgress.user, pntrCat(pntrLeft(g_ProofInProgress.user,
      step - sbPfLen + 1), pntrRight(g_ProofInProgress.user,
      step + 1), NULL));
} /* deleteSubProof */


/* Check to see if a statement will match the g_ProofInProgress.target (or .user)
   of an unknown step.  Returns 1 if match, 0 if not, 2 if unification
   timed out. */
char checkStmtMatch(long statemNum, long step)
{
  char targetFlag;
  char userFlag = 1; /* Default if no user field */
  pntrString *stateVector = NULL_PNTRSTRING;
  nmbrString *mString; /* Pointer only; not allocated */
  nmbrString *scheme = NULL_NMBRSTRING;
  long targetLen, mStringLen, reqVars, stsym, tasym, sym, var, hyp, numHyps;
  flag breakFlag;
  flag firstSymbsAreConstsFlag;

  targetLen = nmbrLen((g_ProofInProgress.target)[step]);
  if (!targetLen) bug(1807);

  /* If the statement is a hypothesis, just see if it unifies. */
  if (g_Statement[statemNum].type == (char)e_ || g_Statement[statemNum].type ==
      (char)f_) {

    /* Make sure it's a hypothesis of the statement being proved */
    breakFlag = 0;
    numHyps = g_Statement[g_proveStatement].numReqHyp;
    for (hyp = 0; hyp < numHyps; hyp++) {
      if (g_Statement[g_proveStatement].reqHypList[hyp] == statemNum) {
        breakFlag = 1;
        break;
      }
    }
    if (!breakFlag) { /* Not a required hypothesis; is it optional? */
      numHyps = nmbrLen(g_Statement[g_proveStatement].optHypList);
      for (hyp = 0; hyp < numHyps; hyp++) {
        if (g_Statement[g_proveStatement].optHypList[hyp] == statemNum) {
          breakFlag = 1;
          break;
        }
      }
      if (!breakFlag) { /* Not a hypothesis of statement being proved */
        targetFlag = 0;
        goto returnPoint;
      }
    }

    g_unifTrialCount = 1; /* Reset unification timeout */
    targetFlag = unifyH((g_ProofInProgress.target)[step],
        g_Statement[statemNum].mathString, &stateVector, 0);
   if (nmbrLen((g_ProofInProgress.user)[step])) {
      g_unifTrialCount = 1; /* Reset unification timeout */
      userFlag = unifyH((g_ProofInProgress.user)[step],
        g_Statement[statemNum].mathString, &stateVector, 0);
    }
    goto returnPoint;
  }

  mString = g_Statement[statemNum].mathString;
  mStringLen = g_Statement[statemNum].mathStringLen;

  /* For speedup - 1st, 2nd, & last math symbols should match if constants */
  /* (The speedup is only done for .target; the .user is assumed to be
     infrequent.) */
  /* First symbol */
  firstSymbsAreConstsFlag = 0;
  stsym = mString[0];
  tasym = ((nmbrString *)((g_ProofInProgress.target)[step]))[0];
  if (g_MathToken[stsym].tokenType == (char)con_) {
    if (g_MathToken[tasym].tokenType == (char)con_) {
      firstSymbsAreConstsFlag = 1; /* The first symbols are constants */
      if (stsym != tasym) {
        targetFlag = 0;
        goto returnPoint;
      }
    }
  }
  /* Last symbol */
  stsym = mString[mStringLen - 1];
  tasym = ((nmbrString *)((g_ProofInProgress.target)[step]))[targetLen - 1];
  if (stsym != tasym) {
    if (g_MathToken[stsym].tokenType == (char)con_) {
      if (g_MathToken[tasym].tokenType == (char)con_) {
        targetFlag = 0;
        goto returnPoint;
      }
    }
  }
  /* Second symbol */
  if (targetLen > 1 && mStringLen > 1 && firstSymbsAreConstsFlag) {
    stsym = mString[1];
    tasym = ((nmbrString *)((g_ProofInProgress.target)[step]))[1];
    if (stsym != tasym) {
      if (g_MathToken[stsym].tokenType == (char)con_) {
        if (g_MathToken[tasym].tokenType == (char)con_) {
          targetFlag = 0;
          goto returnPoint;
        }
      }
    }
  }

  /* Change variables in statement to dummy variables for unification */
  nmbrLet(&scheme, mString);
  reqVars = nmbrLen(g_Statement[statemNum].reqVarList);
  if (reqVars + g_pipDummyVars > g_dummyVars) {
    /* Declare more dummy vars if necessary */
    declareDummyVars(reqVars + g_pipDummyVars - g_dummyVars);
  }
  for (var = 0; var < reqVars; var++) {
    /* Put dummy var mapping into g_MathToken[].tmp field */
    g_MathToken[g_Statement[statemNum].reqVarList[var]].tmp = g_mathTokens + 1 +
        g_pipDummyVars + var;
  }
  for (sym = 0; sym < mStringLen; sym++) {
    if (g_MathToken[scheme[sym]].tokenType != (char)var_)
        continue;
    /* Use dummy var mapping from g_MathToken[].tmp field */
    scheme[sym] = g_MathToken[scheme[sym]].tmp;
  }

  /* Now see if we can unify */
  g_unifTrialCount = 1; /* Reset unification timeout */
  targetFlag = unifyH((g_ProofInProgress.target)[step],
      scheme, &stateVector, 0);
  if (nmbrLen((g_ProofInProgress.user)[step])) {
    g_unifTrialCount = 1; /* Reset unification timeout */
    userFlag = unifyH((g_ProofInProgress.user)[step],
      scheme, &stateVector, 0);
  }

 returnPoint:
  nmbrLet(&scheme, NULL_NMBRSTRING);
  purgeStateVector(&stateVector);

  if (!targetFlag || !userFlag) return (0);
  if (targetFlag == 1 && userFlag == 1) return (1);
  return (2);

} /* checkStmtMatch */

/* Check to see if a (user-specified) math string will match the
   g_ProofInProgress.target (or .user) of an step.  Returns 1 if match, 0 if
   not, 2 if unification timed out. */
char checkMStringMatch(nmbrString *mString, long step)
{
  pntrString *stateVector = NULL_PNTRSTRING;
  char targetFlag;
  char sourceFlag = 1; /* Default if no .source */

  g_unifTrialCount = 1; /* Reset unification timeout */
  targetFlag = unifyH(mString, (g_ProofInProgress.target)[step],
      &stateVector, 0);
  if (nmbrLen((g_ProofInProgress.source)[step])) {
    g_unifTrialCount = 1; /* Reset unification timeout */
    sourceFlag = unifyH(mString, (g_ProofInProgress.source)[step],
        &stateVector, 0);
  }

  purgeStateVector(&stateVector);

  if (!targetFlag || !sourceFlag) return (0);
  if (targetFlag == 1 && sourceFlag == 1) return (1);
  return (2);

} /* checkMStringMatch */


/* Find proof of formula or simple theorem (no new vars in $e's) */
/* maxEDepth is the maximum depth at which statements with $e hypotheses are
   considered.  A value of 0 means none are considered. */
/* The caller must deallocate the returned nmbrString. */
nmbrString *proveFloating(nmbrString *mString, long statemNum, long maxEDepth,
    long step, /* 0 means step 1; used for messages */
    flag noDistinct, /* 1 means don't try statements with $d's */
    flag overrideFlag, /* 1 means to override usage locks, 2 means to
              override silently (for web-page syntax breakdown in mmcmds.c) */
    flag mathboxFlag) {

  long reqHyps, optHyps;
  long hyp, stmt, sym, var, i, j;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *scheme = NULL_NMBRSTRING;
  pntrString *hypList = NULL_PNTRSTRING;
  nmbrString *hypOrdMap = NULL_NMBRSTRING; /* Order remapping for speedup */
  pntrString *hypProofList = NULL_PNTRSTRING;
  pntrString *stateVector = NULL_PNTRSTRING;
  nmbrString *stmtMathPtr;
  nmbrString *hypSchemePtr;
  nmbrString *hypProofPtr;
  nmbrString *makeSubstPtr;
  flag reEntryFlag;
  flag tmpFlag;
  flag breakFlag;
  flag firstEHypFlag;
  long schemeLen, schemeVars, schReqHyps, hypLen, reqVars;
  long saveUnifTrialCount;
  static long depth = 0;
  static long trials;
  static flag maxDepthExceeded;
  long selfScanSteps;
  long selfScanStep;
  long prfMbox;

/*E*/  long unNum;
/*E*/if (db8)print2("%s\n", cat(space(depth+2), "Entered: ",
/*E*/   nmbrCvtMToVString(mString), NULL));

  prfMbox = getMathboxNum(statemNum);

  if (depth == 0) {
    trials = 0; /* Initialize trials */
    maxDepthExceeded = 0;
  } else {
    trials++;
  }
  depth++; /* Update backtracking depth */
  if (trials > g_userMaxProveFloat) {
    nmbrLet(&proof, NULL_NMBRSTRING);
    print2(
"Exceeded trial limit at step %ld.  You may increase with SET SEARCH_LIMIT.\n",
        (long)(step + 1));
    goto returnPoint;
  }

  if (maxDepthExceeded) {
    /* Pop out of the recursive calls to avoid an infinite loop */
    nmbrLet(&proof, NULL_NMBRSTRING);
    goto returnPoint;
  }

#define MAX_DEPTH 40  /* > this, infinite loop assumed */ /*???User setting?*/
  if (depth > MAX_DEPTH) {
    nmbrLet(&proof, NULL_NMBRSTRING);
/*??? Document in Metamath manual. */
    printLongLine(cat(
       "?Warning: A possible infinite loop was found in $f hypothesis ",
       "backtracking (i.e., depth > ", str((double)MAX_DEPTH),
       ").  The last proof attempt was for math string \"",
       nmbrCvtMToVString(mString),
       "\".  Your axiom system may have an error ",
       "or you may have to SET EMPTY_SUBSTITUTION ON.", NULL), " ", " ");
    maxDepthExceeded = 1;  /* Flag to exit recursion */
    goto returnPoint;
  }


  /* First see if mString matches a required or optional hypothesis; if so,
     we're done; the proof is just the hypothesis. */
  reqHyps = g_Statement[statemNum].numReqHyp;
  for (hyp = 0; hyp < reqHyps; hyp++) {
    if (nmbrEq(mString,
        g_Statement[g_Statement[statemNum].reqHypList[hyp]].mathString)) {
      nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING,
          g_Statement[statemNum].reqHypList[hyp]));
      goto returnPoint;
    }
  }
  optHyps = nmbrLen(g_Statement[statemNum].optHypList);
  for (hyp = 0; hyp < optHyps; hyp++) {
    if (nmbrEq(mString,
        g_Statement[g_Statement[statemNum].optHypList[hyp]].mathString)) {
      nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING,
          g_Statement[statemNum].optHypList[hyp]));
      goto returnPoint;
    }
  }

  /* Scan all proved steps in the current proof to see if the
     statement has already been proved in another subproof */
  selfScanSteps = nmbrLen(g_ProofInProgress.proof); /* Original proof length */
  /* Note: proveFloating() can be called from typeStatement() (for HTML syntax
     breakdown), and we don't want to do a self-scan that case.  If
     g_ProofInProgress.proof has a non-zero length, it tells us that
     we are in Proof Assistant mode.  If g_ProofInProgress.proof has zero
     length, the loop below will be skipped, and we're still OK. */
  /* We scan backwards for maximum speed since IMPROVE ALL processes steps
     backwards, so we maximize the chance of a proved hit earlier on */
  for (selfScanStep = selfScanSteps - 1; selfScanStep >= 0; selfScanStep--) {
    if (nmbrEq(mString, (g_ProofInProgress.target)[selfScanStep])) {
      /* The step matches.  Now see if the step was proved. */

      /* Get the subproof at the step */
      /* Note that for subproof length of 1, the 2nd argument of nmbrSeg
         evaluates to selfScanStep + 1, so nmbrSeg will be length 1 */
      nmbrLet(&proof, nmbrSeg(g_ProofInProgress.proof, selfScanStep -
          subproofLen(g_ProofInProgress.proof, selfScanStep) + 2,
          selfScanStep + 1));

      /* Check to see that the subproof has no unknown steps. */
      if (nmbrElementIn(1, proof, -(long)'?')) {
        /* Clear out the trial proof */
        nmbrLet(&proof, NULL_NMBRSTRING);
        /* And give up this trial */
        continue; /* next selfScanStep */
      }
      /* Otherwise, we've found our proof; use it and exit */
      goto returnPoint;
    } /* if (nmbrEq(mString, (g_ProofInProgress.target)[selfScanStep]) */
  } /* Next selfScanStep */

  /* Scan all statements up to the current statement to see if we can unify */

  /* Reversed scan order so that w3a will match before
     wa, helping to prevent an exponential blowup for definition syntax
     breakdown.  If wa is first, then wa will incorrectly match a w3a
     subexpression, with the incorrect trial only detected deeper down;
     whereas w3a will rarely match a wa subexpression, so the trial match
     will get rejected immediately. */
  for (stmt = statemNum - 1; stmt >= 1; stmt--) {

    /* Separated quick filter for reuse in other functions */
    if (quickMatchFilter(stmt, mString, 0/*no dummy vars*/) == 0) continue;

    if (!overrideFlag && getMarkupFlag(stmt, USAGE_DISCOURAGED)) {
      /* Skip usage-discouraged statements */
      continue;
    }

    /* Skip statements in other mathboxes unless /INCLUDE_MATHBOXES.  (We don't
       care about the first mathbox since there are no others above it.) */
    if (mathboxFlag == 0 && prfMbox >= 2) {
      /* Note that g_mathboxStart[] starts a 0 */
      if (stmt > g_mathboxStmt && stmt < g_mathboxStart[prfMbox - 1]) {
        continue;
      }
    }

    /* noDistinct is set by NO_DISTICT qualifier in IMPROVE */
    if (noDistinct) {
      /* Skip the statement if it has a $d requirement.  This option
         prevents illegal minimizations that would violate $d requirements
         since the Proof Assistant does not check for $d violations. */
      if (nmbrLen(g_Statement[stmt].reqDisjVarsA)) {
        continue;
      }
    }

    stmtMathPtr = g_Statement[stmt].mathString;
    schemeLen = nmbrLen(stmtMathPtr);
    schReqHyps = g_Statement[stmt].numReqHyp;
    reqVars = nmbrLen(g_Statement[stmt].reqVarList);

    /* Skip any statements with $e hypotheses based on maxEDepth */
    /* (This prevents exponential growth of backtracking) */
    breakFlag = 0;
    firstEHypFlag = 1;
    for (hyp = 0; hyp < schReqHyps; hyp++) {
      if (g_Statement[g_Statement[stmt].reqHypList[hyp]].type == (char)e_) {
        /* (???Maybe, in the future, we'd want to do this only for depths >
           a small nonzero amount -- specified by global variable) */
        if (depth > maxEDepth) {
          breakFlag = 1;
          break;
        } else {
          /* We should also skip cases where a $e hypothesis has a variable
             not in the assertion. */
          if (firstEHypFlag) { /* This scan is needed only once */
            /* First, set g_MathToken[].tmp for each required variable */
            for (var = 0; var < reqVars; var++) {
              g_MathToken[g_Statement[stmt].reqVarList[var]].tmp = 1;
            }
            /* Next, clear g_MathToken[].tmp for each symbol in scheme */
            for (sym = 0; sym < schemeLen; sym++) {
              g_MathToken[stmtMathPtr[sym]].tmp = 0;
            }
            /* If any were left over, a $e hyp. has a new variable. */
            for (var = 0; var < reqVars; var++) {
              if (g_MathToken[g_Statement[stmt].reqVarList[var]].tmp) {
                breakFlag = 1;
                break;
              }
            }
            if (breakFlag) break;
            firstEHypFlag = 0; /* Don't need to do this scan again for stmt. */
          } /* End if firstHypFlag */
        } /* End if depth > maxEDepth */
      } /* End if $e */
    } /* Next hyp */
    if (breakFlag) continue; /* To next stmt */



    /* Change all variables in the statement to dummy vars for unification */
    nmbrLet(&scheme, stmtMathPtr);
    schemeVars = reqVars; /* S.b. same after eliminated new $e vars above */
    if (schemeVars + g_pipDummyVars > g_dummyVars) {
      /* Declare more dummy vars if necessary */
      declareDummyVars(schemeVars + g_pipDummyVars - g_dummyVars);
    }
    for (var = 0; var < schemeVars; var++) {
      /* Put dummy var mapping into g_MathToken[].tmp field */
      g_MathToken[g_Statement[stmt].reqVarList[var]].tmp = g_mathTokens + 1 +
          g_pipDummyVars + var;
    }
    for (sym = 0; sym < schemeLen; sym++) {
      if (g_MathToken[stmtMathPtr[sym]].tokenType != (char)var_) continue;
      /* Use dummy var mapping from g_MathToken[].tmp field */
      scheme[sym] = g_MathToken[stmtMathPtr[sym]].tmp;
    }

    /* Change all variables in the statement's hyps to dummy vars for subst. */
    pntrLet(&hypList, pntrNSpace(schReqHyps));
    nmbrLet(&hypOrdMap, nmbrSpace(schReqHyps));
    pntrLet(&hypProofList, pntrNSpace(schReqHyps));
    for (hyp = 0; hyp < schReqHyps; hyp++) {
      hypSchemePtr = NULL_NMBRSTRING;
      nmbrLet(&hypSchemePtr,
        g_Statement[g_Statement[stmt].reqHypList[hyp]].mathString);
      hypLen = nmbrLen(hypSchemePtr);
      for (sym = 0; sym < hypLen; sym++) {
        if (g_MathToken[hypSchemePtr[sym]].tokenType
            != (char)var_) continue;
        /* Use dummy var mapping from g_MathToken[].tmp field */
        hypSchemePtr[sym] = g_MathToken[hypSchemePtr[sym]].tmp;
      }
      hypList[hyp] = hypSchemePtr;
      hypOrdMap[hyp] = hyp;
    }

    g_unifTrialCount = 1; /* Reset unification timeout */
    reEntryFlag = 0; /* For unifyH() */

/*E*/unNum = 0;
    while (1) { /* Try all possible unifications */
      tmpFlag = unifyH(scheme, mString, &stateVector, reEntryFlag);
      if (!tmpFlag) break; /* (Next) unification not possible */
      if (tmpFlag == 2) {
        print2(
"Unification timed out.  SET UNIFICATION_TIMEOUT larger for better results.\n");
        g_unifTrialCount = 1; /* Reset unification timeout */
        break; /* Treat timeout as if unification not possible */
      }

/*E*/unNum++;
/*E*/if (db8)print2("%s\n", cat(space(depth+2), "Testing unification ",
/*E*/   str((double)unNum), " statement ", g_Statement[stmt].labelName,
/*E*/   ": ", nmbrCvtMToVString(scheme), NULL));
      reEntryFlag = 1; /* For next unifyH() */

      /* Make substitutions into each hypothesis, and try to prove that
         hypothesis */
      nmbrLet(&proof, NULL_NMBRSTRING);
      breakFlag = 0;
      for (hyp = 0; hyp < schReqHyps; hyp++) {
/*E*/if (db8)print2("%s\n", cat(space(depth+2), "Proving hyp. ",
/*E*/   str((double)(hypOrdMap[hyp])), "(#", str((double)hyp), "):  ",
/*E*/   nmbrCvtMToVString(hypList[hypOrdMap[hyp]]), NULL));
        makeSubstPtr = makeSubstUnif(&tmpFlag, hypList[hypOrdMap[hyp]],
            stateVector);
        if (tmpFlag) bug(1808); /* No dummy vars. should result unless bad $a's*/
                            /*??? Implement an error check for this in parser */

        saveUnifTrialCount = g_unifTrialCount; /* Save unification timeout */
        hypProofPtr = proveFloating(makeSubstPtr, statemNum, maxEDepth, step,
            noDistinct,
            overrideFlag, /* 3-May-2016 nm */
            mathboxFlag /* 5-Aug-2020 nm */
            );
        g_unifTrialCount = saveUnifTrialCount; /* Restore unification timeout */

        nmbrLet(&makeSubstPtr, NULL_NMBRSTRING); /* Deallocate */
        if (!nmbrLen(hypProofPtr)) {
          /* Not possible */
          breakFlag = 1;
          break;
        }

        /* Deallocate in case this is the 2nd or later pass of main
           unification */
        nmbrLet((nmbrString **)(&hypProofList[hypOrdMap[hyp]]),
            NULL_NMBRSTRING);

        hypProofList[hypOrdMap[hyp]] = hypProofPtr;
      }
      if (breakFlag) {
       /* Proof is not possible for some hypothesis. */

       /* Perhaps the search limit was reached. */
       if (trials > g_userMaxProveFloat) {
         /* Deallocate hypothesis schemes and proofs */
         for (hyp = 0; hyp < schReqHyps; hyp++) {
           nmbrLet((nmbrString **)(&hypList[hyp]), NULL_NMBRSTRING);
           nmbrLet((nmbrString **)(&hypProofList[hyp]), NULL_NMBRSTRING);
         }
         /* The error message has already been printed. */
         nmbrLet(&proof, NULL_NMBRSTRING);
         goto returnPoint;
       }

       /* Speedup:  Move the hypothesis for which the proof was not found
          to the beginning of the hypothesis list, so it will be tried
          first next time. */
       j = hypOrdMap[hyp];
       for (i = hyp; i >= 1; i--) {
         hypOrdMap[i] = hypOrdMap[i - 1];
       }
       hypOrdMap[0] = j;

       continue; /* Not possible; get next unification */

      } /* End if breakFlag */

      /* Proofs were found for all hypotheses */

      /* Build the proof */
      for (hyp = 0; hyp < schReqHyps; hyp++) {
        nmbrLet(&proof, nmbrCat(proof, hypProofList[hyp], NULL));
      }

      if (getMarkupFlag(stmt, USAGE_DISCOURAGED)) {
        switch (overrideFlag) {
          case 0: bug(1869); break; /* Should never get here if no override */
          case 2: break; /* Accept overrided silently (in mmcmds.c syntax
                            breakdown calls for $a web pages) */
          case 1:  /* Normal override */
            /* print2("\n"); */ /* Enable for more emphasis */
            print2(
          ">>> ?Warning: Overriding discouraged usage of statement \"%s\".\n",
                g_Statement[stmt].labelName);
            /* print2("\n"); */ /* Enable for more emphasis */
            break;
          default: bug(1870); /* Illegal value */
        } /* end switch (overrideFlag) */
      } /* end if (getMarkupFlag(stmt, USAGE_DISCOURAGED)) */

      /* TODO: Put this in proveByReplacement? */
      /* Notify mathbox user when other mathboxes are used */
      if (mathboxFlag != 0) {  /* Skip unless /INCLUDE_MATHBOXES was specified */
        /* See if it's in another mathbox; if so, let user know */
        assignMathboxInfo();
        if (stmt > g_mathboxStmt && g_proveStatement > g_mathboxStmt) {
          if (stmt < g_mathboxStart[getMathboxNum(g_proveStatement) - 1]) {
            printLongLine(cat("Used \"", g_Statement[stmt].labelName,
                  "\" from the mathbox for ",
                  g_mathboxUser[getMathboxNum(stmt) - 1], ".",
                  NULL),
                "  ", " ");
          }
        }
      }

      nmbrLet(&proof, nmbrAddElement(proof, stmt)); /* Complete the proof */

      /* Deallocate hypothesis schemes and proofs */
      for (hyp = 0; hyp < schReqHyps; hyp++) {
        nmbrLet((nmbrString **)(&hypList[hyp]), NULL_NMBRSTRING);
        nmbrLet((nmbrString **)(&hypProofList[hyp]), NULL_NMBRSTRING);
      }
      goto returnPoint;

    } /* End while (next unifyH() call) */

    /* Deallocate hypothesis schemes and proofs */
    for (hyp = 0; hyp < schReqHyps; hyp++) {
      nmbrLet((nmbrString **)(&hypList[hyp]), NULL_NMBRSTRING);
      nmbrLet((nmbrString **)(&hypProofList[hyp]), NULL_NMBRSTRING);
    }

  } /* Next stmt */

  nmbrLet(&proof, NULL_NMBRSTRING);  /* Proof not possible */

 returnPoint:
  /* Deallocate unification state vector */
  purgeStateVector(&stateVector);

  nmbrLet(&scheme, NULL_NMBRSTRING);
  pntrLet(&hypList, NULL_PNTRSTRING);
  nmbrLet(&hypOrdMap, NULL_NMBRSTRING);
  pntrLet(&hypProofList, NULL_PNTRSTRING);
  depth--; /* Restore backtracking depth */
/*E*/if(db8)print2("%s\n", cat(space(depth+2), "Returned: ",
/*E*/   nmbrCvtRToVString(proof,
/*E*/                0, /*explicitTargets*/
/*E*/                0 /*statemNum, used only if explicitTargets*/), NULL));
/*E*/if(db8){if(!depth)print2("Trials: %ld\n", trials);}
  return (proof); /* Caller must deallocate */
} /* proveFloating */



/* This function does quick check for some common conditions that prevent
   a trial statement (scheme) from being unified with a given instance.
   Return value 0 means it can't be unified, 1 means it might be unifiable. */
INLINE flag quickMatchFilter(long trialStmt, nmbrString *mString,
    long dummyVarFlag /* 0 if no dummy vars in mString */) {
  /* This function used to be part of proveFloating().
     It was separated out for reuse in other places */
  long sym;
  long firstSymbol, secondSymbol, lastSymbol;
  nmbrString *stmtMathPtr;
  flag breakFlag;
  long schemeLen, mStringLen;

  if (g_Statement[trialStmt].type != (char)p_ &&
      g_Statement[trialStmt].type != (char)a_) return 0; /* Not $a or $p */

  /* This section is common to all trial statements and in principle
     could be computed once for speedup (it used to be when this code
     was in g_proveStatement() ), but it doesn't seem too compute-intensive. */
  mStringLen = nmbrLen(mString);
  firstSymbol = mString[0];
  if (g_MathToken[firstSymbol].tokenType != (char)con_) firstSymbol = 0;
  if (mStringLen > 1) {
    secondSymbol = mString[1];
    if (g_MathToken[secondSymbol].tokenType != (char)con_) secondSymbol = 0;
    /* If first symbol is a variable, second symbol shouldn't be tested. */
    if (!firstSymbol) secondSymbol = 0;
  } else {
    secondSymbol = 0;
  }
  lastSymbol = mString[mStringLen - 1];
  if (g_MathToken[lastSymbol].tokenType != (char)con_) lastSymbol = 0;
  /* (End of common section) */


  stmtMathPtr = g_Statement[trialStmt].mathString;

  /* Speedup:  if first or last tokens in instance and scheme are constants,
     they must match */
  if (firstSymbol) { /* First symbol in mString is a constant */
    if (firstSymbol != stmtMathPtr[0]) {
      if (g_MathToken[stmtMathPtr[0]].tokenType == (char)con_) return 0;
    }
  }

  schemeLen = nmbrLen(stmtMathPtr);

  /* ...Continuation of speedup */
  if (secondSymbol) { /* Second symbol in mString is a constant */
    if (schemeLen > 1) {
      if (secondSymbol != stmtMathPtr[1]) {
        /* Second symbol should be tested only if 1st symbol is a constant */
        if (g_MathToken[stmtMathPtr[0]].tokenType == (char)con_) {
          if (g_MathToken[stmtMathPtr[1]].tokenType == (char)con_)
              return 0;
        }
      }
    }
  }
  if (lastSymbol) { /* Last symbol in mString is a constant */
    if (lastSymbol != stmtMathPtr[schemeLen - 1]) {
      if (g_MathToken[stmtMathPtr[schemeLen - 1]].tokenType ==
         (char)con_) return 0;
    }
  }

  /* Speedup:  make sure all constants in scheme are in instance (i.e.
     mString) */
  /* First, set g_MathToken[].tmp for all symbols in scheme */
  for (sym = 0; sym < schemeLen; sym++) {
    g_MathToken[stmtMathPtr[sym]].tmp = 1;
  }
  /* Next, clear g_MathToken[].tmp for all symbols in instance */
  for (sym = 0; sym < mStringLen; sym++) {
    g_MathToken[mString[sym]].tmp = 0;
  }
  /* Finally, check that they got cleared for all constants in scheme */
  /* Only do this when there are no dummy variables in mString; this
     is the case when dummyVarFlag = 0 (we depend on caller to set this
     correctly) */
  if (dummyVarFlag == 0) {
    breakFlag = 0;
    for (sym = 0; sym < schemeLen; sym++) {
      if (g_MathToken[stmtMathPtr[sym]].tokenType == (char)con_) {
        if (g_MathToken[stmtMathPtr[sym]].tmp) {
          breakFlag = 1;
          break;
        }
      }
    }
    if (breakFlag) return 0; /* No match */
  } /* if dummyVarFlag == 0 */

  return 1;

} /* quickMatchFilter */


/* Shorten proof by using specified statement. */
void minimizeProof(long repStatemNum, long prvStatemNum,
    flag allowGrowthFlag)
{
  /* repStatemNum is the statement number we're trying to use
     in the proof to shorten it */
  /* prvStatemNum is the statement number we're proving */
  /* allowGrowthFlag means to make the replacement when possible,
     even if it doesn't shorten the proof length */

  long plen, step, mlen, sym, sublen;
  long startingPlen = 0;
  flag foundFlag, breakFlag;
  nmbrString *mString; /* Pointer only; not allocated */
  nmbrString *newSubProofPtr = NULL_NMBRSTRING; /* Pointer only; not allocated;
                however initialize for nmbrLen function before it's assigned */
  if (allowGrowthFlag) startingPlen = nmbrLen(g_ProofInProgress.proof);

  while (1) {
    plen = nmbrLen(g_ProofInProgress.proof);
    foundFlag = 0;
    for (step = plen - 1; step >= 0; step--) {
      /* Reject step with dummy vars */
      mString = (g_ProofInProgress.target)[step];
      mlen = nmbrLen(mString);
      breakFlag = 0;
      for (sym = 0; sym < mlen; sym++) {
        if (mString[sym] > g_mathTokens) {
          /* It is a dummy var. (i.e. work variable $1, $2, etc.) */
          breakFlag = 1;
          break;
        }
      }
      if (breakFlag) continue;  /* Step has dummy var.; don't try it */

      /* Reject step not matching replacement step */
      if (!checkStmtMatch(repStatemNum, step)) {
        continue;
      }

      /* Try the replacement */
      /* Don't replace a step with itself (will cause infinite loop in
         ALLOW_GROWTH mode) */
      if ((g_ProofInProgress.proof)[step] != repStatemNum
          /* || 1 */  /* For special replacement with same label; also below */
          /* When not in ALLOW_GROWTH mode i.e. when an infinite loop can't
             occur, we _do_ let a label be tested against itself so that e.g. a
             do-nothing chain of bitr's/pm4.2's will be trimmed off with a
             better bitr match. */
          || !allowGrowthFlag) {
        newSubProofPtr = replaceStatement(repStatemNum,
            step,
            prvStatemNum,
            1,/*scan just subproof for speed*/
            0,/*noDistinct=0 OK since searchMethod=0 will only
               call proveFloating for $f's */
            0,/*searchMethod=0: call proveFloating only for $f's*/
            0,/*improveDepth=0 OK since we call proveFloating only for $f's*/
            2,/*overrideFlag=2(silent) OK since MINIMIZE_WITH checked it*/
            1/*mathboxFlag=1 since MINIMIZE_WITH has checked it before here*/
            );
      }
      if (!nmbrLen(newSubProofPtr)) continue;
                                           /* Replacement was not successful */

      if (nmbrElementIn(1, newSubProofPtr, -(long)'?')) {
        /* 8/28/99 Don't do a replacement if the replacement has unknown
           steps - this causes assignKnownSteps to abort, and it's not
           clear if we should do that anyway since it doesn't necessarily
           minimize the proof */
        nmbrLet(&newSubProofPtr, NULL_NMBRSTRING); /* Deallocate */
        continue;
      }

      /* Get the subproof at step s */
      sublen = subproofLen(g_ProofInProgress.proof, step);
      if (sublen > nmbrLen(newSubProofPtr) || allowGrowthFlag) {
        /* Success - proof length was reduced */
        /* 7-Jun-2011 nm Delete the old subproof only if it is not an unknown
           step (since if it is an unknown step, it is already deleted) */
        if ((g_ProofInProgress.proof)[step] == -(long)'?') {
          /* 7-Jun-2011 nm This can only occur in / ALLOW_GROWTH mode */
          if (!allowGrowthFlag) bug(1831);
        } else {
          deleteSubProof(step);
        }
        addSubProof(newSubProofPtr, step - sublen + 1);
        assignKnownSteps(step - sublen + 1, nmbrLen(newSubProofPtr));
        foundFlag = 1;
        nmbrLet(&newSubProofPtr, NULL_NMBRSTRING);
        break;
      }

      nmbrLet(&newSubProofPtr, NULL_NMBRSTRING);
    } /* next step */

    if (!foundFlag) break; /* Done */

#define MAX_GROWTH_FACTOR 2
    if (allowGrowthFlag && plen > MAX_GROWTH_FACTOR * startingPlen) {
      /* This will prevent an infinite loop in some cases with ALLOW_GROWTH,
         for example 'MINIMIZE_WITH idi/ALLOW_GROWTH' in 'PROVE a1i' */
      print2("Suppressed excessive ALLOW_GROWTH growth.\n");
      break; /* Too much growth */
    }
    /* break; */  /* For special replacement with same label: always break
                      to prevent inf loop */
  } /* end while */

} /* minimizeProof */


/* Initialize g_ProofInProgress.source of the step, and .target of all
   hypotheses, to schemes using new dummy variables. */
void initStep(long step)
{
  long stmt, reqHyps, pos, hyp, sym, reqVars, var, mlen;
  nmbrString *reqHypPos = NULL_NMBRSTRING;
  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated */

  stmt = (g_ProofInProgress.proof)[step];
  if (stmt < 0) {
    if (stmt == -(long)'?') {
      /* Initialize unknown step source to nothing */
      nmbrLet((nmbrString **)(&((g_ProofInProgress.source)[step])),
          NULL_NMBRSTRING);
    } else {
/*E*/print2("step %ld stmt %ld\n",step,stmt);
      bug(1809); /* Packed ("squished") proof not handled (yet?) */
    }
    return;
  }
  if (g_Statement[stmt].type == (char)e_ || g_Statement[stmt].type == (char)f_) {
    /* A hypothesis -- initialize to the actual statement */
    nmbrLet((nmbrString **)(&((g_ProofInProgress.source)[step])),
        g_Statement[stmt].mathString);
    return;
  }

  /* It must be an assertion ($a or $p) */

  /* Assign the assertion to .source */
  nmbrLet((nmbrString **)(&((g_ProofInProgress.source)[step])),
      g_Statement[stmt].mathString);

  /* Find the position in proof of all required hyps, and
     assign them */
  reqHyps = g_Statement[stmt].numReqHyp;
  nmbrLet(&reqHypPos, nmbrSpace(reqHyps)); /* Preallocate */
  pos = step - 1; /* Step with last hyp */
  for (hyp = reqHyps - 1; hyp >= 0; hyp--) {
    reqHypPos[hyp] = pos;
    nmbrLet((nmbrString **)(&((g_ProofInProgress.target)[pos])),
        g_Statement[g_Statement[stmt].reqHypList[hyp]].mathString);
                                           /* Assign the hypothesis to target */
    if (hyp > 0) { /* Don't care about subproof length for 1st hyp */
      pos = pos - subproofLen(g_ProofInProgress.proof, pos);
                                             /* Get to step with previous hyp */
    }
  }

  /* Change the variables in the assertion and hypotheses to dummy variables */
  reqVars = nmbrLen(g_Statement[stmt].reqVarList);
  if (g_pipDummyVars + reqVars > g_dummyVars) {
    /* Declare more dummy vars if necessary */
    declareDummyVars(g_pipDummyVars + reqVars - g_dummyVars);
  }
  for (var = 0; var < reqVars; var++) {
    /* Put dummy var mapping into g_MathToken[].tmp field */
    g_MathToken[g_Statement[stmt].reqVarList[var]].tmp = g_mathTokens + 1 +
      g_pipDummyVars + var;
  }
  /* Change vars in assertion */
  nmbrTmpPtr = (g_ProofInProgress.source)[step];
  mlen = nmbrLen(nmbrTmpPtr);
  for (sym = 0; sym < mlen; sym++) {
    if (g_MathToken[nmbrTmpPtr[sym]].tokenType == (char)var_) {
      /* Use dummy var mapping from g_MathToken[].tmp field */
      nmbrTmpPtr[sym] = g_MathToken[nmbrTmpPtr[sym]].tmp;
    }
  }
  /* Change vars in hypotheses */
  for (hyp = 0; hyp < reqHyps; hyp++) {
    nmbrTmpPtr = (g_ProofInProgress.target)[reqHypPos[hyp]];
    mlen = nmbrLen(nmbrTmpPtr);
    for (sym = 0; sym < mlen; sym++) {
      if (g_MathToken[nmbrTmpPtr[sym]].tokenType == (char)var_) {
        /* Use dummy var mapping from g_MathToken[].tmp field */
        nmbrTmpPtr[sym] = g_MathToken[nmbrTmpPtr[sym]].tmp;
      }
    }
  }

  /* Update the number of dummy vars used so far */
  g_pipDummyVars = g_pipDummyVars + reqVars;

  nmbrLet(&reqHypPos, NULL_NMBRSTRING); /* Deallocate */

  return;
} /* initStep */




/* Look for completely known subproofs in g_ProofInProgress.proof and
   assign g_ProofInProgress.target and .source.  Calls assignKnownSteps(). */
void assignKnownSubProofs(void)
{
  long plen, pos, subplen, q;
  flag breakFlag;

  plen = nmbrLen(g_ProofInProgress.proof);
  /* Scan proof for known subproofs (backwards, to get biggest ones first) */
  for (pos = plen - 1; pos >= 0; pos--) {
    subplen = subproofLen(g_ProofInProgress.proof, pos); /* Find length of subpr*/
    breakFlag = 0;
    for (q = pos - subplen + 1; q <= pos; q++) {
      if ((g_ProofInProgress.proof)[q] == -(long)'?') {
        breakFlag = 1;
        break;
      }
    }
    if (breakFlag) continue; /* Skip subproof - it has an unknown step */

    /* See if all steps in subproof are assigned and known; if so, don't assign
       them again. */
    /* (???Add this code if needed for speedup) */

    /* Assign the steps of the known subproof to g_ProofInProgress.target */
    assignKnownSteps(pos - subplen + 1, subplen);

    /* Adjust pos for next pass through 'for' loop */
    pos = pos - subplen + 1;

  } /* Next pos */
  return;
} /* assignKnownSubProofs */


/* This function assigns math strings to all steps (g_ProofInProgress.target and
   .source fields) in a subproof with all known steps. */
void assignKnownSteps(long startStep, long sbProofLen)
{

  long stackPtr, st;
  nmbrString *stack = NULL_NMBRSTRING;
  nmbrString *instance = NULL_NMBRSTRING;
  nmbrString *scheme = NULL_NMBRSTRING;
  nmbrString *assertion = NULL_NMBRSTRING;
  long pos, stmt, reqHyps, instLen, instPos, schemeLen, schemePos, hypLen,
      hypPos, hyp, reqVars, var, assLen, assPos;
  flag tmpFlag;
  pntrString *stateVector = NULL_PNTRSTRING;

  nmbrLet(&stack, nmbrSpace(sbProofLen));
  stackPtr = 0;
  for (pos = startStep; pos < startStep + sbProofLen; pos++) {
    stmt = (g_ProofInProgress.proof)[pos];

    if (stmt <= 0) {
      if (stmt != -(long)'?') bug(1810);
                                     /* Packed proofs are not handled (yet?) */
      if (stmt == -(long)'?') bug(1830);
                                    /* Unknown proofs are not handled (yet?) */
    }

    if (g_Statement[stmt].type == (char)e_ || g_Statement[stmt].type == (char)f_){
      /* It's a hypothesis or unknown step; assign step; push the stack */
      nmbrLet((nmbrString **)(&((g_ProofInProgress.source)[pos])),
          g_Statement[stmt].mathString);
      stack[stackPtr] = pos;
      stackPtr++;
    } else {
      /* It's an assertion. */

      /* Assemble the hypotheses for unification */
      reqHyps = g_Statement[stmt].numReqHyp;

      instLen = 1; /* First "$|$" separator token */
      for (st = stackPtr - reqHyps; st < stackPtr; st++) {
        if (st < 0) bug(1850); /* Proof sent in may be corrupted */
        /* Add 1 for "$|$" separator token */
        instLen = instLen + nmbrLen((g_ProofInProgress.source)[stack[st]]) + 1;
      }
      /* Preallocate instance */
      nmbrLet(&instance, nmbrSpace(instLen));
      /* Assign instance */
      instance[0] = g_mathTokens; /* "$|$" separator */
      instPos = 1;
      for (st = stackPtr - reqHyps; st < stackPtr; st++) {
        hypLen = nmbrLen((g_ProofInProgress.source)[stack[st]]);
        for (hypPos = 0; hypPos < hypLen; hypPos++) {
          instance[instPos] =
              ((nmbrString *)((g_ProofInProgress.source)[stack[st]]))[hypPos];
          instPos++;
        }
        instance[instPos] = g_mathTokens; /* "$|$" separator */
        instPos++;
      }
      if (instLen != instPos) bug(1811); /* ???Delete after debugging */

      schemeLen = 1; /* First "$|$" separator token */
      for (hyp = 0; hyp < reqHyps; hyp++) {
        /* Add 1 for "$|$" separator token */
        schemeLen = schemeLen +
            g_Statement[g_Statement[stmt].reqHypList[hyp]].mathStringLen + 1;
      }
      /* Preallocate scheme */
      nmbrLet(&scheme, nmbrSpace(schemeLen));
      /* Assign scheme */
      scheme[0] = g_mathTokens; /* "$|$" separator */
      schemePos = 1;
      for (hyp = 0; hyp < reqHyps; hyp++) {
        hypLen = g_Statement[g_Statement[stmt].reqHypList[hyp]].mathStringLen;
        for (hypPos = 0; hypPos < hypLen; hypPos++) {
          scheme[schemePos] =
              g_Statement[g_Statement[stmt].reqHypList[hyp]].mathString[hypPos];
          schemePos++;
        }
        scheme[schemePos] = g_mathTokens; /* "$|$" separator */
        schemePos++;
      }
      if (schemeLen != schemePos) bug(1812); /* ???Delete after debugging */

      /* Change variables in scheme to dummy variables for unification */
      reqVars = nmbrLen(g_Statement[stmt].reqVarList);
      if (reqVars + g_pipDummyVars > g_dummyVars) {
        /* Declare more dummy vars if necessary */
        declareDummyVars(reqVars + g_pipDummyVars - g_dummyVars);
      }
      for (var = 0; var < reqVars; var++) {
        /* Put dummy var mapping into g_MathToken[].tmp field */
        g_MathToken[g_Statement[stmt].reqVarList[var]].tmp = g_mathTokens + 1 +
          g_pipDummyVars + var;
      }
      for (schemePos = 0; schemePos < schemeLen; schemePos++) {
        if (g_MathToken[scheme[schemePos]].tokenType
            != (char)var_) continue;
        /* Use dummy var mapping from g_MathToken[].tmp field */
        scheme[schemePos] = g_MathToken[scheme[schemePos]].tmp;
      }

      /* Change variables in assertion to dummy variables for substitition */
      nmbrLet(&assertion, g_Statement[stmt].mathString);
      assLen = nmbrLen(assertion);
      for (assPos = 0; assPos < assLen; assPos++) {
        if (g_MathToken[assertion[assPos]].tokenType
            != (char)var_) continue;
        /* Use dummy var mapping from g_MathToken[].tmp field */
        assertion[assPos] = g_MathToken[assertion[assPos]].tmp;
      }

      /* Unify scheme and instance */
      g_unifTrialCount = 0; /* Reset unification to no timeout */
      tmpFlag = unifyH(scheme, instance, &stateVector, 0);
      if (!tmpFlag) {
        /* This is possible if the starting proof had an error
           in it.  Give the user some information then give up */
        printLongLine(cat("?Error in step ", str((double)pos + 1),
            ":  Could not simultaneously unify the hypotheses of \"",
            g_Statement[stmt].labelName, "\":\n    ",
            nmbrCvtMToVString(scheme),
            "\nwith the following statement list:\n    ",
            nmbrCvtMToVString(instance),
            "\n(The $|$ tokens are internal statement separation markers)",
            "\nZapping targets so we can proceed (but you should exit the ",
            "Proof Assistant and fix this problem)",
            "\n(This may take a while; please wait...)",
            NULL), "", " ");
        purgeStateVector(&stateVector);
        goto returnPoint;
      }
      /* Substitute and assign assertion to proof in progress */
      nmbrLet((nmbrString **)(&((g_ProofInProgress.source)[pos])), NULL_NMBRSTRING);
      (g_ProofInProgress.source)[pos] = makeSubstUnif(&tmpFlag, assertion,
          stateVector);
      if (tmpFlag) bug(1814); /* All vars s.b. assigned */

      /* Verify unification is unique; also deallocates stateVector */
      if (unifyH(scheme, instance, &stateVector, 1)) bug(1815); /* Not unique */

      /* Adjust stack */
      stackPtr = stackPtr - reqHyps;
      stack[stackPtr] = pos;
      stackPtr++;

    } /* End if (not) $e, $f */
  } /* Next pos */

  if (stackPtr != 1) bug(1816); /* Make sure stack emptied */

 returnPoint:
  /* Assign .target field for all but last step */
  for (pos = startStep; pos < startStep + sbProofLen - 1; pos++) {
    nmbrLet((nmbrString **)(&((g_ProofInProgress.target)[pos])),
        (g_ProofInProgress.source)[pos]);
  }

  /* Deallocate (stateVector was deallocated by 2nd unif. call) */
  nmbrLet(&stack, NULL_NMBRSTRING);
  nmbrLet(&instance, NULL_NMBRSTRING);
  nmbrLet(&scheme, NULL_NMBRSTRING);
  nmbrLet(&assertion, NULL_NMBRSTRING);
  return;
} /* assignKnownSteps */


/* Interactively unify a step.  Calls interactiveUnify(). */
/* If two unifications must take place (.target,.user and .source,.user),
   then the user must invoke this command twice, as only one will be
   done at a time.  ???Note in manual. */
/* If messageFlag is 1, a message will be issued if the
   step is already unified.   If messageFlag is 0, show the step #
   being unified.  If messageFlag is 2, don't print step #, and do nothing
   if step is already unified. */
void interactiveUnifyStep(long step, char messageFlag)
{
  pntrString *stateVector = NULL_PNTRSTRING;
  char unifFlag;

  /* Target should never be empty */
  if (!nmbrLen((g_ProofInProgress.target)[step])) bug (1817);

  /* First, see if .target and .user should be unified */
  /* If not, then see if .source and .user should be unified */
  /* If not, then see if .target and .source should be unified */
  if (nmbrLen((g_ProofInProgress.user)[step])) {
    if (!nmbrEq((g_ProofInProgress.target)[step], (g_ProofInProgress.user)[step])) {
      if (messageFlag == 0) print2("Step %ld:\n", step + 1);
      unifFlag = interactiveUnify((g_ProofInProgress.target)[step],
        (g_ProofInProgress.user)[step], &stateVector);
      goto subAndReturn;
    }
    if (nmbrLen((g_ProofInProgress.source)[step])) {
      if (!nmbrEq((g_ProofInProgress.source)[step], (g_ProofInProgress.user)[step])) {
        if (messageFlag == 0) print2("Step %ld:\n", step + 1);
        unifFlag = interactiveUnify((g_ProofInProgress.source)[step],
          (g_ProofInProgress.user)[step], &stateVector);
        goto subAndReturn;
      }
    }
  } else {
    if (nmbrLen((g_ProofInProgress.source)[step])) {
      if (!nmbrEq((g_ProofInProgress.target)[step], (g_ProofInProgress.source)[step])) {
        if (messageFlag == 0) print2("Step %ld:\n", step + 1);
        unifFlag = interactiveUnify((g_ProofInProgress.target)[step],
          (g_ProofInProgress.source)[step], &stateVector);
        goto subAndReturn;
      }
    }
  }

  /* The step must already be unified */
  if (messageFlag == 1) {
    print2("?Step %ld is already unified.\n", step + 1);
  }
  unifFlag = 0; /* To skip subst. below */

 subAndReturn:
  /* If the unification was successful, make substitutions everywhere
     before returning */
  if (unifFlag == 1) {

    g_proofChangedFlag = 1; /* Flag to push 'undo' stack */

    makeSubstAll(stateVector);

  } /* End if unifFlag = 1 */

  purgeStateVector(&stateVector);

  return;

} /* interactiveUnifyStep */


/* Interactively select one of several possible unifications */
/* Returns:  0 = no unification possible
             1 = unification was selected; held in stateVector
             2 = unification timed out
             3 = no unification was selected */
char interactiveUnify(nmbrString *schemeA, nmbrString *schemeB,
    pntrString **stateVector)
{

  long var, i;
  long unifCount, unifNum;
  char unifFlag;
  flag reEntryFlag;
  nmbrString *stackUnkVar; /* Pointer only - not allocated */
  nmbrString *unifiedScheme; /* Pointer only - not allocated */
  nmbrString *stackUnkVarLen; /* Pointer only - not allocated */
  nmbrString *stackUnkVarStart; /* Pointer only - not allocated */
  long stackTop;
  vstring tmpStr = "";
  nmbrString *nmbrTmp = NULL_NMBRSTRING;
  char returnValue;

  /* Present unifications in increasing order of the number
     of symbols in the unified result.  It seems that usually the unification
     with the fewest symbols in the correct one. */
  nmbrString *unifWeight = NULL_NMBRSTRING; /* Symbol count in unification */
  long unifTrialWeight;
  long maxUnifWeight;
  long minUnifWeight;
  long unifTrials;
  long thisUnifWeight;
  long onesCount;
  nmbrString *substResult = NULL_NMBRSTRING;
  long unkCount;

  if (nmbrEq(schemeA, schemeB)) bug(1818); /* No reason to call this */

  /* Count the number of possible unifications */
  g_unifTrialCount = 1; /* Reset unification timeout */
  unifCount = 0;
  reEntryFlag = 0;
  minUnifWeight = -1;
  maxUnifWeight = 0;
  while (1) {
    unifFlag = unifyH(schemeA, schemeB, &(*stateVector), reEntryFlag);
    if (unifFlag == 2) {
      printLongLine(
          cat("Unify:  ", nmbrCvtMToVString(schemeA), NULL), "    ", " ");
      printLongLine(
          cat(" with:  ", nmbrCvtMToVString(schemeB), NULL), "    ", " ");
      print2(
"The unification timed out.  Increase timeout (SET UNIFICATION_TIMEOUT) or\n");
      print2(
"assign some variables (LET VARIABLE) or the step (LET STEP) manually.\n");
      returnValue = 2;
      goto returnPoint;
    }
    if (!unifFlag) break;
    reEntryFlag = 1;


    /* Compute heuristic "weight" of resulting unification */
    /* The unification with the least "weight" is intended to be the
       most likely correct choice.  The heuristic was based on
       empirical observations of typical unification sets */

    stackTop = ((nmbrString *)((*stateVector)[11]))[1];
    stackUnkVarStart = (nmbrString *)((*stateVector)[2]);
    stackUnkVarLen = (nmbrString *)((*stateVector)[3]);
    unifiedScheme = (nmbrString *)((*stateVector)[8]);

    /* Heuristic */
    thisUnifWeight = stackTop * 2;
    onesCount = 0;
    unkCount = 0;
    for (var = 0; var <= stackTop; var++) {
      /* Heuristic */
      thisUnifWeight = thisUnifWeight + stackUnkVarLen[var];
      /* Heuristic - Subtract for subst. of length 1 */
      if (stackUnkVarLen[var] == 1) onesCount++;

      /* Count the number of unknown variables in substitution result */
      nmbrLet(&substResult, nmbrMid(unifiedScheme, stackUnkVarStart[var] + 1,
              stackUnkVarLen[var]));
      for (i = 0; i < nmbrLen(substResult); i++) {
        if (substResult[i] > g_mathTokens) unkCount++;
      }

    } /* Next var */
    thisUnifWeight = thisUnifWeight - onesCount;
    thisUnifWeight = thisUnifWeight + 7 * unkCount;


    /* Get new min and max weight for interactive scan ordering */
    if (thisUnifWeight > maxUnifWeight) maxUnifWeight = thisUnifWeight;
    if (thisUnifWeight < minUnifWeight || minUnifWeight == -1)
      minUnifWeight = thisUnifWeight;

    nmbrLet(&unifWeight, nmbrAddElement(unifWeight, 0));

    unifWeight[unifCount] = thisUnifWeight;
    unifCount++;
    if (nmbrLen(unifWeight) != unifCount) bug(1827);
  } /* while (1) */

  if (!unifCount) {
    printf("The unification is not possible.  The proof has an error.\n");
    returnValue = 0;
    goto returnPoint;
  }
  if (unifCount > 1) {
    printLongLine(cat("There are ", str((double)unifCount),
      " possible unifications.  Please select the correct one or QUIT if",
      " you want to UNIFY later.", NULL),
        "    ", " ");
    printLongLine(cat("Unify:  ", nmbrCvtMToVString(schemeA), NULL),
        "    ", " ");
    printLongLine(cat(" with:  ", nmbrCvtMToVString(schemeB), NULL),
        "    ", " ");
  }

  /* Scan possible unifications in order of increasing unified scheme
     size.  This is not an optimal way to do it since the unification must
     be completely redone for each trial size, but since it is interactive
     the speed should be tolerable.  A faster method would be to save
     the unifications and present them for selection in sorted order, but
     this would require more code. */
  unifTrials = 0;
  for (unifTrialWeight = minUnifWeight; unifTrialWeight <= maxUnifWeight;
      unifTrialWeight++) {

    if (!nmbrElementIn(1, unifWeight, unifTrialWeight)) continue;

    g_unifTrialCount = 1; /* Reset unification timeout */
    reEntryFlag = 0;

    for (unifNum = 1; unifNum <= unifCount; unifNum++) {
      unifFlag = unifyH(schemeA, schemeB, &(*stateVector), reEntryFlag);
      if (unifFlag != 1) bug(1819);

      reEntryFlag = 1;
      if (unifWeight[unifNum - 1] != unifTrialWeight) continue;

      if (unifCount == 1) {
        print2("Step was successfully unified.\n");
        returnValue = 1;
        goto returnPoint;
      }

      unifTrials++;
      print2("Unification #%ld of %ld (weight = %ld):\n",
          unifTrials, unifCount, unifTrialWeight);

      stackTop = ((nmbrString *)((*stateVector)[11]))[1];
      stackUnkVar = (nmbrString *)((*stateVector)[1]);
      stackUnkVarStart = (nmbrString *)((*stateVector)[2]);
      stackUnkVarLen = (nmbrString *)((*stateVector)[3]);
      unifiedScheme = (nmbrString *)((*stateVector)[8]);
      for (var = 0; var <= stackTop; var++) {
        printLongLine(cat("  Replace \"",
          g_MathToken[stackUnkVar[var]].tokenName,"\" with \"",
            nmbrCvtMToVString(
                nmbrMid(unifiedScheme,stackUnkVarStart[var] + 1,
                stackUnkVarLen[var])), "\"", NULL),"    "," ");
        /* Clear temporary string allocation during print */
        let(&tmpStr,"");
        nmbrLet(&nmbrTmp,NULL_NMBRSTRING);
      } /* Next var */

      while(1) {
        tmpStr = cmdInput1("  Accept (A), reject (R), or quit (Q) <A>? ");
        if (!tmpStr[0]) {
          /* Default value - accept */
          returnValue = 1;
          goto returnPoint;
        }
        if (tmpStr[0] == 'R' || tmpStr[0] == 'r') {
          if (!tmpStr[1]) {
            let(&tmpStr, "");
            break;
          }
        }
        if (tmpStr[0] == 'Q' || tmpStr[0] == 'q') {
          if (!tmpStr[1]) {
            /*return (3);*/
            returnValue = 3;
            goto returnPoint;
          }
        }
        if (tmpStr[0] == 'A' || tmpStr[0] == 'a') {
          if (!tmpStr[1]) {
            /*return (1);*/
            returnValue = 1;
            goto returnPoint;
          }
        }
        let(&tmpStr, "");
      }

    } /* Next unifNum */

  } /* Next unifTrialWeight */

  /* (The user should reject everything to test for this bug) */
  if (unifTrials != unifCount) bug(1829);

  /* No unification was selected */
  returnValue = 3;
  goto returnPoint;

 returnPoint:
  let(&tmpStr, ""); /* Deallocate */
  nmbrLet(&unifWeight, NULL_NMBRSTRING); /* Deallocate */
  nmbrLet(&substResult, NULL_NMBRSTRING); /* Deallocate */
  return returnValue;

} /* interactiveUnify */



/* Automatically unify steps that have unique unification */
/* Prints "congratulation" if congrats = 1 */
void autoUnify(flag congrats)
{
  long step, plen;
  char unifFlag;
  flag somethingChanged = 1;
  int pass;
  nmbrString *schemeAPtr; /* Pointer only; not allocated */
  nmbrString *schemeBPtr; /* Pointer only; not allocated */
  pntrString *stateVector = NULL_PNTRSTRING;
  flag somethingNotUnified = 0;

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  schemeAPtr = NULL_NMBRSTRING;
  schemeBPtr = NULL_NMBRSTRING;

  plen = nmbrLen(g_ProofInProgress.proof);

  while (somethingChanged) {
    somethingChanged = 0;
    for (step = 0; step < plen; step++) {
      /* stepChanged = 0; */

      for (pass = 0; pass < 3; pass++) {

        switch (pass) {
          case 0:
            /* Check target vs. user */
            schemeAPtr = (g_ProofInProgress.target)[step];
            if (!nmbrLen(schemeAPtr))
              print2("?Bad unification selected:  "
                "A proof step should never be completely empty\n");
            schemeBPtr = (g_ProofInProgress.user)[step];
            break;
          case 1:
            /* Check source vs. user */
            schemeAPtr = (g_ProofInProgress.source)[step];
            schemeBPtr = (g_ProofInProgress.user)[step];
            break;
          case 2:
            /* Check target vs. source */
            schemeAPtr = (g_ProofInProgress.target)[step];
            schemeBPtr = (g_ProofInProgress.source)[step];
            break;
        }
        if (nmbrLen(schemeAPtr) && nmbrLen(schemeBPtr)) {
          if (!nmbrEq(schemeAPtr, schemeBPtr)) {
            g_unifTrialCount = 1; /* Reset unification timeout */
            unifFlag = uniqueUnif(schemeAPtr, schemeBPtr, &stateVector);
            if (unifFlag != 1) somethingNotUnified = 1;
            if (unifFlag == 2) {
              print2("A unification timeout occurred at step %ld.\n", step + 1);
            }
            if (!unifFlag) {
              print2("Step %ld cannot be unified.  "
                "THERE IS AN ERROR IN THE PROOF.\n", (long)(step + 1));
              continue;
            }
            if (unifFlag == 1) {
              /* Make substitutions to all steps */
              makeSubstAll(stateVector);
              somethingChanged = 1;
              g_proofChangedFlag = 1; /* Flag for undo stack */
              /* This message can be annoying. */
              /*
              print2("Step %ld was successfully unified.\n", (long)(step + 1));
              */
            }
          }
        }
      } /* Next pass */
    } /* Next step */
  } /* End while somethingChanged */

  purgeStateVector(&stateVector);

  /* Check to see if proof is complete */
  if (congrats) {
    if (!somethingNotUnified) {
      if (!nmbrElementIn(1, g_ProofInProgress.proof, -(long)'?')) {
        print2(
  "CONGRATULATIONS!  The proof is complete.  Use SAVE NEW_PROOF to save it.\n");
        print2(
  "Note:  The Proof Assistant does not detect $d violations.  After saving\n");
        print2(
  "the proof, you should verify it with VERIFY PROOF.\n");
      }
    }
  }

  return;

} /* autoUnify */


/* Make stateVector substitutions in all steps.  The stateVector must
   contain the result of a valid unification. */
void makeSubstAll(pntrString *stateVector) {

  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated */
  long plen, step;
  flag tmpFlag;

  plen = nmbrLen(g_ProofInProgress.proof);
  for (step = 0; step < plen; step++) {

    nmbrTmpPtr = (g_ProofInProgress.target)[step];
    (g_ProofInProgress.target)[step] = makeSubstUnif(&tmpFlag, nmbrTmpPtr,
      stateVector);
    nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);

    nmbrTmpPtr = (g_ProofInProgress.source)[step];
    if (nmbrLen(nmbrTmpPtr)) {
      (g_ProofInProgress.source)[step] = makeSubstUnif(&tmpFlag, nmbrTmpPtr,
        stateVector);
      nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);
    }

    nmbrTmpPtr = (g_ProofInProgress.user)[step];
    if (nmbrLen(nmbrTmpPtr)) {
      (g_ProofInProgress.user)[step] = makeSubstUnif(&tmpFlag, nmbrTmpPtr,
        stateVector);
      nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);
    }

  } /* Next step */
  return;
} /* makeSubstAll */

/* Replace a dummy variable with a user-specified math string */
void replaceDummyVar(long dummyVar, nmbrString *mString)
{
  long numSubs = 0;
  long numSteps = 0;
  long plen, step, sym, slen;
  flag stepChanged;
  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated */

  plen = nmbrLen(g_ProofInProgress.proof);
  for (step = 0; step < plen; step++) {

    stepChanged = 0;

    nmbrTmpPtr = (g_ProofInProgress.target)[step];
    slen = nmbrLen(nmbrTmpPtr);
    for (sym = slen - 1; sym >= 0; sym--) {
      if (nmbrTmpPtr[sym] == dummyVar + g_mathTokens) {
        nmbrLet((nmbrString **)(&((g_ProofInProgress.target)[step])),
            nmbrCat(nmbrLeft(nmbrTmpPtr, sym), mString,
            nmbrRight(nmbrTmpPtr, sym + 2), NULL));
        nmbrTmpPtr = (g_ProofInProgress.target)[step];
        stepChanged = 1;
        numSubs++;
      }
    } /* Next sym */

    nmbrTmpPtr = (g_ProofInProgress.source)[step];
    slen = nmbrLen(nmbrTmpPtr);
    for (sym = slen - 1; sym >= 0; sym--) {
      if (nmbrTmpPtr[sym] == dummyVar + g_mathTokens) {
        nmbrLet((nmbrString **)(&((g_ProofInProgress.source)[step])),
            nmbrCat(nmbrLeft(nmbrTmpPtr, sym), mString,
            nmbrRight(nmbrTmpPtr, sym + 2), NULL));
        nmbrTmpPtr = (g_ProofInProgress.source)[step];
        stepChanged = 1;
        numSubs++;
      }
    } /* Next sym */

    nmbrTmpPtr = (g_ProofInProgress.user)[step];
    slen = nmbrLen(nmbrTmpPtr);
    for (sym = slen - 1; sym >= 0; sym--) {
      if (nmbrTmpPtr[sym] == dummyVar + g_mathTokens) {
        nmbrLet((nmbrString **)(&((g_ProofInProgress.user)[step])),
            nmbrCat(nmbrLeft(nmbrTmpPtr, sym), mString,
            nmbrRight(nmbrTmpPtr, sym + 2), NULL));
        nmbrTmpPtr = (g_ProofInProgress.user)[step];
        stepChanged = 1;
        numSubs++;
      }
    } /* Next sym */

    if (stepChanged) numSteps++;
  } /* Next step */

  if (numSubs) {
    g_proofChangedFlag = 1; /* Flag to push 'undo' stack */
    print2("%ld substitutions were made in %ld steps.\n", numSubs, numSteps);
  } else {
    print2("?The dummy variable $%ld is nowhere in the proof.\n", dummyVar);
  }

  return;
} /* replaceDummyVar */

/* Get subproof length of a proof, starting at endStep and going backwards.
   Note that the first step is 0, the second is 1, etc. */
long subproofLen(nmbrString *proof, long endStep)
{
  long stmt, p, lvl;
  lvl = 1;
  p = endStep + 1;
  while (lvl) {
    p--;
    lvl--;
    if (p < 0) bug(1821);
    stmt = proof[p];
    if (stmt < 0) { /* Unknown step or local label */
      continue;
    }
    if (g_Statement[stmt].type == (char)e_ ||
        g_Statement[stmt].type == (char)f_) { /* A hypothesis */
      continue;
    }
    lvl = lvl + g_Statement[stmt].numReqHyp;
  }
  return (endStep - p + 1);
} /* subproofLen */


/* If testStep has no dummy variables, return 0;
   if testStep has isolated dummy variables (that don't affect rest of
   proof), return 1;
   if testStep has dummy variables used elsewhere in proof, return 2 */
char checkDummyVarIsolation(long testStep) /* 0=1st step, 1=2nd, etc. */
{
  long proofLen, hyp, parentStep, tokpos, token;
  char dummyVarIndicator;
  long prfStep, parentStmt;
  nmbrString *dummyVarList = NULL_NMBRSTRING;
  flag bugCheckFlag;
  char hypType;

  /* Get list of dummy variables */
  for (tokpos = 0; tokpos < nmbrLen((g_ProofInProgress.target)[testStep]);
      tokpos++) {
    token = ((nmbrString *)((g_ProofInProgress.target)[testStep]))[tokpos];
    if (token > g_mathTokens/*global*/) {
      if (!nmbrElementIn(1, dummyVarList, token)) {
        nmbrLet(&dummyVarList, nmbrAddElement(dummyVarList, token));
      }
    }
  }
  if (nmbrLen(dummyVarList) == 0) {
    dummyVarIndicator = 0; /* No dummy variables */
    goto RETURN_POINT;
  }
  /* g_ProofInProgress is global */
  proofLen = nmbrLen(g_ProofInProgress.proof);
  if (testStep == proofLen - 1) {
    dummyVarIndicator = 1; /* Dummy variables don't affect rest of proof
       (ignoring the subproof of testStep, which would be replaced by
       a replaceStatement success later on) */
    goto RETURN_POINT;
  }

  parentStep = getParentStep(testStep); /* testStep is a hyp of parent step */

  /* Check if parent step has the dummy vars - if not, they will not
     occur outside of the subproof of the parent step */
  for (tokpos = 0; tokpos < nmbrLen((g_ProofInProgress.target)[parentStep]);
      tokpos++) {
    token = ((nmbrString *)((g_ProofInProgress.target)[parentStep]))[tokpos];
    if (token > g_mathTokens/*global*/) {
      if (nmbrElementIn(1, dummyVarList, token)) {
        /* One of testStep's dummy vars occurs in the parent, so it
           could be used elsewhere and is thus not "isolated" */
        dummyVarIndicator = 2;
        goto RETURN_POINT;
      }
    }
  }
  /* Check all hypotheses of parentStep other than testStep - if none have
     testStep's dummy vars, then the dummy vars are "isolated" */
  parentStmt = (g_ProofInProgress.proof)[parentStep];
  if (parentStmt < 0) bug(1845);
  if (g_Statement[parentStmt].type != (char)a_ &&
      g_Statement[parentStmt].type != (char)p_) bug(1846);
  bugCheckFlag = 0;
  prfStep = parentStep - 1;
  for (hyp = g_Statement[parentStmt].numReqHyp - 1; hyp >= 0; hyp--) {
    if (hyp < g_Statement[parentStmt].numReqHyp - 1) { /* Skip computation at
                                       first loop iteration */
      /* Skip to proof step of previous hypothesis of parent step */
      prfStep = prfStep - subproofLen(g_ProofInProgress.proof, prfStep);
    }
    if (prfStep == testStep) { /* Don't check the hypothesis of testStep */
      bugCheckFlag = 1; /* Make sure we encountered it during scan */
      continue;
    }
    hypType = g_Statement[g_Statement[parentStmt].reqHypList[hyp]].type;
    if (hypType == (char)e_) {
      /* Check whether (other) $e hyps of parent step have the dummy vars
         of testStep */
      for (tokpos = 0; tokpos < nmbrLen((g_ProofInProgress.target)[prfStep]);
          tokpos++) {
        token = ((nmbrString *)((g_ProofInProgress.target)[prfStep]))[tokpos];
        if (token > g_mathTokens/*global*/) {
          if (nmbrElementIn(1, dummyVarList, token)) {
            /* One of testStep's dummy vars occurs in the parent, so it
               could be used elsewhere and is thus not "isolated" */
            dummyVarIndicator = 2;
            goto RETURN_POINT;
          }
        }
      } /* next tokpos */
    } else if (hypType != (char)f_) {
      bug(1848);
    }
  } /* next hyp */
  if (bugCheckFlag == 0) bug(1847); /* Scan didn't encounter testStep */
  /* If we passed the whole scan, the dummy vars are "isolated" */
  dummyVarIndicator = 1;

 RETURN_POINT:
  nmbrLet(&dummyVarList, NULL_NMBRSTRING); /* Deallocate */
  return dummyVarIndicator;
} /* checkDummyVarIsolation */


/* Given a starting step, find its parent (the step it is a hypothesis of) */
/* If the starting step is the last proof step, just return it */
long getParentStep(long startStep) /* 0=1st step, 1=2nd, etc. */
{
  long proofLen;
  long stackPtr, prfStep, stmt;

  /* g_ProofInProgress is global */
  proofLen = nmbrLen(g_ProofInProgress.proof);

  stackPtr = 0;
  for (prfStep = startStep + 1; prfStep < proofLen; prfStep++) {
    stmt = (g_ProofInProgress.proof)[prfStep];
    if (stmt < 0) { /* Unknown step or local label */
      if (stmt != -(long)'?') bug(1842); /* We don't handle compact proofs */
      stackPtr++;
    } else if (g_Statement[stmt].type == (char)e_ ||
          g_Statement[stmt].type == (char)f_) { /* A hypothesis */
      stackPtr++;
    } else {
      if (g_Statement[stmt].type != (char)a_ &&
          g_Statement[stmt].type != (char)p_) bug(1843);
      stackPtr = stackPtr - g_Statement[stmt].numReqHyp + 1;
      if (stackPtr <= 0) return prfStep; /* This identifies the parent step */
    }
  } /* next prfStep */
  if (startStep != proofLen - 1) bug(1844); /* Didn't find parent... */
  return startStep; /* ...unless we started with the last proof step */
} /* getParentStep */


/* This function puts numNewVars dummy variables, named "$nnn", at the end
   of the g_MathToken array and modifies the global variable g_dummyVars. */
/* Note:  The g_MathToken array will grow forever as this gets called;
   it is never purged, as this might worsen memory fragmentation. */
/* ???Should we add a purge function? */
void declareDummyVars(long numNewVars)
{

  long i;

  long saveTempAllocStack;
  saveTempAllocStack = g_startTempAllocStack;
  g_startTempAllocStack = g_tempAllocStackTop; /* For let() stack cleanup */

  for (i = 0; i < numNewVars; i++) {

    g_dummyVars++;
    /* First, check to see if we need to allocate more g_MathToken memory */
    if (g_mathTokens + 1 + g_dummyVars >= g_MAX_MATHTOKENS) {
      /* The +1 above accounts for the dummy "$|$" boundary token */
      /* Reallocate */
      /* Add 1000 so we won't have to do this very often */
      g_MAX_MATHTOKENS = g_MAX_MATHTOKENS + 1000;
      g_MathToken = realloc(g_MathToken, (size_t)g_MAX_MATHTOKENS *
        sizeof(struct mathToken_struct));
      if (!g_MathToken) outOfMemory("#10 (mathToken)");
    }

    g_MathToken[g_mathTokens + g_dummyVars].tokenName = "";
                                  /* Initialize vstring before let() */
    let(&g_MathToken[g_mathTokens + g_dummyVars].tokenName,
        cat("$", str((double)g_dummyVars), NULL));
    g_MathToken[g_mathTokens + g_dummyVars].length =
        (long)strlen(g_MathToken[g_mathTokens + g_dummyVars].tokenName);
    g_MathToken[g_mathTokens + g_dummyVars].scope = g_currentScope;
    g_MathToken[g_mathTokens + g_dummyVars].active = 1;
    g_MathToken[g_mathTokens + g_dummyVars].tokenType = (char)var_;
    g_MathToken[g_mathTokens + g_dummyVars].tmp = 0;

  }

  g_startTempAllocStack = saveTempAllocStack;

  return;

} /* declareDummyVars */



/* Copy inProofStruct to outProofStruct.  A proof structure contains
   the state of the proof in the Proof Assistant MM-PA.  The one
   used by MM-PA is the global g_ProofInProgress.  This function lets
   it be copied for temporary storage and retrieval. */
void copyProofStruct(struct pip_struct *outProofStruct,
    struct pip_struct inProofStruct)
{
  long proofLen, j;
  /* First, make sure the output structure is empty to prevent memory
     leaks. */
  deallocProofStruct(&(*outProofStruct));

  /* Get the proof length of the input structure */
  proofLen = nmbrLen(inProofStruct.proof);
  if (proofLen == 0) bug(1854); /* An empty proof should never occur
    here; proof should have at least one step (possibly unknown) */
  if (proofLen == 0) return;  /* The input proof is empty */
  nmbrLet(&((*outProofStruct).proof), inProofStruct.proof);

  /* Allocate pointers to empty nmbrStrings that will be assigned
     the proof step contents */
  pntrLet(&((*outProofStruct).target), pntrNSpace(proofLen));
  pntrLet(&((*outProofStruct).source), pntrNSpace(proofLen));
  pntrLet(&((*outProofStruct).user), pntrNSpace(proofLen));

  if (proofLen != pntrLen(inProofStruct.target)) bug(1855);
  if (proofLen != pntrLen(inProofStruct.source)) bug(1856);
  if (proofLen != pntrLen(inProofStruct.user)) bug(1857);
  /* Copy the individual proof step contents */
  for (j = 0; j < proofLen; j++) {
    nmbrLet((nmbrString **)(&(((*outProofStruct).target)[j])),
        (inProofStruct.target)[j]);
    nmbrLet((nmbrString **)(&(((*outProofStruct).source)[j])),
        (inProofStruct.source)[j]);
    nmbrLet((nmbrString **)(&(((*outProofStruct).user)[j])),
        (inProofStruct.user)[j]);
  }
  return;
} /* copyProofStruct */


/* Create an initial proof structure needed for the Proof Assistant, given
   a starting proof.  Normally, proofStruct is the global g_ProofInProgress,
   although we've made it an argument to help modularize the function.  There
   are still globals such as g_pipDummyVars, updated by various functions. */
void initProofStruct(struct pip_struct *proofStruct, nmbrString *proof,
    long proveStmt)
{
  nmbrString *tmpProof = NULL_NMBRSTRING;
  long plen, step;
  /* Right now, only non-packed proofs are handled. */
  nmbrLet(&tmpProof, nmbrUnsquishProof(proof));

  /* Assign initial proof structure */
  if (nmbrLen((*proofStruct).proof)) bug(1876); /* Should've been deall.*/
  nmbrLet(&((*proofStruct).proof), tmpProof);
  plen = nmbrLen((*proofStruct).proof);
  pntrLet(&((*proofStruct).target), pntrNSpace(plen));
  pntrLet(&((*proofStruct).source), pntrNSpace(plen));
  pntrLet(&((*proofStruct).user), pntrNSpace(plen));
  nmbrLet((nmbrString **)(&(((*proofStruct).target)[plen - 1])),
      g_Statement[proveStmt].mathString);
  g_pipDummyVars = 0; /* (Global) number of dummy (work) $nn variables, updated
        by function calls below */
  /* Assign known subproofs */
  assignKnownSubProofs();
  /* Initialize remaining steps */
  for (step = 0; step < plen/*proof length*/; step++) {
    if (!nmbrLen(((*proofStruct).source)[step])) {
      initStep(step);
    }
  }
  /* Unify whatever can be unified */
  autoUnify(0); /* 0 means no "congrats" message */

  /* Deallocate memory */
  nmbrLet(&tmpProof, NULL_NMBRSTRING);
  return;
} /* initProofStruct */


/* Deallocate memory used by a proof structure and set it to the initial
   state.  A proof structure contains the state of the proof in the Proof
   Assistant MM-PA.  It is assumed that proofStruct was declared with:
     struct pip_struct proofStruct = {
       NULL_NMBRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING };
   This function sets it back to that initial assignment. */
void deallocProofStruct(struct pip_struct *proofStruct)
{
  long proofLen, j;
  /* Deallocate proof structure */
  proofLen = nmbrLen((*proofStruct).proof);
  if (proofLen == 0) return;  /* Already deallocated */
  nmbrLet(&((*proofStruct).proof), NULL_NMBRSTRING);
  for (j = 0; j < proofLen; j++) {
    nmbrLet((nmbrString **)(&(((*proofStruct).target)[j])), NULL_NMBRSTRING);
    nmbrLet((nmbrString **)(&(((*proofStruct).source)[j])), NULL_NMBRSTRING);
    nmbrLet((nmbrString **)(&(((*proofStruct).user)[j])), NULL_NMBRSTRING);
  }
  pntrLet(&((*proofStruct).target), NULL_PNTRSTRING);
  pntrLet(&((*proofStruct).source), NULL_PNTRSTRING);
  pntrLet(&((*proofStruct).user), NULL_PNTRSTRING);
  return;
} /* deallocProofStruct */


#define DEFAULT_UNDO_STACK_SIZE 20
/* This function handles the UNDO/REDO commands.  It is called
   with action PUS_INIT then with PUS_PUSH upon entering MM-PA.  It is
   called with PUS_INIT upon exiting MM-PA.  It should be called with
   PUS_PUSH after every command changing the proof.

   PUS_UNDO and PUS_REDO are called by the UNDO and REDO CLI commands.

   PUS_NEW_SIZE is called by the SET UNDO command to change the size
   of the undo stack.  SET UNDO may be called outside or inside MM-PA;
   in the latter case, the current UNDO stack is aborted (discarded).
   If inside of MM-PA, PUS_PUSH must be called after PUS_NEW_SIZE.

   PUS_GET_SIZE does not affect the stack; it returns the maximum UNDOs
   PUS_GET_STATUS does not affect the stack; it returns 0 if it is
     safe to exit MM-PA without saving (assuming there was no SAVE NEW_PROOF
     while the UNDO stack was not empty).

   Inputs:
   proofStruct - must be current proof in progress (&g_ProofInProgress)
       for PUS_PUSH, PUS_UNDO, and PUS_REDO actions; may be NULL for
       other actions
   action - the action the function should perform
   info - description of command which will be reversed by UNDO; required
       for all PUS_PUSH actions (except the first upon entering MM-PA that
       loads the starting proof structure).  It is ignored for all other
       actions and may be the empty string.
   newSize - for PUS_NEW_SIZE, the new size (>= 0).

   Return value = see PUS_GET_SIZE and PUS_GET_STATUS above
 */
long processUndoStack(struct pip_struct *proofStruct,
    char action,  /* PUS_INIT 1 Deallocates and initializes undo stack
                     PUS_PUSH 2 Pushes the current proof state onto the stack
                     PUS_UNDO 3 Restores the previous proof state
                     PUS_REDO 4 Reverses PUS_UNDO
                     PUS_NEW_SIZE 5 Changes size of stack
                     PUS_GET_SIZE 6 Returns stack size
                     PUS_GET_STATUS 7 Returns proof changed status */
    vstring info, /* Info to print upon PUS_UNDO or PUS_REDO */
    long newSize) /* New maximum number of UNDOs for PUS_NEW_SIZE */
{

  static struct pip_struct *proofStack = NULL;
  static pntrString *infoStack = NULL_PNTRSTRING; /* UNDO/REDO command info */
  static long stackSize = DEFAULT_UNDO_STACK_SIZE; /* Change w/ SET UNDO */
  static long stackEnd = -1;
  static long stackPtr = -1;
  static flag firstTime = 1;
  static flag stackOverflowed = 0; /* For user msg and prf chg determination */
  static flag stackAborted = 0;  /* For proof changed determination */
  long i;

  if (stackPtr < -1 || stackPtr > stackEnd || stackPtr > stackSize - 1
      || stackEnd < -1 || stackEnd > stackSize -1 ) {
    bug(1858);
  }

  if (firstTime == 1) { /* First time ever called */
    firstTime = 0;
    proofStack = malloc((size_t)(stackSize) * sizeof(struct pip_struct));
    if (!proofStack) bug(1859);
    for (i = 0; i < stackSize; i++) { /* Set to empty proofs */
      proofStack[i].proof = NULL_NMBRSTRING;
      proofStack[i].target = NULL_PNTRSTRING;
      proofStack[i].source = NULL_PNTRSTRING;
      proofStack[i].user = NULL_PNTRSTRING;
    }
    pntrLet(&infoStack, pntrSpace(stackSize)); /* Set to empty vstrings */
  }

  if (!proofStack) bug(1860);

  switch (action) {
    case PUS_GET_SIZE:
    case PUS_GET_STATUS:
      break;  /* Do nothing; just return stack size */

    case PUS_INIT:
    case PUS_NEW_SIZE:
      /* Deallocate old contents */
      for (i = 0; i <= stackEnd; i++) {
        deallocProofStruct(&(proofStack[i]));
        let((vstring *)(&(infoStack[i])), "");
      }

      /* If UNDOs weren't exhausted and thus are abandoned due to size change,
         this flag will prevent the program from falsely thinking the proof
         hasn't changed */
      if (action == PUS_NEW_SIZE) {
        if (stackPtr > 0) {
          print2("The previous UNDOs are no longer available.\n");
          stackAborted = 1;
        }
        /* Since we're going to reset stackOverflowed, save its state
           in stackAborted so we don't falsely exit MM-PA without saving */
        if (stackOverflowed) stackAborted = 1;
      }

      stackEnd = -1; /* Nothing in UNDO stack now */
      stackPtr = -1;
      stackOverflowed = 0;

      if (action == PUS_INIT) {
        stackAborted = 0;
        break;
      }

      /* Re-size the stack */
      /* Free the old stack (pntrLet() below will free old infoStack) */
      free(proofStack);
      /* Reinitialize new stack */
      stackSize = newSize + 1;
      if (stackSize < 1) bug(1867);
      proofStack = malloc((size_t)(stackSize) * sizeof(struct pip_struct));
      if (!proofStack) bug(1861);
      for (i = 0; i < stackSize; i++) { /* Set to empty proofs */
        proofStack[i].proof = NULL_NMBRSTRING;
        proofStack[i].target = NULL_PNTRSTRING;
        proofStack[i].source = NULL_PNTRSTRING;
        proofStack[i].user = NULL_PNTRSTRING;
      }
      pntrLet(&infoStack, pntrSpace(stackSize)); /* Set to empty vstrings */
      break;

    case PUS_PUSH:
      /* Warning: PUS_PUSH must be called upon entering Proof Assistant to put
         the original proof into stack locaton 0.  It also must be
         called after PUS_NEW_SIZE if inside of MM-PA. */

      /* Any new command after UNDO should erase the REDO part */
      if (stackPtr < stackEnd) {
        for (i = stackPtr + 1; i <= stackEnd; i++) {
          deallocProofStruct(&(proofStack[i]));
          let((vstring *)(&(infoStack[i])), "");
        }
        stackEnd = stackPtr;
      }

      /* If the stack is full, deallocate bottom of stack and move things
         down to make room for new stack entry */
      if (stackPtr == stackSize - 1) {
        stackOverflowed = 1; /* To  modify user message if UNDO exhausted */
        deallocProofStruct(&(proofStack[0])); /* Deallocate the bottom entry */
        let((vstring *)(&(infoStack[0])), "");
        for (i = 0; i < stackSize - 1; i++) {
          /* Instead of
               "copyProofStruct(&(proofStack[i]), proofStack[i + 1]);
            (which involves de/reallocation), copy the pointers directly
            for improved speed */
          proofStack[i].proof = proofStack[i + 1].proof;
          proofStack[i].target = proofStack[i + 1].target;
          proofStack[i].source = proofStack[i + 1].source;
          proofStack[i].user = proofStack[i + 1].user;
          infoStack[i] = infoStack[i + 1];
        }
        /* Now initialize the top of the stack pointers (don't deallocate since
           its old contents are pointed to by the next one down) */
        proofStack[stackPtr].proof = NULL_NMBRSTRING;
        proofStack[stackPtr].target = NULL_PNTRSTRING;
        proofStack[stackPtr].source = NULL_PNTRSTRING;
        proofStack[stackPtr].user = NULL_PNTRSTRING;
        infoStack[stackPtr] = "";
        stackPtr--;
        stackEnd--;
        if (stackPtr != stackSize - 2 || stackPtr != stackEnd) bug(1862);
      }

      /* Add the new command to the stack */
      stackPtr++;
      stackEnd++;
      if (stackPtr != stackEnd) bug(1863);
      copyProofStruct(&(proofStack[stackPtr]), *proofStruct);
      let((vstring *)(&(infoStack[stackPtr])), info);
      break;

    case PUS_UNDO:
      if (stackPtr < 0) bug(1864); /* A first PUSH wasn't called upon entry to
                   Proof Assistant (MM-PA) */
      if (stackPtr == 0) {
        if (stackOverflowed == 0) {
          print2("There is nothing to undo.\n");
        } else {
          printLongLine(cat("Exceeded maximum of ", str((double)stackSize - 1),
              " UNDOs.  To increase the number, see HELP SET UNDO.",
              NULL), "", " ");
        }
        break;
      }

      /* Print the Undid message for the most recent action */
      printLongLine(cat("Undid:  ", infoStack[stackPtr],
              NULL), "", " ");
      stackPtr--;
      /* Restore the version of the proof before that action */
      copyProofStruct(&(*proofStruct), proofStack[stackPtr]);
      break;

    case PUS_REDO:
      if (stackPtr == stackEnd) {
        print2("There is nothing more to redo.\n");
        break;
      }

      /* Move up stack pointer and return its entry. */
      stackPtr++;
      /* Restore the last undo and print the message for its action */
      copyProofStruct(&(*proofStruct), proofStack[stackPtr]);
      printLongLine(cat("Redid:  ", infoStack[stackPtr],
              NULL), "", " ");
      break;

    default:
      bug(1865);
  } /* end switch(action) */

  if (stackPtr < -1 || stackPtr > stackEnd || stackPtr > stackSize - 1
      || stackEnd < -1 || stackEnd > stackSize -1 ) {
    bug(1866);
  }

  if (action == PUS_GET_STATUS) {
    /* Return the OR of all conditions which might indicate that the
       proof has changed, so that it may not be safe to exit MM-PA without
       a warning to save the proof */
    return (stackOverflowed || stackAborted || stackPtr != 0);
  } else {
    return stackSize - 1;
  }
} /* processUndoStack */
