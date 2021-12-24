/*****************************************************************************/
/*        Copyright (C) 2017  Thierry Arnoux                                 */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* This module processes structured typesetting output. */

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
#include "mmcmdl.h" /* For texFileName */
#include "mmwsts.h"
#include "mmwtex.h" /* For texDefsRead */
#include "mmhtbl.h" /* For caching results */

#define STS_MAX_TYPES 10		/* basic types. For set.mm, there are 4 (|-,wff,class,set) */
#define STS_MAX_RECURSION 50	/* max depth of a formula (heuristic to avoid loops) */
#define STS_MAX_SCHEME_VAR 20	/* max variables per schemes */
#define STS_MAX_TOKEN_REUSE 20
#define STS_MAX_LEN_FOR_CACHE 20 /* put only formulas of up to 20 tokens into cache */
#define STS_CACHE_BUCKETS 50000	 /* size of a cache hash bucket */

/* The structure containing the structured typesetting replacement schemes */
struct stsScheme_struct {
  char type; /* 'i' for identifier, 's' for scheme. */
  nmbrString *scheme; /* The scheme */
  vstring typesetting; /* The typesetting to be displayed */
  int varCount; /* The number of variables in the scheme */
};

/* The structure containing information about the STS variable tokens */
struct stsVar_struct {
  long stsType; /* type of the token in the STS (must be a constant tokenId) */
  long stsSchemeId; /* number of the schemed in which this variable is defined + 1. */
  vstring anchor; /* anchor for the STS, or "" if not allocated yet */
  flag isTerminal; /* whether or not this is one of the terminal type constants */
};

/* Current output format for STS */
vstring stsFormat = "";

/* STS header to be added to the HTML HEAD element of each generated page */
vstring stsHeader = "";

/* Prefix/Suffix for the output of each formula when in-text (in comments) */
vstring stsInText[2] = { "", "" };

/* Prefix/Suffix for the output of each formula when displayed (in proofs) */
vstring stsDisplayed[2] = { "", "" };

/* The pointer containing the structured typesetting replacement schemes */
struct stsScheme_struct *stsScheme = NULL;
long stsSchemes = 0;

/* The pointer containing the structured typesetting replacement schemes */
struct stsVar_struct *stsVar = NULL;
long stsVars = 0;

/* The basic types used for parsing tokens or comments */
nmbrString *stsTerminalTypes = NULL_NMBRSTRING;

/* debug trace levels for STS */
flag dbs1 = 0; /* shows top-level calls to STS printing */
flag dbs3 = 0; /* shows substitution results */
flag dbs5 = 0; /* shows all substitutions tried */
flag dbs7 = 0; /* shows details of substitution "unification" */
flag dbs9 = 0; /* shows details of substring searches in "unification" */
flag stsUseCache = 0; /* store generation results into cache */

/**/
void debugOn() {
  dbs1 = 1; /* shows top-level calls to STS printing */
  dbs3 = 1; /* shows substitution results */
  dbs5 = 1; /* shows all substitutions tried */
  dbs7 = 0; /* shows details of substitution "unification" */
  dbs9 = 0; /* shows details of substring searches in "unification" */
}

/**/
void debugOff() {
  dbs1 = 0; /* shows top-level calls to STS printing */
  dbs3 = 0; /* shows substitution results */
  dbs5 = 0; /* shows all substitutions tried */
  dbs7 = 0; /* shows details of substitution "unification" */
  dbs9 = 0; /* shows details of substring searches in "unification" */
}


vstring stsChunkName(char c) {
  switch(c) {
  case 'i': return "identifier";
  case 's': return "scheme";
  case 'd': return "display code";
  case 't': return "text code";
  case 'h': return "header code";
  case 'c': return "command line";
  case 'u': return "terminal types";
  default: return "unknown chunk";
  }
}

/* Cache to speed up conversions */
hashtable stsCache;

/* Math symbol comparison for bsearch */
/* Here, key is pointer to a character string. */
/* Here we search only the global tokens, those
 * which endStatement is the last statement */
int mathSrchGlbCmp(const void *key, const void *data)
{
  /* Returns -1 if key < data, 1 if key > data */
  int ret = strcmp(key, mathToken[ *((long *)data) ].tokenName);
  if(ret != 0) return ret;

  /* We have found two identical entries.
   * However we shall return 0 (match) only if the token is global. */
  if(mathToken[ *((long *)data) ].endStatement == statements) return 0;

  /* Find the direction in which the target token is */
  for(long *ptr = (long*)data; !strcmp(key, mathToken[ *ptr ].tokenName); ptr++)
    if(mathToken[ *ptr ].endStatement == statements) return 1;
  return -1;
}

/* Similar to mmpars's parseMathTokens, this is a general-purpose
 * function to parse a math token string. However this version does
 * not allow to skip spaces. */
