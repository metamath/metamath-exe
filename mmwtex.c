/*****************************************************************************/
/*               Copyright (C) 1998, NORMAN D. MEGILL                        */
/*****************************************************************************/

/*34567890123456 (79-character line to adjust text window width) 678901234567*/
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
#include "mmpars.h" /* For rawSourceError */
#include "mmwtex.h"
#include "mmcmdl.h" /* For texFileName */

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
char dollarSubst = '$'; /* Subst for $ in converting to old $l,$m,$n comments */
                      /* Use '$' instead of 2 for debugging */

flag readTexDefs(vstring tex_def_fn, flag promptForFile)
{

  FILE *tex_def_fp;
  char *fileBuf;
  long fileBufSize;
  char *fbPtr;
  long charCount;
  long i, j;
  long lineNum;
  long tokenLen;
  char zapChar;

  if (texDefsRead) bug(2301); /* Shouldn't read file twice */

  if (promptForFile) {
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

  /* See if last char is newline (needed for comment terminator); if not, add */
  if (fileBuf[charCount - 1] != '\n') {
    fileBuf[charCount] = '\n';
    fileBuf[charCount + 1] = 0;
    rawSourceError(fileBuf, &fileBuf[charCount - 1], 1, 0, tex_def_fn,cat(
        "The last line in the file does not end with a \"line break\"",
        " character.  The \"line break\" character is a line feed in Unix",
        " and a carriage return on the Macintosh.",NULL));
    charCount++;
  }

  /* Count the number of symbols defined */
  numSymbs = 0;
  fbPtr = fileBuf;
  lineNum = 1;
  while (1) {
    fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
    tokenLen = texDefTokenLen(fbPtr);
    if (!tokenLen) break; /* End of file */

    zapChar = fbPtr[tokenLen]; /* Character to restore after zapping source */
    fbPtr[tokenLen] = 0; /* Create end of string */
    if (strcmp(fbPtr, "define")) {
      lineNum = 1;
      for (i = 0; i < (fbPtr - fileBuf); i++) {
        if (fileBuf[i] == '\n') lineNum++;
      }
      rawSourceError(fileBuf, fbPtr, tokenLen, lineNum, tex_def_fn,
          "Expected the keyword \"define\" here.");
      free (fileBuf);
      return (0);
    }
    fbPtr[tokenLen] = zapChar;
    fbPtr = fbPtr + tokenLen;

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
    fbPtr = fbPtr + tokenLen;

    fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
    tokenLen = texDefTokenLen(fbPtr);
    zapChar = fbPtr[tokenLen]; /* Character to restore after zapping source */
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

    while (1) {

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
      fbPtr = fbPtr + tokenLen;

      fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
      tokenLen = texDefTokenLen(fbPtr);
      if (fbPtr[0] != '+' && fbPtr[0] != ';') {
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

      if (fbPtr[0] == ';') break;

      fbPtr = fbPtr + tokenLen;
    } /* End while */
    fbPtr = fbPtr + tokenLen;

    numSymbs++;
  } /* End while */

  if (fbPtr != fileBuf + charCount) bug(2305);

  print2("%ld symbol definitions were read from \"%s\".\n",
      numSymbs, tex_def_fn);

  texDefs = malloc(numSymbs * sizeof(struct texDef_struct));
  if (!texDefs) outOfMemory("#99 (TeX symbols)");

  /* Second scan:  (assumed to be error-free)  Assign the
     texDefs array */
  numSymbs = 0;
  fbPtr = fileBuf;
  while (1) {

    /* Skip "define" */
    fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
    tokenLen = texDefTokenLen(fbPtr);
    if (!tokenLen) break; /* End of file */
    fbPtr = fbPtr + tokenLen;

    /* Get ASCII token; note that leading and trailing quotes are omitted. */
    fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
    tokenLen = texDefTokenLen(fbPtr);
    zapChar = fbPtr[tokenLen - 1]; /* Char to restore after zapping source */
    fbPtr[tokenLen - 1] = 0; /* Create end of string */
    texDefs[numSymbs].tokenName = "";
    let(&texDefs[numSymbs].tokenName, fbPtr + 1);
    fbPtr[tokenLen - 1] = zapChar;
    fbPtr = fbPtr + tokenLen;

    /* Change double internal quotes to single quotes */
    j = strlen(texDefs[numSymbs].tokenName);
    for (i = 0; i < j - 1; i++) {
      if ((texDefs[numSymbs].tokenName[i] == '\"' &&
          texDefs[numSymbs].tokenName[i + 1] == '\"') ||
          (texDefs[numSymbs].tokenName[i] == '\'' &&
          texDefs[numSymbs].tokenName[i + 1] == '\'')) {
        let(&texDefs[numSymbs].tokenName, cat(left(texDefs[numSymbs].tokenName,
            i + 1), right(texDefs[numSymbs].tokenName, i + 3), NULL));
        j--;
      }
    }

    /* Skip "as" */
    fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
    tokenLen = texDefTokenLen(fbPtr);
    fbPtr = fbPtr + tokenLen;


    /* Initialize TeX equiv. */
    texDefs[numSymbs].texEquiv = "";

    while (1) {

      /* Append TeX equiv.; note leading and trailing quotes are omitted. */
      fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
      tokenLen = texDefTokenLen(fbPtr);
      zapChar = fbPtr[tokenLen - 1]; /* Char to restore after zapping source */
      fbPtr[tokenLen - 1] = 0; /* Create end of string */
      let(&texDefs[numSymbs].texEquiv, cat(texDefs[numSymbs].texEquiv,
          fbPtr + 1, NULL));
      fbPtr[tokenLen - 1] = zapChar;
      fbPtr = fbPtr + tokenLen;

      /* Skip "+" or ";" */
      fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
      tokenLen = texDefTokenLen(fbPtr);
      if (fbPtr[0] != '+' && fbPtr[0] != ';') bug(2306);

      if (fbPtr[0] == ';') break;
      fbPtr = fbPtr + tokenLen;
    } /* End while */
    fbPtr = fbPtr + tokenLen;


    /* Change double internal quotes to single quotes */
    j = strlen(texDefs[numSymbs].texEquiv);
    for (i = 0; i < j - 1; i++) {
      if ((texDefs[numSymbs].texEquiv[i] == '\"' &&
          texDefs[numSymbs].texEquiv[i + 1] == '\"') ||
          (texDefs[numSymbs].texEquiv[i] == '\'' &&
          texDefs[numSymbs].texEquiv[i + 1] == '\'')) {
        let(&texDefs[numSymbs].texEquiv, cat(left(texDefs[numSymbs].texEquiv,
            i + 1), right(texDefs[numSymbs].texEquiv, i + 3), NULL));
        j--;
      }
    }

    numSymbs++;
  } /* End while */


  /* Sort the labels for later lookup */
  qsort(texDefs, numSymbs, sizeof(struct texDef_struct), texSortCmp);

  /* Check for duplicate definitions */
  for (i = 1; i < numSymbs; i++) {
    if (!strcmp(texDefs[i].tokenName, texDefs[i - 1].tokenName)) {
      printLongLine(cat("?Error:  Token ", texDefs[i].tokenName,
          " is defined more than once.", NULL), "", " ");
    }
  }

  free(fileBuf);  /* Deallocate file source buffer */
  texDefsRead = 1;  /* Set flag that it's been read in */
  return (1);

}

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
      if (!ptr1) bug(2307);
      i = ptr1 - ptr + 1;
      continue;
    }
    if (tmpchr == '/') { /* Embedded c-style comment */
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
  return(NULL); /* Dummy return - never executed */
}


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
  return(NULL); /* Dummy return - never executed */
}

