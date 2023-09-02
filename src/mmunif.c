/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

// mmunif.c - Unifications for proof assistant (note: unifications for normal
// proof verification is done in mmveri.c).

// This module deals with an object called the stateVector, which is a pntrString
// of 16 pointers (called entries 0 through 15 below) to either other pntrStrings
// or to nmbrStrings.  In the description, data in stateVector may be referred to
// by the local C variable the data is typically assigned to, such as
// "unkVarsLen".  The word "variable" in the context of scheme content refers
// to temporary (or "work" or "dummy") variables $1, $2, etc.  The entries are not
// organized in logical order for historical reasons, e.g. entry 11 logically
// comes first.
//
// Entry 11 is a nmbrString of length 4 holding individual parameters.
//
// 11[0] is the total number of variables ($1, $2, etc.) in schemeA and schemeB,
// i.e. the two schemes being unified.
//
//   unkVarsLen = ((nmbrString *)((*stateVector)[11]))[0];
//
// 11[1] or stackTop is the number of variables (minus 1) that will require
// substitutions in order to perform the unification.  Warning:  stackTop may be
// -1, which could be confused with "end of nmbrString" by some nmbrString
// functions.
//
//   stackTop = ((nmbrString *)((*stateVector)[11]))[1];
//
// 11[2] is the number of variables in schemeA, used by oneDirUnif() (only).
//
//   schemeAUnkVarsLen = ((nmbrString *)((*stateVector)[11]))[2];
//
// 11[3] is the number of entries in the "Henty filter", used by unifyH() (only).
//
//   g_hentyFilterSize = ((nmbrString *)((*stateVector)[11]))[3];
//
// Entry 8 is the result of unifying schemeA and schemeB, which are the two
// schemes being unified.
//
//   unifiedScheme = (nmbrString *)((*stateVector)[8]);
//
// Entries 0 through 3 each have length unkVarsLen.  Entry 1 is a list of token
// numbers for the temporary variables substituted in the unification.
//
// Entry 0 has all variables ($1, $2, etc.) in schemeA and schemeB.
//
//   unkVars = (nmbrString *)((*stateVector)[0]);
//
// In entries 1 through 3, only variables 0 through stackTop (inclusive) have
// meaning.  These entries, along with unifiedScheme, determine what variables
// were substituted and there substitutions.
//
// Entry 1 is the list of variables that were substituted.
// Entry 2 is the location of the substitution in unifiedScheme, for each variable
// in entry 1.
// Entry 3 is the length of the substitution for each variable in entry 1.
//
//   stackUnkVar = (nmbrString *)((*stateVector)[1]);
//   stackUnkVarStart = (nmbrString *)((*stateVector)[2]);
//   stackUnkVarLen = (nmbrString *)((*stateVector)[3]);
//
// Entries 4 thru 7 each point to unkVarsLen nmbrString's.  These entries save the
// data needed to resume unification at any point.  Entries 4 and 5 are
// nmbrString's of length unkVarsLen.  Entries 6 and 7 will have variable length.
// Only the first stackTop+1 nmbrString's have meaning.  Note that stackTop may be
// -1.
//
//   stackSaveUnkVarStart = (pntrString *)((*stateVector)[4]);
//   stackSaveUnkVarLen = (pntrString *)((*stateVector)[5]);
//   stackSaveSchemeA = (pntrString *)((*stateVector)[6]);
//   stackSaveSchemeB = (pntrString *)((*stateVector)[7]);
//
// Entries 9 and 10 save the contents of 2 and 3 in oneDirUnif (only)
//
//   nmbrLet((nmbrString **)(&(*stateVector)[9]),
//       (nmbrString *)((*stateVector)[2]));
//   nmbrLet((nmbrString **)(&(*stateVector)[10]),
//       (nmbrString *)((*stateVector)[3]));
//
// Entries 12 through 15 hold the "Henty filter", i.e. a list of all "normalized"
// unifications so far.  Used by unifyH() (only).  Each entry 12 through 15 is a
// list of pointers of length g_hentyFilterSize, each pointing to g_hentyFilterSize
// nmbrString's.  The Henty filter eliminates redundant equivalent unifications.
//
// Entry 12[i] is a list of variables substituted by the normalized unification.
// Entry 13[i] is the start of each substitution in hentySubstList.
// Entry 14[i] is the length of each substitution in hentySubstList.
// Entry 15[i] is the unified scheme that resulted from the particular unification.
// Note:  i = 0 through g_hentyFilterSize-1 below.
//
//   hentyVars = (nmbrString *)(((pntrString *)((*stateVector)[12]))[i]);
//   hentyVarStart = (nmbrString *)(((pntrString *)((*stateVector)[13]))[i]);
//   hentyVarLen = (nmbrString *)(((pntrString *)((*stateVector)[14]))[i]);
//   hentySubstList = (nmbrString *)(((pntrString *)((*stateVector)[15]))[i]);

#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h"
#include "mmunif.h"
#include "mmpfas.h" // For proveStatement global variable

// long g_minSubstLen = 0; // User-settable value - 0 or 1
// It was decided to disallow empty subst by default
// since most formal systems don't need it.
long g_minSubstLen = 1;
// User-defined upper limit (# backtracks) for unification trials.
// 1-Jun-04 nm Changed g_userMaxUnifTrials from 1000 to 100000, which
// is not a problem with today's faster computers.  This results in
// fewer annoying "Unification timed out" messages, but the drawback
// is that (rarely) there may be hundreds of unification
// choices for the user (which the user can quit from though).
long g_userMaxUnifTrials = 100000; // Initial value
long g_unifTrialCount = 0; // 0 means don't time out; 1 means start counting trials.
long g_unifTimeouts = 0; // Number of timeouts so far for this command.
flag g_hentyFilter = 1; // Default to ON (turn OFF for debugging).
flag g_bracketMatchInit = 0; // Global so eraseSource() (mmcmds.c) can clear it.

// Additional local prototypes
void hentyNormalize(nmbrString **hentyVars, nmbrString **hentyVarStart,
    nmbrString **hentyVarLen, nmbrString **hentySubstList,
    pntrString **stateVector);
flag hentyMatch(
    nmbrString *hentyVars,
    nmbrString *hentyVarStart,
    // nmbrString *hentyVarLen,
    nmbrString *hentySubstList,
    pntrString **stateVector);
void hentyAdd(nmbrString *hentyVars, nmbrString *hentyVarStart,
    nmbrString *hentyVarLen, nmbrString *hentySubstList,
    pntrString **stateVector);

// For heuristics
int maxNestingLevel = -1;
int nestingLevel = 0;

// For improving rejection of impossible substitutions
nmbrString_def(g_firstConst);
nmbrString_def(g_lastConst);
nmbrString_def(g_oneConst);

