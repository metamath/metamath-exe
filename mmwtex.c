/*****************************************************************************/
/*       Copyright (C) 2000  NORMAN D. MEGILL nm@alum.mit.edu                */
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
#include "mmutil.h"
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h" /* For rawSourceError and mathSrchCmp */
#include "mmwtex.h"
#include "mmcmdl.h" /* For texFileName */

/* 6/27/99 - Now, all LaTeX and HTML definitions are taken from the source
   file (read in the by READ... command).  In the source file, there should
   be a single comment $( ... $) containing the keyword $t.  The definitions
   start after the $t and end at the $).  Between $t and $), the definition
   source should exist.  See the file set.mm for an example. */

/* Storage of graphics for math tokens */
vstring graTokenName[MAX_GRAPHIC_DEFS]; /* ASCII abbreviation of token */
int graTokenLength[MAX_GRAPHIC_DEFS];   /* length of graTokenName */
vstring graTokenDef[MAX_GRAPHIC_DEFS]; /* Graphics definition */
int maxGraDef = 0; /* Number of tokens defined so far */
int maxGraTokenLength = 0; /* Largest graTokenName length */

/* HTML flag: 0 = TeX, 1 = HTML */
flag htmlFlag = 0;

/* Tex output file */
FILE *texFilePtr = NULL;
flag texFileOpenFlag = 0;

/* Tex dictionary */
FILE *tex_dict_fp;
vstring tex_dict_fn = "";

/* Global variables */
flag texDefsRead = 0;

/* Variables local to this module */
struct texDef_struct {
  vstring tokenName; /* ASCII token */
  vstring texEquiv; /* Converted to TeX */
};
struct texDef_struct *texDefs;
long numSymbs;
char dollarSubst = 2;
    /* Substitute character for $ in converting to obsolete $l,$m,$n
       comments - Use '$' instead of non-printable ASCII 2 for debugging */
vstring htmlVarColors = ""; /* Set by htmlvarcolor commands */
vstring htmlTitle = ""; /* Set by htmltitle command */
vstring htmlHome = ""; /* Set by htmlhome command */