/* Token comparison for qsort */
int texSortCmp(const void *key1, const void *key2)
{
  /* Returns -1 if key1 < key2, 0 if equal, 1 if key1 > key2 */
  return (strcmp( ((struct texDef_struct *)key1)->tokenName,
      ((struct texDef_struct *)key2)->tokenName));
}


/* Token comparison for bsearch */
int texSrchCmp(/*const*/ void *key, /*const*/ void *data)
{
  /* Returns -1 if key < data, 0 if equal, 1 if key > data */
  return (strcmp(key,
      ((struct texDef_struct *)data)->tokenName));
}

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
      } /* End switch mtoken[i] */
    }

    if (k > 1) { /* Adjust iteration and length */
      i = i + k - 1;
      j = j + k - 1;
    }
  } /* Next i */

  return(ttstr);


}


/* Convert ascii token to TeX equivalent */
/* The "$" math delimiter is not placed around the returned arg. here */
/* *** Note: The caller must deallocate the returned string */
vstring tokenToTex(vstring mtoken)
{
  vstring tex = "";
  vstring tmpStr;
  long i, j, k;
  void *texDefsPtr; /* For binary search */

  texDefsPtr = (void *)bsearch(mtoken, texDefs, numSymbs,
      sizeof(struct texDef_struct), texSrchCmp);
  if (texDefsPtr) { /* Found it */
    let(&tex, ((struct texDef_struct *)texDefsPtr)->texEquiv);
  } else {
    /* If it wasn't found, use built-in conversion rules */
    let(&tex, mtoken);

    /* First, see if it's a tilde followed by a letter */
    /* If so, remove the tilde. */
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
}


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

}


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
}


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
      print2(
"\\documentstyle[leqno]{article}\n");
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
      print2(
"\\input amssym.def  %% Load in AMS fonts\n");
      print2(
"\\input amssym.tex  %% Load in AMS fonts\n");
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
    print2("<META NAME=\"GENERATOR\" CONTENT=\"Metamath\">\n");
    print2("%s\n", cat(" <TITLE>Quantum Logic Explorer - ",
        left(texFileName, instr(1, texFileName, ".htm") - 1),
        "</TITLE>", NULL));
    print2("<META HTML-EQUIV=\"Keywords\"\n");
    print2("CONTENT=\"Quantum Logic\"></HEAD>\n");
    /*print2("<BODY BGCOLOR=\"#D2FFFF\">\n");*/
    print2("<BODY BGCOLOR=\"#EEFFFA\">\n");
    print2("<A HREF=\"mmexplorer.html\">\n");
    print2("<IMG SRC=\"cowboy.gif\"\n");
    print2("ALT=\"[Image of cowboy]Home Page\"\n");
   print2("WIDTH=27 HEIGHT=32 ALIGN=LEFT><FONT SIZE=-2 FACE=ARIAL>Home\n");
    print2("</FONT></A><H1><CENTER><FONT COLOR=\"#006600\">Quantum\n");
    print2("Logic Explorer</FONT></CENTER></H1><HR>\n");
  } /* htmlFlag */
  fprintf(texFilePtr, "%s", printString);
  outputToString = 0;
  let(&printString, "");

}

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

  cmtptr = commentPtr;

  if (!texDefsRead) return; /* TeX defs were not read (error was detected) */

  /* Convert line to the old $m..$n and $l..$n formats (using dollarSubst
     instead of $) */
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
        } else {
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
      clen++;
      i++;
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
      if (mode == 0) displayMode = 1; /* No text after math mode text */
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
    printLongLine(outputLine, "", htmlFlag ? " " : "\\");
    let(&tmp, ""); /* Clear temporary allocation stack */

    if (lastLineFlag) break; /* Done */
  }

  print2("\n");
  outputToString = 0; /* Restore normal output */
  fprintf(texFilePtr, "%s", printString);
  let(&printString, "");
  let(&sourceLine, "");
  let(&outputLine, "");
  let(&cmt, "");


}


