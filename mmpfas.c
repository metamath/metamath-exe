/*****************************************************************************/
/*               Copyright (C) 1999, NORMAN D. MEGILL                        */
/*****************************************************************************/

/*34567890123456 (79-character line to adjust text window width) 678901234567*/

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

long proveStatement = 0; /* The statement to be proved */
flag proofChangedFlag; /* Flag to push 'undo' stack */

long userMaxProveFloat = 10000; /* Upper limit for proveFloating */

long dummyVars = 0; /* Total number of dummy variables declared */
long pipDummyVars = 0; /* Number of dummy variables used by proof in progress */

/* Structure for holding a proof in progress. */
/* This structure should be deallocated after use. */
struct pip_struct proofInProgress = {
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

  printLongLine(cat("Step ", str(step + 1), ":  ", nmbrCvtMToVString(
      proofInProgress.target[step]), NULL), "  ", " ");
  if (nmbrLen(proofInProgress.user[step])) {
    printLongLine(cat("Step ", str(step + 1), "(user):  ", nmbrCvtMToVString(
        proofInProgress.user[step]), NULL), "  ", " ");
  }
  /* Allocate a flag for each step to be tested */
  /* 1 means no match, 2 means match */
  let(&matchFlags, string(proveStatement, 1));
  /* 1 means no timeout, 2 means timeout */
  let(&timeoutFlags, string(proveStatement, 1));
  for (stmt = 1; stmt < proveStatement; stmt++) {
    if (statement[stmt].type != (char)e__ &&
        statement[stmt].type != (char)f__ &&
        statement[stmt].type != (char)a__ &&
        statement[stmt].type != (char)p__) continue;

    /* See if the maximum number of requested essential hypotheses is
       exceeded */
    if (maxEssential != -1) {
      essHypCount = 0;
      for (hyp = 0; hyp < statement[stmt].numReqHyp; hyp++) {
        if (statement[statement[stmt].reqHypList[hyp]].type == (char)e__) {
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
    let(&tmpStr1, cat("There are ", str(matchCount), " matches for step ",
      str(step + 1), ".  View them (Y, N) <N>? ", NULL));
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
  for (stmt = 1; stmt < proveStatement; stmt++) {
    if (matchFlags[stmt] == 2) {
      matchList[matchListPos] = stmt;
      matchListPos++;
    }
  }

  nmbrLet(&timeoutList, nmbrSpace(timeoutCount));
  timeoutListPos = 0;
  for (stmt = 1; stmt < proveStatement; stmt++) {
    if (timeoutFlags[stmt] == 2) {
      timeoutList[timeoutListPos] = stmt;
      timeoutListPos++;
    }
  }

  let(&tmpStr1, nmbrCvtRToVString(matchList));
  let(&tmpStr4, nmbrCvtRToVString(timeoutList));

  printLongLine(cat("Step ", str(step + 1), " matches statements:  ", tmpStr1,
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
        statement[stmt].labelName);
  } else {

    while (1) {
      let(&tmpStr3, cat("What statement to select for step ", str(step + 1),
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
      if (!strcmp(tmpStr2, statement[matchList[matchListPos]].labelName)) break;
    }
    if (matchListPos < matchCount) {
      stmt = matchList[matchListPos];
    } else {
      for (timeoutListPos = 0; timeoutListPos < timeoutCount;
          timeoutListPos++) {
      if (!strcmp(tmpStr2, statement[timeoutList[timeoutListPos]].labelName))
          break;
      } /* Next timeoutListPos */
      if (timeoutListPos == timeoutCount) bug(1801);
      stmt = timeoutList[timeoutListPos];
    }
    print2("Step %ld was assigned statement %s.\n",
        (long)(step + 1),
        statement[stmt].labelName);

  } /* End if matchCount == 1 */

  /* Add to statement to the proof */
  assignStatement(matchList[matchListPos], step);
  proofChangedFlag = 1; /* Flag for 'undo' stack */

  let(&tmpStr1, "");
  let(&tmpStr4, "");
  let(&tmpStr2, "");
  let(&matchFlags, "");
  let(&timeoutFlags, "");
  nmbrLet(&matchList, NULL_NMBRSTRING);
  nmbrLet(&timeoutList, NULL_NMBRSTRING);
  return;

}



/* Assign a statement to an unknown proof step */
void assignStatement(long statemNum, long step)
{
  long hyp;
  nmbrString *hypList = NULL_NMBRSTRING;

  if (proofInProgress.proof[step] != -(long)'?') bug(1802);

  /* Add the statement to the proof */
  nmbrLet(&hypList, nmbrSpace(statement[statemNum].numReqHyp + 1));
  for (hyp = 0; hyp < statement[statemNum].numReqHyp; hyp++) {
    /* A hypothesis of the added statement */
    hypList[hyp] = -(long)'?';
  }
  hypList[statement[statemNum].numReqHyp] = statemNum; /* The added statement */
  addSubProof(hypList, step);
  initStep(step + statement[statemNum].numReqHyp);
  nmbrLet(&hypList, NULL_NMBRSTRING);
  return;
}


/* Replace a known step with another statement.  Returns 0-length proof
   if an exact hypothesis match wasn't found, otherwise returns a subproof
   starting at the replaced step, with the step replaced by
   replStatemNum. */
nmbrString *replaceStatement(long replStatemNum, long step, long provStmtNum)
{
  nmbrString *mString; /* Pointer only */
  long reqHyps;
  long hyp, sym, var, i, j, substep;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *scheme = NULL_NMBRSTRING;
  pntrString *hypList = NULL_PNTRSTRING;
  nmbrString *hypListMap = NULL_NMBRSTRING; /* Order remapping for speedup */
  pntrString *hypProofList = NULL_PNTRSTRING;
  pntrString *stateVector = NULL_PNTRSTRING;
  nmbrString *stmtMathPtr;
  nmbrString *hypSchemePtr;
  nmbrString *hypProofPtr;
  nmbrString *makeSubstPtr;
  pntrString *hypMakeSubstList = NULL_PNTRSTRING;
  pntrString *hypStateVectorList = NULL_PNTRSTRING;
  vstring hypReEntryFlagList = "";
  nmbrString *hypStepList = NULL_NMBRSTRING;
  flag reEntryFlag;
  flag tmpFlag;
  flag breakFlag;
  flag substepBreakFlag;
  long schemeLen, mStringLen, schemeVars, schReqHyps, hypLen, reqVars,
      schEHyps, subPfLen;
  long saveUnifTrialCount;
  flag reenterFFlag;
  flag dummyVarFlag; /* Flag that replacement hypothesis under consideration has
                        dummy variables */
  nmbrString *hypTestPtr; /* Points to what we are testing hyp. against */
  flag hypOrSubproofFlag; /* 0 means testing against hyp., 1 against subproof*/

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  substep = 0;


  mString = proofInProgress.target[step];
  mStringLen = nmbrLen(mString);

  /* Get length of the existing subproof at the replacement step.  The
     existing subproof will be scanned to match the $e hypotheses of the
     replacement statement.  */
  subPfLen = subProofLen(proofInProgress.proof, step);


  if (statement[replStatemNum].type != (char)a__ &&
      statement[replStatemNum].type != (char)p__)
    bug(1822); /* Not $a or $p */
  stmtMathPtr = statement[replStatemNum].mathString;

  schemeLen = nmbrLen(stmtMathPtr);

  schReqHyps = statement[replStatemNum].numReqHyp;
  reqVars = nmbrLen(statement[replStatemNum].reqVarList);

  /* Change all variables in the statement to dummy vars for unification */
  nmbrLet(&scheme, stmtMathPtr);
  schemeVars = reqVars;
  if (schemeVars + pipDummyVars > dummyVars) {
    /* Declare more dummy vars if necessary */
    declareDummyVars(schemeVars + pipDummyVars - dummyVars);
  }
  for (var = 0; var < schemeVars; var++) {
    /* Put dummy var mapping into mathToken[].tmp field */
    mathToken[statement[replStatemNum].reqVarList[var]].tmp = mathTokens + 1 +
        pipDummyVars + var;
  }
  for (sym = 0; sym < schemeLen; sym++) {
    if (mathToken[stmtMathPtr[sym]].tokenType != (char)var__) continue;
    /* Use dummy var mapping from mathToken[].tmp field */
    scheme[sym] = mathToken[stmtMathPtr[sym]].tmp;
  }

  /* Change all variables in the statement's hyps to dummy vars for subst. */
  pntrLet(&hypList, pntrNSpace(schReqHyps));
  nmbrLet(&hypListMap, nmbrSpace(schReqHyps));
  pntrLet(&hypProofList, pntrNSpace(schReqHyps));

  for (hyp = 0; hyp < schReqHyps; hyp++) {
    hypSchemePtr = NULL_NMBRSTRING;
    nmbrLet(&hypSchemePtr,
      statement[statement[replStatemNum].reqHypList[hyp]].mathString);
    hypLen = nmbrLen(hypSchemePtr);
    for (sym = 0; sym < hypLen; sym++) {
      if (mathToken[hypSchemePtr[sym]].tokenType
          != (char)var__) continue;
      /* Use dummy var mapping from mathToken[].tmp field */
      hypSchemePtr[sym] = mathToken[hypSchemePtr[sym]].tmp;
    }
    hypList[hyp] = hypSchemePtr;
    hypListMap[hyp] = hyp;
  }

  /* Move all $e's to front of hypothesis list */
  schEHyps = 0;
  for (hyp = 0; hyp < schReqHyps; hyp++) {
    if (statement[statement[replStatemNum].reqHypList[hypListMap[hyp]]].type
         == (char)e__) {
      j = hypListMap[hyp];
      hypListMap[hyp] = hypListMap[schEHyps];
      hypListMap[schEHyps] = j;
      schEHyps++;
    }
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


  unifTrialCount = 1; /* Reset unification timeout */
  reEntryFlag = 0; /* For unifyH() */

  /* Number of required hypotheses of statement we're proving */
  reqHyps = statement[provStmtNum].numReqHyp;

  while (1) { /* Try all possible unifications */
    tmpFlag = unifyH(scheme, mString, &stateVector, reEntryFlag);
    if (!tmpFlag) break; /* (Next) unification not possible */
    if (tmpFlag == 2) {
      print2(
"Unification timed out.  SET UNIFICATION_TIMEOUT larger for better results.\n");
      unifTrialCount = 1; /* Reset unification timeout */
      break; /* Treat timeout as if unification not possible */
    }

    reEntryFlag = 1; /* For next unifyH() */

    /* Make substitutions into each hypothesis, and try to prove that
       hypothesis */
    nmbrLet(&proof, NULL_NMBRSTRING);
    breakFlag = 0;
    for (hyp = 0; hyp < schReqHyps; hyp++) {

      /* Make substitutions from replacement statement's stateVector */
      nmbrLet((nmbrString **)(&(hypMakeSubstList[hypListMap[hyp]])),
          NULL_NMBRSTRING); /* Deallocate previous pass if any */
      hypMakeSubstList[hypListMap[hyp]] =
          makeSubstUnif(&dummyVarFlag, hypList[hypListMap[hyp]],
          stateVector);

      /* Make substitutions from each earlier hypothesis unification */
      for (i = 0; i < hyp; i++) {
        /* Only do substitutions for $e's -- the $f's will have no
           dummy vars., and they may not even have a stateVector
           since they may have been found with proveFloating below */
        if (i >= schEHyps) break;

        makeSubstPtr = makeSubstUnif(&dummyVarFlag,
            hypMakeSubstList[hypListMap[hyp]],
            hypStateVectorList[hypListMap[i]]);
        nmbrLet((nmbrString **)(&(hypMakeSubstList[hypListMap[hyp]])),
            NULL_NMBRSTRING);
        hypMakeSubstList[hypListMap[hyp]] = makeSubstPtr;
      }

      if (hyp < schEHyps) {
        /* It's a $e hypothesis */
        if (statement[statement[replStatemNum].reqHypList[hypListMap[hyp]]
            ].type != (char)e__) bug (1823);
      } else {
        /* It's a $f hypothesis */
        if (statement[statement[replStatemNum].reqHypList[hypListMap[hyp]]
             ].type != (char)f__) bug(1824);
        /* At this point there should be no dummy variables in $f
           hypotheses */
        if (dummyVarFlag) bug(1825);
      }


      /* Scan all known steps of existing subproof to find a hypothesis
         match */
      substepBreakFlag = 0;
      reenterFFlag = 0;
      if (hypReEntryFlagList[hypListMap[hyp]] == 2) {
        /* Reentry flag is set; we're continuing with a previously unified
           subproof step */
        substep = hypStepList[hypListMap[hyp]];

        /* If we are re-entering the unification for a $f, it means we
           backtracked from a later failure, and there won't be another
           unification possible.  In this case we should bypass the
           proveFloating call to force a further backtrack.  (Otherwise
           we will have an infinite loop.)  Note that for $f's, all
           variables will be known so there will only be one unification
           anyway. */
        if (hyp >= schEHyps) reenterFFlag = 1;

      } else {
        if (hypReEntryFlagList[hypListMap[hyp]] == 1) {
          /* Start at the beginning of the subproof */
          substep = step - subPfLen + 1;
          /* Later enhancement:  start at required hypotheses */
          /* (Here we use the trick of shifting down the starting
              substep to below the real subproof start) */
          substep = substep - reqHyps;
        } else {
          if (hypReEntryFlagList[hypListMap[hyp]] == 3) {
            /* This is the case where proveFloating previously found a
               proof for the step, and we've backtracked.  In this case,
               we want to backtrack further - no scan or proveFloating
               call again. */
            hypReEntryFlagList[hypListMap[hyp]] = 1;
            reenterFFlag = 1; /* Skip proveFloating call */
            substep = step; /* Skip loop */
          } else {
            bug(1826);
          }
        }
      }
      for (substep = substep; substep < step; substep++) {

        if (substep < step - subPfLen + 1) {
          /* We're scanning required hypotheses */
          hypOrSubproofFlag = 0;
          /* Point to what we are testing hyp. against */
          /* (Note offset to substep needed to compensate for trick) */
          hypTestPtr =
              statement[statement[provStmtNum].reqHypList[
              substep - (step - subPfLen + 1 - reqHyps)]].mathString;
        } else {
          /* We're scanning the subproof */
          hypOrSubproofFlag = 1;
          /* Point to what we are testing hyp. against */
          hypTestPtr = proofInProgress.target[substep];

          /* Do not consider unknown subproof steps or those with
             unknown variables */
          j = 0; /* Break flag */
          i = nmbrLen(hypTestPtr);
          if (i == 0) bug(1824); /* Shouldn't be empty */
          for (i = i - 1; i >= 0; i--) {
            if (((nmbrString *)hypTestPtr)[i] >
                mathTokens) {
              j = 1;
              break;
            }
          }
          if (j) continue;  /* Subproof step has dummy var.; don't use it */

        }

        /* Speedup - skip if no dummy vars in hyp and statements not equal */
        if (!dummyVarFlag) {
          if (!nmbrEq(hypTestPtr, hypMakeSubstList[hypListMap[hyp]])) {
            continue;
          }
        }

        /* Speedup - skip if 1st symbols are constants and don't match */
        /* First symbol */
        i = hypTestPtr[0];
        j = ((nmbrString *)(hypMakeSubstList[hypListMap[hyp]]))[0];
        if (mathToken[i].tokenType == (char)con__) {
          if (mathToken[j].tokenType == (char)con__) {
            if (i != j) continue;
          }
        }

        /* unifTrialCount = 1; */ /* ??Don't reset it here in order to
           detect exponential blowup in hypotheses trials */
        unifTrialCount = 1; /* Reset unification timeout counter */
        tmpFlag = unifyH(hypMakeSubstList[hypListMap[hyp]],
            hypTestPtr,
            (pntrString **)(&(hypStateVectorList[hypListMap[hyp]])),
            /* (Remember: 1 = false, 2 = true in hypReEntryFlagList) */
            hypReEntryFlagList[hypListMap[hyp]] - 1);
        if (!tmpFlag || tmpFlag == 2) {
          /* (Next) unification not possible or timeout */
          if (tmpFlag == 2) {
            print2(
"Unification timed out.  SET UNIFICATION_TIMEOUT larger for better results.\n");
            unifTrialCount = 1; /* Reset unification timeout */
            /* Deallocate unification state vector */
            purgeStateVector(
                (pntrString **)(&(hypStateVectorList[hypListMap[hyp]])));
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
            if (hypReEntryFlagList[hypListMap[hyp]] - 1 == 1) {
              /* There are no dummy variables, so previous match
                 was exact.  Force the substep loop to terminate as
                 if nothing further was found.  (If we don't do this,
                 there could be, say 50 more matches for "var x",
                 so we might need a factor of 50 greater runtime for each
                 replacement hypothesis having this situation.) */
              substep = step - 1; /* Make this the last loop pass */
            }
          }

          hypReEntryFlagList[hypListMap[hyp]] = 1;
          continue;
        } else {

          /* tmpFlag = 1:  a unification was found */
          if (tmpFlag != 1) bug(1825);

          /* (Speedup) */
          /* If this subproof step has previously occurred in a hypothesis
             or an earlier subproof step, don't consider it since that
             would be redundant. */
          if (hypReEntryFlagList[hypListMap[hyp]] == 1
              /* && 0 */  /* To skip this for testing */
              ) {
            j = 0; /* Break flag */
            for (i = step - subPfLen + 1 - reqHyps; i < substep; i++) {
              if (i < step - subPfLen + 1) {
                /* A required hypothesis */
                if (nmbrEq(hypTestPtr,
                    statement[statement[provStmtNum].reqHypList[
                    i - (step - subPfLen + 1 - reqHyps)]].mathString)) {
                  j = 1;
                  break;
                }
              } else {
                /* A subproof step */
                if (nmbrEq(hypTestPtr,
                    proofInProgress.target[i])) {
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
                  (pntrString **)(&(hypStateVectorList[hypListMap[hyp]])));
              continue;
            }
          } /* end if not reentry */
          /* (End speedup) */


          hypReEntryFlagList[hypListMap[hyp]] = 2; /* For next unifyH() */
          hypStepList[hypListMap[hyp]] = substep;

          if (!hypOrSubproofFlag) {
            /* We're scanning required hypotheses */
            nmbrLet((nmbrString **)(&hypProofList[hypListMap[hyp]]),
                nmbrAddElement(NULL_NMBRSTRING,
                statement[provStmtNum].reqHypList[
                substep - (step - subPfLen + 1 - reqHyps)]));
          } else {
            /* We're scanning the subproof */
            i = subProofLen(proofInProgress.proof, substep);
            nmbrLet((nmbrString **)(&hypProofList[hypListMap[hyp]]),
                nmbrSeg(proofInProgress.proof, substep - i + 2, substep + 1));
          }

          substepBreakFlag = 1;
          break;
        } /* end if (!tmpFlag || tmpFlag = 2) */
      } /* next substep */

      if (!substepBreakFlag) {
        /* There was no (completely known) step in the subproof that
           matched the hypothesis.  If it's a $f hypothesis, we will try
           to prove it by itself. */
        /* (However, if this is 2nd pass of backtrack, i.e. reenterFFlag is
           set, we already got an exact $f match earlier and don't need this
           scan, and shouldn't do it to prevent inf. loop.) */
        if (hyp >= schEHyps && !reenterFFlag) {
          /* It's a $f hypothesis */
          saveUnifTrialCount = unifTrialCount; /* Save unification timeout */
          hypProofPtr =
              proveFloating(hypMakeSubstList[hypListMap[hyp]],
              provStmtNum, 0, step);
          unifTrialCount = saveUnifTrialCount; /* Restore unif. timeout */
          if (nmbrLen(hypProofPtr)) {
            nmbrLet((nmbrString **)(&hypProofList[hypListMap[hyp]]),
                NULL_NMBRSTRING);
            hypProofList[hypListMap[hyp]] = hypProofPtr;
            substepBreakFlag = 1;
            hypReEntryFlagList[hypListMap[hyp]] = 3;
          }
        } /* end if $f */
      }

      if (!substepBreakFlag) {
        /* We must backtrack */
        if (hyp == 0) {
          /* No more possibilities to try */
          breakFlag = 1;
          break;
        }
        hyp = hyp - 2; /* Go back one interation (subtract 2 to offset
                          end of loop increment */
      }
    } /* next hyp */


    if (breakFlag) {
      /* Proof was not found for some hypothesis. */
      continue; /* Get next unification */
    } /* End if breakFlag */

    /* Proofs were found for all hypotheses */

    /* Build the proof */
    for (hyp = 0; hyp < schReqHyps; hyp++) {
      nmbrLet(&proof, nmbrCat(proof, hypProofList[hyp], NULL));
    }
    nmbrLet(&proof, nmbrAddElement(proof, replStatemNum)); /* Complete the proof */

    /* Deallocate hypothesis schemes and proofs */
    for (hyp = 0; hyp < schReqHyps; hyp++) {
      nmbrLet((nmbrString **)(&hypList[hyp]), NULL_NMBRSTRING);
      nmbrLet((nmbrString **)(&hypProofList[hyp]), NULL_NMBRSTRING);
    }
    goto returnPoint;

  } /* End while (next unifyH() call for main replacement statement) */

  /* Deallocate hypothesis schemes and proofs */
  for (hyp = 0; hyp < schReqHyps; hyp++) {
    nmbrLet((nmbrString **)(&hypList[hyp]), NULL_NMBRSTRING);
    nmbrLet((nmbrString **)(&hypProofList[hyp]), NULL_NMBRSTRING);
    nmbrLet((nmbrString **)(&hypMakeSubstList[hyp]), NULL_NMBRSTRING);
    purgeStateVector((pntrString **)(&(hypStateVectorList[hyp])));
  }

  nmbrLet(&proof, NULL_NMBRSTRING);  /* Proof not possible */

 returnPoint:
  /* Deallocate unification state vector */
  purgeStateVector(&stateVector);

  nmbrLet(&scheme, NULL_NMBRSTRING);
  pntrLet(&hypList, NULL_PNTRSTRING);
  nmbrLet(&hypListMap, NULL_NMBRSTRING);
  pntrLet(&hypProofList, NULL_PNTRSTRING);
  pntrLet(&hypMakeSubstList, NULL_PNTRSTRING);
  pntrLet(&hypStateVectorList, NULL_PNTRSTRING);
  let(&hypReEntryFlagList, "");
  nmbrLet(&hypStepList, NULL_NMBRSTRING);


/*E*/if(db8)print2("%s\n", cat("Returned: ",
/*E*/   nmbrCvtRToVString(proof), NULL));
  return (proof); /* Caller must deallocate */
}


/* Add a subproof in place of an unknown step to proofInProgress.  The
   .target, .source, and .user fields are initialized to empty (except
   .target and .user of the deleted unknown step are retained). */
void addSubProof(nmbrString *subProof, long step) {
  long sbPfLen;

  if (proofInProgress.proof[step] != -(long)'?') bug(1803);
                     /* Only unknown steps should be allowed at cmd interface */
  sbPfLen = nmbrLen(subProof);
  nmbrLet(&proofInProgress.proof, nmbrCat(nmbrLeft(proofInProgress.proof, step),
      subProof, nmbrRight(proofInProgress.proof, step + 2), NULL));
  pntrLet(&proofInProgress.target, pntrCat(pntrLeft(proofInProgress.target,
      step), pntrNSpace(sbPfLen - 1), pntrRight(proofInProgress.target,
      step + 1), NULL));
  /* Deallocate .source in case not empty (if not, though, it's a bug) */
  if (nmbrLen(proofInProgress.source[step])) bug(1804);
 /*nmbrLet((nmbrString **)(&(proofInProgress.source[step])), NULL_NMBRSTRING);*/
  pntrLet(&proofInProgress.source, pntrCat(pntrLeft(proofInProgress.source,
      step), pntrNSpace(sbPfLen - 1), pntrRight(proofInProgress.source,
      step + 1), NULL));
  pntrLet(&proofInProgress.user, pntrCat(pntrLeft(proofInProgress.user,
      step), pntrNSpace(sbPfLen - 1), pntrRight(proofInProgress.user,
      step + 1), NULL));
}

/* Delete a subproof starting (in reverse from) step.  The step is replaced
   with an unknown step, and its .target and .user fields are retained. */
void deleteSubProof(long step) {
  long sbPfLen, pos;

  if (proofInProgress.proof[step] == -(long)'?') bug (1805);
                       /* Unknown step should not be allowed at cmd interface */
  sbPfLen = subProofLen(proofInProgress.proof, step);
  nmbrLet(&proofInProgress.proof, nmbrCat(nmbrAddElement(
      nmbrLeft(proofInProgress.proof, step - sbPfLen + 1), -(long)'?'),
      nmbrRight(proofInProgress.proof, step + 2), NULL));
  for (pos = step - sbPfLen + 1; pos <= step; pos++) {
    if (pos < step) {
      /* Deallocate .target and .user */
      nmbrLet((nmbrString **)(&(proofInProgress.target[pos])), NULL_NMBRSTRING);
      nmbrLet((nmbrString **)(&(proofInProgress.user[pos])), NULL_NMBRSTRING);
    }
    /* Deallocate .source */
    nmbrLet((nmbrString **)(&(proofInProgress.source[pos])), NULL_NMBRSTRING);
  }
  pntrLet(&proofInProgress.target, pntrCat(pntrLeft(proofInProgress.target,
      step - sbPfLen + 1), pntrRight(proofInProgress.target,
      step + 1), NULL));
  pntrLet(&proofInProgress.source, pntrCat(pntrLeft(proofInProgress.source,
      step - sbPfLen + 1), pntrRight(proofInProgress.source,
      step + 1), NULL));
  pntrLet(&proofInProgress.user, pntrCat(pntrLeft(proofInProgress.user,
      step - sbPfLen + 1), pntrRight(proofInProgress.user,
      step + 1), NULL));
}


/* Check to see if a statement will match the proofInProgress.target (or .user)
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

  /* This is no longer a bug.  (Could be true for REPLACE command.)
  if (proofInProgress.proof[step] != -(long)'?') bug(1806);
  */

  targetLen = nmbrLen(proofInProgress.target[step]);
  if (!targetLen) bug(1807);

  /* If the statement is a hypothesis, just see if it unifies. */
  if (statement[statemNum].type == (char)e__ || statement[statemNum].type ==
      (char)f__) {

    /* Make sure it's a hypothesis of the statement being proved */
    breakFlag = 0;
    numHyps = statement[proveStatement].numReqHyp;
    for (hyp = 0; hyp < numHyps; hyp++) {
      if (statement[proveStatement].reqHypList[hyp] == statemNum) {
        breakFlag = 1;
        break;
      }
    }
    if (!breakFlag) { /* Not a required hypothesis; is it optional? */
      numHyps = nmbrLen(statement[proveStatement].optHypList);
      for (hyp = 0; hyp < numHyps; hyp++) {
        if (statement[proveStatement].optHypList[hyp] == statemNum) {
          breakFlag = 1;
          break;
        }
      }
      if (!breakFlag) { /* Not a hypothesis of statement being proved */
        targetFlag = 0;
        goto returnPoint;
      }
    }

    unifTrialCount = 1; /* Reset unification timeout */
    targetFlag = unifyH(proofInProgress.target[step],
        statement[statemNum].mathString, &stateVector, 0);
   if (nmbrLen(proofInProgress.user[step])) {
      unifTrialCount = 1; /* Reset unification timeout */
      userFlag = unifyH(proofInProgress.user[step],
        statement[statemNum].mathString, &stateVector, 0);
    }
    goto returnPoint;
  }

  mString = statement[statemNum].mathString;
  mStringLen = statement[statemNum].mathStringLen;

  /* For speedup - 1st, 2nd, & last math symbols should match if constants */
  /* (The speedup is only done for .target; the .user is assumed to be
     infrequent.) */
  /* First symbol */
  firstSymbsAreConstsFlag = 0;
  stsym = mString[0];
  tasym = ((nmbrString *)(proofInProgress.target[step]))[0];
  if (mathToken[stsym].tokenType == (char)con__) {
    if (mathToken[tasym].tokenType == (char)con__) {
      firstSymbsAreConstsFlag = 1; /* The first symbols are constants */
      if (stsym != tasym) {
        targetFlag = 0;
        goto returnPoint;
      }
    }
  }
  /* Last symbol */
  stsym = mString[mStringLen - 1];
  tasym = ((nmbrString *)(proofInProgress.target[step]))[targetLen - 1];
  if (stsym != tasym) {
    if (mathToken[stsym].tokenType == (char)con__) {
      if (mathToken[tasym].tokenType == (char)con__) {
        targetFlag = 0;
        goto returnPoint;
      }
    }
  }
  /* Second symbol */
  if (targetLen > 1 && mStringLen > 1 && firstSymbsAreConstsFlag) {
    stsym = mString[1];
    tasym = ((nmbrString *)(proofInProgress.target[step]))[1];
    if (stsym != tasym) {
      if (mathToken[stsym].tokenType == (char)con__) {
        if (mathToken[tasym].tokenType == (char)con__) {
          targetFlag = 0;
          goto returnPoint;
        }
      }
    }
  }

  /* Change variables in statement to dummy variables for unification */
  nmbrLet(&scheme, mString);
  reqVars = nmbrLen(statement[statemNum].reqVarList);
  if (reqVars + pipDummyVars > dummyVars) {
    /* Declare more dummy vars if necessary */
    declareDummyVars(reqVars + pipDummyVars - dummyVars);
  }
  for (var = 0; var < reqVars; var++) {
    /* Put dummy var mapping into mathToken[].tmp field */
    mathToken[statement[statemNum].reqVarList[var]].tmp = mathTokens + 1 +
        pipDummyVars + var;
  }
  for (sym = 0; sym < mStringLen; sym++) {
    if (mathToken[scheme[sym]].tokenType != (char)var__)
        continue;
    /* Use dummy var mapping from mathToken[].tmp field */
    scheme[sym] = mathToken[scheme[sym]].tmp;
  }

  /* Now see if we can unify */
  unifTrialCount = 1; /* Reset unification timeout */
  targetFlag = unifyH(proofInProgress.target[step],
      scheme, &stateVector, 0);
  if (nmbrLen(proofInProgress.user[step])) {
    unifTrialCount = 1; /* Reset unification timeout */
    userFlag = unifyH(proofInProgress.user[step],
      scheme, &stateVector, 0);
  }

 returnPoint:
  nmbrLet(&scheme, NULL_NMBRSTRING);
  purgeStateVector(&stateVector);

  if (!targetFlag || !userFlag) return (0);
  if (targetFlag == 1 && userFlag == 1) return (1);
  return (2);

}

/* Check to see if a (user-specified) math string will match the
   proofInProgress.target (or .user) of an step.  Returns 1 if match, 0 if
   not, 2 if unification timed out. */
char checkMStringMatch(nmbrString *mString, long step)
{
  pntrString *stateVector = NULL_PNTRSTRING;
  char targetFlag;
  char sourceFlag = 1; /* Default if no .source */

  unifTrialCount = 1; /* Reset unification timeout */
  targetFlag = unifyH(mString, proofInProgress.target[step],
      &stateVector, 0);
  if (nmbrLen(proofInProgress.source[step])) {
    unifTrialCount = 1; /* Reset unification timeout */
    sourceFlag = unifyH(mString, proofInProgress.source[step],
        &stateVector, 0);
  }

  purgeStateVector(&stateVector);

  if (!targetFlag || !sourceFlag) return (0);
  if (targetFlag == 1 && sourceFlag == 1) return (1);
  return (2);

}

/* Find proof of formula or simple theorem (no new vars in $e's) */
/* maxEDepth is the maximum depth at which statements with $e hypotheses are
   considered.  A value of 0 means none are considered. */
nmbrString *proveFloating(nmbrString *mString, long statemNum, long maxEDepth,
    long step/*For messages*/)
{

  long reqHyps, optHyps;
  long hyp, stmt, sym, var, i, j;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *scheme = NULL_NMBRSTRING;
  pntrString *hypList = NULL_PNTRSTRING;
  nmbrString *hypListMap = NULL_NMBRSTRING; /* Order remapping for speedup */
  pntrString *hypProofList = NULL_PNTRSTRING;
  pntrString *stateVector = NULL_PNTRSTRING;
  long firstSymbol, secondSymbol, lastSymbol;
  nmbrString *stmtMathPtr;
  nmbrString *hypSchemePtr;
  nmbrString *hypProofPtr;
  nmbrString *makeSubstPtr;
  flag reEntryFlag;
  flag tmpFlag;
  flag breakFlag;
  flag firstEHypFlag;
  long schemeLen, mStringLen, schemeVars, schReqHyps, hypLen, reqVars;
  long saveUnifTrialCount;
  static long depth = 0;
  static long trials;
/*E*/  long unNum;
/*E*/if (db8)print2("%s\n", cat(space(depth+2), "Entered: ",
/*E*/   nmbrCvtMToVString(mString), NULL));

  if (depth == 0) {
    trials = 0; /* Initialize trials */
  } else {
    trials++;
  }
  depth++; /* Update backtracking depth */
  if (trials > userMaxProveFloat) {
    nmbrLet(&proof, NULL_NMBRSTRING);
    print2(
"Exceeded trial limit at step %ld.  You may increase with SET SEARCH_LIMIT.\n",
        (long)(step + 1));
    goto returnPoint;
  }
#define MAX_DEPTH 40  /* > this, infinite loop assumed */ /*???User setting?*/
  if (depth > MAX_DEPTH) {
    nmbrLet(&proof, NULL_NMBRSTRING);
/*??? Document in Metamath manual. */
    printLongLine(cat(
       "?Warning:  A possible infinite loop was found in $f hypothesis ",
       "backtracking (i.e., depth > ", str(MAX_DEPTH),
       ").  The last proof attempt was for math string \"",
       nmbrCvtMToVString(mString),
       "\".  Your axiom system may have an error.", NULL), " ", " ");
    goto returnPoint;
  }


  /* First see if mString matches a required or optional hypothesis; if so,
     we're done; the proof is just the hypothesis. */
  reqHyps = statement[statemNum].numReqHyp;
  for (hyp = 0; hyp < reqHyps; hyp++) {
    if (nmbrEq(mString,
        statement[statement[statemNum].reqHypList[hyp]].mathString)) {
      nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING,
          statement[statemNum].reqHypList[hyp]));
      goto returnPoint;
    }
  }
  optHyps = nmbrLen(statement[statemNum].optHypList);
  for (hyp = 0; hyp < optHyps; hyp++) {
    if (nmbrEq(mString,
        statement[statement[statemNum].optHypList[hyp]].mathString)) {
      nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING,
          statement[statemNum].optHypList[hyp]));
      goto returnPoint;
    }
  }

  /* Scan all statements up to the current statement to see if we can unify */

  mStringLen = nmbrLen(mString);

  /* For speedup */
  firstSymbol = mString[0];
  if (mathToken[firstSymbol].tokenType != (char)con__) firstSymbol = 0;
  if (mStringLen > 1) {
    secondSymbol = mString[1];
    if (mathToken[secondSymbol].tokenType != (char)con__) secondSymbol = 0;
    /* If first symbol is a variable, second symbol shouldn't be tested. */
    if (!firstSymbol) secondSymbol = 0;
  } else {
    secondSymbol = 0;
  }
  lastSymbol = mString[mStringLen - 1];
  if (mathToken[lastSymbol].tokenType != (char)con__) lastSymbol = 0;

  for (stmt = 1; stmt < statemNum; stmt++) {

    if (statement[stmt].type != (char)a__ &&
        statement[stmt].type != (char)p__) continue; /* Not $a or $p */
    stmtMathPtr = statement[stmt].mathString;

    /* Speedup:  if first or last tokens in instance and scheme are constants,
       they must match */
    if (firstSymbol) { /* First symbol in mString is a constant */
      if (firstSymbol != stmtMathPtr[0]) {
        if (mathToken[stmtMathPtr[0]].tokenType == (char)con__) continue;
      }
    }

    schemeLen = nmbrLen(stmtMathPtr);

    /* ...Continuation of speedup */
    if (secondSymbol) { /* Second symbol in mString is a constant */
      if (schemeLen > 1) {
        if (secondSymbol != stmtMathPtr[1]) {
          /* Second symbol should be tested only if 1st symbol is a constant */
          if (mathToken[stmtMathPtr[0]].tokenType == (char)con__) {
            if (mathToken[stmtMathPtr[1]].tokenType == (char)con__)
                continue;
          }
        }
      }
    }
    if (lastSymbol) { /* Last symbol in mString is a constant */
      if (lastSymbol != stmtMathPtr[schemeLen - 1]) {
        if (mathToken[stmtMathPtr[schemeLen - 1]].tokenType ==
           (char)con__) continue;
      }
    }

    schReqHyps = statement[stmt].numReqHyp;
    reqVars = nmbrLen(statement[stmt].reqVarList);

    /* Skip any statements with $e hypotheses based on maxEDepth */
    /* (This prevents exponential growth of backtracking) */
    breakFlag = 0;
    firstEHypFlag = 1;
    for (hyp = 0; hyp < schReqHyps; hyp++) {
      if (statement[statement[stmt].reqHypList[hyp]].type == (char)e__) {
        /* (???Maybe, in the future, we'd want to do this only for depths >
           a small nonzero amount -- specified by global variable) */
        if (depth > maxEDepth) {
          breakFlag = 1;
          break;
        } else {
          /* We should also skip cases where a $e hypothesis has a variable
             not in the assertion. */
          if (firstEHypFlag) { /* This scan is needed only once */
            /* First, set mathToken[].tmp for each required variable */
            for (var = 0; var < reqVars; var++) {
              mathToken[statement[stmt].reqVarList[var]].tmp = 1;
            }
            /* Next, clear mathToken[].tmp for each symbol in scheme */
            for (sym = 0; sym < schemeLen; sym++) {
              mathToken[stmtMathPtr[sym]].tmp = 0;
            }
            /* If any were left over, a $e hyp. has a new variable. */
            for (var = 0; var < reqVars; var++) {
              if (mathToken[statement[stmt].reqVarList[var]].tmp) {
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



    /* Speedup:  make sure all constants in scheme are in instance (i.e.
       mString) */
    /* First, set mathToken[].tmp for all symbols in scheme */
    for (sym = 0; sym < schemeLen; sym++) {
      mathToken[stmtMathPtr[sym]].tmp = 1;
    }
    /* Next, clear mathToken[].tmp for all symbols in instance */
    for (sym = 0; sym < mStringLen; sym++) {
      mathToken[mString[sym]].tmp = 0;
    }
    /* Finally, check that they got cleared for all constants in scheme */
    breakFlag = 0;
    for (sym = 0; sym < schemeLen; sym++) {
      if (mathToken[stmtMathPtr[sym]].tokenType == (char)con__) {
        if (mathToken[stmtMathPtr[sym]].tmp) {
          breakFlag = 1;
          break;
        }
      }
    }
    if (breakFlag) continue;


    /* Change all variables in the statement to dummy vars for unification */
    nmbrLet(&scheme, stmtMathPtr);
    schemeVars = reqVars; /* S.b. same after eliminated new $e vars above */
    if (schemeVars + pipDummyVars > dummyVars) {
      /* Declare more dummy vars if necessary */
      declareDummyVars(schemeVars + pipDummyVars - dummyVars);
    }
    for (var = 0; var < schemeVars; var++) {
      /* Put dummy var mapping into mathToken[].tmp field */
      mathToken[statement[stmt].reqVarList[var]].tmp = mathTokens + 1 +
          pipDummyVars + var;
    }
    for (sym = 0; sym < schemeLen; sym++) {
      if (mathToken[stmtMathPtr[sym]].tokenType != (char)var__) continue;
      /* Use dummy var mapping from mathToken[].tmp field */
      scheme[sym] = mathToken[stmtMathPtr[sym]].tmp;
    }

    /* Change all variables in the statement's hyps to dummy vars for subst. */
    pntrLet(&hypList, pntrNSpace(schReqHyps));
    nmbrLet(&hypListMap, nmbrSpace(schReqHyps));
    pntrLet(&hypProofList, pntrNSpace(schReqHyps));
    for (hyp = 0; hyp < schReqHyps; hyp++) {
      hypSchemePtr = NULL_NMBRSTRING;
      nmbrLet(&hypSchemePtr,
        statement[statement[stmt].reqHypList[hyp]].mathString);
      hypLen = nmbrLen(hypSchemePtr);
      for (sym = 0; sym < hypLen; sym++) {
        if (mathToken[hypSchemePtr[sym]].tokenType
            != (char)var__) continue;
        /* Use dummy var mapping from mathToken[].tmp field */
        hypSchemePtr[sym] = mathToken[hypSchemePtr[sym]].tmp;
      }
      hypList[hyp] = hypSchemePtr;
      hypListMap[hyp] = hyp;
    }

    unifTrialCount = 1; /* Reset unification timeout */
    reEntryFlag = 0; /* For unifyH() */

/*E*/unNum = 0;
    while (1) { /* Try all possible unifications */
      tmpFlag = unifyH(scheme, mString, &stateVector, reEntryFlag);
      if (!tmpFlag) break; /* (Next) unification not possible */
      if (tmpFlag == 2) {
        print2(
"Unification timed out.  SET UNIFICATION_TIMEOUT larger for better results.\n");
        unifTrialCount = 1; /* Reset unification timeout */
        break; /* Treat timeout as if unification not possible */
      }

/*E*/unNum++;
/*E*/if (db8)print2("%s\n", cat(space(depth+2), "Testing unification ",
/*E*/   str(unNum), " statement ", statement[stmt].labelName,
/*E*/   ": ", nmbrCvtMToVString(scheme), NULL));
      reEntryFlag = 1; /* For next unifyH() */

      /* Make substitutions into each hypothesis, and try to prove that
         hypothesis */
      nmbrLet(&proof, NULL_NMBRSTRING);
      breakFlag = 0;
      for (hyp = 0; hyp < schReqHyps; hyp++) {
/*E*/if (db8)print2("%s\n", cat(space(depth+2), "Proving hyp. ",
/*E*/   str(hypListMap[hyp]), "(#", str(hyp), "):  ",
/*E*/   nmbrCvtMToVString(hypList[hypListMap[hyp]]), NULL));
        makeSubstPtr = makeSubstUnif(&tmpFlag, hypList[hypListMap[hyp]],
            stateVector);
        if (tmpFlag) bug(1808); /* No dummy vars. should result unless bad $a's*/
                            /*??? Implement an error check for this in parser */

        saveUnifTrialCount = unifTrialCount; /* Save unification timeout */
        hypProofPtr = proveFloating(makeSubstPtr, statemNum, maxEDepth, step);
        unifTrialCount = saveUnifTrialCount; /* Restore unification timeout */

        nmbrLet(&makeSubstPtr, NULL_NMBRSTRING); /* Deallocate */
        if (!nmbrLen(hypProofPtr)) {
          /* Not possible */
          breakFlag = 1;
          break;
        }

        /* Deallocate in case this is the 2nd or later pass of main
           unification */
        nmbrLet((nmbrString **)(&hypProofList[hypListMap[hyp]]),
            NULL_NMBRSTRING);

        hypProofList[hypListMap[hyp]] = hypProofPtr;
      }
      if (breakFlag) {
       /* Proof is not possible for some hypothesis. */

       /* Speedup:  Move the hypothesis for which the proof was not found
          to the beginning of the hypothesis list, so it will be tried
          first next time. */
       j = hypListMap[hyp];
       for (i = hyp; i >= 1; i--) {
         hypListMap[i] = hypListMap[i - 1];
       }
       hypListMap[0] = j;

       continue; /* Not possible; get next unification */

      } /* End if breakFlag */

      /* Proofs were found for all hypotheses */

      /* Build the proof */
      for (hyp = 0; hyp < schReqHyps; hyp++) {
        nmbrLet(&proof, nmbrCat(proof, hypProofList[hyp], NULL));
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
  nmbrLet(&hypListMap, NULL_NMBRSTRING);
  pntrLet(&hypProofList, NULL_PNTRSTRING);
  depth--; /* Restore backtracking depth */
/*E*/if(db8)print2("%s\n", cat(space(depth+2), "Returned: ",
/*E*/   nmbrCvtRToVString(proof), NULL));
/*E*/if(db8){if(!depth)print2("Trials: %ld\n", trials);}
  return (proof); /* Caller must deallocate */
}



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
  flag foundFlag, breakFlag;
  nmbrString *mString; /* Pointer only; not allocated */
  nmbrString *newSubProofPtr = NULL_NMBRSTRING; /* Pointer only; not allocated;
                however initialize for nmbrLen function before it's assigned */

  while (1) {
    plen = nmbrLen(proofInProgress.proof);
    foundFlag = 0;
    for (step = plen - 1; step >= 0; step--) {
      /* Reject step with dummy vars */
      mString = proofInProgress.target[step];
      mlen = nmbrLen(mString);
      breakFlag = 0;
      for (sym = 0; sym < mlen; sym++) {
        if (mString[sym] > mathTokens) {
          breakFlag = 1;
          break;
        }
      }
      if (breakFlag) continue;  /* Step has dummy var.; don't try it */

      /* Reject step not matching replacement step */
      if (!checkStmtMatch(repStatemNum, step)) continue;

      /* Try the replacement */
      if (proofInProgress.proof[step] != repStatemNum) {
                                   /* Don't replace a step with itself (will
                                   cause infinite loop in ALLOW_GROWTH mode) */
        newSubProofPtr = replaceStatement(repStatemNum, step,
            prvStatemNum);
      }
      if (!nmbrLen(newSubProofPtr)) continue;
                                           /* Replacement was not successful */

      /* Get the subproof at step s */
      sublen = subProofLen(proofInProgress.proof, step);
      if (sublen > nmbrLen(newSubProofPtr) || allowGrowthFlag) {
        /* Success - proof length was reduced */
        deleteSubProof(step);
        addSubProof(newSubProofPtr, step - sublen + 1);
        assignKnownSteps(step - sublen + 1, nmbrLen(newSubProofPtr));
        foundFlag = 1;
        nmbrLet(&newSubProofPtr, NULL_NMBRSTRING);
        break;
      }

      nmbrLet(&newSubProofPtr, NULL_NMBRSTRING);
    } /* next step */

    if (!foundFlag) break; /* Done */
  } /* end while */

}




/* Initialize proofInProgress.source of the step, and .target of all
   hypotheses, to schemes using new dummy variables. */
void initStep(long step)
{
  long stmt, reqHyps, pos, hyp, sym, reqVars, var, mlen;
  nmbrString *reqHypPos = NULL_NMBRSTRING;
  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated */

  stmt = proofInProgress.proof[step];
  if (stmt < 0) {
    if (stmt == -(long)'?') {
      /* Initialize unknown step source to nothing */
      nmbrLet((nmbrString **)(&(proofInProgress.source[step])),
          NULL_NMBRSTRING);
    } else {
/*E*/print2("step %ld stmt %ld\n",step,stmt);
      bug(1809); /* Compact proof not handled (yet?) */
    }
    return;
  }
  if (statement[stmt].type == (char)e__ || statement[stmt].type == (char)f__) {
    /* A hypothesis -- initialize to the actual statement */
    nmbrLet((nmbrString **)(&(proofInProgress.source[step])),
        statement[stmt].mathString);
    return;
  }

  /* It must be an assertion ($a or $p) */

  /* Assign the assertion to .source */
  nmbrLet((nmbrString **)(&(proofInProgress.source[step])),
      statement[stmt].mathString);

  /* Find the position in proof of all required hyps, and
     assign them */
  reqHyps = statement[stmt].numReqHyp;
  nmbrLet(&reqHypPos, nmbrSpace(reqHyps)); /* Preallocate */
  pos = step - 1; /* Step with last hyp */
  for (hyp = reqHyps - 1; hyp >= 0; hyp--) {
    reqHypPos[hyp] = pos;
    nmbrLet((nmbrString **)(&(proofInProgress.target[pos])),
        statement[statement[stmt].reqHypList[hyp]].mathString);
                                           /* Assign the hypothesis to target */
    if (hyp > 0) { /* Don't care about subproof length for 1st hyp */
      pos = pos - subProofLen(proofInProgress.proof, pos);
                                             /* Get to step with previous hyp */
    }
  }

  /* Change the variables in the assertion and hypotheses to dummy variables */
  reqVars = nmbrLen(statement[stmt].reqVarList);
  if (pipDummyVars + reqVars > dummyVars) {
    /* Declare more dummy vars if necessary */
    declareDummyVars(pipDummyVars + reqVars - dummyVars);
  }
  for (var = 0; var < reqVars; var++) {
    /* Put dummy var mapping into mathToken[].tmp field */
    mathToken[statement[stmt].reqVarList[var]].tmp = mathTokens + 1 +
      pipDummyVars + var;
  }
  /* Change vars in assertion */
  nmbrTmpPtr = proofInProgress.source[step];
  mlen = nmbrLen(nmbrTmpPtr);
  for (sym = 0; sym < mlen; sym++) {
    if (mathToken[nmbrTmpPtr[sym]].tokenType == (char)var__) {
      /* Use dummy var mapping from mathToken[].tmp field */
      nmbrTmpPtr[sym] = mathToken[nmbrTmpPtr[sym]].tmp;
    }
  }
  /* Change vars in hypotheses */
  for (hyp = 0; hyp < reqHyps; hyp++) {
    nmbrTmpPtr = proofInProgress.target[reqHypPos[hyp]];
    mlen = nmbrLen(nmbrTmpPtr);
    for (sym = 0; sym < mlen; sym++) {
      if (mathToken[nmbrTmpPtr[sym]].tokenType == (char)var__) {
        /* Use dummy var mapping from mathToken[].tmp field */
        nmbrTmpPtr[sym] = mathToken[nmbrTmpPtr[sym]].tmp;
      }
    }
  }

  /* Update the number of dummy vars used so far */
  pipDummyVars = pipDummyVars + reqVars;

  nmbrLet(&reqHypPos, NULL_NMBRSTRING); /* Deallocate */

  return;
}




/* Look for completely known subproofs in proofInProgress.proof and
   assign proofInProgress.target and .source.  Calls assignKnownSteps(). */
void assignKnownSubProofs(void)
{
  long plen, pos, subplen, q;
  flag breakFlag;

  plen = nmbrLen(proofInProgress.proof);
  /* Scan proof for known subproofs (backwards, to get biggest ones first) */
  for (pos = plen - 1; pos >= 0; pos--) {
    subplen = subProofLen(proofInProgress.proof, pos); /* Find length of subpr*/
    breakFlag = 0;
    for (q = pos - subplen + 1; q <= pos; q++) {
      if (proofInProgress.proof[q] == -(long)'?') {
        breakFlag = 1;
        break;
      }
    }
    if (breakFlag) continue; /* Skip subproof - it has an unknown step */

    /* See if all steps in subproof are assigned and known; if so, don't assign
       them again. */
    /* (???Add this code if needed for speedup) */

    /* Assign the steps of the known subproof to proofInProgress.target */
    assignKnownSteps(pos - subplen + 1, subplen);

    /* Adjust pos for next pass through 'for' loop */
    pos = pos - subplen + 1;

  } /* Next pos */
  return;
}


/* This function assigns math strings to all steps (proofInProgress.target and
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
    stmt = proofInProgress.proof[pos];
    if (stmt <= 0) bug(1810); /* Compact proofs are not handled (yet?) */
    if (statement[stmt].type == (char)e__ || statement[stmt].type == (char)f__){
      /* It's a hypothesis; assign step; push the stack */
      nmbrLet((nmbrString **)(&(proofInProgress.source[pos])),
          statement[stmt].mathString);
      stack[stackPtr] = pos;
      stackPtr++;
    } else {
      /* It's an assertion. */

      /* Assemble the hypotheses for unification */
      reqHyps = statement[stmt].numReqHyp;

      instLen = 1; /* First "$|$" separator token */
      for (st = stackPtr - reqHyps; st < stackPtr; st++) {
        /* Add 1 for "$|$" separator token */
        instLen = instLen + nmbrLen(proofInProgress.source[stack[st]]) + 1;
      }
      /* Preallocate instance */
      nmbrLet(&instance, nmbrSpace(instLen));
      /* Assign instance */
      instance[0] = mathTokens; /* "$|$" separator */
      instPos = 1;
      for (st = stackPtr - reqHyps; st < stackPtr; st++) {
        hypLen = nmbrLen(proofInProgress.source[stack[st]]);
        for (hypPos = 0; hypPos < hypLen; hypPos++) {
          instance[instPos] =
              ((nmbrString *)(proofInProgress.source[stack[st]]))[hypPos];
          instPos++;
        }
        instance[instPos] = mathTokens; /* "$|$" separator */
        instPos++;
      }
      if (instLen != instPos) bug(1811); /* ???Delete after debugging */

      schemeLen = 1; /* First "$|$" separator token */
      for (hyp = 0; hyp < reqHyps; hyp++) {
        /* Add 1 for "$|$" separator token */
        schemeLen = schemeLen +
            statement[statement[stmt].reqHypList[hyp]].mathStringLen + 1;
      }
      /* Preallocate scheme */
      nmbrLet(&scheme, nmbrSpace(schemeLen));
      /* Assign scheme */
      scheme[0] = mathTokens; /* "$|$" separator */
      schemePos = 1;
      for (hyp = 0; hyp < reqHyps; hyp++) {
        hypLen = statement[statement[stmt].reqHypList[hyp]].mathStringLen;
        for (hypPos = 0; hypPos < hypLen; hypPos++) {
          scheme[schemePos] =
              statement[statement[stmt].reqHypList[hyp]].mathString[hypPos];
          schemePos++;
        }
        scheme[schemePos] = mathTokens; /* "$|$" separator */
        schemePos++;
      }
      if (schemeLen != schemePos) bug(1812); /* ???Delete after debugging */

      /* Change variables in scheme to dummy variables for unification */
      reqVars = nmbrLen(statement[stmt].reqVarList);
      if (reqVars + pipDummyVars > dummyVars) {
        /* Declare more dummy vars if necessary */
        declareDummyVars(reqVars + pipDummyVars - dummyVars);
      }
      for (var = 0; var < reqVars; var++) {
        /* Put dummy var mapping into mathToken[].tmp field */
        mathToken[statement[stmt].reqVarList[var]].tmp = mathTokens + 1 +
          pipDummyVars + var;
      }
      for (schemePos = 0; schemePos < schemeLen; schemePos++) {
        if (mathToken[scheme[schemePos]].tokenType
            != (char)var__) continue;
        /* Use dummy var mapping from mathToken[].tmp field */
        scheme[schemePos] = mathToken[scheme[schemePos]].tmp;
      }

      /* Change variables in assertion to dummy variables for substitition */
      nmbrLet(&assertion, statement[stmt].mathString);
      assLen = nmbrLen(assertion);
      for (assPos = 0; assPos < assLen; assPos++) {
        if (mathToken[assertion[assPos]].tokenType
            != (char)var__) continue;
        /* Use dummy var mapping from mathToken[].tmp field */
        assertion[assPos] = mathToken[assertion[assPos]].tmp;
      }

      /* Unify scheme and instance */
      unifTrialCount = 0; /* Reset unification to no timeout */
      tmpFlag = unifyH(scheme, instance, &stateVector, 0);
      if (!tmpFlag) {
        /*bug(1813);*/ /* Not poss. */
        /* Actually this is possible if the starting proof had an error
           in it.  Give the user some information then give up */
        printLongLine(cat("?Error in step ", str(pos + 1),
            ":  Could not simultaneously unify the hypotheses of \"",
            statement[stmt].labelName, "\":\n    ",
            nmbrCvtMToVString(scheme),
            "\nwith the following statement list:\n    ",
            nmbrCvtMToVString(instance),
            "\n(The $|$ tokens are internal statement separation markers)",
            "\nZapping targets so we can procede (but you should exit the ",
            "Proof Assistant and fix this problem)",
            NULL), "", " ");
        purgeStateVector(&stateVector);
        goto returnPoint;
      }
      /* Substitute and assign assertion to proof in progress */
      nmbrLet((nmbrString **)(&(proofInProgress.source[pos])), NULL_NMBRSTRING);
      proofInProgress.source[pos] = makeSubstUnif(&tmpFlag, assertion,
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
    nmbrLet((nmbrString **)(&(proofInProgress.target[pos])),
        proofInProgress.source[pos]);
  }

  /* Deallocate (stateVector was deallocated by 2nd unif. call) */
  nmbrLet(&stack, NULL_NMBRSTRING);
  nmbrLet(&instance, NULL_NMBRSTRING);
  nmbrLet(&scheme, NULL_NMBRSTRING);
  nmbrLet(&assertion, NULL_NMBRSTRING);
  return;
}


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
  if (!nmbrLen(proofInProgress.target[step])) bug (1817);

  /* First, see if .target and .user should be unified */
  /* If not, then see if .source and .user should be unified */
  /* If not, then see if .target and .source should be unified */
  if (nmbrLen(proofInProgress.user[step])) {
    if (!nmbrEq(proofInProgress.target[step], proofInProgress.user[step])) {
      if (messageFlag == 0) print2("Step %ld:\n", step + 1);
      unifFlag = interactiveUnify(proofInProgress.target[step],
        proofInProgress.user[step], &stateVector);
      goto subAndReturn;
    }
    if (nmbrLen(proofInProgress.source[step])) {
      if (!nmbrEq(proofInProgress.source[step], proofInProgress.user[step])) {
        if (messageFlag == 0) print2("Step %ld:\n", step + 1);
        unifFlag = interactiveUnify(proofInProgress.source[step],
          proofInProgress.user[step], &stateVector);
        goto subAndReturn;
      }
    }
  } else {
    if (nmbrLen(proofInProgress.source[step])) {
      if (!nmbrEq(proofInProgress.target[step], proofInProgress.source[step])) {
        if (messageFlag == 0) print2("Step %ld:\n", step + 1);
        unifFlag = interactiveUnify(proofInProgress.target[step],
          proofInProgress.source[step], &stateVector);
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

    proofChangedFlag = 1; /* Flag to push 'undo' stack */

    makeSubstAll(stateVector);

  } /* End if unifFlag = 1 */

  purgeStateVector(&stateVector);

  return;

}


/* Interactively select one of several possible unifications */
/* Returns:  0 = no unification possible
             1 = unification was selected; held in stateVector
             2 = unification timed out
             3 = no unification was selected */
char interactiveUnify(nmbrString *schemeA, nmbrString *schemeB,
    pntrString **stateVector)
{

  long var;
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

  if (nmbrEq(schemeA, schemeB)) bug(1818); /* No reason to call this */

  /* Count the number of possible unifications */
  unifTrialCount = 1; /* Reset unification timeout */
  unifCount = 0;
  reEntryFlag = 0;
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
      return (2);
    }
    if (!unifFlag) break;
    reEntryFlag = 1;
    unifCount++;
  }
  if (!unifCount) {
    printf("The unification is not possible.  The proof has an error.\n");
    return(0); /* Not possible */
  }
  if (unifCount > 1) {
    print2(
"There are %ld possible unifications.  Please select the correct one.\n",
      unifCount);
    printLongLine(cat("Unify:  ", nmbrCvtMToVString(schemeA), NULL),
        "    ", " ");
    printLongLine(cat(" with:  ", nmbrCvtMToVString(schemeB), NULL),
        "    ", " ");
  }
  unifTrialCount = 1; /* Reset unification timeout */
  reEntryFlag = 0;
  for (unifNum = 1; unifNum <= unifCount; unifNum++) {
    if (unifCount > 1) {
      print2("Unification #%ld:\n", unifNum);
    }
    unifFlag = unifyH(schemeA, schemeB, &(*stateVector), reEntryFlag);
    if (unifFlag != 1) bug(1819);

    if (unifCount == 1) {
      print2("Step was successfully unified.\n");
      return(1); /* Always accept unique unification */
    }

    reEntryFlag = 1;

    stackTop = ((nmbrString *)((*stateVector)[11]))[1];
    stackUnkVar = (nmbrString *)((*stateVector)[1]);
    stackUnkVarStart = (nmbrString *)((*stateVector)[2]);
    stackUnkVarLen = (nmbrString *)((*stateVector)[3]);
    unifiedScheme = (nmbrString *)((*stateVector)[8]);
    for (var = 0; var <= stackTop; var++) {
      printLongLine(cat("  Replace \"",
        mathToken[stackUnkVar[var]].tokenName,"\" with \"",
          nmbrCvtMToVString(
              nmbrMid(unifiedScheme,stackUnkVarStart[var] + 1,
              stackUnkVarLen[var])), "\"", NULL),"    "," ");
      /* Clear temporary string allocation during print */
      let(&tmpStr,"");
      nmbrLet(&nmbrTmp,NULL_NMBRSTRING);
    } /* Next var */

    while(1) {
      tmpStr = cmdInput1("  Accept (A), reject (R), or quit (Q) <R>? ");
      if (!tmpStr[0]) {
        let(&tmpStr, "");
        break;
      }
      if (tmpStr[0] == 'R' || tmpStr[0] == 'r') {
        if (!tmpStr[1]) {
          let(&tmpStr, "");
          break;
        }
      }
      if (tmpStr[0] == 'Q' || tmpStr[0] == 'q') {
        if (!tmpStr[1]) {
          let(&tmpStr, "");
          return(3);
        }
      }
      if (tmpStr[0] == 'A' || tmpStr[0] == 'a') {
        if (!tmpStr[1]) {
          let(&tmpStr, "");
          return(1);
        }
      }
      let(&tmpStr, "");
    }

  } /* Next unifNum */

  return (3);  /* No unification was selected */

}



/* Automatically unify steps that have unique unification */
/* Prints "congratulation" if congrats = 1 */
void autoUnify(flag congrats)
{
  long step, plen;
  char unifFlag;
  flag stepChanged;
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

  plen = nmbrLen(proofInProgress.proof);

  while (somethingChanged) {
    somethingChanged = 0;
    for (step = 0; step < plen; step++) {
      stepChanged = 0;

      for (pass = 0; pass < 3; pass++) {

        switch (pass) {
          case 0:
            /* Check target vs. user */
            schemeAPtr = proofInProgress.target[step];
            if (!nmbrLen(schemeAPtr)) print2(
 "?Bad unification selected:  A proof step should never be completely empty\n");
      /*if (!nmbrLen(schemeAPtr)) bug (1820);*/ /* Target can never be empty */
            schemeBPtr = proofInProgress.user[step];
            break;
          case 1:
            /* Check source vs. user */
            schemeAPtr = proofInProgress.source[step];
            schemeBPtr = proofInProgress.user[step];
            break;
          case 2:
            /* Check target vs. source */
            schemeAPtr = proofInProgress.target[step];
            schemeBPtr = proofInProgress.source[step];
            break;
        }
        if (nmbrLen(schemeAPtr) && nmbrLen(schemeBPtr)) {
          if (!nmbrEq(schemeAPtr, schemeBPtr)) {
            unifTrialCount = 1; /* Reset unification timeout */
            unifFlag = uniqueUnif(schemeAPtr, schemeBPtr, &stateVector);
            if (unifFlag != 1) somethingNotUnified = 1;
            if (unifFlag == 2) {
              print2("A unification timeout occurred at step %ld.\n", step + 1);
            }
            if (!unifFlag) {
              print2(
              "Step %ld cannot be unified.  THERE IS AN ERROR IN THE PROOF.\n",
                  (long)(step + 1));
              print2(
"If your system needs empty substitutions, see HELP SET EMPTY_SUBSTITUTION.\n"
                 );
              continue;
            }
            if (unifFlag == 1) {
              /* Make substitutions to all steps */
              makeSubstAll(stateVector);
              somethingChanged = 1;
              proofChangedFlag = 1; /* Flag for undo stack */
              print2("Step %ld was successfully unified.\n", (long)(step + 1));
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
      if (!nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
        print2(
  "CONGRATULATIONS!  The proof is complete.  Use SAVE NEW_PROOF to save it.\n");
      }
    }
  }

  return;

}


/* Make stateVector substitutions in all steps.  The stateVector must
   contain the result of a valid unification. */
void makeSubstAll(pntrString *stateVector) {

  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated */
  long plen, step;
  flag tmpFlag;

  plen = nmbrLen(proofInProgress.proof);
  for (step = 0; step < plen; step++) {

    nmbrTmpPtr = proofInProgress.target[step];
    proofInProgress.target[step] = makeSubstUnif(&tmpFlag, nmbrTmpPtr,
      stateVector);
    nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);

    nmbrTmpPtr = proofInProgress.source[step];
    if (nmbrLen(nmbrTmpPtr)) {
      proofInProgress.source[step] = makeSubstUnif(&tmpFlag, nmbrTmpPtr,
        stateVector);
      nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);
    }

    nmbrTmpPtr = proofInProgress.user[step];
    if (nmbrLen(nmbrTmpPtr)) {
      proofInProgress.source[step] = makeSubstUnif(&tmpFlag, nmbrTmpPtr,
        stateVector);
      nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);
    }

  } /* Next step */
  return;
}

/* Replace a dummy variable with a user-specified math string */
void replaceDummyVar(long dummyVar, nmbrString *mString)
{
  long numSubs = 0;
  long numSteps = 0;
  long plen, step, sym, slen;
  flag stepChanged;
  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated */

  plen = nmbrLen(proofInProgress.proof);
  for (step = 0; step < plen; step++) {

    stepChanged = 0;

    nmbrTmpPtr = proofInProgress.target[step];
    slen = nmbrLen(nmbrTmpPtr);
    for (sym = slen - 1; sym >= 0; sym--) {
      if (nmbrTmpPtr[sym] == dummyVar + mathTokens) {
        nmbrLet((nmbrString **)(&(proofInProgress.target[step])),
            nmbrCat(nmbrLeft(nmbrTmpPtr, sym), mString,
            nmbrRight(nmbrTmpPtr, sym + 2), NULL));
        nmbrTmpPtr = proofInProgress.target[step];
        stepChanged = 1;
        numSubs++;
      }
    } /* Next sym */

    nmbrTmpPtr = proofInProgress.source[step];
    slen = nmbrLen(nmbrTmpPtr);
    for (sym = slen - 1; sym >= 0; sym--) {
      if (nmbrTmpPtr[sym] == dummyVar + mathTokens) {
        nmbrLet((nmbrString **)(&(proofInProgress.source[step])),
            nmbrCat(nmbrLeft(nmbrTmpPtr, sym), mString,
            nmbrRight(nmbrTmpPtr, sym + 2), NULL));
        nmbrTmpPtr = proofInProgress.source[step];
        stepChanged = 1;
        numSubs++;
      }
    } /* Next sym */

    nmbrTmpPtr = proofInProgress.user[step];
    slen = nmbrLen(nmbrTmpPtr);
    for (sym = slen - 1; sym >= 0; sym--) {
      if (nmbrTmpPtr[sym] == dummyVar + mathTokens) {
        nmbrLet((nmbrString **)(&(proofInProgress.user[step])),
            nmbrCat(nmbrLeft(nmbrTmpPtr, sym), mString,
            nmbrRight(nmbrTmpPtr, sym + 2), NULL));
        nmbrTmpPtr = proofInProgress.user[step];
        stepChanged = 1;
        numSubs++;
      }
    } /* Next sym */

    if (stepChanged) numSteps++;
  } /* Next step */

  if (numSubs) {
    proofChangedFlag = 1; /* Flag to push 'undo' stack */
    print2("%ld substitutions were made in %ld steps.\n", numSubs, numSteps);
  } else {
    print2("?The dummy variable $%ld is nowhere in the proof.\n", dummyVar);
  }

  return;
}

/* Get subproof length of a proof, starting at endStep and going backwards */
long subProofLen(nmbrString *proof, long endStep)
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
    if (statement[stmt].type == (char)e__ ||
        statement[stmt].type == (char)f__) { /* A hypothesis */
      continue;
    }
    lvl = lvl + statement[stmt].numReqHyp;
  }
  return (endStep - p + 1);
}



/* This function puts numNewVars dummy variables, named "$nnn", at the end
   of the mathToken array and modifies the global variable dummyVars. */
/* Note:  The mathToken array will grow forever as this gets called;
   it is never purged, as this might worsen memory fragmentation. */
/* ???Should we add a purge function? */
void declareDummyVars(long numNewVars)
{

  long i;

  long saveTempAllocStack;
  saveTempAllocStack = startTempAllocStack;
  startTempAllocStack = tempAllocStackTop; /* For let() stack cleanup */

  for (i = 0; i < numNewVars; i++) {

    dummyVars++;
    /* First, check to see if we need to allocate more mathToken memory */
    if (mathTokens + 1 + dummyVars >= MAX_MATHTOKENS) {
      /* The +1 above accounts for the dummy "$|$" boundary token */
      /* Reallocate */
      /* Add 1000 so we won't have to do this very often */
      MAX_MATHTOKENS = MAX_MATHTOKENS + 1000;
      mathToken = realloc(mathToken, MAX_MATHTOKENS *
  	sizeof(struct mathToken_struct));
      if (!mathToken) outOfMemory("#10 (mathToken)");
    }

    mathToken[mathTokens + dummyVars].tokenName = "";
				  /* Initialize vstring before let() */
    let(&mathToken[mathTokens + dummyVars].tokenName,
        cat("$", str(dummyVars), NULL));
    mathToken[mathTokens + dummyVars].length =
        strlen(mathToken[mathTokens + dummyVars].tokenName);
    mathToken[mathTokens + dummyVars].scope = currentScope;
    mathToken[mathTokens + dummyVars].active = 1;
    mathToken[mathTokens + dummyVars].tokenType = (char)var__;
    mathToken[mathTokens + dummyVars].tmp = 0;

  }

  startTempAllocStack = saveTempAllocStack;

  return;

}


