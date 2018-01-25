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
#include <time.h>  /* 16-Aug-2016 nm For ELAPSED_TIME */
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmcmdl.h" /* 9/3/99 - for commandPrompt global */

#ifdef __WATCOMC__
  /* Bugs in WATCOMC:
     1. #include <conio.h> has compile errors
     2. Two consecutive calls to vprintf after va_start causes register dump
            or corrupts 2nd arg.
  */
/* From <conio.h>: */
#ifndef _CONIO_H_INCLUDED
extern int cprintf(const char *f__mt,...);
#define _CONIO_H_INCLUDED
#endif
#endif  /* End #ifdef __WATCOMC__ */

#ifdef THINK_C
#include <console.h>
#endif

#define QUOTED_SPACE 3 /* ASCII 3 that temporarily zaps a space */


int errorCount = 0;

/* Global variables used by print2() */
flag logFileOpenFlag = 0;
FILE *logFilePtr;
FILE *listFile_fp = NULL;
/* Global variables used by print2() */
flag outputToString = 0;
vstring printString = "";
/* Global variables used by cmdInput() */
long commandFileNestingLevel = 0;
FILE *commandFilePtr[MAX_COMMAND_FILE_NESTING + 1];
vstring commandFileName[MAX_COMMAND_FILE_NESTING + 1];
flag commandFileSilent[MAX_COMMAND_FILE_NESTING + 1];
flag commandFileSilentFlag = 0;
                                   /* 23-Oct-2006 nm For SUBMIT ... /SILENT */

FILE *inputDef_fp,*input_fp /*,*output_fp*/; /* File pointers */
                             /* 31-Dec-2017 nm output_fp deleted */
vstring inputDef_fn="",input_fn="",output_fn="";        /* File names */

long screenWidth = MAX_LEN; /* Width default = 79 */
/* screenHeight is one less than the physical screen to account for the
   prompt line after pausing. */
long screenHeight = SCREEN_HEIGHT; /* Default = 23 */ /* 18-Nov-05 nm */
int printedLines = 0; /* Lines printed since last user input (mod scrn hght) */
flag scrollMode = 1; /* Flag for continuous (0) or prompted (1) scroll */
flag quitPrint = 0; /* Flag that user quit the output */
flag localScrollMode = 1; /* 0 = Scroll continuously only till next prompt */

/* Buffer for B (back) command at end-of-page prompt - for future use */
pntrString *backBuffer = NULL_PNTRSTRING;
long backBufferPos = 0;
flag backFromCmdInput = 0; /* User typed "B" at main prompt */

/* Special:  if global flag outputToString = 1, then the output is not
             printed but is added to global string printString */
/* Returns 0 if user typed "q" during scroll prompt; this lets a procedure
   interrupt it's output for speedup (rest of output will be suppressed anyway
   until next command line prompt) */