// Typical call:
//   nmbrStringXxx = makeSubstUnif(&newVarFlag,trialScheme,
//       stateVector);
//   Call this after calling unify().
//   trialScheme should have the same unknown variable names as were in
//       the schemes given to unify().
//   nmbrStringXxx will have these unknown variables substituted with
//       the result of the unification.
//   newVarFlag is 1 if there are new $nn variables in nmbrStringXxx.
// The caller must deallocate the returned nmbrString.
nmbrString *makeSubstUnif(flag *newVarFlag,
    const nmbrString *trialScheme, pntrString *stateVector)
{
  long p,q,i,j,k,m,tokenNum;
  long schemeLen;
  nmbrString_def(result);
  nmbrString_def(stackUnkVar);
  nmbrString *unifiedScheme; // Pointer only - not allocated
  nmbrString *stackUnkVarLen; // Pointer only - not allocated
  nmbrString *stackUnkVarStart; // Pointer only - not allocated
  long stackTop;
/*E*/long d;
/*E*/vstring_def(tmpStr);
/*E*/let(&tmpStr,tmpStr);

  stackTop = ((nmbrString *)(stateVector[11]))[1];
  nmbrLet(&stackUnkVar,nmbrLeft((nmbrString *)(stateVector[1]), stackTop + 1));
  stackUnkVarStart = (nmbrString *)(stateVector[2]); // stackUnkVarStart
  stackUnkVarLen = (nmbrString *)(stateVector[3]); // stackUnkVarLen
  unifiedScheme = (nmbrString *)(stateVector[8]);

/*E*/if(db7)print2("Entered makeSubstUnif.\n");
/*E*/if(db7)printLongLine(cat("unifiedScheme is ",
/*E*/    nmbrCvtMToVString(unifiedScheme), NULL), "", " ");
/*E*/if(db7)printLongLine(cat("trialScheme is ",
/*E*/    nmbrCvtMToVString(trialScheme), NULL), "", " ");
/*E*/if(db7)print2("stackTop is %ld.\n",stackTop);
/*E*/for (d = 0; d <= stackTop; d++) {
/*E*/  if(db7)print2("Unknown var %ld is %s.\n",d,
/*E*/      g_MathToken[stackUnkVar[d]].tokenName);
/*E*/  if(db7)print2("  Its start is %ld; its length is %ld.\n",
/*E*/      stackUnkVarStart[d],stackUnkVarLen[d]);
/*E*/}
  schemeLen = nmbrLen(trialScheme);
  // Make the substitutions into trialScheme.
  // First, calculate the length of the final result.
  q = 0;
  *newVarFlag = 0; // Flag that there are new variables in the output string
/*E*/if(db7)print2("schemeLen is %ld.\n",schemeLen);
  for (p = 0; p < schemeLen; p++) {
/*E*/if(db7)print2("p is %ld.\n",p);
    tokenNum = trialScheme[p];
/*E*/if(db7)print2("token is %s, tokenType is %ld\n",g_MathToken[tokenNum].tokenName,
/*E*/  (long)g_MathToken[tokenNum].tokenType);
    if (g_MathToken[tokenNum].tokenType == (char)con_) {
      q++;
    } else {
      if (tokenNum > g_mathTokens) {
        // It's a candidate for substitution
        m = nmbrElementIn(1,stackUnkVar,tokenNum);
/*E*/if(db7)print2("token is %s, m is %ld\n",g_MathToken[tokenNum].tokenName,m);
        if (m) {
          // It will be substituted
          q = q + stackUnkVarLen[m - 1];
          // Flag the token position
          g_MathToken[tokenNum].tmp = m - 1;
        } else {
          // It will not be substituted
          *newVarFlag = 1; // The result will contain an "unknown" variable
          q++;
          // Flag the token position
          g_MathToken[tokenNum].tmp = -1;
        }
      } else {
        // It's not an "unknown" variable, so it won't be substituted
        q++;
      }
    }
  }
  // Allocate space for the final result
  nmbrLet(&result, nmbrSpace(q));
  // Assign the final result
  q = 0;
  for (p = 0; p < schemeLen; p++) {
    tokenNum = trialScheme[p];
    if (g_MathToken[tokenNum].tokenType == (char)con_) {
      result[q] = tokenNum;
      q++;
    } else {
      if (tokenNum > g_mathTokens) {
        // It's a candidate for substitution
        k = g_MathToken[tokenNum].tmp; // Position in stackUnkVar
        if (k != -1) {
          // It will be substituted
          m = stackUnkVarStart[k]; // Start of substitution
          j = stackUnkVarLen[k]; // Length of substitution
          for (i = 0; i < j; i++) {
            result[q + i] = unifiedScheme[m + i];
          }
          q = q + j;
        } else {
          // It will not be substituted
          result[q] = tokenNum;
          q++;
        }
      } else {
        // It's not an "unknown" variable, so it won't be substituted
        result[q] = tokenNum;
        q++;
      }
    } // end "if a constant"
  }
/*E*/if(db7)print2("after newVarFlag %d\n",(int)*newVarFlag);
/*E*/if(db7)print2("final len is %ld\n",q);
/*E*/if(db7)printLongLine(cat("result ",nmbrCvtMToVString(result),NULL),""," ");
  free_nmbrString(stackUnkVar); // Deallocate
  return (result);
} // makeSubstUnif

