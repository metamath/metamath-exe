/*****************************************************************************/
/*        Copyright (C) 2021  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h"
// #include "mmcmds.h" // For getContribs() if used
#include "mmpfas.h" // Needed for g_pipDummyVars, subproofLen()
#include "mmunif.h" // Needed for g_minSubstLen
#include "mmcmdl.h" // Needed for g_rootDirectory
// Potential statements in source file (upper limit) for 
// memory allocation purposes.
long potentialStatements;
// Illegal characters for labels -- initialized
// by parseLabels()
flag illegalLabelChar[256];
// (Next 2 are global in mmpars.c only)
long *g_labelKeyBase; // Start of assertion ($a, $p) labels
long g_numLabelKeys; // Number of assertion labels

long *g_allLabelKeyBase; // Start of all labels
long g_numAllLabelKeys; // Number of all labels

// Working structure for parsing proofs.
// This structure should be deallocated by the ERASE command.
long g_wrkProofMaxSize = 0; // Maximum size so far - it may grow
long wrkMathPoolMaxSize = 0; // Max mathStringPool size so far - it may grow
struct wrkProof_struct g_WrkProof;

// This function returns a pointer to a buffer containing the contents of an
// input file and its 'include' calls. 'Size' returns the buffer's size.
// Partial parsing is done; when 'include' statements are found, this function
// is called recursively.
// The file g_input_fn is assumed to be opened when this is called.
char *readRawSource(vstring fileBuf, long *size) {
  long charCount = 0;
  char *fbPtr;
  flag insideComment;
  char mode;
  char tmpch;

  charCount = *size;

  // Look for $[ and $] 'include' statement start and end.
  // These won't happen since they're now expanded earlier.
  // But it can't hurt, since the code is already written.
  // TODO - clean it up for speedup?
  fbPtr = fileBuf;
  mode = 0; // 0 = outside of 'include', 1 = inside of 'include'
  insideComment = 0; // 1 = inside $( $) comment
  while (1) {
    // Find a keyword or newline
    tmpch = fbPtr[0];
    if (!tmpch) { // End of file
      if (insideComment) {
        rawSourceError(fileBuf, fbPtr - 1, 2,
         "The last comment in the file is incomplete.  \"$)\" was expected.");
      } else {
        if (mode != 0) {
          rawSourceError(fileBuf, fbPtr - 1, 2,
   "The last include statement in the file is incomplete.  \"$]\" was expected."
           );
        }
      }
      break;
    }
    if (tmpch != '$') {
      if (!isgraph((unsigned char)tmpch) && !isspace((unsigned char)tmpch)) {
        rawSourceError(fileBuf, fbPtr, 1,
            cat("Illegal character (ASCII code ",
            str((double)((unsigned char)tmpch)),
            " decimal).",NULL));
      }
      fbPtr++;
      continue;
    }

    // Detect missing whitespace around keywords (per current
    // Metamath language spec).
    if (fbPtr > fileBuf) { // If this '$' is not the 1st file character
      if (isgraph((unsigned char)(fbPtr[-1]))) {
        // The character before the '$' is not white space
        if (!insideComment || fbPtr[1] == ')') {
          // Inside comments, we only care about the "$)" keyword
          rawSourceError(fileBuf, fbPtr, 2,
              "A keyword must be preceded by white space.");
        }
      }
    }
    fbPtr++;
    // If the character after '$' is not end of file (which
    // would be an error anyway, but detected elsewhere).
    if (fbPtr[0]) {       
      if (isgraph((unsigned char)(fbPtr[1]))) {
        // The character after the character after the '$' is not white
        // space (nor end of file).
        if (!insideComment || fbPtr[0] == ')') {
          // Inside comments, we only care about the "$)" keyword
          rawSourceError(fileBuf, fbPtr + 1, 1,
              "A keyword must be followed by white space.");
          }
      }
    }

    switch (fbPtr[0]) {
      case '(': // Start of comment
        if (insideComment) {
          rawSourceError(fileBuf, fbPtr - 1, 2,
          "Nested comments are not allowed.");
        }
        insideComment = 1;
        continue;
      case ')': // End of comment
        if (!insideComment) {
          rawSourceError(fileBuf, fbPtr - 1, 2,
              "A comment terminator was found outside of a comment.");
        }
        insideComment = 0;
        continue;
    }
    if (insideComment) continue;
    switch (fbPtr[0]) {
      case '[':
        if (mode != 0) {
          rawSourceError(fileBuf, fbPtr - 1, 2,
              "Nested include statements are not allowed.");
        } else {
          // $[ ... $] should have been processed by readSourceAndIncludes()
          rawSourceError(fileBuf, fbPtr - 1, 2,
              "\"$[\" is unterminated or has ill-formed \"$]\".");
        }
        continue;
      case ']':
        if (mode == 0) {
          rawSourceError(fileBuf, fbPtr - 1, 2,
              "A \"$[\" is required before \"$]\".");
          continue;
        }

        // Because $[ $] are already expanded here, this should never happen
        bug(1759);

        continue;

      case '{':
      case '}':
      case '.':
        potentialStatements++; // Number of potential statements for malloc
        break;
    } // End switch fbPtr[0]
  } // End while

  if (fbPtr != fileBuf + charCount) {
    // To help debugging:
    printf("fbPtr=%ld fileBuf=%ld charCount=%ld diff=%ld\n",
        (long)fbPtr, (long)fileBuf, charCount, fbPtr - fileBuf - charCount);
    bug(1704);
  }

  print2("%ld bytes were read into the source buffer.\n", charCount);

  if (*size != charCount) bug(1761);
  return fileBuf;
} // readRawSource

// This function initializes the g_Statement[] structure array and assigns
// sections of the raw input text.  statements is updated.
// g_sourcePtr is assumed to point to the raw input buffer.
// g_sourceLen is assumed to be length of the raw input buffer.
void parseKeywords(void)
{
  long i, j, k;
  char *fbPtr;
  flag insideComment;
  char mode, type;
  char *startSection;
  char tmpch;
  long dollarPCount = 0; // For statistics only
  long dollarACount = 0; // For statistics only

  // Initialization to avoid compiler warning (should not be theoretically
  // necessary).
  type = 0;

  // Determine the upper limit of the number of new statements added for
  // allocation purposes (computed in readRawInput).
  potentialStatements = potentialStatements + 3; // To be cautious
/*E*/if(db5)print2("There are up to %ld potential statements.\n",
/*E*/   potentialStatements);

  // Reallocate the statement array for all potential statements
  g_Statement = realloc(g_Statement, (size_t)potentialStatements
      * sizeof(struct statement_struct));
  if (!g_Statement) outOfMemory("#4 (statement)");

  // Initialize the statement array
  i = 0;
  g_Statement[i].lineNum = 0; // assigned by assignStmtFileAndLineNum()
  g_Statement[i].fileName = ""; // assigned by assignStmtFileAndLineNum()
  g_Statement[i].labelName = "";
  g_Statement[i].uniqueLabel = 0;
  g_Statement[i].type = illegal_;
  g_Statement[i].scope = 0;
  g_Statement[i].beginScopeStatementNum = 0;
  g_Statement[i].endScopeStatementNum = 0;
  g_Statement[i].labelSectionPtr = "";
  g_Statement[i].labelSectionLen = 0;
  g_Statement[i].labelSectionChanged = 0;
  g_Statement[i].statementPtr = ""; // input to assignStmtFileAndLineNum()
  g_Statement[i].mathSectionPtr = "";
  g_Statement[i].mathSectionLen = 0;
  g_Statement[i].mathSectionChanged = 0;
  g_Statement[i].proofSectionPtr = "";
  g_Statement[i].proofSectionLen = 0;
  g_Statement[i].proofSectionChanged = 0;
  g_Statement[i].mathString = NULL_NMBRSTRING;
  g_Statement[i].mathStringLen = 0;
  g_Statement[i].proofString = NULL_NMBRSTRING;
  g_Statement[i].reqHypList = NULL_NMBRSTRING;
  g_Statement[i].optHypList = NULL_NMBRSTRING;
  g_Statement[i].numReqHyp = 0;
  g_Statement[i].reqVarList = NULL_NMBRSTRING;
  g_Statement[i].optVarList = NULL_NMBRSTRING;
  g_Statement[i].reqDisjVarsA = NULL_NMBRSTRING;
  g_Statement[i].reqDisjVarsB = NULL_NMBRSTRING;
  g_Statement[i].reqDisjVarsStmt = NULL_NMBRSTRING;
  g_Statement[i].optDisjVarsA = NULL_NMBRSTRING;
  g_Statement[i].optDisjVarsB = NULL_NMBRSTRING;
  g_Statement[i].optDisjVarsStmt = NULL_NMBRSTRING;
  g_Statement[i].pinkNumber = 0;
  g_Statement[i].headerStartStmt = 0;
  for (i = 1; i < potentialStatements; i++) {
    g_Statement[i] = g_Statement[0];
  }

  // In case there is no relevant statement (originally added for MARKUP
  // command).
  g_Statement[0].labelName = "(N/A)";

/*E*/if(db5)print2("Finished initializing statement array.\n");

  // Fill in the statement array with raw source text
  fbPtr = g_sourcePtr;
  mode = 0; // 0 = label section, 1 = math section, 2 = proof section
  insideComment = 0; // 1 = inside comment
  startSection = fbPtr;

  while (1) {
    // Find a keyword or newline
    tmpch = fbPtr[0];
    if (!tmpch) { // End of file
      if (mode != 0) {
        sourceError(fbPtr - 1, 2, g_statements,
            "Expected \"$.\" here (last line of file).");
        if (g_statements) { // Adjustment for error messages
          startSection = g_Statement[g_statements].labelSectionPtr;
          g_statements--;
        }
      }
      break;
    }

    if (tmpch != '$') {
      fbPtr++;
      continue;
    }
    fbPtr++;
    switch (fbPtr[0]) {
      case '$': // "$$" means literal "$"
        fbPtr++;
        continue;
      case '(': // Start of comment
        insideComment = 1;
        continue;
      case ')': // End of comment
        insideComment = 0;
        continue;
    }
    if (insideComment) continue;
    switch (fbPtr[0]) {
      case 'c':  type = c_; break;
      case 'v':  type = v_; break;
      case 'e':  type = e_; break;
      case 'f':  type = f_; break;
      case 'd':  type = d_; break;
      case 'a':  type = a_; dollarACount++; break;
      case 'p':  type = p_; dollarPCount++; break;
      case '{':  type = lb_; break;
      case '}':  type = rb_; break;
    }
    switch (fbPtr[0]) {
      case 'c':
      case 'v':
      case 'e':
      case 'f':
      case 'd':
      case 'a':
      case 'p':
      case '{':
      case '}':
        if (mode != 0) {
          if (mode == 2 || type != p_) {
            sourceError(fbPtr - 1, 2, g_statements,
                "Expected \"$.\" here.");
          } else {
            sourceError(fbPtr - 1, 2, g_statements,
                "Expected \"$=\" here.");
          }
          continue;
        }
        // Initialize a new statement
        g_statements++;
        g_Statement[g_statements].type = type;
        g_Statement[g_statements].labelSectionPtr = startSection;
        g_Statement[g_statements].labelSectionLen = fbPtr - startSection - 1;
        // The character after label section is used by
        // assignStmtFileAndLineNum() to determine the "line number" for the
        // statement as a whole.
        g_Statement[g_statements].statementPtr = startSection
            + g_Statement[g_statements].labelSectionLen;
        startSection = fbPtr + 1;
        if (type != lb_ && type != rb_) mode = 1;
        continue;
      default:
        if (mode == 0) {
          sourceError(fbPtr - 1, 2, g_statements, cat(
              "Expected \"$c\", \"$v\", \"$e\", \"$f\", \"$d\",",
              " \"$a\", \"$p\", \"${\", or \"$}\" here.",NULL));
          continue;
        }
        if (mode == 1) {
          if (type == p_ && fbPtr[0] != '=') {
            sourceError(fbPtr - 1, 2, g_statements,
                "Expected \"$=\" here.");
            if (fbPtr[0] == '.') {
              mode = 2; // If $. switch mode to help reduce error msgs
            }
          }
          if (type != p_ && fbPtr[0] != '.') {
            sourceError(fbPtr - 1, 2, g_statements,
                "Expected \"$.\" here.");
            continue;
          }
          // Add math section to statement
          g_Statement[g_statements].mathSectionPtr = startSection;
          g_Statement[g_statements].mathSectionLen = fbPtr - startSection - 1;
          startSection = fbPtr + 1;
          if (type == p_ && mode != 2) // !error msg case
          {
            mode = 2; // Switch mode to proof section
          } else {
            mode = 0;
          }
          continue;
        } // End if mode == 1
        if (mode == 2) {
          if (fbPtr[0] != '.') {
            sourceError(fbPtr - 1, 2, g_statements,
                "Expected \"$.\" here.");
            continue;
          }
          // Add proof section to statement
          g_Statement[g_statements].proofSectionPtr = startSection;
          g_Statement[g_statements].proofSectionLen = fbPtr - startSection - 1;
          startSection = fbPtr + 1;
          mode = 0;
          continue;
        } // End if mode == 2
    } // End switch fbPtr[0]
  } // End while

  if (fbPtr != g_sourcePtr + g_sourceLen) bug(1706);

  print2("The source has %ld statements; %ld are $a and %ld are $p.\n",
       g_statements, dollarACount, dollarPCount);

  // Put chars after the last $. into the label section of a dummy statement
  g_Statement[g_statements + 1].type = illegal_;
  g_Statement[g_statements + 1].labelSectionPtr = startSection;
  g_Statement[g_statements + 1].labelSectionLen = fbPtr - startSection;
  // Point to last character of file in case we ever need lineNum/fileName
  g_Statement[g_statements + 1].statementPtr = fbPtr - 1;

  // Initialize the pink number to print after the statement labels
  // in HTML output.
  // The pink number only counts $a and $p statements, unlike the statement
  // number which also counts $f, $e, $c, $v, ${, $}
  j = 0;
  k = 0;
  for (i = 1; i <= g_statements; i++) {
    if (g_Statement[i].type == a_ || g_Statement[i].type == p_) {

      // Use the statement _after_ the previous $a or $p; that is the start
      // of the "header area" for use by getSectionHeadings() in mmwtex.c.
      // headerStartStmt will be equal to the current statement if the
      // previous statement is also a $a or $p).
      g_Statement[i].headerStartStmt = k + 1;
      k = i;

      j++;
      g_Statement[i].pinkNumber = j;
    }
  }
  // Also, put the largest pink number in the last statement, no
  // matter what it kind it is, so we can look up the largest number in
  // pinkHTML() in mmwtex.c
  g_Statement[g_statements].pinkNumber = j;

/*E*/if(db5){for (i=1; i<=g_statements; i++){
/*E*/  if (i == 5) { print2("(etc.)\n");} else { if (i<5) {
/*E*/  assignStmtFileAndLineNum(i);
/*E*/  print2("Statement %ld: line %ld file %s.\n",i,g_Statement[i].lineNum,
/*E*/      g_Statement[i].fileName);
/*E*/}}}}
}

// This function parses the label sections of the g_Statement[] structure array.
// g_sourcePtr is assumed to point to the beginning of the raw input buffer.
// g_sourceLen is assumed to be length of the raw input buffer.
void parseLabels(void) {
  long i, j, k;
  char *fbPtr;
  char type;
  long stmt;
  flag dupFlag;

  // Define the legal label characters
  for (i = 0; i < 256; i++) {
    illegalLabelChar[i] = !isalnum(i);
  }
  illegalLabelChar['-'] = 0;
  illegalLabelChar['_'] = 0;
  illegalLabelChar['.'] = 0;

  // Scan all statements and extract their labels
  for (stmt = 1; stmt <= g_statements; stmt++) {
    type = g_Statement[stmt].type;
    fbPtr = g_Statement[stmt].labelSectionPtr;
    fbPtr = fbPtr + whiteSpaceLen(fbPtr);
    j = tokenLen(fbPtr);
    if (j) {
      for (k = 0; k < j; k++) {
        if (illegalLabelChar[(unsigned char)fbPtr[k]]) {
          sourceError(fbPtr + k, 1, stmt,
        "Only letters, digits, \"_\", \"-\", and \".\" are allowed in labels.");
          break;
        }
      }
      switch (type) {
        case d_:
        case rb_:
        case lb_:
        case v_:
        case c_:
          sourceError(fbPtr, j, stmt,
                "A label isn't allowed for this statement type.");
      }
      g_Statement[stmt].labelName = malloc((size_t)j + 1);
      if (!g_Statement[stmt].labelName) outOfMemory("#5 (label)");
      g_Statement[stmt].labelName[j] = 0;
      memcpy(g_Statement[stmt].labelName, fbPtr, (size_t)j);
      fbPtr = fbPtr + j;
      fbPtr = fbPtr + whiteSpaceLen(fbPtr);
      j = tokenLen(fbPtr);
      if (j) {
        sourceError(fbPtr, j, stmt,
            "A statement may have only one label.");
      }
    } else {
      switch (type) {
        case e_:
        case f_:
        case a_:
        case p_:
          sourceError(fbPtr, 2, stmt,
                "A label is required for this statement type.");
      }
    }
  } // Next stmt

  // Make sure there is no token after the last statement
  fbPtr = g_Statement[g_statements + 1].labelSectionPtr; // Last (dummy) statement
  i = whiteSpaceLen(fbPtr);
  j = tokenLen(fbPtr + i);
  if (j) {
    sourceError(fbPtr + i, j, 0,
        "There should be no tokens after the last statement.");
  }

  // Sort the labels for later lookup
  g_labelKey = malloc(((size_t)g_statements + 1) * sizeof(long));
  if (!g_labelKey) outOfMemory("#6 (g_labelKey)");
  for (i = 1; i <= g_statements; i++) {
    g_labelKey[i] = i;
  }
  g_labelKeyBase = &g_labelKey[1];
  g_numLabelKeys = g_statements;
  qsort(g_labelKeyBase, (size_t)g_numLabelKeys, sizeof(long), labelSortCmp);

  // Skip null labels.
  for (i = 1; i <= g_statements; i++) {
    if (g_Statement[g_labelKey[i]].labelName[0]) break;
  }
  g_labelKeyBase = &g_labelKey[i];
  g_numLabelKeys = g_statements - i + 1;
/*E*/if(db5)print2("There are %ld non-empty labels.\n", g_numLabelKeys);
/*E*/if(db5){print2("The first (up to 5) sorted labels are:\n");
/*E*/  for (i=0; i<5; i++) {
/*E*/    if (i >= g_numLabelKeys) break;
/*E*/    print2("%s ",g_Statement[g_labelKeyBase[i]].labelName);
/*E*/  } print2("\n");}

  // Copy the keys for all possible labels for lookup by the
  // squishProof command when local labels are generated in packed proofs.
  g_allLabelKeyBase = malloc((size_t)g_numLabelKeys * sizeof(long));
  if (!g_allLabelKeyBase) outOfMemory("#60 (g_allLabelKeyBase)");
  memcpy(g_allLabelKeyBase, g_labelKeyBase, (size_t)g_numLabelKeys * sizeof(long));
  g_numAllLabelKeys = g_numLabelKeys;

  // Now back to the regular label stuff.
  // Check for duplicate labels.
  // (This will go away if local labels on hypotheses are allowed.)
  dupFlag = 0;
  for (i = 0; i < g_numLabelKeys; i++) {
    if (dupFlag) {
      // This "if" condition causes only the 2nd in a pair of duplicate labels
      // to have an error message.
      dupFlag = 0;
      if (!strcmp(g_Statement[g_labelKeyBase[i]].labelName,
          g_Statement[g_labelKeyBase[i - 1]].labelName)) dupFlag = 1;
    }
    if (i < g_numLabelKeys - 1) {
      if (!strcmp(g_Statement[g_labelKeyBase[i]].labelName,
          g_Statement[g_labelKeyBase[i + 1]].labelName)) dupFlag = 1;
    }
    if (dupFlag) {
      fbPtr = g_Statement[g_labelKeyBase[i]].labelSectionPtr;
      k = whiteSpaceLen(fbPtr);
      j = tokenLen(fbPtr + k);
      sourceError(fbPtr + k, j, g_labelKeyBase[i],
         "This label is declared more than once.  All labels must be unique.");
    }
  }
}

// This functions retrieves all possible math symbols from $c and $v
// statements.
void parseMathDecl(void) {
  long potentialSymbols;
  long stmt;
  char *fbPtr;
  long i, j, k;
  char *tmpPtr;
  nmbrString *nmbrTmpPtr;
  long oldG_mathTokens;
  void *voidPtr; // bsearch returned value

  // Find the upper limit of the number of symbols declared for
  // pre-allocation:  at most, the number of symbols is half the number of
  // characters, since $c and $v statements require white space.
  potentialSymbols = 0;
  for (stmt = 1; stmt <= g_statements; stmt++) {
    switch (g_Statement[stmt].type) {
      case c_:
      case v_:
        potentialSymbols = potentialSymbols + g_Statement[stmt].mathSectionLen;
    }
  }
  potentialSymbols = (potentialSymbols / 2) + 2;
/*E*/if(db5)print2("%ld potential symbols were computed.\n",potentialSymbols);
  g_MathToken = realloc(g_MathToken, (size_t)potentialSymbols *
      sizeof(struct mathToken_struct));
  if (!g_MathToken) outOfMemory("#7 (g_MathToken)");

  // Scan $c and $v statements to accumulate all possible math symbols
  g_mathTokens = 0;
  for (stmt = 1; stmt <= g_statements; stmt++) {
    switch (g_Statement[stmt].type) {
      case c_:
      case v_:
        oldG_mathTokens = g_mathTokens;
        fbPtr = g_Statement[stmt].mathSectionPtr;
        while (1) {
          i = whiteSpaceLen(fbPtr);
          j = tokenLen(fbPtr + i);
          if (!j) break;
          tmpPtr = malloc((size_t)j + 1); // Math symbol name
          if (!tmpPtr) outOfMemory("#8 (symbol name)");
          tmpPtr[j] = 0; // End of string
          memcpy(tmpPtr, fbPtr + i, (size_t)j);
          fbPtr = fbPtr + i + j;
          // Create a new math symbol
          g_MathToken[g_mathTokens].tokenName = tmpPtr;
          g_MathToken[g_mathTokens].length = j;
          if (g_Statement[stmt].type == c_) {
            g_MathToken[g_mathTokens].tokenType = (char)con_;
          } else {
            g_MathToken[g_mathTokens].tokenType = (char)var_;
          }
          g_MathToken[g_mathTokens].active = 0;
          g_MathToken[g_mathTokens].scope = 0; // Unknown for now
          g_MathToken[g_mathTokens].tmp = 0; // Not used for now
          g_MathToken[g_mathTokens].statement = stmt;
          // Unknown for now (Assign to 'g_statements' in case it's active until the end).
          g_MathToken[g_mathTokens].endStatement = g_statements;
          g_mathTokens++;
        }

        // Create the symbol list for this statement
        j = g_mathTokens - oldG_mathTokens; // Number of tokens in this statement
        nmbrTmpPtr = poolFixedMalloc((j + 1) * (long)(sizeof(nmbrString)));
        nmbrTmpPtr[j] = -1;
        for (i = 0; i < j; i++) {
          nmbrTmpPtr[i] = oldG_mathTokens + i;
        }
        g_Statement[stmt].mathString = nmbrTmpPtr;
        g_Statement[stmt].mathStringLen = j;
        if (!j) {
          sourceError(fbPtr, 2, stmt,
           "At least one math symbol should be declared.");
        }
    } // end switch (g_Statement[stmt].type)
  } // next stmt

/*E*/if(db5)print2("%ld math symbols were declared.\n",g_mathTokens);
  // Reallocate from potential to actual to reduce memory space.
  // Add 100 to allow for initial Proof Assistant use, and up to 100
  // errors in undeclared token references.
  g_MAX_MATHTOKENS = g_mathTokens + 100;
  g_MathToken = realloc(g_MathToken, (size_t)g_MAX_MATHTOKENS *
      sizeof(struct mathToken_struct));
  if (!g_MathToken) outOfMemory("#10 (g_MathToken)");

  // Create a special "$|$" boundary token to separate real and dummy ones
  g_MathToken[g_mathTokens].tokenName = "";
  let(&g_MathToken[g_mathTokens].tokenName, "$|$");
  g_MathToken[g_mathTokens].length = 2; // Never used
  g_MathToken[g_mathTokens].tokenType = (char)con_;
  g_MathToken[g_mathTokens].active = 0; // Never used
  g_MathToken[g_mathTokens].scope = 0; // Never used
  g_MathToken[g_mathTokens].tmp = 0; // Never used
  g_MathToken[g_mathTokens].statement = 0; // Never used
  g_MathToken[g_mathTokens].endStatement = g_statements; // Never used

  // Sort the math symbols for later lookup
  g_mathKey = malloc((size_t)g_mathTokens * sizeof(long));
  if (!g_mathKey) outOfMemory("#11 (g_mathKey)");
  for (i = 0; i < g_mathTokens; i++) {
    g_mathKey[i] = i;
  }
  qsort(g_mathKey, (size_t)g_mathTokens, sizeof(long), mathSortCmp);
/*E*/if(db5){print2("The first (up to 5) sorted math tokens are:\n");
/*E*/  for (i=0; i<5; i++) {
/*E*/    if (i >= g_mathTokens) break;
/*E*/    print2("%s ",g_MathToken[g_mathKey[i]].tokenName);
/*E*/  } print2("\n");}

  // Check for labels with the same name as math tokens.
  // (This section implements the Metamath spec change proposed by O'Cat that
  // lets labels and math tokens occupy the same namespace and thus forbids
  // them from having common names.)
  // For maximum speed, we scan M math tokens and look each up in the list
  // of L labels.  The we have M * log L comparisons, which is optimal when
  // (as in most cases) M << L.
  for (i = 0; i < g_mathTokens; i++) {
    // See if the math token is in the list of labels
    voidPtr = (void *)bsearch(g_MathToken[i].tokenName, g_labelKeyBase,
        (size_t)g_numLabelKeys, sizeof(long), labelSrchCmp);
    if (voidPtr) { // A label matching the token was found
      stmt = (*(long *)voidPtr); // Statement number
      fbPtr = g_Statement[stmt].labelSectionPtr;
      k = whiteSpaceLen(fbPtr);
      j = tokenLen(fbPtr + k);
      // Note that the line and file are only assigned when requested,
      // for speedup.
      assignStmtFileAndLineNum(stmt);
      assignStmtFileAndLineNum(g_MathToken[i].statement);
      sourceError(fbPtr + k, j, stmt, cat(
         "This label has the same name as the math token declared on line ",
         str((double)(g_Statement[g_MathToken[i].statement].lineNum)),
         " of file \"",
         g_Statement[g_MathToken[i].statement].fileName,
         "\".", NULL));
    }
  }
}

