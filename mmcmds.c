/*****************************************************************************/
/*        Copyright (C) 2004  NORMAN MEGILL  nm at alum.mit.edu              */
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
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmdl.h" /* For texFileName */
#include "mmcmds.h"
#include "mminou.h"
#include "mmpars.h"
#include "mmveri.h"
#include "mmwtex.h" /* For htmlVarColors,... */
#include "mmpfas.h"

vstring mainFileName = "";
flag printHelp = 0;

/* For HTML output */
vstring printStringForReferencedBy = "";

/* For MIDI */
flag midiFlag = 0;
vstring midiParam = "";

/* Type (i.e. print) a statement */
void typeStatement(long showStatement,
  flag briefFlag,
  flag commentOnlyFlag,
  flag texFlag,
  flag htmlFlag)
{
  /* From HELP SHOW STATEMENT: Optional qualifiers:
    / TEX - This qualifier will write the statement information to the
        LaTeX file previously opened with OPEN TEX.
            [Note:  texFlag=1 and htmlFlag=0 with this qualifier.]

    / HTML - This qualifier will write the statement information to the
        HTML file previously opened with OPEN HTML.
            [Note:  texFlag=1 and htmlFlag=1 with this qualifier.]

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

  /* For syntax breakdown of definitions in HTML page */
  long zapStatement1stToken;
  static long wffToken = -1; /* array index of the hard-coded token "wff" -
      static so we only have to look it up once - set to -2 if not found */

  subType = 0; /* Assign to prevent compiler warnings - not theor. necessary */

  if (!showStatement) bug(225); /* Must be 1 or greater */

  if (!commentOnlyFlag && !briefFlag) {
    let(&str1, cat("Statement ", str(showStatement),
        " is located on line ", str(statement[showStatement].lineNum),
        " of the file ", NULL));
    if (!texFlag) {
      printLongLine(cat(str1,
        "\"", statement[showStatement].fileName,
        "\".",NULL), "", " ");
    } else {
      if (!htmlFlag) let(&printString, "");
      outputToString = 1; /* Flag for print2 to add to printString */
                     /* Note that printTexLongMathString resets it */
      if (!(htmlFlag && texFlag)) {
        /*
        printLongLine(cat(str1, "{\\tt ",
            asciiToTt(statement[showStatement].fileName),
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
        if (statement[showStatement].type == (char)p__) {
          subType = THEOREM;
        } else {
          /* Must be a__ due to filter in main() */
          if (statement[showStatement].type != (char)a__) bug(228);
          if (strcmp("|-", mathToken[
              (statement[showStatement].mathString)[0]].tokenName)) {
            subType = SYNTAX;
          } else {
            if (!strcmp("ax-", left(statement[showStatement].labelName, 3))) {
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
        str2 = pinkHTML(showStatement);
        printLongLine(cat("<CENTER><B><FONT SIZE=\"+1\">", str1,
            " <FONT COLOR=", GREEN_TITLE_COLOR,
            ">", statement[showStatement].labelName,
            "</FONT></FONT></B>", str2, "</CENTER>", NULL), "", "\"");
      } /* (htmlFlag && texFlag) */
      outputToString = 0;
    } /* texFlag */
  }

  if (!briefFlag || commentOnlyFlag) {
    let(&str1, "");
    str1 = getDescription(showStatement);
    if (!str1[0]    /* No comment */
        || (str1[0] == '[' && str1[strlen(str1) - 1] == ']')) { /* Date stamp
            from previous proof */
      print2("?Warning:  Statement \"%s\" has no comment\n",
          statement[showStatement].labelName);
    /*if (str1[0]) {*/ /*old*/
    } else {
      if (!texFlag) {
        printLongLine(cat("\"", str1, "\"", NULL), "", " ");
      } else {
        /* 10/10/02 This is now done in mmwtex.c printTexComment */
        /* Although it will affect the 2nd (and only other) call to
           printTexComment below, that call is obsolete and there should
           be no side effect. */
        /*******
        if (htmlFlag && texFlag) {
          let(&str1, cat("<CENTER><B>Description: </B>", str1,
              "</CENTER><BR>", NULL));
        }
        *******/
        if (!htmlFlag) {  /* LaTeX */
          /* 6-Dec-03 Add separation space between theorems */
          let(&str1, cat("\n\\vspace{1ex}\n\n", str1, NULL));
        }
        printTexComment(str1, 1);
      }
    }
  }
  if (commentOnlyFlag && !briefFlag) goto returnPoint;

  if ((briefFlag && !texFlag) ||
       (htmlFlag && texFlag) /* HTML page 12/23/01 */) {
    /* In BRIEF mode screen output, show $d's */
    /* This section was added 8/31/99 */

    /* 12/23/01 - added algorithm to HTML pages also; the string to print out
       is stored in htmlDistinctVars for later printing */

    /* 12/23/01 */
    if (htmlFlag && texFlag) {
      let(&htmlDistinctVars, "");
      htmlDistinctVarsCommaFlag = 0;
    }

    let(&str1, "");
    nmbrTmpPtr1 = statement[showStatement].reqDisjVarsA;
    nmbrTmpPtr2 = statement[showStatement].reqDisjVarsB;
    i = nmbrLen(nmbrTmpPtr1);
    if (i /* Number of mandatory $d pairs */) {
      nmbrLet(&nmbrDDList, NULL_NMBRSTRING);
      for (k = 0; k < i; k++) {
        /* Is one of the variables in the current list? */
        if (!nmbrElementIn(1, nmbrDDList, nmbrTmpPtr1[k]) &&
            !nmbrElementIn(1, nmbrDDList, nmbrTmpPtr2[k])) {
          /* No, so close out the current list */
          if (!(htmlFlag && texFlag)) { /* 12/23/01 */
            if (k == 0) let(&str1, "$d");
            else let(&str1, cat(str1, " $.  $d", NULL));
          } else {

            /* 12/23/01 */
            let(&htmlDistinctVars, cat(htmlDistinctVars, " &nbsp; ",
                NULL));
            htmlDistinctVarsCommaFlag = 0;

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
              if (!(htmlFlag && texFlag)) {
                if (k == 0) let(&str1, "$d");
                else let(&str1, cat(str1, " $.  $d", NULL));
              } else {

                /* 12/23/01 */
                let(&htmlDistinctVars, cat(htmlDistinctVars, " &nbsp; ",
                    NULL));
                htmlDistinctVarsCommaFlag = 0;

              }
              nmbrLet(&nmbrDDList, NULL_NMBRSTRING);
              break;
            }
          } /* If $d var in current list is not same as one we're adding */
        } /* Next n */
        /* If the variable is not already in current list, add it */
        if (!nmbrElementIn(1, nmbrDDList, nmbrTmpPtr1[k])) {
          if (!(htmlFlag && texFlag)) {
            let(&str1, cat(str1, " ", mathToken[nmbrTmpPtr1[k]].tokenName,
                NULL));
          } else {

            /* 12/23/01 */
            if (htmlDistinctVarsCommaFlag) {
              let(&htmlDistinctVars, cat(htmlDistinctVars, ",", NULL));
            }
            htmlDistinctVarsCommaFlag = 1;
            let(&str2, "");
            str2 = tokenToTex(mathToken[nmbrTmpPtr1[k]].tokenName);
                 /* tokenToTex allocates str2; we must deallocate it */
            let(&htmlDistinctVars, cat(htmlDistinctVars, str2, NULL));

          }
          nmbrLet(&nmbrDDList, nmbrAddElement(nmbrDDList, nmbrTmpPtr1[k]));
        }
        if (!nmbrElementIn(1, nmbrDDList, nmbrTmpPtr2[k])) {
          if (!(htmlFlag && texFlag)) {
            let(&str1, cat(str1, " ", mathToken[nmbrTmpPtr2[k]].tokenName,
                NULL));
          } else {

            /* 12/23/01 */
            if (htmlDistinctVarsCommaFlag) {
              let(&htmlDistinctVars, cat(htmlDistinctVars, ",", NULL));
            }
            htmlDistinctVarsCommaFlag = 1;
            let(&str2, "");
            str2 = tokenToTex(mathToken[nmbrTmpPtr2[k]].tokenName);
                 /* tokenToTex allocates str2; we must deallocate it */
            let(&htmlDistinctVars, cat(htmlDistinctVars, str2, NULL));

          }
          nmbrLet(&nmbrDDList, nmbrAddElement(nmbrDDList, nmbrTmpPtr2[k]));
        }
      } /* Next k */
      /* Close out entire list */
      if (!(htmlFlag && texFlag)) {
        let(&str1, cat(str1, " $.", NULL));
        printLongLine(str1, "  ", " ");
      } else {

        /* 12/23/01 */
        /* (do nothing) */
        /*let(&htmlDistinctVars, cat(htmlDistinctVars, "<BR>", NULL));*/

      }
    } /* if i(#$d's) > 0 */
  }

  if (briefFlag || texFlag /*(texFlag && htmlFlag)*/) { /* 6-Dec-03 */
    /* For BRIEF mode, print $e hypotheses (only) before statement */
    /* Also do it for HTML output */
    /* 6-Dec-03  For the LaTeX output, now print hypotheses before statement */
    j = nmbrLen(statement[showStatement].reqHypList);
    k = 0;
    for (i = 0; i < j; i++) {
      /* Count the number of essential hypotheses */
      if (statement[statement[showStatement].reqHypList[i]].type
        == (char)e__) k++;

      /* Added 5/26/03 */
      /* For syntax definitions, also include $f hypotheses so user can more
         easily match them in syntax breakdowns of axioms and definitions */
      if (subType == SYNTAX && (texFlag && htmlFlag)) {
        if (statement[statement[showStatement].reqHypList[i]].type
          == (char)f__) k++;
      }

    }
    if (k) {
      if (texFlag) {
        outputToString = 1;
      }
      if (texFlag && htmlFlag) {
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
        k = statement[showStatement].reqHypList[i];
        if (statement[k].type != (char)e__

            /* Added 5/26/03 */
            /* For syntax definitions, include $f hypotheses so user can more
               easily match them in syntax breakdowns of axioms & definitions */
            && !(subType == SYNTAX && (texFlag && htmlFlag)
                && statement[k].type == (char)f__)

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
        } else {
          if (!(htmlFlag && texFlag)) {
            let(&str3, space(strlen(str2)));
            printTexLongMath(statement[k].mathString,
                str2, str3, 0, 0);
          } else {
            outputToString = 1;
            print2("<TR ALIGN=LEFT><TD>%s</TD><TD>\n",
                statement[k].labelName);
            /* Print hypothesis */
            printTexLongMath(statement[k].mathString, "", "", 0, 0);
          }
        }
      } /* next i */
      if (texFlag && htmlFlag) {
        outputToString = 1;
        print2("</TABLE></CENTER>\n");
      }
    } /* if k (#essential hyp) */
  }

  let(&str1, "");
  type = statement[showStatement].type;
  if (type == p__) let(&str1, " $= ...");
  if (!texFlag) let(&str2, cat(str(showStatement), " ", NULL));
  else let(&str2, "  ");
  let(&str2, cat(str2, statement[showStatement].labelName,
      " $",chr(type), " ", NULL));
  if (!texFlag) {
    printLongLine(cat(str2,
        nmbrCvtMToVString(statement[showStatement].mathString),
        str1, " $.", NULL), "      ", " ");
  } else {
    if (!(htmlFlag && texFlag)) {
      let(&str3, space(strlen(str2))); /* 3rd argument of printTexLongMath
          cannot be temp allocated */
      printTexLongMath(statement[showStatement].mathString,
          str2, str3, 0, 0);
    } else {
      outputToString = 1;
      print2("<CENTER><TABLE BORDER CELLSPACING=0 BGCOLOR=%s\n",
          MINT_BACKGROUND_COLOR);
      /* For bobby.cast.org approval */
      print2("SUMMARY=\"Assertion\">\n");
      print2("<CAPTION><B>Assertion</B></CAPTION>\n");
      print2("<TR><TH>Ref\n");
      print2("</TH><TH>Expression</TH></TR>\n");
      print2(
       "<TR ALIGN=LEFT><TD><FONT COLOR=%s><B>%s</B></FONT></TD><TD>\n",
          GREEN_TITLE_COLOR,
          statement[showStatement].labelName);
      printTexLongMath(statement[showStatement].mathString, "", "", 0, 0);
      outputToString = 1;
      print2("</TABLE></CENTER>\n");
    }
  }

  if (briefFlag) goto returnPoint;

  /* 6-Dec-03 In the LaTeX output, the hypotheses used to be printed after the
     statement.  Now they are printed before (see above 6-Dec-03 comments),
     so some code below is commented out. */
  switch (type) {
    case a__:
    case p__:
      /* 6-Dec-03  This is not really needed but keeps output consistent
         with previous version.  It puts a blank line before the HTML
         "distinct variable" list. */
      if (texFlag && htmlFlag) { /* 6-Dec-03 */
        outputToString = 1;
        print2("\n");
        outputToString = 0;
      }

      /*if (!(htmlFlag && texFlag)) {*/
      if (!texFlag) {  /* 6-Dec-03 fix */
        print2("Its mandatory hypotheses in RPN order are:\n");
      }
      /*if (texFlag) outputToString = 0;*/ /* 6-Dec-03 */
      j = nmbrLen(statement[showStatement].reqHypList);
      for (i = 0; i < j; i++) {
        k = statement[showStatement].reqHypList[i];
        if (statement[k].type != (char)e__ && (!htmlFlag && texFlag))
          continue; /* 9/2/99 Don't put $f's in LaTeX output */
        let(&str2, cat("  ",statement[k].labelName,
            " $", chr(statement[k].type), " ", NULL));
        if (!texFlag) {
          printLongLine(cat(str2,
              nmbrCvtMToVString(statement[k].mathString), " $.", NULL),
              "      "," ");
        } else {
          if (!(htmlFlag && texFlag)) {  /* LaTeX */
            /*let(&str3, space(strlen(str2)));*/ /* 6-Dec-03 */
            /* This clears out printString */
            /*printTexLongMath(statement[k].mathString,
                str2, str3, 0, 0);*/ /* 6-Dec-03 */
          }
        }
      }
      /* 6-Dec-03  This is not really needed but keeps output consistent
         with previous version.  It puts a blank line before the HTML
         "distinct variable" list. */
      if (texFlag && htmlFlag) { /* 6-Dec-03 */
        outputToString = 1;
        print2("\n");
        outputToString = 0;
      }
      /*if (j == 0 && !(htmlFlag && texFlag)) print2("  (None)\n");*/
      if (j == 0 && !texFlag) print2("  (None)\n"); /* 6-Dec-03 fix */
      let(&str1, "");
      nmbrTmpPtr1 = statement[showStatement].reqDisjVarsA;
      nmbrTmpPtr2 = statement[showStatement].reqDisjVarsB;
      i = nmbrLen(nmbrTmpPtr1);
      if (i) {
        for (k = 0; k < i; k++) {
          if (!texFlag) {
            let(&str1, cat(str1, ", <",
                mathToken[nmbrTmpPtr1[k]].tokenName, ",",
                mathToken[nmbrTmpPtr2[k]].tokenName, ">", NULL));
          } else {
            if (htmlFlag && texFlag) {
              let(&str2, "");
              str2 = tokenToTex(mathToken[nmbrTmpPtr1[k]].tokenName);
                   /* tokenToTex allocates str2; we must deallocate it */
              let(&str1, cat(str1, " &nbsp; ", str2, NULL));
              let(&str2, "");
              str2 = tokenToTex(mathToken[nmbrTmpPtr2[k]].tokenName);
              let(&str1, cat(str1, ",", str2, NULL));
            }
          }
        }
        if (!texFlag)
          printLongLine(cat(
              "Its mandatory disjoint variable pairs are:  ",
              right(str1,3),NULL),"  "," ");
      }
      if (type == p__ &&
          nmbrLen(statement[showStatement].optHypList)
          && !texFlag) {
        printLongLine(cat(
           "Its optional hypotheses are:  ",
            nmbrCvtRToVString(
            statement[showStatement].optHypList),NULL),
            "      "," ");
      }
      nmbrTmpPtr1 = statement[showStatement].optDisjVarsA;
      nmbrTmpPtr2 = statement[showStatement].optDisjVarsB;
      i = nmbrLen(nmbrTmpPtr1);
      if (i && type == p__) {
        if (!texFlag) {
          let(&str1, "");
        } else {
          if (htmlFlag && texFlag) {
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
            if (htmlFlag && texFlag) {
                   /* tokenToTex allocates str2; we must deallocate it */
              /*  12/1/01 don't output dummy variables
              let(&str2, "");
              str2 = tokenToTex(mathToken[nmbrTmpPtr1[k]].tokenName);
              let(&str1, cat(str1, " &nbsp; ", str2, NULL));
              let(&str2, "");
              str2 = tokenToTex(mathToken[nmbrTmpPtr2[k]].tokenName);
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
      if (texFlag && htmlFlag && str1[0]) {
        outputToString = 1;
        printLongLine(cat("<CENTER>Substitutions into these variable",
            " pairs may not have variables in common: ",
            str1, "</CENTER>", NULL), "", " ");
        outputToString = 0;
      }
      ***********/


      /* 12/23/01 */
      if (texFlag && htmlFlag && htmlDistinctVars[0]) {
        outputToString = 1;
        printLongLine(cat(
     "<CENTER><A HREF=\"mmset.html#distinct\">Distinct variable</A> group(s): ",
            htmlDistinctVars, "</CENTER>", NULL), "", "\"");
        outputToString = 0;
      }

      if (texFlag) {
        outputToString = 1;
        if (htmlFlag && texFlag) print2("<HR NOSHADE SIZE=1>\n");
        outputToString = 0; /* Restore normal output */
        /* will be done automatically at closing
        fprintf(texFilePtr, "%s", printString);
        let(&printString, "");
        */
        break;
      }
      let(&str1, nmbrCvtMToVString(
          statement[showStatement].reqVarList));
      if (!strlen(str1)) let(&str1, "(None)");
      printLongLine(cat(
          "The statement and its hypotheses require the variables:  ",
          str1, NULL), "      ", " ");
      if (type == p__ &&
          nmbrLen(statement[showStatement].optVarList)) {
        printLongLine(cat(
            "These additional variables are allowed in its proof:  "
            ,nmbrCvtMToVString(
            statement[showStatement].optVarList),NULL),"      ",
            " ");
        /*??? Add variables required by proof */
      }
      /* Note:  statement[].reqVarList is only stored for $a and $p
         statements, not for $e or $f. */
      let(&str1, nmbrCvtMToVString(
          statement[showStatement].reqVarList));
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
  if (htmlFlag && texFlag) {

    /* For syntax declarations, find the first definition that follows
       it.  It is up to the user to arrange the database so that a
       meaningful definition is picked. */
    if (subType == SYNTAX) {
      for (i = showStatement + 1; i <= statements; i++) {
        if (statement[i].type == (char)a__) {
          if (!strcmp("|-", mathToken[
              (statement[i].mathString)[0]].tokenName)) {
            /* It's a definition or axiom */
            /* See if each constant token in the syntax statement
               exists in the definition; if not don't use the definition */
            j = 1;
            /* We start with k=1 for 2nd token (1st is wff, class, etc.) */
            for (k = 1; k < statement[showStatement].mathStringLen; k++) {
              if (mathToken[(statement[showStatement].mathString)[k]].
                  tokenType == (char)con__) {
                if (!nmbrElementIn(1, statement[i].mathString,
                    (statement[showStatement].mathString)[k])) {
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
        if (statement[showStatement].type != (char)a__) bug(231);
        statement[showStatement].type = (char)p__;
        /* Temporarily zap statement with "wff" token in 1st position
           so parseProof will not give errors (in typeProof() call) */
        zapStatement1stToken = (statement[showStatement].mathString)[0];
        (statement[showStatement].mathString)[0] = wffToken;
        if (strcmp("|-", mathToken[zapStatement1stToken].tokenName)) bug(230);

        nmbrTmpPtr1 = NULL_NMBRSTRING;
        nmbrLet(&nmbrTmpPtr1, statement[showStatement].mathString);

        /* Find proof of formula or simple theorem (no new vars in $e's) */
        /* maxEDepth is the maximum depth at which statements with $e
           hypotheses are
           considered.  A value of 0 means none are considered. */
        nmbrTmpPtr2 = proveFloating(nmbrTmpPtr1 /*mString*/,
            showStatement /*statemNum*/, 0 /*maxEDepth*/,
            0 /*step - 0 = step 1 */ /*For messages*/);

        if (nmbrLen(nmbrTmpPtr2)) {
          /* A proof for the step was found. */
          /* Get compact form of proof for shorter display */
          nmbrLet(&nmbrTmpPtr2, nmbrSquishProof(nmbrTmpPtr2));
          /* Temporarily zap proof into statement structure */
          /* (The bug check makes sure there is no proof attached to the
              definition - this would be impossible) */
          if (strcmp(statement[showStatement].proofSectionPtr, "")) bug(231);
          if (statement[showStatement].proofSectionLen != 0) bug(232);
          let(&str1, nmbrCvtRToVString(nmbrTmpPtr2));
          /* Temporarily zap proof into the $a statement */
          statement[showStatement].proofSectionPtr = str1;
          statement[showStatement].proofSectionLen = strlen(str1) - 1;

          /* Display the HTML proof of syntax breakdown */
          typeProof(showStatement,
              0 /*pipFlag*/,
              0 /*startStep*/,
              0 /*endStep*/,
              0 /*startIndent*/, /* Not used */
              0 /*endIndent*/,
              0 /*essentialFlag*/, /* <- also used as def flag in typeProof */
              1 /*renumberFlag*/,
              0 /*unknownFlag*/,
              0 /*notUnifiedFlag*/,
              0 /*reverseFlag*/,
              1 /*noIndentFlag*/,
              0 /*splitColumn*/,
              1 /*texFlag*/,  /* Means either latex or html */
              1 /*htmlFlag*/);

          /* Restore the zapped statement structure */
          statement[showStatement].proofSectionPtr = "";
          statement[showStatement].proofSectionLen = 0;

          /* Deallocate storage */
          let(&str1, "");
          nmbrLet(&nmbrTmpPtr2, NULL_NMBRSTRING);

        } /* if (nmbrLen(nmbrTmpPtr2)) */

        /* Restore the zapped statement structure */
        statement[showStatement].type = (char)a__;
        (statement[showStatement].mathString)[0] = zapStatement1stToken;

        /* Deallocate storage */
        nmbrLet(&nmbrTmpPtr1, NULL_NMBRSTRING);

      } /* if (wffToken >= 0) */

    } /* if (subType == DEFINITION) */


  } /* if (htmlFlag && texFlag) */
  /* End of finding definition for syntax statement */



  /* 10/6/99 - Start of creating used-by list for html page */
  if (htmlFlag && texFlag) {
    /* 10/25/02 Clear out any previous printString accumulation
       for printStringForReferencedBy case below */
    fprintf(texFilePtr, "%s", printString);
    let(&printString, "");
    /* Start outputting to printString */
    if (outputToString != 0) bug(242);
    outputToString = 1;
    if (subType != SYNTAX) { /* Only do this for
        definitions and axioms, not syntax statements */
      let(&str1, "");
      str1 = traceUsage(showStatement, 0 /* recursiveFlag */);
      if (str1[0]) { /* Used by at least one */
        /* str1 will have a leading space before each label */
        /* Convert usage list to html links */
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
        /* Convert str1 to trailing space after each label */
        let(&str1, cat(right(str1, 2), " ", NULL));
        i = 0;
        while (1) {
          j = i + 1;
          i = instr(j, str1, " ");
          if (!i) break;
          /* Extract the label */
          let(&str3, seg(str1, j, i - 1));
          /* Find the statement number */
          m = lookupLabel(str3);
          if (m < 0) {
            /* The lookup should never fail */
            bug(240);
            continue;
          }
          /* It should be a $p */
          if (statement[m].type != p__) bug(241);
          /* Get the pink number */
          let(&str4, "");
          str4 = pinkHTML(m);
          /* Assemble the href */
          let(&str2, cat(str2, " &nbsp;<A HREF=\"",
              str3, ".html\">",
              str3, "</A>", str4, NULL));
        }
        let(&str2, cat(str2, "</FONT></TD></TR>", NULL));
        printLongLine(str2, "", "\"");
      }
    }
    if (subType == THEOREM) {
      /* 10/25/02 The "referenced by" does not show up after the proof
         because we moved the typeProof() to below.  Therefore, we save
         printString into a temporary global holding variable to print
         at the proper place inside of typeProof().  Ugly but necessary
         with present design. */
      let(&printStringForReferencedBy, printString);
      let(&printString, "");
    }

    /* Printing of the trailer in mmwtex.c will close out string later */
    outputToString = 0;
  } /* if (htmlFlag && texFlag) */
  /* 10/6/99 - End of used-by list for html page */


  /* 10/25/02  Moved this to after the block above, so referenced statements
     show up first for convenience */
  if (htmlFlag && texFlag) {
    /*** Output the html proof for $p statements ***/
    /* Note that we also output the axiom and definition usage
       lists inside this function */
    if (statement[showStatement].type == (char)p__) {
      typeProof(showStatement,
          0 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*startIndent*/, /* Not used */
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          1 /*renumberFlag*/,
          0 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          1 /*noIndentFlag*/,
          0 /*splitColumn*/,
          1 /*texFlag*/,  /* Means either latex or html */
          1 /*htmlFlag*/);
    } /* if (statement[showStatement].type == (char)p__) */
  } /* if (htmlFlag && texFlag) */
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
  let(&htmlDistinctVars, "");
} /* typeStatement */

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
  long startIndent, long endIndent,
  flag essentialFlag, /* <- also used as definition/axiom flag for HTML
      syntax breakdown when called from typeStatement() */
  flag renumberFlag,
  flag unknownFlag,
  flag notUnifiedFlag,
  flag reverseFlag,
  flag noIndentFlag, /* Means Lemmon-style proof */
  long splitColumn,
  flag texFlag,
  flag htmlFlag /* htmlFlag added 6/27/99 */
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
    / COLUMN <number> - Overrides the default column at which
        the formula display starts in a Lemmon style display.  May be
        used only in conjuction with / LEMMON.
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
  long i, plen, step, stmt, lens, lent, maxStepNum;
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
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *localLabels = NULL_NMBRSTRING;
  nmbrString *localLabelNames = NULL_NMBRSTRING;
  nmbrString *indentationLevel = NULL_NMBRSTRING;
  nmbrString *targetHyps = NULL_NMBRSTRING;
  nmbrString *essentialFlags = NULL_NMBRSTRING;
  nmbrString *stepRenumber = NULL_NMBRSTRING;
  nmbrString *notUnifiedFlags = NULL_NMBRSTRING;
  nmbrString *unprovedList = NULL_NMBRSTRING; /* For traceProofWork() */
  long maxLabelLen = 0;
  long maxStepNumLen = 1;
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

  if (htmlFlag && texFlag) {
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
    if (wrkProof.errorSeverity > 1) return; /* Display could crash */
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
     in the proof. */
  if (htmlFlag && texFlag && !noIndentFlag /* Lemmon */) {
    /* Only Lemmon-style proofs are implemented for html */
    bug(218);
  }
  if (htmlFlag && texFlag) {
    for (step = 0; step < plen; step++) {
      stmt = proof[step];
      if (stmt < 0) continue;  /* Unknown or label ref */
      type = statement[stmt].type;
      if (type == f__ || type == e__  /* It's a hypothesis */
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
    if (htmlFlag && texFlag && proof[step] < 0) stepPrintFlag = 0;
    /* For standard numbering, stepPrintFlag will be always be 1 here */
    if (stepPrintFlag) {
      i++;
      stepRenumber[step] = i; /* Numbering for step to be printed */
      maxStepNum = i; /* To compute maxStepNumLen below */
    }
  }

  /* Get the printed character length of the largest step number */
  i = maxStepNum;
  while (i >= 10) {
    i = i/10; /* The number is printed in base 10 */
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
      lens = strlen(statement[stmt].labelName);
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
    if (htmlFlag && texFlag) {
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
        if (htmlFlag && texFlag) bug(220); /* If html, a step referencing a
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
          if (!(htmlFlag && texFlag)) { /* No local label declaration is
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
          if (!essentialFlag || statement[hypPtr[hyp]].type == (char)e__) {
            i = stepRenumber[hypStep];
            if (i == 0) {
              if (!(htmlFlag && texFlag)) bug(221);
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
      } else {  /* not noIndentFlag */
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
          printf("?Unsupported:  HTML or LaTeX proof for NEW_PROOF.\n");
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

  if (htmlFlag && texFlag) {
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
      let(&statementUsedFlags, string(statements + 1, 'n')); /* Init. to 'no' */
      for (step = 0; step < plen; step++) {
        stmt = proof[step];
        /* Convention: collect all $a's that don't begin with "|-" */
        if (stmt > 0) {
          if (statement[stmt].type == a__) {
            if (strcmp("|-", mathToken[
                (statement[stmt].mathString)[0]].tokenName)) {
              statementUsedFlags[stmt] = 'y'; /* Flag to use it */
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
            if (statement[statemNum].type != (char)p__) bug(234);
            nmbrTmpPtr1 = NULL_NMBRSTRING;
            nmbrLet(&nmbrTmpPtr1, statement[statemNum].mathString);
          } else {
            /* Ignore $f */
            if (statement[statement[statemNum].reqHypList[i]].type
                == (char)f__) continue;
            /* Must therefore be a $e */
            if (statement[statement[statemNum].reqHypList[i]].type
                != (char)e__) bug(234);
            nmbrTmpPtr1 = NULL_NMBRSTRING;
            nmbrLet(&nmbrTmpPtr1,
                statement[statement[statemNum].reqHypList[i]].mathString);
          }
          if (strcmp("|-", mathToken[nmbrTmpPtr1[0]].tokenName)) bug(235);
          /* Turn "|-" assertion into a "wff" assertion */
          nmbrTmpPtr1[0] = wffToken;

          /* Find proof of formula or simple theorem (no new vars in $e's) */
          /* maxEDepth is the maximum depth at which statements with $e
             hypotheses are
             considered.  A value of 0 means none are considered. */
          nmbrTmpPtr2 = proveFloating(nmbrTmpPtr1 /*mString*/,
              statemNum /*statemNum*/, 0 /*maxEDepth*/,
              0 /* step; 0 = step 1 */ /*For messages*/);
          if (!nmbrLen(nmbrTmpPtr2)) bug(236); /* Didn't find syntax proof */

          /* Add to list of syntax statements used */
          for (step = 0; step < nmbrLen(nmbrTmpPtr2); step++) {
            stmt = nmbrTmpPtr2[step];
            /* Convention: collect all $a's that don't begin with "|-" */
            if (stmt > 0) {
              if (statementUsedFlags[stmt] == 'n') { /* For slight speedup */
                if (statement[stmt].type == a__) {
                  if (strcmp("|-", mathToken[
                      (statement[stmt].mathString)[0]].tokenName)) {
                    statementUsedFlags[stmt] = 'y'; /* Flag to use it */
                  } else {
                    /* In a syntax proof there should be no |- */
                    bug(237);
                  }
                }
              }
            } else {
              /* This is not a compressed proof */
              bug(238);
            }
          }

          /* Deallocate memory */
          nmbrLet(&nmbrTmpPtr2, NULL_NMBRSTRING);
          nmbrLet(&nmbrTmpPtr1, NULL_NMBRSTRING);
        } /* next i */
      } /* if (wffToken >= 0) */
      /* End of section added 2/5/02 */
      /******************************************************************/

      let(&tmpStr, "");
      for (stmt = 1; stmt <= statements; stmt++) {
        if (statementUsedFlags[stmt] == 'y') {
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
                (char)con__) {
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
                    ].tokenName);
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
        printLongLine(printStringForReferencedBy, "", "\"");
        let(&printStringForReferencedBy, "");
      }


      /* Get list of axioms and definitions assumed by proof */
      let(&statementUsedFlags, "");
      traceProofWork(statemNum,
          1 /*essentialFlag*/,
          &statementUsedFlags, /*&statementUsedFlags*/
          &unprovedList /* &unprovedList */);
      if (strlen(statementUsedFlags) != statements + 1) bug(227);

      /* First get axioms */
      let(&tmpStr, "");
      for (stmt = 1; stmt <= statements; stmt++) {
        if (statementUsedFlags[stmt] == 'y' && statement[stmt].type == a__) {
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
        if (statementUsedFlags[stmt] == 'y' && statement[stmt].type == a__) {
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
  nmbrLet(&unprovedList, NULL_NMBRSTRING);
  nmbrLet(&localLabels, NULL_NMBRSTRING);
  nmbrLet(&localLabelNames, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&targetHyps, NULL_NMBRSTRING);
  nmbrLet(&indentationLevel, NULL_NMBRSTRING);
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);
  nmbrLet(&stepRenumber, NULL_NMBRSTRING);
  nmbrLet(&notUnifiedFlags, NULL_NMBRSTRING);
} /* typeProof */

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
      print2("\\vspace{1ex}\n");
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
        if (!htmlFlag) {
          print2("\n");
          print2("\n");
          print2("\\vspace{1ex}\n");
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

      type = statement[stmt].type;
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
                str2, str3, 0, 0);
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
            str2, str3, 0, 0);
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

  long stmt, pos;
  vstring statementUsedFlags = ""; /* y/n flags that statement is used */
  vstring outputString = "";
  nmbrString *unprovedList = NULL_NMBRSTRING;

  if (axiomFlag) {
    print2(
"Statement \"%s\" assumes the following axioms ($a statements):\n",
        statement[statemNum].labelName);
  } else {
    print2(
"The proof of statement \"%s\" uses the following earlier statements:\n",
        statement[statemNum].labelName);
  }

  traceProofWork(statemNum, essentialFlag, &statementUsedFlags,
      &unprovedList);
  if (strlen(statementUsedFlags) != statements + 1) bug(226);

  /* Build the output string */
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
    print2("Warning:  the following traced statement(s) were not proved:\n");
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
  nmbrLet(&unprovedList, NULL_NMBRSTRING);
}

/* Traces back the statements used by a proof, recursively.  Returns
   a nmbrString with a list of statements and unproved statements */
void traceProofWork(long statemNum,
  flag essentialFlag,
  vstring *statementUsedFlagsP, /* 'y'/'n' flag that statement is used */
  nmbrString **unprovedListP)
{

  long pos, stmt, plen, slen, step;
  nmbrString *statementList = NULL_NMBRSTRING;
  nmbrString *proof = NULL_NMBRSTRING;
  nmbrString *essentialFlags = NULL_NMBRSTRING;

  nmbrLet(&statementList, nmbrSpace(statements));
  statementList[0] = statemNum;
  slen = 1;
  nmbrLet(&(*unprovedListP), NULL_NMBRSTRING); /* List of unproved statements */

  let(&(*statementUsedFlagsP), string(statements + 1, 'n')); /* Init. to 'no' */
  for (pos = 0; pos < slen; pos++) {
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
        if (statement[stmt].type != a__ && statement[stmt].type != p__) {
          continue;
        }
      }
      /* Add this statement to the statement list if not already in it */
      if ((*statementUsedFlagsP)[stmt] == 'n') {
        statementList[slen] = stmt;
        slen++;
        (*statementUsedFlagsP)[stmt] = 'y';
      }
    } /* Next step */
  } /* Next pos */

  /* Deallocate */
  nmbrLet(&essentialFlags, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);
  nmbrLet(&statementList, NULL_NMBRSTRING);
  return;

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

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  essentialplen = 0;

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
/* The output string must be deallocated by the user. */
vstring traceUsage(long statemNum,
  flag recursiveFlag) {

  long lastPos, stmt, slen, pos;
  flag tmpFlag;
  vstring statementUsedFlags = ""; /* 'y'/'n' flag that statement is used */
  vstring outputString = "";
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
      /* For temporary unofficial use by NDM to help build command files: */
      /*
      print2("prove %s$min %s/a$sa n/compr$q\n",
          statement[stmt].labelName,statement[statemNum].labelName);
      */
    } /* End if (statementUsedFlag[stmt] == 'y') */
  } /* Next stmt */

  /* Deallocate */
  let(&statementUsedFlags, "");
  nmbrLet(&statementList, NULL_NMBRSTRING);
  nmbrLet(&proof, NULL_NMBRSTRING);

  return (outputString);

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


/* Returns the amount of indentation of a statement label.  Used to
   determine how much to indent a saved proof. */
long getSourceIndentation(long statemNum) {
  char *fbPtr; /* Source buffer pointer */
  char *startLabel;
  long indentation = 0;

  fbPtr = statement[statemNum].mathSectionPtr;
  if (fbPtr[0] == 0) return 0;
  startLabel = statement[statemNum].labelSectionPtr;
  if (startLabel[0] == 0) return 0;
  while (1) { /* Go back to first line feed prior to the label */
    if (fbPtr <= startLabel) break;
    if (fbPtr[0] == '\n') break;
    if (fbPtr[0] == ' ') {
      indentation++; /* Space increments indentation */
    } else {
      indentation = 0; /* Non-space (i.e. a label character) resets back to 0 */
    }
    fbPtr--;
  }
  return indentation;
}


/* This implements the READ command (although the / VERIFY qualifier is
   processed separately in metamath.c). */
void readInput(void)
{

  /* Temporary variables and strings */
  long p;

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

/* This function implements the WRITE SOURCE command. */
void writeInput(flag cleanFlag)
{

  /* Temporary variables and strings */
  long i, p;
  vstring str1 = "";
  vstring str2 = "";
  long skippedCount = 0;

  /* nm 24-Feb-04 I decided we don't really the xxx... placeholders if we
     update the mirrors only once a week or so - because by then we will
     almost certainly have added new theorems (which become new placeholders)
     which defeat the purpose, since everything will get renumbered
     anyway in that case.  The placeholders can make set.mm confusing. */
  /* Comment out the following statement to enable it */
  flag xxxFlag = 0; /* Change to 1 to enable xxx... statement placeholders */

  print2("Creating and writing \"%s\"...\n",output_fn);

  /* Process statements */
  for (i = 1; i <= statements + 1; i++) {

    /* Added 24-Oct-03 nm */
    if (cleanFlag && statement[i].type == (char)p__) {
      /* Clean out any proof-in-progress (that user has flagged with a ? in its
         date comment field) */
      /* Get the comment section after the statement */
      let(&str1, space(statement[i + 1].labelSectionLen));
      memcpy(str1, statement[i + 1].labelSectionPtr,
          statement[i + 1].labelSectionLen);
      /* Make sure it's a date comment */
      let(&str1, edit(str1, 2 + 4)); /* Discard whitespace + control chrs */
      p = instr(1, str1, "]$)"); /* Get end of date comment */
      let(&str1, left(str1, p)); /* Discard stuff after date comment */
      if (instr(1, str1, "$([") == 0) {
        printLongLine(cat(
            "?Warning:  The proof for $p statement \"", statement[i].labelName,
            "\" does not have a date comment after it.  It will not be",
            " removed by the CLEAN qualifier.", NULL), " ", "  ");
      } else {
        /* See if the date comment has a "?" in it */
        if (instr(1, str1, "?")) {
          skippedCount++;
          let(&str2, cat(str2, " ", statement[i].labelName, NULL));
          /* Write at least the date from the _previous_ statement's
             post-comment section so that WRITE RECENT can pick it up. */
          let(&str1, space(statement[i].labelSectionLen));
          memcpy(str1, statement[i].labelSectionPtr,
              statement[i].labelSectionLen);
          /* nm 19-Jan-04 Don't discard w.s. because string will be returned to
             the source file */
          /*let(&str1, edit(str1, 2 + 4));*/ /* Discard whitespace + ctrl chrs */
          p = instr(1, str1, "]$)"); /* Get end of date comment */
          if (p == 0) {
            /* In the future, "] $)" may flag date end to conform with
               space-around-keyword Metamath spec recommendation */
            p = instr(1, str1, "] $)"); /* Get end of date comment */
            if (p != 0) p++; /* Match what it would be for old standard */
          }
          if (p != 0) {  /* The previous statement has a date comment */
            let(&str1, left(str1, p + 2)); /* Discard stuff after date cmnt */
            if (xxxFlag || !instr(1, str1, "?"))
              /* Don't bother to print $([?]$) of previous statement because
                 the previous statement was skipped */
              fprintf(output_fp, "%s\n", str1);
                                             /* Output just the date comment */
          }

          /* Output a dummy xxx... statement below if xxxFlag */
          /* Otherwise output nothing (i.e. suppress the statement) */
          if (xxxFlag) {
            /* nm 19-Jan-04 - output a dummy statement for use as a placeholder
               so the numbers won't all shift by one when the real theorem is
               finalized, to make incremental rsync site updates smaller */
            p = getSourceIndentation(i);
            /* ???Note:  the code below is custom for set.mm and won't work
               with another formal system.  This is not a problem now because
               currently this is only used with set.mm, but it may be a
               problem in the future. */
            let(&str1, cat("\n", space(p),
                "$( Dummy placeholder for a possible future theorem. $)",
                "\n",
                /*space(p), "xxx", str(skippedCount),*/
                space(p), "xxx", str(statement[i].pinkNumber),
                " $p |- ( ph -> ph ) $=",
                "\n",
                space(p + 2), "wph xxxid $.", NULL));
            fprintf(output_fp, "%s", str1);
            /* nm 19-Jan-04 - end of outputting dummy statement */
          } /* if (xxxFlag) */

          /* Skip the normal output of this statement */
          continue;
        } /* if (instr(1, str1, "?")) */
      } /* (else clause) if (instr(1, str1, "$([") == 0) */
    } /* if cleanFlag */

    let(&str1,""); /* Deallocate vstring */
    str1 = outputStatement(i);
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
          " deleted by the CLEAN qualifier: ", str2, ".",
          "  You should ERASE, READ the new output file, and run VERIFY PROOF",
          " to ensure that no proof dependencies were broken.",
          NULL), " ", "  ");
    }
  }

  let(&str1,""); /* Deallocate vstring */
  let(&str2,""); /* Deallocate vstring */
}

void writeDict(void)
{
  print2("This function has not been implemented yet.\n");
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
  vstring header = "";
  flag errorFound;

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

  errorFound = 0;
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
      print2("All proofs in the database were verified.\n");
    } else {
      print2("All proofs in the database passed the syntax-only check.\n");
    }
  }
  let(&emptyProofList, ""); /* Deallocate */

}



/* Called by help() - prints a help line */
/* THINK C gives compilation error if H() is lower-case h() -- why? */
void H(vstring helpLine)
{
  if (printHelp) {
    print2("%s\n", helpLine);
  }
}



/******** 8/28/00 ***********************************************************/
/******** The MIDI output algorithm is in this function, outputMidi(). ******/
/*** Warning:  If you want to experiment with the MIDI output, please
     confine changes to this function.  Changes to other code
     in this file is not recommended. ***/

void outputMidi(long plen, nmbrString *indentationLevels,
  nmbrString *logicalFlags, vstring midiParam, vstring statementLabel) {

  /* plen = length of proof
     indentationLevels[step] = indentation level in "show proof xxx /full"
         where step varies from 0 to plen-1
     logicalFlags[step] = 0 for formula-building step, 1 for logical step
     midiParam = string passed by user in "midi xxx /parameter <midiParam>"
     statementLabel = label of statement whose proof is being scanned */

  /* This function is called when the user types "midi xxx /parameter
     <midiParam>".  The proof steps of theorem xxx are numbered successively
     from 0 to plen-1.  The arrays indentationLevels[] and logicalFlags[]
     have already been populated for you.  */

  /* The purpose of this function is to create an ASCII file called xxx.txt
     that contains the ASCII format for a t2mf input file.  The mf2t package
     is available at http://www.hitsquad.com/smm/programs/mf2t/download.shtml.
     To convert xxx.txt to xxx.mid you type "t2mf sbth.txt sbth.mid". */

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
#define MIN_NOTE 28
#define MAX_NOTE 103

  /* Note: "flag" is just "char"; "vstring" is just "char *" - except
     that by this program's convention vstring allocation is controlled
     by string functions in mmvstr.c; and "nmbrString" is just "long *" */

  long step; /* Proof step from 0 to plen-1 */
  long midiNote; /* Current midi note to be output */
  long midiTime; /* Midi time stamp */
  long midiPreviousFormulaStep; /* Note saved from previous step */
  long midiPreviousLogicalStep; /* Note saved from previous step */
  vstring midiFileName = ""; /* All vstrings MUST be initialized to ""! */
  FILE *midiFilePtr; /* Output file pointer */
  long midiBaseline; /* Baseline note */
  long midiMaxIndent; /* Maximum indentation (to find dyn range of notes) */
  long midiMinNote; /* Smallest note in output */
  long midiMaxNote; /* Largest note in output */
  long midiNoteInc; /* Note increment per proof indentation level */
  flag midiSyncopate; /* 1 = syncopate the output */
  flag midiHesitate; /* 1 = silence all repeated notes */
  long midiTempo; /* larger = faster */
  vstring midiLocalParam = ""; /* To manipulate user's parameter string */
  vstring tmpStr = ""; /* Temporary string */

  /************* Initialization ***************/

  midiTime = 0; /* MIDI time stamp */
  midiPreviousFormulaStep = 0; /* Note in previous formula-building step */
  midiPreviousLogicalStep = 0; /* Note in previous logical step */
  midiFilePtr = NULL; /* Output file pointer */

  /* Parse the parameter string passed by the user */
  let(&midiLocalParam, edit(midiParam, 32)); /* Convert to uppercase */

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
  /* Set the tempo: 96=fast, 48=slow */
  if (strchr(midiLocalParam, 'F') != NULL) {
    midiTempo = 2 * TEMPO;
  } else {
    midiTempo = TEMPO;
  }
  /* End of parsing user's parameter string */

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

  midiNoteInc = 5; /* Starting note increment, plus 1 */
  do { /* Decrement note incr until song will fit MIDI dyn range */
    midiNoteInc--;
    /* Compute the baseline note to which add the proof indentation
      times the midiNoteInc.  The "12" is to allow for the shift
      of one octave down of the sustained notes on "essential"
      (i.e. logical, not formula-building) steps. */
    midiBaseline = ((MAX_NOTE + MIN_NOTE) / 2) -
      (((midiMaxIndent * midiNoteInc) - 12) / 2);
    midiMinNote = midiBaseline - 12;
    midiMaxNote = midiBaseline + (midiMaxIndent * midiNoteInc);
  } while ((midiMinNote < MIN_NOTE || midiMaxNote > MAX_NOTE) &&
      midiNoteInc > 0);

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
  let(&tmpStr, cat("Theorem ", statementLabel, " ", midiParam,
      space(30), NULL));
  let(&tmpStr, left(tmpStr, 38));
  fprintf(midiFilePtr, "0 Meta Text \"%s\"\n", tmpStr);
  fprintf(midiFilePtr,
      "0 Meta Copyright \"Copyright (C) 2002 nm at alum.mit.edu \"\n");

  /************** Scan the proof ************************/

  for (step = 0; step < plen; step++) {
  /* Handle the processing that takes place at each proof step */
    if (!logicalFlags[step]) {

      /*** Process the higher fast notes for formula-building steps ***/
      midiNote = (midiNoteInc * indentationLevels[step]) + midiBaseline;
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
      midiNote = (midiNoteInc * indentationLevels[step])
          + midiBaseline;
      midiNote = midiNote - 12; /* Down 1 octave */
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

} /* outputMidi() */