/* Warning:  contPrefix may not be temporarily allocated */
void printTexLongMath(nmbrString *mathString, vstring startPrefix,
    vstring contPrefix)
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
  let(&sPrefix, startPrefix); /* 7/3/98 Save it; it may be temp alloc */

  if (!texDefsRead) return; /* TeX defs were not read (error was detected) */
  outputToString = 1; /* Redirect print2 and printLongLine to printString */
  /*let(&printString, "");*/ /* May have stuff to be printed 7/4/98 */

  tex = asciiToTt(sPrefix); /* asciiToTt allocates; we must deallocate */
  if (!htmlFlag) {
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
        printLongLine(cat("<TR><TD>", htmStep, "</TD><TD>",
            htmHyp, "</TD><TD><A HREF=\"", htmRef, ".html\">", htmRef,
            "</A></TD><TD>", NULL), "", " ");
      }
      let(&htmStep, ""); /* Deallocate */
      let(&htmHyp, ""); /* Deallocate */
      let(&htmRef, ""); /* Deallocate */
    } /* strlen(sPrefix) */
  }
  let(&tex, ""); /* Deallocate */
  let(&sPrefix, ""); /* Deallocate */

  let(&texLine, "");
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
}

void printTexTrailer(flag texTrailerFlag) {

  if (texTrailerFlag) {
    outputToString = 1; /* Redirect print2 and printLongLine to printString */
    /*let(&printString, "");*/ /* May have stuff to be printed 7/4/98 */
    if (!htmlFlag) {
      print2("\\end{document}\n");
    } else {
      print2("<CENTER><FONT SIZE=-2 FACE=ARIAL>\n");
      print2("Colors of variables:\n");
      print2("<FONT COLOR=\"#993300\">wff</FONT><BR>\n");
      print2("<A HREF=\"definitions.html\">Definition list</A> |\n");
      print2("<A HREF=\"theorems.html\">Theorem list</A><BR>\n");
      print2("Copyright (c) 1998 Norman D. Megill <A \n");
      print2("HREF=\"mailto:nm@alum.mit.edu\">&lt;nm@alum.mit.edu></A>\n");
      print2("</FONT></CENTER>\n");
      print2("</BODY></HTML>\n");
    }
    outputToString = 0; /* Restore normal output */
    fprintf(texFilePtr, "%s", printString);
    let(&printString, "");
  }

}