// This functions parses statement contents, except for proofs
void parseStatements(void) {
  long stmt;
  char type;
  long i, j, k, m, n, p;
  char *fbPtr;
  long mathStringLen;
  long tokenNum;
  long lowerKey, upperKey;
  long symbolLen, origSymbolLen, mathSectionLen, g_mathKeyNum;
  void *g_mathKeyPtr; // bsearch returned value
  int maxScope;
  long reqHyps, optHyps, reqVars, optVars;
  flag reqFlag;
  int undeclErrorCount = 0;
  vstring_def(tmpStr);

  nmbrString *nmbrTmpPtr;

  long *mathTokenSameAs; // Flag that symbol is unique (for speed up)
  long *reverseMathKey; // Map from g_mathTokens to g_mathKey

  long *labelTokenSameAs; // Flag that label is unique (for speed up)
  long *reverseLabelKey; // Map from statement # to label key
  flag *labelActiveFlag; // Flag that label is active

  struct activeConstStack_struct {
    long tokenNum;
    int scope;
  };
  struct activeConstStack_struct *activeConstStack; // Stack of active consts
  long activeConstStackPtr = 0;

  struct activeVarStack_struct {
    long tokenNum;
    int scope;
    char tmpFlag; // Used by hypothesis variable scan; must be 0 otherwise
  };
  struct activeVarStack_struct *activeVarStack; // Stack of active variables
  nmbrString *wrkVarPtr1;
  nmbrString *wrkVarPtr2;
  long activeVarStackPtr = 0;

  struct activeEHypStack_struct { // Stack of $e hypotheses
    long statemNum;
    nmbrString *varList; // List of variables in the hypothesis
    int scope;
  };
  struct activeEHypStack_struct *activeEHypStack;
  long activeEHypStackPtr = 0;
  struct activeFHypStack_struct { // Stack of $f hypotheses
    long statemNum;
    nmbrString *varList; // List of variables in the hypothesis
    int scope;
  };
  struct activeFHypStack_struct *activeFHypStack;
  long activeFHypStackPtr = 0;
  nmbrString *wrkHypPtr1;
  nmbrString *wrkHypPtr2;
  nmbrString *wrkHypPtr3;
  // Starting value; could be as large as g_statements.
  long activeHypStackSize = 30; 
                                   

  struct activeDisjHypStack_struct { // Stack of disjoint variables in $d's
    long tokenNumA; // First variable in disjoint pair
    long tokenNumB; // Second variable in disjoint pair
    long statemNum; // Statement it occurred in
    int scope;
  };
  struct activeDisjHypStack_struct *activeDisjHypStack;
  nmbrString *wrkDisjHPtr1A;
  nmbrString *wrkDisjHPtr1B;
  nmbrString *wrkDisjHPtr1Stmt;
  nmbrString *wrkDisjHPtr2A;
  nmbrString *wrkDisjHPtr2B;
  nmbrString *wrkDisjHPtr2Stmt;
  long activeDisjHypStackPtr = 0;
  // Starting value; could be as large as about g_mathTokens^2/2
  long activeDisjHypStackSize = 30;

  // Temporary working space
  long wrkLen;
  nmbrString *wrkNmbrPtr;
  char *wrkStrPtr;

  long maxSymbolLen; // Longest math symbol (for speedup)
  flag *symbolLenExists; // A symbol with this length exists (for speedup)

  long beginScopeStmtNum = 0;

  // Initialization to avoid compiler warning (should not be theoretically
  // necessary).
  mathStringLen = 0;
  tokenNum = 0;

  // Initialize flags for g_mathKey array that identify math symbols as
  // unique (when 0) or, if not unique, the flag is a number identifying a group
  // of identical names.
  mathTokenSameAs = malloc((size_t)g_mathTokens * sizeof(long));
  if (!mathTokenSameAs) outOfMemory("#12 (mathTokenSameAs)");
  reverseMathKey = malloc((size_t)g_mathTokens * sizeof(long));
  if (!reverseMathKey) outOfMemory("#13 (reverseMathKey)");
  for (i = 0; i < g_mathTokens; i++) {
    mathTokenSameAs[i] = 0; // 0 means unique
    reverseMathKey[g_mathKey[i]] = i; // Initialize reverse map to g_mathKey
  }
  for (i = 1; i < g_mathTokens; i++) {
    if (!strcmp(g_MathToken[g_mathKey[i]].tokenName,
        g_MathToken[g_mathKey[i - 1]].tokenName)) {
      if (!mathTokenSameAs[i - 1]) mathTokenSameAs[i - 1] = i;
      mathTokenSameAs[i] = mathTokenSameAs[i - 1];
    }
  }

  // Initialize flags for g_labelKey array that identify labels as
  // unique (when 0) or, if not unique, the flag is a number identifying a group
  // of identical names
  labelTokenSameAs = malloc(((size_t)g_statements + 1) * sizeof(long));
  if (!labelTokenSameAs) outOfMemory("#112 (labelTokenSameAs)");
  reverseLabelKey = malloc(((size_t)g_statements + 1) * sizeof(long));
  if (!reverseLabelKey) outOfMemory("#113 (reverseLabelKey)");
  labelActiveFlag = malloc(((size_t)g_statements + 1) * sizeof(flag));
  if (!labelActiveFlag) outOfMemory("#114 (labelActiveFlag)");
  for (i = 1; i <= g_statements; i++) {
    labelTokenSameAs[i] = 0; // Initialize:  0 = unique
    reverseLabelKey[g_labelKey[i]] = i; // Initialize reverse map to g_labelKey
    labelActiveFlag[i] = 0; // Initialize
  }
  for (i = 2; i <= g_statements; i++) {
    if (!strcmp(g_Statement[g_labelKey[i]].labelName,
        g_Statement[g_labelKey[i - 1]].labelName)) {
      if (!labelTokenSameAs[i - 1]) labelTokenSameAs[i - 1] = i;
      labelTokenSameAs[i] = labelTokenSameAs[i - 1];
    }
  }

  // Initialize variable and hypothesis stacks

  // Allocate g_MAX_MATHTOKENS and not just g_mathTokens of them so that
  // they can accommodate any extra non-declared tokens (which get
  // declared as part of error handling, where the g_MAX_MATHTOKENS
  // limit is checked).
  activeConstStack = malloc((size_t)g_MAX_MATHTOKENS
      * sizeof(struct activeConstStack_struct));
  activeVarStack = malloc((size_t)g_MAX_MATHTOKENS
      * sizeof(struct activeVarStack_struct));
  wrkVarPtr1 = malloc((size_t)g_MAX_MATHTOKENS * sizeof(nmbrString));
  wrkVarPtr2 = malloc((size_t)g_MAX_MATHTOKENS * sizeof(nmbrString));
  if (!activeConstStack || !activeVarStack || !wrkVarPtr1 || !wrkVarPtr2)
      outOfMemory("#14 (activeVarStack)");

  activeEHypStack = malloc((size_t)activeHypStackSize
      * sizeof(struct activeEHypStack_struct));
  activeFHypStack = malloc((size_t)activeHypStackSize
      * sizeof(struct activeFHypStack_struct));
  wrkHypPtr1 = malloc((size_t)activeHypStackSize * sizeof(nmbrString));
  wrkHypPtr2 = malloc((size_t)activeHypStackSize * sizeof(nmbrString));
  wrkHypPtr3 = malloc((size_t)activeHypStackSize * sizeof(nmbrString));
  if (!activeEHypStack || !activeFHypStack || !wrkHypPtr1 || !wrkHypPtr2 ||
      !wrkHypPtr3)
      outOfMemory("#15 (activeHypStack)");

  activeDisjHypStack = malloc((size_t)activeDisjHypStackSize *
      sizeof(struct activeDisjHypStack_struct));
  wrkDisjHPtr1A = malloc((size_t)activeDisjHypStackSize * sizeof(nmbrString));
  wrkDisjHPtr1B = malloc((size_t)activeDisjHypStackSize * sizeof(nmbrString));
  wrkDisjHPtr1Stmt = malloc((size_t)activeDisjHypStackSize
      * sizeof(nmbrString));
  wrkDisjHPtr2A = malloc((size_t)activeDisjHypStackSize * sizeof(nmbrString));
  wrkDisjHPtr2B = malloc((size_t)activeDisjHypStackSize * sizeof(nmbrString));
  wrkDisjHPtr2Stmt = malloc((size_t)activeDisjHypStackSize
      * sizeof(nmbrString));
  if (!activeDisjHypStack
      || !wrkDisjHPtr1A || !wrkDisjHPtr1B || !wrkDisjHPtr1Stmt
      || !wrkDisjHPtr2A || !wrkDisjHPtr2B || !wrkDisjHPtr2Stmt)
      outOfMemory("#27 (activeDisjHypStack)");

  // Initialize temporary working space for parsing tokens
  wrkLen = 1;
  wrkNmbrPtr = malloc((size_t)wrkLen * sizeof(nmbrString));
  if (!wrkNmbrPtr) outOfMemory("#22 (wrkNmbrPtr)");
  wrkStrPtr = malloc((size_t)wrkLen + 1);
  if (!wrkStrPtr) outOfMemory("#23 (wrkStrPtr)");

  // Find declared math symbol lengths (used to speed up parsing)
  maxSymbolLen = 0;
  for (i = 0; i < g_mathTokens; i++) {
    if (g_MathToken[i].length > maxSymbolLen) {
      maxSymbolLen = g_MathToken[i].length;
    }
  }
  symbolLenExists = malloc(((size_t)maxSymbolLen + 1) * sizeof(flag));
  if (!symbolLenExists) outOfMemory("#25 (symbolLenExists)");
  for (i = 0; i <= maxSymbolLen; i++) {
    symbolLenExists[i] = 0;
  }
  for (i = 0; i < g_mathTokens; i++) {
    symbolLenExists[g_MathToken[i].length] = 1;
  }

  g_currentScope = 0;
  beginScopeStmtNum = 0;

  // Scan all statements.  Fill in statement structure and look for errors.
  for (stmt = 1; stmt <= g_statements; stmt++) {

    g_Statement[stmt].beginScopeStatementNum = beginScopeStmtNum;
    // endScopeStatementNum is always 0 except in ${ statements
    g_Statement[stmt].endScopeStatementNum = 0;
    g_Statement[stmt].scope = g_currentScope;
    type = g_Statement[stmt].type;
    // ****** Determine scope, stack active variables, process math strings ******

    switch (type) {
      case lb_:
        g_currentScope++;
        // Not really an out-of-memory situation, but use the error msg.
        if (g_currentScope > 32000) outOfMemory("#16 (more than 32000 \"${\"s)");
        // Note that g_Statement[stmt].beginScopeStatementNum for this ${
        // points to the previous ${ (or 0 if in outermost scope).
        beginScopeStmtNum = stmt;
        // Note that g_Statement[stmt].endScopeStatementNum for this ${
        // will be assigned in the rb_ case below.
        break;
      case rb_:
        // Remove all variables and hypotheses in current scope from stack

        while (activeConstStackPtr) {
          if (activeConstStack[activeConstStackPtr - 1].scope < g_currentScope)
              break;
          activeConstStackPtr--;
          g_MathToken[activeConstStack[activeConstStackPtr].tokenNum].active = 0;
          g_MathToken[activeConstStack[activeConstStackPtr].tokenNum
              ].endStatement = stmt;
        }

        while (activeVarStackPtr) {
          if (activeVarStack[activeVarStackPtr - 1].scope < g_currentScope) break;
          activeVarStackPtr--;
          g_MathToken[activeVarStack[activeVarStackPtr].tokenNum].active = 0;
          g_MathToken[activeVarStack[activeVarStackPtr].tokenNum].endStatement
              = stmt;
        }

        while (activeEHypStackPtr) {
          if (activeEHypStack[activeEHypStackPtr - 1].scope < g_currentScope)
              break;
          activeEHypStackPtr--;
          // Make the label inactive
          labelActiveFlag[activeEHypStack[activeEHypStackPtr].statemNum] = 0;                              
          free(activeEHypStack[activeEHypStackPtr].varList);
        }
        while (activeFHypStackPtr) {
          if (activeFHypStack[activeFHypStackPtr - 1].scope < g_currentScope)
              break;
          activeFHypStackPtr--;
          // Make the label inactive
          labelActiveFlag[activeFHypStack[activeFHypStackPtr].statemNum] = 0;                               
          free(activeFHypStack[activeFHypStackPtr].varList);
        }
        while (activeDisjHypStackPtr) {
          if (activeDisjHypStack[activeDisjHypStackPtr - 1].scope
              < g_currentScope) break;
          activeDisjHypStackPtr--;
        }
        g_currentScope--;
        if (g_currentScope < 0) {
          sourceError(g_Statement[stmt].labelSectionPtr +
              g_Statement[stmt].labelSectionLen, 2, stmt,
              "Too many \"$}\"s at this point.");
        }
        // We're not in outermost scope (precaution if there were too many $}'s)
        if (beginScopeStmtNum > 0) {
          if (g_Statement[beginScopeStmtNum].type != lb_) bug(1773);
          // Populate the previous ${ with a pointer to this $}
          g_Statement[beginScopeStmtNum].endScopeStatementNum = stmt;
          // Update beginScopeStmtNum with start of outer scope
          beginScopeStmtNum
              = g_Statement[beginScopeStmtNum].beginScopeStatementNum;
        }

        break;
      case c_:
      case v_:
        // Scan all symbols declared (they have already been parsed) and
        // flag them as active, add to stack, and check for errors.
        if (type == c_) {
          if (g_currentScope > 0) {
            sourceError(g_Statement[stmt].labelSectionPtr +
                g_Statement[stmt].labelSectionLen, 2, stmt,
        "A \"$c\" constant declaration may occur in the outermost scope only.");
          }
        }

        i = 0; // Symbol position in mathString
        nmbrTmpPtr = g_Statement[stmt].mathString;
        while (1) {
          tokenNum = nmbrTmpPtr[i];
          if (tokenNum == -1) break; // Done scanning symbols in $v or $c
          if (mathTokenSameAs[reverseMathKey[tokenNum]]) {
            // The variable name is not unique.  Find out if there's a
            // conflict with the others.
            lowerKey = reverseMathKey[tokenNum];
            upperKey = lowerKey;
            j = mathTokenSameAs[lowerKey];
            while (lowerKey) {
              if (j != mathTokenSameAs[lowerKey - 1]) break;
              lowerKey--;
            }
            while (upperKey < g_mathTokens - 1) {
              if (j != mathTokenSameAs[upperKey + 1]) break;
              upperKey++;
            }
            for (j = lowerKey; j <= upperKey; j++) {
              if (g_MathToken[g_mathKey[j]].active) {
                // Detect conflicting active vars declared
                // in multiple scopes.
                if (g_MathToken[g_mathKey[j]].scope <= g_currentScope) {
                  mathTokenError(i, nmbrTmpPtr, stmt,
                      "This symbol has already been declared in this scope.");
                }
              }
            }

            // Make sure that no constant has the same name
            // as a variable or vice-versa.
            k = 0; // Flag for $c
            m = 0; // Flag for $v
            for (j = lowerKey; j <= upperKey; j++) {
              if (g_MathToken[g_mathKey[j]].tokenType == (char)con_) k = 1;
              if (g_MathToken[g_mathKey[j]].tokenType == (char)var_) m = 1;
            }
            if ((k == 1 && g_MathToken[tokenNum].tokenType == (char)var_) ||
                (m == 1 && g_MathToken[tokenNum].tokenType == (char)con_)) {
               mathTokenError(i, nmbrTmpPtr, stmt,
                   "A symbol may not be both a constant and a variable.");
            }
          }

          // Flag the token as active
          g_MathToken[tokenNum].active = 1;
          g_MathToken[tokenNum].scope = g_currentScope;

          if (type == v_) {

            // Identify this stack position in the g_MathToken array, for use
            // by the hypothesis variable scan below.
            g_MathToken[tokenNum].tmp = activeVarStackPtr;

            // Add the symbol to the stack
            activeVarStack[activeVarStackPtr].tokenNum = tokenNum;
            activeVarStack[activeVarStackPtr].scope = g_currentScope;
            activeVarStack[activeVarStackPtr].tmpFlag = 0;
            activeVarStackPtr++;
          } else {

            // Add the symbol to the stack
            activeConstStack[activeConstStackPtr].tokenNum = tokenNum;
            activeConstStack[activeConstStackPtr].scope = g_currentScope;
            activeConstStackPtr++;
          }

          i++;
        }
        break;
      case d_:
      case f_:
      case e_:
      case a_:
      case p_:
        // Make sure we have enough working space
        mathSectionLen = g_Statement[stmt].mathSectionLen;
        if (wrkLen < mathSectionLen) {
          free(wrkNmbrPtr);
          free(wrkStrPtr);
          wrkLen = mathSectionLen + 100;
          wrkNmbrPtr = malloc((size_t)wrkLen * sizeof(nmbrString));
          if (!wrkNmbrPtr) outOfMemory("#20 (wrkNmbrPtr)");
          wrkStrPtr = malloc((size_t)wrkLen + 1);
          if (!wrkStrPtr) outOfMemory("#21 (wrkStrPtr)");
        }

        // Scan the math section for tokens
        mathStringLen = 0;
        fbPtr = g_Statement[stmt].mathSectionPtr;
        while (1) {
          fbPtr = fbPtr + whiteSpaceLen(fbPtr);
          origSymbolLen = tokenLen(fbPtr);
          if (!origSymbolLen) break; // Done scanning source line

          // Scan for largest matching token from the left
          nextAdjToken:
          // don't allow missing white space
          symbolLen = origSymbolLen;

          memcpy(wrkStrPtr, fbPtr, (size_t)symbolLen);

          // ???Speed-up is possible by rewriting this now unnecessary code
          for (; symbolLen > 0; symbolLen = 0) {

            // symbolLenExists means a symbol of this length was declared
            if (!symbolLenExists[symbolLen]) continue;
            wrkStrPtr[symbolLen] = 0; // Define end of trial token to look up
            g_mathKeyPtr = (void *)bsearch(wrkStrPtr, g_mathKey, (size_t)g_mathTokens,
                sizeof(long), mathSrchCmp);
            if (!g_mathKeyPtr) continue; // Trial token was not declared
            g_mathKeyNum = (long *)g_mathKeyPtr - g_mathKey; // Pointer arithmetic!
            if (mathTokenSameAs[g_mathKeyNum]) { // Multiply-declared symbol
              lowerKey = g_mathKeyNum;
              upperKey = lowerKey;
              j = mathTokenSameAs[lowerKey];
              while (lowerKey) {
                if (j != mathTokenSameAs[lowerKey - 1]) break;
                lowerKey--;
              }
              while (upperKey < g_mathTokens - 1) {
                if (j != mathTokenSameAs[upperKey + 1]) break;
                upperKey++;
              }
              // Find the active symbol with the most recent declaration.
              // (Note:  Here, 'active' means it's on the stack, not the
              // official def.)
              maxScope = -1;
              for (i = lowerKey; i <= upperKey; i++) {
                j = g_mathKey[i];
                if (g_MathToken[j].active) {
                  if (g_MathToken[j].scope > maxScope) {
                    tokenNum = j;
                    maxScope = g_MathToken[j].scope;
                    if (maxScope == g_currentScope) break; // Speedup
                  }
                }
              }
              if (maxScope == -1) {
                tokenNum = g_mathKey[g_mathKeyNum]; // Pick an arbitrary one
                sourceError(fbPtr, symbolLen, stmt,
       "This math symbol is not active (i.e. was not declared in this scope).");
                // ??? (This is done in 3 places. Make it a fn call & clean up?
                // Prevent stray pointers later
                g_MathToken[tokenNum].tmp = 0; // Loc in active variable stack
                if (!activeVarStackPtr) { // Make a fictitious entry
                  activeVarStack[activeVarStackPtr].tokenNum = tokenNum;
                  activeVarStack[activeVarStackPtr].scope = g_currentScope;
                  activeVarStack[activeVarStackPtr].tmpFlag = 0;
                  activeVarStackPtr++;
                }
              }
            } else { // The symbol was declared only once.
              // Same as: tokenNum = g_mathKey[g_mathKeyNum]; but faster
              tokenNum = *((long *)g_mathKeyPtr);  
              if (!g_MathToken[tokenNum].active) {
                sourceError(fbPtr, symbolLen, stmt,
       "This math symbol is not active (i.e. was not declared in this scope).");
                // Prevent stray pointers later
                g_MathToken[tokenNum].tmp = 0; // Loc in active variable stack
                if (!activeVarStackPtr) { // Make a fictitious entry
                  activeVarStack[activeVarStackPtr].tokenNum = tokenNum;
                  activeVarStack[activeVarStackPtr].scope = g_currentScope;
                  activeVarStack[activeVarStackPtr].tmpFlag = 0;
                  activeVarStackPtr++;
                }
              }
            } // End if multiply-defined symbol
            break; // The symbol was found, so we are done
          } // Next symbolLen
          if (symbolLen == 0) { // Symbol was not found
            symbolLen = tokenLen(fbPtr);
            sourceError(fbPtr, symbolLen, stmt,
      "This math symbol was not declared (with a \"$c\" or \"$v\" statement).");
            // Call the symbol a dummy token of type variable so that spurious
            // errors (constants in $d's) won't be flagged also.  Prevent
            // stray pointer to active variable stack.
            undeclErrorCount++;
            tokenNum = g_mathTokens + undeclErrorCount;
            if (tokenNum >= g_MAX_MATHTOKENS) {
              // There are current 100 places for bad tokens
              print2(
"?Error: The temporary space for holding bad tokens has run out, because\n");
              print2(
"there are too many errors.  Therefore we will force an \"out of memory\"\n");
              print2("program abort:\n");
              outOfMemory("#33 (too many errors)");
            }
            g_MathToken[tokenNum].tokenName = "";
            let(&g_MathToken[tokenNum].tokenName, left(fbPtr,symbolLen));
            g_MathToken[tokenNum].length = symbolLen;
            g_MathToken[tokenNum].tokenType = (char)var_;
            // Prevent stray pointers later
            g_MathToken[tokenNum].tmp = 0; // Location in active variable stack
            if (!activeVarStackPtr) { // Make a fictitious entry
              activeVarStack[activeVarStackPtr].tokenNum = tokenNum;
              activeVarStack[activeVarStackPtr].scope = g_currentScope;
              activeVarStack[activeVarStackPtr].tmpFlag = 0;
              activeVarStackPtr++;
            }
          }

          if (type == d_) {
            if (g_MathToken[tokenNum].tokenType == (char)con_) {
              sourceError(fbPtr, symbolLen, stmt,
                  "Constant symbols are not allowed in a \"$d\" statement.");
            }
          } else {
            if (mathStringLen == 0) {
              if (g_MathToken[tokenNum].tokenType != (char)con_) {
                sourceError(fbPtr, symbolLen, stmt, cat(
                    "The first symbol must be a constant in a \"$",
                    chr(type), "\" statement.", NULL));
              }
            } else {
              if (type == f_) {
                if (mathStringLen == 1) {
                  if (g_MathToken[tokenNum].tokenType == (char)con_) {
                    sourceError(fbPtr, symbolLen, stmt,
                "The second symbol must be a variable in a \"$f\" statement.");
                  }
                } else {
                  if (mathStringLen == 2) {
                    sourceError(fbPtr, symbolLen, stmt,
               "There cannot be more than two symbols in a \"$f\" statement.");
                  }
                }
              }
            }
          }

          // Add symbol to mathString
          wrkNmbrPtr[mathStringLen] = tokenNum;
          mathStringLen++;
          fbPtr = fbPtr + symbolLen; // Move on to next symbol

          if (symbolLen < origSymbolLen) {
            // This symbol is not separated from next by white space.
            // Speed-up: don't call tokenLen again; just jump past it.
            origSymbolLen = origSymbolLen - symbolLen;
            goto nextAdjToken; // (Instead of continue)
          }
        } // End while

        if (type == d_) {
          if (mathStringLen < 2) {
            sourceError(fbPtr, 2, stmt,
                "A \"$d\" statement requires at least two variable symbols.");
          }
        } else {
          if (!mathStringLen) {
            sourceError(fbPtr, 2, stmt,
                "This statement type requires at least one math symbol.");
          } else {
            if (type == f_ && mathStringLen < 2) {
              sourceError(fbPtr, 2, stmt,
                  "A \"$f\" statement requires two math symbols.");
            }
          }
        }

        // Assign mathString to statement array
        nmbrTmpPtr = poolFixedMalloc(
            (mathStringLen + 1) * (long)(sizeof(nmbrString)));
        for (i = 0; i < mathStringLen; i++) {
          nmbrTmpPtr[i] = wrkNmbrPtr[i];
        }
        nmbrTmpPtr[mathStringLen] = -1;
        g_Statement[stmt].mathString = nmbrTmpPtr;
        g_Statement[stmt].mathStringLen = mathStringLen;
/*E*/if(db5){if(stmt<5)print2("Statement %ld mathString: %s.\n",stmt,
/*E*/  nmbrCvtMToVString(nmbrTmpPtr)); if(stmt==5)print2("(etc.)\n");}

        break; // Switch case break
      default:
        bug(1707);
    } // End switch

    // ***** Process hypothesis and variable stacks *****
    // (The switch section above does not depend on what is done in this
    // section, although this section assumes the above section has been done.
    // Variables valid only in this pass of the above section are so
    // indicated.)

    switch (type) {
      case f_:
      case e_:
      case a_:
      case p_:
        // Flag the label as active
        labelActiveFlag[stmt] = 1;
    } // End switch

    switch (type) {
      case d_:

        nmbrTmpPtr = g_Statement[stmt].mathString;
        // Stack all possible pairs of disjoint variables.
        // mathStringLen is from the above switch section; it is valid
        // only in this pass of the above section.
        for (i = 0; i < mathStringLen - 1; i++) {
          p = nmbrTmpPtr[i];
          for (j = i + 1; j < mathStringLen; j++) {
            n = nmbrTmpPtr[j];
            // Get the disjoint variable pair m and n, sorted by tokenNum
            if (p < n) {
              m = p;
            } else {
              if (p > n) {
                // Swap them
                m = n;
                n = p;
              } else {
                mathTokenError(j, nmbrTmpPtr, stmt,
                    "All variables in a \"$d\" statement must be unique.");
                break;
              }
            }
            // See if this pair of disjoint variables is already on the stack;
            // if so, don't add it again.
            for (k = 0; k < activeDisjHypStackPtr; k++) {
              if (m == activeDisjHypStack[k].tokenNumA)
                if (n == activeDisjHypStack[k].tokenNumB)
                  break; // It matches
            }
            if (k == activeDisjHypStackPtr) {
              // It wasn't already on the stack, so add it.
              // Increase stack size if necessary.
              if (activeDisjHypStackPtr >= activeDisjHypStackSize) {
                free(wrkDisjHPtr1A);
                free(wrkDisjHPtr1B);
                free(wrkDisjHPtr1Stmt);
                free(wrkDisjHPtr2A);
                free(wrkDisjHPtr2B);
                free(wrkDisjHPtr2Stmt);
                activeDisjHypStackSize = activeDisjHypStackSize + 100;
                activeDisjHypStack = realloc(activeDisjHypStack,
                    (size_t)activeDisjHypStackSize
                    * sizeof(struct activeDisjHypStack_struct));
                wrkDisjHPtr1A = malloc((size_t)activeDisjHypStackSize
                    * sizeof(nmbrString));
                wrkDisjHPtr1B = malloc((size_t)activeDisjHypStackSize
                    * sizeof(nmbrString));
                wrkDisjHPtr1Stmt = malloc((size_t)activeDisjHypStackSize
                    * sizeof(nmbrString));
                wrkDisjHPtr2A = malloc((size_t)activeDisjHypStackSize
                    * sizeof(nmbrString));
                wrkDisjHPtr2B = malloc((size_t)activeDisjHypStackSize
                    * sizeof(nmbrString));
                wrkDisjHPtr2Stmt = malloc((size_t)activeDisjHypStackSize
                    * sizeof(nmbrString));
                if (!activeDisjHypStack
                    || !wrkDisjHPtr1A || !wrkDisjHPtr1B || !wrkDisjHPtr1Stmt
                    || !wrkDisjHPtr2A || !wrkDisjHPtr2B || !wrkDisjHPtr2Stmt)
                    outOfMemory("#28 (activeDisjHypStack)");
              }
              activeDisjHypStack[activeDisjHypStackPtr].tokenNumA = m;
              activeDisjHypStack[activeDisjHypStackPtr].tokenNumB = n;
              activeDisjHypStack[activeDisjHypStackPtr].scope = g_currentScope;
              activeDisjHypStack[activeDisjHypStackPtr].statemNum = stmt;

              activeDisjHypStackPtr++;
            }
          } // Next j
        } // Next i

        break; // Switch case break

      case f_:
      case e_:

        // Increase stack size if necessary.
        // For convenience, we will keep the size greater than the sum of
        // active $e and $f hypotheses, as this is the size needed for the
        // wrkHypPtr's, even though it wastes (temporary) memory for the
        // activeE and activeF structure arrays.
        if (activeEHypStackPtr + activeFHypStackPtr >= activeHypStackSize) {
          free(wrkHypPtr1);
          free(wrkHypPtr2);
          free(wrkHypPtr3);
          activeHypStackSize = activeHypStackSize + 100;
          activeEHypStack = realloc(activeEHypStack, (size_t)activeHypStackSize
              * sizeof(struct activeEHypStack_struct));
          activeFHypStack = realloc(activeFHypStack, (size_t)activeHypStackSize
              * sizeof(struct activeFHypStack_struct));
          wrkHypPtr1 = malloc((size_t)activeHypStackSize * sizeof(nmbrString));
          wrkHypPtr2 = malloc((size_t)activeHypStackSize * sizeof(nmbrString));
          wrkHypPtr3 = malloc((size_t)activeHypStackSize * sizeof(nmbrString));
          if (!activeEHypStack || !activeFHypStack || !wrkHypPtr1 ||
              !wrkHypPtr2 || !wrkHypPtr3) outOfMemory("#32 (activeHypStack)");
        }

        // Add the hypothesis to the stack
        if (type == e_) {
          activeEHypStack[activeEHypStackPtr].statemNum = stmt;
          activeEHypStack[activeEHypStackPtr].scope = g_currentScope;
        } else {
          activeFHypStack[activeFHypStackPtr].statemNum = stmt;
          activeFHypStack[activeFHypStackPtr].scope = g_currentScope;
        }

        // Create the list of variables used by this hypothesis
        reqVars = 0;
        j = 0;
        nmbrTmpPtr = g_Statement[stmt].mathString;
        k = nmbrTmpPtr[j]; // Math symbol number
        while (k != -1) {
          if (g_MathToken[k].tokenType == (char)var_) {
            if (!activeVarStack[g_MathToken[k].tmp].tmpFlag) {
              // Variable has not been already added to list
              wrkVarPtr1[reqVars] = k;
              reqVars++;
              activeVarStack[g_MathToken[k].tmp].tmpFlag = 1;
            }
          }
          j++;
          k = nmbrTmpPtr[j];
        }
        nmbrTmpPtr = malloc(((size_t)reqVars + 1) * sizeof(nmbrString));
        if (!nmbrTmpPtr) outOfMemory("#32 (hypothesis variables)");
        memcpy(nmbrTmpPtr, wrkVarPtr1, (size_t)reqVars * sizeof(nmbrString));
        nmbrTmpPtr[reqVars] = -1;
        // Clear the variable flags for future re-use
        for (i = 0; i < reqVars; i++) {
          activeVarStack[g_MathToken[nmbrTmpPtr[i]].tmp].tmpFlag = 0;
        }

        if (type == e_) {
          activeEHypStack[activeEHypStackPtr].varList = nmbrTmpPtr;
          activeEHypStackPtr++;
        } else {
          activeFHypStack[activeFHypStackPtr].varList = nmbrTmpPtr;
          activeFHypStackPtr++;
        }

        break; // Switch case break

      case a_:
      case p_:

        // Scan this statement for required variables
        reqVars = 0;
        j = 0;
        nmbrTmpPtr = g_Statement[stmt].mathString;
        k = nmbrTmpPtr[j]; // Math symbol number
        while (k != -1) {
          if (g_MathToken[k].tokenType == (char)var_) {
            if (!activeVarStack[g_MathToken[k].tmp].tmpFlag) {
              // Variable has not been already added to list
              wrkVarPtr1[reqVars] = k;
              reqVars++;
              // 2 means it's an original variable in the assertion
              // (For error-checking).
              activeVarStack[g_MathToken[k].tmp].tmpFlag = 2;
            }
          }
          j++;
          k = nmbrTmpPtr[j];
        }

        // Scan $e stack for required variables and required hypotheses
        for (i = 0; i < activeEHypStackPtr; i++) {

          // Add $e hypotheses to required list
          wrkHypPtr1[i] = activeEHypStack[i].statemNum;

          // Add the $e's variables to required variable list
          nmbrTmpPtr = activeEHypStack[i].varList;
          j = 0; // Location in variable list
          k = nmbrTmpPtr[j]; // Symbol number of variable
          while (k != -1) {
            if (!activeVarStack[g_MathToken[k].tmp].tmpFlag) {
              // Variable has not been already added to list
              wrkVarPtr1[reqVars] = k;
              reqVars++;
            }
            // Could have been 0 or 2; 1 = in some hypothesis
            activeVarStack[g_MathToken[k].tmp].tmpFlag = 1;         
            j++;
            k = nmbrTmpPtr[j];
          }
        }

        reqHyps = activeEHypStackPtr; // The number of required hyp's so far

        // We have finished determining required variables, so allocate the
        // permanent list for the statement array.
        nmbrTmpPtr = poolFixedMalloc((reqVars + 1)
            * (long)(sizeof(nmbrString)));
        // if (!nmbrTmpPtr) outOfMemory("#30 (reqVars)"); // Not necessary w/ poolMalloc
        memcpy(nmbrTmpPtr, wrkVarPtr1, (size_t)reqVars * sizeof(nmbrString));
        nmbrTmpPtr[reqVars] = -1;
        g_Statement[stmt].reqVarList = nmbrTmpPtr;

        // Scan the list of $f hypotheses to find those that are required
        optHyps = 0;
        for (i = 0; i < activeFHypStackPtr; i++) {
          nmbrTmpPtr = activeFHypStack[i].varList; // Variable list
          tokenNum = nmbrTmpPtr[0];
          if (tokenNum == -1) {
            // Default if no variables (an error in current version):
            // Add it to list of required hypotheses.
            wrkHypPtr1[reqHyps] = activeFHypStack[i].statemNum;
            reqHyps++;
            continue;
          } else {
            reqFlag = activeVarStack[g_MathToken[tokenNum].tmp].tmpFlag;
          }
          if (reqFlag) {
            // Add it to list of required hypotheses
            wrkHypPtr1[reqHyps] = activeFHypStack[i].statemNum;
            reqHyps++;
            reqFlag = 1;
            // Could have been 2; 1 = in some hypothesis
            activeVarStack[g_MathToken[tokenNum].tmp].tmpFlag = 1;               
          } else {
            // Add it to list of optional hypotheses
            wrkHypPtr2[optHyps] = activeFHypStack[i].statemNum;
            optHyps++;
          }

          // Scan the other variables in the $f hyp to check for conflicts.
          j = 1;
          tokenNum = nmbrTmpPtr[1];
          while (tokenNum != -1) {
            if (activeVarStack[g_MathToken[tokenNum].tmp].tmpFlag == 2) {
              // 2 = in $p; 1 = in some hypothesis
              activeVarStack[g_MathToken[tokenNum].tmp].tmpFlag = 1;
            }
            if (reqFlag != activeVarStack[g_MathToken[tokenNum].tmp].tmpFlag) {
              k = activeFHypStack[i].statemNum;
              m = nmbrElementIn(1, g_Statement[k].mathString, tokenNum);
              n = nmbrTmpPtr[0];
              if (reqFlag) {
                mathTokenError(m - 1, g_Statement[k].mathString, k,
                    cat("This variable does not occur in statement ",
                    str((double)stmt)," (label \"",g_Statement[stmt].labelName,
                    "\") or statement ", str((double)stmt),
                    "'s \"$e\" hypotheses, whereas variable \"",
                    g_MathToken[n].tokenName,
                   "\" DOES occur.  A \"$f\" hypothesis may not contain such a",
                    " mixture of variables.",NULL));
              } else {
                mathTokenError(m - 1, g_Statement[k].mathString, k,
                    cat("This variable occurs in statement ",
                    str((double)stmt)," (label \"",g_Statement[stmt].labelName,
                    "\") or statement ", str((double)stmt),
                    "'s \"$e\" hypotheses, whereas variable \"",
                    g_MathToken[n].tokenName,
               "\" does NOT occur.  A \"$f\" hypothesis may not contain such a",
                    " mixture of variables.",NULL));
              }
              break;
            } // End if
            j++;
            tokenNum = nmbrTmpPtr[j];
          } // End while
        } // Next i

        // Error check:  make sure that all variables in the original statement
        // appeared in some hypothesis.
        j = 0;
        nmbrTmpPtr = g_Statement[stmt].mathString;
        k = nmbrTmpPtr[j]; // Math symbol number
        while (k != -1) {
          if (g_MathToken[k].tokenType == (char)var_) {
            if (activeVarStack[g_MathToken[k].tmp].tmpFlag == 2) {
              // The variable did not appear in any hypothesis
              mathTokenError(j, g_Statement[stmt].mathString, stmt,
                    cat("This variable does not occur in any active ",
                    "\"$e\" or \"$f\" hypothesis.  All variables in \"$a\" and",
                    " \"$p\" statements must appear in at least one such",
                    " hypothesis.",NULL));
              activeVarStack[g_MathToken[k].tmp].tmpFlag = 1; // One msg per var
            }
          }
          j++;
          k = nmbrTmpPtr[j];
        }

        // We have finished determining required $e & $f hyps, so allocate the
        // permanent list for the statement array.
        // First, sort the required hypotheses by statement number order
        // into wrkHypPtr3.
        i = 0; // Start of $e's in wrkHypPtr1
        j = activeEHypStackPtr; // Start of $f's in wrkHypPtr1
        for (k = 0; k < reqHyps; k++) {
          if (i >= activeEHypStackPtr) {
            wrkHypPtr3[k] = wrkHypPtr1[j];
            j++;
            continue;
          }
          if (j >= reqHyps) {
            wrkHypPtr3[k] = wrkHypPtr1[i];
            i++;
            continue;
          }
          if (wrkHypPtr1[i] > wrkHypPtr1[j]) {
            wrkHypPtr3[k] = wrkHypPtr1[j];
            j++;
          } else {
            wrkHypPtr3[k] = wrkHypPtr1[i];
            i++;
          }
        }

        // Now do the allocation
        nmbrTmpPtr = poolFixedMalloc((reqHyps + 1)
            * (long)(sizeof(nmbrString)));
        // if (!nmbrTmpPtr) outOfMemory("#33 (reqHyps)"); // Not necessary w/ poolMalloc
        memcpy(nmbrTmpPtr, wrkHypPtr3, (size_t)reqHyps * sizeof(nmbrString));
        nmbrTmpPtr[reqHyps] = -1;
        g_Statement[stmt].reqHypList = nmbrTmpPtr;
        g_Statement[stmt].numReqHyp = reqHyps;

        // We have finished determining optional $f hyps, so allocate the
        // permanent list for the statement array.
        if (type == p_) { // Optional ones are not used by $a statements
          nmbrTmpPtr = poolFixedMalloc((optHyps + 1)
              * (long)(sizeof(nmbrString)));
          memcpy(nmbrTmpPtr, wrkHypPtr2, (size_t)optHyps * sizeof(nmbrString));
          nmbrTmpPtr[optHyps] = -1;
          g_Statement[stmt].optHypList = nmbrTmpPtr;
        }

        // Scan the list of disjoint variable ($d) hypotheses to find those
        // that are required.
        optHyps = 0;
        reqHyps = 0;
        for (i = 0; i < activeDisjHypStackPtr; i++) {
          m = activeDisjHypStack[i].tokenNumA; // First var in disjoint pair
          n = activeDisjHypStack[i].tokenNumB; // 2nd var in disjoint pair
          if (activeVarStack[g_MathToken[m].tmp].tmpFlag &&
              activeVarStack[g_MathToken[n].tmp].tmpFlag) {
            // Both variables in the disjoint pair are required, so put the
            // disjoint hypothesis in the required list.
            wrkDisjHPtr1A[reqHyps] = m;
            wrkDisjHPtr1B[reqHyps] = n;
            wrkDisjHPtr1Stmt[reqHyps] =
                activeDisjHypStack[i].statemNum;
            reqHyps++;
          } else {
            // At least one variable is not required, so the disjoint hypothesis\
            // is not required.
            wrkDisjHPtr2A[optHyps] = m;
            wrkDisjHPtr2B[optHyps] = n;
            wrkDisjHPtr2Stmt[optHyps] =
                activeDisjHypStack[i].statemNum;
            optHyps++;
          }
        }

        // We have finished determining required $d hyps, so allocate the
        // permanent list for the statement array.

        nmbrTmpPtr = poolFixedMalloc((reqHyps + 1)
            * (long)(sizeof(nmbrString)));
        memcpy(nmbrTmpPtr, wrkDisjHPtr1A, (size_t)reqHyps
            * sizeof(nmbrString));
        nmbrTmpPtr[reqHyps] = -1;
        g_Statement[stmt].reqDisjVarsA = nmbrTmpPtr;

        nmbrTmpPtr = poolFixedMalloc((reqHyps + 1)
            * (long)(sizeof(nmbrString)));
        memcpy(nmbrTmpPtr, wrkDisjHPtr1B, (size_t)reqHyps
            * sizeof(nmbrString));
        nmbrTmpPtr[reqHyps] = -1;
        g_Statement[stmt].reqDisjVarsB = nmbrTmpPtr;

        nmbrTmpPtr = poolFixedMalloc((reqHyps + 1)
            * (long)(sizeof(nmbrString)));
        memcpy(nmbrTmpPtr, wrkDisjHPtr1Stmt, (size_t)reqHyps
            * sizeof(nmbrString));
        nmbrTmpPtr[reqHyps] = -1;
        g_Statement[stmt].reqDisjVarsStmt = nmbrTmpPtr;

        // We have finished determining optional $d hyps, so allocate the
        // permanent list for the statement array.

        if (type == p_) { // Optional ones are not used by $a statements

          nmbrTmpPtr = poolFixedMalloc((optHyps + 1)
              * (long)(sizeof(nmbrString)));
          memcpy(nmbrTmpPtr, wrkDisjHPtr2A, (size_t)optHyps
              * sizeof(nmbrString));
          nmbrTmpPtr[optHyps] = -1;
          g_Statement[stmt].optDisjVarsA = nmbrTmpPtr;

          nmbrTmpPtr = poolFixedMalloc((optHyps + 1)
              * (long)(sizeof(nmbrString)));
          memcpy(nmbrTmpPtr, wrkDisjHPtr2B, (size_t)optHyps
              * sizeof(nmbrString));
          nmbrTmpPtr[optHyps] = -1;
          g_Statement[stmt].optDisjVarsB = nmbrTmpPtr;

          nmbrTmpPtr = poolFixedMalloc((optHyps + 1)
              * (long)(sizeof(nmbrString)));
          memcpy(nmbrTmpPtr, wrkDisjHPtr2Stmt, (size_t)optHyps
              * sizeof(nmbrString));
          nmbrTmpPtr[optHyps] = -1;
          g_Statement[stmt].optDisjVarsStmt = nmbrTmpPtr;
        }

        // Create list of optional variables (i.e. active but not required)
        optVars = 0;
        for (i = 0; i < activeVarStackPtr; i++) {
          if (activeVarStack[i].tmpFlag) {
            activeVarStack[i].tmpFlag = 0; // Clear it for future use
          } else {
            wrkVarPtr2[optVars] = activeVarStack[i].tokenNum;
            optVars++;
          }
        }
        // We have finished determining optional variables, so allocate the
        // permanent list for the statement array.
        if (type == p_) { // Optional ones are not used by $a statements
          nmbrTmpPtr = poolFixedMalloc((optVars + 1)
              * (long)(sizeof(nmbrString)));
          memcpy(nmbrTmpPtr, wrkVarPtr2, (size_t)optVars * sizeof(nmbrString));
          nmbrTmpPtr[optVars] = -1;
          g_Statement[stmt].optVarList = nmbrTmpPtr;
        }

        if (optVars + reqVars != activeVarStackPtr) bug(1708);

        break; // Switch case break
    }

    // If a $a statement consists of a single constant,
    // e.g. "$a wff $.", it means an empty expression (wff) is allowed.
    // Before the user had to allow this manually with
    // SET EMPTY_SUBSTITUTION ON; now it is done automatically.
    type = g_Statement[stmt].type;
    if (type == a_) {
      if (g_minSubstLen) {
        if (g_Statement[stmt].mathStringLen == 1) {
          g_minSubstLen = 0;
          printLongLine(cat("SET EMPTY_SUBSTITUTION was",
             " turned ON (allowed) for this database.", NULL),
             "    ", " ");
        }
      }
    }

    // Ensure the current Metamath spec is met:  "There may
    // not be be two active $f statements containing the same variable.  Each
    // variable in a $e, $a, or $p statement must exist in an active $f
    // statement."  (Metamath book, p. 94).
    // This section of code is stand-alone and may be removed without side
    // effects (other than less stringent error checking).
    // ??? To do (maybe):  This might be better placed in-line with the scan
    // above, for faster speed and to get the pointer to the token for the
    // error message, but it would require a careful code analysis above.
    if (type == a_ || type == p_) {
      // Scan each hypothesis (and the statement itself in last pass)
      reqHyps = nmbrLen(g_Statement[stmt].reqHypList);
      for (i = 0; i <= reqHyps; i++) {
        if (i < reqHyps) {
          m = (g_Statement[stmt].reqHypList)[i];
        } else {
          m = stmt;
        }
        if (g_Statement[m].type != f_) { // Check $e,$a,$p
          // This block implements: "Each variable in a $e, $a, or $p
          // statement must exist in an active $f statement" (Metamath
          // book p. 94).
          nmbrTmpPtr = g_Statement[m].mathString;
          // Scan all the vars in the $e (i<reqHyps) or $a/$p (i=reqHyps)
          for (j = 0; j < g_Statement[m].mathStringLen; j++) {
            tokenNum = nmbrTmpPtr[j];
            // Ignore constants
            if (g_MathToken[tokenNum].tokenType == (char)con_) continue;
            p = 0; // Initialize flag that we found a $f with the variable
            // Scan all the mandatory $f's before this $e,$a,$p
            for (k = 0; k < i; k++) {
              n = (g_Statement[stmt].reqHypList)[k];
              if (g_Statement[n].type != f_) continue; // Only check $f
               // This was already verified earlier; but if there was an
               // error, don't cause memory violation by going out of bounds.
              if (g_Statement[n].mathStringLen != 2) continue;
              if ((g_Statement[n].mathString)[1] == tokenNum) {
                p = 1; // Set flag that we found a $f with the variable
                break;
              }
            } // next k ($f hyp scan)
            if (!p) {
              sourceError(g_Statement[m].mathSectionPtr, // fbPtr
                  0, // tokenLen
                  m, // stmt
                  cat(
                  "The variable \"", g_MathToken[tokenNum].tokenName,
                  "\" does not appear in an active \"$f\" statement.", NULL));
            }
          } // next j (variable scan)
        } else { // g_Statement[m].type == f_
          // This block implements: "There may not be be two active $f
          // statements containing the same variable" (Metamath book p. 94).
          // Check for duplicate vars in active $f's

          // This was already verified earlier; but if there was an error, don't
          // cause memory violation by going out of bounds.
          if (g_Statement[m].mathStringLen != 2) continue; 
          tokenNum = (g_Statement[m].mathString)[1];
          // Scan all the mandatory $f's before this $f
          for (k = 0; k < i; k++) {
            n = (g_Statement[stmt].reqHypList)[k];
            if (g_Statement[n].type != f_) continue; // Only check $f
            // This was already verified earlier; but if there was an error, don't     
            // cause memory violation by going out of bounds.
            if (g_Statement[n].mathStringLen != 2) continue; 
            if ((g_Statement[n].mathString)[1] == tokenNum) {
              // We found 2 $f's with the same variable
              assignStmtFileAndLineNum(n);
              sourceError(g_Statement[m].mathSectionPtr, // fbPtr
                  0, // tokenLen
                  m, // stmt
                  cat(
                  "The variable \"", g_MathToken[tokenNum].tokenName,
                "\" already appears in the earlier active \"$f\" statement \"",
                  g_Statement[n].labelName, "\" on line ",
                  str((double)(g_Statement[n].lineNum)),
                  " in file \"",
                  g_Statement[n].fileName,
                  "\".", NULL));
              break; // Optional: suppresses add'l error msgs for this stmt
            }
          } // next k ($f hyp scan)
        } // if not $f else is $f
      } // next i ($e hyp scan of this statement, or its $a/$p)
    } // if stmt is $a or $p
  } // Next stmt

  if (g_currentScope > 0) {
    if (g_currentScope == 1) {
      let(&tmpStr,"A \"$}\" is");
    } else {
      let(&tmpStr,cat(str((double)g_currentScope)," \"$}\"s are",NULL));
    }
    sourceError(g_Statement[g_statements].labelSectionPtr +
        g_Statement[g_statements].labelSectionLen, 2, 0,
        cat(tmpStr," missing at the end of the file.",NULL));
  }

  // Filter out all hypothesis labels from the label key array.  We do not
  // need them anymore, since they are stored locally in each statement
  // structure.  Removing them will speed up lookups during proofs, and
  // will prevent a lookup from finding an inactive hypothesis label (thus
  // forcing an error message).
  j = 0;
/*E*/if(db5)print2("Number of label keys before filter: %ld",g_numLabelKeys);
  for (i = 0; i < g_numLabelKeys; i++) {
    type = g_Statement[g_labelKeyBase[i]].type;
    if (type == e_ || type == f_) {
      j++;
    } else {
      g_labelKeyBase[i - j] = g_labelKeyBase[i];
    }
  }
  g_numLabelKeys = g_numLabelKeys - j;
/*E*/if(db5)print2(".  After: %ld\n",g_numLabelKeys);

  // Deallocate temporary space
  free(mathTokenSameAs);
  free(reverseMathKey);
  free(labelTokenSameAs);
  free(reverseLabelKey);
  free(labelActiveFlag);
  free(activeConstStack);
  free(activeVarStack);
  free(wrkVarPtr1);
  free(wrkVarPtr2);
  for (i = 0; i < activeEHypStackPtr; i++) {
    free(activeEHypStack[i].varList);
  }
  free(activeEHypStack);
  for (i = 0; i < activeFHypStackPtr; i++) {
    free(activeFHypStack[i].varList);
  }
  free(activeFHypStack);
  free(wrkHypPtr1);
  free(wrkHypPtr2);
  free(wrkHypPtr3);
  free(activeDisjHypStack);
  free(wrkDisjHPtr1A);
  free(wrkDisjHPtr1B);
  free(wrkDisjHPtr1Stmt);
  free(wrkDisjHPtr2A);
  free(wrkDisjHPtr2B);
  free(wrkDisjHPtr2Stmt);
  free(wrkNmbrPtr);
  free(wrkStrPtr);
  free(symbolLenExists);
  free_vstring(tmpStr);
}