flag print2(char* fmt,...)
{
  /* This performs the same operations as printf, except that if a log file is
    open, the characters will also be printed to the log file. */
  /* Also, scrolling is paused at each page if in scroll-prompted mode. */
  va_list ap;
  char c;
  long nlpos, lineLen, charsPrinted;
#ifdef THINK_C
  int ii, jj;
#endif
  char printBuffer[PRINTBUFFERSIZE];
  long i;

  if (backBufferPos == 0) {
    /* Initialize backBuffer - 1st time in program */
    /* Warning:  Don't call bug(), because it calls print2. */
    if (pntrLen(backBuffer)) {
      printf("*** BUG #1501\n");
#if __STDC__
      fflush(stdout);
#endif
    }
    backBufferPos = 1;
    pntrLet(&backBuffer, pntrAddElement(backBuffer));
    /* Note: pntrAddElement() initializes the added element to the
       empty string, so we don't need a separate initialization. */
    /* backBuffer[backBufferPos - 1] = ""; */  /* already done */
  }

  if ((!quitPrint && commandFileNestingLevel == 0 && (scrollMode == 1
           && localScrollMode == 1)
      /* 18-Nov-05 nm - now a variable settable with SET HEIGHT */
      && printedLines >= /*SCREEN_HEIGHT*/ screenHeight && !outputToString)
      || backFromCmdInput) {
    /* It requires a scrolling prompt */
    while(1) {
      if (backFromCmdInput && backBufferPos == pntrLen(backBuffer))
        break; /* Exhausted buffer */
      if (backBufferPos < 1 || backBufferPos > pntrLen(backBuffer)) {
        /* Warning:  Don't call bug(), because it calls print2. */
        printf("*** BUG #1502 %ld\n", backBufferPos);
#if __STDC__
        fflush(stdout);
#endif
      }
      if (backBufferPos == 1) {
        printf(
"Press <return> for more, Q <return> to quit, S <return> to scroll to end... "
          );
#if __STDC__
        fflush(stdout);
#endif
      } else {
        printf(
"Press <return> for more, Q <ret> quit, S <ret> scroll, B <ret> back up... "
         );
#if __STDC__
        fflush(stdout);
#endif
      }
      c = (char)(getchar());
      if (c == '\n') {
        if (backBufferPos == pntrLen(backBuffer)) {
          /* Normal output */
          break;
        } else {
          /* Get output from buffer */
          backBufferPos++;
          printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
#if __STDC__
          fflush(stdout);
#endif
          continue;
        }
      }
      if (getchar() == '\n') {
        if (c == 'q' || c == 'Q') {
          if (!backFromCmdInput)
            quitPrint = 1;
          break;
        }
        if (c == 's' || c == 'S') {

          if (backBufferPos < pntrLen(backBuffer)) {
            /* Print rest of buffer to screen */
            /*
            for (backBufferPos = backBufferPos + 1; backBufferPos <=
                pntrLen(backBuffer); backBufferPos++) {
            */
            /* 11-Sep-04 Don't use global var as loop var */
            while (backBufferPos + 1 <= pntrLen(backBuffer)) {
              backBufferPos++;
              printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
#if __STDC__
              fflush(stdout);
#endif
            }
          }
          if (!backFromCmdInput)
            localScrollMode = 0; /* Continuous scroll */
          break;

        }
        if (backBufferPos > 1) {
          if (c == 'b' || c == 'B') {
            backBufferPos--;
            printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
#if __STDC__
            fflush(stdout);
#endif
            continue;
          }
        }

        printf("%c", 7); /* Bell */
#if __STDC__
        fflush(stdout);
#endif
        continue;
      }
      while (c != '\n') c = (char)(getchar());
    } /* While 1 */
    if (backFromCmdInput)
      goto PRINT2_RETURN;
    printedLines = 0; /* Reset the number of lines printed on the screen */
    if (!quitPrint) {
      backBufferPos++;
      pntrLet(&backBuffer, pntrAddElement(backBuffer));
      /* Note: pntrAddElement() initializes the added element to the
         empty string, so we don't need a separate initialization. */
      /* backBuffer[backBufferPos - 1] = ""; */  /* already done */
    }
  }



  if (quitPrint && !outputToString)
    goto PRINT2_RETURN;    /* User typed 'q'
      above or earlier; 8/27/99: don't return if we're outputting to
      a string since we want to complete the writing to the string. */

  va_start(ap, fmt);
  charsPrinted = vsprintf(printBuffer, fmt, ap); /* Put formatted string into
      buffer */
  va_end(ap);

  /* Normally, long proofs are broken up into 80-or-less char lines
     by this point (via printLongLine) so this should never be a problem
     for them.  But in principle a very long line argument to print2
     could be a problem, although currently it should never occur
     (except maybe in long lines in tools? - if so, switch to printLongLine
     there to fix the bug). */
  /* Warning:  Don't call bug(), because it calls print2. */
  if (charsPrinted >= PRINTBUFFERSIZE
      /* || charsPrinted < 0 */
      /* There is a bug on the Sun with gcc version 2.7.2.2 where
         vsprintf returns approx. -268437768, so ignore the bug */
      ) {
    printf("*** BUG 1503\n");
    printf("?PRINTBUFFERSIZE %ld <= charsPrinted %ld in mminou.c\n",
        (long)PRINTBUFFERSIZE, charsPrinted);
    printf("?Memory may now be corrupted.\n");
    printf("?Save your work, exit, and verify output files.\n");
    printf("?You should recompile with increased PRINTBUFFERSIZE.\n");
#if __STDC__
    fflush(stdout);
#endif
  }

  nlpos = instr(1, printBuffer, "\n");
  lineLen = (long)strlen(printBuffer);

  /* 10/14/02 Change any ASCII 3's back to spaces, where they were set in
     printLongLine to handle the broken quote problem */
  for (i = 0; i < lineLen; i++) {
    if (printBuffer[i] == QUOTED_SPACE) printBuffer[i] = ' ';
  }

  if ((lineLen > screenWidth + 1) /* && (screenWidth != MAX_LEN) */
         && !outputToString  /* for HTML 7/3/98 */ ) {
    /* Force wrapping of lines that are too long by recursively calling
       print2() via printLongLine().  Note:  "+ 1" above accounts for \n. */
    /* Note that breakMatch is "" so it may break in middle of a word */
    if (!nlpos) {
      /* No end of line */
      printLongLine(left(printBuffer, lineLen), "", "");
    } else {
      printLongLine(left(printBuffer, lineLen - 1), "", "");
    }
    goto PRINT2_RETURN;
  }

  if (!outputToString && !commandFileSilentFlag) {
           /* 22-Oct-2006 nm Added commandFileSilentFlag for SUBMIT /SILENT,
              here and elsewhere in mminou.c */
    if (nlpos == 0) { /* Partial line (usu. status bar) - print immediately */

#ifdef __WATCOMC__
      cprintf("%s", printBuffer); /* Immediate console I/O (printf buffers it)*/
#else
      printf("%s", printBuffer);
#endif

#ifdef THINK_C
      /* Force a console flush to see it (otherwise only CR flushes it) */
      cgetxy(&ii, &jj, stdout);
#endif

#if __STDC__
      /*
      The following change to mminou.c was necessary on my Unix (Linux)
      system to get the `verify proof *' progress bar to display
      progressively (rather than all at once). I've conditionalized it on
      __STDC__, since it should be harmless on any ANSI C system.
        -- Stephen McCamant  smccam @ uclink4.berkeley.edu 12/9/00
      */
      fflush(stdout);
#endif

    } else {
      printf("%s", printBuffer); /* Normal line */
#if __STDC__
      fflush(stdout);
#endif
      printedLines++;
      if (!(scrollMode == 1 && localScrollMode == 1)) {
        /* Even in non-scroll (continuous output) mode, still put paged-mode
           lines into backBuffer in case user types a "B" command later,
           so user can page back from end. */
        /* 18-Nov-05 nm - now a variable settable with SET HEIGHT */
        if (printedLines > /*SCREEN_HEIGHT*/ screenHeight) {
          printedLines = 1;
          backBufferPos++;
          pntrLet(&backBuffer, pntrAddElement(backBuffer));
          /* Note: pntrAddElement() initializes the added element to the
             empty string, so we don't need a separate initialization. */
          /* backBuffer[backBufferPos - 1] = ""; */  /* already done */
        }
      }
    }
    /* Add line to backBuffer string array */
    /* Warning:  Don't call bug(), because it calls print2. */
    if (backBufferPos < 1) {
      printf("*** PROGRAM BUG #1504\n");
#if __STDC__
      fflush(stdout);
#endif
    }
    let((vstring *)(&(backBuffer[backBufferPos - 1])), cat(
        (vstring)(backBuffer[backBufferPos - 1]), printBuffer, NULL));
  } /* End if !outputToString */

  if (logFileOpenFlag && !outputToString /* && !commandFileSilentFlag */) {
    fprintf(logFilePtr, "%s", printBuffer);  /* Print to log file */
#if __STDC__
    /* 10-Oct-2016 nm */
    fflush(logFilePtr);
#endif
  }

  if (listMode && listFile_fp != NULL && !outputToString) {
    /* Put line in list.tmp as comment */
    fprintf(listFile_fp, "! %s", printBuffer);  /* Print to list command file */
  }

  if (outputToString) {
    let(&printString, cat(printString, printBuffer, NULL));
  }

  /* Check for lines too long */
  if (lineLen > screenWidth + 1) { /* The +1 ignores \n */
    /* Warning:  Don't call bug(), because it calls print2. */
    printf("*** PROGRAM BUG #1505 (not serious, but please report it)\n");
    printf("Line exceeds screen width; caller should use printLongLine.\n");
    printf("%ld %s\n", lineLen, printBuffer);
    /*printf(NULL);*/  /* Force crash on VAXC to see where it came from */
#if __STDC__
    fflush(stdout);
#endif
  }
  /* \n not allowed in middle of line */
  /* Warning:  Don't call bug(), because it calls print2. */
  if (nlpos != 0 && nlpos != lineLen) {
    printf("*** PROGRAM BUG #1506\n");
#if __STDC__
    fflush(stdout);
#endif
  }

 PRINT2_RETURN:
  return (!quitPrint);
}


/* printLongLine automatically puts a newline \n in the output line. */
/* startNextLine is the string to place before continuation lines. */
/* breakMatch is a list of characters at which the line can be broken. */
/* Special:  startNextLine starts with "~" means add tilde after broken line */
/* Special:  breakMatch begins with "&" means compressed proof */
/* Special:  breakMatch empty means break line anywhere */
/* Special:  breakMatch begins with octal 1 means nested tree display (right
             justify continuation lines); 1 is changed to space for
             break matching */
/* Special:  if breakMatch is \, then put % at end of previous line for LaTeX*/
/* Special:  if breakMatch is " (quote), treat as if space but don't break
             quotes, and also let lines grow long - use this call for all HTML
             code */ /* Added 10/14/02 */