/*???????????????OBSOLETE FROM HERE ON????????????????????????????????????*/
int errorCheck(vstring isString, vstring shouldBeString)
{
  /*???? THIS FUNCTION SHOULD BECOME OBSOLETE ???*/
  static int errorCount = 0;
  if (strcmp(isString,shouldBeString)) {
    printf("***ERROR*** Token found is %s, should be %s\n",
        isString,shouldBeString);
    errorCount++;
    if (errorCount > 18) {
      printf("***TOO MANY ERRORS***");
      return 1;
    }
  }
  return 0;
}


void parseGraphicsDef()
/* This functions reads lines from file inputDef_fp and fills in
   the arrays graTokenName, graTokenLength, and graTokenDef.
   The number of array entries is assigned to maxGraDef.
   The length of the longest graTokenName is assigned to maxGraTokenLength.
*/


{

  int tokenLen;
  char tokenBuffer[256];

  /* Initialize control variables for token()
      \ should be treated as a comment delimiter
   */
  vmc_lexControl.single=        ":+;";  /* Single character tokens */
  vmc_lexControl.space=                 " \t\n"         ;/* Tokens delimiters */
  vmc_lexControl.commentStart=          "!"             ;/* Start of comments */
  vmc_lexControl.commentEnd=            "\n"            ;/* End of comments */
  vmc_lexControl.enclosureStart=        "\"\'"          ;/* Start of special enclosure tokens */
  vmc_lexControl.enclosureEnd=          "\"\'"          ;/* End of special enclosure tokens  */

  while (1)     /* Process the definition file until EOF */
  {

    tokenLen=vmc_lexGetSymbol(tokenBuffer,inputDef_fp);
    /* We are done if no token is returned */
    if (!tokenLen) break;
    if (errorCheck(tokenBuffer,"GraphicsDef")) return;

    if (maxGraDef>=MAX_GRAPHIC_DEFS) {
      printf("***ERROR*** More than %d GraphicDefs",MAX_GRAPHIC_DEFS);
      return;
    }

    /* Initialize vstring arrays */
    graTokenName[maxGraDef] = ""; /* Initialize */
    graTokenDef[maxGraDef] = ""; /* Initialize */

    tokenLen=vmc_lexGetSymbol(tokenBuffer,inputDef_fp);
    /*let(&graTokenName[maxGraDef],tokenBuffer);*/

    /* Strip quotes from quoted token */
    let(&graTokenName[maxGraDef],right(left(tokenBuffer,len(tokenBuffer)-1),2));

    /* Assign token length to length with quotes removed */
    tokenLen = tokenLen-2;
    graTokenLength[maxGraDef]=tokenLen;

    /* Update maximum token length */
    if (maxGraTokenLength < tokenLen) maxGraTokenLength = tokenLen;

    tokenLen=vmc_lexGetSymbol(tokenBuffer,inputDef_fp);
    if (errorCheck(tokenBuffer,"denotes")) return;

    tokenLen=vmc_lexGetSymbol(tokenBuffer,inputDef_fp);
    /*** Strip quotes from quoted token ***/
    let(&graTokenDef[maxGraDef],right(left(tokenBuffer,len(tokenBuffer)-1),2));

    tokenLen=vmc_lexGetSymbol(tokenBuffer,inputDef_fp);

    /*** Add continuation lines ***/
    while (!strcmp(tokenBuffer,"+")) {

      tokenLen=vmc_lexGetSymbol(tokenBuffer,inputDef_fp);
      /* Strip quotes from quoted token */
      /* and concatenate to token buffer */
      let(&graTokenDef[maxGraDef],cat(graTokenDef[maxGraDef],
                right(left(tokenBuffer,len(tokenBuffer)-1),2),NULL));

      tokenLen=vmc_lexGetSymbol(tokenBuffer,inputDef_fp);
    }

    maxGraDef++;
    if (errorCheck(tokenBuffer,";")) return;
    vmc_clearTokenBuffer();

  }
}