flag readTexDefs(vstring tex_def_fn, flag promptForFile)
{

  FILE *tex_def_fp;
  char *fileBuf;
  char *startPtr;
  long fileBufSize;
  char *fbPtr;
  char *tmpPtr;
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

  /* Initial values below will be overridden if user command exists */
  let(&htmlTitle, "Metamath Proof Explorer"); /* Set by htmltitle command */
  let(&htmlHome, "mmexplorer.html"); /* Set by htmlhome command */

  if (texDefsRead) bug(2301); /* Shouldn't read file twice */

  if (promptForFile) {

    /******* obsolete 6/27/99
    if (!htmlFlag) {
      tex_def_fn = cmdInput1(
          "What is the name of the LaTeX definition file <latex.def>? ");
      if (!tex_def_fn[0]) {
        let(&tex_def_fn, "latex.def");
      }
    } else {
      tex_def_fn = cmdInput1(
          "What is the name of the HTML definition file <html.def>? ");
      if (!tex_def_fn[0]) {
        let(&tex_def_fn, "html.def");
      }
    }
    *******/
    if (!htmlFlag) {
      /*
      tex_def_fn = cmdInput1(cat(
          "What is the name of the file with LaTeX definitions <",
          input_fn, ">? ", NULL));
      */
      let(&tex_def_fn, "");
      if (!tex_def_fn[0]) {
        let(&tex_def_fn, input_fn);
      }
    } else {
      /*
      tex_def_fn = cmdInput1(cat(
          "What is the name of the HTML definition file <",
          input_fn, ">? ", NULL));
      */
      let(&tex_def_fn, "");
      if (!tex_def_fn[0]) {
        let(&tex_def_fn, input_fn);
      }
    }

  }
  tex_def_fp = fopen(tex_def_fn, "rb");
  if (!tex_def_fp) {
    print2("?The file \"%s\" could not be opened.\n", tex_def_fn);
    return (0);
  }

#ifndef SEEK_END
/* The GCC compiler doesn't have this ANSI standard constant defined. */
#define SEEK_END 2
#endif

  if (fseek(tex_def_fp, 0, SEEK_END)) bug(2302);
  fileBufSize = ftell(tex_def_fp);
/*E*/if(db5)print2("In binary mode the file has %ld bytes.\n",fileBufSize);

  /* Close and reopen the input file in text mode */
  fclose(tex_def_fp);
  tex_def_fp = fopen(tex_def_fn, "r");
  if (!tex_def_fp) bug(2303);

  /* Allocate space for the entire input file */
  fileBufSize = fileBufSize + 10; /* Add a factor for unknown text formats */
  fileBuf = malloc(fileBufSize * sizeof(char));
  if (!fileBuf) outOfMemory("#1 (fileBuf)");

  /* Put the entire input file into the buffer as a giant character string */
  charCount = fread(fileBuf, sizeof(char), fileBufSize - 2, tex_def_fp);
  if (!feof(tex_def_fp)) {
    /* If this bug occurs (due to obscure future format such as compressed
       text files) we'll have to add a realloc statement. */
    print2("Bin char cnt = %ld  Text char cnt = %ld\n", fileBufSize, charCount);
    bug(2304);
  }
  fclose(tex_def_fp);
  fileBuf[charCount] = 0; /* End of string */
/*E*/if(db5)print2("In text mode the file has %ld bytes.\n",charCount);

  /* See if last chr is newline (needed for comment terminator); if not, add */
  if (fileBuf[charCount - 1] != '\n') {
    fileBuf[charCount] = '\n';
    fileBuf[charCount + 1] = 0;
    rawSourceError(fileBuf, &fileBuf[charCount - 1], 1, 0, tex_def_fn,cat(
        "The last line in the file does not end with a \"line break\"",
        " character.  The \"line break\" character is a line feed in Unix",
        " and a carriage return on the Macintosh.",NULL));
    charCount++;
  }

#define LATEXDEF 1
#define HTMLDEF 2
#define HTMLVARCOLOR 3
#define HTMLTITLE 4
#define HTMLHOME 5


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
      print2("?There is no $t command in the file \"%s\".\n", tex_def_fn);
      print2(
"The file should have exactly one comment of the form $(...$t...$) with\n");
      print2("the LaTeX and HTML definitions between $t and $).\n");
      free(fileBuf);
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
    "?There is no $) comment closure after the $t command in \"%s\".\n",
          tex_def_fn);
      free(fileBuf);
      return 0;
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
      cmd = lookup(fbPtr, "latexdef,htmldef,htmlvarcolor,htmltitle,htmlhome");
      fbPtr[tokenLen] = zapChar;
      if (cmd == 0) {
        lineNum = 1;
        for (i = 0; i < (fbPtr - fileBuf); i++) {
          if (fileBuf[i] == '\n') lineNum++;
        }
        rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, tex_def_fn,
            cat("Expected \"latexdef\", \"htmldef\", \"htmlvarcolor\",",
            " \"htmltitle\" or \"htmlhome\" here.", NULL));
        free(fileBuf);
        return (0);
      }
      fbPtr = fbPtr + tokenLen;

      if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME) {
         /* Get next token - string in quotes */
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLen = texDefTokenLen(fbPtr);

        /* Process token - string in quotes */
        if (fbPtr[0] != '\"' && fbPtr[0] != '\'') {
          if (!tokenLen) { /* Abnormal end-of-file */
            fbPtr--; /* Format for error message */
            tokenLen++;
          }
          lineNum = 1;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, tex_def_fn,
              "Expected a quoted string here.");
          free (fileBuf);
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

          if ((cmd == LATEXDEF && !htmlFlag) || (cmd == HTMLDEF && htmlFlag)) {
            texDefs[numSymbs].tokenName = "";
            let(&(texDefs[numSymbs].tokenName), token);
          }
        }

        fbPtr = fbPtr + tokenLen;
      } /* if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME) */

      if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME) {
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
          lineNum = 1;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, tex_def_fn,
              "Expected the keyword \"as\" here.");
          free (fileBuf);
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
          lineNum = 1;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, tex_def_fn,
              "Expected a quoted string here.");
          free (fileBuf);
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
          lineNum = 1;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, tex_def_fn,
              "Expected \"+\" or \";\" here.");
          free (fileBuf);
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

        if ((cmd == LATEXDEF && !htmlFlag) || (cmd == HTMLDEF && htmlFlag)) {
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
      }

      if ((cmd == LATEXDEF && !htmlFlag) || (cmd == HTMLDEF && htmlFlag)) {
        numSymbs++;
      }

    } /* End while */

    if (fbPtr != fileBuf + charCount) bug(2305);

    if (parsePass == 1 ) {
      print2("%ld typesetting statements were read from \"%s\".\n",
          numSymbs, tex_def_fn);
      texDefs = malloc(numSymbs * sizeof(struct texDef_struct));
      if (!texDefs) outOfMemory("#99 (TeX symbols)");
    }

  } /* next parsePass */


  /* Sort the labels for later lookup */
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

  let(&token, ""); /* Deallocate */
  free(fileBuf);  /* Deallocate file source buffer */
  texDefsRead = 1;  /* Set global flag that it's been read in */
  return (1);

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
  long i, j, k;

  let(&ttstr, s);
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
          let(&ttstr,cat(left(ttstr,i),"&lt;",right(ttstr,i+2),NULL));
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
   label, or math).  If 1st char. is not "$" (dollarSubst), text mode is
   assumed.  mode = 0 means end of comment reached.  srcptr is left at 1st
   char. of start of next comment section. */