void printLongLine(vstring line, vstring startNextLine, vstring breakMatch)
{
  vstring longLine = "";
  vstring multiLine = "";
  vstring prefix = "";
  vstring startNextLine1 = "";
  vstring breakMatch1 = "";
  long i, j, p;
  long startNextLineLen;
  flag firstLine;
  flag tildeFlag = 0;
  flag treeIndentationFlag = 0;

  /* 10/14/02 added for HTML handling */
  /* 26-Jun-2014 nm No longer needed? */
  /* flag htmlFlag = 0; */
               /* 1 means printLongLine was called with "\"" as
                        breakMatch argument (for HTML code) */
  flag quoteMode = 0; /* 1 means inside quote */
  /*char quoteChar = '"';*/ /* Current quote character */
  /*long quoteStartPos = 0;*/ /* Start of quote */
  long saveScreenWidth; /* To let screenWidth grow temporarily */

  long saveTempAllocStack;

  /* Blank line (the rest of algorithm would ignore it; output and return) */
  if (!line[0]) {
    /* 10/14/02 Do a dummy let() so caller can always depend on printLongLine
       to empty the tempalloc string stack (for the rest of this code, the
       first let() will do this) */
    let(&longLine, "");
    print2("\n");
    return;
  }

  /* Change the stack allocation start to prevent arguments from being
     deallocated.  We need to do this because more than one argument
     may be passed in with cat(), chr(), etc. and let() can only grab
     one, destroying the others  */
  saveTempAllocStack = startTempAllocStack;
  startTempAllocStack = tempAllocStackTop; /* For let() stack cleanup */
  /* Added 10/14/02 */
  /* Grab the input arguments */
  let(&multiLine, line);
  let(&startNextLine1, startNextLine);
  let(&breakMatch1, breakMatch);
  /* Now relax - back to normal; we can let temporary allocation stack die. */
  startTempAllocStack = saveTempAllocStack;

  /* 10/14/02 */
  /* We must copy input argument breakMatch to a variable string because we
     will be zapping one of its characters, and ordinarily breakMatch is
     passed in as a constant string.  However, this is now done with argument
     grabbing above so we're OK. */

  /* Flag to right justify continuation lines */
  if (breakMatch1[0] == 1) {
    treeIndentationFlag = 1;
    breakMatch1[0] = ' '; /* Change to a space (the real break character) */
  }

  /* HTML mode */   /* Added 10/14/02 */
  /* The HTML mode is intended not to break inside quoted HTML tag
     strings.  All HTML output should be called with this mode.
     Since we don't parse HTML this method is not perfect.  Only double
     quotes are inspected, so all HTML strings with spaces must be
     surrounded by double quotes.  If text quotes surround a tag
     with a quoted string, this code will not work and must be
     enhanced. */
  /* Whenever we are inside of a quote, we change a space to ASCII 3 to
     prevent matching it.  The reverse is done in the print2() function,
     where all ASCII 3's are converted back to space. */
  /* Note added 10/20/02: tidy.exe breaks HREF quotes with new line.
     Check HTML spec - do we really need this code? */
  j = (long)strlen(multiLine);
  /* Do a bug check to make sure no real ASCII 3's are ever printed */
  for (i = 0; i < j; i++) {
    if (multiLine[i] == QUOTED_SPACE) bug(1514); /* Should never be the case */
  }
  if (breakMatch1[0] == '\"') {
    /* htmlFlag = 1; */ /* 26-Jun-2014 nm No longer needed? */
    breakMatch1[0] = ' '; /* Change to a space (the real break character) */
    /* Scan string for quoted strings */
    quoteMode = 0;

    /* 19-Nov-2007 nm  New algorithm:  don't put line breaks in anything inside
       _double_ quotes that follows an = sign, such as TITLE="abc def".  Ignore
       _single_ quotes (which could be apostrophes).  The HTML output code
       must be (and so far is) written to conform to this. */
    i = 0;
    while (multiLine[i]) {
      if (multiLine[i] == '"' && i > 0) {
        if (!quoteMode && multiLine[i - 1] == '=')
          quoteMode = 1;
        else
          quoteMode = 0;
      }
      if (multiLine[i] == ' ' && quoteMode)
        multiLine[i] = QUOTED_SPACE;
      i++;
    }

/* 19-Nov-2007 nm  Bypass old complex code that doesn't work right */
/**************** THE CODE BELOW IS OBSOLETE AND WILL BE DELETED. ********/
#ifdef OBSOLETE_QUOTE_HANDLING_CODE
    for (i = 0; i < j; i++) {

      /* 10/24/02 - This code has problems with SHOW STATEMENT/ALT_HTML
         It is bypassed completely now. */
      if (i == i) break;

      /* Special case: ignore ALT='"' in set.mm */
      if (multiLine[i] == '"' && i >= 5
          && !strncmp("ALT='\"'\"", multiLine + i - 5, 7))
        continue;
      if (!quoteMode) {
        /* If we wanted to handle double and single nested quotes: */
        /* if (multiLine[i] == '\'' || multiLine[i] == '"') { */
        /* But since single quotes are ambiguous with apostrophes, we will
           demand that only double quotes be used around HTML tag strings
           that have spaces such as <FONT FACE="Arial Narrow">. */
        if (multiLine[i] == '"') {
          quoteMode = 1;
          quoteStartPos = i;
          quoteChar = multiLine[i];
        }
      } else {
        if (multiLine[i] == quoteChar) {
          quoteMode = 0;
        }
      }
      if (quoteMode == 1 && multiLine[i] == ' ') {
        /* Zap the space with ASCII 3 */
        multiLine[i] = QUOTED_SPACE;
      }
    }
    /* If we ended in quoteMode, it wasn't a real quote.  Revert the
       space zapping. */
    if (quoteMode == 1) {
      for (i = quoteStartPos; i < j; i++) {
        if (multiLine[i] == QUOTED_SPACE) multiLine[i] = ' ';
      }
    }
    /* As a special case for more safety, we'll zap the ubiquitous
       "Arial Narrow" used for the little pink numbers. */
    i = 0;
    while (1) {
      i = instr(i + 1, multiLine, "Arial Narrow");
      if (i) multiLine[i + 4] = QUOTED_SPACE;
      else break;
    }
#endif
/**************** END OF OBSOLETE SECTION ********/

  }


  /* The tilde is a special flag for printLongLine to print a
     tilde before the carriage return in a split line, not after */
  if (startNextLine1[0] == '~') {
    tildeFlag = 1;
    let(&startNextLine1, " ");
  }


  while (multiLine[0]) { /* While there are multi caller-inserted newlines */

    /* Process caller-inserted newlines */
    p = instr(1, multiLine, "\n");
    if (p) {
      /* Get the next caller's line */
      let(&longLine, left(multiLine, p - 1));
      /* Postpone the remaining lines to multiLine for next time around */
      /* /@ 12-Jun-2011 nm Put continuation line start (normally spaces) at
         the beginning of explicit new line, removing spaces that may
         already be there - for use after blank line in outputStatement()
         in mmpars.c @/
      let(&multiLine, cat(startNextLine1,
          edit(right(multiLine, p + 1), 8 /@ Discard leading spaces @/),
          NULL)); */
     /* The above is bad, because it doesn't allow flexible user indentation */
      /* OLD */ let(&multiLine, right(multiLine, p + 1));
    } else {
      let(&longLine, multiLine);
      let(&multiLine, "");
    }

    saveScreenWidth = screenWidth;
   HTML_RESTART:
    /* Now we will break up one caller's line */
    firstLine = 1;

    startNextLineLen = (long)strlen(startNextLine1);
    /* Prevent infinite loop if next line prefix is longer than screen */
    if (startNextLineLen > screenWidth - 4) {
      startNextLineLen = screenWidth - 4;
      let(&startNextLine1, left(startNextLine1, screenWidth - 4));
    }
    while ((signed)(strlen(longLine)) + (1 - firstLine) * startNextLineLen >
        screenWidth - (long)tildeFlag - (long)(breakMatch1[0] == '\\')) {
      p = screenWidth - (long)tildeFlag - (long)(breakMatch1[0] == '\\') + 1;
      if (!firstLine) p = p - startNextLineLen;

      if (p < 4) bug(1524);  /* This may cause out-of-string ref below */
      /* Assume compressed proof if 1st char of breakMatch1 is "&" */
      if (breakMatch1[0] == '&'
          && ((!instr(p, left(longLine, (long)strlen(longLine) - 3), " ")
              && longLine[p - 3] != ' ') /* Don't split trailing "$." */
            /* 2-Jan-2014 nm Added the condition below: */
            || longLine[p - 4] == ')')) /* Label sect ends in col 77 */ {
        /* We're in the compressed proof section; break line anywhere */
        p = p + 0;  /* Don't change position */
        /* 27-Dec-2013 nm */
        /* In the case where the last space occurs at column 79 i.e.
           screenWidth, break the line at column 78.  This can happen
           when compressed proof ends at column 78, followed by space
           and "$."  It prevents an extraneous trailing space on the line. */
        if (longLine[p - 2] == ' ') p--; /* 27-Dec-2013 */
      } else {
        if (!breakMatch1[0]) {
          p = p + 0; /* Break line anywhere; don't change position */
        } else {
          if (breakMatch1[0] == '&') {
            /* Compressed proof */
            /* 27-Dec-2013 nm - no longer add the trailing space */
            /* p = p - 1; */ /* We will add a trailing space to line for easier
                          label searches by the user during editing */
          }
          if (p <= 0) bug(1518);
          /*while (!instr(1, breakMatch1, mid(longLine,p,1)) && p > 0) {*/
          /* Speedup */
          /* while (strchr(breakMatch1, longLine[p - 1]) == NULL) { */
          /* 24-Feb-2010 nm For LaTeX, match space, not backslash */
          /* (Todo:  is backslash match mode really needed?) */
          while (strchr(breakMatch1[0] != '\\' ? breakMatch1 : " ",
              longLine[p - 1]) == NULL) {
            p--;
            if (!p) break;
          }
          /* if (p <= 0 && htmlFlag) { */
          /* 25-Jun-2014 nm We will now not break any line at non-space,
             since it causes more problems that it solves e.g. with
             WRITE SOURCE.../REWRAP with long URLs */
          if (p <= 0) {
            /* The line couldn't be broken.  Since it's an HTML line, we
               can increase screenWidth until it will fit. */
            screenWidth++;
            /******* for debugging screen width change
            if (outputToString){
              outputToString = 0;
              print2("debug: screenWidth = %ld\n", screenWidth);
              outputToString = 1;
            }
            ********* end debug */
            /* If this bug happens, we'll have to increase PRINTBUFFERSIZE
               or change the HTML code being printed. */
            if (screenWidth >= PRINTBUFFERSIZE - 1) bug(1517);
            goto HTML_RESTART; /* Ugly but another while loop nesting would
                                  be even more confusing */
          }

          if (breakMatch1[0] == '&') {
            /* Compressed proof */
            /* 27-Dec-2013 nm - no longer add the trailing space */
            /* p = p + 1; */ /* We will add a trailing space to line for easier
                          label searches by the user during editing */
          }
        } /* end if (!breakMatch1[0]) else */
      } /* end if (breakMatch1[0] == '&' &&... else */

      if (p <= 0) {
        /* Break character not found; give up at
           screenWidth - (long)tildeFlag  - (long)(breakMatch1[0] == '\\')+ 1 */
        p = screenWidth - (long)tildeFlag  - (long)(breakMatch1[0] == '\\')+ 1;
        if (!firstLine) p = p - startNextLineLen;
        if (p <= 0) p = 1; /* If startNextLine too long */
      }
      if (!p) bug(1515); /* p should never be 0 by this point */
      /* If we broke at a non-space 1st char, line length won't get reduced */
      /* Hopefully this will never happen with the breakMatch's we use,
         otherwise the code will require a rework. */
      if (p == 1 && longLine[0] != ' ') bug(1516);
      if (firstLine) {
        firstLine = 0;
        let(&prefix, "");
      } else {
        let(&prefix, startNextLine1);
        if (treeIndentationFlag) {
          if (startNextLineLen + p - 1 < screenWidth) {
            /* Right justify output for continuation lines */
            let(&prefix, cat(prefix, space(screenWidth - startNextLineLen
                - p + 1), NULL));
          }
        }
      }
      if (!tildeFlag) {
        /* 7-Sep-2010 nm - Don't do this anymore with new (24-Feb-2010) LaTeX
           output, since it isn't needed, and worse, it causes words in the
           description to be joined together without space.  (It might be better
           to analyze if breakMatch1[0] == '\\' is needed at all.) */
        /*** start of 7-Sep-2010 commented out code
        if (breakMatch1[0] == '\\') {
          /@ Add LaTeX comment char to ignore carriage return @/
          print2("%s\n",cat(prefix, left(longLine,p - 1), "%", NULL));
        } else {
        *** end of 7-Sep-2010 commented out code */
          print2("%s\n",cat(prefix, left(longLine,p - 1), NULL));
        /*** start of 7-Sep-2010 commented out code
        }
        *** end of 7-Sep-2010 commented out code */
      } else {
        print2("%s\n",cat(prefix, left(longLine,p - 1), "~", NULL));
      }
      if (longLine[p - 1] == ' ' &&
          breakMatch1[0] /* But not "break anywhere" line */) {
        /* Remove leading space for neatness */
        if (longLine[p] == ' ') {
          /* There could be 2 spaces at the end of a sentence. */
          let(&longLine, right(longLine, p + 2));
        } else {
          let(&longLine, right(longLine,p + 1));
        }
      } else {
        let(&longLine, right(longLine,p));
      }
    } /* end while longLine too long */
    if (!firstLine) {
      if (treeIndentationFlag) {
        /* Right justify output for continuation lines */
        print2("%s\n",cat(startNextLine1, space(screenWidth
            - startNextLineLen - (long)(strlen(longLine))), longLine, NULL));
      } else {
        print2("%s\n",cat(startNextLine1, longLine, NULL));
      }
    } else {
      print2("%s\n",longLine);
    }
    screenWidth = saveScreenWidth; /* Restore to normal */

  } /* end while multiLine != "" */

  let(&multiLine, ""); /* Deallocate */
  let(&longLine, ""); /* Deallocate */
  let(&prefix, ""); /* Deallocate */
  let(&startNextLine1, ""); /* Deallocate */
  let(&breakMatch1, ""); /* Deallocate */

  return;
} /* printLongLine */