void outputHeader()
{
  fprintf(tex_dict_fp, "\\input amssym.def\n");
  fprintf(tex_dict_fp, "\\input amssym.tex\n");
  return;
}

void outputTrailer()
{

    /* close the TeX format */
    fprintf(tex_dict_fp,"\\end\n");

  return;
}


void outputDefinitions()
{
/*???MUST BE REDONE */
  int m,c,i,j,k;
  vstring token="";
  vstring tmp="";

    c=4;        /* Number of columns */
    m=maxGraDef/c+1;    /* Number of lines */
    if ((m-1)*c == maxGraDef) m=m-1;
    printf("The output file has %d lines with %d columns each.\n",
        m,c);
    for (i=1; i<=m; i++) {
      for (j=0; j<c; j++) {

        if (i+j*m-1 >= maxGraDef) break;
        let(&token,graTokenName[i+j*m-1]);

        /* Put "\" in front of reserved TeX characters */
        for (k=0; k<len(token); k++) {
          /*???see conversion in outputTextString*/
          if (instr(1,"{}\\$%#~_^&",mid(token,k+1,1))) {

            if (token[k] == '\\') {
              let(&token,
                  cat(left(token,k),"\\backslash ",right(token,k+1),NULL));
              k = k + 11; /* Update loop counter */
            } else {
              let(&token,cat(left(token,k),"\\",right(token,k+1),NULL));
              k++; /* Update loop counter */
            }
          }
          let(&tmp,""); /*Free vstring allocation*/
        }

        /*??? tabs are wrong for TeX*/
        fprintf(tex_dict_fp, "%s\\tab %s\n",token,graTokenDef[i+j*m-1]);
        if (j<c-1) fprintf(tex_dict_fp,"\\tab \\tab ");
      } /* next j */
      fprintf(tex_dict_fp,"\\par \n");
    } /* next i */
  return;
}

vstring outputMathToken(vstring mathToken)
{
  int k;

  for (k=0; k<maxGraDef; k++) {
    if (!strcmp(mathToken,graTokenName[k])) break;
  }
  if (k==maxGraDef) {
    /* Token does not have a graphics definition; put it out as is */
    errorMessage("",0,0,0,cat(
    /*??? ADD ERROR LINE, ETC. HERE */
        "The symbol \"",mathToken,
        "\" does not have a TeX definition in \"",inputDef_fn,
        "\".\nIt will be output to as is to \"",tex_dict_fn,"\"",NULL),
        input_fn,0,(char)_notice);
    return (mathToken);
  } else {
    return (graTokenDef[k]);
  }
}

