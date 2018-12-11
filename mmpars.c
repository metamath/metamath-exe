/*****************************************************************************/
/*        Copyright (C) 2018  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <setjmp.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h"
/* #include "mmcmds.h" */  /* For getContribs() if used */
#include "mmpfas.h" /* Needed for pipDummyVars, subproofLen() */
#include "mmunif.h" /* Needed for minSubstLen */
#include "mmcmdl.h" /* Needed for rootDirectory */

long potentialStatements; /* Potential statements in source file (upper
                                 limit) for memory allocation purposes */
flag illegalLabelChar[256]; /* Illegal characters for labels -- initialized
                               by parseLabels() */
long *labelKeyBase; /* Start of assertion ($a, $p) labels */
long numLabelKeys; /* Number of assertion labels */

long *allLabelKeyBase; /* Start of all labels */
long numAllLabelKeys; /* Number of all labels */


/* Working structure for parsing proofs */
/* ???This structure should be deallocated by the ERASE command. */
long wrkProofMaxSize = 0; /* Maximum size so far - it may grow */
long wrkMathPoolMaxSize = 0; /* Max mathStringPool size so far - it may grow */
struct wrkProof_struct wrkProof;



/* This function returns a pointer to a buffer containing the contents of an
   input file and its 'include' calls.  'Size' returns the buffer's size.
   Partial parsing is done; when 'include' statements are found, this function
   is called recursively.
   The file input_fn is assumed to be opened when this is called. */
char *readRawSource(/*vstring inputFn,*/ /* 2-Feb-2018 nm Unused argument */
         vstring fileBuf, /* added 31-Dec-2017 */
         /*long bufOffsetSoFar,*/  /* deleted 31-Dec-2017 nm */
         long *size /* 31-Dec-2017 Now an input argument */)
{
  long charCount = 0;
  /*char *fileBuf;*/ /* 31-Dec-2017 */
  char *fbPtr;
  /*long lineNum;*/ /* 2-Feb-2018 Now computed in rawSourceError() */
  flag insideComment;
  /* flag insideLineComment; */ /* obsolete */
  char mode;
  char tmpch;

  /* 31-Dec-2017 nm */
  charCount = *size;

  /* Look for $[ and $] 'include' statement start and end */
  /* 31-Dec-2017 nm - these won't happen since they're now expanded earlier */
  /* But it can't hurt, since the code is already written.
     TODO - clean it up for speedup? */
  fbPtr = fileBuf;
  /*lineNum = 1;*/
  mode = 0; /* 0 = outside of 'include', 1 = inside of 'include' */
  insideComment = 0; /* 1 = inside $( $) comment */
  /* insideLineComment = 0; */ /* 1 = inside $! comment */
  while (1) {
    /* Find a keyword or newline */
    /* fbPtr = strpbrk(fbPtr, "\n$"); */ /* Takes 10 msec on VAX 4000/60 ! */
    tmpch = fbPtr[0];
    if (!tmpch) { /* End of file */
      if (insideComment) {
        rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum - 1, inputFn,*/
         "The last comment in the file is incomplete.  \"$)\" was expected.");
      } else {
        if (mode != 0) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum - 1, inputFn,*/
   "The last include statement in the file is incomplete.  \"$]\" was expected."
           );
        }
      }
      break;
    }
    if (tmpch != '$') {
      if (tmpch == '\n') {
        /* insideLineComment = 0; */ /* obsolete */
        /*lineNum++;*/
      } else {
        /* if (!insideComment && !insideLineComment) { */
          if (!isgraph((unsigned char)tmpch) && !isspace((unsigned char)tmpch)) {
            /* 19-Oct-2010 nm Used to bypass "lineNum++" below, which messed up
               line numbering. */
            rawSourceError(fileBuf, fbPtr, 1, /*lineNum, inputFn,*/
                cat("Illegal character (ASCII code ",
                str((double)((unsigned char)tmpch)),
                " decimal).",NULL));
        /* } */
        }
      }
      fbPtr++;
      continue;
    }


    /* 11-Sep-2009 nm Detect missing whitespace around keywords (per current
       Metamath language spec) */
    if (fbPtr > fileBuf) {  /* If this '$' is not the 1st file character */
      if (isgraph((unsigned char)(fbPtr[-1]))) {
        /* The character before the '$' is not white space */
        if (!insideComment || fbPtr[1] == ')') {
          /* Inside comments, we only care about the "$)" keyword */
          rawSourceError(fileBuf, fbPtr, 2, /*lineNum, inputFn,*/
              "A keyword must be preceded by white space.");
        }
      }
    }
    fbPtr++; /* (This line was already here before 11-Sep-2009 mod) */
    if (fbPtr[0]) {  /* If the character after '$' is not end of file (which
                        would be an error anyway, but detected elsewhere) */
      if (isgraph((unsigned char)(fbPtr[1]))) {
        /* The character after the character after the '$' is not white
           space (nor end of file) */
        if (!insideComment || fbPtr[0] == ')') {
          /* Inside comments, we only care about the "$)" keyword */
          rawSourceError(fileBuf, fbPtr + 1, 1, /*lineNum, inputFn,*/
              "A keyword must be followed by white space.");
          }
      }
    }
    /* End of 11-Sep-2009 mod */

    switch (fbPtr[0]) {
      case '(': /* Start of comment */
        if (insideComment) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
          "Nested comments are not allowed.");
        }
        insideComment = 1;
        continue;
      case ')': /* End of comment */
        if (!insideComment) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              "A comment terminator was found outside of a comment.");
        }
        insideComment = 0;
        continue;
      /* Comment to end-of-line */  /* obsolete */
      /*
      case '!':
        if (!insideComment) {
          insideLineComment = 1;
          fbPtr++;
          continue;
        }
      */
    }
    if (insideComment) continue;
    switch (fbPtr[0]) {
      case '[':
        if (mode != 0) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              "Nested include statements are not allowed.");
        } else {
          /* 31-Dec-2017 nm */
          /* $[ ... $] should have been processed by readSourceAndIncludes() */
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              "\"$[\" is unterminated or has ill-formed \"$]\".");

        }
        continue;
      case ']':
        if (mode == 0) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              "A \"$[\" is required before \"$]\".");
          continue;
        }

        /* 31-Dec-2017 nm */
        /* Because $[ $] are already expanded here, this should never happen */
        bug(1759);

        continue;

      case '{':
      case '}':
      case '.':
        potentialStatements++; /* Number of potential statements for malloc */
        break;
    } /* End switch fbPtr[0] */
  } /* End while */

  if (fbPtr != fileBuf + charCount) {
    /* To help debugging: */
    printf("fbPtr=%ld fileBuf=%ld charCount=%ld diff=%ld\n",
        (long)fbPtr, (long)fileBuf, charCount, fbPtr - fileBuf - charCount);
    bug(1704);
  }

  /* 31-Dec-2017 nm */
  print2("%ld bytes were read into the source buffer.\n", charCount);

  if (*size != charCount) bug(1761);
  return (fileBuf);

} /* readRawSource */

/* This function initializes the statement[] structure array and assigns
   sections of the raw input text.  statements is updated.
   sourcePtr is assumed to point to the raw input buffer.
   sourceLen is assumed to be length of the raw input buffer. */
void parseKeywords(void)
{
  long i, j, k;
  char *fbPtr;
  flag insideComment;
  char mode, type;
  char *startSection;
  char tmpch;
  long dollarPCount = 0; /* For statistics only */
  long dollarACount = 0; /* For statistics only */

  /*** 9-Jan-2018 nm Deleted
  /@ Variables needed for line number and file name @/
  long inclCallNum;
  char *nextInclPtr;
  long lineNum;
  vstring fileName;
  ***/

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  type = 0;

  /* Determine the upper limit of the number of new statements added for
     allocation purposes (computed in readRawInput) */
  potentialStatements = potentialStatements + 3; /* To be cautious */
/*E*/if(db5)print2("There are up to %ld potential statements.\n",
/*E*/   potentialStatements);

  /* Reallocate the statement array for all potential statements */
  statement = realloc(statement, (size_t)potentialStatements
      * sizeof(struct statement_struct));
  if (!statement) outOfMemory("#4 (statement)");

  /* Initialize the statement array */
  i = 0;
  statement[i].lineNum = 0;   /* assigned by assignStmtFileAndLineNum() */
  statement[i].fileName = ""; /* assigned by assignStmtFileAndLineNum() */
  statement[i].labelName = "";
  statement[i].uniqueLabel = 0;
  statement[i].type = illegal_;
  statement[i].scope = 0;
  statement[i].beginScopeStatementNum = 0;
  statement[i].labelSectionPtr = "";
  statement[i].labelSectionLen = 0; /* 3-May-2017 nm */
  statement[i].labelSectionChanged = 0;
  statement[i].statementPtr = ""; /* input to assignStmtFileAndLineNum() */
  statement[i].mathSectionPtr = "";
  statement[i].mathSectionLen = 0;
  statement[i].mathSectionChanged = 0; /* 3-May-2017 nm */
  statement[i].proofSectionPtr = "";
  statement[i].proofSectionLen = 0;
  statement[i].proofSectionChanged = 0; /* 3-May-2017 nm */
  statement[i].mathString = NULL_NMBRSTRING;
  statement[i].mathStringLen = 0;
  statement[i].proofString = NULL_NMBRSTRING;
  statement[i].reqHypList = NULL_NMBRSTRING;
  statement[i].optHypList = NULL_NMBRSTRING;
  statement[i].numReqHyp = 0;
  statement[i].reqVarList = NULL_NMBRSTRING;
  statement[i].optVarList = NULL_NMBRSTRING;
  statement[i].reqDisjVarsA = NULL_NMBRSTRING;
  statement[i].reqDisjVarsB = NULL_NMBRSTRING;
  statement[i].reqDisjVarsStmt = NULL_NMBRSTRING;
  statement[i].optDisjVarsA = NULL_NMBRSTRING;
  statement[i].optDisjVarsB = NULL_NMBRSTRING;
  statement[i].optDisjVarsStmt = NULL_NMBRSTRING;
  statement[i].pinkNumber = 0;
  statement[i].headerStartStmt = 0; /* 18-Dec-2016 nm */
  for (i = 1; i < potentialStatements; i++) {
    statement[i] = statement[0];
  }

  /* 10-Dec-2018 nm */
  /* In case there is no relevant statement (originally added for MARKUP
     command) */
  statement[0].labelName = "(N/A)";

/*E*/if(db5)print2("Finished initializing statement array.\n");

  /* Fill in the statement array with raw source text */
  fbPtr = sourcePtr;
  mode = 0; /* 0 = label section, 1 = math section, 2 = proof section */
  insideComment = 0; /* 1 = inside comment */
  startSection = fbPtr;

  while (1) {
    /* Find a keyword or newline */
    /* fbPtr = strpbrk(fbPtr, "\n$"); */ /* Takes 10 msec on VAX 4000/60 ! */
    tmpch = fbPtr[0];
    if (!tmpch) { /* End of file */
      if (mode != 0) {
        sourceError(fbPtr - 1, 2, statements,
            "Expected \"$.\" here (last line of file).");
        if (statements) { /* Adjustment for error messages */
          startSection = statement[statements].labelSectionPtr;
          statements--;
        }
      }
      break;
    }

    if (tmpch != '$') {
      /* 9-Jan-2018 nm Deleted - now computed by assignStmtFileAndLineNum() */
      /* if (tmpch == '\n') lineNum++; */
      fbPtr++;
      continue;
    }
    fbPtr++;
    switch (fbPtr[0]) {
      case '$': /* "$$" means literal "$" */
        fbPtr++;
        continue;
      case '(': /* Start of comment */
        /* if (insideComment) { */
          /* "Nested comments are not allowed." - detected by readRawSource */
        /* } */
        insideComment = 1;
        continue;
      case ')': /* End of comment */
        /* if (!insideComment) { */
          /* "Comment terminator found outside of comment."- detected by
             readRawSource */
        /* } */
        insideComment = 0;
        continue;
      case '!': /* Comment to end-of-line */
        if (insideComment) continue; /* Not really a comment */
        /* 8/23/99 - Made this syntax obsolete.  It was really obsolete
           before but tolerating it created problems with LaTeX output. */
        /******* Commented out these lines to force an error upon "$!"
        fbPtr = strchr(fbPtr, (int)'\n');
        if (!fbPtr) bug(1705);
        lineNum++;
        fbPtr++;
        continue;
        *******/
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
            sourceError(fbPtr - 1, 2, statements,
                "Expected \"$.\" here.");
          } else {
            sourceError(fbPtr - 1, 2, statements,
                "Expected \"$=\" here.");
          }
          continue;
        }
        /* Initialize a new statement */
        statements++;
        /*** 9-Jan-2018 nm Now assigned by assignStmtFileAndLineNum() as needed
        statement[statements].lineNum = lineNum;
        statement[statements].fileName = fileName;
        ****/
        statement[statements].type = type;
        statement[statements].labelSectionPtr = startSection;
        statement[statements].labelSectionLen = fbPtr - startSection - 1;
        /* The character after label section is used by
           assignStmtFileAndLineNum() to determine the "line number" for the
           statement as a whole */
        statement[statements].statementPtr = startSection
            + statement[statements].labelSectionLen; /* 9-Jan-2018 nm */
        startSection = fbPtr + 1;
        if (type != lb_ && type != rb_) mode = 1;
        continue;
      default:
        if (mode == 0) {
          sourceError(fbPtr - 1, 2, statements, cat(
              "Expected \"$c\", \"$v\", \"$e\", \"$f\", \"$d\",",
              " \"$a\", \"$p\", \"${\", or \"$}\" here.",NULL));
          continue;
        }
        if (mode == 1) {
          if (type == p_ && fbPtr[0] != '=') {
            sourceError(fbPtr - 1, 2, statements,
                "Expected \"$=\" here.");
            if (fbPtr[0] == '.') {
              mode = 2; /* If $. switch mode to help reduce error msgs */
            }
          }
          if (type != p_ && fbPtr[0] != '.') {
            sourceError(fbPtr - 1, 2, statements,
                "Expected \"$.\" here.");
            continue;
          }
          /* Add math section to statement */
          statement[statements].mathSectionPtr = startSection;
          statement[statements].mathSectionLen = fbPtr - startSection - 1;
          startSection = fbPtr + 1;
          if (type == p_ && mode != 2 /* !error msg case */) {
            mode = 2; /* Switch mode to proof section */
          } else {
            mode = 0;
          }
          continue;
        } /* End if mode == 1 */
        if (mode == 2) {
          if (fbPtr[0] != '.') {
            sourceError(fbPtr - 1, 2, statements,
                "Expected \"$.\" here.");
            continue;
          }
          /* Add proof section to statement */
          statement[statements].proofSectionPtr = startSection;
          statement[statements].proofSectionLen = fbPtr - startSection - 1;
          startSection = fbPtr + 1;
          mode = 0;
          continue;
        } /* End if mode == 2 */
    } /* End switch fbPtr[0] */
  } /* End while */

  if (fbPtr != sourcePtr + sourceLen) bug(1706);

  print2("The source has %ld statements; %ld are $a and %ld are $p.\n",
       statements, dollarACount, dollarPCount);

  /* Put chars after the last $. into the label section of a dummy statement */
  /* statement[statements + 1].lineNum = lineNum - 1; */
  /* 10/14/02 Changed this to lineNum so mmwtex $t error msgs will be correct */
  /* Here, lineNum will be one plus the number of lines in the file */
  /*** 9-Jan-2018 nm Now assigned by assignStmtFileAndLineNum() as needed
  statement[statements].lineNum = lineNum;
  statement[statements].fileName = fileName;
  ****/
  statement[statements + 1].type = illegal_;
  statement[statements + 1].labelSectionPtr = startSection;
  statement[statements + 1].labelSectionLen = fbPtr - startSection;
  /* Point to last character of file in case we ever need lineNum/fileName */
  statement[statements + 1].statementPtr = fbPtr - 1; /* 9-Jan-2018 */

  /* 10/25/02 Initialize the pink number to print after the statement labels
   in HTML output. */
  /* The pink number only counts $a and $p statements, unlike the statement
     number which also counts $f, $e, $c, $v, ${, $} */
  j = 0;
  k = 0; /* 18-Dec-2016 nm */
  for (i = 1; i <= statements; i++) {
    if (statement[i].type == a_ || statement[i].type == p_) {

      /* 18-Dec-2016 nm */
      /* Use the statement _after_ the previous $a or $p; that is the start
         of the "header area" for use by getSectionHeadings() in mmwtex.c.
         headerStartStmt will be equal to the current statement if the
         previous statement is also a $a or $p) */
      statement[i].headerStartStmt = k + 1;
      k = i;

      j++;
      statement[i].pinkNumber = j;
    }
  }
  /* 10-Jan-04  Also, put the largest pink number in the last statement, no
     matter what it kind it is, so we can look up the largest number in
     pinkHTML() in mmwtex.c */
  statement[statements].pinkNumber = j;


/*E*/if(db5){for (i=1; i<=statements; i++){
/*E*/  if (i == 5) { print2("(etc.)\n");} else { if (i<5) {
/*E*/  assignStmtFileAndLineNum(i);
/*E*/  print2("Statement %ld: line %ld file %s.\n",i,statement[i].lineNum,
/*E*/      statement[i].fileName);
/*E*/}}}}

}

/* This function parses the label sections of the statement[] structure array.
   sourcePtr is assumed to point to the beginning of the raw input buffer.
   sourceLen is assumed to be length of the raw input buffer. */
void parseLabels(void)
{
  long i, j, k;
  char *fbPtr;
  char type;
  long stmt;
  flag dupFlag;

  /* Define the legal label characters */
  for (i = 0; i < 256; i++) {
    illegalLabelChar[i] = !isalnum(i);
  }
  illegalLabelChar['-'] = 0;
  illegalLabelChar['_'] = 0;
  illegalLabelChar['.'] = 0;


  /* Scan all statements and extract their labels */
  for (stmt = 1; stmt <= statements; stmt++) {
    type = statement[stmt].type;
    fbPtr = statement[stmt].labelSectionPtr;
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
      statement[stmt].labelName = malloc((size_t)j + 1);
      if (!statement[stmt].labelName) outOfMemory("#5 (label)");
      statement[stmt].labelName[j] = 0;
      memcpy(statement[stmt].labelName, fbPtr, (size_t)j);
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
  } /* Next stmt */

  /* Make sure there is no token after the last statement */
  fbPtr = statement[statements + 1].labelSectionPtr; /* Last (dummy) statement*/
  i = whiteSpaceLen(fbPtr);
  j = tokenLen(fbPtr + i);
  if (j) {
    sourceError(fbPtr + i, j, 0,
        "There should be no tokens after the last statement.");
  }

  /* Sort the labels for later lookup */
  labelKey = malloc(((size_t)statements + 1) * sizeof(long));
  if (!labelKey) outOfMemory("#6 (labelKey)");
  for (i = 1; i <= statements; i++) {
    labelKey[i] = i;
  }
  labelKeyBase = &labelKey[1];
  numLabelKeys = statements;
  qsort(labelKeyBase, (size_t)numLabelKeys, sizeof(long), labelSortCmp);

  /* Skip null labels. */
  for (i = 1; i <= statements; i++) {
    if (statement[labelKey[i]].labelName[0]) break;
  }
  labelKeyBase = &labelKey[i];
  numLabelKeys = statements - i + 1;
/*E*/if(db5)print2("There are %ld non-empty labels.\n", numLabelKeys);
/*E*/if(db5){print2("The first (up to 5) sorted labels are:\n");
/*E*/  for (i=0; i<5; i++) {
/*E*/    if (i >= numLabelKeys) break;
/*E*/    print2("%s ",statement[labelKeyBase[i]].labelName);
/*E*/  } print2("\n");}



  /* Copy the keys for all possible labels for lookup by the
     squishProof command when local labels are generated in packed proofs. */
  allLabelKeyBase = malloc((size_t)numLabelKeys * sizeof(long));
  if (!allLabelKeyBase) outOfMemory("#60 (allLabelKeyBase)");
  memcpy(allLabelKeyBase, labelKeyBase, (size_t)numLabelKeys * sizeof(long));
  numAllLabelKeys = numLabelKeys;

  /* Now back to the regular label stuff. */
  /* Check for duplicate labels */
  /* (This will go away if local labels on hypotheses are allowed.) */
  /* 17-Sep-2005 nm - This code was reinstated to conform to strict spec.
     The old check for duplicate active labels (see other comment for this
     date below) was removed since it becomes redundant . */
  dupFlag = 0;
  for (i = 0; i < numLabelKeys; i++) {
    if (dupFlag) {
      /* This "if" condition causes only the 2nd in a pair of duplicate labels
         to have an error message. */
      dupFlag = 0;
      if (!strcmp(statement[labelKeyBase[i]].labelName,
          statement[labelKeyBase[i - 1]].labelName)) dupFlag = 1;
    }
    if (i < numLabelKeys - 1) {
      if (!strcmp(statement[labelKeyBase[i]].labelName,
          statement[labelKeyBase[i + 1]].labelName)) dupFlag = 1;
    }
    if (dupFlag) {
      fbPtr = statement[labelKeyBase[i]].labelSectionPtr;
      k = whiteSpaceLen(fbPtr);
      j = tokenLen(fbPtr + k);
      sourceError(fbPtr + k, j, labelKeyBase[i],
         "This label is declared more than once.  All labels must be unique.");
    }
  }

}

/* This functions retrieves all possible math symbols from $c and $v
   statements. */
void parseMathDecl(void)
{
  long potentialSymbols;
  long stmt;
  char *fbPtr;
  long i, j, k;
  char *tmpPtr;
  nmbrString *nmbrTmpPtr;
  long oldMathTokens;
  void *voidPtr; /* bsearch returned value */  /* 4-Jun-06 nm */

  /* Find the upper limit of the number of symbols declared for
     pre-allocation:  at most, the number of symbols is half the number of
     characters, since $c and $v statements require white space. */
  potentialSymbols = 0;
  for (stmt = 1; stmt <= statements; stmt++) {
    switch (statement[stmt].type) {
      case c_:
      case v_:
        potentialSymbols = potentialSymbols + statement[stmt].mathSectionLen;
    }
  }
  potentialSymbols = (potentialSymbols / 2) + 2;
/*E*/if(db5)print2("%ld potential symbols were computed.\n",potentialSymbols);
  mathToken = realloc(mathToken, (size_t)potentialSymbols *
      sizeof(struct mathToken_struct));
  if (!mathToken) outOfMemory("#7 (mathToken)");

  /* Scan $c and $v statements to accumulate all possible math symbols */
  mathTokens = 0;
  for (stmt = 1; stmt <= statements; stmt++) {
    switch (statement[stmt].type) {
      case c_:
      case v_:
        oldMathTokens = mathTokens;
        fbPtr = statement[stmt].mathSectionPtr;
        while (1) {
          i = whiteSpaceLen(fbPtr);
          j = tokenLen(fbPtr + i);
          if (!j) break;
          tmpPtr = malloc((size_t)j + 1); /* Math symbol name */
          if (!tmpPtr) outOfMemory("#8 (symbol name)");
          tmpPtr[j] = 0; /* End of string */
          memcpy(tmpPtr, fbPtr + i, (size_t)j);
          fbPtr = fbPtr + i + j;
          /* Create a new math symbol */
          mathToken[mathTokens].tokenName = tmpPtr;
          mathToken[mathTokens].length = j;
          if (statement[stmt].type == c_) {
            mathToken[mathTokens].tokenType = (char)con_;
          } else {
            mathToken[mathTokens].tokenType = (char)var_;
          }
          mathToken[mathTokens].active = 0;
          mathToken[mathTokens].scope = 0; /* Unknown for now */
          mathToken[mathTokens].tmp = 0; /* Not used for now */
          mathToken[mathTokens].statement = stmt;
          mathToken[mathTokens].endStatement = statements; /* Unknown for now */
                /* (Assign to 'statements' in case it's active until the end) */
          mathTokens++;

        }

        /* Create the symbol list for this statement */
        j = mathTokens - oldMathTokens; /* Number of tokens in this statement */
        nmbrTmpPtr = poolFixedMalloc((j + 1) * (long)(sizeof(nmbrString)));
        /* if (!nmbrTmpPtr) outOfMemory("#9 (symbol table)"); */ /*??? Not nec. with poolMalloc */
        nmbrTmpPtr[j] = -1;
        for (i = 0; i < j; i++) {
          nmbrTmpPtr[i] = oldMathTokens + i;
        }
        statement[stmt].mathString = nmbrTmpPtr;
        statement[stmt].mathStringLen = j;
        if (!j) {
          sourceError(fbPtr, 2, stmt,
           "At least one math symbol should be declared.");
        }
    } /* end switch (statement[stmt].type) */
  } /* next stmt */

/*E*/if(db5)print2("%ld math symbols were declared.\n",mathTokens);
  /* Reallocate from potential to actual to reduce memory space */
  /* Add 100 to allow for initial Proof Assistant use, and up to 100
     errors in undeclared token references */
  MAX_MATHTOKENS = mathTokens + 100;
  mathToken = realloc(mathToken, (size_t)MAX_MATHTOKENS *
      sizeof(struct mathToken_struct));
  if (!mathToken) outOfMemory("#10 (mathToken)");

  /* Create a special "$|$" boundary token to separate real and dummy ones */
  mathToken[mathTokens].tokenName = "";
  let(&mathToken[mathTokens].tokenName, "$|$");
  mathToken[mathTokens].length = 2; /* Never used */
  mathToken[mathTokens].tokenType = (char)con_;
  mathToken[mathTokens].active = 0; /* Never used */
  mathToken[mathTokens].scope = 0; /* Never used */
  mathToken[mathTokens].tmp = 0; /* Never used */
  mathToken[mathTokens].statement = 0; /* Never used */
  mathToken[mathTokens].endStatement = statements; /* Never used */


  /* Sort the math symbols for later lookup */
  mathKey = malloc((size_t)mathTokens * sizeof(long));
  if (!mathKey) outOfMemory("#11 (mathKey)");
  for (i = 0; i < mathTokens; i++) {
    mathKey[i] = i;
  }
  qsort(mathKey, (size_t)mathTokens, sizeof(long), mathSortCmp);
/*E*/if(db5){print2("The first (up to 5) sorted math tokens are:\n");
/*E*/  for (i=0; i<5; i++) {
/*E*/    if (i >= mathTokens) break;
/*E*/    print2("%s ",mathToken[mathKey[i]].tokenName);
/*E*/  } print2("\n");}


  /* 4-Jun-06 nm Check for labels with the same name as math tokens */
  /* (This section implements the Metamath spec change proposed by O'Cat that
     lets labels and math tokens occupy the same namespace and thus forbids
     them from having common names.) */
  /* For maximum speed, we scan M math tokens and look each up in the list
     of L labels.  The we have M * log L comparisons, which is optimal when
     (as in most cases) M << L. */
  for (i = 0; i < mathTokens; i++) {
    /* See if the math token is in the list of labels */
    voidPtr = (void *)bsearch(mathToken[i].tokenName, labelKeyBase,
        (size_t)numLabelKeys, sizeof(long), labelSrchCmp);
    if (voidPtr) { /* A label matching the token was found */
      stmt = (*(long *)voidPtr); /* Statement number */
      fbPtr = statement[stmt].labelSectionPtr;
      k = whiteSpaceLen(fbPtr);
      j = tokenLen(fbPtr + k);
      assignStmtFileAndLineNum(stmt); /* 9-Jan-2018 nm */
      sourceError(fbPtr + k, j, stmt, cat(
         "This label has the same name as the math token declared on line ",
         str((double)(statement[mathToken[i].statement].lineNum)), NULL));
    }
  }
  /* End of 4-Jun-06 */


}


