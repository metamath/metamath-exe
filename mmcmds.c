/*****************************************************************************/
/*               Copyright (C) 1997, NORMAN D. MEGILL                        */
/*****************************************************************************/

/*34567890123456 (79-character line to adjust text window width) 678901234567*/

/* mmcmds.c - assorted user commands */

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "mmutil.h"
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmds.h"
#include "mminou.h"
#include "mmpars.h"
#include "mmveri.h"
#include "mmwtex.h"
#include "mmpfas.h"

vstring mainFileName = "";
flag printHelp = 0;

void typeExpandedProof(nmbrString *reason,
    pntrString *expansion, nmbrString *stepNumbers,
    char displayModeFlag)
/* Prints out an expanded proof in a form that can be cut and pasted into
   the source file. */
/* reason[] is the proof; expansion[] points to the expanded
   math token string for each step in reason[]. */
/* If stepNumbers is not empty, its steps will be used instead of
   the computed steps (used for partially displayed proofs) */
/* If displayModeFlag is 1, only '?' (unknown) steps will be displayed. */
/* If displayModeFlag is 2, only steps not unified will be displayed. */
{
  vstring str1 = "", str2 = "";
  long pos,targpos,ilev,oldilev,srcpos,step,plen,i,tokenNum;
  long max_proof_line = 0;
  char pass;
  flag splitFlag;
  char srcType;
  flag printedALine = 0;

  /* Pass 0 gets the longest label assignment field (max_proof_line)
     so that the printout can be adjusted for neatness.  Pass 1 does
     the actual printout. */
  for (pass = 0; pass < 2; pass++) {
  /*??? Indent the stuff in this loop...*/

  ilev = 0; /* Indentation level */
  step = 0; /* Step number */
#define INDENT_INC 2 /* Indentation increment */
  plen = pntrLen(expansion); /* Length of reason */
  for (pos = 0; pos < plen; pos++) {
    if (stepNumbers[0] == -1) {
      step++;
    } else {
      /* One of these will be zero, the other not */
      step = stepNumbers[pos + 2] + stepNumbers[pos + 3];
    }
    let(&str1,str(step));
    let(&str1,cat(str1,space(ilev * INDENT_INC + 1 - len(str1))," ",NULL));
    tokenNum = reason[pos];
    if (tokenNum < 0) bug(201);
    let(&str1,cat(str1,statement[tokenNum].labelName,NULL));
    targpos = pos; /* Assignment target */
    pos++;
    tokenNum = reason[pos];
    if (tokenNum != -(long)'=') bug(202);
    let(&str1,cat(str1,"=",NULL));
    pos++;
    tokenNum = reason[pos];
    if (tokenNum != -(long)'?' && tokenNum != -(long)'(' && tokenNum < 0) {
      bug(203);
    }
    oldilev = ilev;
    if (tokenNum == -(long)'(') {
      let(&str1,cat(str1,"(",NULL));
      ilev++;
      pos++;
      tokenNum = reason[pos];
    }
    if (tokenNum == -(long)'?') {
      let(&str1,cat(str1,"?",NULL));
      srcType = '?';
    } else {
      let(&str1,cat(str1,statement[tokenNum].labelName,NULL));
      srcType = statement[tokenNum].type;
    }
    srcpos = pos;
    pos++;
    tokenNum = reason[pos];
    while (tokenNum == -(long)')') {
      let(&str1,cat(str1,")",NULL));
      ilev--;
      if (ilev < 0) bug(204);
      pos++;
      tokenNum = reason[pos];
    }
    pos--;

    if (pass == 0) {
      /* Update the longest proof line if needed, then continue */
      if (max_proof_line < len(str1) - oldilev * INDENT_INC + 4) {
        max_proof_line = len(str1) - oldilev * INDENT_INC + 1;
      }
      continue; /* The first pass doesn't print anything */
    }

    if (displayModeFlag == 1 && srcType != '?') continue;
                                              /* "Display unknown steps" flag */
    splitFlag = 0;
    /* Split the line if the target and the source are different */
    if (reason[srcpos] != -(long)'?') {
      if (strcmp(expansion[targpos],expansion[srcpos])) {
        /* (The verify proof function will not assign target assumptions) */
        if (len(expansion[targpos])) {
          splitFlag = 1;
          i = instr(1,str1,"=");
          let(&str2,cat(space(i - 1),right(str1,i),NULL));
          let(&str1,left(str1,i - 1));
          let(&str1,cat(str1,space(len(str1) - len(str2)),NULL));
        }
      }
    }
    if (displayModeFlag == 2 && !splitFlag) continue;
                                     /* "Display steps not yet unified" flag */
    let(&str1,cat(str1,space(oldilev * INDENT_INC + max_proof_line - len(str1)),
        " ",NULL));
    if (!splitFlag) {
      if (srcType == '?') {
        printLongLine(cat(str1,"$",chr(srcType),
            " ",expansion[targpos],NULL),
            space(len(str1) + 3)," ");
      } else {
        printLongLine(cat(str1,"$",chr(srcType),
            " ",expansion[srcpos],NULL),
            space(len(str1) + 3)," ");
      }
    } else {
      printLongLine(cat(str1,"   ",
          expansion[targpos],NULL),
          space(len(str1) + 3)," ");
      let(&str2,cat(str2,space(oldilev * INDENT_INC + max_proof_line
          - len(str2))," ",NULL));
      printLongLine(cat(str2,"$",chr(srcType),
          " ",expansion[srcpos],NULL),
          space(len(str2) + 3)," ");
    }
    printedALine = 1;
  } /* Next pos */

  } /* Next pass */
  if (!printedALine) {
    if (displayModeFlag == 1) {
      print2("The proof has no unknown steps.\n");
    }
    if (displayModeFlag == 2) {
      print2("All assignments in the proof have been unified.\n");
    }
  }

  let(&str1,"");
  let(&str2,"");
}


void typeCompactProof(nmbrString *reason)
/* Prints out a compact proof in a form that can be cut and pasted into
   the source file.  The first two tokens in reason are removed, as
   reason is assumed to be in the "internal" format "proveStatement = (...)". */
/* reason[] is the proof. */
{
  vstring str1 = "";
  long j;
  nmbrString *squishedProof = NULL_NMBRSTRING;

  /*squishedProof = nmbrSquishProof(nmbrRight(reason,3));*/
  nmbrLet(&squishedProof, nmbrRight(reason,3));

  let(&str1,nmbrCvtRToVString(squishedProof));
  nmbrLet(&squishedProof,NULL_NMBRSTRING); /* Deallocate */
  /* Remove spaces around '=' */
  j = 1;
  while (1) {
    j = instr(j, str1, " = ");
    if (!j) break;
    let(&str1,cat(left(str1, j - 1), "=", right(str1, j + 3), NULL));
  }
  /* Remove space after '(' */
  j = 1;
  while (1) {
    j = instr(j, str1, "( ");
    if (!j) break;
    let(&str1,cat(left(str1, j - 1), "(", right(str1, j + 2), NULL));
  }
  j = 1;
  /* Remove space before ')' */
  while (1) {
    j = instr(j, str1, " )");
    if (!j) break;
    let(&str1,cat(left(str1, j - 1), ")", right(str1, j + 2), NULL));
  }
  /* Remove space after ')' */
/*
  j = 1;
  while (1) {
    j = instr(j, str1, ") ");
    if (!j) break;
    let(&str1,cat(left(str1, j - 1), ")", right(str1, j + 2), NULL));
  }
*/
  /* Remove space after '{' */
  j = 1;
  while (1) {
    j = instr(j, str1, "{ ");
    if (!j) break;
    let(&str1,cat(left(str1, j - 1), "{", right(str1, j + 2), NULL));
  }
  j = 1;
  /* Remove space before '}' */
  while (1) {
    j = instr(j, str1, " }");
    if (!j) break;
    let(&str1,cat(left(str1, j - 1), "}", right(str1, j + 2), NULL));
  }
  /* Remove space after '}' */
/*
  j = 1;
  while (1) {
    j = instr(j, str1, "} ");
    if (!j) break;
    let(&str1,cat(left(str1, j - 1), "}", right(str1, j + 2), NULL));
  }
*/
  printLongLine(str1,""," ()");
  let(&str1,"");
}


void typeEnumProof(nmbrString *reason)
{
  /* This function types a compact proof with the step numbers listed below
     it on each line. */
  long i,rlen;
  vstring oLine = "";
  vstring rToken = "";
  nmbrString *stepNumbs; /* Pointer only - not directly allocated */

  long nmbrSaveTempAllocStack;
  /* Change the stack allocation start to prevent arguments from being
     deallocated */
  nmbrSaveTempAllocStack = nmbrStartTempAllocStack;
  nmbrStartTempAllocStack = nmbrTempAllocStackTop; /* For nmbrLet() stack cleanup*/

  rlen = nmbrLen(reason);
  stepNumbs = nmbrGetProofStepNumbs(reason);
  for (i = 0; i < rlen; i++) {
    if (reason[i] < 0) {
      let(&rToken,chr(-reason[i]));
    } else {
      let(&rToken,statement[reason[i]].labelName);
    }
    if (i == 0) {
      let(&oLine,rToken);
    } else {
      /* For most compact proof (commented out; harder to read): */
      /* if ((reason[i - 1] >= 0 || reason[i] >=0)
          || reason[i] == -(long)'?'
          || stepNumbs[i - 1] != 0) { */
      if (1) {
        let(&oLine,cat(oLine," ",NULL));
      }
      let(&oLine,cat(oLine,rToken,NULL));
    }
    if (stepNumbs[i] != 0) {
      let(&oLine,cat(oLine,"<",str(stepNumbs[i]),">",NULL));
    }
  }
  printLongLine(oLine,""," ");

  nmbrLet(&stepNumbs,NULL_NMBRSTRING); /* Deallocate */
  let(&oLine,""); /* Deallocate */
  let(&rToken,""); /* Deallocate */

  nmbrStartTempAllocStack = nmbrSaveTempAllocStack;
  return;
}


