/*****************************************************************************/
/*       Copyright (C) 2002  NORMAN D. MEGILL nm@alum.mit.edu                */
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


int errorCount = 0;

/* Global variables used by print2() */
flag logFileOpenFlag = 0;
FILE *logFilePtr;
FILE *listFile_fp = NULL;
/* Global variables used by print2() */
flag outputToString = 0;
vstring printString = "";
/* Global variables used by cmdInput() */
flag commandFileOpenFlag = 0;
FILE *commandFilePtr;
vstring commandFileName = "";

FILE *inputDef_fp,*input_fp,*output_fp; /* File pointers */
vstring inputDef_fn="",input_fn="",output_fn="";        /* File names */

long screenWidth = 79; /* Screen width */
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

  if (backBufferPos == 0) {
    /* Initialize backBuffer - 1st time in program */
    /* Warning:  Don't call bug(), because it calls print2. */
    if (pntrLen(backBuffer)) printf("*** BUG #1501\n");
    backBufferPos = 1;
    pntrLet(&backBuffer, pntrAddElement(backBuffer));
  }

  if ((!quitPrint && !commandFileOpenFlag && (scrollMode == 1
           && localScrollMode == 1)
      && printedLines >= SCREEN_HEIGHT && !outputToString)
      || backFromCmdInput) {
    /* It requires a scrolling prompt */
    while(1) {
      if (backFromCmdInput && backBufferPos == pntrLen(backBuffer))
        break; /* Exhausted buffer */
      if (backBufferPos < 1 || backBufferPos > pntrLen(backBuffer))
        /* Warning:  Don't call bug(), because it calls print2. */
        printf("*** BUG #1502 %ld\n", backBufferPos);
      if (backBufferPos == 1) {
        printf(
"Press <return> for more, Q <return> to quit, S <return> to scroll to end... "
          );
      } else {
        printf(
"Press <return> for more, Q <ret> quit, S <ret> scroll, B <ret> back up... "
         );
      }
      c = getchar();
      if (c == '\n') {
        if (backBufferPos == pntrLen(backBuffer)) {
          /* Normal output */
          break;
        } else {
          /* Get output from buffer */
          backBufferPos++;
          printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
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
            for (backBufferPos = backBufferPos + 1; backBufferPos <=
                pntrLen(backBuffer); backBufferPos++) {
              printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
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
            continue;
          }
        }

        printf("%c", 7); /* Bell */
        continue;
      }
      while (c != '\n') c = getchar();
    } /* While 1 */
    if (backFromCmdInput) return (!quitPrint);
    printedLines = 0; /* Reset the number of lines printed on the screen */
    if (!quitPrint) {
      backBufferPos++;
      pntrLet(&backBuffer, pntrAddElement(backBuffer));
    }
  }



  if (quitPrint && !outputToString) return (!quitPrint); /* User typed 'q'
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
  }

  nlpos = instr(1, printBuffer, "\n");
  lineLen = strlen(printBuffer);
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
    return (!quitPrint);
  }

  if (!outputToString) {
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

#ifdef __STDC__
      /*
      The following change to mminou.c was necessary on my Unix (Linux)
      system to get the `verify proof *' progress bar to display
      progressively (rather than all at once). I've conditionalized it on
      __STDC__, since it should be harmless on any ANSI C system.
        -- Stephen McCamant smccam@uclink4.berkeley.edu 12/9/00
      */
      fflush(stdout);