vstring cmdInput(FILE *stream, vstring ask)
{
  /* This function prints a prompt (if 'ask' is not NULL) and gets a line from
    the input stream.  NULL is returned when end-of-file is encountered.
    New memory is allocated each time linput is called.  This space must
    be freed by the caller. */
  vstring g = ""; /* Always init vstrings to "" for let(&...) to work */
  long i;
#define CMD_BUFFER_SIZE 2000

  while (1) { /* For "B" backup loop */
    if (ask != NULL && !commandFileSilentFlag) {
      printf("%s",ask);
#if __STDC__
      fflush(stdout);
#endif
    }
    let(&g, space(CMD_BUFFER_SIZE)); /* Allocate CMD_BUFFER_SIZE+1 bytes */
    if (g[CMD_BUFFER_SIZE]) bug(1520); /* Bug in let() (improbable) */
    g[CMD_BUFFER_SIZE - 1] = 0; /* For overflow detection */
    if (!fgets(g, CMD_BUFFER_SIZE, stream)) {
      /* End of file */
      let(&g, ""); /* Deallocate memory */
      return NULL;
    }
    if (g[CMD_BUFFER_SIZE - 1]) {
      /* Detect input overflow */
      /* Warning:  Don't call bug() - it calls print2 which may call this. */
      printf("***BUG #1508\n");
#if __STDC__
      fflush(stdout);
#endif
    }
    i = (long)strlen(g);
/*E*/db = db - (CMD_BUFFER_SIZE - i); /* Adjust string usage to detect leaks */
    /* Detect operating system bug of inputting no characters */
    if (!i) {
      printf("***BUG #1507\n");
#if __STDC__
      fflush(stdout);
#endif

    /* 12-Oct-2006 Fix bug that occurs when the last line in the file has
       no new-line (reported by Marnix Klooster) */
    } else {
      if (g[i - 1] != '\n') {
        /* Warning:  Don't call bug() - it calls print2 which may call this. */
        if (!feof(stream)) {
          printf("***BUG #1525\n");
#if __STDC__
          fflush(stdout);
#endif
        }
        /* Add a new-line so processing below will behave correctly. */
        let(&g, cat(g, chr('\n'), NULL));
/*E*/db = db + (CMD_BUFFER_SIZE - i); /* Cancel extra piece of string */
        i++;
      }
    /* 12-Oct-2006 End of new code */

    }

    if (g[1]) {
      i--;
      if (g[i] != '\n') {
        printf("***BUG #1519\n");
#if __STDC__
        fflush(stdout);
#endif
      }
      g[i]=0;   /* Eliminate new-line character by zapping it */
/*E*/db = db - 1;
    } else {
      if (g[0] != '\n') {
        printf("***BUG #1521\n");
#if __STDC__
        fflush(stdout);
#endif
      }
      /* Eliminate new-line by deallocating vstring space (if we just zap
         character [0], let() will later think g is an empty string constant
         and will never deallocate g) */
      let(&g, "");
    }

    /* If user typed "B" (for back), go back to the back buffer to
       let the user scroll through it */
    if ((!strcmp(g, "B") || !strcmp(g, "b")) /* User typed "B" */
        && pntrLen(backBuffer) > 1   /* The back-buffer still exists and
                                         there was a previous page */
        && commandFileNestingLevel == 0
        && (scrollMode == 1 && localScrollMode == 1)
        && !outputToString) {
      /* Set variables so only backup buffer will be looked at in print2() */
      backBufferPos = pntrLen(backBuffer) - 1;
      printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
#if __STDC__
      fflush(stdout);
#endif
      backFromCmdInput = 1; /* Flag for print2() */
      print2(""); /* Only the backup buffer will be looked at */
      backFromCmdInput = 0;
    } else {
      /* If the command line is empty (at main prompt), let user still
         type "B" for convenience in case too many
         returns where hit while scrolling */
      if (commandFileNestingLevel > 0) break;
                            /* 23-Aug-04 We're taking from a SUBMIT
                              file so break out of loop that looks for "B" */
      if (ask == NULL) {
        printf("***BUG #1523\n"); /* 23-Aug-04 In non-SUBMIT
          mode 'ask' won't be NULL, so flag non-fatal bug here just in case */
#if __STDC__
        fflush(stdout);
#endif
      }
      if (g[0]) break; /* 23-Aug-04 Command line not empty so break out of loop
                          that looks for "B" */
      if (ask != NULL &&
          /* User entered empty command line but not at a prompt */
          /* commandPrompt is assigned in metamath.c and declared in
             mmcmdl.h */
          strcmp(ask, commandPrompt)) {
        break; /* Break out of loop that looks for "B" */
      }
    }
  } /* while 1 */

  return g;
} /* cmdInput */

