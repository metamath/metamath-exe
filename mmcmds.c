/*****************************************************************************/
/*        Copyright (C) 2015  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* mmcmds.c - assorted user commands */

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <time.h>  /* 28-May-04 nm For clock() */
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmdl.h" /* For texFileName */
#include "mmcmds.h"
#include "mminou.h"
#include "mmpars.h"
#include "mmveri.h"
#include "mmwtex.h" /* For htmlVarColors,... */
#include "mmpfas.h"
#include "mmunif.h" /* 26-Sep-2010 nm For bracketMatchInit, minSubstLen */

/* vstring mainFileName = ""; */ /* 28-Dec-05 nm Obsolete */
flag printHelp = 0;

/* For HTML output */
vstring printStringForReferencedBy = "";

/* For MIDI */
flag midiFlag = 0;
vstring midiParam = "";

/* Type (i.e. print) a statement */
void typeStatement(long showStmt,
  flag briefFlag,
  flag commentOnlyFlag,
  flag texFlag,
  flag htmlFlg)
{
  /* From HELP SHOW STATEMENT: Optional qualifiers:
    / TEX - This qualifier will write the statement information to the
        LaTeX file previously opened with OPEN TEX.
            [Note:  texFlag=1 and htmlFlg=0 with this qualifier.]

    / HTML - This qualifier will write the statement information to the
        HTML file previously opened with OPEN HTML.
            [Note:  texFlag=1 and htmlFlg=1 with this qualifier.]

    / COMMENT_ONLY - This qualifier will show only the comment that
        immediatley precedes the statement.  This is useful when you are
        using Metamath to preprocess LaTeX source you have created (see
        HELP TEX)

    / BRIEF - This qualifier shows the statement and its $e hypotheses
        only.
  */
  long i, j, k, m, n;
  vstring str1 = "", str2 = "", str3 = "";
  nmbrString *nmbrTmpPtr1; /* Pointer only; not allocated directly */
  nmbrString *nmbrTmpPtr2; /* Pointer only; not allocated directly */
  nmbrString *nmbrDDList = NULL_NMBRSTRING;
  flag q1, q2;
  flag type;
  flag subType;
  vstring htmlDistinctVars = ""; /* 12/23/01 */
  char htmlDistinctVarsCommaFlag = 0; /* 12/23/01 */
  vstring str4 = ""; /* 10/10/02 */
  vstring str5 = ""; /* 19-Sep-2012 nm */
  long distVarGrps = 0;  /* 11-Aug-2006 nm */

  /* For syntax breakdown of definitions in HTML page */
  long zapStatement1stToken;
  static long wffToken = -1; /* array index of the hard-coded token "wff" -
      static so we only have to look it up once - set to -2 if not found */

  subType = 0; /* Assign to prevent compiler warnings - not theor. necessary */

  if (!showStmt) bug(225); /* Must be 1 or greater */

  if (!commentOnlyFlag && !briefFlag) {
    let(&str1, cat("Statement ", str(showStmt),
        " is located on line ", str(statement[showStmt].lineNum),
        " of the file ", NULL));
    if (!texFlag) {
      printLongLine(cat(str1,
        "\"", statement[showStmt].fileName,
        "\".",
        /* 8-Feb-2007 nm Added HTML page info to SHOW STATEMENT ... /FULL */
        (statement[showStmt].pinkNumber == 0) ?   /* !=0 means $a or $p */
           "" :
           cat("  Its statement number for HTML pages is ",
               str(statement[showStmt].pinkNumber), ".", NULL),
        NULL), "", " ");
    } else {
      if (!htmlFlg) let(&printString, "");
      outputToString = 1; /* Flag for print2 to add to printString */
                     /* Note that printTexLongMathString resets it */
      if (!(htmlFlg && texFlag)) {
        /*
        printLongLine(cat(str1, "{\\tt ",
            asciiToTt(statement[showStmt].fileName),
            "}.", NULL), "", " ");
        */
      } else {
        /* For categorizing html pages, we use the source file convention
           that syntax statements don't start with "|-" and that axioms
           have labels starting with "ax-".  It is up to the database
           creator to follow this standard, which is not enforced. */
#define SYNTAX 1
#define DEFINITION 2
#define AXIOM 3
#define THEOREM 4
        if (statement[showStmt].type == (char)p_) {
          subType = THEOREM;
        } else {
          /* Must be a_ due to filter in main() */
          if (statement[showStmt].type != (char)a_) bug(228);
          if (strcmp("|-", mathToken[
              (statement[showStmt].mathString)[0]].tokenName)) {
            subType = SYNTAX;
          } else {
            if (!strcmp("ax-", left(statement[showStmt].labelName, 3))) {
              subType = AXIOM;
            } else {
              subType = DEFINITION;
            }
          }
        }
        switch (subType) {
          case SYNTAX:
            let(&str1, "Syntax Definition"); break;
          case DEFINITION:
            let(&str1, "Definition"); break;
          case AXIOM:
            let(&str1, "Axiom"); break;
          case THEOREM:
            let(&str1, "Theorem"); break;
          default:
            bug(229);
        }

        /* Print a small pink statement number after the statement */
        let(&str2, "");
        str2 = pinkHTML(showStmt);
        printLongLine(cat("<CENTER><B><FONT SIZE=\"+1\">", str1,
            " <FONT COLOR=", GREEN_TITLE_COLOR,
            ">", statement[showStmt].labelName,
            "</FONT></FONT></B>", str2, "</CENTER>", NULL), "", "\"");
      } /* (htmlFlg && texFlag) */
      outputToString = 0;
    } /* texFlag */
  }

  if (!briefFlag || commentOnlyFlag) {
    let(&str1, "");
    str1 = getDescription(showStmt);
    if (!str1[0]    /* No comment */
        || (str1[0] == '[' && str1[strlen(str1) - 1] == ']')
        /* 7-Sep-04 Allow both "$([<date>])$" and "$( [<date>] )$" */
        || (strlen(str1) > 1 &&
            str1[1] == '[' && str1[strlen(str1) - 2] == ']')) { /* Date stamp
            from previous proof */
      print2("?Warning:  Statement \"%s\" has no comment\n",
          statement[showStmt].labelName);
      /* 14-Sep-2010 nm We must print a blank comment to have \begin{lemma} */
      if (texFlag && !htmlFlg && !oldTexFlag) {
        let(&str1, "TO DO: PUT DESCRIPTION HERE");
      }
    }
    if (str1[0]) {
      if (!texFlag) {
        printLongLine(cat("\"", str1, "\"", NULL), "", " ");
      } else {
        /* 10/10/02 This is now done in mmwtex.c printTexComment */
        /* Although it will affect the 2nd (and only other) call to
           printTexComment below, that call is obsolete and there should
           be no side effect. */
        /*******
        if (htmlFlg && texFlag) {
          let(&str1, cat("<CENTER><B>Description: </B>", str1,
              "</CENTER><BR>", NULL));
        }
        *******/
        if (!htmlFlg) {  /* LaTeX */
          if (!oldTexFlag) {
            /* 14-Sep-2010 */
            let(&str1, cat("\\begin{lemma}\\label{lem:",
                statement[showStmt].labelName, "} ", str1, NULL));
          } else {
            /* 6-Dec-03 Add separation space between theorems */
            let(&str1, cat("\n\\vspace{1ex} %2\n\n", str1, NULL));
          }
        }
        printTexComment(str1, 1);
      }
    }
  }
  if (commentOnlyFlag && !briefFlag) goto returnPoint;

  if ((briefFlag && !texFlag) ||
       (htmlFlg && texFlag) /* HTML page 12/23/01 */) {
    /* In BRIEF mode screen output, show $d's */
    /* This section was added 8/31/99 */

    /* 12/23/01 - added algorithm to HTML pages also; the string to print out
       is stored in htmlDistinctVars for later printing */

    /* 12/23/01 */
    if (htmlFlg && texFlag) {
      let(&htmlDistinctVars, "");
      htmlDistinctVarsCommaFlag = 0;
    }

    /* Note added 22-Aug-04:  This algorithm is used to re-merge $d pairs
       into groups of 3 or more when possible, for a more compact display.
       The algorithm does not merge groups optimally, but it should be
       adequate.  For example, in set.mm (e.g. old r19.23aivv):
         $d x ps $.  $d y ps $.  $d y A $.  $d x y $.
       produces in SHOW STATEMENT (note redundant 3rd $d):
         $d ps x y $.  $d y A $.  $d x y $.
       However, in set.mm the equivalent (and better anyway):
         $d x y ps $.  $d y A $.
       produces the same thing when remerged in SHOW STATEMENT. */
    let(&str1, "");
    nmbrTmpPtr1 = statement[showStmt].reqDisjVarsA;
    nmbrTmpPtr2 = statement[showStmt].reqDisjVarsB;
    i = nmbrLen(nmbrTmpPtr1);
    if (i /* Number of mandatory $d pairs */) {
      nmbrLet(&nmbrDDList, NULL_NMBRSTRING);
      for (k = 0; k < i; k++) {
        /* Is one of the variables in the current list? */
        if (!nmbrElementIn(1, nmbrDDList, nmbrTmpPtr1[k]) &&
            !nmbrElementIn(1, nmbrDDList, nmbrTmpPtr2[k])) {
          /* No, so close out the current list */
          if (!(htmlFlg && texFlag)) { /* 12/23/01 */
            if (k == 0) let(&str1, "$d");
            else let(&str1, cat(str1, " $.  $d", NULL));
          } else {

            /* 12/23/01 */
            let(&htmlDistinctVars, cat(htmlDistinctVars, " &nbsp; ",
                NULL));
            htmlDistinctVarsCommaFlag = 0;
            distVarGrps++;  /* 11-Aug-2006 nm */

          }
          nmbrLet(&nmbrDDList, NULL_NMBRSTRING);
        }
        /* Are both variables required to be distinct from all others
           in current list? */
        for (n = 0; n < nmbrLen(nmbrDDList); n++) {
          if (nmbrDDList[n] != nmbrTmpPtr1[k] &&
              nmbrDDList[n] != nmbrTmpPtr2[k]) {
            q1 = 0; q2 = 0;
            for (m = 0; m < i; m++) {
              if ((nmbrTmpPtr1[m] == nmbrDDList[n] &&
                      nmbrTmpPtr2[m] == nmbrTmpPtr1[k]) ||
                  (nmbrTmpPtr2[m] == nmbrDDList[n] &&
                      nmbrTmpPtr1[m] == nmbrTmpPtr1[k])) {
                q1 = 1; /* 1st var is required to be distinct */
              }
              if ((nmbrTmpPtr1[m] == nmbrDDList[n] &&
                      nmbrTmpPtr2[m] == nmbrTmpPtr2[k]) ||
                  (nmbrTmpPtr2[m] == nmbrDDList[n] &&
                      nmbrTmpPtr1[m] == nmbrTmpPtr2[k])) {
                q2 = 1;  /* 2nd var is required to be distinct */
              }
              if (q1 && q2) break; /* Found both */
            }  /* Next m */
            if (!q1 || !q2) {
              /* One of the variables is not required to be distinct from
                all others in the current list, so close out current list */
              if (!(htmlFlg && texFlag)) {
                if (k == 0) let(&str1, "$d");
                else let(&str1, cat(str1, " $.  $d", NULL));
              } else {

                /* 12/23/01 */
                let(&htmlDistinctVars, cat(htmlDistinctVars, " &nbsp; ",
                    NULL));
                htmlDistinctVarsCommaFlag = 0;
                distVarGrps++;  /* 11-Aug-2006 nm */

              }
              nmbrLet(&nmbrDDList, NULL_NMBRSTRING);
              break;
            }
          } /* If $d var in current list is not same as one we're adding */
        } /* Next n */
        /* If the variable is not already in current list, add it */
        if (!nmbrElementIn(1, nmbrDDList, nmbrTmpPtr1[k])) {
          if (!(htmlFlg && texFlag)) {
            let(&str1, cat(str1, " ", mathToken[nmbrTmpPtr1[k]].tokenName,
                NULL));
          } else {

            /* 12/23/01 */
            if (htmlDistinctVarsCommaFlag) {
              let(&htmlDistinctVars, cat(htmlDistinctVars, ",", NULL));
            }
            htmlDistinctVarsCommaFlag = 1;
            let(&str2, "");
            str2 = tokenToTex(mathToken[nmbrTmpPtr1[k]].tokenName, showStmt);
                 /* tokenToTex allocates str2; we must deallocate it */
            let(&htmlDistinctVars, cat(htmlDistinctVars, str2, NULL));

          }
          nmbrLet(&nmbrDDList, nmbrAddElement(nmbrDDList, nmbrTmpPtr1[k]));
        }
        if (!nmbrElementIn(1, nmbrDDList, nmbrTmpPtr2[k])) {
          if (!(htmlFlg && texFlag)) {
            let(&str1, cat(str1, " ", mathToken[nmbrTmpPtr2[k]].tokenName,
                NULL));
          } else {

            /* 12/23/01 */
            if (htmlDistinctVarsCommaFlag) {
              let(&htmlDistinctVars, cat(htmlDistinctVars, ",", NULL));
            }
            htmlDistinctVarsCommaFlag = 1;
            let(&str2, "");
            str2 = tokenToTex(mathToken[nmbrTmpPtr2[k]].tokenName, showStmt);
                 /* tokenToTex allocates str2; we must deallocate it */
            let(&htmlDistinctVars, cat(htmlDistinctVars, str2, NULL));

          }
          nmbrLet(&nmbrDDList, nmbrAddElement(nmbrDDList, nmbrTmpPtr2[k]));
        }
      } /* Next k */
      /* Close out entire list */
      if (!(htmlFlg && texFlag)) {
        let(&str1, cat(str1, " $.", NULL));
        printLongLine(str1, "  ", " ");
      } else {

        /* 12/23/01 */
        /* (do nothing) */
        /*let(&htmlDistinctVars, cat(htmlDistinctVars, "<BR>", NULL));*/

      }
    } /* if i(#$d's) > 0 */
  }

  if (briefFlag || texFlag /*(texFlag && htmlFlg)*/) { /* 6-Dec-03 */
    /* For BRIEF mode, print $e hypotheses (only) before statement */
    /* Also do it for HTML output */
    /* 6-Dec-03  For the LaTeX output, now print hypotheses before statement */
    j = nmbrLen(statement[showStmt].reqHypList);
    k = 0;
    for (i = 0; i < j; i++) {
      /* Count the number of essential hypotheses */
      if (statement[statement[showStmt].reqHypList[i]].type
        == (char)e_) k++;

      /* Added 5/26/03 */
      /* For syntax definitions, also include $f hypotheses so user can more
         easily match them in syntax breakdowns of axioms and definitions */
      if (subType == SYNTAX && (texFlag && htmlFlg)) {
        if (statement[statement[showStmt].reqHypList[i]].type
          == (char)f_) k++;
      }

    }
    if (k) {
      if (texFlag) {
        /* Note that printTexLongMath resets it to 0 */
        outputToString = 1;
      }
      if (texFlag && htmlFlg) {
        print2("<CENTER><TABLE BORDER CELLSPACING=0 BGCOLOR=%s\n",
            MINT_BACKGROUND_COLOR);
        /* For bobby.cast.org approval */
        print2("SUMMARY=\"%s\">\n", (k == 1) ? "Hypothesis" : "Hypotheses");
        print2("<CAPTION><B>%s</B></CAPTION>\n",
            (k == 1) ? "Hypothesis" : "Hypotheses");
        print2("<TR><TH>Ref\n");
        print2("</TH><TH>Expression</TH></TR>\n");
      }
      for (i = 0; i < j; i++) {
        k = statement[showStmt].reqHypList[i];
        if (statement[k].type != (char)e_

            /* Added 5/26/03 */
            /* For syntax definitions, include $f hypotheses so user can more
               easily match them in syntax breakdowns of axioms & definitions */
            && !(subType == SYNTAX && (texFlag && htmlFlg)
                && statement[k].type == (char)f_)

            ) continue;

        if (!texFlag) {
          let(&str2, cat(str(k), " ", NULL));
        } else {
          let(&str2, "  ");
        }
        let(&str2, cat(str2, statement[k].labelName,
            " $", chr(statement[k].type), " ", NULL));
        if (!texFlag) {
          printLongLine(cat(str2,
              nmbrCvtMToVString(statement[k].mathString), " $.", NULL),
              "      "," ");
        } else { /* if texFlag */
          /* texFlag was (misleadingly) included below to facilitate search
             for "htmlFlg && texFlag". */
          if (!(htmlFlg && texFlag)) {
            if (!oldTexFlag) {  /* 14-Sep-2010 nm */
              /* Do nothing */
            } else {
              let(&str3, space((long)strlen(str2)));
              printTexLongMath(statement[k].mathString,
                  str2, str3, 0, 0);
            }
          } else {
            outputToString = 1;
            print2("<TR ALIGN=LEFT><TD>%s</TD><TD>\n",
                statement[k].labelName);
            /* Print hypothesis */
            printTexLongMath(statement[k].mathString, "", "", 0, 0);
          }
        }
      } /* next i */
      if (texFlag && htmlFlg) {
        outputToString = 1;
        print2("</TABLE></CENTER>\n");
      }
    } /* if k (#essential hyp) */
  }

  let(&str1, "");
  type = statement[showStmt].type;
  if (type == p_) let(&str1, " $= ...");
  if (!texFlag)
    let(&str2, cat(str(showStmt), " ", NULL));
  else
    let(&str2, "  ");
  let(&str2, cat(str2, statement[showStmt].labelName,
      " $",chr(type), " ", NULL));
  if (!texFlag) {
    printLongLine(cat(str2,
        nmbrCvtMToVString(statement[showStmt].mathString),
        str1, " $.", NULL), "      ", " ");
  } else {
    if (!(htmlFlg && texFlag)) {  /* really !htmlFlg & texFlag */
      if (!oldTexFlag) {
        /* 14-Sep-2010 nm new LaTeX code: */
        outputToString = 1;
        print2("\\begin{align}\n");
        let(&str3, "");
        /* Get HTML hypotheses => assertion */
        str3 = getTexOrHtmlHypAndAssertion(showStmt); /* In mmwtex.c */
        printLongLine(cat(str3,
              /* No space before \label to make it easier to find last
                 parenthesis in a post-processing script */
              "\\label{eq:",
              statement[showStmt].labelName,
              "}",
              NULL), "    ", " ");
       /* print2("    \\label{eq:%s}\n",statement[showStmt].labelName); */
        print2("\\end{align}\n");
        print2("\\end{lemma}\n");
        fprintf(texFilePtr, "%s", printString);
        let(&printString, "");
        outputToString = 0;

      } else { /* old TeX code */
        let(&str3, space((long)strlen(str2))); /* 3rd argument of printTexLongMath
            cannot be temp allocated */
        printTexLongMath(statement[showStmt].mathString,
            str2, str3, 0, 0);
      }
    } else { /* (htmlFlg && texFlag) */
      outputToString = 1;
      print2("<CENTER><TABLE BORDER CELLSPACING=0 BGCOLOR=%s\n",
          MINT_BACKGROUND_COLOR);
      /* For bobby.cast.org approval */
      print2("SUMMARY=\"Assertion\">\n");
      print2("<CAPTION><B>Assertion</B></CAPTION>\n");
      print2("<TR><TH>Ref\n");
      print2("</TH><TH>Expression</TH></TR>\n");
      printLongLine(cat(
       "<TR ALIGN=LEFT><TD><FONT COLOR=",
          GREEN_TITLE_COLOR, "><B>", statement[showStmt].labelName,
          "</B></FONT></TD><TD>", NULL), "      ", " ");
      printTexLongMath(statement[showStmt].mathString, "", "", 0, 0);
      outputToString = 1;
      print2("</TABLE></CENTER>\n");
    }
  }

  if (briefFlag) goto returnPoint;

  /* 6-Dec-03 In the LaTeX output, the hypotheses used to be printed after the
     statement.  Now they are printed before (see above 6-Dec-03 comments),
     so some code below is commented out. */
  switch (type) {
    case a_:
    case p_:
      /* 6-Dec-03  This is not really needed but keeps output consistent
         with previous version.  It puts a blank line before the HTML
         "distinct variable" list. */
      if (texFlag && htmlFlg) { /* 6-Dec-03 */
        outputToString = 1;
        print2("\n");
        outputToString = 0;
      }

      /*if (!(htmlFlg && texFlag)) {*/
      if (!texFlag) {  /* 6-Dec-03 fix */
        print2("Its mandatory hypotheses in RPN order are:\n");
      }
      /*if (texFlag) outputToString = 0;*/ /* 6-Dec-03 */
      j = nmbrLen(statement[showStmt].reqHypList);
      for (i = 0; i < j; i++) {
        k = statement[showStmt].reqHypList[i];
        if (statement[k].type != (char)e_ && (!htmlFlg && texFlag))
          continue; /* 9/2/99 Don't put $f's in LaTeX output */
        let(&str2, cat("  ",statement[k].labelName,
            " $", chr(statement[k].type), " ", NULL));
        if (!texFlag) {
          printLongLine(cat(str2,
              nmbrCvtMToVString(statement[k].mathString), " $.", NULL),
              "      "," ");
        } else {
          if (!(htmlFlg && texFlag)) {  /* LaTeX */
            /*let(&str3, space((long)strlen(str2)));*/ /* 6-Dec-03 */
            /* This clears out printString */
            /*printTexLongMath(statement[k].mathString,
                str2, str3, 0, 0);*/ /* 6-Dec-03 */
          }
        }
      }
      /* 6-Dec-03  This is not really needed but keeps output consistent
         with previous version.  It puts a blank line before the HTML
         "distinct variable" list. */
      if (texFlag && htmlFlg) { /* 6-Dec-03 */
        outputToString = 1;
        print2("\n");
        outputToString = 0;
      }
      /*if (j == 0 && !(htmlFlg && texFlag)) print2("  (None)\n");*/
      if (j == 0 && !texFlag) print2("  (None)\n"); /* 6-Dec-03 fix */
      let(&str1, "");
      nmbrTmpPtr1 = statement[showStmt].reqDisjVarsA;
      nmbrTmpPtr2 = statement[showStmt].reqDisjVarsB;
      i = nmbrLen(nmbrTmpPtr1);
      if (i) {
        for (k = 0; k < i; k++) {
          if (!texFlag) {
            let(&str1, cat(str1, ", <",
                mathToken[nmbrTmpPtr1[k]].tokenName, ",",
                mathToken[nmbrTmpPtr2[k]].tokenName, ">", NULL));
          } else {
            if (htmlFlg && texFlag) {
              let(&str2, "");
              str2 = tokenToTex(mathToken[nmbrTmpPtr1[k]].tokenName, showStmt);
                   /* tokenToTex allocates str2; we must deallocate it */
              let(&str1, cat(str1, " &nbsp; ", str2, NULL));
              let(&str2, "");
              str2 = tokenToTex(mathToken[nmbrTmpPtr2[k]].tokenName, showStmt);
              let(&str1, cat(str1, ",", str2, NULL));
            }
          }
        }
        if (!texFlag)
          printLongLine(cat(
              "Its mandatory disjoint variable pairs are:  ",
              right(str1,3),NULL),"  "," ");
      }
      if (type == p_ &&
          nmbrLen(statement[showStmt].optHypList)
          && !texFlag) {
        printLongLine(cat(
           "Its optional hypotheses are:  ",
            nmbrCvtRToVString(
            statement[showStmt].optHypList),NULL),
            "      "," ");
      }
      nmbrTmpPtr1 = statement[showStmt].optDisjVarsA;
      nmbrTmpPtr2 = statement[showStmt].optDisjVarsB;
      i = nmbrLen(nmbrTmpPtr1);
      if (i && type == p_) {
        if (!texFlag) {
          let(&str1, "");
        } else {
          if (htmlFlg && texFlag) {
            /*  12/1/01 don't output dummy variables
            let(&str1, cat(str1,
            " &nbsp; (Dummy variables for use in proof:) ", NULL));
            */
          }
        }
        for (k = 0; k < i; k++) {
          if (!texFlag) {
            let(&str1, cat(str1, ", <",
                mathToken[nmbrTmpPtr1[k]].tokenName, ",",
                mathToken[nmbrTmpPtr2[k]].tokenName, ">", NULL));
          } else {
            if (htmlFlg && texFlag) {
                   /* tokenToTex allocates str2; we must deallocate it */
              /*  12/1/01 don't output dummy variables
              let(&str2, "");
              str2 = tokenToTex(mathToken[nmbrTmpPtr1[k]].tokenName, showStmt);
              let(&str1, cat(str1, " &nbsp; ", str2, NULL));
              let(&str2, "");
              str2 = tokenToTex(mathToken[nmbrTmpPtr2[k]].tokenName, showStmt);
              let(&str1, cat(str1, ",", str2, NULL));
              */
            }
          }
        }
        if (!texFlag) {
          printLongLine(cat(
              "Its optional disjoint variable pairs are:  ",
              right(str1,3),NULL),"  "," ");
        }
      }

      /* Before 12/23/01 **********
           Future: once stable, take out redundant code producing str1
      if (texFlag && htmlFlg && str1[0]) {
        outputToString = 1;
        printLongLine(cat("<CENTER>Substitutions into these variable",
            " pairs may not have variables in common: ",
            str1, "</CENTER>", NULL), "", " ");
        outputToString = 0;
      }
      ***********/


      /* 12/23/01 */
      if (texFlag && htmlFlg && htmlDistinctVars[0]) {
        outputToString = 1;
        printLongLine(cat(
     "<CENTER><A HREF=\"mmset.html#distinct\">Distinct variable</A> group",
            /* 11-Aug-2006 nm Determine whether "group" or "groups". */
            distVarGrps > 1 ? "s" : "",  /* 11-Aug-2006 */
            ": ", htmlDistinctVars, "</CENTER>", NULL), "", "\"");
        /* original code:
        printLongLine(cat(
     "<CENTER><A HREF=\"mmset.html#distinct\">Distinct variable</A> group(s): ",
            htmlDistinctVars, "</CENTER>", NULL), "", "\"");
        */
        outputToString = 0;
      }

      /* 4-Jan-2014 nm */
      if (texFlag && htmlFlg) {
        let(&str2, "");
        str2 = htmlAllowedSubst(showStmt);
        if (str2[0] != 0) {
          outputToString = 1;
          /* Print the list of allowed free variables */
          printLongLine(str2, "", "\"");
          outputToString = 0;
        }
      }

      if (texFlag) {
        outputToString = 1;
        if (htmlFlg && texFlag) print2("<HR NOSHADE SIZE=1>\n");
        outputToString = 0; /* Restore normal output */
        /* will be done automatically at closing
        fprintf(texFilePtr, "%s", printString);
        let(&printString, "");
        */
        break;
      }
      let(&str1, nmbrCvtMToVString(
          statement[showStmt].reqVarList));
      if (!strlen(str1)) let(&str1, "(None)");
      printLongLine(cat(
          "The statement and its hypotheses require the variables:  ",
          str1, NULL), "      ", " ");
      if (type == p_ &&
          nmbrLen(statement[showStmt].optVarList)) {
        printLongLine(cat(
            "These additional variables are allowed in its proof:  "
            ,nmbrCvtMToVString(
            statement[showStmt].optVarList),NULL),"      ",
            " ");
        /*??? Add variables required by proof */
      }
      /* Note:  statement[].reqVarList is only stored for $a and $p
         statements, not for $e or $f. */
      let(&str1, nmbrCvtMToVString(
          statement[showStmt].reqVarList));
      if (!strlen(str1)) let(&str1, "(None)");
      printLongLine(cat("The variables it contains are:  ",
          str1, NULL),
          "      ", " ");
      break;
    default:
      break;
  } /* End switch(type) */
  if (texFlag) {
    outputToString = 0;
    /* will be done automatically at closing
    fprintf(texFilePtr, "%s", printString);
    let(&printString, "");
    */
  }

  /* Start of finding definition for syntax statement */
  if (htmlFlg && texFlag) {

    /* For syntax declarations, find the first definition that follows
       it.  It is up to the user to arrange the database so that a
       meaningful definition is picked. */
    if (subType == SYNTAX) {
      for (i = showStmt + 1; i <= statements; i++) {
        if (statement[i].type == (char)a_) {
          if (!strcmp("|-", mathToken[
              (statement[i].mathString)[0]].tokenName)) {
            /* It's a definition or axiom */
            /* See if each constant token in the syntax statement
               exists in the definition; if not don't use the definition */
            j = 1;
            /* We start with k=1 for 2nd token (1st is wff, class, etc.) */
            for (k = 1; k < statement[showStmt].mathStringLen; k++) {
              if (mathToken[(statement[showStmt].mathString)[k]].
                  tokenType == (char)con_) {
                if (!nmbrElementIn(1, statement[i].mathString,
                    (statement[showStmt].mathString)[k])) {
                  /* The definition being considered doesn't have one of
                     the constant symbols in the syntax statement, so
                     reject it */
                  j = 0;
                  break; /* Out of k loop */
                }
              }
            } /* Next k */
            if (j) {
              /* Successful - use this definition or axiom as the reference */
              outputToString = 1;
              let(&str1, left(statement[i].labelName, 3));
              let(&str2, "");
              str2 = pinkHTML(i);
              if (!strcmp(str1, "ax-")) {
                printLongLine(cat(
                    "<CENTER>This syntax is primitive.",
                    "  The first axiom using it is <A HREF=\"",
                    statement[i].labelName, ".html\">",
                    statement[i].labelName,
                    "</A>", str2, ".</CENTER><HR NOSHADE SIZE=1>",
                    NULL), "", "\"");
              } else {
                printLongLine(cat(
                    "<CENTER>See definition <A HREF=\"",
                    statement[i].labelName, ".html\">",
                    statement[i].labelName, "</A>", str2,
                    " for more information.</CENTER><HR NOSHADE SIZE=1>",
                    NULL), "", "\"");
              }

              /* 10/10/02 Moved here from mmwtex.c */
              /*print2("<FONT SIZE=-1 FACE=sans-serif>Colors of variables:\n");*/
              printLongLine(cat(
                  "<CENTER><TABLE CELLSPACING=7><TR><TD ALIGN=LEFT><FONT SIZE=-1>",
                  "<B>Colors of variables:</B> ",
                  htmlVarColors, "</FONT></TD></TR>",
                  NULL), "", "\"");
              outputToString = 0;
              break; /* Out of i loop */
            }
          }
        }
      } /* Next i */
    } /* if (subType == SYNTAX) */


    /* For definitions, we pretend that the definition is a "wff" (hard-coded
       here; the .mm database provided by the user must use this convention).
       We use the proof assistant tools to prove that the statement is
       a wff, then we print the wff construction proof to the HTML file. */
    if (subType == DEFINITION || subType == AXIOM) {

      /* Look up the token "wff" if we haven't found it before */
      if (wffToken == -1) { /* First time */
        wffToken = -2; /* In case it's not found because the user's source
            used a convention different for "wff" for wffs */
        for (i = 0; i < mathTokens; i++) {
          if (!strcmp("wff", mathToken[i].tokenName)) {
            wffToken = i;
            break;
          }
        }
      }

      if (wffToken >= 0) {
        /* Temporarily zap statement type from $a to $p */
        if (statement[showStmt].type != (char)a_) bug(231);
        statement[showStmt].type = (char)p_;
        /* Temporarily zap statement with "wff" token in 1st position
           so parseProof will not give errors (in typeProof() call) */
        zapStatement1stToken = (statement[showStmt].mathString)[0];
        (statement[showStmt].mathString)[0] = wffToken;
        if (strcmp("|-", mathToken[zapStatement1stToken].tokenName)) bug(230);

        nmbrTmpPtr1 = NULL_NMBRSTRING;
        nmbrLet(&nmbrTmpPtr1, statement[showStmt].mathString);

        /* Find proof of formula or simple theorem (no new vars in $e's) */
        /* maxEDepth is the maximum depth at which statements with $e
           hypotheses are
           considered.  A value of 0 means none are considered. */
        nmbrTmpPtr2 = proveFloating(nmbrTmpPtr1 /*mString*/,
            showStmt /*statemNum*/, 0 /*maxEDepth*/,
            0, /*step:  0 = step 1 */ /*For messages*/
            0  /*not noDistinct*/);

        if (nmbrLen(nmbrTmpPtr2)) {
          /* A proof for the step was found. */
          /* Get packed form of proof for shorter display */
          nmbrLet(&nmbrTmpPtr2, nmbrSquishProof(nmbrTmpPtr2));
          /* Temporarily zap proof into statement structure */
          /* (The bug check makes sure there is no proof attached to the
              definition - this would be impossible) */
          if (strcmp(statement[showStmt].proofSectionPtr, "")) bug(231);
          if (statement[showStmt].proofSectionLen != 0) bug(232);
          let(&str1, nmbrCvtRToVString(nmbrTmpPtr2));
          /* Temporarily zap proof into the $a statement */
          statement[showStmt].proofSectionPtr = str1;
          statement[showStmt].proofSectionLen = (long)strlen(str1) - 1;

          /* Display the HTML proof of syntax breakdown */
          typeProof(showStmt,
              0 /*pipFlag*/,
              0 /*startStep*/,
              0 /*endStep*/,
              0 /*endIndent*/,
              0 /*essentialFlag*/, /* <- also used as def flag in typeProof */
              1 /*renumberFlag*/,
              0 /*unknownFlag*/,
              0 /*notUnifiedFlag*/,
              0 /*reverseFlag*/,
              1 /*noIndentFlag*/,
              0 /*splitColumn*/,
              0 /*skipRepeatedSteps*/,  /* 28-Jun-2013 nm */
              1 /*texFlag*/,  /* Means either latex or html */
              1 /*htmlFlg*/);

          /* Restore the zapped statement structure */
          statement[showStmt].proofSectionPtr = "";
          statement[showStmt].proofSectionLen = 0;

          /* Deallocate storage */
          let(&str1, "");
          nmbrLet(&nmbrTmpPtr2, NULL_NMBRSTRING);

        } else { /* if (nmbrLen(nmbrTmpPtr2)) else */
          /* 5-Aug-2011 nm */
          /* Proof was not found - probable syntax error */
          if (outputToString != 0) bug(246);
          printLongLine(cat(
              "?Warning:  Unable to generate syntax breakdown for \"",
              statement[showStmt].labelName,
              "\".", NULL), "    ", " ");
        }


        /* Restore the zapped statement structure */
        statement[showStmt].type = (char)a_;
        (statement[showStmt].mathString)[0] = zapStatement1stToken;

        /* Deallocate storage */
        nmbrLet(&nmbrTmpPtr1, NULL_NMBRSTRING);

      } /* if (wffToken >= 0) */

    } /* if (subType == DEFINITION) */


  } /* if (htmlFlg && texFlag) */
  /* End of finding definition for syntax statement */


  /* 10/6/99 - Start of creating used-by list for html page */
  if (htmlFlg && texFlag) {
    /* 10/25/02 Clear out any previous printString accumulation
       for printStringForReferencedBy case below */
    fprintf(texFilePtr, "%s", printString);
    let(&printString, "");
    /* Start outputting to printString */
    if (outputToString != 0) bug(242);
    outputToString = 1;
    if (subType != SYNTAX) { /* Only do this for
        definitions, axioms, and theorems, not syntax statements */
      let(&str1, "");
      str1 = traceUsage(showStmt,
          0, /* recursiveFlag */
          0 /* cutoffStmt */);
      /* if (str1[0]) { */ /* Used by at least one */
      /* 18-Jul-2015 nm */
      if (str1[0] == 'Y') { /* Used by at least one */
        /* str1[i] will be 'Y' if used by showStmt */
        /* Convert usage list str1 to html links */
        switch (subType) {
          case AXIOM:  let(&str3, "axiom"); break;
          case DEFINITION: let(&str3, "definition"); break;
          case THEOREM: let(&str3, "theorem"); break;
          default: bug(233);
        }
        /******* pre 10/10/02
        let(&str2, cat("<FONT SIZE=-1 FACE=sans-serif>This ", str3,
            " is referenced by: ", NULL));
        *******/
        /* 10/10/02 */
        let(&str2, cat("<TR><TD ALIGN=LEFT><FONT SIZE=-1><B>This ", str3,
            " is referenced by:</B>", NULL));


        /********* 18-Jul-2015 Deleted code *********************/
        /*
        /@ Convert str1 to trailing space after each label @/
        let(&str1, cat(right(str1, 2), " ", NULL));
        let(&str5, ""); /@ Buffer for very long strings @/ /@ 19-Sep-2012 nm @/
        i = 0;
        while (1) {
          j = i + 1;
          i = instr(j, str1, " ");
          if (!i) break;
          /@ Extract the label @/
          let(&str3, seg(str1, j, i - 1));
          /@ Find the statement number @/
          m = lookupLabel(str3);
          if (m < 0) {
            /@ The lookup should never fail @/
            bug(240);
            continue;
          }
          /@ It should be a $p @/
          if (statement[m].type != p_) bug(241);
          /@ Get the pink number @/
          let(&str4, "");
          str4 = pinkHTML(m);
          /@ Assemble the href @/
          let(&str2, cat(str2, " &nbsp;<A HREF=\"",
              str3, ".html\">",
              str3, "</A>", str4, NULL));
          /@ 8-Aug-2008 nm If line is very long, print it out and reset
             it to speed up program (SHOW STATEMENT syl/HTML is very slow) @/
          /@ 8-Aug-2008 nm This doesn't solve problem, because the bottleneck
             is printing printStringForReferencedBy below.  This whole
             code section needs to be redesigned to solve the speed problem. @/
          /@
          if (strlen(str2) > 500) {
            printLongLine(str2, "", "\"");
            let(&str2, "");
          }
          @/
          /@ 19-Sep-2012 nm Try again to fix SHOW STATEMENT syl/HTML speed
             without a major rewrite @/
          /@
          Unfortunately, makes little difference.  Using lcc:
          orig    real    1m6.676s
          500     real    1m2.285s
          3000    real    1m2.181s
          3000    real    1m1.663s
          3000    real    1m1.785s
          5000    real    1m0.678s
          5000    real    1m0.169s
          5000    real    1m1.951s
          5000    real    1m2.307s
          5000    real    1m1.717s
          7000    real    1m2.048s
          7000    real    1m2.012s
          7000    real    1m1.817s
          10000   real    1m2.779s
          10000   real    1m1.830s
          20000   real    1m1.431s
          50000   real    1m1.325s
          100000  real    1m3.172s
          100000  real    1m4.657s
          (Added 17-Jul-2015: The 1 minute is probably due to the old
              traceUsage algorithm that built a huge string of labels; should
              be faster now.)
          @/
          /@ Accumulate large cat buffer when small cats exceed certain size @/
          if (strlen(str2) > 5000) {
            let(&str5, cat(str5, str2, NULL));
            let(&str2, "");
          }
          /@ End 19-Sep-2012 @/
        }
        /@ let(&str2, cat(str2, "</FONT></TD></TR>", NULL)); @/ /@ old @/
         /@ 19-Sep-2012 nm Include buffer in output string@/
        let(&str2, cat(str5, str2, "</FONT></TD></TR>", NULL));
        printLongLine(str2, "", "\"");
        */
        /**************** 18-Jul-2015 End of deleted code *********/


        let(&str5, ""); /* Buffer for very long strings */ /* 19-Sep-2012 nm */
        /* Scan all future statements in str1 Y/N list */
        for (m = showStmt + 1; m <= statements; m++) {
          /* Scan the used-by map */
          if (str1[m] != 'Y') continue;
          /* Get the label */
          let(&str3, statement[m].labelName);
          /* It should be a $p */
          if (statement[m].type != p_) bug(241);
          /* Get the pink number */
          let(&str4, "");
          str4 = pinkHTML(m);
          /* Assemble the href */
          let(&str2, cat(str2, " &nbsp;<A HREF=\"",
              str3, ".html\">",
              /*str3, "</A>", str4, NULL));*/
              str3, "</A>\n", str4, NULL));  /* 18-Jul-2015 nm */
          /* 8-Aug-2008 nm If line is very long, print it out and reset
             it to speed up program (SHOW STATEMENT syl/HTML is very slow) */
          /* 8-Aug-2008 nm This doesn't solve problem, because the bottleneck
             is printing printStringForReferencedBy below.  This whole
             code section needs to be redesigned to solve the speed problem. */
          /* 19-Sep-2012 nm Try again to fix SHOW STATEMENT syl/HTML speed
             without a major rewrite.  Unfortunately, made little difference. */
          /* 18-Jul-2015: Part of slowdown was due to the old
              traceUsage algorithm that built a huge string of labels.  Improved
             from 313 sec to 280 sec for 'sh st syl/a'; still a problem. */
          /* Accumulate large cat buffer when small cats exceed certain size */
          if (strlen(str2) > 5000) {
            let(&str5, cat(str5, str2, NULL));
            let(&str2, "");
          }
          /* End 19-Sep-2012 */
        } /* next m (statement number) */
        /* let(&str2, cat(str2, "</FONT></TD></TR>", NULL)); */ /* old */
         /* 19-Sep-2012 nm Include buffer in output string*/
        let(&str2, cat(str5, str2, "</FONT></TD></TR>", NULL));
        /*printLongLine(str2, "", "\"");*/ /* 18-Jul-2015 nm Deleted */
        if (printString[0]) bug(256);  /* 18-Jul-2015 nm */
        let(&printString, str2); /* 18-Jul-2015 nm */
      } /* if (str1[0] == 'Y') */
    } /* if (subType != SYNTAX) */
    if (subType == THEOREM) {
      /* 10/25/02 The "referenced by" does not show up after the proof
         because we moved the typeProof() to below.  Therefore, we save
         printString into a temporary global holding variable to print
         at the proper place inside of typeProof().  Ugly but necessary
         with present design. */
      /* In the case of THEOREM, we save and reset the printString.  In the
         case of != THEOREM (i.e. AXIOM and DEFINITION), printString will
         be printed and cleared below. */
      let(&printStringForReferencedBy, printString);
      let(&printString, "");
    }

    /* Printing of the trailer in mmwtex.c will close out string later */
    outputToString = 0;
  } /* if (htmlFlg && texFlag) */
  /* 10/6/99 - End of used-by list for html page */


  /* 10/25/02  Moved this to after the block above, so referenced statements
     show up first for convenience */
  if (htmlFlg && texFlag) {
    /*** Output the html proof for $p statements ***/
    /* Note that we also output the axiom and definition usage
       lists inside this function */
    if (statement[showStmt].type == (char)p_) {
      typeProof(showStmt,
          0 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          1 /*renumberFlag*/,
          0 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          1 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*skipRepeatedSteps*/,  /* 28-Jun-2013 nm */
          1 /*texFlag*/,  /* Means either latex or html */
          1 /*htmlFlg*/);
    } /* if (statement[showStmt].type == (char)p_) */
  } /* if (htmlFlg && texFlag) */
  /* End of html proof for $p statements */

  /* typeProof should have cleared this out */
  if (printStringForReferencedBy[0]) bug(243);

 returnPoint:
  /* Deallocate strings */
  nmbrLet(&nmbrDDList, NULL_NMBRSTRING);
  let(&str1, "");
  let(&str2, "");
  let(&str3, "");
  let(&str4, "");
  let(&str5, "");
  let(&htmlDistinctVars, "");
} /* typeStatement */