nmbrString *parseMathStrict(vstring text)
{
  /* The result */
  nmbrString *mathString = NULL_NMBRSTRING;
  long mathStringLen = 0;

  /* Temporary working space */
  long wrkLen;
  nmbrString *wrkNmbrPtr;

  long i;
  char *fbPtr;
  long textLen, tokenLen_;
  long *mathKeyPtr; /* bsearch returned value */

  /* Make sure that mathTokens has been initialized */
  if (!mathTokens) bug(1717);

  textLen = (long)strlen(text);

  /* Initialize temporary working space for parsing tokens */
  /* Assume the worst case of one token per 2 text characters */
  wrkLen = textLen/2+1;
  wrkNmbrPtr = malloc((size_t)wrkLen * sizeof(nmbrString));
  if (!wrkNmbrPtr) outOfMemory("#502 (wrkNmbrPtr)");

  /* Scan the math section for tokens */
  fbPtr = text;
  while (1) {
    fbPtr = fbPtr + whiteSpaceLen(fbPtr);
    tokenLen_ = tokenLen(fbPtr);
    if (!tokenLen_) break; /* Done scanning source line */

    fbPtr[tokenLen_] = 0; /* End token - There's no come back */
    //printf("Searching for %s\n", fbPtr);
    mathKeyPtr = (long *)bsearch(fbPtr, mathKey,
	(size_t)mathTokens, sizeof(long), mathSrchGlbCmp);
    if (!mathKeyPtr) {
      /* Unknown token, escape. */
      print2("?Unknown token %s\n", fbPtr);
      free(wrkNmbrPtr);
      return NULL_NMBRSTRING;
    }
    wrkNmbrPtr[mathStringLen] = *mathKeyPtr;
    mathStringLen++;
    fbPtr = fbPtr + tokenLen_ + 1; /* Move on to next token */
    if(fbPtr >= text + textLen) break;
  }

  /* Assign mathString */
  nmbrLet(&mathString, nmbrSpace(mathStringLen));
  for (i = 0; i < mathStringLen; i++)
    mathString[i] = wrkNmbrPtr[i];
  if (mathStringLen) nmbrMakeTempAlloc(mathString); /* Flag for dealloc*/
  free(wrkNmbrPtr);
  return mathString;
}

/* Store a couple key/object into the cache */
void stsStoreCache(nmbrString **tKey, vstring *tObject, nmbrString *sKey, vstring sObject) {
  *tKey = NULL_NMBRSTRING;
  nmbrLet(tKey, sKey);

  *tObject = "";
  let(tObject, sObject);
}

/* Free a couple key/object from the cache */
void stsFreeCache(nmbrString **key, vstring *object) {
  nmbrLet(key, NULL_NMBRSTRING);
  let(object, NULL);
}

/* Dump a couple key/object from the cache */
void stsDumpCache(nmbrString *key, vstring object) {
//  printf("- %s -> %s\n", nmbrCvtMToVString(key), object);
  printf("- (%d) %s [%s]\n", nmbrHash(key), nmbrCvtMToVString(key), nmbrCvtAnyToVString(key));
}