vstring cmdInput1(vstring ask)
{
  /* This function gets a line from either the terminal or the command file
    stream depending on commandFileNestingLevel > 0.  It calls cmdInput(). */
  /* Warning: the calling program must deallocate the returned string. */
  vstring commandLn = "";
  vstring ask1 = "";
  long p, i;

  let(&ask1, ask); /* In case ask is temporarily allocated (i.e in case it
                      will become deallocated at next let() */
  /* Look for lines too long */
  while ((signed)(strlen(ask1)) > screenWidth) {
    p = screenWidth - 1;
    while (ask1[p] != ' ' && p > 0) p--;
    if (!p) p = screenWidth - 1;
    print2("%s\n", left(ask1, p));
    let(&ask1, right(ask1, p + 1));
  }
  /* Allow 10 characters for answer */
  if ((signed)(strlen(ask1)) > screenWidth - 10) {
    p = screenWidth - 11;
    while (ask1[p] != ' ' && p > 0) p--;
    if (p) {  /* (Give up if no spaces) */
      print2("%s\n", left(ask1, p));
      let(&ask1, right(ask1, p + 1));
    }
  }

  printedLines = 0; /* Reset number of lines printed since last user input */
  quitPrint = 0; /* Reset quit print flag */
  localScrollMode = 1; /* Reset to prompted scroll */

  while (1) {
    if (commandFileNestingLevel == 0) {
      commandLn = cmdInput(stdin, ask1);
      if (!commandLn) {
        commandLn = ""; /* Init vstring (was NULL) */
        /* 21-Feb-2010 nm Allow ^D to exit */
        /* 21-Feb-2010 Removed line: */
        /* let(&commandLn, "^Z"); */
        /* 21-Feb-2010 Added lines: */
        if (strcmp(left(ask1, 2), "Do")) {
          /* ^Z or ^D found at MM>, MM-PA>, or TOOLS> prompt */
          let(&commandLn, "EXIT");
        } else {
          /* Detected the question "Do you want to EXIT anyway (Y, N) <N>?" */
          /* Force exit with Y, to prevent infinite loop */
          let(&commandLn, "Y");
        }
        printf("%s\n", commandLn); /* Let user see what's happening */
        /* 21-Feb-2010 end of change */
      }
      if (logFileOpenFlag) fprintf(logFilePtr, "%s%s\n", ask1, commandLn);

      /* Clear backBuffer from previous scroll session */
      for (i = 0; i < pntrLen(backBuffer); i++) {
        let((vstring *)(&(backBuffer[i])), "");
      }
      backBufferPos = 1;
      pntrLet(&backBuffer, NULL_PNTRSTRING);
      pntrLet(&backBuffer, pntrAddElement(backBuffer));
      /* Note: pntrAddElement() initializes the added element to the
         empty string, so we don't need a separate initialization. */
      /* backBuffer[backBufferPos - 1] = ""; */  /* already done */

      /* Add user's typing to the backup buffer for display on 1st screen */
      let((vstring *)(&(backBuffer[backBufferPos - 1])), cat(
          (vstring)(backBuffer[backBufferPos - 1]), ask1,
          commandLn, "\n", NULL));

      if (listMode && listFile_fp != NULL) {
        /* Put line in list.tmp as comment */
        fprintf(listFile_fp, "! %s\n", commandLn);
      }

    } else { /* Get line from SUBMIT file */
      commandLn = cmdInput(commandFilePtr[commandFileNestingLevel], NULL);
      if (!commandLn) { /* EOF found */
        fclose(commandFilePtr[commandFileNestingLevel]);
        print2("%s[End of command file \"%s\".]\n", ask1,
            commandFileName[commandFileNestingLevel]);
        let(&(commandFileName[commandFileNestingLevel]), "");
                                                        /* Deallocate string */
        commandFileNestingLevel--;
        commandLn = "";
        if (commandFileNestingLevel == 0) {
          commandFileSilentFlag = 0; /* 23-Oct-2006 nm Added SUBMIT / SILENT */
        } else {
          commandFileSilentFlag = commandFileSilent[commandFileNestingLevel];
               /* Revert to previous nesting level's silent flag */
        }
        break; /*continue;*/
      }

      /* 22-Jan-2018 nm */
      /* Tolerate CRs in SUBMIT files (e.g. created on Windows and
         run on Linux) */
      let(&commandLn, edit(commandLn, 8192/* remove CR */));

      print2("%s%s\n", ask1, commandLn);
    }
    break;
  }

  let(&ask1, ""); /* 10/20/02 Deallocate */
  return commandLn;
} /* cmdInput1 */