void typeEnumProof2(nmbrString *reason)
{
  /* This function types a compact proof with the step numbers listed below
     it on each line. */
  vstring rLine = "";
  vstring sLine = "";
  vstring tmpStr = "";
  long i,j,p,q,step,rlen,rToken;
  nmbrString *stepNumbs; /* Pointer only - not directly allocated */

  long nmbrSaveTempAllocStack;
  /* Change the stack allocation start to prevent arguments from being
     deallocated */
  nmbrSaveTempAllocStack = nmbrStartTempAllocStack;
  nmbrStartTempAllocStack = nmbrTempAllocStackTop; /* For nmbrLet() stack cleanup*/

  rlen = nmbrLen(reason);
  stepNumbs = nmbrGetProofStepNumbs(reason);
  let(&rLine,cat(nmbrCvtRToVString(reason)," ",NULL));
  p = 1;
  rToken = -1;
  while (1) {
    q = instr(p,rLine," ");
    if (q) {
      rToken++;
      if (stepNumbs[rToken]) {
        let(&tmpStr,str(stepNumbs[rToken]));
      } else {
        let(&tmpStr,"");
      }
      i = strlen(tmpStr); /* Length of step number */
      j = q - p; /* Length of token name */
      if (i <= j) {
        let(&tmpStr,cat(tmpStr,space(j - i + 1),NULL));
      } else {
        let(&rLine,cat(left(rLine,q - 1),space(i - j),right(rLine,q),NULL));
        q = q + i - j;
      }
      let(&sLine,cat(sLine,tmpStr,NULL));
    } /* End if q */
    if (q > MAX_LEN || !q) {
      print2("%s\n",left(rLine,p - 1));
      print2("%s\n",left(sLine,p - 1));
      if (!q) break;
      let(&rLine,right(rLine,p));
      let(&sLine,right(sLine,p));
      q = q - p + 1;
      p = 1;
    }
    p = q + 1;
  }
    
  nmbrLet(&stepNumbs,NULL_NMBRSTRING);
  let(&tmpStr,"");
  let(&rLine,"");
  let(&sLine,"");
  nmbrStartTempAllocStack = nmbrSaveTempAllocStack;
  return;
}


/* Displays a proof (or part of a proof, depending on arguments). */
/* Note that parseProof() and verifyProof() are assumed to have been called,
   so that the wrkProof structure elements are assigned for the current
   statement. */
