/*****************************************************************************/
/*        Copyright (C) 2019  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* This module processes LaTeX and HTML output. */

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "mmutil.h"
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h" /* For rawSourceError and mathSrchCmp and lookupLabel */
#include "mmwtex.h"
#include "mmcmdl.h" /* For texFileName */

/* 6/27/99 - Now, all LaTeX and HTML definitions are taken from the source
   file (read in the by READ... command).  In the source file, there should
   be a single comment $( ... $) containing the keyword $t.  The definitions
   start after the $t and end at the $).  Between $t and $), the definition
   source should exist.  See the file set.mm for an example. */

flag oldTexFlag = 0; /* Use TeX macros in output (obsolete) */

flag htmlFlag = 0;  /* HTML flag: 0 = TeX, 1 = HTML */
flag altHtmlFlag = 0;  /* Use "althtmldef" instead of "htmldef".  This is
    intended to allow the generation of pages with the Unicode font
    instead of the individual GIF files. */
flag briefHtmlFlag = 0;  /* Output statement lists only, for statement display
                in other HTML pages, such as the Proof Explorer home page */
long extHtmlStmt = 0; /* At this statement and above, use the exthtmlxxx
    variables for title, links, etc.  This was put in to allow proper
    generation of the Hilbert Space Explorer extension to the set.mm
    database. */

/* 29-Jul-2008 nm Sandbox stuff */
long sandboxStmt = 0; /* At this statement and above, use SANDBOX_COLOR
    background for theorem, mmrecent, & mmbiblio lists */
    /* 0 means it hasn't been looked up yet; statements + 1 means
       there is no mathbox */

/* This is the list of characters causing the space before the opening "`"
   in a math string in a comment to be removed for HTML output. */
#define OPENING_PUNCTUATION "(['\""
/* This is the list of characters causing the space after the closing "`"
   in a math string in a comment to be removed for HTML output. */
#define CLOSING_PUNCTUATION ".,;)?!:]'\"_-"

/* Tex output file */
FILE *texFilePtr = NULL;
flag texFileOpenFlag = 0;

/* Tex dictionary */
FILE *tex_dict_fp;
vstring tex_dict_fn = "";

/* Global variables */
flag texDefsRead = 0;
struct texDef_struct *texDefs; /* 27-Oct-2012 nm Made global for "erase" */

/* Variables local to this module (except some $t variables) */
long numSymbs;
#define DOLLAR_SUBST 2
/*char dollarSubst = 2;*/
    /* Substitute character for $ in converting to obsolete $l,$m,$n
       comments - Use '$' instead of non-printable ASCII 2 for debugging */

/* Variables set by the language in the set.mm etc. $t statement */
/* Some of these are global; see mmwtex.h */
vstring htmlCSS = ""; /* Set by htmlcss commands */  /* 14-Jan-2016 nm */
vstring htmlFont = ""; /* Set by htmlfont commands */  /* 14-Jan-2016 nm */
vstring htmlVarColor = ""; /* Set by htmlvarcolor commands */
vstring htmlTitle = ""; /* Set by htmltitle command */
  /* 16-Aug-2016 nm */
  vstring htmlTitleAbbr = ""; /* Extracted from htmlTitle */
vstring htmlHome = ""; /* Set by htmlhome command */
  /* 16-Aug-2016 nm */
  /* Future - assign these in the $t set.mm comment instead of htmlHome */
  vstring htmlHomeHREF = ""; /* Extracted from htmlHome */
  vstring htmlHomeIMG = ""; /* Extracted from htmlHome */
vstring htmlBibliography = ""; /* Optional; set by htmlbibliography command */
vstring extHtmlLabel = ""; /* Set by exthtmllabel command - where extHtml starts */
vstring extHtmlTitle = ""; /* Set by exthtmltitle command (global!) */
  vstring extHtmlTitleAbbr = ""; /* Extracted from htmlTitle */
vstring extHtmlHome = ""; /* Set by exthtmlhome command */
  /* 16-Aug-2016 nm */
  /* Future - assign these in the $t set.mm comment instead of htmlHome */
  vstring extHtmlHomeHREF = ""; /* Extracted from extHtmlHome */
  vstring extHtmlHomeIMG = ""; /* Extracted from extHtmlHome */
vstring extHtmlBibliography = ""; /* Optional; set by exthtmlbibliography command */
vstring htmlDir = ""; /* Directory for GIF version, set by htmldir command */
vstring altHtmlDir = ""; /* Directory for Unicode Font version, set by
                            althtmldir command */

/* 29-Jul-2008 nm Sandbox stuff */
vstring sandboxHome = "";
  vstring sandboxHomeHREF = ""; /* Extracted from extHtmlHome */
  vstring sandboxHomeIMG = ""; /* Extracted from extHtmlHome */
vstring sandboxTitle = "";
  /* 16-Aug-2016 nm */
  vstring sandboxTitleAbbr = "";

/* Variables holding all HTML <a name..> tags from bibiography pages  */
vstring htmlBibliographyTags = "";
vstring extHtmlBibliographyTags = "";


/* 17-Nov-2015 nm Added */
void eraseTexDefs(void) {
  /* Deallocate the texdef/htmldef storage */ /* Added 27-Oct-2012 nm */
  long i;
  if (texDefsRead) {  /* If not (already deallocated or never allocated) */
    texDefsRead = 0;

    for (i = 0; i < numSymbs; i++) {  /* Deallocate structure member i */
      let(&(texDefs[i].tokenName), "");
      let(&(texDefs[i].texEquiv), "");
    }
    free(texDefs); /* Deallocate the structure */
  }
  return;
} /* eraseTexDefs */


/* Returns 2 if there were severe parsing errors, 1 if there were warnings but
   no errors, 0 if no errors or warnings */
flag readTexDefs(
  flag errorsOnly,  /* 1 = suppress non-error messages */
  flag noGifCheck   /* 1 = don't check for missing GIFs */)
{

  char *fileBuf;
  char *startPtr;
  long lineNumOffset = 0;
  char *fbPtr;
  char *tmpPtr;
  char *tmpPtr2;
  long charCount;
  long i, j, k, p;
  long lineNum;
  long tokenLength;
  char zapChar;
  long cmd;
  long parsePass;
  vstring token = "";
  vstring partialToken = ""; /* 6-Aug-2011 nm */
  FILE *tmpFp;
  static flag saveHtmlFlag = -1; /* -1 to force 1st read */  /* 17-Nov-2015 nm */
  static flag saveAltHtmlFlag = 1; /* -1 to force 1st read */ /* 17-Nov-2015 nm */
  flag warningFound = 0; /* 1 if a warning was found */
  char *dollarTStmtPtr = NULL; /* Pointer to label section of statement with
                              the $t comment */ /* 2-Feb-2018 nm */

  /* bsearch returned values for use in error-checking */
  void *mathKeyPtr; /* bsearch returned value for math symbol lookup */
  void *texDefsPtr; /* For binary search */

  /* 17-Nov-2015 nm */
  /* if (texDefsRead) { */
  if (saveHtmlFlag != htmlFlag || saveAltHtmlFlag != altHtmlFlag
      || !texDefsRead) {   /* 3-Jan-2016 nm */
    /* One or both changed - we need to erase and re-read */
    eraseTexDefs();
    saveHtmlFlag = htmlFlag; /* Save for next call to readTexDefs() */
    saveAltHtmlFlag = altHtmlFlag;  /* Save for next call to readTexDefs() */
    if (htmlFlag == 0 /* Tex */ && altHtmlFlag == 1) {
      bug(2301); /* Nonsensical combination */
    }
  } else {
    /* Nothing changed; don't need to read again */
    return 0; /* No errors */
  }
  /* } */

  /* Initial values below will be overridden if a user assignment exists in the
     $t comment of the xxx.mm input file */
  let(&htmlTitle, "Metamath Test Page"); /* Set by htmltitle command in $t
                                                 comment */
  /* let(&htmlHome, "<A HREF=\"http://metamath.org\">Home</A>"); */
  /* 16-Aug-2016 nm */
  let(&htmlHome, cat("<A HREF=\"mmset.html\"><FONT SIZE=-2 FACE=sans-serif>",
    "<IMG SRC=\"mm.gif\" BORDER=0 ALT=",
    "\"Home\" HEIGHT=32 WIDTH=32 ALIGN=MIDDLE STYLE=\"margin-bottom:0px\">",
    "Home</FONT></A>", NULL));
                                     /* Set by htmlhome command in $t comment */

  if (errorsOnly == 0) {
    print2("Reading definitions from $t statement of %s...\n", input_fn);
  }
  /******* 10/14/02 Rewrote this section so xxx.mm is not read again *******/
  /* Find the comment with the $t */
  fileBuf = ""; /* This used to point to the input file buffer of an external
                   latex.def file; now it's from the xxx.mm $t comment, so we
                   make it a normal string */
  let(&fileBuf, "");
  /* Note that statement[statements + 1] is a special (empty) statement whose
     labelSection holds any comment after the last statement. */
  for (i = 1; i <= statements + 1; i++) {
    /* We do low-level stuff on the xxx.mm input file buffer for speed */
    tmpPtr = statement[i].labelSectionPtr;
    j = statement[i].labelSectionLen;
    /* Note that for statement[statements + 1], the lineNum is one plus the
       number of lines in the file */
    if (!fileBuf[0]) lineNumOffset = statement[i].lineNum; /* Save for later */
        /* (Don't save if we're in scan to end for double $t detection) */
    zapChar = tmpPtr[j]; /* Save the original character */
    tmpPtr[j] = 0; /* Create an end-of-string */
    if (instr(1, tmpPtr, "$t")) {
      /* Found a $t comment */
      /* Make sure this isn't a second one in another statement */
      /* (A second one in the labelSection of one statement will trigger
         an error below.) */
      if (fileBuf[0]) {
        print2(
  "?Error: There are two comments containing a $t keyword in \"%s\".\n",
            input_fn);
        let(&fileBuf, "");
        return 2;
      }
      let(&fileBuf, tmpPtr);
      tmpPtr[j] = zapChar;
      dollarTStmtPtr = statement[i].labelSectionPtr; /* 2-Feb-2018 nm */
      /* break; */ /* Continue to end to detect double $t */
    }
    tmpPtr[j] = zapChar; /* Restore the xxx.mm input file buffer */
  } /* next i */
  /* If the $t wasn't found, fileBuf will be "", causing error message below. */
  /* Compute line number offset of beginning of statement[i].labelSection for
     use in error messages */
  j = (long)strlen(fileBuf);
  for (i = 0; i < j; i++) {
    if (fileBuf[i] == '\n') lineNumOffset--;
  }
  /******* End of 10/14/02 rewrite *******/


#define LATEXDEF 1
#define HTMLDEF 2
#define HTMLVARCOLOR 3
#define HTMLTITLE 4
#define HTMLHOME 5
/* Added 12/27/01 */
#define ALTHTMLDEF 6
#define EXTHTMLTITLE 7
#define EXTHTMLHOME 8
#define EXTHTMLLABEL 9
#define HTMLDIR 10
#define ALTHTMLDIR 11
/* Added 10/9/02 */
#define HTMLBIBLIOGRAPHY 12
#define EXTHTMLBIBLIOGRAPHY 13
/* Added 14-Jan-2016 nm */
#define HTMLCSS 14
#define HTMLFONT 15

  startPtr = fileBuf;

  /* 6/27/99 Find $t command */
  while (1) {
    if (startPtr[0] == '$') {
      if (startPtr[1] == 't') {
        startPtr++;
        break;
      }
    }
    if (startPtr[0] == 0) break;
    startPtr++;
  }
  if (startPtr[0] == 0) {
    print2("?Error: There is no $t command in the file \"%s\".\n", input_fn);
    print2(
"The file should have exactly one comment of the form $(...$t...$) with\n");
    print2("the LaTeX and HTML definitions between $t and $).\n");
    let(&fileBuf, "");  /* was: free(fileBuf); */
    return 2;
  }
  startPtr++; /* Move to 1st char after $t */

  /* 6/27/99 Search for the ending $) and zap the $) to be end-of-string */
  tmpPtr = startPtr;
  while (1) {
    if (tmpPtr[0] == '$') {
      if (tmpPtr[1] == ')') {
        break;
      }
    }
    if (tmpPtr[0] == 0) break;
    tmpPtr++;
  }
  if (tmpPtr[0] == 0) {
    print2(
  "?Error: There is no $) comment closure after the $t keyword in \"%s\".\n",
        input_fn);
    let(&fileBuf, "");  /* was: free(fileBuf); */
    return 2;
  }

  /* 10/9/02 Make sure there aren't two comments with $t commands */
  tmpPtr2 = tmpPtr;
  while (1) {
    if (tmpPtr2[0] == '$') {
      if (tmpPtr2[1] == 't') {
        print2(
  "?Error: There are two comments containing a $t keyword in \"%s\".\n",
            input_fn);
        let(&fileBuf, "");  /* was: free(fileBuf); */
        return 2;
      }
    }
    if (tmpPtr2[0] == 0) break;
    tmpPtr2++;
  }

   /* Force end of string at the $ in $) */
  tmpPtr[0] = '\n';
  tmpPtr[1] = 0;

  charCount = tmpPtr + 1 - fileBuf; /* For bugcheck */

  for (parsePass = 1; parsePass <= 2; parsePass++) {
    /* Pass 1 - Count the number of symbols defined and alloc texDefs array */
    /* Pass 2 - Assign the texDefs array */
    numSymbs = 0;
    fbPtr = startPtr;

    while (1) {

      /* Get next token */
      fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
      tokenLength = texDefTokenLen(fbPtr);

      /* Process token - command */
      if (!tokenLength) break; /* End of file */
      zapChar = fbPtr[tokenLength]; /* Char to restore after zapping source */
      fbPtr[tokenLength] = 0; /* Create end of string */
      cmd = lookup(fbPtr,
          "latexdef,htmldef,htmlvarcolor,htmltitle,htmlhome"
          ",althtmldef,exthtmltitle,exthtmlhome,exthtmllabel,htmldir"
          ",althtmldir,htmlbibliography,exthtmlbibliography"
          ",htmlcss,htmlfont"   /* 14-Jan-2016 nm */
          );
      fbPtr[tokenLength] = zapChar;
      if (cmd == 0) {
        lineNum = lineNumOffset;
        for (i = 0; i < (fbPtr - fileBuf); i++) {
          if (fileBuf[i] == '\n') lineNum++;
        }
        rawSourceError(/*fileBuf*/sourcePtr,  /* 2-Feb-2018 nm */
            /*fbPtr*/dollarTStmtPtr + (fbPtr - fileBuf), tokenLength,
            /*lineNum, input_fn,*/   /* 2-Feb-2018 nm These arguments are now
                computed in rawSourceError() */
            cat("Expected \"latexdef\", \"htmldef\", \"htmlvarcolor\",",
            " \"htmltitle\", \"htmlhome\", \"althtmldef\",",
            " \"exthtmltitle\", \"exthtmlhome\", \"exthtmllabel\",",
            " \"htmldir\", \"althtmldir\",",
            " \"htmlbibliography\", \"exthtmlbibliography\",",
            " \"htmlcss\", or \"htmlfont\" here.",     /* 14-Jan-2016 nm */
            NULL));
        let(&fileBuf, "");  /* was: free(fileBuf); */
        return 2;
      }
      fbPtr = fbPtr + tokenLength;

      if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME
          && cmd != EXTHTMLTITLE && cmd != EXTHTMLHOME && cmd != EXTHTMLLABEL
          && cmd != HTMLDIR && cmd != ALTHTMLDIR
          && cmd != HTMLBIBLIOGRAPHY && cmd != EXTHTMLBIBLIOGRAPHY
          && cmd != HTMLCSS /* 14-Jan-2016 nm */
          && cmd != HTMLFONT /* 14-Jan-2016 nm */
          ) {
         /* Get next token - string in quotes */
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLength = texDefTokenLen(fbPtr);

        /* Process token - string in quotes */
        if (fbPtr[0] != '\"' && fbPtr[0] != '\'') {
          if (!tokenLength) { /* Abnormal end-of-file */
            fbPtr--; /* Format for error message */
            tokenLength++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(/*fileBuf*/sourcePtr,  /* 2-Feb-2018 nm */
            /*fbPtr*/dollarTStmtPtr + (fbPtr - fileBuf),
              tokenLength, /*lineNum, input_fn,*/
              "Expected a quoted string here.");
          let(&fileBuf, "");  /* was: free(fileBuf); */
          return 2;
        }
        if (parsePass == 2) {
          zapChar = fbPtr[tokenLength - 1]; /* Chr to restore after zapping src */
          fbPtr[tokenLength - 1] = 0; /* Create end of string */
          let(&token, fbPtr + 1); /* Get ASCII token; note that leading and
              trailing quotes are omitted. */
          fbPtr[tokenLength - 1] = zapChar;

          /* Change double internal quotes to single quotes */
          /* 6-Aug-2011 nm Do this only for double quotes matching the
             outer quotes.  fbPtr[0] is the quote character. */
          if (fbPtr[0] != '\"' && fbPtr[0] != '\'') bug(2329);
          j = (long)strlen(token);
          for (i = 0; i < j - 1; i++) {
            if (token[i] == fbPtr[0] &&
                token[i + 1] == fbPtr[0]) {
              let(&token, cat(left(token,
                  i + 1), right(token, i + 3), NULL));
              j--;
            }
          }

          if ((cmd == LATEXDEF && !htmlFlag)
              || (cmd == HTMLDEF && htmlFlag && !altHtmlFlag)
              || (cmd == ALTHTMLDEF && htmlFlag && altHtmlFlag)) {
            texDefs[numSymbs].tokenName = "";
            let(&(texDefs[numSymbs].tokenName), token);
          }
        } /* if (parsePass == 2) */

        fbPtr = fbPtr + tokenLength;
      } /* if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME...) */

      if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME
          && cmd != EXTHTMLTITLE && cmd != EXTHTMLHOME && cmd != EXTHTMLLABEL
          && cmd != HTMLDIR && cmd != ALTHTMLDIR
          && cmd != HTMLBIBLIOGRAPHY && cmd != EXTHTMLBIBLIOGRAPHY
          && cmd != HTMLCSS /* 14-Jan-2016 nm */
          && cmd != HTMLFONT /* 14-Jan-2016 nm */
          ) {
        /* Get next token -- "as" */
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLength = texDefTokenLen(fbPtr);
        zapChar = fbPtr[tokenLength]; /* Char to restore after zapping source */
        fbPtr[tokenLength] = 0; /* Create end of string */
        if (strcmp(fbPtr, "as")) {
          if (!tokenLength) { /* Abnormal end-of-file */
            fbPtr--; /* Format for error message */
            tokenLength++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(/*fileBuf*/sourcePtr,  /* 2-Feb-2018 nm */
            /*fbPtr*/dollarTStmtPtr + (fbPtr - fileBuf),
              tokenLength, /*lineNum, input_fn,*/
              "Expected the keyword \"as\" here.");
          let(&fileBuf, "");  /* was: free(fileBuf); */
          return 2;
        }
        fbPtr[tokenLength] = zapChar;
        fbPtr = fbPtr + tokenLength;
      } /* if (cmd != HTMLVARCOLOR && ... */

      if (parsePass == 2) {
        /* Initialize LaTeX/HTML equivalent */
        let(&token, "");
      }

      /* Scan   "<string>" + "<string>" + ...   until ";" found */
      while (1) {

        /* Get next token - string in quotes */
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLength = texDefTokenLen(fbPtr);
        if (fbPtr[0] != '\"' && fbPtr[0] != '\'') {
          if (!tokenLength) { /* Abnormal end-of-file */
            fbPtr--; /* Format for error message */
            tokenLength++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(/*fileBuf*/sourcePtr,  /* 2-Feb-2018 nm */
            /*fbPtr*/dollarTStmtPtr + (fbPtr - fileBuf),
              tokenLength, /*lineNum, input_fn,*/
              "Expected a quoted string here.");
          let(&fileBuf, "");  /* was: free(fileBuf); */
          return 2;
        }
        if (parsePass == 2) {
          zapChar = fbPtr[tokenLength - 1]; /* Chr to restore after zapping src */
          fbPtr[tokenLength - 1] = 0; /* Create end of string */
          /* let(&token, cat(token, fbPtr + 1, NULL)); old before 6-Aug-2011 */
           /* 6-Aug-2011 nm */
          let(&partialToken, fbPtr + 1); /* Get ASCII token; note that leading
              and trailing quotes are omitted. */
          fbPtr[tokenLength - 1] = zapChar;

          /* 6-Aug-2011 nm */
          /* Change double internal quotes to single quotes */
          /* Do this only for double quotes matching the
             outer quotes.  fbPtr[0] is the quote character. */
          if (fbPtr[0] != '\"' && fbPtr[0] != '\'') bug(2330);
          j = (long)strlen(partialToken);
          for (i = 0; i < j - 1; i++) {
            if (token[i] == fbPtr[0] &&
                token[i + 1] == fbPtr[0]) {
              let(&partialToken, cat(left(partialToken,
                  i + 1), right(token, i + 3), NULL));
              j--;
            }
          }

          /* 6-Aug-2011 nm */
          /* Check that string is on a single line */
          tmpPtr2 = strchr(partialToken, '\n');
          if (tmpPtr2 != NULL) {

            /* 26-Dec-2011 nm - added to initialize lineNum */
            lineNum = lineNumOffset;
            for (i = 0; i < (fbPtr - fileBuf); i++) {
              if (fileBuf[i] == '\n') lineNum++;
            }

            rawSourceError(/*fileBuf*/sourcePtr,  /* 2-Feb-2018 nm */
                /*fbPtr*/dollarTStmtPtr + (fbPtr - fileBuf),
                tmpPtr2 - partialToken + 1 /*tokenLength on current line*/,
                /*lineNum, input_fn,*/
                "String should be on a single line.");
          }

          /* 6-Aug-2011 nm */
          /* Combine the string part to the main token we're building */
          let(&token, cat(token, partialToken, NULL));

        } /* (parsePass == 2) */

        fbPtr = fbPtr + tokenLength;


        /* Get next token - "+" or ";" */
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLength = texDefTokenLen(fbPtr);
        if ((fbPtr[0] != '+' && fbPtr[0] != ';') || tokenLength != 1) {
          if (!tokenLength) { /* Abnormal end-of-file */
            fbPtr--; /* Format for error message */
            tokenLength++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') {
              lineNum++;
            }
          }
          rawSourceError(/*fileBuf*/sourcePtr,  /* 2-Feb-2018 nm */
            /*fbPtr*/dollarTStmtPtr + (fbPtr - fileBuf),
              tokenLength, /*lineNum, input_fn,*/
              "Expected \"+\" or \";\" here.");
          let(&fileBuf, "");  /* was: free(fileBuf); */
         return 2;
        }
        fbPtr = fbPtr + tokenLength;

        if (fbPtr[-1] == ';') break;

      } /* End while */


      if (parsePass == 2) {

        /* 6-Aug-2011 nm This was moved above (and modified) because
           each string part may have different outer quotes, so we can't
           do the whole string at once like the attempt below. */
        /* old before 6-Aug-2011 */
        /* Change double internal quotes to single quotes */
        /*
        j = (long)strlen(token);
        for (i = 0; i < j - 1; i++) {
          if ((token[i] == '\"' &&
              token[i + 1] == '\"') ||
              (token[i] == '\'' &&
              token[i + 1] == '\'')) {
            let(&token, cat(left(token,
                i + 1), right(token, i + 3), NULL));
            j--;
          }
        }
        */
        /* end of old before 6-Aug-2011 */

        if ((cmd == LATEXDEF && !htmlFlag)
            || (cmd == HTMLDEF && htmlFlag && !altHtmlFlag)
            || (cmd == ALTHTMLDEF && htmlFlag && altHtmlFlag)) {
          texDefs[numSymbs].texEquiv = "";
          let(&(texDefs[numSymbs].texEquiv), token);
        }
        if (cmd == HTMLVARCOLOR) {
          let(&htmlVarColor, cat(htmlVarColor, " ", token, NULL));
        }
        if (cmd == HTMLTITLE) {
          let(&htmlTitle, token);
        }
        if (cmd == HTMLHOME) {
          let(&htmlHome, token);
        }
        if (cmd == EXTHTMLTITLE) {
          let(&extHtmlTitle, token);
        }
        if (cmd == EXTHTMLHOME) {
          let(&extHtmlHome, token);
        }
        if (cmd == EXTHTMLLABEL) {
          let(&extHtmlLabel, token);
        }
        if (cmd == HTMLDIR) {
          let(&htmlDir, token);
        }
        if (cmd == ALTHTMLDIR) {
          let(&altHtmlDir, token);
        }
        if (cmd == HTMLBIBLIOGRAPHY) {
          let(&htmlBibliography, token);
        }
        if (cmd == EXTHTMLBIBLIOGRAPHY) {
          let(&extHtmlBibliography, token);
        }
        if (cmd == HTMLCSS) {
          let(&htmlCSS, token);
          /* 14-Jan-2016 nm User's CSS */ /* 13-Dec-2018 nm Moved up to here */
          /* Convert characters "\n" to new line - maybe do for other fields too? */
          do {
            p = instr(1, htmlCSS, "\\n");
            if (p != 0) {
              let(&htmlCSS, cat(left(htmlCSS, p - 1), "\n",
                  right(htmlCSS, p + 2), NULL));
            }
          } while (p != 0);
        }
        if (cmd == HTMLFONT) {
          let(&htmlFont, token);
        }
      }

      if ((cmd == LATEXDEF && !htmlFlag)
          || (cmd == HTMLDEF && htmlFlag && !altHtmlFlag)
          || (cmd == ALTHTMLDEF && htmlFlag && altHtmlFlag)) {
        numSymbs++;
      }

    } /* End while */

    if (fbPtr != fileBuf + charCount) bug(2305);

    if (parsePass == 1 ) {
      if (errorsOnly == 0) {
        print2("%ld typesetting statements were read from \"%s\".\n",
            numSymbs, input_fn);
      }
      texDefs = malloc((size_t)numSymbs * sizeof(struct texDef_struct));
      if (!texDefs) outOfMemory("#99 (TeX symbols)");
    }

  } /* next parsePass */


  /* Sort the tokens for later lookup */
  qsort(texDefs, (size_t)numSymbs, sizeof(struct texDef_struct), texSortCmp);

  /* Check for duplicate definitions */
  for (i = 1; i < numSymbs; i++) {
    if (!strcmp(texDefs[i].tokenName, texDefs[i - 1].tokenName)) {
      printLongLine(cat("?Warning:  Token ", texDefs[i].tokenName,
          " is defined more than once in ",
          htmlFlag ? "an htmldef" : "a latexdef", " statement.", NULL),
          "", " ");
      warningFound = 1;
    }
  }

  /* Check to make sure all definitions are for a real math token */
  for (i = 0; i < numSymbs; i++) {
    /* Note:  mathKey, mathTokens, and mathSrchCmp are assigned or defined
       in mmpars.c. */
    mathKeyPtr = (void *)bsearch(texDefs[i].tokenName, mathKey,
        (size_t)mathTokens, sizeof(long), mathSrchCmp);
    if (!mathKeyPtr) {
      printLongLine(cat("?Warning:  The token \"", texDefs[i].tokenName,
          "\", which was defined in ", htmlFlag ? "an htmldef" : "a latexdef",
          " statement, was not declared in any $v or $c statement.", NULL),
          "", " ");
      warningFound = 1;
    }
  }

  /* Check to make sure all math tokens have typesetting definitions */
  for (i = 0; i < mathTokens; i++) {
    texDefsPtr = (void *)bsearch(mathToken[i].tokenName, texDefs,
        (size_t)numSymbs, sizeof(struct texDef_struct), texSrchCmp);
    if (!texDefsPtr) {
      printLongLine(cat("?Warning:  The token \"", mathToken[i].tokenName,
       "\", which was defined in a $v or $c statement, was not declared in ",
          htmlFlag ? "an htmldef" : "a latexdef", " statement.", NULL),
          "", " ");
      warningFound = 1;
    }
  }

  /* 26-Jun-2011 nm Added this check */
  /* Check to make sure all GIFs are present */
  if (htmlFlag) {
    for (i = 0; i < numSymbs; i++) {
      tmpPtr = texDefs[i].texEquiv;
      k = 0;
      while (1) {
        j = instr(k + 1, tmpPtr, "IMG SRC=");
                   /* Note that only an exact match with
                      "IMG SRC=" is currently handled */
        if (j == 0) break;
        k = instr(j + 9, texDefs[i].texEquiv, mid(tmpPtr, j + 8, 1));
                                           /* Get position of trailing quote */
                                    /* Future:  use strchr instead of mid()
                                       for efficiency? */
        let(&token, seg(tmpPtr, j + 9, k - 1));  /* Get name of .gif (.png) */
        if (k == 0) break;  /* Future: we may want to issue "missing
                                     trailing quote" warning */
           /* (We test k after the let() so that the temporary string stack
              entry created by mid() is emptied and won't overflow */
        if (noGifCheck == 0) { /* 17-Nov-2015 nm */
          tmpFp = fopen(token, "r"); /* See if it exists */
          if (!tmpFp) {
            printLongLine(cat("?Warning:  The file \"", token,
                "\", which is referenced in an htmldef",
                " statement, was not found.", NULL),
                "", " ");
            warningFound = 1;
          } else {
            fclose(tmpFp);
          }
        }
      }
    }
  }


  /* Look up the extended database start label */
  if (extHtmlLabel[0]) {
    for (i = 1; i <= statements; i++) {
      if (!strcmp(extHtmlLabel, statement[i].labelName)) break;
    }
    if (i > statements) {
      printLongLine(cat("?Warning: There is no statement with label \"",
          extHtmlLabel,
          "\" (specified by exthtmllabel in the database source $t comment).  ",
          "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
      warningFound = 1;
    }
    extHtmlStmt = i;
  } else {
    /* There is no extended database; set threshold to beyond end of db */
    extHtmlStmt = statements + 1;
  }

  /* 29-Jul-2008 nm Sandbox stuff */
  /* 24-Jul-2009 nm Changed name of sandbox to "mathbox" */
  /* 28-Jun-2011 nm Use lookupLabel for speedup */
  if (sandboxStmt == 0) {
    sandboxStmt = lookupLabel("mathbox");
    if (sandboxStmt == -1)
      sandboxStmt = statements + 1;  /* Default beyond db end if none */
  }
  /*
  for (i = 1; i <= statements; i++) {
    /@ For now (and probably forever) the sandbox start theorem is
       hardcoded to "sandbox", like in set.mm @/
    /@ if (!strcmp("sandbox", statement[i].labelName)) { @/
    /@ 24-Jul-2009 nm Changed name of sandbox to "mathbox" @/
    if (!strcmp("mathbox", statement[i].labelName)) {
      sandboxStmt = i;
      break;
    }
  }
  */
  /* In case there is not extended (Hilbert Space Explorer) section,
     but there is a sandbox section, make the extended section "empty". */
  if (extHtmlStmt == statements + 1) extHtmlStmt = sandboxStmt;
  let(&sandboxHome, cat("<A HREF=\"mmtheorems.html#sandbox:bighdr\">",
    "<FONT SIZE=-2 FACE=sans-serif>",
    "<IMG SRC=\"_sandbox.gif\" BORDER=0 ALT=",
    "\"Table of Contents\" HEIGHT=32 WIDTH=32 ALIGN=MIDDLE>",
    "Table of Contents</FONT></A>", NULL));
  let(&sandboxHomeHREF, "mmtheorems.html#sandbox:bighdr");
  let(&sandboxHomeIMG, "_sandbox.gif");
  let(&sandboxTitleAbbr, "Users' Mathboxes");
  /*let(&sandboxTitle, "User Sandbox");*/
  /* 24-Jul-2009 nm Changed name of sandbox to "mathbox" */
  let(&sandboxTitle, "Users' Mathboxes");

  /* 16-Aug-2016 nm */
  /*
  let(&sandboxHomeHREF, "mmtheorems.html#sandbox:bighdr");
  let(&sandboxHomeIMG, "_sandbox.gif");
  let(&sandboxTitleAbbr, "Mathboxes");
  */

  /* 16-Aug-2016 nm */
  /* Extract derived variables from the $t variables */
  /* (In the future, it might be better to do this directly in the $t.) */
  i = instr(1, htmlHome, "HREF=\"") + 5;
  if (i == 5) {
    printLongLine(
        "?Warning: In the $t comment, htmlHome has no 'HREF=\"'.", "", " ");
    warningFound = 1;
  }
  j = instr(i + 1, htmlHome, "\"");
  let(&htmlHomeHREF, seg(htmlHome, i + 1, j - 1));
  i = instr(1, htmlHome, "IMG SRC=\"") + 8;
  if (i == 8) {
    printLongLine(
        "?Warning: In the $t comment, htmlHome has no 'IMG SRC=\"'.", "", " ");
    warningFound = 1;
  }
  j = instr(i + 1, htmlHome, "\"");
  let(&htmlHomeIMG, seg(htmlHome, i + 1, j - 1));


  /* 16-Aug-2016 nm */
  /* Compose abbreviated title from capital letters */
  j = (long)strlen(htmlTitle);
  let(&htmlTitleAbbr, "");
  for (i = 1; i <= j; i++) {
    if (htmlTitle[i - 1] >= 'A' && htmlTitle[i -1] <= 'Z') {
      let(&htmlTitleAbbr, cat(htmlTitleAbbr, chr(htmlTitle[i - 1]), NULL));
    }
  }
  let(&htmlTitleAbbr, cat(htmlTitleAbbr, " Home", NULL));

  /* 18-Aug-2016 nm Added conditional test for extended section */
  if (extHtmlStmt < statements + 1 /* If extended section exists */
      /* nm 8-Dec-2017 nm */
      && extHtmlStmt != sandboxStmt) { /* and is not an empty dummy section */
    i = instr(1, extHtmlHome, "HREF=\"") + 5;
    if (i == 5) {
      printLongLine(
          "?Warning: In the $t comment, extHtmlHome has no 'HREF=\"'.", "", " ");
      warningFound = 1;
    }
    j = instr(i + 1, extHtmlHome, "\"");
    let(&extHtmlHomeHREF, seg(extHtmlHome, i + 1, j - 1));
    i = instr(1, extHtmlHome, "IMG SRC=\"") + 8;
    if (i == 8) {
      printLongLine(
          "?Warning: In the $t comment, extHtmlHome has no 'IMG SRC=\"'.", "", " ");
      warningFound = 1;
    }
    j = instr(i + 1, extHtmlHome, "\"");
    let(&extHtmlHomeIMG, seg(extHtmlHome, i + 1, j - 1));
    /* Compose abbreviated title from capital letters */
    j = (long)strlen(extHtmlTitle);
    let(&extHtmlTitleAbbr, "");
    for (i = 1; i <= j; i++) {
      if (extHtmlTitle[i - 1] >= 'A' && extHtmlTitle[i -1] <= 'Z') {
        let(&extHtmlTitleAbbr, cat(extHtmlTitleAbbr,
            chr(extHtmlTitle[i - 1]), NULL));
      }
    }
    let(&extHtmlTitleAbbr, cat(extHtmlTitleAbbr, " Home", NULL));
  }

  /* 16-Aug-2016 nm For debugging - remove later */
  /*
  printf("h=%s i=%s a=%s\n",htmlHomeHREF,htmlHomeIMG,htmlTitleAbbr);
  printf("eh=%s ei=%s ea=%s\n",extHtmlHomeHREF,extHtmlHomeIMG,extHtmlTitleAbbr);
  */



  let(&token, ""); /* Deallocate */
  let(&partialToken, ""); /* Deallocate */  /* 6-Aug-2011 nm */
  let(&fileBuf, "");  /* was: free(fileBuf); */
  texDefsRead = 1;  /* Set global flag that it's been read in */
  return warningFound; /* Return indicator that parsing passed (0) or
                           had warning(s) (1) */

} /* readTexDefs */

/* This function returns the length of the white space starting at ptr.
   Comments are considered white space.  ptr should point to the first character
   of the white space.  If ptr does not point to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned. */
long texDefWhiteSpaceLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  char *ptr1;
  while (1) {
    tmpchr = ptr[i];
    if (!tmpchr) return (i); /* End of string */
    if (isalnum((unsigned char)(tmpchr))) return (i); /* Alphanumeric string */

    /* 6-Aug-2011 nm Removed this undocumented feature */
    /*
    if (ptr[i] == '!') { /@ Comment to end-of-line @/
      ptr1 = strchr(ptr + i + 1, '\n');
      if (!ptr1) bug(2306);
      i = ptr1 - ptr + 1;
      continue;
    }
    */

    if (tmpchr == '/') { /* Embedded c-style comment - used to ignore
        comments inside of Metamath comment for LaTeX/HTML definitions */
      if (ptr[i + 1] == '*') {
        while (1) {
          ptr1 = strchr(ptr + i + 2, '*');
          if (!ptr1) {
            return(i + (long)strlen(&ptr[i])); /* Unterminated comment - goto EOF */
          }
          if (ptr1[1] == '/') break;
          i = ptr1 - ptr;
        }
        i = ptr1 - ptr + 2;
        continue;
      } else {
        return(i);
      }
    }
    if (isgraph((unsigned char)tmpchr)) return (i);
    i++;
  }
  bug(2307);
  return(0); /* Dummy return - never executed */
} /* texDefWhiteSpaceLen */


/* This function returns the length of the token (non-white-space) starting at
   ptr.  Comments are considered white space.  ptr should point to the first
   character of the token.  If ptr points to a white space character, 0
   is returned.  If ptr points to a null character, 0 is returned.  If ptr
   points to a quoted string, the quoted string is returned.  A non-alphanumeric\
   characters ends a token and is a single token. */
long texDefTokenLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  char *ptr1;
  tmpchr = ptr[i];
  if (tmpchr == '\"') {
    while (1) {
      ptr1 = strchr(ptr + i + 1, '\"');
      if (!ptr1) {
        return(i + (long)strlen(&ptr[i])); /* Unterminated quote - goto EOF */
      }
      if (ptr1[1] != '\"') return(ptr1 - ptr + 1); /* Double quote is literal */
      i = ptr1 - ptr + 1;
    }
  }
  if (tmpchr == '\'') {
    while (1) {
      ptr1 = strchr(ptr + i + 1, '\'');
      if (!ptr1) {
        return(i + (long)strlen(&ptr[i])); /* Unterminated quote - goto EOF */
      }
      if (ptr1[1] != '\'') return(ptr1 - ptr + 1); /* Double quote is literal */
      i = ptr1 - ptr + 1;
    }
  }
  if (ispunct((unsigned char)tmpchr)) return (1); /* Single-char token */
  while (1) {
    tmpchr = ptr[i];
    if (!isalnum((unsigned char)tmpchr)) return (i); /* End of alphnum. token */
    i++;
  }
  bug(2308);
  return(0); /* Dummy return - never executed */
} /* texDefTokenLen */