char unify(
    const nmbrString *schemeA,
    const nmbrString *schemeB,
    // nmbrString **unifiedScheme, // stateVector[8] holds this
    pntrString **stateVector,
    long reEntryFlag)
{

// This function unifies two math token strings, schemeA and
// schemeB.  The result is contained in unifiedScheme.
// 0 is returned if no assignment is possible, 1 if an assignment was
// found, and 2 if the unification timed out.
// If reEntryFlag is 1, the next possible set of assignments, if any,
// is returned.  (*stateVector) contains the state of the previous
// call.  It is the caller's responsibility to deallocate the
// contents of (*stateVector) when done, UNLESS a 0 is returned.
// The caller must assign (*stateVector) to a legal pntrString
// (e.g. NULL_PNTRSTRING) before calling.
//
// All variables with a tokenNum > g_mathTokens are assumed
// to be "unknown" variables that can be assigned; all other
// variables are treated like constants in the unification
// algorithm.
//
// The "unknown" variable assignments are contained in (*stateVector)
// (which is a complex structure, described above).  Some "unknown"
// variables may have no assignment, in which case they will
// remain "unknown", and others may have assignments which include
// "unknown" variables.  The (*stateVector) entries 9 and 10 are used
// by oneDirUnif() only.

  long stackTop;
  nmbrString *unkVars; // List of all unknown vars
  long unkVarsLen;
  long schemeAUnkVarsLen;
  nmbrString *stackUnkVar; // Location of stacked var in unkVars
  nmbrString *stackUnkVarStart; // Start of stacked var in unifiedScheme
  nmbrString *stackUnkVarLen; // Length of stacked var assignment
  // stackUnkVarStart at the time a variable was first stacked.
  pntrString *stackSaveUnkVarStart;
  // stackUnkVarLen at the time a variable was first stacked.
  pntrString *stackSaveUnkVarLen;
  // Pointer to saved schemeA at the time the variable was first stacked.
  pntrString *stackSaveSchemeA;
  // Pointer to saved schemeB at the time the variable was first stacked.
  pntrString *stackSaveSchemeB;
  nmbrString *unifiedScheme; // Final result
  long p; // Current position in schemeA or schemeB
  long substToken; // Token from schemeA or schemeB that will be substituted
  nmbrString_def(substitution); // String to be subst. for substToken
  nmbrString *nmbrTmpPtr; // temp pointer only
  pntrString *pntrTmpPtr; // temp pointer only
  nmbrString_def(schA); // schemeA with dummy token at end
  nmbrString_def(schB); // schemeB with dummy token at end
  long i,j,k,m, pairingMismatches;
  flag breakFlag;
  flag schemeAFlag;
  flag timeoutAbortFlag = 0;
  vstring mToken; // Pointer only; not allocated

  // For detection of simple impossible unifications
  flag impossible;
  long stmt;

  // For bracket matching heuristic for set.mm
  static char bracketMatchOn; // Default is 'on'
  // static char g_bracketMatchInit = 0; // Global so ERASE can init it
  long bracketScanStart, bracketScanStop; // For one-time $a scan
  flag bracketMismatchFound;

/*E*/long d;
/*E*/vstring_def(tmpStr);
/*E*/let(&tmpStr,tmpStr);
/*E*/if(db5)print2("Entering unify() with reEntryFlag = %ld.\n",
/*E*/  (long)reEntryFlag);
/*E*/if(db5)printLongLine(cat("schemeA is ",
/*E*/    nmbrCvtMToVString(schemeA),".",NULL),"    ","  ");
/*E*/if(db5)printLongLine(cat("schemeB is ",
/*E*/    nmbrCvtMToVString(schemeB),".",NULL),"    ","  ");

  // Initialization to avoid compiler warning (should not be theoretically necessary).
  p = 0;
  bracketMismatchFound = 0;

  // Fast early exit -- first or last constants of schemes don't match
  if (g_MathToken[schemeA[0]].tokenType == (char)con_) {
    if (g_MathToken[schemeB[0]].tokenType == (char)con_) {
      if (schemeA[0] != schemeB[0]) {
        return (0);
      }
    }
  }
  // (j and k are used below also)
  j = nmbrLen(schemeA);
  k = nmbrLen(schemeB);
  if (!j || !k) bug(1901);
  if (g_MathToken[schemeA[j-1]].tokenType == (char)con_) {
    if (g_MathToken[schemeB[k-1]].tokenType == (char)con_) {
      if (schemeA[j-1] != schemeB[k-1]) {
        return 0;
      }
    }
  }
  // Add dummy token to end of schemeA and schemeB.
  // Use one beyond the last mathTokenArray entry for this.
  nmbrLet(&schA, nmbrAddElement(schemeA, g_mathTokens));
  nmbrLet(&schB, nmbrAddElement(schemeB, g_mathTokens));

  // Initialize the usage of constants as the first, last,
  // only constant in a $a statement - for rejecting some simple impossible
  // substitutions - Speed-up: this is now done once and never deallocated.
  //
  // g_firstConst is now cleared in eraseSource.c() (mmcmds.c)
  // to trigger this initialization after "erase".
  if (!nmbrLen(g_firstConst)) {
    // nmbrSpace() sets all entries to 0, not 32 (ASCII space)
    nmbrLet(&g_firstConst, nmbrSpace(g_mathTokens));
    nmbrLet(&g_lastConst, nmbrSpace(g_mathTokens));
    nmbrLet(&g_oneConst, nmbrSpace(g_mathTokens));
    // for (stmt = 1; stmt < proveStatement; stmt++) {
    // Do it for all statements since we do it once permanently now
    for (stmt = 1; stmt <= g_statements; stmt++) {
      if (g_Statement[stmt].type != (char)a_)
        continue; // Not $a
      if (g_Statement[stmt].mathStringLen < 2) continue;
      // Look at first symbol after variable type symbol
      if (g_MathToken[(g_Statement[stmt].mathString)[1]].tokenType == (char)con_) {
        g_firstConst[(g_Statement[stmt].mathString)[1]] = 1; // Set flag
        if (g_Statement[stmt].mathStringLen == 2) {
          g_oneConst[(g_Statement[stmt].mathString)[1]] = 1; // Set flag
        }
      }
      // Look at last symbol
      if (g_MathToken[(g_Statement[stmt].mathString)[
          g_Statement[stmt].mathStringLen - 1]].tokenType == (char)con_) {
        g_lastConst[(g_Statement[stmt].mathString)[
          g_Statement[stmt].mathStringLen - 1]] = 1; // Set flag for const
      }
    } // Next stmt
  }

  if (!reEntryFlag) {
    // First time called
    p = 0;

    // Collect the list of "unknown" variables.
    // (Pre-allocate max. length)
    // (Note j and k assignment above)
    unkVars = NULL_NMBRSTRING;
    nmbrLet(&unkVars, nmbrSpace(j + k));
    unkVarsLen = 0;
    for (i = 0; i < j; i++) {
      if (schemeA[i] > g_mathTokens) {
        // It's an "unknown" variable
        breakFlag = 0;
        for (m = 0; m < unkVarsLen; m++) {
          if (unkVars[m] == schemeA[i]) {
            // It's already been added to the list
            breakFlag = 1;
          }
        }
        if (!breakFlag) {
          // Add the new "unknown" var
          unkVars[unkVarsLen++] = schemeA[i];
        }
      }
    }
    // Save the length of the list of unknown variables in schemeA
    schemeAUnkVarsLen = unkVarsLen;
    for (i = 0; i < k; i++) {
      if (schemeB[i] > g_mathTokens) {
        // It's an "unknown" variable
        breakFlag = 0;
        for (m = 0; m < unkVarsLen; m++) {
          if (unkVars[m] == schemeB[i]) {
            // It's already been added to the list
            breakFlag = 1;
          }
        }
        if (!breakFlag) {
          // Add the new "unknown" var
          unkVars[unkVarsLen++] = schemeB[i];
        }
      }
    }

    // Deallocate old (*stateVector) assignments
    if (pntrLen(*stateVector)) {
    // if (((nmbrString *)((*stateVector)[11]))[0] != -1) { // ???Change to nmbrLen?
      // If (*stateVector) not an empty nmbrString
      for (i = 4; i <= 7; i++) {
        pntrTmpPtr = (pntrString *)((*stateVector)[i]);
        for (j = 0; j < ((nmbrString *)((*stateVector)[11]))[0]; j++) {
          nmbrLet((nmbrString **)(&pntrTmpPtr[j]),
              NULL_NMBRSTRING);
        }
        pntrLet((pntrString **)(&(*stateVector)[i]),
            NULL_PNTRSTRING);
      }
      for (i = 0; i <= 3; i++) {
        nmbrLet((nmbrString **)(&(*stateVector)[i]),
            NULL_NMBRSTRING);
      }
      for (i = 8; i <= 10; i++) {
        nmbrLet((nmbrString **)(&(*stateVector)[i]),
            NULL_NMBRSTRING);
      }
      k = pntrLen((pntrString *)((*stateVector)[12]));
      for (i = 12; i < 16; i++) {
        pntrTmpPtr = (pntrString *)((*stateVector)[i]);
        for (j = 0; j < k; j++) {
          nmbrLet((nmbrString **)(&pntrTmpPtr[j]),
              NULL_NMBRSTRING);
        }
        pntrLet((pntrString **)(&(*stateVector)[i]),
            NULL_PNTRSTRING);
      }
      // Leave [11] pre-allocated to length 4
    } else {
      // It was never allocated before -- do it now
      // Allocate stateVector - it will be assigned upon exiting
      pntrLet(&(*stateVector), pntrPSpace(16));
      nmbrLet((nmbrString **)(&(*stateVector)[11]), nmbrSpace(4));
    }

    // Pre-allocate the (*stateVector) structure
    stackTop = -1;
    stackUnkVar = NULL_NMBRSTRING;
    stackUnkVarStart = NULL_NMBRSTRING;
    stackUnkVarLen = NULL_NMBRSTRING;
    stackSaveUnkVarStart = NULL_PNTRSTRING;
    stackSaveUnkVarLen = NULL_PNTRSTRING;
    stackSaveSchemeA = NULL_PNTRSTRING;
    stackSaveSchemeB = NULL_PNTRSTRING;
    unifiedScheme = NULL_NMBRSTRING;
    nmbrLet(&stackUnkVar, nmbrSpace(unkVarsLen));
    nmbrLet(&stackUnkVarStart, stackUnkVar);
    nmbrLet(&stackUnkVarLen, stackUnkVar);

    // These next 4 hold pointers to nmbrStrings
    pntrLet(&stackSaveUnkVarStart, pntrNSpace(unkVarsLen));
    pntrLet(&stackSaveUnkVarLen, stackSaveUnkVarStart);
    pntrLet(&stackSaveSchemeA, stackSaveUnkVarStart);
    pntrLet(&stackSaveSchemeB, stackSaveUnkVarStart);
    for (i = 0; i < unkVarsLen; i++) {
      // Preallocate the stack space for these
      nmbrLet((nmbrString **)(&stackSaveUnkVarStart[i]),
          stackUnkVar);
      nmbrLet((nmbrString **)(&stackSaveUnkVarLen[i]),
          stackUnkVar);
    }

    // Set a flag that the "unknown" variables are not on the stack yet.
    // (Otherwise this will be the position on the stack.)
    for (i = 0; i < unkVarsLen; i++) {
      g_MathToken[unkVars[i]].tmp = -1;
    }
  } else { // reEntryFlag != 0

    // We are re-entering to get the next possible assignment.

    // Restore the (*stateVector) variables
    unkVarsLen = ((nmbrString *)((*stateVector)[11]))[0];
    unkVars = (nmbrString *)((*stateVector)[0]);
    stackTop = ((nmbrString *)((*stateVector)[11]))[1];
    stackUnkVar = (nmbrString *)((*stateVector)[1]);
    stackUnkVarStart = (nmbrString *)((*stateVector)[2]);
    stackUnkVarLen = (nmbrString *)((*stateVector)[3]);
    stackSaveUnkVarStart = (pntrString *)((*stateVector)[4]);
    stackSaveUnkVarLen = (pntrString *)((*stateVector)[5]);
    stackSaveSchemeA = (pntrString *)((*stateVector)[6]);
    stackSaveSchemeB = (pntrString *)((*stateVector)[7]);
    unifiedScheme = (nmbrString *)((*stateVector)[8]);
    schemeAUnkVarsLen = ((nmbrString *)((*stateVector)[11]))[2]; // Used by oneDirUnif()

    // Set the location of the "unknown" variables on the stack.
    // (This may have been corrupted outside this function.)
    for (i = 0; i < unkVarsLen; i++) {
      g_MathToken[unkVars[i]].tmp = -1; // Not on the stack
    }
    for (i = 0; i <= stackTop; i++) {
      g_MathToken[stackUnkVar[i]].tmp = i;
    }

    // Force a backtrack to the next assignment
    goto backtrack;
   reEntry1: // goto backtrack will come back here if reEntryFlag is set
    reEntryFlag = 0;
  }

  // Perform the unification

 scan:
/*E*/if(db6)print2("Entered scan: p=%ld\n",p);
/*E*/if(db6)print2("Enter scan sbA %s\n",nmbrCvtMToVString(schA));
/*E*/if(db6)print2("Enter scan sbB %s\n",nmbrCvtMToVString(schB));
/*E*/if(db6)let(&tmpStr,tmpStr);
  while (schA[p] == schB[p] &&
      schA[p + 1] != -1) {
    p++;
  }
/*E*/if(db6)print2("First mismatch: p=%ld\n",p);

  if (schA[p] == g_mathTokens
      || schB[p] == g_mathTokens) {
    // One of the strings is at the end.
    if (schA[p] != schB[p]) {
      // But one is longer than the other.
      if (schA[p] <= g_mathTokens &&
          schB[p] <= g_mathTokens) {
        // Neither token is an unknown variable.  (Otherwise we might be able
        // to assign the unknown variable to a null string, thus making
        // the schemes match, so we shouldn't backtrack.)
/*E*/if(db6)print2("Backtracked because end-of-string\n");
        goto backtrack;
      }
    } else {
      if (schA[p + 1] == -1) {
        // End of schA; a successful unification occurred
        goto done;
      }
      // Otherwise, we are in the middle of several schemes being unified
      // simultaneously, so just continue.
      // (g_mathTokens should be used by the caller to separate
      // schemes that are joined together for simultaneous unification.)
    }
  }

  // This test, combined with switchover to schemeB in backtrack,
  // prevents variable lockup, for example where
  // schemeA = ?1 B C, schemeB = ?2 A B C. Without this test, schemeB
  // becomes ?1 A B C, and then it can never match schemeA. This should be
  // checked out further:  what about more than 2 variables? This kind of
  // "variable lockup" may be a more serious problem.
  //
  // A test case:
  // schemeA is $$ |- ( ( ?463 -> -. -. ph ) -> ( -. ph -> ( ph -> -. -. ph ) ) ).
  // schemeB is $$ |- ( ?464 -> ( ?465 -> ?464 ) ).
  if (schB[p] > g_mathTokens && schA[p] > g_mathTokens) {
    // Both scheme A and scheme B have variables in the match position.
    // Which one to use?
    // * If neither A nor B is on the stack, use A. Backtrack will put B
    //   on the stack when A's possibilities are exhausted.
    // * If A is on the stack, use A.
    // * If B is on the stack, use B.
    // * If A and B are on the stack, bug.
    // In other words:  if B is not on the stack, use A.
    if (g_MathToken[schB[p]].tmp == -1) {
      // B is not on the stack
      goto schAUnk;
    } else {
      if (g_MathToken[schA[p]].tmp != -1) bug(1902); // Both are on the stack
      goto schBUnk;
    }
  }

 schBUnk:
  if (schB[p] > g_mathTokens) {
/*E*/if(db6)print2("schB has unknown variable\n");
    // An "unknown" variable is in scheme B
    schemeAFlag = 0;
    substToken = schB[p];

    if (g_MathToken[substToken].tmp == -1) {
      // The "unknown" variable is not on the stack; add it
      stackTop++;
      stackUnkVar[stackTop] = substToken;
      g_MathToken[substToken].tmp = stackTop;
      stackUnkVarStart[stackTop] = p;
      // Start with a variable length of 0 or 1
      stackUnkVarLen[stackTop] = g_minSubstLen;
      // Save the rest of the current state for backtracking
      nmbrTmpPtr = (nmbrString *)(stackSaveUnkVarStart[stackTop]);
      for (i = 0; i <= stackTop; i++) {
        nmbrTmpPtr[i] = stackUnkVarStart[i];
      }
      nmbrTmpPtr = (nmbrString *)(stackSaveUnkVarLen[stackTop]);
      for (i = 0; i <= stackTop; i++) {
        nmbrTmpPtr[i] = stackUnkVarLen[i];
      }
      nmbrLet((nmbrString **)(&stackSaveSchemeA[stackTop]),
          schA);
      nmbrLet((nmbrString **)(&stackSaveSchemeB[stackTop]),
          schB);
    }

    if (substToken != stackUnkVar[stackTop]) {
      print2("PROGRAM BUG #1903\n");
      print2("substToken is %s\n", g_MathToken[substToken].tokenName);
      print2("stackTop %ld\n", stackTop);
      print2("p %ld stackUnkVar[stackTop] %s\n", p,
        g_MathToken[stackUnkVar[stackTop]].tokenName);
      print2("schA %s\nschB %s\n", nmbrCvtMToVString(schA),
        nmbrCvtMToVString(schB));
      bug(1903);
    }
    nmbrLet(&substitution, nmbrMid(schA, p + 1,
        stackUnkVarLen[stackTop]));
    goto substitute;
  }

 schAUnk:
  if (schA[p] > g_mathTokens) {
/*E*/if(db6)print2("schA has unknown variable\n");
    // An "unknown" variable is in scheme A
    schemeAFlag = 1;
    substToken = schA[p];
    if (g_MathToken[substToken].tmp == -1) {
      // The "unknown" variable is not on the stack; add it
      stackTop++;
      stackUnkVar[stackTop] = substToken;
      g_MathToken[substToken].tmp = stackTop;
      stackUnkVarStart[stackTop] = p;
      // Start with a variable length of 0 or 1
      stackUnkVarLen[stackTop] = g_minSubstLen;
      // Save the rest of the current state for backtracking
      nmbrTmpPtr = (nmbrString *)(stackSaveUnkVarStart[stackTop]);
      for (i = 0; i <= stackTop; i++) {
        nmbrTmpPtr[i] = stackUnkVarStart[i];
      }
      nmbrTmpPtr = (nmbrString *)(stackSaveUnkVarLen[stackTop]);
      for (i = 0; i <= stackTop; i++) {
        nmbrTmpPtr[i] = stackUnkVarLen[i];
      }
      nmbrLet((nmbrString **)(&stackSaveSchemeA[stackTop]),
          schA);
      nmbrLet((nmbrString **)(&stackSaveSchemeB[stackTop]),
          schB);
    }

    if (substToken != stackUnkVar[stackTop]) {
/*E*/print2("PROGRAM BUG #1904\n");
/*E*/print2("\nsubstToken is %s\n",g_MathToken[substToken].tokenName);
/*E*/print2("stack top %ld\n",stackTop);
/*E*/print2("p %ld stackUnkVar[stackTop] %s\n",p,
/*E*/g_MathToken[stackUnkVar[stackTop]].tokenName);
/*E*/print2("schA %s\nschB %s\n",nmbrCvtMToVString(schA),nmbrCvtMToVString(schB));
      bug(1904);
    }
    nmbrLet(&substitution, nmbrMid(schB, p + 1,
        stackUnkVarLen[stackTop]));
    goto substitute;
  }

  // Neither scheme has an unknown variable; unification with current assignment
  // failed, so backtrack.
/*E*/if(db6)print2("Neither scheme has unknown variable\n");
  goto backtrack;

 substitute:
/*E*/if(db6)print2("Entering substitute...\n");
/*E*/for (d = 0; d <= stackTop; d++) {
/*E*/  if(db6)print2("Unknown var %ld is %s.\n",d,
/*E*/      g_MathToken[stackUnkVar[d]].tokenName);
/*E*/  if(db6)print2("  Its start is %ld; its length is %ld.\n",
/*E*/      stackUnkVarStart[d],stackUnkVarLen[d]);
/*E*/}
  // Subst. all occurrences of substToken with substitution in schA and schB.
  // ???We could speed things up by making substitutions before the pointer to
  // ???to the unifiedScheme only, and keeping track of 3 pointers (A, B, &
  // ???unified schemes); unifiedScheme would hold only stuff before pointer.

  // First, we must make sure that the substToken doesn't occur in the
  // substutition.
  if (nmbrElementIn(1, substitution, substToken)) {
/*E*/if(db6)print2("Substituted token occurs in substitution string\n");
    goto backtrack;
  }

  // Next, we must make sure that the end of string doesn't occur in the
  // substitution.
  // (This takes care of the case where the unknown variable aligns with
  // end of string character; in this case, only a null substitution is
  // permissible. If the substitution length is 1 or greater, this "if"
  // statement will detect it.)
  if (substitution[0] == g_mathTokens) {
/*E*/if(db6)print2("End of string token occurs in substitution string\n");
    // We must pop the stack here rather than in backtrack, because we
    // are already one token beyond the end of a scheme, and backtrack
    // would therefore test one token beyond that, missing the fact that
    // the substitution has overflowed beyond the end of a scheme.

    // Set the flag that it's not on the stack and pop stack.
    g_MathToken[stackUnkVar[stackTop]].tmp = -1;
    stackTop--;
    goto backtrack;
  }

  // Bracket matching is customized to set.mm to result in fewer ambiguous
  // unifications.
  // Automatically disable bracket matching if any $a has
  // unmatched brackets.
  // The static variable g_bracketMatchInit tells us to check all $a's
  // if it is 0; if 1, skip the $a checking.  Make sure that the RESET
  // command sets g_bracketMatchInit=0.
  // ???  To do:  put individual bracket type checks into a loop or
  // function call for code efficiency (but don't slow down program); maybe
  // read the bracket types to check from a list; maybe refine so that only
  // the mismatched bracket types found in the $a scan are skipped, but
  // matched one are not.
  for (i = g_bracketMatchInit; i <= 1; i++) {
    // This loop has 2 passes (0 and 1) if g_bracketMatchInit=0 to set
    // bracketMatchOn = 0 or 1, and 1 pass otherwise.
    bracketMismatchFound = 0; // Don't move down; needed for break below
    if (g_bracketMatchInit == 0) { // Initialization pass
      if (i != 0) bug(1908);
      // Scan all ($a) statements
      bracketScanStart = 1;
      bracketScanStop = g_statements;
    } else { // Normal pass
      if (i != 1) bug(1909);
      // Skip the whole bracket check because a mismatched bracket was found in
      // some $a in the initialization pass.
      if (!bracketMatchOn) break;
      // Set dummy parameters to force a single loop pass
      bracketScanStart = 0;
      bracketScanStop = 0;
    }
    for (m = bracketScanStart; m <= bracketScanStop; m++) {
      if (g_bracketMatchInit == 0) { // Initialization pass
        if (g_Statement[m].type != a_) continue;
        nmbrTmpPtr = g_Statement[m].mathString;
      } else { // Normal pass
        nmbrTmpPtr = substitution;
      }
      j = nmbrLen(nmbrTmpPtr);

      // Make sure left and right parentheses match
      pairingMismatches = 0; // Counter of parens: + for "(" and - for ")"
      for (k = 0; k < j; k++) {
        mToken = g_MathToken[nmbrTmpPtr[k]].tokenName;
        if (mToken[0] == '(' && mToken[1] == 0 ) {
          pairingMismatches++;
        } else if (mToken[0] == ')' && mToken[1] == 0 ) {
          pairingMismatches--;
          if (pairingMismatches < 0) break; // Detect wrong order
        }
      } // Next k
      if (pairingMismatches != 0) {
        bracketMismatchFound = 1;
        break;
      }

      // Make sure left and right braces match
      pairingMismatches = 0; // Counter of braces: + for "{" and - for "}"
      for (k = 0; k < j; k++) {
        mToken = g_MathToken[nmbrTmpPtr[k]].tokenName;
        if (mToken[0] == '{' && mToken[1] == 0 ) pairingMismatches++;
        else
          if (mToken[0] == '}' && mToken[1] == 0 ) {
            pairingMismatches--;
            if (pairingMismatches < 0) break; // Detect wrong order
          }
      } // Next k
      if (pairingMismatches != 0) {
        bracketMismatchFound = 1;
        break;
      }

      // Make sure left and right brackets match
      pairingMismatches = 0; // Counter of brackets: + for "[" and - for "]"
      for (k = 0; k < j; k++) {
        mToken = g_MathToken[nmbrTmpPtr[k]].tokenName;
        if (mToken[0] == '[' && mToken[1] == 0 )
          pairingMismatches++;
        else
          if (mToken[0] == ']' && mToken[1] == 0 ) {
            pairingMismatches--;
            if (pairingMismatches < 0) break; // Detect wrong order
          }
      } // Next k
      if (pairingMismatches != 0) {
        bracketMismatchFound = 1;
        break;
      }

      // Make sure left and right triangle brackets match
      pairingMismatches = 0; // Counter of brackets: + for "<.", - for ">."
      for (k = 0; k < j; k++) {
        mToken = g_MathToken[nmbrTmpPtr[k]].tokenName;
        if (mToken[1] == 0) continue;
        if (mToken[0] == '<' && mToken[1] == '.' && mToken[2] == 0 )
            pairingMismatches++;
        else
          if (mToken[0] == '>' && mToken[1] == '.' && mToken[2] == 0 ) {
            pairingMismatches--;
            if (pairingMismatches < 0) break; // Detect wrong order
          }
      } // Next k
      if (pairingMismatches != 0) {
        bracketMismatchFound = 1;
        break;
      }

      // Make sure underlined brackets match
      pairingMismatches = 0; // Counter of brackets: + for "[_", - for "]_"
      for (k = 0; k < j; k++) {
        mToken = g_MathToken[nmbrTmpPtr[k]].tokenName;
        if (mToken[1] == 0) continue;
        if (mToken[0] == '[' && mToken[1] == '_' && mToken[2] == 0 )
            pairingMismatches++;
        else
          if (mToken[0] == ']' && mToken[1] == '_' && mToken[2] == 0 ) {
            pairingMismatches--;
            if (pairingMismatches < 0) break; // Detect wrong order
          }
      } // Next k
      if (pairingMismatches != 0) {
        bracketMismatchFound = 1;
        break;
      }
    } // next m

    if (g_bracketMatchInit == 0) { // Initialization pass
      // We've finished the one-time $a scan.  Set flags accordingly.
      if (bracketMismatchFound) { // Some $a has a bracket mismatch
        if (m < 1 || m > g_statements) bug(1910);
        printLongLine(cat("The bracket matching unification heuristic was",
           " turned off for this database because of a bracket mismatch in",
           " statement \"",
           // (m should be accurate due to break above)
           g_Statement[m].labelName,
           "\".", NULL),
           "    ", " ");
        // printLongLine(cat("The bracket matching unification heuristic was",
        // " turned off for this database.", NULL),
        // "    ", " ");
        bracketMatchOn = 0; // Turn off static flag for this database
      } else { // Normal pass
        bracketMatchOn = 1; // Turn it on
      }
/*E*/if(db6)print2("bracketMatchOn = %ld\n", (long)bracketMatchOn);
      g_bracketMatchInit = 1; // We're done with the one-time $a scan
    }
  } // next i

  if (bracketMismatchFound) goto backtrack;

  j = nmbrLen(substitution);

  // Quick scan to reject some impossible unifications: If the
  // first symbol in a substitution is a constant, it must match
  // the 2nd constant of some earlier $a statement (e.g. "<" matches
  // "class <", "Ord (/)" matches "class Ord A"). Same applies to
  // last symbol.
  // This prefilter is too aggressive when empty substitutions
  // are allowed. Therefore added "g_minSubstLen > 0" to fix miu.mm theorem1
  // Proof Assistant failure reported by Josh Purinton.
  if (j /* subst len */ > 0 && g_minSubstLen > 0) {
    impossible = 0;
    if (g_MathToken[substitution[0]].tokenType == (char)con_) {
      if (!g_firstConst[substitution[0]]
         || (j == 1 && !g_oneConst[substitution[0]])) {
        impossible = 1;
      }
    }
    if (g_MathToken[substitution[j - 1]].tokenType == (char)con_) {
      if (!g_lastConst[substitution[j - 1]]) {
        impossible = 1;
      }
    }
    if (impossible) {
/*E*/if(db6)print2("Impossible subst: %s\n", nmbrCvtMToVString(substitution));
      goto backtrack;
    }
  }

  // Now perform the substitutions
/*E*/if(db6)print2("Substitution is '%s'\n",nmbrCvtMToVString(substitution));
  k = 1;
  while (1) {
    // Perform the substitutions into scheme A
    k = nmbrElementIn(k, schA, substToken);
    if (!k) break;

    if (schemeAFlag) {
      // The token to be substituted was in scheme A
      // Adjust position and earlier var. starts and lengths
      if (k - 1 <= p) {
        if (k <= p) {
          // Adjust assignments in stack
          for (i = 0; i <= stackTop; i++) {
            if (k - 1 < stackUnkVarStart[i]) {
              stackUnkVarStart[i] = stackUnkVarStart[i] + j-1;
            } else {
              if (k <= stackUnkVarStart[i] +
                  stackUnkVarLen[i]) {
                stackUnkVarLen[i] = stackUnkVarLen[i] + j - 1;
              }
            }
          }
        }
        p = p + j - 1; // Adjust scan position
/*E*/if(db6)print2("Scheme A adjusted p=%ld\n",p);
      }
    }

    nmbrLet(&schA, nmbrCat(
        nmbrLeft(schA, k - 1), substitution, nmbrRight(schA, k + 1), NULL));
    k = k + j - 1;
  }
  k = 1;
  while (1) {
    // Perform the substitutions into scheme B
    k = nmbrElementIn(k, schB, substToken);
    if (!k) break;

    if (!schemeAFlag) {
      // The token to be substituted was in scheme B.
      // Adjust scan position and earlier var. starts and lengths.
      if (k - 1 <= p) {
        if (k <= p) {
          // Adjust assignments in stack
          for (i = 0; i <= stackTop; i++) {
            if (k - 1 < stackUnkVarStart[i]) {
              stackUnkVarStart[i] = stackUnkVarStart[i] + j-1;
            } else {
              if (k <= stackUnkVarStart[i] +
                  stackUnkVarLen[i]) {
                stackUnkVarLen[i] = stackUnkVarLen[i] + j - 1;
              }
            }
          }
        }
        p = p + j - 1; // Adjust scan position
      }
/*E*/if(db6)print2("Scheme B adjusted p=%ld\n",p);
    }

    nmbrLet(&schB, nmbrCat(
        nmbrLeft(schB, k - 1), substitution, nmbrRight(schB, k + 1), NULL));
    k = k + j - 1;
  }
  p++;
/*E*/if(db6)print2("Scheme A or B final p=%ld\n",p);
/*E*/if(db6)print2("after sub sbA %s\n",nmbrCvtMToVString(schA));
/*E*/if(db6)print2("after sub sbB %s\n",nmbrCvtMToVString(schB));
/*E*/for (d = 0; d <= stackTop; d++) {
/*E*/  if(db6)print2("Unknown var %ld is %s.\n",d,
/*E*/      g_MathToken[stackUnkVar[d]].tokenName);
/*E*/  if(db6)print2("  Its start is %ld; its length is %ld.\n",
/*E*/      stackUnkVarStart[d],stackUnkVarLen[d]);
/*E*/}
  goto scan;

 backtrack:
/*E*/if(db6)print2("Entered backtrack with p=%ld stackTop=%ld\n",p,stackTop);
  if (stackTop < 0) {
    goto abort;
  }
  if (g_unifTrialCount > 0) { // Flag that timeout is active
    g_unifTrialCount++;
    if (g_unifTrialCount > g_userMaxUnifTrials) {
      g_unifTimeouts++; // Update number of timeouts found
/*E*/if(db5)print2("Aborted due to timeout: %ld > %ld\n",
/*E*/    g_unifTrialCount, g_userMaxUnifTrials);
      timeoutAbortFlag = 1;
      goto abort;
    }
  }
  // Add 1 to stackTop variable length
  nmbrTmpPtr = (nmbrString *)(stackSaveUnkVarLen[stackTop]);
  nmbrTmpPtr[stackTop]++;
  // Restore the state from the stack top
  nmbrLet(&stackUnkVarStart, (nmbrString *)(stackSaveUnkVarStart[stackTop]));
  nmbrLet(&schA, (nmbrString *)(stackSaveSchemeA[stackTop]));
  nmbrLet(&schB, (nmbrString *)(stackSaveSchemeB[stackTop]));
  // Restore the scan position
  p = stackUnkVarStart[stackTop];
 switchVarToB:
  // Restore the state from the stack top
  nmbrLet(&stackUnkVarLen, (nmbrString *)(stackSaveUnkVarLen[stackTop]));

  // If the variable overflows the end of the scheme its assigned to,
  // pop the stack.
/*E*/if(db6)print2("Backtracked to token %s.\n",
/*E*/  g_MathToken[stackUnkVar[stackTop]].tokenName);
  if (stackUnkVar[stackTop] == schA[p]) {
    // It was in scheme A; see if it overflows scheme B
    if (schB[p - 1 + stackUnkVarLen[stackTop]]
        == g_mathTokens) {
/*E*/if(db6)print2("It was in scheme A; overflowed scheme B: p=%ld, len=%ld.\n",
/*E*/  p,stackUnkVarLen[stackTop]);
      // Set the flag that it's not on the stack
      g_MathToken[stackUnkVar[stackTop]].tmp = -1;

      // See if the token in scheme B at this position is also a variable.
      // If so, switch the stack top variable to the one in scheme B and
      // restart the scan on its length.
      if (schB[p] > g_mathTokens) {
/*E*/if(db6)print2("Switched var-var match to scheme B token %s\n",
/*E*/     g_MathToken[stackUnkVar[stackTop]].tokenName);
        // The scheme B variable will not be on the stack.
        if (g_MathToken[schB[p]].tmp != -1) bug(1905);
        // Make the token in scheme B become the variable at the stack top
        stackUnkVar[stackTop] = schB[p];
        g_MathToken[schB[p]].tmp = stackTop;
        // Start with a variable length of 0 or 1
        stackUnkVarLen[stackTop] = g_minSubstLen;
        // Initialize stackTop variable length
        nmbrTmpPtr = (nmbrString *)(stackSaveUnkVarLen[stackTop]);
        nmbrTmpPtr[stackTop] = g_minSubstLen;
        // Restart the backtrack with double variable switched to scheme B
        goto switchVarToB;
      }

      // Pop stack
      stackTop--;
      goto backtrack;
    }
  } else {
    // It was in scheme B; see if it overflows scheme A
    if (schA[p - 1 + stackUnkVarLen[stackTop]]
        == g_mathTokens) {
/*E*/if(db6)print2("It was in scheme B; overflowed scheme A: p=%ld, len=%ld.\n",
/*E*/  p,stackUnkVarLen[stackTop]);
      // Set the flag that it's not on the stack and pop stack
      g_MathToken[stackUnkVar[stackTop]].tmp = -1;
      stackTop--;
      goto backtrack;
    }
  }
/*E*/if(db6)print2("Exited backtrack with p=%ld stackTop=%ld\n",p,stackTop);
  if (reEntryFlag) goto reEntry1;
  goto scan; // Continue the scan

 done:

  // Assign the final result
  nmbrLet(&unifiedScheme, nmbrLeft(schA, nmbrLen(schA) - 1));
/*E*/if(db5)print2("Backtrack count was %ld\n",g_unifTrialCount);
/*E*/if(db5)printLongLine(cat("Unified scheme is ",
/*E*/    nmbrCvtMToVString(unifiedScheme),".",NULL),"    ","  ");
  // Assign the 12 components of (*stateVector).  Some of the components hold
  // pointers to pointer strings.
  // 0 holds the unkVars array and length.
  ((nmbrString *)((*stateVector)[11]))[0] = unkVarsLen;
  (*stateVector)[0] = unkVars;
  // 1 holds the stack top and the "unknown" vars on the stack
  ((nmbrString *)((*stateVector)[11]))[1] = stackTop;
  (*stateVector)[1] = stackUnkVar;
  (*stateVector)[2] = stackUnkVarStart;
  (*stateVector)[3] = stackUnkVarLen;
  (*stateVector)[4] = stackSaveUnkVarStart;
  (*stateVector)[5] = stackSaveUnkVarLen;
  (*stateVector)[6] = stackSaveSchemeA;
  (*stateVector)[7] = stackSaveSchemeB;
  // Save the result
  (*stateVector)[8] = unifiedScheme;
  // Components 9 and 10 save the previous assignment.
  // This is handled by oneDirUnif() if oneDirUnif() is called.
  ((nmbrString *)((*stateVector)[11]))[2] = schemeAUnkVarsLen; // Used by oneDirUnif()

/*E*/if(db5)printSubst(*stateVector);
  // Deallocate nmbrStrings
  free_nmbrString(schA);
  free_nmbrString(schB);
  free_nmbrString(substitution);
  return (1);

 abort:
/*E*/if(db5)print2("Backtrack count was %ld\n",g_unifTrialCount);
  // Deallocate stateVector contents
  free_nmbrString(unkVars);
  free_nmbrString(stackUnkVar);
  free_nmbrString(stackUnkVarStart);
  free_nmbrString(stackUnkVarLen);
  for (i = 0; i < unkVarsLen; i++) {
    // Deallocate the stack space for these
    nmbrLet((nmbrString **)(&stackSaveUnkVarStart[i]),
        NULL_NMBRSTRING);
    nmbrLet((nmbrString **)(&stackSaveUnkVarLen[i]),
        NULL_NMBRSTRING);
    nmbrLet((nmbrString **)(&stackSaveSchemeA[i]),
        NULL_NMBRSTRING);
    nmbrLet((nmbrString **)(&stackSaveSchemeB[i]),
        NULL_NMBRSTRING);
  }
  free_pntrString(stackSaveUnkVarStart);
  free_pntrString(stackSaveUnkVarLen);
  free_pntrString(stackSaveSchemeA);
  free_pntrString(stackSaveSchemeB);
  free_nmbrString(unifiedScheme);
  // Deallocate entries used by oneDirUnif()
  free_nmbrString(*(nmbrString **)(&(*stateVector)[9]));
  free_nmbrString(*(nmbrString **)(&(*stateVector)[10]));
  // Deallocate entries used by unifyH()
  k = pntrLen((pntrString *)((*stateVector)[12]));
  for (i = 12; i < 16; i++) {
    pntrTmpPtr = (pntrString *)((*stateVector)[i]);
    for (j = 0; j < k; j++) {
      free_nmbrString(*(nmbrString **)(&pntrTmpPtr[j]));
    }
    free_pntrString(*(pntrString **)(&(*stateVector)[i]));
  }
  // Deallocate the stateVector itself
  ((nmbrString *)((*stateVector)[11]))[1] = 0;
  // stackTop: Make sure it's not -1 before calling nmbrLet()
  free_nmbrString(*(nmbrString **)(&(*stateVector)[11]));
  free_pntrString(*stateVector);

  // Deallocate nmbrStrings
  free_nmbrString(schA);
  free_nmbrString(schB);
  free_nmbrString(substitution);

  if (timeoutAbortFlag) {
    return (2);
  }
  return (0);
} // unify