void typeProof(long statemNum,
  flag pipFlag, /* Means use proofInProgress; statemNum must be proveStatement*/
  long startStep, long endStep,
  long startIndent, long endIndent,
  flag essentialFlag,
  flag renumberFlag,
  flag unknownFlag,
  flag notUnifiedFlag,
  flag reverseFlag,
  flag noIndentFlag,
  long splitColumn,
  flag texFlag)
{
  long i, plen, maxLocalLen, step, stmt, lens, lent, maxStepNum;
  vstring proofStr = "";
  vstring tmpStr = "";
  vstring tmpStr1 = "";
  vstring locLabDecl = "";
  vstring tgtLabel = "";
  vstring srcLabel = "";
  vstring startPrefix = "";
  vstring tgtPrefix = "";
  vstring srcPrefix = "";
  vstring userPrefix = "";
  vstring contPrefix = "";
  vstring ptr;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *localLabels = NULL_NMBRSTRING;
  nmbrString *localLabelNames = NULL_NMBRSTRING;
  nmbrString *indentationLevel = NULL_NMBRSTRING;
  nmbrString *targetHyps = NULL_NMBRSTRING;
  nmbrString *essentialFlags = NULL_NMBRSTRING;
  nmbrString *stepRenumber = NULL_NMBRSTRING;
  nmbrString *notUnifiedFlags = NULL_NMBRSTRING;
  long nextLocLabNum = 1; /* Next number to be used for a local label */
  long maxLabelLen = 0;
  long maxStepNumLen = 1;
  void *voidPtr; /* bsearch result */
  char type;
  flag stepPrintFlag;
  long fromStep, toStep, byStep;
  vstring hypStr = "";
  nmbrString *hypPtr;
  long hyp, hypStep;

  if (!pipFlag) {
    nmbrLet(&proof, wrkProof.proofString); /* The proof */
  } else {
    nmbrLet(&proof, proofInProgress.proof); /* The proof */
  }
  plen = nmbrLen(proof);
 
  /* Collect local labels */
  for (step = 0; step < plen; step++) {
    stmt = proof[step];
    if (stmt <= -1000) {
      stmt = -1000 - stmt;
      if (!nmbrElementIn(1, localLabels, stmt)) {
        nmbrLet(&localLabels, nmbrAddElement(localLabels, stmt));
      }
    }
  }

  /* localLabelNames[] hold an integer which, when converted to string,
    is the local label name. */
  nmbrLet(&localLabelNames, nmbrSpace(plen));

  /* Get the indentation level */
  nmbrLet(&indentationLevel, nmbrGetIndentation(proof, 0));
  
  /* Get the target hypotheses */
  nmbrLet(&targetHyps, nmbrGetTargetHyp(proof, statemNum));
  
  /* Get the essential step flags, if required */
  if (essentialFlag) {
    nmbrLet(&essentialFlags, nmbrGetEssential(proof));
  } else {
    nmbrLet(&essentialFlags, NULL_NMBRSTRING);
  }

  /* Get the step renumbering */
  nmbrLet(&stepRenumber, nmbrSpace(plen));
  if (renumberFlag && essentialFlag) {
    i = 0;
    maxStepNum = 0; /* To compute maxStepNumLen below */
    for (step = 0; step < plen; step++) {
      if (essentialFlags[step]) {
        i++;
        stepRenumber[step] = i; /* Number essential steps only */
        maxStepNum = i; /* To compute maxStepNumLen below */
      }
    }
  } else {
    for (step = 0; step < plen; step++) {
      stepRenumber[step] = step + 1; /* Standard numbering */
    }
    maxStepNum = plen; /* To compute maxStepNumLen below */
  }

  /* Get the printed length of the largest step number */
  i = maxStepNum;
  while (i >= 10) {
    i = i/10;
    maxStepNumLen++;
  }
      
  /* Get steps not unified (pipFlag only) */
  if (notUnifiedFlag) {
    if (!pipFlag) bug(205);
    nmbrLet(&notUnifiedFlags, nmbrSpace(plen));
    for (step = 0; step < plen; step++) {
      notUnifiedFlags[step] = 0;
      if (nmbrLen(proofInProgress.source[step])) {
        if (!nmbrEq(proofInProgress.target[step],
            proofInProgress.source[step])) notUnifiedFlags[step] = 1;
      }
      if (nmbrLen(proofInProgress.user[step])) {
        if (!nmbrEq(proofInProgress.target[step],
            proofInProgress.user[step])) notUnifiedFlags[step] = 1;
      }
    }
  }


  /* Get local labels and maximum label length */
  /* lent = target length, lens = source length */
  for (step = 0; step < plen; step++) {
    lent = strlen(statement[targetHyps[step]].labelName);
    stmt = proof[step];
    if (stmt < 0) {
      if (stmt <= -1000) {
        stmt = -1000 - stmt;
        /* stmt is now the step number a local label refers to */
        lens = strlen(str(localLabelNames[stmt]));
      } else {
        lens = 1; /* '?' */
      }
    } else {
      if (nmbrElementIn(1, localLabels, step)) {
        /* This statement declares a local label */
        /* First, get a name for the local label, using the next integer that
           does not match any integer used for a statement label. */
        let(&tmpStr1,str(nextLocLabNum));
        while (1) {
          voidPtr = (void *)bsearch(tmpStr,
              allLabelKeyBase, numAllLabelKeys,
              sizeof(long), labelSrchCmp);
          if (!voidPtr) break; /* It does not conflict */
          nextLocLabNum++; /* Try the next one */
          let(&tmpStr1, str(nextLocLabNum));
        }
        localLabelNames[step] = nextLocLabNum;
        nextLocLabNum++; /* Prepare for next local label */
      }
      lens = strlen(statement[stmt].labelName);
    }
    /* Find longest label assignment, excluding local label declaration */
    if (maxLabelLen < lent + 1 + lens) {
      maxLabelLen = lent + 1 + lens; /* Target, =, source */
    }
  } /* Next step */

  /* Print the steps */
  if (reverseFlag) {
    fromStep = plen - 1;
    toStep = -1;
    byStep = -1;
  } else {
    fromStep = 0;
    toStep = plen;
    byStep = 1;
  }
  for (step = fromStep; step != toStep; step = step + byStep) {

    /* Filters to decide whether to print the step */
    stepPrintFlag = 1;
    if (startStep > 0) { /* The user's FROM_STEP */
      if (step + 1 < startStep) stepPrintFlag = 0;
    }
    if (endStep > 0) { /* The user's TO_STEP */
      if (step + 1 > endStep) stepPrintFlag = 0;
    }
    if (endIndent > 0) { /* The user's INDENTATION_DEPTH */
      if (indentationLevel[step] + 1 > endIndent) stepPrintFlag = 0;
    }
    if (essentialFlag) {
      if (!essentialFlags[step]) stepPrintFlag = 0;
    }
    if (notUnifiedFlag) {
      if (!notUnifiedFlags[step]) stepPrintFlag = 0;
    }
    if (unknownFlag) {
      if (proof[step] != -(long)'?') stepPrintFlag = 0;
    }

    if (!stepPrintFlag) continue;

    if (noIndentFlag) {
      let(&tgtLabel, "");
    } else {
      let(&tgtLabel, statement[targetHyps[step]].labelName);
    }
    let(&locLabDecl, ""); /* Local label declaration */
    stmt = proof[step];
    if (stmt < 0) {
      if (stmt <= -1000) {
        stmt = -1000 - stmt;
        /* stmt is now the step number a local label refers to */
        if (noIndentFlag) {
          let(&srcLabel, cat("@", str(localLabelNames[stmt]), NULL));
        } else {
          let(&srcLabel, cat("=", str(localLabelNames[stmt]), NULL));
        }
        type = statement[proof[stmt]].type;
      } else {
        if (stmt != -(long)'?') bug(206);
        if (noIndentFlag) {
          let(&srcLabel, chr(-stmt)); /* '?' */
        } else {
          let(&srcLabel, cat("=", chr(-stmt), NULL)); /* '?' */
        }
        type = '?';
      }
    } else {
      if (nmbrElementIn(1, localLabels, step)) {
        /* This statement declares a local label */
        if (noIndentFlag) {
          let(&locLabDecl, cat("@", str(localLabelNames[step]), ":", NULL));
        } else {
          let(&locLabDecl, cat(str(localLabelNames[step]), ":", NULL));
        }
      }

      if (noIndentFlag) {
        let(&srcLabel, statement[stmt].labelName);

        /* For non-indented mode, add step numbers of hypotheses after label */
        let(&hypStr, "");
        hypStep = step - 1;
        hypPtr = statement[stmt].reqHypList;
        for (hyp = statement[stmt].numReqHyp - 1; hyp >=0; hyp--) {
          if (!essentialFlag || statement[hypPtr[hyp]].type == (char)e__) {
            if (!hypStr[0]) {
              let(&hypStr, str(stepRenumber[hypStep]));
            } else {
              let(&hypStr, cat(str(stepRenumber[hypStep]), ",", hypStr, NULL));
            }
          }
          if (hyp < statement[stmt].numReqHyp) {
            /* Move down to previous hypothesis */
            hypStep = hypStep - subProofLen(proof, hypStep);
          }
        } /* Next hyp */

        if (hypStr[0]) {
          /* Add hypothesis list after label */
          let(&srcLabel, cat(hypStr, " ", srcLabel, NULL));
        }

      } else {
        let(&srcLabel, cat("=", statement[stmt].labelName, NULL));
      }
      type = statement[stmt].type;
    }

    
#define PF_INDENT_INC 2
    /* Print the proof line */
    if (stepPrintFlag) {

      if (noIndentFlag) {
        let(&startPrefix, cat(
            space(maxStepNumLen - strlen(str(stepRenumber[step]))),
            str(stepRenumber[step]),
            " ",
            srcLabel,
            space(splitColumn - strlen(srcLabel) - strlen(locLabDecl) - 1
                - maxStepNumLen - 1),
            " ", locLabDecl,
            NULL));
        if (pipFlag) {
          let(&tgtPrefix, startPrefix);
          let(&srcPrefix, cat(
              space(maxStepNumLen - strlen(str(stepRenumber[step]))),
              space(strlen(str(stepRenumber[step]))),
              " ",
              space(splitColumn - 1
                  - maxStepNumLen),
              NULL));
          let(&userPrefix, cat(
              space(maxStepNumLen - strlen(str(stepRenumber[step]))),
              space(strlen(str(stepRenumber[step]))),
              " ",
              "(User)",
              space(splitColumn - strlen("(User)") - 1
                  - maxStepNumLen),
              NULL));
        }
        let(&contPrefix, space(strlen(startPrefix) + 4));
      } else {
        let(&startPrefix, cat(
            space(maxStepNumLen - strlen(str(stepRenumber[step]))),
            str(stepRenumber[step]),
            " ",
            space(indentationLevel[step] * PF_INDENT_INC - strlen(locLabDecl)),
            locLabDecl,
            tgtLabel,
            srcLabel,
            space(maxLabelLen - strlen(tgtLabel) - strlen(srcLabel)),
            NULL));
        if (pipFlag) {
          let(&tgtPrefix, cat(
              space(maxStepNumLen - strlen(str(stepRenumber[step]))),
              str(stepRenumber[step]),
              " ",
              space(indentationLevel[step] * PF_INDENT_INC - strlen(locLabDecl)),
              locLabDecl,
              tgtLabel,
              space(strlen(srcLabel)),
              space(maxLabelLen - strlen(tgtLabel) - strlen(srcLabel)),
              NULL));
          let(&srcPrefix, cat(
              space(maxStepNumLen - strlen(str(stepRenumber[step]))),
              space(strlen(str(stepRenumber[step]))),
              " ",
              space(indentationLevel[step] * PF_INDENT_INC - strlen(locLabDecl)),
              space(strlen(locLabDecl)),
              space(strlen(tgtLabel)),
              srcLabel,
              space(maxLabelLen - strlen(tgtLabel) - strlen(srcLabel)),
              NULL));
          let(&userPrefix, cat(
              space(maxStepNumLen - strlen(str(stepRenumber[step]))),
              space(strlen(str(stepRenumber[step]))),
              " ",
              space(indentationLevel[step] * PF_INDENT_INC - strlen(locLabDecl)),
              space(strlen(locLabDecl)),
              space(strlen(tgtLabel)),
              "=(User)",
              space(maxLabelLen - strlen(tgtLabel) - strlen("=(User)")),
              NULL));
        }
        /*
        let(&contPrefix, space(maxStepNumLen + 1 + indentationLevel[step] *
            PF_INDENT_INC
            + maxLabelLen + 4));
        */
        let(&contPrefix, ""); /* Continuation lines use whole screen width */
      }

      if (!pipFlag) {

        if (!texFlag) {
          printLongLine(cat(startPrefix," $", chr(type), " ",
              nmbrCvtMToVString(wrkProof.mathStringPtrs[step]),
              NULL),
              contPrefix,
              chr(1)); /* chr(1) is right-justify flag for printLongLine */
        } else {
          printTexLongMath(wrkProof.mathStringPtrs[step],
              cat(startPrefix, " $", chr(type), " ", NULL),
              contPrefix);
        }

      } else { /* pipFlag */

        if (!nmbrEq(proofInProgress.target[step], proofInProgress.source[step])
            && nmbrLen(proofInProgress.source[step])) {

          if (!texFlag) {
            printLongLine(cat(tgtPrefix, " $", chr(type), " ",
                nmbrCvtMToVString(proofInProgress.target[step]),
                NULL),
                contPrefix,
                chr(1)); /* chr(1) is right-justify flag for printLongLine */
            printLongLine(cat(srcPrefix,"  = ",
                nmbrCvtMToVString(proofInProgress.source[step]),
                NULL),
                contPrefix,
                chr(1)); /* chr(1) is right-justify flag for printLongLine */
          } else {
            printTexLongMath(proofInProgress.target[step],
                cat(tgtPrefix, " $", chr(type), " ", NULL),
                contPrefix);
            printTexLongMath(proofInProgress.source[step],
                cat(srcPrefix, "  = ", NULL),
                contPrefix);
          }
        } else {
          if (!texFlag) {
            printLongLine(cat(startPrefix, " $", chr(type), " ",
                nmbrCvtMToVString(proofInProgress.target[step]),
                NULL),
                contPrefix,
                chr(1)); /* chr(1) is right-justify flag for printLongLine */
          } else {
            printTexLongMath(proofInProgress.target[step],
                cat(startPrefix, " $", chr(type), " ", NULL),
                contPrefix);
          }

        }
        if (nmbrLen(proofInProgress.user[step])) {

          if (!texFlag) {
            printLongLine(cat(userPrefix, "  = ",
                nmbrCvtMToVString(proofInProgress.user[step]),
                NULL),
                contPrefix,
                chr(1)); /* chr(1) is right-justify flag for printLongLine */
          } else {
            printTexLongMath(proofInProgress.user[step],
                cat(userPrefix, "  = ", NULL),
                contPrefix);
          }

        }
      }
    }
        
    
  } /* Next step */

  let(&tmpStr, "");
  let(&tmpStr1, "");
  let(&locLabDecl, "");
  let(&tgtLabel, "");
  let(&srcLabel, "");
  let(&startPrefix, "");
  let(&tgtPrefix, "");
  let(&srcPrefix, "");
  let(&userPrefix, "");
  let(&contPrefix, "");
  let(&hypStr, "");
  nmbrLet(&localLabels, NULL_NMBRSTRING);
  nmbrLet(&localLabelNames, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&targetHyps, NULL_NMBRSTRING);
  nmbrLet(&indentationLevel, NULL_NMBRSTRING);
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);
  nmbrLet(&stepRenumber, NULL_NMBRSTRING);
  nmbrLet(&notUnifiedFlags, NULL_NMBRSTRING);

}

/* Show details of one proof step */
/* Note:  detailStep is the actual step number (starting with 1), not
   the actual step - 1. */