/* Token comparison for qsort */
int texSortCmp(const void *key1, const void *key2)
{
  /* Returns -1 if key1 < key2, 0 if equal, 1 if key1 > key2 */
  /* Note:  ptr->fld == (*ptr).fld
            str.fld == (&str)->fld   */
  return (strcmp( ((struct texDef_struct *)key1)->tokenName,
      ((struct texDef_struct *)key2)->tokenName));
} /* texSortCmp */


/* Token comparison for bsearch */
int texSrchCmp(const void *key, const void *data)
{
  /* Returns -1 if key < data, 0 if equal, 1 if key > data */
  return (strcmp(key,
      ((struct texDef_struct *)data)->tokenName));
} /* texSrchCmp */

/* Convert ascii to a string of \tt tex; must not have control chars */
/* (The caller must surround it by {\tt }) */
/* ***Note:  The caller must deallocate returned string */
vstring asciiToTt(vstring s)
{

  vstring ttstr = "";
  vstring tmp = "";
  long i, j, k;

  let(&ttstr, s); /* In case the input s is temporarily allocated */
  j = (long)strlen(ttstr);

  /* Put special \tt font characters in a form that TeX can understand */
  for (i = 0; i < j; i++) {
    k = 1;
    if (!htmlFlag) {
      switch (ttstr[i]) {
        /* For all unspecified cases, TeX will accept the character 'as is' */
        case ' ':
        case '$':
        case '%':
        case '#':
        case '{':
        case '}':
        case '&':
          let(&ttstr,cat(left(ttstr,i),"\\",right(ttstr,i+1),NULL));
          k = 2;
          break;
        case '^':
          let(&ttstr,cat(left(ttstr,i),"\\^{ }",right(ttstr,i+2),NULL));
          k = 5;
          break;
        case '\\':
        case '|':
        case '<':
        case '>':
        case '"':
        case '~':
        case '_':
          /* Note:  this conversion will work for any character, but
             results in the most TeX source code. */
          let(&ttstr,cat(left(ttstr,i),"\\char`\\",right(ttstr,i+1),NULL));
          k = 8;
          break;
      } /* End switch mtoken[i] */
    } else {
      switch (ttstr[i]) {
        /* For all unspecified cases, HTML will accept the character 'as is' */
        /* 1-Oct-04 Don't convert to &amp; anymore but leave as is.  This
           will allow the user to insert HTML entities for Unicode etc.
           directly in the database source. */
        /*
        case '&':
          let(&ttstr,cat(left(ttstr,i),"&amp;",right(ttstr,i+2),NULL));
          k = 5;
          break;
        */
        case '<':
          /* 11/15/02 Leave in some special HTML tags (case must match) */
          /* This was done specially for the set.mm inf3 comment */
          /*
          if (!strcmp(mid(ttstr, i + 1, 5), "<PRE>")) {
            let(&ttstr, ttstr); /@ Purge stack to prevent overflow by 'mid' @/
            i = i + 5;
            break;
          }
          if (!strcmp(mid(ttstr, i + 1, 6), "</PRE>")) {
            let(&ttstr, ttstr); /@ Purge stack to prevent overflow by 'mid' @/
            i = i + 6;
            break;
          }
          */
          /* 26-Dec-2011 nm - changed <PRE> to more general <HTML> */
          if (!strcmp(mid(ttstr, i + 1, 6), "<HTML>")) {
            let(&ttstr, ttstr); /* Purge stack to prevent overflow by 'mid' */
            i = i + 6;
            break;
          }
          if (!strcmp(mid(ttstr, i + 1, 7), "</HTML>")) {
            let(&ttstr, ttstr); /* Purge stack to prevent overflow by 'mid' */
            i = i + 7;
            break;
          }
          let(&ttstr,cat(left(ttstr,i),"&lt;",right(ttstr,i+2),NULL));
          k = 4;
          break;
        case '>':
          let(&ttstr,cat(left(ttstr,i),"&gt;",right(ttstr,i+2),NULL));
          k = 4;
          break;
        case '"':
          let(&ttstr,cat(left(ttstr,i),"&quot;",right(ttstr,i+2),NULL));
          k = 6;
          break;
      } /* End switch mtoken[i] */
    }

    if (k > 1) { /* Adjust iteration and length */
      i = i + k - 1;
      j = j + k - 1;
    }
  } /* Next i */

  let(&tmp, "");  /* Deallocate */
  return(ttstr);
} /* asciiToTt */


/* Convert ascii token to TeX equivalent */
/* The "$" math delimiter is not placed around the returned arg. here */
/* *** Note: The caller must deallocate the returned string */
vstring tokenToTex(vstring mtoken, long statemNum /*for error msgs*/)
{
  vstring tex = "";
  vstring tmpStr;
  long i, j, k;
  void *texDefsPtr; /* For binary search */
  flag saveOutputToString;

  if (!texDefsRead) {
    bug(2320); /* This shouldn't be called if definitions weren't read */
  }

  texDefsPtr = (void *)bsearch(mtoken, texDefs, (size_t)numSymbs,
      sizeof(struct texDef_struct), texSrchCmp);
  if (texDefsPtr) { /* Found it */
    let(&tex, ((struct texDef_struct *)texDefsPtr)->texEquiv);
  } else {
    /* 9/5/99 If it wasn't found, give user a warning... */
    saveOutputToString = outputToString;
    outputToString = 0;
    /* if (statemNum <= 0 || statemNum > statements) bug(2331); */ /* OLD */
    /* 19-Sep-2012 nm It is possible for statemNum to be 0 when
       tokenToTex() is called (via getTexLongMath()) from
       printTexLongMath(), when its hypStmt argument is 0 (= not associated
       with a statement).  (Reported by Wolf Lammen.) */
    if (statemNum < 0 || statemNum > statements) bug(2331);
    if (statemNum > 0) {   /* Include statement label in error message */
      printLongLine(cat("?Warning: In the comment for statement \"",
          statement[statemNum].labelName,
          "\", math symbol token \"", mtoken,
          "\" does not have a LaTeX and/or an HTML definition.", NULL),
          "", " ");
    } else { /* There is no statement associated with the error message */
      printLongLine(cat("?Warning: Math symbol token \"", mtoken,
          "\" does not have a LaTeX and/or an HTML definition.", NULL),
          "", " ");
    }
    outputToString = saveOutputToString;
    /* ... but we'll still leave in the old default conversion anyway: */

    /* If it wasn't found, use built-in conversion rules */
    let(&tex, mtoken);

    /* First, see if it's a tilde followed by a letter */
    /* If so, remove the tilde.  (This is actually obsolete.) */
    /* (The tilde was an escape in the obsolete syntax.) */
    if (tex[0] == '~') {
      if (isalpha((unsigned char)(tex[1]))) {
        let(&tex, right(tex, 2)); /* Remove tilde */
      }
    }

    /* Next, convert punctuation characters to tt font */
    j = (long)strlen(tex);
    for (i = 0; i < j; i++) {
      if (ispunct((unsigned char)(tex[i]))) {
        tmpStr = asciiToTt(chr(tex[i]));
        if (!htmlFlag)
          let(&tmpStr, cat("{\\tt ", tmpStr, "}", NULL));
        k = (long)strlen(tmpStr);
        let(&tex,
            cat(left(tex, i), tmpStr, right(tex, i + 2), NULL));
        i = i + k - 1; /* Adjust iteration */
        j = j + k - 1; /* Adjust length */
        let(&tmpStr, ""); /* Deallocate */
      }
    } /* Next i */

    /* Make all letters Roman; put inside mbox */
    if (!htmlFlag)
      let(&tex, cat("\\mbox{\\rm ", tex, "}", NULL));

  } /* End if */

  return (tex);
} /* tokenToTex */


/* Converts a comment section in math mode to TeX.  Each math token
   MUST be separated by white space.   TeX "$" does not surround the output. */
vstring asciiMathToTex(vstring mathComment, long statemNum)
{

  vstring tex;
  vstring texLine = "";
  vstring lastTex = "";
  vstring token = "";
  flag alphnew, alphold, unknownnew, unknownold;
  /* flag firstToken; */  /* Not used */
  long i;
  vstring srcptr;

  srcptr = mathComment;

  let(&texLine, "");
  let(&lastTex, "");
  /* firstToken = 1; */
  while(1) {
    i = whiteSpaceLen(srcptr);
    srcptr = srcptr + i;
    i = tokenLen(srcptr);
    if (!i) break; /* Done */
    let(&token, space(i));
    memcpy(token, srcptr, (size_t)i);
    srcptr = srcptr + i;
    tex = tokenToTex(token, statemNum); /* Convert token to TeX */
              /* tokenToTex allocates tex; we must deallocate it */

    if (!htmlFlag) {
      /* If this token and previous token begin with letter, add a thin
           space between them */
      /* Also, anything not in table will have space added */
      alphnew = !!isalpha((unsigned char)(tex[0])); /* 31-Aug-2012 nm Added
            "!!" here and below because isalpha returns an integer, whose
            unspecified non-zero value could be truncated to 0 when
            converted to char.  Thanks to Wolf Lammen for pointing this out. */
      unknownnew = 0;
      if (!strcmp(left(tex, 10), "\\mbox{\\rm ")) { /* Token not in table */
        unknownnew = 1;
      }
      alphold = !!isalpha((unsigned char)(lastTex[0]));
      unknownold = 0;
      if (!strcmp(left(lastTex, 10), "\\mbox{\\rm ")) { /* Token not in table*/
        unknownold = 1;
      }
   /*if ((alphold && alphnew) || unknownold || (unknownnew && !firstToken)) {*/
      /* Put thin space only between letters and/or unknowns  11/3/94 */
      if ((alphold || unknownold) && (alphnew || unknownnew)) {
        /* Put additional thin space between two letters */
        let(&texLine, cat(texLine, "\\,", tex, " ", NULL));
      } else {
        let(&texLine, cat(texLine, tex, " ", NULL));
      }
    } else {
      let(&texLine, cat(texLine, tex, NULL));
    }
    let(&lastTex, ""); /* Deallocate */
    lastTex = tex; /* Pass deallocation responsibility for tex to lastTex */

    /* firstToken = 0; */

  } /* End while (1) */
  let(&lastTex, ""); /* Deallocate */
  let(&token, ""); /* Deallocate */

  return (texLine);

} /* asciiMathToTex */


/* Gets the next section of a comment that is in the current mode (text,
   label, or math).  If 1st char. is not "$" (DOLLAR_SUBST), text mode is
   assumed.  mode = 0 means end of comment reached.  srcptr is left at 1st
   char. of start of next comment section. */
vstring getCommentModeSection(vstring *srcptr, char *mode)
{
  vstring modeSection = "";
  vstring ptr; /* Not allocated */
  flag addMode = 0;
  if (!outputToString) bug(2319);  /* 10/10/02 */

  if ((*srcptr)[0] != DOLLAR_SUBST /*'$'*/) {
    if ((*srcptr)[0] == 0) { /* End of string */
      *mode = 0; /* End of comment */
      return ("");
    } else {
      *mode = 'n'; /* Normal text */
      addMode = 1;
    }
  } else {
    switch ((*srcptr)[1]) {
      case 'l':
      case 'm':
      case 'n':
        *mode = (*srcptr)[1];
        break;
      case ')':  /* Obsolete */
        bug(2317);
        /* Leave old code in case user continues through the bug */
        *mode = 0; /* End of comment */
        return ("");
        break;
      default:
        *mode = 'n';
        break;
    }
  }

  ptr = (*srcptr) + 1;
  while (1) {
    if (ptr[0] == DOLLAR_SUBST /*'$'*/) {
      switch (ptr[1]) {
        case 'l':
        case 'm':
        case 'n':
        case ')':  /* Obsolete (will never happen) */
          if (ptr[1] == ')') bug(2318);
          let(&modeSection, space(ptr - (*srcptr)));
          memcpy(modeSection, *srcptr, (size_t)(ptr - (*srcptr)));
          if (addMode) {
            let(&modeSection, cat(chr(DOLLAR_SUBST), "n", /*"$n"*/ modeSection,
                NULL));
          }
          *srcptr = ptr;
          return (modeSection);
          break;
      }
    } else {
      if (ptr[0] == 0) {
          let(&modeSection, space(ptr - (*srcptr)));
          memcpy(modeSection, *srcptr, (size_t)(ptr - (*srcptr)));
          if (addMode) {
            let(&modeSection, cat(chr(DOLLAR_SUBST), "n", /*"$n"*/ modeSection,
                NULL));
          }
          *srcptr = ptr;
          return (modeSection);
      }
    }
    ptr++;
  } /* End while */
  return(NULL); /* Dummy return - never executes */
} /* getCommentModeSection */