#endif

    } else {
      printf("%s", printBuffer); /* Normal line */
      printedLines++;
      if (!(scrollMode == 1 && localScrollMode == 1)) {
        /* Even in non-scroll (continuous output) mode, still put paged-mode
           lines into backBuffer in case user types a "B" command later,
           so user can page back from end. */
        if (printedLines > SCREEN_HEIGHT) {
          printedLines = 1;
          backBufferPos++;
          pntrLet(&backBuffer, pntrAddElement(backBuffer));
        }
      }
    }
    /* Add line to backBuffer string array */
    /* Warning:  Don't call bug(), because it calls print2. */
    if (backBufferPos < 1) printf("*** PROGRAM BUG #1504\n");
    let((vstring *)(&(backBuffer[backBufferPos - 1])), cat(
        (vstring)(backBuffer[backBufferPos - 1]), printBuffer, NULL));
  } /* End if !outputToString */

  if (logFileOpenFlag && !outputToString) {
    fprintf(logFilePtr, "%s", printBuffer);  /* Print to log file */
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
    printf("*** PROGRAM BUG #1505\n");
    printf("%ld %s\n", lineLen, printBuffer);
    /*printf(NULL);*/  /* Force crash on VAXC to see where it came from */
  }
  /* \n not allowed in middle of line */
  /* Warning:  Don't call bug(), because it calls print2. */
  if (nlpos != 0 && nlpos != lineLen) printf("*** PROGRAM BUG #1506\n");
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
void printLongLine(vstring line, vstring startNextLine, vstring breakMatch)
{
  vstring tmp = "";
  vstring tmpStr = "";
  vstring tmpStr1 = "";
  vstring tmpStr2 = "";
  vstring startNextLine1 = "";
  long p, savep;
  long startNextLineLen;
  flag firstLine;
  flag tildeFlag = 0;
  flag treeIndentationFlag = 0;

  long saveTempAllocStack;

  /* Blank line (the rest of algorithm would ignored it, so output & return) */
  if (!line[0]) {
    print2("\n");
    return;
  }

  /* Flag to right justify continuation lines */
  if (breakMatch[0] == 1) {
    treeIndentationFlag = 1;
    breakMatch[0] = ' '; /* Change to a space (the real break character) */
  }


  /* Change the stack allocation start to prevent arguments from being
     deallocated */
  saveTempAllocStack = startTempAllocStack;
  startTempAllocStack = tempAllocStackTop; /* For let() stack cleanup */

  let(&tmpStr1,line);

   /* The tilde is a special flag for printLongLine to print a
      tilde before the carriage return in a split line, not after */
  if (startNextLine[0] == '~') {
    tildeFlag = 1;
    let(&startNextLine1," ");
  } else {
    let(&startNextLine1,startNextLine);
  }


  while (tmpStr1[0]) {

    /* Process caller-inserted newlines */
    p = instr(1, tmpStr1, "\n");
    if (p) {
      let(&tmpStr, left(tmpStr1, p - 1));
      let(&tmpStr1, right(tmpStr1, p + 1));
    } else {
      let(&tmpStr, tmpStr1);
      let(&tmpStr1, "");
    }
    firstLine = 1;

    startNextLineLen = strlen(startNextLine1);
    /* Prevent infinite loop if next line prefix is longer that screen */
    if (startNextLineLen > screenWidth - 4) {
      startNextLineLen = screenWidth - 4;
      let(&startNextLine1, left(startNextLine1, screenWidth - 4));
    }
    while (strlen(tmpStr) + (1 - firstLine) * startNextLineLen >
        screenWidth - (long)tildeFlag - (long)(breakMatch[0] == '\\')) {
      p = screenWidth - (long)tildeFlag - (long)(breakMatch[0] == '\\') + 1;
      if (!firstLine) p = p - startNextLineLen;

      /* Assume compressed proof if 1st char of breakMatch is "&" */
      if (breakMatch[0] == '&'
          && !instr(p, left(tmpStr, strlen(tmpStr) - 3), " ")
          && tmpStr[p - 3] != ' ') /* Don't split trailing "$." */ {
        /* We're in the compressed proof section; break line anywhere */
        p = p;
      } else {
        if (!breakMatch[0]) {
          p = p; /* Break line anywhere */
        } else {
          if (breakMatch[0] == '&') {
            /* Compressed proof */
            p = p - 1; /* We will add a trailing space to line for easier
                          label searches by the user during editing */
          }
          /* Normal long line */
          /* p > 0 prevents infinite loop */
          savep = p;
          while (!instr(1, breakMatch, mid(tmpStr,p,1)) && p > 0) {
            p--;
            let(&tmp, ""); /* Clear temp alloc stack from 'mid' call */
          }
          /* 2/8/02 If a break point was not found, see if we can break just before a ">".
             This partially fixes some very long HTML strings without spaces, such as
             the hypothesis list of gomaex3 in ql.mm, preventing broken HTML tags.
             This is not a guaranteed fix, but the case is rare (hopefully). */
          /* 2/8/02 (later) - fixed another way for this case; see 2/8/02 comment
             in mmwtex.c.  However leave it in case of some other future bizarre problem,
             like the htmldefs in set.mm not having any spaces */
          if (p <= 0 && breakMatch[0] == ' ') {
            p = savep;
            while (!instr(1, ">", mid(tmpStr,p,1)) && p > 0) {
              p--;
              let(&tmp, ""); /* Clear temp alloc stack from 'mid' call */
            }
          }
          if (breakMatch[0] == '&') {
            /* Compressed proof */
            p = p + 1; /* We will add a trailing space to line for easier
                          label searches by the user during editing */
          }
        }
      }

      if (p <= 0) {
        /* Break character not found; give up at
           screenWidth - (long)tildeFlag  - (long)(breakMatch[0] == '\\')+ 1 */
        p = screenWidth - (long)tildeFlag  - (long)(breakMatch[0] == '\\')+ 1;
        if (!firstLine) p = p - startNextLineLen;
        if (p <= 0) p = 1; /* If startNextLine too long */
      }
      if (firstLine) {
        firstLine = 0;
        let(&tmpStr2, "");
      } else {
        let(&tmpStr2, startNextLine1);
        if (treeIndentationFlag) {
          if (startNextLineLen + p - 1 < screenWidth) {
            /* Right justify output for continuation lines */
            let(&tmpStr2, cat(tmpStr2, space(screenWidth - startNextLineLen
                - p + 1), NULL));
          }
        }
      }
      if (!tildeFlag) {
        if (breakMatch[0] == '\\') {
          /* Add LaTeX comment char to ignore carriage return */
          print2("%s\n",cat(tmpStr2, left(tmpStr,p - 1), "%", NULL));
        } else {
          print2("%s\n",cat(tmpStr2, left(tmpStr,p - 1), NULL));
        }
      } else {
        print2("%s\n",cat(tmpStr2, left(tmpStr,p - 1), "~", NULL));
      }
      if (tmpStr[p - 1] == ' ' &&
          breakMatch[0] /* But not "break anywhere" line */) {
        /* Remove leading space for neatness */
        if (tmpStr[p] == ' ') {
          /* There could be 2 spaces at the end of a sentence. */
          let(&tmpStr, right(tmpStr, p + 2));
        } else {
          let(&tmpStr, right(tmpStr,p + 1));
        }
      } else {
        let(&tmpStr, right(tmpStr,p));
      }
    } /* end while tmpStr too long */
    if (!firstLine) {
      if (treeIndentationFlag) {
        /* Right justify output for continuation lines */
        print2("%s\n",cat(startNextLine1, space(screenWidth
              - startNextLineLen - strlen(tmpStr)), tmpStr, NULL));
      } else {
        print2("%s\n",cat(startNextLine1, tmpStr, NULL));
      }
    } else {
      print2("%s\n",tmpStr);
    }

  } /* end while tmpStr1 != "" */

  let(&tmpStr, ""); /* Deallocate */
  let(&tmpStr2, ""); /* Deallocate */
  let(&startNextLine1, ""); /* Deallocate */


  startTempAllocStack = saveTempAllocStack;
  return;
}