vstring outputTextString(vstring textString)
{
  vstring line="";
  vstring outputLine="";
  int i,p;

  long saveTempAllocStack;
  saveTempAllocStack = startTempAllocStack;
  startTempAllocStack = tempAllocStackTop; /* For let() stack cleanup */

  let(&line,textString);

    /* Convert reserved TeX characters */
    p = strlen(line);
    for (i = 0; i < p; i++) {
      switch (line[i]) {
        case ' ':
          /* Only convert second or later space unless first on line */
          if (i > 0) {
            if (line[i-1] == ' ') {
              let(&line,cat(left(line,i),"\\",right(line,i+1),NULL));
              i++; /* Update loop counter */
              p++;
            }
          } else {
            let(&line,cat(left(line,i),"\\",right(line,i+1),NULL));
            i++; /* Update loop counter */
            p++;
          }
          break;
        case '$':
        case '%':
        case '#':
        case '_':
        case '&':
          let(&line,cat(left(line,i),"\\",right(line,i+1),NULL));
          i++; /* Update loop counter */
          p++;
          break;
        case '{':
          let(&line,cat(left(line,i),"$\\{$",right(line,i+2),NULL));
          i = i + 3; /* Update loop counter */
          p = p + 3;
          break;
        case '}':
          let(&line,cat(left(line,i),"$\\}$",right(line,i+2),NULL));
          i = i + 3; /* Update loop counter */
          p = p + 3;
          break;
        case '\\':
          let(&line,cat(left(line,i),"$\\backslash$",right(line,i+2),NULL));
          i = i + 11; /* Update loop counter */
          p = p + 11;
          break;
        case '<':
          let(&line,cat(left(line,i),"$<$",right(line,i+2),NULL));
          i = i + 2; /* Update loop counter */
          p = p + 2;
          break;
        case '>':
          let(&line,cat(left(line,i),"$>$",right(line,i+2),NULL));
          i = i + 2; /* Update loop counter */
          p = p + 2;
          break;
        case '^':
          let(&line,cat(left(line,i),"$\\^{ }$",right(line,i+2),NULL));
          i = i + 4; /* Update loop counter */
          p = p + 4;
          break;
      }
    }

    /* !!!!Change tabs to "\tab " (for TeX) */
        /*??? tabs are wrong for TeX*/
    for (i=0; i<len(line); i++) {
      if (line[i] == '\t') {
        let(&line,cat(left(line,i),"\\tab ",right(line,i+2),NULL));
        i=i+4; /* Update loop counter */
      }
    }

    /* !!!!Change carriage-return to new paragraph for Word TeX */
    for (i=0; i<len(line); i++) {
      if (line[i] == '\n') {
        let(&line,cat(left(line,i),"\\par",right(line,i+1),NULL));
        i=i+4; /* Update loop counter */
      }
    }

    /* !!!!Output the line */
    /*
    while (len(line)>80) {
      p = 80;
      while (p > 1) {
        if (line[p-1] == '\\') break;
        p = p - 1;
      }
      if (p <= 1) p = len(line);
      / * A carriage return is treated as an extra space * /
      if (!strcmp(mid(line,p-2,2),"\\ ")) {
        let(&outputLine,cat(outputLine,left(line,p-3),"\n",NULL));
        let(&line,right(line,p));
        continue;
      }
      if (!strcmp(mid(line,p,2),"\\ ")) {
        let(&outputLine,cat(outputLine,left(line,p-1),"\n",NULL));
        let(&line,right(line,p+2));
        continue;
      }
      let(&outputLine,cat(outputLine,left(line,p-1),"\n",NULL));
      let(&line,right(line,p));
    }
    */
    let(&outputLine,cat(outputLine,line,NULL));
    let(&line,""); /* Free allocated space */
    /* ??? How do we free up temporary vstring allocation?  IN CALLING ROUTINE*/
    /*if (outputLine[0])
      let(&outputLine,cat("{\\tt",outputLine,"}","\n",NULL));*/

  startTempAllocStack = saveTempAllocStack;
  if (outputLine[0]) makeTempAlloc(outputLine); /* Flag it for deletion */
  return (outputLine);

}