/* The texHeaderFlag means this:
    If !htmlFlag (i.e. TeX mode), then 1 means print header
    If htmlFlag, then 1 means include "Previous Next" links on page,
    based on the global showStatement variable
*/
void printTexHeader(flag texHeaderFlag)
{

  long i, j, k;
  vstring tmpStr = "";

  /* 2-Aug-2009 nm - "Mathbox for <username>" mod */
  vstring localSandboxTitle = "";
  vstring hugeHdr = ""; /* 21-Jun-2014 nm */
  vstring bigHdr = "";
  vstring smallHdr = "";
  vstring tinyHdr = "";  /* 21-Aug-2017 nm */
  vstring hugeHdrComment = ""; /* 8-May-2015 nm */
  vstring bigHdrComment = ""; /* 8-May-2015 nm */
  vstring smallHdrComment = ""; /* 8-May-2015 nm */
  vstring tinyHdrComment = ""; /* 21-Aug-2017 nm */

  /*if (!texDefsRead) {*/ /* 17-Nov-2015 nm Now done in readTexDefs() */
  if (2/*error*/ == readTexDefs(0/*errorsOnly=0*/, 0 /*noGifCheck=0*/)) {
    print2(
       "?There was an error in the $t comment's LaTeX/HTML definitions.\n");
    return;
  }
  /*}*/

  outputToString = 1;  /* Redirect print2 and printLongLine to printString */
  /*let(&printString, "");*/ /* May have stuff to be printed 7/4/98 */
  if (!htmlFlag) {
    print2("%s This LaTeX file was created by Metamath on %s %s.\n",
       "%", date(), time_());

    /* 14-Sep-2010 nm Added OLD_TEX (oldTexFlag) */
    if (texHeaderFlag && !oldTexFlag) {
      print2("\\documentclass{article}\n");
      print2("\\usepackage{graphicx} %% For rotated iota\n"); /* 29-Nov-2013 nm */
      print2("\\usepackage{amssymb}\n");
      print2("\\usepackage{amsmath} %% For \\begin{align}...\n");
      print2("\\usepackage{amsthm}\n");
      print2("\\theoremstyle{plain}\n");
      print2("\\newtheorem{theorem}{Theorem}[section]\n");
      print2("\\newtheorem{definition}[theorem]{Definition}\n");
      print2("\\newtheorem{lemma}[theorem]{Lemma}\n");
      print2("\\newtheorem{axiom}{Axiom}\n");
      print2("\\allowdisplaybreaks[1] %% Allow page breaks in {align}\n");
      print2("\\usepackage[plainpages=false,pdfpagelabels]{hyperref}\n");
      print2("\\hypersetup{colorlinks} %% Get rid of boxes around links\n");
                                                        /* 2-May-2017 */
      print2("\\begin{document}\n");
      print2("\n");
    }

    if (texHeaderFlag && oldTexFlag) {
      /******* LaTeX 2.09
      print2(
"\\documentstyle[leqno]{article}\n");
      *******/
      /* LaTeX 2e */
      print2(
    "\\documentclass[leqno]{article}\n");
      /* LaTeX 2e */
      print2("\\usepackage{graphicx}\n"); /* 29-Nov-2013 nm For rotated iota */
      print2(
    "\\usepackage{amssymb}\n");
      print2(
"\\raggedbottom\n");
      print2(
"\\raggedright\n");
      print2(
"%%\\title{Your title here}\n");
      print2(
"%%\\author{Your name here}\n");
      print2(
"\\begin{document}\n");
      /******* LaTeX 2.09
      print2(
"\\input amssym.def  %% Load in AMS fonts\n");
      print2(
"\\input amssym.tex  %% Load in AMS fonts\n");
      *******/
      print2(
"%%\\maketitle\n");
      print2(
"\\newbox\\mlinebox\n");
      print2(
"\\newbox\\mtrialbox\n");
      print2(
"\\newbox\\startprefix  %% Prefix for first line of a formula\n");
      print2(
"\\newbox\\contprefix  %% Prefix for continuation line of a formula\n");
      print2(
"\\def\\startm{  %% Initialize formula line\n");
      print2(
"  \\setbox\\mlinebox=\\hbox{\\unhcopy\\startprefix}\n");
      print2(
"}\n");
      print2(
"\\def\\m#1{  %% Add a symbol to the formula\n");
      print2(
"  \\setbox\\mtrialbox=\\hbox{\\unhcopy\\mlinebox $\\,#1$}\n");
      print2(
"  \\ifdim\\wd\\mtrialbox>\\hsize\n");
      print2(
"    \\box\\mlinebox\n");
      print2(
"    \\setbox\\mlinebox=\\hbox{\\unhcopy\\contprefix $\\,#1$}\n");
      print2(
"  \\else\n");
      print2(
"    \\setbox\\mlinebox=\\hbox{\\unhbox\\mtrialbox}\n");
      print2(
"  \\fi\n");
      print2(
"}\n");
      print2(
"\\def\\endm{  %% Output the last line of a formula\n");
      print2(
"  \\box\\mlinebox\n");
      print2(
"}\n");
    }
  } else { /* htmlFlag */

    print2(
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n");
    print2(     "    \"http://www.w3.org/TR/html4/loose.dtd\">\n");
    print2("<HTML LANG=\"EN-US\">\n");
    print2("<HEAD>\n");
    print2("%s%s\n", "<META HTTP-EQUIV=\"Content-Type\" ",
        "CONTENT=\"text/html; charset=iso-8859-1\">");
    /* 13-Aug-2016 nm */
    /* Improve mobile device display per David A. Wheeler */
    print2(
"<META NAME=\"viewport\" CONTENT=\"width=device-width, initial-scale=1.0\">\n"
        );

    print2("<STYLE TYPE=\"text/css\">\n");
    print2("<!--\n");
    /* 15-Apr-2015 nm - align math symbol images to text */
    /* 18-Oct-2015 nm Image alignment fix */
    /* 10-Jan-2016 nm Optional information but takes unnecessary file space */
    /* (change @ to * if uncommenting)
    print2(
        "/@ Math symbol images will be shifted down 4 pixels to align with\n");
    print2(
        "   normal text for compatibility with various browsers.  The old\n");
    print2(
        "   ALIGN=TOP for math symbol images did not align in all browsers\n");
    print2(
        "   and should be deleted.  All other images must override this\n");
    print2(
        "   shift with STYLE=\"margin-bottom:0px\". @/\n");
    */
    print2("img { margin-bottom: -4px }\n");
#ifndef RAINBOW_OPTION
    /* Print style sheet for pink number that goes after statement label */
    print2(".p { font-family: \"Arial Narrow\";\n");
    print2("     font-size: x-small;\n");
    /* Strip off quotes from color (css doesn't like them) */
    printLongLine(cat("     color: ", seg(PINK_NUMBER_COLOR, 2,
        (long)strlen(PINK_NUMBER_COLOR) - 1), ";", NULL), "", "&");
    print2("   }\n");
#else
    /* Print style sheet for rainbow-colored number that goes after
       statement label */
    print2(".r { font-family: \"Arial Narrow\";\n");
    print2("     font-size: x-small;\n");
    /* There is no color */
    print2("   }\n");
#endif

#ifdef INDENT_HTML_PROOFS
    /* nm 3-Feb-04 Experiment to indent web proof displays */
    /* Print style sheet for HTML proof indentation number */
    /* ??? Future - combine with above style sheet */
    print2(".i { font-family: \"Arial Narrow\";\n");
    print2("     font-size: x-small;\n");
    print2("     color: gray;\n");
    print2("   }\n");
#endif
    print2("-->\n");
    print2("</STYLE>\n");
    printLongLine(htmlCSS, "", " ");

    /*
    /@ 12-Jan-2016 nm Per Mario Carneiro's suggestion @/
    print2(".set { color: red; font-style: italic }\n");
    print2(".class { color: #C3C; font-style: italic }\n");
    print2(".wff { color: blue; font-style: italic }\n");
    /@ .symvar = use symbol as class variable @/
    print2(".symvar { text-decoration: underline dotted; color: #C3C}\n");
    print2(".typecode { color: gray }\n");
    print2(".hidden { color: gray }\n");
    /@ 10-Jan-2016 nm Experiment to always include a style sheet @/
    /@   Todo: decide on name and directory for .css's @/
    print2("<LINK href=\"mmstyle.css\" title=\"mmstyle\"\n");
    print2("      rel=\"stylesheet\" type=\"text/css\">\n");
    print2("<LINK href=\"mmstylealt.css\" title=\"mmstylealt\"\n");
    print2("      rel=\"alternate stylesheet\" type=\"text/css\">\n");
    */

    /*
    print2("<META NAME=\"ROBOTS\" CONTENT=\"NONE\">\n");
    print2("<META NAME=\"GENERATOR\" CONTENT=\"Metamath\">\n");
    */
    /*
    if (showStatement < extHtmlStmt) {
      print2("%s\n", cat("<TITLE>", htmlTitle, " - ",
          / * Strip off ".html" * /
          left(texFileName, (long)strlen(texFileName) - 5),
          / *left(texFileName, instr(1, texFileName, ".htm") - 1),* /
          "</TITLE>", NULL));
    } else {
      print2("%s\n", cat("<TITLE>", extHtmlTitle, " - ",
          / * Strip off ".html" * /
          left(texFileName, (long)strlen(texFileName) - 5),
          / *left(texFileName, instr(1, texFileName, ".htm") - 1),* /
          "</TITLE>", NULL));
    }
    */
    /* 4-Jun-06 nm - Put theorem name before "Metamath Proof Explorer" etc. */
    if (showStatement < extHtmlStmt) {
      print2("%s\n", cat("<TITLE>",
          /* Strip off ".html" */
          left(texFileName, (long)strlen(texFileName) - 5),
          /*left(texFileName, instr(1, texFileName, ".htm") - 1),*/
          " - ", htmlTitle,
          "</TITLE>", NULL));
    /*} else {*/
    } else if (showStatement < sandboxStmt) { /* 29-Jul-2008 nm Sandbox stuff */
      print2("%s\n", cat("<TITLE>",
          /* Strip off ".html" */
          left(texFileName, (long)strlen(texFileName) - 5),
          /*left(texFileName, instr(1, texFileName, ".htm") - 1),*/
          " - ", extHtmlTitle,
          "</TITLE>", NULL));

    /* 29-Jul-2008 nm Sandbox stuff */
    } else {

      /* 2-Aug-2009 nm - "Mathbox for <username>" mod */
      /* Scan from this statement backwards until a big header is found */
      for (i = showStatement; i > sandboxStmt; i--) {
        if (statement[i].type == a_ || statement[i].type == p_) {
                                                            /* 18-Dec-2016 nm */
          /* Note: only bigHdr is used; the other 5 returned strings are
             ignored */
          getSectionHeadings(i, &hugeHdr, &bigHdr, &smallHdr,
              &tinyHdr,
              /* 5-May-2015 nm */
              &hugeHdrComment, &bigHdrComment, &smallHdrComment,
              &tinyHdrComment);
          if (bigHdr[0] != 0) break;
        } /* 18-Dec-2016 nm */
      } /* next i */
      if (bigHdr[0]) {
        /* A big header was found; use it for the page title */
        let(&localSandboxTitle, bigHdr);
      } else {
        /* A big header was not found (should not happen if set.mm is
           formatted right, but use default just in case) */
        let(&localSandboxTitle, sandboxTitle);
      }
      let(&hugeHdr, "");   /* Deallocate memory */
      let(&bigHdr, "");   /* Deallocate memory */
      let(&smallHdr, ""); /* Deallocate memory */
      let(&tinyHdr, ""); /* Deallocate memory */
      let(&hugeHdrComment, "");   /* Deallocate memory */
      let(&bigHdrComment, "");   /* Deallocate memory */
      let(&smallHdrComment, ""); /* Deallocate memory */
      let(&tinyHdrComment, ""); /* Deallocate memory */
      /* 2-Aug-2009 nm - end of "Mathbox for <username>" mod */

      printLongLine(cat("<TITLE>",
          /* Strip off ".html" */
          left(texFileName, (long)strlen(texFileName) - 5),
          /*left(texFileName, instr(1, texFileName, ".htm") - 1),*/
          /*" - ", sandboxTitle,*/
          /* 2-Aug-2009 nm - "Mathbox for <username>" mod */
          " - ", localSandboxTitle,
          "</TITLE>", NULL), "", "\"");

    }
    /* Icon for bookmark */
    print2("%s%s\n", "<LINK REL=\"shortcut icon\" HREF=\"favicon.ico\" ",
        "TYPE=\"image/x-icon\">");
    /*
    print2("<META HTML-EQUIV=\"Keywords\"\n");
    print2("CONTENT=\"%s\">\n", htmlTitle);
    */

    print2("</HEAD>\n");
    /*print2("<BODY BGCOLOR=\"#D2FFFF\">\n");*/
    /*print2("<BODY BGCOLOR=%s>\n", MINT_BACKGROUND_COLOR);*/
    print2("<BODY BGCOLOR=\"#FFFFFF\">\n");

    /* 16-Aug-2016 nm Major revision of header */
    print2("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 WIDTH=\"100%s\">\n",
         "%");
    print2("  <TR>\n");
    print2("    <TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%s\"><A HREF=\n", "%");
    print2("    \"%s\"><IMG SRC=\"%s\"\n",
        (showStatement < extHtmlStmt ? htmlHomeHREF :
             (showStatement < sandboxStmt ? extHtmlHomeHREF :
             sandboxHomeHREF)),
        /* Note that we assume that the upper-left image is 32x32 */
        (showStatement < extHtmlStmt ? htmlHomeIMG :
             (showStatement < sandboxStmt ? extHtmlHomeIMG :
             sandboxHomeIMG)));
    print2("      BORDER=0\n");
    print2("      ALT=\"%s\"\n",
        (showStatement < extHtmlStmt ? htmlTitleAbbr :
             (showStatement < sandboxStmt ? extHtmlTitleAbbr :
             sandboxTitleAbbr)));
    print2("      TITLE=\"%s\"\n",
        (showStatement < extHtmlStmt ? htmlTitleAbbr :
             (showStatement < sandboxStmt ? extHtmlTitleAbbr :
             sandboxTitleAbbr)));
    print2(
      "      HEIGHT=32 WIDTH=32 ALIGN=TOP STYLE=\"margin-bottom:0px\"></A>\n");
    print2("    </TD>\n");
    print2("    <TD ALIGN=CENTER VALIGN=TOP><FONT SIZE=\"+3\" COLOR=%s><B>\n",
      GREEN_TITLE_COLOR);
    /* Allow plenty of room for long titles (although over 79 chars. will
       trigger bug 1505). */
    print2("%s\n",
        (showStatement < extHtmlStmt ? htmlTitle :
             (showStatement < sandboxStmt ? extHtmlTitle :
             localSandboxTitle)));
    print2("      </B></FONT></TD>\n");
    /* print2("    </TD>\n"); */ /* 26-Dec-2016 nm Delete extra </TD> */


    /*
    print2("<TABLE BORDER=0 WIDTH=\"100%s\"><TR>\n", "%");
    print2("<TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%s\"\n", "%");
    */

    /*
    print2("<A HREF=\"mmexplorer.html\">\n");
    print2("<IMG SRC=\"cowboy.gif\"\n");
    print2("ALT=\"[Image of cowboy]Home Page\"\n");
    print2("WIDTH=27 HEIGHT=32 ALIGN=LEFT><FONT SIZE=-1 FACE=sans-serif>Home\n");
    print2("</FONT></A>");
    */
    /*
    if (showStatement < extHtmlStmt) {
      printLongLine(cat("ROWSPAN=2>", htmlHome, "</TD>", NULL), "", "\"");
    /@} else {@/
    } else if (showStatement < sandboxStmt) { /@ 29-Jul-2008 nm Sandbox stuff @/
      printLongLine(cat("ROWSPAN=2>", extHtmlHome, "</TD>", NULL), "", "\"");

    /@ 29-Jul-2008 nm Sandbox stuff @/
    } else {
      printLongLine(cat("ROWSPAN=2>", sandboxHome, "</TD>", NULL), "", "\"");

    }

    if (showStatement < extHtmlStmt) {
      printLongLine(cat(
          "<TD ALIGN=CENTER VALIGN=TOP ROWSPAN=2><FONT SIZE=\"+3\" COLOR=",
          GREEN_TITLE_COLOR, "><B>", htmlTitle, "</B></FONT>", NULL), "", "\"");
    /@} else {@/
    } else if (showStatement < sandboxStmt) { /@ 29-Jul-2008 nm Sandbox stuff @/
      printLongLine(cat(
          "<TD ALIGN=CENTER VALIGN=TOP ROWSPAN=2><FONT SIZE=\"+3\" COLOR=",
          GREEN_TITLE_COLOR, "><B>", extHtmlTitle, "</B></FONT>", NULL), "", "\"");

    /@ 29-Jul-2008 nm Sandbox stuff @/
    } else {
      printLongLine(cat(
          "<TD ALIGN=CENTER VALIGN=TOP ROWSPAN=2><FONT SIZE=\"+3\" COLOR=",
          GREEN_TITLE_COLOR, "><B>",
          /@sandboxTitle,@/
          /@ 2-Aug-2009 nm - "Mathbox for <username>" mod @/
          localSandboxTitle,
           "</B></FONT>", NULL), "", "\"");

    }
    */


    if (texHeaderFlag) {  /* For HTML, 1 means to put prev/next links */
      /* Put Previous/Next links into web page */
      print2("    <TD ALIGN=RIGHT VALIGN=TOP WIDTH=\"25%s\">\n", "%");
      print2("      <FONT SIZE=-1 FACE=sans-serif>\n");
      /* Find the previous statement with a web page */
      j = 0;
      k = 0;
      for (i = showStatement - 1; i >= 1; i--) {
        if ((statement[i].type == (char)p_ ||
            statement[i].type == (char)a_ )
            /* Skip dummy "xxx..." statements.  We rely on lazy
               evaluation to prevent array bound overflow. */
            /* 16-Aug-2016 nm Took out this obsolete feature */
            /*
            && (statement[i].labelName[0] != 'x'
              || statement[i].labelName[1] != 'x'
              || statement[i].labelName[2] != 'x')
            */
            ) {
          j = i;
          break;
        }
      }
      if (j == 0) {
        k = 1; /* First statement flag */
        /* For the first statement, wrap to last one */
        for (i = statements; i >= 1; i--) {
          if ((statement[i].type == (char)p_ ||
              statement[i].type == (char)a_ )
              /* Skip dummy "xxx..." statements.  We rely on lazy
                 evaluation to prevent array bound overflow. */
              /* 16-Aug-2016 nm Took out this obsolete feature */
              /*
              && (statement[i].labelName[0] != 'x'
                || statement[i].labelName[1] != 'x'
                || statement[i].labelName[2] != 'x')
              */
              ) {
            j = i;
            break;
          }
        }
      }
      if (j == 0) bug(2314);
      print2("      <A HREF=\"%s.html\">\n",
          statement[j].labelName);
      if (!k) {
        print2("      &lt; Previous</A>&nbsp;&nbsp;\n");
      } else {
        print2("      &lt; Wrap</A>&nbsp;&nbsp;\n");
      }
      /* Find the next statement with a web page */
      j = 0;
      k = 0;
      for (i = showStatement + 1; i <= statements; i++) {
        if ((statement[i].type == (char)p_ ||
            statement[i].type == (char)a_ )
            /* Skip dummy "xxx..." statements.  We rely on lazy
               evaluation to prevent array bound overflow. */
            /* 16-Aug-2016 nm Took out this obsolete feature */
            /*
            && (statement[i].labelName[0] != 'x'
              || statement[i].labelName[1] != 'x'
              || statement[i].labelName[2] != 'x')
            */
            ) {
          j = i;
          break;
        }
      }
      if (j == 0) {
        k = 1; /* Last statement flag */
        /* For the last statement, wrap to first one */
        for (i = 1; i <= statements; i++) {
          if ((statement[i].type == (char)p_ ||
              statement[i].type == (char)a_ )
              /* Skip dummy "xxx..." statements.  We rely on lazy
                 evaluation to prevent array bound overflow. */
              /* 16-Aug-2016 nm Took out this obsolete feature */
              /*
              && (statement[i].labelName[0] != 'x'
                || statement[i].labelName[1] != 'x'
                || statement[i].labelName[2] != 'x')
              */
              ) {
            j = i;
            break;
          }
        }
      }
      if (j == 0) bug(2315);
      /*print2("<A HREF=\"%s.html\">Next</A></FONT>\n",*/
      if (!k) {
        print2("      <A HREF=\"%s.html\">Next &gt;</A>\n",
            statement[j].labelName);
      } else {
        print2("      <A HREF=\"%s.html\">Wrap &gt;</A>\n",
            statement[j].labelName);
      }

      print2("      </FONT><FONT FACE=sans-serif SIZE=-2>\n");

      /* ??? Is the closing </FONT> printed if there is no altHtml?
         This should be tested.  8/9/03 ndm */

      /* 15-Aug-04 nm Compute the theorem list page number.  ??? Temporarily
         we assume it to be 100 (hardcoded).  Todo: This should be fixed to use
         the same as the THEOREMS_PER_PAGE in WRITE THEOREMS (have a SET
         global variable in place of THEOREMS_PER_PAGE?) */
      i = ((statement[showStatement].pinkNumber - 1) / 100) + 1; /* Page # */
      /* let(&tmpStr, cat("mmtheorems", (i == 1) ? "" : str(i), ".html#", */
      /* 18-Jul-2015 nm - all thm pages now have page num after mmtheorems
         since mmtheorems.html is now just the table of contents */
      let(&tmpStr, cat("mmtheorems", str((double)i), ".html#",
          statement[showStatement].labelName, NULL)); /* Link to page/stmt */
      /*print2("      <BR><A HREF=\"%s\">Nearby theorems</A>\n",
            tmpStr);*/
      /* 3-May-2017 nm Break up lines w/ long labels to prevent bug 1505 */
      printLongLine(cat("      <BR><A HREF=\"", tmpStr,
            "\">Nearby theorems</A>", NULL), " ", " ");

      /* Print the GIF/Unicode Font choice, if directories are specified */
      /*
      if (htmlDir[0]) {

        if (altHtmlFlag) {
          print2("      <BR><A HREF=\"%s%s\">GIF version</A>\n",
                htmlDir, texFileName);

        } else {
          print2("      <BR><A HREF=\"%s%s\">Unicode version</A>\n",
                altHtmlDir, texFileName);

        }
      }
      */
      print2("      </FONT>\n");
      print2("    </TD>\n");
      print2("  </TR>\n");
      print2("  <TR>\n");
      print2("    <TD COLSPAN=2 ALIGN=LEFT VALIGN=TOP><FONT SIZE=-2\n");
      print2("      FACE=sans-serif>\n");
      print2("      <A HREF=\"../mm.html\">Mirrors</A>&nbsp; &gt;\n");
      print2("      &nbsp;<A HREF=\"../index.html\">Home</A>&nbsp; &gt;\n");
      print2("      &nbsp;<A HREF=\"%s\">%s</A>&nbsp; &gt;\n",
          (showStatement < extHtmlStmt ? htmlHomeHREF :
               (showStatement < sandboxStmt ? extHtmlHomeHREF :
               htmlHomeHREF)),
          (showStatement < extHtmlStmt ? htmlTitleAbbr :
               (showStatement < sandboxStmt ? extHtmlTitleAbbr :
               htmlTitleAbbr)));
      print2("      &nbsp;<A HREF=\"mmtheorems.html\">Th. List</A>&nbsp; &gt;\n");
      if (showStatement >= sandboxStmt) {
        print2("      &nbsp;<A HREF=\"mmtheorems.html#sandbox:bighdr\">\n");
        print2("      Mathboxes</A>&nbsp; &gt;\n");
      }
      print2("      &nbsp;%s\n",
          /* Strip off ".html" */
          left(texFileName, (long)strlen(texFileName) - 5));
      print2("      </FONT>\n");
      print2("    </TD>\n");
      print2("    <TD COLSPAN=1 ALIGN=RIGHT VALIGN=TOP>\n");
      print2("      <FONT SIZE=-2 FACE=sans-serif>\n");


      /* 3-Jun-2018 nm Add link to Thierry Arnoux's site */
      /* This was added to version 0.162 to become 0.162-thierry */
      /****** THIS IS TEMPORARY AND MAY BE CHANGED OR DELETED *********/
      printLongLine(cat("      <A HREF=\"",
          "http://metamath.tirix.org/", texFileName,
          "\">Structured version</A>&nbsp;&nbsp;", NULL), "", " ");
      /****** END OF LINKING TO THIERRY ARNOUX'S SITE ************/

      /* Print the GIF/Unicode Font choice, if directories are specified */
      if (htmlDir[0]) {

        if (altHtmlFlag) {
          print2("      <A HREF=\"%s%s\">GIF version</A>\n",
                htmlDir, texFileName);

        } else {
          print2("      <A HREF=\"%s%s\">Unicode version</A>\n",
                altHtmlDir, texFileName);

        }
      }

    } else { /* texHeaderFlag=0 for HTML means not to put prev/next links */
      print2("      </TD><TD ALIGN=RIGHT VALIGN=TOP\n");
      print2("       ><FONT FACE=sans-serif SIZE=-2>\n", "%");

      /* Print the GIF/Unicode Font choice, if directories are specified */
      if (htmlDir[0]) {
        print2("\n");
        if (altHtmlFlag) {
          print2("This is the Unicode version.<BR>\n");
          print2("<A HREF=\"%s%s\">Change to GIF version</A>\n",
              htmlDir, texFileName);
        } else {
          print2("This is the GIF version.<BR>\n");
          print2("<A HREF=\"%s%s\">Change to Unicode version</A>\n",
              altHtmlDir, texFileName);
        }
      }
      else {
        print2("&nbsp;\n");
      }

    }

    print2("      </FONT>\n");
    print2("    </TD>\n");
    print2("  </TR>\n");
    print2("</TABLE>\n");



    print2("<HR NOSHADE SIZE=1>\n");

  } /* htmlFlag */
  fprintf(texFilePtr, "%s", printString);
  outputToString = 0;
  let(&printString, "");

  /* Deallocate strings */
  let(&tmpStr, "");

} /* printTexHeader */

/* Prints an embedded comment in TeX or HTML.  The commentPtr must point to the first
   character after the "$(" in the comment.  The printout ends when the first
   "$)" or null character is encountered.   commentPtr must not be a temporary
   allocation.   htmlCenterFlag, if 1, means to center the HTML and add a
   "Description:" prefix. */
/* The output is printed to the global texFilePtr. */
/* Note: the global long "showStatement" is referenced to determine whether
   to read bibliography from mmset.html or mmhilbert.html (or other
   htmlBibliography or extHtmlBibliography file pair). */
/* Returns 1 if an error or warning message was printed */ /* 17-Nov-2015 */
flag printTexComment(vstring commentPtr, flag htmlCenterFlag,
    /* 17-Nov-2015 nm */
    long actionBits, /* see below */
        /* 13-Dec-2018 */
        /* Indicators for actionBits:
            #define ERRORS_ONLY 1 - just report errors, don't print output
            #define PROCESS_SYMBOLS 2
            #define PROCESS_LABELS 4
            #define ADD_COLORED_LABEL_NUMBER 8
            #define PROCESS_BIBREFS 16
            #define PROCESS_UNDERSCORES 32
            #define CONVERT_TO_HTML 64 - convert '<' to '&gt;' unless
                     <HTML>, </HTML> present
            #define METAMATH_COMMENT 128 - $) terminates string
            #define PROCESS_EVERYTHING PROCESS_SYMBOLS + PROCESS_LABELS \
             + ADD_COLORED_LABEL_NUMBER + PROCESS_BIBREFS \
             + PROCESS_UNDERSCORES + CONVERT_HTML + METAMATH_COMMENT \
        */
        /* 10-Dec-2018 nm - expanded meaning of errorsOnly for MARKUP commmand:
             2 = process as if in <HTML>...</HTML> preformatted mode but
                 don't strip <HTML>...</HTML> tags
             3 = same as 2, but convert ONLY math symbols
           (These new values were added instead of addina a new argument,
           so as not to have to modify ~60 other calls to this function) */

    flag noFileCheck)  /* 1 = ignore missing external files (gifs, bib, etc.) */
{
  vstring cmtptr; /* Not allocated */
  vstring srcptr; /* Not allocated */
  vstring lineStart; /* Not allocated */
  vstring tmpStr = "";
  vstring modeSection; /* Not allocated */
  vstring sourceLine = "";
  vstring outputLine = "";
  vstring tmp = "";
  flag textMode, mode, lastLineFlag, displayMode;
  vstring tmpComment = ""; /* Added 10/10/02 */
  /* 11/15/02 A comment section with <PRE>...</PRE> is formatted into a
              monospaced text table */
  /* 26-Dec-2011 nm - changed <PRE> to more general <HTML> */
  /*flag preformattedMode = 0; /@ HTML <PRE> preformatted mode @/ */
  flag preformattedMode = 0; /* HTML <HTML> preformatted mode */

  /* 10/10/02 For bibliography hyperlinks */
  vstring bibTag = "";
  vstring bibFileName = "";
  vstring bibFileContents = "";
  vstring bibFileContentsUpper = ""; /* Uppercase version */
  vstring bibTags = "";
  long pos1, pos2, htmlpos1, htmlpos2, saveScreenWidth;
  flag tmpMathMode; /* 15-Feb-2014 nm */

  /* Variables for converting ` ` and ~ to old $m,$n and $l,$n formats in
     order to re-use the old code */
  /* Note that DOLLAR_SUBST will replace the old $. */
  vstring cmt = "";
  vstring cmtMasked = ""; /* cmt with math syms blanked */ /* 20-Aug-2014 nm */
          /* 20-Oct-2018 - also mask ~ label */
  vstring tmpMasked = ""; /* tmp with math syms blanked */ /* 20-Aug-2014 nm */
  vstring tmpStrMasked = ""; /* tmpStr w/ math syms blanked */ /* 20-Aug-2014 */
  long i, clen;
  flag returnVal = 0; /* 1 means error/warning */ /* 17-Nov-2015 nm */

  /* 13-Dec-2018 nm */
  /* Internal flags derived from actionBits argument, for MARKUP command use */
  flag errorsOnly;
  flag processSymbols;
  flag processLabels;
  flag addColoredLabelNumber;
  flag processBibrefs;
  flag processUnderscores;
  flag convertToHtml;
  flag metamathComment;

  /* Assign local Booleans for actionBits mask */
  errorsOnly = (actionBits & ERRORS_ONLY ) != 0;
  processSymbols = (actionBits & PROCESS_SYMBOLS ) != 0;
  processLabels = (actionBits & PROCESS_LABELS ) != 0;
  addColoredLabelNumber = (actionBits & ADD_COLORED_LABEL_NUMBER ) != 0;
  processBibrefs = (actionBits & PROCESS_BIBREFS ) != 0;
  processUnderscores = (actionBits & PROCESS_UNDERSCORES ) != 0;
  convertToHtml = (actionBits & CONVERT_TO_HTML ) != 0;
  metamathComment = (actionBits & METAMATH_COMMENT ) != 0;

  /* We must let this procedure handle switching output to string mode */
  if (outputToString) bug(2309);
  /* The LaTeX (or HTML) file must be open */
  if (errorsOnly == 0) {   /* 17-Nov-2015 nm */
    if (!texFilePtr) bug(2321);
  }

  cmtptr = commentPtr;

  if (!texDefsRead) {
    return returnVal; /* TeX defs were not read (error was detected
                               and flagged to the user elsewhere) */
  }

  /* Convert line to the old $m..$n and $l..$n formats (using DOLLAR_SUBST
     instead of "$") - the old syntax is obsolete but we do this conversion
     to re-use some old code */
  if (metamathComment != 0) {
    i = instr(1, cmtptr, "$)");      /* If it points to source buffer */
    if (!i) i = (long)strlen(cmtptr) + 1;  /* If it's a stand-alone string */
  } else {
    i = (long)strlen(cmtptr) + 1;
  }
  let(&cmt, left(cmtptr, i - 1));

  /* All actions on cmt should be mirrored on cmdMasked, except that
     math symbols are replaced with blanks in cmdMasked */
  let(&cmtMasked, cmt); /* 20-Aug-2014 nm */


  /* This section is independent and can be removed without side effects */
  if (htmlFlag) {
    /* Convert special characters <, &, etc. to HTML entities */
    /* But skip converting math symbols inside ` ` */
    /* 26-Dec-2011 nm Detect preformatted HTML (this is crude, since it
       will apply to whole comment - perhaps fine-tune this later) */
    if (convertToHtml != 0) { /* 13-Dec-2018 nm */
      if (instr(1, cmt, "<HTML>") != 0) preformattedMode = 1;
    } else {
      preformattedMode = 1; /* For MARKUP command - don't convert HTML */
    }
    mode = 1; /* 1 normal, -1 math token */
    let(&tmp, "");
    let(&tmpMasked, "");
    while (1) {
      pos1 = 0;
      while (1) {
        pos1 = instr(pos1 + 1, cmt, "`");
        if (!pos1) break;
        if (cmt[pos1] == '`') {
          pos1++;  /* Skip `` escape */
          continue;
        }
        break;
      }
      if (!pos1) pos1 = (long)strlen(cmt) + 1;
      if (mode == 1 && preformattedMode == 0) {
        let(&tmpStr, "");
        /* asciiToTt() is where "<" is converted to "&lt;" etc. */
        tmpStr = asciiToTt(left(cmt, pos1));
        let(&tmpStrMasked, tmpStr);
      } else {
        let(&tmpStr, left(cmt, pos1));
        /* 20-Aug-2014 nm */
        if (mode == -1) { /* Math mode */
          /* Replace math symbols with spaces to prevent confusing them
             with markup in sections below */
          let(&tmpStrMasked, cat(space(pos1 - 1),
              mid(cmtMasked, pos1, 1), NULL));
        } else { /* Preformatted mode but not math mode */
          let(&tmpStrMasked, left(cmtMasked, pos1));
        }
      }
      let(&tmp, cat(tmp, tmpStr, NULL));
      let(&tmpMasked, cat(tmpMasked, tmpStrMasked, NULL));
      let(&cmt, right(cmt, pos1 + 1));
      let(&cmtMasked, right(cmtMasked, pos1 + 1));
      if (!cmt[0]) break;
      mode = (char)(-mode);
    }
    let(&cmt, tmp);
    let(&cmtMasked, tmpMasked);
    let(&tmpStr, ""); /* Deallocate */
    let(&tmpStrMasked, "");
  }


  /* 10/10/02 Add leading and trailing HTML markup to comment here
     (instead of in caller).  Also convert special characters. */
  if (htmlFlag) {
    /* This used to be done in mmcmds.c */
    if (htmlCenterFlag) {  /* Note:  this should be 0 in MARKUP command */
      let(&cmt, cat("<CENTER><TABLE><TR><TD ALIGN=LEFT><B>Description: </B>",
          cmt, "</TD></TR></TABLE></CENTER>", NULL));
      let(&cmtMasked,
          cat("<CENTER><TABLE><TR><TD ALIGN=LEFT><B>Description: </B>",
          cmtMasked, "</TD></TR></TABLE></CENTER>", NULL));
    }
  }


  /* Added Oct-20-2018 nm */
  /* Mask out _ (underscore) in labels so they won't become subscripts
     (reported by Benoit Jubin) */
  /* This section is independent and can be removed without side effects */
  if (htmlFlag != 0
      /* && processSymbols != 0*/ /* Mode 3 processes symbols only */
         /* (Actually we should do this all the time; this is the mask) */
      ) {
    pos1 = 0;
    while (1) {   /* Look for label start */
      pos1 = instr(pos1 + 1, cmtMasked, "~");
      if (!pos1) break;
      if (cmtMasked[pos1] == '~') {
        pos1++;  /* Skip ~~ escape */
        continue;
      }
      /* Skip whitespace after ~ */
      while (1) {
        if (cmtMasked[pos1] == 0) break;  /* End of line */
        if (isspace((unsigned char)(cmtMasked[pos1]))) {
          pos1++;
          continue;
        } else { /* Found start of label */
          break;
        }
      }
      /* Skip non-whitespace after ~ find end of label */
      while (1) {
        if (cmtMasked[pos1] == 0) break;  /* End of line */
        if (!(isspace((unsigned char)(cmtMasked[pos1])))) {
          if (cmtMasked[pos1] == '_') {
            /* Put an "?" in place of label character in mask */
            cmtMasked[pos1] = '?';
          }
          pos1++;
          continue;
        } else { /* Found end of label */
          break;
        }
      }  /* while (1) */
    } /* while (1) */
  } /* if htmlFlag */


  /* 5-Dec-03 Handle dollar signs in comments converted to LaTeX */
  /* This section is independent and can be removed without side effects */
  /* This must be done before the underscores below so subscript $'s */
  /* won't be converted to \$'s */
  if (!htmlFlag) {  /* LaTeX */

    /* MARKUP command doesn't handle LaTeX */
    /* 25-Jan-2019 nm - this gets set by PROCESS_EVERYTHING in many calls;
       it's not specific to MARKUP and not a bug. */
    /* if (metamathComment != 0) bug(2343); */

    pos1 = 0;
    while (1) {
      pos1 = instr(pos1 + 1, cmt, "$");
      if (!pos1) break;
      /*
      /@ Don't modify anything inside of <PRE>...</PRE> tags @/
      if (pos1 > instr(1, cmt, "<PRE>") && pos1 < instr(1, cmt, "</PRE>"))
        continue;
      */
      /* Don't modify anything inside of <HTML>...</HTML> tags */
      if (pos1 > instr(1, cmt, "<HTML>") && pos1 < instr(1, cmt, "</HTML>"))
        continue;
      let(&cmt, cat(left(cmt, pos1 - 1), "\\$",
          right(cmt, pos1 + 1), NULL));
      let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "\\$",
          right(cmtMasked, pos1 + 1), NULL));
      pos1 = pos1 + 1; /* Adjust for 2-1 extra chars in "let" above */
    } /* while (1) */
  }

  /* 15-Feb-2014 nm */
  /* 1-May-2017 nm - moved this section up to BEFORE the underscore handling
     below, so that "{\em...}" won't be converted to "\}\em...\}" */
  /* Convert any remaining special characters for LaTeX */
  /* This section is independent and can be removed without side effects */
  if (!htmlFlag) { /* i.e. LaTeX mode */
    /* At this point, the comment begins e.g "\begin{lemma}\label{lem:abc}" */
    pos1 = instr(1, cmt, "} ");
    if (pos1) {
      pos1++; /* Start after the "}" */
    } else {
      pos1 = 1; /* If not found, start from beginning of line */
    }
    pos2 = (long)strlen(cmt);
    tmpMathMode = 0;
    for (pos1 = pos1 + 0; pos1 <= pos2; pos1++) {
      /* Don't modify anything inside of math symbol strings
         (imperfect - only works if `...` is not split across lines?) */
      if (cmt[pos1 - 1] == '`') tmpMathMode = (flag)(1 - tmpMathMode);
      if (tmpMathMode) continue;
      if (pos1 > 1) {
        if (cmt[pos1 - 1] == '_' && cmt[pos1 - 2] == '$') {
          /* The _ is part of "$_{...}$" earlier conversion */
          continue;
        }
      }
      /* $%#{}&^\\|<>"~_ are converted by asciiToTt() */
      /* Omit \ and $ since they be part of an earlier converston */
      /* Omit ~ since it is part of label ref */
      /* Omit " since it legal */
      /* Because converting to \char` causes later math mode problems due to `,
         we change |><_ to /)(- (an ugly workaround) */
      switch(cmt[pos1 - 1]) {
        case '|': cmt[pos1 - 1] = '/'; break;
        case '<': cmt[pos1 - 1] = '{'; break;
        case '>': cmt[pos1 - 1] = '}'; break;
        case '_': cmt[pos1 - 1] = '-'; break;
      }
      if (strchr("%#{}&^|<>_", cmt[pos1 - 1]) != NULL) {
        let(&tmpStr, "");
        tmpStr = asciiToTt(chr(cmt[pos1 - 1]));
        let(&cmt, cat(left(cmt, pos1 - 1), tmpStr,
            right(cmt, pos1 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), tmpStr,
            right(cmtMasked, pos1 + 1), NULL));
        pos1 += (long)strlen(tmpStr) - 1;
        pos2 += (long)strlen(tmpStr) - 1;
      }
    } /* Next pos1 */
  } /* if (!htmlFlag) */

  /* 10-Oct-02 Handle underscores in comments converted to HTML:  Convert _abc_
     to <I>abc</I> for book titles, etc.; convert a_n to a<SUB>n</SUB> for
     subscripts */
  /* 5-Dec-03 Added LaTeX handling */
  /* This section is independent and can be removed without side effects */
  if (htmlFlag != 0     /* 5-Dec-03 */
      && processUnderscores != 0) {  /* 13-Dec-2018 nm */
    pos1 = 0;
    while (1) {
      /* pos1 = instr(pos1 + 1, cmt, "_"); */
      /* 20-Aug-2014 nm Only look at non-math part of comment */
      pos1 = instr(pos1 + 1, cmtMasked, "_");
      if (!pos1) break;
      /*
      /@ Don't modify anything inside of <PRE>...</PRE> tags @/
      if (pos1 > instr(1, cmt, "<PRE>") && pos1 < instr(1, cmt, "</PRE>"))
        continue;
      */
      /* Don't modify anything inside of <HTML>...</HTML> tags */
      if (pos1 > instr(1, cmt, "<HTML>") && pos1 < instr(1, cmt, "</HTML>"))
        continue;
      /* 23-Jul-2006 nm Don't modify anything inside of math symbol strings
         (imperfect - only works if `...` is not split across lines) */
      /* 20-Aug-2014 nm No longer necessary since we now use cmtMasked */
      /*
      if (pos1 > instr(1, cmt, "`") && pos1 < instr(instr(1, cmt, "`") + 1,
          cmt, "`"))
        continue;
      */

      /* 5-Apr-2007 nm Don't modify external hyperlinks containing "_" */
      pos2 = pos1 - 1;
      while (1) { /* Get to previous whitespace */
        if (pos2 == 0 || isspace((unsigned char)(cmt[pos2]))) break;
        pos2--;
      }
      if (!strcmp(mid(cmt, pos2 + 2, 7), "http://")) {
        continue;
      }
      /* 20-Aug-2014 nm Add https */
      if (!strcmp(mid(cmt, pos2 + 2, 8), "https://")) {
        continue;
      }
      /* 20-Oct-2018 nm Add mm */
      if (!strcmp(mid(cmt, pos2 + 2, 2), "mm")) {
        continue;
      }

      /* Opening "_" must be <whitespace>_<alphanum> for <I> tag */
      if (pos1 > 1) {
        /* Check for not whitespace and not opening punctuation */
        if (!isspace((unsigned char)(cmt[pos1 - 2]))
            && strchr(OPENING_PUNCTUATION, cmt[pos1 - 2]) == NULL) {
          /* Check for not whitespace and not closing punctuation */
          if (!isspace((unsigned char)(cmt[pos1]))
            && strchr(CLOSING_PUNCTUATION, cmt[pos1]) == NULL) {

            /* 28-Sep-03 - Added subscript handling */
            /* Found <nonwhitespace>_<nonwhitespace> - assume subscript */
            /* Locate the whitepace (or end of string) that closes subscript */
            /* Note:  This algorithm is not perfect in that the subscript
               is assumed to end at closing punctuation, which theoretically
               could be part of the subscript itself, such as a subscript
               with a comma in it. */
            pos2 = pos1 + 1;
            while (1) {
              if (!cmt[pos2]) break; /* End of string */
              /* Look for whitespace or closing punctuation */
              if (isspace((unsigned char)(cmt[pos2]))
                  || strchr(OPENING_PUNCTUATION, cmt[pos2]) != NULL
                  || strchr(CLOSING_PUNCTUATION, cmt[pos2]) != NULL) break;
              pos2++; /* Move forward through subscript */
            }
            pos2++; /* Adjust for left, seg, etc. that start at 1 not 0 */
            if (htmlFlag) {  /* HTML */
              /* Put <SUB>...</SUB> around subscript */
              let(&cmt, cat(left(cmt, pos1 - 1),
                  "<SUB><FONT SIZE=\"-1\">",
                  seg(cmt, pos1 + 1, pos2 - 1), /* Skip (delete) "_" */
                  "</FONT></SUB>", right(cmt, pos2), NULL));
              let(&cmtMasked, cat(left(cmtMasked, pos1 - 1),
                  "<SUB><FONT SIZE=\"-1\">",
                  seg(cmtMasked, pos1 + 1, pos2 - 1), /* Skip (delete) "_" */
                  "</FONT></SUB>", right(cmtMasked, pos2), NULL));
              pos1 = pos2 + 33; /* Adjust for 34-1 extra chars in "let" above */
            } else {  /* LaTeX */
              /* Put _{...} around subscript */
              let(&cmt, cat(left(cmt, pos1 - 1), "$_{",
                  seg(cmt, pos1 + 1, pos2 - 1),  /* Skip (delete) "_" */
                  "}$", right(cmt, pos2), NULL));
              let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "$_{",
                  seg(cmtMasked, pos1 + 1, pos2 - 1),  /* Skip (delete) "_" */
                  "}$", right(cmtMasked, pos2), NULL));
              pos1 = pos2 + 4; /* Adjust for 5-1 extra chars in "let" above */
            }
            continue;
            /* 23-Sep-03 - End of subscript handling */

          } else {
            /* Found <nonwhitespace>_<whitespace> - not an opening "_" */
            /* Do nothing in this case */
            continue;
          }
        }
      }
      if (!isalnum((unsigned char)(cmt[pos1]))) continue;
      /* pos2 = instr(pos1 + 1, cmt, "_"); */
      /* 20-Aug-2014 nm Only look at non-math part of comment */
      pos2 = instr(pos1 + 1, cmtMasked, "_");
      if (!pos2) break;
      /* Closing "_" must be <alphanum>_<nonalphanum> */
      if (!isalnum((unsigned char)(cmt[pos2 - 2]))) continue;
      if (isalnum((unsigned char)(cmt[pos2]))) continue;
      if (htmlFlag) {  /* HTML */
        let(&cmt, cat(left(cmt, pos1 - 1), "<I>",
            seg(cmt, pos1 + 1, pos2 - 1),
            "</I>", right(cmt, pos2 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "<I>",
            seg(cmtMasked, pos1 + 1, pos2 - 1),
            "</I>", right(cmtMasked, pos2 + 1), NULL));
        pos1 = pos2 + 5; /* Adjust for 7-2 extra chars in "let" above */
      } else {  /* LaTeX */
        let(&cmt, cat(left(cmt, pos1 - 1), "{\\em ",
            seg(cmt, pos1 + 1, pos2 - 1),
            "}", right(cmt, pos2 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "{\\em ",
            seg(cmtMasked, pos1 + 1, pos2 - 1),
            "}", right(cmtMasked, pos2 + 1), NULL));
        pos1 = pos2 + 4; /* Adjust for 6-2 extra chars in "let" above */
      }
    }
  }

  /* 1-May-2017 nm */
  /* Convert opening double quote to `` for LaTeX */
  /* This section is independent and can be removed without side effects */
  if (!htmlFlag) { /* If LaTeX mode */
    i = 1; /* Even/odd counter: 1 = left quote, 0 = right quote */
    pos1 = 0;
    while (1) {
      /* cmtMasked has math symbols blanked */
      pos1 = instr(pos1 + 1, cmtMasked, "\"");
      if (pos1 == 0) break;
      if (i == 1) {
        /* Warning:  "`" needs to be escaped (i.e. repeated) to prevent it
           from being treated as a math symbol delimiter below.  So "````"
           will become "``" in the LaTeX output. */
        let(&cmt, cat(left(cmt, pos1 - 1), "````",
            right(cmt, pos1 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "````",
            right(cmtMasked, pos1 + 1), NULL));
      }
      i = 1 - i; /* Count to next even or odd */
    }
  }

  /* 10/10/02 Put bibliography hyperlinks in comments converted to HTML:
        [Monk2] becomes <A HREF="mmset.html#monk2>[Monk2]</A> etc. */
  /* This section is independent and can be removed without side effects */
  if (htmlFlag
      && processBibrefs != 0 /* 13-Dec-2018 nm */
      ) {
    /* Assign local tag list and local HTML file name */
    if (showStatement < extHtmlStmt) {
      let(&bibTags, htmlBibliographyTags);
      let(&bibFileName, htmlBibliography);
    /*} else {*/
    } else if (showStatement < sandboxStmt) { /* 29-Jul-2008 nm Sandbox stuff */
      let(&bibTags, extHtmlBibliographyTags);
      let(&bibFileName, extHtmlBibliography);

    /* 29-Jul-2008 nm Sandbox stuff */
    } else {
      let(&bibTags, htmlBibliographyTags);  /* Go back to Mm Prf Explorer */
      let(&bibFileName, htmlBibliography);

    }
    if (bibFileName[0]) {
      /* The user specified a bibliography file in the xxx.mm $t comment
         (otherwise we don't do anything) */
      pos1 = 0;
      while (1) {
        /* Look for any bibliography tags to convert to hyperlinks */
        /* The biblio tag should be in brackets e.g. "[Monk2]" */
        /* pos1 = instr(pos1 + 1, cmt, "["); */
        /* 20-Aug-2014 nm Only look at non-math part of comment */
        pos1 = instr(pos1 + 1, cmtMasked, "[");
        if (!pos1) break; /* 3-Feb-2019 nm Moved this to before block below */

        /* 9-Dec-2018 nm */
        /* Escape a double [[ */
        if (cmtMasked[pos1] == '[') {  /* This is the char after "[" above */
          /* Remove the first "[" */
          let(&cmt, cat(left(cmt, pos1 - 1),
              right(cmt, pos1 + 1), NULL));
          let(&cmtMasked, cat(left(cmtMasked, pos1 - 1),
              right(cmtMasked, pos1 + 1), NULL));
          /* The pos1-th position (starting at 1) is now the "[" that remains */
          continue;
        }

        /* pos2 = instr(pos1 + 1, cmt, "]"); */
        /* 20-Aug-2014 nm Only look at non-math part of comment */
        pos2 = instr(pos1 + 1, cmtMasked, "]");
        if (!pos2) break;

        /* 30-Jun-2011 nm */
        /* See if we are in math mode */
        /* clen = (long)strlen(cmt); */ /* 18-Sep-2013 never used */
        /* 20-Aug-2014 nm Deleted since cmtMasked takes care of it */
        /*
        mode = 0; /@ 0 = normal, 1 = math @/
        for (i = 0; i < pos1; i++) {
          if (cmt[i] == '`' && cmt[i + 1] != '`') {
            mode = (char)(1 - mode);
          }
        }
        if (mode) continue; /@ Don't process [...] brackets in math mode @/
        */

        /* let(&bibTag, seg(cmt, pos1, pos2)); */
        /* 20-Aug-2014 nm Get bibTag from cmtMasked as extra precaution */
        let(&bibTag, seg(cmtMasked, pos1, pos2));
        /* There should be no white space in the tag */
        if ((signed)(strcspn(bibTag, " \n\r\t\f")) < pos2 - pos1 + 1) continue;
        /* OK, we have a good tag.  If the file with bibliography has not been
           read in yet, let's do so here for error-checking. */

        /* Start of error-checking */
        if (noFileCheck == 0) {  /* 17-Nov-2015 nm */
          if (!bibTags[0]) {
            /* The bibliography file has not be read in yet. */
            let(&bibFileContents, "");
            if (errorsOnly == 0) { /* 17-Nov-2015 nm */
              print2("Reading HTML bibliographic tags from file \"%s\"...\n",
                  bibFileName);
            }
            bibFileContents = readFileToString(bibFileName, 0,
                &i /* charCount; not used here */); /* 31-Dec-2017 nm */
            if (!bibFileContents) {
              /* The file was not found or had some problem (use verbose mode = 1
                 in 2nd argument of readFileToString for debugging). */
              printLongLine(cat("?Warning:  Couldn't open or read the file \"",
                  bibFileName,
                  "\".  The bibliographic hyperlinks will not be checked for",
                  " correctness.  The first one is \"", bibTag,
                  "\" in the comment for statement \"",
                  statement[showStatement].labelName, "\".",
                  NULL), "", " ");
              returnVal = 1; /* Error/warning printed */ /* 17-Nov-2015 nm */
              bibFileContents = ""; /* Restore to normal string */
              let(&bibTags, "?"); /* Assign to a nonsense tag that won't match
                  but tells us an attempt was already made to read the file */
            } else {
              /* Note: In an <A NAME=...> tag, HTML is case-insensitive for A and
                 NAME but case-sensitive for the token after the = */
              /* Strip all whitespace */
              let(&bibFileContents, edit(bibFileContents, 2));
              /* Uppercase version for HTML tag search */
              let(&bibFileContentsUpper, edit(bibFileContents, 32));
              htmlpos1 = 0;
              while (1) {  /* Look for all <A NAME=...></A> HTML tags */
                htmlpos1 = instr(htmlpos1 + 1, bibFileContentsUpper, "<ANAME=");
                /* Note stripped space after <A... - not perfectly robust but
                   good enough if HTML file is legal since <ANAME is not an HTML
                   tag (let's not get into a regex discussion though...) */
                if (!htmlpos1) break;
                htmlpos1 = htmlpos1 + 7;  /* Point ot beginning of tag name */
                /* Extract tag, ignoring any surrounding quotes */
                if (bibFileContents[htmlpos1 - 1] == '\''
                    || bibFileContents[htmlpos1 - 1] == '"') htmlpos1++;
                htmlpos2 = instr(htmlpos1, bibFileContents, ">");
                if (!htmlpos2) break;
                htmlpos2--; /* Move to character before ">" */
                if (bibFileContents[htmlpos2 - 1] == '\''
                    || bibFileContents[htmlpos2 - 1] == '"') htmlpos2--;
                if (htmlpos2 <= htmlpos1) continue;  /* Ignore bad HTML syntax */
                let(&tmp, cat("[",
                    seg(bibFileContents, htmlpos1, htmlpos2), "]", NULL));
                /* Check if tag is already in list */
                if (instr(1, bibTags, tmp)) {
                  printLongLine(cat("?Error: There two occurrences of",
                      " bibliographic reference \"",
                      seg(bibFileContents, htmlpos1, htmlpos2),
                      "\" in the file \"", bibFileName, "\".", NULL), "", " ");
                  returnVal = 1; /* Error/warning printed */ /* 17-Nov-2015 nm */
                }
                /* Add tag to tag list */
                let(&bibTags, cat(bibTags, tmp, NULL));
              } /* end while */
              if (!bibTags[0]) {
                /* No tags found; put dummy partial tag meaning "file read" */
                let(&bibTags, "[");
              }
            } /* end if (!bibFIleContents) */
          } /* end if (noFileCheck == 0) */
          /* Assign to permanent tag list for next time */
          if (showStatement < extHtmlStmt) {
            let(&htmlBibliographyTags, bibTags);
          /*} else {*/
          } else if (showStatement < sandboxStmt) {
                                             /* 29-Jul-2008 nm Sandbox stuff */
            let(&extHtmlBibliographyTags, bibTags);

          /* 29-Jul-2008 nm Sandbox stuff */
          } else {
            let(&htmlBibliographyTags, bibTags);

          }
          /* Done reading in HTML file with bibliography */
        } /* end if (!bibTags[0]) */
        /* See if the tag we found is in the bibliography file */
        if (bibTags[0] == '[') {
          /* We have a tag list from the bibliography file */
          if (!instr(1, bibTags, bibTag)) {
            printLongLine(cat("?Error: The bibliographic reference \"", bibTag,
                "\" in statement \"", statement[showStatement].labelName,
                "\" was not found as an <A NAME=\"",
                seg(bibTag, 2, pos2 - pos1),
                "\"></A> anchor in the file \"", bibFileName, "\".", NULL),
                "", " ");
            returnVal = 1; /* Error/warning printed */ /* 17-Nov-2015 nm */
          }
        }
        /* End of error-checking */

        /* Make an HTML reference for the tag */
        let(&tmp, cat("[<A HREF=\"",
            bibFileName, "#", seg(bibTag, 2, pos2 - pos1), "\">",
            seg(bibTag, 2, pos2 - pos1), "</A>]", NULL));
        let(&cmt, cat(left(cmt, pos1 - 1), tmp, right(cmt,
            pos2 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), tmp, right(cmtMasked,
            pos2 + 1), NULL));
        pos1 = pos1 + (long)strlen(tmp) - (long)strlen(bibTag); /* Adjust comment position */
      } /* end while(1) */
    } /* end if (bibFileName[0]) */
  } /* end of if (htmlFlag) */
  /* 10/10/02 End of bibliography hyperlinks */

  /* All actions on cmt should be mirrored on cmdMasked, except that
     math symbols are replaced with blanks in cmdMasked */
  if (strlen(cmt) != strlen(cmtMasked)) bug(2334); /* Should be in sync */

  /* Starting here, we no longer use cmtMasked, so syncing it with cmt
     isn't important anymore. */

  clen = (long)strlen(cmt);
  mode = 'n';
  for (i = 0; i < clen; i++) {
    if (cmt[i] == '`') {
      if (cmt[i + 1] == '`') {
        if (processSymbols != 0) { /* 13-Dec-2018 nm */
          /* Escaped ` = actual ` */
          let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
          clen--;
        }
      } else {
        /* 13-Dec-2018 nm We will still enter and exit math mode when
           processSymbols=0 so as to skip ~ in math symbols.  However,
           we don't insert the "DOLLAR_SUBST mode" so that later on
           it will look like normal text */
        /* Enter or exit math mode */
        if (mode != 'm') {
          mode = 'm';
        } else {
          mode = 'n';
        }

        if (processSymbols != 0) {  /* 13-Dec-2018 nm */
          let(&cmt, cat(left(cmt, i), chr(DOLLAR_SUBST) /*$*/, chr(mode),
              right(cmt, i+2), NULL));
          clen++;
          i++;
        }

        /* 10/10/02 */
        /* If symbol is preceded by opening punctuation and a space, take out
           the space so it looks better. */
        if (mode == 'm'
            && processSymbols != 0 /* 13-Dec-2018 nm */
            ) {
          let(&tmp, mid(cmt, i - 2, 2));
          if (!strcmp("( ", tmp)) {
            let(&cmt, cat(left(cmt, i - 2), right(cmt, i), NULL));
            clen = clen - 1;
          }
          /* We include quotes since symbols are often enclosed in them. */
          let(&tmp, mid(cmt, i - 8, 8));
          if (!strcmp("&quot; ", right(tmp, 2))
              && strchr("( ", tmp[0]) != NULL) {
            let(&cmt, cat(left(cmt, i - 2), right(cmt, i), NULL));
            clen = clen - 1;
          }
          let(&tmp, "");
        }
        /* If symbol is followed by a space and closing punctuation, take out
           the space so it looks better. */
        if (mode == 'n'
            && processSymbols != 0 /* 13-Dec-2018 nm */
            ) {
          /* (Why must it be i + 2 here but i + 1 in label version below?
             Didn't investigate but seems strange.) */
          let(&tmp, mid(cmt, i + 2, 2));
          if (tmp[0] == ' ' && strchr(CLOSING_PUNCTUATION, tmp[1]) != NULL) {
            let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
            clen = clen - 1;
          }
          /* We include quotes since symbols are often enclosed in them. */
          let(&tmp, mid(cmt, i + 2, 8));
          if (strlen(tmp) < 8)
              let(&tmp, cat(tmp, space(8 - (long)strlen(tmp)), NULL));
          if (!strcmp(" &quot;", left(tmp, 7))
              && strchr(CLOSING_PUNCTUATION, tmp[7]) != NULL) {
            let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
            clen = clen - 1;
          }
          let(&tmp, "");
        }

      }
    }
    if (cmt[i] == '~' && mode != 'm') {
      if (cmt[i + 1] == '~' /* Escaped ~ */
          || processLabels == 0 /* 13-Dec-2018 nm */
          ) {
        if (processLabels != 0) {  /* 13-Dec-2018 nm */
          /* Escaped ~ = actual ~ */
          let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
          clen--;
        }
      } else {
        /* Enter or exit label mode */
        if (mode != 'l') {
          mode = 'l';
          /* 9/5/99 - If there is whitespace after the ~, then remove
             all whitespace immediately after the ~ to join the ~ to
             the label.  This enhances the Metamath syntax so that
             whitespace is now allowed between the ~ and the label, which
             makes it easier to do global substitutions of labels in a
             text editor. */
          while (isspace((unsigned char)(cmt[i + 1])) && clen > i + 1) {
            let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
            clen--;
          }
        } else {
          /* This really should never happen */
          /* 1-Oct-04 If you see this bug, the most likely cause is a tilde
             character in a comment that does not prefix a label or hyperlink.
             The most common problem is the "~" inside a hyperlink that
             specifies a user's directory.  To fix it, use a double tilde
             "~~" to escape it, which will become a single tilde on output.
             (At some future point this bug should be converted to a
             meaningful error message that doesn't abort the program.) */
          /* bug(2310); */
          /* 2-Oct-2015 nm Changed to error message */
          outputToString = 0;
          printLongLine(cat("?Warning:  There is a \"~\" inside of a label",
              " in the comment of statement \"",
              statement[showStatement].labelName,
              "\".  Use \"~~\" to escape \"~\" in an http reference.",
              NULL), "", " ");
          returnVal = 1; /* Error/warning printed */ /* 17-Nov-2015 nm */
          outputToString = 1;
          mode = 'n';
        }
        let(&cmt, cat(left(cmt, i), chr(DOLLAR_SUBST) /*$*/, chr(mode),
            right(cmt, i+2), NULL));
        clen++;
        i++;

        /* 10/10/02 */
        /* If label is preceded by opening punctuation and space, take out
           the space so it looks better. */
        let(&tmp, mid(cmt, i - 2, 2));
        /*printf("tmp#%s#\n",tmp);*/
        if (!strcmp("( ", tmp) || !strcmp("[ ", tmp)) {
          let(&cmt, cat(left(cmt, i - 2), right(cmt, i), NULL));
          clen = clen - 1;
        }
        let(&tmp, "");

      }
    }

    if (processLabels == 0 && mode == 'l') {
      /* We should have prevented it from ever getting into label mode */
      bug(2344); /* 13-Dec-2018 nm */
    }

    if ((isspace((unsigned char)(cmt[i]))
            || cmt[i] == '<') /* 17-Nov-2012 nm If the label ends the comment,
               "</TD>" with no space will be appended before this section. */
        && mode == 'l') {
      /* Whitespace exits label mode */
      mode = 'n';
      let(&cmt, cat(left(cmt, i), chr(DOLLAR_SUBST) /*$*/, chr(mode),
          right(cmt, i+1), NULL));
      clen = clen + 2;
      i = i + 2;

      /* 10/10/02 */
      /* If label is followed by space and end punctuation, take out the space
         so it looks better. */
      let(&tmp, mid(cmt, i + 1, 2));
      if (tmp[0] == ' ' && strchr(CLOSING_PUNCTUATION, tmp[1]) != NULL) {
        let(&cmt, cat(left(cmt, i), right(cmt, i + 2), NULL));
        clen = clen - 1;
      }
      let(&tmp, "");

    }
    /* clen should always remain comment length - do a sanity check here */
    if ((signed)(strlen(cmt)) != clen) {
      bug(2311);
    }
  } /* Next i */
  /* End convert line to the old $m..$n and $l..$n */


  /* 29-May-2017 nm */
  /* Put <HTML> and </HTML> at beginning of line so preformattedMode won't
     be switched on or off in the middle of processing a line */
  /* This also fixes the problem where multiple <HTML>...</HTML> on one
     line aren't all removed in HTML output, causing w3c validation errors */
  /* Note:  "Q<HTML><sup>2</sup></HTML>." will have a space around "2" because
     of this fix.  Instead use "<HTML>Q<sup>2</sup>.</HTML>" or just "Q^2." */
  pos1 = -1; /* So -1 + 2 = 1 = start of string for instr() */
  while (1) {
    pos1 = instr(pos1 + 2/*skip new \n*/, cmt, "<HTML>");
    if (pos1 == 0
      || convertToHtml == 0 /* Don't touch <HTML> in MARKUP command */
                                 /* 13-Dec-2018 nm */
      ) break;

    /* If <HTML> begins a line (after stripping spaces), don't put a \n so
       that we don't trigger new paragraph mode */
    let(&tmpStr, edit(left(cmt, pos1 - 1), 2/*discard spaces & tabs*/));
    i = (long)strlen(tmpStr);
    if (i == 0) continue;
    if (tmpStr[i - 1] == '\n') continue;

    let(&cmt, cat(left(cmt, pos1 - 1), "\n", right(cmt, pos1), NULL));
  }
  pos1 = -1; /* So -1 + 2 = 1 = start of string for instr() */
  while (1) {
    pos1 = instr(pos1 + 2/*skip new \n*/, cmt, "</HTML>");
    if (pos1 == 0
      || convertToHtml == 0 /* Don't touch </HTML> in MARKUP command */
                                 /* 13-Dec-2018 nm */
      ) break;

    /* If </HTML> begins a line (after stripping spaces), don't put a \n so
       that we don't trigger new paragraph mode */
    let(&tmpStr, edit(left(cmt, pos1 - 1), 2/*discard spaces & tabs*/));
    i = (long)strlen(tmpStr);
    if (i == 0) continue;
    if (tmpStr[i - 1] == '\n') continue;

    let(&cmt, cat(left(cmt, pos1 - 1), "\n", right(cmt, pos1), NULL));
  }


  cmtptr = cmt; /* cmtptr is for scanning cmt */

  outputToString = 1; /* Redirect print2 and printLongLine to printString */
  /*let(&printString, "");*/ /* May have stuff to be printed 7/4/98 */

  while (1) {
    /* Get a "line" of text, up to the next new-line.  New-lines embedded
       in $m and $l sections are ignored, so that these sections will not
       be dangling. */
    lineStart = cmtptr;
    textMode = 1;
    lastLineFlag = 0;
    while (1) {
      if (cmtptr[0] == 0) {
        lastLineFlag = 1;
        break;
      }
      if (cmtptr[0] == '\n' && textMode) break;
      /* if (cmtptr[0] == '$') { */
      if (cmtptr[0] == DOLLAR_SUBST) {  /* 14-Feb-2014 nm */
        if (cmtptr[1] == ')') {
          bug(2312); /* Obsolete (should never happen) */
          lastLineFlag = 1;
          break;
        }
      }
      if (cmtptr[0] == DOLLAR_SUBST /*'$'*/) {
        cmtptr++;
        if (cmtptr[0] == 'm') textMode = 0; /* Math mode */
        if (cmtptr[0] == 'l') textMode = 0; /* Label mode */
        if (cmtptr[0] == 'n') textMode = 1; /* Normal mode */
      }
      cmtptr++;
    }
    let(&sourceLine, space(cmtptr - lineStart));
    memcpy(sourceLine, lineStart, (size_t)(cmtptr - lineStart));
    cmtptr++;  /* Get past new-line to prepare for next line's scan */

    /* If the line contains only math mode text, use TeX display mode. */
    displayMode = 0;
    let(&tmpStr, edit(sourceLine, 8 + 128)); /* Trim spaces */
    if (!strcmp(right(tmpStr, (long)strlen(tmpStr) - 1), cat(chr(DOLLAR_SUBST), "n",
        NULL))) let(&tmpStr, left(tmpStr, (long)strlen(tmpStr) - 2)); /* Strip $n */
    srcptr = tmpStr;
    modeSection = getCommentModeSection(&srcptr, &mode);
    let(&modeSection, ""); /* Deallocate */
    if (mode == 'm') {
      modeSection = getCommentModeSection(&srcptr, &mode);
      let(&modeSection, ""); /* Deallocate */
      /* 9/9/99  displayMode is obsolete.  Because it depends on a manual
         user edit, by default it will create a LaTeX error. Turn it off. */
      /*if (mode == 0) displayMode = 1;*/ /* No text after math mode text */
    }
    let(&tmpStr, ""); /* Deallocate */


    /* Convert all sections of the line to text, math, or labels */
    let(&outputLine, "");
    srcptr = sourceLine;
    while (1) {
      modeSection = getCommentModeSection(&srcptr, &mode);
      if (!mode) break; /* Done */
      let(&modeSection, right(modeSection, 3)); /* Remove mode-change command */
      switch (mode) {
        case 'n': /* Normal text */
          let(&outputLine, cat(outputLine, modeSection, NULL));
          break;
        case 'l': /* Label mode */

          if (processLabels == 0) { /* 13-Dec-2018 nm */
            /* Labels should be treated as normal text */
            bug(2345);
          }

          let(&modeSection, edit(modeSection, 8 + 128 + 16));  /* Discard
                      leading and trailing blanks; reduce spaces to one space */
          let(&tmpStr, "");
          tmpStr = asciiToTt(modeSection);
          if (!tmpStr[0]) { /* Can't be blank */
            /* bug(2313); */ /* 2-Oct-2015 nm Commented out */

            /* 2-Oct-2015 nm */
            /* This can happen if ~ is followed by ` (start of math string) */
            outputToString = 0;
            printLongLine(cat("?Error:  There is a \"~\" with no label",
                " in the comment of statement \"",
                statement[showStatement].labelName,
                "\".  Check that \"`\" inside of a math symbol is",
                " escaped with \"``\".",
                NULL), "", " ");
            returnVal = 1; /* Error/warning printed */ /* 17-Nov-2015 nm */
            outputToString = 1;

          }

          /* 10/10/02 - obsolete */
          /* 9/5/99 - Make sure the label in a comment corresponds to a real
             statement so we won't have broken HTML links (or misleading
             LaTeX information) - since label references in comments are
             rare, we just do a linear scan instead of binary search */
          /******* 10/10/02
          for (i = 1; i <= statements; i++) {
            if (!strcmp(statement[i].labelName, tmpStr)) break;
          }
          if (i > statements) {
            outputToString = 0;
            printLongLine(cat("?Error: Statement \"", tmpStr,
               "\" (referenced in comment) does not exist.", NULL), "", " ");
            outputToString = 1;
            returnVal = 1;
          }
          *******/

          if (!strcmp("http://", left(tmpStr, 7))
              || !strcmp("https://", left(tmpStr, 8)) /* 20-Aug-2014 nm */
              || !strcmp("mm", left(tmpStr, 2)) /* 20-Oct-2018 nm */
              ) {
            /* 4/13/04 nm - If the "label" begins with 'http://', then
               assume it is an external hyperlink and not a real label.
               This is kind of a syntax kludge but it is easy to do.
               20-Oct-2018 nm - Added starting with 'mm', which is illegal
               for set.mm labels - e.g. mmtheorems.html#abc */

            if (htmlFlag) {
              let(&outputLine, cat(outputLine, "<A HREF=\"", tmpStr,
                  "\">", tmpStr, "</A>", tmp, NULL));
            } else {

              /* 1-May-2017 nm */
              /* Generate LaTeX version of the URL */
              i = instr(1, tmpStr, "\\char`\\~");
              /* The url{} function automatically converts ~ to LaTeX */
              if (i != 0) {
                let(&tmpStr, cat(left(tmpStr, i - 1), right(tmpStr, i + 7),
                    NULL));
              }
              let(&outputLine, cat(outputLine, "\\url{", tmpStr,
                  "}", tmp, NULL));

            }
          } else {
            /* 10/10/02 Do binary search through just $a's and $p's (there
               are no html pages for local labels) */
            i = lookupLabel(tmpStr);
            if (i < 0) {
              outputToString = 0;
              printLongLine(cat("?Warning: The label token \"", tmpStr,
                  "\" (referenced in comment of statement \"",
                  statement[showStatement].labelName,
                  "\") is not a $a or $p statement label.", NULL), "", " ");
              outputToString = 1;
              returnVal = 1; /* Error/warning printed */ /* 17-Nov-2015 nm */
            }

            if (!htmlFlag) {
              let(&outputLine, cat(outputLine, "{\\tt ", tmpStr,
                 "}", NULL));
            } else {
              let(&tmp, "");
              if (addColoredLabelNumber != 0) { /* 13-Dec-2018 nm */
                /* When the error above occurs, i < 0 will cause pinkHTML()
                   to issue "(future)" for pleasant readability */
                tmp = pinkHTML(i);
              }
              if (i < 0) {
                /* Error output - prevent broken link */
                let(&outputLine, cat(outputLine, "<FONT COLOR=blue ",
                    ">", tmpStr, "</FONT>", tmp, NULL));
              } else {
                /* Normal output - put hyperlink to the statement */
                let(&outputLine, cat(outputLine, "<A HREF=\"", tmpStr,
                    ".html\">", tmpStr, "</A>", tmp, NULL));
              }
            }
          } /* if (!strcmp("http://", left(tmpStr, 7))) ... else */
          let(&tmpStr, ""); /* Deallocate */
          break;
        case 'm': /* Math mode */

          if (processSymbols == 0) { /* 13-Dec-2018 nm */
            /* Math symbols should be treated as normal text */
            bug(2345);
          }

          let(&tmpStr, "");
          tmpStr = asciiMathToTex(modeSection, showStatement);
          if (!htmlFlag) {
            if (displayMode) {
              /* It the user's responsibility to establish equation environment
                 in displayMode. */
              let(&outputLine, cat(outputLine, /*"\\[",*/ edit(tmpStr, 128),
                /*"\\]",*/ NULL));  /* edit = remove trailing spaces */
            } else {
              let(&outputLine, cat(outputLine, "$", edit(tmpStr, 128),
                "$", NULL));  /* edit = remove trailing spaces */
            }
          } else {
            /* 10/25/02 Trim leading, trailing spaces in case punctuation
               surrounds the math symbols in the comment */
            let(&tmpStr, edit(tmpStr, 8 + 128));
            /* 14-Jan-2016 nm */
            /* Enclose math symbols in a span to be used for font selection */
            let(&tmpStr, cat(
                (altHtmlFlag ? cat("<SPAN ", htmlFont, ">", NULL) : ""),
                                               /* 14-Jan-2016 nm */
                tmpStr,
                (altHtmlFlag ? "</SPAN>" : ""),    /* 14-Jan-2016 nm */
                NULL));
            let(&outputLine, cat(outputLine, tmpStr, NULL)); /* html */
          }
          let(&tmpStr, ""); /* Deallocate */
          break;
      } /* End switch(mode) */
      let(&modeSection, ""); /* Deallocate */
    }
    let(&outputLine, edit(outputLine, 128)); /* remove trailing spaces */

    if (htmlFlag) {
      /* Change blank lines into paragraph breaks except in <HTML> mode */
      if (!outputLine[0]) { /* Blank line */
        if (preformattedMode == 0
            && convertToHtml == 1 /* Not MARKUP command */ /* 13-Dec-2018 nm */
            ) {  /* Make it a paragraph break */
          let(&outputLine,
              /* "<P>"); */
              /* 9-May-2015 nm Prevent space after last paragraph */
              "<P STYLE=\"margin-bottom:0em\">");
        }
      }
      /* 11/15/02 If a statement comment has a section embedded in
         <PRE>...</PRE>, we make it a table with monospaced text and a
         background color */
      /* pos1 = instr(1, outputLine, "<PRE>"); */
      /* 26-Dec-2011 nm - changed <PRE> to more general <HTML> */
      pos1 = instr(1, outputLine, "<HTML>");
      if (pos1 != 0
          && convertToHtml == 1 /* 13-Dec-2018 nm */
          ) {
        /* 26-Dec-2011 nm - The line below is probably redundant since we
           set preformattedMode ealier.  Maybe add a bug check to make sure
           it is 1 here. */
        preformattedMode = 1; /* So we don't put <P> for blank lines */
        /* 26-Dec-2011 nm - Took out fancy table for simplicity
        let(&outputLine, cat(left(outputLine, pos1 - 1),
            "<P><CENTER><TABLE BORDER=0 CELLSPACING=0 CELLPADDING=10 BGCOLOR=",
            /@ MINT_BACKGROUND_COLOR, @/
            "\"#F0F0F0\"", /@ Very light gray @/
            "><TR><TD ALIGN=LEFT>", right(outputLine, pos1), NULL));
        */
        /* 26-Dec-2011 nm - Strip out the "<HTML>" string */
        let(&outputLine, cat(left(outputLine, pos1 - 1),
            right(outputLine, pos1 + 6), NULL));
      }
      /* pos1 = instr(1, outputLine, "</PRE>"); */
      pos1 = instr(1, outputLine, "</HTML>");
      if (pos1 != 0
          && convertToHtml == 1 /* 13-Dec-2018 nm */
          ) {
        preformattedMode = 0;
        /* 26-Dec-2011 nm - Took out fancy table for simplicity
        let(&outputLine, cat(left(outputLine, pos1 + 5),
            "</TD></TR></TABLE></CENTER>", right(outputLine, pos1 + 6), NULL));
        */
        /* 26-Dec-2011 nm - Strip out the "</HTML>" string */
        let(&outputLine, cat(left(outputLine, pos1 - 1),
            right(outputLine, pos1 + 7), NULL));
      }
    }

    if (!htmlFlag) { /* LaTeX */
      /* Convert <PRE>...</PRE> HTML tags to LaTeX */
      /* 26-Dec-2011 nm - leave this in for now */
      while (1) {
        pos1 = instr(1, outputLine, "<PRE>");
        if (pos1) {
          let(&outputLine, cat(left(outputLine, pos1 - 1), "\\begin{verbatim} ",
              right(outputLine, pos1 + 5), NULL));
        } else {
          break;
        }
      }
      while (1) {
        pos1 = instr(1, outputLine, "</PRE>");
        if (pos1) {
          let(&outputLine, cat(left(outputLine, pos1 - 1), "\\end{verbatim} ",
              right(outputLine, pos1 + 6), NULL));
        } else {
          break;
        }
      }
      /* 26-Dec-2011 nm - strip out <HTML>, </HTML> */
      /* The HTML part may screw up LaTeX; maybe we should just take out
         any HTML code completely in the future? */
      while (1) {
        pos1 = instr(1, outputLine, "<HTML>");
        if (pos1) {
          let(&outputLine, cat(left(outputLine, pos1 - 1),
              right(outputLine, pos1 + 6), NULL));
        } else {
          break;
        }
      }
      while (1) {
        pos1 = instr(1, outputLine, "</HTML>");
        if (pos1) {
          let(&outputLine, cat(left(outputLine, pos1 - 1),
              right(outputLine, pos1 + 7), NULL));
        } else {
          break;
        }
      }
    }

    saveScreenWidth = screenWidth;
    /* 26-Dec-2011 nm - in <PRE> mode, we don't want to wrap the HTML
       output with spurious newlines */
    if (preformattedMode) screenWidth = PRINTBUFFERSIZE - 2;
    if (errorsOnly == 0) {
      printLongLine(outputLine, "", htmlFlag ? "\"" : "\\");
    }
    screenWidth = saveScreenWidth;

    let(&tmp, ""); /* Clear temporary allocation stack */

    if (lastLineFlag) break; /* Done */
  } /* end while(1) */

  if (htmlFlag) {
    if (convertToHtml != 0) { /* Not MARKUP command */ /* 13-Dec-2018 nm */
      print2("\n"); /* Don't change what the previous code did */
    } else {
      /* 13-Dec-2018 nm */
      /* Add newline if string is not empty and has no newline at end */
      if (printString[0] != 0) {
        i = (long)strlen(printString);
        if (printString[i - 1] != '\n')  {
          print2("\n");
        } else {
          /* 13-Dec-2018 nm */
          /* There is an extra \n added by something previous.  Until
             we figure out what, take it off so that MARKUP output will
             equal input when no processing qualifiers are used. */
          if (i > 1) {
            if (printString[i - 2] == '\n') {
              let(&printString, left(printString, i - 1));
            }
          }
        }
      }
    }
  } else { /* LaTeX mode */
    if (!oldTexFlag) {
      /* 14-Sep-2010 nm Suppress blank line for LaTeX */
      /* print2("\n"); */
    } else {
      print2("\n");
    }
  }

  outputToString = 0; /* Restore normal output */
  if (errorsOnly == 0) { /* 17-Nov-2015 nm */
    fprintf(texFilePtr, "%s", printString);
  }

  let(&printString, ""); /* Deallocate strings */
  let(&sourceLine, "");
  let(&outputLine, "");
  let(&cmt, "");
  let(&cmtMasked, "");
  let(&tmpComment, "");
  let(&tmp, "");
  let(&tmpMasked, "");
  let(&tmpStr, "");
  let(&tmpStrMasked, "");
  let(&bibTag, "");
  let(&bibFileName, "");
  let(&bibFileContents, "");
  let(&bibFileContentsUpper, "");
  let(&bibTags, "");

  return returnVal; /* 1 if error/warning found */

} /* printTexComment */