// Parse proof of one statement in source file.  Uses g_WrkProof structure.
// Returns 0 if OK; returns 1 if proof is incomplete (is empty or has '?'
// tokens);  returns 2 if error found; returns 3 if severe error found
// (e.g. RPN stack violation); returns 4 if not a $p statement.
char parseProof(long statemNum)
{

  long i, j, k, m, tok, step;
  char *fbPtr;
  long tokLength;
  long numReqHyp;
  long numOptHyp;
  long numActiveHyp;
  char zapSave;
  flag labelFlag;
  char returnFlag = 0;
  nmbrString *nmbrTmpPtr;
  void *voidPtr; // bsearch returned value
  vstring tmpStrPtr;

  flag explicitTargets = 0; // Proof is of form <target>=<source>
  // Source file pointers and token sizes for targets in a /EXPLICIT proof
  pntrString_def(targetPntr); // Pointers to target tokens
  nmbrString_def(targetNmbr); // Size of target tokens
  // Variables for rearranging /EXPLICIT proof
  nmbrString_def(wrkProofString); // Holds g_WrkProof.proofString
  long hypStepNum, hypSubProofLen, conclSubProofLen;
  long matchingHyp;
  nmbrString_def(oldStepNums); // Just numbers 0 to numSteps-1
  pntrString_def(reqHypSubProof); // Subproofs of hypotheses
  // Local label flag for subproofs of hypotheses
  pntrString_def(reqHypOldStepNums); 
  nmbrString_def(rearrangedSubProofs);
  nmbrString_def(rearrangedOldStepNums);
  flag subProofMoved; // Flag to restart scan after moving subproof

  if (g_Statement[statemNum].type != p_) {
    bug(1723); // should never get here
    g_WrkProof.errorSeverity = 4;
    return 4; // Do nothing if not $p
  }
  fbPtr = g_Statement[statemNum].proofSectionPtr; // Start of proof section
  // The proof was never assigned (could be a $p statement
  // with no $=; this would have been detected earlier).
  if (fbPtr[0] == 0) { 
    g_WrkProof.errorSeverity = 4;
    return 4; // Pretend it's an empty proof
  }
  fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  if (fbPtr[0] == '(') { // "(" is flag for compressed proof
    g_WrkProof.errorSeverity = parseCompressedProof(statemNum);
    return (g_WrkProof.errorSeverity);
  }

  // Make sure we have enough working space to hold the proof
  // The worst case is less than the number of chars in the source,
  // plus the number of active hypotheses.

  numOptHyp = nmbrLen(g_Statement[statemNum].optHypList);
  if (g_Statement[statemNum].proofSectionLen + g_Statement[statemNum].numReqHyp
      + numOptHyp > g_wrkProofMaxSize) {
    if (g_wrkProofMaxSize) { // Not the first allocation
      free(g_WrkProof.tokenSrcPtrNmbr);
      free(g_WrkProof.tokenSrcPtrPntr);
      free(g_WrkProof.stepSrcPtrNmbr);
      free(g_WrkProof.stepSrcPtrPntr);
      free(g_WrkProof.localLabelFlag);
      free(g_WrkProof.hypAndLocLabel);
      free(g_WrkProof.localLabelPool);
      poolFree(g_WrkProof.proofString);
      free(g_WrkProof.mathStringPtrs);
      free(g_WrkProof.RPNStack);
      free(g_WrkProof.compressedPfLabelMap);
    }
    g_wrkProofMaxSize = g_Statement[statemNum].proofSectionLen
        + g_Statement[statemNum].numReqHyp + numOptHyp
        // 2 is minimum for 1-step proof; the other terms could all be 0
        + 2;
    g_WrkProof.tokenSrcPtrNmbr = malloc((size_t)g_wrkProofMaxSize
        * sizeof(nmbrString));
    g_WrkProof.tokenSrcPtrPntr = malloc((size_t)g_wrkProofMaxSize
        * sizeof(pntrString));
    g_WrkProof.stepSrcPtrNmbr = malloc((size_t)g_wrkProofMaxSize
        * sizeof(nmbrString));
    g_WrkProof.stepSrcPtrPntr = malloc((size_t)g_wrkProofMaxSize
        * sizeof(pntrString));
    g_WrkProof.localLabelFlag = malloc((size_t)g_wrkProofMaxSize
        * sizeof(flag));
    g_WrkProof.hypAndLocLabel =
        malloc((size_t)g_wrkProofMaxSize * sizeof(struct sortHypAndLoc));
    g_WrkProof.localLabelPool = malloc((size_t)g_wrkProofMaxSize);
    // Use poolFixedMalloc instead of poolMalloc so that it won't get
    // trimmed by memUsedPoolPurge.
    g_WrkProof.proofString =
        poolFixedMalloc(g_wrkProofMaxSize * (long)(sizeof(nmbrString)));
    g_WrkProof.mathStringPtrs =
        malloc((size_t)g_wrkProofMaxSize * sizeof(nmbrString));
    g_WrkProof.RPNStack = malloc((size_t)g_wrkProofMaxSize * sizeof(nmbrString));
    g_WrkProof.compressedPfLabelMap =
         malloc((size_t)g_wrkProofMaxSize * sizeof(nmbrString));
    if (!g_WrkProof.tokenSrcPtrNmbr ||
        !g_WrkProof.tokenSrcPtrPntr ||
        !g_WrkProof.stepSrcPtrNmbr ||
        !g_WrkProof.stepSrcPtrPntr ||
        !g_WrkProof.localLabelFlag ||
        !g_WrkProof.hypAndLocLabel ||
        !g_WrkProof.localLabelPool ||
        !g_WrkProof.mathStringPtrs ||
        !g_WrkProof.RPNStack
        ) outOfMemory("#99 (g_WrkProof)");
  }

  // Initialization for this proof
  g_WrkProof.errorCount = 0; // Used as threshold for how many error msgs/proof
  g_WrkProof.numSteps = 0;
  g_WrkProof.numTokens = 0;
  g_WrkProof.numHypAndLoc = 0;
  g_WrkProof.numLocalLabels = 0;
  g_WrkProof.RPNStackPtr = 0;
  g_WrkProof.localLabelPoolPtr = g_WrkProof.localLabelPool;

  // fbPtr points to the first token now.

  // First break up proof section of source into tokens
  while (1) {
    tokLength = proofTokenLen(fbPtr);
    if (!tokLength) break;
    g_WrkProof.tokenSrcPtrPntr[g_WrkProof.numTokens] = fbPtr;
    g_WrkProof.tokenSrcPtrNmbr[g_WrkProof.numTokens] = tokLength;
    g_WrkProof.numTokens++;
    fbPtr = fbPtr + tokLength;
    fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  }

  // If there are no tokens, the proof is unknown; make the token a '?'
  // (g_WrkProof.tokenSrcPtrPntr won't point to the source, but this is OK since
  // there will never be an error message for it.)
  if (!g_WrkProof.numTokens) {

    // For now, this is an error.
    if (!g_WrkProof.errorCount) {
      sourceError(fbPtr, 2, statemNum,
          "The proof is empty.  If you don't know the proof, make it a \"?\".");
    }
    g_WrkProof.errorCount++;
    if (returnFlag < 1) returnFlag = 1;

    // Allow empty proofs anyway
    g_WrkProof.numTokens = 1;
    g_WrkProof.tokenSrcPtrPntr[0] = "?";
    g_WrkProof.tokenSrcPtrNmbr[0] = 1; // Length
  }

  // Copy active (opt + req) hypotheses to hypAndLocLabel look-up table
  nmbrTmpPtr = g_Statement[statemNum].optHypList;
  // Transfer optional hypotheses
  while (1) {
    i = nmbrTmpPtr[g_WrkProof.numHypAndLoc];
    if (i == -1) break;
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelTokenNum = i;
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelName =
        g_Statement[i].labelName;
    g_WrkProof.numHypAndLoc++;
  }
  // Transfer required hypotheses
  j = g_Statement[statemNum].numReqHyp;
  nmbrTmpPtr = g_Statement[statemNum].reqHypList;
  for (i = 0; i < j; i++) {
    k = nmbrTmpPtr[i];
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelTokenNum = k;
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelName =
        g_Statement[k].labelName;
    g_WrkProof.numHypAndLoc++;
  }

  // Sort the hypotheses by label name for lookup
  numActiveHyp = g_WrkProof.numHypAndLoc; // Save for bsearch later
  qsort(g_WrkProof.hypAndLocLabel, (size_t)(g_WrkProof.numHypAndLoc),
      sizeof(struct sortHypAndLoc), hypAndLocSortCmp);

  // Scan the parsed tokens for local label assignments
  fbPtr = g_WrkProof.tokenSrcPtrPntr[0];
  if (fbPtr[0] == ':') {
    if (!g_WrkProof.errorCount) {
      sourceError(fbPtr, 1, statemNum,
          "The colon at proof step 1 must be preceded by a local label.");
    }
    if (returnFlag < 2) returnFlag = 2;
    g_WrkProof.tokenSrcPtrPntr[0] = "?";
    g_WrkProof.tokenSrcPtrNmbr[0] = 1; // Length
    g_WrkProof.errorCount++;
  }
  fbPtr = g_WrkProof.tokenSrcPtrPntr[g_WrkProof.numTokens - 1];
  if (fbPtr[0] == ':') {
    if (!g_WrkProof.errorCount) {
      sourceError(fbPtr, 1, statemNum,
          "The colon in the last proof step must be followed by a label.");
    }
    if (returnFlag < 2) returnFlag = 2;
    g_WrkProof.errorCount++;
    g_WrkProof.numTokens--;
  }
  labelFlag = 0;
  for (tok = 0; tok < g_WrkProof.numTokens; tok++) {
    fbPtr = g_WrkProof.tokenSrcPtrPntr[tok];

    // If next token is = then this token is a target for /EXPLICIT format,
    // so don't increment the proof step number.
    if (tok < g_WrkProof.numTokens - 2) {
      if (((char *)((g_WrkProof.tokenSrcPtrPntr)[tok + 1]))[0] == '=') {
        explicitTargets = 1; // Flag that proof has explicit targets
        continue;
      }
    }
    if (fbPtr[0] == '=') continue; // Skip the = token

    // Save pointer to source file vs. step for error messages
    g_WrkProof.stepSrcPtrNmbr[g_WrkProof.numSteps] =
        g_WrkProof.tokenSrcPtrNmbr[tok]; // Token length
    g_WrkProof.stepSrcPtrPntr[g_WrkProof.numSteps] = fbPtr; // Token ptr

    // Save fact that this step has a local label declaration
    g_WrkProof.localLabelFlag[g_WrkProof.numSteps] = labelFlag;
    labelFlag = 0;

    g_WrkProof.numSteps++;
    if (fbPtr[0] != ':') continue;

    // Colon found -- previous token is a label
    labelFlag = 1;

    g_WrkProof.numSteps = g_WrkProof.numSteps - 2;
    fbPtr = g_WrkProof.tokenSrcPtrPntr[tok - 1]; // The local label
    tokLength = g_WrkProof.tokenSrcPtrNmbr[tok - 1]; // Its length

    // Check for illegal characters
    for (j = 0; j < tokLength; j++) {
      if (illegalLabelChar[(unsigned char)fbPtr[j]]) {
        if (!g_WrkProof.errorCount) {
          sourceError(fbPtr + j, 1, statemNum,cat(
              "The local label at proof step ",
              str((double)(g_WrkProof.numSteps + 1)),
              " is incorrect.  Only letters,",
              " digits, \"_\", \"-\", and \".\" are allowed in local labels.",
              NULL));
        }
        if (returnFlag < 2) returnFlag = 2;
        g_WrkProof.errorCount++;
      }
    }

    // Add the label to the local label pool and hypAndLocLabel table
    memcpy(g_WrkProof.localLabelPoolPtr, fbPtr, (size_t)tokLength);
    g_WrkProof.localLabelPoolPtr[tokLength] = 0; // String terminator
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelTokenNum =
       -g_WrkProof.numSteps - 1000; // offset of -1000 is flag for local label
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelName
        = g_WrkProof.localLabelPoolPtr;

    // Make sure local label is different from all earlier $a and $p labels
    voidPtr = (void *)bsearch(g_WrkProof.localLabelPoolPtr, g_labelKeyBase,
        (size_t)g_numLabelKeys, sizeof(long), labelSrchCmp);
    if (voidPtr) { // It was found
      j = *(long *)voidPtr; // Statement number
      if (j <= statemNum) {
        if (!g_WrkProof.errorCount) {
          assignStmtFileAndLineNum(j);
          sourceError(fbPtr, tokLength, statemNum,cat(
              "The local label at proof step ",
              str((double)(g_WrkProof.numSteps + 1)),
              " is the same as the label of statement ",
              str((double)j),
              " at line ",
              str((double)(g_Statement[j].lineNum)),
              " in file \"",
              g_Statement[j].fileName,
              "\".  Local labels must be different from active statement labels.",
              NULL));
        }
        g_WrkProof.errorCount++;
        if (returnFlag < 2) returnFlag = 2;
      }
    }

    // Make sure local label is different from all active $e and $f labels
    voidPtr = (void *)bsearch(g_WrkProof.localLabelPoolPtr,
        g_WrkProof.hypAndLocLabel,
        (size_t)numActiveHyp, sizeof(struct sortHypAndLoc), hypAndLocSrchCmp);
    if (voidPtr) { // It was found
      j = ( (struct sortHypAndLoc *)voidPtr)->labelTokenNum; // Statement number
      if (!g_WrkProof.errorCount) {
        assignStmtFileAndLineNum(j);
        sourceError(fbPtr, tokLength, statemNum,cat(
            "The local label at proof step ",
            str((double)(g_WrkProof.numSteps + 1)),
            " is the same as the label of statement ",
            str((double)j),
            " at line ",
            str((double)(g_Statement[j].lineNum)),
            " in file \"",
            g_Statement[j].fileName,
            "\".  Local labels must be different from active statement labels.",
            NULL));
      }
      g_WrkProof.errorCount++;
      if (returnFlag < 2) returnFlag = 2;
      g_WrkProof.numHypAndLoc--; // Ignore the label
    }

    g_WrkProof.numHypAndLoc++;
    g_WrkProof.localLabelPoolPtr = &g_WrkProof.localLabelPoolPtr[tokLength + 1];
  } // Next i

  // Collect all target labels in /EXPLICIT format.
  // I decided not to make targetPntr, targetNmbr part of the g_WrkProof
  // structure since other proof formats don't assign it, so we can't
  // reference it reliably outside of this function.  And it would waste
  // some memory if we don't use /EXPLICIT, which is intended primarily
  // for database maintenance.
  if (explicitTargets == 1) {
    pntrLet(&targetPntr, pntrSpace(g_WrkProof.numSteps));
    nmbrLet(&targetNmbr, nmbrSpace(g_WrkProof.numSteps));
    step = 0;
    for (tok = 0; tok < g_WrkProof.numTokens - 2; tok++) {
      // If next token is = then this token is a target for /EXPLICIT format,
      // so don't increment the proof step number.
      if (((char *)((g_WrkProof.tokenSrcPtrPntr)[tok + 1]))[0] == '=') {
        fbPtr = g_WrkProof.tokenSrcPtrPntr[tok];
        if (step >= g_WrkProof.numSteps) {
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr, g_WrkProof.tokenSrcPtrNmbr[tok], statemNum, cat(
                "There are more target labels than proof steps.", NULL));
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          break;
        }
        targetPntr[step] = fbPtr;
        targetNmbr[step] = g_WrkProof.tokenSrcPtrNmbr[tok];
        if (g_WrkProof.tokenSrcPtrPntr[tok + 2]
            != g_WrkProof.stepSrcPtrPntr[step]) {
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr, g_WrkProof.tokenSrcPtrNmbr[tok], statemNum, cat(
                "The target label for step ", str((double)step + 1),
                " is not assigned to that step.  ",
                "(Check for missing or extra \"=\".)", NULL));
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }
        step++;
      }
    } // next tok
    if (step != g_WrkProof.numSteps) {
      if (!g_WrkProof.errorCount) {
        sourceError(
            (char *)((g_WrkProof.tokenSrcPtrPntr)[g_WrkProof.numTokens - 1]),
            g_WrkProof.tokenSrcPtrNmbr[g_WrkProof.numTokens - 1],
            statemNum, cat(
                "There are ", str((double)(g_WrkProof.numSteps)), " proof steps but only ",
                str((double)step), " target labels.", NULL));
      }
      g_WrkProof.errorCount++;
      if (returnFlag < 2) returnFlag = 2;
    }
  } // if explicitTargets == 1

  if (g_WrkProof.numHypAndLoc > numActiveHyp) { // There were local labels

    // Sort the local labels into the hypAndLocLabel look-up table
    qsort(g_WrkProof.hypAndLocLabel, (size_t)(g_WrkProof.numHypAndLoc),
        sizeof(struct sortHypAndLoc), hypAndLocSortCmp);

    // Check for duplicate local labels
    for (i = 1; i < g_WrkProof.numHypAndLoc; i++) {
      if (!strcmp(g_WrkProof.hypAndLocLabel[i - 1].labelName,
          g_WrkProof.hypAndLocLabel[i].labelName)) { // Duplicate label
        // Get the step numbers
        j = -(g_WrkProof.hypAndLocLabel[i - 1].labelTokenNum + 1000);
        k = -(g_WrkProof.hypAndLocLabel[i].labelTokenNum + 1000);
        if (j > k) {
          m = j;
          j = k; // Smaller step number
          k = m; // Larger step number
        }
        // Find the token - back up a step then move forward to loc label
        fbPtr = g_WrkProof.stepSrcPtrPntr[k - 1]; // Previous step
        fbPtr = fbPtr + g_WrkProof.stepSrcPtrNmbr[k - 1];
        fbPtr = fbPtr + whiteSpaceLen(fbPtr);
        if (!g_WrkProof.errorCount) {
          sourceError(fbPtr,
              proofTokenLen(fbPtr), statemNum,
              cat("The local label at proof step ", str((double)k + 1),
              " is the same as the one declared at step ",
              str((double)j + 1), ".", NULL));
        }
        g_WrkProof.errorCount++;
        if (returnFlag < 2) returnFlag = 2;
      } // End if duplicate label
    } // Next i
  } // End if there are local labels

  // Build the proof string and check the RPN stack
  g_WrkProof.proofString[g_WrkProof.numSteps] = -1; // End of proof
  // Zap mem pool actual length (because nmbrLen will be used later on this)
  nmbrZapLen(g_WrkProof.proofString, g_WrkProof.numSteps);
  if (explicitTargets == 1) {
    // List of original step numbers to keep track of local label movement
    nmbrLet(&oldStepNums, nmbrSpace(g_WrkProof.numSteps));
    for (i = 0; i < g_WrkProof.numSteps; i++) {
      oldStepNums[i] = i;
    }
  }

  for (step = 0; step < g_WrkProof.numSteps; step++) {
    tokLength = g_WrkProof.stepSrcPtrNmbr[step];
    fbPtr = g_WrkProof.stepSrcPtrPntr[step];

    // Handle unknown proof steps
    if (fbPtr[0] == '?') {
      // Flag that proof is partially unknown
      if (returnFlag < 1) returnFlag = 1;
      g_WrkProof.proofString[step] = -(long)'?';
      // Treat "?" like a hypothesis - push stack and continue
      g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = step;
      g_WrkProof.RPNStackPtr++;
      continue;
    }

    // Temporarily zap the token's end with a null for string comparisons
    zapSave = fbPtr[tokLength];
    fbPtr[tokLength] = 0; // Zap source

    // See if the proof token is a hypothesis or local label ref.
    voidPtr = (void *)bsearch(fbPtr, g_WrkProof.hypAndLocLabel,
        (size_t)(g_WrkProof.numHypAndLoc), sizeof(struct sortHypAndLoc),
        hypAndLocSrchCmp);
    if (voidPtr) {
      fbPtr[tokLength] = zapSave; // Unzap source
      j = ((struct sortHypAndLoc *)voidPtr)->labelTokenNum; // Label lookup number
      g_WrkProof.proofString[step] = j; // Proof string

      // Push the stack
      g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = step;
      g_WrkProof.RPNStackPtr++;

      if (j < 0) { // It's a local label reference
        i = -1000 - j; // Step number referenced
        if (i < 0) bug(1734);

        // Make sure we don't reference a later step
        if (i > step) {
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr, tokLength, statemNum,cat("Proof step ",
                str((double)step + 1),
                " references a local label before it is declared.",
                NULL));
          }
          g_WrkProof.proofString[step] = -(long)'?';
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }

        if (g_WrkProof.localLabelFlag[step]) {
          if (!g_WrkProof.errorCount) {
            // Chained labels not allowed because it complicates the language
            // but doesn't buy anything.
            sourceError(fbPtr, tokLength, statemNum, cat(
                "The local label reference at proof step ",
                str((double)step + 1),
                " declares a local label.  Only \"$a\" and \"$p\" statement",
                " labels may have local label declarations.",NULL));
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }
      } else { // It's a hypothesis reference
        if (g_WrkProof.localLabelFlag[step]) {
          // Not allowed because it complicates the language but doesn't
          // buy anything; would make $e to $f assignments harder to detect.
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr, tokLength, statemNum, cat(
                "The hypothesis reference at proof step ",
                str((double)step + 1),
                " declares a local label.  Only \"$a\" and \"$p\" statement",
                " labels may have local label declarations.",NULL));
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }
        if (j <= 0) bug(1709);
      }
      continue;
    } // End if local label or hypothesis

    // See if token is an assertion label
    voidPtr = (void *)bsearch(fbPtr, g_labelKeyBase, (size_t)g_numLabelKeys,
        sizeof(long), labelSrchCmp);
    fbPtr[tokLength] = zapSave; // Unzap source
    if (!voidPtr) {
      if (!g_WrkProof.errorCount) {
        sourceError(fbPtr, tokLength, statemNum, cat(
            "The token at proof step ",
            str((double)step + 1),
            " is not an active statement label or a local label.",NULL));
      }
      g_WrkProof.errorCount++;
      g_WrkProof.proofString[step] = -(long)'?';
      // Push the stack
      g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = step;
      g_WrkProof.RPNStackPtr++;
      if (returnFlag < 2) returnFlag = 2;
      continue;
    }

    // It's an assertion ($a or $p)
    j = *(long *)voidPtr; // Statement number
    if (g_Statement[j].type != a_ && g_Statement[j].type != p_) bug(1710);
    g_WrkProof.proofString[step] = j; // Assign $a/$p label to proof string

    if (j >= statemNum) { // Error
      if (!g_WrkProof.errorCount) {
        if (j == statemNum) {
          sourceError(fbPtr, tokLength, statemNum, cat(
              "The label at proof step ",
              str((double)step + 1),
              " is the label of this statement.  A statement may not be used to",
              " prove itself.",NULL));
        } else {
          assignStmtFileAndLineNum(j);
          sourceError(fbPtr, tokLength, statemNum, cat(
              "The label \"", g_Statement[j].labelName, "\" at proof step ",
              str((double)step + 1),
              " is the label of a future statement (at line ",
              str((double)(g_Statement[j].lineNum)),
              " in file ",g_Statement[j].fileName,
      ").  Only local labels or previous, active statements may be referenced.",
              NULL));
        }
      }
      g_WrkProof.errorCount++;
      if (returnFlag < 2) returnFlag = 2;
    }

    // It's a valid assertion, so pop the stack
    numReqHyp = g_Statement[j].numReqHyp;

    // Error check for exhausted stack
    if (g_WrkProof.RPNStackPtr < numReqHyp) { // Stack exhausted -- error
      if (!g_WrkProof.errorCount) {
        tmpStrPtr = shortDumpRPNStack();
        if (strcmp(left(tmpStrPtr,18),"RPN stack is empty")){
          i = instr(1,tmpStrPtr,"contains ");
          let(&tmpStrPtr,cat(left(tmpStrPtr,i + 7)," only",
            right(tmpStrPtr,i + 8),
            NULL));
        }
        if (numReqHyp == 1) {
          let(&tmpStrPtr,cat("a hypothesis but the ",tmpStrPtr,NULL));
        } else {
          let(&tmpStrPtr,cat(str((double)numReqHyp)," hypotheses but the ",tmpStrPtr,
              NULL));
        }
        sourceError(fbPtr, tokLength, statemNum, cat(
            "At proof step ",
            str((double)step + 1),", statement \"",
            g_Statement[j].labelName,"\" requires ",
            tmpStrPtr,".",NULL));
        free_vstring(tmpStrPtr);
      }
      // Treat it like an unknown step so stack won't get exhausted
      g_WrkProof.errorCount++;
      g_WrkProof.proofString[step] = -(long)'?';
      // Push the stack
      g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = step;
      g_WrkProof.RPNStackPtr++;
      if (returnFlag < 3) returnFlag = 3;
      continue;
    } // End if stack exhausted

    // For proofs saved with /EXPLICIT, the user may have changed the order
    // of hypotheses.  First, get the subproofs for the hypotheses.  Then
    // reassemble them in the right order.
    if (explicitTargets == 1) {
      // nmbrString to rearrange proof then when done reassign to
      // g_WrkProof.proofString structure component.
      nmbrLet(&wrkProofString, g_WrkProof.proofString);
            
      nmbrTmpPtr = g_Statement[j].reqHypList;
      numReqHyp = g_Statement[j].numReqHyp;
      conclSubProofLen = subproofLen(wrkProofString, step);
      // Initialize to NULL_NMBRSTRINGs
      pntrLet(&reqHypSubProof, pntrNSpace(numReqHyp));
      // Initialize to NULL_NMBRSTRINGs
      pntrLet(&reqHypOldStepNums, pntrNSpace(numReqHyp));   
      k = 0; // Total hypothesis subproof lengths for error checking
      for (i = 0; i < numReqHyp; i++) {
        m = g_WrkProof.RPNStackPtr - numReqHyp + i; // Stack position of hyp
        hypStepNum = g_WrkProof.RPNStack[m]; // Step number of hypothesis i
        hypSubProofLen = subproofLen(wrkProofString, hypStepNum);
        k += hypSubProofLen;
        nmbrLet((nmbrString **)(&(reqHypSubProof[i])),
            // For nmbrSeg, 1 = first step
            nmbrSeg(wrkProofString,
                hypStepNum - hypSubProofLen + 2, hypStepNum + 1));
        nmbrLet((nmbrString **)(&(reqHypOldStepNums[i])),
            // For nmbrSeg, 1 = first step
            nmbrSeg(oldStepNums,
                hypStepNum - hypSubProofLen + 2, hypStepNum + 1));
      } // Next i
      // Uncomment above if bad proof triggers this bug
      if (k != conclSubProofLen - 1 /* && returnFlag < 2 */) {
        bug(1731);
      }
      free_nmbrString(rearrangedSubProofs);
      matchingHyp = -1; // In case there are no hypotheses
      for (i = 0; i < numReqHyp; i++) {
        matchingHyp = -1;
        for (k = 0; k < numReqHyp; k++) {
          m = g_WrkProof.RPNStackPtr - numReqHyp + k; // Stack position of hyp
          hypStepNum = g_WrkProof.RPNStack[m]; // Step number of hypothesis k

          // Temporarily zap the token's end with a null for string comparisons
          fbPtr = targetPntr[hypStepNum];
          zapSave = fbPtr[targetNmbr[hypStepNum]];
          fbPtr[targetNmbr[hypStepNum]] = 0; // Zap source
          // See if hypothesis i matches the target label k i.e. hypStepNum
          if (!strcmp(g_Statement[nmbrTmpPtr[i]].labelName, fbPtr)) {
            matchingHyp = k;
          }
          fbPtr[targetNmbr[hypStepNum]] = zapSave; // Unzap source
          if (matchingHyp != -1) break;
        } // next k (0 to numReqHyp-1)
        if (matchingHyp == -1) {
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr, 1 /* token length */, statemNum, cat(
                "The target labels for the hypotheses for step ", str((double)step + 1),
                " do not match hypothesis \"",
                g_Statement[nmbrTmpPtr[i]].labelName,
                "\" of the assertion \"",
                g_Statement[j].labelName,
                "\" in step ",  str((double)step + 1), ".",
                NULL));
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          break; // Give up; don't try to rearrange hypotheses
        }
        // Accumulate the subproof for hypothesis i
        nmbrLet(&rearrangedSubProofs, nmbrCat(rearrangedSubProofs,
            reqHypSubProof[matchingHyp], NULL));
        nmbrLet(&rearrangedOldStepNums, nmbrCat(rearrangedOldStepNums,
            reqHypOldStepNums[matchingHyp], NULL));
      } // next i (0 to numReqHyp-1)

      if (matchingHyp != -1) { // All hypotheses found
        if (nmbrLen(rearrangedSubProofs) != conclSubProofLen - 1
          // && returnFlag < 2
          ) {
          // Uncomment above if bad proof triggers this bug
          bug(1732);
        }
        nmbrLet(&(wrkProofString), nmbrCat(
            nmbrLeft(wrkProofString, step - conclSubProofLen + 1),
            rearrangedSubProofs,
            nmbrRight(wrkProofString, step + 1), NULL));
        nmbrLet(&oldStepNums, nmbrCat(
            nmbrLeft(oldStepNums, step - conclSubProofLen + 1),
            rearrangedOldStepNums,
            nmbrRight(oldStepNums, step + 1), NULL));
      }

      // Reassign g_WrkProof.proofString from rearranged wrkProofString
      for (i = 0; i < step; i++) {
        // Nothing from step to end has been changed, so stop at step
        (g_WrkProof.proofString)[i] = wrkProofString[i];
      }
      if ((g_WrkProof.proofString)[step] != wrkProofString[step]) bug(1735);

      // Deallocate
      for (i = 0; i < numReqHyp; i++) {
        free_nmbrString(*(nmbrString **)(&(reqHypSubProof[i])));
        free_nmbrString(*(nmbrString **)(&(reqHypOldStepNums[i])));
      }
      free_pntrString(reqHypSubProof);
      free_pntrString(reqHypOldStepNums);
      free_nmbrString(rearrangedSubProofs);
      free_nmbrString(rearrangedOldStepNums);
      free_nmbrString(wrkProofString);
    } // if explicitTargets

    numReqHyp = g_Statement[j].numReqHyp;

    // Pop the stack
    g_WrkProof.RPNStackPtr = g_WrkProof.RPNStackPtr - numReqHyp;
    g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = step;
    g_WrkProof.RPNStackPtr++;
  } // Next step

  // The stack should have one entry
  if (g_WrkProof.RPNStackPtr != 1) {
    tmpStrPtr = shortDumpRPNStack();
    fbPtr = g_WrkProof.stepSrcPtrPntr[g_WrkProof.numSteps - 1];
    if (!g_WrkProof.errorCount) {
      sourceError(fbPtr, proofTokenLen(fbPtr), statemNum, cat("After proof step ",
          str((double)(g_WrkProof.numSteps))," (the last step), the ",
          tmpStrPtr,".  It should contain exactly one entry.",NULL));
    }
    g_WrkProof.errorCount++;
    if (returnFlag < 3) returnFlag = 3;
  }

  if (explicitTargets) {
    // Correct the local label refs in the rearranged proof
    for (step = 0; step < g_WrkProof.numSteps; step++) {
      // This is slow lookup with n^2 behavior, but should be ok since
      // /EXPLICIT isn't used that often.
      k = (g_WrkProof.proofString)[step]; // Will be <= -1000 if local label
      if (k <= -1000) {
        k = -1000 - k; // Restore step number
        if (k < 0 || k >= g_WrkProof.numSteps) bug(1733);
        // Find the original step
        if (oldStepNums[k] == k) {
          // Wasn't changed, so skip renumbering for speedup
          continue;
        }
        i = 0; // For bug check
        // Find the original step number and change it to the new one
        for (m = 0; m < g_WrkProof.numSteps; m++) {
          if (oldStepNums[m] == k) {
            (g_WrkProof.proofString)[step] = -1000 - m;
            i = 1; // Found
            break;
          }
        }
        if (i == 0) bug(1740);
      }
    } // next step

    // Check if any local labels point to future steps: if so, we should
    // moved the subproof they point to down (since many functions require
    // that a local label be declared before it is used).
    do {
      subProofMoved = 0; // Flag to rescan after moving a subproof.
      // TODO: restart loop after step just finished for speedup?
      // (maybe not worth it).
      // We could just change subProofMoved to restartStep (long) and use
      // restartStep > 0 as the flag, since we would restart at the
      // last step processed plus 1.
      for (step = 0; step < g_WrkProof.numSteps; step++) {
        k = (g_WrkProof.proofString)[step]; // Will be <= -1000 if local label
        if (k <= -1000) { // References local label i.e. subproof
          k = -1000 - k; // Restore step number subproof ends at
          if (k > step) { // Refers to label declared after this step
            m = subproofLen(g_WrkProof.proofString, k);
            // m = nmbrGetSubProofLen(g_WrkProof.proofString, k);

            // At this point:
            //     step = the step referencing a future subproof
            //     k = end of future subproof
            //     m = length of future subproof

            // TODO - make this a direct assignment for speedup?
            // (with most $f's reversed, this has about 13K hits during
            // 'verify proof *' - maybe not enough to justify rewriting this
            // to make direct assignment instead of nmbrLet().)
            // Replace the step with the subproof it references.
            // Note that nmbrXxx() positions start at 1, not 0; add 1 to step.
            nmbrLet(&wrkProofString, nmbrCat(
                // Proof before future label ref:
                nmbrLeft(g_WrkProof.proofString, step),
                // The future subproof moved to the current step:
                nmbrMid(g_WrkProof.proofString, k - m + 2, m),
                // The steps between this step and the future subproof:
                nmbrSeg(g_WrkProof.proofString, step + 2, k - m + 1),
                // The future subproof replaced with the current step (a local
                // label to be renumbered below):
                nmbrMid(g_WrkProof.proofString, step + 1, 1),
                // The rest of the steps to the end of proof:
                nmbrRight(g_WrkProof.proofString, k + 2),
                NULL));
            if (nmbrLen(wrkProofString) != g_WrkProof.numSteps) {
              bug(1736); // Make sure proof length didn't change
            }
            if (wrkProofString[k] != (g_WrkProof.proofString)[step]) {
              // Make sure future subproof is now the future local
              // label (to be renumbered below).
              bug(1737);
            }

            // We now have this wrkProofString[...] content:
            //    [0]...[step-1]                   same as original proof
            //    [step+1]...[step+m-1]            moved subproof
            //    [step+m]...[k-1]                 shifted orig proof
            //    [k]...[k]                        subproof replaced by loc label
            //    [k+1]...[g_WrkProof.numSteps-1]    same as orig proof

            // Correct all local labels
            for (i = 0; i < g_WrkProof.numSteps; i++) {
              j = (wrkProofString)[i]; // Will be <= -1000 if local label
              if (j > -1000) continue; // Not loc label ref
              j = -1000 - j; // Restore step number subproof ends at
              // Note: the conditions before the "&&" below are redundant
              // but provide better sanity checking.
              if (j >= 0 && j < step) { // Before moved subproof
                // j = j; // Same as orig proof
              } else if (j == step) { // The original local label ref
                bug(1738); // A local label shouldn't ref a local label
              // Steps shifted up by subproof insertion
              } else if (j > step && j <= k - m) {          
                j = j + m - 1; // Offset by size of subproof moved down -1
              // Reference to inside the moved subproof
              } else if (j > k - m && j <= k) {
                j = j + step + m - 1 - k; // Shift down
              // Ref to after moved subproof
              } else if (j > k && j <= g_WrkProof.numSteps - 1) {
                // j = j; // Same as orig proof
              } else {
                bug(1739); // Cases not exhausted or j is out of range
              }
              (wrkProofString)[i] = -j - 1000; // Update new proof
            } // next i

            // Transfer proof back to original
            for (i = 0; i < g_WrkProof.numSteps; i++) {
              (g_WrkProof.proofString)[i] = wrkProofString[i];
            }

            // Set flag that a subproof was moved and restart the scan
            subProofMoved = 1;
            // Reassign g_WrkProof.proofString from new wrkProofString
            for (i = 0; i < g_WrkProof.numSteps; i++) {
              (g_WrkProof.proofString)[i] = wrkProofString[i];
            }
            break; // Break out of the 'for (step...' loop
          } // if k>step
        } // if k<= -1000
      } // next step
    } while (subProofMoved);

    // Deallocate
    free_pntrString(targetPntr);
    free_nmbrString(targetNmbr);
    free_nmbrString(oldStepNums);
    free_nmbrString(wrkProofString);
  } // if (explicitTargets)

  g_WrkProof.errorSeverity = returnFlag;
  return returnFlag;
} // parseProof()