vstring cmdInput(FILE *stream, vstring ask)
{
  /* This function prints a prompt (if 'ask' is not NULL) and gets a line from
    the stream.  NULL is returned when end-of-file is encountered.
    New memory is allocated each time linput is called.  This space must
    be freed by the user if not needed. */
  vstring g = "";
#define CMD_BUFFER 2000

  while (1) { /* For "B" backup loop */
    if (ask) printf("%s",ask);
    let(&g, space(CMD_BUFFER)); /* Allow for up to CMD_BUFFER characters */
    if (!fgets(g, CMD_BUFFER, stream)) {
      /* End of file */
      return NULL;
    }
    /* Warning:  Don't call bug(), because it calls print2. */
    if (!strlen(g)) printf("*** BUG #1507\n");
    /* Warning:  Don't call bug(), because it calls print2. */
    if (strlen(g) > CMD_BUFFER - 1) printf("*** BUG #1508\n");
/*E*/db = db - (CMD_BUFFER - strlen(g));
    if (g[1]) {
      g[strlen(g)-1]=0;   /* Eliminate new-line character */
/*E*/db = db - 1;
    } else {
      let(&g, ""); /* Deallocate vstring space (otherwise empty string will
                      trick let() into not doing it) */
    }

    /* If user typed "B" (for backup), go back to the backup buffer to
       let the user scroll through it */
    if ((!strcmp(g, "B") || !strcmp(g, "b")) /* User typed "B" */
        && pntrLen(backBuffer) > 1   /* The back-buffer still exists and
                                         there was a previous page */
        && !commandFileOpenFlag
        && (scrollMode == 1 && localScrollMode == 1)
        && !outputToString) {
      /* Set variables so only backup buffer will be looked at in print2() */
      backBufferPos = pntrLen(backBuffer) - 1;
      printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
      backFromCmdInput = 1; /* Flag for print2() */
      print2(""); /* Only the backup buffer will be looked at */
      backFromCmdInput = 0;
    } else {
      /* If the command line is empty (at main prompt), let user still
         type "B" for convenience in case too many
         returns where hit while scrolling */
      if (g[0] ||  /* Command line not empty */
          /* Or user entered empty command line but not at a prompt */
          /* commandPrompt is assigned in metamath.c and declared in
             mmcmdl.h */
          strcmp(ask, commandPrompt)) {
        break; /* Break out of loop that looks for "B" */
      }
    }
  } /* while 1 */

  return g;
}