void showDetailStep(long statemNum, long detailStep) {

  long i, j, k, plen, maxLocalLen, step, stmt, sourceStmt, targetStmt;
  vstring proofStr = "";
  vstring tmpStr = "";
  vstring tmpStr1 = "";
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *localLabels = NULL_NMBRSTRING;
  nmbrString *localLabelNames = NULL_NMBRSTRING;
  nmbrString *targetHyps = NULL_NMBRSTRING;
  long nextLocLabNum = 1; /* Next number to be used for a local label */
  void *voidPtr; /* bsearch result */
  char type;

  /* Error check */
  i = parseProof(statemNum);
  if (i) {
    printLongLine("?The proof is incomplete or has an error", "", " ");
    return;
  }
  plen = nmbrLen(wrkProof.proofString);
  if (plen < detailStep || detailStep < 1) {
    printLongLine(cat("?The step number should be from 1 to ",
        str(plen), NULL), "", " ");
    return;
  }
 
  /* Structure getStep is declared in mmveri.h. */
  getStep.stepNum = detailStep; /* Non-zero is flag for verifyProof */
  parseProof(statemNum); /* ???Do we need to do this again? */
  verifyProof(statemNum);


  nmbrLet(&proof, wrkProof.proofString); /* The proof */
  plen = nmbrLen(proof);

  /* Collect local labels */
  for (step = 0; step < plen; step++) {
    stmt = proof[step];
    if (stmt <= -1000) {
      stmt = -1000 - stmt;
      if (!nmbrElementIn(1, localLabels, stmt)) {
        nmbrLet(&localLabels, nmbrAddElement(localLabels, stmt));
      }
    }
  }

  /* localLabelNames[] hold an integer which, when converted to string,
    is the local label name. */
  nmbrLet(&localLabelNames, nmbrSpace(plen));

  /* Get the target hypotheses */
  nmbrLet(&targetHyps, nmbrGetTargetHyp(proof, statemNum));
  
  /* Get local labels */
  for (step = 0; step < plen; step++) {
    stmt = proof[step];
    if (stmt >= 0) {
      if (nmbrElementIn(1, localLabels, step)) {
        /* This statement declares a local label */
        /* First, get a name for the local label, using the next integer that
           does not match any integer used for a statement label. */
        let(&tmpStr1,str(nextLocLabNum));
        while (1) {
          voidPtr = (void *)bsearch(tmpStr,
              allLabelKeyBase, numAllLabelKeys,
              sizeof(long), labelSrchCmp);
          if (!voidPtr) break; /* It does not conflict */
          nextLocLabNum++; /* Try the next one */
          let(&tmpStr1,str(nextLocLabNum));
        }
        localLabelNames[step] = nextLocLabNum;
        nextLocLabNum++; /* Prepare for next local label */
      }
    }
  } /* Next step */

  /* Print the step */
  let(&tmpStr, statement[targetHyps[detailStep - 1]].labelName);
  let(&tmpStr1, ""); /* Local label declaration */
  stmt = proof[detailStep - 1];
  if (stmt < 0) {
    if (stmt <= -1000) {
      stmt = -1000 - stmt;
      /* stmt is now the step number a local label refers to */
      let(&tmpStr, cat(tmpStr,"=",str(localLabelNames[stmt]), NULL));
      type = statement[proof[stmt]].type;
    } else {
      if (stmt != -(long)'?') bug(207);
      let(&tmpStr, cat(tmpStr,"=",chr(-stmt), NULL)); /* '?' */
      type = '?';
    }
  } else {
    if (nmbrElementIn(1, localLabels, detailStep - 1)) {
      /* This statement declares a local label */
      let(&tmpStr1, cat(str(localLabelNames[detailStep - 1]), ":",
          NULL));
    }
    let(&tmpStr, cat(tmpStr, "=", statement[stmt].labelName, NULL));
    type = statement[stmt].type;
  }
    
  /* Print the proof line */
  printLongLine(cat("Proof step ",
      str(detailStep),
      ":  ",
      tmpStr1,
      tmpStr,
      " $",
      chr(type),
      " ",
      nmbrCvtMToVString(wrkProof.mathStringPtrs[detailStep - 1]),
      NULL),
      "  ",
      " ");

  /* Print details about the step */
  let(&tmpStr, cat("This step assigns ", NULL));
  let(&tmpStr1, "");
  stmt = proof[detailStep - 1];
  sourceStmt = stmt;
  if (stmt < 0) {
    if (stmt <= -1000) {
      stmt = -1000 - stmt;
      /* stmt is now the step number a local label refers to */
      let(&tmpStr, cat(tmpStr, "step ", str(stmt),
          " (via local label reference \"",
          str(localLabelNames[stmt]), "\") to ", NULL));
    } else {
      if (stmt != -(long)'?') bug(208);
      let(&tmpStr, cat(tmpStr, "an unknown statement to ", NULL));
    }
  } else {
    let(&tmpStr, cat(tmpStr, "source \"", statement[stmt].labelName,
        "\" ($", chr(statement[stmt].type), ") to ", NULL));
    if (nmbrElementIn(1, localLabels, detailStep - 1)) {
      /* This statement declares a local label */
      let(&tmpStr1, cat("  This step also declares the local label ",
          str(localLabelNames[detailStep - 1]),
          ", which is used later on.",
          NULL));
    }
  }
  targetStmt = targetHyps[detailStep - 1];
  if (detailStep == plen) {
    let(&tmpStr, cat(tmpStr, "the final assertion being proved.", NULL));
  } else {
    let(&tmpStr, cat(tmpStr, "target \"", statement[targetStmt].labelName,
    "\" ($", chr(statement[targetStmt].type), ").", NULL));
  }

  let(&tmpStr, cat(tmpStr, tmpStr1, NULL));

  if (sourceStmt >= 0) {
    if (statement[sourceStmt].type == a__
        || statement[sourceStmt].type == p__) {
      j = nmbrLen(statement[sourceStmt].reqHypList);
      if (j != nmbrLen(getStep.sourceHyps)) bug(209);
      if (!j) {
        let(&tmpStr, cat(tmpStr,
            "  The source assertion requires no hypotheses.", NULL));
      } else {
        if (j == 1) {
          let(&tmpStr, cat(tmpStr,
              "  The source assertion requires the hypothesis ", NULL));
        } else {
          let(&tmpStr, cat(tmpStr,
              "  The source assertion requires the hypotheses ", NULL));
        }
        for (i = 0; i < j; i++) {
          let(&tmpStr, cat(tmpStr, "\"",
              statement[statement[sourceStmt].reqHypList[i]].labelName,
              "\" ($",
              chr(statement[statement[sourceStmt].reqHypList[i]].type),
              ", step ", str(getStep.sourceHyps[i] + 1), ")", NULL));
          if (i == 0 && j == 2) {
            let(&tmpStr, cat(tmpStr, " and ", NULL));
          }
          if (i < j - 2 && j > 2) {
            let(&tmpStr, cat(tmpStr, ", ", NULL));
          }
          if (i == j - 2 && j > 2) {
            let(&tmpStr, cat(tmpStr, ", and ", NULL));
          }
        }
        let(&tmpStr, cat(tmpStr, ".", NULL));
      }
    }
  }  

  if (detailStep < plen) {
    let(&tmpStr, cat(tmpStr,
         "  The parent assertion of the target hypothesis is \"",
        statement[getStep.targetParentStmt].labelName, "\" ($",
        chr(statement[getStep.targetParentStmt].type),", step ",
        str(getStep.targetParentStep), ").", NULL));
  } else {
    let(&tmpStr, cat(tmpStr,
        "  The target has no parent because it is the assertion being proved.",
        NULL));
  }

  printLongLine(tmpStr, "", " ");

  if (sourceStmt >= 0) {
    if (statement[sourceStmt].type == a__
        || statement[sourceStmt].type == p__) {
      print2("The source assertion before substitution was:\n");
      printLongLine(cat("    ", statement[sourceStmt].labelName, " $",
          chr(statement[sourceStmt].type), " ", nmbrCvtMToVString(
          statement[sourceStmt].mathString), NULL),
          "        ", " ");
      j = nmbrLen(getStep.sourceSubstsNmbr);
      if (j == 1) {
        printLongLine(cat(
            "The following substitution was made to the source assertion:",
            NULL),""," ");
      } else {
        printLongLine(cat(
            "The following substitutions were made to the source assertion:",
            NULL),""," ");
      }
      if (!j) {
        print2("    (None)\n");
      } else {
        print2("    Variable  Substituted with\n");
        for (i = 0; i < j; i++) {
          printLongLine(cat("     ",
              mathToken[getStep.sourceSubstsNmbr[i]].tokenName," ",
              space(9 - strlen(
                mathToken[getStep.sourceSubstsNmbr[i]].tokenName)),
              nmbrCvtMToVString(getStep.sourceSubstsPntr[i]), NULL),
              "                ", " ");
        }
      }
    }
  }

  if (detailStep < plen) {
    print2("The target hypothesis before substitution was:\n");
    printLongLine(cat("    ", statement[targetStmt].labelName, " $",
        chr(statement[targetStmt].type), " ", nmbrCvtMToVString(
        statement[targetStmt].mathString), NULL),
        "        ", " ");
    j = nmbrLen(getStep.targetSubstsNmbr);
    if (j == 1) {
      printLongLine(cat(
          "The following substitution was made to the target hypothesis:",
          NULL),""," ");
    } else {
      printLongLine(cat(
          "The following substitutions were made to the target hypothesis:",
          NULL),""," ");
    }
    if (!j) {
      print2("    (None)\n");
    } else {
      print2("    Variable  Substituted with\n");
      for (i = 0; i < j; i++) {
        printLongLine(cat("     ",
            mathToken[getStep.targetSubstsNmbr[i]].tokenName, " ",
            space(9 - strlen(
              mathToken[getStep.targetSubstsNmbr[i]].tokenName)),
            nmbrCvtMToVString(getStep.targetSubstsPntr[i]), NULL),
            "                ", " ");
      }
    }
  }

  cleanWrkProof();
  getStep.stepNum = 0; /* Zero is flag for verifyProof to ignore getStep info */

  /* Deallocate getStep contents */
  j = pntrLen(getStep.sourceSubstsPntr);
  for (i = 0; i < j; i++) {
    nmbrLet((nmbrString **)(&getStep.sourceSubstsPntr[i]),
        NULL_NMBRSTRING);
  }
  j = pntrLen(getStep.targetSubstsPntr);
  for (i = 0; i < j; i++) {
    nmbrLet((nmbrString **)(&getStep.targetSubstsPntr[i]),
        NULL_NMBRSTRING);
  }
  nmbrLet(&getStep.sourceHyps, NULL_NMBRSTRING);
  pntrLet(&getStep.sourceSubstsPntr, NULL_PNTRSTRING);
  nmbrLet(&getStep.sourceSubstsNmbr, NULL_NMBRSTRING);
  pntrLet(&getStep.targetSubstsPntr, NULL_PNTRSTRING);
  nmbrLet(&getStep.targetSubstsNmbr, NULL_NMBRSTRING);

  /* Deallocate other strings */
  let(&tmpStr, "");
  let(&tmpStr1, "");
  nmbrLet(&localLabels, NULL_NMBRSTRING);
  nmbrLet(&localLabelNames, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&targetHyps, NULL_NMBRSTRING);

}