/* 4-Jan-2014 nm */
/* Get the HTML string of "allowed substitutions" list for an axiom
   or theorem's web page.  It should be called only if we're in
   HTML output mode i.e.  */
/* This is HARD-CODED FOR SET.MM and will not produce meaningful
   output for other databases (so far none) with $d's */
/* Caller must deallocate returned string */
vstring htmlAllowedSubst(long showStmt)
{
  nmbrString *reqHyp; /* Pointer only; not allocated directly */
  long numReqHyps;
  nmbrString *reqDVA; /* Pointer only; not allocated directly */
  nmbrString *reqDVB; /* Pointer only; not allocated directly */
  long numDVs;
  nmbrString *setVar = NULL_NMBRSTRING; /* set (individual) variables */
  char *strptr;
  vstring str1 = "";
  long setVars;
  long wffOrClassVar;
  vstring setVarDVFlag = "";
  flag found, first;
  long i, j, k;
  vstring htmlAllowedList = "";
  long countInfo = 0;

  reqDVA = statement[showStmt].reqDisjVarsA;
  reqDVB = statement[showStmt].reqDisjVarsB;
  numDVs = nmbrLen(reqDVA);

  reqHyp = statement[showStmt].reqHypList;
  numReqHyps = nmbrLen(reqHyp);

  /* This function should be called only for web page generation */
  /*if (!(htmlFlag && texFlag)) bug(250);*/  /* texFlag is not global */
  if (!htmlFlag) bug(250);

  if (statement[showStmt].mathStringLen < 1) bug(254);
  if (strcmp("|-", mathToken[
            (statement[showStmt].mathString)[0]].tokenName)) {
    /* Don't process syntax statements */
    goto RETURN_POINT;
  }

  if (numDVs == 0) {  /* Don't create a hint list if no $d's */
    /*let(&htmlAllowedList, "(no restrictions)");*/
    goto RETURN_POINT;
  }

  /* Collect list of all set variables in the theorem */
  /* First, count the number of set variables */
  setVars = 0;
  for (i = 0; i < numReqHyps; i++) {
    /* Scan "set" variables */
    if (statement[reqHyp[i]].type == (char)e_) continue;
    if (statement[reqHyp[i]].type != (char)f_) bug(251);
    if (statement[reqHyp[i]].mathStringLen != 2)
      bug(252); /* $f must have 2 tokens */
    strptr = mathToken[
              (statement[reqHyp[i]].mathString)[0]].tokenName;
    if (strcmp("set", strptr)) continue;
                                  /* Not a set variable */
    setVars++;
  }
  /* Next, create a list of them in setVar[] */
  j = 0;
  nmbrLet(&setVar, nmbrSpace(setVars));
  for (i = 0; i < numReqHyps; i++) {
    /* Scan "set" variables */
    if (statement[reqHyp[i]].type == (char)e_) continue;
    strptr = mathToken[
              (statement[reqHyp[i]].mathString)[0]].tokenName;
    if (strcmp("set", strptr)) continue;
                                  /* Not a set variable */
    setVar[j] = (statement[reqHyp[i]].mathString)[1];
    j++;
  }
  if (j != setVars) bug(253);

  /* Scan "wff" and "class" variables for attached $d's */
  for (i = 0; i < numReqHyps; i++) {
    /* Look for a "wff" and "class" variable */
    if (statement[reqHyp[i]].type == (char)e_) continue;
    strptr = mathToken[
              (statement[reqHyp[i]].mathString)[0]].tokenName;
    if (strcmp("wff", strptr) && strcmp("class", strptr)) continue;
                                  /* Not a wff or class variable */
    wffOrClassVar = (statement[reqHyp[i]].mathString)[1];
    let(&setVarDVFlag, string(setVars, 'N')); /* No $d yet */
    /* Scan for attached $d's */
    for (j = 0; j < numDVs; j++) {
      found = 0;
      if (wffOrClassVar == reqDVA[j]) {
        for (k = 0; k < setVars; k++) {
          if (setVar[k] == reqDVB[j]) {
            setVarDVFlag[k] = 'Y';
            found = 1;
            break;
          }
        }
      }
      if (found) continue;
      /* Repeat with swapped $d arguments */
      if (wffOrClassVar == reqDVB[j]) {
        for (k = 0; k < setVars; k++) {
          if (setVar[k] == reqDVA[j]) {
            setVarDVFlag[k] = 'Y';
            break;
          }
        }
      }
    } /* next $d */

    /* Collect set vars that don't have $d's with this wff or class var */
    /* First, if there aren't any, then omit this wff or class var */
    found = 0;
    for (j = 0; j < setVars; j++) {
      if (setVarDVFlag[j] == 'N') {
        found = 1;
        break;
      }
    }
    if (found == 0) continue; /* All set vars have $d with this wff or class */

    let(&str1, "");
    str1 = tokenToTex(mathToken[wffOrClassVar].tokenName, showStmt);
         /* tokenToTex allocates str1; we must deallocate it eventually */
    countInfo++;
    let(&htmlAllowedList, cat(htmlAllowedList, " &nbsp; ",
        str1, "(", NULL));
    first = 1;
    for (j = 0; j < setVars; j++) {
      if (setVarDVFlag[j] == 'N') {
        let(&str1, "");
        str1 = tokenToTex(mathToken[setVar[j]].tokenName, showStmt);
        let(&htmlAllowedList, cat(htmlAllowedList,
            (first == 0) ? "," : "", str1, NULL));
        if (first == 0) countInfo++;
        first = 0;
      }
    }
    let(&htmlAllowedList, cat(htmlAllowedList, ")", NULL));

  } /* next i (wff or class var) */

 RETURN_POINT:

  if (htmlAllowedList[0] != 0) {
    let(&htmlAllowedList, cat("<CENTER>",
     /*
     "<A HREF=\"mmset.html#allowedsubst\">Allowed substitution",
     (countInfo != 1) ? "s" : "", "</A>: ",
     */
     "<A HREF=\"mmset.html#allowedsubst\">Allowed substitution</A> hint",
     (countInfo != 1) ? "s" : "", ": ",
        htmlAllowedList, "</CENTER>", NULL));
  }

  /* Deallocate strings */
  nmbrLet(&setVar, NULL_NMBRSTRING);
  let(&str1, "");
  let(&setVarDVFlag, "");

  return htmlAllowedList;
} /* htmlAllowedSubst */