void printTexLongMath(nmbrString *mathString,
    vstring startPrefix, /* Start prefix in "screen display" mode e.g.
         "abc $p"; it is converted to the appropriate format.  Non-zero
         length means proof step in HTML mode, as opposed to assertion etc. */
    vstring contPrefix, /* Prefix for continuation lines.  Not used in
         HTML mode.  Warning:  contPrefix must not be temporarily allocated
         (as a cat, left, etc. argument) by caller */
    long hypStmt, /* hypStmt, if non-zero, is the statement number to be
                     referenced next to the hypothesis link in html */
    long indentationLevel) /* nm 3-Feb-04 Indentation amount of proof step -
                              note that this is 0 for last step of proof */
{
/* 23-Apr-04 nm Changed "top" level from 0 to 1 - hopefully slightly less
   confusing since before user had to scroll to bottom to know that it
   started at 0 and not 1 */
#define INDENTATION_OFFSET 1
  long i;
  long pos;
  vstring tex = "";
  vstring texLine = "";
  vstring sPrefix = ""; /* 7/3/98 */
  vstring htmStep = ""; /* 7/4/98 */
  vstring htmStepTag = ""; /* 30-May-2015 nm */
  vstring htmHyp = ""; /* 7/4/98 */
  vstring htmRef = ""; /* 7/4/98 */
  vstring htmLocLab = ""; /* 7/4/98 */
  vstring tmp = "";  /* 10/10/02 */
  vstring descr = ""; /* 19-Nov-2007 nm */
  char refType = '?'; /* 14-Sep-2010 nm  'e' means $e, etc. */

  let(&sPrefix, startPrefix); /* 7/3/98 Save it; it may be temp alloc */

  if (!texDefsRead) return; /* TeX defs were not read (error was printed) */
  outputToString = 1; /* Redirect print2 and printLongLine to printString */
  /* May have stuff to be printed 7/4/98 */
  /*if (!htmlFlag) let(&printString, "");*/ /* Removed 6-Dec-03 */

  /* Note that the "tex" assignment below will be used only when !htmlFlag
     and oldTexFlag, or when htmlFlag and len(sPrefix)>0 */
  let(&tex, "");
  tex = asciiToTt(sPrefix); /* asciiToTt allocates; we must deallocate */
      /* Example: sPrefix = " 4 2,3 ax-mp  $a " */
      /*          tex = "\ 4\ 2,3\ ax-mp\ \ \$a\ " in !htmlFlag mode */
      /*          tex = " 4 2,3 ax-qmp  $a " in htmlFlag mode */
  let(&texLine, "");

  /* 14-Sep-2010 nm Get statement type of proof step reference */
  i = instr(1, sPrefix, "$");
  if (i) refType = sPrefix[i]; /* Character after the "$" */

  /* 14-Sep-2010 nm Moved this code from below to use for !oldTexFlag */
  if (htmlFlag || !oldTexFlag) {

    /* Process a proof step prefix */
    if (strlen(sPrefix)) { /* It's a proof step */
      /* Make each token a separate table column for HTML */
      /* This is a kludge that only works with /LEMMON style proofs! */
      /* 14-Sep-2010 nm Note that asciiToTt() above puts "\ " when not in
         htmlFlag mode, so use sPrefix instead of tex so it will work in
         !oldTexFlag mode */

      /* 1-May-2017 nm Fix for non-/LEMMON format used by TeX mode */
      /* In HTML mode, sPrefix has two possible formats:
           "2 ax-1  $a "
           "3 1,2 ax-mp  $a "
         In LaTeX mode (!htmlFlag), sPrefix has one format:
           "8   maj=ax-1  $a "
           "9 a1i=ax-mp $a " */
      /* Later on 1-May-2017: the LaTeX mode now returns same as HTML mode. */

      /* let(&tex, edit(tex, 8 + 16 + 128)); */
      let(&tex, edit(sPrefix, 8/*no leading spaces*/
           + 16/*reduce spaces and tabs*/
           + 128/*no trailing spaces*/));

      i = 0;
      pos = 1;
      while (pos) {
        pos = instr(1, tex, " ");
        if (pos) {
          if (i > 3) { /* 2/8/02 - added for extra safety for the future */
            bug(2316);
          }
          /* Deleted 26-Jun-2017 nm - this check is too conservative; we
             could have starting tex="2 1 a4s @2: $p" which will result
             in pos=4, i=3.  "show proof drsb1/tex" triggered this bug. */
          /*
          if (!htmlFlag && i > 2) { /@ 1-May-2017 nm @/
            bug(2341);
          }
          */
          if (i == 0) let(&htmStep, left(tex, pos - 1));
          if (i == 1) let(&htmHyp, left(tex, pos - 1));
          if (i == 2) let(&htmRef, left(tex, pos - 1));
          /* 26-Jun-2017 nm */
          if (i == 3) let(&htmLocLab, left(tex, pos - 1));

          let(&tex, right(tex, pos + 1));
          i++;
        }
      }

      /* 26-Jun-2017 nm */
      if (i == 3 && htmRef[0] == '@') {
        /* The referenced statement has no hypotheses but has a local
           label e.g."2 a4s @2: $p" */
        let(&htmLocLab, htmRef);
        let(&htmRef, htmHyp);
        let(&htmHyp, "");
      }

      if (i < 3) {
        /* The referenced statement has no hypotheses e.g.
           "4 ax-1 $a" */
        let(&htmRef, htmHyp);
        let(&htmHyp, "");

        /* 1-May-2017 nm Fix bug reported by Ari Ferrera*/
        /* Change "maj=ax-1" to "ax-1" so \ref{} produced by
           "show proof .../tex" will match \label{} produced by
           "show statement .../tex" */
        /* Later on 1-May-2017:  earlier we set the noIndentFlag (Lemmon
           proof) in the SHOW PROOF.../TEX call in metamath.c, so the
           hypothesis ref list will be available just like in the HTML
           output. */
        /* 1-May-2017 nm - Delete the below if we keep the noIndentFlag solution. */
        /*
        if (!htmlFlag) {
          pos = instr(1, htmRef, "=");
          if (!pos) bug(2342);
          let(&htmRef, right(htmRef, pos + 1));
        }
        */
        /* We now consider "=" a bug since the call via typeProof() in
           metamath.c now always has noIndentFlag = 1. */
        if (!htmlFlag) {
          pos = instr(1, htmRef, "=");
          if (pos) bug(2342);
        }


      }
    } /* if (strlen(sPrefix)) (end processing proof step prefix) */
  }

  if (!htmlFlag) {

    /* 27-Jul-05 nm Added SIMPLE_TEX */
    if (!oldTexFlag) {
      /* 14-Sep-2010 nm Old 27-Jul-05 version commented out: */
      /* printLongLine(cat("\\texttt{", tex, "}", NULL), "", " ");  */
      /* let(&tex, ""); */ /* Deallocate */
      /* tex = asciiToTt(contPrefix); */
      /* printLongLine(cat("\\texttt{", tex, "}", NULL), "", " "); */
      /* print2("\\begin{eqnarray}\n"); */
    } else {
      /* 9/2/99 Trim down long start prefixes so they won't overflow line,
         by putting their tokens into \m macros */
#define TRIMTHRESHOLD 60
      i = (long)strlen(tex);
      while (i > TRIMTHRESHOLD) {
        if (tex[i] == '\\') {
          /* Move to math part */
          let(&texLine, cat("\\m{\\mbox{\\tt", right(tex, i + 1), "}}",
              texLine, NULL));
          /* Take off of prefix part */
          let(&tex, left(tex, i));
        }
        i--;
      }

      printLongLine(cat(
          "\\setbox\\startprefix=\\hbox{\\tt ", tex, "}", NULL), "", "\\");
      let(&tex, ""); /* Deallocate */
      tex = asciiToTt(contPrefix);
      printLongLine(cat(
          "\\setbox\\contprefix=\\hbox{\\tt ", tex, "}", NULL), "", "\\");
      print2("\\startm\n");
    }
  } else { /* htmlFlag */
    if (strlen(sPrefix)) { /* It's a proof step */

      if (htmHyp[0] == 0)
        let(&htmHyp, "&nbsp;");  /* Insert blank field for Lemmon ref w/out hyp */

      /*** Start of 9-Sep-2010 ***/
      /* 9-Sep-2010 Stefan Allen - put hyperlinks on hypothesis
         label references in SHOW STATEMENT * /HTML, ALT_HTML output */
      /*Add hyperlink references to the proof */
      /* let(&htmStep, cat("<A NAME=\"",htmStep,"\">",htmStep,"</A>",NULL)); */
      /* 30-May-2015 nm Use a separate tag to put into the math cell,
         so it will link to the top of the math cell */
      let(&htmStepTag, cat("<A NAME=\"", htmStep, "\">","</A>", NULL));
      i = 1;
      pos = 1;
      while (pos && strcmp(htmHyp, "&nbsp;")) {
        pos = instr(i,htmHyp, ",");
        if (!pos) pos = len(htmHyp) + 1;
        let(&htmHyp, cat(left(htmHyp, i - 1),
            "<A HREF=\"#",
            seg(htmHyp, i, pos - 1),
            "\">",
            seg(htmHyp, i, pos - 1),
            "</A>",
            right(htmHyp, pos),
            NULL));
        /* Break out of loop if we hit the end */
        pos += 16 + len(seg(htmHyp, i, pos - 1)) + 1;
        if (!instr(i, htmHyp, ",")) break;
        i = pos;
      }
      /*** End of 9-Sep-2010 ***/

      /* 2/8/02 Add a space after each comma so very long hypotheses
         lists will wrap in an HTML table cell, e.g. gomaex3 in ql.mm */
      pos = instr(1, htmHyp, ",");
      while (pos) {
        let(&htmHyp, cat(left(htmHyp, pos), " ", right(htmHyp, pos + 1), NULL));
        pos = instr(pos + 1, htmHyp, ",");
      }

      /* if (!strcmp(tex, "$e") || !strcmp(tex, "$f")) { */ /* Old */
      if (refType == 'e' || refType == 'f') { /* 14-Sep-2010 nm Speedup */
        /* A hypothesis - don't include link */
        printLongLine(cat("<TR ALIGN=LEFT><TD>", htmStep, "</TD><TD>",
            htmHyp, "</TD><TD>", htmRef,
            "</TD><TD>",
            htmStepTag, /* 30-May-2015 nm */
                /* Put the <A NAME=...></A> tag at start of math symbol cell */
            NULL), "", "\"");
      } else {
        if (hypStmt <= 0) {
          printLongLine(cat("<TR ALIGN=LEFT><TD>", htmStep, "</TD><TD>",
              htmHyp, "</TD><TD><A HREF=\"", htmRef, ".html\">", htmRef,
              "</A></TD><TD>",
              htmStepTag, /* 30-May-2015 nm */
                /* Put the <A NAME=...></A> tag at start of math symbol cell */
              NULL), "", "\"");
        } else {
          /* Include step number reference.  The idea is that this will
             help the user to recognized "important" (vs. early trivial
             logic) steps.  This prints a small pink statement number
             after the hypothesis statement label. */
          let(&tmp, "");
          tmp = pinkHTML(hypStmt);
          /* Special case for pink number here: to make table more compact,
             we allow the pink number to break off of the href by changing
             PINK_NBSP to a space.  Elsewhere we don't allow such line
             breaks. */
          /* 10/14/02 I decided I don't like it so I took it out. */
          /*******
          let(&tmp, cat(" ", right(tmp, (long)strlen(PINK_NBSP) + 1), NULL));
          *******/

#define TOOLTIP
#ifdef TOOLTIP
          /* 19-Nov-2007 nm Get description for mod below */
          let(&descr, ""); /* Deallocate previous description */
          descr = getDescription(hypStmt);
          let(&descr, edit(descr, 4 + 16)); /* Discard lf/cr; reduce spaces */
#define MAX_DESCR_LEN 87
          if (strlen(descr) > MAX_DESCR_LEN) { /* Truncate long lines */
            i = MAX_DESCR_LEN - 3;
            while (i >= 0) { /* Get to previous word boundary */
              if (descr[i] == ' ') break;
              i--;
            }
            let(&descr, cat(left(descr, i), "...", NULL));
          }
          i = 0;
          while (descr[i] != 0) { /* Convert double quote to single */
            descr[i] = (char)(descr[i] == '"' ? '\'' : descr[i]);
            i++;
          }
#endif

          printLongLine(cat("<TR ALIGN=LEFT><TD>", htmStep, "</TD><TD>",
              htmHyp, "</TD><TD><A HREF=\"", htmRef, ".html\"",

#ifdef TOOLTIP
              /* 19-Nov-2007 nm Put in a TITLE entry for mouseover tooltip,
                 as suggested by Reinder Verlinde */
              " TITLE=\"", descr, "\"",
#endif

              ">", htmRef,
              "</A>", tmp,
              "</TD><TD>",
              htmStepTag, /* 30-May-2015 nm */
                /* Put the <A NAME=...></A> tag at start of math symbol cell */
              NULL), "", "\"");
        }
      }
#ifdef INDENT_HTML_PROOFS
      /* nm 3-Feb-04 Experiment to indent web proof displays */
      let(&tmp, "");
      for (i = 1; i <= indentationLevel; i++) {
        let(&tmp, cat(tmp, ". ", NULL));
      }
      let(&tmp, cat("<SPAN CLASS=i>",
          tmp,
          str((double)(indentationLevel + INDENTATION_OFFSET)), "</SPAN>",
          NULL));
      printLongLine(tmp, "", "\"");
      let(&tmp, "");
#endif
    } /* strlen(sPrefix) */
  } /* htmlFlag */
  let(&tex, ""); /* Deallocate */
  let(&sPrefix, ""); /* Deallocate */

  let(&tex, "");
  tex = getTexLongMath(mathString, hypStmt); /* 20-Sep-03 */
  let(&texLine, cat(texLine, tex, NULL));

  if (!htmlFlag) {  /* LaTeX */
    /* 27-Jul-05 nm Added for new LaTeX (!oldTexFlag) */
    if (!oldTexFlag) {
      /* 14-Sep-2010 nm */
      if (refType == 'e' || refType == 'f') {
        /* A hypothesis - don't include \ref{} */
        printLongLine(cat("  ",
            /* If not first step, so print "\\" LaTeX line break */
            !strcmp(htmStep, "1") ? "" : "\\\\ ",
            htmStep,  /* Step number */
            " && ",
            " & ",
            texLine,
            /* Don't put space to help prevent bad line break */
            "&\\text{Hyp~",
            /* The following puts a hypothesis number such as "2" if
               $e label is "abc.2"; if no ".", will be whole label */
            right(htmRef, instr(1, htmRef, ".") + 1),
            "}\\notag%",
            /* Add full label as LaTeX comment - note lack of space after
               "%" above to prevent bad line break */
            htmRef, NULL),
            "    \\notag \\\\ && & \\qquad ",  /* Continuation line prefix */
            " ");
      } else {
        printLongLine(cat("  ",
            /* If not first step, so print "\\" LaTeX line break */
            !strcmp(htmStep, "1") ? "" : "\\\\ ",
            htmStep,  /* Step number */
            " && ",

            /* Local label if any e.g. "@2:" */ /* 26-Jun-2017 nm */
            (htmLocLab[0] != 0) ? cat(htmLocLab, "\\ ", NULL) : "",

            " & ",
            texLine,
            /* Don't put space to help prevent bad line break */

            /* 1-May-2017 nm Surround \ref with \mbox for non-math-mode
               symbolic labels (due to \tag{..} in mmcmds.c).  Also,
               move hypotheses to after referenced label */
            "&",
            "(",

            /* Don't make local label a \ref */ /* 26-Jun-2017 nm */
            (htmRef[0] != '@') ?
                cat("\\mbox{\\ref{eq:", htmRef, "}}", NULL)
                : htmRef,

            htmHyp[0] ? "," : "",
            htmHyp,
            ")\\notag", NULL),
            /* before 1-May-2017:
            "&", htmHyp, htmHyp[0] ? "," : "",
            "(\\ref{eq:", htmRef, "})\\notag", NULL),
            */

            "    \\notag \\\\ && & \\qquad ",  /* Continuation line prefix */
            " ");
      }
      /* 14-Sep-2010 nm - commented out below */
      /* printLongLine(texLine, "", " "); */
      /* print2("\\end{eqnarray}\n"); */
      /* 6 && \vdash& ( B \ton_3 B ) \ton_3 A & (\ref{eq:q2}),4,5 \notag \\ */
      /*print2(" & (\\ref{eq:%s}%s \\notag\n",???,??? );*/
    } else {
      printLongLine(texLine, "", "\\");
      print2("\\endm\n");
    }
  } else {  /* HTML */
    printLongLine(cat(texLine, "</TD></TR>", NULL), "", "\"");
  }

  outputToString = 0; /* Restore normal output */
  fprintf(texFilePtr, "%s", printString);
  let(&printString, "");

  let(&descr, ""); /*Deallocate */  /* 17-Nov-2007 nm */
  let(&htmStep, ""); /* Deallocate */
  let(&htmStepTag, ""); /* Deallocate */
  let(&htmHyp, ""); /* Deallocate */
  let(&htmRef, ""); /* Deallocate */
  let(&htmLocLab, ""); /* Deallocate */ /* 26-Jun-2017 nm */
  let(&tmp, ""); /* Deallocate */
  let(&texLine, ""); /* Deallocate */
  let(&tex, ""); /* Deallocate */
} /* printTexLongMath */