void errorMessage(vstring line, long lineNum, long column, long tokenLength,
  vstring error, vstring fileName, long statementNum, flag severity)
{
  /* Note:  "line" may be terminated with \n.  "error" and "fileName"
     should NOT be terminated with \n.  This is done for the convenience
     of the calling functions. */
  vstring errorPointer = "";
  vstring tmpStr = "";
  vstring prntStr = "";
  vstring line1 = "";
  int j;
  /*flag saveOutputToString;*/ /* 22-May-2016 nm */ /* 9-Jun-2016 reverted */

  /* 22-May-2016 nm */
  /* Prevent putting error message in printString */
  /* 9-Jun-2016 nm Revert this change, because 'minimize_with' makes
     use of the string to hold the DV violation error message.
     We can reinstate this fix when 'minimize_with' is improved to
     call a DV-checking function directly. */
  /*
  saveOutputToString = outputToString;
  outputToString = 0;
  */

  /* Make sure vstring argument doesn't get deallocated with another let */
/*??? USE SAVETEMPALLOC*/
  let(&tmpStr,error); /* error will get deallocated here */
  error = "";
  let(&error,tmpStr); /* permanently allocate error */

  /* Add a newline to line1 if there is none */
  if (line) {
    if (line[strlen(line) - 1] != '\n') {
      let(&line1, line);
    } else {
      bug(1509);
    }
  } else {
    line1 = NULL;
  }

  if (fileName) {
    /* Put a blank line between error msgs if we are parsing a file */
    print2("\n");
  }

  switch (severity) {
    case (char)notice_:
      let(&prntStr, "?Notice"); break;
    case (char)warning_:
      let(&prntStr, "?Warning"); break;
    case (char)error_:
      let(&prntStr, "?Error"); break;
    case (char)fatal_:
      let(&prntStr, "?Fatal error"); break;
  }
  if (lineNum) {
    let(&prntStr, cat(prntStr, " on line ", str((double)lineNum), NULL));
    if (fileName) {
      let(&prntStr, cat(prntStr, " of file \"", fileName, "\"", NULL));
    }
  } else {
    if (fileName) {
      let(&prntStr, cat(prntStr, " in file \"", fileName, "\"", NULL));
    }
  }
  if (statementNum) {
    let(&prntStr, cat(prntStr, " at statement ", str((double)statementNum), NULL));
    if (statement[statementNum].labelName[0]) {
      let(&prntStr, cat(prntStr, ", label \"",
          statement[statementNum].labelName, "\"", NULL));
    }
    let(&prntStr, cat(prntStr, ", type \"$", chr(statement[statementNum].type),
        "\"", NULL));
  }
  printLongLine(cat(prntStr, ":", NULL), "", " ");
  if (line1) printLongLine(line1, "", "");
  if (line1 && column && tokenLength) {
    let(&errorPointer,"");
    for (j=0; j<column-1; j++) {
      /* Make sure that tabs on the line with the error are accounted for so
         that the error pointer lines up correctly */
      if (line1[j] == '\t') let (&errorPointer,cat(errorPointer,"\t",NULL));
      else let(&errorPointer,cat(errorPointer," ",NULL));
    }
    for (j=0; j<tokenLength; j++)
      let(&errorPointer,cat(errorPointer,"^",NULL));
    printLongLine(errorPointer, "", "");
  }
  printLongLine(error,""," ");
  if (severity == 2) errorCount++;

  /* ???Should there be a limit? */
  /* if (errorCount > 1000) {
    print2("\n"); print2("?Too many errors - aborting Metamath.\n");
    exit(0);
  } */

  /* 22-May-2016 nm */
  /* Restore output to printString if it was enabled before */
  /* 9-Jun-2016 nm Reverted */
  /*
  outputToString = saveOutputToString;
  */

  if (severity == 3) {
    print2("Aborting Metamath.\n");
    exit(0);
  }
  let(&errorPointer,"");
  let(&tmpStr,"");
  let(&prntStr,"");
  let(&error,"");
  if (line1) let(&line1,"");
} /* errorMessage() */




/* Opens files with error message; opens output files with
   backup of previous version.   Mode must be "r" or "w" or "d" (delete). */