// oneDirUnif() is like unify(), except that when reEntryFlag is 1,
// a new unification is returned ONLY if the assignments to the
// variables in schemeA have changed.  This is used to speed up the
// program.
// ???This whole thing may be screwed up -- it seems to be based on
// all unknown vars (including those without substitutions), not just
// the ones on the stack.
flag oneDirUnif(
    const nmbrString *schemeA,
    const nmbrString *schemeB,
    pntrString **stateVector,
    long reEntryFlag)
{
long i;
flag tmpFlag;
long schemeAUnkVarsLen;
nmbrString *stackUnkVarStart; // Pointer only - not allocated
nmbrString *stackUnkVarLen; // Pointer only - not allocated
nmbrString *oldStackUnkVarStart; // Pointer only - not allocated
nmbrString *oldStackUnkVarLen; // Pointer only - not allocated

  if (!reEntryFlag) {
    tmpFlag = unify(schemeA, schemeB, stateVector, 0);
    if (tmpFlag) {
      // Save the initial variable assignments
      nmbrLet((nmbrString **)(&(*stateVector)[9]),
          (nmbrString *)((*stateVector)[2]));
      nmbrLet((nmbrString **)(&(*stateVector)[10]),
          (nmbrString *)((*stateVector)[3]));
    }
    return (tmpFlag);
  } else {
    while (1) {
      tmpFlag = unify(schemeA, schemeB, stateVector, 1);
      if (!tmpFlag) return (0);
      // Check to see if the variables in schemeA changed
      schemeAUnkVarsLen = ((nmbrString *)((*stateVector)[11]))[2];
      stackUnkVarStart = (nmbrString *)((*stateVector)[2]);
      stackUnkVarLen = (nmbrString *)((*stateVector)[3]);
      oldStackUnkVarStart = (nmbrString *)((*stateVector)[9]);
      oldStackUnkVarLen = (nmbrString *)((*stateVector)[10]);
      for (i = 0; i < schemeAUnkVarsLen; i++) {
        if (stackUnkVarStart[i] != oldStackUnkVarStart[i]) {
          // The assignment changed
          // Save the new assignment
          nmbrLet(&oldStackUnkVarStart, stackUnkVarStart);
          nmbrLet(&oldStackUnkVarLen, stackUnkVarLen);
          return (1);
        }
        if (stackUnkVarLen[i] != oldStackUnkVarLen[i]) {
          // The assignment changed
          // Save the new assignment
          nmbrLet(&oldStackUnkVarStart, stackUnkVarStart);
          nmbrLet(&oldStackUnkVarLen, stackUnkVarLen);
          return (1);
        }
      }
    } // End while (1)
  }
  return(0); // Dummy return value - never happens
} // oneDirUnif