// Parse proof in compressed format.
// Parse proof of one statement in source file.  Uses wrkProof structure.
// Returns 0 if OK; returns 1 if proof is incomplete (is empty or has '?'
// tokens);  returns 2 if error found; returns 3 if severe error found
// (e.g. RPN stack violation); returns 4 if not a $p statement.
char parseCompressedProof(long statemNum)
{

  long i, j, k, step, stmt;
  char *fbPtr;
  char *fbStartProof;
  char *labelStart;
  long tokLength;
  long numReqHyp;
  long numOptHyp;
  char zapSave;
  flag breakFlag;
  char returnFlag = 0;
  nmbrString *nmbrTmpPtr;
  void *voidPtr; // bsearch returned value
  vstring tmpStrPtr;
  flag hypLocUnkFlag; // Hypothesis, local label ref, or unknown step
  long labelMapIndex;

  static unsigned char chrWeight[256]; // Proof label character weights
  static unsigned char chrType[256]; // Proof character types
  static flag chrTablesInited = 0;
  static char *digits = "0123456789";
  static char *letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static char labelChar = ':';
  static long lettersLen;
  static long digitsLen;

  // Used to detect old buggy compression
  long bggyProofLen;
  char bggyZapSave;
  flag bggyAlgo;

  // Initialization to avoid compiler warning (should not be theoretically
  // necessary).
  labelStart = "";

  // Do one-time initialization
  if (!chrTablesInited) {
    chrTablesInited = 1;

    // Compression standard with all cap letters.
    // (For 500-700 step proofs, we only lose about 18% of file size --
    // but the compressed proof is more pleasing to the eye).
    letters = "ABCDEFGHIJKLMNOPQRST"; // LSB is base 20
    digits = "UVWXY"; // MSB's are base 5
    labelChar = 'Z'; // Was colon

    lettersLen = (long)strlen(letters);
    digitsLen = (long)strlen(digits);

    // Initialize compressed proof label character weights
    // Initialize compressed proof character types
    for (i = 0; i < 256; i++) {
      chrWeight[i] = 0;
      chrType[i] = 6; // Illegal
    }
    j = lettersLen;
    for (i = 0; i < j; i++) {
      chrWeight[(long)(letters[i])] = (unsigned char)i;
      chrType[(long)(letters[i])] = 0; // Letter
    }
    j = digitsLen;
    for (i = 0; i < j; i++) {
      chrWeight[(long)(digits[i])] = (unsigned char)i;
      chrType[(long)(digits[i])] = 1; // Digit
    }
    for (i = 0; i < 256; i++) {
      if (isspace(i)) chrType[i] = 3; // White space
    } // Next i
    chrType[(long)(labelChar)] = 2; // Colon
    chrType['$'] = 4; // Dollar
    chrType['?'] = 5; // Question mark
  }

  if (g_Statement[statemNum].type != p_) {
    bug(1724); // should never get here
    return 4; // Do nothing if not $p
  }
  fbPtr = g_Statement[statemNum].proofSectionPtr; // Start of proof section
  // The proof was never assigned (could be a $p statement
  // with no $=; this would have been detected earlier).
  if (fbPtr[0] == 0) { 
    bug(1711);
  }
  fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  if (fbPtr[0] != '(') { // ( is flag for start of compressed proof
    bug(1712);
  }

  // Make sure we have enough working space to hold the proof.
  // The worst case is less than the number of chars in the source,
  // plus the number of active hypotheses.

  numOptHyp = nmbrLen(g_Statement[statemNum].optHypList);
  if (g_Statement[statemNum].proofSectionLen + g_Statement[statemNum].numReqHyp
      + numOptHyp > g_wrkProofMaxSize) {
    if (g_wrkProofMaxSize) { // Not the first allocation
      free(g_WrkProof.tokenSrcPtrNmbr);
      free(g_WrkProof.tokenSrcPtrPntr);
      free(g_WrkProof.stepSrcPtrNmbr);
      free(g_WrkProof.stepSrcPtrPntr);
      free(g_WrkProof.localLabelFlag);
      free(g_WrkProof.hypAndLocLabel);
      free(g_WrkProof.localLabelPool);
      poolFree(g_WrkProof.proofString);
      free(g_WrkProof.mathStringPtrs);
      free(g_WrkProof.RPNStack);
      free(g_WrkProof.compressedPfLabelMap);
    }
    g_wrkProofMaxSize = g_Statement[statemNum].proofSectionLen
        + g_Statement[statemNum].numReqHyp + numOptHyp;
    g_WrkProof.tokenSrcPtrNmbr = malloc((size_t)g_wrkProofMaxSize
        * sizeof(nmbrString));
    g_WrkProof.tokenSrcPtrPntr = malloc((size_t)g_wrkProofMaxSize
        * sizeof(pntrString));
    g_WrkProof.stepSrcPtrNmbr = malloc((size_t)g_wrkProofMaxSize
        * sizeof(nmbrString));
    g_WrkProof.stepSrcPtrPntr = malloc((size_t)g_wrkProofMaxSize
        * sizeof(pntrString));
    g_WrkProof.localLabelFlag = malloc((size_t)g_wrkProofMaxSize
        * sizeof(flag));
    g_WrkProof.hypAndLocLabel =
        malloc((size_t)g_wrkProofMaxSize * sizeof(struct sortHypAndLoc));
    g_WrkProof.localLabelPool = malloc((size_t)g_wrkProofMaxSize);
    // Use poolFixedMalloc instead of poolMalloc so that it won't get trimmed by
    // memUsedPoolPurge.
    g_WrkProof.proofString = 
        poolFixedMalloc(g_wrkProofMaxSize * (long)(sizeof(nmbrString)));
    g_WrkProof.mathStringPtrs =
        malloc((size_t)g_wrkProofMaxSize * sizeof(nmbrString));
    g_WrkProof.RPNStack = malloc((size_t)g_wrkProofMaxSize * sizeof(nmbrString));
    g_WrkProof.compressedPfLabelMap =
         malloc((size_t)g_wrkProofMaxSize * sizeof(nmbrString));
    if (!g_WrkProof.tokenSrcPtrNmbr ||
        !g_WrkProof.tokenSrcPtrPntr ||
        !g_WrkProof.stepSrcPtrNmbr ||
        !g_WrkProof.stepSrcPtrPntr ||
        !g_WrkProof.localLabelFlag ||
        !g_WrkProof.hypAndLocLabel ||
        !g_WrkProof.localLabelPool ||
        !g_WrkProof.mathStringPtrs ||
        !g_WrkProof.RPNStack
        ) outOfMemory("#99 (g_WrkProof)");
  }

  // Initialization for this proof
  g_WrkProof.errorCount = 0; // Used as threshold for how many error msgs/proof
  g_WrkProof.numSteps = 0;
  g_WrkProof.numTokens = 0;
  g_WrkProof.numHypAndLoc = 0;
  g_WrkProof.numLocalLabels = 0;
  g_WrkProof.RPNStackPtr = 0;
  g_WrkProof.localLabelPoolPtr = g_WrkProof.localLabelPool;

  fbPtr++;
  // fbPtr points to the first token now.

  // ****** This part of the code is heavily borrowed from the regular
  // ****** proof parsing, with local label and RPN handling removed,
  // ****** in order to easily parse the label section.

  // First break up the label section of proof into tokens
  while (1) {
    fbPtr = fbPtr + whiteSpaceLen(fbPtr);
    tokLength = proofTokenLen(fbPtr);
    if (!tokLength) {
      if (!g_WrkProof.errorCount) {
        sourceError(fbPtr, 2, statemNum,
            "A \")\" which ends the label list is not present.");
      }
      g_WrkProof.errorCount++;
      if (returnFlag < 3) returnFlag = 3;
      break;
    }
    if (fbPtr[0] == ')') { // End of label list
      fbPtr++;
      break;
    }
    g_WrkProof.stepSrcPtrPntr[g_WrkProof.numSteps] = fbPtr;
    g_WrkProof.stepSrcPtrNmbr[g_WrkProof.numSteps] = tokLength;
    g_WrkProof.numSteps++;
    fbPtr = fbPtr + tokLength;
  }

  fbStartProof = fbPtr; // Save pointer to start of compressed proof

  // Copy active (opt + req) hypotheses to hypAndLocLabel look-up table
  nmbrTmpPtr = g_Statement[statemNum].optHypList;
  // Transfer optional hypotheses
  while (1) {
    i = nmbrTmpPtr[g_WrkProof.numHypAndLoc];
    if (i == -1) break;
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelTokenNum = i;
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelName =
        g_Statement[i].labelName;
    g_WrkProof.numHypAndLoc++;
  }
  // Transfer required hypotheses
  j = g_Statement[statemNum].numReqHyp;
  nmbrTmpPtr = g_Statement[statemNum].reqHypList;
  for (i = 0; i < j; i++) {
    k = nmbrTmpPtr[i];
    // Required hypothesis labels are not allowed; the -1000 - k is a
    // flag that tells that they are required for error detection.
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelTokenNum = -1000 - k;
    g_WrkProof.hypAndLocLabel[g_WrkProof.numHypAndLoc].labelName =
        g_Statement[k].labelName;
    g_WrkProof.numHypAndLoc++;
  }

  // Sort the hypotheses by label name for lookup
  qsort(g_WrkProof.hypAndLocLabel, (size_t)(g_WrkProof.numHypAndLoc),
      sizeof(struct sortHypAndLoc), hypAndLocSortCmp);

  // Build the proof string (actually just a list of labels)
  g_WrkProof.proofString[g_WrkProof.numSteps] = -1; // End of proof
  // Zap mem pool actual length (because nmbrLen will be used later on this)
  nmbrZapLen(g_WrkProof.proofString, g_WrkProof.numSteps);

  // Scan proof string with the label list (not really proof steps; we're just
  // using the structure for convenience).
  for (step = 0; step < g_WrkProof.numSteps; step++) {
    tokLength = g_WrkProof.stepSrcPtrNmbr[step];
    fbPtr = g_WrkProof.stepSrcPtrPntr[step];

    // Temporarily zap the token's end with a null for string comparisons
    zapSave = fbPtr[tokLength];
    fbPtr[tokLength] = 0; // Zap source

    // See if the proof token is a hypothesis
    voidPtr = (void *)bsearch(fbPtr, g_WrkProof.hypAndLocLabel,
        (size_t)(g_WrkProof.numHypAndLoc), sizeof(struct sortHypAndLoc),
        hypAndLocSrchCmp);
    if (voidPtr) {
      // It's a hypothesis reference
      fbPtr[tokLength] = zapSave; // Unzap source
      // Label lookup number
      j = ((struct sortHypAndLoc *)voidPtr)->labelTokenNum;

      // Make sure it's not a required hypothesis, which is implicitly declared.
      if (j < 0) { // Minus is used as flag for required hypothesis
        j = -1000 - j; // Restore it to prevent side effects of the error
        if (!g_WrkProof.errorCount) {
          sourceError(fbPtr, tokLength, statemNum,
              "Required hypotheses may not be explicitly declared.");
        }
        g_WrkProof.errorCount++;
        // Tolerate this error so we can continue to work
        // on proof in Proof Assistant.
        if (returnFlag < 1) returnFlag = 1;
      }

      g_WrkProof.proofString[step] = j; // Proof string
      if (j <= 0) bug(1713);
      continue;
    } // End if hypothesis

    // See if token is an assertion label
    voidPtr = (void *)bsearch(fbPtr, g_labelKeyBase, (size_t)g_numLabelKeys,
        sizeof(long), labelSrchCmp);
    fbPtr[tokLength] = zapSave; // Unzap source
    if (!voidPtr) {
      if (!g_WrkProof.errorCount) {
        sourceError(fbPtr, tokLength, statemNum,
         "This token is not the label of an assertion or optional hypothesis.");
      }
      g_WrkProof.errorCount++;
      g_WrkProof.proofString[step] = -(long)'?';
      if (returnFlag < 2) returnFlag = 2;
      continue;
    }

    // It's an assertion ($a or $p)
    j = *(long *)voidPtr; // Statement number
    if (g_Statement[j].type != a_ && g_Statement[j].type != p_) bug(1714);
    g_WrkProof.proofString[step] = j; // Proof string

    if (j >= statemNum) { // Error
      if (!g_WrkProof.errorCount) {
        if (j == statemNum) {
          sourceError(fbPtr, tokLength, statemNum, cat(
              "The label at proof step ",
              str((double)step + 1),
             " is the label of this statement.  A statement may not be used to",
              " prove itself.",NULL));
        } else {
          assignStmtFileAndLineNum(j);
          sourceError(fbPtr, tokLength, statemNum, cat(
              "The label \"", g_Statement[j].labelName, "\" at proof step ",
              str((double)step + 1),
              " is the label of a future statement (at line ",
              str((double)(g_Statement[j].lineNum)),
              " in file ",g_Statement[j].fileName,
              ").  Only previous statements may be referenced.",
              NULL));
        }
      }
      g_WrkProof.errorCount++;
      if (returnFlag < 2) returnFlag = 2;
    }
  } // Next step

  // ***** Create the starting label map (local labels will be
  //       added as they are found) *****
  g_WrkProof.compressedPfNumLabels = g_Statement[statemNum].numReqHyp;
  nmbrTmpPtr = g_Statement[statemNum].reqHypList;
  for (i = 0; i < g_WrkProof.compressedPfNumLabels; i++) {
    g_WrkProof.compressedPfLabelMap[i] = nmbrTmpPtr[i];
  }
  for (i = 0; i < g_WrkProof.numSteps; i++) {
    g_WrkProof.compressedPfLabelMap[i + g_WrkProof.compressedPfNumLabels] =
        g_WrkProof.proofString[i];
  }
  g_WrkProof.compressedPfNumLabels = g_WrkProof.compressedPfNumLabels +
      g_WrkProof.numSteps;

  // Re-initialization for the actual proof
  g_WrkProof.numSteps = 0;
  g_WrkProof.RPNStackPtr = 0;

  // ***** Parse the compressed part of the proof *****

  // Check to see if the old buggy compression is used.  If so,
  // warn the user to reformat, and switch to the buggy algorithm so that
  // parsing can proceed.
  bggyProofLen = g_Statement[statemNum].proofSectionLen -
             (fbPtr - g_Statement[statemNum].proofSectionPtr);
  // Zap a zero at the end of the proof so we can use C string operations
  bggyZapSave = fbPtr[bggyProofLen];
  fbPtr[bggyProofLen] = 0;
  // If the proof has "UVA" but doesn't have "UUA", it means the buggy
  // algorithm was used.
  bggyAlgo = 0;
  if (strstr(fbPtr, "UV") != NULL) {
    if (strstr(fbPtr, "UU") == NULL) {
      bggyAlgo = 1;
      print2("?Warning: the proof of \"%s\" uses obsolete compression.\n",
          g_Statement[statemNum].labelName);
      print2(" Please SAVE PROOF * / COMPRESSED to reformat your proofs.\n");
    }
  }
  fbPtr[bggyProofLen] = bggyZapSave;

  // (Build the proof string and check the RPN stack)
  fbPtr = fbStartProof;
  breakFlag = 0;
  labelMapIndex = 0;
  while (1) {
    switch (chrType[(long)(fbPtr[0])]) {
      case 0: // "Letter" (i.e. A...T)
        if (!labelMapIndex) labelStart = fbPtr; // Save for error msg

        // Save pointer to source file vs. step for error messages
        tokLength = fbPtr - labelStart + 1; // Token length
        g_WrkProof.stepSrcPtrNmbr[g_WrkProof.numSteps] = tokLength;
        g_WrkProof.stepSrcPtrPntr[g_WrkProof.numSteps] = labelStart; // Token ptr

        // Corrected algorithm provided by Marnix Klooster.
        // Decoding can be done as follows:
        //   * n := 0
        //   * for each character c:
        //      * if c in ['U'..'Y']: n := n * 5 + (c - 'U' + 1)
        //      * if c in ['A'..'T']: n := n * 20 + (c - 'A' + 1)
        labelMapIndex = labelMapIndex * lettersLen +
            chrWeight[(long)(fbPtr[0])];
        if (labelMapIndex >= g_WrkProof.compressedPfNumLabels) {
          if (!g_WrkProof.errorCount) {
            sourceError(labelStart, tokLength, statemNum, cat(
     "This compressed label reference is outside the range of the label list.",
                "  The compressed label value is ", str((double)labelMapIndex),
                " but the largest label defined is ",
                str((double)(g_WrkProof.compressedPfNumLabels - 1)), ".", NULL));
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          labelMapIndex = 0; // Make it something legal to avoid side effects
        }

        stmt = g_WrkProof.compressedPfLabelMap[labelMapIndex];
        g_WrkProof.proofString[g_WrkProof.numSteps] = stmt;

        // Update stack
        hypLocUnkFlag = 0;
        if (stmt < 0) { // Local label or '?'
          hypLocUnkFlag = 1;
        } else {
          if (g_Statement[stmt].type != (char)a_ &&
              // Hypothesis
              g_Statement[stmt].type != (char)p_) hypLocUnkFlag = 1;
        }
        if (hypLocUnkFlag) { // Hypothesis, local label ref, or unknown step
          g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = g_WrkProof.numSteps;
          g_WrkProof.RPNStackPtr++;
        } else { // An assertion

          // It's a valid assertion, so pop the stack
          numReqHyp = g_Statement[stmt].numReqHyp;

          // Error check for exhausted stack
          if (g_WrkProof.RPNStackPtr < numReqHyp) { // Stack exhausted -- error
            if (!g_WrkProof.errorCount) {
              tmpStrPtr = shortDumpRPNStack();
              if (strcmp(left(tmpStrPtr,18),"RPN stack is empty")) {
                i = instr(1,tmpStrPtr,"contains ");
                let(&tmpStrPtr,cat(left(tmpStrPtr,i + 7)," only",
                  right(tmpStrPtr,i + 8),
                  NULL));
              }
              if (numReqHyp == 1) {
                let(&tmpStrPtr,cat("a hypothesis but the ",tmpStrPtr,NULL));
              } else {
                let(&tmpStrPtr,cat(str((double)numReqHyp)," hypotheses but the ",tmpStrPtr,
                    NULL));
              }
              sourceError(fbPtr, tokLength, statemNum, cat(
                  "At proof step ",
                  str((double)(g_WrkProof.numSteps + 1)),", statement \"",
                  g_Statement[stmt].labelName,"\" requires ",
                  tmpStrPtr,".",NULL));
              free_vstring(tmpStrPtr);
            }
            // Treat it like an unknown step so stack won't get exhausted
            g_WrkProof.errorCount++;
            g_WrkProof.proofString[g_WrkProof.numSteps] = -(long)'?';
            // Push the stack
            g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = g_WrkProof.numSteps;
            g_WrkProof.RPNStackPtr++;
            if (returnFlag < 3) returnFlag = 3;
            continue;
          } // End if stack exhausted

          numReqHyp = g_Statement[stmt].numReqHyp;

          // Pop the stack
          g_WrkProof.RPNStackPtr = g_WrkProof.RPNStackPtr - numReqHyp;
          g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = g_WrkProof.numSteps;
          g_WrkProof.RPNStackPtr++;
        }

        g_WrkProof.numSteps++;
        labelMapIndex = 0; // Reset it for next label
        break;

      case 1: // "Digit" (i.e. U...Y)
        if (!labelMapIndex) {
          labelMapIndex = chrWeight[(long)(fbPtr[0])] + 1;
          labelStart = fbPtr; // Save label start for error msg
        } else {
          labelMapIndex = labelMapIndex * digitsLen +
              chrWeight[(long)(fbPtr[0])] + 1;
          if (bggyAlgo) labelMapIndex--; // Adjust for buggy algorithm
        }
        break;

      case 2: // "Colon" (i.e. Z)
        if (labelMapIndex) { // In the middle of some digits
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr, 1, statemNum,
             "A compressed label character was expected here.");
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          labelMapIndex = 0;
        }

        // Put local label in label map
        g_WrkProof.compressedPfLabelMap[g_WrkProof.compressedPfNumLabels] =
          -1000 - (g_WrkProof.numSteps - 1);
        g_WrkProof.compressedPfNumLabels++;

        hypLocUnkFlag = 0;

        if (g_WrkProof.numSteps == 0) {
          // This will happen if labelChar (Z) is in 1st char pos of
          // compressed proof.
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr, 1, statemNum, cat(
              "A local label character must occur after a proof step.",NULL));
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          // We want to break here to prevent out of bound g_WrkProof.proofString
          // index below.
          break;
        }

        stmt = g_WrkProof.proofString[g_WrkProof.numSteps - 1];
        if (stmt < 0) { // Local label or '?'
          hypLocUnkFlag = 1;
        } else {
          if (g_Statement[stmt].type != (char)a_ &&
              // Hypothesis
              g_Statement[stmt].type != (char)p_) hypLocUnkFlag = 1;
        }
        if (hypLocUnkFlag) { // Hypothesis, local label ref, or unknown step.
          // If local label references a hypothesis or other local label,
          // it is not allowed because it complicates the language but doesn't
          // buy anything; would make $e to $f assignments harder to detect.
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr, 1, statemNum, cat(
                "The hypothesis or local label reference at proof step ",
                str((double)(g_WrkProof.numSteps)),
                " declares a local label.  Only \"$a\" and \"$p\" statement",
                " labels may have local label declarations.",NULL));
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }

        break;

      case 3: // White space
        break;

      case 4: // Dollar
        // See if we're at the end of the statement
        if (fbPtr[1] == '.') {
          breakFlag = 1;
          break;
        }
        // Otherwise, it must be a comment
        if (fbPtr[1] != '(' && fbPtr[1] != '!') {
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr + 1, 1, statemNum,
             "Expected \".\", \"(\", or \"!\" here.");
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        } else {
          // -1 because fbPtr will get incremented at end of loop
          fbPtr = fbPtr + whiteSpaceLen(fbPtr) - 1;
        }
        break;

      case 5: // Question mark
        if (labelMapIndex) { // In the middle of some digits
          if (!g_WrkProof.errorCount) {
            sourceError(fbPtr, 1, statemNum,
             "A compressed label character was expected here.");
          }
          g_WrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          labelMapIndex = 0;
        }

        // Save pointer to source file vs. step for error messages
        g_WrkProof.stepSrcPtrNmbr[g_WrkProof.numSteps] = 1;
        g_WrkProof.stepSrcPtrPntr[g_WrkProof.numSteps] = fbPtr; // Token ptr

        g_WrkProof.proofString[g_WrkProof.numSteps] = -(long)'?';
        // returnFlag = 1 means that proof has unknown steps.
        // Ensure that a proof with unknown steps doesn't
        // reset the severe error flag if returnFlag > 1.
        if (returnFlag < 1) returnFlag = 1;

        // Update stack
        g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = g_WrkProof.numSteps;
        g_WrkProof.RPNStackPtr++;

        g_WrkProof.numSteps++;
        break;

      case 6: // Illegal
        if (!g_WrkProof.errorCount) {
          sourceError(fbPtr, 1, statemNum,
           "This character is not legal in a compressed proof.");
        }
        g_WrkProof.errorCount++;
        if (returnFlag < 2) returnFlag = 2;
        break;
      default:
        bug(1715);
        break;
    } // End switch chrType[fbPtr[0]]

    if (breakFlag) break;
    fbPtr++;
  } // End while (1)

  if (labelMapIndex) { // In the middle of some digits
    if (!g_WrkProof.errorCount) {
      sourceError(fbPtr, 1, statemNum,
       "A compressed label character was expected here.");
    }
    g_WrkProof.errorCount++;
    if (returnFlag < 2) returnFlag = 2;
  }

  // If proof is empty, make it have one unknown step
  if (g_WrkProof.numSteps == 0) {

    // For now, this is an error.
    if (!g_WrkProof.errorCount) {
      sourceError(fbPtr, 2, statemNum,
          "The proof is empty.  If you don't know the proof, make it a \"?\".");
    }
    g_WrkProof.errorCount++;

    // Save pointer to source file vs. step for error messages
    g_WrkProof.stepSrcPtrNmbr[g_WrkProof.numSteps] = 1;
    g_WrkProof.stepSrcPtrPntr[g_WrkProof.numSteps] = fbPtr; // Token ptr

    g_WrkProof.proofString[g_WrkProof.numSteps] = -(long)'?';
    if (returnFlag < 1) returnFlag = 1; // Flag that proof has unknown steps

    // Update stack
    g_WrkProof.RPNStack[g_WrkProof.RPNStackPtr] = g_WrkProof.numSteps;
    g_WrkProof.RPNStackPtr++;

    g_WrkProof.numSteps++;
  }

  g_WrkProof.proofString[g_WrkProof.numSteps] = -1; // End of proof
  // Zap mem pool actual length (because nmbrLen will be used later on this)
  nmbrZapLen(g_WrkProof.proofString, g_WrkProof.numSteps);

  // The stack should have one entry
  if (g_WrkProof.RPNStackPtr != 1) {
    tmpStrPtr = shortDumpRPNStack();
    fbPtr = g_WrkProof.stepSrcPtrPntr[g_WrkProof.numSteps - 1];
    if (!g_WrkProof.errorCount) {
      sourceError(fbPtr, proofTokenLen(fbPtr), statemNum,
          cat("After proof step ",
          str((double)(g_WrkProof.numSteps))," (the last step), the ",
          tmpStrPtr,".  It should contain exactly one entry.",NULL));
    }
    g_WrkProof.errorCount++;
   if (returnFlag < 3) returnFlag = 3;
  }

  g_WrkProof.errorSeverity = returnFlag;
  return returnFlag;
} // parseCompressedProof