/* 31-Dec-2017 nm Added "safe" delete */
/* 31-Dec-2017 nm Added noVersioningFlag = don't create ~1 backup */
FILE *fSafeOpen(vstring fileName, vstring mode, flag noVersioningFlag)
{
  FILE *fp;
  vstring prefix = "";
  vstring postfix = "";
  vstring bakName = "";
  vstring newBakName = "";
  long v;
  long lastVersion; /* Last version before gap */ /* nm 29-Apr-2007 */

  if (!strcmp(mode, "r")) {
    fp = fopen(fileName, "r");
    if (!fp) {
      print2("?Sorry, couldn't open the file \"%s\".\n", fileName);
    }
    return (fp);
  }

  if (!strcmp(mode, "w")
      || !strcmp(mode, "d")) {    /* 31-Dec-2017 */
    /* See if the file already exists. */
    fp = fopen(fileName, "r");

    if (fp) {
      fclose(fp);
      if (noVersioningFlag) goto skip_backup; /* 31-Dec-2017 nm */

#define VERSIONS 9
      /* The file exists.  Rename it. */

#if defined __WATCOMC__ /* MSDOS */
      /* Make sure file name before extension is 8 chars or less */
      i = instr(1, fileName, ".");
      if (i) {
        let(&prefix, left(fileName, i - 1));
        let(&postfix, right(fileName, i));
      } else {
        let(&prefix, fileName);
        let(&postfix, "");
      }
      let(&prefix, cat(left(prefix, 5), "~", NULL));
      let(&postfix, cat("~", postfix, NULL));
      if (0) goto skip_backup; /* Prevent compiler warning */

#elif defined __GNUC__ /* Assume unix */
      let(&prefix, cat(fileName, "~", NULL));
      let(&postfix, "");

#elif defined THINK_C /* Assume Macintosh */
      let(&prefix, cat(fileName, "~", NULL));
      let(&postfix, "");

#elif defined VAXC /* Assume VMS */
      /* For debugging on VMS: */
      /* let(&prefix, cat(fileName, "-", NULL));
         let(&postfix, "-"); */
      /* Normal: */
      goto skip_backup;

#else /* Unknown; assume unix standard */
      /*if (1) goto skip_backup;*/  /* [if no backup desired] */
      let(&prefix, cat(fileName, "~", NULL));
      let(&postfix, "");

#endif


      /* See if the lowest version already exists. */
      let(&bakName, cat(prefix, str(1), postfix, NULL));
      fp = fopen(bakName, "r");
      if (fp) {
        fclose(fp);
        /* The lowest version already exists; rename all to higher versions. */

        /* Find last version before gap, if any */ /* 29-Apr-2007 nm */
        lastVersion = 0;
        for (v = 1; v <= VERSIONS; v++) {
          let(&bakName, cat(prefix, str((double)v), postfix, NULL));
          fp = fopen(bakName, "r");
          if (!fp) break; /* Version gap found; skip rest of scan */
          fclose(fp);
          lastVersion = v;
        }

        /* If there are no gaps before version VERSIONS, delete it. */
        if (lastVersion == VERSIONS) {  /* 29-Apr-2007 nm */
          let(&bakName, cat(prefix, str((double)VERSIONS), postfix, NULL));
          fp = fopen(bakName, "r");
          if (fp) {
            fclose(fp);
            remove(bakName);
          }
          lastVersion--;  /* 29-Apr-2007 nm */
        }

        for (v = lastVersion; v >= 1; v--) {  /* 29-Apr-2007 nm */
          let(&bakName, cat(prefix, str((double)v), postfix, NULL));
          fp = fopen(bakName, "r");
          if (!fp) continue;
          fclose(fp);
          let(&newBakName, cat(prefix, str((double)v + 1), postfix, NULL));
          rename(bakName/*old*/, newBakName/*new*/);
        }

      }
      let(&bakName, cat(prefix, str(1), postfix, NULL));
      rename(fileName, bakName);

      /***
      printLongLine(cat("The file \"", fileName,
          "\" already exists.  The old file is being renamed to \"",
          bakName, "\".", NULL), "  ", " ");
      ***/
    } /* End if file already exists */
   skip_backup:

    if (!strcmp(mode, "w")) {
      fp = fopen(fileName, "w");
      if (!fp) {
        print2("?Sorry, couldn't open the file \"%s\".\n", fileName);
      }
    } else {
      /* 31-Dec-2017 nm */
      /* For "safe" delete, we simply skip opening the file. */
      if (strcmp(mode, "d")) {
        bug(1526);
      }
      fp = NULL;
    }

    let(&prefix, "");
    let(&postfix, "");
    let(&bakName, "");
    let(&newBakName, "");

    return (fp);
  } /* End if mode = "w" or "d" */

  bug(1510); /* Illegal mode */
  return(NULL);

} /* fSafeOpen() */


/* Renames a file with backup of previous version.  If non-zero
   is returned, there was an error. */
int fSafeRename(vstring oldFileName, vstring newFileName)
{
  int error = 0;
  int i;
  FILE *fp;
  /* Open the new file to force renaming of existing ones */
  fp = fSafeOpen(newFileName, "w", 0/*noVersioningFlag*/);
  if (!fp) error = -1;
  /* Delete the file just created */
  if (!error) {
    error = fclose(fp);
    if (error) print2("?Empty \"%s\" couldn't be closed.\n", newFileName);
  }
  if (!error) {
    error = remove(newFileName);
    /* On Windows 95, the first attempt may not succeed. */
    if (error) {
      for (i = 2; i < 1000; i++) {
        error = remove(newFileName);
        if (!error) break;
      }
      if (!error)
        print2("OS WARNING: File delete succeeded only after %i attempts.", i);
    }

    if (error) print2("?Empty \"%s\" couldn't be deleted.\n", newFileName);
  }
  /* Rename the old one to it */
  if (!error) {
    error = rename(oldFileName, newFileName);
    if (error) print2("?Rename of \"%s\" to \"%s\" failed.\n", oldFileName,
        newFileName);
  }
  if (error) {
    print2("?Sorry, couldn't rename the file \"%s\" to \"%s\".\n", oldFileName,
        newFileName);
    print2("\"%s\" may be empty; try recovering from \"%s\".\n", newFileName,
        oldFileName);
  }
  return error;
} /* fSafeRename */


/* Finds the name of the first file of the form filePrefix +
   nnn + ".tmp" that does not exist.  THE CALLER MUST DEALLOCATE
   THE RETURNED STRING [i.e. assign function return directly
   to a local empty vstring with = and not with let(), e.g.
        let(&str1, "");
        str1 = fTmpName("zz~list");  ]
   The file whose name is the returned string is not left open;
   the caller must separately open the file. */
vstring fGetTmpName(vstring filePrefix)
{
  FILE *fp;
  vstring fname = "";
  static long counter = 0;
  while (1) {
    counter++;
    let(&fname, cat(filePrefix, str((double)counter), ".tmp", NULL));
    fp = fopen(fname, "r");
    if (!fp) break;

    /* Fix resource leak reported by David Binderman 10-Oct-2016 */
    fclose(fp);  /* 10-Oct-2016 nm */

    if (counter > 1000) {
      print2("?Warning: too many %snnn.tmp files - will reuse %s\n",
          filePrefix, fname);
      break;
    }
  }
  return fname; /* Caller must deallocate! */
} /* fGetTmpName() */