// uniqueUnif() is like unify(), but there is no reEntryFlag, and 3 possible
// values are returned:
//   0: no unification was possible.
//   1: exactly one unification was possible, and stateVector is valid.
//   2: unification timed out.
//   3: more than one unification was possible.
char uniqueUnif(
    const nmbrString *schemeA,
    const nmbrString *schemeB,
    pntrString **stateVector)
{
  pntrString_def(saveStateVector);
  pntrString *pntrTmpPtr1; // Pointer only; not allocated
  pntrString *pntrTmpPtr2; // Pointer only; not allocated
  long i, j, k;
  char tmpFlag;

  tmpFlag = unifyH(schemeA, schemeB, stateVector, 0);
  if (!tmpFlag) {
    return (0); // No unification possible
  }
  if (tmpFlag == 2) {
    return (2); // Unification timed out
  }

  // Save the state vector
  pntrLet(&saveStateVector,*stateVector);
  if (pntrLen(*stateVector) != 16) bug(1906);
  for (i = 0; i < 4; i++) {
    // Force new space to be allocated for each pointer
    saveStateVector[i] = NULL_NMBRSTRING;
    nmbrLet((nmbrString **)(&saveStateVector[i]),
        (nmbrString *)((*stateVector)[i]));
  }
  for (i = 4; i <= 7; i++) {
    // Force new space to be allocated for each pointer
    saveStateVector[i] = NULL_PNTRSTRING;
    pntrLet((pntrString **)(&saveStateVector[i]),
        (pntrString *)((*stateVector)[i]));
  }
  for (i = 8; i < 12; i++) {
    // Force new space to be allocated for each pointer
    saveStateVector[i] = NULL_NMBRSTRING;
    nmbrLet((nmbrString **)(&saveStateVector[i]),
        (nmbrString *)((*stateVector)[i]));
  }
  for (i = 12; i < 16; i++) {
    // Force new space to be allocated for each pointer
    saveStateVector[i] = NULL_PNTRSTRING;
    pntrLet((pntrString **)(&saveStateVector[i]),
        (pntrString *)((*stateVector)[i]));
  }
  k = ((nmbrString *)((*stateVector)[11]))[0]; // unkVarsLen
  for (i = 4; i <= 7; i++) {
    pntrTmpPtr1 = (pntrString *)(saveStateVector[i]);
    pntrTmpPtr2 = (pntrString *)((*stateVector)[i]);
    for (j = 0; j < k; j++) {
      // Force new space to be allocated for each pointer
      pntrTmpPtr1[j] = NULL_NMBRSTRING;
      nmbrLet((nmbrString **)(&pntrTmpPtr1[j]),
          (nmbrString *)(pntrTmpPtr2[j]));
    }
  }
  k = pntrLen((pntrString *)((*stateVector)[12]));
  for (i = 12; i < 16; i++) {
    pntrTmpPtr1 = (pntrString *)(saveStateVector[i]);
    pntrTmpPtr2 = (pntrString *)((*stateVector)[i]);
    for (j = 0; j < k; j++) {
      // Force new space to be allocated for each pointer
      pntrTmpPtr1[j] = NULL_NMBRSTRING;
      nmbrLet((nmbrString **)(&pntrTmpPtr1[j]),
          (nmbrString *)(pntrTmpPtr2[j]));
    }
  }

  // See if there is a second unification
  tmpFlag = unifyH(schemeA, schemeB, stateVector, 1);

  if (!tmpFlag) {
    // There is no 2nd unification. Unify cleared the original stateVector,
    // so return the saved version.
    *stateVector = saveStateVector;
    return (1); // Return flag that unification is unique.
  }

  // There are two or more unifications. Deallocate the stateVector
  // we just saved before returning, since we will not use it.
  // stackTop: Make sure it's not -1 before calling nmbrLet().
  ((nmbrString *)(saveStateVector[11]))[1] = 0;
  for (i = 4; i <= 7; i++) {
    pntrTmpPtr1 = (pntrString *)(saveStateVector[i]);
    // ((nmbrString *)(saveStateVector[11]))[0] is unkVarsLen
    for (j = 0; j < ((nmbrString *)(saveStateVector[11]))[0]; j++) {           
      free_nmbrString(*(nmbrString **)(&pntrTmpPtr1[j]));
    }
  }
  for (i = 0; i <= 3; i++) {
    free_nmbrString(*(nmbrString **)(&saveStateVector[i]));
  }
  for (i = 4; i <= 7; i++) {
    free_pntrString(*(pntrString **)(&saveStateVector[i]));
  }
  for (i = 8; i <= 11; i++) {
    free_nmbrString(*(nmbrString **)(&saveStateVector[i]));
  }
  k = pntrLen((pntrString *)(saveStateVector[12]));
  for (i = 12; i < 16; i++) {
    pntrTmpPtr1 = (pntrString *)(saveStateVector[i]);
    for (j = 0; j < k; j++) {
      free_nmbrString(*(nmbrString **)(&pntrTmpPtr1[j]));
    }
    free_pntrString(*(pntrString **)(&saveStateVector[i]));
  }
  free_pntrString(saveStateVector);

  if (tmpFlag == 2) {
    return (2); // Unification timed out
  }

  return (3); // Return flag that unification is not unique
} // uniqueUnif