// The caller must deallocate the returned nmbrString!
// This function just gets the proof so the caller doesn't have to worry
// about cleaning up the g_WrkProof structure. The returned proof is normal
// or compressed depending on the .mm source; called nmbrUnsquishProof() to
// make sure it is uncompressed if required.
// If there is a severe error in the proof, a 1-step proof with "?" will
// be returned.
// If printFlag = 1, then error messages are printed, otherwise they aren't.
// This is only partially implemented; some errors may still result in a
// printout. TODO: pass printFlag to parseProof(), verifyProof().
// TODO: use this function to simplify some code that calls parseProof
// directly.
nmbrString *getProof(long statemNum, flag printFlag) {
  nmbrString_def(proof);
  parseProof(statemNum);
  // We do not need verifyProof() since we don't care about the math
  // strings for the proof steps in this function.
  // verifyProof(statemNum); // Necessary to set RPN stack ptrs before calling cleanWrkProof()
  if (g_WrkProof.errorSeverity > 1) {
    if (printFlag) print2(
         "The starting proof has a severe error.  It will not be used.\n");
    nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING, -(long)'?'));
  } else {
    nmbrLet(&proof, g_WrkProof.proofString);
  }
  // Note: the g_WrkProof structure is never deallocated but grows to
  // accommodate the largest proof found so far. The ERASE command will
  // deallocate it, though. cleanWrkProof() just deallocates math strings
  // assigned by verifyProof() that aren't needed by this function.
  // cleanWrkProof(); // Deallocate verifyProof() storage.
  return proof;
} // getProof

void rawSourceError(char *startFile, char *ptr, long tokLen, vstring errMsg) {
  char *startLine;
  char *endLine;
  vstring_def(errLine);
  vstring_def(errorMsg);
  vstring_def(fileName);
  long lineNum;

  let(&errorMsg, errMsg); // Prevent deallocation of errMsg

  fileName = getFileAndLineNum(startFile /* =g_sourcePtr */, ptr, &lineNum);

  // Get the line with the error on it
  startLine = ptr;
  while (startLine[0] != '\n' && startLine > startFile) {
    startLine--;
  }
  if (startLine[0] == '\n' && startLine != ptr) // In case of 0-length line
    startLine++; // Go to 1st char on line
  endLine = ptr;
  while (endLine[0] != '\n' && endLine[0] != 0) {
    endLine++;
  }
  endLine--;
  let(&errLine, space(endLine - startLine + 1));
  if (endLine - startLine + 1 < 0) bug(1721);
  memcpy(errLine, startLine, (size_t)(endLine - startLine) + 1);
  errorMessage(errLine, lineNum, ptr - startLine + 1, tokLen, errorMsg,
      fileName, 0, (char)error_);
  print2("\n");
  free_vstring(errLine);
  free_vstring(errorMsg);
  free_vstring(fileName);
} // rawSourceError

// The global g_sourcePtr is assumed to point to the start of the raw
// input buffer.
// The global g_sourceLen is assumed to be length of the raw input buffer.
// The global g_IncludeCall array is referenced.
void sourceError(char *ptr, long tokLen, long stmtNum, vstring errMsg)
{
  char *startLine;
  char *endLine;
  vstring_def(errLine);
  long lineNum;
  vstring_def(fileName);
  vstring_def(errorMsg);

  // Used for the case where a source file section has been modified
  char *locSourcePtr;

  // Initialize local pointers to raw input source
  locSourcePtr = g_sourcePtr;
  // errMsg may become deallocated if this function is
  // called with a string function argument (cat, etc.).
  let(&errorMsg, errMsg); 

  if (!stmtNum) {
    lineNum = 0;
    goto SKIP_LINE_NUM; // This isn't a source file parse
  }
  if (ptr < g_sourcePtr || ptr > g_sourcePtr + g_sourceLen) {
    // The pointer is outside the raw input buffer, so it must be a
    // SAVEd proof or other overwritten section, so there is no line number.
    // Reassign the beginning and end of the source pointer to the
    // changed section.
    if (g_Statement[stmtNum].labelSectionChanged == 1
         && ptr >= g_Statement[stmtNum].labelSectionPtr
         && ptr <= g_Statement[stmtNum].labelSectionPtr
             + g_Statement[stmtNum].labelSectionLen) {
      locSourcePtr = g_Statement[stmtNum].labelSectionPtr;
    } else if (g_Statement[stmtNum].mathSectionChanged == 1
         && ptr >= g_Statement[stmtNum].mathSectionPtr
         && ptr <= g_Statement[stmtNum].mathSectionPtr
             + g_Statement[stmtNum].mathSectionLen) {
      locSourcePtr = g_Statement[stmtNum].mathSectionPtr;
    } else if (g_Statement[stmtNum].proofSectionChanged == 1
         && ptr >= g_Statement[stmtNum].proofSectionPtr
         && ptr <= g_Statement[stmtNum].proofSectionPtr
             + g_Statement[stmtNum].proofSectionLen) {
      locSourcePtr = g_Statement[stmtNum].proofSectionPtr;
    } else {
      // ptr points to neither the original source nor a modified section
      bug(1741);
    }

    lineNum = 0;
    goto SKIP_LINE_NUM;
  }

  // free_vstring(fileName); // No need - already assigned to empty string
  fileName = getFileAndLineNum(locSourcePtr /* =g_sourcePtr here */, ptr, &lineNum);

 SKIP_LINE_NUM:
  // Get the line with the error on it
  if (lineNum != 0 && ptr > locSourcePtr) {
    startLine = ptr - 1; // Allows pointer to point to \n.
  } else {
    // Special case:  Error message starts at beginning of file or
    // the beginning of a changed section.
    // Or, it's a non-source statement; must not point to \n.
    startLine = ptr;
  }

  // Scan back to beginning of line with error
  while (startLine[0] != '\n' && startLine > locSourcePtr) startLine--;
  if (startLine[0] == '\n') startLine++;

  // Scan forward to end of line with error
  endLine = ptr;
  while (endLine[0] != '\n' && endLine[0] != 0) {
    endLine++;
  }
  endLine--;

  // Save line with error (with no newline on it)
  let(&errLine, space(endLine - startLine + 1));
  memcpy(errLine, startLine, (size_t)(endLine - startLine) + 1);

  if (!lineNum) {
    // Not a source file parse
    errorMessage(errLine, lineNum, ptr - startLine + 1, tokLen, errorMsg,
        NULL, stmtNum, (char)error_);
  } else {
    errorMessage(errLine, lineNum,
        ptr - startLine + 1, tokLen, // column
        errorMsg,
        fileName,
        stmtNum,
        (char)error_ // severity
        );
  }
  free_vstring(errLine);
  free_vstring(errorMsg);
  free_vstring(fileName);
} // sourceError

void mathTokenError(long tokenNum, // 0 is 1st one
    nmbrString *tokenList, long stmtNum, vstring errMsg)
{
  long i;
  char *fbPtr;
  fbPtr = g_Statement[stmtNum].mathSectionPtr;
  fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  // Scan past the tokens before the desired one.
  // We use the parsed token length rather than tokenLen() to
  // account for adjacent tokens with no white space.
  for (i = 0; i < tokenNum; i++) {
    fbPtr = fbPtr + g_MathToken[tokenList[i]].length;
    fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  }
  sourceError(fbPtr, g_MathToken[tokenList[tokenNum]].length,
      stmtNum, errMsg);
} // mathTokenError

vstring shortDumpRPNStack(void) {
  // The caller must deallocate the returned string.
  vstring_def(tmpStr);
  vstring_def(tmpStr2);
  long i, k, m;

  for (i = 0; i < g_WrkProof.RPNStackPtr; i++) {
     k = g_WrkProof.RPNStack[i]; // Step
     let(&tmpStr,space(g_WrkProof.stepSrcPtrNmbr[k]));
     memcpy(tmpStr,g_WrkProof.stepSrcPtrPntr[k],
         (size_t)(g_WrkProof.stepSrcPtrNmbr[k])); // Label at step
     let(&tmpStr2,cat(
         tmpStr2,", ","\"",tmpStr,"\" (step ", str((double)k + 1),")",NULL));
  }
  let(&tmpStr2,right(tmpStr2,3));
  if (g_WrkProof.RPNStackPtr == 2) {
    m = instr(1, tmpStr2, ",");
    let(&tmpStr2,cat(left(tmpStr2,m - 1)," and ",
        right(tmpStr2,m + 1),NULL));
  }
  if (g_WrkProof.RPNStackPtr > 2) {
    for (m = (long)strlen(tmpStr2); m > 0; m--) { // Find last comma
      if (tmpStr2[m - 1] == ',') break;
    }
    let(&tmpStr2,cat(left(tmpStr2,m - 1),", and ",
        right(tmpStr2,m + 1),NULL));
  }
  if (g_WrkProof.RPNStackPtr == 1) {
    let(&tmpStr2,cat("one entry, ",tmpStr2,NULL));
  } else {
    let(&tmpStr2,cat(str((double)(g_WrkProof.RPNStackPtr))," entries: ",tmpStr2,NULL));
  }
  let(&tmpStr2,cat("RPN stack contains ",tmpStr2,NULL));
  if (g_WrkProof.RPNStackPtr == 0) let(&tmpStr2,"RPN stack is empty");
  free_vstring(tmpStr);
  return tmpStr2;
} // shortDumpRPNStack

// ???Todo:  use this elsewhere in mmpars.c to modularize this lookup
// Lookup $a or $p label and return statement number.
// Return -1 if not found.
long lookupLabel(const char *label)
{
  void *voidPtr; // bsearch returned value
  long statemNum;
  // Find the statement number
  voidPtr = (void *)bsearch(label, g_labelKeyBase, (size_t)g_numLabelKeys,
      sizeof(long), labelSrchCmp);
  if (!voidPtr) {
    return -1;
  }
  statemNum = (*(long *)voidPtr); // Statement number
  if (g_Statement[statemNum].type != a_ && g_Statement[statemNum].type != p_)
    bug(1718);
  return statemNum;
} // lookupLabel

// Label comparison for qsort
int labelSortCmp(const void *key1, const void *key2) {
  // Returns -1 if key1 < key2, 0 if equal, 1 if key1 > key2
  return strcmp(g_Statement[ *((long *)key1) ].labelName,
      g_Statement[ *((long *)key2) ].labelName);
} // labelSortCmp

// Label comparison for bsearch
int labelSrchCmp(const void *key, const void *data) {
  // Returns -1 if key < data, 0 if equal, 1 if key > data
  return strcmp(key, g_Statement[ *((long *)data) ].labelName);
} // labelSrchCmp

// Math symbol comparison for qsort
int mathSortCmp(const void *key1, const void *key2) {
  // Returns -1 if key1 < key2, 0 if equal, 1 if key1 > key2
  return strcmp(g_MathToken[ *((long *)key1) ].tokenName,
      g_MathToken[ *((long *)key2) ].tokenName);
}

// Math symbol comparison for bsearch
// Here, key is pointer to a character string.
int mathSrchCmp(const void *key, const void *data) {
  // Returns -1 if key < data, 0 if equal, 1 if key > data
  return strcmp(key, g_MathToken[ *((long *)data) ].tokenName);
}

// Hypotheses and local label comparison for qsort
int hypAndLocSortCmp(const void *key1, const void *key2) {
  // Returns -1 if key1 < key2, 0 if equal, 1 if key1 > key2
  return strcmp(
    ((struct sortHypAndLoc *)key1)->labelName,
    ((struct sortHypAndLoc *)key2)->labelName);
}

// Hypotheses and local label comparison for bsearch.
// Here, key is pointer to a character string.
int hypAndLocSrchCmp(const void *key, const void *data) {
  // Returns -1 if key < data, 0 if equal, 1 if key > data
  return strcmp(key, ((struct sortHypAndLoc *)data)->labelName);
}

// This function returns the length of the white space starting at ptr.
// Comments are considered white space.  ptr should point to the first character
// of the white space.  If ptr does not point to a white space character, 0
// is returned.  If ptr points to a null character, 0 is returned.
long whiteSpaceLen(char *ptr) {
  long i = 0;
  char tmpchr;
  char *ptr1;
  while (1) {
    tmpchr = ptr[i];
    if (!tmpchr) return (i); // End of string
    if (tmpchr == '$') {
      if (ptr[i + 1] == '(') {
        while (1) {
          // ptr1 = strchr(ptr + i + 2, '$');
          // in-line code for speed
          // (for the lcc-win32 compiler, this speeds it up from 94 sec
          // for set.mm read to 4 sec).
          for (ptr1 = ptr + i + 2; ptr1[0] != '$'; ptr1++) {
            if (ptr1[0] == 0) {
              if ('$' != 0)
                ptr1 = NULL;
              break;
            }
          }
          // end in-line strchr code
          if (!ptr1) {
            return i + (long)strlen(&ptr[i]); // Unterminated comment - goto EOF
          }
          if (ptr1[1] == ')') break;
          i = ptr1 - ptr;
        }
        i = ptr1 - ptr + 2;
        continue;
      } else {
        if (ptr[i + 1] == '!') {
          ptr1 = strchr(ptr + i + 2, '\n');
          if (!ptr1) bug(1716);
          i = ptr1 - ptr + 1;
          continue;
        }
        return i;
      }
    } // if (tmpchr == '$')
    if (isgraph((unsigned char)tmpchr)) return i;
    i++;
  }
  return 0; // Dummy return - never happens
} // whiteSpaceLen

// For .mm file splitting.
// This function is like whiteSpaceLen() except that comments are NOT
// considered white space.  ptr should point to the first character
// of the white space.  If ptr does not point to a white space character, 0
// is returned.  If ptr points to a null character, 0 is returned.
long rawWhiteSpaceLen(char *ptr) {
  long i = 0;
  char tmpchr;
  while (1) {
    tmpchr = ptr[i];
    if (!tmpchr) return (i); // End of string
    if (isgraph((unsigned char)tmpchr)) return i;
    i++;
  }
  return 0; // Dummy return - never happens
} // rawWhiteSpaceLen

// This function returns the length of the token (non-white-space) starting at
// ptr.  Comments are considered white space.  ptr should point to the first
// character of the token.  If ptr points to a white space character, 0
// is returned.  If ptr points to a null character, 0 is returned.  If ptr
// points to a keyword, 0 is returned.  A keyword ends a token.
// Tokens may be of the form "$nn"; this is tolerated (used in parsing
// user math strings in parseMathTokens()).  An (illegal) token of this form
// in the source will be detected earlier, so this won't cause
// syntax violations to slip by in the source.
long tokenLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  while (1) {
    tmpchr = ptr[i];
    if (tmpchr == '$') {
      if (ptr[i + 1] == '$') { // '$$' character
        i = i + 2;
        continue;
      } else {
        // Tolerate digit after "$"
        if (ptr[i + 1] >= '0' && ptr[i + 1] <= '9') {
          i = i + 2;
          continue;
        } else {
          return i; // Keyword or comment
        }
      }
    }
    if (!isgraph((unsigned char)tmpchr)) return i; // White space or null
    i++;
  }
  return 0; // Dummy return (never happens)
} // tokenLen