/* Summary of statements in proof */
void proofStmtSumm(long statemNum, flag essentialFlag, flag texFlag) {

  long i, j, k, pos, stmt, plen, slen, step;
  char type;
  vstring statementUsedFlags = ""; /* 'y'/'n' flag that statement is used */
  vstring str1 = "";
  vstring str2 = "";
  vstring str3 = "";
  nmbrString *statementList = NULL_NMBRSTRING;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *essentialFlags = NULL_NMBRSTRING;

  if (!texFlag) {
    print2("Summary of statements used in the proof of \"%s\":\n",
        statement[statemNum].labelName);
  } else {
      outputToString = 1; /* Flag for print2 to add to printString */
      print2("\n");
      print2("\\vspace{1ex}\n");
      printLongLine(cat("Summary of statements used in the proof of ",
          "{\\tt ",
          asciiToTt(statement[statemNum].labelName),
          "}:", NULL), "", " ");
      outputToString = 0;
      fprintf(texFilePtr, "%s", printString);
      let(&printString, "");
  }
  
  if (statement[statemNum].type != p__) {
    print2("  This is not a provable ($p) statement.\n");
    return;
  }

  parseProof(statemNum);
  nmbrLet(&proof, wrkProof.proofString); /* The proof */
  plen = nmbrLen(proof);
  /* Get the essential step flags, if required */
  if (essentialFlag) {
    nmbrLet(&essentialFlags, nmbrGetEssential(proof));
  }

  for (step = 0; step < plen; step++) {
    if (essentialFlag) {
      if (!essentialFlags[step]) continue;     /* Ignore floating hypotheses */
    }
    stmt = proof[step];
    if (stmt < 0) {
      continue; /* Ignore '?' and local labels */
    }
    if (1) { /* Limit list to $a and $p only */
      if (statement[stmt].type != a__ && statement[stmt].type != p__) {
        continue;
      }
    }
    /* Add this statement to the statement list if not already in it */
    if (!nmbrElementIn(1, statementList, stmt)) {
      nmbrLet(&statementList, nmbrAddElement(statementList, stmt));
    }
  } /* Next step */

  /* Prepare the output */
  /* First, fill in the statementUsedFlags char array.  This allows us to sort
     the output by statement number without calling a sort routine. */
  slen = nmbrLen(statementList);
  let(&statementUsedFlags, string(statements + 1, 'n')); /* Init. to 'no' */
  for (pos = 0; pos < slen; pos++) {
    stmt = statementList[pos];
    if (stmt > statemNum || stmt < 1) bug(210);
    statementUsedFlags[stmt] = 'y';
  }
  /* Next, build the output string */
  for (stmt = 1; stmt < statemNum; stmt++) {
    if (statementUsedFlags[stmt] == 'y') {

      let(&str1, cat(" is located on line ", str(statement[stmt].lineNum),
          " of the file ", NULL));
      if (!texFlag) {
        print2("\n");
        printLongLine(cat("Statement ", statement[stmt].labelName, str1,
          "\"", statement[stmt].fileName,
          "\".",NULL), "", " ");
      } else {
        outputToString = 1; /* Flag for print2 to add to printString */
        print2("\n");
        print2("\n");
        print2("\\vspace{1ex}\n");
        printLongLine(cat("Statement {\\tt ",
            asciiToTt(statement[stmt].labelName), "} ",
            str1, "{\\tt ",
            asciiToTt(statement[stmt].fileName),
            "}.", NULL), "", " ");
        print2("\n");
        outputToString = 0;
        fprintf(texFilePtr, "%s", printString);
        let(&printString, "");
      }

      type = statement[stmt].type;
      let(&str1, "");
      str1 = getDescription(stmt);
      if (str1[0]) {
        if (!texFlag) {
          printLongLine(cat("\"", str1, "\"", NULL), "", " ");
        } else {
          printTexComment(str1);
        }
      }

      /* print2("Its mandatory hypotheses in RPN order are:\n"); */
      j = nmbrLen(statement[stmt].reqHypList);
      for (i = 0; i < j; i++) {
        k = statement[stmt].reqHypList[i];
        if (!essentialFlag || statement[k].type != f__) {
          let(&str2, cat("  ",statement[k].labelName,
              " $", chr(statement[k].type), " ", NULL));
          if (!texFlag) {
            printLongLine(cat(str2,
                nmbrCvtMToVString(statement[k].mathString), " $.", NULL),
                "      "," ");
          } else {
            let(&str3, space(strlen(str2)));
            printTexLongMath(statement[k].mathString,
                str2, str3);
          }
        }
      }

      let(&str1, "");
      type = statement[stmt].type;
      if (type == p__) let(&str1, " $= ...");
      let(&str2, cat("  ", statement[stmt].labelName,
          " $",chr(type), " ", NULL));
      if (!texFlag) {
        printLongLine(cat(str2,
            nmbrCvtMToVString(statement[stmt].mathString),
            str1, " $.", NULL), "      ", " ");
      } else {
        let(&str3, space(strlen(str2)));
        printTexLongMath(statement[stmt].mathString,
            str2, str3);
      }

    } /* End if (statementUsedFlag[stmt] == 'y') */
  } /* Next stmt */

  let(&statementUsedFlags, ""); /* 'y'/'n' flag that statement is used */
  let(&str1, "");
  let(&str2, "");
  let(&str3, "");
  nmbrLet(&statementList, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);

}


/* Traces back the statements used by a proof, recursively. */
void traceProof(long statemNum,
  flag essentialFlag,
  flag axiomFlag)
{

  long pos, stmt, plen, slen, step;
  vstring statementUsedFlags = ""; /* 'y'/'n' flag that statement is used */
  vstring outputString = "";
  nmbrString *statementList = NULL_NMBRSTRING;
  nmbrString *unprovedList = NULL_NMBRSTRING;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *essentialFlags = NULL_NMBRSTRING;
  
  if (axiomFlag) {
    print2(
"Statement \"%s\" assumes the following axioms ($a statements):\n",
        statement[statemNum].labelName);
  } else {
    print2(
"The proof of statement \"%s\" uses the following earlier statements:\n",
        statement[statemNum].labelName);
  }
  
  nmbrLet(&statementList, nmbrAddElement(statementList, statemNum)); /* Init. */
  nmbrLet(&unprovedList, NULL_NMBRSTRING); /* List of unproved statements */
  
  for (pos = 0; pos < nmbrLen(statementList); pos++) {
    if (statement[statementList[pos]].type != p__) {
      continue; /* Not a $p */
    }
    parseProof(statementList[pos]);
    nmbrLet(&proof, wrkProof.proofString); /* The proof */
    plen = nmbrLen(proof);
    /* Get the essential step flags, if required */
    if (essentialFlag) {
      nmbrLet(&essentialFlags, nmbrGetEssential(proof));
    }
    for (step = 0; step < plen; step++) {
      if (essentialFlag) {
        if (!essentialFlags[step]) continue;
                                                /* Ignore floating hypotheses */
      }
      stmt = proof[step];
      if (stmt < 0) {
        if (stmt > -1000) {
          /* '?' */
          if (!nmbrElementIn(1, unprovedList, statementList[pos])) {
            nmbrLet(&unprovedList, nmbrAddElement(unprovedList,
                statementList[pos]));
                                        /* Add to list of unproved statements */
          }
        }
        continue; /* Ignore '?' and local labels */
      }
      if (1) { /* Limit list to $a and $p only */
        if (statement[stmt].type != a__ && statement[stmt].type != p__) {
          continue;
        }
      }
      /* Add this statement to the statement list if not already in it */
      if (!nmbrElementIn(1, statementList, stmt)) {
        nmbrLet(&statementList, nmbrAddElement(statementList, stmt));
      }
    } /* Next step */
  } /* Next pos */
    
  
  /* Prepare the output */
  /* First, fill in the statementUsedFlags char array.  This allows us to sort
     the output by statement number without calling a sort routine. */
  slen = nmbrLen(statementList);
  let(&statementUsedFlags, string(statements + 1, 'n')); /* Init. to 'no' */
  for (pos = 1; pos < slen; pos++) { /* Start with 1 (ignore traced statement)*/
    stmt = statementList[pos];
    if (stmt > statemNum || stmt < 1) bug(211);
    statementUsedFlags[stmt] = 'y';
  }
  /* Next, build the output string */
  let(&outputString, "");
  for (stmt = 1; stmt < statemNum; stmt++) {
    if (statementUsedFlags[stmt] == 'y') {
      if (axiomFlag) {
        if (statement[stmt].type == a__) {
          let(&outputString, cat(outputString, " ", statement[stmt].labelName,
              NULL));
        }
      } else {
        let(&outputString, cat(outputString, " ", statement[stmt].labelName,
            NULL));
        switch (statement[stmt].type) {
          case a__: let(&outputString, cat(outputString, "($a)", NULL)); break;
          case e__: let(&outputString, cat(outputString, "($e)", NULL)); break;
          case f__: let(&outputString, cat(outputString, "($f)", NULL)); break;
        }
      }
    } /* End if (statementUsedFlag[stmt] == 'y') */
  } /* Next stmt */
  if (outputString[0]) {
    let(&outputString, cat(" ", outputString, NULL));
  } else {
    let(&outputString, "  (None)");
  }

  /* Print the output */
  printLongLine(outputString, "  ", " ");
  
  /* Print any unproved statements */
  if (nmbrLen(unprovedList)) {
    print2("Warning:  the following traced statements were not proved:\n");
    let(&outputString, "");
    for (pos = 0; pos < nmbrLen(unprovedList); pos++) {
      let(&outputString, cat(outputString, " ", statement[unprovedList[
          pos]].labelName, NULL));
    }
    let(&outputString, cat("  ", outputString, NULL));
    printLongLine(outputString, "  ", " ");
  }
  
  /* Deallocate */
  let(&outputString, "");
  let(&statementUsedFlags, "");
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);
  nmbrLet(&statementList, NULL_NMBRSTRING);
  nmbrLet(&unprovedList, NULL_NMBRSTRING);

}