/* Displays a proof (or part of a proof, depending on arguments). */
/* Note that parseProof() and verifyProof() are assumed to have been called,
   so that the wrkProof structure elements are assigned for the current
   statement. */
/* 8/28/00 - this is also used for the MIDI output, since we conveniently
   have the necessary proof information here.  The function outputMidi()
   is called from within. */
void typeProof(long statemNum,
  flag pipFlag, /* Means use proofInProgress; statemNum must be proveStatement*/
  long startStep, long endStep,
  long endIndent,
  flag essentialFlag, /* <- also used as definition/axiom flag for HTML
      syntax breakdown when called from typeStatement() */
  flag renumberFlag,
  flag unknownFlag,
  flag notUnifiedFlag,
  flag reverseFlag,
  flag noIndentFlag, /* Means Lemmon-style proof */
  long splitColumn, /* START_COLUMN */
  flag skipRepeatedSteps, /* NO_REPEATED_STEPS */  /* 28-Jun-2013 nm */
  flag texFlag,
  flag htmlFlg /* htmlFlg added 6/27/99 */
  /* flag midiFlag - global to avoid changing many calls to typeProof() */
  )
{
  /* From HELP SHOW PROOF: Optional qualifiers:
    / ESSENTIAL - the proof tree is trimmed of all $f hypotheses before
        being displayed.
    / FROM_STEP <step> - the display starts at the specified step.  If
        this qualifier is omitted, the display starts at the first step.
    / TO_STEP <step> - the display ends at the specified step.  If this
        qualifier is omitted, the display ends at the last step.
    / TREE_DEPTH <number> - Only steps at less than the specified proof
        tree depth are displayed.  Useful for obtaining an overview of
        the proof.
    / REVERSE - the steps are displayed in reverse order.
    / RENUMBER - when used with / ESSENTIAL, the steps are renumbered
        to correspond only to the essential steps.
    / TEX - the proof is converted to LaTeX and stored in the file opened
        with OPEN TEX.
    / HTML - the proof is converted to HTML and stored in the file opened
        with OPEN HTML.
    / LEMMON - The proof is displayed in a non-indented format known
        as Lemmon style, with explicit previous step number references.
        If this qualifier is omitted, steps are indented in a tree format.
    / START_COLUMN <number> - Overrides the default column at which
        the formula display starts in a Lemmon style display.  May be
        used only in conjuction with / LEMMON.
    / NO_REPEATED_STEPS - When a proof step is identical to an earlier
        step, it will not be repeated.  Instead, a reference to it will be
        changed to a reference to the earlier step.  In particular,
        SHOW PROOF <label> / LEMMON / RENUMBER / NO_REPEATED_STEPS
        will have the same proof step numbering as the web page proof
        generated by SHOW STATEMENT  <label> / HTML, rather than
        the proof step numbering of the indented format
        SHOW PROOF <label> / RENUMBER.  This qualifier affects only
        displays also using the / LEMMON qualifier.
    / NORMAL - The proof is displayed in normal format suitable for
        inclusion in a source file.  May not be used with any other
        qualifier.
    / COMPRESSED - The proof is displayed in compressed format
        suitable for inclusion in a source file.  May not be used with
        any other qualifier.
    / STATEMENT_SUMMARY - Summarizes all statements (like a brief SHOW
        STATEMENT) used by the proof.  May not be used with any other
        qualifier except / ESSENTIAL.
    / DETAILED_STEP <step> - Shows the details of what is happening at
        a specific proof step.  May not be used with any other qualifier.
    / MIDI - 8/28/00 - puts out a midi sound file instead of a proof
        - determined by the global variable midiFlag, not by a parameter to
        typeProof()
  */
  long i, j, plen, step, stmt, lens, lent, maxStepNum;
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
  vstring statementUsedFlags = "";
  vstring startStringWithNum = ""; /* 22-Apr-2015 nm */
  vstring startStringWithoutNum = ""; /* 22-Apr-2015 nm */
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *localLabels = NULL_NMBRSTRING;
  nmbrString *localLabelNames = NULL_NMBRSTRING;
  nmbrString *indentationLevel = NULL_NMBRSTRING;
  nmbrString *targetHyps = NULL_NMBRSTRING;
  nmbrString *essentialFlags = NULL_NMBRSTRING;
  nmbrString *stepRenumber = NULL_NMBRSTRING;
  nmbrString *notUnifiedFlags = NULL_NMBRSTRING;
  nmbrString *unprovedList = NULL_NMBRSTRING; /* For traceProofWork() */
  nmbrString *relativeStepNums = NULL_NMBRSTRING; /* For unknownFlag */
  long maxLabelLen = 0;
  long maxStepNumLen = 1;
  long maxStepNumOffsetLen = 0; /* 22-Apr-2015 nm */
  char type;
  flag stepPrintFlag;
  long fromStep, toStep, byStep;
  vstring hypStr = "";
  nmbrString *hypPtr;
  long hyp, hypStep;

  /* For statement syntax breakdown (see section below added 2/5/02 for better
     syntax hints), we declare the following 3 variables. */
  static long wffToken = -1; /* array index of the hard-coded token "wff" -
      static so we only have to look it up once - set to -2 if not found */
  nmbrString *nmbrTmpPtr1; /* Pointer only; not allocated directly */
  nmbrString *nmbrTmpPtr2; /* Pointer only; not allocated directly */

  /* 31-Jan-2010 nm */
  /* skipRepeatedSteps = 0; */ /* 28-Jun-2010 nm Now set by parameter */
  if (htmlFlg && texFlag) skipRepeatedSteps = 1; /* Keep old behavior */
  /* Comment out the following line if you want to revert to the old
     Lemmon-style behavior with local label references for reused compressed
     proof steps is desired.  The only reason for doing this is to obtain
     the same steps and step numbers as the indented proof, rather than
     those on the HTML pages. */
  /* if (noIndentFlag) skipRepeatedSteps = 1; */
           /* 28-Jun-2013 nm Now set by parameter */

  if (htmlFlg && texFlag) {
    outputToString = 1; /* Flag for print2 to add to printString */
    print2("<CENTER><TABLE BORDER CELLSPACING=0 BGCOLOR=%s\n",
        MINT_BACKGROUND_COLOR);
    if (essentialFlag) {
      /* For bobby.cast.org approval */
      print2("SUMMARY=\"Proof of theorem\">\n");
      print2("<CAPTION><B>Proof of Theorem <FONT\n");
    } else {
      /* This is a syntax breakdown "proof" of a definition called
         from typeStatement */

      if (!strcmp("ax-", left(statement[showStatement].labelName, 3))) {
        /* For bobby.cast.org approval */
        print2("SUMMARY=\"Detailed syntax breakdown of axiom\">\n");
        print2("<CAPTION><B>Detailed syntax breakdown of Axiom <FONT\n");
      } else {
        /* For bobby.cast.org approval */
        print2("SUMMARY=\"Detailed syntax breakdown of definition\">\n");
        print2("<CAPTION><B>Detailed syntax breakdown of Definition <FONT\n");
      }
    }
    printLongLine(cat("   COLOR=", GREEN_TITLE_COLOR, ">",
        asciiToTt(statement[statemNum].labelName),
        "</FONT></B></CAPTION>", NULL), "", "\"");
    print2(
        "<TR><TH>Step</TH><TH>Hyp</TH><TH>Ref\n");
    print2("</TH><TH>Expression</TH></TR>\n");
    outputToString = 0;
    /* printTexLongMath in typeProof will do this
    fprintf(texFilePtr, "%s", printString);
    let(&printString, "");
    */
  }

  if (!pipFlag) {
    parseProof(showStatement);
    if (wrkProof.errorSeverity > 1) {
      /* 2-Nov-2014 nm Prevent population of printString outside of web
         page generation to fix bug 1114 (reported by Sefan O'Rear). */
      if (htmlFlg && texFlag) {
        /* Print warning and close out proof table */
        outputToString = 1;
        print2(
      "<TD COLSPAN=4><B><FONT COLOR=RED>WARNING: Proof has a severe error.\n");
        print2("</FONT></B></TD></TR>\n");
        outputToString = 0;
        /* 18-Nov-2012 nm Fix bug 243 */
        /* Clear out printStringForReferencedBy to prevent bug 243 above */
        let(&printStringForReferencedBy, "");
      }
      return; /* verifyProof() could crash */
    }
    verifyProof(showStatement);
  }

  if (!pipFlag) {
    nmbrLet(&proof, wrkProof.proofString); /* The proof */
    if (midiFlag) { /* 8/28/00 */
      /* Get the uncompressed version of the proof */
      nmbrLet(&proof, nmbrUnsquishProof(proof));
    }
  } else {
    nmbrLet(&proof, proofInProgress.proof); /* The proof */
  }
  plen = nmbrLen(proof);

  /* 6/27/99 - to reduce the number of steps displayed in an html proof,
     we will use a local label to reference the 2nd or later reference to a
     hypothesis, so the hypothesis won't have to be shown multiple times
     in the proof.
     31-Jan-2010 - do this for all Lemmon-style proofs */
  if (htmlFlg && texFlag && !noIndentFlag /* Lemmon */) {
    /* Only Lemmon-style proofs are implemented for html */
    bug(218);
  }
  /*if (htmlFlg && texFlag) {*/   /* pre-31-Jan-2010 */
  /* 31-Jan-2010 nm Do this for all Lemmon-style proofs */
  if (skipRepeatedSteps) {
    for (step = 0; step < plen; step++) {
      stmt = proof[step];
      if (stmt < 0) continue;  /* Unknown or label ref */
      type = statement[stmt].type;
      if (type == f_ || type == e_  /* It's a hypothesis */
          || statement[stmt].numReqHyp == 0) { /* A statement w/ no hyp */
        for (i = 0; i < step; i++) {
          if (stmt == proof[i]) {
            /* The hypothesis at 'step' matches an earlier hypothesis at i,
               so we will backreference 'step' to i with a local label */
            proof[step] = -1000 - i;
            break;
          }
        } /* next i */
      }
    } /* next step */
  }


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
  if (essentialFlag || midiFlag) {
    nmbrLet(&essentialFlags, nmbrGetEssential(proof));
  } else {
    nmbrLet(&essentialFlags, NULL_NMBRSTRING);
  }

  /* 8/28/00 We now have enough information for the MIDI output, so
     do it */
  if (midiFlag) {
    outputMidi(plen, indentationLevel,
        essentialFlags, midiParam, statement[statemNum].labelName);
    goto typeProof_return;
  }

  /* Get the step renumbering */
  nmbrLet(&stepRenumber, nmbrSpace(plen)); /* This initializes all step
      renumbering to step 0.  Later, we will use (for html) the fact that
      a step renumbered to 0 is a step to be skipped (6/27/99). */
  i = 0;
  maxStepNum = 0;
  for (step = 0; step < plen; step++) {
    stepPrintFlag = 1; /* Note: stepPrintFlag is reused below with a
        slightly different meaning (i.e. it will be printed after
        a filter such as notUnified is applied) */
    if (renumberFlag && essentialFlag) {
      if (!essentialFlags[step]) stepPrintFlag = 0;
    }
    /* if (htmlFlg && texFlag && proof[step] < 0) stepPrintFlag = 0; */
    /* 31-Jan-2010 nm changed to: */
    if (skipRepeatedSteps && proof[step] < 0) stepPrintFlag = 0;
    /* For standard numbering, stepPrintFlag will be always be 1 here */
    if (stepPrintFlag) {
      i++;
      stepRenumber[step] = i; /* Numbering for step to be printed */
      maxStepNum = i; /* To compute maxStepNumLen below */
    }
  }

  /* 22-Apr-2015 nm */
  /* Get the relative offset (0, -1, -2,...) for unknown steps */
  if (unknownFlag) {
    if (!pipFlag) bug(255);
    relativeStepNums = getRelStepNums(proofInProgress.proof);
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

  /* Get the printed character length of the largest step number */
  i = maxStepNum;
  while (i >= 10) {
    i = i/10; /* The number is printed in base 10 */
    maxStepNumLen++;
  }
  /* 22-Apr-2015 nm */
  /* Add extra space for negative offset numbers e.g. "3:-1" */
  if (unknownFlag) {
    maxStepNumOffsetLen = 3; /* :, -, # */
    j = 0;
    for (i = 0; i < plen; i++) {
      j = relativeStepNums[i];
      if (j <= 0) break; /* Found first unknown step (largest offset) */
    }
    while (j <= -10) {
      j = j/10; /* The number is printed in base 10 */
      maxStepNumOffsetLen++;
    }
  }



  /* Get local labels and maximum label length */
  /* lent = target length, lens = source length */
  for (step = 0; step < plen; step++) {
    lent = (long)strlen(statement[targetHyps[step]].labelName);
    stmt = proof[step];
    if (stmt < 0) {
      if (stmt <= -1000) {
        stmt = -1000 - stmt;
        /* stmt is now the step number a local label refers to */
        lens = (long)strlen(str(localLabelNames[stmt]));
        let(&tmpStr1, ""); /* Clear temp alloc stack for str function */
      } else {
        if (stmt != -(long)'?') bug (219); /* the only other possibility */
        lens = 1; /* '?' (unknown step) */
      }
    } else {
      if (nmbrElementIn(1, localLabels, step)) {

        /* 6/27/99 The new philosophy is to number all local labels with the
           actual step number referenced, for better readability.  This means
           that if a *.mm label is a pure number, there may be ambiguity in
           the proof display, but this is felt to be too rare to be a serious
           drawback. */
        localLabelNames[step] = stepRenumber[step];

      }
      lens = (long)strlen(statement[stmt].labelName);
    }
    /* Find longest label assignment, excluding local label declaration */
    if (maxLabelLen < lent + 1 + lens) {
      maxLabelLen = lent + 1 + lens; /* Target, =, source */
    }
  } /* Next step */

  /* Print the steps */
  if (reverseFlag
      && !midiFlag /* 8/28/00 */
      ) {
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

    /* 6/27/99 Skip steps that are local label references for html */
    /* if (htmlFlg && texFlag) { */
    /* 31-Jan-2010 nm changed to: */
    if (skipRepeatedSteps) {
      if (stepRenumber[step] == 0) stepPrintFlag = 0;
    }

    /* 8/28/00 For MIDI files, ignore all qualifiers and process all steps */
    if (midiFlag) stepPrintFlag = 1;

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
        /* if (htmlFlg && texFlag) bug(220); */
        /* 31-Jan-2010 nm Changed to: */
        if (skipRepeatedSteps) bug(220); /* If html, a step referencing a
            local label will never be printed since it will be skipped above */
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
          /* if (!(htmlFlg && texFlag)) { */
          /* 31-Jan-2010 nm Changed to: */
          if (!(skipRepeatedSteps)) { /* No local label declaration is
              shown for html */
            let(&locLabDecl, cat("@", str(localLabelNames[step]), ":", NULL));
          }
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
          if (!essentialFlag || statement[hypPtr[hyp]].type == (char)e_) {
            i = stepRenumber[hypStep];
            if (i == 0) {
              /* if (!(htmlFlg && texFlag)) bug(221); */
              /* 31-Jan-2010 nm Changed to: */
              if (!(skipRepeatedSteps)) bug(221);
              if (proof[hypStep] != -(long)'?') {
                if (proof[hypStep] > -1000) bug(222);
                if (localLabelNames[-1000 - proof[hypStep]] == 0) bug(223);
                if (localLabelNames[-1000 - proof[hypStep]] !=
                    stepRenumber[-1000 - proof[hypStep]]) bug(224);
                /* Get the step number the hypothesis refers to */
                i = stepRenumber[-1000 - proof[hypStep]];
              } else {
                /* The hypothesis refers to an unknown step - use i as flag */
                i = -(long)'?';
              }
            }
            if (!hypStr[0]) {
              if (i != -(long)'?') {
                let(&hypStr, str(i));
              } else {
                let(&hypStr, "?");
              }
            } else {
              /* Put comma between more than one hypothesis reference */
              if (i != -(long)'?') {
                let(&hypStr, cat(str(i), ",", hypStr, NULL));
              } else {
                let(&hypStr, cat("?", ",", hypStr, NULL));
              }
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
            space(maxStepNumLen - (long)strlen(str(stepRenumber[step]))),
            str(stepRenumber[step]),
            " ",
            srcLabel,
            space(splitColumn - (long)strlen(srcLabel) - (long)strlen(locLabDecl) - 1
                - maxStepNumLen - 1),
            " ", locLabDecl,
            NULL));
        if (pipFlag) {
          let(&tgtPrefix, startPrefix);
          let(&srcPrefix, cat(
              space(maxStepNumLen - (long)strlen(str(stepRenumber[step]))),
              space((long)strlen(str(stepRenumber[step]))),
              " ",
              space(splitColumn - 1
                  - maxStepNumLen),
              NULL));
          let(&userPrefix, cat(
              space(maxStepNumLen - (long)strlen(str(stepRenumber[step]))),
              space((long)strlen(str(stepRenumber[step]))),
              " ",
              "(User)",
              space(splitColumn - (long)strlen("(User)") - 1
                  - maxStepNumLen),
              NULL));
        }
        let(&contPrefix, space((long)strlen(startPrefix) + 4));
      } else {  /* not noIndentFlag */

        /* 22-Apr-2015 nm */
        /* Compute prefix with and without step number.  For 'show new_proof
           /unknown', unknownFlag is set, and we add the negative offset. */
        let(&tmpStr, "");
        if (unknownFlag) {
          if (relativeStepNums[step] < 0) {
            let(&tmpStr, cat(" ", str(relativeStepNums[step]), NULL));
          }
          let(&tmpStr, cat(tmpStr, space(maxStepNumOffsetLen
              - (long)(strlen(tmpStr))), NULL));
        }

        let(&startStringWithNum, cat(
            space(maxStepNumLen - (long)strlen(str(stepRenumber[step]))),
            str(stepRenumber[step]),
            tmpStr,
            " ", NULL));
        let(&startStringWithoutNum, space(maxStepNumLen + 1));


        let(&startPrefix, cat(
            startStringWithNum,
            space(indentationLevel[step] * PF_INDENT_INC - (long)strlen(locLabDecl)),
            locLabDecl,
            tgtLabel,
            srcLabel,
            space(maxLabelLen - (long)strlen(tgtLabel) - (long)strlen(srcLabel)),
            NULL));
        if (pipFlag) {
          let(&tgtPrefix, cat(
              startStringWithNum,
              space(indentationLevel[step] * PF_INDENT_INC - (long)strlen(locLabDecl)),
              locLabDecl,
              tgtLabel,
              space((long)strlen(srcLabel)),
              space(maxLabelLen - (long)strlen(tgtLabel) - (long)strlen(srcLabel)),
              NULL));
          let(&srcPrefix, cat(
              startStringWithoutNum,
              space(indentationLevel[step] * PF_INDENT_INC - (long)strlen(locLabDecl)),
              space((long)strlen(locLabDecl)),
              space((long)strlen(tgtLabel)),
              srcLabel,
              space(maxLabelLen - (long)strlen(tgtLabel) - (long)strlen(srcLabel)),
              NULL));
          let(&userPrefix, cat(
              startStringWithoutNum,
              space(indentationLevel[step] * PF_INDENT_INC - (long)strlen(locLabDecl)),
              space((long)strlen(locLabDecl)),
              space((long)strlen(tgtLabel)),
              "=(User)",
              space(maxLabelLen - (long)strlen(tgtLabel) - (long)strlen("=(User)")),
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
          if (!midiFlag) { /* 8/28/00 */
            printLongLine(cat(startPrefix," $", chr(type), " ",
                nmbrCvtMToVString(wrkProof.mathStringPtrs[step]),
                NULL),
                contPrefix,
                chr(1));
                /* chr(1) is right-justify flag for printLongLine */
          }
        } else {  /* TeX or HTML */
          printTexLongMath(wrkProof.mathStringPtrs[step],
              cat(startPrefix, " $", chr(type), " ", NULL),
              contPrefix, stmt, indentationLevel[step]);
        }

      } else { /* pipFlag */
        if (texFlag) {
          /* nm 3-Feb-04  Added this bug check - it doesn't make sense to
             do this and it hasn't been tested anyway */
          print2("?Unsupported:  HTML or LaTeX proof for NEW_PROOF.\n");
          bug(244);
        }

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
          } else { /* TeX or HTML */
            printTexLongMath(proofInProgress.target[step],
                cat(tgtPrefix, " $", chr(type), " ", NULL),
                contPrefix, 0, 0);
            printTexLongMath(proofInProgress.source[step],
                cat(srcPrefix, "  = ", NULL),
                contPrefix, 0, 0);
          }
        } else {
          if (!texFlag) {
            printLongLine(cat(startPrefix, " $", chr(type), " ",
                nmbrCvtMToVString(proofInProgress.target[step]),
                NULL),
                contPrefix,
                chr(1)); /* chr(1) is right-justify flag for printLongLine */
          } else {  /* TeX or HTML */
            printTexLongMath(proofInProgress.target[step],
                cat(startPrefix, " $", chr(type), " ", NULL),
                contPrefix, 0, 0);
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
                contPrefix, 0, 0);
          }

        }
      }
    }


  } /* Next step */

  if (!pipFlag) {
    cleanWrkProof(); /* Deallocate verifyProof storage */
  }

  if (htmlFlg && texFlag) {
    outputToString = 1;
    print2("</TABLE></CENTER>\n");

    /* 10/10/02 Moved here from mmwtex.c */
    /*print2("<FONT SIZE=-1 FACE=sans-serif>Colors of variables:\n");*/
    printLongLine(cat(
        "<CENTER><TABLE CELLSPACING=5><TR><TD ALIGN=LEFT><FONT SIZE=-1>",
        "<B>Colors of variables:</B> ",
        htmlVarColors, "</FONT></TD></TR>",
        NULL), "", "\"");

    if (essentialFlag) {  /* Means this is not a syntax breakdown of a
        definition which is called from typeStatement() */

      /* Create list of syntax statements used */
      let(&statementUsedFlags, string(statements + 1, 'N')); /* Init. to 'no' */
      for (step = 0; step < plen; step++) {
        stmt = proof[step];
        /* Convention: collect all $a's that don't begin with "|-" */
        if (stmt > 0) {
          if (statement[stmt].type == a_) {
            if (strcmp("|-", mathToken[
                (statement[stmt].mathString)[0]].tokenName)) {
              statementUsedFlags[stmt] = 'Y'; /* Flag to use it */
            }
          }
        }
      }

      /******************************************************************/
      /* Start of section added 2/5/02 - for a more complete syntax hints
         list in the HTML pages, parse the wffs comprising the hypotheses
         and the statement, and add their syntax to the hints list. */

      /* Look up the token "wff" (hard-coded) if we haven't found it before */
      if (wffToken == -1) { /* First time */
        wffToken = -2; /* In case it's not found because the user's source
            used a convention different for "wff" for wffs */
        for (i = 0; i < mathTokens; i++) {
          if (!strcmp("wff", mathToken[i].tokenName)) {
            wffToken = i;
            break;
          }
        }
      }

      if (wffToken >= 0) {

        /* Scan the statement being proved and its essential hypotheses,
           and find a proof for each of them expressed as a wff */
        for (i = -1; i < statement[statemNum].numReqHyp; i++) {
          /* i = -1 is the statement itself; i >= 0 is hypotheses i */
          if (i == -1) {
            /* If it's not a $p we shouldn't be here */
            if (statement[statemNum].type != (char)p_) bug(245);
            nmbrTmpPtr1 = NULL_NMBRSTRING;
            nmbrLet(&nmbrTmpPtr1, statement[statemNum].mathString);
          } else {
            /* Ignore $f */
            if (statement[statement[statemNum].reqHypList[i]].type
                == (char)f_) continue;
            /* Must therefore be a $e */
            if (statement[statement[statemNum].reqHypList[i]].type
                != (char)e_) bug(234);
            nmbrTmpPtr1 = NULL_NMBRSTRING;
            nmbrLet(&nmbrTmpPtr1,
                statement[statement[statemNum].reqHypList[i]].mathString);
          }
          if (strcmp("|-", mathToken[nmbrTmpPtr1[0]].tokenName)) {
            /* 1-Oct-05 nm Since non-standard logics may not have this,
               just break out of this section gracefully */
            nmbrTmpPtr2 = NULL_NMBRSTRING; /* To be known after break */
            break;
            /* bug(235); */  /* 1-Oct-05 nm No longer a bug */
          }
          /* Turn "|-" assertion into a "wff" assertion */
          nmbrTmpPtr1[0] = wffToken;

          /* Find proof of formula or simple theorem (no new vars in $e's) */
          /* maxEDepth is the maximum depth at which statements with $e
             hypotheses are
             considered.  A value of 0 means none are considered. */
          nmbrTmpPtr2 = proveFloating(nmbrTmpPtr1 /*mString*/,
              statemNum /*statemNum*/, 0 /*maxEDepth*/,
              0, /* step; 0 = step 1 */ /*For messages*/
              0  /*not noDistinct*/);
          if (!nmbrLen(nmbrTmpPtr2)) {
            /* 1-Oct-05 nm Since a proof may not be found for non-standard
               logics, just break out of this section gracefully */
            break;
            /* bug(236); */ /* Didn't find syntax proof */
          }

          /* Add to list of syntax statements used */
          for (step = 0; step < nmbrLen(nmbrTmpPtr2); step++) {
            stmt = nmbrTmpPtr2[step];
            /* Convention: collect all $a's that don't begin with "|-" */
            if (stmt > 0) {
              if (statementUsedFlags[stmt] == 'N') { /* For slight speedup */
                if (statement[stmt].type == a_) {
                  if (strcmp("|-", mathToken[
                      (statement[stmt].mathString)[0]].tokenName)) {
                    statementUsedFlags[stmt] = 'Y'; /* Flag to use it */
                  } else {
                    /* In a syntax proof there should be no |- */
                    /* (In the future, we may want to break instead of
                       calling it a bug, if it's a problem for non-standard
                       logics.) */
                    bug(237);
                  }
                }
              }
            } else {
              /* proveFloating never returns a compressed proof */
              bug(238);
            }
          }

          /* Deallocate memory */
          nmbrLet(&nmbrTmpPtr2, NULL_NMBRSTRING);
          nmbrLet(&nmbrTmpPtr1, NULL_NMBRSTRING);
        } /* next i */
        /* 1-Oct-05 nm Deallocate memory in case we broke out above */
        nmbrLet(&nmbrTmpPtr2, NULL_NMBRSTRING);
        nmbrLet(&nmbrTmpPtr1, NULL_NMBRSTRING);
      } /* if (wffToken >= 0) */
      /* End of section added 2/5/02 */
      /******************************************************************/

      let(&tmpStr, "");
      for (stmt = 1; stmt <= statements; stmt++) {
        if (statementUsedFlags[stmt] == 'Y') {
          if (!tmpStr[0]) {
            let(&tmpStr,
               /* 10/10/02 */
               /*"<FONT SIZE=-1><FONT FACE=sans-serif>Syntax hints:</FONT> ");*/
               "<TR><TD ALIGN=LEFT><FONT SIZE=-1><B>Syntax hints:</B> ");
          }

          /* 10/6/99 - Get the main symbol in the syntax */
          /* This section can be deleted if not wanted - it is custom
             for set.mm and might not work with other .mm's */
          let(&tmpStr1, "");
          for (i = 1 /* Skip |- */; i < statement[stmt].mathStringLen; i++) {
            if (mathToken[(statement[stmt].mathString)[i]].tokenType ==
                (char)con_) {
              /* Skip parentheses, commas, etc. */
              if (strcmp(mathToken[(statement[stmt].mathString)[i]
                      ].tokenName, "(")
                  && strcmp(mathToken[(statement[stmt].mathString)[i]
                      ].tokenName, ",")
                  && strcmp(mathToken[(statement[stmt].mathString)[i]
                      ].tokenName, ")")
                  && strcmp(mathToken[(statement[stmt].mathString)[i]
                      ].tokenName, ":")
                  ) {
                tmpStr1 =
                    tokenToTex(mathToken[(statement[stmt].mathString)[i]
                    ].tokenName, stmt);
                break;
              }
            }
          } /* Next i */
          /* Special cases hard-coded for set.mm */
          if (!strcmp(statement[stmt].labelName, "wbr")) /* binary relation */
            let(&tmpStr1, "<i> class class class </i>");
          if (!strcmp(statement[stmt].labelName, "cv"))
            let(&tmpStr1, "[set variable]");
          /* 10/10/02 Let's don't do cv - confusing to reader */
          if (!strcmp(statement[stmt].labelName, "cv"))
            continue;
          if (!strcmp(statement[stmt].labelName, "co")) /* operation */
            let(&tmpStr1, "(<i>class class class</i>)");
          let(&tmpStr, cat(tmpStr, " &nbsp;", tmpStr1, NULL));
          /* End of 10/6/99 section - Get the main symbol in the syntax */

          let(&tmpStr1, "");
          tmpStr1 = pinkHTML(stmt);
          /******* 10/10/02
          let(&tmpStr, cat(tmpStr, "<FONT FACE=sans-serif><A HREF=\"",
              statement[stmt].labelName, ".html\">",
              statement[stmt].labelName, "</A></FONT> &nbsp; ", NULL));
          *******/
          let(&tmpStr, cat(tmpStr, "<A HREF=\"",
              statement[stmt].labelName, ".html\">",
              statement[stmt].labelName, "</A>", tmpStr1, NULL));


        }
      }
      if (tmpStr[0]) {
        let(&tmpStr, cat(tmpStr,
            "</FONT></TD></TR>", NULL));
        printLongLine(tmpStr, "", "\"");
      }
      /* End of syntax hints list */


      /* 10/25/02 Output "referenced by" list here */
      if (printStringForReferencedBy[0]) {
        /* printLongLine takes 130 sec for 'sh st syl/a' */
        /*printLongLine(printStringForReferencedBy, "", "\"");*/
        /* 18-Jul-2015 nm Speedup for 'sh st syl/a' */
        if (outputToString != 1) bug(257);
        let(&printString, cat(printString, printStringForReferencedBy, NULL));
        let(&printStringForReferencedBy, "");
      }


      /* Get list of axioms and definitions assumed by proof */
      let(&statementUsedFlags, "");
      traceProofWork(statemNum,
          1, /*essentialFlag*/
          "", /*traceToList*/ /* 18-Jul-2015 nm */
          &statementUsedFlags, /*&statementUsedFlags*/
          &unprovedList /* &unprovedList */);
      if ((signed)(strlen(statementUsedFlags)) != statements + 1) bug(227);

      /* First get axioms */
      let(&tmpStr, "");
      for (stmt = 1; stmt <= statements; stmt++) {
        if (statementUsedFlags[stmt] == 'Y' && statement[stmt].type == a_) {
          let(&tmpStr1, left(statement[stmt].labelName, 3));
          if (!strcmp(tmpStr1, "ax-")) {
            if (!tmpStr[0]) {
              let(&tmpStr,
 /******* 10/10/02
 "<FONT SIZE=-1 FACE=sans-serif>The theorem was proved from these axioms: ");
 *******/
 "<TR><TD ALIGN=LEFT><FONT SIZE=-1><B>This theorem was proved from axioms:</B>");
            }
            let(&tmpStr1, "");
            tmpStr1 = pinkHTML(stmt);
            let(&tmpStr, cat(tmpStr, " &nbsp;<A HREF=\"",
                statement[stmt].labelName, ".html\">",
                statement[stmt].labelName, "</A>", tmpStr1, NULL));
          }
        }
      } /* next stmt */
      if (tmpStr[0]) {
        let(&tmpStr, cat(tmpStr, "</FONT></TD></TR>", NULL));
        printLongLine(tmpStr, "", "\"");
      }

      /* 10/10/02 Then get definitions */
      let(&tmpStr, "");
      for (stmt = 1; stmt <= statements; stmt++) {
        if (statementUsedFlags[stmt] == 'Y' && statement[stmt].type == a_) {
          let(&tmpStr1, left(statement[stmt].labelName, 3));
          if (!strcmp(tmpStr1, "df-")) {
            if (!tmpStr[0]) {
              let(&tmpStr,
 "<TR><TD ALIGN=LEFT><FONT SIZE=-1><B>This theorem depends on definitions:</B>");
            }
            let(&tmpStr1, "");
            tmpStr1 = pinkHTML(stmt);
            let(&tmpStr, cat(tmpStr, " &nbsp;<A HREF=\"",
                statement[stmt].labelName, ".html\">",
                statement[stmt].labelName, "</A>", tmpStr1, NULL));
          }
        }
      } /* next stmt */
      if (tmpStr[0]) {
        let(&tmpStr, cat(tmpStr, "</FONT></TD></TR>", NULL));
        printLongLine(tmpStr, "", "\"");
      }

      /* Print any unproved statements */
      if (nmbrLen(unprovedList)) {
        if (nmbrLen(unprovedList) == 1 &&
            !strcmp(statement[unprovedList[0]].labelName,
            statement[statemNum].labelName)) {
          /* When the unproved list consists only of the statement that
             was traced, it means the statement traced has no
             proof (or it has a proof, but is incomplete and all earlier
             ones do have complete proofs). */
          printLongLine(cat(
"<TR><TD ALIGN=left >&nbsp;<B><FONT COLOR=\"#FF6600\">WARNING: This theorem has an",
              " incomplete proof.</FONT></B><BR></TD></TR>", NULL), "", "\"");

        } else {
          printLongLine(cat(
"<TR><TD ALIGN=left >&nbsp;</TD><TD><B><FONT COLOR=\"#FF6600\">WARNING: This proof depends",
              " on the following unproved theorem(s): ",
              NULL), "", "\"");
          let(&tmpStr, "");
          for (i = 0; i < nmbrLen(unprovedList); i++) {
            let(&tmpStr, cat(tmpStr, " <A HREF=\"",
                statement[unprovedList[i]].labelName, ".html\">",
                statement[unprovedList[i]].labelName, "</A>",
                NULL));
          }
          printLongLine(cat(tmpStr, "</B></FONT></TD></TR>", NULL), "", "\"");
        }
      }

      /* End of axiom list */

    }  /* if essentialFlag */


    /* Printing of the trailer in mmwtex.c will close out string later */
    outputToString = 0;
  }

 typeProof_return:
  let(&tmpStr, "");
  let(&tmpStr1, "");
  let(&statementUsedFlags, "");
  let(&locLabDecl, "");
  let(&tgtLabel, "");
  let(&srcLabel, "");
  let(&startPrefix, "");
  let(&tgtPrefix, "");
  let(&srcPrefix, "");
  let(&userPrefix, "");
  let(&contPrefix, "");
  let(&hypStr, "");
  let(&startStringWithNum, ""); /* 22-Apr-2015 nm */
  let(&startStringWithoutNum, ""); /* 22-Apr-2015 nm */
  nmbrLet(&unprovedList, NULL_NMBRSTRING);
  nmbrLet(&localLabels, NULL_NMBRSTRING);
  nmbrLet(&localLabelNames, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&targetHyps, NULL_NMBRSTRING);
  nmbrLet(&indentationLevel, NULL_NMBRSTRING);
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);
  nmbrLet(&stepRenumber, NULL_NMBRSTRING);
  nmbrLet(&notUnifiedFlags, NULL_NMBRSTRING);
  nmbrLet(&relativeStepNums, NULL_NMBRSTRING); /* 22-Apr-2015 nm */
} /* typeProof() */