void printTexTrailer(flag texTrailerFlag) {

  if (texTrailerFlag) {
    outputToString = 1; /* Redirect print2 and printLongLine to printString */
    if (!htmlFlag) let(&printString, "");
        /* May have stuff to be printed 7/4/98 */
    if (!htmlFlag) {
      print2("\\end{document}\n");
    } else {
      /*******  10/10/02 Moved to mmcmds.c so it can be printed immediately
                after proof; made htmlVarColor global for this
      print2("<FONT SIZE=-1 FACE=sans-serif>Colors of variables:\n");
      printLongLine(cat(htmlVarColor, "</FONT>", NULL), "", " ");
      *******/
      print2("</TABLE></CENTER>\n");
      print2("<TABLE BORDER=0 WIDTH=\"100%s\">\n", "%");
      print2("<TR><TD WIDTH=\"25%s\">&nbsp;</TD>\n", "%");
      print2("<TD ALIGN=CENTER VALIGN=BOTTOM>\n");
      print2("<FONT SIZE=-2 FACE=sans-serif>\n");
      /*
      print2("<A HREF=\"definitions.html\">Definition list</A> |\n");
      print2("<A HREF=\"theorems.html\">Theorem list</A><BR>\n");
      */
      /*
      print2("Copyright &copy; 2002 \n");
      print2("The Metamath Home Page is\n");
      */
      /*
      print2("<A HREF=\"http://metamath.org\">metamath.org mirrors</A>\n");
      */
      print2("Copyright terms:\n");
      print2("<A HREF=\"../copyright.html#pd\">Public domain</A>\n");
      print2("</FONT></TD><TD ALIGN=RIGHT VALIGN=BOTTOM WIDTH=\"25%s\">\n",
          "%");
      print2("<FONT SIZE=-2 FACE=sans-serif>\n");
      print2("<A HREF=\"http://validator.w3.org/check?uri=referer\">\n");
      print2("W3C validator</A>\n");
      print2("</FONT></TD></TR></TABLE>\n");

      /* Todo: Decide to use or not use this */
      /*
      print2("<SCRIPT SRC=\"http://www.google-analytics.com/urchin.js\"\n");
      print2("  TYPE=\"text/javascript\">\n");
      print2("</SCRIPT>\n");
      print2("<SCRIPT TYPE=\"text/javascript\">\n");
      print2("  _uacct = \"UA-1862729-1\";\n");
      print2("  urchinTracker();\n");
      print2("</SCRIPT>\n");
      */

      print2("</BODY></HTML>\n");
    }
    outputToString = 0; /* Restore normal output */
    fprintf(texFilePtr, "%s", printString);
    let(&printString, "");
  }

} /* printTexTrailer */


/* Added 4-Dec-03 - WRITE THEOREM_LIST command:  Write out theorem list
   into mmtheorems.html, mmtheorems1.html,... */