nmbrString *stmtFoundList = NULL_NMBRSTRING;
long indentShift = 0;

/* Traces back the statements used by a proof, recursively, with tree display.*/
void traceProofTree(long statemNum,
  flag essentialFlag, long endIndent)
{
  if (statement[statemNum].type != p__) {
    print2("Statement %s is not a $p statement.\n",
        statement[statemNum].labelName);
    return;
  }

  printLongLine(cat("The proof tree traceback for statement \"",
      statement[statemNum].labelName,
      "\" follows.  The statements used by each proof are indented one level in,",
      " below the statement being proved.  Hypotheses are not included.",
      NULL),
      "", " ");
  print2("\n");

  nmbrLet(&stmtFoundList, NULL_NMBRSTRING);
  indentShift = 0;
  traceProofTreeRec(statemNum, essentialFlag, endIndent, 0);
  nmbrLet(&stmtFoundList, NULL_NMBRSTRING);
}


void traceProofTreeRec(long statemNum,
  flag essentialFlag, long endIndent, long recursDepth)
{
  long i, j, pos, stmt, plen, slen, step;
  vstring outputStr = "";
  nmbrString *localFoundList = NULL_NMBRSTRING;
  nmbrString *localPrintedList = NULL_NMBRSTRING;
  flag unprovedFlag = 0;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *essentialFlags = NULL_NMBRSTRING;

  
  let(&outputStr, "");
  outputStr = getDescription(statemNum); /* Get statement comment */
  let(&outputStr, edit(outputStr, 8 + 16 + 128)); /* Trim and reduce spaces */
  slen = len(outputStr);
  for (i = 0; i < slen; i++) { 
    /* Change newlines to spaces in comment */
    if (outputStr[i] == '\n') {
      outputStr[i] = ' ';
    }
  }

#define INDENT_INCR 3

  if ((recursDepth * INDENT_INCR - indentShift) > 50) {
    indentShift = indentShift + 40;
    print2("****** Shifting indentation.  Total shift is now %ld.\n",
      (long)indentShift);
  }
  if ((recursDepth * INDENT_INCR - indentShift) < 1 && indentShift != 0) {
    indentShift = indentShift - 40;
    print2("****** Shifting indentation.  Total shift is now %ld.\n",
      (long)indentShift);
  }

  let(&outputStr, cat(space(recursDepth * INDENT_INCR - indentShift),
      statement[statemNum].labelName, " $", chr(statement[statemNum].type),
      "  \"", edit(outputStr, 8 + 128), "\"", NULL));

#define MAX_LINE_LEN 79
  if (len(outputStr) > MAX_LINE_LEN) {
    let(&outputStr, cat(left(outputStr, MAX_LINE_LEN - 3), "...", NULL));
  }

  if (statement[statemNum].type == p__ || statement[statemNum].type == a__) {
    /* Only print assertions to reduce output bulk */
    print2("%s\n", outputStr);
  }
  
  if (statement[statemNum].type != p__) {
    let(&outputStr, "");
    return;
  }

  if (endIndent) {
    /* An indentation level limit is set */
    if (endIndent < recursDepth + 2) {
      let(&outputStr, "");
      return;
    }
  }

  parseProof(statemNum);
  nmbrLet(&proof, wrkProof.proofString); /* The proof */
  plen = nmbrLen(proof);
  /* Get the essential step flags, if required */
  if (essentialFlag) {
    nmbrLet(&essentialFlags, nmbrGetEssential(proof));
  }
  nmbrLet(&localFoundList, NULL_NMBRSTRING);
  nmbrLet(&localPrintedList, NULL_NMBRSTRING);
  for (step = 0; step < plen; step++) {
    if (essentialFlag) {
      if (!essentialFlags[step]) continue;
                                                /* Ignore floating hypotheses */
    }
    stmt = proof[step];
    if (stmt < 0) {
      if (stmt > -1000) {
        /* '?' */
        unprovedFlag = 1;
      }
      continue; /* Ignore '?' and local labels */
    }
    if (!nmbrElementIn(1, localFoundList, stmt)) {
      nmbrLet(&localFoundList, nmbrAddElement(localFoundList, stmt));
    }
    if (!nmbrElementIn(1, stmtFoundList, stmt)) {
      traceProofTreeRec(stmt, essentialFlag, endIndent, recursDepth + 1);
      nmbrLet(&localPrintedList, nmbrAddElement(localPrintedList, stmt));
      nmbrLet(&stmtFoundList, nmbrAddElement(stmtFoundList, stmt));
    }
  } /* Next step */

  /* See if there are any old statements printed previously */
  slen = nmbrLen(localFoundList);
  let(&outputStr, "");
  for (pos = 0; pos < slen; pos++) {
    stmt = localFoundList[pos];
    if (!nmbrElementIn(1, localPrintedList, stmt)) {
      /* Don't include $f, $e in output */
      if (statement[stmt].type == p__ || statement[stmt].type == a__) {
        let(&outputStr, cat(outputStr, " ", 
            statement[stmt].labelName, NULL));
      }
    }
  }

  if (len(outputStr)) {
    printLongLine(cat(space(INDENT_INCR * (recursDepth + 1) - 1 - indentShift),
      outputStr, " (shown above)", NULL),
      space(INDENT_INCR * (recursDepth + 2) - indentShift), " ");
  }

  if (unprovedFlag) {
    printLongLine(cat(space(INDENT_INCR * (recursDepth + 1) - indentShift),
      "*** Statement ", statement[statemNum].labelName, " has not been proved."
      , NULL),
      space(INDENT_INCR * (recursDepth + 2)), " ");
  }
  
  let(&outputStr, "");
  nmbrLet(&localFoundList, NULL_NMBRSTRING);
  nmbrLet(&localPrintedList, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);

}