/* Parse a file containing the structured typesetting rules. */
int parsetSTSRules(vstring format) {
  vstring inputFn = "";
  vstring chunk = "";
  char chunkType = 0;
  flag ots = outputToString;
  outputToString = 0;

  /* If the same format was already parsed, nothing to do. */
  if(strcmp(stsFormat, format) == 0) {
    return 0;
  }
  /* Format is invalid until fully parsed. */
  let(&stsFormat, "");

  /* Create a cache for exported statements to speed up */
  if(stsUseCache) {
    stsCache = htcreate(format, STS_CACHE_BUCKETS, "", (hashFunc*)&nmbrHash, (eqFunc*)&nmbrEq,
			(letFunc*)&stsStoreCache, (freeFunc*)&stsFreeCache,
			(eqFunc*)&stsDumpCache);
  }

  /* Build the name of the MMTS file to load */
  let(&inputFn, cat(rootDirectory ,
      left(input_fn, instr(1,input_fn,".mm")-1), "-", format, ".mmts", NULL));

  FILE *sts_fp = fSafeOpen(inputFn, "r", 1);
  if(!sts_fp) return 0; /* Error message is provided by fSafeOpen */

  /* Determine file size */
  #ifndef SEEK_END
  /* An older GCC compiler didn't have this ANSI standard constant defined. */
  #define SEEK_END 2
  #endif
  if (fseek(sts_fp, 0, SEEK_END)) bug(5701);
  long fileBufSize = ftell(sts_fp);
  /*E*/if(db5)print2("In binary mode the file has %ld bytes.\n",fileBufSize);

  /* Allocate space for the entire input file */
  fileBufSize = fileBufSize + 10; /* Add a factor for unknown text formats */
  char *fileBuf = malloc((size_t)fileBufSize);
  if (!fileBuf) outOfMemory("#501 (fileBuf)");

  if (fseek(sts_fp, 0, SEEK_SET)) bug(5702);
  /* Put the entire input file into the buffer as a giant character string */
  long charCount = (long)fread(fileBuf, sizeof(char), (size_t)fileBufSize - 2,
			       sts_fp);
  if (!feof(sts_fp)) {
    print2("Note:  This bug will occur if there is a disk file read error.\n");
    /* If this bug occurs (due to obscure future format such as compressed
       text files) we'll have to add a realloc statement. */
    bug(5703);
  }
  fclose(sts_fp);
  fileBuf[charCount] = 0; /* End of string */

  /* Initialize the STS Schemes list */
  stsScheme = malloc((size_t)MAX_MATHTOKENS * sizeof(struct stsScheme_struct));
  if (!stsScheme) {
    print2("*** FATAL ***  Could not allocate stsScheme space\n");
    bug(5001);
    }
  stsSchemes = 0;

  /* Initialize and clear the STS Variable list */
  stsVar = calloc((size_t)MAX_MATHTOKENS, sizeof(struct stsVar_struct));
  if (!stsVar) {
    print2("*** FATAL ***  Could not allocate stsVar space\n");
    bug(5002);
    }

  /* Actual parsing starts */
  print2("Reading structured typesetting from \"%s\"...\n", inputFn);

  /* Look for $[ and $] 'include' statement start and end */
  char *fbPtr = fileBuf;
  char *startPtr = NULL;
  long lineNum = 1;
  flag insideComment = 0; /* 1 = inside $( $) comment */
  flag insideChunk = 0;  /* 1 = inside $s $: scheme */
  flag insideTypeSet = 0; /* 1 = inside $: $. typeset */
  while (1) {
    /* Find a keyword or newline */
    char tmpch = fbPtr[0];
    if (!tmpch) { /* End of file */
      if (insideComment) {
        rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum - 1, inputFn,*/
         "The last comment in the file is incomplete.  \"$)\" was expected.");
	if (insideChunk) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              cat("The last ", stsChunkName(chunkType),
		  " in the file is incomplete.", NULL));
        }
        if (insideTypeSet) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              "The last typesetting in the file is incomplete.");
        }
      }
      break;
    }
    if (tmpch != '$') {
      if (tmpch == '\n') {
        lineNum++;
      } else {
          if (!isgraph((unsigned char)tmpch) && !isspace((unsigned char)tmpch)) {
            rawSourceError(fileBuf, fbPtr, 1, /*lineNum, inputFn,*/
                cat("Illegal character (ASCII code ",
                str((double)((unsigned char)tmpch)),
                " decimal).",NULL));
        }
      }
      fbPtr++;
      continue;
    }

    /* Detect missing whitespace around keywords (per current Metamath
     * language spec) */
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
    fbPtr++;
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

      case 'd': /* Start of display code */
      case 't': /* Start of text code */
      case 'h': /* Start of header code */
      case 'c': /* Start of command line */
      case 'i': /* Start of identifier */
      case 's': /* Start of scheme */
      case 'u': /* Start of terminal type */
	if (insideComment) continue;
	if (insideChunk) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              cat("A scheme start was found within a ",
		  stsChunkName(chunkType), ".", NULL));
        }
        if (insideTypeSet) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              cat("A ",stsChunkName(fbPtr[0]),
		  " start was found within a typeset.", NULL));
        }
        chunkType = fbPtr[0];
        startPtr = fbPtr + 1;
        insideChunk = 1;
	continue;

      case ':': /* Start of typesetting */
	if (insideComment) continue;
        if (!insideChunk || (chunkType != 's' && chunkType != 'i')) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              "A scheme end was found outside of a scheme.");
        }
        if (insideTypeSet) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              "A scheme end was found within a typeset.");
        }
        vstring text = mid(startPtr, 0, fbPtr - startPtr - 1);
        //print2("end scheme line %ld! %s\n", lineNum, text);
        stsScheme[stsSchemes].type = chunkType;
        stsScheme[stsSchemes].scheme = NULL_NMBRSTRING;
	// parseMathTokens is too permissive. Wrote own token parser.
        //nmbrLet(&(stsScheme[stsSchemes].scheme), parseMathTokens(text, statements));
        nmbrLet(&(stsScheme[stsSchemes].scheme), parseMathStrict(text));

        int l = nmbrLen(stsScheme[stsSchemes].scheme);
        if(l < 2) {
          if(l == 0)
	    rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
		"Empty scheme or unknown token.");
          else
	    rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
		"A scheme shall contain at least 2 tokens.");
          chunkType = '$'; // invalid chunk type, so that the scheme is skipped.
          startPtr = fbPtr + 1;
          insideChunk = 0;
	  insideTypeSet = 1;
	  stsSchemes--;
	  continue;
          }

        /* Store the number of variables in the scheme (each shall be different)  */
        stsScheme[stsSchemes].varCount = 0;
        for(int i=0;i<l;i++)
          if(mathToken[stsScheme[stsSchemes].scheme[i]].tokenType == var_)
            stsScheme[stsSchemes].varCount++;

        /* Store the variable "type" (wff, set or class for set.mm)  */
        if(chunkType == 'i') {
          if(l != 2)
            rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
                "An identifier definition shall contain exactly 2 tokens.");
          long index = stsScheme[stsSchemes].scheme[1];
          stsVar[index].stsType = stsScheme[stsSchemes].scheme[0];
          stsVar[index].stsSchemeId = stsSchemes + 1;
          stsVar[index].anchor = "";

          if(mathToken[stsVar[index].stsType].tokenType != con_)
            rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
                "An identifier definition shall start with a constant.");
          if(mathToken[index].tokenType != var_)
            rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
                "An identifier definition shall end with a variable.");
	    //print2("Token %s has type %s\n", mathToken[index].tokenName, mathToken[stsVar[index].stsType].tokenName);
        }

        startPtr = fbPtr + 1;
        insideChunk = 0;
	insideTypeSet = 1;
	continue;

      case '.': /* End of chunk/typesetting */
	if (insideComment) continue;
	if (!insideChunk && !insideTypeSet) {
          rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
              "A chunk/typeset end was found outside a chunk or typeset.");
	}
	insideChunk = 0;
	switch(chunkType) {
	  case 'i':
	  case 's':
	    /* End of typesetting, record the new scheme. */
	    if(!insideTypeSet) {
	      rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
		  cat("A typeset end was found within a ",
		      stsChunkName(chunkType), ".", NULL));
	    }
	    stsScheme[stsSchemes].typesetting = "";
	    let(&(stsScheme[stsSchemes].typesetting), mid(startPtr, 2, fbPtr - startPtr - 3));
	    /* Record the schemeId */
	    if(nmbrLen(stsScheme[stsSchemes].scheme) == 2
	       && stsVar[stsScheme[stsSchemes].scheme[1]].stsSchemeId == 0)
		stsVar[stsScheme[stsSchemes].scheme[1]].stsSchemeId = stsSchemes + 1;