void writeTheoremList(long theoremsPerPage, flag showLemmas)
{
  nmbrString *nmbrStmtNmbr = NULL_NMBRSTRING;
  long pages, page, assertion, assertions, lastAssertion;
  long s, p, i1, i2;
  vstring str1 = "";
  vstring str3 = "";
  vstring str4 = "";
  vstring prevNextLinks = "";
  long partCntr;        /* Counter for hugeHdr */ /* 21-Jun-2014 */
  long sectionCntr;     /* Counter for bigHdr */ /* 21-Jun-2014 */
  long subsectionCntr;  /* Counter for smallHdr */ /* 21-Jun-2014 */
  long subsubsectionCntr;  /* Counter for tinyHdr */ /* 21-Aug-2017 */
  vstring outputFileName = "";
  FILE *outputFilePtr;
  long passNumber; /* 18-Oct-2015 1/2 for summary/detailed table of contents */

  /* 31-Jul-2006 for table of contents mod */
  vstring hugeHdr = ""; /* 21-Jun-2014 nm */
  vstring bigHdr = "";
  vstring smallHdr = "";
  vstring tinyHdr = ""; /* 21-Aug-2017 nm */
  vstring hugeHdrComment = ""; /* 8-May-2015 nm */
  vstring bigHdrComment = ""; /* 8-May-2015 nm */
  vstring smallHdrComment = ""; /* 8-May-2015 nm */
  vstring tinyHdrComment = ""; /* 21-Aug-2017 nm */
  long stmt, i;
  pntrString *pntrHugeHdr = NULL_PNTRSTRING;
  pntrString *pntrBigHdr = NULL_PNTRSTRING;
  pntrString *pntrSmallHdr = NULL_PNTRSTRING;
  pntrString *pntrTinyHdr = NULL_PNTRSTRING; /* 21-Aug-2017 nm */
  pntrString *pntrHugeHdrComment = NULL_PNTRSTRING; /* 8-May-2015 nm */
  pntrString *pntrBigHdrComment = NULL_PNTRSTRING; /* 8-May-2015 nm */
  pntrString *pntrSmallHdrComment = NULL_PNTRSTRING; /* 8-May-2015 nm */
  pntrString *pntrTinyHdrComment = NULL_PNTRSTRING; /* 21-Aug-2017 nm */
  vstring hdrCommentMarker = ""; /* 4-Aug-2018 nm */
  vstring hdrCommentAnchor = ""; /* 20-Oct-2018 nm */
  flag hdrCommentAnchorDone = 0; /* 20-Oct-2018 nm */

  /* Populate the statement map */
  /* ? ? ? Future:  is assertions same as statement[statements].pinkNumber? */
  nmbrLet(&nmbrStmtNmbr, nmbrSpace(statements + 1));
  assertions = 0; /* Number of $p's + $a's */
  for (s = 1; s <= statements; s++) {
    if (statement[s].type == a_ || statement[s].type == p_) {
      assertions++; /* Corresponds to pink number */
      nmbrStmtNmbr[assertions] = s;
    }
  }
  if (assertions != statement[statements].pinkNumber) bug(2328);

  /* 31-Jul-2006 nm Table of contents mod */
  /* Allocate array for section headers found */
  pntrLet(&pntrHugeHdr, pntrSpace(statements + 1));
  pntrLet(&pntrBigHdr, pntrSpace(statements + 1));
  pntrLet(&pntrSmallHdr, pntrSpace(statements + 1));
  pntrLet(&pntrTinyHdr, pntrSpace(statements + 1)); /* 21-Aug-2017 nm */
  pntrLet(&pntrHugeHdrComment, pntrSpace(statements + 1)); /* 8-May-2015 nm */
  pntrLet(&pntrBigHdrComment, pntrSpace(statements + 1)); /* 8-May-2015 nm */
  pntrLet(&pntrSmallHdrComment, pntrSpace(statements + 1)); /* 8-May-2015 nm */
  pntrLet(&pntrTinyHdrComment, pntrSpace(statements + 1)); /* 21-Aug-2017 nm */

  pages = ((assertions - 1) / theoremsPerPage) + 1;
  /* for (page = 1; page <= pages; page++) { */
  /* 8-May-2015 nm */
  for (page = 0; page <= pages; page++) {
    /* Open file */
    let(&outputFileName,
        /* cat("mmtheorems", (page > 1) ? str(page) : "", ".html", NULL)); */
        /* 8-May-2015 nm */
        cat("mmtheorems", (page > 0) ? str((double)page) : "", ".html", NULL));
    print2("Creating %s\n", outputFileName);
    outputFilePtr = fSafeOpen(outputFileName, "w", 0/*noVersioningFlag*/);
    if (!outputFilePtr) goto TL_ABORT; /* Couldn't open it (error msg was provided)*/

    /* Output header */
    /* TODO 14-Jan-2016: why aren't we using printTexHeader? */

    outputToString = 1;
    print2(
        "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n");
    print2(     "    \"http://www.w3.org/TR/html4/loose.dtd\">\n");
    print2("<HTML LANG=\"EN-US\">\n");
    print2("<HEAD>\n");
    print2("%s%s\n", "<META HTTP-EQUIV=\"Content-Type\" ",
        "CONTENT=\"text/html; charset=iso-8859-1\">");
    /* 13-Aug-2016 nm */
    /* Improve mobile device display per David A. Wheeler */
    print2(
"<META NAME=\"viewport\" CONTENT=\"width=device-width, initial-scale=1.0\">\n"
        );

    print2("<STYLE TYPE=\"text/css\">\n");
    print2("<!--\n");
    /* 15-Apr-2015 nm - align math symbol images to text */
    print2("img { margin-bottom: -4px }\n");
#ifndef RAINBOW_OPTION
    /* Print style sheet for pink number that goes after statement label */
    print2(".p { font-family: \"Arial Narrow\";\n");
    print2("     font-size: x-small;\n");
    /* Strip off quotes from color (css doesn't like them) */
    printLongLine(cat("     color: ", seg(PINK_NUMBER_COLOR, 2,
        (long)strlen(PINK_NUMBER_COLOR) - 1), ";", NULL), "", "&");
    print2("   }\n");
#else
    /* Print style sheet for colored number that goes after statement label */
    print2(".r { font-family: \"Arial Narrow\";\n");
    print2("     font-size: x-small;\n");
    print2("   }\n");
#endif
    print2("-->\n");
    print2("</STYLE>\n");
    printLongLine(htmlCSS, "", " ");

    /*
    print2("%s\n", cat("<TITLE>", htmlTitle, " - ",
        / * Strip off ".html" * /
        left(outputFileName, (long)strlen(outputFileName) - 5),
        "</TITLE>", NULL));
    */
    /* 4-Jun-06 nm - Put page name before "Metamath Proof Explorer" etc. */
    /* 15-Nov-2015 nm - Change print2() to printLongLine() */
    printLongLine(cat("<TITLE>",
        /* Strip off ".html" */
        /*
        left(outputFileName, (long)strlen(outputFileName) - 5),
        */

        /* "List p. ", str(page), */ /* 21-Jun-2014 */
        /* 9-May-2015 nm */
        ((page == 0)
            ? "TOC of Theorem List"
            : cat("P. ", str((double)page), " of Theorem List", NULL)),

        " - ",
        htmlTitle,
        "</TITLE>",
        NULL), "", "\"");
    /* Icon for bookmark */
    print2("%s%s\n", "<LINK REL=\"shortcut icon\" HREF=\"favicon.ico\" ",
        "TYPE=\"image/x-icon\">");

    /* 18-Oct-2015 nm Image alignment fix */
    print2(
        "<STYLE TYPE=\"text/css\">\n");
    print2(
        "<!--\n");
    /* Optional information but takes unnecessary file space */
    /* (change @ to * if uncommenting)
    print2(
        "/@ Math symbol GIFs will be shifted down 4 pixels to align with\n");
    print2(
        "   normal text for compatibility with various browsers.  The old\n");
    print2(
        "   ALIGN=TOP for math symbol images did not align in all browsers\n");
    print2(
        "   and should be deleted.  All other images must override this\n");
    print2(
        "   shift with STYLE=\"margin-bottom:0px\". @/\n");
    */
    print2(
        "img { margin-bottom: -4px }\n");
    print2(
        "-->\n");
    print2(
        "</STYLE>\n");

    print2("</HEAD>\n");
    print2("<BODY BGCOLOR=\"#FFFFFF\">\n");
    print2("<TABLE BORDER=0 WIDTH=\"100%s\"><TR>\n", "%");
    print2("<TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%s\"\n", "%");
    printLongLine(cat("ROWSPAN=2>", htmlHome, "</TD>", NULL), "", "\"");
    printLongLine(cat(
        "<TD NOWRAP ALIGN=CENTER ROWSPAN=2><FONT SIZE=\"+3\" COLOR=",
        GREEN_TITLE_COLOR, "><B>", htmlTitle, "</B></FONT>",
        "<BR><FONT SIZE=\"+2\" COLOR=",      /* 21-Jun-2014 */
        GREEN_TITLE_COLOR,                   /* 21-Jun-2014 */
        /* "><B>Statement List (", */
        /* 9-May-2015 nm */
        "><B>Theorem List (",
        ((page == 0)
            ? "Table of Contents"
            /* : cat("(p. ", str(page), " of ", str(pages), NULL)), */
            /* 9-May-2015 nm Remove stray "(" */
            : cat("p. ", str((double)page), " of ", str((double)pages), NULL)),
        ")</B></FONT>",
        NULL), "", "\"");


    /* Put Previous/Next links into web page */
    print2("</TD><TD NOWRAP ALIGN=RIGHT VALIGN=TOP WIDTH=\"25%c\"><FONT\n", '%');
    print2(" SIZE=-1 FACE=sans-serif>\n");

    /* Output title with current page */
    /* Output previous and next */


    /* 21-Jun-2014 nm Assign prevNextLinks once here since it is used 3 times */
    let(&prevNextLinks, cat("<A HREF=\"mmtheorems",
        /*
        (page > 1)
            ? ((page - 1 > 1) ? str(page - 1) : "")
            : ((pages > 1) ? str(pages) : ""),
        */
        /* 8-May-2015 */
        (page > 0)
            ? ((page - 1 > 0) ? str((double)page - 1) : "")
            : ((pages > 0) ? str((double)pages) : ""),
        ".html\">", NULL));
    /* if (page > 1) { */
    /* 8-May-2015 */
    if (page > 0) {
      let(&prevNextLinks, cat(prevNextLinks,
          "&lt; Previous</A>&nbsp;&nbsp;", NULL));
    } else {
      let(&prevNextLinks, cat(prevNextLinks, "&lt; Wrap</A>&nbsp;&nbsp;", NULL));
    }
    let(&prevNextLinks, cat(prevNextLinks, "<A HREF=\"mmtheorems",
        (page < pages)
            ? str((double)page + 1)
            : "",
        ".html\">", NULL));
    if (page < pages) {
      let(&prevNextLinks, cat(prevNextLinks, "Next &gt;</A>", NULL));
    } else {
      let(&prevNextLinks, cat(prevNextLinks, "Wrap &gt;</A>", NULL));
    }

    printLongLine(prevNextLinks,
        " ",  /* Start continuation line with space */
        "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
    /********* old code that the above printLongLine replaces
    let(&str1, cat("<A HREF=\"mmtheorems",
        (page > 1)
            ? ((page - 1 > 1) ? str(page - 1) : "")
            : ((pages > 1) ? str(pages) : ""),
        ".html\">", NULL));
    if (page > 1) {
      print2("%s&lt; Previous</A>&nbsp;&nbsp;\n", str1);
    } else {
      print2("%s&lt; Wrap</A>&nbsp;&nbsp;\n", str1);
    }
    let(&str1, cat("<A HREF=\"mmtheorems",
        (page < pages)
            ? str(page + 1)
            : "",
        ".html\">", NULL));
    if (page < pages) {
      print2("%sNext &gt;</A>\n", str1);
    } else {
      print2("%sWrap &gt;</A>\n", str1);
    }
    ******************************/


    /* Finish up header */
    /* Print the GIF/Unicode Font choice, if directories are specified */
    if (htmlDir[0]) {
      if (altHtmlFlag) {
        /* 4-Aug-2018 nm */
        print2("</FONT></TD></TR><TR><TD ALIGN=RIGHT><FONT FACE=sans-serif\n");
        print2("SIZE=-2>Bad symbols? Try the\n");
        print2("<BR><A HREF=\"%s%s\">GIF\n",
            htmlDir, outputFileName);
        print2("version</A>.</FONT></TD>\n");
        /* 4-Aug-2018 nm - removed Firefox and IE browser references
        print2("</FONT></TD></TR><TR><TD ALIGN=RIGHT><FONT FACE=sans-serif\n");
        print2("SIZE=-2>Bad symbols?\n");
        print2("Use <A HREF=\"http://mozilla.org\">Firefox</A><BR>\n");
        print2("(or <A HREF=\"%s%s\">GIF version</A> for IE).</FONT></TD>\n",
            htmlDir, outputFileName);
        */
      } else {
        print2("</FONT></TD></TR><TR><TD ALIGN=RIGHT><FONT FACE=sans-serif\n");
        print2("SIZE=-2>Browser slow? Try the\n");
        print2("<BR><A HREF=\"%s%s\">Unicode\n",
            altHtmlDir, outputFileName);
        print2("version</A>.</FONT></TD>\n");
      }
    }
    /*print2("</TR></TABLE>\n");*/
    /*print2("<HR NOSHADE SIZE=1>\n");*/

    /* 4-Aug-2018 nm */
    /* Make breadcrumb font to match other pages */
    print2("<TR>\n");
    print2(
      "<TD COLSPAN=3 ALIGN=LEFT VALIGN=TOP><FONT SIZE=-2 FACE=sans-serif>\n");
    print2("<BR>\n");  /* Add a little more vertical space */

    /* Print some useful links */
    /* print2("<CENTER>\n"); */
    print2("<A HREF=\"../mm.html\">Mirrors</A>\n");
    print2("&nbsp;&gt;&nbsp;<A HREF=\"../index.html\">\n");
    print2("Metamath Home Page</A>\n");

    /* print2("&nbsp;&gt;&nbsp;<A HREF=\"mmset.html\">\n"); */
    /* 15-Apr-2015 nm */
    /* Normally, htmlBibliography in the .mm file will have the
       project home page, and we depend on this here rather than
       extracting from htmlHome */
    print2("&nbsp;&gt;&nbsp;<A HREF=\"%s\">\n", htmlBibliography);

    /* print2("MPE Home Page</A>\n"); */
    /* 15-Apr-2015 nm */
    /* Put a meaningful abbreviation for the project home page
       by extracting capital letters from title */
    let(&str1, "");
    s = (long)strlen(htmlTitle);
    for (i = 0; i < s; i++) {
      if (htmlTitle[i] >= 'A' && htmlTitle[i] <= 'Z') {
        let(&str1, cat(str1, chr(htmlTitle[i]), NULL));
      }
    }
    print2("%s Home Page</A>\n", str1);

    /* if (page != 1) { */
    /* 8-May-2015 nm */
    if (page != 0) {
      print2("&nbsp;&gt;&nbsp;<A HREF=\"mmtheorems.html\">\n");
      /* print2("Statement List Contents</A>\n"); */
      /* 9-May-2015 nm */
      print2("Theorem List Contents</A>\n");
    } else {
      print2("&nbsp;&gt;&nbsp;\n");
      /* print2("Statement List Contents\n"); */
      /* 9-May-2015 nm */
      print2("Theorem List Contents\n");
    }

    /* 15-Apr-2015 nm */
    /* Use "extHtmlStmt <= statements" as an indicator that we're doing
       Metamath Proof Explorer; the others don't have mmrecent.html pages */
    /*if (extHtmlStmt <= statements) {*/ /* extHtmlStmt = statements + 1
                                              unless mmset.html */
    /* 8-Dec-2017 nm */
    if (extHtmlStmt < sandboxStmt) { /* extHtmlStmt >= sandboxStmt
                                              unless mmset.html */
      print2("&nbsp;&gt;&nbsp;<A HREF=\"mmrecent.html\">\n");
      print2("Recent Proofs</A>\n");
    }


    print2("&nbsp; &nbsp; &nbsp; <B><FONT COLOR=%s>\n", GREEN_TITLE_COLOR);
    print2("This page:</FONT></B> \n");
    /* 8-May-2015 nm Deleted, since ToC is now separate page: */
    /*
    if (page == 1) {
      print2("<A HREF=\"#mmstmtlst\">Start of list</A>&nbsp; &nbsp; \n");
    }
    */

    /* 18-Oct-2015 nm */
    if (page == 0) {
      print2(
          "&nbsp;<A HREF=\"#mmdtoc\">Detailed Table of Contents</A>&nbsp;\n");
    }

    print2("<A HREF=\"#mmpglst\">Page List</A>\n");

    /* 4-Aug-2018 nm */
    /* Change breadcrumb font to match other pages */
    print2("</FONT>\n");
    print2("</TD>\n");
    print2("</TR></TABLE>\n");

    /* print2("</CENTER>\n"); */
    print2("<HR NOSHADE SIZE=1>\n");

    /* Write out HTML page so far */
    fprintf(outputFilePtr, "%s", printString);
    outputToString = 0;
    let(&printString, "");

    /***** 21-Jun-2014 Moved to bottom of page ********
    /@ Output links to the other pages @/
    fprintf(outputFilePtr, "Jump to page: \n");
    for (p = 1; p <= pages; p++) {

      /@ Construct the pink number range @/
      let(&str3, "");
      str3 = pinkRangeHTML(
          nmbrStmtNmbr[(p - 1) @ theoremsPerPage + 1],
          (p < pages) ?
            nmbrStmtNmbr[p @ theoremsPerPage] :
            nmbrStmtNmbr[assertions]);

      /@ 31-Jul-2006 nm Change "1" to "Contents + 1" @/
      if (p == page) {
        let(&str1,
            (p == 1) ? "Contents + 1" : str(p) /@ 31-Jul-2006 nm @/
            ); /@ Current page shouldn't have link to self @/
      } else {
        let(&str1, cat("<A HREF=\"mmtheorems",
            (p == 1) ? "" : str(p),
            /@ (p == 1) ? ".html#mmtc\">" : ".html\">", @/ /@ 31-Aug-2006 nm @/
            ".html\">", /@ 8-Feb-2007 nm Friendlier, because you can start
                  scrolling through the page before it finishes loading,
                  without its jumping to #mmtc (start of Table of Contents)
                  when it's done. @/
            (p == 1) ? "Contents + 1" : str(p) /@ 31-Jul-2006 nm @/
            , "</A>", NULL));
      }
      let(&str1, cat(str1, PINK_NBSP, str3, NULL));
      fprintf(outputFilePtr, "%s\n", str1);
    }
    ******** end of 21-Jun-2014 move *******/

    /* 31-Jul-2006 nm Add table of contents to first WRITE THEOREM page */
    /* if (page == 1) { */ /* We're on page 1 */
    /* 8-May-2015 nm */
    if (page == 0) {  /* We're on ToC page */

      /* Pass 1: table of contents summary; pass 2: detail */ /* 18-Oct-2015 */
      for (passNumber = 1; passNumber <= 2; passNumber++) {

        outputToString = 1;

        /* 18-Oct-2015 deleted
        print2(
        "<P><CENTER><A NAME=\"mmtc\"></A><B>Table of Contents</B></CENTER>\n");
        */
        /* 18-Oct-2015 nm */
        if (passNumber == 1) {

/* 24-Oct-2018 nm Temporary hopefully */
/* 31-Oct-2018 nm Implemented workaround; see below under this date */
/*
print2("<P><CENTER><B><FONT COLOR=RED>The Chrome browser has a\n");
print2("bug that hyperlinks to incorrect anchors.\n");
print2("If you click on PART 2 in the summary, it should go to PART 2 in\n");
print2("the detailed section.  If it goes to PART 1, your browser has the bug.\n");
print2("If your Chrome version works, let me (Norm Megill) know\n");
print2("the version number so I can mention it here.  Otherwise you will\n");
print2("need another browser to navigate. This page works with Firefox\n");
print2("and Internet Explorer and passes validator.w3.org.\n");
print2("</FONT></B></CENTER>\n");
*/


          print2(
              "<P><CENTER><B>Table of Contents Summary</B></CENTER>\n");
        } else {
          print2(
  "<P><CENTER><A NAME=\"mmdtoc\"></A><B>Detailed Table of Contents</B><BR>\n");

          /* 4-Aug-2018 nm */
          print2(
           "<B>(* means the section header has a description)</B></CENTER>\n");

        }

        fprintf(outputFilePtr, "%s", printString);

        outputToString = 0;
        let(&printString, "");

        let(&hugeHdr, "");
        let(&bigHdr, "");
        let(&smallHdr, "");
        let(&tinyHdr, "");
        let(&hugeHdrComment, "");
        let(&bigHdrComment, "");
        let(&smallHdrComment, "");
        let(&tinyHdrComment, "");
        partCntr = 0;    /* Initialize counters */    /* 21-Jun-2014 */
        sectionCntr = 0;
        subsectionCntr = 0;
        subsubsectionCntr = 0; /* 21-Aug-2017 nm */
        for (stmt = 1; stmt <= statements; stmt++) {

          /* 18-Dec-2016 nm moved to below the "if"
          getSectionHeadings(stmt, &hugeHdr, &bigHdr, &smallHdr,
              /@ 5-May-2015 nm @/
              &hugeHdrComment, &bigHdrComment, &smallHdrComment);
          */

          /* Output the headers for $a and $p statements */
          if (statement[stmt].type == p_ || statement[stmt].type == a_) {
            hdrCommentAnchorDone = 0; /* 20-Oct-2018 nm */
            getSectionHeadings(stmt, &hugeHdr, &bigHdr, &smallHdr,
                &tinyHdr, /* 21-Aug-2017 nm */
                /* 5-May-2015 nm */
                &hugeHdrComment, &bigHdrComment, &smallHdrComment,
                &tinyHdrComment);  /* 21-Aug-2017 nm */
            if (hugeHdr[0] || bigHdr[0] || smallHdr[0] || tinyHdr[0]) {
              /* Write to the table of contents */
              outputToString = 1;
              i = ((statement[stmt].pinkNumber - 1) / theoremsPerPage)
                  + 1; /* Page # */
              /* let(&str3, cat("mmtheorems", (i == 1) ? "" : str(i), ".html#", */
                          /* Note that page 1 has no number after mmtheorems */
              /* 8-May-2015 nm */
              let(&str3, cat("mmtheorems", str((double)i), ".html#",
                  /* statement[stmt].labelName, NULL)); */
                  "mm", str((double)(statement[stmt].pinkNumber)), NULL));
                     /* Link to page/location - no theorem can be named "mm*" */
              let(&str4, "");
              str4 = pinkHTML(stmt);
              /*let(&str4, right(str4, (long)strlen(PINK_NBSP) + 1));*/
                                                          /* Discard "&nbsp;" */
              if (hugeHdr[0]) {    /* 21-Jun-2014 nm */

                /* Create part number */
                partCntr++;
                sectionCntr = 0;
                subsectionCntr = 0;
                subsubsectionCntr = 0; /* 21-Aug-2017 nm */
                let(&hugeHdr, cat("PART ", str((double)partCntr), "&nbsp;&nbsp;",
                    hugeHdr, NULL));

                /* 4-Aug-2018 nm */
                /* Put an asterisk before the header if the header has
                   a comment */
                if (hugeHdrComment[0] != 0 && passNumber == 2) {
                  let(&hdrCommentMarker, "*");
                  /* 20-Oct-2018 nm */
                  if (hdrCommentAnchorDone == 0) {
                    let(&hdrCommentAnchor, cat(

                        /* 31-Oct-2018 nm "&#8203;" is a "zero-width" space to
                           workaround Chrome bug that jumps to wrong anchor. */
                        "&#8203;",

                        "<A NAME=\"",
                        statement[stmt].labelName, "\"></A>", NULL));
                    hdrCommentAnchorDone = 1;
                  } else {
                    let(&hdrCommentAnchor, "");
                  }
                } else {
                  let(&hdrCommentMarker, "");
                  let(&hdrCommentAnchor, "");
                }

                printLongLine(cat(

                    /* 18-Oct-2015 nm */
                    /* In detailed section, add an anchor to reach it from
                       summary section */
                    (passNumber == 2) ?
                         cat("<A NAME=\"dtl:", str((double)partCntr),
                             "\"></A>", NULL) : "",

                    /* 29-Jul-2008 nm Add an anchor to the "sandbox" theorem
                       for use by mmrecent.html */
                    /* 21-Jun-2014 nm We use "sandbox:bighdr" for both here and
                       below so that either huge or big header type could
                       be used to start mathbox sections */
                    (stmt == sandboxStmt && bigHdr[0] == 0
                          /* 4-Aug-2018 nm */
                          && passNumber == 1 /* Only in summary TOC */
                          ) ?
                        /* Note the colon so it won't conflict w/ theorem
                           name anchor */
                        "<A NAME=\"sandbox:bighdr\"></A>" : "",

                    hdrCommentAnchor, /* 20-Oct-2018 nm */
                    "<A HREF=\"",

                    /* 18-Oct-2015 nm */
                    (passNumber == 1) ?
                        cat("#dtl:", str((double)partCntr), NULL)  /* Link to detailed toc */
                        : cat(str3, "h", NULL), /* Link to thm list */

                    "\"><B>",
                    hdrCommentMarker, /* 4-Aug-2018 nm */
                    hugeHdr, "</B></A>",
                    /*
                    " &nbsp; <A HREF=\"",
                    statement[stmt].labelName, ".html\">",
                    statement[stmt].labelName, "</A>",
                    str4,
                    */
                    "<BR>", NULL),
                    " ",  /* Start continuation line with space */
                    "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
                if (passNumber == 2) { /* 18-Oct-2015 nm */
                  /* Assign to array for use during theorem output */
                  let((vstring *)(&pntrHugeHdr[stmt]), hugeHdr);
                  let((vstring *)(&pntrHugeHdrComment[stmt]), hugeHdrComment);
                }
                let(&hugeHdr, "");
                let(&hugeHdrComment, "");
              }
              if (bigHdr[0]) {

                /* Create section number */  /* 21-Jun-2014 */
                sectionCntr++;
                subsectionCntr = 0;
                subsubsectionCntr = 0; /* 21-Aug-2017 nm */
                let(&bigHdr, cat(str((double)partCntr), ".", str((double)sectionCntr),
                    "&nbsp;&nbsp;",
                    bigHdr, NULL));

                /* 4-Aug-2018 nm */
                /* Put an asterisk before the header if the header has
                   a comment */
                if (bigHdrComment[0] != 0 && passNumber == 2) {
                  let(&hdrCommentMarker, "*");
                  /* 20-Oct-2018 nm */
                  if (hdrCommentAnchorDone == 0) {
                    let(&hdrCommentAnchor, cat(

                        /* 31-Oct-2018 nm "&#8203;" is a "zero-width" space to
                           workaround Chrome bug that jumps to wrong anchor. */
                        "&#8203;",

                        "<A NAME=\"",
                        statement[stmt].labelName, "\"></A>", NULL));
                    hdrCommentAnchorDone = 1;
                  } else {
                    let(&hdrCommentAnchor, "");
                  }
                } else {
                  let(&hdrCommentMarker, "");
                  let(&hdrCommentAnchor, "");
                }

                printLongLine(cat(
                    "&nbsp; &nbsp; &nbsp; ", /* Indentation spacing */

                    /* 18-Oct-2015 nm */
                    /* In detailed section, add an anchor to reach it from
                       summary section */
                    (passNumber == 2) ?
                         cat("<A NAME=\"dtl:", str((double)partCntr), ".",
                             str((double)sectionCntr), "\"></A>", NULL)
                         : "",

                    /* 29-Jul-2008 nm Add an anchor to the "sandbox" theorem
                       for use by mmrecent.html */
                    (stmt == sandboxStmt
                          /* 4-Aug-2018 nm */
                          && passNumber == 1 /* Only in summary TOC */
                          ) ?
                        /* Note the colon so it won't conflict w/ theorem
                           name anchor */
                        "<A NAME=\"sandbox:bighdr\"></A>" : "",
                    hdrCommentAnchor, /* 20-Oct-2018 nm */
                    "<A HREF=\"",

                    /* 18-Oct-2015 nm */
                    (passNumber == 1) ?
                         cat("#dtl:", str((double)partCntr), ".",
                             str((double)sectionCntr),
                             NULL)   /* Link to detailed toc */
                        : cat(str3, "b", NULL), /* Link to thm list */

                    "\"><B>",
                    hdrCommentMarker, /* 4-Aug-2018 */
                    bigHdr, "</B></A>",
                    /*
                    " &nbsp; <A HREF=\"",
                    statement[stmt].labelName, ".html\">",
                    statement[stmt].labelName, "</A>",
                    str4,
                    */
                    "<BR>", NULL),
                    " ",  /* Start continuation line with space */
                    "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
                if (passNumber == 2) { /* 18-Oct-2015 nm */
                  /* Assign to array for use during theorem list output */
                  let((vstring *)(&pntrBigHdr[stmt]), bigHdr);
                  let((vstring *)(&pntrBigHdrComment[stmt]), bigHdrComment);
                }
                let(&bigHdr, "");
                let(&bigHdrComment, "");
              }
              if (smallHdr[0]
                  && passNumber == 2) {  /* Skip in pass 1 (summary) */

                /* Create subsection number */  /* 21-Jun-2014 */
                subsectionCntr++;
                subsubsectionCntr = 0; /* 21-Aug-2017 nm */
                let(&smallHdr, cat(str((double)partCntr), ".",
                    str((double)sectionCntr),
                    ".", str((double)subsectionCntr), "&nbsp;&nbsp;",
                    smallHdr, NULL));

                /* 4-Aug-2018 nm */
                /* Put an asterisk before the header if the header has
                   a comment */
                if (smallHdrComment[0] != 0 && passNumber == 2) {
                  let(&hdrCommentMarker, "*");
                  /* 20-Oct-2018 nm */
                  if (hdrCommentAnchorDone == 0) {
                    let(&hdrCommentAnchor, cat("<A NAME=\"",
                        statement[stmt].labelName, "\"></A>", NULL));
                    hdrCommentAnchorDone = 1;
                  } else {
                    let(&hdrCommentAnchor, "");
                  }
                } else {
                  let(&hdrCommentMarker, "");
                  let(&hdrCommentAnchor, "");
                }

                printLongLine(cat("&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; ",

                    /* 23-May-2008 nm Add an anchor to the "sandbox" theorem
                       for use by mmrecent.html */
                    /*
                    !strcmp(statement[stmt].labelName, "sandbox") ?
                        "<A NAME=\"sandbox:smallhdr\"></A>" : "",
                    */

                    hdrCommentAnchor, /* 20-Oct-2018 nm */
                    "<A HREF=\"", str3, "s\">",
                    hdrCommentMarker, /* 4-Aug-2018 */
                    smallHdr, "</A>",
                    " &nbsp; <A HREF=\"",
                    statement[stmt].labelName, ".html\">",
                    statement[stmt].labelName, "</A>",
                    str4,
                    "<BR>", NULL),
                    " ",  /* Start continuation line with space */
                    "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
                /* Assign to array for use during theorem output */
                let((vstring *)(&pntrSmallHdr[stmt]), smallHdr);
                let(&smallHdr, "");
                let((vstring *)(&pntrSmallHdrComment[stmt]), smallHdrComment);
                let(&smallHdrComment, "");
              }

              /* Added 21-Aug-2017 nm */
              if (tinyHdr[0]
                  && passNumber == 2) {  /* Skip in pass 1 (summary) */

                /* Create subsection number */  /* 21-Jun-2014 */
                subsubsectionCntr++;
                let(&tinyHdr, cat(str((double)partCntr), ".",
                    str((double)sectionCntr),
                    ".", str((double)subsectionCntr),
                    ".", str((double)subsubsectionCntr), "&nbsp;&nbsp;",
                    tinyHdr, NULL));

                /* 4-Aug-2018 nm */
                /* Put an asterisk before the header if the header has
                   a comment */
                if (tinyHdrComment[0] != 0 && passNumber == 2) {
                  let(&hdrCommentMarker, "*");
                  /* 20-Oct-2018 nm */
                  if (hdrCommentAnchorDone == 0) {
                    let(&hdrCommentAnchor, cat("<A NAME=\"",
                        statement[stmt].labelName, "\"></A> ", NULL));
                    hdrCommentAnchorDone = 1;
                  } else {
                    let(&hdrCommentAnchor, "");
                  }
                } else {
                  let(&hdrCommentMarker, "");
                  let(&hdrCommentAnchor, "");
                }

                printLongLine(cat(
         "&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; ",
                    /* 23-May-2008 nm Add an anchor to the "sandbox" theorem
                       for use by mmrecent.html */
                    /*
                    !strcmp(statement[stmt].labelName, "sandbox") ?
                        "<A NAME=\"sandbox:tinyhdr\"></A>" : "",
                    */

                    hdrCommentAnchor, /* 20-Oct-2018 nm */
                    "<A HREF=\"", str3, "s\">",
                    hdrCommentMarker, /* 4-Aug-2018 */
                    tinyHdr, "</A>",
                    " &nbsp; <A HREF=\"",
                    statement[stmt].labelName, ".html\">",
                    statement[stmt].labelName, "</A>",
                    str4,
                    "<BR>", NULL),
                    " ",  /* Start continuation line with space */
                    "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
                /* Assign to array for use during theorem output */
                let((vstring *)(&pntrTinyHdr[stmt]), tinyHdr);
                let(&tinyHdr, "");
                let((vstring *)(&pntrTinyHdrComment[stmt]), tinyHdrComment);
                let(&tinyHdrComment, "");
              }
              /* (End of 21-Aug-2017 addition) */

              fprintf(outputFilePtr, "%s", printString);
              outputToString = 0;
              let(&printString, "");
            } /* if huge or big or small or tiny header */
          } /* if $a or $p */
        } /* next stmt */
        /* 8-May-2015 nm Do we need the HR below? */
        fprintf(outputFilePtr, "<HR NOSHADE SIZE=1>\n");

      } /* next passNumber */ /* 18-Oct-2015 nm */

    } /* if page 0 */
    /* End of 31-Jul-2006 added code for table of contents mod */

    /* 8-May-2015 nm Just skip over instead of a big if indent */
    if (page == 0) goto SKIP_LIST;


    /* Put in color key */
    outputToString = 1;
    print2("<A NAME=\"mmstmtlst\"></A>\n");
    /*if (extHtmlStmt <= statements) {*/ /* extHtmlStmt = statements + 1 in ql.mm */
    /* 8-Dec-2017 nm */
    if (extHtmlStmt < sandboxStmt) { /* extHtmlStmt >= sandboxStmt in ql.mm */
      /* ?? Currently this is customized for set.mm only!! */
      print2("<P>\n");
      print2("<CENTER><TABLE CELLSPACING=0 CELLPADDING=5\n");
      print2("SUMMARY=\"Color key\"><TR>\n");
      print2("\n");
      print2("<TD>Color key:&nbsp;&nbsp;&nbsp;</TD>\n");
      print2("<TD BGCOLOR=%s NOWRAP><A\n", MINT_BACKGROUND_COLOR);
      print2("HREF=\"mmset.html\"><IMG SRC=\"mm.gif\" BORDER=0\n");
      print2("ALT=\"Metamath Proof Explorer\" HEIGHT=32 WIDTH=32\n");
      print2("ALIGN=MIDDLE> &nbsp;Metamath Proof Explorer</A>\n");

      let(&str3, "");
      if (statement[extHtmlStmt].pinkNumber <= 0) bug(2332);
      str3 = pinkRangeHTML(nmbrStmtNmbr[1],
          nmbrStmtNmbr[statement[extHtmlStmt].pinkNumber - 1]);
      printLongLine(cat("<BR>(", str3, ")", NULL),
        " ",  /* Start continuation line with space */
        "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

      print2("</TD>\n");
      print2("\n");
      print2("<TD WIDTH=10>&nbsp;</TD>\n");
      print2("\n");

      /* Hilbert Space Explorer */
      print2("<TD BGCOLOR=%s NOWRAP><A\n", PURPLISH_BIBLIO_COLOR);
      print2(" HREF=\"mmhil.html\"><IMG SRC=\"atomic.gif\"\n");
      print2(
 "BORDER=0 ALT=\"Hilbert Space Explorer\" HEIGHT=32 WIDTH=32 ALIGN=MIDDLE>\n");
      print2("&nbsp;Hilbert Space Explorer</A>\n");

      let(&str3, "");
      /* str3 = pinkRangeHTML(extHtmlStmt, nmbrStmtNmbr[assertions]); */
      if (statement[sandboxStmt].pinkNumber <= 0) bug(2333);
      str3 = pinkRangeHTML(extHtmlStmt,
         nmbrStmtNmbr[statement[sandboxStmt].pinkNumber - 1]);
      printLongLine(cat("<BR>(", str3, ")", NULL),
        " ",  /* Start continuation line with space */
        "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

      print2("</TD>\n");
      print2("\n");
      print2("<TD WIDTH=10>&nbsp;</TD>\n");
      print2("\n");


      /* 29-Jul-2008 nm Sandbox stuff */
      print2("<TD BGCOLOR=%s NOWRAP><A\n", SANDBOX_COLOR);
      print2(
   /*" HREF=\"mmtheorems.html#sandbox:bighdr\"><IMG SRC=\"_sandbox.gif\"\n");*/
      /* 24-Jul-2009 nm Changed name of sandbox to "mathbox" */
       " HREF=\"mathbox.html\"><IMG SRC=\"_sandbox.gif\"\n");
      print2(
     /*"BORDER=0 ALT=\"User Sandboxes\" HEIGHT=32 WIDTH=32 ALIGN=MIDDLE>\n");*/
         /* 24-Jul-2009 nm Changed name of sandbox to "mathbox" */
         "BORDER=0 ALT=\"Users' Mathboxes\" HEIGHT=32 WIDTH=32 ALIGN=MIDDLE>\n");
      /*print2("&nbsp;User Sandboxes</A>\n");*/
      /* 24-Jul-2009 nm Changed name of sandbox to "mathbox" */
      print2("&nbsp;Users' Mathboxes</A>\n");

      let(&str3, "");
      str3 = pinkRangeHTML(sandboxStmt, nmbrStmtNmbr[assertions]);
      printLongLine(cat("<BR>(", str3, ")", NULL),
        " ",  /* Start continuation line with space */
        "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

      print2("</TD>\n");
      print2("\n");
      print2("<TD WIDTH=10>&nbsp;</TD>\n");
      print2("\n");


      print2("</TR></TABLE></CENTER>\n");
    } /* end if (extHtmlStmt < sandboxStmt) */

    /* Write out HTML page so far */
    fprintf(outputFilePtr, "%s", printString);
    outputToString = 0;
    let(&printString, "");


    /* Write the main table header */
    outputToString = 1;
    print2("\n");
    print2("<P><CENTER>\n");
    print2("<TABLE BORDER CELLSPACING=0 CELLPADDING=3 BGCOLOR=%s\n",
        MINT_BACKGROUND_COLOR);
    /* print2("SUMMARY=\"Statement List for %s\">\n", htmlTitle); */
    /* 9-May-2015 nm */
    print2("SUMMARY=\"Theorem List for %s\">\n", htmlTitle);
    let(&str3, "");
    if (page < 1) bug(2335); /* Page 0 ToC should have been skipped */
    str3 = pinkHTML(nmbrStmtNmbr[(page - 1) * theoremsPerPage + 1]);
    let(&str3, right(str3, (long)strlen(PINK_NBSP) + 1)); /* Discard "&nbsp;" */
    let(&str4, "");
    str4 = pinkHTML((page < pages) ?
        nmbrStmtNmbr[page * theoremsPerPage] :
        nmbrStmtNmbr[assertions]);
    let(&str4, right(str4, (long)strlen(PINK_NBSP) + 1)); /* Discard "&nbsp;" */
    /* printLongLine(cat("<CAPTION><B>Statement List for ", htmlTitle, */
    /* 9-May-2015 nm */
    printLongLine(cat("<CAPTION><B>Theorem List for ", htmlTitle,
        " - </B>", str3, "<B>-</B>", str4,
        /* 21-Jun-2014 - redundant, since it's in the title now
        "<B>",
        " - Page ",
        str(page), " of ",
        str(pages),
        "</B>",
        */
        " &nbsp; *Has distinct variable group(s)"
        "</CAPTION>",NULL),
        " ",  /* Start continuation line with space */
        "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
    print2("\n");
    print2("<TR><TH>Type</TH><TH>Label</TH><TH>Description</TH></TR>\n");
    print2("<TR><TH COLSPAN=3>Statement</TH></TR>\n");
    print2("\n");
    print2("<TR BGCOLOR=white><TD COLSPAN=3><FONT SIZE=-3>&nbsp;</FONT></TD></TR>\n");
    print2("\n");
    fprintf(outputFilePtr, "%s", printString);
    outputToString = 0;
    let(&printString, "");

    /* Find the last assertion that will be printed on the page, so
       we will know when a separator between theorems is not needed */
    lastAssertion = 0;
    for (assertion = (page - 1) * theoremsPerPage + 1;
        assertion <= page * theoremsPerPage; assertion++) {
      if (assertion > assertions) break; /* We're beyond the end */

      /* nm 22-Jan-04 Don't count statements whose names begin with "xxx"
         because they will not be output */
      /* 9-May-2015 nm Can this be deleted?  Do we need it anymore? */
      /* Deleted 4-Aug-2018 nm
      if (strcmp("xxx", left(statement[s].labelName, 3))) {
        lastAssertion = assertion;
      }
      let(&str1, ""); /@ Purge string stack if too many left()'s @/
      */

      /* 4-Aug-2018 nm The above was replaced with: */
      lastAssertion = assertion;
    }

    /* Output theorems on the page */
    for (assertion = (page - 1) * theoremsPerPage + 1;
        assertion <= page * theoremsPerPage; assertion++) {
      if (assertion > assertions) break; /* We're beyond the end */

      s = nmbrStmtNmbr[assertion]; /* Statement number */
      /* Output only $p's, not $a's */
      /*if (statement[s].type != p_) continue;*/ /* Now do everything */

      /********* Deleted 3-May-2017 nm
      /@ nm 22-Jan-04 Skip statements whose labels begin "xxx" - this
         means they are temporary placeholders created by
         WRITE SOURCE / CLEAN in writeInput() in mmcmds.c @/
      let(&str1, ""); /@ Purge string stack if too many left()'s @/
      /@ 9-May-2015 nm Can this be deleted?  Do we need it anymore? @/
      if (!strcmp("xxx", left(statement[s].labelName, 3))) continue;
      ******/

      /* Construct the statement type label */
      if (statement[s].type == p_) {
        let(&str1, "Theorem");
      } else if (!strcmp("ax-", left(statement[s].labelName, 3))) {
        let(&str1, "<B><FONT COLOR=red>Axiom</FONT></B>");
      } else if (!strcmp("df-", left(statement[s].labelName, 3))) {
        let(&str1, "<B><FONT COLOR=blue>Definition</FONT></B>");
      } else {
        let(&str1, "<B><FONT COLOR=\"#00CC00\">Syntax</FONT></B>");
      }

      if (s == s + 0) goto skip_date;
      /* OBSOLETE */
      /* Get the date in the comment section after the statement */
      let(&str1, space(statement[s + 1].labelSectionLen));
      memcpy(str1, statement[s + 1].labelSectionPtr,
          (size_t)(statement[s + 1].labelSectionLen));
      let(&str1, edit(str1, 2)); /* Discard spaces and tabs */
      i1 = instr(1, str1, "$([");
      i2 = instr(i1, str1, "]$)");
      if (i1 && i2) {
        let(&str1, seg(str1, i1 + 3, i2 - 1));
      } else {
        let(&str1, "");
      }
     skip_date:

      let(&str3, "");
      str3 = getDescription(s);
      let(&str4, "");
      str4 = pinkHTML(s); /* Get little pink number */
      /* Output the description comment */
      /* Break up long lines for text editors with printLongLine */
      let(&printString, "");
      outputToString = 1;
      print2("\n"); /* Blank line for HTML source human readability */

      /* 31-Jul-2006 nm Table of contents mod */
      if (((vstring)(pntrHugeHdr[s]))[0]) { /* There is a major part break */
        printLongLine(cat(                                  /* 21-Jun-2014 */
                 /* The header */
                 "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3",
                 /* " ALIGN=CENTER><FONT SIZE=\"+1\"><B>", */
                 /* 9-May-2015 nm */
                 "><CENTER><FONT SIZE=\"+1\"><B>",
                 "<A NAME=\"mm", str((double)(statement[s].pinkNumber)),
                     "h\"></A>",   /* Anchor for table of contents */
                 (vstring)(pntrHugeHdr[s]),
                 /* "</B></FONT></TD></TR>", */
                 /* 9-May-2015 nm */
                 "</B></FONT></CENTER>",
                 NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

        /* 8-May-2015 nm */
        /* The comment part of the header, if any */
        if (((vstring)(pntrHugeHdrComment[s]))[0]) {

          /* Open the table row */
          /* print2("%s\n", "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3>"); */\
          /* 9-May-2015 nm - keep comment in same table cell */
          print2("%s\n", "<P STYLE=\"margin-bottom:0em\">");

          /* We are currently printing to printString to allow use of
             printLongLine(); however, the rendering function
             printTexComment uses printString internally, so we have to
             flush the current printString and turn off outputToString mode
             in order to call the rendering function printTexComment. */
          /* (Question:  why do the calls to printTexComment for statement
             descriptions, later, not need to flush the printString?  Is the
             flushing code here redundant?) */
          /* Clear out the printString output in prep for printTexComment */
          outputToString = 0;
          fprintf(outputFilePtr, "%s", printString);
          let(&printString, "");
          showStatement = s; /* For printTexComment */
          texFilePtr = outputFilePtr; /* For printTexComment */
          /* 8-May-2015 ???Future - make this just return a string??? */
          /* printTexComment((vstring)(pntrHugeHdrComment[s]), 0); */
          /* 17-Nov-2015 nm Add 3rd & 4th arguments */
          printTexComment(  /* Sends result to texFilePtr */
              (vstring)(pntrHugeHdrComment[s]),
              0, /* 1 = htmlCenterFlag */
              PROCESS_EVERYTHING, /* actionBits */ /* 13-Dec-2018 nm */
              0 /* 1 = noFileCheck */);
          texFilePtr = NULL;
          outputToString = 1; /* Restore after printTexComment */

          /* Close the table row */
          /* print2("%s\n", "</TD></TR>"); */ /* 9-May-2015 nm */
        }

        /* 9-May-2015 nm */
        /* Close the table row */
        print2("%s\n", "</TD></TR>");

        printLongLine(cat(                                  /* 21-Jun-2014 */
                 /* Separator row */
                 "<TR BGCOLOR=white><TD COLSPAN=3>",
                 "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>",
                 NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
      }
      if (((vstring)(pntrBigHdr[s]))[0]) { /* There is a major section break */
        printLongLine(cat(
                 /* The header */
                 "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3",
                 /* " ALIGN=CENTER><FONT SIZE=\"+1\"><B>", */
                 /* 9-May-2015 nm */
                 "><CENTER><FONT SIZE=\"+1\"><B>",
                 "<A NAME=\"mm", str((double)(statement[s].pinkNumber)),
                     "b\"></A>",      /* Anchor for table of contents */
                 (vstring)(pntrBigHdr[s]),
                 /* "</B></FONT></TD></TR>", */
                 /* 9-May-2015 nm */
                 "</B></FONT></CENTER>",
                 NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

        /* 8-May-2015 nm */
        /* The comment part of the header, if any */
        if (((vstring)(pntrBigHdrComment[s]))[0]) {

          /* Open the table row */
          /* print2("%s\n", "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3>"); */\
          /* 9-May-2015 nm - keep comment in same table cell */
          print2("%s\n", "<P STYLE=\"margin-bottom:0em\">");

          /* We are currently printing to printString to allow use of
             printLongLine(); however, the rendering function
             printTexComment uses printString internally, so we have to
             flush the current printString and turn off outputToString mode
             in order to call the rendering function printTexComment. */
          /* (Question:  why do the calls to printTexComment for statement
             descriptions, later, not need to flush the printString?  Is the
             flushing code here redundant?) */
          /* Clear out the printString output in prep for printTexComment */
          outputToString = 0;
          fprintf(outputFilePtr, "%s", printString);
          let(&printString, "");
          showStatement = s; /* For printTexComment */
          texFilePtr = outputFilePtr; /* For printTexComment */
          /* 8-May-2015 ???Future - make this just return a string??? */
          /* printTexComment((vstring)(pntrBigHdrComment[s]), 0); */
          /* 17-Nov-2015 nm Add 3rd & 4th arguments */
          printTexComment(  /* Sends result to texFilePtr */
              (vstring)(pntrBigHdrComment[s]),
              0, /* 1 = htmlCenterFlag */
              PROCESS_EVERYTHING, /* actionBits */ /* 13-Dec-2018 nm */
              0  /* 1 = noFileCheck */);
          texFilePtr = NULL;
          outputToString = 1; /* Restore after printTexComment */

          /* Close the table row */
          /* print2("%s\n", "</TD></TR>"); */ /* 9-May-2015 nm */
        }

        /* 9-May-2015 nm */
        /* Close the table row */
        print2("%s\n", "</TD></TR>");

        printLongLine(cat(                                  /* 21-Jun-2014 */
                 /* Separator row */
                 "<TR BGCOLOR=white><TD COLSPAN=3>",
                 "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>",
                 NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
      }
      if (((vstring)(pntrSmallHdr[s]))[0]) { /* There is a minor sec break */
        printLongLine(cat(
                 /* The header */
                 "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3",
                 /* " ALIGN=CENTER><B>", */
                 /* 9-May-2015 nm */
                 "><CENTER><B>",
                 "<A NAME=\"mm", str((double)(statement[s].pinkNumber)),
                     "s\"></A>",    /* Anchor for table of contents */
                 (vstring)(pntrSmallHdr[s]),
                 /* "</B></TD></TR>", */
                 /* 9-May-2015 nm */
                 "</B></CENTER>",
                 NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

        /* 8-May-2015 nm */
        /* The comment part of the header, if any */
        if (((vstring)(pntrSmallHdrComment[s]))[0]) {

          /* Open the table row */
          /* print2("%s\n", "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3>"); */\
          /* 9-May-2015 nm - keep comment in same table cell */
          print2("%s\n", "<P STYLE=\"margin-bottom:0em\">");

          /* We are currently printing to printString to allow use of
             printLongLine(); however, the rendering function
             printTexComment uses printString internally, so we have to
             flush the current printString and turn off outputToString mode
             in order to call the rendering function printTexComment. */
          /* (Question:  why do the calls to printTexComment for statement
             descriptions, later, not need to flush the printString?  Is the
             flushing code here redundant?) */
          /* Clear out the printString output in prep for printTexComment */
          outputToString = 0;
          fprintf(outputFilePtr, "%s", printString);
          let(&printString, "");
          showStatement = s; /* For printTexComment */
          texFilePtr = outputFilePtr; /* For printTexComment */
          /* 8-May-2015 ???Future - make this just return a string??? */
          /* printTexComment((vstring)(pntrSmallHdrComment[s]), 0); */
          /* 17-Nov-2015 nm Add 3rd & 4th arguments */
          printTexComment(  /* Sends result to texFilePtr */
              (vstring)(pntrSmallHdrComment[s]),
              0, /* 1 = htmlCenterFlag */
              PROCESS_EVERYTHING, /* actionBits */ /* 13-Dec-2018 nm */
              0  /* 1 = noFileCheck */);
          texFilePtr = NULL;
          outputToString = 1; /* Restore after printTexComment */

          /* Close the table row */
          /* print2("%s\n", "</TD></TR>"); */ /* 9-May-2015 nm */
        }

        /* 9-May-2015 nm */
        /* Close the table row */
        print2("%s\n", "</TD></TR>");

        printLongLine(cat(                                  /* 21-Jun-2014 */
                 /* Separator row */
                 "<TR BGCOLOR=white><TD COLSPAN=3>",
                 "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>",
                 NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
      }

      /* Added 21-Aug-2017 nm */
      if (((vstring)(pntrTinyHdr[s]))[0]) { /* There is a subsubsection break */
        printLongLine(cat(
                 /* The header */
                 "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3",
                 /* " ALIGN=CENTER><B>", */
                 /* 9-May-2015 nm */
                 "><CENTER><B>",
                 "<A NAME=\"mm", str((double)(statement[s].pinkNumber)),
                     "s\"></A>",    /* Anchor for table of contents */
                 (vstring)(pntrTinyHdr[s]),
                 /* "</B></TD></TR>", */
                 /* 9-May-2015 nm */
                 "</B></CENTER>",
                 NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

        /* 8-May-2015 nm */
        /* The comment part of the header, if any */
        if (((vstring)(pntrTinyHdrComment[s]))[0]) {

          /* Open the table row */
          /* print2("%s\n", "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3>"); */\
          /* 9-May-2015 nm - keep comment in same table cell */
          print2("%s\n", "<P STYLE=\"margin-bottom:0em\">");

          /* We are currently printing to printString to allow use of
             printLongLine(); however, the rendering function
             printTexComment uses printString internally, so we have to
             flush the current printString and turn off outputToString mode
             in order to call the rendering function printTexComment. */
          /* (Question:  why do the calls to printTexComment for statement
             descriptions, later, not need to flush the printString?  Is the
             flushing code here redundant?) */
          /* Clear out the printString output in prep for printTexComment */
          outputToString = 0;
          fprintf(outputFilePtr, "%s", printString);
          let(&printString, "");
          showStatement = s; /* For printTexComment */
          texFilePtr = outputFilePtr; /* For printTexComment */
          /* 8-May-2015 ???Future - make this just return a string??? */
          /* printTexComment((vstring)(pntrTinyHdrComment[s]), 0); */
          /* 17-Nov-2015 nm Add 3rd & 4th arguments */
          printTexComment(  /* Sends result to texFilePtr */
              (vstring)(pntrTinyHdrComment[s]),
              0, /* 1 = htmlCenterFlag */
              PROCESS_EVERYTHING, /* actionBits */ /* 13-Dec-2018 nm */
              0  /* 1 = noFileCheck */);
          texFilePtr = NULL;
          outputToString = 1; /* Restore after printTexComment */

          /* Close the table row */
          /* print2("%s\n", "</TD></TR>"); */ /* 9-May-2015 nm */
        }

        /* 9-May-2015 nm */
        /* Close the table row */
        print2("%s\n", "</TD></TR>");

        printLongLine(cat(                                  /* 21-Jun-2014 */
                 /* Separator row */
                 "<TR BGCOLOR=white><TD COLSPAN=3>",
                 "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>",
                 NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
      }
      /* (End of 21-Aug-2017 addition) */

      printLongLine(cat(
            (s < extHtmlStmt)
               ? "<TR>"
               : (s < sandboxStmt)
                   ? cat("<TR BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL)
                   /* 29-Jul-2008 nm Sandbox stuff */
                   : cat("<TR BGCOLOR=", SANDBOX_COLOR, ">", NULL),
            "<TD NOWRAP>",  /* IE breaks up the date */
            str1, /* Date */
            "</TD><TD ALIGN=CENTER><A HREF=\"",
            statement[s].labelName, ".html\">",
            statement[s].labelName, "</A>",
            str4,

            /* 5-Jan-2014 nm */
            /* Add asterisk if statement has distinct var groups */
            (nmbrLen(statement[s].reqDisjVarsA) > 0) ? "*" : "",

            "</TD><TD ALIGN=LEFT>",
            /* 15-Aug-04 nm - Add anchor for hyperlinking to the table row */
            "<A NAME=\"", statement[s].labelName, "\"></A>",

            NULL),  /* Description */
          " ",  /* Start continuation line with space */
          "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

      showStatement = s; /* For printTexComment */
      outputToString = 0; /* For printTexComment */
      texFilePtr = outputFilePtr; /* For printTexComment */
      /* 18-Sep-03 ???Future - make this just return a string??? */
      /* printTexComment(str3, 0); */ /* Sends result to texFilePtr */
      /* 17-Nov-2015 nm Add 3rd & 4th arguments */
      printTexComment(  /* Sends result to texFilePtr */
          str3,
          0, /* 1 = htmlCenterFlag */
          PROCESS_EVERYTHING, /* actionBits */ /* 13-Dec-2018 nm */
          0  /* 1 = noFileCheck */);
      texFilePtr = NULL;
      outputToString = 1; /* Restore after printTexComment */

      /* Get HTML hypotheses => assertion */
      let(&str4, "");
      str4 = getTexOrHtmlHypAndAssertion(s); /* In mmwtex.c */

      /* 19-Aug-05 nm Suppress the math content of lemmas, which can
         be very big and not interesting */
      if (!strcmp(left(str3, 10), "Lemma for ")
          && !showLemmas) {  /* 10-Oct-2012 nm */
        /* Suppress the table row with the math content */
        print2(" <I>[Auxiliary lemma - not displayed.]</I></TD></TR>\n");
      } else {
        /* Output the table row with the math content */
        printLongLine(cat("</TD></TR><TR",

              /*
              (s < extHtmlStmt) ?
                   ">" :
                   cat(" BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL),
              */

              /* 29-Jul-2008 nm Sandbox stuff */
              (s < extHtmlStmt)
                 ? ">"
                 : (s < sandboxStmt)
                     ? cat(" BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL)
                     : cat(" BGCOLOR=", SANDBOX_COLOR, ">", NULL),


            /*** old
            "<TD BGCOLOR=white>&nbsp;</TD><TD COLSPAN=2 ALIGN=CENTER>",
            str4, "</TD></TR>", NULL),
            ****/
            /* 27-Oct-03 nm */
            "<TD COLSPAN=3 ALIGN=CENTER>",
            str4, "</TD></TR>", NULL),

            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
      }

      outputToString = 0;
      fprintf(outputFilePtr, "%s", printString);
      let(&printString, "");

      if (assertion != lastAssertion) {
        /* Put separator row if not last theorem */
        outputToString = 1;
        printLongLine(cat("<TR BGCOLOR=white><TD COLSPAN=3>",
            "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>", NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
        outputToString = 0;
        fprintf(outputFilePtr, "%s", printString);
        let(&printString, "");
      }
    } /* next assertion */

    /* Output trailer */
    outputToString = 1;
    print2("</TABLE></CENTER>\n");
    print2("\n");

    /* 8-May-2015 */
   SKIP_LIST: /* (skipped when page == 0) */
    outputToString = 1; /* To compensate for skipped assignment above */


    /* 21-Jun-2014 nm Put extra Prev/Next hyperlinks here for convenience */
    print2("<TABLE BORDER=0 WIDTH=\"100%c\">\n", '%');
    print2("  <TR>\n");
    print2("    <TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%c\">\n", '%');
    print2("      &nbsp;\n");
    /*
    print2("      <FONT SIZE=-1 FACE=sans-serif>\n");
    print2("      <A HREF=\"mmset.html\">MPE Home</A>&nbsp;&nbsp;\n");
    print2(
  "      <A HREF=\"mmtheorems.html#mmtc\">Table of contents</A></FONT>\n");
    */
    print2("    </TD>\n");
    print2("    <TD NOWRAP ALIGN=CENTER>&nbsp;</TD>\n");
    print2("    <TD NOWRAP ALIGN=RIGHT VALIGN=TOP WIDTH=\"25%c\"><FONT\n", '%');
    print2("      SIZE=-1 FACE=sans-serif>\n");
    printLongLine(cat("      ", prevNextLinks, NULL),
        " ",  /* Start continuation line with space */
        "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
    print2("      </FONT></TD>\n");
    print2("  </TR>\n");
    print2("</TABLE>\n");


    print2("<HR NOSHADE SIZE=1>\n");


    outputToString = 0;
    fprintf(outputFilePtr, "%s", printString);
    let(&printString, "");


    fprintf(outputFilePtr, "<A NAME=\"mmpglst\"></A>\n");
    fprintf(outputFilePtr, "<CENTER>\n");
    fprintf(outputFilePtr, "<B>Page List</B>\n");
    fprintf(outputFilePtr, "</CENTER>\n");


    /* 21-Jun-2014 nm Moved page list to bottom */
    /* Output links to the other pages */
    fprintf(outputFilePtr, "Jump to page: \n");
    /* for (p = 1; p <= pages; p++) { */
    /* 8-May-2015 */
    for (p = 0; p <= pages; p++) {

      /* Construct the pink number range */
      let(&str3, "");
      if (p > 0) {   /* 8-May-2015 */
        str3 = pinkRangeHTML(
            nmbrStmtNmbr[(p - 1) * theoremsPerPage + 1],
            (p < pages) ?
              nmbrStmtNmbr[p * theoremsPerPage] :
              nmbrStmtNmbr[assertions]);
      }

      /* 31-Jul-2006 nm Change "1" to "Contents + 1" */
      if (p == page) {
        let(&str1,
            /* (p == 1) ? "Contents + 1" : str(p) */ /* 31-Jul-2006 nm */
            /* 8-May-2015 nm */
            (p == 0) ? "Contents" : str((double)p)
            ); /* Current page shouldn't have link to self */
      } else {
        let(&str1, cat("<A HREF=\"mmtheorems",
            /* (p == 1) ? "" : str(p), */
            /* 8-May-2015 nm */
            (p == 0) ? "" : str((double)p),
            /* (p == 1) ? ".html#mmtc\">" : ".html\">", */ /* 31-Aug-2006 nm */
            ".html\">", /* 8-Feb-2007 nm Friendlier, because you can start
                  scrolling through the page before it finishes loading,
                  without its jumping to #mmtc (start of Table of Contents)
                  when it's done. */
            /* (p == 1) ? "Contents + 1" : str(p) */ /* 31-Jul-2006 nm */
            /* 8-May-2015 nm */
            (p == 0) ? "Contents" : str((double)p)
            , "</A>", NULL));
      }
      let(&str1, cat(str1, PINK_NBSP, str3, NULL));
      fprintf(outputFilePtr, "%s\n", str1);
    }
    /* End of 21-Jun-2014 */


    outputToString = 1;
    print2("<HR NOSHADE SIZE=1>\n");
    /* nm 22-Jan-04 Take out date because it causes an unnecessary incremental
       site update */
    /*
    print2(" <CENTER><I>\n");
    print2("This page was last updated on %s.\n", date());
    print2("</I></CENTER>\n");
    */

    /*
    print2("<CENTER><FONT SIZE=-2 FACE=ARIAL>\n");
    print2("<A HREF=\"http://metamath.org\">metamath.org mirrors</A>\n");
    print2("</FONT></CENTER>\n");
    */
    /* Add a "Previous" and "Next" links to bottom of page for convenience */
    /************ 21-Jun-2014 nm Done above, put into prevNextLinks string
    let(&str1, cat("<A HREF=\"mmtheorems",
        (page > 1)
            ? ((page - 1 > 1) ? str(page - 1) : "")
            : ((pages > 1) ? str(pages) : ""),
        ".html\">", NULL));
    if (page > 1) {
      let(&str1, cat(str1, "&lt; Previous</A>&nbsp;&nbsp;", NULL));
    } else {
      let(&str1, cat(str1, "&lt; Wrap</A>&nbsp;&nbsp;", NULL));
    }
    let(&str1, cat(str1, "<A HREF=\"mmtheorems",
        (page < pages)
            ? str(page + 1)
            : "",
        ".html\">", NULL));
    if (page < pages) {
      let(&str1, cat(str1, "Next &gt;</A>", NULL));
    } else {
      let(&str1, cat(str1, "Wrap &gt;</A>", NULL));
    }
    *************/

    print2("<TABLE BORDER=0 WIDTH=\"100%c\">\n", '%');
    print2("  <TR>\n");
  /*print2("    <TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%c\">&nbsp;</TD>\n", '%');*/
    /* 31-Aug-2006 nm Changed above line to the 4 following lines: */
    print2("    <TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%c\">\n", '%');
    print2("      &nbsp;\n");
    /*
    print2("      <FONT SIZE=-1 FACE=sans-serif>\n");
    print2("      <A HREF=\"mmset.html\">MPE Home</A>&nbsp;&nbsp;\n");
    print2(
  "      <A HREF=\"mmtheorems.html#mmtc\">Table of contents</A></FONT>\n");
    */
    print2("    </TD>\n");
    print2("    <TD NOWRAP ALIGN=CENTER><FONT SIZE=-2\n");
    print2("      FACE=ARIAL>\n");
    /*
    print2("      <A HREF=\"http://metamath.org\">metamath.org mirrors</A>\n");
    */
    print2("Copyright terms:\n");
    print2("<A HREF=\"../copyright.html#pd\">Public domain</A>\n");
    print2("      </FONT></TD>\n");
    print2("    <TD NOWRAP ALIGN=RIGHT VALIGN=TOP WIDTH=\"25%c\"><FONT\n", '%');
    print2("      SIZE=-1 FACE=sans-serif>\n");
    /*printLongLine(cat("      ", str1, NULL),*/
    printLongLine(cat("      ", prevNextLinks, NULL), /* 21-Jun-2014 */
        " ",  /* Start continuation line with space */
        "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
    print2("      </FONT></TD>\n");
    print2("  </TR>\n");
    print2("</TABLE>\n");

    /* Todo: Decide to use or not use this */
    /*
    print2("<SCRIPT SRC=\"http://www.google-analytics.com/urchin.js\"\n");
    print2("  TYPE=\"text/javascript\">\n");
    print2("</SCRIPT>\n");
    print2("<SCRIPT TYPE=\"text/javascript\">\n");
    print2("  _uacct = \"UA-1862729-1\";\n");
    print2("  urchinTracker();\n");
    print2("</SCRIPT>");
    */

    print2("</BODY></HTML>\n");
    outputToString = 0;
    fprintf(outputFilePtr, "%s", printString);
    let(&printString, "");

    /* Close file */
    fclose(outputFilePtr);
  } /* next page */

 TL_ABORT:
  /* Deallocate memory */
  let(&str1, "");
  let(&str3, "");
  let(&str4, "");
  let(&prevNextLinks, "");
  let(&outputFileName, "");
  let(&hugeHdr, "");
  let(&bigHdr, "");
  let(&smallHdr, "");
  let(&tinyHdr, ""); /* 21-Aug-2017 nm */
  let(&hdrCommentMarker, ""); /* 4-Aug-2018 */
  for (i = 0; i <= statements; i++) let((vstring *)(&pntrHugeHdr[i]), "");
  pntrLet(&pntrHugeHdr, NULL_PNTRSTRING);
  for (i = 0; i <= statements; i++) let((vstring *)(&pntrBigHdr[i]), "");
  pntrLet(&pntrBigHdr, NULL_PNTRSTRING);
  for (i = 0; i <= statements; i++) let((vstring *)(&pntrSmallHdr[i]), "");
  pntrLet(&pntrSmallHdr, NULL_PNTRSTRING);
  /* 21-Aug-2017 nm */
  for (i = 0; i <= statements; i++) let((vstring *)(&pntrTinyHdr[i]), "");
  pntrLet(&pntrTinyHdr, NULL_PNTRSTRING);

} /* writeTheoremList */


/* 18-Dec-2016 nm - use true "header area" as described below, and
   ensure statement argument is $p or $a */
/* 2-Aug-2009 nm - broke this function out from writeTheoremList() */
/* 21-Jun-2014 nm - added hugeHdrTitle */
/* 21-Aug-2017 nm - added tinyHdrTitle */
/* 8-May-2015 nm - added hugeHdrComment, bigHdrComment, smallHdrComment */
/* 21-Aug-2017 nm - added tinyHdrComment */
/* This function extracts any section headers in the comment sections
   prior to the label of statement stmt.   If a huge (####...) header isn't
   found, *hugeHdrTitle will be set to the empty string.
   If a big (#*#*...) header isn't found (or isn't after the last huge header),
   *bigHdrTitle will be set to the empty string.  If a small
   (=-=-...) header isn't found (or isn't after the last huge header or
   the last big header), *smallHdrTitle will be set to the empty string.
   In all 3 cases, only the last occurrence of a header is considered.
   If a tiny
   (-.-....) header isn't found (or isn't after the last huge header or
   the last big header or the last small header), *tinyHdrTitle will be
   set to the empty string.
   In all 4 cases, only the last occurrence of a header is considered. */
/*
20-Jun-2015 metamath Google group email:

There are 3 kinds of section headers, big (####...), medium (#*#*#*...),
and small (=-=-=-).

Call the collection of (outside-of-statement) comments between two
successive $a/$p statements (i.e. those statements that generate web
pages) a "header area".  The algorithm scans the header area for the
_last_ header of each type (big, medium, small) and discards all others.
Then, if there is a medium and it appears before a big, the medium is
discarded.  If there is a small and it appears before a big or medium,
the small is discarded.  In other words, a maximum of one header of
each type is kept, in the order big, medium, and small.

There are two reasons for doing this:  (1) it disregards headers used
for other purposes such as the headers for the set.mm-specific
information at the top that is not of general interest and (2) it
ignores headers for empty sections; for example, a mathbox user might
have a bunch of headers for sections planned for the future, but we
ignore them if those sections are empty (no $a or $p in them).

21-Aug-2017: added "tiny" to "big, medium, small".
*/
void getSectionHeadings(long stmt, vstring *hugeHdrTitle, vstring *bigHdrTitle,
    vstring *smallHdrTitle,
    vstring *tinyHdrTitle,  /* 21-Aug-2017 nm */
    /* Added 8-May-2015 nm */
    vstring *hugeHdrComment,
    vstring *bigHdrComment,
    vstring *smallHdrComment,
    vstring *tinyHdrComment) {  /* 21-Aug-2017 nm */

  /* 31-Jul-2006 for table of contents mod */
  vstring labelStr = "";
  long pos, pos1, pos2, pos3, pos4;

  /* 18-Dec-2016 nm */
  /* We now process only $a or $p statements */
  if (statement[stmt].type != a_ && statement[stmt].type != p_) bug(2340);

  /* 18-Dec-2016 nm */
  /* Get header area between this statement and the statement after the
     previous $a or $p statement */
  /* pos3 and pos4 are used temporarily here; not related to later use */
  pos3 = statement[stmt].headerStartStmt;  /* Statement immediately after the
                previous $a or $p statement (will be this statement if previous
                statement is $a or $p) */
  if (pos3 == 0 || pos3 > stmt) bug(2241);
  pos4 = (statement[stmt].labelSectionPtr
        - statement[pos3].labelSectionPtr)
        + statement[stmt].labelSectionLen;  /* Length of the header area */
  let(&labelStr, space(pos4));
  memcpy(labelStr, statement[pos3].labelSectionPtr,
      (size_t)(pos4));

  /* Old code before 18-Dec-2016
  /@ Get headers from comment section between statements @/
  let(&labelStr, space(statement[stmt].labelSectionLen));
  memcpy(labelStr, statement[stmt].labelSectionPtr,
      (size_t)(statement[stmt].labelSectionLen));
  */

  pos = 0;
  pos2 = 0;
  while (1) {  /* Find last "huge" header, if any */
    /* 4-Nov-2007 nm:  Obviously, the match below will not work if the
       $( line has a trailing space, which some editors might insert.
       The symptom is a missing table of contents entry.  But to detect
       this (and for the #*#* and =-=- matches below) would take a little work
       and perhaps slow things down, and I don't think it is worth it.  I
       put a note in HELP WRITE THEOREM_LIST. */
    pos1 = pos; /* 23-May-2008 */
    pos = instr(pos + 1, labelStr, "$(\n####");

    /* 23-May-2008 nm Tolerate one space after "$(", to handle case of
      one space added to the end of each line with TOOLS to make global
      label changes are easier (still a kludge; this should be made
      white-space insensitive some day) */
    pos1 = instr(pos1 + 1, labelStr, "$( \n####");
    if (pos1 > pos) pos = pos1;

    if (!pos) break;
    if (pos) pos2 = pos;
  }
  if (pos2) { /* Extract "huge" header */
    pos = instr(pos2 + 4, labelStr, "\n"); /* Get to end of #### line */
    pos2 = instr(pos + 1, labelStr, "\n"); /* Find end of title line */
    pos3 = instr(pos2 + 1, labelStr, "\n"); /* Get to end of 2nd #### line */
    while (labelStr[(pos3 - 1) + 1] == '\n') pos3++; /* Skip 1st blank lines */
    pos4 = instr(pos3, labelStr, "$)"); /* Get to end of title comment */
    let(&(*hugeHdrTitle), seg(labelStr, pos + 1, pos2 - 1));
    let(&(*hugeHdrTitle), edit((*hugeHdrTitle), 8 + 128));
                                                /* Trim leading, trailing sp */
    let(&(*hugeHdrComment), seg(labelStr, pos3 + 1, pos4 - 2));
    let(&(*hugeHdrComment), edit((*hugeHdrComment), 8 + 16384));
                                        /* Trim leading sp, trailing sp & lf */
  }
  /* pos = 0; */ /* Leave pos alone so that we start with "huge" header pos,
                    to ignore any earlier "tiny" or "small" or "big" header */
  pos2 = 0;
  while (1) {  /* Find last "big" header, if any */
    /* nm 4-Nov-2007:  Obviously, the match below will not work if the
       $( line has a trailing space, which some editors might insert.
       The symptom is a missing table of contents entry.  But to detect
       this (and for the =-=- match below) would take a little work and
       perhaps slow things down, and I don't think it is worth it.  I
       put a note in HELP WRITE THEOREM_LIST. */
    pos1 = pos; /* 23-May-2008 */
    pos = instr(pos + 1, labelStr, "$(\n#*#*");

    /* 23-May-2008 nm Tolerate one space after "$(", to handle case of
      one space added to the end of each line with TOOLS to make global
      label changes are easier (still a kludge; this should be made
      white-space insensitive some day) */
    pos1 = instr(pos1 + 1, labelStr, "$( \n#*#*");
    if (pos1 > pos) pos = pos1;

    if (!pos) break;
    if (pos) pos2 = pos;
  }
  if (pos2) { /* Extract "big" header */
    pos = instr(pos2 + 4, labelStr, "\n"); /* Get to end of #*#* line */
    pos2 = instr(pos + 1, labelStr, "\n"); /* Find end of title line */
    pos3 = instr(pos2 + 1, labelStr, "\n"); /* Get to end of 2nd #*#* line */
    while (labelStr[(pos3 - 1) + 1] == '\n') pos3++; /* Skip 1st blank lines */
    pos4 = instr(pos3, labelStr, "$)"); /* Get to end of title comment */
    let(&(*bigHdrTitle), seg(labelStr, pos + 1, pos2 - 1));
    let(&(*bigHdrTitle), edit((*bigHdrTitle), 8 + 128));
                                                /* Trim leading, trailing sp */
    let(&(*bigHdrComment), seg(labelStr, pos3 + 1, pos4 - 2));
    let(&(*bigHdrComment), edit((*bigHdrComment), 8 + 16384));
                                        /* Trim leading sp, trailing sp & lf */
  }
  /* pos = 0; */ /* Leave pos alone so that we start with "big" header pos,
                    to ignore any earlier "tiny" or "small" header */
  pos2 = 0;
  while (1) {  /* Find last "small" header, if any */
    pos1 = pos; /* 23-May-2008 */
    pos = instr(pos + 1, labelStr, "$(\n=-=-");

    /* 23-May-2008 nm Tolerate one space after "$(", to handle case of
      one space added to the end of each line with TOOLS to make global
      label changes are easier (still a kludge; this should be made
      white-space insensitive some day) */
    pos1 = instr(pos1 + 1, labelStr, "$( \n=-=-");
    if (pos1 > pos) pos = pos1;

    if (!pos) break;
    if (pos) pos2 = pos;
  }
  if (pos2) { /* Extract "small" header */
    pos = instr(pos2 + 4, labelStr, "\n"); /* Get to end of =-=- line */
    pos2 = instr(pos + 1, labelStr, "\n"); /* Find end of title line */
    pos3 = instr(pos2 + 1, labelStr, "\n"); /* Get to end of 2nd =-=- line */
    while (labelStr[(pos3 - 1) + 1] == '\n') pos3++; /* Skip 1st blank lines */
    pos4 = instr(pos3, labelStr, "$)"); /* Get to end of title comment */
    let(&(*smallHdrTitle), seg(labelStr, pos + 1, pos2 - 1));
    let(&(*smallHdrTitle), edit((*smallHdrTitle), 8 + 128));
                                                /* Trim leading, trailing sp */
    let(&(*smallHdrComment), seg(labelStr, pos3 + 1, pos4 - 2));
    let(&(*smallHdrComment), edit((*smallHdrComment), 8 + 16384));
                                        /* Trim leading sp, trailing sp & lf */
  }

  /* Added 21-Aug-2017 nm */
  /* pos = 0; */ /* Leave pos alone so that we start with "small" header pos,
                    to ignore any earlier "tiny" header */
  pos2 = 0;
  while (1) {  /* Find last "tiny" header, if any */
    pos1 = pos; /* 23-May-2008 */
    pos = instr(pos + 1, labelStr, "$(\n-.-.");

    /* 23-May-2008 nm Tolerate one space after "$(", to handle case of
      one space added to the end of each line with TOOLS to make global
      label changes are easier (still a kludge; this should be made
      white-space insensitive some day) */
    pos1 = instr(pos1 + 1, labelStr, "$( \n-.-.");
    if (pos1 > pos) pos = pos1;

    if (!pos) break;
    if (pos) pos2 = pos;
  }
  if (pos2) { /* Extract "tiny" header */
    pos = instr(pos2 + 4, labelStr, "\n"); /* Get to end of -.-. line */
    pos2 = instr(pos + 1, labelStr, "\n"); /* Find end of title line */
    pos3 = instr(pos2 + 1, labelStr, "\n"); /* Get to end of 2nd -.-. line */
    while (labelStr[(pos3 - 1) + 1] == '\n') pos3++; /* Skip 1st blank lines */
    pos4 = instr(pos3, labelStr, "$)"); /* Get to end of title comment */
    let(&(*tinyHdrTitle), seg(labelStr, pos + 1, pos2 - 1));
    let(&(*tinyHdrTitle), edit((*tinyHdrTitle), 8 + 128));
                                                /* Trim leading, trailing sp */
    let(&(*tinyHdrComment), seg(labelStr, pos3 + 1, pos4 - 2));
    let(&(*tinyHdrComment), edit((*tinyHdrComment), 8 + 16384));
                                        /* Trim leading sp, trailing sp & lf */
  }
  /* (End of 21-Aug-2017 addition) */

  let(&labelStr, "");  /* Deallocate string memory */
  return;
} /* getSectionHeadings */


/* Returns the pink number printed next to statement labels in HTML output */
/* The pink number only counts $a and $p statements, unlike the statement
   number which also counts $f, $e, $c, $v, ${, $} */
/* 10/10/02 This is no longer used? */
#ifdef DUMMY  /* For commenting it out */
long pinkNumber(long statemNum)
{
  long statemMap = 0;
  long i;
  /* Statement map for number of showStatement - this makes the statement
     number more meaningful, by counting only $a and $p. */
  /* ???This could be done once if we want to speed things up, but
     be careful because it will have to be redone if ERASE then READ.
     For the future it could be added to the statement[] structure. */
  statemMap = 0;
  for (i = 1; i <= statemNum; i++) {
    if (statement[i].type == a_ || statement[i].type == p_)
      statemMap++;
  }
  return statemMap;
} /* pinkNumber */
#endif

/* Added 10/10/02 */
/* Returns HTML for the pink number to print after the statement labels
   in HTML output.  (Note that "pink" means "rainbow colored" number now.) */
/* Warning: The caller must deallocate the returned vstring (i.e. this
   function cannot be used in let statements but must be assigned to
   a local vstring for local deallocation) */
vstring pinkHTML(long statemNum)
{
  long statemMap;
  vstring htmlCode = "";
  vstring hexValue = "";

  /* The pink number only counts $a and $p statements, unlike the statement
     number which also counts $f, $e, $c, $v, ${, $} */
  /* 10/25/02 Added pinkNumber to the statement[] structure for speedup. */
  /*
  statemMap = 0;
  for (i = 1; i <= statemNum; i++) {
    if (statement[i].type == a_ || statement[i].type == p_)
      statemMap++;
  }
  */
  if (statemNum > 0) {
    statemMap = statement[statemNum].pinkNumber;
  } else {
    /* -1 means the label wasn't found */
    statemMap = -1;
  }

  /* Note: we put "(future)" when the label wasn't found (an error message
     was also generated previously) */

  /* Without style sheet */
  /*
  let(&htmlCode, cat(PINK_NBSP,
      "<FONT FACE=\"Arial Narrow\" SIZE=-2 COLOR=", PINK_NUMBER_COLOR, ">",
      (statemMap != -1) ? str(statemMap) : "(future)", "</FONT>", NULL));
  */

#ifndef RAINBOW_OPTION
  /* With style sheet */
  let(&htmlCode, cat(PINK_NBSP,
      "<SPAN CLASS=p>",
      (statemMap != -1) ? str((double)statemMap) : "(future)", "</SPAN>", NULL));
#endif

#ifdef RAINBOW_OPTION
  /* ndm 10-Jan-04 With style sheet and explicit color */
  let(&hexValue, "");
  hexValue = spectrumToRGB(statemMap, statement[statements].pinkNumber);
  let(&htmlCode, cat(PINK_NBSP,
      "<SPAN CLASS=r STYLE=\"color:#", hexValue, "\">",
      (statemMap != -1) ? str((double)statemMap) : "(future)", "</SPAN>", NULL));
#endif
  let(&hexValue, "");

  return htmlCode;
} /* pinkHTML */


/* Added 25-Aug-04 */
/* Returns HTML for a range of pink numbers separated by a "-". */
/* Warning: The caller must deallocate the returned vstring (i.e. this
   function cannot be used in let statements but must be assigned to
   a local vstring for local deallocation) */
vstring pinkRangeHTML(long statemNum1, long statemNum2)
{
  vstring htmlCode = "";
  vstring str3 = "";
  vstring str4 = "";

  /* Construct the HTML for a pink number range */
  let(&str3, "");
  str3 = pinkHTML(statemNum1);
  let(&str3, right(str3, (long)strlen(PINK_NBSP) + 1)); /* Discard "&nbsp;" */
  let(&str4, "");
  str4 = pinkHTML(statemNum2);
  let(&str4, right(str4, (long)strlen(PINK_NBSP) + 1)); /* Discard "&nbsp;" */
  let(&htmlCode, cat(str3, "-", str4, NULL));
  let(&str3, ""); /* Deallocate */
  let(&str4, ""); /* Deallocate */
  return htmlCode;
} /* pinkRangeHTML */


#ifdef RAINBOW_OPTION
/* 20-Aug-2006 nm This section was revised so that all colors have
   the same grayscale brightness. */

/* This function converts a "spectrum" color (1 to maxColor) to an
   RBG value in hex notation for HTML.  The caller must deallocate the
   returned vstring to prevent memory leaks.  color = 1 (red) to maxColor
   (violet).  A special case is the color -1, which just returns black. */
/* ndm 10-Jan-04 */
vstring spectrumToRGB(long color, long maxColor) {
  vstring str1 = "";
  double fraction, fractionInPartition;
  long j, red, green, blue, partition;
/* Change PARTITIONS whenever the table below has entries added or removed! */
#define PARTITIONS 28
  static double redRef[PARTITIONS +  1];    /* 20-Aug-2006 nm Made these */
  static double greenRef[PARTITIONS +  1];  /*                static for */
  static double blueRef[PARTITIONS +  1];   /*                speedup */
  static long i = -1;                       /*                below */

  if (i > -1) goto SKIP_INIT; /* 20-Aug-2006 nm - Speedup */
  i = -1; /* For safety */

#define L53empirical
#ifdef L53empirical
  /* Here, we use the maximum saturation possible for a fixed L*a*b color L
     (lightness) value of 53, which corresponds to 50% gray scale.
     Each pure color had either brightness reduced or saturation reduced,
     as required, to achieve L = 53.

     The partitions in the 'ifdef L53obsolete' below were divided into 1000
     subpartitions, then new partitions were determined by reselecting
     partition boundaries based on where their color difference was
     distinguishable (i.e. could semi-comfortably read letters of one color
     with the other as a background, on an LCD display).  Some human judgment
     was involved, and it is probably not completely uniform or optimal.

     A Just Noticeable Difference (JND) algorithm for spacing might be more
     accurate, especially if averaged over several subjects and different
     monitors.  I wrote a program for that - asking the user to identify a word
     in one hue with an adjacent hue as a background, in order to score a
     point - but it was taking too much time, and I decided life is too short.
     I think this is "good enough" though, perhaps not even noticeably
     non-optimum.

     The comment at the end of each line is the hue 0-360 mapped linearly
     to 1-1043 (345 = 1000, i.e. "extreme" purple or almost red).  Partitions
     at the end can be commented out if we want to stop at violet instead of
     almost wrapping around to red via the purples, in order to more accurately
     emulate the color spectrum.  Be sure to update the PARTITIONS constant
     above if a partition is commented out, to avoid a bug trap. */
  i++; redRef[i] = 251; greenRef[i] = 0; blueRef[i] = 0;       /* 1 */
  i++; redRef[i] = 247; greenRef[i] = 12; blueRef[i] = 0;      /* 10 */
  i++; redRef[i] = 238; greenRef[i] = 44; blueRef[i] = 0;      /* 34 */
  i++; redRef[i] = 222; greenRef[i] = 71; blueRef[i] = 0;      /* 58 */
  i++; redRef[i] = 203; greenRef[i] = 89; blueRef[i] = 0;      /* 79 */
  i++; redRef[i] = 178; greenRef[i] = 108; blueRef[i] = 0;     /* 109 */
  i++; redRef[i] = 154; greenRef[i] = 122; blueRef[i] = 0;     /* 140 */
  i++; redRef[i] = 127; greenRef[i] = 131; blueRef[i] = 0;     /* 181 */
  i++; redRef[i] = 110; greenRef[i] = 136; blueRef[i] = 0;     /* 208 */
  i++; redRef[i] = 86; greenRef[i] = 141; blueRef[i] = 0;      /* 242 */
  i++; redRef[i] = 60; greenRef[i] = 144; blueRef[i] = 0;      /* 276 */
  i++; redRef[i] = 30; greenRef[i] = 147; blueRef[i] = 0;      /* 313 */
  i++; redRef[i] = 0; greenRef[i] = 148; blueRef[i] = 22;      /* 375 */
  i++; redRef[i] = 0; greenRef[i] = 145; blueRef[i] = 61;      /* 422 */
  i++; redRef[i] = 0; greenRef[i] = 145; blueRef[i] = 94;      /* 462 */
  i++; redRef[i] = 0; greenRef[i] = 143; blueRef[i] = 127;     /* 504 */
  i++; redRef[i] = 0; greenRef[i] = 140; blueRef[i] = 164;     /* 545 */
  i++; redRef[i] = 0; greenRef[i] = 133; blueRef[i] = 218;     /* 587 */
  i++; redRef[i] = 3; greenRef[i] = 127; blueRef[i] = 255;     /* 612 */
  i++; redRef[i] = 71; greenRef[i] = 119; blueRef[i] = 255;    /* 652 */
  i++; redRef[i] = 110; greenRef[i] = 109; blueRef[i] = 255;   /* 698 */
  i++; redRef[i] = 137; greenRef[i] = 99; blueRef[i] = 255;    /* 740 */
  i++; redRef[i] = 169; greenRef[i] = 78; blueRef[i] = 255;    /* 786 */
  i++; redRef[i] = 186; greenRef[i] = 57; blueRef[i] = 255;    /* 808 */
  i++; redRef[i] = 204; greenRef[i] = 33; blueRef[i] = 249;    /* 834 */
  i++; redRef[i] = 213; greenRef[i] = 16; blueRef[i] = 235;    /* 853 */
  i++; redRef[i] = 221; greenRef[i] = 0; blueRef[i] = 222;     /* 870 */
  i++; redRef[i] = 233; greenRef[i] = 0; blueRef[i] = 172;     /* 916 */
  i++; redRef[i] = 239; greenRef[i] = 0; blueRef[i] = 132;     /* 948 */
  /*i++; redRef[i] = 242; greenRef[i] = 0; blueRef[i] = 98;*/  /* 973 */
  /*i++; redRef[i] = 244; greenRef[i] = 0; blueRef[i] = 62;*/  /* 1000 */
#endif

#ifdef L53obsolete
  /* THIS IS OBSOLETE AND FOR HISTORICAL REFERENCE ONLY; IT MAY BE DELETED. */
  /* Here, we use the maximum saturation possible for a given L value of 53.
     Each pure color has either brightness reduced or saturation reduced,
     as appropriate, until the LAB color L (lightness) value is 53.  The
     comment at the end of each line is the hue (0 to 360).

     Unfortunately, equal hue differences are not equally distinguishable.
     The commented-out lines were an unsuccessful attempt to make them more
     uniform before the final empirical table above was determined. */
  i++; redRef[i] = 251; greenRef[i] = 0; blueRef[i] = 0; /* 0 r */
  i++; redRef[i] = 234; greenRef[i] = 59; blueRef[i] = 0; /* 15 */
  i++; redRef[i] = 196; greenRef[i] = 98; blueRef[i] = 0; /* 30 */
  i++; redRef[i] = 160; greenRef[i] = 120; blueRef[i] = 0; /* 45 */
  /*i++; redRef[i] = 131; greenRef[i] = 131; blueRef[i] = 0;*/ /* 60 */
  i++; redRef[i] = 104; greenRef[i] = 138; blueRef[i] = 0; /* 75 */
  /*i++; redRef[i] = 72; greenRef[i] = 144; blueRef[i] = 0;*/ /* 90 */
  /*i++; redRef[i] = 37; greenRef[i] = 147; blueRef[i] = 0;*/ /* 105 */
  i++; redRef[i] = 0; greenRef[i] = 148; blueRef[i] = 0; /* 120 g */
  /*i++; redRef[i] = 0; greenRef[i] = 148; blueRef[i] = 37;*/ /* 135 */
  /*i++; redRef[i] = 0; greenRef[i] = 145; blueRef[i] = 73;*/ /* 150 */
  i++; redRef[i] = 0; greenRef[i] = 145; blueRef[i] = 109; /* 165 */
  /*i++; redRef[i] = 0; greenRef[i] = 142; blueRef[i] = 142;*/ /* 180 */
  /*i++; redRef[i] = 0; greenRef[i] = 139; blueRef[i] = 185;*/ /* 195 */
  i++; redRef[i] = 0; greenRef[i] = 128; blueRef[i] = 255; /* 210 */
  /*i++; redRef[i] = 73; greenRef[i] = 119; blueRef[i] = 255;*/ /* 225 */
  i++; redRef[i] = 110; greenRef[i] = 110; blueRef[i] = 255; /* 240 b */
  i++; redRef[i] = 138; greenRef[i] = 99; blueRef[i] = 255; /* 255 */
  i++; redRef[i] = 168; greenRef[i] = 81; blueRef[i] = 255; /* 270 */
  i++; redRef[i] = 201; greenRef[i] = 40; blueRef[i] = 255; /* 285 */
  i++; redRef[i] = 222; greenRef[i] = 0; blueRef[i] = 222; /* 300 */
  i++; redRef[i] = 233; greenRef[i] = 0; blueRef[i] = 175; /* 315 */
  i++; redRef[i] = 241; greenRef[i] = 0; blueRef[i] = 120; /* 330 */
  /*i++; redRef[i] = 245; greenRef[i] = 0; blueRef[i] = 61;*/ /* 345 */
#endif

#ifdef L68obsolete
  /* THIS IS OBSOLETE AND FOR HISTORICAL REFERENCE ONLY; IT MAY BE DELETED. */
  /* Each pure color has either brightness reduced or saturation reduced,
     as appropriate, until the LAB color L (lightness) value is 68.

     L = 68 was for the original pink color #FA8072, but the purples end
     up too light to be seen easily on some monitors */
  i++; redRef[i] = 255; greenRef[i] = 122; blueRef[i] = 122; /* 0 r */
  i++; redRef[i] = 255; greenRef[i] = 127; blueRef[i] = 0; /* 30 */
  i++; redRef[i] = 207; greenRef[i] = 155; blueRef[i] = 0; /* 45 */
  i++; redRef[i] = 170; greenRef[i] = 170; blueRef[i] = 0; /* 60 */
  i++; redRef[i] = 93; greenRef[i] = 186; blueRef[i] = 0; /* 90 */
  i++; redRef[i] = 0; greenRef[i] = 196; blueRef[i] = 0; /* 120 g */
  i++; redRef[i] = 0; greenRef[i] = 190; blueRef[i] = 94; /* 150 */
  i++; redRef[i] = 0; greenRef[i] = 185; blueRef[i] = 185; /* 180 */
  i++; redRef[i] = 87; greenRef[i] = 171; blueRef[i] = 255; /* 210 */
  i++; redRef[i] = 156; greenRef[i] = 156; blueRef[i] = 255; /* 240 b */
  i++; redRef[i] = 197; greenRef[i] = 140; blueRef[i] = 255; /* 270 */
  /*i++; redRef[i] = 223; greenRef[i] = 126; blueRef[i] = 255;*/ /* 285 */
  i++; redRef[i] = 255; greenRef[i] = 100; blueRef[i] = 255; /* 300 */
  i++; redRef[i] = 255; greenRef[i] = 115; blueRef[i] = 185; /* 330 */
#endif

#ifdef L53S57obsolete
  /* THIS IS OBSOLETE AND FOR HISTORICAL REFERENCE ONLY; IT MAY BE DELETED. */
  /* Saturation is constant 57%; LAB color L (lightness) is constant 53.
     This looks nice - colors are consistent pastels due to holding saturation
     constant (the maximum possible due to blue) - but we lose some of the
     distinguishability provided by saturated colors */
  i++; redRef[i] = 206; greenRef[i] = 89; blueRef[i] = 89; /* 0 r */
  i++; redRef[i] = 184; greenRef[i] = 105; blueRef[i] = 79; /* 15 */
  i++; redRef[i] = 164; greenRef[i] = 117; blueRef[i] = 71; /* 30 */
  i++; redRef[i] = 145; greenRef[i] = 124; blueRef[i] = 62; /* 45 */
  /*i++; redRef[i] = 130; greenRef[i] = 130; blueRef[i] = 56;*/ /* 60 */
  /*i++; redRef[i] = 116; greenRef[i] = 135; blueRef[i] = 58;*/ /* 75 */
  i++; redRef[i] = 98; greenRef[i] = 137; blueRef[i] = 59; /* 90 */
  /*i++; redRef[i] = 80; greenRef[i] = 140; blueRef[i] = 60;*/ /* 105 */
  i++; redRef[i] = 62; greenRef[i] = 144; blueRef[i] = 62; /* 120 g */
  /*i++; redRef[i] = 61; greenRef[i] = 143; blueRef[i] = 82;*/ /* 135 */
  i++; redRef[i] = 61; greenRef[i] = 142; blueRef[i] = 102; /* 150 */
  /*i++; redRef[i] = 60; greenRef[i] = 139; blueRef[i] = 119;*/ /* 165 */
  /*i++; redRef[i] = 60; greenRef[i] = 140; blueRef[i] = 140;*/ /* 180 */
  /*i++; redRef[i] = 68; greenRef[i] = 136; blueRef[i] = 159;*/ /* 195 */
  i++; redRef[i] = 80; greenRef[i] = 132; blueRef[i] = 185; /* 210 */
  /*i++; redRef[i] = 93; greenRef[i] = 124; blueRef[i] = 216;*/ /* 225 */
  i++; redRef[i] = 110; greenRef[i] = 110; blueRef[i] = 255; /* 240 b */
  i++; redRef[i] = 139; greenRef[i] = 104; blueRef[i] = 242; /* 255 */
  i++; redRef[i] = 159; greenRef[i] = 96; blueRef[i] = 223; /* 270 */
  i++; redRef[i] = 178; greenRef[i] = 89; blueRef[i] = 207; /* 285 */
  i++; redRef[i] = 191; greenRef[i] = 82; blueRef[i] = 191; /* 300 */
  i++; redRef[i] = 194; greenRef[i] = 83; blueRef[i] = 166; /* 315 */
  i++; redRef[i] = 199; greenRef[i] = 86; blueRef[i] = 142; /* 330 */
  /*i++; redRef[i] = 202; greenRef[i] = 87; blueRef[i] = 116;*/ /* 345 */
#endif

  if (i != PARTITIONS) { /* Double-check future edits */
    print2("? %ld partitions but PARTITIONS = %ld\n", i, (long)PARTITIONS);
    bug(2326); /* Don't go further to prevent out-of-range references */
  }

 SKIP_INIT:
  if (color == -1) {
    let(&str1, "000000"); /* Return black for "(future)" color for labels with
                             missing theorems in comments */
    return str1;
  }

  if (color < 1 || color > maxColor) {
    bug(2327);
  }
  fraction = (1.0 * ((double)color - 1)) / (double)maxColor;
                                   /* Fractional position in "spectrum" */
  partition = (long)(PARTITIONS * fraction);  /* Partition number (integer) */
  if (partition >= PARTITIONS) bug(2325); /* Roundoff error? */
  fractionInPartition = 1.0 * (fraction - (1.0 * (double)partition) / PARTITIONS)
      * PARTITIONS; /* The fraction of this partition it covers */
  red = (long)(1.0 * (redRef[partition] +
          fractionInPartition *
              (redRef[partition + 1] - redRef[partition])));
  green = (long)(1.0 * (greenRef[partition] +
          fractionInPartition *
              (greenRef[partition + 1] - greenRef[partition])));
  blue = (long)(1.0 * (blueRef[partition] +
          fractionInPartition *
              (blueRef[partition + 1] - blueRef[partition])));
  /* debug */
  /* i=1;if (outputToString==0) {i=0;outputToString=1;} */
  /*   print2("p%ldc%ld\n", partition, color); outputToString=i; */
  /*printf("red %ld green %ld blue %ld\n", red, green, blue);*/

  if (red < 0 || green < 0 || blue < 0
      || red > 255 || green > 255 || blue > 255) {
    print2("%ld %ld %ld\n", red, green, blue);
    bug(2323);
  }
  let(&str1, "      ");
  j = sprintf(str1, "%02X%02X%02X", (unsigned int)red, (unsigned int)green,
      (unsigned int)blue);
  if (j != 6) bug(2324);
  /* debug */
  /*printf("<FONT COLOR='#%02X%02X%02X'>a </FONT>\n", red, green, blue);*/
  return str1;
} /* spectrumToRGB */
#endif    /* #ifdef RAINBOW_OPTION */


/* Added 20-Sep-03 (broken out of printTexLongMath() for better
   modularization) */
/* Returns the HTML code for GIFs (!altHtmlFlag) or Unicode (altHtmlFlag),
   or LaTeX when !htmlFlag, for the math string (hypothesis or conclusion) that
   is passed in. */
/* Warning: The caller must deallocate the returned vstring. */
vstring getTexLongMath(nmbrString *mathString, long statemNum)
{
  long pos;
  vstring tex = "";
  vstring texLine = "";
  vstring lastTex = "";
  flag alphnew, alphold, unknownnew, unknownold;

  if (!texDefsRead) bug(2322); /* TeX defs were not read */
  let(&texLine, "");

  let(&lastTex, "");
  for (pos = 0; pos < nmbrLen(mathString); pos++) {
    let(&tex, "");
    tex = tokenToTex(mathToken[mathString[pos]].tokenName, statemNum);
              /* tokenToTex allocates tex; we must deallocate it */
    if (!htmlFlag) {  /* LaTeX */
      /* If this token and previous token begin with letter, add a thin
           space between them */
      /* Also, anything not in table will have space added */
      alphnew = !!isalpha((unsigned char)(tex[0]));
      unknownnew = 0;
      if (!strcmp(left(tex, 10), "\\mbox{\\rm ")) { /* Token not in table */
        unknownnew = 1;
      }
      alphold = !!isalpha((unsigned char)(lastTex[0]));
      unknownold = 0;
      if (!strcmp(left(lastTex, 10), "\\mbox{\\rm ")) { /* Token not in table*/
        unknownold = 1;
      }
      /*if ((alphold && alphnew) || unknownold || (unknownnew && pos > 0)) {*/
      /* Put thin space only between letters and/or unknowns  11/3/94 */
      if ((alphold || unknownold) && (alphnew || unknownnew)) {
        /* Put additional thin space between two letters */
        /* 27-Jul-05 nm Added for new LaTeX output */
        if (!oldTexFlag) {
          let(&texLine, cat(texLine, "\\,", tex, " ", NULL));
        } else {
          let(&texLine, cat(texLine, "\\m{\\,", tex, "}", NULL));
        }
      } else {
        /* 27-Jul-05 nm Added for new LaTeX output */
        if (!oldTexFlag) {
          let(&texLine, cat(texLine, "", tex, " ", NULL));
        } else {
          let(&texLine, cat(texLine, "\\m{", tex, "}", NULL));
        }
      }
    } else {  /* HTML */

      /* 7/27/03 When we have something like "E. x e. om x = y", the lack of
         space between om and x looks ugly in HTML.  This kludge adds it in
         for restricted quantifiers not followed by parenthesis, in order
         to make the web page look a little nicer.  E.g. onminex. */
      /* Note that the space is put between the pos-1 and the pos tokens */
      if (pos >=4) {
        if (!strcmp(mathToken[mathString[pos - 2]].tokenName, "e.")
            && (!strcmp(mathToken[mathString[pos - 4]].tokenName, "E.")
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "A.")

              /* 20-Oct-2018 nm per Benoit's 3-Sep, 18-Sep, 9-Oct emails */
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "prod_")
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "E*")
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "iota_")
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "Disj_")

              /* 6-Apr-04 nm - indexed E! */
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "E!")
              /* 12-Nov-05 nm - finite sums */
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "sum_")
              /* 30-Sep-06 nm - infinite cartesian product */
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "X_")
              /* 23-Jan-04 nm - indexed union, intersection */
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "U_")
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "|^|_"))
            /* 23-Jan-04 nm - add space even for parenthesized arg */
            /*&& strcmp(mathToken[mathString[pos]].tokenName, "(")*/
            && strcmp(mathToken[mathString[pos]].tokenName, ")")
            /* It also shouldn't be restricted _to_ an expression in parens. */
            && strcmp(mathToken[mathString[pos - 1]].tokenName, "(")
            /* ...or restricted _to_ a union or intersection 1-Feb-05 */
            && strcmp(mathToken[mathString[pos - 1]].tokenName, "U.")
            && strcmp(mathToken[mathString[pos - 1]].tokenName, "|^|")
            /* ...or restricted _to_ an expression in braces */
            && strcmp(mathToken[mathString[pos - 1]].tokenName, "{")) {
          let(&texLine, cat(texLine, " ", NULL)); /* Add a space */
        }
      }
      /* This one puts a space between the 2 x's in a case like
         "E. x x = y".  E.g. cla4egf */
      if (pos >=2) {
        /* Match a token starting with a letter */
        if (isalpha((unsigned char)(mathToken[mathString[pos]].tokenName[0]))) {
          /* and make sure its length is 1 */
          if (!(mathToken[mathString[pos]].tokenName[1])) {

            /* 20-Sep-2017 nm */
            /* Make sure previous token is a letter also, to prevent unneeded
               space in "ran ( A ..." (e.g. rncoeq, dfiun3g) */
            /* Match a token starting with a letter */
            if (isalpha((unsigned char)(mathToken[mathString[pos - 1]].tokenName[0]))) {
              /* and make sure its length is 1 */
              if (!(mathToken[mathString[pos - 1]].tokenName[1])) {

                /* See if it's 1st letter in a quantified expression */
                if (!strcmp(mathToken[mathString[pos - 2]].tokenName, "E.")
                    || !strcmp(mathToken[mathString[pos - 2]].tokenName, "A.")
                    /* 26-Dec-2016 nm - "not free in" binder */
                    || !strcmp(mathToken[mathString[pos - 2]].tokenName, "F/")
                    /* 6-Apr-04 nm added E!, E* */
                    || !strcmp(mathToken[mathString[pos - 2]].tokenName, "E!")
      /* 4-Jun-06 nm added dom, ran for space btwn A,x in "E! x e. dom A x A y" */
                    || !strcmp(mathToken[mathString[pos - 2]].tokenName, "ran")
                    || !strcmp(mathToken[mathString[pos - 2]].tokenName, "dom")
                    || !strcmp(mathToken[mathString[pos - 2]].tokenName, "E*")) {
                  let(&texLine, cat(texLine, " ", NULL)); /* Add a space */
                }

              } /* 20-Sep-2017 nm */
            } /* 20-Sep-2017 nm */

          }
        }
      }
      /* This one puts a space after a letter followed by a word token
         e.g. "A" and "suc" in "A. x e. U. A suc" in limuni2  1-Feb-05 */
      if (pos >= 1) {
        /* See if the next token is "suc" */
        if (!strcmp(mathToken[mathString[pos]].tokenName, "suc")) {
          /* Match a token starting with a letter for the current token */
          if (isalpha(
              (unsigned char)(mathToken[mathString[pos - 1]].tokenName[0]))) {
            /* and make sure its length is 1 */
            if (!(mathToken[mathString[pos - 1]].tokenName[1])) {
              let(&texLine, cat(texLine, " ", NULL)); /* Add a space */
            }
          }
        }
      }
      /* This one puts a space before any "-." that doesn't come after
         a parentheses e.g. ax-6 has both cases */
      if (pos >=1) {
        /* See if we have a non-parenthesis followed by not */
        if (strcmp(mathToken[mathString[pos - 1]].tokenName, "(")
            && !strcmp(mathToken[mathString[pos]].tokenName, "-.")) {
          let(&texLine, cat(texLine, " ", NULL)); /* Add a space */
        }
      }
      /* nm 9-Feb-04 This one puts a space between "S" and "(" in df-iso. */
      if (pos >=4) {
        if (!strcmp(mathToken[mathString[pos - 4]].tokenName, "Isom")
            && !strcmp(mathToken[mathString[pos - 2]].tokenName, ",")
            && !strcmp(mathToken[mathString[pos]].tokenName, "(")) {
          let(&texLine, cat(texLine, " ", NULL)); /* Add a space */
        }
      }
      /* nm 11-Aug-04 This one puts a space between "}" and "(" in
         funcnvuni proof. */
      if (pos >=1) {
        /* See if we have "}" followed by "(" */
        if (!strcmp(mathToken[mathString[pos - 1]].tokenName, "}")
            && !strcmp(mathToken[mathString[pos]].tokenName, "(")) {
          let(&texLine, cat(texLine, " ", NULL)); /* Add a space */
        }
      }
      /* 7-Mar-2016 nm This one puts a space between "}" and "{" in
         konigsberg proof. */
      if (pos >=1) {
        /* See if we have "}" followed by "(" */
        if (!strcmp(mathToken[mathString[pos - 1]].tokenName, "}")
            && !strcmp(mathToken[mathString[pos]].tokenName, "{")) {
          let(&texLine, cat(texLine, " ", NULL)); /* Add a space */
        }
      }
      /* 7/27/03 end */

      let(&texLine, cat(texLine, tex, NULL));
    } /* if !htmlFlag */
    let(&lastTex, tex); /* Save for next pass */
  } /* Next pos */

  /* 8/9/03 Discard redundant white space to reduce HTML file size */
  let(&texLine, edit(texLine, 8 + 16 + 128));

  /* 1-Feb-2016 nm */
  /* Enclose math symbols in a span to be used for font selection */
  let(&texLine, cat(
      (altHtmlFlag ? cat("<SPAN ", htmlFont, ">", NULL) : ""),
      texLine,
      (altHtmlFlag ? "</SPAN>" : ""), NULL));

  let(&tex, "");
  let(&lastTex, "");
  return texLine;
} /* getTexLongMath */


/* Added 18-Sep-03 (broken out of metamath.c for better modularization) */
/* Returns the TeX, or HTML code for GIFs (!altHtmlFlag) or Unicode
   (altHtmlFlag), for a statement's hypotheses and assertion in the form
   hyp & ... & hyp => assertion */
/* Warning: The caller must deallocate the returned vstring (i.e. this
   function cannot be used in let statements but must be assigned to
   a local vstring for local deallocation) */
vstring getTexOrHtmlHypAndAssertion(long statemNum)
{
  long reqHyps, essHyps, n;
  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated directly */
  vstring texOrHtmlCode = "";
  vstring str2 = "";
  /* Count the number of essential hypotheses essHyps */
  essHyps = 0;
  reqHyps = nmbrLen(statement[statemNum].reqHypList);
  let(&texOrHtmlCode, "");
  for (n = 0; n < reqHyps; n++) {
    if (statement[statement[statemNum].reqHypList[n]].type
        == (char)e_) {
      essHyps++;
      if (texOrHtmlCode[0]) { /* Add '&' between hypotheses */
        if (!htmlFlag) {
          /* Hard-coded for set.mm! */
          let(&texOrHtmlCode, cat(texOrHtmlCode,
                 "\\quad\\&\\quad "
              ,NULL));
        } else {
          if (altHtmlFlag) {
            /* Hard-coded for set.mm! */
            let(&texOrHtmlCode, cat(texOrHtmlCode,
                /* 8/8/03 - Changed from Symbol to Unicode */
/* "&nbsp;&nbsp;&nbsp;<FONT FACE=\"Symbol\"> &#38;</FONT>&nbsp;&nbsp;&nbsp;" */
                "<SPAN ", htmlFont, ">",  /* 1-Feb-2016 nm */
                "&nbsp;&nbsp;&nbsp; &amp;&nbsp;&nbsp;&nbsp;",
                "</SPAN>",   /* 1-Feb-2016 nm */
                NULL));
          } else {
            /* Hard-coded for set.mm! */
            let(&texOrHtmlCode, cat(texOrHtmlCode,
          "&nbsp;&nbsp;&nbsp;<IMG SRC='amp.gif' WIDTH=12 HEIGHT=19 ALT='&amp;'"
                ," ALIGN=TOP>&nbsp;&nbsp;&nbsp;"
                ,NULL));
          }
        }
      } /* if texOrHtmlCode[0] */
      /* Construct HTML hypothesis */
      nmbrTmpPtr = statement[statement[statemNum].reqHypList[n]].mathString;
      let(&str2, "");
      str2 = getTexLongMath(nmbrTmpPtr, statemNum);
      let(&texOrHtmlCode, cat(texOrHtmlCode, str2, NULL));
    }
  }
  if (essHyps) {  /* Add big arrow if there were hypotheses */
    if (!htmlFlag) {
      /* Hard-coded for set.mm! */
      let(&texOrHtmlCode, cat(texOrHtmlCode,
                 "\\quad\\Rightarrow\\quad "
          ,NULL));
    } else {
      if (altHtmlFlag) {
        /* Hard-coded for set.mm! */
        let(&texOrHtmlCode, cat(texOrHtmlCode,
            /* 8/8/03 - Changed from Symbol to Unicode */
/* "&nbsp;&nbsp;&nbsp;<FONT FACE=\"Symbol\"> &#222;</FONT>&nbsp;&nbsp;&nbsp;" */
 /* 29-Aug-2008 nm - added sans-serif to work around FF3 bug that produces
      huge character heights otherwise */
          /*  "&nbsp;&nbsp;&nbsp; &#8658;&nbsp;&nbsp;&nbsp;" */
      "&nbsp;&nbsp;&nbsp; <FONT FACE=sans-serif>&#8658;</FONT>&nbsp;&nbsp;&nbsp;"
            ,NULL));
      } else {
        /* Hard-coded for set.mm! */
        let(&texOrHtmlCode, cat(texOrHtmlCode,
          "&nbsp;&nbsp;&nbsp;<IMG SRC='bigto.gif' WIDTH=15 HEIGHT=19 ALT='=&gt;'"
            ," ALIGN=TOP>&nbsp;&nbsp;&nbsp;"
            ,NULL));
      }
    }
  }
  /* Construct TeX or HTML assertion */
  nmbrTmpPtr = statement[statemNum].mathString;
  let(&str2, "");
  str2 = getTexLongMath(nmbrTmpPtr, statemNum);
  let(&texOrHtmlCode, cat(texOrHtmlCode, str2, NULL));

  /* Deallocate memory */
  let(&str2, "");
  return texOrHtmlCode;
}  /* getTexOrHtmlHypAndAssertion */




/* Added 17-Nov-2015 (broken out of metamath.c for better modularization) */
/* Called by the WRITE BIBLIOGRPAHY command and also by VERIFY MARKUP
   for error checking */
/* Returns 0 if OK, 1 if warning(s), 2 if any error */
flag writeBibliography(vstring bibFile,
    vstring labelMatch, /* Normally "*" except when called by verifyMarkup() */
    flag errorsOnly,  /* 1 = no output, just warning msgs if any */
    flag noFileCheck) /* 1 = ignore missing external files (mmbiblio.html) */
{
  flag errFlag;
  FILE *list1_fp = NULL;
  FILE *list2_fp = NULL;
  long lines, p2, i, j, jend, k, l, m, n, p, q, s, pass1refs;
  vstring str1 = "", str2 = "", str3 = "", str4 = "", newstr = "", oldstr = "";
  pntrString *pntrTmp = NULL_PNTRSTRING;
  flag warnFlag;

  n = 0; /* 14-Jan-2016 nm Old gcc 4.6.3 wrongly says may be uninit ln 5506 */
  pass1refs = 0; /* 25-Jan-2016 gcc 4.5.3 wrongly says may be uninit */
  if (noFileCheck == 1 && errorsOnly == 0) {
    bug(2336); /* If we aren't opening files, a non-error run can't work */
  }
  /* 10/10/02 */
  /* This utility builds the bibliographical cross-references to various
     textbooks and updates the user-specified file normally called
     mmbiblio.html. */
  warnFlag = 0; /* 1 means warning was found, 2 that error was found */
  errFlag = 0; /* Error flag to recover input file and to set return value 2 */
  if (noFileCheck == 0) {
    list1_fp = fSafeOpen(bibFile, "r", 0/*noVersioningFlag*/);
    if (list1_fp == NULL) {
      /* Couldn't open it (error msg was provided)*/
      return 1;
    }
    if (errorsOnly == 0) {
      fclose(list1_fp);
      /* This will rename the input mmbiblio.html as mmbiblio.html~1 */
      list2_fp = fSafeOpen(bibFile, "w", 0/*noVersioningFlag*/);
      if (list2_fp == NULL) {
          /* Couldn't open it (error msg was provided)*/
        return 1;
      }
      /* Note: in older versions the "~1" string was OS-dependent, but we
         don't support VAX or THINK C anymore...  Anyway we reopen it
         here with the renamed file in case the OS won't let us rename
         an opened file during the fSafeOpen for write above. */
      list1_fp = fSafeOpen(cat(bibFile, "~1", NULL), "r", 0/*noVersioningFlag*/);
      if (list1_fp == NULL) bug(2337);
    }
  }
  if (!texDefsRead) {
    htmlFlag = 1;
    /* Now done in readTexDefs() *
    if (errorsOnly == 0 ) {
      print2("Reading definitions from $t statement of %s...\n", input_fn);
    }
    */
    if (2/*error*/ == readTexDefs(errorsOnly, noFileCheck)) {
      errFlag = 2; /* Error flag to recover input file */
      goto BIB_ERROR; /* An error occurred */
    }
  }

  /* Transfer the input file up to the special "<!-- #START# -->" comment */
  if (noFileCheck == 0) {
    while (1) {
      if (!linput(list1_fp, NULL, &str1)) {
        print2(
  "?Error: Could not find \"<!-- #START# -->\" line in input file \"%s\".\n",
            bibFile);
        errFlag = 2; /* Error flag to recover input file */
        break;
      }
      if (errorsOnly == 0) {
        fprintf(list2_fp, "%s\n", str1);
      }
      if (!strcmp(str1, "<!-- #START# -->")) break;
    }
    if (errFlag) goto BIB_ERROR;
  }

  p2 = 1; /* Pass 1 or 2 flag */
  lines = 0;
  while (1) {

    if (p2 == 2) {  /* Pass 2 */
      /* Allocate memory for sorting */
      pntrLet(&pntrTmp, pntrSpace(lines));
      lines = 0;
    }

    /* Scan all $a and $p statements */
    for (i = 1; i <= statements; i++) {
      if (statement[i].type != (char)p_ &&
        statement[i].type != (char)a_) continue;

      /* 13-Dec-2016 nm */
      /* Normally labelMatch is *, but may be more specific for
         use by verifyMarkup() */
      if (!matchesList(statement[i].labelName, labelMatch, '*', '?')) {
        continue;
      }

      /* Omit ...OLD (obsolete) and ...NEW (to be implemented) statements */
      if (instr(1, statement[i].labelName, "NEW")) continue;
      if (instr(1, statement[i].labelName, "OLD")) continue;
      let(&str1, "");
      str1 = getDescription(i); /* Get the statement's comment */
      if (!instr(1, str1, "[")) continue;
      l = (signed)(strlen(str1));
      for (j = 0; j < l; j++) {
        if (str1[j] == '\n') str1[j] = ' '; /* Change newlines to spaces */
        if (str1[j] == '\r') bug(2338);
      }
      let(&str1, edit(str1, 8 + 128 + 16)); /* Reduce & trim whitespace */

      /* 15-Apr-2015 nm */
      /* Clear out math symbols in backquotes to prevent false matches
         to [reference] bracket */
      k = 0; /* Math symbol mode if 1 */
      l = (signed)(strlen(str1));
      for (j = 0; j < l - 1; j++) {
        if (k == 0) {
          if (str1[j] == '`') {
            k = 1; /* Start of math mode */
          }
        } else { /* In math mode */
          if (str1[j] == '`') { /* A backquote */
            if (str1[j + 1] == '`') {
              /* It is an escaped backquote */
              str1[j] = ' ';
              str1[j + 1] = ' ';
            } else {
              k = 0; /* End of math mode */
            }
          } else { /* Not a backquote */
            str1[j] = ' '; /* Clear out the math mode part */
          }
        } /* end if k == 0 */
      } /* next j */

      /* Put spaces before page #s (up to 4 digits) for sorting */
      j = 0;
      while (1) {
        j = instr(j + 1, str1, " p. "); /* Heuristic - match " p. " */
        if (!j) break;
        if (j) {
          for (k = j + 4; k <= (signed)(strlen(str1)) + 1; k++) {
            if (!isdigit((unsigned char)(str1[k - 1]))) {
              let(&str1, cat(left(str1, j + 2),
                  space(4 - (k - (j + 4))), right(str1, j + 3), NULL));
              /* Add ### after page number as marker */
              let(&str1, cat(left(str1, j + 7), "###", right(str1, j + 8),
                  NULL));
              break;
            }
          }
        }
      }
      /* Process any bibliographic references in comment */
      j = 0;
      n = 0;
      while (1) {
        j = instr(j + 1, str1, "["); /* Find reference (not robust) */
        if (!j) break;

        /* 13-Dec-2016 nm Fix mmbiblio.html corruption caused by
           "[Copying an angle]" in df-ibcg in
           set.mm.2016-09-18-JB-mbox-cleanup */
        /* Skip if there is no trailing "]" */
        jend = instr(j, str1, "]");
        if (!jend) break;
        /* Skip bracketed text with spaces */
        /* This is a somewhat ugly workaround that lets us tolerate user
           comments in [...] is there is at least one space.
           The printTexComment() above handles this case with the
           "strcspn(bibTag, " \n\r\t\f")" above; here we just have to look
           for space since we already reduced \n \t to space (\f is probably
           overkill, and any \r's are removed during the READ command) */
        if (instr(1, seg(str1, j, jend), " ")) continue;
        /* 22-Feb-2019 nm */
        /* Skip escaped bracket "[[" */
        if (str1[j] == '[') {  /* (j+1)th character in str1 */
          j++; /* Skip over 2nd "[" */
          continue;
        }
        if (!isalnum((unsigned char)(str1[j]))) continue; /* Not start of reference */
        n++;
        /* Backtrack from [reference] to a starting keyword */
        m = 0;
        let(&str2, edit(str1, 32)); /* to uppercase */

        /* (The string search below is rather inefficient; maybe improve
           the algorithm if speed becomes a problem.) */
        for (k = j - 1; k >= 1; k--) {
          /* **IMPORTANT** Make sure to update mmhlpb.c HELP WRITE BIBLIOGRAPHY
             if new items are added to this list. */
          if (0
              /* Do not add SCHEMA but use AXIOM SCHEMA or THEOREM SCHEMA */
              /* Put the most frequent ones first to speed up search */
              || !strcmp(mid(str2, k, (long)strlen("THEOREM")), "THEOREM")
              || !strcmp(mid(str2, k, (long)strlen("EQUATION")), "EQUATION")
              || !strcmp(mid(str2, k, (long)strlen("DEFINITION")), "DEFINITION")
              || !strcmp(mid(str2, k, (long)strlen("LEMMA")), "LEMMA")
              || !strcmp(mid(str2, k, (long)strlen("EXERCISE")), "EXERCISE")
              || !strcmp(mid(str2, k, (long)strlen("AXIOM")), "AXIOM")

              || !strcmp(mid(str2, k, (long)strlen("CHAPTER")), "CHAPTER")
              || !strcmp(mid(str2, k, (long)strlen("COMPARE")), "COMPARE")
              || !strcmp(mid(str2, k, (long)strlen("CONDITION")), "CONDITION")
              || !strcmp(mid(str2, k, (long)strlen("COROLLARY")), "COROLLARY")
              || !strcmp(mid(str2, k, (long)strlen("EXAMPLE")), "EXAMPLE")
              || !strcmp(mid(str2, k, (long)strlen("FIGURE")), "FIGURE")
              || !strcmp(mid(str2, k, (long)strlen("ITEM")), "ITEM")
              || !strcmp(mid(str2, k, (long)strlen("LEMMAS")), "LEMMAS")
              || !strcmp(mid(str2, k, (long)strlen("LINE")), "LINE")
              || !strcmp(mid(str2, k, (long)strlen("LINES")), "LINES")
              || !strcmp(mid(str2, k, (long)strlen("NOTATION")), "NOTATION")
              || !strcmp(mid(str2, k, (long)strlen("NOTE")), "NOTE")
              || !strcmp(mid(str2, k, (long)strlen("OBSERVATION")), "OBSERVATION")
              || !strcmp(mid(str2, k, (long)strlen("PART")), "PART")
              || !strcmp(mid(str2, k, (long)strlen("POSTULATE")), "POSTULATE")
              || !strcmp(mid(str2, k, (long)strlen("PROBLEM")), "PROBLEM")
              || !strcmp(mid(str2, k, (long)strlen("PROPERTY")), "PROPERTY")
              || !strcmp(mid(str2, k, (long)strlen("PROPOSITION")), "PROPOSITION")
              || !strcmp(mid(str2, k, (long)strlen("REMARK")), "REMARK")
              || !strcmp(mid(str2, k, (long)strlen("RULE")), "RULE")
              || !strcmp(mid(str2, k, (long)strlen("SCHEME")), "SCHEME")
              || !strcmp(mid(str2, k, (long)strlen("SECTION")), "SECTION")
              /* Added 3-Jun-2018 nm */
              || !strcmp(mid(str2, k, (long)strlen("PROOF")), "PROOF")
              || !strcmp(mid(str2, k, (long)strlen("STATEMENT")), "STATEMENT")
              ) {
            m = k;
            break;
          }
          let(&str3, ""); /* Clear tmp alloc stack created by "mid" */
        }
        if (!m) {
          if (p2 == 1) {
            print2(
             "?Warning: Bibliography keyword missing in comment for \"%s\".\n",
                statement[i].labelName);
            print2(
                "    (See HELP WRITE BIBLIOGRAPHY for list of keywords.)\n");
            warnFlag = 1;
          }
          continue; /* Not a bib ref - ignore */
        }
        /* m is at the start of a keyword */
        p = instr(m, str1, "["); /* Start of bibliograpy reference */
        q = instr(p, str1, "]"); /* End of bibliography reference */
        if (q == 0) {
          if (p2 == 1) {
            print2(
        "?Warning: Bibliography reference not found in HTML file in \"%s\".\n",
              statement[i].labelName);
            warnFlag = 1;
          }
          continue; /* Pretend it is not a bib ref - ignore */
        }
        s = instr(q, str1, "###"); /* Page number marker */
        if (!s) {
          if (p2 == 1) {
            print2(
          "?Warning: No page number after [<author>] bib ref in \"%s\".\n",
              statement[i].labelName);
            warnFlag = 1;
          }
          continue; /* No page number given - ignore */
        }
        /* Now we have a real reference; increment reference count */
        lines++;
        if (p2 == 1) continue; /* In 1st pass, we just count refs */

        let(&str2, seg(str1, m, p - 1));     /* "Theorem #" */
        let(&str3, seg(str1, p + 1, q - 1));  /* "[bibref]" w/out [] */
        let(&str4, seg(str1, q + 1, s - 1)); /* " p. nnnn" */
        str2[0] = (char)(toupper((unsigned char)(str2[0])));
        /* Eliminate noise like "of" in "Theorem 1 of [bibref]" */
        for (k = (long)strlen(str2); k >=1; k--) {
          if (0
              || !strcmp(mid(str2, k, (long)strlen(" of ")), " of ")
              || !strcmp(mid(str2, k, (long)strlen(" in ")), " in ")
              || !strcmp(mid(str2, k, (long)strlen(" from ")), " from ")
              || !strcmp(mid(str2, k, (long)strlen(" on ")), " on ")
              ) {
            let(&str2, left(str2, k - 1));
            break;
          }
          let(&str2, str2);
        }

        let(&newstr, "");
        newstr = pinkHTML(i); /* Get little pink number */
        let(&oldstr, cat(
            /* Construct the sorting key */
            /* The space() helps Th. 9 sort before Th. 10 on same page */
            str3, " ", str4, space(20 - (long)strlen(str2)), str2,
            "|||",  /* ||| means end of sort key */
            /* Construct just the statement href for combining dup refs */
            "<A HREF=\"", statement[i].labelName,
            ".html\">", statement[i].labelName, "</A>",
            newstr,
            "&&&",  /* &&& means end of statement href */
            /* Construct actual HTML table row (without ending tag
               so duplicate references can be added) */

            /*
            (i < extHtmlStmt) ?
               "<TR>" :
               cat("<TR BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL),
            */

            /* 29-Jul-2008 nm Sandbox stuff */
            (i < extHtmlStmt)
               ? "<TR>"
               : (i < sandboxStmt)
                   ? cat("<TR BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL)
                   : cat("<TR BGCOLOR=", SANDBOX_COLOR, ">", NULL),

            "<TD NOWRAP>[<A HREF=\"",

            /*
            (i < extHtmlStmt) ?
               htmlBibliography :
               extHtmlBibliography,
            */

            /* 29-Jul-2008 nm Sandbox stuff */
            (i < extHtmlStmt)
               ? htmlBibliography
               : (i < sandboxStmt)
                   ? extHtmlBibliography
                   /* Note that the sandbox uses the mmset.html
                      bibliography */
                   : htmlBibliography,

            "#",
            str3,
            "\">", str3, "</A>]", str4,
            "</TD><TD>", str2, "</TD><TD><A HREF=\"",
            statement[i].labelName,
            ".html\">", statement[i].labelName, "</A>",
            newstr, NULL));
        /* Put construction into string array for sorting */
        let((vstring *)(&pntrTmp[lines - 1]), oldstr);
      } /* while(1) */
    } /* next i */

    /* 'lines' should be the same in both passes */
    if (p2 == 1) {
      pass1refs = lines;
    } else {
      if (pass1refs != lines) bug(2339);
    }

    if (errorsOnly == 0 && p2 == 2) {
      /*
      print2("Pass %ld finished.  %ld references were processed.\n", p2, lines);
      */
      print2("%ld references were processed.\n", lines);
    }
    if (p2 == 2) break;
    p2++;    /* Increment from pass 1 to pass 2 */
  } /* while(1) */

  /* Sort */
  qsortKey = "";
  qsort(pntrTmp, (size_t)lines, sizeof(void *), qsortStringCmp);

  /* Combine duplicate references */
  let(&str1, "");  /* Last biblio ref */
  for (i = 0; i < lines; i++) {
    j = instr(1, (vstring)(pntrTmp[i]), "|||");
    let(&str2, left((vstring)(pntrTmp[i]), j - 1));
    if (!strcmp(str1, str2)) {
      /* n++; */ /* 17-Nov-2015 nm Deleted - why was this here? */
      /* Combine last with this */
      k = instr(j, (vstring)(pntrTmp[i]), "&&&");
      /* Extract statement href */
      let(&str3, seg((vstring)(pntrTmp[i]), j + 3, k -1));
      let((vstring *)(&pntrTmp[i]),
          cat((vstring)(pntrTmp[i - 1]), " &nbsp;", str3, NULL));
      let((vstring *)(&pntrTmp[i - 1]), ""); /* Clear previous line */
    }
    let(&str1, str2);
  }

  /* Write output */
  if (noFileCheck == 0 && errorsOnly == 0) {
    n = 0; /* Table rows written */
    for (i = 0; i < lines; i++) {
      j = instr(1, (vstring)(pntrTmp[i]), "&&&");
      if (j) {  /* Don't print blanked out combined lines */
        n++;
        /* Take off prefixes and reduce spaces */
        let(&str1, edit(right((vstring)(pntrTmp[i]), j + 3), 16));
        j = 1;
        /* Break up long lines for text editors */
        let(&printString, "");
        outputToString = 1;
        printLongLine(cat(str1, "</TD></TR>", NULL),
            " ",  /* Start continuation line with space */
            "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
        outputToString = 0;
        fprintf(list2_fp, "%s", printString);
        let(&printString, "");
      }
    }
  }


  /* Discard the input file up to the special "<!-- #END# -->" comment */
  if (noFileCheck == 0) {
    while (1) {
      if (!linput(list1_fp, NULL, &str1)) {
        print2(
  "?Error: Could not find \"<!-- #END# -->\" line in input file \"%s\".\n",
            bibFile);
        errFlag = 2; /* Error flag to recover input file */
        break;
      }
      if (!strcmp(str1, "<!-- #END# -->")) {
        if (errorsOnly == 0) {
          fprintf(list2_fp, "%s\n", str1);
        }
        break;
      }
    }
    if (errFlag) goto BIB_ERROR;
  }

  if (noFileCheck == 0 && errorsOnly == 0) {
    /* Transfer the rest of the input file */
    while (1) {
      if (!linput(list1_fp, NULL, &str1)) {
        break;
      }

      /* Update the date stamp at the bottom of the HTML page. */
      /* This is just a nicety; no error check is done. */
      if (!strcmp("This page was last updated on ", left(str1, 30))) {
        let(&str1, cat(left(str1, 30), date(), ".", NULL));
      }

      fprintf(list2_fp, "%s\n", str1);
    }

    print2("%ld table rows were written.\n", n);
    /* Deallocate string array */
    for (i = 0; i < lines; i++) let((vstring *)(&pntrTmp[i]), "");
    pntrLet(&pntrTmp,NULL_PNTRSTRING);
  }


 BIB_ERROR:
  if (noFileCheck == 0) {
    fclose(list1_fp);
    if (errorsOnly == 0) {
      fclose(list2_fp);
    }
    if (errorsOnly == 0) {
      if (errFlag) {
        /* Recover input files in case of error */
        remove(bibFile);  /* Delete output file */
        rename(cat(bibFile, "~1", NULL), fullArg[2]);
            /* Restore input file name */
        print2("?The file \"%s\" was not modified.\n", fullArg[2]);
      }
    }
  }
  if (errFlag == 2) warnFlag = 2;
  return warnFlag;
}  /* writeBibliography */