/* Counts the number of steps a completely exploded proof would require */
/* (Recursive) */
/* 0 is returned if some assertions have incomplete proofs. */
double countSteps(long statemNum, flag essentialFlag)
{
  static double *stmtCount;
  static double *stmtNodeCount;
  static long *stmtDist;
  static long *stmtMaxPath;
  static double *stmtAveDist;
  static long *stmtProofLen; /* The actual number of steps in stmt's proof */
  static long *stmtUsage; /* The number of times the statement is used */
  static long level = 0;
  static flag unprovedFlag;
  
  long stmt, plen, step, i, j, k;
  long essentialplen;
  nmbrString *proof = NULL_NMBRSTRING;
  double stepCount;
  double stepNodeCount;
  double stepDistSum;
  nmbrString *essentialFlags = NULL_NMBRSTRING;
  vstring tmpStr = "";
  long actualSteps, actualSubTheorems;
  long actualSteps2, actualSubTheorems2;
  
  /* If this is the top level of recursion, initialize things */
  if (!level) {
    stmtCount = malloc(sizeof(double) * (statements + 1));
    stmtNodeCount = malloc(sizeof(double) * (statements + 1));
    stmtDist = malloc(sizeof(long) * (statements + 1));
    stmtMaxPath = malloc(sizeof(long) * (statements + 1));
    stmtAveDist = malloc(sizeof(double) * (statements + 1));
    stmtProofLen = malloc(sizeof(long) * (statements + 1));
    stmtUsage = malloc(sizeof(long) * (statements + 1));
    if (!stmtCount || !stmtNodeCount || !stmtDist || !stmtMaxPath ||
        !stmtAveDist || !stmtProofLen || !stmtUsage) {
      print2("?Memory overflow.  Step count will be wrong.\n");
      if (stmtCount) free(stmtCount);
      if (stmtNodeCount) free(stmtNodeCount);
      if (stmtDist) free(stmtDist);
      if (stmtMaxPath) free(stmtMaxPath);
      if (stmtAveDist) free(stmtAveDist);
      if (stmtProofLen) free(stmtProofLen);
      if (stmtUsage) free(stmtUsage);
      return (0);
    }
    for (stmt = 1; stmt < statements + 1; stmt++) {
      stmtCount[stmt] = 0;
      stmtUsage[stmt] = 0;
    }
    unprovedFlag = 0; /* Flag that some proof wasn't complete */
  }
  level++;
  stepCount = 0;
  stepNodeCount = 0;
  stepDistSum = 0;
  stmtDist[statemNum] = -2; /* Forces at least one assignment */
  
  if (statement[statemNum].type != (char)p__) {
    /* $a, $e, or $f */
    stepCount = 1;
    stepNodeCount = 0;
    stmtDist[statemNum] = 0;
    goto returnPoint;
  }
  
  parseProof(statemNum);
  nmbrLet(&proof, nmbrUnsquishProof(wrkProof.proofString)); /* The proof */
  plen = nmbrLen(proof);
  /* Get the essential step flags, if required */
  if (essentialFlag) {
    nmbrLet(&essentialFlags, nmbrGetEssential(proof));
  }
  essentialplen = 0;
  for (step = 0; step < plen; step++) {
    if (essentialFlag) {
      if (!essentialFlags[step]) continue;     /* Ignore floating hypotheses */
    }
    essentialplen++;
    stmt = proof[step];
    if (stmt < 0) {
      if (stmt <= -1000) {
        bug(215); /* The proof was expanded; there should be no local labels */
      } else {
        /* '?' */
        unprovedFlag = 1;
        stepCount = stepCount + 1;
        stepNodeCount = stepNodeCount + 1;
        stepDistSum = stepDistSum + 1;
      }
    } else {    
      if (stmtCount[stmt] == 0) {
        /* It has not been computed yet */
        stepCount = stepCount + countSteps(stmt, essentialFlag);
      } else {
        /* It has already been computed */
        stepCount = stepCount + stmtCount[stmt];
      }
      if (statement[stmt].type == (char)p__) {
        /*stepCount--;*/ /* -1 to account for the replacement of this step */
        for (j = 0; j < statement[stmt].numReqHyp; j++) {
          k = statement[stmt].reqHypList[j];
          if (!essentialFlag || statement[k].type == (char)e__) {
            stepCount--;
          }
        }
      }
      stmtUsage[stmt]++;
      if (stmtDist[statemNum] < stmtDist[stmt] + 1) {
        stmtDist[statemNum] = stmtDist[stmt] + 1;
        stmtMaxPath[statemNum] = stmt;
      }
      stepNodeCount = stepNodeCount + stmtNodeCount[stmt];
      stepDistSum = stepDistSum + stmtAveDist[stmt] + 1;
    }
    
  } /* Next step */

 returnPoint:

  /* Assign step count to statement list */
  stmtCount[statemNum] = stepCount;
  stmtNodeCount[statemNum] = stepNodeCount + 1;
  stmtAveDist[statemNum] = stepDistSum / essentialplen;
  stmtProofLen[statemNum] = essentialplen;
  
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);
  
  level--;
  /* If this is the top level of recursion, deallocate */
  if (!level) {
    if (unprovedFlag) stepCount = 0; /* Don't mislead user */

    /* Compute the total actual steps, total actual subtheorems */
    actualSteps = stmtProofLen[statemNum];
    actualSubTheorems = 0;
    actualSteps2 = actualSteps; /* Steps w/ single-use subtheorems eliminated */
    actualSubTheorems2 = 0; /* Multiple-use subtheorems only */
    for (i = 1; i < statemNum; i++) {
      if (statement[i].type == (char)p__ && stmtCount[i] != 0) {
        actualSteps = actualSteps + stmtProofLen[i];
        actualSubTheorems++;
        if (stmtUsage[i] > 1) {
          actualSubTheorems2++;
          actualSteps2 = actualSteps2 + stmtProofLen[i];
        } else {
          actualSteps2 = actualSteps2 + stmtProofLen[i] - 1;
          for (j = 0; j < statement[i].numReqHyp; j++) {
            /* Subtract out hypotheses if subtheorem eliminated */
            k = statement[i].reqHypList[j];
            if (!essentialFlag || statement[k].type == (char)e__) {
              actualSteps2--;
            }
          }
        }
      }
    }

    j = statemNum;
    for (i = stmtDist[statemNum]; i >= 0; i--) {
      if (stmtDist[j] != i) bug(214);
      let(&tmpStr, cat(tmpStr, " <- ", statement[j].labelName,
          NULL));
      j = stmtMaxPath[j];
    }
    printLongLine(cat(
       "The statement's actual proof has ",
           str(stmtProofLen[statemNum]), " steps.  ",
       "Backtracking, a total of ", str(actualSubTheorems),
           " different subtheorems are used.  ",
       "The statement and subtheorems have a total of ",
           str(actualSteps), " actual steps.  ",
       "If subtheorems used only once were eliminated,",
           " there would be a total of ",
           str(actualSubTheorems2), " subtheorems, and ",
       "the statement and subtheorems would have a total of ",
           str(actualSteps2), " steps.  ",
       "The proof would have ", str(stepCount),
       " steps if fully expanded.  ",
       /*
       "The proof tree has ", str(stmtNodeCount[statemNum])," nodes.  ",
       "A random backtrack path has an average path length of ",
       str(stmtAveDist[statemNum]),
       ".  ",
       */
       "The maximum path length is ",
       str(stmtDist[statemNum]),
       ".  A longest path is:  ", right(tmpStr, 5), " .", NULL),
       "", " ");
    let(&tmpStr, "");

    free(stmtCount);
    free(stmtNodeCount);
    free(stmtDist);
    free(stmtMaxPath);
    free(stmtAveDist);
    free(stmtProofLen);
    free(stmtUsage);
  }
  
  return(stepCount);

}