//	    for(int i=1;i<=nmbrLen(stsScheme[stsSchemes].scheme);i++)
//	      if(stsVar[stsScheme[stsSchemes].scheme[i]].stsSchemeId == 0)
//		stsVar[stsScheme[stsSchemes].scheme[i]].stsSchemeId = stsSchemes + 1;
	    stsSchemes++;
	    insideTypeSet = 0;
	    continue;

	  case 'd':
	  case 't':
	    /* End of the display/text chunk */
	    let(&chunk, mid(startPtr, 2, fbPtr - startPtr - 3));
	    long pos = instr(1, chunk, "###");
	    if(pos == 0) {
	      rawSourceError(fileBuf, fbPtr - 1, 2, /*lineNum, inputFn,*/
		  "Display/text chunk shall contain ###, which will be replaced"
		  " by the generated formula in the generated HTML file.");
	      let(&chunk, "");
	      continue;
	    }
	    long end = pos + 3;
	    vstring *target;
	    if(chunkType == 'd') target = stsDisplayed;
	    if(chunkType == 't') target = stsInText;
	    let(&(target[0]), left(chunk, pos-1));
	    let(&(target[1]), mid(chunk, end, len(chunk)-end+1));
	    let(&chunk, "");
	    continue;

	  case 'h':
	    /* End of the header chunk */
	    let(&stsHeader, mid(startPtr, 2, fbPtr - startPtr - 3));
	    continue;

	  case 'c':
	    /* End of the command line chunk */
	    let(&postProcess, mid(startPtr, 2, fbPtr - startPtr - 3));
	    continue;

	  case 'u':
	    /* End of terminal types list chunk */
	    {
	      vstring text = mid(startPtr, 2, fbPtr - startPtr - 3);
	      nmbrLet(&stsTerminalTypes, parseMathStrict(text));
	      for(int i=0;i<nmbrLen(stsTerminalTypes);i++) stsVar[stsTerminalTypes[i]].isTerminal = 1;
	      continue;
	    }

	  case '$':
	    /* Error handling for incorrect schemes */
	    insideTypeSet = 0;
	    continue;
	}
    }
  }

  print2("%ld typesetting statements were read from \"%s\".\n", stsSchemes, inputFn);
  /* Format is invalid */
  let(&stsFormat, format);

  if (!errorCount) {
    print2("No errors were found.\n");
  } else {
    if (errorCount == 1) {
      print2("One error was found.\n");
    } else {
      print2("%ld errors were found.\n", (long)errorCount);
    }
  }

  /* Free vstrings and memory */
  let(&inputFn, "");
  free(fileBuf);

  /* Return back to outputting in a string */
  outputToString = ots;

  texDefsRead = 1;
  return 1;
}

/* initialize a table with the brackets (of set.mm) */
char *bracketsOpnStr = "( { [ [. [_ <.";
char *bracketsClsStr = ") } ] ]. ]_ >.";
nmbrString *bracketsOpn = NULL_NMBRSTRING, *bracketsCls = NULL_NMBRSTRING;

/* Returns whether or not a nmbrString is balanced in terms of brackets */
flag nmbrBalanced(nmbrString *str, long start, long len) {
  /* TODO we could use the global bracketMatchOn to detect if we are in set.mm */
  /*if(!bracketMatchOn) return 1;*/
  if(bracketsOpn == NULL_NMBRSTRING) {
    nmbrLet(&bracketsOpn, parseMathTokens(bracketsOpnStr, statements));
    nmbrLet(&bracketsCls, parseMathTokens(bracketsClsStr, statements));
  }
  long brackets = nmbrLen(bracketsOpn);
  for(int i = 0;i<brackets; i++) {
    long open = bracketsOpn[i];
    long close = bracketsCls[i];
    long pairingMismatches = 0; /* Counter of parens: + for "(" and - for ")" */
    for(int j=0; j<len; j++) {
      if(str[start-1+j] == open) pairingMismatches ++;
      if(str[start-1+j] == close) pairingMismatches --;
    }
    if(pairingMismatches != 0) {
      if(dbs7) print2("Bracket mismatch for %s%s in %s (mismatch=%ld)\n",
		      mathToken[open].tokenName,
		      mathToken[close].tokenName,
		      nmbrCvtMToVString(nmbrMid(str, start, len)),
		      pairingMismatches);
      return 0;
    }
  }
  return 1;
}

/* Buffer the anchors for the variables - this trades a bit of static memory for a bit of processing speed */
vstring getAnchor(long tokenId) {
  vstring anchor = stsVar[tokenId].anchor;
  if(strlen(anchor) == 0 ) {
    let(&anchor, cat("#", mathToken[tokenId].tokenName, "#", NULL));
    stsVar[tokenId].anchor = anchor;
  }
  return anchor;
}

/* Attempts to unify the given mathString (taken as constant) with the
 * scheme (expected to contain variables).
 * varToken, varStart, varLen and state are assumed to be preallocated
 * with enough space for all variables (stsScheme[i].varCount).
 * If the unification is successful,
 * - varPos will contain an array with the position of each variable in the scheme
 * - varStart will contain an array with the start of each variable substitution within mathString
 * - varLen will contain an array with the length of each variable substitution within mathString
 * - state will contain an array of (number of variables - 1) elements with the indices of the next substitution, if any.
 * Return value:
 * - 0 if the unification fails,
 * - 1 if the unification is successful and there are no more possible unifications
 * - 3 if the unification is successful and there are possibly more unifcations
 */