/* Show details of one proof step */
/* Note:  detailStep is the actual step number (starting with 1), not
   the actual step - 1. */
void showDetailStep(long statemNum, long detailStep) {

  long i, j, plen, step, stmt, sourceStmt, targetStmt;
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
              allLabelKeyBase, (size_t)numAllLabelKeys,
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
    if (statement[sourceStmt].type == a_
        || statement[sourceStmt].type == p_) {
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
    if (statement[sourceStmt].type == a_
        || statement[sourceStmt].type == p_) {
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
              space(9 - (long)strlen(
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
            space(9 - (long)strlen(
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

} /* showDetailStep */

/* Summary of statements in proof */
void proofStmtSumm(long statemNum, flag essentialFlag, flag texFlag) {

  long i, j, k, pos, stmt, plen, slen, step;
  char type;
  vstring statementUsedFlags = ""; /* 'Y'/'N' flag that statement is used */
  vstring str1 = "";
  vstring str2 = "";
  vstring str3 = "";
  nmbrString *statementList = NULL_NMBRSTRING;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *essentialFlags = NULL_NMBRSTRING;

  /* 10/10/02 This section is never called in HTML mode anymore.  The code is
     left in though just in case we somehow get here and the user continues
     through the bug. */
  if (texFlag && htmlFlag) bug(239);

  if (!texFlag) {
    print2("Summary of statements used in the proof of \"%s\":\n",
        statement[statemNum].labelName);
  } else {
    outputToString = 1; /* Flag for print2 to add to printString */
    if (!htmlFlag) {
      print2("\n");
      print2("\\vspace{1ex} %%3\n");
      printLongLine(cat("Summary of statements used in the proof of ",
          "{\\tt ",
          asciiToTt(statement[statemNum].labelName),
          "}:", NULL), "", " ");
    } else {
      printLongLine(cat("Summary of statements used in the proof of ",
          "<B>",
          asciiToTt(statement[statemNum].labelName),
          "</B>:", NULL), "", "\"");
    }
    outputToString = 0;
    fprintf(texFilePtr, "%s", printString);
    let(&printString, "");
  }

  if (statement[statemNum].type != p_) {
    print2("  This is not a provable ($p) statement.\n");
    return;
  }

  /* 20-Oct-2013 nm Don't use bad proofs (incomplete proofs are ok) */
  if (parseProof(statemNum) > 1) {
    /* The proof has an error, so use the empty proof */
    nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING, -(long)'?'));
  } else {
    nmbrLet(&proof, wrkProof.proofString);
  }

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
      if (statement[stmt].type != a_ && statement[stmt].type != p_) {
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
  let(&statementUsedFlags, string(statements + 1, 'N')); /* Init. to 'no' */
  for (pos = 0; pos < slen; pos++) {
    stmt = statementList[pos];
    if (stmt > statemNum || stmt < 1) bug(210);
    statementUsedFlags[stmt] = 'Y';
  }
  /* Next, build the output string */
  for (stmt = 1; stmt < statemNum; stmt++) {
    if (statementUsedFlags[stmt] == 'Y') {

      let(&str1, cat(" is located on line ", str(statement[stmt].lineNum),
          " of the file ", NULL));
      if (!texFlag) {
        print2("\n");
        printLongLine(cat("Statement ", statement[stmt].labelName, str1,
          "\"", statement[stmt].fileName,
          "\".",NULL), "", " ");
      } else {
        outputToString = 1; /* Flag for print2 to add to printString */
        if (!htmlFlag) {
          print2("\n");
          print2("\n");
          print2("\\vspace{1ex} %%4\n");
          printLongLine(cat("Statement {\\tt ",
              asciiToTt(statement[stmt].labelName), "} ",
              str1, "{\\tt ",
              asciiToTt(statement[stmt].fileName),
              "}.", NULL), "", " ");
          print2("\n");
        } else {
          printLongLine(cat("Statement <B>",
              asciiToTt(statement[stmt].labelName), "</B> ",
              str1, " <B>",
              asciiToTt(statement[stmt].fileName),
              "</B> ", NULL), "", "\"");
        }
        outputToString = 0;
        fprintf(texFilePtr, "%s", printString);
        let(&printString, "");
      }

      /* type = statement[stmt].type; */ /* 18-Sep-2013 Not used */
      let(&str1, "");
      str1 = getDescription(stmt);
      if (str1[0]) {
        if (!texFlag) {
          printLongLine(cat("\"", str1, "\"", NULL), "", " ");
        } else {
          printTexComment(str1, 1);
        }
      }

      /* print2("Its mandatory hypotheses in RPN order are:\n"); */
      j = nmbrLen(statement[stmt].reqHypList);
      for (i = 0; i < j; i++) {
        k = statement[stmt].reqHypList[i];
        if (!essentialFlag || statement[k].type != f_) {
          let(&str2, cat("  ",statement[k].labelName,
              " $", chr(statement[k].type), " ", NULL));
          if (!texFlag) {
            printLongLine(cat(str2,
                nmbrCvtMToVString(statement[k].mathString), " $.", NULL),
                "      "," ");
          } else {
            let(&str3, space((long)strlen(str2)));
            printTexLongMath(statement[k].mathString,
                str2, str3, 0, 0);
          }
        }
      }

      let(&str1, "");
      type = statement[stmt].type;
      if (type == p_) let(&str1, " $= ...");
      let(&str2, cat("  ", statement[stmt].labelName,
          " $",chr(type), " ", NULL));
      if (!texFlag) {
        printLongLine(cat(str2,
            nmbrCvtMToVString(statement[stmt].mathString),
            str1, " $.", NULL), "      ", " ");
      } else {
        let(&str3, space((long)strlen(str2)));
        printTexLongMath(statement[stmt].mathString,
            str2, str3, 0, 0);
      }

    } /* End if (statementUsedFlag[stmt] == 'Y') */
  } /* Next stmt */

  let(&statementUsedFlags, ""); /* 'Y'/'N' flag that statement is used */
  let(&str1, "");
  let(&str2, "");
  let(&str3, "");
  nmbrLet(&statementList, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);

} /* proofStmtSumm */


/* Traces back the statements used by a proof, recursively. */
/* Returns 1 if at least one label is printed (or would be printed in
   case testOnlyFlag=1); otherwise, returns 0 */
/* matchList suppresses all output except labels matching matchList */
/* testOnlyFlag prevents any printout; it is used to determine whether
   there is an unwanted axiom for MINIMIZE_WITH /FORBID. */
/* void traceProof(long statemNum, */ /* before 20-May-2013 */
flag traceProof(long statemNum, /* 20-May-2013 nm */
  flag essentialFlag,
  flag axiomFlag,
  vstring matchList, /* 19-May-2013 nm */
  vstring traceToList, /* 18-Jul-2015 nm */
  flag testOnlyFlag /* 20-May-2013 nm */)
{

  long stmt, pos;
  vstring statementUsedFlags = ""; /* y/n flags that statement is used */
  vstring outputString = "";
  nmbrString *unprovedList = NULL_NMBRSTRING;
  flag foundFlag = 0;

  /* Make sure we're calling this with $p statements only */
  if (statement[statemNum].type != (char)p_) bug(249);

  if (!testOnlyFlag) {  /* 20-May-2013 nm */
    if (axiomFlag) {
      print2(
  "Statement \"%s\" assumes the following axioms ($a statements):\n",
          statement[statemNum].labelName);
    } else  if (traceToList[0] == 0) {
      print2(
  "The proof of statement \"%s\" uses the following earlier statements:\n",
          statement[statemNum].labelName);
    } else {
      print2(
  "The proof of statement \"%s\" traces back to \"%s\" via:\n",
          statement[statemNum].labelName, traceToList);
    }
  }

  traceProofWork(statemNum,
      essentialFlag,
      traceToList, /* 18-Jul-2015 nm */
      &statementUsedFlags,
      &unprovedList);
  if ((signed)(strlen(statementUsedFlags)) != statements + 1) bug(226);

  /* Build the output string */
  let(&outputString, "");
  for (stmt = 1; stmt < statemNum; stmt++) {
    if (statementUsedFlags[stmt] == 'Y') {

      /* 19-May-2013 nm - Added MATCH qualifier */
      if (matchList[0]) {  /* There is a list to match */
        /* Don't include unmatched labels */
        if (!matchesList(statement[stmt].labelName, matchList, '*', '?'))
          continue;
      }

      /* 20-May-2013 nm  Skip rest of scan in testOnlyFlag mode */
      foundFlag = 1; /* At least one label would be printed */
      if (testOnlyFlag) {
        goto TRACE_RETURN;
      }
      if (axiomFlag) {
        if (statement[stmt].type == a_) {
          let(&outputString, cat(outputString, " ", statement[stmt].labelName,
              NULL));
        }
      } else {
        let(&outputString, cat(outputString, " ", statement[stmt].labelName,
            NULL));
        switch (statement[stmt].type) {
          case a_: let(&outputString, cat(outputString, "($a)", NULL)); break;
          case e_: let(&outputString, cat(outputString, "($e)", NULL)); break;
          case f_: let(&outputString, cat(outputString, "($f)", NULL)); break;
        }
      }
    } /* End if (statementUsedFlag[stmt] == 'Y') */
  } /* Next stmt */

  /* 20-May-2013 nm  Skip printing in testOnlyFlag mode */
  if (testOnlyFlag) {
    goto TRACE_RETURN;
  }

  if (outputString[0]) {
    let(&outputString, cat(" ", outputString, NULL));
  } else {
    let(&outputString, "  (None)");
  }

  /* Print the output */
  printLongLine(outputString, "  ", " ");

  /* Print any unproved statements */
  if (nmbrLen(unprovedList)) {
    print2("Warning:  the following traced statement(s) were not proved:\n");
    let(&outputString, "");
    for (pos = 0; pos < nmbrLen(unprovedList); pos++) {
      let(&outputString, cat(outputString, " ", statement[unprovedList[
          pos]].labelName, NULL));
    }
    let(&outputString, cat("  ", outputString, NULL));
    printLongLine(outputString, "  ", " ");
  }

 TRACE_RETURN:
  /* Deallocate */
  let(&outputString, "");
  let(&statementUsedFlags, "");
  nmbrLet(&unprovedList, NULL_NMBRSTRING);
  return foundFlag;
} /* traceProof */

/* Traces back the statements used by a proof, recursively.  Returns
   a nmbrString with a list of statements and unproved statements */
void traceProofWork(long statemNum,
  flag essentialFlag,
  vstring traceToList, /* 18-Jul-2015 nm */
  vstring *statementUsedFlagsP, /* 'Y'/'N' flag that statement is used */
  nmbrString **unprovedListP)
{

  long pos, stmt, plen, slen, step;
  nmbrString *statementList = NULL_NMBRSTRING;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *essentialFlags = NULL_NMBRSTRING;
  vstring traceToFilter = ""; /* 18-Jul-2015 nm */
  vstring str1 = ""; /* 18-Jul-2015 nm */
  long j; /* 18-Jul-2015 nm */

  /* 18-Jul-2015 nm */
  /* Preprocess the "SHOW TRACE_BACK ... / TO" traceToList list if any */
  if (traceToList[0] != 0) {
    let(&traceToFilter, string(statements + 1, 'N')); /* Init. to 'no' */
    /* Wildcard match scan */
    for (stmt = 1; stmt <= statements; stmt++) {
      if (statement[stmt].type != (char)a_
          && statement[stmt].type != (char)p_)
        continue; /* Not a $a or $p statement; skip it */
      /* Wildcard matching */
      if (!matchesList(statement[stmt].labelName, traceToList, '*', '?'))
        continue;
      let(&str1, "");
      str1 = traceUsage(stmt /*showStatement*/,
          1, /*recursiveFlag*/
          statemNum /* cutoffStmt */);
      traceToFilter[stmt] = 'Y'; /* Include the statement we're showing
                                    usage of */
      if (str1[0] == 'Y') {  /* There is some usage */
        for (j = stmt + 1; j <= statements; j++) {
          /* OR in the usage to the filter */
          if (str1[j] == 'Y') traceToFilter[j] = 'Y';
        }
      }
    } /* Next i (statement number) */
  } /* if (traceToList[0] != 0) */

  nmbrLet(&statementList, nmbrSpace(statements));
  statementList[0] = statemNum;
  slen = 1;
  nmbrLet(&(*unprovedListP), NULL_NMBRSTRING); /* List of unproved statements */
  let(&(*statementUsedFlagsP), string(statements + 1, 'N')); /* Init. to 'no' */
  (*statementUsedFlagsP)[statemNum] = 'Y';  /* nm 22-Nov-2014 */
  for (pos = 0; pos < slen; pos++) {
    if (statement[statementList[pos]].type != p_) {
      continue; /* Not a $p */
    }

    /* 20-Oct-2013 nm Don't use bad proofs (incomplete proofs are ok) */
    if (parseProof(statementList[pos]) > 1) {
      /* The proof has an error, so use the empty proof */
      nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING, -(long)'?'));
    } else {
      nmbrLet(&proof, wrkProof.proofString);
    }

    plen = nmbrLen(proof);
    /* Get the essential step flags, if required */
    if (essentialFlag) {
      nmbrLet(&essentialFlags, nmbrGetEssential(proof));
    }
    for (step = 0; step < plen; step++) {
      if (essentialFlag) {
        if (!essentialFlags[step]) continue;  /* Ignore floating hypotheses */
      }
      stmt = proof[step];
      if (stmt < 0) {
        if (stmt > -1000) {
          /* '?' */
          if (!nmbrElementIn(1, *unprovedListP, statementList[pos])) {
            nmbrLet(&(*unprovedListP), nmbrAddElement(*unprovedListP,
                statementList[pos]));  /* Add to list of unproved statements */
          }
        }
        continue; /* Ignore '?' and local labels */
      }
      if (1) { /* Limit list to $a and $p only */
        if (statement[stmt].type != a_ && statement[stmt].type != p_) {
          continue;
        }
      }
      /* Add this statement to the statement list if not already in it */
      if ((*statementUsedFlagsP)[stmt] == 'N') {
        /*(*statementUsedFlagsP)[stmt] = 'Y';*/  /* 18-Jul-2015 deleted */

        /* 18-Jul-2015 nm */
        if (traceToList[0] == 0) {
          statementList[slen] = stmt;
          slen++;
          (*statementUsedFlagsP)[stmt] = 'Y';
        } else {
          if (traceToFilter[stmt] == 'Y') {
            statementList[slen] = stmt;
            slen++;
            (*statementUsedFlagsP)[stmt] = 'Y';
          }
        }
      }
    } /* Next step */
  } /* Next pos */

  /* Deallocate */
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&statementList, NULL_NMBRSTRING);
  let(&str1, "");
  let(&str1, "");
  return;

} /* traceProofWork */

nmbrString *stmtFoundList = NULL_NMBRSTRING;
long indentShift = 0;

/* Traces back the statements used by a proof, recursively, with tree display.*/
void traceProofTree(long statemNum,
  flag essentialFlag, long endIndent)
{
  if (statement[statemNum].type != p_) {
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
} /* traceProofTree */


void traceProofTreeRec(long statemNum,
  flag essentialFlag, long endIndent, long recursDepth)
{
  long i, pos, stmt, plen, slen, step;
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
#define MAX_LINE_LEN 79

  if ((recursDepth * INDENT_INCR - indentShift) >
      (screenWidth - MAX_LINE_LEN) + 50) {
    indentShift = indentShift + 40 + (screenWidth - MAX_LINE_LEN);
    print2("****** Shifting indentation.  Total shift is now %ld.\n",
      (long)indentShift);
  }
  if ((recursDepth * INDENT_INCR - indentShift) < 1 && indentShift != 0) {
    indentShift = indentShift - 40 - (screenWidth - MAX_LINE_LEN);
    print2("****** Shifting indentation.  Total shift is now %ld.\n",
      (long)indentShift);
  }

  let(&outputStr, cat(space(recursDepth * INDENT_INCR - indentShift),
      statement[statemNum].labelName, " $", chr(statement[statemNum].type),
      "  \"", edit(outputStr, 8 + 128), "\"", NULL));

  if (len(outputStr) > MAX_LINE_LEN + (screenWidth - MAX_LINE_LEN)) {
    let(&outputStr, cat(left(outputStr,
        MAX_LINE_LEN + (screenWidth - MAX_LINE_LEN) - 3), "...", NULL));
  }

  if (statement[statemNum].type == p_ || statement[statemNum].type == a_) {
    /* Only print assertions to reduce output bulk */
    print2("%s\n", outputStr);
  }

  if (statement[statemNum].type != p_) {
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

  /* 20-Oct-2013 nm Don't use bad proofs (incomplete proofs are ok) */
  if (parseProof(statemNum) > 1) {
    /* The proof has an error, so use the empty proof */
    nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING, -(long)'?'));
  } else {
    nmbrLet(&proof, wrkProof.proofString);
  }

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
      if (statement[stmt].type == p_ || statement[stmt].type == a_) {
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

} /* traceProofTreeRec */


/* Called by SHOW TRACE_BACK <label> / COUNT_STEPS */
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

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  essentialplen = 0;

  /* If this is the top level of recursion, initialize things */
  if (!level) {
    stmtCount = malloc((sizeof(double) * ((size_t)statements + 1)));
    stmtNodeCount = malloc(sizeof(double) * ((size_t)statements + 1));
    stmtDist = malloc(sizeof(long) * ((size_t)statements + 1));
    stmtMaxPath = malloc(sizeof(long) * ((size_t)statements + 1));
    stmtAveDist = malloc(sizeof(double) * ((size_t)statements + 1));
    stmtProofLen = malloc(sizeof(long) * ((size_t)statements + 1));
    stmtUsage = malloc(sizeof(long) * ((size_t)statements + 1));
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
      stmtDist[stmt] = 0; /* 18-Sep-2013 Initialize */
    }
    unprovedFlag = 0; /* Flag that some proof wasn't complete */
  }
  level++;
  stepCount = 0;
  stepNodeCount = 0;
  stepDistSum = 0;
  stmtDist[statemNum] = -2; /* Forces at least one assignment */

  if (statement[statemNum].type != (char)p_) {
    /* $a, $e, or $f */
    stepCount = 1;
    stepNodeCount = 0;
    stmtDist[statemNum] = 0;
    goto returnPoint;
  }

  /* 20-Oct-2013 nm Don't use bad proofs (incomplete proofs are ok) */
  if (parseProof(statemNum) > 1) {
    /* The proof has an error, so use the empty proof */
    nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING, -(long)'?'));
  } else {
    nmbrLet(&proof, nmbrUnsquishProof(wrkProof.proofString)); /* The proof */
  }

  plen = nmbrLen(proof);
  /* Get the essential step flags, if required */
  if (essentialFlag) {
    nmbrLet(&essentialFlags, nmbrGetEssential(proof));
  }
  essentialplen = 0;
  for (step = 0; step < plen; step++) {
  /* 12-May-04 nm Use the following loop instead to get an alternate maximum
     path */
  /*for (step = plen-1; step >= 0; step--) {*/
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
      if (statement[stmt].type == (char)p_) {
        /*stepCount--;*/ /* -1 to account for the replacement of this step */
        for (j = 0; j < statement[stmt].numReqHyp; j++) {
          k = statement[stmt].reqHypList[j];
          if (!essentialFlag || statement[k].type == (char)e_) {
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
      if (statement[i].type == (char)p_ && stmtCount[i] != 0) {
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
            if (!essentialFlag || statement[k].type == (char)e_) {
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
       /* 27-May-05 nm stepCount is inaccurate for over 16 or so digits due
          to roundoff errors. */
       "The proof would have ",
         stepCount > 1000000000 ? ">1000000000" : str(stepCount),
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

} /* countSteps */


/* Traces what statements require the use of a given statement */
/* The output string must be deallocated by the user. */
/* 18-Jul-2015 nm changed the meaning of the returned string as follows: */
/* The return string [0] will be 'Y' or 'N' depending on whether there are any
   statements that use statemNum.  Return string [i] will be 'Y' or 'N'
   depending on whether statement[i] uses statemNum.  All i will be populated
   with 'Y'/'N' even if not $a or $p (always 'N' for non-$a,$p). */
/* 18-Jul-2015 added optional 'cutoffStmt' parameter:  if nonzero, then
   statements above cutoffStmt will not be scanned (for speedup) */
vstring traceUsage(long statemNum,
  flag recursiveFlag,
  long cutoffStmt /* for speedup */) {

  long lastPos, stmt, slen, pos;
  flag tmpFlag;
  vstring statementUsedFlags = ""; /* 'Y'/'N' flag that statement is used */
  /* vstring outputString = ""; */ /* 18-Jun-2015 nm Deleted */
  nmbrString *statementList = NULL_NMBRSTRING;
  nmbrString *proof = NULL_NMBRSTRING;

  /* For speed-up code */
  char *fbPtr;
  char *fbPtr2;
  char zapSave;
  flag notEFRec; /* Not ($e or $f or recursive) */

  if (statement[statemNum].type == e_ || statement[statemNum].type == f_
      || recursiveFlag) {
    notEFRec = 0;
  } else {
    notEFRec = 1;
  }

  nmbrLet(&statementList, nmbrAddElement(statementList, statemNum));
  lastPos = 1;

  /* 18-Jul-2015 nm */
  /* For speedup (in traceProofWork), scan only up to cutoffStmt if it
     is specified, otherwise scan all statements. */
  if (cutoffStmt == 0) cutoffStmt = statements;

  /*for (stmt = statemNum + 1; stmt <= statements; stmt++) {*/ /* Scan all stmts*/
  for (stmt = statemNum + 1; stmt <= cutoffStmt; stmt++) { /* Scan stmts*/
    if (statement[stmt].type != p_) continue; /* Ignore if not $p */

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

    /* 20-Oct-2013 nm Don't use bad proofs (incomplete proofs are ok) */
    if (parseProof(stmt) > 1) {
      /* The proof has an error, so use the empty proof */
      nmbrLet(&proof, nmbrAddElement(NULL_NMBRSTRING, -(long)'?'));
    } else {
      nmbrLet(&proof, wrkProof.proofString);
    }

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

  /* Prepare the output */
  /* First, fill in the statementUsedFlags char array.  This allows us to sort
     the output by statement number without calling a sort routine. */
  let(&statementUsedFlags, string(statements + 1, 'N')); /* Init. to 'no' */
  if (slen > 1) statementUsedFlags[0] = 'Y';  /* Used by at least one */
                                              /* 18-Jul-2015 nm */
  for (pos = 1; pos < slen; pos++) { /* Start with 1 (ignore traced statement)*/
    stmt = statementList[pos];
    if (stmt <= statemNum || statement[stmt].type != p_ || stmt > statements)
        bug(212);
    statementUsedFlags[stmt] = 'Y';
  }
  return (statementUsedFlags); /* 18-Jul-2015 nm */

  /********** 18-Jul-2015 nm Deleted code - this is now done by caller ******/
  /*
  /@ Next, build the output string @/
  let(&outputString, "");
  for (stmt = 1; stmt <= statements; stmt++) {
    if (statementUsedFlags[stmt] == 'Y') {
      let(&outputString, cat(outputString, " ", statement[stmt].labelName,
          NULL));
      /@ For temporary unofficial use by NDM to help build command files: @/
      /@
      print2("prove %s$min %s/a$sa n/compr$q\n",
          statement[stmt].labelName,statement[statemNum].labelName);
      @/
    } /@ End if (statementUsedFlag[stmt] == 'Y') @/
  } /@ Next stmt @/

  /@ Deallocate @/
  let(&statementUsedFlags, "");
  nmbrLet(&statementList, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);

  return (outputString);
  */
  /*************** 18-Jul-2015 End of deleted code **************/

} /* traceUsage */



/* This implements the READ command (although the / VERIFY qualifier is
   processed separately in metamath.c). */
void readInput(void)
{

/* 28-Dec-05 nm Obsolete---
  /@ Temporary variables and strings @/
  long p;

  p = instr(1,input_fn,".") - 1;
  if (p == -1) p = len(input_fn);
  let(&mainFileName,left(input_fn,p));
--- 28-Dec-05 nm End obsolete */

  print2("Reading source file \"%s\"...\n",input_fn);

  includeCalls = 0; /* Initialize for readRawSource */
  sourcePtr = readRawSource(input_fn, 0, &sourceLen);
  parseKeywords();
  parseLabels();
  parseMathDecl();
  parseStatements();

} /* readInput */

/* This function implements the WRITE SOURCE command. */
void writeInput(flag cleanFlag, /* 1 = "/ CLEAN" qualifier was chosen */
                flag reformatFlag /* 1 = "/ FORMAT", 2 = "/REWRAP" */ )
{

  /* Temporary variables and strings */
  long i, p;
  vstring str1 = "";
  vstring str2 = "";
  long skippedCount = 0;

  print2("Creating and writing \"%s\"...\n",output_fn);

  /* Process statements */
  for (i = 1; i <= statements + 1; i++) {

    /* Added 24-Oct-03 nm */
    if (cleanFlag && statement[i].type == (char)p_) {
      /* Clean out any proof-in-progress (that user has flagged with a ? in its
         date comment field) */
      /* Get the comment section after the statement */
      let(&str1, space(statement[i + 1].labelSectionLen));
      memcpy(str1, statement[i + 1].labelSectionPtr,
          (size_t)(statement[i + 1].labelSectionLen));
      /* Make sure it's a date comment */
      let(&str1, edit(str1, 2 + 4)); /* Discard whitespace + control chrs */
      p = instr(1, str1, "]$)"); /* Get end of date comment */
      let(&str1, left(str1, p)); /* Discard stuff after date comment */
      if (instr(1, str1, "$([") == 0) {
        printLongLine(cat(
            "?Warning: The proof for $p statement \"", statement[i].labelName,
            "\" does not have a date comment after it and will not be",
            " removed by the CLEAN qualifier.", NULL), " ", " ");
      } else {
        /* See if the date comment has a "?" in it */
        if (instr(1, str1, "?")) {
          skippedCount++;
          let(&str2, cat(str2, " ", statement[i].labelName, NULL));
          /* Write at least the date from the _previous_ statement's
             post-comment section so that WRITE RECENT can pick it up. */
          let(&str1, space(statement[i].labelSectionLen));
          memcpy(str1, statement[i].labelSectionPtr,
              (size_t)(statement[i].labelSectionLen));
          /* nm 19-Jan-04 Don't discard w.s. because string will be returned to
             the source file */
          /*let(&str1, edit(str1, 2 + 4));*/ /* Discard whitespace + ctrl chrs */
          p = instr(1, str1, "]$)"); /* Get end of date comment */
          if (p == 0) {
            /* In the future, "] $)" may flag date end to conform with
               space-around-keyword Metamath spec recommendation */
            /* 7-Sep-04 The future is now */
            p = instr(1, str1, "] $)"); /* Get end of date comment */
            if (p != 0) p++; /* Match what it would be for old standard */
          }
          if (p != 0) {  /* The previous statement has a date comment */
            let(&str1, left(str1, p + 2)); /* Discard stuff after date cmnt */
            if (!instr(1, str1, "?"))
              /* Don't bother to print $([?]$) of previous statement because
                 the previous statement was skipped */
              fprintf(output_fp, "%s\n", str1);
                                             /* Output just the date comment */
          }

          /* Skip the normal output of this statement */
          continue;
        } /* if (instr(1, str1, "?")) */
      } /* (else clause) if (instr(1, str1, "$([") == 0) */
    } /* if cleanFlag */

    let(&str1,""); /* Deallocate vstring */
    str1 = outputStatement(i, cleanFlag, reformatFlag);
    fprintf(output_fp, "%s", str1);
  }
  print2("%ld source statement(s) were written.\n",statements);


  /* Added 24-Oct-03 nm */
  if (cleanFlag) {
    if (skippedCount == 0) {
      print2("No statements were deleted by the CLEAN qualifier.\n");
    } else {
      printLongLine(cat("The following ", str(skippedCount), " statement",
          (skippedCount == 1) ? " was" : "s were",
          " deleted by the CLEAN qualifier:", NULL), "", " ");
      printLongLine(cat(" ", str2, NULL), "  ", " ");
      printLongLine(cat(
          "You should ERASE, READ the new output file, and run VERIFY PROOF",
          " to ensure that no proof dependencies were broken.", NULL),
          "", " ");
    }
  }

  let(&str1,""); /* Deallocate vstring */
  let(&str2,""); /* Deallocate vstring */
} /* writeInput */

void writeDict(void)
{
  print2("This function has not been implemented yet.\n");
  return;
} /* writeDict */

/* Free up all memory space and initialize all variables */
void eraseSource(void)
{
  long i;
  vstring tmpStr;

  /* 24-Jun-2014 nm */
  /* Deallocate wrkProof structure if wrkProofMaxSize != 0 */
  /* Assigned in parseProof() in mmpars.c */
  if (wrkProofMaxSize) { /* It has been allocated */
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
    wrkProofMaxSize = 0;
  }

  if (statements == 0) {
    /* Already called */
    memFreePoolPurge(0);
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

    /* 28-Aug-2013 am - statement[stmt].reqVarList allocated in
       parseStatements() was not always freed by eraseSource().  If reqVars==0
       (in parseStatements()) then statement[i].reqVarList[0]==-1 (in
       eraseSource()) and so eraseSource() thought that
       statement[stmt].reqVarList wass not allocated and should not be freed.

       There were similar problems for reqHypList, reqDisjVarsA, reqDisjVarsB,
       reqDisjVarsStmt, optDisjVarsA, optDisjVarsB, optDisjVarsStmt.

       These were fixed by comparing to NULL_NMBRSTRING instead of -1.

       I think other files (mathString, proofString, optHypList, optVarList)
       should be also fixed, but I could not find simple example for memory
       leak and leave it as it was.  */
    if (statement[i].labelName[0]) free(statement[i].labelName);
    if (statement[i].mathString[0] != -1)
        poolFree(statement[i].mathString);
    if (statement[i].proofString[0] != -1)
        poolFree(statement[i].proofString);
    if (statement[i].reqHypList != NULL_NMBRSTRING)
        poolFree(statement[i].reqHypList);
    if (statement[i].optHypList[0] != -1)
        poolFree(statement[i].optHypList);
    if (statement[i].reqVarList != NULL_NMBRSTRING)
        poolFree(statement[i].reqVarList);
    if (statement[i].optVarList[0] != -1)
        poolFree(statement[i].optVarList);
    if (statement[i].reqDisjVarsA != NULL_NMBRSTRING)
        poolFree(statement[i].reqDisjVarsA);
    if (statement[i].reqDisjVarsB != NULL_NMBRSTRING)
        poolFree(statement[i].reqDisjVarsB);
    if (statement[i].reqDisjVarsStmt != NULL_NMBRSTRING)
        poolFree(statement[i].reqDisjVarsStmt);
    if (statement[i].optDisjVarsA != NULL_NMBRSTRING)
        poolFree(statement[i].optDisjVarsA);
    if (statement[i].optDisjVarsB != NULL_NMBRSTRING)
        poolFree(statement[i].optDisjVarsB);
    if (statement[i].optDisjVarsStmt != NULL_NMBRSTRING)
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

  /* 28-Aug-2013 am - mathToken[mathTokens].tokenName is assigned in
     parseMathDecl() by let().  eraseSource() should free every mathToken and
     there are (mathTokens + dummyVars) tokens. */
  for (i = 0; i <= mathTokens + dummyVars; i++) {
    let(&mathToken[i].tokenName,"");
  }

  memFreePoolPurge(0);
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

  /* Deallocate the texdef/htmldef storage */ /* Added 27-Oct-2012 nm */
  if (texDefsRead) {
    texDefsRead = 0;
    free(texDefs);
  }
  sandboxStmt = 0; /* Used by a non-zero test in mmwtex.c to see if assigned */
  extHtmlStmt = 0; /* May be used by a non-zero test; init to be safe */

  /* Allocate big arrays */
  initBigArrays();

  bracketMatchInit = 0; /* Clear to force mmunif.c to scan $a's again */
  minSubstLen = 1; /* Initialize to the default SET EMPTY_SUBSTITUTION OFF */
} /* eraseSource */


/* If verify = 0, parse the proofs only for gross error checking.
   If verify = 1, do the full verification. */
void verifyProofs(vstring labelMatch, flag verifyFlag) {
  vstring emptyProofList = "";
  long i, k;
  long lineLen = 0;
  vstring header = "";
  flag errorFound;
#ifdef CLOCKS_PER_SEC
  clock_t clockStart;  /* 28-May-04 nm */
#endif

#ifdef __WATCOMC__
  vstring tmpStr="";
#endif

#ifdef VAXC
  vstring tmpStr="";
#endif

#ifdef CLOCKS_PER_SEC
  clockStart = clock();  /* 28-May-04 nm Retrieve start time */
#endif
  if (!strcmp("*", labelMatch) && verifyFlag) {
    /* Use status bar */
    let(&header, "0 10%  20%  30%  40%  50%  60%  70%  80%  90% 100%");
#ifdef XXX /*__WATCOMC__*/
    /* The vsprintf function discards text after "%" in 3rd argument string. */
    /* This is a workaround. */
    for (i = 1; i <= (long)strlen(header); i++) {
      let(&tmpStr, mid(header, i, 1));
      if (tmpStr[0] == '%') let(&tmpStr, "%%");
      print2("%s", tmpStr);
    }
    print2("\n");
#else
#ifdef XXX /*VAXC*/
    /* The vsprintf function discards text after "%" in 3rd argument string. */
    /* This is a workaround. */
    for (i = 1; i <= (long)strlen(header); i++) {
      let(&tmpStr, mid(header, i, 1));
      if (tmpStr[0] == '%') let(&tmpStr, "%%");
      print2("%s", tmpStr);
    }
    print2("\n");
#else
    print2("%s\n", header);
    let(&header, "");
#endif
#endif
  }

  errorFound = 0;
  for (i = 1; i <= statements; i++) {
    if (!strcmp("*", labelMatch) && verifyFlag) {
      while (lineLen < (50 * i) / statements) {
        print2(".");
        lineLen++;
      }
    }

    if (statement[i].type != p_) continue;
    /* 30-Jan-06 nm Added single-character-match argument */
    if (!matchesList(statement[i].labelName, labelMatch, '*', '?')) continue;
    if (strcmp("*",labelMatch) && verifyFlag) {
      /* If not *, print individual labels */
      lineLen = lineLen + (long)strlen(statement[i].labelName) + 1;
      if (lineLen > 72) {
        lineLen = (long)strlen(statement[i].labelName) + 1;
        print2("\n");
      }
      print2("%s ",statement[i].labelName);
    }

    k = parseProof(i);
    if (k >= 2) errorFound = 1;
    if (k < 2) { /* $p with no error */
      if (verifyFlag) {
        if (verifyProof(i) >= 2) errorFound = 1;
        cleanWrkProof(); /* Deallocate verifyProof storage */
      }
    }
    if (k == 1) {
      let(&emptyProofList, cat(emptyProofList, ", ", statement[i].labelName,
          NULL));
    }
  }
  if (verifyFlag) {
    print2("\n");
  }

  if (emptyProofList[0]) {
    printLongLine(cat(
        "Warning:  The following $p statement(s) were not proved:  ",
        right(emptyProofList,3), NULL)," ","  ");
  }
  if (!emptyProofList[0] && !errorFound && !strcmp("*", labelMatch)) {
    if (verifyFlag) {
#ifdef CLOCKS_PER_SEC    /* 28-May-04 nm */
      print2("All proofs in the database were verified in %1.2f s.\n",
           (double)((1.0 * (clock() - clockStart)) / CLOCKS_PER_SEC));
#else
      print2("All proofs in the database were verified.\n");
#endif

    } else {
      print2("All proofs in the database passed the syntax-only check.\n");
    }
  }
  let(&emptyProofList, ""); /* Deallocate */

} /* verifyProofs */


/* If checkFiles = 0, do not open external files.
   If checkFiles = 1, check mm*.html, presence of gifs, etc. */
void verifyMarkup(vstring labelMatch, flag checkFiles) {
  flag f;
  long errCount = 0;
  long stmtNum;
  vstring contributor = ""; vstring contribDate = "";
  vstring reviser = ""; vstring reviseDate = "";
  vstring shortener = ""; vstring shortenDate = "";

  for (stmtNum = 1; stmtNum <= statements; stmtNum++) {
    if (!matchesList(statement[stmtNum].labelName, labelMatch, '*', '?'))
      continue;
    /* Use the error-checking feature of getContrib() extractor */
    f = getContrib(stmtNum, &contributor, &contribDate,
        &reviser, &reviseDate, &shortener, &shortenDate,
        1/*printErrorsFlag*/);
    if (f == 1) errCount++;
  }

  if (checkFiles == 1) {
    /* Future checks here */
  }

  if (errCount == 0) {
    print2("No errors were found.\n");
  } else {
    print2("Errors were found in %ld statements.\n", errCount);
  }

  return;

} /* verifyMarkup */


/* Added 14-Sep-2012 nm */
/* Take a relative step FIRST, LAST, +nn, -nn (relative to the unknown
   essential steps) or ALL, and return the actual step for use by ASSIGN,
   IMPROVE, REPLACE, LET (or 0 in case of ALL, used by IMPROVE).  In case
   stepStr is an unsigned integer nn, it is assumed to already be an actual
   step and is returned as is.  If format is illegal, -1 is returned.  */
long getStepNum(vstring relStep, /* User's argument */
   nmbrString *pfInProgress, /* proofInProgress.proof */
   flag allFlag /* 1 = "ALL" is permissable */)
{
  long pfLen, i, j, relStepVal, actualStepVal;
  flag negFlag = 0;
  nmbrString *essentialFlags = NULL_NMBRSTRING;
  vstring relStepCaps = "";

  let(&relStepCaps, edit(relStep, 32/*upper case*/));
  pfLen = nmbrLen(pfInProgress); /* Proof length */
  relStepVal = (long)(val(relStepCaps)); /* val() tolerates ill-formed numbers */

  if (relStepVal >= 0 && !strcmp(relStepCaps, str(relStepVal))) {
    /* User's argument is an unsigned positive integer */
    actualStepVal = relStepVal;
    if (actualStepVal > pfLen || actualStepVal < 1) {
      print2("?The step must be in the range from 1 to %ld.\n", pfLen);
      actualStepVal = -1;  /* Flag the error */
    }
    goto RETURN_POINT;  /* Already actual step; just return it */
  } else if (!strcmp(relStepCaps, left("FIRST", (long)(strlen(relStepCaps))))) {
    negFlag = 0; /* Scan forwards */
    relStepVal = 0;
  } else if (!strcmp(relStepCaps, left("LAST", (long)(strlen(relStepCaps))))) {
    negFlag = 1; /* Scan backwards */
    relStepVal = 0;
  } else if (relStepCaps[0] == '+') {
    negFlag = 0;
    if (strcmp(right(relStepCaps, 2), str(relStepVal))) {
      print2("?The characters after '+' are not a number.\n");
      actualStepVal = -1; /* Error - not a number after the '+' */
      goto RETURN_POINT;
    }
  } else if (relStepCaps[0] == '-') {
    negFlag = 1;
    if (strcmp(right(relStepCaps, 2), str(- relStepVal))) {
      print2("?The characters after '-' are not a number.\n");
      actualStepVal = -1; /* Error - not a number after the '-' */
      goto RETURN_POINT;
    }
    relStepVal = - relStepVal;
  } else if (!strcmp(relStepCaps, left("ALL", (long)(strlen(relStepCaps))))) {
    if (!allFlag) {
      /* ALL is illegal */
      print2("?You must specify FIRST, LAST, nn, +nn, or -nn.\n");
      actualStepVal = -1; /* Flag that there was an error */
      goto RETURN_POINT;
    }
    actualStepVal = 0; /* 0 is special, meaning "ALL" */
    goto RETURN_POINT;
  } else {
    if (allFlag) {
      print2("?You must specify FIRST, LAST, nn, +nn, -nn, or ALL.\n");
    } else {
      print2("?You must specify FIRST, LAST, nn, +nn, or -nn.\n");
    }
    actualStepVal = -1; /* Flag that there was an error */
    goto RETURN_POINT;
  }

  nmbrLet(&essentialFlags, nmbrGetEssential(pfInProgress));

  /* Get the essential step flags */
  actualStepVal = 0; /* Use zero as flag that step wasn't found */
  if (negFlag) {
    /* Scan proof backwards */
    /* Count back 'relStepVal' unknown steps */
    j = relStepVal + 1;
    for (i = pfLen; i >= 1; i--) {
      if (essentialFlags[i - 1]
          && pfInProgress[i - 1] == -(long)'?') {
        j--;
        if (j == 0) {
          /* Found it */
          actualStepVal = i;
          break;
        }             /* 16-Apr-06 */
      }
    } /* Next i */
  } else {
    /* Scan proof forwards */
    /* Count forward 'relStepVal' unknown steps */
    j = relStepVal + 1;
    for (i = 1; i <= pfLen; i++) {
      if (essentialFlags[i - 1]
          && pfInProgress[i - 1] == -(long)'?') {
        j--;
        if (j == 0) {
          /* Found it */
          actualStepVal = i;
          break;
        }             /* 16-Apr-06 */
      }
    } /* Next i */
  }
  if (actualStepVal == 0) {
    if (relStepVal == 0) {
      print2("?There are no unknown essential steps.\n");
    } else {
      print2("?There are not at least %ld unknown essential steps.\n",
        relStepVal + 1);
    }
    actualStepVal = -1; /* Flag that there was an error */
    goto RETURN_POINT;
  }

 RETURN_POINT:
  /* Deallocate memory */
  let(&relStepCaps, "");
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);

  return actualStepVal;
} /* getStepNum */


/* Added 22-Apr-2015 nm */
/* Convert the actual step numbers of an unassigned step to the relative
   -1, -2, etc. offset for SHOW NEW_PROOF ...  /UNKNOWN, to make it easier
   for the user to ASSIGN the relative step number. A 0 is returned
   for the last unknown step.  The step numbers of known steps are
   unchanged.  */
/* The caller must deallocate the returned nmbrString. */
nmbrString *getRelStepNums(nmbrString *pfInProgress) {
  nmbrString *essentialFlags = NULL_NMBRSTRING;
  nmbrString *relSteps = NULL_NMBRSTRING;
  long i, j, pfLen;

  pfLen = nmbrLen(pfInProgress); /* Get proof length */
  nmbrLet(&relSteps, nmbrSpace(pfLen));  /* Initialize */
  nmbrLet(&essentialFlags, nmbrGetEssential(pfInProgress));
  j = 0;  /* Negative offset (or 0 for last unknown step) */
  for (i = pfLen; i >= 1; i--) {
    if (essentialFlags[i - 1]
        && pfInProgress[i - 1] == -(long)'?') {
      relSteps[i - 1] = j;
      j--; /* It's an essential unknown step; increment negative offset */
    } else {
      relSteps[i - 1] = i; /* Just keep the normal step number */
    }
  }

  /* Deallocate memory */
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);

  return relSteps;
} /* getRelStepNums */


/* 19-Sep-2012 nm */
/* This procedure finds the next statement number whose label matches
   stmtName.  Wildcards are allowed.  If uniqueFlag is 1,
   there must be exactly one match, otherwise an error message is printed,
   and -1 is returned.  If uniqueFlag is 0, the next match is
   returned, or -1 if there are no more matches.  No error messages are
   printed when uniqueFlag is 0, except for the special case of
   startStmt=1.  For use by PROVE, REPLACE, ASSIGN. */
long getStatementNum(vstring stmtName, /* Possibly with wildcards */
    long startStmt, /* Starting statement number (1 for full scan) */
    long maxStmt, /* Matches must be LESS THAN this statement number */
    flag aAllowed, /* 1 means $a is allowed */
    flag pAllowed, /* 1 means $p is allowed */
    flag eAllowed, /* 1 means $e is allowed */
    flag fAllowed, /* 1 means $f is allowed */
    flag efOnlyForMaxStmt, /* If 1, $e and $f must belong to maxStmt */
    flag uniqueFlag) /* If 1, match must be unique */
{
  flag hasWildcard;
  long matchesFound, matchStmt, matchStmt2, stmt;
  char typ;
  flag laterMatchFound = 0; /* For better error message */ /* 16-Jan-2014 nm */

  hasWildcard = 0;
  if (instr(1, stmtName, "*") || instr(1, stmtName, "?"))
    hasWildcard = 1;
  matchesFound = 0;
  matchStmt = 1; /* Set to a legal value in case of bug */
  matchStmt2 = 1; /* Set to a legal value in case of bug */

  for (stmt = startStmt; stmt <= statements; stmt++) {

    /* 16-Jan-2014 nm */
    if (stmt >= maxStmt) {
      if (matchesFound > 0) break; /* Normal exit when a match was found */
      if (!uniqueFlag) break; /* We only want to scan up to maxStmt anyway */
      /* Otherwise, we continue to see if there is a later match, for
         error message purposes */
    }

    if (!statement[stmt].labelName[0]) continue; /* No label */
    typ = statement[stmt].type;

    if ((!aAllowed && typ == (char)a_)
        ||(!pAllowed && typ == (char)p_)
        ||(!eAllowed && typ == (char)e_)
        ||(!fAllowed && typ == (char)f_)) {
      continue; /* Statement type is not allowed */
    }

    if (hasWildcard) {
      if (!matchesList(statement[stmt].labelName, stmtName, '*', '?')) {
        continue;
      }
    } else {
      if (strcmp(stmtName, statement[stmt].labelName)) {
        continue;
      }
    }

    if (efOnlyForMaxStmt) {
      if (maxStmt > statements) bug(247); /* Don't set efOnlyForMaxStmt
                                             in case of PROVE call */
      /* If a $e or $f, it must be a hypothesis of the statement
         being proved */
      if (typ == (char)e_ || typ == (char)f_){
        if (!nmbrElementIn(1, statement[maxStmt].reqHypList, stmt) &&
            !nmbrElementIn(1, statement[maxStmt].optHypList, stmt))
            continue;
      }
    }

    /* 16-Jan-2014 nm */
    if (stmt >= maxStmt) {
      /* For error messages:
         This signals that a later match (after the statement being
         proved, in case of ASSIGN) exists, so the user is trying to
         reference a future statement. */
      laterMatchFound = 1;
      break;
    }

    if (matchesFound == 0) {
      /* This is the first match found; save it */
      matchStmt = stmt;
      /* If uniqueFlag is not set, we're done (don't need to check for
         uniqueness) */
    }
    if (matchesFound == 1) {
      /* This is the 2nd match found; save it for error message */
      matchStmt2 = stmt;
    }
    matchesFound++;
    if (!uniqueFlag) break; /* We are just getting the next match, so done */
    if (!hasWildcard) break; /* Since there can only be 1 match, don't
                                bother to continue */
  }

  if (matchesFound == 0) {
    if (!uniqueFlag) {
      if (startStmt == 1) {
        /* For non-unique scan, print only if we started from beginning */
        print2("?No statement label matches \"%s\".\n", stmtName);
      }
    } else if (aAllowed && pAllowed && eAllowed && fAllowed
               && !efOnlyForMaxStmt) {
      print2("?No statement label matches \"%s\".\n", stmtName);
    } else if (!aAllowed && pAllowed && !eAllowed && !fAllowed) {
      /* This is normally the PROVE command */
      print2("?No $p statement label matches \"%s\".\n", stmtName);
    } else if (!eAllowed && !fAllowed) {
      /* This is normally for REPLACE */
      if (!laterMatchFound) {
        print2("?No $a or $p statement label matches \"%s\".\n",
          stmtName);
      } else {
        /* 16-Jan-2014 nm */
        print2(
   "?You must specify a statement that occurs earlier the one being proved.\n");
      }
    } else {
      /* This is normally for ASSIGN */
      if (!laterMatchFound) {
        printLongLine(cat("?A statement label matching \"",
            stmtName,
            "\" was not found or is not a hypothesis of the statement ",
            "being proved.", NULL), "", " ");
      } else {
        /* 16-Jan-2014 nm */
        print2(
   "?You must specify a statement that occurs earlier the one being proved.\n");
      }
    }
  } else if (matchesFound == 2) {
    printLongLine(cat("?This command requires a unique label, but there are ",
        " 2 matches for \"",
        stmtName, "\":  \"", statement[matchStmt].labelName,
        "\" and \"", statement[matchStmt2].labelName, "\".",
        NULL), "", " ");
  } else if (matchesFound > 2) {
    printLongLine(cat("?This command requires a unique label, but there are ",
        str(matchesFound), " (allowed) matches for \"",
        stmtName, "\".  The first 2 are \"", statement[matchStmt].labelName,
        "\" and \"", statement[matchStmt2].labelName, "\".",
        "  Use SHOW LABELS \"", stmtName, "\" to see all non-$e matches.",
        NULL), "", " ");
  }
  if (!uniqueFlag && matchesFound > 1) bug(248);
  if (matchesFound != 1) matchStmt = -1; /* Error - no (unique) match */
  return matchStmt;
} /* getStatementNum */




/* Called by help() - prints a help line */
/* THINK C gives compilation error if H() is lower-case h() -- why? */
void H(vstring helpLine)
{
  if (printHelp) {
    print2("%s\n", helpLine);
  }
} /* H */



/******** 8/28/00 ***********************************************************/
/******** The MIDI output algorithm is in this function, outputMidi(). ******/
/*** Warning:  If you want to experiment with the MIDI output, please
     confine changes to this function.  Changes to other code
     in this file is not recommended. ***/

void outputMidi(long plen, nmbrString *indentationLevels,
  nmbrString *logicalFlags, vstring midiParameter, vstring statementLabel) {

  /* The parameters have the following meanings.  You should treat them as
     read-only input parameters and should not modify the contents of the
     arrays or strings they point to.

       plen = length of proof
       indentationLevels[step] = indentation level in "show proof xxx /full"
           where step varies from 0 to plen-1
       logicalFlags[step] = 0 for formula-building step, 1 for logical step
       midiParameter = string passed by user in "midi xxx /parameter <midiParameter>"
       statementLabel = label of statement whose proof is being scanned */

  /* This function is called when the user types "midi xxx /parameter
     <midiParameter>".  The proof steps of theorem xxx are numbered successively
     from 0 to plen-1.  The arrays indentationLevels[] and logicalFlags[]
     have already been populated for you. */

  /* The purpose of this function is to create an ASCII file called xxx.txt
     that contains the ASCII format for a t2mf input file.  The mf2t package
     is available at http://www.hitsquad.com/smm/programs/mf2t/download.shtml.
     To convert xxx.txt to xxx.mid you type "t2mf xxx.txt xxx.mid". */

  /* To experiment with this function, you can add your own local variables and
     modify the algorithm as you see fit.  The algorithm is essentially
     contained in the loop "for (step = 0; step < plen; step++)" and in the
     initialization of the local variables that this loop uses.  No global
     variables are used inside this function; the only data used is
     that contained in the input parameters. */

/* Larger TEMPO means faster speed */
#define TEMPO 48
/* The minimum and maximum notes for the dynamic range we allow: */
/* (MIDI notes range from 1 to 127, but 28 to 103 seems reasonably pleasant) */
/* MIDI note number 60 is middle C. */
#define MIN_NOTE 28
#define MAX_NOTE 103

  /* Note: "flag" is just "char"; "vstring" is just "char *" - except
     that by this program's convention vstring allocation is controlled
     by string functions in mmvstr.c; and "nmbrString" is just "long *" */

  long step; /* Proof step from 0 to plen-1 */
  long midiKey; /* Current keyboard key to be output */
  long midiNote; /* Current midi note to be output, mapped from midiKey */
  long midiTime; /* Midi time stamp */
  long midiPreviousFormulaStep; /* Note saved from previous step */
  long midiPreviousLogicalStep; /* Note saved from previous step */
  vstring midiFileName = ""; /* All vstrings MUST be initialized to ""! */
  FILE *midiFilePtr; /* Output file pointer */
  long midiBaseline; /* Baseline note */
  long midiMaxIndent; /* Maximum indentation (to find dyn range of notes) */
  long midiMinKey; /* Smallest keyboard key in output */
  long midiMaxKey; /* Largest keyboard key in output */
  long midiKeyInc; /* Keyboard key increment per proof indentation level */
  flag midiSyncopate; /* 1 = syncopate the output */
  flag midiHesitate; /* 1 = silence all repeated notes */
  long midiTempo; /* larger = faster */
  vstring midiLocalParam = ""; /* To manipulate user's parameter string */
  vstring tmpStr = ""; /* Temporary string */
#define ALLKEYSFLAG 1
#define WHITEKEYSFLAG 2
#define BLACKKEYSFLAG 3
  flag keyboardType; /* ALLKEYSFLAG, WHITEKEYSFLAG, or BLACKKEYSFLAG */
  long absMinKey; /* The smallest note we ever want */
  long absMaxKey; /* The largest note we ever want */
  long key2MidiMap[128]; /* Maps keyboard (possiblty with black or
                    white notes missing) to midi notes */
  long keyboardOctave; /* The number of keys spanning an octave */
  long i;

  /******** Define the keyboard to midi maps (added 5/17/04 nm) *************/
  /* The idea here is to map the proof step to pressing "keyboard" keys
     on keyboards that have all keys, or only the white keys, or only the
     black keys.   The default is all keys, the parameter b means black,
     and the parameter w means white. */
#define ALLKEYS 128
#define WHITEKEYS 75
#define BLACKKEYS 53
  long allKeys[ALLKEYS] =
      {  0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,
        12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,
        24,  25,  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,
        36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
        48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
        60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,
        72,  73,  74,  75,  76,  77,  78,  79,  80,  81,  82,  83,
        84,  85,  86,  87,  88,  89,  90,  91,  92,  93,  94,  95,
        96,  97,  98,  99, 100, 101, 102, 103, 104, 105, 106, 107,
       108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
       120, 121, 122, 123, 124, 125, 126, 127};
  long whiteKeys[WHITEKEYS] =
      {  0,   2,   4,   5,   7,   9,  11,
        12,  14,  16,  17,  19,  21,  23,
        24,  26,  28,  29,  31,  33,  35,
        36,  38,  40,  41,  43,  45,  47,
        48,  50,  52,  53,  55,  57,  59,
        60,  62,  64,  65,  67,  69,  71,
        72,  74,  76,  77,  79,  81,  83,
        84,  86,  88,  89,  91,  93,  95,
        96,  98, 100, 101, 103, 105, 107,
       108, 110, 112, 113, 115, 117, 119,
       120, 122, 124, 125, 127};
  long blackKeys[BLACKKEYS] =
      {  1,   3,   6,   8,  10,
        13,  15,  18,  20,  22,
        25,  27,  30,  32,  34,
        37,  39,  42,  44,  46,
        49,  51,  54,  56,  58,
        61,  63,  66,  68,  70,
        73,  75,  78,  80,  82,
        85,  87,  90,  92,  94,
        97,  99, 102, 104, 106,
       109, 111, 114, 116, 118,
       121, 123, 126};

  /************* Initialization ***************/

  midiTime = 0; /* MIDI time stamp */
  midiPreviousFormulaStep = 0; /* Note in previous formula-building step */
  midiPreviousLogicalStep = 0; /* Note in previous logical step */
  midiFilePtr = NULL; /* Output file pointer */

  /* Parse the parameter string passed by the user */
  let(&midiLocalParam, edit(midiParameter, 32)); /* Convert to uppercase */

  /* Set syncopation */
  if (strchr(midiLocalParam, 'S') != NULL) {
    midiSyncopate = 1; /* Syncopation */
  } else {
    midiSyncopate = 0; /* No syncopation */
  }
  /* Set halting character of syncopation (only has effect if
     syncopation is on) */
  if (strchr(midiLocalParam, 'H') != NULL) {
    midiHesitate = 1; /* Silence all repeated fast notes */
  } else {
    midiHesitate = 0; /* Silence only every other one in a repeated sequence */
  }
  /* Set the tempo: 96=fast, 72=medium, 48=slow */
  if (strchr(midiLocalParam, 'F') != NULL) {
    midiTempo = 2 * TEMPO;  /* Fast */
  } else {
    if (strchr(midiLocalParam, 'M') != NULL) {
      midiTempo = 3 * TEMPO / 2;  /* Medium */
    } else {
      midiTempo = TEMPO; /* Slow */
    }
  }
  /* Get the keyboard type */
  if (strchr(midiLocalParam, 'W') != NULL) {
    keyboardType = WHITEKEYSFLAG;
  } else {
    if (strchr(midiLocalParam, 'B') != NULL) {
      keyboardType = BLACKKEYSFLAG;
    } else {
      keyboardType = ALLKEYSFLAG;
    }
  }
  /* Set the tempo: 96=fast, 48=slow */
  if (strchr(midiLocalParam, 'I') != NULL) {
    /* Do not skip any notes */
    midiKeyInc = 1 + 1;
  } else {
    /* Allow an increment of up to 4 */
    midiKeyInc = 4 + 1;
  }

  /* End of parsing user's parameter string */


  /* Map keyboard key numbers to MIDI notes */
  absMinKey = MIN_NOTE; /* Initialize for ALLKEYSFLAG case */
  absMaxKey = MAX_NOTE;
  keyboardOctave = 12; /* Keyboard keys per octave with no notes skipped */
  switch (keyboardType) {
    case ALLKEYSFLAG:
      for (i = 0; i < 128; i++) key2MidiMap[i] = allKeys[i];
     break;
    case WHITEKEYSFLAG:
      for (i = 0; i < WHITEKEYS; i++) key2MidiMap[i] = whiteKeys[i];
      keyboardOctave = 7;
      /* Get keyboard key for smallest midi note we want */
      for (i = 0; i < WHITEKEYS; i++) {
        if (key2MidiMap[i] >= absMinKey) {
          absMinKey = i;
          break;
        }
      }
      /* Get keyboard key for largest midi note we want */
      for (i = WHITEKEYS - 1; i >= 0; i--) {
        if (key2MidiMap[i] <= absMinKey) {
          absMinKey = i;
          break;
        }
      }
      /* Redundant array bound check for safety */
      if (absMaxKey >= WHITEKEYS) absMaxKey = WHITEKEYS - 1;
      if (absMinKey >= WHITEKEYS) absMinKey = WHITEKEYS - 1;
      break;
    case BLACKKEYSFLAG:
      for (i = 0; i < BLACKKEYS; i++) key2MidiMap[i] = blackKeys[i];
      keyboardOctave = 5;
      /* Get keyboard key for smallest midi note we want */
      for (i = 0; i < BLACKKEYS; i++) {
        if (key2MidiMap[i] >= absMinKey) {
          absMinKey = i;
          break;
        }
      }
      /* Get keyboard key for largest midi note we want */
      for (i = BLACKKEYS - 1; i >= 0; i--) {
        if (key2MidiMap[i] <= absMinKey) {
          absMinKey = i;
          break;
        }
      }
      /* Redundant array bound check for safety */
      if (absMaxKey >= BLACKKEYS) absMaxKey = BLACKKEYS - 1;
      if (absMinKey >= BLACKKEYS) absMinKey = BLACKKEYS - 1;
      break;
  }

  /* Get max indentation, so we can determine the scale factor
     to make midi output fit within dynamic range */
  midiMaxIndent = 0;
  for (step = 0; step < plen; step++) {
    if (indentationLevels[step] > midiMaxIndent)
      midiMaxIndent = indentationLevels[step];
  }

  /* We will use integral note increments multiplied by the indentation
     level.  We pick the largest possible, with a maximum of 4, so that the
     midi output stays within the desired dynamic range.  If the proof has
     *too* large a maximum indentation (not seen so far), the do loop below
     will decrease the note increment to 0, so the MIDI output will just be a
     flat sequence of repeating notes and therefore useless, but at least it
     won't crash the MIDI converter.  */

  /*midiKeyInc = 5;*/ /* Starting note increment, plus 1 */
        /* (This is now set with the I parameter; see above) */
  do { /* Decrement note incr until song will fit MIDI dyn range */
    midiKeyInc--;
    /* Compute the baseline note to which add the proof indentation
      times the midiKeyInc.  The "12" is to allow for the shift
      of one octave down of the sustained notes on "essential"
      (i.e. logical, not formula-building) steps. */
    midiBaseline = ((absMaxKey + absMinKey) / 2) -
      (((midiMaxIndent * midiKeyInc) - keyboardOctave/*12*/) / 2);
    midiMinKey = midiBaseline - keyboardOctave/*12*/;
    midiMaxKey = midiBaseline + (midiMaxIndent * midiKeyInc);
  } while ((midiMinKey < absMinKey || midiMaxKey > absMaxKey) &&
      midiKeyInc > 0);

  /* Open the output file */
  let(&midiFileName, cat(statement[showStatement].labelName,
      ".txt", NULL)); /* Create file name from statement label */
  print2("Creating MIDI source file \"%s\"...", midiFileName);

  /* fSafeOpen() renames existing files with ~1,~2,etc.  This way
     existing user files will not be accidentally destroyed. */
  midiFilePtr = fSafeOpen(midiFileName, "w");
  if (midiFilePtr == NULL) {
    print2("?Couldn't open %s\n", midiFileName);
    goto midi_return;
  }

  /* Output the MIDI header */
  fprintf(midiFilePtr, "MFile 0 1 %ld\n", midiTempo);
  fprintf(midiFilePtr, "MTrk\n");

  /* Create a string exactly 38 characters long for the Meta Text
     label (I'm not sure why, but they are in the t2mf examples) */
  let(&tmpStr, cat("Theorem ", statementLabel, " ", midiParameter,
      space(30), NULL));
  let(&tmpStr, left(tmpStr, 38));
  fprintf(midiFilePtr, "0 Meta Text \"%s\"\n", tmpStr);
  fprintf(midiFilePtr,
      "0 Meta Copyright \"Released to Public Domain by N. Megill\"\n");

  /************** Scan the proof ************************/

  for (step = 0; step < plen; step++) {
  /* Handle the processing that takes place at each proof step */
    if (!logicalFlags[step]) {

      /*** Process the higher fast notes for formula-building steps ***/
      /* Get the "keyboard" key number */
      midiKey = (midiKeyInc * indentationLevels[step]) + midiBaseline;
      /* Redundant prevention of array bound violation */
      if (midiKey < 0) midiKey = 0;
      if (midiKey > absMaxKey) midiKey = absMaxKey;
      /* Map "keyboard" key to MIDI note */
      midiNote = key2MidiMap[midiKey];
      if (midiPreviousFormulaStep != midiNote || !midiSyncopate) {
        /* Turn note on at the current MIDI time stamp */
        fprintf(midiFilePtr, "%ld On ch=2 n=%ld v=75\n", midiTime, midiNote);
        /* Turn note off at the time stamp + 18 */
        fprintf(midiFilePtr, "%ld On ch=2 n=%ld v=0\n", midiTime + 18,
            midiNote);
        midiPreviousFormulaStep = midiNote;
      } else {
        /* Skip turning on the note to give syncopation */
        /* To prevent skipping two notes in a row, set last note
           to 0 so next step it will not be skipped even if the
           note still doesn't change; this makes the syncopation
           more rhythmic */
        if (!midiHesitate) {
          midiPreviousFormulaStep = 0;
        }
      }
      midiTime += 24; /* Add 24 to the MIDI time stamp */

    } else {

      /*** Process the deeper sustained notes for logical steps ***/
      /* The idea here is to shift the note down 1 octave before
         outputting it, so it is distinguished from formula-
         building notes */
      /* Get the "keyboard" key number */
      midiKey = (midiKeyInc * indentationLevels[step]) + midiBaseline;
      /* Redundant prevention of array bound violation */
      if (midiKey < 0) midiKey = 0;
      if (midiKey > absMaxKey) midiKey = absMaxKey;
      /* Map "keyboard" key to MIDI note */
      midiNote = key2MidiMap[midiKey];
      midiNote = midiNote - 12; /* Down 1 octave */
      if (midiNote < 0) midiNote = 0; /* For safety but should be redundant */
      if (midiPreviousLogicalStep) { /* If 0, it's the first time */
        /* Turn off the previous sustained note */
        fprintf(midiFilePtr, "%ld On ch=1 n=%ld v=0\n", midiTime,
            midiPreviousLogicalStep);
      }
      /* Turn on the new sustained note */
      fprintf(midiFilePtr, "%ld On ch=1 n=%ld v=100\n", midiTime,
          midiNote);
      midiTime += 24; /* Add 24 to the MIDI time stamp */
      midiPreviousLogicalStep = midiNote; /* Save for next step */

    }
  } /* next step */

  /****************** Clean up and close output file ****************/

  /* After the last step, do the file closing stuff */
  /* Sustain the very last note a little longer - sounds better */
  midiTime += 72;
  fprintf(midiFilePtr, "%ld On ch=1 n=%ld v=0\n", midiTime,
      midiPreviousLogicalStep);
  /* Output the MIDI file trailer */
  fprintf(midiFilePtr, "%ld Meta TrkEnd\n", midiTime);
  fprintf(midiFilePtr, "TrkEnd\n");
  fclose(midiFilePtr);
  /* Inform the user the run time of the MIDI file */
  print2(" length = %ld sec\n", (long)(midiTime / (2 * midiTempo)));

 midi_return:
  /* Important: all local vstrings must be deallocated to prevent
     memory leakage */
  let(&midiFileName, "");
  let(&tmpStr, "");
  let(&midiLocalParam, "");

} /* outputMidi */