void texOutputStatement(long statementNum,FILE *output_fp)
/* Print out the complete contents of a statement, including all white
   space and comments, from first token through all white space and
   comments after last token.  The output is printed to output_fp. */
/* This allows us to modify the input file with MetaMath and is also
   useful for debugging the getStatement() function. */
{
  vstring tmpStr = "";
  vstring tmpStr1 = "";
  vstring tmpStr3;
  vstring tmpStr4;
  /* Some frequently referenced entries in statement[] structure */
  char type; /* statement[statementNum].type */
  long j,k;

  type = statement[statementNum].type;

  if (statement[statementNum].labelName[0]) {
    /* There is an explicit label */

    /*??? Convert to cvtTexRToVString... call */
      let(&tmpStr1,statement[statementNum].labelName);
      k = strlen(tmpStr1);
      /* Scan for characters that must be converted */
      /* (Only digits, letters, *, ., -, and _ allowed in labels) */
      for (j = 0; j < k; j++) {
        switch (tmpStr1[j]) {
          case '_':
            let(&tmpStr1,cat(left(tmpStr1,j),"\\",right(tmpStr1,j+1),NULL));
            j++; /* Update loop counter */
            break;
        }
      }

    tmpStr4 = outputTextString( ""/*statement[statementNum].whiteSpaceL*/);
    let(&tmpStr,cat("{\\tt ",tmpStr1,
        "}",tmpStr4,NULL));
  }

  switch (type) {
    case (char)lb_: /* ${ */
    case (char)rb_: /* $} */
      tmpStr4 = outputTextString( /* Don't use let to free up alloc */
          cat(tmpStr,"{\\tt\\bf\\","$",chr(statement[statementNum].type),"}",
          ""/*statement[statementNum].whiteSpaceS*/,NULL));
      let(&tmpStr,cat(tmpStr,tmpStr4,NULL));
      break;
    case (char)v__: /* $v */
    case (char)c__: /* $c */
    case (char)d__: /* $d */
    case (char)e__: /* $e */
    case (char)f__: /* $f */
    case (char)a__: /* $a */
      tmpStr4 = outputTextString(
          cat("{\\tt\\bf\\","$",chr(statement[statementNum].type),"}",
          ""/*statement[statementNum].whiteSpaceS*/,NULL));
      let(&tmpStr,cat(tmpStr,tmpStr4,
          cvtTexMToVString(statement[statementNum].mathString),NULL));
      tmpStr4 = outputTextString(
             ""/*statement[statementNum].whiteSpaceSc*/);
      let(&tmpStr,cat(tmpStr,"{\\tt\\bf\\$.}",tmpStr4,NULL));
      break;
    case (char)p__: /* $p */
      tmpStr4 = outputTextString(cat(
          "$",chr(statement[statementNum].type),
        ""/*statement[statementNum].whiteSpaceS*/,NULL));
      tmpStr3 = cvtTexMToVString(statement[statementNum].mathString);
      let(&tmpStr,cat(tmpStr,tmpStr4,
        tmpStr3,NULL));
      tmpStr4 = outputTextString(cat("{\\tt\\bf\\$=}",
          ""/*statement[statementNum].whiteSpaceEq*/,NULL));
      let(&tmpStr,cat(tmpStr,tmpStr4,
          cvtTexRToVString(statement[statementNum].proofString),NULL));
      tmpStr4 = outputTextString(cat(
          "{\\tt\\bf\\$.}",""/*statement[statementNum].whiteSpaceSc*/,NULL));
      let(&tmpStr,cat(tmpStr,tmpStr4,NULL));
      break;
    case (char)illegal_: /* anything else */
      tmpStr4 = outputTextString(cat(
        "$",chr(statement[statementNum].type),
        ""/*statement[statementNum].whiteSpaceS*/,NULL));
      let(&tmpStr,cat(tmpStr,tmpStr4,NULL));
      tmpStr4 = outputTextString(cat(
          "{\\tt\\bf\\$.}",""/*statement[statementNum].whiteSpaceSc*/,NULL));
      let(&tmpStr,cat(tmpStr,tmpStr4,NULL));
      break;
  }

  fprintf(output_fp,tmpStr);
  let (&tmpStr,"");
  let (&tmpStr1,"");
  return;
}