/* Note that variables must be assigned at least one token. */
flag unifySts(nmbrString *mathString, nmbrString *scheme, nmbrString *varPos, nmbrString *varStart, nmbrString *varLen, nmbrString *state) {
  int varCount = 0;
  int lastVarNextOcc = -1; /* The last variable with a next possible occurrence, to prepare the next state */
  long lenString = nmbrLen(mathString);
  long lenScheme = nmbrLen(scheme);
  flag balanced; /* Whether or not the last substitution found was bracket-balanced */

  /* Pruning : skip if shorter than scheme */
  if(lenString < lenScheme) return 0;

  /* Count the number of variables in the scheme, and note their positions */
  for(int i=0;i<lenScheme;i++)
    if(mathToken[scheme[i]].tokenType == var_)
      varPos[varCount++] = i+1;
  if(varCount > STS_MAX_SCHEME_VAR) bug(5003);

  /* Repeat until we found a bracket-balanced variable substitution */
  do {
    lastVarNextOcc = -1;
    /* Handle separately the trivial cases where there are no or one variable. */
    switch(varCount) {
      case 0:
	/* No variable : match the two strings */
	return nmbrEq(scheme, mathString);

//      case 1:
//	/* One variable: match the start and end, assign the rest */
//	varStart[0] = varPos[0];
//	varLen[0] = lenString - lenScheme + 1;
//	if(!nmbrSubEq(scheme, 1, mathString, 1, varStart[0]-1)) return 0;
//	if(!nmbrSubEq(scheme, varPos[0]+1, mathString, varPos[0] + varLen[0], lenScheme - varPos[0])) return 0;
//	if(dbs7) print2("Found unification for %s : %s (start=%ld, len=%ld)\n",mathToken[scheme[varPos[0]-1]].tokenName, nmbrCvtMToVString(nmbrMid(mathString, varStart[0], varLen[0])), varStart[0], varLen[0]);
//	return 1;

      default:
	/* Two variables or more: first match the prefix and the suffix */
	varStart[0] = varPos[0];
	if(!nmbrSubEq(scheme, 1, mathString, 1, varStart[0]-1)) return 0;
	if(!nmbrSubEq(scheme, varPos[varCount-1]+1, mathString, lenString - (lenScheme - varPos[varCount-1] - 1), lenScheme - varPos[varCount-1])) return 0;

	/* Then, for each additional variable, */
	for(int i=0;i<varCount-1;i++) {
	  if(state[i] < 1) state[i] = 1;
	  long conLength = varPos[i+1]-varPos[i]-1;
	  if(dbs9) print2("Searching %ld's occurrence of %s in %s starting from %ld\n", state[i], nmbrCvtMToVString(nmbrMid(scheme, varPos[i]+1, conLength)), nmbrCvtMToVString(mathString), varStart[i]+1);
	  long position = nmbrInstrN(varStart[i]+1, state[i], mathString, scheme, varPos[i]+1, conLength);
	  if(dbs9) print2("Found pos %ld\n", position);
	  if(position == 0) return 0;
	  // note if there is a next occurrence
	  if(nmbrInstrN(varStart[i]+1, state[i]+1, mathString, scheme, varPos[i]+1, conLength) > 0)
	    lastVarNextOcc = i;
	  varLen[i] = position - varStart[i];
	  varStart[i+1] = position + conLength;
	}
	varLen[varCount-1] = lenString - (lenScheme - varPos[varCount-1]) - varStart[varCount-1] + 1;
	if(dbs7) for(int i=0;i<varCount;i++) print2("Found unification for %s : %s (start=%ld, len=%ld)\n",mathToken[scheme[varPos[i]-1]].tokenName, nmbrCvtMToVString(nmbrMid(mathString, varStart[i], varLen[i])), varStart[i], varLen[i]);
	break;
    }

    /* Check that brackets are balanced in all substitutions */
    balanced = 1;
    for(int i = 0; i<varCount; i++)
      balanced &= nmbrBalanced(mathString, varStart[i], varLen[i]);

    /* Compute next state */
    if(lastVarNextOcc >= 0) {
      state[lastVarNextOcc]++;
      for(int i=lastVarNextOcc+1;i<varCount-1;i++)
        state[i] = 1;
        if(dbs7) print2("Next state is %s\n",nmbrCvtAnyToVString(state));
    }
  }
  while(lastVarNextOcc >= 0 && !balanced);
  if(!balanced) return 0; /* Failure, no substitution found was balanced */
  if(lastVarNextOcc >= 0) return 3; /* Success, with other possible substitutions */
  return 1; /* Success */
}


/* Builds a structured typesetting output, for the math string (hypothesis or
   conclusion) that is passed in.
   Returns 1 if successful, 0 otherwise. */