/* This functions parses statement contents, except for proofs */
void parseStatements(void)
{
  long stmt;
  char type;
  long i, j, k, m, n, p;
  char *fbPtr;
  long mathStringLen;
  long tokenNum;
  long lowerKey, upperKey;
  long symbolLen, origSymbolLen, mathSectionLen, mathKeyNum;
  void *mathKeyPtr; /* bsearch returned value */
  int maxScope;
  long reqHyps, optHyps, reqVars, optVars;
  flag reqFlag;
  int undeclErrorCount = 0;
  vstring tmpStr = "";

  nmbrString *nmbrTmpPtr;

  long *mathTokenSameAs; /* Flag that symbol is unique (for speed up) */
  long *reverseMathKey; /* Map from mathTokens to mathKey */

  long *labelTokenSameAs; /* Flag that label is unique (for speed up) */
  long *reverseLabelKey; /* Map from statement # to label key */
  flag *labelActiveFlag; /* Flag that label is active */

  struct activeConstStack_struct {
    long tokenNum;
    int scope;
  };
  struct activeConstStack_struct *activeConstStack; /* Stack of active consts */
  long activeConstStackPtr = 0;

  struct activeVarStack_struct {
    long tokenNum;
    int scope;
    char tmpFlag; /* Used by hypothesis variable scan; must be 0 otherwise */
  };
  struct activeVarStack_struct *activeVarStack; /* Stack of active variables */
  nmbrString *wrkVarPtr1;
  nmbrString *wrkVarPtr2;
  long activeVarStackPtr = 0;

  struct activeEHypStack_struct { /* Stack of $e hypotheses */
    long statemNum;
    nmbrString *varList; /* List of variables in the hypothesis */
    int scope;
  };
  struct activeEHypStack_struct *activeEHypStack;
  long activeEHypStackPtr = 0;
  struct activeFHypStack_struct { /* Stack of $f hypotheses */
    long statemNum;
    nmbrString *varList; /* List of variables in the hypothesis */
    int scope;
  };
  struct activeFHypStack_struct *activeFHypStack;
  long activeFHypStackPtr = 0;
  nmbrString *wrkHypPtr1;
  nmbrString *wrkHypPtr2;
  nmbrString *wrkHypPtr3;
  long activeHypStackSize = 30; /* Starting value; could be as large as
                                   statements. */


  struct activeDisjHypStack_struct { /* Stack of disjoint variables in $d's */
    long tokenNumA; /* First variable in disjoint pair */
    long tokenNumB; /* Second variable in disjoint pair */
    long statemNum; /* Statement it occurred in */
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
  long activeDisjHypStackSize = 30; /* Starting value; could be as large as
                                        about mathTokens^2/2 */

  /* Temporary working space */
  long wrkLen;
  nmbrString *wrkNmbrPtr;
  char *wrkStrPtr;

  long maxSymbolLen; /* Longest math symbol (for speedup) */
  flag *symbolLenExists; /* A symbol with this length exists (for speedup) */

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  mathStringLen = 0;
  tokenNum = 0;

  /* Initialize flags for mathKey array that identify math symbols as
     unique (when 0) or, if not unique, the flag is a number identifying a group
     of identical names */
  mathTokenSameAs = malloc((size_t)mathTokens * sizeof(long));
  if (!mathTokenSameAs) outOfMemory("#12 (mathTokenSameAs)");
  reverseMathKey = malloc((size_t)mathTokens * sizeof(long));
  if (!reverseMathKey) outOfMemory("#13 (reverseMathKey)");
  for (i = 0; i < mathTokens; i++) {
    mathTokenSameAs[i] = 0; /* 0 means unique */
    reverseMathKey[mathKey[i]] = i; /* Initialize reverse map to mathKey */
  }
  for (i = 1; i < mathTokens; i++) {
    if (!strcmp(mathToken[mathKey[i]].tokenName,
        mathToken[mathKey[i - 1]].tokenName)) {
      if (!mathTokenSameAs[i - 1]) mathTokenSameAs[i - 1] = i;
      mathTokenSameAs[i] = mathTokenSameAs[i - 1];
    }
  }

  /* Initialize flags for labelKey array that identify labels as
     unique (when 0) or, if not unique, the flag is a number identifying a group
     of identical names */
  labelTokenSameAs = malloc(((size_t)statements + 1) * sizeof(long));
  if (!labelTokenSameAs) outOfMemory("#112 (labelTokenSameAs)");
  reverseLabelKey = malloc(((size_t)statements + 1) * sizeof(long));
  if (!reverseLabelKey) outOfMemory("#113 (reverseLabelKey)");
  labelActiveFlag = malloc(((size_t)statements + 1) * sizeof(flag));
  if (!labelActiveFlag) outOfMemory("#114 (labelActiveFlag)");
  for (i = 1; i <= statements; i++) {
    labelTokenSameAs[i] = 0; /* Initialize:  0 = unique */
    reverseLabelKey[labelKey[i]] = i; /* Initialize reverse map to labelKey */
    labelActiveFlag[i] = 0; /* Initialize */
  }
  for (i = 2; i <= statements; i++) {
    if (!strcmp(statement[labelKey[i]].labelName,
        statement[labelKey[i - 1]].labelName)) {
      if (!labelTokenSameAs[i - 1]) labelTokenSameAs[i - 1] = i;
      labelTokenSameAs[i] = labelTokenSameAs[i - 1];
    }
  }

  /* Initialize variable and hypothesis stacks */

  /* Allocate MAX_MATHTOKENS and not just mathTokens of them so that
     they can accomodate any extra non-declared tokens (which get
     declared as part of error handling, where the MAX_MATHTOKENS
     limit is checked) */
  activeConstStack = malloc((size_t)MAX_MATHTOKENS
      * sizeof(struct activeConstStack_struct));
  activeVarStack = malloc((size_t)MAX_MATHTOKENS
      * sizeof(struct activeVarStack_struct));
  wrkVarPtr1 = malloc((size_t)MAX_MATHTOKENS * sizeof(nmbrString));
  wrkVarPtr2 = malloc((size_t)MAX_MATHTOKENS * sizeof(nmbrString));
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

  /* Initialize temporary working space for parsing tokens */
  wrkLen = 1;
  wrkNmbrPtr = malloc((size_t)wrkLen * sizeof(nmbrString));
  if (!wrkNmbrPtr) outOfMemory("#22 (wrkNmbrPtr)");
  wrkStrPtr = malloc((size_t)wrkLen + 1);
  if (!wrkStrPtr) outOfMemory("#23 (wrkStrPtr)");

  /* Find declared math symbol lengths (used to speed up parsing) */
  maxSymbolLen = 0;
  for (i = 0; i < mathTokens; i++) {
    if (mathToken[i].length > maxSymbolLen) {
      maxSymbolLen = mathToken[i].length;
    }
  }
  symbolLenExists = malloc(((size_t)maxSymbolLen + 1) * sizeof(flag));
  if (!symbolLenExists) outOfMemory("#25 (symbolLenExists)");
  for (i = 0; i <= maxSymbolLen; i++) {
    symbolLenExists[i] = 0;
  }
  for (i = 0; i < mathTokens; i++) {
    symbolLenExists[mathToken[i].length] = 1;
  }


  currentScope = 0;
  beginScopeStatementNum = 0;

  /* Scan all statements.  Fill in statement structure and look for errors. */
  for (stmt = 1; stmt <= statements; stmt++) {

#ifdef VAXC
    /* This line fixes an obscure bug with the VAXC compiler.  If it is taken
       out, the variable 'stmt' does not get referenced properly when used as
       an array index.  May be due to some boundary condition in optimization?
       The machine code is significantly different with this statement
       removed. */
    stmt = stmt;  /* Work around VAXC bug */
#endif

    statement[stmt].beginScopeStatementNum = beginScopeStatementNum;
    statement[stmt].scope = currentScope;
    type = statement[stmt].type;
    /******* Determine scope, stack active variables, process math strings ****/

    switch (type) {
      case lb_:
        currentScope++;
        if (currentScope > 32000) outOfMemory("#16 (more than 32000 \"${\"s)");
            /* Not really an out-of-memory situation, but use the error msg. */
        break;
      case rb_:
        /* Remove all variables and hypotheses in current scope from stack */

        while (activeConstStackPtr) {
          if (activeConstStack[activeConstStackPtr - 1].scope < currentScope)
              break;
          activeConstStackPtr--;
          mathToken[activeConstStack[activeConstStackPtr].tokenNum].active = 0;
          mathToken[activeConstStack[activeConstStackPtr].tokenNum
              ].endStatement = stmt;
        }

        while (activeVarStackPtr) {
          if (activeVarStack[activeVarStackPtr - 1].scope < currentScope) break;
          activeVarStackPtr--;
          mathToken[activeVarStack[activeVarStackPtr].tokenNum].active = 0;
          mathToken[activeVarStack[activeVarStackPtr].tokenNum].endStatement
              = stmt;
        }

        while (activeEHypStackPtr) {
          if (activeEHypStack[activeEHypStackPtr - 1].scope < currentScope)
              break;
          activeEHypStackPtr--;
          labelActiveFlag[activeEHypStack[activeEHypStackPtr].statemNum] = 0;
                                                   /* Make the label inactive */
          free(activeEHypStack[activeEHypStackPtr].varList);
        }
        while (activeFHypStackPtr) {
          if (activeFHypStack[activeFHypStackPtr - 1].scope < currentScope)
              break;
          activeFHypStackPtr--;
          labelActiveFlag[activeFHypStack[activeFHypStackPtr].statemNum] = 0;
                                                   /* Make the label inactive */
          free(activeFHypStack[activeFHypStackPtr].varList);
        }
        while (activeDisjHypStackPtr) {
          if (activeDisjHypStack[activeDisjHypStackPtr - 1].scope
              < currentScope) break;
          activeDisjHypStackPtr--;
        }
        currentScope--;
        if (currentScope < 0) {
          sourceError(statement[stmt].labelSectionPtr +
              statement[stmt].labelSectionLen, 2, stmt,
              "Too many \"$}\"s at this point.");
        }
        break;
      case c_:
      case v_:
        /* Scan all symbols declared (they have already been parsed) and
           flag them as active, add to stack, and check for errors */

        /* 3-Jun-2018 nm Added this check back in (see 1-May-2018 Google
           Group post) */
        if (type == c_) {
          if (currentScope > 0) {
            sourceError(statement[stmt].labelSectionPtr +
                statement[stmt].labelSectionLen, 2, stmt,
        "A \"$c\" constant declaration may occur in the outermost scope only.");
          }
        }


        i = 0; /* Symbol position in mathString */
        nmbrTmpPtr = statement[stmt].mathString;
        while (1) {
          tokenNum = nmbrTmpPtr[i];
          if (tokenNum == -1) break; /* Done scanning symbols in $v or $c */
          if (mathTokenSameAs[reverseMathKey[tokenNum]]) {
            /* The variable name is not unique.  Find out if there's a
               conflict with the others. */
            lowerKey = reverseMathKey[tokenNum];
            upperKey = lowerKey;
            j = mathTokenSameAs[lowerKey];
            while (lowerKey) {
              if (j != mathTokenSameAs[lowerKey - 1]) break;
              lowerKey--;
            }
            while (upperKey < mathTokens - 1) {
              if (j != mathTokenSameAs[upperKey + 1]) break;
              upperKey++;
            }
            for (j = lowerKey; j <= upperKey; j++) {
              if (mathToken[mathKey[j]].active) {
                /* 18-Jun-2011 nm Detect conflicting active vars declared
                   in multiple scopes */
                if (mathToken[mathKey[j]].scope <= currentScope) {
                /* if (mathToken[mathKey[j]].scope == currentScope) { bad */
                  mathTokenError(i, nmbrTmpPtr, stmt,
                      "This symbol has already been declared in this scope.");
                }
              }
            }


            /************** Start of 9-Dec-2010 ****************/
            /* 9-Dec-2010 nm Make sure that no constant has the same name
               as a variable or vice-versa */
            k = 0; /* Flag for $c */
            m = 0; /* Flag for $v */
            for (j = lowerKey; j <= upperKey; j++) {
              if (mathToken[mathKey[j]].tokenType == (char)con_) k = 1;
              if (mathToken[mathKey[j]].tokenType == (char)var_) m = 1;
            }
            if ((k == 1 && mathToken[tokenNum].tokenType == (char)var_) ||
                (m == 1 && mathToken[tokenNum].tokenType == (char)con_)) {
               mathTokenError(i, nmbrTmpPtr, stmt,
                   "A symbol may not be both a constant and a variable.");
            }
            /************** End of 9-Dec-2010 ****************/

          }

          /* Flag the token as active */
          mathToken[tokenNum].active = 1;
          mathToken[tokenNum].scope = currentScope;

          if (type == v_) {

            /* Identify this stack position in the mathToken array, for use
               by the hypothesis variable scan below */
            mathToken[tokenNum].tmp = activeVarStackPtr;

            /* Add the symbol to the stack */
            activeVarStack[activeVarStackPtr].tokenNum = tokenNum;
            activeVarStack[activeVarStackPtr].scope = currentScope;
            activeVarStack[activeVarStackPtr].tmpFlag = 0;
            activeVarStackPtr++;
          } else {

            /* Add the symbol to the stack */
            activeConstStack[activeConstStackPtr].tokenNum = tokenNum;
            activeConstStack[activeConstStackPtr].scope = currentScope;
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
        /* Make sure we have enough working space */
        mathSectionLen = statement[stmt].mathSectionLen;
        if (wrkLen < mathSectionLen) {
          free(wrkNmbrPtr);
          free(wrkStrPtr);
          wrkLen = mathSectionLen + 100;
          wrkNmbrPtr = malloc((size_t)wrkLen * sizeof(nmbrString));
          if (!wrkNmbrPtr) outOfMemory("#20 (wrkNmbrPtr)");
          wrkStrPtr = malloc((size_t)wrkLen + 1);
          if (!wrkStrPtr) outOfMemory("#21 (wrkStrPtr)");
        }

        /* Scan the math section for tokens */
        mathStringLen = 0;
        fbPtr = statement[stmt].mathSectionPtr;
        while (1) {
          fbPtr = fbPtr + whiteSpaceLen(fbPtr);
          origSymbolLen = tokenLen(fbPtr);
          if (!origSymbolLen) break; /* Done scanning source line */

          /* Scan for largest matching token from the left */
         nextAdjToken:
          /* maxSymbolLen is the longest declared symbol */
          /* 18-Sep-2013 Disable unused old code
          if (origSymbolLen > maxSymbolLen) {
            symbolLen = maxSymbolLen;
          } else {
            symbolLen = origSymbolLen;
          }
          */

          /* New code: don't allow missing white space */
          symbolLen = origSymbolLen;

          memcpy(wrkStrPtr, fbPtr, (size_t)symbolLen);

          /* Old code: tolerate unambiguous missing white space
          for (; symbolLen > 0; symbolLen--) {
          */
          /* New code: don't allow missing white space */
          /* ???Speed-up is possible by rewriting this now unnec. code */
          for (; symbolLen > 0; symbolLen = 0) {

            /* symbolLenExists means a symbol of this length was declared */
            if (!symbolLenExists[symbolLen]) continue;
            wrkStrPtr[symbolLen] = 0; /* Define end of trial token to look up */
            mathKeyPtr = (void *)bsearch(wrkStrPtr, mathKey, (size_t)mathTokens,
                sizeof(long), mathSrchCmp);
            if (!mathKeyPtr) continue; /* Trial token was not declared */
            mathKeyNum = (long *)mathKeyPtr - mathKey; /* Pointer arithmetic! */
            if (mathTokenSameAs[mathKeyNum]) { /* Multiply-declared symbol */
              lowerKey = mathKeyNum;
              upperKey = lowerKey;
              j = mathTokenSameAs[lowerKey];
              while (lowerKey) {
                if (j != mathTokenSameAs[lowerKey - 1]) break;
                lowerKey--;
              }
              while (upperKey < mathTokens - 1) {
                if (j != mathTokenSameAs[upperKey + 1]) break;
                upperKey++;
              }
              /* Find the active symbol with the most recent declaration */
              /* (Note:  Here, 'active' means it's on the stack, not the
                 official def.) */
              maxScope = -1;
              for (i = lowerKey; i <= upperKey; i++) {
                j = mathKey[i];
                if (mathToken[j].active) {
                  if (mathToken[j].scope > maxScope) {
                    tokenNum = j;
                    maxScope = mathToken[j].scope;
                    if (maxScope == currentScope) break; /* Speedup */
                  }
                }
              }
              if (maxScope == -1) {
                tokenNum = mathKey[mathKeyNum]; /* Pick an arbitrary one */
                sourceError(fbPtr, symbolLen, stmt,
       "This math symbol is not active (i.e. was not declared in this scope).");
                /*??? (This is done in 3 places. Make it a fn call & clean up?*/
                /* Prevent stray pointers later */
                mathToken[tokenNum].tmp = 0; /* Loc in active variable stack */
                if (!activeVarStackPtr) { /* Make a ficticious entry */
                  activeVarStack[activeVarStackPtr].tokenNum = tokenNum;
                  activeVarStack[activeVarStackPtr].scope = currentScope;
                  activeVarStack[activeVarStackPtr].tmpFlag = 0;
                  activeVarStackPtr++;
                }
              }
            } else { /* The symbol was declared only once. */
              tokenNum = *((long *)mathKeyPtr);
                  /* Same as: tokenNum = mathKey[mathKeyNum]; but faster */
              if (!mathToken[tokenNum].active) {
                sourceError(fbPtr, symbolLen, stmt,
       "This math symbol is not active (i.e. was not declared in this scope).");
                /* Prevent stray pointers later */
                mathToken[tokenNum].tmp = 0; /* Loc in active variable stack */
                if (!activeVarStackPtr) { /* Make a ficticious entry */
                  activeVarStack[activeVarStackPtr].tokenNum = tokenNum;
                  activeVarStack[activeVarStackPtr].scope = currentScope;
                  activeVarStack[activeVarStackPtr].tmpFlag = 0;
                  activeVarStackPtr++;
                }
              }
            } /* End if multiply-defined symbol */
            break; /* The symbol was found, so we are done */
          } /* Next symbolLen */
          if (symbolLen == 0) { /* Symbol was not found */
            symbolLen = tokenLen(fbPtr);
            sourceError(fbPtr, symbolLen, stmt,
      "This math symbol was not declared (with a \"$c\" or \"$v\" statement).");
            /* Call the symbol a dummy token of type variable so that spurious
               errors (constants in $d's) won't be flagged also.  Prevent
               stray pointer to active variable stack. */
            undeclErrorCount++;
            tokenNum = mathTokens + undeclErrorCount;
            if (tokenNum >= MAX_MATHTOKENS) {
              /* 21-Aug-04 nm */
              /* There are current 100 places for bad tokens */
              print2(
"?Error: The temporary space for holding bad tokens has run out, because\n");
              print2(
"there are too many errors.  Therefore we will force an \"out of memory\"\n");
              print2("program abort:\n");
              outOfMemory("#33 (too many errors)");
            }
            mathToken[tokenNum].tokenName = "";
            let(&mathToken[tokenNum].tokenName, left(fbPtr,symbolLen));
            mathToken[tokenNum].length = symbolLen;
            mathToken[tokenNum].tokenType = (char)var_;
            /* Prevent stray pointers later */
            mathToken[tokenNum].tmp = 0; /* Location in active variable stack */
            if (!activeVarStackPtr) { /* Make a ficticious entry */
              activeVarStack[activeVarStackPtr].tokenNum = tokenNum;
              activeVarStack[activeVarStackPtr].scope = currentScope;
              activeVarStack[activeVarStackPtr].tmpFlag = 0;
              activeVarStackPtr++;
            }
          }

          if (type == d_) {
            if (mathToken[tokenNum].tokenType == (char)con_) {
              sourceError(fbPtr, symbolLen, stmt,
                  "Constant symbols are not allowed in a \"$d\" statement.");
            }
          } else {
            if (mathStringLen == 0) {
              if (mathToken[tokenNum].tokenType != (char)con_) {
                sourceError(fbPtr, symbolLen, stmt, cat(
                    "The first symbol must be a constant in a \"$",
                    chr(type), "\" statement.", NULL));
              }
            } else {
              if (type == f_) {
                if (mathStringLen == 1) {
                  if (mathToken[tokenNum].tokenType == (char)con_) {
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

          /* Add symbol to mathString */
          wrkNmbrPtr[mathStringLen] = tokenNum;
          mathStringLen++;
          fbPtr = fbPtr + symbolLen; /* Move on to next symbol */

          if (symbolLen < origSymbolLen) {
            /* This symbol is not separated from next by white space */
            /* Speed-up: don't call tokenLen again; just jump past it */
            origSymbolLen = origSymbolLen - symbolLen;
            goto nextAdjToken; /* (Instead of continue) */
          }
        } /* End while */

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


        /* Assign mathString to statement array */
        nmbrTmpPtr = poolFixedMalloc(
            (mathStringLen + 1) * (long)(sizeof(nmbrString)));
        /*if (!nmbrTmpPtr) outOfMemory("#24 (mathString)");*/ /*???Not nec. w/ poolMalloc */
        for (i = 0; i < mathStringLen; i++) {
          nmbrTmpPtr[i] = wrkNmbrPtr[i];
        }
        nmbrTmpPtr[mathStringLen] = -1;
        statement[stmt].mathString = nmbrTmpPtr;
        statement[stmt].mathStringLen = mathStringLen;
/*E*/if(db5){if(stmt<5)print2("Statement %ld mathString: %s.\n",stmt,
/*E*/  nmbrCvtMToVString(nmbrTmpPtr)); if(stmt==5)print2("(etc.)\n");}

        break;  /* Switch case break */
      default:
        bug(1707);

    } /* End switch */

    /****** Process hypothesis and variable stacks *******/
    /* (The switch section above does not depend on what is done in this
       section, although this section assumes the above section has been done.
       Variables valid only in this pass of the above section are so
       indicated.) */

    switch (type) {
      case f_:
      case e_:
      case a_:
      case p_:
        /* These types have labels.  Make the label active, and make sure that
           there is no other identical label that is also active. */
        /* (If the label name is unique, we don't have to worry about this.) */
        /* 17-Sep-05 nm - This check is no longer needed since all labels
           must now be unique according to strict spec (see the other comment
           for this date above).  So the code below was commented out. */
        /*
        if (labelTokenSameAs[reverseLabelKey[stmt]]) {
          /@ The label is not unique.  Find out if there's a
             conflict with the others. @/
          lowerKey = reverseLabelKey[stmt];
          upperKey = lowerKey;
          j = labelTokenSameAs[lowerKey];
          while (lowerKey > 1) {
            if (j != labelTokenSameAs[lowerKey - 1]) break;
            lowerKey--;
          }
          while (upperKey < statements) {
            if (j != labelTokenSameAs[upperKey + 1]) break;
            upperKey++;
          }
          for (j = lowerKey; j <= upperKey; j++) {
            if (labelActiveFlag[labelKey[j]]) {
              fbPtr = statement[stmt].labelSectionPtr;
              fbPtr = fbPtr + whiteSpaceLen(fbPtr);
              sourceError(fbPtr, tokenLen(fbPtr), labelKey[j], cat(
                  "This label name is currently active (i.e. in use).",
                  "  It became active at statement ",
                  str(labelKey[j]),
                  ", line ", str(statement[labelKey[j]].lineNum),
                  ", file \"", statement[labelKey[j]].fileName,
                  "\".  Use another name for this label.", NULL));
              break;
            }
          }
        }
        */

        /* Flag the label as active */
        labelActiveFlag[stmt] = 1;

    } /* End switch */


    switch (type) {
      case d_:

        nmbrTmpPtr = statement[stmt].mathString;
        /* Stack all possible pairs of disjoint variables */
        for (i = 0; i < mathStringLen - 1; i++) { /* mathStringLen is from the
             above switch section; it is valid only in this pass of the above
             section. */
          p = nmbrTmpPtr[i];
          for (j = i + 1; j < mathStringLen; j++) {
            n = nmbrTmpPtr[j];
            /* Get the disjoint variable pair m and n, sorted by tokenNum */
            if (p < n) {
              m = p;
            } else {
              if (p > n) {
                /* Swap them */
                m = n;
                n = p;
              } else {
                mathTokenError(j, nmbrTmpPtr, stmt,
                    "All variables in a \"$d\" statement must be unique.");
                break;
              }
            }
            /* See if this pair of disjoint variables is already on the stack;
               if so, don't add it again */
            for (k = 0; k < activeDisjHypStackPtr; k++) {
              if (m == activeDisjHypStack[k].tokenNumA)
                if (n == activeDisjHypStack[k].tokenNumB)
                  break; /* It matches */
            }
            if (k == activeDisjHypStackPtr) {
              /* It wasn't already on the stack, so add it. */
              /* Increase stack size if necessary */
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
              activeDisjHypStack[activeDisjHypStackPtr].scope = currentScope;
              activeDisjHypStack[activeDisjHypStackPtr].statemNum = stmt;

              activeDisjHypStackPtr++;
            }

          } /* Next j */
        } /* Next i */

        break; /* Switch case break */

      case f_:
      case e_:

        /* Increase stack size if necessary */
        /* For convenience, we will keep the size greater than the sum of
           active $e and $f hypotheses, as this is the size needed for the
           wrkHypPtr's, even though it wastes (temporary) memory for the
           activeE and activeF structure arrays. */
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

        /* Add the hypothesis to the stack */
        if (type == e_) {
          activeEHypStack[activeEHypStackPtr].statemNum = stmt;
          activeEHypStack[activeEHypStackPtr].scope = currentScope;
        } else {
          activeFHypStack[activeFHypStackPtr].statemNum = stmt;
          activeFHypStack[activeFHypStackPtr].scope = currentScope;
        }

        /* Create the list of variables used by this hypothesis */
        reqVars = 0;
        j = 0;
        nmbrTmpPtr = statement[stmt].mathString;
        k = nmbrTmpPtr[j]; /* Math symbol number */
        while (k != -1) {
          if (mathToken[k].tokenType == (char)var_) {
            if (!activeVarStack[mathToken[k].tmp].tmpFlag) {
              /* Variable has not been already added to list */
              wrkVarPtr1[reqVars] = k;
              reqVars++;
              activeVarStack[mathToken[k].tmp].tmpFlag = 1;
            }
          }
          j++;
          k = nmbrTmpPtr[j];
        }
        nmbrTmpPtr = malloc(((size_t)reqVars + 1) * sizeof(nmbrString));
        if (!nmbrTmpPtr) outOfMemory("#32 (hypothesis variables)");
        memcpy(nmbrTmpPtr, wrkVarPtr1, (size_t)reqVars * sizeof(nmbrString));
        nmbrTmpPtr[reqVars] = -1;
        /* Clear the variable flags for future re-use */
        for (i = 0; i < reqVars; i++) {
          activeVarStack[mathToken[nmbrTmpPtr[i]].tmp].tmpFlag = 0;
        }

        if (type == e_) {
          activeEHypStack[activeEHypStackPtr].varList = nmbrTmpPtr;
          activeEHypStackPtr++;
        } else {
          /* Taken care of earlier.
          if (nmbrTmpPtr[0] == -1) {
            sourceError(statement[stmt].mathSectionPtr +
                statement[stmt].mathSectionLen, 2, stmt,
                "A \"$f\" statement requires at least one variable.");
          }
          */
          activeFHypStack[activeFHypStackPtr].varList = nmbrTmpPtr;
          activeFHypStackPtr++;
        }

        break;  /* Switch case break */

      case a_:
      case p_:

        /* Scan this statement for required variables */
        reqVars = 0;
        j = 0;
        nmbrTmpPtr = statement[stmt].mathString;
        k = nmbrTmpPtr[j]; /* Math symbol number */
        while (k != -1) {
          if (mathToken[k].tokenType == (char)var_) {
            if (!activeVarStack[mathToken[k].tmp].tmpFlag) {
              /* Variable has not been already added to list */
              wrkVarPtr1[reqVars] = k;
              reqVars++;
              activeVarStack[mathToken[k].tmp].tmpFlag = 2;
                       /* 2 means it's an original variable in the assertion */
                       /* (For error-checking) */
            }
          }
          j++;
          k = nmbrTmpPtr[j];
        }

        /* Scan $e stack for required variables and required hypotheses */
        for (i = 0; i < activeEHypStackPtr; i++) {

          /* Add $e hypotheses to required list */
          wrkHypPtr1[i] = activeEHypStack[i].statemNum;

          /* Add the $e's variables to required variable list */
          nmbrTmpPtr = activeEHypStack[i].varList;
          j = 0; /* Location in variable list */
          k = nmbrTmpPtr[j]; /* Symbol number of variable */
          while (k != -1) {
            if (!activeVarStack[mathToken[k].tmp].tmpFlag) {
              /* Variable has not been already added to list */
              wrkVarPtr1[reqVars] = k;
              reqVars++;
            }
            activeVarStack[mathToken[k].tmp].tmpFlag = 1;
                            /* Could have been 0 or 2; 1 = in some hypothesis */
            j++;
            k = nmbrTmpPtr[j];
          }
        }

        reqHyps = activeEHypStackPtr; /* The number of required hyp's so far */

        /* We have finished determining required variables, so allocate the
           permanent list for the statement array */
        nmbrTmpPtr = poolFixedMalloc((reqVars + 1)
            * (long)(sizeof(nmbrString)));
        /* if (!nmbrTmpPtr) outOfMemory("#30 (reqVars)"); */
                                                    /* Not nec. w/ poolMalloc */
        memcpy(nmbrTmpPtr, wrkVarPtr1, (size_t)reqVars * sizeof(nmbrString));
        nmbrTmpPtr[reqVars] = -1;
        statement[stmt].reqVarList = nmbrTmpPtr;

        /* Scan the list of $f hypotheses to find those that are required */
        optHyps = 0;
        for (i = 0; i < activeFHypStackPtr; i++) {
          nmbrTmpPtr = activeFHypStack[i].varList; /* Variable list */
          tokenNum = nmbrTmpPtr[0];
          if (tokenNum == -1) {
            /* Default if no variables (an error in current version): */
            /* Add it to list of required hypotheses */
            wrkHypPtr1[reqHyps] = activeFHypStack[i].statemNum;
            reqHyps++;
            continue;
          } else {
            reqFlag = activeVarStack[mathToken[tokenNum].tmp].tmpFlag;
          }
          if (reqFlag) {
            /* Add it to list of required hypotheses */
            wrkHypPtr1[reqHyps] = activeFHypStack[i].statemNum;
            reqHyps++;
            reqFlag = 1;
            activeVarStack[mathToken[tokenNum].tmp].tmpFlag = 1;
                                /* Could have been 2; 1 = in some hypothesis */
          } else {
            /* Add it to list of optional hypotheses */
            wrkHypPtr2[optHyps] = activeFHypStack[i].statemNum;
            optHyps++;
          }

          /* Scan the other variables in the $f hyp to check for conflicts. */
          j = 1;
          tokenNum = nmbrTmpPtr[1];
          while (tokenNum != -1) {
            if (activeVarStack[mathToken[tokenNum].tmp].tmpFlag == 2) {
              activeVarStack[mathToken[tokenNum].tmp].tmpFlag = 1;
                                /* 2 = in $p; 1 = in some hypothesis */
            }
            if (reqFlag != activeVarStack[mathToken[tokenNum].tmp].tmpFlag) {
              k = activeFHypStack[i].statemNum;
              m = nmbrElementIn(1, statement[k].mathString, tokenNum);
              n = nmbrTmpPtr[0];
              if (reqFlag) {
                mathTokenError(m - 1, statement[k].mathString, k,
                    cat("This variable does not occur in statement ",
                    str((double)stmt)," (label \"",statement[stmt].labelName,
                    "\") or statement ", str((double)stmt),
                    "'s \"$e\" hypotheses, whereas variable \"",
                    mathToken[n].tokenName,
                   "\" DOES occur.  A \"$f\" hypothesis may not contain such a",
                    " mixture of variables.",NULL));
              } else {
                mathTokenError(m - 1, statement[k].mathString, k,
                    cat("This variable occurs in statement ",
                    str((double)stmt)," (label \"",statement[stmt].labelName,
                    "\") or statement ", str((double)stmt),
                    "'s \"$e\" hypotheses, whereas variable \"",
                    mathToken[n].tokenName,
               "\" does NOT occur.  A \"$f\" hypothesis may not contain such a",
                    " mixture of variables.",NULL));
              }
              break;
            } /* End if */
            j++;
            tokenNum = nmbrTmpPtr[j];
          } /* End while */

        } /* Next i */


        /* Error check:  make sure that all variables in the original statement
           appeared in some hypothesis */
        j = 0;
        nmbrTmpPtr = statement[stmt].mathString;
        k = nmbrTmpPtr[j]; /* Math symbol number */
        while (k != -1) {
          if (mathToken[k].tokenType == (char)var_) {
            if (activeVarStack[mathToken[k].tmp].tmpFlag == 2) {
              /* The variable did not appear in any hypothesis */
              mathTokenError(j, statement[stmt].mathString, stmt,
                    cat("This variable does not occur in any active ",
                    "\"$e\" or \"$f\" hypothesis.  All variables in \"$a\" and",
                    " \"$p\" statements must appear in at least one such",
                    " hypothesis.",NULL));
              activeVarStack[mathToken[k].tmp].tmpFlag = 1; /* One msg per var*/
            }
          }
          j++;
          k = nmbrTmpPtr[j];
        }


        /* We have finished determining required $e & $f hyps, so allocate the
           permanent list for the statement array */
        /* First, sort the required hypotheses by statement number order
           into wrkHypPtr3 */
        i = 0; /* Start of $e's in wrkHypPtr1 */
        j = activeEHypStackPtr; /* Start of $f's in wrkHypPtr1 */
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

        /* Now do the allocation */
        nmbrTmpPtr = poolFixedMalloc((reqHyps + 1)
            * (long)(sizeof(nmbrString)));
        /* if (!nmbrTmpPtr) outOfMemory("#33 (reqHyps)"); */
                                       /* Not nec. w/ poolMalloc */
        memcpy(nmbrTmpPtr, wrkHypPtr3, (size_t)reqHyps * sizeof(nmbrString));
        nmbrTmpPtr[reqHyps] = -1;
        statement[stmt].reqHypList = nmbrTmpPtr;
        statement[stmt].numReqHyp = reqHyps;

        /* We have finished determining optional $f hyps, so allocate the
           permanent list for the statement array */
        if (type == p_) { /* Optional ones are not used by $a statements */
          nmbrTmpPtr = poolFixedMalloc((optHyps + 1)
              * (long)(sizeof(nmbrString)));
          /* if (!nmbrTmpPtr) outOfMemory("#34 (optHyps)"); */ /* Not nec. w/ poolMalloc */
          memcpy(nmbrTmpPtr, wrkHypPtr2, (size_t)optHyps * sizeof(nmbrString));
          nmbrTmpPtr[optHyps] = -1;
          statement[stmt].optHypList = nmbrTmpPtr;
        }


        /* Scan the list of disjoint variable ($d) hypotheses to find those
           that are required */
        optHyps = 0;
        reqHyps = 0;
        for (i = 0; i < activeDisjHypStackPtr; i++) {
          m = activeDisjHypStack[i].tokenNumA; /* First var in disjoint pair */
          n = activeDisjHypStack[i].tokenNumB; /* 2nd var in disjoint pair */
          if (activeVarStack[mathToken[m].tmp].tmpFlag &&
              activeVarStack[mathToken[n].tmp].tmpFlag) {
            /* Both variables in the disjoint pair are required, so put the
               disjoint hypothesis in the required list. */
            wrkDisjHPtr1A[reqHyps] = m;
            wrkDisjHPtr1B[reqHyps] = n;
            wrkDisjHPtr1Stmt[reqHyps] =
                activeDisjHypStack[i].statemNum;
            reqHyps++;
          } else {
            /* At least one variable is not required, so the disjoint hypothesis\
               is not required. */
            wrkDisjHPtr2A[optHyps] = m;
            wrkDisjHPtr2B[optHyps] = n;
            wrkDisjHPtr2Stmt[optHyps] =
                activeDisjHypStack[i].statemNum;
            optHyps++;
          }
        }

        /* We have finished determining required $d hyps, so allocate the
           permanent list for the statement array */

        nmbrTmpPtr = poolFixedMalloc((reqHyps + 1)
            * (long)(sizeof(nmbrString)));
        /* if (!nmbrTmpPtr) outOfMemory("#40 (reqDisjHyps)"); */ /* Not nec. w/ poolMalloc */
        memcpy(nmbrTmpPtr, wrkDisjHPtr1A, (size_t)reqHyps
            * sizeof(nmbrString));
        nmbrTmpPtr[reqHyps] = -1;
        statement[stmt].reqDisjVarsA = nmbrTmpPtr;

        nmbrTmpPtr = poolFixedMalloc((reqHyps + 1)
            * (long)(sizeof(nmbrString)));
        /* if (!nmbrTmpPtr) outOfMemory("#41 (reqDisjHyps)"); */ /* Not nec. w/ poolMalloc */
        memcpy(nmbrTmpPtr, wrkDisjHPtr1B, (size_t)reqHyps
            * sizeof(nmbrString));
        nmbrTmpPtr[reqHyps] = -1;
        statement[stmt].reqDisjVarsB = nmbrTmpPtr;

        nmbrTmpPtr = poolFixedMalloc((reqHyps + 1)
            * (long)(sizeof(nmbrString)));
        /* if (!nmbrTmpPtr) outOfMemory("#42 (reqDisjHyps)"); */ /* Not nec. w/ poolMalloc */
        memcpy(nmbrTmpPtr, wrkDisjHPtr1Stmt, (size_t)reqHyps
            * sizeof(nmbrString));
        nmbrTmpPtr[reqHyps] = -1;
        statement[stmt].reqDisjVarsStmt = nmbrTmpPtr;

        /* We have finished determining optional $d hyps, so allocate the
           permanent list for the statement array */

        if (type == p_) { /* Optional ones are not used by $a statements */

          nmbrTmpPtr = poolFixedMalloc((optHyps + 1)
              * (long)(sizeof(nmbrString)));
          /* if (!nmbrTmpPtr) outOfMemory("#43 (optDisjHyps)"); */ /* Not nec. w/ poolMalloc */
          memcpy(nmbrTmpPtr, wrkDisjHPtr2A, (size_t)optHyps
              * sizeof(nmbrString));
          nmbrTmpPtr[optHyps] = -1;
          statement[stmt].optDisjVarsA = nmbrTmpPtr;

          nmbrTmpPtr = poolFixedMalloc((optHyps + 1)
              * (long)(sizeof(nmbrString)));
          /* if (!nmbrTmpPtr) outOfMemory("#44 (optDisjHyps)"); */ /* Not nec. w/ poolMalloc */
          memcpy(nmbrTmpPtr, wrkDisjHPtr2B, (size_t)optHyps
              * sizeof(nmbrString));
          nmbrTmpPtr[optHyps] = -1;
          statement[stmt].optDisjVarsB = nmbrTmpPtr;

          nmbrTmpPtr = poolFixedMalloc((optHyps + 1)
              * (long)(sizeof(nmbrString)));
          /* if (!nmbrTmpPtr) outOfMemory("#45 (optDisjHyps)"); */ /* Not nec. w/ poolMalloc */
          memcpy(nmbrTmpPtr, wrkDisjHPtr2Stmt, (size_t)optHyps
              * sizeof(nmbrString));
          nmbrTmpPtr[optHyps] = -1;
          statement[stmt].optDisjVarsStmt = nmbrTmpPtr;

        }


        /* Create list of optional variables (i.e. active but not required) */
        optVars = 0;
        for (i = 0; i < activeVarStackPtr; i++) {
          if (activeVarStack[i].tmpFlag) {
            activeVarStack[i].tmpFlag = 0; /* Clear it for future use */
          } else {
            wrkVarPtr2[optVars] = activeVarStack[i].tokenNum;
            optVars++;
          }
        }
        /* We have finished determining optional variables, so allocate the
           permanent list for the statement array */
        if (type == p_) { /* Optional ones are not used by $a statements */
          nmbrTmpPtr = poolFixedMalloc((optVars + 1)
              * (long)(sizeof(nmbrString)));
          /* if (!nmbrTmpPtr) outOfMemory("#31 (optVars)"); */ /* Not nec. w/ poolMalloc */
          memcpy(nmbrTmpPtr, wrkVarPtr2, (size_t)optVars * sizeof(nmbrString));
          nmbrTmpPtr[optVars] = -1;
          statement[stmt].optVarList = nmbrTmpPtr;
        }

        if (optVars + reqVars != activeVarStackPtr) bug(1708);


        break;  /* Switch case break */
    }

    /************** Start of 27-Sep-2010 ****************/
    /* 27-Sep-2010 nm If a $a statement consists of a single constant,
       e.g. "$a wff $.", it means an empty expression (wff) is allowed.
       Before the user had to allow this manually with
       SET EMPTY_SUBSTITUTION ON; now it is done automatically. */
    type = statement[stmt].type;
    if (type == a_) {
      if (minSubstLen) {
        if (statement[stmt].mathStringLen == 1) {
          minSubstLen = 0;
          printLongLine(cat("SET EMPTY_SUBSTITUTION was",
             " turned ON (allowed) for this database.", NULL),
             "    ", " ");
          /* More detailed but more distracting message:
          printLongLine(cat("Statement \"", statement[stmt].labelName,
             "\"  line ", str(statement[stmt].lineNum),
             " allows empty expressions, so SET EMPTY_SUBSTITUTION was",
             " turned ON (allowed) for this database.", NULL),
             "    ", " ");
          */
        }
      }
    }
    /************** End of 27-Sep-2010 ****************/

    /************** Start of 25-Sep-2010 ****************/
    /* 25-Sep-2010 nm Ensure the current Metamath spec is met:  "There may
       not be be two active $f statements containing the same variable.  Each
       variable in a $e, $a, or $p statement must exist in an active $f
       statement."  (Metamath book, p. 94) */
    /* This section of code is stand-alone and may be removed without side
       effects (other than less stringent error checking). */
    /* ??? To do (maybe):  This might be better placed in-line with the scan
       above, for faster speed and to get the pointer to the token for the
       error message, but it would require a careful code analysis above. */
    if (type == a_ || type == p_) {
      /* Scan each hypothesis (and the statement itself in last pass) */
      reqHyps = nmbrLen(statement[stmt].reqHypList);
      for (i = 0; i <= reqHyps; i++) {
        if (i < reqHyps) {
          m = (statement[stmt].reqHypList)[i];
        } else {
          m = stmt;
        }
        if (statement[m].type != f_) { /* Check $e,$a,$p */
          /* This block implements: "Each variable in a $e, $a, or $p
             statement must exist in an active $f statement" (Metamath
             book p. 94). */
          nmbrTmpPtr = statement[m].mathString;
          /* Scan all the vars in the $e (i<reqHyps) or $a/$p (i=reqHyps) */
          for (j = 0; j < statement[m].mathStringLen; j++) {
            tokenNum = nmbrTmpPtr[j];
            if (mathToken[tokenNum].tokenType == (char)con_) continue;
                                            /* Ignore constants */
            p = 0;  /* Initialize flag that we found a $f with the variable */
            /* Scan all the mandatory $f's before this $e,$a,$p */
            for (k = 0; k < i; k++) {
              n = (statement[stmt].reqHypList)[k];
              if (statement[n].type != f_) continue; /* Only check $f */
              if (statement[n].mathStringLen != 2) continue; /* This was
                  already verified earlier; but if there was an error, don't
                  cause memory violation by going out of bounds */
              if ((statement[n].mathString)[1] == tokenNum) {
                p = 1;  /* Set flag that we found a $f with the variable */
                break;
              }
            } /* next k ($f hyp scan) */
            if (!p) {
              sourceError(statement[m].mathSectionPtr/*fbPtr*/,
                  0/*tokenLen*/,
                  m/*stmt*/, cat(
                  "The variable \"", mathToken[tokenNum].tokenName,
                  "\" does not appear in an active \"$f\" statement.", NULL));
            }
          } /* next j (variable scan) */
        } else { /* statement[m].type == f_ */
          /* This block implements: "There may not be be two active $f
             statements containing the same variable" (Metamath book p. 94). */
          /* Check for duplicate vars in active $f's */
          if (statement[m].mathStringLen != 2) continue;  /* This was
                  already verified earlier; but if there was an error, don't
                  cause memory violation by going out of bounds */
          tokenNum = (statement[m].mathString)[1];
          /* Scan all the mandatory $f's before this $f */
          for (k = 0; k < i; k++) {
            n = (statement[stmt].reqHypList)[k];
            if (statement[n].type != f_) continue; /* Only check $f */
            if (statement[n].mathStringLen != 2) continue;  /* This was
                  already verified earlier; but if there was an error, don't
                  cause memory violation by going out of bounds */
            if ((statement[n].mathString)[1] == tokenNum) {
              /* We found 2 $f's with the same variable */
              assignStmtFileAndLineNum(n); /* 9-Jan-2018 nm */
              sourceError(statement[m].mathSectionPtr/*fbPtr*/,
                  0/*tokenLen*/,
                  m/*stmt*/, cat(
                  "The variable \"", mathToken[tokenNum].tokenName,
                "\" already appears in the earlier active \"$f\" statement \"",
                  statement[n].labelName, "\" on line ",
                  str((double)(statement[n].lineNum)),
                  " in file \"",           /* 9-Jan-2018 nm */
                  statement[n].fileName,   /* 9-Jan-2018 nm */
                  "\".", NULL));
              break; /* Optional: suppresses add'l error msgs for this stmt */
            }
          } /* next k ($f hyp scan) */
        } /* if not $f else is $f */
      } /* next i ($e hyp scan of this statement, or its $a/$p) */
    } /* if stmt is $a or $p */
    /************** End of 25-Sep-2010 ****************/

  } /* Next stmt */

  if (currentScope > 0) {
    if (currentScope == 1) {
      let(&tmpStr,"A \"$}\" is");
    } else {
      let(&tmpStr,cat(str((double)currentScope)," \"$}\"s are",NULL));
    }
    sourceError(statement[statements].labelSectionPtr +
        statement[statements].labelSectionLen, 2, 0,
        cat(tmpStr," missing at the end of the file.",NULL));
  }


  /* Filter out all hypothesis labels from the label key array.  We do not
     need them anymore, since they are stored locally in each statement
     structure.  Removing them will speed up lookups during proofs, and
     will prevent a lookup from finding an inactive hypothesis label (thus
     forcing an error message). */
  j = 0;
/*E*/if(db5)print2("Number of label keys before filter: %ld",numLabelKeys);
  for (i = 0; i < numLabelKeys; i++) {
    type = statement[labelKeyBase[i]].type;
    if (type == e_ || type == f_) {
      j++;
    } else {
      labelKeyBase[i - j] = labelKeyBase[i];
    }
  }
  numLabelKeys = numLabelKeys - j;
/*E*/if(db5)print2(".  After: %ld\n",numLabelKeys);


  /* Deallocate temporary space */
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
  free(wrkHypPtr3);  /* 28-Aug-2013 am - added missing free */
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
  let(&tmpStr, "");



}


/* Parse proof of one statement in source file.  Uses wrkProof structure. */
/* Returns 0 if OK; returns 1 if proof is incomplete (is empty or has '?'
   tokens);  returns 2 if error found; returns 3 if severe error found
   (e.g. RPN stack violation); returns 4 if not a $p statement */
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
  void *voidPtr; /* bsearch returned value */
  vstring tmpStrPtr;

  /* 25-Jan-2016 nm */
  flag explicitTargets = 0; /* Proof is of form <target>=<source> */
  /* Source file pointers and token sizes for targets in a /EXPLICIT proof */
  pntrString *targetPntr = NULL_PNTRSTRING; /* Pointers to target tokens */
  nmbrString *targetNmbr = NULL_NMBRSTRING; /* Size of target tokens */
  /* Variables for rearranging /EXPLICIT proof */
  nmbrString *wrkProofString = NULL_NMBRSTRING; /* Holds wrkProof.proofString */
  long hypStepNum, hypSubProofLen, conclSubProofLen;
  long matchingHyp;
  nmbrString *oldStepNums = NULL_NMBRSTRING; /* Just numbers 0 to numSteps-1 */
  pntrString *reqHypSubProof = NULL_PNTRSTRING; /* Subproofs of hypotheses */
  pntrString *reqHypOldStepNums = NULL_PNTRSTRING; /* Local label flag for
                                                     subproofs of hypotheses */
  nmbrString *rearrangedSubProofs = NULL_NMBRSTRING;
  nmbrString *rearrangedOldStepNums = NULL_NMBRSTRING;
  flag subProofMoved; /* Flag to restart scan after moving subproof */
                 /* 10-Mar-2016 nm */

  if (statement[statemNum].type != p_) {
    bug(1723); /* 13-Oct-05 nm - should never get here */
    wrkProof.errorSeverity = 4;
    return (4); /* Do nothing if not $p */
  }
  fbPtr = statement[statemNum].proofSectionPtr; /* Start of proof section */
  if (fbPtr[0] == 0) { /* The proof was never assigned (could be a $p statement
                          with no $=; this would have been detected earlier) */
    wrkProof.errorSeverity = 4;
    return (4); /* Pretend it's an empty proof */
  }
  fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  if (fbPtr[0] == '(') { /* "(" is flag for compressed proof */
    wrkProof.errorSeverity = parseCompressedProof(statemNum);
    return (wrkProof.errorSeverity);
  }

  /* Make sure we have enough working space to hold the proof */
  /* The worst case is less than the number of chars in the source,
     plus the number of active hypotheses */

  numOptHyp = nmbrLen(statement[statemNum].optHypList);
  if (statement[statemNum].proofSectionLen + statement[statemNum].numReqHyp
      + numOptHyp > wrkProofMaxSize) {
    if (wrkProofMaxSize) { /* Not the first allocation */
      free(wrkProof.tokenSrcPtrNmbr);
      free(wrkProof.tokenSrcPtrPntr);
      free(wrkProof.stepSrcPtrNmbr);
      free(wrkProof.stepSrcPtrPntr);
      free(wrkProof.localLabelFlag);
      free(wrkProof.hypAndLocLabel);
      free(wrkProof.localLabelPool);
      poolFree(wrkProof.proofString);
      free(wrkProof.mathStringPtrs);
      free(wrkProof.RPNStack);
      free(wrkProof.compressedPfLabelMap);
    }
    wrkProofMaxSize = statement[statemNum].proofSectionLen
        + statement[statemNum].numReqHyp + numOptHyp
        + 2; /* 2 is minimum for 1-step proof; the other terms could
                all be 0 */
    wrkProof.tokenSrcPtrNmbr = malloc((size_t)wrkProofMaxSize
        * sizeof(nmbrString));
    wrkProof.tokenSrcPtrPntr = malloc((size_t)wrkProofMaxSize
        * sizeof(pntrString));
    wrkProof.stepSrcPtrNmbr = malloc((size_t)wrkProofMaxSize
        * sizeof(nmbrString));
    wrkProof.stepSrcPtrPntr = malloc((size_t)wrkProofMaxSize
        * sizeof(pntrString));
    wrkProof.localLabelFlag = malloc((size_t)wrkProofMaxSize
        * sizeof(flag));
    wrkProof.hypAndLocLabel =
        malloc((size_t)wrkProofMaxSize * sizeof(struct sortHypAndLoc));
    wrkProof.localLabelPool = malloc((size_t)wrkProofMaxSize);
    wrkProof.proofString =
        poolFixedMalloc(wrkProofMaxSize * (long)(sizeof(nmbrString)));
         /* Use poolFixedMalloc instead of poolMalloc so that it won't get
            trimmed by memUsedPoolPurge. */
    wrkProof.mathStringPtrs =
        malloc((size_t)wrkProofMaxSize * sizeof(nmbrString));
    wrkProof.RPNStack = malloc((size_t)wrkProofMaxSize * sizeof(nmbrString));
    wrkProof.compressedPfLabelMap =
         malloc((size_t)wrkProofMaxSize * sizeof(nmbrString));
    if (!wrkProof.tokenSrcPtrNmbr ||
        !wrkProof.tokenSrcPtrPntr ||
        !wrkProof.stepSrcPtrNmbr ||
        !wrkProof.stepSrcPtrPntr ||
        !wrkProof.localLabelFlag ||
        !wrkProof.hypAndLocLabel ||
        !wrkProof.localLabelPool ||
        /* !wrkProof.proofString || */ /* Redundant because of poolMalloc */
        !wrkProof.mathStringPtrs ||
        !wrkProof.RPNStack
        ) outOfMemory("#99 (wrkProof)");
  }

  /* Initialization for this proof */
  wrkProof.errorCount = 0; /* Used as threshold for how many error msgs/proof */
  wrkProof.numSteps = 0;
  wrkProof.numTokens = 0;
  wrkProof.numHypAndLoc = 0;
  wrkProof.numLocalLabels = 0;
  wrkProof.RPNStackPtr = 0;
  wrkProof.localLabelPoolPtr = wrkProof.localLabelPool;

  /* fbPtr points to the first token now. */

  /* First break up proof section of source into tokens */
  while (1) {
    tokLength = proofTokenLen(fbPtr);
    if (!tokLength) break;
    wrkProof.tokenSrcPtrPntr[wrkProof.numTokens] = fbPtr;
    wrkProof.tokenSrcPtrNmbr[wrkProof.numTokens] = tokLength;
    wrkProof.numTokens++;
    fbPtr = fbPtr + tokLength;
    fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  }

  /* If there are no tokens, the proof is unknown; make the token a '?' */
  /* (wrkProof.tokenSrcPtrPntr won't point to the source, but this is OK since
     there will never be an error message for it.) */
  if (!wrkProof.numTokens) {

    /* For now, this is an error. */
    if (!wrkProof.errorCount) {
      sourceError(fbPtr, 2, statemNum,
          "The proof is empty.  If you don't know the proof, make it a \"?\".");
    }
    wrkProof.errorCount++;
    if (returnFlag < 1) returnFlag = 1;

    /* Allow empty proofs anyway */
    wrkProof.numTokens = 1;
    wrkProof.tokenSrcPtrPntr[0] = "?";
    wrkProof.tokenSrcPtrNmbr[0] = 1; /* Length */
  }

  /* Copy active (opt + req) hypotheses to hypAndLocLabel look-up table */
  nmbrTmpPtr = statement[statemNum].optHypList;
  /* Transfer optional hypotheses */
  while (1) {
    i = nmbrTmpPtr[wrkProof.numHypAndLoc];
    if (i == -1) break;
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelTokenNum = i;
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelName =
        statement[i].labelName;
    wrkProof.numHypAndLoc++;
  }
  /* Transfer required hypotheses */
  j = statement[statemNum].numReqHyp;
  nmbrTmpPtr = statement[statemNum].reqHypList;
  for (i = 0; i < j; i++) {
    k = nmbrTmpPtr[i];
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelTokenNum = k;
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelName =
        statement[k].labelName;
    wrkProof.numHypAndLoc++;
  }

  /* Sort the hypotheses by label name for lookup */
  numActiveHyp = wrkProof.numHypAndLoc; /* Save for bsearch later */
  qsort(wrkProof.hypAndLocLabel, (size_t)(wrkProof.numHypAndLoc),
      sizeof(struct sortHypAndLoc), hypAndLocSortCmp);


  /* Scan the parsed tokens for local label assignments */
  fbPtr = wrkProof.tokenSrcPtrPntr[0];
  if (fbPtr[0] == ':') {
    if (!wrkProof.errorCount) {
      sourceError(fbPtr, 1, statemNum,
          "The colon at proof step 1 must be preceded by a local label.");
    }
    if (returnFlag < 2) returnFlag = 2;
    wrkProof.tokenSrcPtrPntr[0] = "?";
    wrkProof.tokenSrcPtrNmbr[0] = 1; /* Length */
    wrkProof.errorCount++;
  }
  fbPtr = wrkProof.tokenSrcPtrPntr[wrkProof.numTokens - 1];
  if (fbPtr[0] == ':') {
    if (!wrkProof.errorCount) {
      sourceError(fbPtr, 1, statemNum,
          "The colon in the last proof step must be followed by a label.");
    }
    if (returnFlag < 2) returnFlag = 2;
    wrkProof.errorCount++;
    wrkProof.numTokens--;
  }
  labelFlag = 0;
  for (tok = 0; tok < wrkProof.numTokens; tok++) {
    fbPtr = wrkProof.tokenSrcPtrPntr[tok];

    /* 25-Jan-2016 nm */
    /* If next token is = then this token is a target for /EXPLICIT format,
       so don't increment the proof step number */
    if (tok < wrkProof.numTokens - 2) {
      if (((char *)((wrkProof.tokenSrcPtrPntr)[tok + 1]))[0] == '=') {
        explicitTargets = 1; /* Flag that proof has explicit targets */
        continue;
      }
    }
    if (fbPtr[0] == '=') continue; /* Skip the = token */

    /* Save pointer to source file vs. step for error messages */
    wrkProof.stepSrcPtrNmbr[wrkProof.numSteps] =
        wrkProof.tokenSrcPtrNmbr[tok]; /* Token length */
    wrkProof.stepSrcPtrPntr[wrkProof.numSteps] = fbPtr; /* Token ptr */

    /* Save fact that this step has a local label declaration */
    wrkProof.localLabelFlag[wrkProof.numSteps] = labelFlag;
    labelFlag = 0;

    wrkProof.numSteps++;
    if (fbPtr[0] != ':') continue;

    /* Colon found -- previous token is a label */
    labelFlag = 1;

    wrkProof.numSteps = wrkProof.numSteps - 2;
    fbPtr = wrkProof.tokenSrcPtrPntr[tok - 1]; /* The local label */
    tokLength = wrkProof.tokenSrcPtrNmbr[tok - 1]; /* Its length */

    /* Check for illegal characters */
    for (j = 0; j < tokLength; j++) {
      if (illegalLabelChar[(unsigned char)fbPtr[j]]) {
        if (!wrkProof.errorCount) {
          sourceError(fbPtr + j, 1, statemNum,cat(
              "The local label at proof step ",
              str((double)(wrkProof.numSteps + 1)),
              " is incorrect.  Only letters,",
              " digits, \"_\", \"-\", and \".\" are allowed in local labels.",
              NULL));
        }
        if (returnFlag < 2) returnFlag = 2;
        wrkProof.errorCount++;
      }
    }

    /* Add the label to the local label pool and hypAndLocLabel table */
    memcpy(wrkProof.localLabelPoolPtr, fbPtr, (size_t)tokLength);
    wrkProof.localLabelPoolPtr[tokLength] = 0; /* String terminator */
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelTokenNum =
       -wrkProof.numSteps - 1000; /* offset of -1000 is flag for local label*/
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelName
        = wrkProof.localLabelPoolPtr;

    /* Make sure local label is different from all earlier $a and $p labels */
    voidPtr = (void *)bsearch(wrkProof.localLabelPoolPtr, labelKeyBase,
        (size_t)numLabelKeys, sizeof(long), labelSrchCmp);
    if (voidPtr) { /* It was found */
      j = *(long *)voidPtr; /* Statement number */
      if (j <= statemNum) {
        if (!wrkProof.errorCount) {
          assignStmtFileAndLineNum(j); /* 9-Jan-2018 nm */
          sourceError(fbPtr, tokLength, statemNum,cat(
              "The local label at proof step ",
              str((double)(wrkProof.numSteps + 1)),
              " is the same as the label of statement ",
              str((double)j),
              " at line ",
              str((double)(statement[j].lineNum)),
              " in file \"",
              statement[j].fileName,
              "\".  Local labels must be different from active statement labels.",
              NULL));
        }
        wrkProof.errorCount++;
        if (returnFlag < 2) returnFlag = 2;
      }
    }

    /* Make sure local label is different from all active $e and $f labels */
    voidPtr = (void *)bsearch(wrkProof.localLabelPoolPtr,
        wrkProof.hypAndLocLabel,
        (size_t)numActiveHyp, sizeof(struct sortHypAndLoc), hypAndLocSrchCmp);
    if (voidPtr) { /* It was found */
      j = ( (struct sortHypAndLoc *)voidPtr)->labelTokenNum; /* Statement number */
      if (!wrkProof.errorCount) {
        assignStmtFileAndLineNum(j); /* 9-Jan-2018 nm */
        sourceError(fbPtr, tokLength, statemNum,cat(
            "The local label at proof step ",
            str((double)(wrkProof.numSteps + 1)),
            " is the same as the label of statement ",
            str((double)j),
            " at line ",
            str((double)(statement[j].lineNum)),
            " in file \"",
            statement[j].fileName,
            "\".  Local labels must be different from active statement labels.",
            NULL));
      }
      wrkProof.errorCount++;
      if (returnFlag < 2) returnFlag = 2;
      wrkProof.numHypAndLoc--; /* Ignore the label */
    }

    wrkProof.numHypAndLoc++;
    wrkProof.localLabelPoolPtr = &wrkProof.localLabelPoolPtr[tokLength + 1];

  } /* Next i */

  /* 25-Jan-2016 nm */
  /* Collect all target labels in /EXPLICIT format */
  /* I decided not to make targetPntr, targetNmbr part of the wrkProof
     structure since other proof formats don't assign it, so we can't
     reference it reliably outside of this function.  And it would waste
     some memory if we don't use /EXPLICIT, which is intended primarily
     for database maintenance. */
  if (explicitTargets == 1) {
    pntrLet(&targetPntr, pntrSpace(wrkProof.numSteps));
    nmbrLet(&targetNmbr, nmbrSpace(wrkProof.numSteps));
    step = 0;
    for (tok = 0; tok < wrkProof.numTokens - 2; tok++) {
      /* If next token is = then this token is a target for /EXPLICIT format,
         so don't increment the proof step number */
      if (((char *)((wrkProof.tokenSrcPtrPntr)[tok + 1]))[0] == '=') {
        fbPtr = wrkProof.tokenSrcPtrPntr[tok];
        if (step >= wrkProof.numSteps) {
          if (!wrkProof.errorCount) {
            sourceError(fbPtr, wrkProof.tokenSrcPtrNmbr[tok], statemNum, cat(
                "There are more target labels than proof steps.", NULL));
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          break;
        }
        targetPntr[step] = fbPtr;
        targetNmbr[step] = wrkProof.tokenSrcPtrNmbr[tok];
        if (wrkProof.tokenSrcPtrPntr[tok + 2]
            != wrkProof.stepSrcPtrPntr[step]) {
          if (!wrkProof.errorCount) {
            sourceError(fbPtr, wrkProof.tokenSrcPtrNmbr[tok], statemNum, cat(
                "The target label for step ", str((double)step + 1),
                " is not assigned to that step.  ",
                "(Check for missing or extra \"=\".)", NULL));
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }
        step++;
      }
    } /* next tok */
    if (step != wrkProof.numSteps) {
      if (!wrkProof.errorCount) {
        sourceError(
            (char *)((wrkProof.tokenSrcPtrPntr)[wrkProof.numTokens - 1]),
            wrkProof.tokenSrcPtrNmbr[wrkProof.numTokens - 1],
            statemNum, cat(
                "There are ", str((double)(wrkProof.numSteps)), " proof steps but only ",
                str((double)step), " target labels.", NULL));
      }
      wrkProof.errorCount++;
      if (returnFlag < 2) returnFlag = 2;
    }
  } /* if explicitTargets == 1 */

  if (wrkProof.numHypAndLoc > numActiveHyp) { /* There were local labels */

    /* Sort the local labels into the hypAndLocLabel look-up table */
    qsort(wrkProof.hypAndLocLabel, (size_t)(wrkProof.numHypAndLoc),
        sizeof(struct sortHypAndLoc), hypAndLocSortCmp);

    /* Check for duplicate local labels */
    for (i = 1; i < wrkProof.numHypAndLoc; i++) {
      if (!strcmp(wrkProof.hypAndLocLabel[i - 1].labelName,
          wrkProof.hypAndLocLabel[i].labelName)) { /* Duplicate label */
        /* Get the step numbers */
        j = -(wrkProof.hypAndLocLabel[i - 1].labelTokenNum + 1000);
        k = -(wrkProof.hypAndLocLabel[i].labelTokenNum + 1000);
        if (j > k) {
          m = j;
          j = k; /* Smaller step number */
          k = m; /* Larger step number */
        }
        /* Find the token - back up a step then move forward to loc label */
        fbPtr = wrkProof.stepSrcPtrPntr[k - 1]; /* Previous step */
        fbPtr = fbPtr + wrkProof.stepSrcPtrNmbr[k - 1];
        fbPtr = fbPtr + whiteSpaceLen(fbPtr);
        if (!wrkProof.errorCount) {
          sourceError(fbPtr,
              proofTokenLen(fbPtr), statemNum,
              cat("The local label at proof step ", str((double)k + 1),
              " is the same as the one declared at step ",
              str((double)j + 1), ".", NULL));
        }
        wrkProof.errorCount++;
        if (returnFlag < 2) returnFlag = 2;
      } /* End if duplicate label */
    } /* Next i */

  } /* End if there are local labels */

  /* Build the proof string and check the RPN stack */
  wrkProof.proofString[wrkProof.numSteps] = -1; /* End of proof */
  nmbrZapLen(wrkProof.proofString, wrkProof.numSteps);
     /* Zap mem pool actual length (because nmbrLen will be used later on this)*/

  /* 25-Jan-2016 nm */
  if (explicitTargets == 1) {
    /* List of original step numbers to keep track of local label movement */
    nmbrLet(&oldStepNums, nmbrSpace(wrkProof.numSteps));
    for (i = 0; i < wrkProof.numSteps; i++) {
      oldStepNums[i] = i;
    }
  }

  for (step = 0; step < wrkProof.numSteps; step++) {
    tokLength = wrkProof.stepSrcPtrNmbr[step];
    fbPtr = wrkProof.stepSrcPtrPntr[step];

    /* Handle unknown proof steps */
    if (fbPtr[0] == '?') {
      if (returnFlag < 1) returnFlag = 1;
                                      /* Flag that proof is partially unknown */
      wrkProof.proofString[step] = -(long)'?';
      /* Treat "?" like a hypothesis - push stack and continue */
      wrkProof.RPNStack[wrkProof.RPNStackPtr] = step;
      wrkProof.RPNStackPtr++;
      continue;
    }

    /* Temporarily zap the token's end with a null for string comparisons */
    zapSave = fbPtr[tokLength];
    fbPtr[tokLength] = 0; /* Zap source */

    /* See if the proof token is a hypothesis or local label ref. */
    voidPtr = (void *)bsearch(fbPtr, wrkProof.hypAndLocLabel,
        (size_t)(wrkProof.numHypAndLoc), sizeof(struct sortHypAndLoc),
        hypAndLocSrchCmp);
    if (voidPtr) {
      fbPtr[tokLength] = zapSave; /* Unzap source */
      j = ((struct sortHypAndLoc *)voidPtr)->labelTokenNum; /* Label lookup number */
      wrkProof.proofString[step] = j; /* Proof string */

      /* Push the stack */
      wrkProof.RPNStack[wrkProof.RPNStackPtr] = step;
      wrkProof.RPNStackPtr++;

      if (j < 0) { /* It's a local label reference */
        i = -1000 - j; /* Step number referenced */
        if (i < 0) bug(1734);

        /* Make sure we don't reference a later step */
        if (i > step) {
          if (!wrkProof.errorCount) {
            sourceError(fbPtr, tokLength, statemNum,cat("Proof step ",
                str((double)step + 1),
                " references a local label before it is declared.",
                NULL));
          }
          wrkProof.proofString[step] = -(long)'?';
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }

        if (wrkProof.localLabelFlag[step]) {
          if (!wrkProof.errorCount) {
            /* Chained labels not allowed because it complicates the language
               but doesn't buy anything */
            sourceError(fbPtr, tokLength, statemNum, cat(
                "The local label reference at proof step ",
                str((double)step + 1),
                " declares a local label.  Only \"$a\" and \"$p\" statement",
                " labels may have local label declarations.",NULL));
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }
      } else { /* It's a hypothesis reference */
        if (wrkProof.localLabelFlag[step]) {
          /* Not allowed because it complicates the language but doesn't
             buy anything; would make $e to $f assignments harder to detect */
          if (!wrkProof.errorCount) {
            sourceError(fbPtr, tokLength, statemNum, cat(
                "The hypothesis reference at proof step ",
                str((double)step + 1),
                " declares a local label.  Only \"$a\" and \"$p\" statement",
                " labels may have local label declarations.",NULL));
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }
        if (j <= 0) bug(1709);
      }
      continue;
    } /* End if local label or hypothesis */

    /* See if token is an assertion label */
    voidPtr = (void *)bsearch(fbPtr, labelKeyBase, (size_t)numLabelKeys,
        sizeof(long), labelSrchCmp);
    fbPtr[tokLength] = zapSave; /* Unzap source */
    if (!voidPtr) {
      if (!wrkProof.errorCount) {
        sourceError(fbPtr, tokLength, statemNum, cat(
            "The token at proof step ",
            str((double)step + 1),
            " is not an active statement label or a local label.",NULL));
      }
      wrkProof.errorCount++;
      wrkProof.proofString[step] = -(long)'?';
      /* Push the stack */
      wrkProof.RPNStack[wrkProof.RPNStackPtr] = step;
      wrkProof.RPNStackPtr++;
      if (returnFlag < 2) returnFlag = 2;
      continue;
    }

    /* It's an assertion ($a or $p) */
    j = *(long *)voidPtr; /* Statement number */
    if (statement[j].type != a_ && statement[j].type != p_) bug(1710);
    wrkProof.proofString[step] = j; /* Assign $a/$p label to proof string */

    if (j >= statemNum) { /* Error */
      if (!wrkProof.errorCount) {
        if (j == statemNum) {
          sourceError(fbPtr, tokLength, statemNum, cat(
              "The label at proof step ",
              str((double)step + 1),
              " is the label of this statement.  A statement may not be used to",
              " prove itself.",NULL));
        } else {
          assignStmtFileAndLineNum(j); /* 9-Jan-2018 nm */
          sourceError(fbPtr, tokLength, statemNum, cat(
              "The label \"", statement[j].labelName, "\" at proof step ",
              str((double)step + 1),
              " is the label of a future statement (at line ",
              str((double)(statement[j].lineNum)),
              " in file ",statement[j].fileName,
      ").  Only local labels or previous, active statements may be referenced.",
              NULL));
        }
      }
      wrkProof.errorCount++;
      if (returnFlag < 2) returnFlag = 2;
    }

    /* It's a valid assertion, so pop the stack */
    numReqHyp = statement[j].numReqHyp;

    /* Error check for exhausted stack */
    if (wrkProof.RPNStackPtr < numReqHyp) { /* Stack exhausted -- error */
      if (!wrkProof.errorCount) {
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
            statement[j].labelName,"\" requires ",
            tmpStrPtr,".",NULL));
        let(&tmpStrPtr, "");
      }
      /* Treat it like an unknown step so stack won't get exhausted */
      wrkProof.errorCount++;
      wrkProof.proofString[step] = -(long)'?';
      /* Push the stack */
      wrkProof.RPNStack[wrkProof.RPNStackPtr] = step;
      wrkProof.RPNStackPtr++;
      if (returnFlag < 3) returnFlag = 3;
      continue;
    } /* End if stack exhausted */

    /**** Start of 25-Jan-2016 nm ***/
    /* For proofs saved with /EXPLICIT, the user may have changed the order
       of hypotheses.  First, get the subproofs for the hypotheses.  Then
       reassemble them in the right order. */
    if (explicitTargets == 1) {
      nmbrLet(&wrkProofString, wrkProof.proofString);
            /* nmbrString to rearrange proof then when done reassign to
               wrkProof.proofString structure component */
      nmbrTmpPtr = statement[j].reqHypList;
      numReqHyp = statement[j].numReqHyp;
      conclSubProofLen = subproofLen(wrkProofString, step);
      pntrLet(&reqHypSubProof, pntrNSpace(numReqHyp));
                                         /* Initialize to NULL_NMBRSTRINGs */
      pntrLet(&reqHypOldStepNums, pntrNSpace(numReqHyp));
                                         /* Initialize to NULL_NMBRSTRINGs */
      k = 0; /* Total hypothesis subproof lengths for error checking */
      for (i = 0; i < numReqHyp; i++) {
        m = wrkProof.RPNStackPtr - numReqHyp + i; /* Stack position of hyp */
        hypStepNum = wrkProof.RPNStack[m]; /* Step number of hypothesis i */
        hypSubProofLen = subproofLen(wrkProofString, hypStepNum);
        k += hypSubProofLen;
        nmbrLet((nmbrString **)(&(reqHypSubProof[i])),
            /* For nmbrSeg, 1 = first step */
            nmbrSeg(wrkProofString,
                hypStepNum - hypSubProofLen + 2, hypStepNum + 1));
        nmbrLet((nmbrString **)(&(reqHypOldStepNums[i])),
            /* For nmbrSeg, 1 = first step */
            nmbrSeg(oldStepNums,
                hypStepNum - hypSubProofLen + 2, hypStepNum + 1));
      } /* Next i */
      if (k != conclSubProofLen - 1 /* && returnFlag < 2 */) {
                        /* Uncomment above if bad proof triggers this bug */
        bug(1731);
      }
      nmbrLet(&rearrangedSubProofs, NULL_NMBRSTRING);
      matchingHyp = -1; /* In case there are no hypotheses */
      for (i = 0; i < numReqHyp; i++) {
        matchingHyp = -1;
        for (k = 0; k < numReqHyp; k++) {
          m = wrkProof.RPNStackPtr - numReqHyp + k; /* Stack position of hyp */
          hypStepNum = wrkProof.RPNStack[m]; /* Step number of hypothesis k */


          /* Temporarily zap the token's end with a null for string comparisons */
          fbPtr = targetPntr[hypStepNum];
          zapSave = fbPtr[targetNmbr[hypStepNum]];
          fbPtr[targetNmbr[hypStepNum]] = 0; /* Zap source */
          /* See if hypothesis i matches the target label k i.e. hypStepNum */
          if (!strcmp(statement[nmbrTmpPtr[i]].labelName, fbPtr)) {
            matchingHyp = k;
          }
          fbPtr[targetNmbr[hypStepNum]] = zapSave; /* Unzap source */
          if (matchingHyp != -1) break;
        } /* next k (0 to numReqHyp-1) */
        if (matchingHyp == -1) {
          if (!wrkProof.errorCount) {
            sourceError(fbPtr, 1/*token length*/, statemNum, cat(
                "The target labels for the hypotheses for step ", str((double)step + 1),
                " do not match hypothesis \"",
                statement[nmbrTmpPtr[i]].labelName,
                "\" of the assertion \"",
                statement[j].labelName,
                "\" in step ",  str((double)step + 1), ".",
                NULL));
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          break; /* Give up; don't try to rearrange hypotheses */
        }
        /* Accumulate the subproof for hypothesis i */
        nmbrLet(&rearrangedSubProofs, nmbrCat(rearrangedSubProofs,
            reqHypSubProof[matchingHyp], NULL));
        nmbrLet(&rearrangedOldStepNums, nmbrCat(rearrangedOldStepNums,
            reqHypOldStepNums[matchingHyp], NULL));
      } /* next i (0 to numReqHyp-1) */

      if (matchingHyp != -1) { /* All hypotheses found */
        if (nmbrLen(rearrangedSubProofs) != conclSubProofLen - 1
             /* && returnFlag < 2 */) {
                          /* Uncomment above if bad proof triggers this bug */
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

      /* Reassign wrkProof.proofString from rearranged wrkProofString */
      for (i = 0; i < step; i++) {
        /* Nothing from step to end has been changed, so stop at step */
        (wrkProof.proofString)[i] = wrkProofString[i];
      }
      if ((wrkProof.proofString)[step] != wrkProofString[step]) bug(1735);

      /* Deallocate */
      for (i = 0; i < numReqHyp; i++) {
        nmbrLet((nmbrString **)(&(reqHypSubProof[i])), NULL_NMBRSTRING);
        nmbrLet((nmbrString **)(&(reqHypOldStepNums[i])), NULL_NMBRSTRING);
      }
      pntrLet(&reqHypSubProof, NULL_PNTRSTRING);
      pntrLet(&reqHypOldStepNums, NULL_PNTRSTRING);
      nmbrLet(&rearrangedSubProofs, NULL_NMBRSTRING);
      nmbrLet(&rearrangedOldStepNums, NULL_NMBRSTRING);
      nmbrLet(&wrkProofString, NULL_NMBRSTRING);
    } /* if explicitTargets */
    /**** End of 25-Jan-2016 ***/

    /* Error check for $e <- $f assignments (illegal) */
    nmbrTmpPtr = statement[j].reqHypList;
    numReqHyp = statement[j].numReqHyp;
    for (i = 0; i < numReqHyp; i++) {

      /* 25-Jan-2016 nm */
      /* Skip this check if /EXPLICIT since hyps may be in random order */
      if (explicitTargets == 1) break;

      if (statement[nmbrTmpPtr[i]].type == e_) {
        m = wrkProof.RPNStackPtr - numReqHyp + i;
        k = wrkProof.proofString[wrkProof.RPNStack[m]];
        if (k > 0) {
          if (statement[k].type == f_) {
            if (!wrkProof.errorCount) {
              sourceError(fbPtr, tokLength, statemNum, cat(
                  "Statement \"",statement[j].labelName,"\" (proof step ",
                  str((double)step + 1),
                  ") has its \"$e\" hypothesis \"",
                  statement[nmbrTmpPtr[i]].labelName,
                  "\" assigned the \"$f\" hypothesis \"",
                  statement[k].labelName,
                  "\" at step ", str((double)(wrkProof.RPNStack[m] + 1)),
                  ".  The assignment of \"$e\" with \"$f\" is not allowed.",
                  NULL));
            }
            wrkProof.errorCount++;
            if (returnFlag < 2) returnFlag = 2;
          }
        }
      }
    }

    /* Pop the stack */
    wrkProof.RPNStackPtr = wrkProof.RPNStackPtr - numReqHyp;
    wrkProof.RPNStack[wrkProof.RPNStackPtr] = step;
    wrkProof.RPNStackPtr++;

  } /* Next step */

  /* The stack should have one entry */
  if (wrkProof.RPNStackPtr != 1) {
    tmpStrPtr = shortDumpRPNStack();
    fbPtr = wrkProof.stepSrcPtrPntr[wrkProof.numSteps - 1];
    if (!wrkProof.errorCount) {
      sourceError(fbPtr, proofTokenLen(fbPtr), statemNum, cat("After proof step ",
          str((double)(wrkProof.numSteps))," (the last step), the ",
          tmpStrPtr,".  It should contain exactly one entry.",NULL));
    }
    wrkProof.errorCount++;
    if (returnFlag < 3) returnFlag = 3;
  }

  /**** Start of 25-Jan-2016 nm ***/
  if (explicitTargets) {
    /* Correct the local label refs in the rearranged proof */
    for (step = 0; step < wrkProof.numSteps; step++) {
      /* This is slow lookup with n^2 behavior, but should be ok since
         /EXPLICIT isn't used that often */
      k = (wrkProof.proofString)[step]; /* Will be <= -1000 if local label */
      if (k <= -1000) {
        k = -1000 - k; /* Restore step number */
        if (k < 0 || k >= wrkProof.numSteps) bug(1733);
        /* Find the original step */
        if (oldStepNums[k] == k) {
            /* Wasn't changed, so skip renumbering for speedup */
          continue;
        }
        i = 0; /* For bug check */
        /* Find the original step number and change it to the new one */
        for (m = 0; m < wrkProof.numSteps; m++) {
          if (oldStepNums[m] == k) {
            (wrkProof.proofString)[step] = -1000 - m;
            i = 1; /* Found */
            break;
          }
        }
        if (i == 0) bug(1740);
      }
    } /* next step */

    /************** Start of section deleted 10-Mar-2016 nm
    /@ Check if any local labels point to future steps: if so, we should
       expand the proof so it will verify (since many functions require
       that a local label be declared before it is used) @/
    for (step = 0; step < wrkProof.numSteps; step++) {
      k = (wrkProof.proofString)[step]; /@ Will be <= -1000 if local label @/
      if (k <= -1000) { /@ References local label i.e. subproof @/
        k = -1000 - k; /@ Restore step number subproof ends at @/
        if (k > step) { /@ Refers to label declared after this step @/
          /@ Expand the proof @/
          nmbrLet(&wrkProofString, nmbrUnsquishProof(wrkProof.proofString));
          /@ Recompress the proof @/
          nmbrLet(&wrkProofString, nmbrSquishProof(wrkProofString));
          /@ The number of steps shouldn't have changed @/
          /@ (If this bug is valid behavior, it means we may have to
             reallocate (grow) the wrkProof structure, which might be
             unpleasant at this point.) @/
          if (nmbrLen(wrkProofString) != wrkProof.numSteps) {
            bug(1736);
          }
          /@ Reassign wrkProof.proofString from new wrkProofString @/
          for (i = 0; i < wrkProof.numSteps; i++) {
            (wrkProof.proofString)[i] = wrkProofString[i];
          }
          break;
        } /@ if k>step @/
      } /@ if k<= -1000 @/
    } /@ next step @/
    ************************* end of 10-Mar-2016 deletion */


    /* 10-Mar-2016 nm (Replace above deleted secton) */
    /* Check if any local labels point to future steps: if so, we should
       moved the subproof they point to down (since many functions require
       that a local label be declared before it is used) */
    do {
      subProofMoved = 0;  /* Flag to rescan after moving a subproof */
      /* TODO: restart loop after step just finished for speedup?
         (maybe not worth it).
         We could just change subProofMoved to restartStep (long) and use
         restartStep > 0 as the flag, since we would restart at the
         last step processed plus 1. */
      for (step = 0; step < wrkProof.numSteps; step++) {
        k = (wrkProof.proofString)[step]; /* Will be <= -1000 if local label */
        if (k <= -1000) { /* References local label i.e. subproof */
          k = -1000 - k; /* Restore step number subproof ends at */
          if (k > step) { /* Refers to label declared after this step */
            m = subproofLen(wrkProof.proofString, k);
            /*m = nmbrGetSubProofLen(wrkProof.proofString, k);*/

            /* At this point:
                   step = the step referencing a future subproof
                   k = end of future subproof
                   m = length of future subproof */

            /* TODO - make this a direct assignment for speedup?
               (with most $f's reversed, this has about 13K hits during
               'verify proof *' - maybe not enough to justify rewriting this
               to make direct assignment instead of nmbrLet().) */
            /* Replace the step with the subproof it references */
            /* Note that nmbrXxx() positions start at 1, not 0; add 1 to step */
            nmbrLet(&wrkProofString, nmbrCat(
                /* Proof before future label ref: */
                nmbrLeft(wrkProof.proofString, step),
                /* The future subproof moved to the current step: */
                nmbrMid(wrkProof.proofString, k - m + 2, m),
                /* The steps between this step and the future subproof: */
                nmbrSeg(wrkProof.proofString, step + 2, k - m + 1),
                /* The future subproof replaced with the current step (a local
                   label to be renumbered below): */
                nmbrMid(wrkProof.proofString, step + 1, 1),
                /* The rest of the steps to the end of proof: */
                nmbrRight(wrkProof.proofString, k + 2),
                NULL));
            if (nmbrLen(wrkProofString) != wrkProof.numSteps) {
              bug(1736); /* Make sure proof length didn't change */
            }
            if (wrkProofString[k] != (wrkProof.proofString)[step]) {
              bug(1737); /* Make sure future subproof is now the future local
                            label (to be renumbered below) */
            }

            /* We now have this wrkProofString[...] content:
                [0]...[step-1]                   same as original proof
                [step+1]...[step+m-1]            moved subproof
                [step+m]...[k-1]                 shifted orig proof
                [k]...[k]                        subproof replaced by loc label
                [k+1]...[wrkProof.numSteps-1]    same as orig proof */

            /* Correct all local labels */
            for (i = 0; i < wrkProof.numSteps; i++) {
              j = (wrkProofString)[i]; /* Will be <= -1000 if local label */
              if (j > -1000) continue; /* Not loc label ref */
              j = -1000 - j; /* Restore step number subproof ends at */
              /* Note: the conditions before the "&&" below are redundant
                 but provide better sanity checking */
              if (j >= 0 && j < step) { /* Before moved subproof */
                /*j = j;*/ /* Same as orig proof */
                /* 24-Mar-2016 workaround to clang complaint about j = j */
                j = j + 0; /* Same as orig proof */
              } else if (j == step) { /* The original local label ref */
                bug(1738); /* A local label shouldn't ref a local label */
              } else if (j > step && j <= k - m) {
                                   /* Steps shifted up by subproof insertion */
                j = j + m - 1; /* Offset by size of subproof moved down -1 */
              } else if (j > k - m && j <= k) {
                                   /* Reference to inside the moved subproof */
                j = j + step + m - 1 - k;  /* Shift down */
              } else if (j > k && j <= wrkProof.numSteps - 1) {
                                              /* Ref to after moved subproof */
                /*j = j;*/ /* Same as orig proof */
                /* 24-Mar-2016 workaround to clang complaint about j = j */
                j = j + 0; /* Same as orig proof */
              } else {
                bug(1739);  /* Cases not exhausted or j is out of range */
              }
              (wrkProofString)[i] = -j - 1000; /* Update new proof */
            } /* next i */

            /* Transfer proof back to original */
            for (i = 0; i < wrkProof.numSteps; i++) {
              (wrkProof.proofString)[i] = wrkProofString[i];
            }

            /* Set flag that a subproof was moved and restart the scan */
            subProofMoved = 1;
            /* Reassign wrkProof.proofString from new wrkProofString */
            for (i = 0; i < wrkProof.numSteps; i++) {
              (wrkProof.proofString)[i] = wrkProofString[i];
            }
            break; /* Break out of the 'for (step...' loop */

          } /* if k>step */
        } /* if k<= -1000 */
      } /* next step */
    } while (subProofMoved);

    /* Deallocate */
    pntrLet(&targetPntr, NULL_PNTRSTRING);
    nmbrLet(&targetNmbr, NULL_NMBRSTRING);
    nmbrLet(&oldStepNums, NULL_NMBRSTRING);
    nmbrLet(&wrkProofString, NULL_NMBRSTRING);
  } /* if (explicitTargets) */
  /**** End of 25-Jan-2016 ***/

  wrkProof.errorSeverity = returnFlag;
  return (returnFlag);

} /* parseProof() */



/* Parse proof in compressed format */
/* Parse proof of one statement in source file.  Uses wrkProof structure. */
/* Returns 0 if OK; returns 1 if proof is incomplete (is empty or has '?'
   tokens);  returns 2 if error found; returns 3 if severe error found
   (e.g. RPN stack violation); returns 4 if not a $p statement */
char parseCompressedProof(long statemNum)
{

  long i, j, k, m, step, stmt;
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
  void *voidPtr; /* bsearch returned value */
  vstring tmpStrPtr;
  flag hypLocUnkFlag;  /* Hypothesis, local label ref, or unknown step */
  long labelMapIndex;

  static unsigned char chrWeight[256]; /* Proof label character weights */
  static unsigned char chrType[256]; /* Proof character types */
  static flag chrTablesInited = 0;
  static char *digits = "0123456789";
  static char *letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
  static char labelChar = ':';
  static long lettersLen;
  static long digitsLen;

  /* 15-Oct-05 nm - Used to detect old buggy compression */
  long bggyProofLen;
  char bggyZapSave;
  flag bggyAlgo;

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  labelStart = "";

  /* Do one-time initialization */
  if (!chrTablesInited) {
    chrTablesInited = 1;

    /* Compression standard with all cap letters */
    /* (For 500-700 step proofs, we only lose about 18% of file size --
        but the compressed proof is more pleasing to the eye) */
    letters = "ABCDEFGHIJKLMNOPQRST"; /* LSB is base 20 */
    digits = "UVWXY"; /* MSB's are base 5 */
    labelChar = 'Z'; /* Was colon */

    lettersLen = (long)strlen(letters);
    digitsLen = (long)strlen(digits);

    /* Initialize compressed proof label character weights */
    /* Initialize compressed proof character types */
    for (i = 0; i < 256; i++) {
      chrWeight[i] = 0;
      chrType[i] = 6; /* Illegal */
    }
    j = lettersLen;
    for (i = 0; i < j; i++) {
      chrWeight[(long)(letters[i])] = (unsigned char)i;
      chrType[(long)(letters[i])] = 0; /* Letter */
    }
    j = digitsLen;
    for (i = 0; i < j; i++) {
      chrWeight[(long)(digits[i])] = (unsigned char)i;
      chrType[(long)(digits[i])] = 1; /* Digit */
    }
    for (i = 0; i < 256; i++) {
      if (isspace(i)) chrType[i] = 3; /* White space */
    } /* Next i */
    chrType[(long)(labelChar)] = 2; /* Colon */
    chrType['$'] = 4; /* Dollar */
    chrType['?'] = 5; /* Question mark */
  }


  if (statement[statemNum].type != p_) {
    bug(1724); /* 13-Oct-05 nm - should never get here */
    return (4); /* Do nothing if not $p */
  }
  fbPtr = statement[statemNum].proofSectionPtr; /* Start of proof section */
  if (fbPtr[0] == 0) { /* The proof was never assigned (could be a $p statement
                          with no $=; this would have been detected earlier) */
    bug(1711);
  }
  fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  if (fbPtr[0] != '(') { /* ( is flag for start of compressed proof */
    bug(1712);
  }

  /* Make sure we have enough working space to hold the proof */
  /* The worst case is less than the number of chars in the source,
     plus the number of active hypotheses */

  numOptHyp = nmbrLen(statement[statemNum].optHypList);
  if (statement[statemNum].proofSectionLen + statement[statemNum].numReqHyp
      + numOptHyp > wrkProofMaxSize) {
    if (wrkProofMaxSize) { /* Not the first allocation */
      free(wrkProof.tokenSrcPtrNmbr);
      free(wrkProof.tokenSrcPtrPntr);
      free(wrkProof.stepSrcPtrNmbr);
      free(wrkProof.stepSrcPtrPntr);
      free(wrkProof.localLabelFlag);
      free(wrkProof.hypAndLocLabel);
      free(wrkProof.localLabelPool);
      poolFree(wrkProof.proofString);
      free(wrkProof.mathStringPtrs);
      free(wrkProof.RPNStack);
      free(wrkProof.compressedPfLabelMap);
    }
    wrkProofMaxSize = statement[statemNum].proofSectionLen
        + statement[statemNum].numReqHyp + numOptHyp;
    wrkProof.tokenSrcPtrNmbr = malloc((size_t)wrkProofMaxSize
        * sizeof(nmbrString));
    wrkProof.tokenSrcPtrPntr = malloc((size_t)wrkProofMaxSize
        * sizeof(pntrString));
    wrkProof.stepSrcPtrNmbr = malloc((size_t)wrkProofMaxSize
        * sizeof(nmbrString));
    wrkProof.stepSrcPtrPntr = malloc((size_t)wrkProofMaxSize
        * sizeof(pntrString));
    wrkProof.localLabelFlag = malloc((size_t)wrkProofMaxSize
        * sizeof(flag));
    wrkProof.hypAndLocLabel =
        malloc((size_t)wrkProofMaxSize * sizeof(struct sortHypAndLoc));
    wrkProof.localLabelPool = malloc((size_t)wrkProofMaxSize);
    wrkProof.proofString =
        poolFixedMalloc(wrkProofMaxSize * (long)(sizeof(nmbrString)));
         /* Use poolFixedMalloc instead of poolMalloc so that it won't get
            trimmed by memUsedPoolPurge. */
    wrkProof.mathStringPtrs =
        malloc((size_t)wrkProofMaxSize * sizeof(nmbrString));
    wrkProof.RPNStack = malloc((size_t)wrkProofMaxSize * sizeof(nmbrString));
    wrkProof.compressedPfLabelMap =
         malloc((size_t)wrkProofMaxSize * sizeof(nmbrString));
    if (!wrkProof.tokenSrcPtrNmbr ||
        !wrkProof.tokenSrcPtrPntr ||
        !wrkProof.stepSrcPtrNmbr ||
        !wrkProof.stepSrcPtrPntr ||
        !wrkProof.localLabelFlag ||
        !wrkProof.hypAndLocLabel ||
        !wrkProof.localLabelPool ||
        /* !wrkProof.proofString || */ /* Redundant because of poolMalloc */
        !wrkProof.mathStringPtrs ||
        !wrkProof.RPNStack
        ) outOfMemory("#99 (wrkProof)");
  }

  /* Initialization for this proof */
  wrkProof.errorCount = 0; /* Used as threshold for how many error msgs/proof */
  wrkProof.numSteps = 0;
  wrkProof.numTokens = 0;
  wrkProof.numHypAndLoc = 0;
  wrkProof.numLocalLabels = 0;
  wrkProof.RPNStackPtr = 0;
  wrkProof.localLabelPoolPtr = wrkProof.localLabelPool;

  fbPtr++;
  /* fbPtr points to the first token now. */


  /****** This part of the code is heavily borrowed from the regular
   ****** proof parsing, with local label and RPN handling removed,
   ****** in order to easily parse the label section. */

  /* First break up the label section of proof into tokens */
  while (1) {
    fbPtr = fbPtr + whiteSpaceLen(fbPtr);
    tokLength = proofTokenLen(fbPtr);
    if (!tokLength) {
      if (!wrkProof.errorCount) {
        sourceError(fbPtr, 2, statemNum,
            "A \")\" which ends the label list is not present.");
      }
      wrkProof.errorCount++;
      if (returnFlag < 3) returnFlag = 3;
      break;
    }
    if (fbPtr[0] == ')') {  /* End of label list */
      fbPtr++;
      break;
    }
    wrkProof.stepSrcPtrPntr[wrkProof.numSteps] = fbPtr;
    wrkProof.stepSrcPtrNmbr[wrkProof.numSteps] = tokLength;
    wrkProof.numSteps++;
    fbPtr = fbPtr + tokLength;
  }

  fbStartProof = fbPtr; /* Save pointer to start of compressed proof */

  /* Copy active (opt + req) hypotheses to hypAndLocLabel look-up table */
  nmbrTmpPtr = statement[statemNum].optHypList;
  /* Transfer optional hypotheses */
  while (1) {
    i = nmbrTmpPtr[wrkProof.numHypAndLoc];
    if (i == -1) break;
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelTokenNum = i;
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelName =
        statement[i].labelName;
    wrkProof.numHypAndLoc++;
  }
  /* Transfer required hypotheses */
  j = statement[statemNum].numReqHyp;
  nmbrTmpPtr = statement[statemNum].reqHypList;
  for (i = 0; i < j; i++) {
    k = nmbrTmpPtr[i];
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelTokenNum = -1000 - k;
       /* Required hypothesis labels are not allowed; the -1000 - k is a
          flag that tells that they are required for error detection */
    wrkProof.hypAndLocLabel[wrkProof.numHypAndLoc].labelName =
        statement[k].labelName;
    wrkProof.numHypAndLoc++;
  }

  /* Sort the hypotheses by label name for lookup */
  qsort(wrkProof.hypAndLocLabel, (size_t)(wrkProof.numHypAndLoc),
      sizeof(struct sortHypAndLoc), hypAndLocSortCmp);

  /* Build the proof string (actually just a list of labels) */
  wrkProof.proofString[wrkProof.numSteps] = -1; /* End of proof */
  nmbrZapLen(wrkProof.proofString, wrkProof.numSteps);
     /* Zap mem pool actual length (because nmbrLen will be used later on this)*/

  /* Scan proof string with the label list (not really proof steps; we're just
     using the structure for convenience) */
  for (step = 0; step < wrkProof.numSteps; step++) {
    tokLength = wrkProof.stepSrcPtrNmbr[step];
    fbPtr = wrkProof.stepSrcPtrPntr[step];

    /* Temporarily zap the token's end with a null for string comparisons */
    zapSave = fbPtr[tokLength];
    fbPtr[tokLength] = 0; /* Zap source */

    /* See if the proof token is a hypothesis */
    voidPtr = (void *)bsearch(fbPtr, wrkProof.hypAndLocLabel,
        (size_t)(wrkProof.numHypAndLoc), sizeof(struct sortHypAndLoc),
        hypAndLocSrchCmp);
    if (voidPtr) {
      /* It's a hypothesis reference */
      fbPtr[tokLength] = zapSave; /* Unzap source */
      j = ((struct sortHypAndLoc *)voidPtr)->labelTokenNum;
                                                       /* Label lookup number */

      /* Make sure it's not a required hypothesis, which is implicitly
         declared */
      if (j < 0) { /* Minus is used as flag for required hypothesis */
        j = -1000 - j; /* Restore it to prevent side effects of the error */
        if (!wrkProof.errorCount) {
          sourceError(fbPtr, tokLength, statemNum,
              "Required hypotheses may not be explicitly declared.");
        }
        wrkProof.errorCount++;
        /* if (returnFlag < 2) returnFlag = 2; */
        /* 19-Aug-2006 nm Tolerate this error so we can continue to work
           on proof in Proof Assistant */
        if (returnFlag < 1) returnFlag = 1;
      }

      wrkProof.proofString[step] = j; /* Proof string */
      if (j <= 0) bug(1713);
      continue;
    } /* End if hypothesis */

    /* See if token is an assertion label */
    voidPtr = (void *)bsearch(fbPtr, labelKeyBase, (size_t)numLabelKeys,
        sizeof(long), labelSrchCmp);
    fbPtr[tokLength] = zapSave; /* Unzap source */
    if (!voidPtr) {
      if (!wrkProof.errorCount) {
        sourceError(fbPtr, tokLength, statemNum,
         "This token is not the label of an assertion or optional hypothesis.");
      }
      wrkProof.errorCount++;
      wrkProof.proofString[step] = -(long)'?';
      if (returnFlag < 2) returnFlag = 2;
      continue;
    }

    /* It's an assertion ($a or $p) */
    j = *(long *)voidPtr; /* Statement number */
    if (statement[j].type != a_ && statement[j].type != p_) bug(1714);
    wrkProof.proofString[step] = j; /* Proof string */

    if (j >= statemNum) { /* Error */
      if (!wrkProof.errorCount) {
        if (j == statemNum) {
          sourceError(fbPtr, tokLength, statemNum, cat(
              "The label at proof step ",
              str((double)step + 1),
             " is the label of this statement.  A statement may not be used to",
              " prove itself.",NULL));
        } else {
          assignStmtFileAndLineNum(j); /* 9-Jan-2018 nm */
          sourceError(fbPtr, tokLength, statemNum, cat(
              "The label \"", statement[j].labelName, "\" at proof step ",
              str((double)step + 1),
              " is the label of a future statement (at line ",
              str((double)(statement[j].lineNum)),
              " in file ",statement[j].fileName,
              ").  Only previous statements may be referenced.",
              NULL));
        }
      }
      wrkProof.errorCount++;
      if (returnFlag < 2) returnFlag = 2;
    }

  } /* Next step */

  /******* Create the starting label map (local labels will be
           added as they are found) *****/
  wrkProof.compressedPfNumLabels = statement[statemNum].numReqHyp;
  nmbrTmpPtr = statement[statemNum].reqHypList;
  for (i = 0; i < wrkProof.compressedPfNumLabels; i++) {
    wrkProof.compressedPfLabelMap[i] = nmbrTmpPtr[i];
  }
  for (i = 0; i < wrkProof.numSteps; i++) {
    wrkProof.compressedPfLabelMap[i + wrkProof.compressedPfNumLabels] =
        wrkProof.proofString[i];
  }
  wrkProof.compressedPfNumLabels = wrkProof.compressedPfNumLabels +
      wrkProof.numSteps;

  /* Re-initialization for the actual proof */
  wrkProof.numSteps = 0;
  wrkProof.RPNStackPtr = 0;

  /******* Parse the compressed part of the proof *****/

  /* 15-Oct-05 nm - Check to see if the old buggy compression is used.  If so,
     warn the user to reformat, and switch to the buggy algorithm so that
     parsing can procede. */
  bggyProofLen = statement[statemNum].proofSectionLen -
             (fbPtr - statement[statemNum].proofSectionPtr);
  /* Zap a zero at the end of the proof so we can use C string operations */
  bggyZapSave = fbPtr[bggyProofLen];
  fbPtr[bggyProofLen] = 0;
  /* If the proof has "UVA" but doesn't have "UUA", it means the buggy
     algorithm was used. */
  bggyAlgo = 0;
  if (strstr(fbPtr, "UV") != NULL) {
    if (strstr(fbPtr, "UU") == NULL) {
      bggyAlgo = 1;
      print2("?Warning: the proof of \"%s\" uses obsolete compression.\n",
          statement[statemNum].labelName);
      print2(" Please SAVE PROOF * / COMPRESSED to reformat your proofs.\n");
    }
  }
  fbPtr[bggyProofLen] = bggyZapSave;

  /* (Build the proof string and check the RPN stack) */
  fbPtr = fbStartProof;
  breakFlag = 0;
  labelMapIndex = 0;
  while (1) {
    switch (chrType[(long)(fbPtr[0])]) {
      case 0: /* "Letter" (i.e. A...T) */
        if (!labelMapIndex) labelStart = fbPtr; /* Save for error msg */

        /* Save pointer to source file vs. step for error messages */
        tokLength = fbPtr - labelStart + 1; /* Token length */
        wrkProof.stepSrcPtrNmbr[wrkProof.numSteps] = tokLength;
        wrkProof.stepSrcPtrPntr[wrkProof.numSteps] = labelStart; /* Token ptr */

        /* 15-Oct-05 nm - Obsolete (skips from YT to UVA, missing UUA) */
        /* (actually, this part is coincidentally the same:)
        labelMapIndex = labelMapIndex * lettersLen +
            chrWeight[(long)(fbPtr[0])];
        */
        /* 15-Oct-05 nm - Corrected algorithm provided by Marnix Klooster. */
        /* Decoding can be done as follows:
             * n := 0
             * for each character c:
                * if c in ['U'..'Y']: n := n * 5 + (c - 'U' + 1)
                * if c in ['A'..'T']: n := n * 20 + (c - 'A' + 1) */
        labelMapIndex = labelMapIndex * lettersLen +
            chrWeight[(long)(fbPtr[0])];
        if (labelMapIndex >= wrkProof.compressedPfNumLabels) {
          if (!wrkProof.errorCount) {
            sourceError(labelStart, tokLength, statemNum, cat(
     "This compressed label reference is outside the range of the label list.",
                "  The compressed label value is ", str((double)labelMapIndex),
                " but the largest label defined is ",
                str((double)(wrkProof.compressedPfNumLabels - 1)), ".", NULL));
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          labelMapIndex = 0; /* Make it something legal to avoid side effects */
        }

        stmt = wrkProof.compressedPfLabelMap[labelMapIndex];
        wrkProof.proofString[wrkProof.numSteps] = stmt;

        /* Update stack */
        hypLocUnkFlag = 0;
        if (stmt < 0) { /* Local label or '?' */
          hypLocUnkFlag = 1;
        } else {
          if (statement[stmt].type != (char)a_ &&
              statement[stmt].type != (char)p_) hypLocUnkFlag = 1;
                                                               /* Hypothesis */
        }
        if (hypLocUnkFlag) { /* Hypothesis, local label ref, or unknown step */
          wrkProof.RPNStack[wrkProof.RPNStackPtr] = wrkProof.numSteps;
          wrkProof.RPNStackPtr++;
        } else { /* An assertion */

          /* It's a valid assertion, so pop the stack */
          numReqHyp = statement[stmt].numReqHyp;

          /* Error check for exhausted stack */
          if (wrkProof.RPNStackPtr < numReqHyp) { /* Stack exhausted -- error */
            if (!wrkProof.errorCount) {
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
                  str((double)(wrkProof.numSteps + 1)),", statement \"",
                  statement[stmt].labelName,"\" requires ",
                  tmpStrPtr,".",NULL));
              let(&tmpStrPtr, "");
            }
            /* Treat it like an unknown step so stack won't get exhausted */
            wrkProof.errorCount++;
            wrkProof.proofString[wrkProof.numSteps] = -(long)'?';
            /* Push the stack */
            wrkProof.RPNStack[wrkProof.RPNStackPtr] = wrkProof.numSteps;
            wrkProof.RPNStackPtr++;
            if (returnFlag < 3) returnFlag = 3;
            continue;
          } /* End if stack exhausted */

          /* Error check for $e <- $f assignments (illegal) */
          nmbrTmpPtr = statement[stmt].reqHypList;
          numReqHyp = statement[stmt].numReqHyp;
          for (i = 0; i < numReqHyp; i++) {
            if (statement[nmbrTmpPtr[i]].type == e_) {
              m = wrkProof.RPNStackPtr - numReqHyp + i;
              k = wrkProof.proofString[wrkProof.RPNStack[m]];
              if (k > 0) {
                if (statement[k].type == f_) {
                  if (!wrkProof.errorCount) {
                    sourceError(fbPtr, tokLength, statemNum, cat(
                        "Statement \"", statement[stmt].labelName,
                        "\" (proof step ",
                        str((double)(wrkProof.numSteps + 1)),
                        ") has its \"$e\" hypothesis \"",
                        statement[nmbrTmpPtr[i]].labelName,
                        "\" assigned the \"$f\" hypothesis \"",
                        statement[k].labelName,
                        "\" at step ", str((double)(wrkProof.RPNStack[m] + 1)),
                      ".  The assignment of \"$e\" with \"$f\" is not allowed.",
                        NULL));
                  }
                  wrkProof.errorCount++;
                  if (returnFlag < 2) returnFlag = 2;
                }
              }
            }
          }

          /* Pop the stack */
          wrkProof.RPNStackPtr = wrkProof.RPNStackPtr - numReqHyp;
          wrkProof.RPNStack[wrkProof.RPNStackPtr] = wrkProof.numSteps;
          wrkProof.RPNStackPtr++;

        }

        wrkProof.numSteps++;
        labelMapIndex = 0; /* Reset it for next label */
        break;

      case 1: /* "Digit" (i.e. U...Y) */
        /* 15-Oct-05 nm - Obsolete (skips from YT to UVA, missing UUA) */
        /*
        if (!labelMapIndex) {
          / * First digit; mod digitsLen+1 * /
          labelMapIndex = chrWeight[(long)(fbPtr[0])] + 1;
          labelStart = fbPtr; / * Save label start for error msg * /
        } else {
          labelMapIndex = labelMapIndex * digitsLen +
              chrWeight[(long)(fbPtr[0])];
        }
        */
        /* 15-Oct-05 nm - Corrected algorithm provided by Marnix Klooster. */
        /* Decoding can be done as follows:
             * n := 0
             * for each character c:
                * if c in ['U'..'Y']: n := n * 5 + (c - 'U' + 1)
                * if c in ['A'..'T']: n := n * 20 + (c - 'A' + 1) */
        if (!labelMapIndex) {
          labelMapIndex = chrWeight[(long)(fbPtr[0])] + 1;
          labelStart = fbPtr; /* Save label start for error msg */
        } else {
          labelMapIndex = labelMapIndex * digitsLen +
              chrWeight[(long)(fbPtr[0])] + 1;
          if (bggyAlgo) labelMapIndex--; /* Adjust for buggy algorithm */
        }
        break;

      case 2: /* "Colon" (i.e. Z) */
        if (labelMapIndex) { /* In the middle of some digits */
          if (!wrkProof.errorCount) {
            sourceError(fbPtr, 1, statemNum,
             "A compressed label character was expected here.");
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          labelMapIndex = 0;
        }

        /* Put local label in label map */
        wrkProof.compressedPfLabelMap[wrkProof.compressedPfNumLabels] =
          -1000 - (wrkProof.numSteps - 1);
        wrkProof.compressedPfNumLabels++;

        hypLocUnkFlag = 0;

        /* 21-Mar-06 nm Fix bug reported by o'cat */
        if (wrkProof.numSteps == 0) {
          /* This will happen if labelChar (Z) is in 1st char pos of
             compressed proof */
          if (!wrkProof.errorCount) {
            sourceError(fbPtr, 1, statemNum, cat(
              "A local label character must occur after a proof step.",NULL));
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          /* We want to break here to prevent out of bound wrkProof.proofString
             index below. */
          break;
        }

        stmt = wrkProof.proofString[wrkProof.numSteps - 1];
        if (stmt < 0) { /* Local label or '?' */
          hypLocUnkFlag = 1;
        } else {
          if (statement[stmt].type != (char)a_ &&
              statement[stmt].type != (char)p_) hypLocUnkFlag = 1;
                                                                /* Hypothesis */
        }
        if (hypLocUnkFlag) { /* Hypothesis, local label ref, or unknown step */
          /* If local label references a hypothesis or other local label,
             it is not allowed because it complicates the language but doesn't
             buy anything; would make $e to $f assignments harder to detect */
          if (!wrkProof.errorCount) {
            sourceError(fbPtr, 1, statemNum, cat(
                "The hypothesis or local label reference at proof step ",
                str((double)(wrkProof.numSteps)),
                " declares a local label.  Only \"$a\" and \"$p\" statement",
                " labels may have local label declarations.",NULL));
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        }

        break;

      case 3: /* White space */
        break;

      case 4: /* Dollar */
        /* See if we're at the end of the statement */
        if (fbPtr[1] == '.') {
          breakFlag = 1;
          break;
        }
        /* Otherwise, it must be a comment */
        if (fbPtr[1] != '(' && fbPtr[1] != '!') {
          if (!wrkProof.errorCount) {
            sourceError(fbPtr + 1, 1, statemNum,
             "Expected \".\", \"(\", or \"!\" here.");
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
        } else {
          fbPtr = fbPtr + whiteSpaceLen(fbPtr) - 1; /* -1 because
                  fbPtr will get incremented at end of loop */
        }
        break;

      case 5: /* Question mark */
        if (labelMapIndex) { /* In the middle of some digits */
          if (!wrkProof.errorCount) {
            sourceError(fbPtr, 1, statemNum,
             "A compressed label character was expected here.");
          }
          wrkProof.errorCount++;
          if (returnFlag < 2) returnFlag = 2;
          labelMapIndex = 0;
        }

        /* Save pointer to source file vs. step for error messages */
        wrkProof.stepSrcPtrNmbr[wrkProof.numSteps] = 1;
        wrkProof.stepSrcPtrPntr[wrkProof.numSteps] = fbPtr; /* Token ptr */

        wrkProof.proofString[wrkProof.numSteps] = -(long)'?';
        /* returnFlag = 1 means that proof has unknown steps */
        /* 6-Oct-05 nm Ensure that a proof with unknown steps doesn't
           reset the severe error flag if returnFlag > 1 */
        /* returnFlag = 1; */ /*bad - resets severe error flag*/
        if (returnFlag < 1) returnFlag = 1; /* 6-Oct-05 */

        /* Update stack */
        wrkProof.RPNStack[wrkProof.RPNStackPtr] = wrkProof.numSteps;
        wrkProof.RPNStackPtr++;

        wrkProof.numSteps++;
        break;

      case 6: /* Illegal */
        if (!wrkProof.errorCount) {
          sourceError(fbPtr, 1, statemNum,
           "This character is not legal in a compressed proof.");
        }
        wrkProof.errorCount++;
        if (returnFlag < 2) returnFlag = 2;
        break;
      default:
        bug(1715);
        break;
    } /* End switch chrType[fbPtr[0]] */

    if (breakFlag) break;
    fbPtr++;

  } /* End while (1) */

  if (labelMapIndex) { /* In the middle of some digits */
    if (!wrkProof.errorCount) {
      sourceError(fbPtr, 1, statemNum,
       "A compressed label character was expected here.");
    }
    wrkProof.errorCount++;
    if (returnFlag < 2) returnFlag = 2;
    /* labelMapIndex = 0; */ /* 18-Sep-2013 never used */
  }

  /* If proof is empty, make it have one unknown step */
  if (wrkProof.numSteps == 0) {

    /* For now, this is an error. */
    if (!wrkProof.errorCount) {
      sourceError(fbPtr, 2, statemNum,
          "The proof is empty.  If you don't know the proof, make it a \"?\".");
    }
    wrkProof.errorCount++;

    /* Save pointer to source file vs. step for error messages */
    wrkProof.stepSrcPtrNmbr[wrkProof.numSteps] = 1;
    wrkProof.stepSrcPtrPntr[wrkProof.numSteps] = fbPtr; /* Token ptr */

    wrkProof.proofString[wrkProof.numSteps] = -(long)'?';
    /* 21-Mar-06 nm Deleted 2 lines below; added 3rd - there could be a
       previous error; see 21-Mar-06 entry above */
    /* if (returnFlag > 0) bug(1722); */ /* 13-Oct-05 nm */
    /* returnFlag = 1; */ /* Flag that proof has unknown steps */
    if (returnFlag < 1) returnFlag = 1; /* Flag that proof has unknown steps */

    /* Update stack */
    wrkProof.RPNStack[wrkProof.RPNStackPtr] = wrkProof.numSteps;
    wrkProof.RPNStackPtr++;

    wrkProof.numSteps++;
    /* 13-Oct-05 nm The line below is redundant */
    /*if (returnFlag < 1) returnFlag = 1;*/ /* Flag for proof with unknown steps */
  }

  wrkProof.proofString[wrkProof.numSteps] = -1; /* End of proof */
  nmbrZapLen(wrkProof.proofString, wrkProof.numSteps);
    /* Zap mem pool actual length (because nmbrLen will be used later on this)*/

  /* The stack should have one entry */
  if (wrkProof.RPNStackPtr != 1) {
    tmpStrPtr = shortDumpRPNStack();
    fbPtr = wrkProof.stepSrcPtrPntr[wrkProof.numSteps - 1];
    if (!wrkProof.errorCount) {
      sourceError(fbPtr, proofTokenLen(fbPtr), statemNum,
          cat("After proof step ",
          str((double)(wrkProof.numSteps))," (the last step), the ",
          tmpStrPtr,".  It should contain exactly one entry.",NULL));
    }
    wrkProof.errorCount++;
   if (returnFlag < 3) returnFlag = 3;
  }

  wrkProof.errorSeverity = returnFlag;
  return (returnFlag);

} /* parseCompressedProof */


/* 11-Sep-2016 nm */
/* The caller must deallocate the returned nmbrString! */
/* This function just gets the proof so the caller doesn't have to worry
   about cleaning up the wrkProof structure.  The returned proof is normal
   or compressed depending on the .mm source; called nmbrUnsquishProof() to
   make sure it is uncompressed if required. */
/* If there is a severe error in the proof, a 1-step proof with "?" will
   be returned. */
/* If printFlag = 1, then error messages are printed, otherwise they aren't.
   This is only partially implemented; some errors may still result in a
   printout.  TODO: pass printFlag to parseProof(), verifyProof() */
/* TODO: use this function to simplify some code that calls parseProof
   directly. */
nmbrString *getProof(long statemNum, flag printFlag) {
  nmbrString *proof = NULL_NMBRSTRING;
  parseProof(statemNum);
  /* We do not need verifyProof() since we don't care about the math
     strings for the proof steps in this function. */
  /* verifyProof(statemNum); */ /* Necessary to set RPN stack ptrs
                             before calling cleanWrkProof() */
  if (wrkProof.errorSeverity > 1) {
    if (printFlag) print2(
         "The starting proof has a severe error.  It will not be used.\n");
    nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING, -(long)'?'));
  } else {
    nmbrLet(&proof, wrkProof.proofString);
  }
  /* Note: the wrkProof structure is never deallocated but grows to
     accomodate the largest proof found so far.  The ERASE command will
     deallocate it, though.   cleanWrkProof() just deallocates math strings
     assigned by verifyProof() that aren't needed by this function. */
  /* cleanWrkProof(); */ /* Deallocate verifyProof() storage */
  return proof;
} /* getProof */



/* 2-Feb-2018 nm - lineNum and fileName are now computed by
   getFileAndLineNum() */
void rawSourceError(char *startFile, char *ptr, long tokLen,
    /*long lineNum, vstring fileName,*/ vstring errMsg)
{
  char *startLine;
  char *endLine;
  vstring errLine = "";
  vstring errorMsg = "";

  /* 2-Feb-2018 nm */
  vstring fileName = "";
  long lineNum;

  let(&errorMsg, errMsg); /* Prevent deallocation of errMsg */

  /* 2-Feb-2018 nm */
  fileName = getFileAndLineNum(startFile/*=sourcePtr*/, ptr, &lineNum);

  /* Get the line with the error on it */
  startLine = ptr;
  while (startLine[0] != '\n' && startLine > startFile) {
    startLine--;
  }
  if (startLine[0] == '\n'
      && startLine != ptr) /* 8/20/04 nm In case of 0-length line */
    startLine++; /* Go to 1st char on line */
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
  let(&errLine, "");
  let(&errorMsg, "");
  let(&fileName, ""); /* 2-Feb-2018 nm */
} /* rawSourceError */

/* The global sourcePtr is assumed to point to the start of the raw input
     buffer.
   The global sourceLen is assumed to be length of the raw input buffer.
   The global includeCall array is referenced. */
void sourceError(char *ptr, long tokLen, long stmtNum, vstring errMsg)
{
  char *startLine;
  char *endLine;
  vstring errLine = "";
  long lineNum;
  vstring fileName = "";
  vstring errorMsg = "";

  /* 3-May-2017 nm */
  /* Used for the case where a source file section has been modified */
  char *locSourcePtr;
  /*long locSourceLen;*/

  /* 3-May-2017 nm */
  /* Initialize local pointers to raw input source */
  locSourcePtr = sourcePtr;
  /*locSourceLen = sourceLen;*/

  let(&errorMsg, errMsg); /* errMsg may become deallocated if this function is
                         called with a string function argument (cat, etc.) */

  if (!stmtNum) {
    lineNum = 0;
    goto SKIP_LINE_NUM; /* This isn't a source file parse */
  }
  if (ptr < sourcePtr || ptr > sourcePtr + sourceLen) {
    /* The pointer is outside the raw input buffer, so it must be a
       SAVEd proof or other overwritten section, so there is no line number. */
    /* 3-May-2017 nm */
    /* Reassign the beginning and end of the source pointer to the
       changed section */
    if (statement[stmtNum].labelSectionChanged == 1
         && ptr >= statement[stmtNum].labelSectionPtr
         && ptr <= statement[stmtNum].labelSectionPtr
             + statement[stmtNum].labelSectionLen) {
      locSourcePtr = statement[stmtNum].labelSectionPtr;
      /*locSourceLen = statement[stmtNum].labelSectionLen;*/
    } else if (statement[stmtNum].mathSectionChanged == 1
         && ptr >= statement[stmtNum].mathSectionPtr
         && ptr <= statement[stmtNum].mathSectionPtr
             + statement[stmtNum].mathSectionLen) {
      locSourcePtr = statement[stmtNum].mathSectionPtr;
      /*locSourceLen = statement[stmtNum].mathSectionLen;*/
    } else if (statement[stmtNum].proofSectionChanged == 1
         && ptr >= statement[stmtNum].proofSectionPtr
         && ptr <= statement[stmtNum].proofSectionPtr
             + statement[stmtNum].proofSectionLen) {
      locSourcePtr = statement[stmtNum].proofSectionPtr;
      /*locSourceLen = statement[stmtNum].proofSectionLen;*/
    } else {
      /* ptr points to neither the original source nor a modified section */
      bug(1741);
    }

    lineNum = 0;
    goto SKIP_LINE_NUM;
  }

  /* 9-Jan-2018 nm */
  /*let(&fileName, "");*/ /* No need - already assigned to empty string */
  fileName = getFileAndLineNum(locSourcePtr/*=sourcePtr here*/, ptr, &lineNum);

 SKIP_LINE_NUM:
  /* Get the line with the error on it */
  if (lineNum != 0 && ptr > locSourcePtr) {
    startLine = ptr - 1; /* Allows pointer to point to \n. */
  } else {
    /* Special case:  Error message starts at beginning of file or
       the beginning of a changed section. */
    /* Or, it's a non-source statement; must not point to \n. */
    startLine = ptr;
  }


  /**** 3-May-2017 nm Deleted
  /@ Scan back to beginning of line with error @/
  while (startLine[0] != '\n' && (!lineNum || startLine > locSourcePtr)
      /@ ASCII 1 flags start of SAVEd proof @/
      && (lineNum || startLine[0] != 1)
      /@ lineNum is 0 (e.g. no stmt); stop scan at beg. of file
         or beginning of a changed section @/
      && startLine != locSourcePtr) {
  ***/

  /* 3-May-2017 nm */
  /* Scan back to beginning of line with error */
  while (startLine[0] != '\n' && startLine > locSourcePtr) {

    startLine--;
  }
  /* if (startLine[0] == '\n' || startLine[0] == 1) startLine++; */
  /* 3-May-2017 nm */
  if (startLine[0] == '\n') startLine++;

  /* Scan forward to end of line with error */
  endLine = ptr;
  while (endLine[0] != '\n' && endLine[0] != 0) {
    endLine++;
  }
  endLine--;

  /* Save line with error (with no newline on it) */
  let(&errLine, space(endLine - startLine + 1));
  memcpy(errLine, startLine, (size_t)(endLine - startLine) + 1);

  if (!lineNum) {
    /* Not a source file parse */
    errorMessage(errLine, lineNum, ptr - startLine + 1, tokLen, errorMsg,
        NULL, stmtNum, (char)error_);
  } else {
    errorMessage(errLine, lineNum,
        ptr - startLine + 1, tokLen, /* column */
        errorMsg,
        /*includeCall[i].source_fn,*/
        fileName,
        stmtNum,
        (char)error_ /* severity */);
  }
  let(&errLine, "");
  let(&errorMsg, "");
  let(&fileName, "");
} /* sourceError */


void mathTokenError(long tokenNum /* 0 is 1st one */,
    nmbrString *tokenList, long stmtNum, vstring errMsg)
{
  long i;
  char *fbPtr;
  fbPtr = statement[stmtNum].mathSectionPtr;
  fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  /* Scan past the tokens before the desired one. */
  /* We use the parsed token length rather than tokenLen() to
     account for adjacent tokens with no white space. */
  for (i = 0; i < tokenNum; i++) {
    fbPtr = fbPtr + mathToken[tokenList[i]].length;
    fbPtr = fbPtr + whiteSpaceLen(fbPtr);
  }
  sourceError(fbPtr, mathToken[tokenList[tokenNum]].length,
      stmtNum, errMsg);
} /* mathTokenError */

vstring shortDumpRPNStack(void) {
  /* The caller must deallocate the returned string. */
  vstring tmpStr = "";
  vstring tmpStr2 = "";
  long i, k, m;

  for (i = 0; i < wrkProof.RPNStackPtr; i++) {
     k = wrkProof.RPNStack[i]; /* Step */
     let(&tmpStr,space(wrkProof.stepSrcPtrNmbr[k]));
     memcpy(tmpStr,wrkProof.stepSrcPtrPntr[k],
         (size_t)(wrkProof.stepSrcPtrNmbr[k])); /* Label at step */
     let(&tmpStr2,cat(
         tmpStr2,", ","\"",tmpStr,"\" (step ", str((double)k + 1),")",NULL));
  }
  let(&tmpStr2,right(tmpStr2,3));
  if (wrkProof.RPNStackPtr == 2) {
    m = instr(1, tmpStr2, ",");
    let(&tmpStr2,cat(left(tmpStr2,m - 1)," and ",
        right(tmpStr2,m + 1),NULL));
  }
  if (wrkProof.RPNStackPtr > 2) {
    for (m = (long)strlen(tmpStr2); m > 0; m--) { /* Find last comma */
      if (tmpStr2[m - 1] == ',') break;
    }
    let(&tmpStr2,cat(left(tmpStr2,m - 1),", and ",
        right(tmpStr2,m + 1),NULL));
  }
  if (wrkProof.RPNStackPtr == 1) {
    let(&tmpStr2,cat("one entry, ",tmpStr2,NULL));
  } else {
    let(&tmpStr2,cat(str((double)(wrkProof.RPNStackPtr))," entries: ",tmpStr2,NULL));
  }
  let(&tmpStr2,cat("RPN stack contains ",tmpStr2,NULL));
  if (wrkProof.RPNStackPtr == 0) let(&tmpStr2,"RPN stack is empty");
  let(&tmpStr, "");
  return(tmpStr2);
} /* shortDumpRPNStack */

/* 10/10/02 */
/* ???Todo:  use this elsewhere in mmpars.c to modularize this lookup */
/* Lookup $a or $p label and return statement number.
   Return -1 if not found. */
long lookupLabel(vstring label)
{
  void *voidPtr; /* bsearch returned value */
  long statemNum;
  /* Find the statement number */
  voidPtr = (void *)bsearch(label, labelKeyBase, (size_t)numLabelKeys,
      sizeof(long), labelSrchCmp);
  if (!voidPtr) {
    return (-1);
  }
  statemNum = (*(long *)voidPtr); /* Statement number */
  if (statement[statemNum].type != a_ && statement[statemNum].type != p_)
      bug(1718);
  return (statemNum);
} /* lookupLabel */


/* Label comparison for qsort */
int labelSortCmp(const void *key1, const void *key2)
{
  /* Returns -1 if key1 < key2, 0 if equal, 1 if key1 > key2 */
  return (strcmp(statement[ *((long *)key1) ].labelName,
      statement[ *((long *)key2) ].labelName));
} /* labelSortCmp */


/* Label comparison for bsearch */
int labelSrchCmp(const void *key, const void *data)
{
  /* Returns -1 if key < data, 0 if equal, 1 if key > data */
  return (strcmp(key,
      statement[ *((long *)data) ].labelName));
} /* labelSrchCmp */


/* Math symbol comparison for qsort */
int mathSortCmp(const void *key1, const void *key2)
{
  /* Returns -1 if key1 < key2, 0 if equal, 1 if key1 > key2 */
  return (strcmp(mathToken[ *((long *)key1) ].tokenName,
      mathToken[ *((long *)key2) ].tokenName));
}


/* Math symbol comparison for bsearch */
/* Here, key is pointer to a character string. */
int mathSrchCmp(const void *key, const void *data)
{
  /* Returns -1 if key < data, 0 if equal, 1 if key > data */
  return (strcmp(key, mathToken[ *((long *)data) ].tokenName));
}


/* Hypotheses and local label comparison for qsort */
int hypAndLocSortCmp(const void *key1, const void *key2)
{
  /* Returns -1 if key1 < key2, 0 if equal, 1 if key1 > key2 */
  return (strcmp( ((struct sortHypAndLoc *)key1)->labelName,
      ((struct sortHypAndLoc *)key2)->labelName));
}


/* Hypotheses and local label comparison for bsearch */
/* Here, key is pointer to a character string. */
int hypAndLocSrchCmp(const void *key, const void *data)
{
  /* Returns -1 if key < data, 0 if equal, 1 if key > data */
  return (strcmp(key, ((struct sortHypAndLoc *)data)->labelName));
}


/* This function returns the length of the white space starting at ptr.
   Comments are considered white space.  ptr should point to the first character
   of the white space.  If ptr does not point to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned. */
long whiteSpaceLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  char *ptr1;
  while (1) {
    tmpchr = ptr[i];
    if (!tmpchr) return (i); /* End of string */
    if (tmpchr == '$') {
      if (ptr[i + 1] == '(') {
        while (1) {
          /*ptr1 = strchr(ptr + i + 2, '$'); */
          /* in-line code for speed */
          /* (for the lcc-win32 compiler, this speeds it up from 94 sec
              for set.mm read to 4 sec) */
          for (ptr1 = ptr + i + 2; ptr1[0] != '$'; ptr1++) {
            if (ptr1[0] == 0) {
              if ('$' != 0)
                ptr1 = NULL;
              break;
            }
          }
          /* end in-line strchr code */
          if (!ptr1) {
            return(i + (long)strlen(&ptr[i])); /* Unterminated comment - goto EOF */
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
        return(i);
      }
    } /* if (tmpchr == '$') */
    if (isgraph((unsigned char)tmpchr)) return (i);
    i++;
  }
  return(0); /* Dummy return - never happens */
} /* whiteSpaceLen */


/* 31-Dec-2017 nm For .mm file splitting */
/* This function is like whiteSpaceLen() except that comments are NOT
   considered white space.  ptr should point to the first character
   of the white space.  If ptr does not point to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned. */
long rawWhiteSpaceLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  while (1) {
    tmpchr = ptr[i];
    if (!tmpchr) return (i); /* End of string */
    if (isgraph((unsigned char)tmpchr)) return (i);
    i++;
  }
  return(0); /* Dummy return - never happens */
} /* rawWhiteSpaceLen */


/* This function returns the length of the token (non-white-space) starting at
   ptr.  Comments are considered white space.  ptr should point to the first
   character of the token.  If ptr points to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned.  If ptr
   points to a keyword, 0 is returned.  A keyword ends a token. */
/* Tokens may be of the form "$nn"; this is tolerated (used in parsing
   user math strings in parseMathTokens()).  An (illegal) token of this form
   in the source will be detected earlier, so this won't cause
   syntax violations to slip by in the source. */
long tokenLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  while (1) {
    tmpchr = ptr[i];
    if (tmpchr == '$') {
      if (ptr[i + 1] == '$') { /* '$$' character */
        i = i + 2;
        continue;
      } else {
        /* Tolerate digit after "$" */
        if (ptr[i + 1] >= '0' && ptr[i + 1] <= '9') {
          i = i + 2;
          continue;
        } else {
          return(i); /* Keyword or comment */
        }
      }
    }
    if (!isgraph((unsigned char)tmpchr)) return (i); /* White space or null */
    i++;
  }
  return(0); /* Dummy return (never happens) */
} /* tokenLen */


/* This function returns the length of the token (non-white-space) starting at
   ptr.  Comments are considered white space.  ptr should point to the first
   character of the token.  If ptr points to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned. */
/* Unlike tokenLen(), keywords are not treated as special.  In particular:
   if ptr points to a keyword, 0 is NOT returned (instead, 2 is returned),
   and a keyword does NOT end a token (which is a relic of days before
   whitespace surrounding a token was part of the spec, but still serves
   a useful purpose in token() for friendlier error detection). */
long rawTokenLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  while (1) {
    tmpchr = ptr[i];
    if (!isgraph((unsigned char)tmpchr)) return (i); /* White space or null */
    i++;
  }
  return(0); /* Dummy return (never happens) */
} /* rawTokenLen */


/* This function returns the length of the proof token starting at
   ptr.  Comments are considered white space.  ptr should point to the first
   character of the token.  If ptr points to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned.  If ptr
   points to a keyword, 0 is returned.  A keyword ends a token.
   ":" and "?" and "(" and ")" and "=" (25-Jan-2016) are considered tokens. */
long proofTokenLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  if (ptr[0] == ':') return (1); /* The token is a colon */
  if (ptr[0] == '?') return (1); /* The token is a "?" */
  if (ptr[0] == '(') return (1); /* The token is a "(" (compressed proof) */
  if (ptr[0] == ')') return (1); /* The token is a ")" (compressed proof) */
  /* 25-Jan-2016 nm */
  if (ptr[0] == '=') return (1); /* The token is a "=" (/EXPLICIT proof) */
  while (1) {
    tmpchr = ptr[i];
    if (tmpchr == '$') {
      if (ptr[i + 1] == '$') { /* '$$' character */
        i = i + 2;
        continue;
      } else {
        return(i); /* Keyword or comment */
      }
    }
    if (!isgraph((unsigned char)tmpchr)) return (i); /* White space or null */
    if (tmpchr == ':') return(i); /* Colon ends a token */
    if (tmpchr == '?') return(i); /* "?" ends a token */
    if (tmpchr == '(') return(i); /* "(" ends a token */
    if (tmpchr == ')') return(i); /* ")" ends a token */
    if (tmpchr == '=') return(i); /* "=" ends a token */ /* 25-Jan-2016 nm */
    i++;
  }
  return(0); /* Dummy return - never happens */
}


/* 9-Jan-2018 nm */
/* Counts the number of \n between start for length chars.
   If length = -1, then use end-of-string 0 to stop.
   If length >= 0, then scan at most length chars, but stop
       if end-of-string 0 is found. */
long countLines(vstring start, long length) {
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
} /* countLines */


/* Return (for output) the complete contents of a statement, including all
   white space and comments, from first token through all white space and
   comments after last token. */
/* This allows us to modify the input file with Metamath. */
/* Note: the text near end of file is obtained from statement[statements
   + 1] */
/* ???This does not yet implement restoration of the various input files;
      all included files are merged into one. */
/* Caller must deallocated returned string. */
/* reformatFlag= 0: WRITE SOURCE, 1: WRITE SOURCE / REFORMAT,
   2: WRITE SOURCE / WRAP */
/* Note that the labelSection, mathSection, and proofSection do not
   contain keywords ($a, $p,...; $=; $.).  The keywords are added
   by this function when the statement is written. */
vstring outputStatement(long stmt, /*flag cleanFlag, 3-May-2017 removed */
    flag reformatFlag)
{
  vstring labelSection = "";
  vstring mathSection = "";
  vstring proofSection = "";
  vstring labelSectionSave = "";
  vstring mathSectionSave = "";
  vstring proofSectionSave = "";
  vstring output = "";
  /* flag newProofFlag; */ /* deleted 3-May-2017 nm */
  /* For reformatting: */
  long pos;
  long indent;
  static long dollarDpos = 0;
  static char previousType = illegal_;  /* '?' in mmdata.h */
  long commentStart;
  long commentEnd;
  vstring comment = "";
  vstring str1 = "";
  long length;

  /* For getContribs in-line error insertion to assist massive corrections
  long i;
  vstring ca = "", cd = "", ra = "", rd = "", sa = "", sd = "", md = "";
  long saveWidth;
  */


  let(&labelSection, space(statement[stmt].labelSectionLen));
  memcpy(labelSection, statement[stmt].labelSectionPtr,
      (size_t)(statement[stmt].labelSectionLen));

  if (stmt == statements + 1) return labelSection; /* Special case - EOF */

  /******* 3-May-2017 nm  "/CLEAN" is no longer supported
  /@ 1-Jul-2011 nm @/
  if (cleanFlag) {
    /@ cleanFlag = 1: User is removing theorems with $( [?] $) dates @/
    /@ Most of the WRITE SOURCE / CLEAN processing is done in the
       writeInput() that calls this.  Here, we just remove any
       $( [?} $) date comment missed by that algorithm. @/
    if (labelSection[0] == '\n') { /@ True unless user edited source @/
      pos = instr(1, labelSection, "$( [?] $)");
      if (pos != 0) {
        pos = instr(pos + 9, labelSection, "\n");
        if (pos != 0) {
          /@ Use pos instead of pos + 1 so that we include the \n @/
          let(&labelSection, right(labelSection, pos));
        }
      }
    }
  }
  ************/

  let(&mathSection, space(statement[stmt].mathSectionLen));
  memcpy(mathSection, statement[stmt].mathSectionPtr,
      (size_t)(statement[stmt].mathSectionLen));

  let(&proofSection, space(statement[stmt].proofSectionLen));
  memcpy(proofSection, statement[stmt].proofSectionPtr,
      (size_t)(statement[stmt].proofSectionLen));

  /* 31-Dec-2017 nm */
  let(&labelSectionSave, labelSection);
  let(&mathSectionSave, mathSection);
  let(&proofSectionSave, proofSection);


  /* 12-Jun-2011 nm Added this section to reformat statements to match the
     current set.mm convention */
  if (reformatFlag > 0) {  /* 1 = WRITE SOURCE / FORMAT or 2 = / REWRAP */
    /* Put standard indentation before the ${, etc. */
#define INDENT_FIRST 2
#define INDENT_INCR 2
    indent = INDENT_FIRST + (INDENT_INCR * statement[stmt].scope);
    /* statement[stmt].scope is at start of stmt not end; adjust $} */
    if (statement[stmt].type == rb_) indent = indent - INDENT_INCR;

    /* 9-Jul-2011 nm Added */
    /* Untab the label section */
    if (strchr(labelSection, '\t') != NULL) { /* Only if has tab (speedup) */
      let(&labelSection, edit(labelSection, 2048/*untab*/));
    }
    /* Untab the math section */
    if (strchr(mathSection, '\t') != NULL) { /* Only if has tab (speedup) */
      let(&mathSection, edit(mathSection, 2048/*untab*/));
    }

    /* Reformat the label section */

    /* Remove spaces a end of line */
    /* This is a pretty inefficient loop, but hopefully lots of spaces
       at end of lines is a rare occurrence */
    while (1) {
      pos = instr(1, labelSection, " \n");
      if (pos == 0) break;
      let(&labelSection, cat(left(labelSection, pos - 1),
          right(labelSection, pos + 1), NULL));
    }

    /* Don't allow more than 2 consecutive blank lines */
    while (1) {
      pos = instr(1, labelSection, "\n\n\n\n");
      if (pos == 0) break;
      let(&labelSection, cat(left(labelSection, pos - 1),
          right(labelSection, pos + 1), NULL));
    }

    switch (statement[stmt].type) {
      case lb_: /* ${ */
      case rb_: /* $} */
      case v_: /* $v */
      case c_: /* $c */
      case d_: /* $d */
        /* Get the last newline */
        pos = rinstr(labelSection, "\n");
        /* If there is none, insert it (unless first line in file) */
        if (pos == 0 && stmt > 1) {
          let(&labelSection, cat(edit(labelSection, 128 /* trailing spaces */),
              "\n", NULL));
          pos = (long)strlen(labelSection) + 1;
        }
        /* Put a blank line between $} and ${ if there is none */
        if (stmt > 1) {
          if (pos == 1 && statement[stmt].type == lb_
              && statement[stmt - 1].type == rb_) {
            let(&labelSection, "\n\n");
            pos = 2;
          }
        }
        let(&labelSection, cat(left(labelSection, pos),
            space(indent), NULL));
        if (statement[stmt].type == d_) {
          let(&mathSection, edit(mathSection,
              4 /* discard LF */ + 16 /* reduce spaces */));
          if (previousType == d_) {
            /* See if the $d can be added to the current line */
            if (dollarDpos + 2 + (signed)(strlen(mathSection)) + 4
                <= screenWidth) {
              let(&labelSection, "  ");  /* 2 spaces between $d's */
              dollarDpos = dollarDpos + 2 + (long)strlen(mathSection) + 4;
            } else {
              /* Add 4 = '$d' length + '$.' length */
              dollarDpos = indent + (long)strlen(mathSection) + 4;
            }
          } else {
            dollarDpos = indent + (long)strlen(mathSection) + 4;
          }
        }
        break;
      case a_:    /* $a */
      case p_:    /* $p */
        /* Get last $( */
        commentStart = rinstr(labelSection,  "$(");
        /* Get last $) */
        commentEnd = rinstr(labelSection, "$)") + 1;
        if (commentEnd < commentStart) bug(1725);
        if (commentStart != 0) {
          let(&comment, seg(labelSection, commentStart, commentEnd));
        } else {
          /* If there is no comment before $a or $p, add dummy comment */
          let(&comment, "$( PLEASE PUT DESCRIPTION HERE. $)");
        }

        /* 4-Nov-2015 The section below is for a one-time attribution in
           definitions and should be commented out for normal use. */
        /******* TODO: DELETE THIS SOMEDAY *********/
        /*********
        long j;
        if (statement[stmt].type == a_
             && instr(1, comment, "(Contributed") == 0
             && (!strcmp(left(statement[stmt].labelName, 3), "df-")
               || !strcmp(left(statement[stmt].labelName, 3), "ax-"))) {
          let(&str1, "");
          str1 = traceUsage(stmt, 0, 0);
          for (i = 1; i <= statements; i++) {
            if (str1[i] == 'Y') break;
          }
          if (i >= statements) {
             let(&ca, "??");
             let(&cd, cat("??", "-???", "-????", NULL));
          } else {

            /@ 3-May-2017 nm (not tested because code is commented out) @/
            let(&ca, "");
            ca = getContrib(i, CONTRIBUTOR);
            let(&cd, "");
            cd = getContrib(i, CONTRIB_DATE);
            let (&rd, "");
            rd = getContrib(i, REVISE_DATE);

            /@@@@@@@@@ deleted 3-May-2017
            getContrib(i, &ca, &cd, &ra, &rd, &sa, &sd, &md, 0);
            @@@@@@/

            if (cd[0] == 0) {
              let(&ca, "??");
              getProofDate(i, &cd, &rd);
              if (rd[0]) let(&cd, rd);
              if (cd[0] == 0) {
                let(&cd, cat("??", "-???", "-????", NULL));
              }
            }
          }
          let(&comment, cat(left(comment, (long)strlen(comment) - 2),
              " (Contributed by ", ca, ", ", cd, ".) $)", NULL));
          let(&ca, "");
          let(&cd, "");
          let(&ra, "");
          let(&rd, "");
          let(&sa, "");
          let(&sd, "");
          let(&str1, "");
        }

        if (statement[stmt].type == p_
           && instr(1, comment, "(Contributed") == 0) {
          getProofDate(stmt, &cd, &rd);
          if (rd[0]) let(&cd, rd);
          if (cd[0] == 0) {
            let(&cd, cat("??", "-???", "-????", NULL));
          }

          i = instr(1, comment, "(Revised") - 1;
          if (i <= 0) i = (long)strlen(comment);
          j = instr(1, comment, "(Proof shorten") - 1;
          if (j <= 0) j = (long)strlen(comment);

          if (j < i) i = j;
          if ((long)strlen(comment) - 2 < i) i = (long)strlen(comment) - 2;

          let(&ca, "??");
          let(&comment, cat(left(comment, i - 1),
              " (Contributed by ", ca, ", ", cd, ".) ", right(comment, i),
              NULL));
          let(&ca, "");
          let(&cd, "");
          let(&ra, "");
          let(&rd, "");
          let(&sa, "");
          let(&sd, "");
          let(&str1, "");
        }
        ************/

        let(&labelSection, left(labelSection, commentStart - 1));
        /* Get the last newline before the comment */
        pos = rinstr(labelSection, "\n");

        /* 9-Jul-2011 nm Added */
        /* If previous statement was $e, take out any blank line */
        if (previousType == e_ && pos == 2 && labelSection[0] == '\n') {
          let(&labelSection, right(labelSection, 2));
          pos = 1;
        }

        /* If there is no '\n', insert it (unless first line in file) */
        if (pos == 0 && stmt > 1) {
          let(&labelSection, cat(edit(labelSection, 128 /* trailing spaces */),
              "\n", NULL));
          pos = (long)strlen(labelSection) + 1;
        }
        let(&labelSection, left(labelSection, pos));

        /* If / REWRAP was specified, unwrap and rewrap the line */
        if (reformatFlag == 2) {
          let(&str1, "");
          str1 = rewrapComment(comment);
          let(&comment, str1);
        }

        /* Make sure that start of new lines inside the comment have no
           trailing space (because printLongLine doesn't do this after
           explict break) */
        pos = 0;
        while (1) {
          pos = instr(pos + 1, comment, "\n");
          if (pos == 0) break;
          length = 0;
          while (1) {  /* Get past any existing leading blanks */
            if (comment[pos + length] != ' ') break;
            length++;
          }
          let(&comment, cat(left(comment, pos),
              (comment[pos + length] != '\n')
                  ? space(indent + 3)
                  : "",  /* Don't add indentation if line is blank */
              right(comment, pos + length + 1), NULL));
        }

        /* Reformat the comment to wrap if necessary */
        if (outputToString == 1) bug(1726);
        outputToString = 1;
        let(&printString, "");
        /* 7-Nov-2015 nm For getContribs in-line error insertion to assist
           massive corrections; maybe delete someday */
        /***********
        saveWidth = screenWidth;
        screenWidth = 9999;
        /@i=getContrib(stmt, &ca, &cd, &ra, &rd, &sa, &sd, &md, 1);@/
        let(&ca, "");
        /@ 3-May-2017 nm @/
        ca = getContrib(stmt, ERROR_CHECK);
        screenWidth = saveWidth;
        ************/
        printLongLine(cat(space(indent), comment, NULL),
            space(indent + 3), " ");
        let(&comment, printString);
        let(&printString, "");
        outputToString = 0;
#define ASCII_4 4
        /* Restore ASCII_4 characters put in by rewrapComment() to space */
        length = (long)strlen(comment);
        for (pos = 2; pos < length - 2; pos++) {
           /* For debugging: */
           /* if (comment[pos] == ASCII_4) comment[pos] = '#'; */
           if (comment[pos] == ASCII_4) comment[pos] = ' ';
        }
        /* Remove any trailing spaces */
        pos = 2;
        while(1) {
          pos = instr(pos + 1, comment, " \n");
          if (!pos) break;
          let(&comment, cat(left(comment, pos - 1), right(comment, pos + 1),
              NULL));
          pos = pos - 2;
        }

        /* Complete the label section */
        let(&labelSection, cat(labelSection, comment,
            space(indent), statement[stmt].labelName, " ", NULL));
        break;
      case e_:    /* $e */
      case f_:    /* $f */
        pos = rinstr(labelSection, statement[stmt].labelName);
        let(&labelSection, left(labelSection, pos - 1));
        pos = rinstr(labelSection, "\n");
        /* If there is none, insert it (unless first line in file) */
        if (pos == 0 && stmt > 1) {
          let(&labelSection, cat(edit(labelSection, 128 /* trailing spaces */),
              "\n", NULL));
          pos = (long)strlen(labelSection) + 1;
        }
        let(&labelSection, left(labelSection, pos));
        /* If previous statement is $d or $e and there is no comment after it,
           discard entire rest of label to get rid of redundant blank lines */
        if (stmt > 1) {
          if ((statement[stmt - 1].type == d_
                || statement[stmt - 1].type == e_)
              && instr(1, labelSection, "$(") == 0) {
            let(&labelSection, "\n");
          }
        }
        /* Complete the label section */
        let(&labelSection, cat(labelSection,
            space(indent), statement[stmt].labelName, " ", NULL));
        break;
      default: bug(1727);
    } /* switch (statement[stmt].type) */

    /* Reformat the math section */
    switch (statement[stmt].type) {
      case lb_: /* ${ */
      case rb_: /* $} */
      case v_: /* $v */
      case c_: /* $c */
      case d_: /* $d */
      case a_: /* $a */
      case p_: /* $p */
      case e_: /* $e */
      case f_: /* $f */
        /* Remove blank lines */
        while (1) {
          pos = instr(1, mathSection, "\n\n");
          if (pos == 0) break;
          let(&mathSection, cat(left(mathSection, pos),
              right(mathSection, pos + 2), NULL));
        }

        /* 6-Mar-2016 nm Turn off wrapping of math section.  It should be
           done manually for best readability. */
        /*
        /@ Remove leading and trailing space and trailing new lines @/
        while(1) {
          let(&mathSection, edit(mathSection,
              8 /@ leading sp @/ + 128 /@ trailing sp @/));
          if (mathSection[strlen(mathSection) - 1] != '\n') break;
          let(&mathSection, left(mathSection, (long)strlen(mathSection) - 1));
        }
        let(&mathSection, cat(" ", mathSection, " ", NULL));
                   /@ Restore standard leading/trailing space stripped above @/
        */

        /* Reduce multiple in-line spaces to single space */
        pos = 0;
        while(1) {
          pos = instr(pos + 1, mathSection, "  ");
          if (pos == 0) break;
          if (pos > 1) {
            if (mathSection[pos - 2] != '\n' && mathSection[pos - 2] != ' ') {
              /* It's not at the start of a line, so reduce it */
              let(&mathSection, cat(left(mathSection, pos),
                  right(mathSection, pos + 2), NULL));
              pos--;
            }
          }
        }

        /* 6-Mar-2016 nm Turn off wrapping of math section.  It should be
           done manually for best readability. */
        /*
        /@ Wrap long lines @/
        length = indent + 2 /@ Prefix length - add 2 for keyword ${, etc. @/
            /@ Add 1 for space after label, if $e, $f, $a, $p @/
            + (((statement[stmt].labelName)[0]) ?
                ((long)strlen(statement[stmt].labelName) + 1) : 0);
        if (outputToString == 1) bug(1728);
        outputToString = 1;
        let(&printString, "");
        printLongLine(cat(space(length), mathSection, "$.", NULL),
            space(indent + 4), " ");
        outputToString = 0;
        let(&mathSection, left(printString, (long)strlen(printString) - 3));
            /@ Trim off "$." plus "\n" @/
        let(&mathSection, right(mathSection, length + 1));
        let(&printString, "");
        */

        break;
      default: bug(1729);
    }

    /* Set previous state for next statement */
    if (statement[stmt].type == d_) {
      /* dollarDpos is computed in the processing above */
    } else {
      dollarDpos = 0; /* Reset it */
    }
    previousType = statement[stmt].type;
    /* let(&comment, ""); */  /* Deallocate string memory */ /* (done below) */
  } /* if reformatFlag */

  let(&output, labelSection);

  /* Add statement keyword */
  let(&output, cat(output, "$", chr(statement[stmt].type), NULL));

  /* Add math section and proof */
  if (statement[stmt].mathSectionLen != 0) {
    let(&output, cat(output, mathSection, NULL));
    /* newProofFlag = 0; */  /* deleted 3-May-2017 nm */

    if (statement[stmt].type == (char)p_) {
      let(&output, cat(output, "$=", proofSection, NULL));

      /******** deleted 3-May-2017 nm
      if (statement[stmt].proofSectionPtr[-1] == 1) {
        /@ ASCII 1 is flag that line is not from original source file @/
        newProofFlag = 1; /@ Proof is result of SAVE (NEW_)PROOF command @/
      }
    }
    /@ If it's not a source file line, the proof text should supply the
       statement terminator, so that additional text may be added after
       the terminator if desired.  (I.e., date in SAVE NEW_PROOF command) @/
    if (!newProofFlag) let(&output, cat(output, "$.", NULL));
    ********/

    /* 3-May-2017 nm */
    }
    let(&output, cat(output, "$.", NULL));

  }

  /* Added 10/24/03 */
  /* Make sure the line has no carriage-returns */
  if (strchr(output, '\r') != NULL) {

    /* 31-Dec-2017 nm */
    /* We are now using readFileToString, so this should never happen. */
    bug(1758);

    /* This may happen with Cygwin's gcc, where DOS CR-LF becomes CR-CR-LF
       in the output file */
    /* Someday we should investigate the use of readFileToString() in
       mminou.c for the main set.mm READ function, to solve this cleanly. */
    let(&output, edit(output, 8192)); /* Discard CR's */
  }

  /* 31-Dec-2017 nm */
  /* This function is no longer used to supply the output, but just to
     do any reformatting/wrapping.  Now writeSourceToBuffer() builds the
     output source.  So instead, we update the statement[] content with
     any changes, which are read by writeSourceToBuffer() and also saved
     in the statement[] array for any future write source.  Eventually
     we should replace WRITE SOURCE.../REWRAP with a REWRAP(?) command. */
  if (strcmp(labelSection, labelSectionSave)) {
    statement[stmt].labelSectionLen = (long)strlen(labelSection);
    if (statement[stmt].labelSectionChanged == 1) {
      let(&(statement[stmt].labelSectionPtr), labelSection);
    } else {
      /* This is the first time we've updated the label section */
      statement[stmt].labelSectionChanged = 1;
      statement[stmt].labelSectionPtr = labelSection;
      labelSection = ""; /* so that labelSectionPtr won't be deallocated */
    }
  }
  if (strcmp(mathSection, mathSectionSave)) {
    statement[stmt].mathSectionLen = (long)strlen(mathSection);
    if (statement[stmt].mathSectionChanged == 1) {
      let(&(statement[stmt].mathSectionPtr), mathSection);
    } else {
      /* This is the first time we've updated the math section */
      statement[stmt].mathSectionChanged = 1;
      statement[stmt].mathSectionPtr = mathSection;
      mathSection = ""; /* so that mathSectionPtr won't be deallocated */
    }
  }
  /* (I don't see anywhere that proofSection will change.  So make
     it a bug check to force us to look into it.) */
  if (strcmp(proofSection, proofSectionSave)) {
    bug(1757); /* This may not be a bug */
    statement[stmt].proofSectionLen = (long)strlen(proofSection);
    if (statement[stmt].proofSectionChanged == 1) {
      let(&(statement[stmt].proofSectionPtr), proofSection);
    } else {
      /* This is the first time we've updated the proof section */
      statement[stmt].proofSectionChanged = 1;
      statement[stmt].proofSectionPtr = proofSection;
      proofSection = ""; /* so that proofSectionPtr won't be deallocated */
    }
  }

  let(&labelSection, "");
  let(&mathSection, "");
  let(&proofSection, "");
  let(&labelSectionSave, "");
  let(&mathSectionSave, "");
  let(&proofSectionSave, "");
  let(&comment, "");
  let(&str1, "");
  return output; /* The calling routine must deallocate this vstring */
} /* outputStatement */

/* 12-Jun-2011 nm */
/* Unwrap the lines in a comment then re-wrap them according to set.mm
   conventions.  This may be overly aggressive, and user should do a
   diff to see if result is as desired.  Called by WRITE SOURCE / REWRAP.
   Caller must deallocate returned vstring. */
vstring rewrapComment(vstring comment1)
{
  /* Punctuation from mmwtex.c */
#define OPENING_PUNCTUATION "(['\""
/* #define CLOSING_PUNCTUATION ".,;)?!:]'\"_-" */
#define CLOSING_PUNCTUATION ".,;)?!:]'\""
#define SENTENCE_END_PUNCTUATION ")'\""
  vstring comment = "";
  vstring commentTemplate = ""; /* Non-breaking space template */
  long length, pos, i, j;
  vstring ch; /* Pointer only; do not allocate */
  flag mathmode = 0;

  let(&comment, comment1); /* Grab arg so it can be reassigned */

  /* Ignore pre-formatted comments */
  /* if (instr(1, comment, "<PRE>") != 0) return comment; */
  if (instr(1, comment, "<HTML>") != 0) return comment;  /* 26-Dec-2011 nm */

  /* Make sure back quotes are surrounded by space */
  pos = 2;
  mathmode = 0;
  while (1) {
    pos = instr(pos + 1, comment, "`");
    if (pos == 0) break;
    mathmode = (flag)(1 - mathmode);
    if (comment[pos - 2] == '`' || comment[pos] == '`') continue;
            /* See if previous or next char is "`"; ignore "``" escape */
    if (comment[pos] != ' ' && comment[pos] != '\n') {
      /* Currently, mmwtex.c doesn't correctly handle broken subscript (_)
         or broken hyphen (-) in the CLOSING_PUNCTUATION, so allow these two as
         exceptions until that is fixed.   E.g. ` a ` _2 doesn't yield
         HTML subscript; instead we need ` a `_2. */
      if (mathmode == 1 || (comment[pos] != '_' && comment[pos] != '-')) {
        /* Add a space after back quote if none */
        let(&comment, cat(left(comment, pos), " ",
            right(comment, pos + 1), NULL));
      }
    }
    if (comment[pos - 2] != ' ') {
      /* Add a space before back quote if none */
      let(&comment, cat(left(comment, pos - 1), " ",
          right(comment, pos), NULL));
      pos++; /* Go past the "`" */
    }
  }

  /* Make sure "~" for labels are surrounded by space */
  if (instr(2, comment, "`") == 0) {  /* For now, just process comments
         not containing math symbols.  More complicated code is needed
         to ignore ~ in math symbols; maybe add it someday. */
    pos = 2;
    while (1) {
      pos = instr(pos + 1, comment, "~");
      if (pos == 0) break;
      if (comment[pos - 2] == '~' || comment[pos] == '~') continue;
              /* See if previous or next char is "~"; ignore "~~" escape */
      if (comment[pos] != ' ') {
        /* Add a space after tilde if none */
        let(&comment, cat(left(comment, pos), " ",
            right(comment, pos + 1), NULL));
      }
      if (comment[pos - 2] != ' ') {
        /* Add a space before tilde if none */
        let(&comment, cat(left(comment, pos - 1), " ",
            right(comment, pos), NULL));
        pos++; /* Go past the "~" */
      }
    }
  }

  /* Change all newlines to space unless double newline */
  /* Note:  it is assumed that blank lines have no spaces
     for this to work; the user must ensure that. */
  length = (long)strlen(comment);
  for (pos = 2; pos < length - 2; pos++) {
    if (comment[pos] == '\n' && comment[pos - 1] != '\n'
        && comment[pos + 1] != '\n')
      comment[pos] = ' ';
  }
  let(&comment, edit(comment, 16 /* reduce spaces */));

  /* Remove spaces and blank lines at end of comment */
  while (1) {
    length = (long)strlen(comment);
    if (comment[length - 3] != ' ') bug(1730);
            /* Should have been syntax err (no space before "$)") */
    if (comment[length - 4] != ' ' && comment[length - 4] != '\n') break;
    let(&comment, cat(left(comment, length - 4),
        right(comment, length - 2), NULL));
  }

  /* Put period at end of comment ending with lowercase letter */
  /* Note:  This will not detect a '~ label' at end of comment.
     A diff by the user is needed to verify it doesn't happen.
     (We could enhace the code here to do that if it becomes a problem.) */
  length = (long)strlen(comment);
  if (islower((unsigned char)(comment[length - 4]))) {
    let(&comment, cat(left(comment, length - 3), ". $)", NULL));
  }

  /* Change to ASCII 4 those spaces where the line shouldn't be
     broken */
  mathmode = 0;
  for (pos = 3; pos < length - 2; pos++) {
    if (comment[pos] == '`') { /* Start or end of math string */
/*
      if (mathmode == 0) {
        if (comment[pos - 1] == ' '
            && strchr(OPENING_PUNCTUATION, comment[pos - 2]) != NULL)
          /@ Keep opening punctuation on same line @/
          comment[pos - 1] = ASCII_4;
      } else {
        if (comment[pos + 1] == ' '
            && strchr(CLOSING_PUNCTUATION, comment[pos + 2]) != NULL)
          /@ Keep closing punctuation on same line @/
          comment[pos + 1] = ASCII_4;
      }
*/
      mathmode = (char)(1 - mathmode);
    }
    if ( mathmode == 1 && comment[pos] == ' ')
      /* We assign comment[] rather than commentTemplate to avoid confusion of
         math with punctuation.  Also, commentTemplate would be misaligned
         anyway due to adding of spaces below. */
      comment[pos] = ASCII_4;
  }

  /* 3-May-2016 nm */
  /* Look for proof discouraged or usage discouraged markup and change their
     spaces to ASCII 4 to prevent line breaks in the middle */
  if (proofDiscouragedMarkup[0] == 0) {
    /* getMarkupFlags() in mmdata.c has never been called, so initialize the
       markup strings to their defaults */
    let(&proofDiscouragedMarkup, PROOF_DISCOURAGED_MARKUP);
    let(&usageDiscouragedMarkup, USAGE_DISCOURAGED_MARKUP);
  }
  pos = instr(1, comment, proofDiscouragedMarkup);
  if (pos != 0) {
    i = (long)strlen(proofDiscouragedMarkup);
    for (j = pos; j < pos + i - 1; j++) { /* Check 2nd thru penultimate char */
      if (comment[j] == ' ') {
        comment[j] = ASCII_4;
      }
    }
  }
  pos = instr(1, comment, usageDiscouragedMarkup);
  if (pos != 0) {
    i = (long)strlen(usageDiscouragedMarkup);
    for (j = pos; j < pos + i - 1; j++) { /* Check 2nd thru penultimate char */
      if (comment[j] == ' ') {
        comment[j] = ASCII_4;
      }
    }
  }


  /* Put two spaces after end of sentence and colon */
  ch = ""; /* Prevent compiler warning */
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
        continue;  /* Ignore initials of names */
      if (strchr(SENTENCE_END_PUNCTUATION, comment[pos]) != NULL)
        pos++;
      if (comment[pos] != ' ') continue;
      if ((comment[pos + 1] >= 'A' && comment[pos + 1] <= 'Z')
          || strchr(OPENING_PUNCTUATION, comment[pos + 1]) != NULL) {
        comment[pos] = ASCII_4; /* Prevent break so next line won't have
                                   leading space; instead, break at 2nd space */
        let(&comment, cat(left(comment, pos + 1), " ",
            right(comment, pos + 2), NULL));
      }
    } /* end while */
  } /* next i */

  length = (long)strlen(comment);
  let(&commentTemplate, space(length));
  for (pos = 3; pos < length - 2; pos++) {
    if (comment[pos] == ' ') {
      if (comment[pos - 1] == '~' && comment[pos - 2] != '~') {
        /* Don't break "~ <label>" */
        commentTemplate[pos] = ASCII_4;
      } else if ((comment[pos - 2] == ' '
            || strchr(OPENING_PUNCTUATION, comment[pos - 2]) != NULL)
          && strchr(OPENING_PUNCTUATION, comment[pos - 1]) != NULL) {
        /* Don't break space after opening punctuation */
        commentTemplate[pos] = ASCII_4;
      } else if ((comment[pos + 2] == ' '
            || comment[pos + 2] == '\n' /* Period etc. before line break */
            || comment[pos + 2] == ASCII_4 /* 2-space sentence break
                              done above where 1st space is ASCII_4 */
            || strchr(CLOSING_PUNCTUATION, comment[pos + 2]) != NULL)
          && strchr(CLOSING_PUNCTUATION, comment[pos + 1]) != NULL) {
        /* Don't break space before closing punctuation */
        commentTemplate[pos] = ASCII_4;
/*
      } else if (comment[pos + 2] == ' '
          && strchr(CLOSING_PUNCTUATION, comment[pos + 1]) != NULL) {
        /@ Don't break space after "~ <label>" if followed by punctuation @/
        commentTemplate[pos] = ASCII_4;
      } else if (comment[pos - 2] == ' '
          && strchr(OPENING_PUNCTUATION, comment[pos - 1]) != NULL) {
        /@ Don't break space before "~ <label>" if preceded by punctuation @/
        commentTemplate[pos] = ASCII_4;
      } else if (comment[pos + 1] == '`'
          && strchr(OPENING_PUNCTUATION, comment[pos - 1]) != NULL) {
        /@ Don't break space between punctuation and math start '`' @/
        commentTemplate[pos] = ASCII_4;
      } else if (comment[pos - 1] == '`'
          && strchr(CLOSING_PUNCTUATION, comment[pos + 1]) != NULL) {
        /@ Don't break space between punctuation and math end '`' @/
        commentTemplate[pos] = ASCII_4;
*/
      } else if (comment[pos - 3] == ' ' && comment[pos - 2] == 'p'
          && comment[pos - 1] == '.') {
        /* Don't break " p. nnn" */
        commentTemplate[pos] = ASCII_4;
      }
    }
  }
  commentTemplate[length - 3] = ASCII_4; /* Last space in comment */

  for (pos = 3; pos < length - 2; pos++) {
    /* Transfer the non-breaking spaces from the template to the comment */
    if (commentTemplate[pos] == ASCII_4) comment[pos] = ASCII_4;
  }

  let(&commentTemplate, "");

  return(comment);
} /* rewrapComment */

/* This is a general-purpose function to parse a math token string,
   typically input by the user at the keyboard.  The statemNum is
   needed to provide a context to determine which symbols are active.
   Lack of whitespace is tolerated according to standard rules.
   mathTokens must be set to the proper value. */
/* The code in this section is complex because it uses the fast parsing
   method borrowed from parseStatements().  On the other hand, it must set
   up some initial tables by scanning the entire mathToken array; this may
   slow it down in some applications. */
/* Warning:  mathTokens must be the correct value (some procedures might
   artificially adjust mathTokens to add dummy tokens [schemes] to the
   mathToken array) */
/* The user text may include existing or new dummy variables of the
   form "?nnn". */
nmbrString *parseMathTokens(vstring userText, long statemNum)
{
  long i, j;
  char *fbPtr;
  long mathStringLen;
  long tokenNum;
  long lowerKey, upperKey;
  long symbolLen, origSymbolLen, mathKeyNum;
  void *mathKeyPtr; /* bsearch returned value */
  int maxScope;
  flag errorFlag = 0; /* Prevents bad token from being added to output */
  int errCount = 0; /* Cumulative error count */
  vstring tmpStr = "";
  vstring nlUserText = "";


  long *mathTokenSameAs; /* Flag that symbol is unique (for speed up) */
  long *reverseMathKey; /* Map from mathTokens to mathKey */


  /* Temporary working space */
  long wrkLen;
  nmbrString *wrkNmbrPtr;
  char *wrkStrPtr;

  /* The answer */
  nmbrString *mathString = NULL_NMBRSTRING;

  long maxSymbolLen; /* Longest math symbol (for speedup) */
  flag *symbolLenExists; /* A symbol with this length exists (for speedup) */

  long nmbrSaveTempAllocStack; /* For nmbrLet() stack cleanup */
  long saveTempAllocStack; /* For let() stack cleanup */
  nmbrSaveTempAllocStack = nmbrStartTempAllocStack;
  nmbrStartTempAllocStack = nmbrTempAllocStackTop;
  saveTempAllocStack = startTempAllocStack;
  startTempAllocStack = tempAllocStackTop;

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  tokenNum = 0;

  /* Add a newline before user text for sourceError() */
  let(&nlUserText, cat("\n", userText, NULL));

  /* Make sure that mathTokens has been initialized */
  if (!mathTokens) bug(1717);

  /* Set the 'active' flag based on statemNum; here 'active' just means it
     would be in the stack during normal parsing, not the Metamath manual
     definition. */
  for (i = 0; i < mathTokens; i++) {
    if (mathToken[i].statement <= statemNum && mathToken[i].endStatement >=
        statemNum) {
      mathToken[i].active = 1;
    } else {
      mathToken[i].active = 0;
    }
  }

  /* Initialize flags for mathKey array that identify math symbols as
     unique (when 0) or, if not unique, the flag is a number identifying a group
     of identical names */
  mathTokenSameAs = malloc((size_t)mathTokens * sizeof(long));
  if (!mathTokenSameAs) outOfMemory("#12 (mathTokenSameAs)");
  reverseMathKey = malloc((size_t)mathTokens * sizeof(long));
  if (!reverseMathKey) outOfMemory("#13 (reverseMathKey)");
  for (i = 0; i < mathTokens; i++) {
    mathTokenSameAs[i] = 0; /* 0 means unique */
    reverseMathKey[mathKey[i]] = i; /* Initialize reverse map to mathKey */
  }
  for (i = 1; i < mathTokens; i++) {
    if (!strcmp(mathToken[mathKey[i]].tokenName,
        mathToken[mathKey[i - 1]].tokenName)) {
      if (!mathTokenSameAs[i - 1]) mathTokenSameAs[i - 1] = i;
      mathTokenSameAs[i] = mathTokenSameAs[i - 1];
    }
  }

  /* Initialize temporary working space for parsing tokens */
  /* Assume the worst case of one token per userText character */
  wrkLen = (long)strlen(userText);
  wrkNmbrPtr = malloc((size_t)wrkLen * sizeof(nmbrString));
  if (!wrkNmbrPtr) outOfMemory("#22 (wrkNmbrPtr)");
  wrkStrPtr = malloc((size_t)wrkLen + 1);
  if (!wrkStrPtr) outOfMemory("#23 (wrkStrPtr)");

  /* Find declared math symbol lengths (used to speed up parsing) */
  maxSymbolLen = 0;
  for (i = 0; i < mathTokens; i++) {
    if (mathToken[i].length > maxSymbolLen) {
      maxSymbolLen = mathToken[i].length;
    }
  }
  symbolLenExists = malloc(((size_t)maxSymbolLen + 1) * sizeof(flag));
  if (!symbolLenExists) outOfMemory("#25 (symbolLenExists)");
  for (i = 0; i <= maxSymbolLen; i++) {
    symbolLenExists[i] = 0;
  }
  for (i = 0; i < mathTokens; i++) {
    symbolLenExists[mathToken[i].length] = 1;
  }


  currentScope = statement[statemNum].scope; /* Scope of the ref. statement */


  /* The code below is indented because it was borrowed from parseStatements().
     We will leave the indentation intact for easier future comparison
     with that code. */

        /* Scan the math section for tokens */
        mathStringLen = 0;
        fbPtr = nlUserText;
        while (1) {
          fbPtr = fbPtr + whiteSpaceLen(fbPtr);
          origSymbolLen = tokenLen(fbPtr);
          if (!origSymbolLen) break; /* Done scanning source line */

          /* Scan for largest matching token from the left */
         nextAdjToken:
          /* maxSymbolLen is the longest declared symbol */
          if (origSymbolLen > maxSymbolLen) {
            symbolLen = maxSymbolLen;
          } else {
            symbolLen = origSymbolLen;
          }
          memcpy(wrkStrPtr, fbPtr, (size_t)symbolLen);
          for (; symbolLen > 0; symbolLen--) {
            /* symbolLenExists means a symbol of this length was declared */
            if (!symbolLenExists[symbolLen]) continue;
            wrkStrPtr[symbolLen] = 0; /* Define end of trial token to look up */
            mathKeyPtr = (void *)bsearch(wrkStrPtr, mathKey,
                (size_t)mathTokens, sizeof(long), mathSrchCmp);
            if (!mathKeyPtr) continue; /* Trial token was not declared */
            mathKeyNum = (long *)mathKeyPtr - mathKey; /* Pointer arithmetic! */
            if (mathTokenSameAs[mathKeyNum]) { /* Multiply-declared symbol */
              lowerKey = mathKeyNum;
              upperKey = lowerKey;
              j = mathTokenSameAs[lowerKey];
              while (lowerKey) {
                if (j != mathTokenSameAs[lowerKey - 1]) break;
                lowerKey--;
              }
              while (upperKey < mathTokens - 1) {
                if (j != mathTokenSameAs[upperKey + 1]) break;
                upperKey++;
              }
              /* Find the active symbol with the most recent declaration */
              /* (Note:  Here, 'active' means it's on the stack, not the
                 official def.) */
              maxScope = -1;
              for (i = lowerKey; i <= upperKey; i++) {
                j = mathKey[i];
                if (mathToken[j].active) {
                  if (mathToken[j].scope > maxScope) {
                    tokenNum = j;
                    maxScope = mathToken[j].scope;
                    if (maxScope == currentScope) break; /* Speedup */
                  }
                }
              }
              if (maxScope == -1) {
                tokenNum = mathKey[mathKeyNum]; /* Pick an arbitrary one */
                errCount++;
                if (errCount <= 1) { /* Print 1st error only */
                  sourceError(fbPtr, symbolLen, /*stmt*/ 0,
       "This math symbol is not active (i.e. was not declared in this scope).");
                }
                errorFlag = 1;
              }
            } else { /* The symbol was declared only once. */
              tokenNum = *((long *)mathKeyPtr);
                  /* Same as: tokenNum = mathKey[mathKeyNum]; but faster */
              if (!mathToken[tokenNum].active) {
                errCount++;
                if (errCount <= 1) { /* Print 1st error only */
                  sourceError(fbPtr, symbolLen, /*stmt*/ 0,
       "This math symbol is not active (i.e. was not declared in this scope).");
                }
                errorFlag = 1;
              }
            } /* End if multiply-defined symbol */
            break; /* The symbol was found, so we are done */
          } /* Next symbolLen */

          if (symbolLen == 0) { /* Symbol was not found */
            /* See if the symbol is a dummy variable name */
            if (fbPtr[0] == '$') {
              symbolLen = tokenLen(fbPtr);
              for (i = 1; i < symbolLen; i++) {
                if (fbPtr[i] < '0' || fbPtr[i] > '9') break;
              }
              symbolLen = i;
              if (symbolLen == 1) {
                symbolLen = 0; /* No # after '$' -- error */
              } else {
                memcpy(wrkStrPtr, fbPtr + 1, (size_t)i - 1);
                wrkStrPtr[i - 1] = 0; /* End of string */
                tokenNum = (long)(val(wrkStrPtr)) + mathTokens;
                /* See if dummy var has been declared; if not, declare it */
                if (tokenNum > pipDummyVars + mathTokens) {
                  declareDummyVars(tokenNum - pipDummyVars - mathTokens);
                }
              }
            } /* End if fbPtr == '$' */
         } /* End if symbolLen == 0 */


          if (symbolLen == 0) { /* Symbol was not found */
            symbolLen = tokenLen(fbPtr);
            errCount++;
            if (errCount <= 1) { /* Print 1st error only */
              sourceError(fbPtr, symbolLen, /*stmt*/ 0,
      "This math symbol was not declared (with a \"$c\" or \"$v\" statement).");
            }
            errorFlag = 1;
          }

          /* Add symbol to mathString */
          if (!errorFlag) {
            wrkNmbrPtr[mathStringLen] = tokenNum;
            mathStringLen++;
          } else {
            errorFlag = 0;
          }
          fbPtr = fbPtr + symbolLen; /* Move on to next symbol */

          if (symbolLen < origSymbolLen) {
            /* This symbol is not separated from next by white space */
            /* Speed-up: don't call tokenLen again; just jump past it */
            origSymbolLen = origSymbolLen - symbolLen;
            goto nextAdjToken; /* (Instead of continue) */
          }
        } /* End while */


        /* Assign mathString */
        nmbrLet(&mathString, nmbrSpace(mathStringLen));
        for (i = 0; i < mathStringLen; i++) {
          mathString[i] = wrkNmbrPtr[i];
        }

  /* End of unconventionally indented section borrowed from parseStatements() */

  startTempAllocStack = saveTempAllocStack;
  nmbrStartTempAllocStack = nmbrSaveTempAllocStack;
  if (mathStringLen) nmbrMakeTempAlloc(mathString); /* Flag for dealloc*/

  /* Deallocate temporary space */
  free(mathTokenSameAs);
  free(reverseMathKey);
  free(wrkNmbrPtr);
  free(wrkStrPtr);
  free(symbolLenExists);
  let(&tmpStr, "");
  let(&nlUserText, "");

  return (mathString);

} /* parseMathTokens */


/* 31-Dec-2017 nm For .mm file splitting */
/* Get the next real $[...$] or virtual $( Begin $[... inclusion */
/* This uses the convention of mmvstr.c where beginning of a string
   is position 1.  However, startOffset = 0 means no offset i.e.
   start at fileBuf. */
void getNextInclusion(char *fileBuf, long startOffset, /* inputs */
    /* outputs: */
    long *cmdPos1, long *cmdPos2,
    long *endPos1, long *endPos2,
    char *cmdType, /* 'B' = "$( Begin [$..." through "$( End [$...",
                      'I' = "[$...",
                      'S' = "$( Skip [$...",
                      'E' = Start missing matched End
                      'N' = no include found; includeFn = "" */
    vstring *fileName /* name of included file */
    )
{

/*
cmdType = 'B' or 'E':
      ....... $( Begin $[ prop.mm $] $)   ......   $( End $[ prop.mm $] $) ...
       ^      ^           ^^^^^^^       ^          ^                      ^
  startOffset cmdPos1    fileName  cmdPos2     endPos1              endPos2
                                             (=0 if no End)   (=0 if no End)

   Note: in the special case of Begin, cmdPos2 points _after_ the whitespace
   after "$( Begin $[ prop.mm $] $)" i.e. the whitespace is considered part of
   the Begin command.  The is needed because prop.mm content doesn't
   necessarily start with whitespace.  prop.mm does, however, end with
   whitespace (\n) as enforced by readFileToString().

cmdType = 'I':
      ............... $[ prop.mm $]  ..............
       ^              ^  ^^^^^^^   ^
  startOffset   cmdPos1 fileName  cmdPos2     endPos1=0  endPos2=0

cmdType = 'S':
      ....... $( Skip $[ prop.mm $] $)   ......
       ^      ^          ^^^^^^^      ^
  startOffset cmdPos1    fileName  cmdPos2     endPos1=0  endPos2=0
*/
  char *fbPtr;
  char *tmpPtr;
  flag lookForEndMode = 0; /* 1 if inside of $( Begin, End, Skip... */
  long i, j;

  fbPtr = fileBuf + startOffset;

  while (1) {
    fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr); /* Count $( as a token */
    j = rawTokenLen(fbPtr); /* Treat $(, $[ as tokens */
    if (j == 0) {
      *cmdType = 'N'; /* No include found */
      break;  /* End of file */
    }
    if (fbPtr[0] != '$') {
      fbPtr = fbPtr + j;
      continue;
    }

    /* Process normal include $[ $] */
    if (fbPtr[1] == '[') {
      if (lookForEndMode == 0) {
        /* If lookForEndMode is 1, ignore everything until we find a matching
           "$( End $[..." */
        *cmdPos1 = fbPtr - fileBuf + 1; /* 1 = beginning of file */
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + whiteSpaceLen(fbPtr); /* Comments = whitespace here */
        j = rawTokenLen(fbPtr); /* Should be file name */
        /* Note that mid, seg, left, right do not waste time computing
           the length of the input string fbPtr */
        let(&(*fileName), left(fbPtr, j));
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + whiteSpaceLen(fbPtr); /* Comments = whitespace here */
        j = rawTokenLen(fbPtr);
        if (j == 2/*speedup*/ && !strncmp("$]", fbPtr, (size_t)j)) {
          *cmdPos2 = fbPtr - fileBuf + j + 1;
          *endPos1 = 0;
          *endPos2 = 0;
          *cmdType = 'I';
          return;
        }
        /* TODO - more precise error message */
        print2("?Missing \"$]\" after \"$[ %s\"\n", *fileName);
        fbPtr = fbPtr + j;
        continue; /* Not a completed include */
      } /* if (lookForEndMode == 0) */
      fbPtr = fbPtr + j;
      continue; /* Either not a legal include - error detected later,
             or we're in lookForEndMode */
    /* Process markup-type include inside comment */
    } else if (fbPtr[1] == '(') {
      /* Process comment starting at "$(" */
      if (lookForEndMode == 0) {
        *cmdPos1 = fbPtr - fileBuf + 1;
      } else {
        *endPos1 = fbPtr - fileBuf + 1;
      }
      fbPtr = fbPtr + j;
      fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr); /* comment != whitespace */
      j = rawTokenLen(fbPtr);
      *cmdType = '?';
      if (j == 5/*speedup*/ && !strncmp("Begin", fbPtr, (size_t)j)) {
        /* If lookForEndMode is 1, we're looking for End matching earlier Begin */
        if (lookForEndMode == 0) {
          *cmdType = 'B';
        }
      } else if (j == 4/*speedup*/ && !strncmp("Skip", fbPtr, (size_t)j)) {
        /* If lookForEndMode is 1, we're looking for End matching earlier Begin */
        if (lookForEndMode == 0) {
          *cmdType = 'S';
        }
      } else if (j == 3/*speedup*/ && !strncmp("End", fbPtr, (size_t)j)) {
        /* If lookForEndMode is 0, there was no matching Begin */
        if (lookForEndMode == 1) {
          *cmdType = 'E';
        }
      }
      if (*cmdType == '?') { /* The comment doesn't qualify as $[ $] markup */
        /* Find end of comment and continue */
        goto GET_PASSED_END_OF_COMMENT;
      } else {
        /* It's Begin or Skip or End */
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr);
        j = rawTokenLen(fbPtr);
        if (j != 2 || strncmp("$[", fbPtr, (size_t)j)) {
          /* Find end of comment and continue */
          goto GET_PASSED_END_OF_COMMENT;
        }
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr);  /* comment != whitespace */
        j = rawTokenLen(fbPtr);
        /* Note that mid, seg, left, right do not waste time computing
           the length of the input string fbPtr */
        if (lookForEndMode == 0) {
          /* It's Begin or Skip */
          let(&(*fileName), left(fbPtr, j));
        } else {
          /* It's an End command */
          if (strncmp(*fileName, fbPtr, (size_t)j)) {
            /* But the file name didn't match, so it's not a matching End */
            /* Find end of comment and continue */
            goto GET_PASSED_END_OF_COMMENT;
          }
        }
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr);  /* comment != whitespace */
        j = rawTokenLen(fbPtr);
        if (j != 2 || strncmp("$]", fbPtr, (size_t)j)) {
          /* The token after the file name isn't "$]" */
          /* Find end of comment and continue */
          goto GET_PASSED_END_OF_COMMENT;
        }
        fbPtr = fbPtr + j;
        fbPtr = fbPtr + rawWhiteSpaceLen(fbPtr);  /* comment != whitespace */
        j = rawTokenLen(fbPtr);
        if (j != 2 || strncmp("$)", fbPtr, (size_t)j)) {
          /* The token after the "$]" isn't "$)" */
          /* Find end of comment and continue */
          goto GET_PASSED_END_OF_COMMENT;
        }
        /* We are now at the end of "$( Begin/Skip/End $[ file $] $)" */
        fbPtr = fbPtr + j;
        if (lookForEndMode == 0) {
          *cmdPos2 = fbPtr - fileBuf + 1
            + ((*cmdType == 'B') ? 1 : 0);/*after whitespace for 'B' (see above)*/
          if (*cmdType == 'S') {  /* Skip command; we're done */
            *endPos1 = 0;
            *endPos2 = 0;
            return;
          }
          if (*cmdType != 'B') bug(1742);
          lookForEndMode = 1;
        } else { /* We're at an End */
          if (*cmdType != 'E') bug(1743);
          /*lookForEndMode = 0;*/ /* Not needed since we will return soon */
          *cmdType = 'B'; /* Restore it to B for Begin/End pair */
          *endPos2 = fbPtr - fileBuf + 1;
          return;
        }
        continue; /* We're past Begin; start search for End */
      } /* Begin, End, or Skip */
    } else if (i != i + 1) { /* Suppress "unreachable code" warning for bug trap below */
      /* It's '$' not followed by '[' or '('; j is token length */
      fbPtr = fbPtr + j;
      continue;
    }
    bug(1746); /* Should never get here */
   GET_PASSED_END_OF_COMMENT:
    /* Note that fbPtr should be at beginning of last token found, which
       may be "$)" (in which case i will be 1 from the instr) */

    /* Don't use instr because it computes string length each call */
    /*i = instr(1, fbPtr, "$)");*/ /* Normally this will be fast because we only
        have to find the end of the comment that we're in */
    /* Emulater the instr() */
    tmpPtr = fbPtr;
    i = 0;
    while (1) {

      /* strchr is incredibly slow under lcc - why?
         Is it computing strlen internally maybe? */
      /*
      tmpPtr = strchr(tmpPtr, '$');
      if (tmpPtr == NULL) {
        i = 0;
        break;
      }
      if (tmpPtr[1] == ')') {
        i = tmpPtr - fbPtr + 1;
        break;
      }
      tmpPtr++;
      */

      /* Emulate strchr */
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

    } /* while (1) */

    if (i == 0) {
      /* TODO: better error msg */
      printf("?End of comment not found\n");
      i = (long)strlen(fileBuf); /* Slow, but this is a rare error */
      fbPtr = fileBuf + i; /* Points to null at end of fileBuf */
    } else {
      fbPtr = fbPtr + i + 2 - 1; /* Skip the "$)" - skip 2 characters, then
           back up 1 since the instr result starts at 1 */
    }
    /* continue; */  /* Not necessary since we're at end of loop */
  } /* while (1) */
  if (j != 0) bug(1744); /* Should be at end of file */
  if (lookForEndMode == 1) {
    /* We didn't find an End */
    *cmdType = 'E';
    *endPos1 = 0; *endPos2 = 0;
  } else {
    *cmdType = 'N'; /* no include was found */
    *cmdPos1 = 0; *cmdPos2 = 0; *endPos1 = 0; *endPos2 = 0;
    let(&(*fileName), "");
  }
  return;

} /* getNextInclusion */



/* This function transfers the content of the statement[] array
   to a linear buffer in preparation for creating the output file.
   Any changes such as modified proofs will be updated in the buffer. */
/* The caller is responsible for deallocating the returned string. */
vstring writeSourceToBuffer(void)
{
  long stmt, size;
  vstring buf = "";
  char *ptr;

  /* Compute the size of the buffer */
  /* Note that statement[statements + 1] is a dummy statement
     containing the text after the last statement in its
     labelSection */
  size = 0;
  for (stmt = 1; stmt <= statements + 1; stmt++) {
    /* Add the sizes of the sections.  When sections don't exist
       (like a proof for a $a statement), the section length is 0. */
    size += statement[stmt].labelSectionLen
        + statement[stmt].mathSectionLen
        + statement[stmt].proofSectionLen;
    /* Add in the 2-char length of keywords, which aren't stored in
       the statement sections */
    switch (statement[stmt].type) {
      case lb_: /* ${ */
      case rb_: /* $} */
        size += 2;
        break;
      case v_: /* $v */
      case c_: /* $c */
      case d_: /* $d */
      case e_: /* $e */
      case f_: /* $f */
      case a_: /* $a */
        size += 4;
        break;
      case p_: /* $p */
        size += 6;
        break;
      case illegal_: /* dummy */
        if (stmt != statements + 1) bug(1747);
        /* The labelLen is text after last statement */
        size += 0; /* There are no keywords in statements + 1 */
        break;
      default: bug(1748);
    } /* switch (statement[stmt].type) */
  } /* next stmt */

  /* Create the output buffer */
  /* We could have created it with let(&buf, space(size)), but malloc should
     be slightly faster since we don't have to initialize each entry */
  buf = malloc((size_t)(size + 1) * sizeof(char));

  ptr = buf; /* Pointer to keep track of buf location */
  /* Transfer the statement[] array to buf */
  for (stmt = 1; stmt <= statements + 1; stmt++) {
    /* Always transfer the label section (text before $ keyword) */
    memcpy(ptr/*dest*/, statement[stmt].labelSectionPtr/*source*/,
        (size_t)(statement[stmt].labelSectionLen)/*size*/);
    ptr += statement[stmt].labelSectionLen;
    switch (statement[stmt].type) {
      case illegal_:
        if (stmt != statements + 1) bug(1749);
        break;
      case lb_: /* ${ */
      case rb_: /* $} */
        ptr[0] = '$';
        ptr[1] = statement[stmt].type;
        ptr += 2;
        break;
      case v_: /* $v */
      case c_: /* $c */
      case d_: /* $d */
      case e_: /* $e */
      case f_: /* $f */
      case a_: /* $a */
        ptr[0] = '$';
        ptr[1] = statement[stmt].type;
        ptr += 2;
        memcpy(ptr/*dest*/, statement[stmt].mathSectionPtr/*source*/,
            (size_t)(statement[stmt].mathSectionLen)/*size*/);
        ptr += statement[stmt].mathSectionLen;
        ptr[0] = '$';
        ptr[1] = '.';
        ptr += 2;
        break;
      case p_: /* $p */
        ptr[0] = '$';
        ptr[1] = statement[stmt].type;
        ptr += 2;
        memcpy(ptr/*dest*/, statement[stmt].mathSectionPtr/*source*/,
            (size_t)(statement[stmt].mathSectionLen)/*size*/);
        ptr += statement[stmt].mathSectionLen;
        ptr[0] = '$';
        ptr[1] = '=';
        ptr += 2;
        memcpy(ptr/*dest*/, statement[stmt].proofSectionPtr/*source*/,
            (size_t)(statement[stmt].proofSectionLen)/*size*/);
        ptr += statement[stmt].proofSectionLen;
        ptr[0] = '$';
        ptr[1] = '.';
        ptr += 2;
        break;
      default: bug(1750);
    } /* switch (statement[stmt].type) */
  } /* next stmt */
  if (ptr - buf != size) bug(1751);
  buf[size] = 0; /* End of string marker */
  return buf;
} /* writeSourceToBuffer */


/* 31-Dec-2017 nm */
/* This function creates split files containing $[ $] inclusions, from
   a nonsplit source with $( Begin $[... etc. inclusions */
/* This function calls itself recursively, and after the recursive call
   the fileBuf (=includeBuf) argument is deallocated. */
/* For the top level call, fileName MUST NOT HAVE A DIRECTORY PATH */
/* Note that fileBuf must be deallocated by initial caller.  This will let the
   caller decide whether to say re-use fileBuf to create an unsplit version
   of the .mm file in case the split version generation encounters an error. */
/*                                     TODO ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */
/*flag(TODO)*/ void writeSplitSource(vstring *fileBuf, vstring fileName,
    flag noVersioningFlag, flag noDeleteFlag) {
  FILE *fp;
  vstring tmpStr1 = "";
  vstring tmpFileName = "";
  vstring includeBuf = "";
  vstring includeFn = "";
  vstring fileNameWithPath = "";
  long size;
  flag writeFlag;
  long startOffset;
  long cmdPos1;
  long cmdPos2;
  long endPos1;
  long endPos2;
  char cmdType;
  startOffset = 0;
  let(&fileNameWithPath, cat(rootDirectory, fileName, NULL));
  while (1) {
    getNextInclusion(*fileBuf, startOffset, /* inputs */
        /* outputs: */
        &cmdPos1, &cmdPos2,
        &endPos1, &endPos2,
        &cmdType, /* 'B' = "$( Begin [$..." through "$( End [$...",
                     'I' = "[$...",
                     'S' = "$( Skip [$...",
                     'E' = Start missing matched End
                     'N' = no include found; includeFn = "" */
        &includeFn /* name of included file */ );
    if (cmdType == 'N') {
      writeFlag = 0;
      /* There are no more includes to expand, so write out the file */
      if (!strcmp(fileName, output_fn)) {
        /* We're writing the top-level file - always create new version */
        writeFlag = 1;
      } else {
        /* We're writing an included file */
        /* See if the file already exists */
        let(&tmpStr1, "");
        tmpStr1 = readFileToString(fileNameWithPath, 0/*quiet*/, &size);
        if (tmpStr1 == NULL) {
          tmpStr1 = ""; /* Prevent seg fault */
          /* If it doesn't exist, see if the ~1 version exists */
          let(&tmpFileName, cat(fileNameWithPath, "~1", NULL));
          tmpStr1 = readFileToString(tmpFileName, 0/*quiet*/, &size);
          if (tmpStr1 == NULL) {
            tmpStr1 = ""; /* Prevent seg fault */
            /* Create and write the file */
            writeFlag = 1;
          } else {
            /* See if the ~1 backup file content changed */
            if (strcmp(tmpStr1, *fileBuf)) {
              /* Content is different; write the file */
              writeFlag = 1;
            } else {
              /* Just rename the ~1 version to the main version */
              print2("Recovering \"%s\" from \"%s\"...\n",
                  fileNameWithPath, tmpFileName);
              rename(tmpFileName/*old*/, fileNameWithPath/*new*/);
            }
          } /* if (tmpStr1 == NULL) */
        } else { /* tmpStr1 != NULL */
          /* The include file already exists; see if the content changed */
          if (strcmp(tmpStr1, *fileBuf)) {
            /* Content is different; write the file */
            writeFlag = 1;
          } else {
            /* Just rename the ~1 version to the main version */
            print2("Content of \"%s\" did not change.\n",
                fileNameWithPath);
            rename(tmpFileName/*old*/, fileNameWithPath/*new*/);
          }
        }
      }
      if (writeFlag == 1) {
        fp = fSafeOpen(fileNameWithPath, "w", 0/*noVersioningFlag*/);
        if (fp == NULL) {
          /* TODO: better error msg?  Abort and don't split? */
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
      } /* if (writeFlag == 1 ) */
      break;
    } else if (cmdType == 'S') {
      /* Change "Skip" to a real inclusion */
      let(&tmpStr1, cat("$[ ", includeFn, " $]", NULL));
      startOffset = cmdPos1 - 1 + (long)strlen(tmpStr1);
      let(&(*fileBuf), cat(left(*fileBuf, cmdPos1 - 1), tmpStr1,
          right(*fileBuf, cmdPos2), NULL));
      continue;
    } else if (cmdType == 'B') {
      /* Extract included file content and call this recursively to process */
      let(&tmpStr1, cat("$[ ", includeFn, " $]", NULL));
      startOffset = cmdPos1 - 1 + (long)strlen(tmpStr1);
      /* We start _after_ the whitespace after cmdPos2 because it wasn't
         in the original included file but was put there by us in
         readSourceAndIncludes() */
      let(&includeBuf, seg(*fileBuf, cmdPos2, endPos1 - 1));
      let(&(*fileBuf), cat(left(*fileBuf, cmdPos1 - 1), tmpStr1,
          right(*fileBuf, endPos2), NULL));
      /* TODO: intercept error from deeper calls? */
      writeSplitSource(&includeBuf, includeFn, noVersioningFlag, noDeleteFlag);
      continue;
    } else if (cmdType == 'I') {
      bug(1752); /* Any real $[ $] should have been converted to commment */
      /* However in theory, user could have faked an assignable description
         if modifiable comments are added in the future...*/
      startOffset = cmdPos2 - 1;
      continue;
    } else if (cmdType == 'E') {
      /* TODO What error message should go here? */
      print2("?Unterminated \"$( Begin $[...\" inclusion markup in \"%s\".",
          fileNameWithPath);
      startOffset = cmdPos2 - 1;
      continue;
    } else {
      /* Should never happen */
      bug(1753);
    }
  } /* while (1) */
  /* Deallocate memory */
  /*let(&(*fileBuf), "");*/ /* Let caller decide whether to do this */
  let(&tmpStr1, "");
  let(&tmpFileName, "");
  let(&includeFn, "");
  let(&includeBuf, "");
  let(&fileNameWithPath, "");
} /* writeSplitSource */


/* 31-Dec-2017 nm */
/* When "write source" does not have the "/split" qualifier, by default
   (i.e. without "/no_delete") the included modules are "deleted" (renamed
   to ~1) since their content will be in the main output file. */
/*flag(TODO)*/ void deleteSplits(vstring *fileBuf, flag noVersioningFlag) {
  FILE *fp;
  vstring includeFn = "";
  vstring fileNameWithPath = "";
  long startOffset;
  long cmdPos1;
  long cmdPos2;
  long endPos1;
  long endPos2;
  char cmdType;
  startOffset = 0;
  while (1) {
    /* We scan the source for all "$( Begin $[ file $] $)...$( End $[ file $]"
       and when found, we "delete" file. */
    getNextInclusion(*fileBuf, startOffset, /* inputs */
        /* outputs: */
        &cmdPos1, &cmdPos2,
        &endPos1, &endPos2,
        &cmdType, /* 'B' = "$( Begin [$..." through "$( End [$...",
                     'I' = "[$...",
                     'S' = "$( Skip [$...",
                     'E' = Start missing matched End
                     'N' = no include found; includeFn = "" */
        &includeFn /* name of included file */ );
    /* We only care about the 'B' command */
    if (cmdType == 'B') {
      let(&fileNameWithPath, cat(rootDirectory, includeFn, NULL));
      /* See if the included file exists */
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
      /* Adjust offset and continue */
      /* We don't go the the normal end of the 'B' because it may
         have other 'B's inside.  Instead, just skip past the Begin. */
      startOffset = cmdPos2 - 1; /* don't use endPos2 */
    } else if (cmdType == 'N') {
      /* We're done */
      break;
    } else if (cmdType == 'S') {
      /* Adjust offset and continue */
      startOffset = cmdPos2 - 1;
    } else if (cmdType == 'E') {
      /* There's a problem, but ignore - should have been reported earlier */
      /* Adjust offset and continue */
      startOffset = cmdPos2 - 1;
    } else if (cmdType == 'I') {
      bug(1755); /* Should never happen */
    } else {
      bug(1756);
    }
    continue;
  } /* while (1) */
  /* Deallocate memory */
  /*let(&(*fileBuf), "");*/ /* Let caller decide whether to do this */
  let(&includeFn, "");
  let(&fileNameWithPath, "");
  return;
} /* deleteSplits */


/* 9-Jan-2018 nm */
/* Get file name and line number given a pointer into the read buffer */
/* The user must deallocate the returned string (file name) */
/* The globals includeCall structure and includeCalls are used */
vstring getFileAndLineNum(vstring buffPtr/*start of read buffer*/,
    vstring currentPtr/*place at which to get file name and line no*/,
    long *lineNum/*return argument*/) {
  long i, smallestOffset, smallestNdx;
  vstring fileName = "";

  /* Make sure it's not outside the read buffer */
  if (currentPtr < buffPtr
      || currentPtr >= buffPtr + includeCall[1].current_offset) {
    bug(1769);
  }

  /* Find the includeCall that is closest to currentPtr but does not
     exceed it */
  smallestOffset = currentPtr - buffPtr; /* Start with includeCall[0] */
  if (smallestOffset < 0) bug(1767);
  smallestNdx = 0; /* Point to includeCall[0] */
  for (i = 0; i <= includeCalls; i++) {
    if (includeCall[i].current_offset <= currentPtr - buffPtr) {
      if ((currentPtr - buffPtr) - includeCall[i].current_offset
          <= smallestOffset) {
        smallestOffset = (currentPtr - buffPtr) - includeCall[i].current_offset;
        smallestNdx = i;
      }
    }
  }
  if (smallestOffset < 0) bug(1768);
  *lineNum = includeCall[smallestNdx].current_line
      + countLines(buffPtr + includeCall[smallestNdx].current_offset,
          smallestOffset);
  /* Assign to new string to prevent original from being deallocated */
  let(&fileName, includeCall[smallestNdx].source_fn);
/*D*//*printf("smallestNdx=%ld smallestOffset=%ld i[].co=%ld\n",smallestNdx,smallestOffset,includeCall[smallestNdx].current_offset);*/
  return fileName;
} /* getFileAndLineNo */


/* 9-Jan-2018 nm */
/* statement[stmtNum].fileName and .lineNum are initialized to "" and 0.
   To save CPU time, they aren't normally assigned until needed, but once
   assigned they can be reused without looking them up again.  This function
   will assign them if they haven't been assigned yet.  It just returns if
   they have been assigned. */
/* The globals statement[] and sourcePtr are used */
void assignStmtFileAndLineNum(long stmtNum) {
  if (statement[stmtNum].lineNum > 0) return; /* Already assigned */
  if (statement[stmtNum].lineNum < 0) bug(1766);
  if (statement[stmtNum].fileName[0] != 0) bug(1770);
  /* We can make a direct string assignment here since previous value was "" */
  statement[stmtNum].fileName = getFileAndLineNum(sourcePtr,
      statement[stmtNum].statementPtr, &(statement[stmtNum].lineNum));
  return;
} /* assignStmtFileAndLineNum */



/* This function returns a pointer to a buffer containing the contents of an
   input file and its 'include' calls.  'Size' returns the buffer's size.  */
/* TODO - ability to flag error to skip raw source function */
/* Recursive function that processes a found include */
/* If NULL is returned, it means a serious error occured (like missing file)
   and reading should be aborted. */
/* Globals used:  includeCall[], includeCalls */
char *readInclude(vstring fileBuf, long fileBufOffset,
    /*vstring parentFileName,*/ vstring sourceFileName,
    long *size, long parentLineNum, flag *errorFlag)
{
  long i;
  /*vstring fileBuf = "";*/
  long inclSize;
  /* flag insideLineComment; */ /* obsolete */
  vstring newFileBuf = "";
  vstring inclPrefix = "";
  vstring tmpSource = "";
  vstring inclSource = "";
  vstring oldSource = "";
  vstring inclSuffix = "";

  long startOffset;
  long cmdPos1;
  long cmdPos2;
  long endPos1;
  long endPos2;
  char cmdType;
  long oldInclSize = 0; /* Init to avoid compiler warning */
  long newInclSize = 0; /* Init to avoid compiler warning */
  long befInclLineNum;
  long aftInclLineNum;
  /*long contLineNum;*/
  vstring includeFn = "";
  vstring fullInputFn = "";
  vstring fullIncludeFn = "";
  long alreadyInclBy;
  long saveInclCalls;

  let(&newFileBuf, fileBuf);  /* TODO - can this be avoided for speedup? */
  /* Look for and process includes */
  startOffset = 0;

  while (1) {
    getNextInclusion(newFileBuf, startOffset, /* inputs */
        /* outputs: */
        &cmdPos1, &cmdPos2,
        &endPos1, &endPos2,
        &cmdType, /* 'B' = "$( Begin [$..." through "$( End [$...",
                     'I' = "[$...",
                     'S' = "$( Skip [$...",
           TODO: add error code for missing $]?
                     'E' = Begin missing matched End
                     'N' = no include found; includeFn = "" */
        &includeFn /* name of included file */ );
    /*
    cmdType = 'B' or 'E':
          ....... $( Begin $[ prop.mm $] $)   ......   $( End $[ prop.mm $] $) ...
           ^      ^           ^^^^^^^       ^          ^                      ^
      startOffset cmdPos1    fileName  cmdPos2     endPos1              endPos2
                                                 (=0 if no End)   (=0 if no End)

       Note: in the special case of Begin, cmdPos2 points _after_ the whitespace
       after "$( Begin $[ prop.mm $] $)" i.e. the whitespace is considered part of
       the Begin command.  The is needed because prop.mm content doesn't
       necessarily start with whitespace.  prop.mm does, however, end with
       whitespace (\n) as enforced by readFileToString().

    cmdType = 'I':
          ............... $[ prop.mm $]  ..............
           ^              ^  ^^^^^^^   ^
      startOffset   cmdPos1 fileName  cmdPos2     endPos1=0  endPos2=0

    cmdType = 'S':
          ....... $( Skip $[ prop.mm $] $)   ......
           ^      ^          ^^^^^^^      ^
      startOffset cmdPos1    fileName  cmdPos2     endPos1=0  endPos2=0
    */

    if (cmdType == 'N') break; /* No (more) includes */
    if (cmdType == 'E') {
      /* TODO: Better error msg here or in getNextInclude */
      print2("?Error: \"$( Begin $[...\" without matching \"$( End $[...\"\n");
      startOffset = cmdPos2; /* Get passed the bad "$( Begin $[..." */
      *errorFlag = 1;
      continue;
    }

    /* Count lines between start of last source continuation and end of
       the inclusion, before the newFileBuf is modified */

    if (includeCall[includeCalls].pushOrPop != 1) bug(1764);
    /*
    contLineNum = includeCall[includeCalls].current_line
      + countLines(newFileBuf, ((cmdType == 'B') ? endPos2 : cmdPos2)
          - includeCall[includeCalls].current_offset);\
    */


    /* If we're here, cmdType is 'B', 'I', or 'S' */

    /* Create 2 new includeCall entries before recursive call, so that
       alreadyInclBy will scan entries in proper order (e.g. if this
       include calls itself at a deeper level - weird but not illegal - the
       deeper one should be Skip'd per the Metamath spec). */
    /* This entry is identified by pushOrPop = 0 */
    includeCalls++;
    /* We will use two more entries here (include call and return), and
       in parseKeywords() a dummy additional top entry is assumed to exist.
       Thus the comparison must be to 3 less than MAX_INCLUDECALLS. */
    if (includeCalls >= MAX_INCLUDECALLS - 3) {
      MAX_INCLUDECALLS = MAX_INCLUDECALLS + 20;
/*E*/if(db5)print2("'Include' call table was increased to %ld entries.\n",
/*E*/    MAX_INCLUDECALLS);
      includeCall = realloc(includeCall, (size_t)MAX_INCLUDECALLS *
          sizeof(struct includeCall_struct));
      if (includeCall == NULL) outOfMemory("#2 (includeCall)");
    }
    includeCall[includeCalls].pushOrPop = 0;

    /* This entry is identified by pushOrPop = 1 */
    includeCalls++;
    includeCall[includeCalls].pushOrPop = 1;
    /* Save the value before recursive calls will increment the global
       includeCalls */
    saveInclCalls = includeCalls;

    includeCall[saveInclCalls - 1].included_fn = "";  /* Initialize string */
    let(&(includeCall[saveInclCalls - 1].included_fn), includeFn); /* Name of the
       file in the inclusion statement e.g. "$( Begin $[ included_fn..." */
    includeCall[saveInclCalls].included_fn = "";
    let(&includeCall[saveInclCalls].included_fn,
        sourceFileName); /* Continuation of parent file after this include */


    /* See if includeFn file has already been included */
    alreadyInclBy = -1;
    for (i = 0; i <= saveInclCalls - 2; i++) {
      if (includeCall[i].pushOrPop == 0
         && !strcmp(includeCall[i].included_fn, includeFn)) {
        /*
        print2("%s",cat(
            "(File \"",
            includeCall[includeCalls].source_fn,
            "\", referenced at line ",
            str((double)(includeCall[includeCalls].calledBy_line)),
            " in \"",
            includeCall[includeCalls].calledBy_fn,
            "\", has already been included.)\n",NULL));
        */
        alreadyInclBy = i;
        break;
      }
    }
    if (alreadyInclBy == -1) {
      /* This is the first time the included file has been included */
      switch (cmdType) {
        case 'B':
          let(&inclPrefix, seg(newFileBuf, cmdPos1, cmdPos2 - 1)); /* Keep trailing
              \n (or other whitespace) as part of prefix for the special
              case of Begin - cmdPos2 points to char after \n */
          let(&inclSuffix, seg(newFileBuf, endPos1, endPos2 - 1));
          let(&tmpSource, seg(newFileBuf, cmdPos2, endPos1 - 1));  /* Save the
              included source */
          inclSize = endPos1 - cmdPos2; /* Actual included source size */

          /* Get the parent line number up to the inclusion */
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos2 - 1 - startOffset);
          includeCall[saveInclCalls - 1].current_line = befInclLineNum - 1;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos2/*start at (cmdPos2+1)th character*/,
              endPos2 - cmdPos2 - 1) + 1;
          includeCall[saveInclCalls].current_line = aftInclLineNum - 1;
          parentLineNum = aftInclLineNum;

          /* Call recursively to expand any includes in the included source */
          /* Use parentLineNum since the inclusion source is in the parent file */
          let(&inclSource, "");
          inclSource = readInclude(tmpSource,
              fileBufOffset + cmdPos1 - 1 + (long)strlen(inclPrefix), /*new offset*/
              /*includeFn,*/ sourceFileName,
              &inclSize /*input/output*/, befInclLineNum, &(*errorFlag));

          oldInclSize = endPos2 - cmdPos1; /* Includes old prefix and suffix */
          /*newInclSize = oldInclSize;*/ /* Includes new prefix and suffix */
          newInclSize = (long)strlen(inclPrefix) + inclSize +
                (long)strlen(inclSuffix); /* Includes new prefix and suffix */
          /* It is already a Begin comment, so leave it alone */
          /* *size = *size; */
          /* Adjust starting position for next inclusion search */
          startOffset = cmdPos1 + newInclSize - 1; /* -1 since startOffset is 0-based but
              cmdPos2 is 1-based */
          break;
        case 'I':
          /* Read the included file */
          let(&fullIncludeFn, cat(rootDirectory, includeFn, NULL));
          let(&tmpSource, "");
          tmpSource = readFileToString(fullIncludeFn, 0/*verbose*/, &inclSize);
          if (tmpSource == NULL) {
            /* TODO: print better error msg?*/
            print2(
                /* 23-Jan-2018 nm */
                "?Error: file \"%s%s\" (included in \"%s\") was not found\n",
                fullIncludeFn, rootDirectory, sourceFileName);
            tmpSource = "";
            inclSize = 0;
            *errorFlag = 1;
          } else {
            print2("Reading included file \"%s\"... %ld bytes\n",
                fullIncludeFn, inclSize);
          }

          /* Change inclusion command to Begin...End comment */
          let(&inclPrefix, cat("$( Begin $[ ", includeFn, " $] $)\n", NULL));
               /* Note that trailing whitespace is part of the prefix in
                  the special case of Begin, because the included file might
                  not start with whitespace.  However, the included file
                  will always end with whitespace i.e. \n as enforced by
                  readFileToString(). */
          let(&inclSuffix, cat("$( End $[ ", includeFn, " $] $)", NULL));

          /* Get the parent line number up to the inclusion */
          /* TODO: compute aftInclLineNum directly and eliminate befInclLineNum */
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos1 - 1 - startOffset);
          includeCall[saveInclCalls - 1].current_line = 0;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos1/*start at (cmdPos1+1)th character*/,
              cmdPos2 - cmdPos1 - 1);
          includeCall[saveInclCalls].current_line = aftInclLineNum;
          parentLineNum = aftInclLineNum;

          /* Call recursively to expand includes in the included source */
          /* Start at line 1 since source is in external file */
          let(&inclSource, "");
          inclSource = readInclude(tmpSource,
              fileBufOffset + cmdPos1 - 1 + (long)strlen(inclPrefix), /*new offset*/
              /*includeFn,*/ includeFn,
              &inclSize /*input/output*/, 1/*parentLineNum*/, &(*errorFlag));

          oldInclSize = cmdPos2 - cmdPos1; /* Includes old prefix and suffix */
          /* "$( Begin $[...$] $)" must have a whitespace
             after it; we use a newline.  readFileToString will ensure a
             newline at its end.  We don't add whitespace after
             "$( End $[...$] $)" but reuse the whitespace after the original
             "$[...$]". */
          let(&newFileBuf, cat(left(newFileBuf, cmdPos1 - 1),
              inclPrefix, inclSource, inclSuffix,
              right(newFileBuf, cmdPos2), NULL));
          *size = *size - (cmdPos2 - cmdPos1) + (long)strlen(inclPrefix)
              + inclSize + (long)strlen(inclSuffix);
          newInclSize = (long)strlen(inclPrefix) + inclSize +
                (long)strlen(inclSuffix); /* Includes new prefix and suffix */
          /* Adjust starting position for next inclusion search (which will
             be at the start of the included file continuing into the remaining
             parent file) */
          startOffset = cmdPos1 + newInclSize - 1; /* -1 since startOffset is
              0-based but cmdPos2 is 1-based */
              /*
              + inclSize  /@ Use instead of strlen for speed @/
              + (long)strlen(inclSuffix) */
              ;  /* -1 since startOffset is 0-based but cmdPos1 is 1-based */
          /* TODO: update line numbers for error msgs */
          break;
        case 'S':
          /* Read the included file */
          let(&fullIncludeFn, cat(rootDirectory, includeFn, NULL));
          let(&tmpSource, "");
          tmpSource = readFileToString(fullIncludeFn, 1/*verbose*/, &inclSize);
          if (tmpSource == NULL) {
            /* TODO: print better error msg */
            print2(
                /* 23-Jan-2018 nm */
                "?Error: file \"%s%s\" (included in \"%s\") was not found\n",
                fullIncludeFn, rootDirectory, sourceFileName);
            *errorFlag = 1;
            tmpSource = ""; /* Prevent seg fault */
            inclSize = 0;
          }

          /* Change Skip comment to Begin...End comment */
          let(&inclPrefix, cat("$( Begin $[ ", includeFn, " $] $)\n", NULL));
          let(&inclSuffix, cat("$( End $[ ", includeFn, " $] $)", NULL));

          /* Get the parent line number up to the inclusion */
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
          /* TODO: compute aftInclLineNum directly and eliminate befInclLineNum */
              cmdPos1 - 1 - startOffset);
          includeCall[saveInclCalls - 1].current_line = 0;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos1/*start at (cmdPos1+1)th character*/,
              cmdPos2 - cmdPos1 - 1);
          includeCall[saveInclCalls].current_line = aftInclLineNum;
          parentLineNum = aftInclLineNum;

          /* Call recursively to expand includes in the included source */
          /* Start at line 1 since source is in external file */
          let(&inclSource, "");
          inclSource = readInclude(tmpSource,
              fileBufOffset + cmdPos1 - 1 + (long)strlen(inclPrefix), /*new offset*/
              /*includeFn,*/ includeFn,
              &inclSize /*input/output*/, 1/*parentLineNum*/, &(*errorFlag));

          oldInclSize = cmdPos2 - cmdPos1; /* Includes old prefix and suffix */
          let(&newFileBuf, cat(left(newFileBuf, cmdPos1 - 1),
              inclPrefix, inclSource, inclSuffix,
              right(newFileBuf, cmdPos2), NULL));
          newInclSize = (long)strlen(inclPrefix) + inclSize +
                (long)strlen(inclSuffix); /* Includes new prefix and suffix */
          *size = *size - (cmdPos2 - cmdPos1) + (long)strlen(inclPrefix)
              + inclSize + (long)strlen(inclSuffix);
          /* Adjust starting position for next inclusion search */
          startOffset = cmdPos1 + newInclSize - 1; /* -1 since startOffset is
              0-based but cmdPos2 is 1-based */
          /* TODO: update line numbers for error msgs */
          break;
        default:
          bug(1745);
      } /* end switch (cmdType) */
    } else {
      /* This file has already been included.  Change Begin and $[ $] to
         Skip.  alreadyInclBy is the index of the previous includeCall that
         included it. */
      if (!(alreadyInclBy > 0)) bug(1765);
      switch (cmdType) {
        case 'B':
          /* Save the included source temporarily */
          let(&inclSource,
              seg(newFileBuf, cmdPos2, endPos1 - 1));
          /* Make sure it's content matches */
          let(&oldSource, "");
          oldSource = includeCall[  /* Connect to source for brevity */
                  alreadyInclBy
                  ].current_includeSource;
          if (strcmp(inclSource, oldSource)) {
            /* TODO - print better error msg */
            print2(
        "?Warning: \"$( Begin $[...\" source, with %ld characters, mismatches\n",
                (long)strlen(inclSource));
            print2(
                "  earlier inclusion, with %ld characters.\n",
                (long)strlen(oldSource));
          }
          oldSource = ""; /* Disconnect from source */
          /* We need to delete it from the source and change to Skip */
          let(&inclPrefix, cat("$( Skip $[ ", includeFn, " $] $)", NULL));
          let(&inclSuffix, "");

          /* Get the parent line number up to the inclusion */
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos2 - 1 - startOffset);
          includeCall[saveInclCalls - 1].current_line = befInclLineNum;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos2/*start at (cmdPos2+1)th character*/,
              endPos2 - cmdPos2 /*- 1*/);
          includeCall[saveInclCalls].current_line = aftInclLineNum;
          parentLineNum = aftInclLineNum;

          let(&inclSource, ""); /* Final source to be stored - none */
          inclSize = 0; /* Size of just the included source */
          oldInclSize = endPos2 - cmdPos1; /* Includes old prefix and suffix */
          let(&newFileBuf, cat(left(newFileBuf, cmdPos1 - 1),
              inclPrefix,
              right(newFileBuf, endPos2), NULL));
          newInclSize = (long)strlen(inclPrefix); /* Includes new prefix and suffix */
          *size = *size - (endPos2 - cmdPos1) + newInclSize;
          /* Adjust starting position for next inclusion search (which may
             occur inside the source we just included, so don't skip passed
             that source) */
          startOffset = cmdPos1 + newInclSize - 1; /* -1 since startOffset is
              0-based but cmdPos2 is 1-based */
          break;
        case 'I':
          /* Change inclusion command to Skip comment */
          let(&inclPrefix, cat("$( Skip $[ ", includeFn, " $] $)", NULL));
          let(&inclSuffix, "");

          /* Get the parent line number up to the inclusion */
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos1 - 1 - startOffset);
          includeCall[saveInclCalls - 1].current_line = befInclLineNum;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos1/*start at (cmdPos1+1)th character*/,
              cmdPos2 - cmdPos1 /*- 1*/);
          includeCall[saveInclCalls].current_line = aftInclLineNum;
          parentLineNum = aftInclLineNum;

          let(&inclSource, ""); /* Final source to be stored - none */
          inclSize = 0; /* Size of just the included source */
          oldInclSize = cmdPos2 - cmdPos1; /* Includes old prefix and suffix */
          let(&newFileBuf, cat(left(newFileBuf, cmdPos1 - 1),
              inclPrefix,
              right(newFileBuf, cmdPos2), NULL));
          newInclSize = (long)strlen(inclPrefix); /* Includes new prefix and suffix */
          *size = *size - (cmdPos2 - cmdPos1) + (long)strlen(inclPrefix);
          /* Adjust starting position for next inclusion search */
          startOffset = cmdPos1 + newInclSize - 1; /* -1 since startOffset is
              0-based but cmdPos2 is 1-based */
          break;
        case 'S':
          /* It is already Skipped, so leave it alone */
          /* *size = *size; */
          /* Adjust starting position for next inclusion search */
          let(&inclPrefix, seg(newFileBuf, cmdPos1, cmdPos2 - 1));
          let(&inclSuffix, "");

          /* Get the parent line number up to the inclusion */
          befInclLineNum = parentLineNum + countLines(
              newFileBuf + startOffset + 1,
              cmdPos1 - 1 - startOffset);
          includeCall[saveInclCalls - 1].current_line = befInclLineNum;
          aftInclLineNum = befInclLineNum + countLines(newFileBuf
              + cmdPos1/*start at (cmdPos1+1)th character*/,
              cmdPos2 - cmdPos1 /*- 1*/);
          includeCall[saveInclCalls].current_line = aftInclLineNum - 1;
          parentLineNum = aftInclLineNum;

          let(&inclSource, ""); /* Final source to be stored - none */
          inclSize = 0; /* Size of just the included source */
          oldInclSize = cmdPos2 - cmdPos1; /* Includes old prefix and suffix */
          newInclSize = oldInclSize; /* Includes new prefix and suffix */
          startOffset = cmdPos1 + newInclSize - 1; /* -1 since startOffset is
              0-based but cmdPos2 is 1-based */
          if (startOffset != cmdPos2 - 1) bug(1772);
          break;
        default:
          bug(1745);
      } /* end switch(cmdType) */
    } /* if alreadyInclBy == -1 */

    /* Assign structure with information for subsequent passes and
       (later) error messages */
    includeCall[saveInclCalls - 1].source_fn = "";  /* Name of the file where the
       inclusion source is located (= parent file for $( Begin $[... etc.) */
    includeCall[saveInclCalls - 1].current_offset = fileBufOffset + cmdPos1 - 1
        + (long)strlen(inclPrefix) - 1; /* This is the starting character position
            of the included file w.r.t entire source buffer */
    if (alreadyInclBy >= 0 || cmdType == 'B') {
      /* No external file was included, so let the includeFn be the
         same as the source */
      let(&includeCall[saveInclCalls - 1].source_fn, sourceFileName);
    } else {
      /* It hasn't been included yet, and cmdType is 'I' or 'S', meaning
         that we bring in an external file */
      let(&includeCall[saveInclCalls - 1].source_fn, includeFn); /* $[ $], Begin,
          or Skip file name for this inclusion */
      /*
      includeCall[saveInclCalls - 1].current_line = 0; */ /* The line number
          of the start of the included file (=1) */
    }
    includeCall[saveInclCalls - 1].current_includeSource = inclSource; /* let() not
        needed because we're just assigning a new name to inclSource */
    inclSource = ""; /* Detach from
        includeCall[saveInclCalls - 1].current_includeSource for later reuse */
    includeCall[saveInclCalls - 1].current_includeSource = "";
    includeCall[saveInclCalls - 1].current_includeLength = inclSize; /* Length of the file
        to be included (0 if the file was previously included) */


    /* Initialize a new include call for the continuation of the parent. */
    /* This entry is identified by pushOrPop = 1 */
    includeCall[saveInclCalls].source_fn = "";  /* Name of the file to be
        included */
    let(&includeCall[saveInclCalls].source_fn,
        sourceFileName); /* Source file containing
         this inclusion */
    includeCall[saveInclCalls].current_offset = fileBufOffset + cmdPos1
        + newInclSize - 1;
        /* This is the position of the continuation of the parent */
    includeCall[saveInclCalls].current_includeSource = ""; /* (Currently) assigned
        only if we may need it for a later Begin comparison */
    includeCall[saveInclCalls].current_includeLength = 0; /* Length of the file
        to be included (0 if the file was previously included) */

  } /* while (1) */

  /* Deallocate strings */
  let(&inclSource, "");
  let(&tmpSource, "");
  let(&oldSource, "");
  let(&inclPrefix, "");
  let(&inclSuffix, "");
  let(&includeFn, "");
  let(&fullInputFn, "");
  let(&fullIncludeFn, "");

  return newFileBuf;
} /* readInclude */