// This function returns the length of the token (non-white-space) starting at
// ptr.  Comments are considered white space.  ptr should point to the first
// character of the token.  If ptr points to a white space character, 0
// is returned.  If ptr points to a null character, 0 is returned.
// Unlike tokenLen(), keywords are not treated as special.  In particular:
// if ptr points to a keyword, 0 is NOT returned (instead, 2 is returned),
// and a keyword does NOT end a token (which is a relic of days before
// whitespace surrounding a token was part of the spec, but still serves
// a useful purpose in token() for friendlier error detection).
long rawTokenLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  while (1) {
    tmpchr = ptr[i];
    if (!isgraph((unsigned char)tmpchr)) return i; // White space or null
    i++;
  }
  return 0; // Dummy return (never happens)
} // rawTokenLen

// This function returns the length of the proof token starting at
// ptr.  Comments are considered white space.  ptr should point to the first
// character of the token.  If ptr points to a white space character, 0
// is returned.  If ptr points to a null character, 0 is returned.  If ptr
// points to a keyword, 0 is returned.  A keyword ends a token.
// ":" and "?" and "(" and ")" and "=" are considered tokens.
long proofTokenLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  if (ptr[0] == ':') return 1; // The token is a colon
  if (ptr[0] == '?') return 1; // The token is a "?"
  if (ptr[0] == '(') return 1; // The token is a "(" (compressed proof)
  if (ptr[0] == ')') return 1; // The token is a ")" (compressed proof)
  if (ptr[0] == '=') return 1; // The token is a "=" (/EXPLICIT proof)
  while (1) {
    tmpchr = ptr[i];
    if (tmpchr == '$') {
      if (ptr[i + 1] == '$') { // '$$' character
        i = i + 2;
        continue;
      } else {
        return i; // Keyword or comment
      }
    }
    if (!isgraph((unsigned char)tmpchr)) return i; // White space or null
    if (tmpchr == ':') return i; // Colon ends a token
    if (tmpchr == '?') return i; // "?" ends a token
    if (tmpchr == '(') return i; // "(" ends a token
    if (tmpchr == ')') return i; // ")" ends a token
    if (tmpchr == '=') return i; // "=" ends a token
    i++;
  }
  return 0; // Dummy return - never happens
}

// Counts the number of \n between start for length chars.
// If length = -1, then use end-of-string 0 to stop.
// If length >= 0, then scan at most length chars, but stop
// if end-of-string 0 is found.
long countLines(const char *start, long length) {
  long lines, i;
  lines = 0;
  if (length == -1) {
    i = 0;
    while (1) {
      if (start[i] == '\n') lines++;
      if (start[i] == 0) break;
      i++;
    }
  } else {
    for (i = 0; i < length; i++) {
      if (start[i] == '\n') lines++;
      if (start[i] == 0) break;
    }
  }
  return lines;
} // countLines

// Return (for output) the complete contents of a statement, including all
// white space and comments, from first token through all white space and
// comments after last token.
// This allows us to modify the input file with Metamath.
// Note: the text near end of file is obtained from g_Statement[g_statements + 1]
// ???This does not yet implement restoration of the various input files;
// all included files are merged into one.
// Caller must deallocated returned string.
// reformatFlag= 0: WRITE SOURCE, 1: WRITE SOURCE / FORMAT,
// 2: WRITE SOURCE / REWRAP.
// Note that the labelSection, mathSection, and proofSection do not
// contain keywords ($a, $p,...; $=; $.).  The keywords are added
// by this function when the statement is written.
// This must be called in sequence for all statements,
// since the previous statement is needed to populate dollarDpos
// and previousType.
vstring outputStatement(long stmt, flag reformatFlag) {
  vstring_def(labelSection);
  vstring_def(mathSection);
  vstring_def(proofSection);
  vstring_def(labelSectionSave);
  vstring_def(mathSectionSave);
  vstring_def(proofSectionSave);
  vstring_def(output);
  // For reformatting:
  long slen; // To save local string length
  long pos;
  long indent;
  static long dollarDpos = 0;
  static char previousType = illegal_; // '?' in mmdata.h
  long commentStart;
  long commentEnd;
  vstring_def(comment);
  vstring_def(str1);
  long length;
  flag nowrapHtml;

  // Re-initialize static variables for a second 'write source'
  if (stmt == 1) {
    previousType = illegal_; // '?' in mmdata.h
    dollarDpos = 0;
  }

  let(&labelSection, space(g_Statement[stmt].labelSectionLen));
  memcpy(labelSection, g_Statement[stmt].labelSectionPtr,
      (size_t)(g_Statement[stmt].labelSectionLen));

  if (stmt == g_statements + 1) return labelSection; // Special case - EOF

  let(&mathSection, space(g_Statement[stmt].mathSectionLen));
  memcpy(mathSection, g_Statement[stmt].mathSectionPtr,
      (size_t)(g_Statement[stmt].mathSectionLen));

  let(&proofSection, space(g_Statement[stmt].proofSectionLen));
  memcpy(proofSection, g_Statement[stmt].proofSectionPtr,
      (size_t)(g_Statement[stmt].proofSectionLen));

  let(&labelSectionSave, labelSection);
  let(&mathSectionSave, mathSection);
  let(&proofSectionSave, proofSection);

  // Reformat statements to match the current set.mm convention
  if (reformatFlag > 0) { // 1 = WRITE SOURCE / FORMAT or 2 = / REWRAP
// Put standard indentation before the ${, etc.
#define INDENT_FIRST 2
#define INDENT_INCR 2
    indent = INDENT_FIRST + (INDENT_INCR * g_Statement[stmt].scope);
    // g_Statement[stmt].scope is at start of stmt not end; adjust $}
    if (g_Statement[stmt].type == rb_) indent = indent - INDENT_INCR;

    // Untab the label section
    if (strchr(labelSection, '\t') != NULL) { // Only if has tab (speedup)
      let(&labelSection, edit(labelSection, 2048)); // untab
    }
    // Untab the math section
    if (strchr(mathSection, '\t') != NULL) { // Only if has tab (speedup)
      let(&mathSection, edit(mathSection, 2048)); // untab
    }

    // Reformat the label section

    // Remove spaces a end of line.
    // This is a pretty inefficient loop, but hopefully lots of spaces
    // at end of lines is a rare occurrence.
    while (1) {
      pos = instr(1, labelSection, " \n");
      if (pos == 0) break;
      let(&labelSection, cat(left(labelSection, pos - 1),
          right(labelSection, pos + 1), NULL));
    }

    // Don't allow more than 2 consecutive blank lines
    while (1) {
      // Match to 3 or more blank lines
      pos = instr(1, labelSection, "\n\n\n\n");
      // If it matches, remove one of the \n and restart loop
      if (pos == 0) break;
      let(&labelSection, cat(left(labelSection, pos - 1),
          right(labelSection, pos + 1), NULL));
    }

    switch (g_Statement[stmt].type) {
      case lb_: // ${
      case rb_: // $}
      case v_: // $v
      case c_: // $c
      case d_: // $d
      // These 5 cases are for keywords that don't have labels, so that
      // labelSection is simply the text before the keyword.

        // Strip any trailing spaces
        let(&labelSection, edit(labelSection, 128)); // trailing spaces
        slen = (long)strlen(labelSection); // Save to avoid recomputing
        // See if last character is \n; if not, add it
        // (If there's no text - just spaces - they've been stripped, and
        // leave labelSection as an empty string).
        // We use slen - 1 because native C strings start at index 0.
        if (slen != 0 && labelSection[slen - 1] != '\n') {
          let(&labelSection, cat(labelSection, "\n", NULL));
          slen++;
        }
        // Put a blank line between $} and ${ if there is none.
        if (g_Statement[stmt].type == lb_
            && previousType == rb_) {
          if (slen == 0) {
            // There's no text (comment) between $} and ${, so make it
            // a blank line.
            let(&labelSection, "\n\n");
            slen = 2;
          } else {
            // There's text between $} and ${
            if (instr(1, labelSection, "\n\n") == 0) {
              // If there's no blank line, add one (note that code above
              // ensures non-empty labelSection will end with \n, so
              // add just 1 more).
              let(&labelSection, cat(labelSection, "\n", NULL));
              slen++;
            }
          } // if slen == 0 else
        } // if $}...${
        if (slen == 0) {
          // If the statement continues on this line, put 2 spaces before it
          let(&labelSection, cat(labelSection, "  ", NULL));
          slen = 2;
        } else {
          // Add indentation to end of labelSection i.e. before the keyword.
          // If there was text (comment) before the keyword on the same line,
          // it now has a \n after it, thus the indentation of the keyword will
          // be consistent.
          let(&labelSection, cat(labelSection, space(indent), NULL));
          slen = slen + indent;
        }
        if (g_Statement[stmt].type == d_) // $d 
        {
          // Try to put as many $d's on one line as will fit.
          // First we remove redundant spaces in mathSection.
          // Don't discard \n so that user can
          // insert \n to break a huge $d with say >40 variables,
          // which itself can exceed line length.
          let(&mathSection, edit(mathSection,
              /* 4 */ /* discard \n */ + 16 /* reduce spaces */));
          // This and previous $d are separated by spaces and newlines only 
          if (strlen(edit(labelSection, 4 /* discard \n */ + 2 /* discard spaces */)) == 0)
              {
            if (previousType == d_) { // The previous statement was a $d
              // See if the $d will fit on the current line
              if (dollarDpos + 2 + (signed)(strlen(mathSection)) + 4
                  <= g_screenWidth) {
                let(&labelSection, "  "); // 2 spaces between $d's
                dollarDpos = dollarDpos + 2 + (long)strlen(mathSection) + 4;
              } else {
                // The $d assembly overflowed; start on new line
                // Add 4 = '$d' length + '$.' length
                dollarDpos = indent + (long)strlen(mathSection) + 4;
                // Start new line
                let(&labelSection, cat("\n", space(indent), NULL));
              }
            } else { // previousType != $d
              // If the previous statement type (keyword) was not $d,
              // we want to start the assembly of $d statements here.
              dollarDpos = indent + (long)strlen(mathSection) + 4;
            } // if previousType == $d else
          } else {
            // There is some text (comment) between this $d and previous,
            // so we restart assembling $d groups on this line.
            dollarDpos = indent + (long)strlen(mathSection) + 4;
          } // if labelSection = spaces and newlines only else
        } // if g_Statement[stmt].type == d_

        break; // End of ${, $}, $v, $c, $d cases
      case a_: // $a
      case p_: // $p
        // Get last $(
        commentStart = rinstr(labelSection,  "$(");
        // Get last $)
        commentEnd = rinstr(labelSection, "$)") + 1;
        if (commentEnd < commentStart) {
          print2("?Make sure syntax passes before running / REWRAP.\n");
          print2("(Forcing a bug check since output may be corrupted.)\n");
          bug(1725);
        }
        if (commentStart != 0) {
          let(&comment, seg(labelSection, commentStart, commentEnd));
        } else {
          // If there is no comment before $a or $p, add dummy comment
          let(&comment, "$( PLEASE PUT DESCRIPTION HERE. $)");
        }

        let(&labelSection, left(labelSection, commentStart - 1));
        // Get the last newline before the comment
        pos = rinstr(labelSection, "\n");

        // If previous statement was $e, take out any blank line
        if (previousType == e_ && pos == 2 && labelSection[0] == '\n') {
          let(&labelSection, right(labelSection, 2));
          pos = 1;
        }

        // If there is no '\n', insert it (unless first line in file)
        if (pos == 0 && stmt > 1) {
          let(&labelSection, cat(edit(labelSection, 128), // trailing spaces
              "\n", NULL));
          pos = (long)strlen(labelSection) + 1;
        }

        // If comment has "<HTML>", don't reformat.
        if (instr(1, comment, "<HTML>") != 0) {
          nowrapHtml = 1;
        } else {
          nowrapHtml = 0;
        }

        if (nowrapHtml == 0) {
          // This strips off leading spaces before $( (start of comment).  Don't
          // do it for <HTML>, since spacing before $( is manual.
          let(&labelSection, left(labelSection, pos));

          if (reformatFlag == 2) {
            // If / REWRAP was specified, unwrap and rewrap the line
            free_vstring(str1);
            str1 = rewrapComment(comment);
            let(&comment, str1);
          }

          // Make sure that start of new lines inside the comment have no
          // trailing space (because printLongLine doesn't do this after
          // explicit break).
          pos = 0;
          while (1) {
            pos = instr(pos + 1, comment, "\n");
            if (pos == 0) break; // Beyond last line in comment
            // Remove leading spaces from comment line
            length = 0;
            while (1) {
              if (comment[pos + length] != ' ') break;
              length++;
            }
            // Add back indent+3 spaces to beginning of line in comment
            let(&comment, cat(left(comment, pos),
                (comment[pos + length] != '\n')
                    ? space(indent + 3)
                    : "", // Don't add indentation if line is blank
                right(comment, pos + length + 1), NULL));
          }

          // Reformat the comment to wrap if necessary
          if (g_outputToString == 1) bug(1726);
          g_outputToString = 1;
          free_vstring(g_printString);

          printLongLine(cat(space(indent), comment, NULL),
              space(indent + 3), " ");
          let(&comment, g_printString);
          free_vstring(g_printString);
          g_outputToString = 0;
#define ASCII_4 4
          // Restore ASCII_4 characters put in by rewrapComment() to space
          length = (long)strlen(comment);
          for (pos = 2; pos < length - 2; pos++) {
             // For debugging:
             // if (comment[pos] == ASCII_4) comment[pos] = '#';
             if (comment[pos] == ASCII_4) comment[pos] = ' ';
          }
        } // if nowrapHtml == 0
        else {
          // If there was an <HTML> tag, we don't modify the comment.
          // However, we need "\n" after "$)" (end of comment), corresponding to
          // the one that was put there by printLongLine() in the normal
          // non-<HTML> case above.
          let(&comment, cat(comment, "\n", NULL));
        }

        // Remove any trailing spaces
        pos = 2;
        while(1) {
          pos = instr(pos + 1, comment, " \n");
          if (!pos) break;
          let(&comment, cat(left(comment, pos - 1), right(comment, pos + 1),
              NULL));
          pos = pos - 2;
        }

        // Complete the label section
        let(&labelSection, cat(labelSection, comment,
            space(indent), g_Statement[stmt].labelName, " ", NULL));
        break; // End of $a, $p cases
      case e_: // $e
      case f_: // $f
        pos = rinstr(labelSection, g_Statement[stmt].labelName);
        let(&labelSection, left(labelSection, pos - 1));
        pos = rinstr(labelSection, "\n");
        // If there is none, insert it (unless first line in file)
        if (pos == 0 && stmt > 1) {
          let(&labelSection, cat(edit(labelSection, 128), // trailing spaces
              "\n", NULL));
          pos = (long)strlen(labelSection) + 1;
        }
        let(&labelSection, left(labelSection, pos));
        // If previous statement is $d or $e and there is no comment after it,
        // discard entire rest of label to get rid of redundant blank lines.
        if ((previousType == d_ || previousType == e_)
            && instr(1, labelSection, "$(") == 0) {
          let(&labelSection, "\n");
        }
        // Complete the label section
        let(&labelSection, cat(labelSection,
            space(indent), g_Statement[stmt].labelName, " ", NULL));
        break; // End of $e, $f cases
      default: bug(1727);
    } // switch (g_Statement[stmt].type)

    // Reformat the math section
    switch (g_Statement[stmt].type) {
      case lb_: // ${
      case rb_: // $}
      case v_: // $v
      case c_: // $c
      case d_: // $d
      case a_: // $a
      case p_: // $p
      case e_: // $e
      case f_: // $f
        // Remove blank lines
        while (1) {
          pos = instr(1, mathSection, "\n\n");
          if (pos == 0) break;
          let(&mathSection, cat(left(mathSection, pos),
              right(mathSection, pos + 2), NULL));
        }

        // Reduce multiple in-line spaces to single space
        pos = 0;
        while (1) {
          pos = instr(pos + 1, mathSection, "  ");
          if (pos == 0) break;
          if (pos > 1) {
            if (mathSection[pos - 2] != '\n' && mathSection[pos - 2] != ' ') {
              // It's not at the start of a line, so reduce it
              let(&mathSection, cat(left(mathSection, pos),
                  right(mathSection, pos + 2), NULL));
              pos--;
            }
          }
        }

        break;
      default: bug(1729);
    }

    // Set previous state for next statement
    if (g_Statement[stmt].type == d_) {
      // dollarDpos is computed in the processing above
    } else {
      dollarDpos = 0; // Reset it
    }
    previousType = g_Statement[stmt].type;
  } // if reformatFlag

  let(&output, labelSection);

  // Add statement keyword
  let(&output, cat(output, "$", chr(g_Statement[stmt].type), NULL));

  // Add math section and proof
  if (g_Statement[stmt].mathSectionLen != 0) {
    let(&output, cat(output, mathSection, NULL));

    if (g_Statement[stmt].type == (char)p_) {
      let(&output, cat(output, "$=", proofSection, NULL));
    }
    let(&output, cat(output, "$.", NULL));
  }

  // Make sure the line has no carriage-returns
  if (strchr(output, '\r') != NULL) {

    // We are now using readFileToString, so this should never happen.
    bug(1758);

    // This may happen with Cygwin's gcc, where DOS CR-LF becomes CR-CR-LF
    // in the output file.
    // Someday we should investigate the use of readFileToString() in
    // mminou.c for the main set.mm READ function, to solve this cleanly.
    let(&output, edit(output, 8192)); // Discard CR's
  }

  // This function is no longer used to supply the output, but just to
  // do any reformatting/wrapping.  Now writeSourceToBuffer() builds the
  // output source.  So instead, we update the g_Statement[] content with
  // any changes, which are read by writeSourceToBuffer() and also saved
  // in the g_Statement[] array for any future write source.  Eventually
  // we should replace WRITE SOURCE.../REWRAP with a REWRAP(?) command.
  if (strcmp(labelSection, labelSectionSave)) {
    g_Statement[stmt].labelSectionLen = (long)strlen(labelSection);
    if (g_Statement[stmt].labelSectionChanged == 1) {
      let(&(g_Statement[stmt].labelSectionPtr), labelSection);
    } else {
      // This is the first time we've updated the label section
      g_Statement[stmt].labelSectionChanged = 1;
      g_Statement[stmt].labelSectionPtr = labelSection;
      labelSection = ""; // so that labelSectionPtr won't be deallocated
    }
  }
  if (strcmp(mathSection, mathSectionSave)) {
    g_Statement[stmt].mathSectionLen = (long)strlen(mathSection);
    if (g_Statement[stmt].mathSectionChanged == 1) {
      let(&(g_Statement[stmt].mathSectionPtr), mathSection);
    } else {
      // This is the first time we've updated the math section
      g_Statement[stmt].mathSectionChanged = 1;
      g_Statement[stmt].mathSectionPtr = mathSection;
      mathSection = ""; // so that mathSectionPtr won't be deallocated
    }
  }
  // (I don't see anywhere that proofSection will change. So make
  // it a bug check to force us to look into it.)
  if (strcmp(proofSection, proofSectionSave)) {
    bug(1757); // This may not be a bug
    g_Statement[stmt].proofSectionLen = (long)strlen(proofSection);
    if (g_Statement[stmt].proofSectionChanged == 1) {
      let(&(g_Statement[stmt].proofSectionPtr), proofSection);
    } else {
      // This is the first time we've updated the proof section
      g_Statement[stmt].proofSectionChanged = 1;
      g_Statement[stmt].proofSectionPtr = proofSection;
      proofSection = ""; // so that proofSectionPtr won't be deallocated
    }
  }

  free_vstring(labelSection);
  free_vstring(mathSection);
  free_vstring(proofSection);
  free_vstring(labelSectionSave);
  free_vstring(mathSectionSave);
  free_vstring(proofSectionSave);
  free_vstring(comment);
  free_vstring(str1);
  return output; // The calling routine must deallocate this vstring
} // outputStatement

// Unwrap the lines in a comment then re-wrap them according to set.mm
// conventions.  This may be overly aggressive, and user should do a
// diff to see if result is as desired.  Called by WRITE SOURCE / REWRAP.
// Caller must deallocate returned vstring.
vstring rewrapComment(const char *comment1) {
// Punctuation from mmwtex.c
#define OPENING_PUNCTUATION "(['\""
// #define CLOSING_PUNCTUATION ".,;)?!:]'\"_-"
#define CLOSING_PUNCTUATION ".,;)?!:]'\""
#define SENTENCE_END_PUNCTUATION ")'\""
  vstring_def(comment);
  vstring_def(commentTemplate); // Non-breaking space template
  long length, pos, i, j;
  vstring ch; // Pointer only; do not allocate
  flag mathmode = 0;

  let(&comment, comment1); // Grab arg so it can be reassigned

  // Make sure back quotes are surrounded by space
  pos = 2;
  mathmode = 0;
  while (1) {
    pos = instr(pos + 1, comment, "`");
    if (pos == 0) break;
    mathmode = (flag)(1 - mathmode);
    // See if previous or next char is "`"; ignore "``" escape
    if (comment[pos - 2] == '`' || comment[pos] == '`') continue;
    if (comment[pos] != ' ' && comment[pos] != '\n') {
      // Currently, mmwtex.c doesn't correctly handle broken subscript (_)
      // or broken hyphen (-) in the CLOSING_PUNCTUATION, so allow these two as
      // exceptions until that is fixed.   E.g. ` a ` _2 doesn't yield
      // HTML subscript; instead we need ` a `_2.
      if (mathmode == 1 || (comment[pos] != '_' && comment[pos] != '-')) {
        // Add a space after back quote if none
        let(&comment, cat(left(comment, pos), " ",
            right(comment, pos + 1), NULL));
      }
    }
    if (comment[pos - 2] != ' ') {
      // Add a space before back quote if none
      let(&comment, cat(left(comment, pos - 1), " ",
          right(comment, pos), NULL));
      pos++; // Go past the "`"
    }
  }

  // Make sure "~" for labels are surrounded by space.
  // For now, just process comments not containing math symbols. 
  // More complicated code is needed to ignore ~ in math symbols; maybe add it someday.
  if (instr(2, comment, "`") == 0) {  
    pos = 2;
    while (1) {
      pos = instr(pos + 1, comment, "~");
      if (pos == 0) break;
      // See if previous or next char is "~"; ignore "~~" escape
      if (comment[pos - 2] == '~' || comment[pos] == '~') continue;
      if (comment[pos] != ' ') {
        // Add a space after tilde if none
        let(&comment, cat(left(comment, pos), " ",
            right(comment, pos + 1), NULL));
      }
      if (comment[pos - 2] != ' ') {
        // Add a space before tilde if none
        let(&comment, cat(left(comment, pos - 1), " ",
            right(comment, pos), NULL));
        pos++; // Go past the "~"
      }
    }
  }

  // Change all newlines to space unless double newline.
  // Note:  it is assumed that blank lines have no spaces
  // for this to work; the user must ensure that.
  length = (long)strlen(comment);
  for (pos = 2; pos < length - 2; pos++) {
    if (comment[pos] == '\n' && comment[pos - 1] != '\n'
        && comment[pos + 1] != '\n')
      comment[pos] = ' ';
  }
  let(&comment, edit(comment, 16)); // reduce spaces

  // Remove spaces and blank lines at end of comment
  while (1) {
    length = (long)strlen(comment);
    // Should have been syntax err (no space before "$)")
    if (comment[length - 3] != ' ') bug(1730);
    if (comment[length - 4] != ' ' && comment[length - 4] != '\n') break;
    let(&comment, cat(left(comment, length - 4),
        right(comment, length - 2), NULL));
  }

  // Put period at end of comment ending with lowercase letter.
  // Note:  This will not detect a '~ label' at end of comment.
  // However, user should have ended it with a period, and if not the
  // label + period is unlikely to be valid and thus will
  // usually be detected by 'verify markup'.
  // (We could enhance the code here to do that if it becomes a problem.)
  length = (long)strlen(comment);
  if (islower((unsigned char)(comment[length - 4]))) {
    let(&comment, cat(left(comment, length - 3), ". $)", NULL));
  }

  // Change to ASCII 4 those spaces where the line shouldn't be broken.
  mathmode = 0;
  for (pos = 3; pos < length - 2; pos++) {
    if (comment[pos] == '`') { // Start or end of math string
      mathmode = (char)(1 - mathmode);
    }
    if ( mathmode == 1 && comment[pos] == ' ')
      // We assign comment[] rather than commentTemplate to avoid confusion of
      // math with punctuation.  Also, commentTemplate would be misaligned
      // anyway due to adding of spaces below.
      comment[pos] = ASCII_4;
  }

  // Look for proof discouraged or usage discouraged markup and change their
  // spaces to ASCII 4 to prevent line breaks in the middle.
  if (g_proofDiscouragedMarkup[0] == 0) {
    // getMarkupFlags() in mmdata.c has never been called, so initialize the
    // markup strings to their defaults.
    let(&g_proofDiscouragedMarkup, PROOF_DISCOURAGED_MARKUP);
    let(&g_usageDiscouragedMarkup, USAGE_DISCOURAGED_MARKUP);
  }
  pos = instr(1, comment, g_proofDiscouragedMarkup);
  if (pos != 0) {
    i = (long)strlen(g_proofDiscouragedMarkup);
    for (j = pos; j < pos + i - 1; j++) { // Check 2nd thru penultimate char
      if (comment[j] == ' ') {
        comment[j] = ASCII_4;
      }
    }
  }
  pos = instr(1, comment, g_usageDiscouragedMarkup);
  if (pos != 0) {
    i = (long)strlen(g_usageDiscouragedMarkup);
    for (j = pos; j < pos + i - 1; j++) { // Check 2nd thru penultimate char
      if (comment[j] == ' ') {
        comment[j] = ASCII_4;
      }
    }
  }

  // Put two spaces after end of sentence and colon
  ch = ""; // Prevent compiler warning
  for (i = 0; i < 4; i++) {
    switch (i) {
      case 0: ch = "."; break;
      case 1: ch = "?"; break;
      case 2: ch = "!"; break;
      case 3: ch = ":";
    }
    pos = 2;
    while (1) {
      pos = instr(pos + 1, comment, ch);
      if (pos == 0) break;
      if (ch[0] == '.' && comment[pos - 2] >= 'A' && comment[pos - 2] <= 'Z')
        continue;  // Ignore initials of names
      if (strchr(SENTENCE_END_PUNCTUATION, comment[pos]) != NULL)
        pos++;
      if (comment[pos] != ' ') continue;
      if ((comment[pos + 1] >= 'A' && comment[pos + 1] <= 'Z')
          || strchr(OPENING_PUNCTUATION, comment[pos + 1]) != NULL) {
        // A change of space to ASCII_4 is not needed, and in fact
        // prevents end of sentence e.g. "." from ever appearing at column 79,
        // triggering an earlier break that makes line unnecessarily short.
        // There is no problem with next line having leading space:
        // it is removed in mminou.c (search
        // for "Remove leading space for neatness" there).  (Note that we use
        // ASCII_4 to prevent bad line breaks, then later change them to
        // spaces.)
        // comment[pos] = ASCII_4;

        // Add a second space after end of sentence, which is recommended for
        // monospaced (typewriter) fonts to more easily see sentence
        // separation.
        let(&comment, cat(left(comment, pos + 1), " ",
            right(comment, pos + 2), NULL));
      }
    } // end while
  } // next i

  length = (long)strlen(comment);
  let(&commentTemplate, space(length));
  for (pos = 3; pos < length - 2; pos++) {
    if (comment[pos] == ' ') {
      if (comment[pos - 1] == '~' && comment[pos - 2] != '~') {
        // Don't break "~ <label>"
        commentTemplate[pos] = ASCII_4;
      } else if ((comment[pos - 2] == ' '
            || strchr(OPENING_PUNCTUATION, comment[pos - 2]) != NULL)
          && strchr(OPENING_PUNCTUATION, comment[pos - 1]) != NULL) {
        // Don't break space after opening punctuation
        commentTemplate[pos] = ASCII_4;
      } else if ((comment[pos + 2] == ' '
            || comment[pos + 2] == '\n' // Period etc. before line break
            // 2-space sentence break done above where 1st space is ASCII_4
            || comment[pos + 2] == ASCII_4 
            || strchr(CLOSING_PUNCTUATION, comment[pos + 2]) != NULL)
          && strchr(CLOSING_PUNCTUATION, comment[pos + 1]) != NULL) {
        // Don't break space before closing punctuation
        commentTemplate[pos] = ASCII_4;

      // } else if (comment[pos + 2] == ' '
      //     && strchr(CLOSING_PUNCTUATION, comment[pos + 1]) != NULL) {
      //   // Don't break space after "~ <label>" if followed by punctuation
      //   commentTemplate[pos] = ASCII_4;
      // } else if (comment[pos - 2] == ' '
      //     && strchr(OPENING_PUNCTUATION, comment[pos - 1]) != NULL) {
      //   // Don't break space before "~ <label>" if preceded by punctuation
      //   commentTemplate[pos] = ASCII_4;
      // } else if (comment[pos + 1] == '`'
      //     && strchr(OPENING_PUNCTUATION, comment[pos - 1]) != NULL) {
      //   // Don't break space between punctuation and math start '`'
      //   commentTemplate[pos] = ASCII_4;
      // } else if (comment[pos - 1] == '`'
      //     && strchr(CLOSING_PUNCTUATION, comment[pos + 1]) != NULL) {
      //   // Don't break space between punctuation and math end '`'
      //   commentTemplate[pos] = ASCII_4;

      } else if (comment[pos - 3] == ' ' && comment[pos - 2] == 'p'
          && comment[pos - 1] == '.') {
        // Don't break " p. nnn"
        commentTemplate[pos] = ASCII_4;
      }
    }
  }
  commentTemplate[length - 3] = ASCII_4; // Last space in comment

  for (pos = 3; pos < length - 2; pos++) {
    // Transfer the non-breaking spaces from the template to the comment
    if (commentTemplate[pos] == ASCII_4) comment[pos] = ASCII_4;
  }

  free_vstring(commentTemplate);
  return comment;
} // rewrapComment