/* Warning: The caller must deallocate the returned vstring. */
flag getSTSLongMathRec(vstring *mmlLine, nmbrString *mathString, long statemNum, int recursionLevel) {
  flag result = 0;
  flag success = 0;
  long stsIndex = 0;
  long lenString = nmbrLen(mathString);
  if(dbs3) print2("%sPrinting \"%s\"!\n", space(recursionLevel), nmbrCvtMToVString(mathString));
  static flag recursionError = 0;

  if(recursionLevel > STS_MAX_RECURSION) {
    print2("Maximum recursion level reached when printing %s!\n", nmbrCvtMToVString(mathString));
    print2("Check your MMTS rules for possible circular references.\n");
    recursionError = 1;
    return 0;
  }

  // Pruning - if matching a single token (type + token), simply look it up.
  if(lenString == 2 && mathToken[mathString[1]].tokenType == con_) {
    stsIndex = stsVar[mathString[1]].stsSchemeId;
    if(stsIndex == 0) {
      //print2("Did not find token for %s.\n", nmbrCvtMToVString(mathString));
      return 0;
    }
    if(nmbrEq(stsScheme[stsIndex - 1].scheme, mathString)) {
	let(mmlLine, stsScheme[stsIndex - 1].typesetting);
	return 1;
    }
  }

  // Caching - try to retrieve result from cache
  if(stsUseCache && lenString < STS_MAX_LEN_FOR_CACHE) {
    let(mmlLine, htget(&stsCache, mathString));
    if(strlen(*mmlLine)) {
      return 1;
    }
  }

  for(stsIndex = 0;(result == 0 || success == 0) && stsIndex < stsSchemes;stsIndex++) {
    if(dbs5) let(mmlLine, ""); // this frees the temp alloc...

    /* Pruning - check the type token */
    if(stsScheme[stsIndex].scheme[0] != mathString[0]) continue;

    /* Pruning : skip if shorter than scheme */
    if(lenString < nmbrLen(stsScheme[stsIndex].scheme)) continue;

    /* Identifier schemes must be fully matched, and there is no substitution */
    if(stsScheme[stsIndex].type == 'i') {
      if(nmbrEq(stsScheme[stsIndex].scheme, mathString)) {
	let(mmlLine, stsScheme[stsIndex].typesetting);
	return 1;
      } else continue;
    }

    /* This case was handled above - skip it here */
    if(nmbrLen(stsScheme[stsIndex].scheme) == 2
	&& mathToken[stsScheme[stsIndex].scheme[1]].tokenType == con_) {
      continue;
    }

    /* Since the number of variables in substitution schemes is very limited
     * (at most 3 in current state), we're allocating the different arrays
     * needed for unification fixed length, on the stack.
     * Adding pool format only for easier printing.
     */
    nmbrString stateVectorV[STS_MAX_SCHEME_VAR+4];
    nmbrString varPosV[STS_MAX_SCHEME_VAR+4];
    nmbrString varStartV[STS_MAX_SCHEME_VAR+4];
    nmbrString varLenV[STS_MAX_SCHEME_VAR+4];
    nmbrString *stateVector = stateVectorV+3, *varPos = varPosV+3, *varStart = varStartV+3, *varLen=varLenV+3;

    long varCount = stsScheme[stsIndex].varCount;
    result = 3;
    success = 0;

    /* Initialize the state vector : first look for the first occurrences */
    for(int i=0;i<varCount-1;i++) stateVector[i] = 1;
    stateVector[varCount-1] = -1;
    stateVector[-1] = sizeof(long) * (varCount-1+1);
    stateVector[-2] = sizeof(long) * (varCount-1+1);

    /* As long as there is a unification, but no next level substitution */
    while((3 == result) && !success) {
      /* Launch unification (Neither unify() nor assignVar() actually match our need. Wrote our own unification) */
      if(dbs5) print2("%sTrying unification of \"%s\" and \"%s\" (state:<%s>)\n",
		     space(recursionLevel),
		     nmbrCvtMToVString(mathString),
		     nmbrCvtMToVString(stsScheme[stsIndex].scheme),
		     nmbrCvtAnyToVString(stateVector));
      result = unifySts(mathString, stsScheme[stsIndex].scheme, varPos, varStart, varLen, stateVector);
      if(dbs5) print2("%sResult is %d\n", space(recursionLevel), result);
      if(result == 3 && dbs3) print2("Unification for %s is not unique!\n", nmbrCvtMToVString(mathString));
      if(result != 0) {
	let(mmlLine, stsScheme[stsIndex].typesetting);

	if(dbs5) print2("%sFound match with %s\n", space(recursionLevel), nmbrCvtMToVString(stsScheme[stsIndex].scheme));
	success = 1; /* Be optimistic an assume next level substitutions will succeed */
	for(int i=0;i<varCount && success;i++) {
	  long varToken = stsScheme[stsIndex].scheme[varPos[i]-1];
	  if(dbs5) print2("%sVariable %d : %s , from %ld len %ld (%s)\n", space(recursionLevel), i, mathToken[varToken].tokenName, varStart[i], varLen[i], mathToken[stsVar[varToken].stsType].tokenName);

	  /* Build the new math string */
	  nmbrString *newMathString = NULL_NMBRSTRING;
	  nmbrLet(&newMathString, nmbrUnshiftElement(
	      nmbrMid(mathString, varStart[i], varLen[i]),
	      stsVar[varToken].stsType));
	  if(dbs5) print2("%sSubstitution : %s\n", space(recursionLevel), nmbrCvtMToVString(newMathString));

	  /* Try unification - recursively */
	  vstring substitution = "";
	  success &= getSTSLongMathRec(&substitution, newMathString, statemNum, recursionLevel+1);

	  /* deallocate new math string */
	  nmbrLet(&newMathString, NULL_NMBRSTRING);

	  if(success) {
	    vstring anchor = getAnchor(varToken);
	    long pos = instr(1, *mmlLine, anchor);
	    while(pos != 0) {
	      long end = pos + len(anchor);
	      let(mmlLine, cat(left(*mmlLine, pos-1), substitution, mid(*mmlLine, end, len(*mmlLine)-end+1), NULL));
	      pos = instr(pos + len(substitution), *mmlLine, anchor);
	    }
	  } /* end of if block on next substitution success */
	  /* Deallocate local strings */
	  let(&substitution, "");
	  if(!success && recursionError) {
	    //print2("When unifying %s with scheme %s (token type %s)\n", nmbrCvtMToVString(mathString), nmbrCvtMToVString(stsScheme[stsIndex].scheme), mathToken[stsVar[varToken].stsType].tokenName);
	    return 0;
	  }
	} /* end of for loop on variable substitutions */
      } /* end of if block on unification success */
      /* Stop if all next level substitutions succeeded */
      if(success) break;
    } /* end of while loop on unifications */
    /* Stop at the first successful unification */
    if(success) break;
  } /* end of for loop on  schemes */

  /* Caching - store result if successful */
  if(stsUseCache && success && (lenString > 2 || mathToken[mathString[1]].tokenType != con_) && lenString < STS_MAX_LEN_FOR_CACHE) {
    htput(&stsCache, mathString, *mmlLine);
  }

  if(result == 0 || success == 0) {
    /* failure */
    if(dbs5 || dbs3) print2("%sNo unification found for %s\n", space(recursionLevel), nmbrCvtMToVString(mathString));
    return 0;
  } else {
    /* success */
    if(dbs5 || dbs3) print2("%sApplied scheme %s for %s, result is %s\n", space(recursionLevel), nmbrCvtMToVString(stsScheme[stsIndex].scheme), nmbrCvtMToVString(mathString), *mmlLine);
    return 1;
  }
}