/* Converts nmbrString to a vstring with correct white space between tokens */
/* Put actual white space between tokens */
vstring cvtTexMToVString(nmbrString *s)
{
  long i;
  vstring tmpStr = "";
  vstring tmpStr2 = "";

  long saveTempAllocStack;
  saveTempAllocStack = startTempAllocStack;
  startTempAllocStack = tempAllocStackTop; /* For let() stack cleanup */

  for (i = 1; i <= nmbrLen(s); i++) {
    let(&tmpStr2,outputMathToken(mathToken[s[i-1]].tokenName));
    /* Add space after backslashed tokens */
    if (tmpStr2[0] == '\\') let(&tmpStr2,cat(tmpStr2," ",NULL));
    let(&tmpStr,cat(tmpStr,
        tmpStr2,NULL));
    let(&tmpStr2,""/*s[i-1].whiteSpace*/);
    if (i == nmbrLen(s)) {
      let(&tmpStr,cat(tmpStr,"$",outputTextString(tmpStr2),NULL));
    } else {
      /* Ignore first space, if any, after token */
      if (strlen(tmpStr2)) {
        if (tmpStr2[0] == ' ') {
          let(&tmpStr2,right(tmpStr2,2));
        }
      }
      if (strlen(tmpStr2)) {
        let(&tmpStr,cat(tmpStr,"$",outputTextString(tmpStr2),"$",NULL));
      }
    }
  }
  let(&tmpStr,cat(" $",tmpStr,NULL));
  let(&tmpStr2,"");

  startTempAllocStack = saveTempAllocStack;
  if (tmpStr[0]) makeTempAlloc(tmpStr); /* Flag it for deletion */
  return (tmpStr);
}

/* Converts rString to a vstring with correct white space between tokens */
/* Put actual white space between tokens */
vstring cvtTexRToVString(nmbrString *s)
{
  long i,j,k;
  vstring tmpStr = "";
  vstring tmpStr1 = "";

  long saveTempAllocStack;
  saveTempAllocStack = startTempAllocStack;
  startTempAllocStack = tempAllocStackTop; /* For let() stack cleanup */

  for (i = 1; i <= nmbrLen(s); i++) {
    if (s[i-1] >= 0) {
      /* A label */
      let(&tmpStr1,statement[s[i-1]].labelName);
      k = strlen(tmpStr1);
      /* Scan for characters that must be converted */
      /* (Only digits, letters, *, ., -, and _ allowed in labels) */
      for (j = 0; j < k; j++) {
        switch (tmpStr1[j]) {
          case '_':
            let(&tmpStr1,cat(left(tmpStr1,j),"\\",right(tmpStr1,j+1),NULL));
            j++; /* Update loop counter */
            break;
        }
      }
      let(&tmpStr,cat(tmpStr,"{\\tt ",tmpStr1,
          "}",NULL));
    } else {
      if (s[i-1] < -2) {
        /* A 1-character keyword */
        let(&tmpStr,cat(tmpStr,chr(-s[i-1]),NULL));
      } else {
        /* -2, which means an illegal token */
        let(&tmpStr,cat(tmpStr,"?",NULL));
      }
    }
    let(&tmpStr,cat(tmpStr,outputTextString(""/*s[i-1].whiteSpace*/),NULL));
  }
  let(&tmpStr1,"");

  startTempAllocStack = saveTempAllocStack;
  if (tmpStr[0]) makeTempAlloc(tmpStr); /* Flag it for deletion */
  return (tmpStr);
}