// Deallocates the contents of a stateVector.
// Note:  If unifyH() returns 0, there were no more unifications and
// the stateVector is left empty, so we don't have to call
// purgeStateVector.  But no harm done if called anyway.
void purgeStateVector(pntrString **stateVector) {

  long i, j, k;
  pntrString *pntrTmpPtr1;

  if (!pntrLen(*stateVector)) return; // It's already been purged

  // stackTop: Make sure it's not -1 before calling nmbrLet()
  ((nmbrString *)((*stateVector)[11]))[1] = 0;
  for (i = 4; i <= 7; i++) {
    pntrTmpPtr1 = (pntrString *)((*stateVector)[i]);
    // ((nmbrString *)((*stateVector)[11]))[0] is unkVarsLen
    for (j = 0; j < ((nmbrString *)((*stateVector)[11]))[0]; j++) {
      free_nmbrString(*(nmbrString **)(&pntrTmpPtr1[j]));
    }
  }
  for (i = 0; i <= 3; i++) {
    free_nmbrString(*(nmbrString **)(&(*stateVector)[i]));
  }
  for (i = 4; i <= 7; i++) {
    free_pntrString(*(pntrString **)(&(*stateVector)[i]));
  }
  for (i = 8; i <= 11; i++) {
    free_nmbrString(*(nmbrString **)(&(*stateVector)[i]));
  }
  k = pntrLen((pntrString *)((*stateVector)[12]));
  for (i = 12; i < 16; i++) {
    pntrTmpPtr1 = (pntrString *)((*stateVector)[i]);
    for (j = 0; j < k; j++) {
      free_nmbrString(*(nmbrString **)(&pntrTmpPtr1[j]));
    }
    free_pntrString(*(pntrString **)(&(*stateVector)[i]));
  }
  free_pntrString(*stateVector);

  return;
} // purgeStateVector