/* Traces what statements require the use of a given statement */
void traceUsage(long statemNum,
  flag recursiveFlag) {
  
  long lastPos, stmt, slen, pos;
  flag tmpFlag;
  vstring statementUsedFlags = ""; /* 'y'/'n' flag that statement is used */
  vstring outputString = "";
  vstring tmpStr = "";
  nmbrString *statementList = NULL_NMBRSTRING;
  nmbrString *proof = NULL_NMBRSTRING;
  
  /* For speed-up code */
  char *fbPtr;
  char *fbPtr2;
  char zapSave;
  flag notEFRec;
  
  if (statement[statemNum].type == e__ || statement[statemNum].type == f__
      || recursiveFlag) {
    notEFRec = 0;
  } else {
    notEFRec = 1;
  }
  
  nmbrLet(&statementList, nmbrAddElement(statementList, statemNum));
  lastPos = 1;
  for (stmt = statemNum + 1; stmt <= statements; stmt++) { /* Scan all stmts*/
    if (statement[stmt].type != p__) continue; /* Ignore if not $p */
    
    /* Speed up:  Do a character search for the statement label in the proof,
       before parsing the proof.  Skip this if the label refers to a $e or $f
       because these might not have their labels explicit in a compressed
       proof.  Also, bypass speed up in case of recursive search. */
    if (notEFRec) {
      fbPtr = statement[stmt].proofSectionPtr; /* Start of proof */
      if (fbPtr[0] == 0) { /* The proof was never assigned */
        continue; /* Don't bother */
      }
      fbPtr = fbPtr + whiteSpaceLen(fbPtr); /* Get past white space */
      if (fbPtr[0] == '(') { /* "(" is flag for compressed proof */
        fbPtr2 = fbPtr;
        while (fbPtr2[0] != ')') {
          fbPtr2++;
          if (fbPtr2[0] == 0) bug(217); /* Didn't find closing ')' */
        }
      } else {
        /* A non-compressed proof; use whole proof */
        fbPtr2 = statement[stmt].proofSectionPtr +
            statement[stmt].proofSectionLen;
      }
      zapSave = fbPtr2[0];
      fbPtr2[0] = 0; /* Zap source for character string termination */
      if (!instr(1, fbPtr, statement[statemNum].labelName)) {
        fbPtr2[0] = zapSave; /* Restore source buffer */
        /* There is no string match for label in proof; don't bother to
           parse. */
        continue;
      } else {
        /* The label was found in the ASCII source.  Procede with parse. */
        fbPtr2[0] = zapSave; /* Restore source buffer */
      }
    } /* (End of speed-up code) */
       
    tmpFlag = 0;
    parseProof(stmt); /* Parse proof into wrkProof structure */
    nmbrLet(&proof, wrkProof.proofString); /* The proof */
    tmpFlag = 0;
    for (pos = 0; pos < lastPos; pos++) {
      if (nmbrElementIn(1, proof, statementList[pos])) {
        tmpFlag = 1;
        break;
      }
    }
    if (!tmpFlag) continue;
    /* The traced statement is used in this proof */
    /* Add this statement to the statement list */
    nmbrLet(&statementList, nmbrAddElement(statementList, stmt));
    if (recursiveFlag) lastPos++;
  } /* Next stmt */
  
  slen = nmbrLen(statementList);

  if (slen - 1 == 0) {
    printLongLine(cat("Statement \"", statement[statemNum].labelName,
        "\" is not referenced in the proof of any statement.", NULL), "", " ");
  } else {
    if (recursiveFlag) {
      let(&tmpStr, "\" directly or indirectly affects");
    } else {
      let(&tmpStr, "\" is directly referenced in");
    }
    if (slen - 1 == 1) {
      printLongLine(cat("Statement \"", statement[statemNum].labelName,
          tmpStr, " the proof of ",
          str(slen - 1), " statement:", NULL), "", " ");
    } else {
      printLongLine(cat("Statement \"", statement[statemNum].labelName,
          tmpStr, " the proofs of ",
          str(slen - 1), " statements:", NULL), "", " ");
    }
  }
  
  /* Prepare the output */
  /* First, fill in the statementUsedFlags char array.  This allows us to sort
     the output by statement number without calling a sort routine. */
  let(&statementUsedFlags, string(statements + 1, 'n')); /* Init. to 'no' */
  for (pos = 1; pos < slen; pos++) { /* Start with 1 (ignore traced statement)*/
    stmt = statementList[pos];
    if (stmt <= statemNum || statement[stmt].type != p__ || stmt > statements)
        bug(212);
    statementUsedFlags[stmt] = 'y';
  }
  /* Next, build the output string */
  let(&outputString, "");
  for (stmt = 1; stmt <= statements; stmt++) {
    if (statementUsedFlags[stmt] == 'y') {
      let(&outputString, cat(outputString, " ", statement[stmt].labelName,
          NULL));
    } /* End if (statementUsedFlag[stmt] == 'y') */
  } /* Next stmt */
  if (outputString[0]) {
    let(&outputString, cat(" ", outputString, NULL));
  } else {
    let(&outputString, "  (None)");
  }

  /* Print the output */
  printLongLine(outputString, "  ", " ");
  
  /* Deallocate */
  let(&outputString, "");
  let(&statementUsedFlags, "");
  nmbrLet(&statementList, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  
}



/* Returns the last embedded comment (if any) in the label section of
   a statement.  This is used to provide the user with information in the SHOW
   STATEMENT command.  The caller must deallocate the result. */
vstring getDescription(long statemNum) {
  char *fbPtr; /* Source buffer pointer */
  vstring description = "";
  char *startDescription;
  char *endDescription;
  char *startLabel;
  
  fbPtr = statement[statemNum].mathSectionPtr;
  if (!fbPtr[0]) return (description);
  startLabel = statement[statemNum].labelSectionPtr;
  if (!startLabel[0]) return (description);
  endDescription = NULL;
  while (1) { /* Get end of embedded comment */
    if (fbPtr <= startLabel) break;
    if (fbPtr[0] == '$' && fbPtr[1] == ')') {
      endDescription = fbPtr;
      break;
    }
    fbPtr--;
  }
  if (!endDescription) return (description); /* No embedded comment */
  while (1) { /* Get start of embedded comment */
    if (fbPtr < startLabel) bug(216);
    if (fbPtr[0] == '$' && fbPtr[1] == '(') {
      startDescription = fbPtr + 2;
      break;
    }
    fbPtr--;
  }
  let(&description, space(endDescription - startDescription));
  memcpy(description, startDescription, endDescription - startDescription);
  if (description[endDescription - startDescription - 1] == '\n') {
    /* Trim trailing new line */
    let(&description, left(description, endDescription - startDescription - 1));
  }
  /* Discard leading and trailing blanks */
  let(&description, edit(description, 8 + 128));
  return (description);
}


void readInput(void)
{

  /* Temporary variables and strings */
  long i,p;

  p = instr(1,input_fn,".") - 1;
  if (p == -1) p = len(input_fn);
  let(&mainFileName,left(input_fn,p));

  print2("Reading source file \"%s\"...\n",input_fn);

  includeCalls = 0; /* Initialize for readRawSource */
  sourcePtr = readRawSource(input_fn, 0, &sourceLen);
  parseKeywords();
  parseLabels();
  parseMathDecl();
  parseStatements();

}

void writeInput(void)
{

  /* Temporary variables and strings */
  long i,p;
  vstring str1 = "";

  print2("Creating and writing \"%s\"...\n",output_fn);

  /* Process statements */
  for (i = 1; i <= statements + 1; i++) {
    str1 = outputStatement(i);
    fprintf(output_fp, "%s", str1);
    let(&str1,""); /* Deallocate vstring */
  }
  print2("%ld source statements were written.\n",statements);


}

void writeDict(void)
{
  /* Temporary variables and strings */
  long i,p,dictFlag;
  vstring str1=""; /* Remember to initialize all vstrings! */

  /* Open the files */
  inputDef_fp=vmc_fopen(&inputDef_fn,
        "Name of LaTeX symbol definition input file",
        "latex.def","r",0);

  print2("Reading LaTeX symbol definitions from \"%s\"...\n",inputDef_fn);

  /* Initialize the lexical analyzer */
  vmc_lexSetup();

  /*** Read in and parse the graphics symbol definition file ***/
  parseGraphicsDef();


  /* Put out a list of definitions */

  print2("Creating and writing \"%s\"...\n",tex_dict_fn);

  /*** Word-processor specific ***/
  outputHeader();

  /*** Word-processor specific ***/
  outputDefinitions();

  /*** Word-processor specific ***/
  outputTrailer();

  return;
}

/* Free up all memory space and initialize all variables */
void eraseSource(void)
{
  long i;
  vstring tmpStr;
  
  /*??? Deallocate wrkProof structure if wrkProofMaxSize != 0 */
  
  if (statements == 0) {
    /* Already called */
    memFreePoolPurge(0);
    memUsedPoolPurge(0);
    return;
  }
  
  for (i = 0; i <= includeCalls; i++) {
    let(&includeCall[i].current_fn,"");
    let(&includeCall[i].calledBy_fn,"");
    let(&includeCall[i].includeSource,"");
  }
  /* Deallocate the statement[] array */
  for (i = 1; i <= statements + 1; i++) { /* statements + 1 is a dummy statement
                                          to hold source after last statement */
    if (statement[i].labelName[0]) free(statement[i].labelName);
    if (statement[i].mathString[0] != -1)
        poolFree(statement[i].mathString);
    if (statement[i].proofString[0] != -1)
        poolFree(statement[i].proofString);
    if (statement[i].reqHypList[0] != -1)
        poolFree(statement[i].reqHypList);
    if (statement[i].optHypList[0] != -1)
        poolFree(statement[i].optHypList);
    if (statement[i].reqVarList[0] != -1)
        poolFree(statement[i].reqVarList);
    if (statement[i].optVarList[0] != -1)
        poolFree(statement[i].optVarList);
    if (statement[i].reqDisjVarsA[0] != -1)
        poolFree(statement[i].reqDisjVarsA);
    if (statement[i].reqDisjVarsB[0] != -1)
        poolFree(statement[i].reqDisjVarsB);
    if (statement[i].reqDisjVarsStmt[0] != -1)
        poolFree(statement[i].reqDisjVarsStmt);
    if (statement[i].optDisjVarsA[0] != -1)
        poolFree(statement[i].optDisjVarsA);
    if (statement[i].optDisjVarsB[0] != -1)
        poolFree(statement[i].optDisjVarsB);
    if (statement[i].optDisjVarsStmt[0] != -1)
        poolFree(statement[i].optDisjVarsStmt);

    /* See if the proof was "saved" */
    if (statement[i].proofSectionLen) {
      if (statement[i].proofSectionPtr[-1] == 1) {
        /* Deallocate proof if not original source */
        /* (ASCII 1 is the flag for this) */
        tmpStr = statement[i].proofSectionPtr - 1;
        let(&tmpStr, "");
      }
    }

  } /* Next i (statement) */

  memFreePoolPurge(0);
  memUsedPoolPurge(0);
  statements = 0;
  errorCount = 0;

  free(statement);
  free(includeCall);
  free(mathToken);
  dummyVars = 0; /* For Proof Assistant */
  free(sourcePtr);
  free(labelKey);
  free(allLabelKeyBase);

  /* Deallocate the wrkProof structure */
  /*???*/
  
  /* Allocate big arrays */
  initBigArrays();
}


/* If verify = 0, parse the proofs only for gross error checking.
   If verify = 1, do the full verification. */
void verifyProofs(vstring labelMatch, flag verifyFlag) {
  vstring emptyProofList = "";
  long i, k;
  long lineLen = 0;
  long nextPercent;
  vstring header = "";

#ifdef __WATCOMC__
  vstring tmpStr="";
#endif

#ifdef VAXC
  vstring tmpStr="";
#endif

  if (!strcmp("*", labelMatch) && verifyFlag) {
    /* Use status bar */
    let(&header, "0 10%  20%  30%  40%  50%  60%  70%  80%  90% 100%"); 
#ifdef XXX /*__WATCOMC__*/
    /* The vsprintf function discards text after "%" in 3rd argument string. */
    /* This is a workaround. */
    for (i = 1; i <= strlen(header); i++) {
      let(&tmpStr, mid(header, i, 1));
      if (tmpStr[0] == '%') let(&tmpStr, "%%");
      print2("%s", tmpStr);
    }
    print2("\n");
#else
#ifdef XXX /*VAXC*/
    /* The vsprintf function discards text after "%" in 3rd argument string. */
    /* This is a workaround. */
    for (i = 1; i <= strlen(header); i++) {
      let(&tmpStr, mid(header, i, 1));
      if (tmpStr[0] == '%') let(&tmpStr, "%%");
      print2("%s", tmpStr);
    }
    print2("\n");
#else
    print2("%s\n", header);                                             
#endif
#endif
  }

  for (i = 1; i <= statements; i++) {
    if (!strcmp("*", labelMatch) && verifyFlag) {
      while (lineLen < (50 * i) / statements) {
        print2(".");
        lineLen++;
      }
    }

    if (statement[i].type != p__) continue;
    if (!matches(statement[i].labelName, labelMatch, '*')) continue;
    if (strcmp("*",labelMatch) && verifyFlag) {
      /* If not *, print individual labels */
      lineLen = lineLen + strlen(statement[i].labelName) + 1;
      if (lineLen > 72) {
        lineLen = strlen(statement[i].labelName) + 1;
        print2("\n");
      }
      print2("%s ",statement[i].labelName);
    }
      
    k = parseProof(i);
    if (k < 2) { /* $p with no error */
      if (verifyFlag) {
        verifyProof(i);
        cleanWrkProof(); /* Deallocate verifyProof storage */
      }
    }
    if (k == 1) {
      let(&emptyProofList,cat(emptyProofList,", ",statement[i].labelName,
          NULL));
    }
  }
  if (verifyFlag) {
    print2("\n");
  }

  if (strlen(emptyProofList)) {
    printLongLine(cat(
        "Warning:  The following $p statements were not proved:  ",
        right(emptyProofList,3),NULL)," ","  ");
  }
  let(&emptyProofList,""); /* Deallocate */

}



/* Called by help() - prints a help line */
/* THINK C gives compilation error if H() is lower-case h() -- why? */
void H(vstring helpLine)
{
  if (printHelp) {
    print2("%s\n", helpLine);
  }
}