/* This function returns a pointer to a buffer containing the contents of an
   input file and its 'include' calls.  'Size' returns the buffer's size.  */
/* TODO - ability to flag error to skip raw source function */
/* If NULL is returned, it means a serious error occured (like missing file)
   and reading should be aborted. */
char *readSourceAndIncludes(vstring inputFn /*input*/, long *size /*output*/)
{
  long i;
/*D*//*long j;*/
/*D*//*vstring s=""; */
  vstring fileBuf = "";
  vstring newFileBuf = "";

  vstring fullInputFn = "";
  flag errorFlag = 0;

  /* Read starting file */
  let(&fullInputFn, cat(rootDirectory, inputFn, NULL));
  fileBuf = readFileToString(fullInputFn, 1/*verbose*/, &(*size));
  if (fileBuf == NULL) {
    print2(
        "?Error: file \"%s\" was not found\n", fullInputFn);
    fileBuf = "";
    *size = 0;
    errorFlag = 1;
    /* goto RETURN_POINT; */ /* Don't go now so that includeCall[]
         strings will be initialized.  If error, then blank fileBuf will
         cause main while loop to break immediately after first
         getNextInclusion() call. */
  }
  print2("Reading source file \"%s\"... %ld bytes\n", fullInputFn, *size);

  /* Create a ficticious initial include for the main file (at least 2
     includeCall structure array entries have been already been allocated
     in initBigArrays() in mmdata.c) */
  includeCalls = 0;
  includeCall[includeCalls].pushOrPop = 0; /* 0 means start of included file,
       1 means continuation of including file */
  includeCall[includeCalls].source_fn = "";
  let(&includeCall[includeCalls].source_fn, inputFn); /* $[ $], Begin,
      of Skip file name for this inclusion */
  includeCall[includeCalls].included_fn = "";
  let(&includeCall[includeCalls].included_fn, inputFn); /* $[ $], Begin,
      of Skip file name for this inclusion */
  includeCall[includeCalls].current_offset = 0;  /* This is the starting
      character position of the included file w.r.t entire source buffer */
  includeCall[includeCalls].current_line = 1; /* The line number
      of the start of the included file (=1) or the continuation line of
      the parent file */
  includeCall[includeCalls].current_includeSource = ""; /* (Currently) assigned
      only if we may need it for a later Begin comparison */
  includeCall[includeCalls].current_includeLength = *size; /* Length of the file
      to be included (0 if the file was previously included) */

  /* Create a ficticious entry for the "continuation" after the
     main file, to make error message line searching easier */
  includeCalls++;
  includeCall[includeCalls].pushOrPop = 1; /* 0 means start of included file,
       1 means continuation of including file */
  includeCall[includeCalls].source_fn = "";
  /*let(&includeCall[includeCalls].source_fn, inputFn);*/ /* Leave empty;
        there is no "continuation" file for the main file, so no need to assign
        (it won't be used) */
  includeCall[includeCalls].included_fn = "";
  /*let(&includeCall[includeCalls].included_fn, inputFn);*/ /* $[ $], Begin,
      of Skip file name for this inclusion */
  includeCall[includeCalls].current_line = -1; /* Ideally this should be
      countLines(fileBuf), but since it's never used we don't bother to
      call countLines(fileBuf) to save CPU time */
  includeCall[includeCalls].current_includeSource = ""; /* (Currently) assigned
      only if we may need it for a later Begin comparison */
  includeCall[includeCalls].current_includeLength = 0; /* The "continuation"
      of the main file is ficticious, so just set it to 0 length */

  /* Recursively expand the source of an included file */
  newFileBuf = "";
  newFileBuf = readInclude(fileBuf, 0, /*inputFn,*/ inputFn, &(*size),
      1/*parentLineNum*/, &errorFlag);
  includeCall[1].current_offset = *size;  /* This is the starting
      character position of the included file w.r.t entire source buffer.
      Here, it points to the nonexistent character just beyond end of main file
      (after all includes are expanded).
      Note that readInclude() may change includeCalls, so use 1 explicitly. */
  let(&fileBuf, ""); /* Deallocate */
/*D*//*printf("*size=%ld\n",*size);                                       */
/*D*//*for(i=0;i<*size;i++){                                              */
/*D*//*let(&s,"");                                                        */
/*D*//*s=getFileAndLineNum(newFileBuf,newFileBuf+i,&j);                   */
/*D*//*printf("i=%ld ln=%ld fn=%s ch=%c\n",i,j,s,(newFileBuf+i)[0]);  }   */
  if (errorFlag == 1) {
    /* The read should be aborted by the caller. */
    /* Deallocate the strings in the includeCall[] structure. */
    for (i = 0; i <= includeCalls; i++) {
      let(&includeCall[i].source_fn, "");
      let(&includeCall[i].included_fn, "");
      let(&includeCall[i].current_includeSource, "");
      includeCalls = -1; /* For the eraseSource() function in mmcmds.c */
    }
    return NULL;
  } else {
/*D*//*for (i=0; i<=includeCalls;i++)                                       */
/*D*//*  printf("i=%ld p=%ld f=%s,%s l=%ld o=%ld s=%ld\n",                  */
/*D*//*i,(long)includeCall[i].pushOrPop,includeCall[i].source_fn,           */
/*D*//*includeCall[i].included_fn,includeCall[i].current_line,              */
/*D*//*includeCall[i].current_offset,includeCall[i].current_includeLength); */
    return newFileBuf;
  }

} /* readSourceAndIncludes */