int zzz=0;
/* Returns a structured typesetting output, for the math string (hypothesis or
   conclusion) that is passed in. */
/* Warning: The caller must deallocate the returned vstring. */
flag getSTSLongMath(vstring *mmlLine, nmbrString *mathString, flag displayed, long statemNum, flag textwarn) {
  if(dbs1 || dbs3 || dbs5) outputToString = 0;
  if(dbs1) print2("Printing \"%s\"!\n", nmbrCvtMToVString(mathString));
  /* Call the recursive function - here we don't care about the return value. */
  flag success = getSTSLongMathRec(mmlLine, mathString, statemNum, 0);
  outputToString = 1;

  if(!success) {
//    // Try again with traces on
//    outputToString = 0;
//    debugOn();
//    getSTSLongMathRec(mmlLine, mathString, statemNum, 0);
//    debugOff();

    /* Return a generic error message */
    if(textwarn)
      let(mmlLine, cat("No typesetting for: ",
			nmbrCvtMToVString(mathString),NULL));
    else
      let(mmlLine, cat("No typesetting for: <code>",
			nmbrCvtMToVString(mathString),"</code>",NULL));
  }
  else {
    /* Add the HTML container */
    vstring *xfix;
    if(displayed) xfix = stsDisplayed; else xfix = stsInText;
    let(mmlLine, cat(xfix[0], *mmlLine, xfix[1], NULL));
  }

  if(stsUseCache) {
    outputToString = 0;
    htstats(&stsCache);
    if(++zzz%100==0)htdump(&stsCache);
    outputToString = 1;
  }
  return success;
}


/* Returns the code to be included in the HTML head element
 * for the given format */
vstring getSTSHeader() {
  return stsHeader;
}

/* Go through all non-definition axioms,
 * and check whether there is a corresponding STS scheme.
 * Prints a warning message for each missing declaration */
void verifySts(vstring format) {
  vstring texLine = "";
  int warnCount = 0;
  int checks = 0;

  print2("Verifying axioms have %s STS schemes...\n", format);
  if(strcmp(stsFormat, format)) {
    if(!parsetSTSRules(format)) {
      print2("?Aborting verify.\n");
      return;
    }
  }

  for(int statemNum=0;statemNum<statements;statemNum++) {
    /* Skip non-axioms and definitions */
    if(statement[statemNum].type != (char)a_) continue;
    if(strncmp(statement[statemNum].labelName,"df-",3) == 0) continue;
    if(strncmp(statement[statemNum].labelName,"ax-",3) == 0) continue;

    flag success = getSTSLongMath(&texLine, statement[statemNum].mathString, 1, statemNum, 1);
    outputToString = 0;
    if(!success) {
      print2("?Warning: %s\n", texLine);
      warnCount++;
    }
    /* Get rid of the texLine */
    let(&texLine, "");
    checks++;
  }

  print2("%ld syntax axioms were checked.\n", checks);
  if (!warnCount) {
    print2("No warnings were found.\n");
  } else {
    if (warnCount == 1) {
      print2("One warning was found.\n");
    } else {
      print2("%ld warnings were found.\n", (long)warnCount);
    }
  }
}


/* Returns the token ID for a given token */
long tokenId(vstring token) {
  void *mathKeyPtr; /* For binary search */
  mathKeyPtr = (void *)bsearch(token, mathKey, (size_t)mathTokens,
      sizeof(long), mathSrchCmp);
  if (!mathKeyPtr) return -1;
  return *((long *)mathKeyPtr);
}

