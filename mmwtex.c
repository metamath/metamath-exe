/*****************************************************************************/
/*        Copyright (C) 2003  NORMAN MEGILL  nm at alum.mit.edu              */
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
#include <setjmp.h>
#include "mmutil.h"
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h" /* For rawSourceError and mathSrchCmp */
#include "mmwtex.h"
#include "mmcmdl.h" /* For texFileName */
#include "mmcmds.h" /* For pinkNumber */

/* 6/27/99 - Now, all LaTeX and HTML definitions are taken from the source
   file (read in the by READ... command).  In the source file, there should
   be a single comment $( ... $) containing the keyword $t.  The definitions
   start after the $t and end at the $).  Between $t and $), the definition
   source should exist.  See the file set.mm for an example. */

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



/* Tex output file */
FILE *texFilePtr = NULL;
flag texFileOpenFlag = 0;

/* Tex dictionary */
FILE *tex_dict_fp;
vstring tex_dict_fn = "";

/* Global variables */
flag texDefsRead = 0;

/* Variables local to this module (except some $t variables) */
struct texDef_struct {
  vstring tokenName; /* ASCII token */
  vstring texEquiv; /* Converted to TeX */
};
struct texDef_struct *texDefs;
long numSymbs;
#define DOLLAR_SUBST 2
/*char dollarSubst = 2;*/
    /* Substitute character for $ in converting to obsolete $l,$m,$n
       comments - Use '$' instead of non-printable ASCII 2 for debugging */

/* Variables set by the language in the set.mm etc. $t statement */
/* Some of these are global; see mmwtex.h */
vstring htmlVarColors = ""; /* Set by htmlvarcolor commands */
vstring htmlTitle = ""; /* Set by htmltitle command */
vstring htmlHome = ""; /* Set by htmlhome command */
vstring htmlBibliography = ""; /* Optional; set by htmlbibliography command */
vstring extHtmlLabel = ""; /* Set by exthtmllabel command - where extHtml starts */
vstring extHtmlTitle = ""; /* Set by exthtmltitle command (global!) */
vstring extHtmlHome = ""; /* Set by exthtmlhome command */
vstring extHtmlBibliography = ""; /* Optional; set by exthtmlbibliography command */
vstring htmlDir = ""; /* Directory for GIF version, set by htmldir command */
vstring altHtmlDir = ""; /* Directory for Unicode Font version, set by
                            althtmldir command */

/* Variables holding all HTML <a name..> tags from bibiography pages  */
vstring htmlBibliographyTags = "";
vstring extHtmlBibliographyTags = "";