// This is a general-purpose function to parse a math token string,
// typically input by the user at the keyboard.  The statemNum is
// needed to provide a context to determine which symbols are active.
// Lack of whitespace is tolerated according to standard rules.
// g_mathTokens must be set to the proper value.
// The code in this section is complex because it uses the fast parsing
// method borrowed from parseStatements().  On the other hand, it must set
// up some initial tables by scanning the entire g_MathToken array; this may
// slow it down in some applications.
// Warning:  g_mathTokens must be the correct value (some procedures might
// artificially adjust g_mathTokens to add dummy tokens [schemes] to the
// g_MathToken array).
// The user text may include existing or new dummy variables of the
// form "?nnn".
nmbrString *parseMathTokens(vstring userText, long statemNum)
{
  long i, j;
  char *fbPtr;
  long mathStringLen;
  long tokenNum;
  long lowerKey, upperKey;
  long symbolLen, origSymbolLen, g_mathKeyNum;
  void *g_mathKeyPtr; // bsearch returned value
  int maxScope;
  flag errorFlag = 0; // Prevents bad token from being added to output
  int errCount = 0; // Cumulative error count
  vstring_def(tmpStr);
  vstring_def(nlUserText);

  long *mathTokenSameAs; // Flag that symbol is unique (for speed up)
  long *reverseMathKey; // Map from g_mathTokens to g_mathKey

  // Temporary working space
  long wrkLen;
  nmbrString *wrkNmbrPtr;
  char *wrkStrPtr;

  // The answer
  nmbrString_def(mathString);

  long maxSymbolLen; // Longest math symbol (for speedup)
  flag *symbolLenExists; // A symbol with this length exists (for speedup)

  long nmbrSaveTempAllocStack; // For nmbrLet() stack cleanup
  long saveTempAllocStack; // For let() stack cleanup
  nmbrSaveTempAllocStack = g_nmbrStartTempAllocStack;
  g_nmbrStartTempAllocStack = g_nmbrTempAllocStackTop;
  saveTempAllocStack = g_startTempAllocStack;
  g_startTempAllocStack = g_tempAllocStackTop;

  // Initialization to avoid compiler warning (should not be theoretically necessary)
  tokenNum = 0;

  // Add a newline before user text for sourceError()
  let(&nlUserText, cat("\n", userText, NULL));

  // Make sure that g_mathTokens has been initialized
  if (!g_mathTokens) bug(1717);

  // Set the 'active' flag based on statemNum; here 'active' just means it
  // would be in the stack during normal parsing, not the Metamath manual
  // definition.
  for (i = 0; i < g_mathTokens; i++) {
    if (g_MathToken[i].statement <= statemNum && g_MathToken[i].endStatement >=
        statemNum) {
      g_MathToken[i].active = 1;
    } else {
      g_MathToken[i].active = 0;
    }
  }

  // Initialize flags for g_mathKey array that identify math symbols as
  // unique (when 0) or, if not unique, the flag is a number identifying a group
  // of identical names.
  mathTokenSameAs = malloc((size_t)g_mathTokens * sizeof(long));
  if (!mathTokenSameAs) outOfMemory("#12 (mathTokenSameAs)");
  reverseMathKey = malloc((size_t)g_mathTokens * sizeof(long));
  if (!reverseMathKey) outOfMemory("#13 (reverseMathKey)");
  for (i = 0; i < g_mathTokens; i++) {
    mathTokenSameAs[i] = 0; // 0 means unique
    reverseMathKey[g_mathKey[i]] = i; // Initialize reverse map to g_mathKey
  }
  for (i = 1; i < g_mathTokens; i++) {
    if (!strcmp(g_MathToken[g_mathKey[i]].tokenName,
        g_MathToken[g_mathKey[i - 1]].tokenName)) {
      if (!mathTokenSameAs[i - 1]) mathTokenSameAs[i - 1] = i;
      mathTokenSameAs[i] = mathTokenSameAs[i - 1];
    }
  }

  // Initialize temporary working space for parsing tokens
  // Assume the worst case of one token per userText character
  wrkLen = (long)strlen(userText);
  wrkNmbrPtr = malloc((size_t)wrkLen * sizeof(nmbrString));
  if (!wrkNmbrPtr) outOfMemory("#22 (wrkNmbrPtr)");
  wrkStrPtr = malloc((size_t)wrkLen + 1);
  if (!wrkStrPtr) outOfMemory("#23 (wrkStrPtr)");

  // Find declared math symbol lengths (used to speed up parsing)
  maxSymbolLen = 0;
  for (i = 0; i < g_mathTokens; i++) {
    if (g_MathToken[i].length > maxSymbolLen) {
      maxSymbolLen = g_MathToken[i].length;
    }
  }
  symbolLenExists = malloc(((size_t)maxSymbolLen + 1) * sizeof(flag));
  if (!symbolLenExists) outOfMemory("#25 (symbolLenExists)");
  for (i = 0; i <= maxSymbolLen; i++) {
    symbolLenExists[i] = 0;
  }
  for (i = 0; i < g_mathTokens; i++) {
    symbolLenExists[g_MathToken[i].length] = 1;
  }

  g_currentScope = g_Statement[statemNum].scope; // Scope of the ref. statement

  // The code below is indented because it was borrowed from parseStatements().
  // We will leave the indentation intact for easier future comparison
  // with that code.

        // Scan the math section for tokens
        mathStringLen = 0;
        fbPtr = nlUserText;
        while (1) {
          fbPtr = fbPtr + whiteSpaceLen(fbPtr);
          origSymbolLen = tokenLen(fbPtr);
          if (!origSymbolLen) break; // Done scanning source line

         // Scan for largest matching token from the left
         nextAdjToken:
          // maxSymbolLen is the longest declared symbol
          if (origSymbolLen > maxSymbolLen) {
            symbolLen = maxSymbolLen;
          } else {
            symbolLen = origSymbolLen;
          }
          memcpy(wrkStrPtr, fbPtr, (size_t)symbolLen);
          for (; symbolLen > 0; symbolLen--) {
            // symbolLenExists means a symbol of this length was declared
            if (!symbolLenExists[symbolLen]) continue;
            wrkStrPtr[symbolLen] = 0; // Define end of trial token to look up
            g_mathKeyPtr = (void *)bsearch(wrkStrPtr, g_mathKey,
                (size_t)g_mathTokens, sizeof(long), mathSrchCmp);
            if (!g_mathKeyPtr) continue; // Trial token was not declared
            g_mathKeyNum = (long *)g_mathKeyPtr - g_mathKey; // Pointer arithmetic!
            if (mathTokenSameAs[g_mathKeyNum]) { // Multiply-declared symbol
              lowerKey = g_mathKeyNum;
              upperKey = lowerKey;
              j = mathTokenSameAs[lowerKey];
              while (lowerKey) {
                if (j != mathTokenSameAs[lowerKey - 1]) break;
                lowerKey--;
              }
              while (upperKey < g_mathTokens - 1) {
                if (j != mathTokenSameAs[upperKey + 1]) break;
                upperKey++;
              }
              // Find the active symbol with the most recent declaration.
              // (Note:  Here, 'active' means it's on the stack, not the
              // official def.)
              maxScope = -1;
              for (i = lowerKey; i <= upperKey; i++) {
                j = g_mathKey[i];
                if (g_MathToken[j].active) {
                  if (g_MathToken[j].scope > maxScope) {
                    tokenNum = j;
                    maxScope = g_MathToken[j].scope;
                    if (maxScope == g_currentScope) break; // Speedup
                  }
                }
              }
              if (maxScope == -1) {
                tokenNum = g_mathKey[g_mathKeyNum]; // Pick an arbitrary one
                errCount++;
                if (errCount <= 1) { // Print 1st error only
                  sourceError(fbPtr, symbolLen, /* stmt */ 0,
       "This math symbol is not active (i.e. was not declared in this scope).");
                }
                errorFlag = 1;
              }
            } else { // The symbol was declared only once.
              tokenNum = *((long *)g_mathKeyPtr);
                  // Same as: tokenNum = g_mathKey[g_mathKeyNum]; but faster
              if (!g_MathToken[tokenNum].active) {
                errCount++;
                if (errCount <= 1) { // Print 1st error only
                  sourceError(fbPtr, symbolLen, /* stmt */ 0,
       "This math symbol is not active (i.e. was not declared in this scope).");
                }
                errorFlag = 1;
              }
            } // End if multiply-defined symbol
            break; // The symbol was found, so we are done
          } // Next symbolLen

          if (symbolLen == 0) { // Symbol was not found
            // See if the symbol is a dummy variable name
            if (fbPtr[0] == '$') {
              symbolLen = tokenLen(fbPtr);
              for (i = 1; i < symbolLen; i++) {
                if (fbPtr[i] < '0' || fbPtr[i] > '9') break;
              }
              symbolLen = i;
              if (symbolLen == 1) {
                symbolLen = 0; // No # after '$' -- error
              } else {
                memcpy(wrkStrPtr, fbPtr + 1, (size_t)i - 1);
                wrkStrPtr[i - 1] = 0; // End of string
                tokenNum = (long)(val(wrkStrPtr)) + g_mathTokens;
                // See if dummy var has been declared; if not, declare it
                if (tokenNum > g_pipDummyVars + g_mathTokens) {
                  declareDummyVars(tokenNum - g_pipDummyVars - g_mathTokens);
                }
              }
            } // End if fbPtr == '$'
         } // End if symbolLen == 0

          if (symbolLen == 0) { // Symbol was not found
            symbolLen = tokenLen(fbPtr);
            errCount++;
            if (errCount <= 1) { // Print 1st error only
              sourceError(fbPtr, symbolLen, /* stmt */ 0,
      "This math symbol was not declared (with a \"$c\" or \"$v\" statement).");
            }
            errorFlag = 1;
          }

          // Add symbol to mathString
          if (!errorFlag) {
            wrkNmbrPtr[mathStringLen] = tokenNum;
            mathStringLen++;
          } else {
            errorFlag = 0;
          }
          fbPtr = fbPtr + symbolLen; // Move on to next symbol

          if (symbolLen < origSymbolLen) {
            // This symbol is not separated from next by white space.
            // Speed-up: don't call tokenLen again; just jump past it.
            origSymbolLen = origSymbolLen - symbolLen;
            goto nextAdjToken; // (Instead of continue)
          }
        } // End while

        // Assign mathString
        nmbrLet(&mathString, nmbrSpace(mathStringLen));
        for (i = 0; i < mathStringLen; i++) {
          mathString[i] = wrkNmbrPtr[i];
        }

  // End of unconventionally indented section borrowed from parseStatements()

  g_startTempAllocStack = saveTempAllocStack;
  g_nmbrStartTempAllocStack = nmbrSaveTempAllocStack;

  // Deallocate temporary space
  free(mathTokenSameAs);
  free(reverseMathKey);
  free(wrkNmbrPtr);
  free(wrkStrPtr);
  free(symbolLenExists);
  free_vstring(tmpStr);
  free_vstring(nlUserText);

  return nmbrMakeTempAlloc(mathString); // Flag for dealloc
} // parseMathTokens

// For .mm file splitting.
// Get the next real $[...$] or virtual $( Begin $[... inclusion.
// This uses the convention of mmvstr.c where beginning of a string
// is position 1.  However, startOffset = 0 means no offset i.e.
// start at fileBuf.
void getNextInclusion(char *fileBuf, long startOffset, // inputs
    // outputs:
    long *cmdPos1, long *cmdPos2,
    long *endPos1, long *endPos2,
    char *cmdType, // 'B' = "$( Begin [$..." through "$( End [$...",
                   // 'I' = "[$...",
                   // 'S' = "$( Skip [$...",
                   // 'E' = Start missing matched End
                   // 'N' = no include found; includeFn = ""
    vstring *fileName // name of included file
    )
{

// cmdType = 'B' or 'E':
//       ....... $( Begin $[ prop.mm $] $)   ......   $( End $[ prop.mm $] $) ...
//        ^      ^           ^^^^^^^       ^          ^                      ^
//   startOffset cmdPos1    fileName  cmdPos2     endPos1              endPos2
//                                              (=0 if no End)   (=0 if no End)
//
//    Note: in the special case of Begin, cmdPos2 points _after_ the whitespace
//    after "$( Begin $[ prop.mm $] $)" i.e. the whitespace is considered part of
//    the Begin command.  The is needed because prop.mm content doesn't
//    necessarily start with whitespace.  prop.mm does, however, end with
//    whitespace (\n) as enforced by readFileToString().
//
// cmdType = 'I':
//       ............... $[ prop.mm $]  ..............
//        ^              ^  ^^^^^^^   ^
//   startOffset   cmdPos1 fileName  cmdPos2     endPos1=0  endPos2=0
//
// cmdType = 'S':
//       ....... $( Skip $[ prop.mm $] $)   ......
//        ^      ^          ^^^^^^^      ^
//   startOffset cmdPos1    fileName  cmdPos2     endPos1=0  endPos2=0

  char *fbPtr;
  char *tmpPtr;
  flag lookForEndMode = 0; // 1 if inside of $( Begin, End, Skip...
  long i, j;

  fbPtr = fileBuf + startOffset;

  while (1) {
    fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr); // Count $( as a token
    j = rawTokenLen(fbPtr); // Treat $(, $[ as tokens
    if (j == 0) {
      *cmdType = 'N'; // No include found
      break; // End of file
    }
    if (fbPtr[0] != '$') {
      fbPtr = fbPtr + j;
      continue;
    }

    // Process normal include $[ $]
    if (fbPtr[1] == '[') {
      if (lookForEndMode == 0) {
        // If lookForEndMode is 1, ignore everything until we find a matching
        // "$( End $[..."
        *cmdPos1 = fbPtr - fileBuf + 1; // 1 = beginning of file
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + whiteSpaceLen(fbPtr); // Comments = whitespace here
        j = rawTokenLen(fbPtr); // Should be file name
        // Note that mid, seg, left, right do not waste time computing
        // the length of the input string fbPtr.
        let(&(*fileName), left(fbPtr, j));
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + whiteSpaceLen(fbPtr); // Comments = whitespace here
        j = rawTokenLen(fbPtr);
        if (j == 2 /* speedup */ && !strncmp("$]", fbPtr, (size_t)j)) {
          *cmdPos2 = fbPtr - fileBuf + j + 1;
          *endPos1 = 0;
          *endPos2 = 0;
          *cmdType = 'I';
          return;
        }
        // TODO - more precise error message
        print2("?Missing \"$]\" after \"$[ %s\"\n", *fileName);
        fbPtr = fbPtr + j;
        continue; // Not a completed include
      } // if (lookForEndMode == 0)
      fbPtr = fbPtr + j;
      // Either not a legal include - error detected later, or we're in lookForEndMode
      continue;
    // Process markup-type include inside comment
    } else if (fbPtr[1] == '(') {
      // Process comment starting at "$("
      if (lookForEndMode == 0) {
        *cmdPos1 = fbPtr - fileBuf + 1;
      } else {
        *endPos1 = fbPtr - fileBuf + 1;
      }
      fbPtr = fbPtr + j;
      fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr); // comment != whitespace
      j = rawTokenLen(fbPtr);
      *cmdType = '?';
      if (j == 5 /* speedup */ && !strncmp("Begin", fbPtr, (size_t)j)) {
        // If lookForEndMode is 1, we're looking for End matching earlier Begin
        if (lookForEndMode == 0) {
          *cmdType = 'B';
        }
      } else if (j == 4 /* speedup */ && !strncmp("Skip", fbPtr, (size_t)j)) {
        // If lookForEndMode is 1, we're looking for End matching earlier Begin
        if (lookForEndMode == 0) {
          *cmdType = 'S';
        }
      } else if (j == 3 /* speedup */ && !strncmp("End", fbPtr, (size_t)j)) {
        // If lookForEndMode is 0, there was no matching Begin
        if (lookForEndMode == 1) {
          *cmdType = 'E';
        }
      }
      if (*cmdType == '?') { // The comment doesn't qualify as $[ $] markup
        // Find end of comment and continue
        goto GET_PASSED_END_OF_COMMENT;
      } else {
        // It's Begin or Skip or End
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr);
        j = rawTokenLen(fbPtr);
        if (j != 2 || strncmp("$[", fbPtr, (size_t)j)) {
          // Find end of comment and continue
          goto GET_PASSED_END_OF_COMMENT;
        }
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr); // comment != whitespace
        j = rawTokenLen(fbPtr);
        // Note that mid, seg, left, right do not waste time computing
        // the length of the input string fbPtr.
        if (lookForEndMode == 0) {
          // It's Begin or Skip
          let(&(*fileName), left(fbPtr, j));
        } else {
          // It's an End command
          if (strncmp(*fileName, fbPtr, (size_t)j)) {
            // But the file name didn't match, so it's not a matching End.
            // Find end of comment and continue.
            goto GET_PASSED_END_OF_COMMENT;
          }
        }
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr); // comment != whitespace
        j = rawTokenLen(fbPtr);
        if (j != 2 || strncmp("$]", fbPtr, (size_t)j)) {
          // The token after the file name isn't "$]".
          // Find end of comment and continue.
          goto GET_PASSED_END_OF_COMMENT;
        }
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr); // comment != whitespace
        j = rawTokenLen(fbPtr);
        if (j != 2 || strncmp("$)", fbPtr, (size_t)j)) {
          // The token after the "$]" isn't "$)".
          // Find end of comment and continue.
          goto GET_PASSED_END_OF_COMMENT;
        }
        // We are now at the end of "$( Begin/Skip/End $[ file $] $)"
        fbPtr = fbPtr + j;
        if (lookForEndMode == 0) {
          *cmdPos2 = fbPtr - fileBuf + 1
            + ((*cmdType == 'B') ? 1 : 0); // after whitespace for 'B' (see above)
          if (*cmdType == 'S') { // Skip command; we're done
            *endPos1 = 0;
            *endPos2 = 0;
            return;
          }
          if (*cmdType != 'B') bug(1742);
          lookForEndMode = 1;
        } else { // We're at an End
          if (*cmdType != 'E') bug(1743);
          // lookForEndMode = 0; // Not needed since we will return soon
          *cmdType = 'B'; // Restore it to B for Begin/End pair
          *endPos2 = fbPtr - fileBuf + 1;
          return;
        }
        continue; // We're past Begin; start search for End
      } // Begin, End, or Skip
    } else if (i != i + 1) { // Suppress "unreachable code" warning for bug trap below
      // It's '$' not followed by '[' or '('; j is token length
      fbPtr = fbPtr + j;
      continue;
    }
    bug(1746); // Should never get here
   GET_PASSED_END_OF_COMMENT:
    // Note that fbPtr should be at beginning of last token found, which
    // may be "$)" (in which case i will be 1 from the instr).

    // Don't use instr because it computes string length each call.
    // i = instr(1, fbPtr, "$)"); // Normally this will be fast because we only
    // have to find the end of the comment that we're in.
    // Emulate the instr()
    tmpPtr = fbPtr;
    i = 0;
    while (1) {

      // strchr is incredibly slow under lcc - why?
      // Is it computing strlen internally maybe?

      // tmpPtr = strchr(tmpPtr, '$');
      // if (tmpPtr == NULL) {
      //   i = 0;
      //   break;
      // }
      // if (tmpPtr[1] == ')') {
      //   i = tmpPtr - fbPtr + 1;
      //   break;
      // }
      // tmpPtr++;

      // Emulate strchr
      while (tmpPtr[0] != '$') {
        if (tmpPtr[0] == 0) break;
        tmpPtr++;
      }
      if (tmpPtr[0] == 0) {
        i = 0;
        break;
      }
      if (tmpPtr[1] == ')') {
        i = tmpPtr - fbPtr + 1;
        break;
      }
      tmpPtr++;
    } // while (1)

    if (i == 0) {
      // TODO: better error msg
      printf("?End of comment not found\n");
      i = (long)strlen(fileBuf); // Slow, but this is a rare error
      fbPtr = fileBuf + i; // Points to null at end of fileBuf
    } else {
      // Skip the "$)" - skip 2 characters, then back up 1 since 
      // the instr result starts at 1.
      fbPtr = fbPtr + i + 2 - 1;
    }
    // continue; // Not necessary since we're at end of loop
  } // while (1)
  if (j != 0) bug(1744); // Should be at end of file
  if (lookForEndMode == 1) {
    // We didn't find an End
    *cmdType = 'E';
    *endPos1 = 0; *endPos2 = 0;
  } else {
    *cmdType = 'N'; // no include was found
    *cmdPos1 = 0; *cmdPos2 = 0; *endPos1 = 0; *endPos2 = 0;
    free_vstring(*fileName);
  }
  return;
} // getNextInclusion

// This function transfers the content of the g_Statement[] array
// to a linear buffer in preparation for creating the output file.
// Any changes such as modified proofs will be updated in the buffer.
// The caller is responsible for deallocating the returned string.
vstring writeSourceToBuffer(void) {
  long stmt, size;
  vstring_def(buf);
  char *ptr;

  // Compute the size of the buffer.
  // Note that g_Statement[g_statements + 1] is a dummy statement
  // containing the text after the last statement in its
  // labelSection.
  size = 0;
  for (stmt = 1; stmt <= g_statements + 1; stmt++) {
    // Add the sizes of the sections.  When sections don't exist
    // (like a proof for a $a statement), the section length is 0.
    size += g_Statement[stmt].labelSectionLen
        + g_Statement[stmt].mathSectionLen
        + g_Statement[stmt].proofSectionLen;
    // Add in the 2-char length of keywords, which aren't stored in
    // the statement sections.
    switch (g_Statement[stmt].type) {
      case lb_: // ${
      case rb_: // $}
        size += 2;
        break;
      case v_: // $v
      case c_: // $c
      case d_: // $d
      case e_: // $e
      case f_: // $f
      case a_: // $a
        size += 4;
        break;
      case p_: // $p
        size += 6;
        break;
      case illegal_: // dummy
        if (stmt != g_statements + 1) bug(1747);
        // The labelLen is text after last statement
        size += 0; // There are no keywords in g_statements + 1
        break;
      default: bug(1748);
    } // switch (g_Statement[stmt].type)
  } // next stmt

  // Create the output buffer.
  // We could have created it with let(&buf, space(size)), but malloc should
  // be slightly faster since we don't have to initialize each entry.
  buf = malloc((size_t)(size + 1) * sizeof(char));

  ptr = buf; // Pointer to keep track of buf location
  // Transfer the g_Statement[] array to buf
  for (stmt = 1; stmt <= g_statements + 1; stmt++) {
    // Always transfer the label section (text before $ keyword)
    memcpy(ptr /* dest */, g_Statement[stmt].labelSectionPtr, // source
        (size_t)(g_Statement[stmt].labelSectionLen) ); // size
    ptr += g_Statement[stmt].labelSectionLen;
    switch (g_Statement[stmt].type) {
      case illegal_:
        if (stmt != g_statements + 1) bug(1749);
        break;
      case lb_: // ${
      case rb_: // $}
        ptr[0] = '$';
        ptr[1] = g_Statement[stmt].type;
        ptr += 2;
        break;
      case v_: // $v
      case c_: // $c
      case d_: // $d
      case e_: // $e
      case f_: // $f
      case a_: // $a
        ptr[0] = '$';
        ptr[1] = g_Statement[stmt].type;
        ptr += 2;
        memcpy(ptr /* dest */, g_Statement[stmt].mathSectionPtr, // source
            (size_t)(g_Statement[stmt].mathSectionLen)); // size
        ptr += g_Statement[stmt].mathSectionLen;
        ptr[0] = '$';
        ptr[1] = '.';
        ptr += 2;
        break;
      case p_: // $p
        ptr[0] = '$';
        ptr[1] = g_Statement[stmt].type;
        ptr += 2;
        memcpy(ptr /* dest */, g_Statement[stmt].mathSectionPtr, // source
            (size_t)(g_Statement[stmt].mathSectionLen)); // size
        ptr += g_Statement[stmt].mathSectionLen;
        ptr[0] = '$';
        ptr[1] = '=';
        ptr += 2;
        memcpy(ptr /* dest */, g_Statement[stmt].proofSectionPtr, // source
            (size_t)(g_Statement[stmt].proofSectionLen)); // size
        ptr += g_Statement[stmt].proofSectionLen;
        ptr[0] = '$';
        ptr[1] = '.';
        ptr += 2;
        break;
      default: bug(1750);
    } // switch (g_Statement[stmt].type)
  } // next stmt
  if (ptr - buf != size) bug(1751);
  buf[size] = 0; // End of string marker
  return buf;
} // writeSourceToBuffer

// This function creates split files containing $[ $] inclusions, from
// an unsplit source with $( Begin $[... etc. inclusions.
// This function calls itself recursively, and after the recursive call
// the fileBuf (=includeBuf) argument is deallocated.
// For the top level call, fileName MUST NOT HAVE A DIRECTORY PATH.
// Note that fileBuf must be deallocated by initial caller. This will let the
// caller decide whether to say re-use fileBuf to create an unsplit version
// of the .mm file in case the split version generation encounters an error.
//                                     TODO ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
/* flag(TODO) */ void writeSplitSource(vstring *fileBuf, const char *fileName,
    flag noVersioningFlag, flag noDeleteFlag) {
  FILE *fp;
  vstring_def(tmpStr1);
  vstring_def(tmpFileName);
  vstring_def(includeBuf);
  vstring_def(includeFn);
  vstring_def(fileNameWithPath);
  long size;
  flag writeFlag;
  long startOffset;
  long cmdPos1;
  long cmdPos2;
  long endPos1;
  long endPos2;
  char cmdType;
  startOffset = 0;
  let(&fileNameWithPath, cat(g_rootDirectory, fileName, NULL));
  while (1) {
    getNextInclusion(*fileBuf, startOffset, // inputs
        // outputs:
        &cmdPos1, &cmdPos2,
        &endPos1, &endPos2,
        &cmdType, // 'B' = "$( Begin [$..." through "$( End [$...",
                  // 'I' = "[$...",
                  // 'S' = "$( Skip [$...",
                  // 'E' = Start missing matched End
                  // 'N' = no include found; includeFn = ""
        &includeFn); // name of included file
    if (cmdType == 'N') {
      writeFlag = 0;
      // There are no more includes to expand, so write out the file
      if (!strcmp(fileName, g_output_fn)) {
        // We're writing the top-level file - always create new version
        writeFlag = 1;
      } else {
        // We're writing an included file.
        // See if the file already exists.
        free_vstring(tmpStr1);
        tmpStr1 = readFileToString(fileNameWithPath, 0 /* quiet */, &size);
        if (tmpStr1 == NULL) {
          tmpStr1 = ""; // Prevent seg fault
          // If it doesn't exist, see if the ~1 version exists
          let(&tmpFileName, cat(fileNameWithPath, "~1", NULL));
          tmpStr1 = readFileToString(tmpFileName, 0 /* quiet */, &size);
          if (tmpStr1 == NULL) {
            tmpStr1 = ""; // Prevent seg fault
            // Create and write the file
            writeFlag = 1;
          } else {
            // See if the ~1 backup file content changed
            if (strcmp(tmpStr1, *fileBuf)) {
              // Content is different; write the file
              writeFlag = 1;
            } else {
              // Just rename the ~1 version to the main version
              print2("Recovering \"%s\" from \"%s\"...\n",
                  fileNameWithPath, tmpFileName);
              rename(tmpFileName /* old */, fileNameWithPath /* new */);
            }
          } // if (tmpStr1 == NULL)
        } else { // tmpStr1 != NULL
          // The include file already exists; see if the content changed
          if (strcmp(tmpStr1, *fileBuf)) {
            // Content is different; write the file
            writeFlag = 1;
          } else {
            // Just rename the ~1 version to the main version
            print2("Content of \"%s\" did not change.\n",
                fileNameWithPath);
            rename(tmpFileName /* old */, fileNameWithPath /* new */);
          }
        }
      }
      if (writeFlag == 1) {
        fp = fSafeOpen(fileNameWithPath, "w", 0 /* noVersioningFlag */);
        if (fp == NULL) {
          // TODO: better error msg?  Abort and don't split?
          print2("?Error: couldn't create the file \"%s\"\n", fileNameWithPath);
          print2("  Make sure any directories needed have been created.\n");
          print2("  Try WRITE SOURCE without / SPLIT to recover your work.\n");
          break;
        } else {
          print2("Writing \"%s\"...\n", fileNameWithPath);
          fprintf(fp, "%s", *fileBuf);
          fclose(fp);
          break;
        }
      } // if (writeFlag == 1 )
      break;
    } else if (cmdType == 'S') {
      // Change "Skip" to a real inclusion
      let(&tmpStr1, cat("$[ ", includeFn, " $]", NULL));
      startOffset = cmdPos1 - 1 + (long)strlen(tmpStr1);
      let(&(*fileBuf), cat(left(*fileBuf, cmdPos1 - 1), tmpStr1,
          right(*fileBuf, cmdPos2), NULL));
      continue;
    } else if (cmdType == 'B') {
      // Extract included file content and call this recursively to process
      let(&tmpStr1, cat("$[ ", includeFn, " $]", NULL));
      startOffset = cmdPos1 - 1 + (long)strlen(tmpStr1);
      // We start _after_ the whitespace after cmdPos2 because it wasn't
      // in the original included file but was put there by us in
      // readSourceAndIncludes().
      let(&includeBuf, seg(*fileBuf, cmdPos2, endPos1 - 1));
      let(&(*fileBuf), cat(left(*fileBuf, cmdPos1 - 1), tmpStr1,
          right(*fileBuf, endPos2), NULL));
      // TODO: intercept error from deeper calls?
      writeSplitSource(&includeBuf, includeFn, noVersioningFlag, noDeleteFlag);
      continue;
    } else if (cmdType == 'I') {
      bug(1752); // Any real $[ $] should have been converted to comment.
      // However in theory, user could have faked an assignable description
      // if modifiable comments are added in the future...
      startOffset = cmdPos2 - 1;
      continue;
    } else if (cmdType == 'E') {
      // TODO What error message should go here?
      print2("?Unterminated \"$( Begin $[...\" inclusion markup in \"%s\".",
          fileNameWithPath);
      startOffset = cmdPos2 - 1;
      continue;
    } else {
      // Should never happen
      bug(1753);
    }
  } // while (1)
  // Deallocate memory
  // free_vstring(*fileBuf); // Let caller decide whether to do this
  free_vstring(tmpStr1);
  free_vstring(tmpFileName);
  free_vstring(includeFn);
  free_vstring(includeBuf);
  free_vstring(fileNameWithPath);
} // writeSplitSource