/* Added 10/10/02 */
/* This function returns a character string containing the entire contents of
   an ASCII file, or Unicode file with only ASCII characters.   On some
   systems it is faster than reading the file line by line.  The caller
   must deallocate the returned string.  If a NULL is returned, the file
   could not be opened or had a non-ASCII Unicode character or some other
   problem.   If verbose is 0, error and warning messages are suppressed. */
/* 31-Dec-2017 nm Added charCount return arg */
vstring readFileToString(vstring fileName, char verbose, long *charCount) {
  FILE *inputFp;
  long fileBufSize;
  /*long charCount;*/ /* 31-Dec-2017 nm */
  char *fileBuf;
  long i, j;

  /* Find out the upper limit of the number of characters in the file. */
  /* Do this by opening the file in binary and seeking to the end. */
  inputFp = fopen(fileName, "rb");
  if (!inputFp) {
    if (verbose) print2("?Sorry, couldn't open the file \"%s\".\n", fileName);
    return (NULL);
  }
#ifndef SEEK_END
/* An older GCC compiler didn't have this ANSI standard constant defined. */
#define SEEK_END 2
#endif
  if (fseek(inputFp, 0, SEEK_END)) bug(1511);
  fileBufSize = ftell(inputFp);

  /* Close and reopen the input file in text mode */
  /* Text mode is needed for VAX, DOS, etc. with non-Unix end-of-lines */
  fclose(inputFp);
  inputFp = fopen(fileName, "r");
  if (!inputFp) bug(1512);

  /* Allocate space for the entire input file */
  fileBufSize = fileBufSize + 10;
            /* Add a factor for unknown text formats (just a guess) */
  fileBuf = malloc((size_t)fileBufSize);
  if (!fileBuf) {
    if (verbose) print2(
        "?Sorry, there was not enough memory to read the file \"%s\".\n",
        fileName);
    fclose(inputFp);
    return (NULL);
  }

  /* Put the entire input file into the buffer as a giant character string */
  (* charCount) = (long)fread(fileBuf, sizeof(char), (size_t)fileBufSize - 2,
      inputFp);
  if (!feof(inputFp)) {
    print2("Note:  This bug will occur if there is a disk file read error.\n");
    /* If this bug occurs (due to obscure future format such as compressed
       text files) we'll have to add a realloc statement. */
    bug(1513);
  }
  fclose(inputFp);

  fileBuf[(*charCount)] = 0;

  /* See if it's Unicode */
  /* This only handles the case where all chars are in the ASCII subset */
  if ((*charCount) > 1) {
    if (fileBuf[0] == '\377' && fileBuf[1] == '\376') {
      /* Yes, so strip out null high-order bytes */
      if (2 * ((*charCount) / 2) != (*charCount)) {
        if (verbose) print2(
"?Sorry, there are an odd number of characters (%ld) %s \"%s\".\n",
            (*charCount), "in Unicode file", fileName);
        free(fileBuf);
        return (NULL);
      }
      i = 0; /* ASCII character position */
      j = 2; /* Unicode character position */
      while (j < (*charCount)) {
        if (fileBuf[j + 1] != 0) {
          if (verbose) print2(
              "?Sorry, the Unicode file \"%s\" %s %ld at byte %ld.\n",
              fileName, "has a non-ASCII \ncharacter code",
              (long)(fileBuf[j]) + ((long)(fileBuf[j + 1]) * 256), j);
          free(fileBuf);
          return (NULL);
        }
        if (fileBuf[j] == 0) {
          if (verbose) print2(
              "?Sorry, the Unicode file \"%s\" %s at byte %ld.\n",
              fileName, "has a null character", j);
          free(fileBuf);
          return (NULL);
        }
        fileBuf[i] = fileBuf[j];
        /* Suppress any carriage-returns */
        if (fileBuf[i] == '\r') {
          i--;
        }
        i++;
        j = j + 2;
      }
      fileBuf[i] = 0; /* ASCII string terminator */
      (*charCount) = i;
    }
  }

  /* Make sure the file has no carriage-returns */
  if (strchr(fileBuf, '\r') != NULL) {
    if (verbose) print2(
       "?Warning: the file \"%s\" has carriage-returns.  Cleaning them up...\n",
        fileName);
    /* Clean up the file, e.g. DOS or Mac file on Unix */
    i = 0;
    j = 0;
    while (j <= (*charCount)) {
      if (fileBuf[j] == '\r') {
        if (fileBuf[j + 1] == '\n') {
          /* DOS file - skip '\r' */
          j++;
        } else {
          /* Mac file - change '\r' to '\n' */
          fileBuf[j] = '\n';
        }
      }
      fileBuf[i] = fileBuf[j];
      i++;
      j++;
    }
    (*charCount) = i - 1; /* nm 6-Feb-04 */
  }

  /* Make sure the last line is not a partial line */
  if (fileBuf[(*charCount) - 1] != '\n') {
    if (verbose) print2(
        "?Warning: the last line in file \"%s\" is incomplete.\n",
        fileName);
    /* Add the end-of-line */
    fileBuf[(*charCount)] = '\n';
    (*charCount)++;
    fileBuf[(*charCount)] = 0;
  }

  if (fileBuf[(*charCount)] != 0) {  /* nm 6-Feb-04 */
    bug(1522); /* Keeping track of charCount went wrong somewhere */
  }

  /* Make sure there aren't null characters */
  i = (long)strlen(fileBuf);
  if ((*charCount) != i) {
    if (verbose) {
      print2(
          "?Warning: the file \"%s\" is not an ASCII file.\n",
          fileName);
      print2(
          "Its size is %ld characters with null at character %ld.\n",
          (*charCount), strlen(fileBuf));
    }
  }
/*E*/db = db + i;  /* For memory usage tracking (ignore stuff after null) */

  /******* For debugging
  print2("In binary mode the file has %ld bytes.\n", fileBufSize - 10);
  print2("In text mode the file has %ld bytes.\n", (*charCount));
  *******/

  return ((char *)fileBuf);
} /* readFileToString */


/* 16-Aug-2016 nm */
/* Returns total elapsed time in seconds since starting session (for the
   lcc compiler) or the CPU time used (for the gcc compiler).  The
   argument is assigned the time since the last call to this function. */
double getRunTime(double *timeSinceLastCall) {
#ifdef CLOCKS_PER_SEC
  static clock_t timePrevious = 0;
  clock_t timeNow;
  timeNow = clock();
  *timeSinceLastCall = (double)((1.0 * (double)(timeNow - timePrevious))
          /CLOCKS_PER_SEC);
  timePrevious = timeNow;
  return (double)((1.0 * (double)timeNow)/CLOCKS_PER_SEC);
  /* Example of printing double format: */
  /* print2("Total elapsed time = %4.2f s\n", t) */
#else
  print2("The clock() function is not implemented on this computer.\n");
  *timeSinceLastCall = 0;
  return 0;
#endif
}


/* 4-May-2017 Ari Ferrera */
void freeInOu() {
  long i, j;
  j = pntrLen(backBuffer);
  for (i = 0; i < j; i++) let((vstring *)(&backBuffer[i]), "");
  pntrLet(&backBuffer, NULL_PNTRSTRING);
}