vstring cmdInput1(vstring ask)
{
  /* This function gets a line from either the terminal or the command file
    stream depending on commandFileOpenFlag.  It calls cmdInput(). */
  /* Warning: the calling program must deallocate the returned string. */
  vstring commandLine = "";
  vstring ask1 = "";
  long p, i;

  let(&ask1, ask);
  /* Look for lines too long */
  while (strlen(ask1) > screenWidth) {
    p = screenWidth - 1;
    while (ask1[p] != ' ' && p > 0) p--;
    if (!p) p = screenWidth - 1;
    print2("%s\n", left(ask1, p));
    let(&ask1, right(ask1, p + 1));
  }
  /* Allow 10 characters for answer */
  if (strlen(ask1) > screenWidth - 10) {
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
    if (!commandFileOpenFlag) {
      commandLine = cmdInput(stdin,ask1);
      if (!commandLine) {
        commandLine = ""; /* Init vstring (was NULL) */
        let(&commandLine, "^Z"); /* ^Z found */
      }
      if (logFileOpenFlag) fprintf(logFilePtr, "%s%s\n", ask1, commandLine);

      /* Clear backBuffer from previous scroll session */
        for (i = 0; i < pntrLen(backBuffer); i++) {
        let((vstring *)(&(backBuffer[i])), "");
      }
      backBufferPos = 1;
      pntrLet(&backBuffer, NULL_PNTRSTRING);
      pntrLet(&backBuffer, pntrAddElement(backBuffer));

      /* Add user's typing to the backup buffer for display on 1st screen */
      let((vstring *)(&(backBuffer[backBufferPos - 1])), cat(
          (vstring)(backBuffer[backBufferPos - 1]), ask1,
          commandLine, "\n", NULL));

      if (listMode && listFile_fp != NULL) {
        /* Put line in list.tmp as comment */
        fprintf(listFile_fp, "! %s\n", commandLine);
      }

    } else {
      commandLine = cmdInput(commandFilePtr, NULL);
      if (!commandLine) { /* EOF found */
        fclose(commandFilePtr);
        print2("%s[End of command file \"%s\".]\n", ask1, commandFileName);
        commandFileOpenFlag = 0;
        commandLine = "";
        break; /*continue;*/
      }
      print2("%s%s\n", ask1, commandLine);
    }
    break;
  }

  return commandLine;
}