// When "write source" does not have the "/split" qualifier, by default
// (i.e. without "/no_delete") the included modules are "deleted" (renamed
// to ~1) since their content will be in the main output file.
/* flag(TODO) */ void deleteSplits(vstring *fileBuf, flag noVersioningFlag) {
  FILE *fp;
  vstring_def(includeFn);
  vstring_def(fileNameWithPath);
  long startOffset;
  long cmdPos1;
  long cmdPos2;
  long endPos1;
  long endPos2;
  char cmdType;
  startOffset = 0;
  while (1) {
    // We scan the source for all "$( Begin $[ file $] $)...$( End $[ file $]"
    // and when found, we "delete" file.
    getNextInclusion(*fileBuf, startOffset, // inputs
        // outputs:
        &cmdPos1, &cmdPos2,
        &endPos1, &endPos2,
        &cmdType, // 'B' = "$( Begin [$..." through "$( End [$...",
                  // 'I' = "[$...",
                  // 'S' = "$( Skip [$...",
                  // 'E' = Start missing matched End
                  // 'N' = no include found; includeFn = ""
        &includeFn); // name of included file
    // We only care about the 'B' command
    if (cmdType == 'B') {
      let(&fileNameWithPath, cat(g_rootDirectory, includeFn, NULL));
      // See if the included file exists
      fp = fopen(fileNameWithPath, "r");
      if (fp != NULL) {
        fclose(fp);
        if (noVersioningFlag == 1) {
          print2("Deleting \"%s\"...\n", fileNameWithPath);
        } else {
          print2("Renaming \"%s\" to \"%s~1\"...\n", fileNameWithPath,
              fileNameWithPath);
        }
        fp = fSafeOpen(fileNameWithPath, "d", noVersioningFlag);
      }
      // Adjust offset and continue.
      // We don't go the the normal end of the 'B' because it may
      // have other 'B's inside.  Instead, just skip past the Begin.
      startOffset = cmdPos2 - 1; // don't use endPos2
    } else if (cmdType == 'N') {
      // We're done
      break;
    } else if (cmdType == 'S') {
      // Adjust offset and continue
      startOffset = cmdPos2 - 1;
    } else if (cmdType == 'E') {
      // There's a problem, but ignore - should have been reported earlier.
      // Adjust offset and continue.
      startOffset = cmdPos2 - 1;
    } else if (cmdType == 'I') {
      bug(1755); // Should never happen
    } else {
      bug(1756);
    }
    continue;
  } // while (1)
  // Deallocate memory
  // free_vstring(*fileBuf); // Let caller decide whether to do this
  free_vstring(includeFn);
  free_vstring(fileNameWithPath);
  return;
} // deleteSplits

// Get file name and line number given a pointer into the read buffer.
// The user must deallocate the returned string (file name).
// The global g_IncludeCall structure and g_includeCalls are used.
vstring getFileAndLineNum(const char *buffPtr, // start of read buffer
    const char *currentPtr, // place at which to get file name and line no
    long *lineNum) // return argument
{
  long i, smallestOffset, smallestNdx;
  vstring_def(fileName);

  // Make sure it's not outside the read buffer
  if (currentPtr < buffPtr
      || currentPtr >= buffPtr + g_IncludeCall[1].current_offset) {
    bug(1769);
  }

  // Find the g_IncludeCall that is closest to currentPtr but does not
  // exceed it.
  smallestOffset = currentPtr - buffPtr; // Start with g_IncludeCall[0]
  if (smallestOffset < 0) bug(1767);
  smallestNdx = 0; // Point to g_IncludeCall[0]
  for (i = 0; i <= g_includeCalls; i++) {
    if (g_IncludeCall[i].current_offset <= currentPtr - buffPtr) {
      if ((currentPtr - buffPtr) - g_IncludeCall[i].current_offset
          <= smallestOffset) {
        smallestOffset = (currentPtr - buffPtr) - g_IncludeCall[i].current_offset;
        smallestNdx = i;
      }
    }
  }
  if (smallestOffset < 0) bug(1768);
  *lineNum = g_IncludeCall[smallestNdx].current_line
      + countLines(buffPtr + g_IncludeCall[smallestNdx].current_offset,
          smallestOffset);
  // Assign to new string to prevent original from being deallocated
  let(&fileName, g_IncludeCall[smallestNdx].source_fn);
  // D // printf("smallestNdx=%ld smallestOffset=%ld i[].co=%ld\n",smallestNdx,smallestOffset,g_IncludeCall[smallestNdx].current_offset);
  return fileName;
} // getFileAndLineNo

// g_Statement[stmtNum].fileName and .lineNum are initialized to "" and 0.
// To save CPU time, they aren't normally assigned until needed, but once
// assigned they can be reused without looking them up again.  This function
// will assign them if they haven't been assigned yet. It just returns if
// they have been assigned.
// The globals g_Statement[] and g_sourcePtr are used
void assignStmtFileAndLineNum(long stmtNum) {
  if (g_Statement[stmtNum].lineNum > 0) return; // Already assigned
  if (g_Statement[stmtNum].lineNum < 0) bug(1766);
  if (g_Statement[stmtNum].fileName[0] != 0) bug(1770); // Should be empty string
  // We can make a direct string assignment here since previous value was ""
  g_Statement[stmtNum].fileName = getFileAndLineNum(g_sourcePtr,
      g_Statement[stmtNum].statementPtr, &(g_Statement[stmtNum].lineNum));
  return;
} // assignStmtFileAndLineNum

// This function returns a pointer to a buffer containing the contents of an
// input file and its 'include' calls.  'Size' returns the buffer's size.
// TODO - ability to flag error to skip raw source function.
// Recursive function that processes a found include.
// If NULL is returned, it means a serious error occurred (like missing file)
// and reading should be aborted.
// Globals used:  g_IncludeCall[], g_includeCalls
vstring readInclude(const char *fileBuf, long fileBufOffset,
    const char *sourceFileName, long *size, long parentLineNum, flag *errorFlag)
{
  long i;
  long inclSize;
  vstring_def(newFileBuf);
  vstring_def(inclPrefix);
  vstring_def(tmpSource);
  vstring_def(inclSource);
  vstring_def(oldSource);
  vstring_def(inclSuffix);

  long startOffset;
  long cmdPos1;
  long cmdPos2;
  long endPos1;
  long endPos2;
  char cmdType;
  long oldInclSize = 0; // Init to avoid compiler warning
  long newInclSize = 0; // Init to avoid compiler warning
  long befInclLineNum;
  long aftInclLineNum;
  vstring_def(includeFn);
  vstring_def(fullInputFn);
  vstring_def(fullIncludeFn);
  long alreadyInclBy;
  long saveInclCalls;

  let(&newFileBuf, fileBuf); // TODO - can this be avoided for speedup?
  // Look for and process includes
  startOffset = 0;

  while (1) {
    getNextInclusion(newFileBuf, startOffset, // inputs
        // outputs:
        &cmdPos1, &cmdPos2,
        &endPos1, &endPos2,
        //    'B' = "$( Begin [$..." through "$( End [$...",
        //    'I' = "[$...",
        //    'S' = "$( Skip [$...",
        // TODO: add error code for missing $]?
        //    'E' = Begin missing matched End
        //    'N' = no include found; includeFn = ""
        &cmdType,
        &includeFn); // name of included file

    // cmdType = 'B' or 'E':
    //       ....... $( Begin $[ prop.mm $] $)   ......   $( End $[ prop.mm $] $) ...
    //        ^      ^           ^^^^^^^       ^          ^                      ^
    //   startOffset cmdPos1    fileName  cmdPos2     endPos1              endPos2
    //                                              (=0 if no End)   (=0 if no End)
    //
    //    Note: in the special case of Begin, cmdPos2 points _after_ the whitespace
    //    after "$( Begin $[ prop.mm $] $)" i.e. the whitespace is considered part of
    //    the Begin command.  The is needed because prop.mm content doesn't
    //    necessarily start with whitespace.  prop.mm does, however, end with
    //    whitespace (\n) as enforced by readFileToString().
    //
    // cmdType = 'I':
    //       ............... $[ prop.mm $]  ..............
    //        ^              ^  ^^^^^^^   ^
    //   startOffset   cmdPos1 fileName  cmdPos2     endPos1=0  endPos2=0
    //
    // cmdType = 'S':
    //       ....... $( Skip $[ prop.mm $] $)   ......
    //        ^      ^          ^^^^^^^      ^
    //   startOffset cmdPos1    fileName  cmdPos2     endPos1=0  endPos2=0

    if (cmdType == 'N') break; // No (more) includes
    if (cmdType == 'E') {
      // TODO: Better error msg here or in getNextInclude
      print2("?Error: \"$( Begin $[...\" without matching \"$( End $[...\"\n");
      startOffset = cmdPos2; // Get passed the bad "$( Begin $[..."
      *errorFlag = 1;
      continue;
    }

    // Count lines between start of last source continuation and end of
    // the inclusion, before the newFileBuf is modified.

    if (g_IncludeCall[g_includeCalls].pushOrPop != 1) bug(1764);
    
    // contLineNum = g_IncludeCall[g_includeCalls].current_line
    //   + countLines(newFileBuf, ((cmdType == 'B') ? endPos2 : cmdPos2)
    //       - g_IncludeCall[g_includeCalls].current_offset);\

    // If we're here, cmdType is 'B', 'I', or 'S'

    // Create 2 new includeCall entries before recursive call, so that
    // alreadyInclBy will scan entries in proper order (e.g. if this
    // include calls itself at a deeper level - weird but not illegal - the
    // deeper one should be Skip'd per the Metamath spec).
    // This entry is identified by pushOrPop = 0
    g_includeCalls++;
    // We will use two more entries here (include call and return), and
    // in parseKeywords() a dummy additional top entry is assumed to exist.
    // Thus the comparison must be to 3 less than g_MAX_INCLUDECALLS.
    if (g_includeCalls >= g_MAX_INCLUDECALLS - 3) {
      g_MAX_INCLUDECALLS = g_MAX_INCLUDECALLS + 20;
/*E*/if(db5)print2("'Include' call table was increased to %ld entries.\n",
/*E*/    g_MAX_INCLUDECALLS);
      g_IncludeCall = realloc(g_IncludeCall, (size_t)g_MAX_INCLUDECALLS *
          sizeof(struct includeCall_struct));
      if (g_IncludeCall == NULL) outOfMemory("#2 (g_IncludeCall)");
    }
    g_IncludeCall[g_includeCalls].pushOrPop = 0;

    // This entry is identified by pushOrPop = 1
    g_includeCalls++;
    g_IncludeCall[g_includeCalls].pushOrPop = 1;
    // Save the value before recursive calls will increment the global
    // g_includeCalls.
    saveInclCalls = g_includeCalls;

    g_IncludeCall[saveInclCalls - 1].included_fn = ""; // Initialize string
    // Name of the file in the inclusion statement e.g. "$( Begin $[ included_fn..."
    let(&(g_IncludeCall[saveInclCalls - 1].included_fn), includeFn);
    g_IncludeCall[saveInclCalls].included_fn = "";
    let(&g_IncludeCall[saveInclCalls].included_fn,
        sourceFileName); // Continuation of parent file after this include

    // See if includeFn file has already been included
    alreadyInclBy = -1;
    for (i = 0; i <= saveInclCalls - 2; i++) {
      if (g_IncludeCall[i].pushOrPop == 0
         && !strcmp(g_IncludeCall[i].included_fn, includeFn)) {

        // print2("%s",cat(
        //     "(File \"",
        //     g_IncludeCall[g_includeCalls].source_fn,
        //     "\", referenced at line ",
        //     str((double)(g_IncludeCall[g_includeCalls].calledBy_line)),
        //     " in \"",
        //     g_IncludeCall[g_includeCalls].calledBy_fn,
        //     "\", has already been included.)\n",NULL));
        
        alreadyInclBy = i;
        break;
      }
    }
    if (alreadyInclBy == -1) {
      // This is the first time the included file has been included
      switch (cmdType) {
        case 'B':
          // Keep trailing \n (or other whitespace) as part of prefix for the special
          // case of Begin - cmdPos2 points to char after \n.
          let(&inclPrefix, seg(newFileBuf, cmdPos1, cmdPos2 - 1));
          let(&inclSuffix, seg(newFileBuf, endPos1, endPos2 - 1));
          // Save the included source
          let(&tmpSource, seg(newFileBuf, cmdPos2, endPos1 - 1));
          inclSize = endPos1 - cmdPos2; // Actual included source size

          // Get the parent line number up to the inclusion
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos2 - 1 - startOffset);
          g_IncludeCall[saveInclCalls - 1].current_line = befInclLineNum - 1;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos2, // start at (cmdPos2+1)th character
              endPos2 - cmdPos2 - 1) + 1;
          g_IncludeCall[saveInclCalls].current_line = aftInclLineNum - 1;
          parentLineNum = aftInclLineNum;

          // Call recursively to expand any includes in the included source.
          // Use parentLineNum since the inclusion source is in the parent file.
          free_vstring(inclSource);
          inclSource = readInclude(tmpSource,
              fileBufOffset + cmdPos1 - 1 + (long)strlen(inclPrefix), // new offset
              /* includeFn, */ sourceFileName,
              &inclSize /* input/output */, befInclLineNum, &(*errorFlag));

          oldInclSize = endPos2 - cmdPos1; // Includes old prefix and suffix
          // newInclSize = oldInclSize; // Includes new prefix and suffix
          newInclSize = (long)strlen(inclPrefix) + inclSize +
                (long)strlen(inclSuffix); // Includes new prefix and suffix
          // It is already a Begin comment, so leave it alone
          // *size = *size;
          // Adjust starting position for next inclusion search
          // -1 since startOffset is 0-based but cmdPos2 is 1-based
          startOffset = cmdPos1 + newInclSize - 1;
          break;
        case 'I':
          // Read the included file
          let(&fullIncludeFn, cat(g_rootDirectory, includeFn, NULL));
          free_vstring(tmpSource);
          tmpSource = readFileToString(fullIncludeFn, 0 /* verbose */, &inclSize);
          if (tmpSource == NULL) {
            // TODO: print better error msg?
            print2(
                "?Error: file \"%s%s\" (included in \"%s\") was not found\n",
                fullIncludeFn, g_rootDirectory, sourceFileName);
            tmpSource = "";
            inclSize = 0;
            *errorFlag = 1;
          } else {
            print2("Reading included file \"%s\"... %ld bytes\n",
                fullIncludeFn, inclSize);
          }

          // Change inclusion command to Begin...End comment.
          // Note that trailing whitespace is part of the prefix in
          // the special case of Begin, because the included file might
          // not start with whitespace. However, the included file
          // will always end with whitespace i.e. \n as enforced by
          // readFileToString().
          let(&inclPrefix, cat("$( Begin $[ ", includeFn, " $] $)\n", NULL));
               
          let(&inclSuffix, cat("$( End $[ ", includeFn, " $] $)", NULL));

          // Get the parent line number up to the inclusion.
          // TODO: compute aftInclLineNum directly and eliminate befInclLineNum.
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos1 - 1 - startOffset);
          g_IncludeCall[saveInclCalls - 1].current_line = 0;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos1, // start at (cmdPos1+1)th character
              cmdPos2 - cmdPos1 - 1);
          g_IncludeCall[saveInclCalls].current_line = aftInclLineNum;
          parentLineNum = aftInclLineNum;

          // Call recursively to expand includes in the included source.
          // Start at line 1 since source is in external file.
          free_vstring(inclSource);
          inclSource = readInclude(tmpSource,
              fileBufOffset + cmdPos1 - 1 + (long)strlen(inclPrefix), // new offset
              /* includeFn, */ includeFn,
              &inclSize /* input/output */, 1 /* parentLineNum */, &(*errorFlag));

          oldInclSize = cmdPos2 - cmdPos1; // Includes old prefix and suffix
          // "$( Begin $[...$] $)" must have a whitespace
          // after it; we use a newline.  readFileToString will ensure a
          // newline at its end.  We don't add whitespace after
          // "$( End $[...$] $)" but reuse the whitespace after the original
          // "$[...$]".
          let(&newFileBuf, cat(left(newFileBuf, cmdPos1 - 1),
              inclPrefix, inclSource, inclSuffix,
              right(newFileBuf, cmdPos2), NULL));
          *size = *size - (cmdPos2 - cmdPos1) + (long)strlen(inclPrefix)
              + inclSize + (long)strlen(inclSuffix);
          newInclSize = (long)strlen(inclPrefix) + inclSize +
                (long)strlen(inclSuffix); // Includes new prefix and suffix
          // Adjust starting position for next inclusion search (which will
          // be at the start of the included file continuing into the remaining
          // parent file).
          // -1 since startOffset is 0-based but cmdPos2 is 1-based
          startOffset = cmdPos1 + newInclSize - 1; 
              // + inclSize  // Use instead of strlen for speed
              // + (long)strlen(inclSuffix);
              // -1 since startOffset is 0-based but cmdPos1 is 1-based
          // TODO: update line numbers for error msgs.
          break;
        case 'S':
          // Read the included file
          let(&fullIncludeFn, cat(g_rootDirectory, includeFn, NULL));
          free_vstring(tmpSource);
          tmpSource = readFileToString(fullIncludeFn, 1 /* verbose */, &inclSize);
          if (tmpSource == NULL) {
            // TODO: print better error msg
            print2(
                "?Error: file \"%s%s\" (included in \"%s\") was not found\n",
                fullIncludeFn, g_rootDirectory, sourceFileName);
            *errorFlag = 1;
            tmpSource = ""; // Prevent seg fault
            inclSize = 0;
          }

          // Change Skip comment to Begin...End comment
          let(&inclPrefix, cat("$( Begin $[ ", includeFn, " $] $)\n", NULL));
          let(&inclSuffix, cat("$( End $[ ", includeFn, " $] $)", NULL));

          // Get the parent line number up to the inclusion
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
          // TODO: compute aftInclLineNum directly and eliminate befInclLineNum
              cmdPos1 - 1 - startOffset);
          g_IncludeCall[saveInclCalls - 1].current_line = 0;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos1, // start at (cmdPos1+1)th character
              cmdPos2 - cmdPos1 - 1);
          g_IncludeCall[saveInclCalls].current_line = aftInclLineNum;
          parentLineNum = aftInclLineNum;

          // Call recursively to expand includes in the included source.
          // Start at line 1 since source is in external file.
          free_vstring(inclSource);
          inclSource = readInclude(tmpSource,
              fileBufOffset + cmdPos1 - 1 + (long)strlen(inclPrefix), // new offset
              /* includeFn, */ includeFn,
              &inclSize /* input/output */, 1 /* parentLineNum */, &(*errorFlag));

          oldInclSize = cmdPos2 - cmdPos1; // Includes old prefix and suffix
          let(&newFileBuf, cat(left(newFileBuf, cmdPos1 - 1),
              inclPrefix, inclSource, inclSuffix,
              right(newFileBuf, cmdPos2), NULL));
          newInclSize = (long)strlen(inclPrefix) + inclSize +
                (long)strlen(inclSuffix); // Includes new prefix and suffix
          *size = *size - (cmdPos2 - cmdPos1) + (long)strlen(inclPrefix)
              + inclSize + (long)strlen(inclSuffix);
          // Adjust starting position for next inclusion search
          // -1 since startOffset is 0-based but cmdPos2 is 1-based
          startOffset = cmdPos1 + newInclSize - 1;
          // TODO: update line numbers for error msgs
          break;
        default:
          bug(1745);
      } // end switch (cmdType)
    } else {
      // This file has already been included. Change Begin and $[ $] to
      // Skip. alreadyInclBy is the index of the previous g_IncludeCall that
      // included it.
      if (!(alreadyInclBy > 0)) bug(1765);
      switch (cmdType) {
        case 'B':
          // Save the included source temporarily
          let(&inclSource,
              seg(newFileBuf, cmdPos2, endPos1 - 1));
          // Make sure it's content matches
          free_vstring(oldSource);
          oldSource = g_IncludeCall[ // Connect to source for brevity
                  alreadyInclBy
                  ].current_includeSource;
          if (strcmp(inclSource, oldSource)) {
            // TODO - print better error msg
            print2(
        "?Warning: \"$( Begin $[...\" source, with %ld characters, mismatches\n",
                (long)strlen(inclSource));
            print2(
                "  earlier inclusion, with %ld characters.\n",
                (long)strlen(oldSource));
          }
          oldSource = ""; // Disconnect from source
          // We need to delete it from the source and change to Skip
          let(&inclPrefix, cat("$( Skip $[ ", includeFn, " $] $)", NULL));
          let(&inclSuffix, "");

          // Get the parent line number up to the inclusion
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos2 - 1 - startOffset);
          g_IncludeCall[saveInclCalls - 1].current_line = befInclLineNum;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos2, // start at (cmdPos2+1)th character
              endPos2 - cmdPos2 /* - 1 */);
          g_IncludeCall[saveInclCalls].current_line = aftInclLineNum;
          parentLineNum = aftInclLineNum;

          let(&inclSource, ""); // Final source to be stored - none
          inclSize = 0; // Size of just the included source
          oldInclSize = endPos2 - cmdPos1; // Includes old prefix and suffix
          let(&newFileBuf, cat(left(newFileBuf, cmdPos1 - 1),
              inclPrefix,
              right(newFileBuf, endPos2), NULL));
          newInclSize = (long)strlen(inclPrefix); // Includes new prefix and suffix
          *size = *size - (endPos2 - cmdPos1) + newInclSize;
          // Adjust starting position for next inclusion search (which may
          // occur inside the source we just included, so don't skip passed
          // that source).
          // -1 since startOffset is 0-based but cmdPos2 is 1-based
          startOffset = cmdPos1 + newInclSize - 1; 
          break;
        case 'I':
          // Change inclusion command to Skip comment
          let(&inclPrefix, cat("$( Skip $[ ", includeFn, " $] $)", NULL));
          let(&inclSuffix, "");

          // Get the parent line number up to the inclusion
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos1 - 1 - startOffset);
          g_IncludeCall[saveInclCalls - 1].current_line = befInclLineNum;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos1, // start at (cmdPos1+1)th character
              cmdPos2 - cmdPos1 /* - 1 */);
          g_IncludeCall[saveInclCalls].current_line = aftInclLineNum;
          parentLineNum = aftInclLineNum;

          let(&inclSource, ""); // Final source to be stored - none
          inclSize = 0; // Size of just the included source
          oldInclSize = cmdPos2 - cmdPos1; // Includes old prefix and suffix
          let(&newFileBuf, cat(left(newFileBuf, cmdPos1 - 1),
              inclPrefix,
              right(newFileBuf, cmdPos2), NULL));
          newInclSize = (long)strlen(inclPrefix); // Includes new prefix and suffix
          *size = *size - (cmdPos2 - cmdPos1) + (long)strlen(inclPrefix);
          // Adjust starting position for next inclusion search
          // -1 since startOffset is 0-based but cmdPos2 is 1-based
          startOffset = cmdPos1 + newInclSize - 1;
          break;
        case 'S':
          // It is already Skipped, so leave it alone.
          // *size = *size;
          // Adjust starting position for next inclusion search
          let(&inclPrefix, seg(newFileBuf, cmdPos1, cmdPos2 - 1));
          let(&inclSuffix, "");

          // Get the parent line number up to the inclusion
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos1 - 1 - startOffset);
          g_IncludeCall[saveInclCalls - 1].current_line = befInclLineNum;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos1, // start at (cmdPos1+1)th character
              cmdPos2 - cmdPos1 /* - 1 */);
          g_IncludeCall[saveInclCalls].current_line = aftInclLineNum - 1;
          parentLineNum = aftInclLineNum;

          let(&inclSource, ""); // Final source to be stored - none
          inclSize = 0; // Size of just the included source
          oldInclSize = cmdPos2 - cmdPos1; // Includes old prefix and suffix
          newInclSize = oldInclSize; // Includes new prefix and suffix
          // -1 since startOffset is 0-based but cmdPos2 is 1-based
          startOffset = cmdPos1 + newInclSize - 1;
          if (startOffset != cmdPos2 - 1) bug(1772);
          break;
        default:
          bug(1745);
      } // end switch(cmdType)
    } // if alreadyInclBy == -1

    // Assign structure with information for subsequent passes and (later) error messages.
    // Name of the file where the inclusion source is located 
    // (= parent file for $( Begin $[... etc.)
    g_IncludeCall[saveInclCalls - 1].source_fn = "";
    g_IncludeCall[saveInclCalls - 1].current_offset = fileBufOffset + cmdPos1 - 1
        // This is the starting character position
        // of the included file w.r.t entire source buffer.
        + (long)strlen(inclPrefix) - 1;
    if (alreadyInclBy >= 0 || cmdType == 'B') {
      // No external file was included, so let the includeFn be the
      // same as the source.
      let(&g_IncludeCall[saveInclCalls - 1].source_fn, sourceFileName);
    } else {
      // It hasn't been included yet, and cmdType is 'I' or 'S', meaning
      // that we bring in an external file.
      // $[ $], Begin, or Skip file name for this inclusion
      let(&g_IncludeCall[saveInclCalls - 1].source_fn, includeFn);
      // g_IncludeCall[saveInclCalls - 1].current_line = 0; // The line number
      // of the start of the included file (=1)
    }
    // let() not needed because we're just assigning a new name to inclSource
    g_IncludeCall[saveInclCalls - 1].current_includeSource = inclSource;
    // Detach from g_IncludeCall[saveInclCalls - 1].current_includeSource for later reuse
    inclSource = "";
    let(&g_IncludeCall[saveInclCalls - 1].current_includeSource, "");
    // Length of the file to be included (0 if the file was previously included) 
    g_IncludeCall[saveInclCalls - 1].current_includeLength = inclSize;
    // Initialize a new include call for the continuation of the parent.
    // This entry is identified by pushOrPop = 1
    // Name of the file to be included.
    g_IncludeCall[saveInclCalls].source_fn = "";
    // Source file containing this inclusion
    let(&g_IncludeCall[saveInclCalls].source_fn,
        sourceFileName);
    // This is the position of the continuation of the parent
    g_IncludeCall[saveInclCalls].current_offset = fileBufOffset + cmdPos1
        + newInclSize - 1;
    // (Currently) assigned only if we may need it for a later Begin comparison
    g_IncludeCall[saveInclCalls].current_includeSource = "";
    // Length of the file to be included (0 if the file was previously included)
    g_IncludeCall[saveInclCalls].current_includeLength = 0;
  } // while (1)

  // Deallocate strings
  free_vstring(inclSource);
  free_vstring(tmpSource);
  free_vstring(oldSource);
  free_vstring(inclPrefix);
  free_vstring(inclSuffix);
  free_vstring(includeFn);
  free_vstring(fullInputFn);
  free_vstring(fullIncludeFn);

  return newFileBuf;
} // readInclude

// This function returns a pointer to a buffer containing the contents of an
// input file and its 'include' calls.  'Size' returns the buffer's size.
// TODO - ability to flag error to skip raw source function.
// If NULL is returned, it means a serious error occurred (like missing file)
// and reading should be aborted.
vstring readSourceAndIncludes(const char *inputFn /* input */, long *size /* output */) {
  long i;
// D // long j;
// D // vstring_def(s);
  vstring_def(fileBuf);
  vstring_def(newFileBuf);

  vstring_def(fullInputFn);
  flag errorFlag = 0;

  // Read starting file
  let(&fullInputFn, cat(g_rootDirectory, inputFn, NULL));
  fileBuf = readFileToString(fullInputFn, 1 /* verbose */, &(*size));
  if (fileBuf == NULL) {
    print2("?Error: file \"%s\" was not found\n", fullInputFn);
    fileBuf = "";
    *size = 0;
    errorFlag = 1;
    // goto RETURN_POINT; // Don't go now so that g_IncludeCall[]
    // strings will be initialized.  If error, then blank fileBuf will
    // cause main while loop to break immediately after first
    // getNextInclusion() call.
  }
  print2("Reading source file \"%s\"... %ld bytes\n", fullInputFn, *size);
  free_vstring(fullInputFn);

  // Create a fictitious initial include for the main file (at least 2
  // g_IncludeCall structure array entries have been already been allocated
  // in initBigArrays() in mmdata.c).
  g_includeCalls = 0;
  // 0 means start of included file, 1 means continuation of including file
  g_IncludeCall[g_includeCalls].pushOrPop = 0;
  g_IncludeCall[g_includeCalls].source_fn = "";
  // $[ $], Begin, of Skip file name for this inclusion
  let(&g_IncludeCall[g_includeCalls].source_fn, inputFn);
  g_IncludeCall[g_includeCalls].included_fn = "";
  // $[ $], Begin, of Skip file name for this inclusion
  let(&g_IncludeCall[g_includeCalls].included_fn, inputFn);
  // This is the starting character position of the included file w.r.t entire source buffer 
  g_IncludeCall[g_includeCalls].current_offset = 0;
  // The line number of the start of the included file (=1) or the
  // continuation line of the parent file.
  g_IncludeCall[g_includeCalls].current_line = 1;
  // (Currently) assigned only if we may need it for a later Begin comparison
  g_IncludeCall[g_includeCalls].current_includeSource = "";
  // Length of the file to be included (0 if the file was previously included)
  g_IncludeCall[g_includeCalls].current_includeLength = *size;

  // Create a fictitious entry for the "continuation" after the
  // main file, to make error message line searching easier.
  g_includeCalls++;
  // 0 means start of included file, 1 means continuation of including file
  g_IncludeCall[g_includeCalls].pushOrPop = 1;
  g_IncludeCall[g_includeCalls].source_fn = "";
  // let(&g_IncludeCall[g_includeCalls].source_fn, inputFn); // Leave empty;
  // there is no "continuation" file for the main file, so no need to assign
  // (it won't be used).
  g_IncludeCall[g_includeCalls].included_fn = "";
  // let(&g_IncludeCall[g_includeCalls].included_fn, inputFn); // $[ $], Begin,
  // of Skip file name for this inclusion.
  // Ideally this should be countLines(fileBuf), but since it's never used we don't
  // bother to call countLines(fileBuf) to save CPU time.
  g_IncludeCall[g_includeCalls].current_line = -1;
  // (Currently) assigned only if we may need it for a later Begin comparison
  g_IncludeCall[g_includeCalls].current_includeSource = "";
  // The "continuation" of the main file is fictitious, so just set it to 0 length
  g_IncludeCall[g_includeCalls].current_includeLength = 0;

  // Recursively expand the source of an included file
  newFileBuf = "";
  newFileBuf = readInclude(fileBuf, 0, /* inputFn, */ inputFn, &(*size),
      1 /* parentLineNum */, &errorFlag);
  // This is the starting character position of the included file w.r.t entire
  // source buffer. Here, it points to the nonexistent character just beyond end
  // of main file (after all includes are expanded).
  // Note that readInclude() may change g_includeCalls, so use 1 explicitly.
  g_IncludeCall[1].current_offset = *size;
  free_vstring(fileBuf); // Deallocate
// D // printf("*size=%ld\n",*size);
// D // for(i=0;i<*size;i++){
// D // free_vstring(s);
// D // s=getFileAndLineNum(newFileBuf,newFileBuf+i,&j);
// D // printf("i=%ld ln=%ld fn=%s ch=%c\n",i,j,s,(newFileBuf+i)[0]);  }
  if (errorFlag == 1) {
    // The read should be aborted by the caller.
    // Deallocate the strings in the g_IncludeCall[] structure.
    for (i = 0; i <= g_includeCalls; i++) {
      free_vstring(g_IncludeCall[i].source_fn);
      free_vstring(g_IncludeCall[i].included_fn);
      free_vstring(g_IncludeCall[i].current_includeSource);
      g_includeCalls = -1; // For the eraseSource() function in mmcmds.c
    }
    return NULL;
  } else {
// D // for (i=0; i<=g_includeCalls;i++)
// D // printf("i=%ld p=%ld f=%s,%s l=%ld o=%ld s=%ld\n",
// D // i,(long)g_IncludeCall[i].pushOrPop,g_IncludeCall[i].source_fn,
// D // g_IncludeCall[i].included_fn,g_IncludeCall[i].current_line,
// D // g_IncludeCall[i].current_offset,g_IncludeCall[i].current_includeLength);
    return newFileBuf;
  }
} // readSourceAndIncludes