vstring getCommentModeSection(vstring *srcptr, char *mode)
{
  vstring modeSection = "";
  vstring ptr; /* Not allocated */
  flag addMode = 0;

  if ((*srcptr)[0] != dollarSubst /*'$'*/) {
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
    if (ptr[0] == dollarSubst /*'$'*/) {
      switch (ptr[1]) {
        case 'l':
        case 'm':
        case 'n':
        case ')':  /* Obsolete (will never happen) */
          let(&modeSection, space(ptr - (*srcptr)));
          memcpy(modeSection, *srcptr, ptr - (*srcptr));
          if (addMode) {
            let(&modeSection, cat(chr(dollarSubst), "n", /*"$n"*/ modeSection,
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
            let(&modeSection, cat(chr(dollarSubst), "n", /*"$n"*/ modeSection,
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


void printTexHeader(flag texHeaderFlag)
{

  if (!texDefsRead) {
    if (!readTexDefs("", 1)) {
      print2("?There was an error in the TeX symbol definition file.\n");
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
    print2("<HTML>\n");
    print2("<HEAD>\n");
    print2("<META HTTP-EQUIV=\"Content-Type\"\n");
    print2(" CONTENT=\"text/html; charset=iso-8859-1\">\n");
    print2("<META NAME=\"ROBOTS\" CONTENT=\"NONE\">\n");
    print2("<META NAME=\"GENERATOR\" CONTENT=\"Metamath\">\n");
    print2("%s\n", cat(" <TITLE>", htmlTitle, " - ",
        left(texFileName, instr(1, texFileName, ".htm") - 1),
        "</TITLE>", NULL));
    print2("<META HTML-EQUIV=\"Keywords\"\n");
    print2("CONTENT=\"%s\"></HEAD>\n", htmlTitle);
    /*print2("<BODY BGCOLOR=\"#D2FFFF\">\n");*/
    print2("<BODY BGCOLOR=\"#EEFFFA\">\n");

    /*
    print2("<A HREF=\"mmexplorer.html\">\n");
    print2("<IMG SRC=\"cowboy.gif\"\n");
    print2("ALT=\"[Image of cowboy]Home Page\"\n");
    print2("WIDTH=27 HEIGHT=32 ALIGN=LEFT><FONT SIZE=-2 FACE=ARIAL>Home\n");
    print2("</FONT></A>");
    */
    printLongLine(htmlHome, "", " ");

    print2("<H1><CENTER><FONT\n");
    print2("COLOR=\"#006600\">%s</FONT></CENTER></H1><HR>\n", htmlTitle);
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
  vstring tmpStr; /* Not allocated */
  vstring modeSection; /* Not allocated */
  vstring sourceLine = "";
  vstring outputLine = "";
  vstring tmp = "";
  flag textMode, mode, lastLineFlag, displayMode;

  /* Variables for converting ` ` and ~ to old $m,$n and $l,$n formats in
     order to re-use the old code */
  /* Note that dollarSubst will replace the old $. */
  vstring cmt = "";
  long i, clen;

  /* We should let this procedure handle switching output to string mode */
  if (outputToString) bug(2309);

  cmtptr = commentPtr;

  if (!texDefsRead) return; /* TeX defs were not read (error was detected) */

  /* Convert line to the old $m..$n and $l..$n formats (using dollarSubst
     instead of "$") - the old syntax is obsolete but we do this conversion
     to re-use some old code */
  i = instr(1, cmtptr, "$)");
  if (!i) i = strlen(cmtptr) + 1;
  let(&cmt, left(cmtptr, i - 1));
  clen = i - 1;
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
        let(&cmt, cat(left(cmt, i), chr(dollarSubst) /*$*/, chr(mode),
            right(cmt, i+2), NULL));
        clen++;
        i++;
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
        let(&cmt, cat(left(cmt, i), chr(dollarSubst) /*$*/, chr(mode),
            right(cmt, i+2), NULL));
        clen++;
        i++;
      }
    }
    if (isspace(cmt[i]) && mode == 'l') {
      /* Whitespace exits label mode */
      mode = 'n';
      let(&cmt, cat(left(cmt, i), chr(dollarSubst) /*$*/, chr(mode),
          right(cmt, i+1), NULL));
      clen = clen + 2;
      i = i + 2;
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
      if (cmtptr[0] == '$') {  /* Obsolete (will never happen) */
        bug(2312);
        if (cmtptr[1] == ')') {
          lastLineFlag = 1;
          break;
        }
      }
      if (cmtptr[0] == dollarSubst /*'$'*/) {
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
    tmpStr = "";
    let(&tmpStr, edit(sourceLine, 8 + 128)); /* Trim spaces */
    if (!strcmp(right(tmpStr, strlen(tmpStr) - 1), cat(chr(dollarSubst), "n",
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
          tmpStr = asciiToTt(modeSection);
          if (!tmpStr[0]) bug(2313); /* Can't be blank */

          /* 9/5/99 - Make sure the label in a comment corresponds to a real
             statement so we won't have broken HTML links (or misleading
             LaTeX information) - since label references in comments are
             rare, we just do a linear scan instead of binary search */
          for (i = 1; i <= statements; i++) {
            if (!strcmp(statement[i].labelName, tmpStr)) break;
          }
          if (i > statements) {
            outputToString = 0;
            printLongLine(cat("?Error: Statement \"", tmpStr,
               "\" (referenced in comment) does not exist.", NULL), "", " ");
            outputToString = 1;
          }

          if (!htmlFlag) {
            let(&outputLine, cat(outputLine, "{\\tt ", tmpStr,
               "}", NULL));
          } else {
            let(&outputLine, cat(outputLine, "<A HREF=\"", tmpStr,
               ".html\">", tmpStr, "</A>", NULL));
          }
          let(&tmpStr, ""); /* Deallocate */
          break;
        case 'm': /* Math mode */
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
            let(&outputLine, cat(outputLine, tmpStr, NULL)); /* html */
          }
          let(&tmpStr, ""); /* Deallocate */
          break;
      } /* End switch(mode) */
      let(&modeSection, ""); /* Deallocate */
    }
    let(&outputLine, edit(outputLine, 128)); /* remove trailing spaces */
    if (htmlFlag) {
      if (!outputLine[0]) { /* Blank line */
        let(&outputLine, "<P>"); /* Make it a paragraph break */
      }
    }
    printLongLine(outputLine, "", htmlFlag ? " " : "\\");
    let(&tmp, ""); /* Clear temporary allocation stack */

    if (lastLineFlag) break; /* Done */
  }

  print2("\n");
  outputToString = 0; /* Restore normal output */
  fprintf(texFilePtr, "%s", printString);
  let(&printString, ""); /* Deallocate strings */
  let(&sourceLine, "");
  let(&outputLine, "");
  let(&cmt, "");

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
  flag alphnew, alphold, unknownnew, unknownold;
  long hypStmtMap;

  /* Statement map for number of hypStmt - this makes the statement number
     next to the hypothesis link more meaningful, by counting only $a and
     $p. */
  /* ???This could be done once if we want to speed things up, but
     be careful because it will have to be redone if ERASE then READ */
  hypStmtMap = 0;
  for (i = 1; i <= hypStmt; i++) {
    if (statement[i].type == a__ || statement[i].type == p__) hypStmtMap++;
  }

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
      if (!strcmp(tex, "$e") || !strcmp(tex, "$f")) {
        /* A hypothesis - don't include link */
        printLongLine(cat("<TR><TD>", htmStep, "</TD><TD>",
            htmHyp, "</TD><TD>", htmRef,
            "</TD><TD>", NULL), "", " ");
      } else {
        if (hypStmt <= 0) {
          printLongLine(cat("<TR><TD>", htmStep, "</TD><TD>",
              htmHyp, "</TD><TD><A HREF=\"", htmRef, ".html\">", htmRef,
              "</A></TD><TD>", NULL), "", " ");
        } else {
          /* Include step number reference.  The idea is that this will
             help the user to recognized "important" (vs. early trivial
             logic) steps.  This prints a small pink statement number
             after the hypothesis statement label. */
          printLongLine(cat("<TR><TD>", htmStep, "</TD><TD>",
              htmHyp, "</TD><TD><A HREF=\"", htmRef, ".html\">", htmRef,
              "</A>",
              "<FONT FACE=\"Arial Narrow\" SIZE=-2 COLOR=\"#FF6666\"> ",
              str(hypStmtMap), "</FONT>",
              "</TD><TD>", NULL), "", " ");
        }
      }
      let(&htmStep, ""); /* Deallocate */
      let(&htmHyp, ""); /* Deallocate */
      let(&htmRef, ""); /* Deallocate */
    } /* strlen(sPrefix) */
  }
  let(&tex, ""); /* Deallocate */
  let(&sPrefix, ""); /* Deallocate */

  let(&lastTex, "");
  for (pos = 0; pos < nmbrLen(mathString); pos++) {
    tex = tokenToTex(mathToken[mathString[pos]].tokenName);
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
      /*if ((alphold && alphnew) || unknownold || (unknownnew && pos > 0)) {*/
      /* Put thin space only between letters and/or unknowns  11/3/94 */
      if ((alphold || unknownold) && (alphnew || unknownnew)) {
        /* Put additional thin space between two letters */
        let(&texLine, cat(texLine, "\\m{\\,", tex, "}", NULL));
      } else {
        let(&texLine, cat(texLine, "\\m{", tex, "}", NULL));
      }
    } else {
      let(&texLine, cat(texLine, tex, NULL));
    }
    let(&lastTex, ""); /* Deallocate */
    lastTex = tex; /* Pass deallocation responsibility for tex to lastTex */

  } /* Next pos */
  let(&lastTex, ""); /* Deallocate */
  if (!htmlFlag) {
    printLongLine(texLine, "", "\\");
    print2("\\endm\n");
  } else {
    printLongLine(cat(texLine, "</TD></TR>", NULL), "", " ");
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
      print2("<FONT SIZE=-2 FACE=ARIAL>Colors of variables:\n");
      printLongLine(cat(htmlVarColors, "</FONT><BR>", NULL), "", " ");
      print2("<BR><CENTER><FONT SIZE=-2 FACE=ARIAL>\n");
      /*
      print2("<A HREF=\"definitions.html\">Definition list</A> |\n");
      print2("<A HREF=\"theorems.html\">Theorem list</A><BR>\n");
      */
      print2("Copyright (GPL) 2000 Norman D. Megill <A \n");
      print2("HREF=\"mailto:nm@alum.mit.edu\">&lt;nm@alum.mit.edu></A>\n");
      print2("</FONT></CENTER>\n");
      print2("</BODY></HTML>\n");
    }
    outputToString = 0; /* Restore normal output */
    fprintf(texFilePtr, "%s", printString);
    let(&printString, "");
  }

} /* printTexTrailer */