// Prints the substitutions determined by unify for debugging purposes
void printSubst(pntrString *stateVector) {
  long d;
  nmbrString *stackUnkVar; // Pointer only - not allocated
  nmbrString *unifiedScheme; // Pointer only - not allocated
  nmbrString *stackUnkVarLen; // Pointer only - not allocated
  nmbrString *stackUnkVarStart; // Pointer only - not allocated
  long stackTop;
  vstring_def(tmpStr);
  nmbrString_def(nmbrTmp);

  stackTop = ((nmbrString *)(stateVector[11]))[1];
  stackUnkVar = (nmbrString *)(stateVector[1]);
  stackUnkVarStart = (nmbrString *)(stateVector[2]);
  stackUnkVarLen = (nmbrString *)(stateVector[3]);
  unifiedScheme = (nmbrString *)(stateVector[8]);

  for (d = 0; d <= stackTop; d++) {
    printLongLine(cat(" Variable '",
        g_MathToken[stackUnkVar[d]].tokenName,"' was replaced with '",
        nmbrCvtMToVString(
            nmbrMid(unifiedScheme,stackUnkVarStart[d] + 1,
            stackUnkVarLen[d])),"'.",NULL),"    "," ");
    // Clear temporary string allocation
    free_vstring(tmpStr);
    free_nmbrString(nmbrTmp);
  }
} // printSubst