flag readTexDefs(void)
{

  char *fileBuf;
  char *startPtr;
  long lineNumOffset = 0;
  char *fbPtr;
  char *tmpPtr;
  char *tmpPtr2;
  long charCount;
  long i, j;
  long lineNum;
  long tokenLen;
  char zapChar;
  long cmd;
  long parsePass;
  vstring token = "";

  /* bsearch returned values for use in error-checking */
  void *mathKeyPtr; /* bsearch returned value for math symbol lookup */
  void *texDefsPtr; /* For binary search */

  /* Initial values below will be overridden if a user assignment exists in the
     $t comment of the xxx.mm input file */
  let(&htmlTitle, "Metamath Test Page"); /* Set by htmltitle command in $t
                                                 comment */
  let(&htmlHome, "<A HREF=\"http://metamath.org\">Home</A>");
                                     /* Set by htmlhome command in $t comment */

  if (texDefsRead) bug(2301); /* Shouldn't parse the $t comment twice */

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
        return 0;
      }
      let(&fileBuf, tmpPtr);
      tmpPtr[j] = zapChar;
      /* break; */ /* Continue to end to detect double $t */
    }
    tmpPtr[j] = zapChar; /* Restore the xxx.mm input file buffer */
  }
  /* If the $t wasn't found, fileBuf will be "", causing error message below. */
  /* Compute line number offset of beginning of statement[i].labelSection for
     use in error messages */
  j = strlen(fileBuf);
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
    return 0;
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
    return 0;
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
        return 0;
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
      tokenLen = texDefTokenLen(fbPtr);

      /* Process token - command */
      if (!tokenLen) break; /* End of file */
      zapChar = fbPtr[tokenLen]; /* Char to restore after zapping source */
      fbPtr[tokenLen] = 0; /* Create end of string */
      cmd = lookup(fbPtr,
          "latexdef,htmldef,htmlvarcolor,htmltitle,htmlhome"
        ",althtmldef,exthtmltitle,exthtmlhome,exthtmllabel,htmldir,althtmldir"
        ",htmlbibliography,exthtmlbibliography");
      fbPtr[tokenLen] = zapChar;
      if (cmd == 0) {
        lineNum = lineNumOffset;
        for (i = 0; i < (fbPtr - fileBuf); i++) {
          if (fileBuf[i] == '\n') lineNum++;
        }
        rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, input_fn,
            cat("Expected \"latexdef\", \"htmldef\", \"htmlvarcolor\",",
            " \"htmltitle\", \"htmlhome\", \"althtmldef\",",
            " \"exthtmltitle\", \"exthtmlhome\", \"exthtmllabel\",",
            " \"htmldir\", \"althtmldir\",",
            " \"htmlbibliography\", or \"exthtmlbibliography\" here.",
            NULL));
        let(&fileBuf, "");  /* was: free(fileBuf); */
        return (0);
      }
      fbPtr = fbPtr + tokenLen;

      if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME
          && cmd != EXTHTMLTITLE && cmd != EXTHTMLHOME && cmd != EXTHTMLLABEL
          && cmd != HTMLDIR && cmd != ALTHTMLDIR
          && cmd != HTMLBIBLIOGRAPHY && cmd != EXTHTMLBIBLIOGRAPHY) {
         /* Get next token - string in quotes */
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLen = texDefTokenLen(fbPtr);

        /* Process token - string in quotes */
        if (fbPtr[0] != '\"' && fbPtr[0] != '\'') {
          if (!tokenLen) { /* Abnormal end-of-file */
            fbPtr--; /* Format for error message */
            tokenLen++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, input_fn,
              "Expected a quoted string here.");
          let(&fileBuf, "");  /* was: free(fileBuf); */
          return (0);
        }
        if (parsePass == 2) {
          zapChar = fbPtr[tokenLen - 1]; /* Chr to restore after zapping src */
          fbPtr[tokenLen - 1] = 0; /* Create end of string */
          let(&token, fbPtr + 1); /* Get ASCII token; note that leading and
              trailing quotes are omitted. */
          fbPtr[tokenLen - 1] = zapChar;

          /* Change double internal quotes to single quotes */
          j = strlen(token);
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

          if ((cmd == LATEXDEF && !htmlFlag)
              || (cmd == HTMLDEF && htmlFlag && !altHtmlFlag)
              || (cmd == ALTHTMLDEF && htmlFlag && altHtmlFlag)) {
            texDefs[numSymbs].tokenName = "";
            let(&(texDefs[numSymbs].tokenName), token);
          }
        }

        fbPtr = fbPtr + tokenLen;
      } /* if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME...) */

      if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME
          && cmd != EXTHTMLTITLE && cmd != EXTHTMLHOME && cmd != EXTHTMLLABEL
          && cmd != HTMLDIR && cmd != ALTHTMLDIR
          && cmd != HTMLBIBLIOGRAPHY && cmd != EXTHTMLBIBLIOGRAPHY) {
        /* Get next token -- "as" */
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLen = texDefTokenLen(fbPtr);
        zapChar = fbPtr[tokenLen]; /* Char to restore after zapping source */
        fbPtr[tokenLen] = 0; /* Create end of string */
        if (strcmp(fbPtr, "as")) {
          if (!tokenLen) { /* Abnormal end-of-file */
            fbPtr--; /* Format for error message */
            tokenLen++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, input_fn,
              "Expected the keyword \"as\" here.");
          let(&fileBuf, "");  /* was: free(fileBuf); */
          return (0);
        }
        fbPtr[tokenLen] = zapChar;
        fbPtr = fbPtr + tokenLen;
      }

      if (parsePass == 2) {
        /* Initialize LaTeX/HTML equivalent */
        let(&token, "");
      }

      while (1) {

        /* Get next token - string in quotes */
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLen = texDefTokenLen(fbPtr);
        if (fbPtr[0] != '\"' && fbPtr[0] != '\'') {
          if (!tokenLen) { /* Abnormal end-of-file */
            fbPtr--; /* Format for error message */
            tokenLen++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, input_fn,
              "Expected a quoted string here.");
          let(&fileBuf, "");  /* was: free(fileBuf); */
         return (0);
        }
        if (parsePass == 2) {
          zapChar = fbPtr[tokenLen - 1]; /* Chr to restore after zapping src */
          fbPtr[tokenLen - 1] = 0; /* Create end of string */
          let(&token, cat(token, fbPtr + 1, NULL)); /* Append TeX equiv.; note
              leading and trailing quotes are omitted. */
          fbPtr[tokenLen - 1] = zapChar;
        }
        fbPtr = fbPtr + tokenLen;


        /* Get next token - "+" or ";" */
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLen = texDefTokenLen(fbPtr);
        if ((fbPtr[0] != '+' && fbPtr[0] != ';') || tokenLen != 1) {
          if (!tokenLen) { /* Abnormal end-of-file */
            fbPtr--; /* Format for error message */
            tokenLen++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') {
              lineNum++;
            }
          }
          rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, input_fn,
              "Expected \"+\" or \";\" here.");
          let(&fileBuf, "");  /* was: free(fileBuf); */
         return (0);
        }
        fbPtr = fbPtr + tokenLen;

        if (fbPtr[-1] == ';') break;

      } /* End while */


      if (parsePass == 2) {
        /* Change double internal quotes to single quotes */
        j = strlen(token);
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

        if ((cmd == LATEXDEF && !htmlFlag)
            || (cmd == HTMLDEF && htmlFlag && !altHtmlFlag)
            || (cmd == ALTHTMLDEF && htmlFlag && altHtmlFlag)) {
          texDefs[numSymbs].texEquiv = "";
          let(&(texDefs[numSymbs].texEquiv), token);
        }
        if (cmd == HTMLVARCOLOR) {
          let(&htmlVarColors, cat(htmlVarColors, " ", token, NULL));
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
      }

      if ((cmd == LATEXDEF && !htmlFlag)
          || (cmd == HTMLDEF && htmlFlag && !altHtmlFlag)
          || (cmd == ALTHTMLDEF && htmlFlag && altHtmlFlag)) {
        numSymbs++;
      }

    } /* End while */

    if (fbPtr != fileBuf + charCount) bug(2305);

    if (parsePass == 1 ) {
      print2("%ld typesetting statements were read from \"%s\".\n",
          numSymbs, input_fn);
      texDefs = malloc(numSymbs * sizeof(struct texDef_struct));
      if (!texDefs) outOfMemory("#99 (TeX symbols)");
    }

  } /* next parsePass */


  /* Sort the tokens for later lookup */
  qsort(texDefs, numSymbs, sizeof(struct texDef_struct), texSortCmp);

  /* Check for duplicate definitions */
  for (i = 1; i < numSymbs; i++) {
    if (!strcmp(texDefs[i].tokenName, texDefs[i - 1].tokenName)) {
      printLongLine(cat("?Error:  Token ", texDefs[i].tokenName,
          " is defined more than once in ",
          htmlFlag ? "an htmldef" : "a latexdef", " statement.", NULL),
          "", " ");
    }
  }

  /* Check to make sure all definitions are for a real math token */
  for (i = 0; i < numSymbs; i++) {
    /* Note:  mathKey, mathTokens, and mathSrchCmp are assigned or defined
       in mmpars.c. */
    mathKeyPtr = (void *)bsearch(texDefs[i].tokenName, mathKey, mathTokens,
        sizeof(long), mathSrchCmp);
    if (!mathKeyPtr) {
      printLongLine(cat("?Error:  The token \"", texDefs[i].tokenName,
          "\", which was defined in ", htmlFlag ? "an htmldef" : "a latexdef",
          " statement, was not declared in any $v or $c statement.", NULL),
          "", " ");
    }
  }

  /* Check to make sure all math tokens have typesetting definitions */
  for (i = 0; i < mathTokens; i++) {
    texDefsPtr = (void *)bsearch(mathToken[i].tokenName, texDefs, numSymbs,
        sizeof(struct texDef_struct), texSrchCmp);
    if (!texDefsPtr) {
      printLongLine(cat("?Error:  The token \"", mathToken[i].tokenName,
       "\", which was defined in a $v or $c statement, was not declared in ",
          htmlFlag ? "an htmldef" : "a latexdef", " statement.", NULL),
          "", " ");
    }
  }

  /* Look up the extended database start label */
  if (extHtmlLabel[0]) {
    for (i = 1; i <= statements; i++) {
      if (!strcmp(extHtmlLabel,statement[i].labelName)) break;
    }
    if (i > statements) {
      printLongLine(cat("?Error: There is no statement with label \"",
          extHtmlLabel,
          "\" (specified by exthtmllabel in the database source $t comment).  ",
          "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
    }
    extHtmlStmt = i;
  } else {
    /* There is no extended database; set threshold to beyond end of db */
    extHtmlStmt = statements + 1;
  }

  let(&token, ""); /* Deallocate */
  let(&fileBuf, "");  /* was: free(fileBuf); */
  texDefsRead = 1;  /* Set global flag that it's been read in */
  return (1); /* Return indicator that parsing passed */

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
    if (isalnum(tmpchr)) return (i); /* Alphanumeric string */
    if (ptr[i] == '!') { /* Comment to end-of-line */
      ptr1 = strchr(ptr + i + 1, '\n');
      if (!ptr1) bug(2306);
      i = ptr1 - ptr + 1;
      continue;
    }
    if (tmpchr == '/') { /* Embedded c-style comment - used to ignore
        comments inside of Metamath comment for LaTeX/HTML definitions */
      if (ptr[i + 1] == '*') {
        while (1) {
          ptr1 = strchr(ptr + i + 2, '*');
          if (!ptr1) {
            return(i + strlen(&ptr[i])); /* Unterminated comment - goto EOF */
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
        return(i + strlen(&ptr[i])); /* Unterminated quote - goto EOF */
      }
      if (ptr1[1] != '\"') return(ptr1 - ptr + 1); /* Double quote is literal */
      i = ptr1 - ptr + 1;
    }
  }
  if (tmpchr == '\'') {
    while (1) {
      ptr1 = strchr(ptr + i + 1, '\'');
      if (!ptr1) {
        return(i + strlen(&ptr[i])); /* Unterminated quote - goto EOF */
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
  j = strlen(ttstr);

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
        case '&':
          let(&ttstr,cat(left(ttstr,i),"&amp;",right(ttstr,i+2),NULL));
          k = 5;
          break;
        case '<':
          /* 11/15/02 Leave in some special HTML tags (case must match) */
          /* This was done specially for the set.mm inf3 comment */
          if (!strcmp(mid(ttstr, i + 1, 5), "<PRE>")) {
            let(&ttstr, ttstr); /* Purge stack to prevent overflow by 'mid' */
            i = i + 5;
            break;
          }
          if (!strcmp(mid(ttstr, i + 1, 6), "</PRE>")) {
            let(&ttstr, ttstr); /* Purge stack to prevent overflow by 'mid' */
            i = i + 6;
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
vstring tokenToTex(vstring mtoken)
{
  vstring tex = "";
  vstring tmpStr;
  long i, j, k;
  void *texDefsPtr; /* For binary search */
  flag saveOutputToString;

  if (!texDefsRead) {
    bug(2320); /* This shouldn't be called if definitions weren't read */
  }

  texDefsPtr = (void *)bsearch(mtoken, texDefs, numSymbs,
      sizeof(struct texDef_struct), texSrchCmp);
  if (texDefsPtr) { /* Found it */
    let(&tex, ((struct texDef_struct *)texDefsPtr)->texEquiv);
  } else {
    /* 9/5/99 If it wasn't found, give user a warning... */
    saveOutputToString = outputToString;
    outputToString = 0;
    printLongLine(cat("?Error: Math symbol token \"", mtoken,
        "\" does not have a LaTeX and/or an HTML definition.", NULL),
        "", " ");
    outputToString = saveOutputToString;
    /* ... but we'll still leave in the old default conversion anyway: */

    /* If it wasn't found, use built-in conversion rules */
    let(&tex, mtoken);

    /* First, see if it's a tilde followed by a letter */
    /* If so, remove the tilde.  (This is actually obsolete.) */
    /* (The tilde was an escape in the obsolete syntax.) */
    if (tex[0] == '~') {
      if (isalpha(tex[1])) {
        let(&tex, right(tex, 2)); /* Remove tilde */
      }
    }

    /* Next, convert punctuation characters to tt font */
    j = strlen(tex);
    for (i = 0; i < j; i++) {
      if (ispunct(tex[i])) {
        tmpStr = asciiToTt(chr(tex[i]));
        if (!htmlFlag)
          let(&tmpStr, cat("{\\tt ", tmpStr, "}", NULL));
        k = strlen(tmpStr);
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
vstring asciiMathToTex(vstring mathComment)
{

  vstring tex;
  vstring texLine = "";
  vstring lastTex = "";
  vstring token = "";
  flag alphnew, alphold, unknownnew, unknownold;
  flag firstToken;
  long i;
  vstring srcptr;

  srcptr = mathComment;

  let(&texLine, "");
  let(&lastTex, "");
  firstToken = 1;
  while(1) {
    i = whiteSpaceLen(srcptr);
    srcptr = srcptr + i;
    i = tokenLen(srcptr);
    if (!i) break; /* Done */
    let(&token, space(i));
    memcpy(token, srcptr, i);
    srcptr = srcptr + i;
    tex = tokenToTex(token); /* Convert token to TeX */
              /* tokenToTex allocates tex; we must deallocate it */

    if (!htmlFlag) {
      /* If this token and previous token begin with letter, add a thin
           space between them */
      /* Also, anything not in table will have space added */
      alphnew = isalpha(tex[0]);
      unknownnew = 0;
      if (!strcmp(left(tex, 10), "\\mbox{\\rm ")) { /* Token not in table */
        unknownnew = 1;
      }
      alphold = isalpha(lastTex[0]);
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

    firstToken = 0;

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
          memcpy(modeSection, *srcptr, ptr - (*srcptr));
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
          memcpy(modeSection, *srcptr, ptr - (*srcptr));
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

  if (!texDefsRead) {
    if (!readTexDefs()) {
      print2(
          "?There was an error in the $t comment's Latex/HTML definitions.\n");
      return;
    }
  }

  outputToString = 1;  /* Redirect print2 and printLongLine to printString */
  /*let(&printString, "");*/ /* May have stuff to be printed 7/4/98 */
  if (!htmlFlag) {
    print2("%s This LaTeX file was created by Metamath on %s %s.\n",
       "%", date(), time_());

    if (texHeaderFlag) {
      /******* LaTeX 2.09
      print2(
"\\documentstyle[leqno]{article}\n");
      *******/
      /* LaTeX 2e */
      print2(
    "\\documentclass[leqno]{article}\n");
      /* LaTeX 2e */
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

    /* Print style sheet for pink number that goes after statement label */
    print2("<STYLE TYPE=\"text/css\">\n");
    print2("<!--\n");
    print2(".p { font-family: \"Arial Narrow\";\n");
    print2("     font-size: x-small;\n");
    /* Strip off quotes from color (css doesn't like them) */
    printLongLine(cat("     color: ", seg(PINK_NUMBER_COLOR, 2,
        strlen(PINK_NUMBER_COLOR) - 1), ";", NULL), "", "&");
    print2("   }\n");
    print2("-->\n");
    print2("</STYLE>\n");

    /*
    print2("<META NAME=\"ROBOTS\" CONTENT=\"NONE\">\n");
    print2("<META NAME=\"GENERATOR\" CONTENT=\"Metamath\">\n");
    */
    if (showStatement < extHtmlStmt) {
      print2("%s\n", cat("<TITLE>", htmlTitle, " - ",
          /* Strip off ".html" */
          left(texFileName, strlen(texFileName) - 5),
          /*left(texFileName, instr(1, texFileName, ".htm") - 1),*/
          "</TITLE>", NULL));
    } else {
      print2("%s\n", cat("<TITLE>", extHtmlTitle, " - ",
          /* Strip off ".html" */
          left(texFileName, strlen(texFileName) - 5),
          /*left(texFileName, instr(1, texFileName, ".htm") - 1),*/
          "</TITLE>", NULL));
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

    print2("<TABLE BORDER=0 WIDTH=\"100%s\"><TR>\n", "%");
    print2("<TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%s\"\n", "%");

    /*
    print2("<A HREF=\"mmexplorer.html\">\n");
    print2("<IMG SRC=\"cowboy.gif\"\n");
    print2("ALT=\"[Image of cowboy]Home Page\"\n");
    print2("WIDTH=27 HEIGHT=32 ALIGN=LEFT><FONT SIZE=-1 FACE=sans-serif>Home\n");
    print2("</FONT></A>");
    */
    if (showStatement < extHtmlStmt) {
      printLongLine(cat("ROWSPAN=2>", htmlHome, "</TD>", NULL), "", "\"");
    } else {
      printLongLine(cat("ROWSPAN=2>", extHtmlHome, "</TD>", NULL), "", "\"");
    }

    if (showStatement < extHtmlStmt) {
      printLongLine(cat(
          "<TD NOWRAP ALIGN=CENTER ROWSPAN=2><FONT SIZE=\"+3\" COLOR=",
          GREEN_TITLE_COLOR, "><B>", htmlTitle, "</B></FONT>", NULL), "", "\"");
    } else {
      printLongLine(cat(
          "<TD NOWRAP ALIGN=CENTER ROWSPAN=2><FONT SIZE=\"+3\" COLOR=",
          GREEN_TITLE_COLOR, "><B>", extHtmlTitle, "</B></FONT>", NULL), "", "\"");
    }


    if (texHeaderFlag) {
      /* Put Previous/Next links into web page */
      /*print2("</TD><TD ALIGN=RIGHT VALIGN=TOP><FONT SIZE=-1 FACE=sans-serif>\n");*/
      print2("</TD><TD NOWRAP ALIGN=RIGHT VALIGN=TOP WIDTH=\"25%s\"><FONT\n", "%");
      print2(" SIZE=-1 FACE=sans-serif>\n");
      /* Find the previous statement with a web page */
      j = 0;
      k = 0;
      for (i = showStatement - 1; i >= 1; i--) {
        if (statement[i].type == (char)p__ ||
            statement[i].type == (char)a__ ) {
          j = i;
          break;
        }
      }
      if (j == 0) {
        k = 1; /* First statement flag */
        /* For the first statement, wrap to last one */
        for (i = statements; i >= 1; i--) {
          if (statement[i].type == (char)p__ ||
              statement[i].type == (char)a__ ) {
            j = i;
            break;
          }
        }
      }
      if (j == 0) bug(2314);
      print2("<A HREF=\"%s.html\">\n",
          statement[j].labelName);
      if (!k) {
        print2("&lt; Previous</A>&nbsp;&nbsp;\n");
      } else {
        print2("&lt; Wrap</A>&nbsp;&nbsp;\n");
      }
      /* Find the next statement with a web page */
      j = 0;
      k = 0;
      for (i = showStatement + 1; i <= statements; i++) {
        if (statement[i].type == (char)p__ ||
            statement[i].type == (char)a__ ) {
          j = i;
          break;
        }
      }
      if (j == 0) {
        k = 1; /* Last statement flag */
        /* For the last statement, wrap to first one */
        for (i = 1; i <= statements; i++) {
          if (statement[i].type == (char)p__ ||
              statement[i].type == (char)a__ ) {
            j = i;
            break;
          }
        }
      }
      if (j == 0) bug(2315);
      /*print2("<A HREF=\"%s.html\">Next</A></FONT>\n",*/
      if (!k) {
        print2("<A HREF=\"%s.html\">Next &gt;</A>\n",
            statement[j].labelName);
      } else {
        print2("<A HREF=\"%s.html\">Wrap &gt;</A>\n",
            statement[j].labelName);
      }

      /* ??? Is the closing </FONT> printed if there is no altHtml?
         This should be tested.  8/9/03 ndm */

      /* Print the GIF/Unicode Font choice, if directories are specified */
      if (htmlDir[0]) {
        if (altHtmlFlag) {
          print2("</FONT></TD></TR><TR><TD ALIGN=RIGHT><FONT FACE=sans-serif\n");
          print2("SIZE=-2>Symbols look wrong? <BR>Try the\n");
          print2(" <A HREF=\"%s%s\">GIF version</A>.</FONT></TD>\n",
              htmlDir, texFileName);
        } else {
          print2("</FONT></TD></TR><TR><TD ALIGN=RIGHT><FONT FACE=sans-serif\n");
          print2("SIZE=-2>Browser slow? Try the\n");
          /* 8/8/03 Symbol font is obsolete
          print2("<BR><A HREF=\"%s%s\">Symbol\n",
              altHtmlDir, texFileName);
          print2("font version</A>.</FONT></TD>\n");
          */
          print2("<BR><A HREF=\"%s%s\">Unicode\n",
              altHtmlDir, texFileName);
          print2("version</A>.</FONT></TD>\n");
        }
      }

    } else {
      print2("</TD><TD ALIGN=RIGHT VALIGN=TOP\n");
      print2(" WIDTH=\"25%s\">&nbsp;\n", "%");

      /* Print the GIF/Unicode Font choice, if directories are specified */
      if (htmlDir[0]) {
        print2("<FONT FACE=sans-serif SIZE=-2>\n");
        if (altHtmlFlag) {
          print2("<A HREF=\"%s%s\">GIF version</A></FONT>\n",
              htmlDir, texFileName);
        } else {
          print2("<A HREF=\"%s%s\">Unicode version</A></FONT>\n",
              altHtmlDir, texFileName);
        }
      }

      print2("</TD></TR><TR><TD ALIGN=RIGHT VALIGN=TOP\n");
      print2(" WIDTH=\"25%s\">&nbsp;</TD>\n", "%");
    }

    print2("</TR></TABLE>\n");

    /*
    print2("<CENTER><H1><FONT\n");
    if (showStatement < extHtmlStmt) {
      print2("COLOR=%s>%s</FONT></H1></CENTER>\n",
         GREEN_TITLE_COLOR, htmlTitle);
    } else {
      print2("COLOR=%s>%s</FONT></H1></CENTER>\n",
          GREEN_TITLE_COLOR, extHtmlTitle);
    }
    */

    print2("<HR NOSHADE SIZE=1>\n");

  } /* htmlFlag */
  fprintf(texFilePtr, "%s", printString);
  outputToString = 0;
  let(&printString, "");

} /* printTexHeader */

/* Prints an embedded comment in TeX.  The commentPtr must point to the first
   character after the "$(" in the comment.  The printout ends when the first
   "$)" or null character is encountered.   commentPtr must not be a temporary
   allocation.  */
void printTexComment(vstring commentPtr)
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
  flag preMode = 0; /* HTML <PRE> preformatted mode */

  /* 10/10/02 For bibliography hyperlinks */
  vstring bibTag = "";
  vstring bibFileName = "";
  vstring bibFileContents = "";
  vstring bibFileContentsUpper = ""; /* Uppercase version */
  vstring bibTags = "";
  long pos1, pos2, htmlpos1, htmlpos2;

  /* Variables for converting ` ` and ~ to old $m,$n and $l,$n formats in
     order to re-use the old code */
  /* Note that DOLLAR_SUBST will replace the old $. */
  vstring cmt = "";
  long i, clen;

  /* We must let this procedure handle switching output to string mode */
  if (outputToString) bug(2309);

  cmtptr = commentPtr;

  if (!texDefsRead) return; /* TeX defs were not read (error was detected
                               and flagged to the user elsewhere) */

  /* Convert line to the old $m..$n and $l..$n formats (using DOLLAR_SUBST
     instead of "$") - the old syntax is obsolete but we do this conversion
     to re-use some old code */
  i = instr(1, cmtptr, "$)");      /* If it points to source buffer */
  if (!i) i = strlen(cmtptr) + 1;  /* If it's a stand-alone string */
  let(&cmt, left(cmtptr, i - 1));

  /* 10/10/02 Add leading and trailing HTML markup to comment here
     (instead of in caller).  Also convert special characters. */
  /* This section is independent and can be removed without side effects */
  if (htmlFlag) {
    /* Convert special characters <, &, etc. to HTML entities */
    /* But skip converting math symbols inside ` ` */
    pos1 = 0;
    mode = 1; /* 1 normal, -1 math token */
    let(&tmp, "");
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
      if (!pos1) pos1 = strlen(cmt) + 1;
      if (mode == 1) {
        let(&tmpStr, "");
        tmpStr = asciiToTt(left(cmt, pos1));
      } else {
        let(&tmpStr, left(cmt, pos1));
      }
      let(&tmp, cat(tmp, tmpStr, NULL));
      let(&cmt, right(cmt, pos1 + 1));
      if (!cmt[0]) break;
      mode = -mode;
    }
    let(&cmt, tmp);
    let(&tmpStr, ""); /* Deallocate */

    /* This used to be done in mmcmds.c */
    let(&cmt, cat("<CENTER><TABLE><TR><TD ALIGN=LEFT><B>Description: </B>", cmt,
        "</TD></TR></TABLE></CENTER>", NULL));
  }

  /* 10/10/02 Convert _abc_ emphasis in comments to <I>abc</I> in HTML for
     book titles, etc. */
  /* This section is independent and can be removed without side effects */
  if (htmlFlag) {
    pos1 = 0;
    while (1) {
      pos1 = instr(pos1 + 1, cmt, "_");
      if (!pos1) break;
      /* Opening "_" must be <nonalphanum>_<alphanum> */
      if (pos1 > 1) {
        if (isalnum(cmt[pos1 - 2])) continue;
      }
      if (!isalnum(cmt[pos1])) continue;
      pos2 = instr(pos1 + 1, cmt, "_");
      if (!pos2) break;
      /* Closing "_" must be <alphanum>_<nonalphanum> */
      if (!isalnum(cmt[pos2 - 2])) continue;
      if (isalnum(cmt[pos2])) continue;
      let(&cmt, cat(left(cmt, pos1 - 1), "<I>", seg(cmt, pos1 + 1, pos2 - 1),
          "</I>", right(cmt, pos2 + 1), NULL));
      pos1 = pos2 + 5; /* Adjust for 5 extra chars in "let" above */
    }
  }

  /* 10/10/02 Added bibliography hyperlinks */
  /* This section is independent and can be removed without side effects */
  if (htmlFlag) {
    /* Assign local tag list and local HTML file name */
    if (showStatement < extHtmlStmt) {
      let(&bibTags, htmlBibliographyTags);
      let(&bibFileName, htmlBibliography);
    } else {
      let(&bibTags, extHtmlBibliographyTags);
      let(&bibFileName, extHtmlBibliography);
    }
    if (bibFileName[0]) {
      /* The user specified a bibliography file in the xxx.mm $t comment
         (otherwise we don't do anything) */
      pos1 = 0;
      while (1) {
        /* Look for any bibliography tags to convert to hyperlinks */
        /* The biblio tag should be in brackets e.g. "[Monk2]" */
        pos1 = instr(pos1 + 1, cmt, "[");
        if (!pos1) break;
        pos2 = instr(pos1 + 1, cmt, "]");
        if (!pos2) break;
        let(&bibTag, seg(cmt, pos1, pos2));
        /* There should be no white space in the tag */
        if (strcspn(bibTag, " \n\r\t\f") < pos2 - pos1 + 1) continue;
        /* OK, we have a good tag.  If the file with bibliography has not been
           read in yet, let's do so here for error-checking. */

        /* Start of error-checking */
        if (!bibTags[0]) {
          /* The bibliography file has not be read in yet. */
          let(&bibFileContents, "");
          print2("Reading HTML bibliographic tags from file \"%s\"...\n",
              bibFileName);
          bibFileContents = readFileToString(bibFileName, 0);
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
              }
              /* Add tag to tag list */
              let(&bibTags, cat(bibTags, tmp, NULL));
            } /* end while */
            if (!bibTags[0]) {
              /* No tags found; put dummy partial tag meaning "file read" */
              let(&bibTags, "[");
            }
          } /* end if (!bibFIleContents) */
          /* Assign to permanent tag list for next time */
          if (showStatement < extHtmlStmt) {
            let(&htmlBibliographyTags, bibTags);
          } else {
            let(&extHtmlBibliographyTags, bibTags);
          }
          /* Done reading in HTML file with bibliography */
        } /* end if (!bibTags[0]) */
        /* See if the tag we found is in the bibliography file */
        if (bibTags[0] == '[') {
          /* We have a tag list from the bibliography file */
          if (!instr(1, bibTags, bibTag)) {
            printLongLine(cat("?Error: The bibliographic reference \"", bibTag,
                "\" in statement \"", statement[showStatement].labelName,
                "\" was not found as an an <A NAME=\"",
                seg(bibTag, 2, pos2 - pos1),
                "\"></A> tag in the file \"", bibFileName, "\".", NULL),
                "", " ");
          }
        }
        /* End of error-checking */

        /* Make an HTML reference for the tag */
        let(&tmp, cat("[<A HREF=\"",
            bibFileName, "#", seg(bibTag, 2, pos2 - pos1), "\">",
            seg(bibTag, 2, pos2 - pos1), "</A>]", NULL));
        let(&cmt, cat(left(cmt, pos1 - 1), tmp, right(cmt,
            pos2 + 1), NULL));
        pos1 = pos1 + strlen(tmp) - strlen(bibTag); /* Adjust comment position */
      } /* end while(1) */
    } /* end if (bibFileName[0]) */
  } /* end of if (htmlFlag) */
  /* 10/10/02 End of bibliography hyperlinks */


  clen = strlen(cmt);
  mode = 'n';
  for (i = 0; i < clen; i++) {
    if (cmt[i] == '`') {
      if (cmt[i + 1] == '`') {
        /* Escaped ` = actual ` */
        let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
        clen--;
      } else {
        /* Enter or exit math mode */
        if (mode != 'm') {
          mode = 'm';
        } else {
          mode = 'n';
        }
        let(&cmt, cat(left(cmt, i), chr(DOLLAR_SUBST) /*$*/, chr(mode),
            right(cmt, i+2), NULL));
        clen++;
        i++;

        /* 10/10/02 */
        /* If symbol is preceded by a space and opening punctuation, take out
           the space so it looks better. */
        if (mode == 'm') {
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
        if (mode == 'n') {
          /* (Why must it be i + 2 here but i + 1 in label version below?
             Didn't investigate but seems strange.) */
          let(&tmp, mid(cmt, i + 2, 2));
          if (tmp[0] == ' ' && strchr(".,;)?!:]", tmp[1]) != NULL) {
            let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
            clen = clen - 1;
          }
          /* We include quotes since symbols are often enclosed in them. */
          let(&tmp, mid(cmt, i + 2, 8));
          if (strlen(tmp) < 8)
              let(&tmp, cat(tmp, space(8 - strlen(tmp)), NULL));
          if (!strcmp(" &quot;", left(tmp, 7))
              && strchr(".,;)?!:] ", tmp[7]) != NULL) {
            let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
            clen = clen - 1;
          }
          let(&tmp, "");
        }

      }
    }
    if (cmt[i] == '~' && mode != 'm') {
      if (cmt[i + 1] == '~') {
        /* Escaped ~ = actual ~ */
        let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
        clen--;
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
          while (isspace(cmt[i + 1]) && clen > i + 1) {
            let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
            clen--;
          }
        } else {
          /* This really should never happen */
          bug(2310);
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
    if (isspace(cmt[i]) && mode == 'l') {
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
      if (tmp[0] == ' ' && strchr(".,;)?!:]", tmp[1]) != NULL) {
        let(&cmt, cat(left(cmt, i), right(cmt, i + 2), NULL));
        clen = clen - 1;
      }
      let(&tmp, "");

    }
    /* clen should always remain comment length - do a sanity check here */
    if (strlen(cmt) != clen) {
      bug(2311);
    }
  } /* Next i */
  cmtptr = cmt;
  /* End convert line to the old $m..$n and $l..$n */


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
      if (cmtptr[0] == '$') {
        if (cmtptr[1] == ')') {
          bug(2312); /* Obsolete (will never happen) */
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
    memcpy(sourceLine, lineStart, cmtptr - lineStart);
    cmtptr++;  /* Get past new-line to prepare for next line's scan */

    /* If the line contains only math mode text, use TeX display mode. */
    displayMode = 0;
    let(&tmpStr, edit(sourceLine, 8 + 128)); /* Trim spaces */
    if (!strcmp(right(tmpStr, strlen(tmpStr) - 1), cat(chr(DOLLAR_SUBST), "n",
        NULL))) let(&tmpStr, left(tmpStr, strlen(tmpStr) - 2)); /* Strip $n */
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
          let(&modeSection, edit(modeSection, 8 + 128 + 16));  /* Discard
                      leading and trailing blanks; reduce spaces to one space */
          let(&tmpStr, "");
          tmpStr = asciiToTt(modeSection);
          if (!tmpStr[0]) { /* Can't be blank */
            bug(2313);
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
          }
          *******/

          /* 10/10/02 Do binary search through just $a's and $p's (there are no
             html pages for local labels) */
          i = lookupLabel(tmpStr);
          if (i < 0) {
            outputToString = 0;
            printLongLine(cat("?Error: The token \"", tmpStr,
                "\" (referenced in comment of statement \"",
                statement[showStatement].labelName,
                "\") is not a $a or $p statement label.", NULL), "", " ");
            outputToString = 1;
          }

          if (!htmlFlag) {
            let(&outputLine, cat(outputLine, "{\\tt ", tmpStr,
               "}", NULL));
          } else {
            let(&tmp, "");
            tmp = pinkHTML(i);
            let(&outputLine, cat(outputLine, "<A HREF=\"", tmpStr,
               ".html\">", tmpStr, "</A>", tmp, NULL));
          }
          let(&tmpStr, ""); /* Deallocate */
          break;
        case 'm': /* Math mode */
          let(&tmpStr, "");
          tmpStr = asciiMathToTex(modeSection);
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
            let(&outputLine, cat(outputLine, tmpStr, NULL)); /* html */
          }
          let(&tmpStr, ""); /* Deallocate */
          break;
      } /* End switch(mode) */
      let(&modeSection, ""); /* Deallocate */
    }
    let(&outputLine, edit(outputLine, 128)); /* remove trailing spaces */

    if (htmlFlag) {
      /* Change blank lines into paragraph breaks except in <PRE> mode */
      if (!outputLine[0]) { /* Blank line */
        if (!preMode) let(&outputLine, "<P>"); /* Make it a paragraph break */
      }
      /* 11/15/02 If a statement comment has a section embedded in
         <PRE>...</PRE>, we make it a table with monospaced text and a
         background color */
      pos1 = instr(1, outputLine, "<PRE>");
      if (pos1) {
        preMode = 1; /* So we don't put <P> for blank lines */
        let(&outputLine, cat(left(outputLine, pos1 - 1),
            "<P><CENTER><TABLE BORDER=0 CELLSPACING=0 CELLPADDING=10 BGCOLOR=",
            /* MINT_BACKGROUND_COLOR, */
            "\"#F0F0F0\"", /* Very light grey */
            "><TR><TD ALIGN=LEFT>", right(outputLine, pos1), NULL));
      }
      pos1 = instr(1, outputLine, "</PRE>");
      if (pos1) {
        preMode = 0;
        let(&outputLine, cat(left(outputLine, pos1 + 5),
            "</TD></TR></TABLE></CENTER>", right(outputLine, pos1 + 6), NULL));
      }
    }
    printLongLine(outputLine, "", htmlFlag ? "\"" : "\\");
    let(&tmp, ""); /* Clear temporary allocation stack */

    if (lastLineFlag) break; /* Done */
  } /* end while(1) */

  print2("\n");
  outputToString = 0; /* Restore normal output */
  fprintf(texFilePtr, "%s", printString);

  let(&printString, ""); /* Deallocate strings */
  let(&sourceLine, "");
  let(&outputLine, "");
  let(&cmt, "");
  let(&tmpComment, "");
  let(&tmp, "");
  let(&tmpStr, "");
  let(&bibTag, "");
  let(&bibFileName, "");
  let(&bibFileContents, "");
  let(&bibFileContentsUpper, "");
  let(&bibTags, "");

} /* printTexComment */


/* Warning:  contPrefix must not be temporarily allocated by caller */
void printTexLongMath(nmbrString *mathString, vstring startPrefix,
    vstring contPrefix, long hypStmt)
    /* hypStmt, if non-zero, is the statement number to be referenced
       next to the hypothesis link in html */
{
  long i;
  long pos;
  vstring tex;
  vstring texLine = "";
  vstring lastTex = "";
  vstring sPrefix = ""; /* 7/3/98 */
  vstring htmStep = ""; /* 7/4/98 */
  vstring htmHyp = ""; /* 7/4/98 */
  vstring htmRef = ""; /* 7/4/98 */
  vstring tmp = "";  /* 10/10/02 */
  flag alphnew, alphold, unknownnew, unknownold;

  let(&sPrefix, startPrefix); /* 7/3/98 Save it; it may be temp alloc */

  if (!texDefsRead) return; /* TeX defs were not read (error was detected) */
  outputToString = 1; /* Redirect print2 and printLongLine to printString */
  if (!htmlFlag)  /* May have stuff to be printed 7/4/98 */
    let(&printString, "");

  tex = asciiToTt(sPrefix); /* asciiToTt allocates; we must deallocate */
  let(&texLine, "");
  if (!htmlFlag) {

    /* 9/2/99 Trim down long start prefixes so they won't overflow line,
       by putting their tokens into \m macros */
#define TRIMTHRESHOLD 60
    i = strlen(tex);
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
  } else { /* htmlFlag */
    if (strlen(sPrefix)) { /* It's a proof step */
      /* Make each token a separate table column */
      /* This is a kludge that only works with /LEMMON style proofs! */
      let(&tex, edit(tex, 8 + 16 + 128));
      i = 0;
      pos = 1;
      while (pos) {
        pos = instr(1, tex, " ");
        if (pos) {
          if (i > 3) { /* 2/8/02 - added for extra safety for the future */
            bug(2316);
          }
          if (i == 0) let(&htmStep, left(tex, pos - 1));
          if (i == 1) let(&htmHyp, left(tex, pos - 1));
          if (i == 2) let(&htmRef, left(tex, pos - 1));
          let(&tex, right(tex, pos + 1));
          i++;
        }
      }
      if (i < 3) { /* Insert blank field for Lemmon ref w/out hyp */
        let(&htmRef, htmHyp);
        let(&htmHyp, "&nbsp;");
      }

      /* 2/8/02 Add a space after each comma so very long hypotheses lists will wrap in
         an HTML table cell, e.g. gomaex3 in ql.mm */
      pos = instr(1, htmHyp, ",");
      while (pos) {
        let(&htmHyp, cat(left(htmHyp, pos), " ", right(htmHyp, pos + 1), NULL));
        pos = instr(pos + 1, htmHyp, ",");
      }

      if (!strcmp(tex, "$e") || !strcmp(tex, "$f")) {
        /* A hypothesis - don't include link */
        printLongLine(cat("<TR ALIGN=LEFT><TD>", htmStep, "</TD><TD>",
            htmHyp, "</TD><TD>", htmRef,
            "</TD><TD>", NULL), "", "\"");
      } else {
        if (hypStmt <= 0) {
          printLongLine(cat("<TR ALIGN=LEFT><TD>", htmStep, "</TD><TD>",
              htmHyp, "</TD><TD><A HREF=\"", htmRef, ".html\">", htmRef,
              "</A></TD><TD>", NULL), "", "\"");
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
          let(&tmp, cat(" ", right(tmp, strlen(PINK_NBSP) + 1), NULL));
          *******/
          printLongLine(cat("<TR ALIGN=LEFT><TD>", htmStep, "</TD><TD>",
              htmHyp, "</TD><TD><A HREF=\"", htmRef, ".html\">", htmRef,
              "</A>", tmp,
              "</TD><TD>", NULL), "", "\"");
        }
      }
      let(&htmStep, ""); /* Deallocate */
      let(&htmHyp, ""); /* Deallocate */
      let(&htmRef, ""); /* Deallocate */
      let(&tmp, ""); /* Deallocate */
    } /* strlen(sPrefix) */
  }
  let(&tex, ""); /* Deallocate */
  let(&sPrefix, ""); /* Deallocate */

  let(&lastTex, "");
  for (pos = 0; pos < nmbrLen(mathString); pos++) {
    tex = tokenToTex(mathToken[mathString[pos]].tokenName);
              /* tokenToTex allocates tex; we must deallocate it */
    if (!htmlFlag) {  /* LaTeX */
      /* If this token and previous token begin with letter, add a thin
           space between them */
      /* Also, anything not in table will have space added */
      alphnew = isalpha(tex[0]);
      unknownnew = 0;
      if (!strcmp(left(tex, 10), "\\mbox{\\rm ")) { /* Token not in table */
        unknownnew = 1;
      }
      alphold = isalpha(lastTex[0]);
      unknownold = 0;
      if (!strcmp(left(lastTex, 10), "\\mbox{\\rm ")) { /* Token not in table*/
        unknownold = 1;
      }
      /*if ((alphold && alphnew) || unknownold || (unknownnew && pos > 0)) {*/
      /* Put thin space only between letters and/or unknowns  11/3/94 */
      if ((alphold || unknownold) && (alphnew || unknownnew)) {
        /* Put additional thin space between two letters */
        let(&texLine, cat(texLine, "\\m{\\,", tex, "}", NULL));
      } else {
        let(&texLine, cat(texLine, "\\m{", tex, "}", NULL));
      }
    } else {  /* HTML */

      /* 7/27/03 When we have something like "E. x e. om x = y", the lack of
         space between om and x looks ugly in HTML.  This kludge adds it in
         for restricted quantifiers not followed by parenthesis, in order
         to make the web page look a little nicer.  E.g. onminex. */
      if (pos >=4) {
        if (!strcmp(mathToken[mathString[pos - 2]].tokenName, "e.")
            && (!strcmp(mathToken[mathString[pos - 4]].tokenName, "E.")
              || !strcmp(mathToken[mathString[pos - 4]].tokenName, "A."))
            && strcmp(mathToken[mathString[pos]].tokenName, "(")
            /* It also shouldn't be restricted _to_ an expression in parens. */
            && strcmp(mathToken[mathString[pos - 1]].tokenName, "(")) {
          let(&texLine, cat(texLine, " ", NULL)); /* Add a space */
        }
      }
      /* This one puts a space between the 2 x's in a case like
         "E. x x = y".  E.g. cla4egf */
      if (pos >=2) {
        /* Match a token starting with a letter */
        if (isalpha(mathToken[mathString[pos]].tokenName[0])) {
          /* and make sure its length is 1 */
          if (!(mathToken[mathString[pos]].tokenName[1])) {
            /* See if it's 1st letter in a quantified expression */
            if (!strcmp(mathToken[mathString[pos - 2]].tokenName, "E.")
                || !strcmp(mathToken[mathString[pos - 2]].tokenName, "A.")) {
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
      /* 7/27/03 end */

      let(&texLine, cat(texLine, tex, NULL));
    } /* if !htmlFlag */
    let(&lastTex, ""); /* Deallocate */
    lastTex = tex; /* Pass deallocation responsibility for tex to lastTex */

  } /* Next pos */
  let(&lastTex, ""); /* Deallocate */
  if (!htmlFlag) {  /* LaTeX */
    printLongLine(texLine, "", "\\");
    print2("\\endm\n");
  } else {  /* HTML */
    /* 8/9/03 Discard redundant white space to reduce HTML file size */
    let(&texLine, edit(texLine, 8 + 16 + 128));

    /* 4/22/01 The code below is optional but tries to eliminate redundant
       symbol font specifications to make output shorter */
    /* 8/8/03 Symbol font is finally obsolete; don't bother to do this
       anymore.  (Some day delete this code that is skipped over.) */
    if (pos == pos) /* Same as "if (1)" but prevents compiler warning */
      goto skip_SymbolFont;
    pos = 0;
    while (1) {
      /* Get to the start of a symbol font specification */
      pos = instr(pos + 1, texLine, "<FONT FACE=\"Symbol\">");
      if (pos == 0) break;
      while (1) {   /* Anchoring at pos, scan rest of line */
        i = instr(pos, texLine, "</FONT>"); /* End of specification */
        if (i == 0) {
          /* For whatever reason, there is no matching </FONT> - so skip it */
          break;
        }
        /* See if an new symbol font spec starts immediately; if not
           we are done (inefficient code but it works) */
        if (i != instr(i, texLine, "</FONT><FONT FACE=\"Symbol\">")) break;
        /* Chop out the redundant </FONT><FONT FACE="Symbol"> */
        let(&texLine, cat(left(texLine, i - 1), right(texLine, i + 27), NULL));
      }
    }
   skip_SymbolFont:  /* 8/8/03 */
    /* End of 4/22/01 code */

    printLongLine(cat(texLine, "</TD></TR>", NULL), "", "\"");
  }


  outputToString = 0; /* Restore normal output */
  fprintf(texFilePtr, "%s", printString);
  let(&printString, "");

  let(&texLine, ""); /* Deallocate */
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
                after proof; made htmlVarColors global for this
      print2("<FONT SIZE=-1 FACE=sans-serif>Colors of variables:\n");
      printLongLine(cat(htmlVarColors, "</FONT>", NULL), "", " ");
      *******/
      print2("</TABLE></CENTER><CENTER><FONT SIZE=-2 FACE=sans-serif>\n");
      /*
      print2("<A HREF=\"definitions.html\">Definition list</A> |\n");
      print2("<A HREF=\"theorems.html\">Theorem list</A><BR>\n");
      */
      /*
      print2("Copyright &copy; 2002 \n");
      */
      print2("The Metamath Home Page is\n");
      print2("<A HREF=\"http://metamath.org\">metamath.org</A>\n");
      print2("</FONT></CENTER>\n");
      print2("</BODY></HTML>\n");
    }
    outputToString = 0; /* Restore normal output */
    fprintf(texFilePtr, "%s", printString);
    let(&printString, "");
  }

} /* printTexTrailer */



/* Returns the pink number printed next to statement labels in HTML output */
/* The pink number only counts $a and $p statements, unlike the statement
   number which also counts $f, $e, $c, $v, ${, $} */
/* 10/10/02 This is no longer used? */
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
    if (statement[i].type == a__ || statement[i].type == p__)
      statemMap++;
  }
  return statemMap;
}


/* Added 10/10/02 */
/* Returns HTML for the pink number to print after the statement labels
   in HTML output. */
/* Warning: The caller must deallocate the returned vstring (i.e. this
   function cannot be used in let statements but must be assigned to
   a local vstring for local deallocation) */
vstring pinkHTML(long statemNum)
{
  long statemMap;
  vstring htmlCode = "";

  /* The pink number only counts $a and $p statements, unlike the statement
     number which also counts $f, $e, $c, $v, ${, $} */
  /* 10/25/02 Added pinkNumber to the statement[] structure for speedup. */
  /*
  statemMap = 0;
  for (i = 1; i <= statemNum; i++) {
    if (statement[i].type == a__ || statement[i].type == p__)
      statemMap++;
  }
  */
  statemMap = statement[statemNum].pinkNumber;

  /* Without style sheet */
  /*
  let(&htmlCode, cat(PINK_NBSP,
      "<FONT FACE=\"Arial Narrow\" SIZE=-2 COLOR=", PINK_NUMBER_COLOR, ">",
      str(statemMap), "</FONT>", NULL));
  */

  /* With style sheet */
  let(&htmlCode, cat(PINK_NBSP,
      "<SPAN CLASS=p>",
      str(statemMap), "</SPAN>", NULL));

  return htmlCode;
}