/* Returns the HTML code string for a given token.
 * With STS, we normally don't map isolated tokens, but full constructs.
 * So this requires specific schemes to be added for single tokens when it
 * is not defined as a variable or constant (e.g. for ' ( ' or ' -> ' )
 * If it is not the case, this function will fall back to displaying the
 * token in ASCII */
/* *** Note: The caller must deallocate the returned string */
vstring stsToken(long tokenId, long stateNum) {
  vstring str2 = "";
  if(stsVar == NULL) {
    print2("Error: STS has not been initialized.");
    exit(-1);
  }

  // Expect to find
  long schemeId = stsVar[tokenId].stsSchemeId;
  //printf("Printing token %s, schemeId = %ld\n", mathToken[tokenId].tokenName, schemeId);
  //if(schemeId == 0)
  //  printf("Var not found : %s\n", mathToken[tokenId].tokenName);
  if(schemeId >= 1 && schemeId <= stsSchemes && stsVar[stsScheme[schemeId - 1].scheme[0]].isTerminal) {
    /* If there is a single token scheme, build the token from the scheme itself. */
    getSTSLongMath(&str2, stsScheme[schemeId - 1].scheme, 0, 0, 0);
  }
  else {
    /* Otherwise fallback to metamath token in case token unavailable */
    let(&str2, cat("<code style=\"color:red;\">",mathToken[tokenId].tokenName,"</code>", NULL));
  }
  return str2;
}

/*
 * Converts a string of ASCII text using STS.
 * This is used for comment sections in math mode.
 * Each math token MUST be separated by white space.
 */
vstring asciiToMathSts(vstring text, long statemNum) {
  vstring mmlLine = "";
  nmbrString *orgString = NULL_NMBRSTRING;
  nmbrString *mathString = NULL_NMBRSTRING;
  flag result = 0;

//  outputToString = 0;
//  print2("Going to try %ld types!\n", stsTypes);
//  dbs5=1;
  /* 20-Jan-2019 tar - use "statements" as stateNum, since all tokens are valid. */
  nmbrLet(&orgString, parseMathTokens(text, statements));
  for(int i=0;!result && i<nmbrLen(stsTerminalTypes);i++) {
    nmbrLet(&mathString, nmbrUnshiftElement(orgString, stsTerminalTypes[i])); // was 39
    outputToString = 0;
//    print2("Trying to print \"%s\"!\n", nmbrCvtMToVString(mathString));
    result = getSTSLongMath(&mmlLine, mathString, 0, statemNum, 1);
  }
  if(!result) {
    // In case we don't manage to parse the math string, we shall
    // revert to token-by-token conversion.
    let(&mmlLine, asciiMathToTexNoSts(text, statemNum));
  }
  nmbrLet(&mathString, NULL_NMBRSTRING);
  nmbrLet(&orgString, NULL_NMBRSTRING);
  return mmlLine;
}


//  print2("<style type=\"text/css\">\n");
//  print2(".MathJax_SVG_Display { margin: .2em 0em !important }\n");
//  print2(".MathJax_SVG_Display { float: left; width: inherit !important; }\n");
//  print2(".i { float: left; }\n");
//  print2(".hyp th,.stmt th,.proof th { border: 1px solid #CCC; }\n");
//  print2(".hyp td,.stmt td,.proof td { border: 1px solid #CCC; }\n");
//  print2(".proof tr:nth-child(even) { background-color: #F8FFFC; }\n");
//  print2(".hyp tr:nth-child(even) { background-color: #F8FFFC; }\n");
//  print2("tr.hant { background-color: #D0F0FF !important; }\n");
//  print2("tr.hstmt { background-color: #C0D0FF !important; }\n");
//  print2("tr.hdep { background-color: #F0F0FF !important; }\n");
//  print2("</style>\n");
//
//  /* Highlight antecedents when hovering above a proof.*/
//  print2("<script src=\"http://ajax.googleapis.com/ajax/libs/jquery/1.8.2/jquery.js\" type=\"text/javascript\"></script>\n");
//  print2("<script src=\"../hant.js\" type=\"text/javascript\"></script>\n");
//
//  /* This can be used to control the display of the MathJax content.
//   * Like alignment, line breaks, etc. */
//  print2("<script type=\"text/x-mathjax-config\">\n");
//  print2("MathJax.Hub.Config({\n");
////      print2("  jax: [\"input/MathML\",\"output/SVG\"],\n");
////      print2("  extensions: [\"mml2jax.js\"],\n");
////      print2("  \"fast-preview\": { disabled: true },\n");
//  print2("  SVG: { linebreaks: { width: \"container\", automatic: true }"
////	  ", styles:{\".MathJax_Display\": { \"float\": \"left\" } }"
//	  "}\n");
//  print2("});</script>\n");
//  print2("<script type=\"text/javascript\" src=\n");
//  print2("\"https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.1/\n");
//  print2("MathJax.js?config=MML_CHTML\"></script>\n");
//  print2("<script type=\"text/javascript\" src=\"../mmmathml.js\"></script>\n");
//  /*
//  print2("<script type=\"text/x-mathjax-config;executed=true\">\n");
//  print2("MathJax.Hub.Config({SVG:{styles:{\".MathJax_Display\": {\n");
//  print2("\"float\": \"left\"\n");
//  print2("}}}});</script>\n");
//  */
//  /*
//  print2("<script type=\"text/x-mathjax-config;executed=true\">\n");
//  print2("MathJax.Hub.Config({SVG:{styles:{\".MathJax_Display\": {\n");
//  print2("\"float\": \"left\"\n");
//  print2("}}}});</script>\n");
//  */