void errorMessage(vstring line, long lineNum, short column, short tokenLength,
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
    case (char)_notice:
      let(&prntStr, "?Notice"); break;
    case (char)_warning:
      let(&prntStr, "?Warning"); break;
    case (char)_error:
      let(&prntStr, "?Error"); break;
    case (char)_fatal:
      let(&prntStr, "?Fatal error"); break;
  }
  if (lineNum) {
    let(&prntStr, cat(prntStr, " on line ", str(lineNum), NULL));
    if (fileName)
      let(&prntStr, cat(prntStr, " of file \"", fileName, "\"", NULL));
  } else {
    if (fileName)
      let(&prntStr, cat(prntStr, " in file \"", fileName, "\"", NULL));
  }
  if (statementNum) {
    let(&prntStr, cat(prntStr, " at statement ", str(statementNum), NULL));
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
    print2("\n"); print2("?Too many errors - aborting MetaMath.\n");
    exit(0);
  } */

  if (severity == 3) {
    print2("Aborting MetaMath.\n");
    exit(0);
  }
  let(&errorPointer,"");
  let(&tmpStr,"");
  let(&prntStr,"");
  let(&error,"");
  if (line1) let(&line1,"");
}




/* Opens files with error message; opens output files with
   backup of previous version.   Mode must be "r" or "w". */
FILE *fSafeOpen(vstring fileName, vstring mode)
{
  FILE *fp;
  vstring prefix = "";
  vstring postfix = "";
  vstring bakName = "";
  vstring newBakName = "";
  long v;

  if (!strcmp(mode, "r")) {
    fp = fopen(fileName, "r");
    if (!fp) {
      print2("?Sorry, couldn't open the file \"%s\".\n", fileName);
    }
    return (fp);
  }

  if (!strcmp(mode, "w")) {
    /* See if the file already exists. */
    fp = fopen(fileName, "r");

    if (fp) {
      fclose(fp);

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
        /* The lowest version already exists; rename all to lower versions. */

        /* If version VERSIONS exists, delete it. */
        let(&bakName, cat(prefix, str(VERSIONS), postfix, NULL));
        fp = fopen(bakName, "r");
        if (fp) {
          fclose(fp);
          remove(bakName);
        }

        for (v = VERSIONS - 1; v >= 1; v--) {
          let(&bakName, cat(prefix, str(v), postfix, NULL));
          fp = fopen(bakName, "r");
          if (!fp) continue;
          fclose(fp);
          let(&newBakName, cat(prefix, str(v + 1), postfix, NULL));
          rename(bakName, newBakName);
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
   /*skip_backup:*/

    fp = fopen(fileName, "w");
    if (!fp) {
      print2("?Sorry, couldn't open the file \"%s\".\n", fileName);
    }

    let(&prefix, "");
    let(&postfix, "");
    let(&bakName, "");
    let(&newBakName, "");

    return (fp);
  } /* End if mode = "w" */

  bug(1510); /* Illegal mode */
  return(NULL);

}


/* Renames a file with backup of previous version.  If non-zero
   is returned, there was an error. */
int fSafeRename(vstring oldFileName, vstring newFileName)
{
  int error = 0;
  int i;
  FILE *fp;
  /* Open the new file to force renaming of existing ones */
  fp = fSafeOpen(newFileName, "w");
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
}


/* Finds the name of the first file of the form filePrefix +
   nnn + ".tmp" that does not exist.  THE CALLER MUST DEALLOCATE
   THE RETURNED STRING [i.e. assign function return directly
   to a local empty vstring with = and not with let(), e.g.
        let(&str1, "");
        str1 = fTmpName("zz~list");  ] */
vstring fGetTmpName(vstring filePrefix)
{
  FILE *fp;
  vstring fname = "";
  static long counter = 0;
  while (1) {
    counter++;
    let(&fname, cat(filePrefix, str(counter), ".tmp", NULL));
    fp = fopen(fname, "r");
    if (!fp) break;
    if (counter > 1000) {
      print2("Warning: too many %snnn.tmp files - will reuse %s\n",
          filePrefix, fname);
      break;
    }
  }
  return fname; /* Caller must deallocate! */
}