// unifyH() is like unify(), except that when reEntryFlag is 1, a new
// unification is returned ONLY if the normalized unification does not
// previously exist in the "Henty filter" part of the  stateVector.  This
// reduces ambiguous unifications.  The values returned are the same as those
// returned by unify().  (The redundancy of equivalent unifications was
// a deficiency pointed out by Jeremy Henty.)
char unifyH(
    const nmbrString *schemeA,
    const nmbrString *schemeB,
    pntrString **stateVector,
    long reEntryFlag)
{
  char tmpFlag;
  nmbrString_def(hentyVars);
  nmbrString_def(hentyVarStart);
  nmbrString_def(hentyVarLen);
  nmbrString_def(hentySubstList);

  // Bypass this filter if SET HENTY_FILTER OFF is selected.
  if (!g_hentyFilter) return unify(schemeA, schemeB, stateVector, reEntryFlag);

  if (!reEntryFlag) {
    tmpFlag = unify(schemeA, schemeB, stateVector, 0);
    if (tmpFlag == 1) { // Unification OK

      // Get the normalized equivalent substitutions
      hentyNormalize(&hentyVars, &hentyVarStart, &hentyVarLen,
          &hentySubstList, stateVector);

      // This is the first unification so add it to the filter then return 1
      hentyAdd(hentyVars, hentyVarStart, hentyVarLen,
          hentySubstList, stateVector);
    }
    return (tmpFlag);
  } else {
    while (1) {
      tmpFlag = unify(schemeA, schemeB, stateVector, 1);
      if (tmpFlag == 1) { // 0 = not possible, 1 == OK, 2 = timed out

        // Get the normalized equivalent substitution
        hentyNormalize(&hentyVars, &hentyVarStart, &hentyVarLen,
            &hentySubstList, stateVector);

        // Scan the Henty filter to see if this substitution is in it
        if (!hentyMatch(hentyVars, hentyVarStart, // hentyVarLen,
            hentySubstList, stateVector)) {

          // If it's not in there, this is a new unification so add it
          // to the filter then return 1.
          hentyAdd(hentyVars, hentyVarStart, hentyVarLen,
              hentySubstList, stateVector);
          return (1);
        }
      } else {
        // No unification is possible, or it timed out
        break;
      }

      // If we get here this unification is in the Henty filter, so bypass it
      // and get the next unification.
    } // End while (1)

    // Deallocate memory (when reEntryFlag is 1 and (not possible or timeout)).
    // (In the other cases, hentyVars and hentySubsts pointers are assigned
    // directly to stateVector so they should not be deallocated.)
    free_nmbrString(hentyVars);
    free_nmbrString(hentyVarStart);
    free_nmbrString(hentyVarLen);
    free_nmbrString(hentySubstList);
    return (tmpFlag);
  }
} // unifyH

// Extract and normalize the unification substitutions.
void hentyNormalize(nmbrString **hentyVars, nmbrString **hentyVarStart,
    nmbrString **hentyVarLen, nmbrString **hentySubstList,
    pntrString **stateVector)
{
  long vars, var1, var2, schLen;
  long n, el, rra, rrb, rrc, ir, i, j; // Variables for heap sort
  long totalSubstLen, pos;
  nmbrString_def(substList);

  // Extract the substitutions.
  vars = ((nmbrString *)((*stateVector)[11]))[1] + 1; // stackTop + 1
  nmbrLet((nmbrString **)(&(*hentyVars)), nmbrLeft(
      (nmbrString *)((*stateVector)[1]), vars)); // stackUnkVar
  nmbrLet((nmbrString **)(&(*hentyVarStart)), nmbrLeft(
      (nmbrString *)((*stateVector)[2]), vars)); // stackUnkVarStart
  nmbrLet((nmbrString **)(&(*hentyVarLen)), nmbrLeft(
      (nmbrString *)((*stateVector)[3]), vars)); // stackUnkVarLen
  nmbrLet((nmbrString **)(&(*hentySubstList)),
      (nmbrString *)((*stateVector)[8])); // unifiedScheme

  // First, if a variable is substituted with another variable,
  // reverse the substitution if the substituted variable has a larger
  // tokenNum.
  for (i = 0; i < vars; i++) {
    if ((*hentyVarLen)[i] == 1) {
      var2 = (*hentySubstList)[(*hentyVarStart)[i]];
      if (var2 > g_mathTokens) {
        // It's a variable-for-variable substitution
        var1 = (*hentyVars)[i];
        if (var1 > var2) {
          // Swap the variables
          (*hentyVars)[i] = var2;
          schLen = nmbrLen(*hentySubstList);
          for (j = 0; j < schLen; j++) {
            if ((*hentySubstList)[(*hentyVarStart)[i]] == var2) {
              (*hentySubstList)[(*hentyVarStart)[i]] = var1;
            }
          } // Next j
        } // End if (var1 > var2)
      } // End if (var2 > g_mathTokens)
    } // End if ((*hentyVarLen)[i] == 1)
  } // Next i

  // Next, sort the variables to be substituted in tokenNum order
  // Heap sort from "Numerical Recipes" (Press et. al.) p. 231
  // Note:  The algorithm in the text has a bug; it does not work for n<2.
  n = vars;
  if (n < 2) goto heapExit;
  el = n / 2 + 1;
  ir = n;
 label10:
  if (el > 1) {
    el = el - 1;
    rra = (*hentyVars)[el - 1];
    rrb = (*hentyVarStart)[el - 1];
    rrc = (*hentyVarLen)[el - 1];
  } else {
    rra = (*hentyVars)[ir - 1];
    rrb = (*hentyVarStart)[ir - 1];
    rrc = (*hentyVarLen)[ir - 1];
    (*hentyVars)[ir - 1] = (*hentyVars)[0];
    (*hentyVarStart)[ir - 1] = (*hentyVarStart)[0];
    (*hentyVarLen)[ir - 1] = (*hentyVarLen)[0];
    ir = ir - 1;
    if (ir == 1) {
      (*hentyVars)[0] = rra;
      (*hentyVarStart)[0] = rrb;
      (*hentyVarLen)[0] = rrc;
      goto heapExit;
    }
  }
  i = el;
  j = el + el;
 label20:
  if (j <= ir) {
    if (j < ir) {
      if ((*hentyVars)[j - 1] < (*hentyVars)[j]) j = j + 1;
    }
    if (rra < (*hentyVars)[j - 1]) {
      (*hentyVars)[i - 1] = (*hentyVars)[j - 1];
      (*hentyVarStart)[i - 1] = (*hentyVarStart)[j - 1];
      (*hentyVarLen)[i - 1] = (*hentyVarLen)[j - 1];
      i = j;
      j = j + j;
    } else {
      j = ir + 1;
    }
    goto label20;
  }
  (*hentyVars)[i - 1] = rra;
  (*hentyVarStart)[i - 1] = rrb;
  (*hentyVarLen)[i - 1] = rrc;
  goto label10;

 heapExit:

  // Finally, reconstruct the list of substitutions in
  // variable tokenNum order.
  totalSubstLen = 0;
  for (i = 0; i < vars; i++) {
    totalSubstLen = totalSubstLen + (*hentyVarLen)[i];
  }
  // For speedup, preallocate total string needed for the substitution list
  nmbrLet(&substList, nmbrSpace(totalSubstLen));

  pos = 0; // Position in list of substitutions
  for (i = 0; i < vars; i++) {
    for (j = 0; j < (*hentyVarLen)[i]; j++) {
      substList[pos + j] = (*hentySubstList)[(*hentyVarStart)[i] + j];
    }
    (*hentyVarStart)[i] = pos; // Never used, but assign in case it ever does
    pos = pos + (*hentyVarLen)[i];
  }
  if (pos != totalSubstLen) bug(1907);
  nmbrLet((nmbrString **)(&(*hentySubstList)), substList);

  // Deallocate memory
  free_nmbrString(substList);

  return;
} // hentyNormalize

// Check to see if an equivalent unification exists in the Henty filter
flag hentyMatch(
    nmbrString *hentyVars, 
    nmbrString *hentyVarStart,
    // nmbrString *hentyVarLen,
    nmbrString *hentySubstList,
    pntrString **stateVector)
{
  long i, size;

  size = pntrLen((pntrString *)((*stateVector)[12]));

  for (i = 0; i < size; i++) {
    if (nmbrEq(hentyVars,
        (nmbrString *)(((pntrString *)((*stateVector)[12]))[i]))) {
      if (nmbrEq(hentyVarStart,
          (nmbrString *)(((pntrString *)((*stateVector)[13]))[i]))) {
        // (We don't need to look at [14] because it is determined by [13])
        if (nmbrEq(hentySubstList,
            (nmbrString *)(((pntrString *)((*stateVector)[15]))[i]))) {
          return(1); // A previous equivalent unification was found
        }
      }
    }
  } // Next i

  return (0); // There was no previous equivalent unification
} // hentyMatch

// Add an entry to the Henty filter
void hentyAdd(nmbrString *hentyVars, nmbrString *hentyVarStart,
    nmbrString *hentyVarLen, nmbrString *hentySubstList,
    pntrString **stateVector)
{
  long size;
  size = pntrLen((pntrString *)((*stateVector)[12]));

  pntrLet((pntrString **)(&(*stateVector)[12]), pntrAddGElement(
      (pntrString *)((*stateVector)[12])));
  ((pntrString *)((*stateVector)[12]))[size] = hentyVars;
  pntrLet((pntrString **)(&(*stateVector)[13]), pntrAddGElement(
      (pntrString *)((*stateVector)[13])));
  ((pntrString *)((*stateVector)[13]))[size] = hentyVarStart;
  pntrLet((pntrString **)(&(*stateVector)[14]), pntrAddGElement(
      (pntrString *)((*stateVector)[14])));
  ((pntrString *)((*stateVector)[14]))[size] = hentyVarLen;
  pntrLet((pntrString **)(&(*stateVector)[15]), pntrAddGElement(
      (pntrString *)((*stateVector)[15])));
  ((pntrString *)((*stateVector)[15]))[size] =
      hentySubstList;
} // hentyAdd
