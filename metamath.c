/*****************************************************************************/
/*        Copyright (C) 2004  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#define MVERSION "0.07v 27-Feb-04"
/* Metamath Proof Verifier - main program */
/* See the book "Metamath" for description of Metamath and run instructions */

/* The overall functionality of the modules is as follows:
    metamath.c - Contains main(); executes or calls commands
    mmcmdl.c - Command line interpreter
    mmcmds.c - Extends metamath.c command() to execute SHOW and other
               commands; added after command() became too bloated (still is:)
    mmdata.c - Defines global data structures and manipulates arrays
               with functions similar to BASIC string functions;
               memory management; converts between proof formats
    mmhlpa.c - The help file, part 1.
    mmhlpb.c - The help file, part 2.
    mminou.c - Basic input and output interface
    mmmaci.c - THINK C Macintosh interface (probably obsolete now)
    mmpars.c - Parses the source file
    mmpfas.c - Proof Assistant
    mmunif.c - Unification algorithm for Proof Assistant
    mmutil.c - Miscellaneous I/O utilities for non-ANSI compilers (has become
               obsolete and is now an empty shell)
    mmveri.c - Proof verifier for source file
    mmvstr.c - BASIC-like string functions
    mmwtex.c - LaTeX/HTML source generation
    mmword.c - File revision utility (for TOOLS utility) (not generally useful)
*/

/*****************************************************************************/
/* ------------- Compilation Instructions ---------------------------------- */
/*****************************************************************************/

/* These are the instructions for the gcc compiler (standard under Linux, and
   available on the web as the djgpp package for Windows).
   1. Make sure each .c file is present in the compilation directory, and that
      each .c file (except metamath.c) has its corresponding .h file present.
   2. In the directory where these files are present, type:
      For Linux:  gcc metamath.c mm*.c -o metamath
      For Windows (in a DOS box):  gcc metamath.c mm*.c -o metamath.exe
   3. For better speed, you may optionally include the -O switch after gcc.
*/


/*****************************************************************************/


/*----------------------------------------------------------------------*/


#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#ifdef THINK_C
#include <console.h>
#endif
#include "mmutil.h"
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmdl.h"
#include "mmcmds.h"
#include "mmhlpa.h"
#include "mmhlpb.h"
#include "mminou.h"
#include "mmpars.h"
#include "mmveri.h"
#include "mmpfas.h"
#include "mmunif.h"
#include "mmword.h"
#include "mmwtex.h"
#ifdef THINK_C
#include "mmmaci.h"
#endif

void command(int argc, char *argv[]);

int qsortStringCmp(const void *p1, const void *p2);
vstring qsortKey; /* Pointer only; do not deallocate */

int main(int argc, char *argv[])
{

/* argc is the number of arguments; argv points to an array containing them */
#ifdef THINK_C
/* Set console attributes */
console_options.pause_atexit = 0; /* No pause at exit */
console_options.title = (unsigned char*)"\pMetamath";
#endif

#ifdef THINK_C
  /* The standard stream triggers the console package to initialize the
     Macintosh Toolbox managers and use the console interface.  cshow must
     be called before using our own window to prevent crashing (THINK C
     Standard Library Reference p. 43). */
  cshow(stdout);
  /* Initialize MacIntosh interface */
  /*ToolBoxInit(); */ /* cshow did this automatically */
  /* Display opening window */
  /*
  WindowInit();
  DrawMyPicture();
  */
  /* Wait for mouse click or key */
  /*while (!Button());*/
#endif


  /****** If listMode is set to 1 here, the startup will be Text Tools
          utilities, and Metamath will be disabled ***************************/
  listMode = 0; /* Force Metamath mode as startup */


  toolsMode = listMode;

  if (!listMode) {
    /*print2("Metamath - Version %s\n", MVERSION);*/
    print2("Metamath - Version %s%s", MVERSION, space(25 - strlen(MVERSION)));
  }
  /* if (argc < 2) */ print2("Type HELP for help, EXIT to exit.\n");

  /* Allocate big arrays */
  initBigArrays();

  /* Process a command line until EXIT */
  command(argc, argv);

  /* Close logging command file */
  if (listMode && listFile_fp != NULL) {
    fclose(listFile_fp);
  }

  return 0;

}



void command(int argc, char *argv[])
{
  /* Command line user interface -- this is an infinite loop; it fetches and
     processes a command; returns only if the command is 'EXIT' or 'QUIT' and
     never returns otherwise. */
  long argsProcessed = 0;  /* Number of argv initial command-line
                                     arguments processed so far */

  /* The variables in command() are static so that they won't be destroyed
    by a longjmp return to setjmp. */
  long i, j, k, m, l, n, p, q, s /*,tokenNum*/;
  vstring str1 = "", str2 = "", str3 = "", str4 = "";
  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated directly */
  nmbrString *nmbrTmp = NULL_NMBRSTRING;
  nmbrString *nmbrSaveProof = NULL_NMBRSTRING;
  /*pntrString *pntrTmpPtr;*/ /* Pointer only; not allocated directly */
  /*pntrString *pntrTmpPtr1;*/ /* Pointer only; not allocated directly */
  /*pntrString *pntrTmpPtr2;*/ /* Pointer only; not allocated directly */
  pntrString *pntrTmp = NULL_PNTRSTRING;
  pntrString *expandedProof = NULL_PNTRSTRING;
  flag tmpFlag;

  /* Variables for SHOW PROOF */
  flag pipFlag; /* Proof-in-progress flag */
  long startStep; long endStep;
  long startIndent; long endIndent; /* Also for SHOW TRACE_BACK */
  flag essentialFlag; /* Also for SHOW TRACE_BACK */
  flag renumberFlag; /* Flag to use essential step numbering */
  flag unknownFlag;
  flag notUnifiedFlag;
  flag reverseFlag;
  long detailStep;
  flag noIndentFlag; /* Flag to use non-indented display */
  long splitColumn; /* Column at which formula starts in non-indented display */
  flag texFlag; /* Flag for TeX */
  flag saveFlag; /* Flag to save in source */
  long indentation; /* Number of spaces to indent proof */
  vstring labelMatch = ""; /* SHOW PROOF <label> argument */

  flag axiomFlag; /* For SHOW TRACE_BACK */
  flag recursiveFlag; /* For SHOW USAGE */
  long fromLine, toLine; /* For TYPE, SEARCH */
  long searchWindow; /* For SEARCH */
  FILE *type_fp; /* For TYPE, SEARCH */
  long maxEssential; /* For MATCH */
  nmbrString *essentialFlags = NULL_NMBRSTRING; /* For ASSIGN/IMPROVE LAST */

  flag texHeaderFlag; /* For OPEN TEX, CLOSE TEX */
  flag commentOnlyFlag; /* For SHOW STATEMENT */
  flag briefFlag; /* For SHOW STATEMENT */
  flag linearFlag; /* For SHOW LABELS */

  /* toolsMode-specific variables */
  flag commandProcessedFlag = 0; /* Set when the first command line processed;
                                    used to exit shell command line mode */
  FILE *list1_fp;
  FILE *list2_fp;
  FILE *list3_fp;
  vstring list2_fname = "", list2_ftmpname = "";
  vstring list3_ftmpname = "";
  vstring oldstr = "", newstr = "";
  long lines, changedLines, oldChangedLines, twoMatches, p1, p2;
  long firstChangedLine;
  flag cmdMode, changedFlag, outMsgFlag;
  double sum;
  vstring bufferedLine = "";

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  p = 0;
  q = 0;
  s = 0;
  texHeaderFlag = 0;
  firstChangedLine = 0;

  while (1) {

    if (listMode) {
      /* If called from the OS shell with arguments, do one command
         then exit program. */
      /* (However, let a SUBMIT job complete) */
      if (argc > 1 && commandProcessedFlag && !commandFileOpenFlag) return;
    }

    errorCount = 0; /* Reset error count before each read or proof parse. */

    /* Deallocate stuff that may have been used in previous pass */
    let(&str1,"");
    let(&str2,"");
    let(&str3,"");
    let(&str4,"");
    nmbrLet(&nmbrTmp, NULL_NMBRSTRING);
    pntrLet(&pntrTmp, NULL_PNTRSTRING);
    nmbrLet(&nmbrSaveProof, NULL_NMBRSTRING);
    nmbrLet(&essentialFlags, NULL_NMBRSTRING);
    j = nmbrLen(rawArgNmbr);
    if (j != rawArgs) bug(1110);
    j = pntrLen(rawArgPntr);
    if (j != rawArgs) bug(1111);
    rawArgs = 0;
    for (i = 0; i < j; i++) let((vstring *)(&rawArgPntr[i]), "");
    pntrLet(&rawArgPntr, NULL_PNTRSTRING);
    nmbrLet(&rawArgNmbr, NULL_NMBRSTRING);
    j = pntrLen(fullArg);
    for (i = 0; i < j; i++) let((vstring *)(&fullArg[i]),"");
    pntrLet(&fullArg,NULL_PNTRSTRING);
    j = pntrLen(expandedProof);
    if (j) {
      for (i = 0; i < j; i++) {
        let((vstring *)(&expandedProof[i]),"");
      }
     pntrLet(&expandedProof,NULL_PNTRSTRING);
    }

    let(&list2_fname, "");
    let(&list2_ftmpname, "");
    let(&list3_ftmpname, "");
    let(&oldstr, "");
    let(&newstr, "");
    let(&labelMatch, "");
    /* (End of space deallocation) */

    midiFlag = 0; /* 8/28/00 Initialize here in case SHOW PROOF exits early */

    if (memoryStatus) {
      /*??? Change to user-friendly message */
#ifdef THINK_C
      print2("Memory:  string %ld xxxString %ld free %ld\n",db,db3,(long)FreeMem());
      getPoolStats(&i, &j, &k);
      print2("Pool:  free alloc %ld  used alloc %ld  used actual %ld\n",i,j,k);
#else
      print2("Memory:  string %ld xxxString %ld\n",db,db3);
#endif
      getPoolStats(&i, &j, &k);
      print2("Pool:  free alloc %ld  used alloc %ld  used actual %ld\n",i,j,k);
    }

    if (!toolsMode) {
      if (PFASmode) {
        let(&commandPrompt,"MM-PA> ");
      } else {
        let(&commandPrompt,"MM> ");
      }
    } else {
      if (listMode) {
        let(&commandPrompt,"Tools> ");
      } else {
        let(&commandPrompt,"TOOLS> ");
      }
    }

    let(&commandLine,""); /* Deallocate previous contents */

    if (!commandProcessedFlag && argc > 1 && argsProcessed < argc - 1
        && !commandFileOpenFlag) {
      if (toolsMode) {
        /* If program was compiled in TOOLS mode, the command-line argument
           is assumed to be a single TOOLS command; build the equivalent
           TOOLS command */
        for (i = 1; i < argc; i++) {
          argsProcessed++;
          /* Put quotes around an argument with spaces or tabs or quotes
             or empty string */
          if (instr(1, argv[i], " ") || instr(1, argv[i], "\t")
              || instr(1, argv[i], "\"") || instr(1, argv[i], "'")
              || (argv[i])[0] == 0) {
            /* If it contains a double quote, use a single quote */
            if (instr(1, argv[i], "\"")) {
              let(&str1, cat("'", argv[i], "'", NULL));
            } else {
              /* (??? (TODO)Case of both ' and " is not handled) */
              let(&str1, cat("\"", argv[i], "\"", NULL));
            }
          } else {
            let(&str1, argv[i]);
          }
          let(&commandLine, cat(commandLine, (i == 1) ? "" : " ", str1, NULL));
        }
      } else {
        /* If program was compiled in default (Metamath) mode, each command-line
           argument is considered a full Metamath command.  User is responsible
           for ensuring necessary quotes around arguments are passed in. */
        argsProcessed++;
        scrollMode = 0; /* Set continuous scrolling until completed */
        let(&commandLine, cat(commandLine, argv[argsProcessed], NULL));
        if (argc == 2 && instr(1, argv[1], " ") == 0) {
          /* Assume the user intended a READ command.  This special mode allows
             invocation via "metamath xxx.mm". */
          if (instr(1, commandLine, "\"") || instr(1, commandLine, "'")) {
            /* If it already has quotes don't put quotes */
            let(&commandLine, cat("READ ", commandLine, NULL));
          } else {
            /* Put quotes so / won't be interpreted as qualifier separator */
            let(&commandLine, cat("READ \"", commandLine, "\"", NULL));
          }
        }
      }
      print2("%s\n", cat(commandPrompt, commandLine, NULL));
    } else {
      /* Get command from user input or SUBMIT script file */
      commandLine = cmdInput1(commandPrompt);
    }
    if (argsProcessed == argc && !commandProcessedFlag) {
      commandProcessedFlag = 1;
      scrollMode = 1; /* Set prompted (default) scroll mode */
    }
    if (argsProcessed == argc - 1) {
      argsProcessed++; /* Indicates restore scroll mode next time around */
      if (toolsMode) {
        /* If program was compiled in TOOLS mode, we're only going to execute
           one command; set flag to exit next time around */
        commandProcessedFlag = 1;
      }
    }

    /* See if it's an operating system command */
    /* (This is a command line that begins with a quote) */
    if (commandLine[0] == '\'' || commandLine[0] == '\"') {
      /* See if this computer has this feature */
      if (!system(NULL)) {
        print2("?This computer does not accept an operating system command.\n");
        continue;
      } else {
        /* Strip off quote and trailing quote if any */
        let(&str1, right(commandLine, 2));
        if (commandLine[0]) { /* (Prevent stray pointer if empty string) */
          if (commandLine[0] == commandLine[strlen(commandLine) - 1]) {
            let(&str1, left(str1, strlen(str1) - 1));
          }
        }
        /* Do the operating system command */
        system(str1);
#ifdef VAXC
        printf("\n"); /* Last line from VAX doesn't have new line */
#endif
        continue;
      }
    }

    parseCommandLine(commandLine);
    if (rawArgs == 0) continue; /* Empty or comment line */
    if (!processCommandLine()) continue;

    if (commandEcho || (toolsMode && listFile_fp != NULL)) {
      /* Build the complete command and print it for the user */
      k = pntrLen(fullArg);
      let(&str1,"");
      for (i = 0; i < k; i++) {
        if (instr(1, fullArg[i], " ") || instr(1, fullArg[i], "\t")
            || instr(1, fullArg[i], "\"") || instr(1, fullArg[i], "'")
            || ((char *)(fullArg[i]))[0] == 0) {
          /* If the argument has spaces or tabs or quotes
             or is empty string, put quotes around it */
          if (instr(1, fullArg[i], "\"")) {
            let(&str1, cat(str1, "'", fullArg[i], "' ", NULL));
          } else {
            /* (???Case of both ' and " is not handled) */
            let(&str1, cat(str1, "\"", fullArg[i], "\" ", NULL));
          }
        } else {
          let(&str1, cat(str1, fullArg[i], " ", NULL));
        }
      }
      let(&str1, left(str1,strlen(str1) - 1)); /* Trim trailing space */
      /* The tilde is a special flag for printLongLine to print a
         tilde before the carriage return in a split line, not after */
      if (commandEcho) printLongLine(str1, "~", " ");
      if (toolsMode && listFile_fp != NULL) {
        /* Put line in list.tmp as command */
        fprintf(listFile_fp, "%s\n", str1);  /* Print to list command file */
      }

    }

    if (cmdMatches("BEEP") || cmdMatches("B")) {
      /* Print a bell (if user types ahead "B", the bell lets him know when
         his command is finished - useful for long-running commands */
      print2("%c",7);
      continue;
    }

    if (cmdMatches("HELP")) {
      /* Build the complete command */
      k = pntrLen(fullArg);
      let(&str1,"");
      for (i = 0; i < k; i++) {
        let(&str1, cat(str1, fullArg[i], " ", NULL));
      }
      if (toolsMode) {
        help0(left(str1, strlen(str1) - 1));
        help1(left(str1, strlen(str1) - 1));
      } else {
        help1(left(str1, strlen(str1) - 1));
        help2(left(str1, strlen(str1) - 1));
      }
      continue;
    }


    if (cmdMatches("SET SCROLL")) {
      if (cmdMatches("SET SCROLL CONTINUOUS")) {
        scrollMode = 0;
        print2("Continuous scrolling is now in effect.\n");
      } else {
        scrollMode = 1;
        print2("Prompted scrolling is now in effect.\n");
      }
      continue;
    }

    if (cmdMatches("EXIT") || cmdMatches("QUIT")) {
    /*???        || !strcmp(cmd,"^Z")) { */

      if (toolsMode && !listMode) {
        /* Quitting tools command from within Metamath */
        if (!PFASmode) {
          print2(
 "Exiting the Text Tools.  Type EXIT again to exit Metamath.\n");
        } else {
          print2(
 "Exiting the Text Tools.  Type EXIT again to exit the Proof Assistant.\n");
        }
        toolsMode = 0;
        continue;
      }

      if (PFASmode) {

        if (proofChanged) {
          print2("Warning:  You have not saved changes to the proof.\n");
          str1 = cmdInput1("Do you want to EXIT anyway (Y, N) <N>? ");
          if (str1[0] != 'y' && str1[0] != 'Y') {
            print2("Use SAVE NEW_PROOF to save the proof.\n");
            continue;
          }
          proofChanged = 0;
        }

        print2(
 "Exiting the Proof Assistant.  Type EXIT again to exit Metamath.\n");

        /* Deallocate proof structure */
        i = nmbrLen(proofInProgress.proof);
        nmbrLet(&proofInProgress.proof, NULL_NMBRSTRING);
        for (j = 0; j < i; j++) {
          nmbrLet((nmbrString **)(&(proofInProgress.target[j])),
              NULL_NMBRSTRING);
          nmbrLet((nmbrString **)(&(proofInProgress.source[j])),
              NULL_NMBRSTRING);
          nmbrLet((nmbrString **)(&(proofInProgress.user[j])),
              NULL_NMBRSTRING);
        }
        pntrLet(&proofInProgress.target, NULL_PNTRSTRING);
        pntrLet(&proofInProgress.source, NULL_PNTRSTRING);
        pntrLet(&proofInProgress.user, NULL_PNTRSTRING);

        PFASmode = 0;
        continue;
      } else {

        if (sourceChanged) {
          print2("Warning:  You have not saved changes to the source.\n");
          str1 = cmdInput1("Do you want to EXIT anyway (Y, N) <N>? ");
          if (str1[0] != 'y' && str1[0] != 'Y') {
            print2("Use WRITE SOURCE to save the changes.\n");
            continue;
          }
          sourceChanged = 0;
        }

        if (texFileOpenFlag) {
          print2("The %s file \"%s\" was closed.\n",
              htmlFlag ? "HTML" : "LaTeX", texFileName);
          printTexTrailer(texHeaderFlag);
          fclose(texFilePtr);
          texFileOpenFlag = 0;
        }
        if (logFileOpenFlag) {
          print2("The log file \"%s\" was closed %s %s.\n",logFileName,
              date(),time_());
          fclose(logFilePtr);
          logFileOpenFlag = 0;
        }
        return; /* Exit from program */
      }
    }

    if (cmdMatches("SUBMIT")) {
      if (commandFileOpenFlag) {
        printf("?You may not nest SUBMIT commands.\n");
        continue;
      }
      let(&commandFileName, fullArg[1]);
      commandFilePtr = fSafeOpen(commandFileName, "r");
      if (!commandFilePtr) continue; /* Couldn't open (err msg was provided) */
      print2("Taking command lines from file \"%s\"...\n",commandFileName);
      commandFileOpenFlag = 1;
      continue;
    }

    if (toolsMode) {
      /* Start of toolsMode-specific commands */
#define ADD_MODE 1
#define DELETE_MODE 2
#define CLEAN_MODE 3
#define SUBSTITUTE_MODE 4
#define SWAP_MODE 5
#define INSERT_MODE 6
#define BREAK_MODE 7
#define BUILD_MODE 8
#define MATCH_MODE 9
#define RIGHT_MODE 10
      cmdMode = 0;
      if (cmdMatches("ADD")) cmdMode = ADD_MODE;
      else if (cmdMatches("DELETE")) cmdMode = DELETE_MODE;
      else if (cmdMatches("CLEAN")) cmdMode = CLEAN_MODE;
      else if (cmdMatches("SUBSTITUTE") || cmdMatches("S"))
        cmdMode = SUBSTITUTE_MODE;
      else if (cmdMatches("SWAP")) cmdMode = SWAP_MODE;
      else if (cmdMatches("INSERT")) cmdMode = INSERT_MODE;
      else if (cmdMatches("BREAK")) cmdMode = BREAK_MODE;
      else if (cmdMatches("BUILD")) cmdMode = BUILD_MODE;
      else if (cmdMatches("MATCH")) cmdMode = MATCH_MODE;
      else if (cmdMatches("RIGHT")) cmdMode = RIGHT_MODE;
      if (cmdMode) {
        list1_fp = fSafeOpen(fullArg[1], "r");
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        if (cmdMode == RIGHT_MODE) {
          /* Find the longest line */
          p = 0;
          while (linput(list1_fp, NULL, &str1)) {
            if (p < strlen(str1)) p = strlen(str1);
          }
          rewind(list1_fp);
        }
        let(&list2_fname, fullArg[1]);
        if (list2_fname[strlen(list2_fname) - 2] == '~') {
          let(&list2_fname, left(list2_fname, strlen(list2_fname) - 2));
          print2("The output file will be called %s.\n", list2_fname);
        }
        let(&list2_ftmpname, "");
        list2_ftmpname = fGetTmpName("zz~tools");
        list2_fp = fSafeOpen(list2_ftmpname, "w");
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */
        lines = 0;
        changedLines = 0;
        twoMatches = 0;
        changedFlag = 0;
        outMsgFlag = 0;
        switch (cmdMode) {
          case ADD_MODE:
            break;
          case DELETE_MODE:
            break;
          case CLEAN_MODE:
            let(&str4, edit(fullArg[2], 32));
            q = 0;
            if (instr(1, str4, "P") > 0) q = q + 1;
            if (instr(1, str4, "D") > 0) q = q + 2;
            if (instr(1, str4, "G") > 0) q = q + 4;
            if (instr(1, str4, "B") > 0) q = q + 8;
            if (instr(1, str4, "R") > 0) q = q + 16;
            if (instr(1, str4, "C") > 0) q = q + 32;
            if (instr(1, str4, "E") > 0) q = q + 128;
            if (instr(1, str4, "Q") > 0) q = q + 256;
            if (instr(1, str4, "L") > 0) q = q + 512;
            if (instr(1, str4, "T") > 0) q = q + 1024;
            if (instr(1, str4, "U") > 0) q = q + 2048;
            if (instr(1, str4, "V") > 0) q = q + 4096;
            break;
          case SUBSTITUTE_MODE:
            let(&newstr, fullArg[3]); /* The replacement string */
            if (((vstring)(fullArg[4]))[0] == 'A' ||
                ((vstring)(fullArg[4]))[0] == 'a') { /* ALL */
              q = -1;
            } else {
              q = val(fullArg[4]);
              if (q == 0) q = 1;    /* The occurrence # of string to subst */
            }
            s = 0;
            /*
            if (!strcmp(fullArg[2], "\\n")) {
            */
            s = instr(1, fullArg[2], "\\n");
            if (s) {
              /*s = 1;*/ /* Replace lf flag */
              q = 1; /* Only 1st occurrence makes sense in this mode */
            }
            if (!strcmp(fullArg[3], "\\n")) {
              let(&newstr, "\n"); /* Replace with lf */
            }
            break;
          case SWAP_MODE:
            break;
          case INSERT_MODE:
            p = val(fullArg[3]);
            break;
          case BREAK_MODE:
            outMsgFlag = 1;
            break;
          case BUILD_MODE:
            let(&str4, "");
            outMsgFlag = 1;
            break;
          case MATCH_MODE:
            outMsgFlag = 1;
        } /* End switch */
        let(&bufferedLine, "");
        /*
        while (linput(list1_fp, NULL, &str1)) {
        */
        while (1) {
          if (bufferedLine[0]) {
            /* Get input from buffered line (from rejected \n replacement) */
            let(&str1, bufferedLine);
            let(&bufferedLine, "");
          } else {
            if (!linput(list1_fp, NULL, &str1)) break;
          }
          lines++;
          oldChangedLines = changedLines;
          let(&str2, str1);
          switch (cmdMode) {
            case ADD_MODE:
              let(&str2, cat(fullArg[2], str1, fullArg[3], NULL));
              if (strcmp(str1, str2)) changedLines++;
              break;
            case DELETE_MODE:
              p1 = instr(1, str1, fullArg[2]);
              if (strlen(fullArg[2]) == 0) p1 = 1;
              p2 = instr(p1, str1, fullArg[3]);
              if (strlen(fullArg[3]) == 0) p2 = strlen(str1) + 1;
              if (p1 != 0 && p2 != 0) {
                let(&str2, cat(left(str1, p1 - 1), right(str1, p2 + strlen(
                    fullArg[3])), NULL));
                changedLines++;
              }
              break;
            case CLEAN_MODE:
              if (q) {
                let(&str2, edit(str1, q));
                if (strcmp(str1, str2)) changedLines++;
              }
              break;
            case SUBSTITUTE_MODE:
              let(&str2, str1);
              p = 0;
              p1 = 0;

              k = 1;
              /* See if an additional match on line is required */
              if (((vstring)(fullArg[5]))[0] != 0) {
                if (!instr(1, str2, fullArg[5])) {
                  /* No match on line; prevent any substitution */
                  k = 0;
                }
              }

              if (s && k) { /* We're asked to replace a newline char */
                /* Read in the next line */
                /*
                if (linput(list1_fp, NULL, &str4)) {
                  let(&str2, cat(str1, "\\n", str4, NULL));
                */
                if (linput(list1_fp, NULL, &bufferedLine)) {
                  /* Join the next line and see if the string matches */
                  if (instr(1, cat(str1, "\\n", bufferedLine, NULL),
                      fullArg[2])) {
                    let(&str2, cat(str1, "\\n", bufferedLine, NULL));
                    let(&bufferedLine, "");
                  } else {
                    k = 0; /* No match - leave bufferedLine for next pass */
                  }
                } else { /* EOF reached */
                  print2("Warning: file %s has an odd number of lines\n",
                      fullArg[1]);
                }
              }

              while (k) {
                p1 = instr(p1 + 1, str2, fullArg[2]);
                if (!p1) break;
                p++;
                if (p == q || q == -1) {
                  let(&str2, cat(left(str2, p1 - 1), newstr,
                      right(str2, p1 + strlen(fullArg[2])), NULL));
                  if (newstr[0] == '\n') {
                    /* Replacement string is an lf */
                    lines++;
                    changedLines++;
                  }
                  p1 = p1 - strlen(fullArg[2]) + strlen(newstr);
                  if (q != -1) break;
                }
              }
              if (strcmp(str1, str2)) changedLines++;
              break;
            case SWAP_MODE:
              p1 = instr(1, str1, fullArg[2]);
              if (p1) {
                p2 = instr(p1 + 1, str1, fullArg[2]);
                if (p2) twoMatches++;
                let(&str2, cat(right(str1, p1) + strlen(fullArg[2]),
                    fullArg[2], left(str1, p1 - 1), NULL));
                if (strcmp(str1, str2)) changedLines++;
              }
              break;
            case INSERT_MODE:
              if (strlen(str2) < p - 1)
                let(&str2, cat(str2, space(p - 1 - strlen(str2)), NULL));
              let(&str2, cat(left(str2, p - 1), fullArg[2],
                  right(str2, p), NULL));
              if (strcmp(str1, str2)) changedLines++;
              break;
            case BREAK_MODE:
              let(&str2, str1);
              changedLines++;
              for (i = 0; i < strlen(fullArg[2]); i++) {
                p = 0;
                while (1) {
                  p = instr(p + 1, str2, chr(((vstring)(fullArg[2]))[i]));
                  if (!p) break;
                  /* Put spaces arount special one-char tokens */
                  let(&str2, cat(left(str2, p - 1), " ",
                      mid(str2, p, 1),
                      " ", right(str2, p + 1), NULL));
                  p++;
                }
              }
              let(&str2, edit(str2, 8 + 16 + 128)); /* Reduce & trim spaces */
              for (p = strlen(str2) - 1; p >= 0; p--) {
                if (str2[p] == ' ') {
                  str2[p] = '\n';
                  changedLines++;
                }
              }
              if (!str2[0]) changedLines--; /* Don't output blank line */
              break;
            case BUILD_MODE:
              if (str2[0] != 0) { /* Ignore blank lines */
                if (str4[0] == 0) {
                  let(&str4, str2);
                } else {
                  if (strlen(str4) + strlen(str2) > 72) {
                    let(&str4, cat(str4, "\n", str2, NULL));
                    changedLines++;
                  } else {
                    let(&str4, cat(str4, " ", str2, NULL));
                  }
                }
                p = instr(1, str4, "\n");
                if (p) {
                  let(&str2, left(str4, p - 1));
                  let(&str4, right(str4, p + 1));
                } else {
                  let(&str2, "");
                }
              }
              break;
            case MATCH_MODE:
              if (((vstring)(fullArg[2]))[0] == 0) {
                /* Match any non-blank line */
                p = str1[0];
              } else {
                p = instr(1, str1, fullArg[2]);
              }
              if (((vstring)(fullArg[3]))[0] == 'n' ||
                  ((vstring)(fullArg[3]))[0] == 'N') {
                p = !p;
              }
              if (p) changedLines++;
              break;
            case RIGHT_MODE:
              let(&str2, cat(space(p - strlen(str2)), str2, NULL));
              if (strcmp(str1, str2)) changedLines++;
              break;
          } /* End switch(cmdMode) */
          if (lines == 1) let(&str3, left(str2, 79)); /* For msg */
          if (oldChangedLines != changedLines && !changedFlag) {
            changedFlag = 1;
            let(&str3, left(str2, 79)); /* For msg */
            firstChangedLine = lines;
            if ((SUBSTITUTE_MODE && newstr[0] == '\n')
                || BUILD_MODE) /* Joining lines */
              firstChangedLine = 1; /* Better message */
          }
          if (((cmdMode != BUILD_MODE && cmdMode != BREAK_MODE)
              || str2[0] != 0)
              && (cmdMode != MATCH_MODE || p))
            fprintf(list2_fp, "%s\n", str2);
        } /* Next input line */
        if (cmdMode == BUILD_MODE) {
          if (str4[0]) {
            /* Output last partial line */
            fprintf(list2_fp, "%s\n", str4);
            changedLines++;
            if (!str3[0]) {
              let(&str3, str4); /* For msg */
            }
          }
        }
        /* Convert any lf's to 1st line in string for readability of msg */
        p = instr(1, str3, "\n");
        if (p) let(&str3, left(str3, p - 1));
        if (!outMsgFlag) {
          if (!changedFlag) {
            if (!lines) {
              print2("The file %s has no lines.\n", fullArg[1]);
            } else {
              print2(
"The file %s has %ld lines; none were changed.  First line:\n",
                list2_fname, lines);
              print2("%s\n", str3);
            }
          } else {
            print2(
"The file %s has %ld lines; %ld were changed.  First change is on line %ld:\n",
                list2_fname, lines, changedLines, firstChangedLine);
            print2("%s\n", str3);
          }
          if (twoMatches) {
            print2(
"Warning:  %ld lines has more than one \"%s\".  The first one was used.\n",
                 twoMatches, fullArg[2]);
          }
        } else {
          if (changedLines == 0) let(&str3, "");
          print2(
"The input had %ld lines, the output has %ld lines.  First output line:\n",
              lines, changedLines);
          print2("%s\n", str3);
        }
        fclose(list1_fp);
        fclose(list2_fp);
        fSafeRename(list2_ftmpname, list2_fname);
        continue;
      } /* end if cmdMode */

#define SORT_MODE 1
#define UNDUPLICATE_MODE 2
#define DUPLICATE_MODE 3
#define UNIQUE_MODE 4
#define REVERSE_MODE 5
      cmdMode = 0;
      if (cmdMatches("SORT")) cmdMode = SORT_MODE;
      else if (cmdMatches("UNDUPLICATE")) cmdMode = UNDUPLICATE_MODE;
      else if (cmdMatches("DUPLICATE")) cmdMode = DUPLICATE_MODE;
      else if (cmdMatches("UNIQUE")) cmdMode = UNIQUE_MODE;
      else if (cmdMatches("REVERSE")) cmdMode = REVERSE_MODE;
      if (cmdMode) {
        list1_fp = fSafeOpen(fullArg[1], "r");
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        let(&list2_fname, fullArg[1]);
        if (list2_fname[strlen(list2_fname) - 2] == '~') {
          let(&list2_fname, left(list2_fname, strlen(list2_fname) - 2));
          print2("The output file will be called %s.\n", list2_fname);
        }
        let(&list2_ftmpname, "");
        list2_ftmpname = fGetTmpName("zz~tools");
        list2_fp = fSafeOpen(list2_ftmpname, "w");
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */

        /* Count the lines */
        lines = 0;
        while (linput(list1_fp, NULL, &str1)) lines++;
        if (cmdMode != SORT_MODE  && cmdMode != REVERSE_MODE) {
          print2("The input file has %ld lines.\n", lines);
        }

        /* Close and reopen the input file */
        fclose(list1_fp);
        list1_fp = fSafeOpen(fullArg[1], "r");
        /* Allocate memory */
        pntrLet(&pntrTmp, pntrSpace(lines));
        /* Assign the lines to string array */
        for (i = 0; i < lines; i++) linput(list1_fp, NULL,
            (vstring *)(&pntrTmp[i]));

        /* Sort */
        if (cmdMode != REVERSE_MODE) {
          if (cmdMode == SORT_MODE) {
            qsortKey = fullArg[2]; /* Do not deallocate! */
          } else {
            qsortKey = "";
          }
          qsort(pntrTmp, lines, sizeof(void *), qsortStringCmp);
        } else { /* Reverse the lines */
          for (i = lines / 2; i < lines; i++) {
            qsortKey = pntrTmp[i]; /* Use qsortKey as handy tmp var here */
            pntrTmp[i] = pntrTmp[lines - 1 - i];
            pntrTmp[lines - 1 - i] = qsortKey;
          }
        }

        /* Output sorted lines */
        changedLines = 0;
        let(&str3, "");
        for (i = 0; i < lines; i++) {
          j = 0; /* Flag that line should be printed */
          switch (cmdMode) {
            case SORT_MODE:
            case REVERSE_MODE:
              j = 1;
              break;
            case UNDUPLICATE_MODE:
              if (i == 0) {
                j = 1;
              } else {
                if (strcmp((vstring)(pntrTmp[i - 1]), (vstring)(pntrTmp[i]))) {
                  j = 1;
                }
              }
              break;
            case DUPLICATE_MODE:
              if (i > 0) {
                if (!strcmp((vstring)(pntrTmp[i - 1]), (vstring)(pntrTmp[i]))) {
                  if (i == lines - 1) {
                    j = 1;
                  } else {
                    if (strcmp((vstring)(pntrTmp[i]),
                        (vstring)(pntrTmp[i + 1]))) {
                      j = 1;
                    }
                  }
                }
              }
              break;
            case UNIQUE_MODE:
              if (i < lines - 1) {
                if (strcmp((vstring)(pntrTmp[i]), (vstring)(pntrTmp[i + 1]))) {
                  if (i == 0) {
                    j = 1;
                  } else {
                    if (strcmp((vstring)(pntrTmp[i - 1]),
                        (vstring)(pntrTmp[i]))) {
                      j = 1;
                    }
                  }
                }
              } else {
                if (i == 0) {
                  j = 1;
                } else {
                  if (strcmp((vstring)(pntrTmp[i - 1]),
                        (vstring)(pntrTmp[i]))) {
                      j = 1;
                  }
                }
              }
              break;
          } /* end switch (cmdMode) */
          if (j) {
            fprintf(list2_fp, "%s\n", (vstring)(pntrTmp[i]));
            changedLines++;
            if (changedLines == 1)
              let(&str3, left((vstring)(pntrTmp[i]), 79));
          }
        } /* next i */
        print2("The output file has %ld lines.  The first line is:\n",
            changedLines);
        print2("%s\n", str3);

        /* Deallocate memory */
        for (i = 0; i < lines; i++) let((vstring *)(&pntrTmp[i]), "");
        pntrLet(&pntrTmp,NULL_PNTRSTRING);

        fclose(list1_fp);
        fclose(list2_fp);
        fSafeRename(list2_ftmpname, list2_fname);
        continue;
      } /* end if cmdMode */

      if (cmdMatches("PARALLEL")) {
        list1_fp = fSafeOpen(fullArg[1], "r");
        list2_fp = fSafeOpen(fullArg[2], "r");
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */
        let(&list3_ftmpname, "");
        list3_ftmpname = fGetTmpName("zz~tools");
        list3_fp = fSafeOpen(list3_ftmpname, "w");
        if (!list3_fp) continue; /* Couldn't open it (error msg was provided) */

        p1 = 1; p2 = 1; /* not eof */
        p = 0; q = 0; /* lines */
        j = 0; /* 1st line flag */
        let(&str3, "");
        while (1) {
          let(&str1, "");
          if (p1) {
            p1 = (linput(list1_fp, NULL, &str1) != NULL);
            if (p1) p++;
            else let(&str1, "");
          }
          let(&str2, "");
          if (p2) {
            p2 = (linput(list2_fp, NULL, &str2) != NULL);
            if (p2) q++;
            else let(&str2, "");
          }
          if (!p1 && !p2) break;
          let(&str4, cat(str1, fullArg[4], str2, NULL));
          fprintf(list3_fp, "%s\n", str4);
          if (!j) {
            let(&str3, str4); /* Save 1st line for msg */
            j = 1;
          }
        }
        if (p == q) {
          print2(
"The input files each had %ld lines.  The first output line is:\n", p);
        } else {
          print2(
"Warning: file \"%s\" had %ld lines while file \"%s\" had %ld lines.\n",
              fullArg[1], p, fullArg[2], q);
          if (p < q) p = q;
          print2("The output file \"%s\" has %ld lines.  The first line is:\n",
              fullArg[3], p);
        }
        print2("%s\n", str3);

        fclose(list1_fp);
        fclose(list2_fp);
        fclose(list3_fp);
        fSafeRename(list3_ftmpname, fullArg[3]);
        continue;
      }


      if (cmdMatches("NUMBER")) {
        list1_fp = fSafeOpen(fullArg[1], "w");
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        j = strlen(str(val(fullArg[2])));
        k = strlen(str(val(fullArg[3])));
        if (k > j) j = k;
        for (i = val(fullArg[2]); i <= val(fullArg[3]);
            i = i + val(fullArg[4])) {
          let(&str1, str(i));
          fprintf(list1_fp, "%s\n", str1);
        }
        fclose(list1_fp);
        continue;
      }

      if (cmdMatches("COUNT")) {
        list1_fp = fSafeOpen(fullArg[1], "r");
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        p1 = 0;
        p2 = 0;
        lines = 0;
        q = 0; /* Longest line length */
        i = 0; /* First longest line */
        j = 0; /* Number of longest lines */
        sum = 0.0; /* Sum of numeric content of lines */
        firstChangedLine = 0;
        while (linput(list1_fp, NULL, &str1)) {
          lines++;

          /* Longest line */
          if (q < strlen(str1)) {
            q = strlen(str1);
            let(&str4, str1);
            i = lines;
            j = 0;
          }

          if (q == strlen(str1)) {
            j++;
          }

          if (instr(1, str1, fullArg[2])) {
            if (!firstChangedLine) {
              firstChangedLine = lines;
              let(&str3, str1);
            }
            p1++;
            p = 0;
            while (1) {
              p = instr(p + 1, str1, fullArg[2]);
              if (!p) break;
              p2++;
            }
          }
          sum = sum + val(str1);
        }
        print2(
"The file has %ld lines.  The string \"%s\" occurs %ld times on %ld lines.\n",
            lines, fullArg[2], p2, p1);
        if (firstChangedLine) {
          print2("The first occurrence is on line %ld:\n", firstChangedLine);
          print2("%s\n", str3);
        }
        print2(
"The first longest line (out of %ld) is line %ld and has %ld characters:\n",
            j, i, q);
        printLongLine(str4, "    "/*startNextLine*/, ""/*breakMatch*/);
            /* breakMatch empty means break line anywhere */  /* 6-Dec-03 */
        print2("If each line were a number, their sum would be %s\n", str(sum));
        fclose(list1_fp);
        continue;
      }

      if (cmdMatches("TYPE") || cmdMatches("T")) {
        list1_fp = fSafeOpen(fullArg[1], "r");
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        if (rawArgs == 2) {
          n = 10;
        } else {
          if (((vstring)(fullArg[2]))[0] == 'A' ||
              ((vstring)(fullArg[2]))[0] == 'a') { /* ALL */
            n = -1;
          } else {
            n = val(fullArg[2]);
          }
        }
        for (i = 0; i < n || n == -1; i++) {
          if (!linput(list1_fp, NULL, &str1)) break;
          if (!print2("%s\n", str1)) break;
        }
        fclose(list1_fp);
        continue;
      } /* end TYPE */

      if (cmdMatches("TAG")) {
        list1_fp = fSafeOpen(fullArg[1], "r");
        list2_fp = fSafeOpen(fullArg[2], "r");
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */
        if (!getRevision(fullArg[4])) {
          print2(
"?The revision tag must be of the form /*nn*/ or /*#nn*/.  Please try again.\n");
          continue;
        }
        let(&list3_ftmpname, "");
        list3_ftmpname = fGetTmpName("zz~tools");
        list3_fp = fSafeOpen(list3_ftmpname, "w");
        if (!list3_fp) continue; /* Couldn't open it (error msg was provided) */

        revise(list1_fp, list2_fp, list3_fp, fullArg[4],
            (long)val(fullArg[5]));

        fSafeRename(list3_ftmpname, fullArg[3]);
        continue;
      }

      if (cmdMatches("COPY") || cmdMatches("C")) {
        let(&list2_ftmpname, "");
        list2_ftmpname = fGetTmpName("zz~tools");
        list2_fp = fSafeOpen(list2_ftmpname, "w");
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */
        lines = 0;
        let(&str4, cat(fullArg[1], ",", NULL));
        lines = 0;
        j = 0; /* Error flag */
        while (1) {
          if (!str4[0]) break; /* Done scanning list */
          p = instr(1, str4, ",");
          let(&str3, left(str4, p - 1));
          let(&str4, right(str4, p + 1));
          list1_fp = fSafeOpen((str3), "r");
          if (!list1_fp) { /* Couldn't open it (error msg was provided) */
            j = 1; /* Error flag */
            break;
          }
          n = 0;
          while (linput(list1_fp, NULL, &str1)) {
            lines++; n++;
            fprintf(list2_fp, "%s\n", str1);
          }
          if (instr(1, fullArg[1], ",")) { /* More than 1 input file */
            print2("The input file \"%s\" has %ld lines.\n", str3, n);
          }
          fclose(list1_fp);
        }
        if (j) continue; /* One of the input files couldn't be opened */
        fclose(list2_fp);
        print2("The output file \"%s\" has %ld lines.\n", fullArg[2], lines);
        fSafeRename(list2_ftmpname, fullArg[2]);
        continue;
      }

      print2("?This command has not been implemented yet.\n");
      continue;
    } /* End of toolsMode-specific commands */

    if (cmdMatches("TOOLS")) {
      print2(
"Entering the Text Tools utilities.  Type HELP for help, EXIT to exit.\n");
      toolsMode = 1;
      continue;
    }

    if (cmdMatches("READ")) {
      if (statements) {
        printLongLine(cat(
            "?Sorry, reading of more than one source file is not allowed.  ",
            "You may type ERASE to start over.  Note that additional source ",
            "files may be included in the source file with \"$[ $]\".",
            NULL),"  "," ");
        continue;
      }
      let(&input_fn, fullArg[1]);
      input_fp = fSafeOpen(input_fn, "r");
      if (!input_fp) continue; /* Couldn't open it (error msg was provided) */

      fclose(input_fp);
      readInput();
      if (switchPos("/ VERIFY")) {
        verifyProofs("*",1); /* Parse and verify */
      } else {
        /* verifyProofs("*",0); */ /* Parse only (for gross error checking) */
      }

      /* 10/21/02 - detect Microsoft bugs reported by several users, when the
         HTML output files are named "con.html" etc. */
      /* If we want a standard error message underlining token, this could go
         in mmpars.c */
      /* From Microsoft's site:
         "The following reserved words cannot be used as the name of a file:
         CON, PRN, AUX, CLOCK$, NUL, COM1, COM2, COM3, COM4, COM5, COM6, COM7,
         COM8, COM9, LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, and LPT9.
         Also, reserved words followed by an extension - for example,
         NUL.tx7 - are invalid file names." */
      /* Check for labels that will lead to illegal Microsoft file names for
         Windows users.  Don't bother checking CLOCK$ since $ is already
         illegal */
      let(&str1, cat(
         ",CON,PRN,AUX,NUL,COM1,COM2,COM3,COM4,COM5,COM6,COM7,",
         "COM8,COM9,LPT1,LPT2,LPT3,LPT4,LPT5,LPT6,LPT7,LPT8,LPT9,", NULL));
      for (i = 1; i <= statements; i++) {
        let(&str2, cat(",", edit(statement[i].labelName, 32/*uppercase*/), ",",
            NULL));
        if (instr(1, str1, str2) ||
            /* 5-Jan-04 mm*.html is reserved for mmtheorems.html, etc. */
            !strcmp(",MM", left(str2, 3))) {
          print2("\n");
          printLongLine(cat("?Error in statement \"",
              statement[i].labelName, "\" at line ", str(statement[i].lineNum),
              " in file \"", statement[i].fileName,
              "\".  To workaround a Microsoft operating system limitation, the",
              " the following reserved words cannot be used for label names:",
              " CON, PRN, AUX, CLOCK$, NUL, COM1, COM2, COM3, COM4, COM5,",
              " COM6, COM7, COM8, COM9, LPT1, LPT2, LPT3, LPT4, LPT5, LPT6,",
              " LPT7, LPT8, and LPT9.  Also, \"mm*.html\" is reserved for",
              " Metamath file names.  Use another name for this label.", NULL),
              "", " ");
          errorCount++;
        }
      }
      /* 10/21/02 end */

      if (!errorCount) {
        let(&str1, "No errors were found.");
        if (!switchPos("/ VERIFY")) {
            let(&str1, cat(str1,
       "  However, proofs were not checked.  Type VERIFY PROOF *",
       " if you want to check them.",
            NULL));
        }
        printLongLine(str1, "", " ");
      } else {
        print2("\n");
        if (errorCount == 1) {
          print2("One error was found.\n");
        } else {
          print2("%ld errors were found.\n", (long)errorCount);
        }
      }

      continue;
    }

    if (cmdMatches("WRITE SOURCE")) {
      let(&output_fn, fullArg[2]);
      output_fp = fSafeOpen(output_fn, "w");
      if (!output_fp) continue; /* Couldn't open it (error msg was provided)*/

      /* Added 24-Oct-03 nm */
      if (switchPos("/ CLEAN") > 0) {
        cmdMode = 1; /* Clean out any proof-in-progress (that user has flagged
                        with a ? in its date comment field) */
      } else {
        cmdMode = 0; /* Output all proofs (normal) */
      }

      writeInput(cmdMode); /* Added argument 24-Oct-03 nm */
      fclose(output_fp);
      if (cmdMode == 0) sourceChanged = 0; /* Don't unset flag if CLEAN option
                                    since some new proofs may not be saved. */
      continue;
    } /* End of WRITE SOURCE */


    if (cmdMatches("WRITE THEOREM_LIST")) {
      /* 4-Dec-03 - Write out an HTML summary of the theorems to
         mmtheorems.html, mmtheorems2.html,... */
      /* THEOREMS_PER_PAGE is the default number of proof descriptions to output. */
#define THEOREMS_PER_PAGE 100
      /* i is the actual number of proof descriptions to output. */
      /* See if the user overrode the default. */
      i = switchPos("/ THEOREMS_PER_PAGE");
      if (i) {
        i = val(fullArg[i + 1]); /* Use user's value */
      } else {
        i = THEOREMS_PER_PAGE; /* Use the default value */
      }

      if (!texDefsRead) {
        htmlFlag = 1;
        print2("Reading definitions from $t statement of %s...\n", input_fn);
        if (!readTexDefs()) {
          tmpFlag = 1; /* Error flag to recover input file */
          continue; /* An error occurred */
        }
      } else {
        /* Current limitation - can only read def's from .mm file once */
        if (!htmlFlag) {
          print2("?You cannot use both LaTeX and HTML in the same session.\n");
          print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
          continue;
        }
      }
      /* Output the theorem list */
      writeTheoremList(i); /* (located in mmwtex.c) */
      continue;
    }  /* End of "WRITE THEOREM_LIST" */


    if (cmdMatches("WRITE BIBLIOGRAPHY")) {
      /* 10/10/02 -
         This utility builds the bibliographical cross-references to various
         textbooks and updates the user-specified file normally called
         mmbiblio.html.
       */
      tmpFlag = 0; /* Error flag to recover input file */
      list1_fp = fSafeOpen(fullArg[2], "r");
      if (list1_fp == NULL) {
        /* Couldn't open it (error msg was provided)*/
        continue;
      }
      fclose(list1_fp);
      /* This will rename the input mmbiblio.html as mmbiblio.html~1 */
      list2_fp = fSafeOpen(fullArg[2], "w");
      if (list2_fp == NULL) {
          /* Couldn't open it (error msg was provided)*/
        continue;
      }
      /* Note: in older versions the "~1" string was OS-dependent, but we
         don't support VAX or THINK C anymore...  Anyway we reopen it
         here with the renamed file in case the OS won't let us rename
         an opened file during the fSafeOpen for write above. */
      list1_fp = fSafeOpen(cat(fullArg[2], "~1", NULL), "r");
      if (list1_fp == NULL) bug(1116);
      if (!texDefsRead) {
        print2("Reading definitions from $t statement of %s...\n", input_fn);
        if (!readTexDefs()) {
          tmpFlag = 1; /* Error flag to recover input file */
          goto bib_error; /* An error occurred */
        }
      }

      /* Transfer the input file up to the special "<!-- #START# -->" comment */
      while (1) {
        if (!linput(list1_fp, NULL, &str1)) {
          print2(
"?Error: Could not find \"<!-- #START# -->\" line in input file \"%s\".\n",
              fullArg[2]);
          tmpFlag = 1; /* Error flag to recover input file */
          break;
        }
        fprintf(list2_fp, "%s\n", str1);
        if (!strcmp(str1, "<!-- #START# -->")) break;
      }
      if (tmpFlag) goto bib_error;

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
          if (statement[i].type != (char)p__ &&
            statement[i].type != (char)a__) continue;
          /* Omit ...OBS (obsolete) and ...NEW (to be implemented) statements */
          if (instr(1, statement[i].labelName, "NEW")) continue;
          if (instr(1, statement[i].labelName, "OBS")) continue;
          let(&str1, "");
          str1 = getDescription(i); /* Get the statement's comment */
          if (!instr(1, str1, "[")) continue;
          for (j = 0; j < strlen(str1); j++) {
            if (str1[j] == '\n') str1[j] = ' '; /* Change newlines to spaces */
            if (str1[j] == '\r') bug(1119);
          }
          let(&str1, edit(str1, 8 + 128 + 16)); /* Reduce & trim whitespace */
          /* Put spaces before page #s (up to 4 digits) for sorting */
          j = 0;
          while (1) {
            j = instr(j + 1, str1, " p. "); /* Heuristic - match " p. " */
            if (!j) break;
            if (j) {
              for (k = j + 4; k <= strlen(str1) + 1; k++) {
                if (!isdigit(str1[k - 1])) {
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
            if (!isalnum(str1[j])) continue; /* Not start of reference */
            n++;
            /* Backtrack from [reference] to a starting keyword */
            m = 0;
            let(&str2, edit(str1, 32)); /* to uppercase */
            for (k = j - 1; k >= 1; k--) {
              if (0
                  || !strcmp(mid(str2, k, strlen("COMPARE")), "COMPARE")
                  || !strcmp(mid(str2, k, strlen("DEFINITION")), "DEFINITION")
                  || !strcmp(mid(str2, k, strlen("THEOREM")), "THEOREM")
                  || !strcmp(mid(str2, k, strlen("PROPOSITION")), "PROPOSITION")
                  || !strcmp(mid(str2, k, strlen("LEMMA")), "LEMMA")
                  || !strcmp(mid(str2, k, strlen("COROLLARY")), "COROLLARY")
                  || !strcmp(mid(str2, k, strlen("AXIOM")), "AXIOM")
                  || !strcmp(mid(str2, k, strlen("RULE")), "RULE")
                  || !strcmp(mid(str2, k, strlen("REMARK")), "REMARK")
                  || !strcmp(mid(str2, k, strlen("EXERCISE")), "EXERCISE")
                  || !strcmp(mid(str2, k, strlen("PROBLEM")), "PROBLEM")
                  || !strcmp(mid(str2, k, strlen("NOTATION")), "NOTATION")
                  || !strcmp(mid(str2, k, strlen("EXAMPLE")), "EXAMPLE")
                  || !strcmp(mid(str2, k, strlen("PROPERTY")), "PROPERTY")
                  || !strcmp(mid(str2, k, strlen("FIGURE")), "FIGURE")
                  || !strcmp(mid(str2, k, strlen("POSTULATE")), "POSTULATE")
                  || !strcmp(mid(str2, k, strlen("EQUATION")), "EQUATION")
                  || !strcmp(mid(str2, k, strlen("SCHEME")), "SCHEME")
                  /* Don't use SCHEMA since we may have THEOREM SCHEMA
                  || !strcmp(mid(str2, k, strlen("SCHEMA")), "SCHEMA")
                  */
                  || !strcmp(mid(str2, k, strlen("CHAPTER")), "CHAPTER")
                  ) {
                m = k;
                break;
              }
              let(&str3, ""); /* Clear tmp alloc stack created by "mid" */
            }
            if (!m) {
              if (p == 0) print2("Skipped (no keyword match): %s\n",
                  statement[i].labelName);
              continue; /* Not a bib ref - ignore */
            }
            /* m is at the start of a keyword */
            p = instr(m, str1, "["); /* Start of bibliograpy reference */
            q = instr(p, str1, "]"); /* End of bibliography reference */
            if (!q) {
              if (p == 0) print2("Skipped (not a valid reference): %s\n",
                  statement[i].labelName);
              continue; /* Not a bib ref - ignore */
            }
            s = instr(q, str1, "###"); /* Page number marker */
            if (!s) {
              if (p == 0) print2("Skipped (no page number): %s\n",
                  statement[i].labelName);
              continue; /* No page number given - ignore */
            }
            /* Now we have a real reference; increment reference count */
            lines++;
            if (p2 == 1) continue; /* In 1st pass, we just count refs */

            let(&str2, seg(str1, m, p - 1));     /* "Theorem #" */
            let(&str3, seg(str1, p + 1, q - 1));  /* "[bibref]" w/out [] */
            let(&str4, seg(str1, q + 1, s - 1)); /* " p. nnnn" */
            str2[0] = toupper(str2[0]);
            /* Eliminate noise like "of" in "Theorem 1 of [bibref]" */
            for (k = strlen(str2); k >=1; k--) {
              if (0
                  || !strcmp(mid(str2, k, strlen(" of ")), " of ")
                  || !strcmp(mid(str2, k, strlen(" in ")), " in ")
                  || !strcmp(mid(str2, k, strlen(" from ")), " from ")
                  || !strcmp(mid(str2, k, strlen(" on ")), " on ")
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
                str3, " ", str4, space(20 - strlen(str2)), str2,
                "|||",  /* ||| means end of sort key */
                /* Construct just the statement href for combining dup refs */
                "<A HREF=\"", statement[i].labelName,
                ".html\">", statement[i].labelName, "</A>",
                newstr,
                "&&&",  /* &&& means end of statement href */
                /* Construct actual HTML table row (without ending tag
                   so duplicate references can be added) */
                (i < extHtmlStmt) ?
                   "<TR>" :
                   cat("<TR BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL),
                "<TD NOWRAP>[<A HREF=\"",
                (i < extHtmlStmt) ?
                   htmlBibliography :
                   extHtmlBibliography,
                "#",
                str3,
                "\">", str3, "</A>]", str4,
                "</TD><TD>", str2, "</TD><TD><A HREF=\"",
                statement[i].labelName,
                ".html\">", statement[i].labelName, "</A>",
                newstr, NULL));
            /* Put construction into string array for sorting */
            let((vstring *)(&pntrTmp[lines - 1]), oldstr);
          }
        }
        /* 'lines' should be the same in both passes */
        print2("Pass %ld finished.  %ld references were processed.\n", p2, lines);
        if (p2 == 2) break;
        p2++;    /* Increment from pass 1 to pass 2 */
      }

      /* Sort */
      qsortKey = "";
      qsort(pntrTmp, lines, sizeof(void *), qsortStringCmp);

      /* Combine duplicate references */
      let(&str1, "");  /* Last biblio ref */
      for (i = 0; i < lines; i++) {
        j = instr(1, (vstring)(pntrTmp[i]), "|||");
        let(&str2, left((vstring)(pntrTmp[i]), j - 1));
        if (!strcmp(str1, str2)) {
          n++;
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
      n = 0;
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


      /* Discard the input file up to the special "<!-- #END# -->" comment */
      while (1) {
        if (!linput(list1_fp, NULL, &str1)) {
          print2(
"?Error: Could not find \"<!-- #END# -->\" line in input file \"%s\".\n",
              fullArg[2]);
          tmpFlag = 1; /* Error flag to recover input file */
          break;
        }
        if (!strcmp(str1, "<!-- #END# -->")) {
          fprintf(list2_fp, "%s\n", str1);
          break;
        }
      }
      if (tmpFlag) goto bib_error;

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


     bib_error:
      fclose(list1_fp);
      fclose(list2_fp);
      if (tmpFlag) {
        /* Recover input files in case of error */
        remove(fullArg[2]);  /* Delete output file */
        rename(cat(fullArg[2], "~1", NULL), fullArg[2]);
            /* Restore input file name */
        print2("?The file \"%s\" was not modified.\n", fullArg[2]);
      }
      continue;
    }  /* End of "WRITE BIBLIOGRAPHY" */


    if (cmdMatches("WRITE RECENT_ADDITIONS")) {
      /* 18-Sep-03 -
         This utility creates a list of recent proof descriptions and updates
         the user-specified file normally called mmrecent.html.
       */

      /* RECENT_COUNT is the default number of proof descriptions to output. */
#define RECENT_COUNT 100
      /* i is the actual number of proof descriptions to output. */
      /* See if the user overrode the default. */
      i = switchPos("/ LIMIT");
      if (i) {
        i = val(fullArg[i + 1]); /* Use user's value */
      } else {
        i = RECENT_COUNT; /* Use the default value */
      }

      if (!texDefsRead) {
        htmlFlag = 1;
        print2("Reading definitions from $t statement of %s...\n", input_fn);
        if (!readTexDefs()) {
          tmpFlag = 1; /* Error flag to recover input file */
          continue; /* An error occurred */
        }
      } else {
        /* Current limitation - can only read def's from .mm file once */
        if (!htmlFlag) {
          print2("?You cannot use both LaTeX and HTML in the same session.\n");
          print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
          continue;
        }
      }

      tmpFlag = 0; /* Error flag to recover input file */
      list1_fp = fSafeOpen(fullArg[2], "r");
      if (list1_fp == NULL) {
        /* Couldn't open it (error msg was provided)*/
        continue;
      }
      fclose(list1_fp);
      /* This will rename the input mmrecent.html as mmrecent.html~1 */
      list2_fp = fSafeOpen(fullArg[2], "w");
      if (list2_fp == NULL) {
          /* Couldn't open it (error msg was provided)*/
        continue;
      }
      /* Note: in older versions the "~1" string was OS-dependent, but we
         don't support VAX or THINK C anymore...  Anyway we reopen it
         here with the renamed file in case the OS won't let us rename
         an opened file during the fSafeOpen for write above. */
      list1_fp = fSafeOpen(cat(fullArg[2], "~1", NULL), "r");
      if (list1_fp == NULL) bug(1117);

      /* Transfer the input file up to the special "<!-- #START# -->" comment */
      while (1) {
        if (!linput(list1_fp, NULL, &str1)) {
          print2(
"?Error: Could not find \"<!-- #START# -->\" line in input file \"%s\".\n",
              fullArg[2]);
          tmpFlag = 1; /* Error flag to recover input file */
          break;
        }
        fprintf(list2_fp, "%s\n", str1);
        if (!strcmp(str1, "<!-- #START# -->")) break;
      }
      if (tmpFlag) goto wrrecent_error;

      /* Get and parse today's date */
      let(&str1, date());
      j = instr(1, str1, "-");
      k = val(left(str1, j - 1)); /* Day */
#define MONTHS "JanFebMarAprMayJunJulAugSepOctNovDec"
      l = ((instr(1, MONTHS, mid(str1, j + 1, 3)) - 1) / 3) + 1; /* 1 = Jan */
      m = val(mid(str1, j + 5, 2));  /* Year */
#define START_YEAR 93 /* Earliest 19xx year in set.mm database */
      if (m < START_YEAR) {
        m = m + 2000;
      } else {
        m = m + 1900;
      }

      n = 0; /* Count of how many output so far */
      while (n < i /*RECENT_COUNT*/ && m > START_YEAR + 1900 - 1) {
        /* Build date string to match */
        let(&str1, cat("$([", str(k), "-", mid(MONTHS, 3 * l - 2, 3), "-",
            right(str(m), 3), "]$)", NULL));
        for (s = statements; s >= 1; s--) {

          if (statement[s].type != (char)p__) continue;

          /* Get the comment section after the statement */
          let(&str2, space(statement[s + 1].labelSectionLen));
          memcpy(str2, statement[s + 1].labelSectionPtr,
              statement[s + 1].labelSectionLen);
          /* See if the date comment matches */
          if (instr(1, str2, str1)) {
            /* We have a match, so increment the match count */
            n++;

            let(&str3, "");
            str3 = getDescription(s);
            let(&str4, "");
            str4 = pinkHTML(s); /* Get little pink number */
            /* Output the description comment */
            /* Break up long lines for text editors with printLongLine */
            let(&printString, "");
            outputToString = 1;
            print2("\n"); /* Blank line for HTML human readability */
            printLongLine(cat(
                  (s < extHtmlStmt) ?
                       "<TR>" :
                       cat("<TR BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL),
                  "<TD NOWRAP>",  /* IE breaks up the date */
                  mid(str1, 4, strlen(str1) - 6), /* Date */
                  "</TD><TD ALIGN=CENTER><A HREF=\"",
                  statement[s].labelName, ".html\">",
                  statement[s].labelName, "</A>",
                  str4, "</TD><TD>", NULL),  /* Description */
                " ",  /* Start continuation line with space */
                "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

            showStatement = s; /* For printTexComment */
            outputToString = 0; /* For printTexComment */
            texFilePtr = list2_fp;
            /* 18-Sep-03 ???Future - make this just return a string??? */
            printTexComment(str3, 0); /* Sends result to texFilePtr */
            texFilePtr = NULL;
            outputToString = 1; /* Restore after printTexComment */

            /* Get HTML hypotheses => assertion */
            let(&str4, "");
            str4 = getHTMLHypAndAssertion(s); /* In mmwtex.c */
            printLongLine(cat("</TD></TR><TR",
                  (s < extHtmlStmt) ?
                       ">" :
                       cat(" BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL),
                /*** old
                "<TD BGCOLOR=white>&nbsp;</TD><TD COLSPAN=2 ALIGN=CENTER>",
                str4, "</TD></TR>", NULL),
                ****/
                /* 27-Oct-03 nm */
                "<TD COLSPAN=3 ALIGN=CENTER>",
                str4, "</TD></TR>", NULL),

                " ",  /* Start continuation line with space */
                "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

            outputToString = 0;
            fprintf(list2_fp, "%s", printString);
            let(&printString, "");

            if (n >= i /*RECENT_COUNT*/) break; /* We're done */

            /* 27-Oct-03 nm Put separator row if not last theorem */
            outputToString = 1;
            printLongLine(cat("<TR BGCOLOR=white><TD COLSPAN=3>",
                "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>", NULL),
                " ",  /* Start continuation line with space */
                "\""); /* Don't break inside quotes e.g. "Arial Narrow" */
            outputToString = 0;
            fprintf(list2_fp, "%s", printString);
            let(&printString, "");

          }
        } /* Next s - statement number */
        /* Decrement date */
        if (k > 1) {
          k--; /* Decrement day */
        } else {
          k = 31; /* Non-existent day 31's will never match, which is OK */
          if (l > 1) {
            l--; /* Decrement month */
          } else {
            l = 12; /* Dec */
            m --; /* Decrement year */
          }
        }
      } /* next while - Scan next date */


      /* Discard the input file up to the special "<!-- #END# -->" comment */
      while (1) {
        if (!linput(list1_fp, NULL, &str1)) {
          print2(
"?Error: Could not find \"<!-- #END# -->\" line in input file \"%s\".\n",
              fullArg[2]);
          tmpFlag = 1; /* Error flag to recover input file */
          break;
        }
        if (!strcmp(str1, "<!-- #END# -->")) {
          fprintf(list2_fp, "%s\n", str1);
          break;
        }
      }
      if (tmpFlag) goto wrrecent_error;

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

      print2("The %ld most recent theorem(s) were written.\n", n);

     wrrecent_error:
      fclose(list1_fp);
      fclose(list2_fp);
      if (tmpFlag) {
        /* Recover input files in case of error */
        remove(fullArg[2]);  /* Delete output file */
        rename(cat(fullArg[2], "~1", NULL), fullArg[2]);
            /* Restore input file name */
        print2("?The file \"%s\" was not modified.\n", fullArg[2]);
      }
      continue;
    }  /* End of "WRITE RECENT_ADDITIONS" */


    if (cmdMatches("SHOW LABELS")) {
        texFlag = 0;
        if (switchPos("/ HTML")) texFlag = 1;
        linearFlag = 0;
        if (switchPos("/ LINEAR")) linearFlag = 1;
        if (switchPos("/ ALL")) {
          m = 1;  /* Include $e, $f statements */
          if (!(htmlFlag && texFlag)) print2(
  "The labels that match are shown with statement number, label, and type.\n");
        } else {
          m = 0;  /* Show $a, $p only */
          if (!(htmlFlag && texFlag)) print2(
"The assertions that match are shown with statement number, label, and type.\n");
        }
        if (htmlFlag && texFlag) {
          printf(
            "?HTML qualifier is now implemented in SHOW STATEMENT * / HTML\n");
          continue;
        } /* if (htmlFlag && texFlag) */
        j = 0;
        k = 0;
        for (i = 1; i <= statements; i++) {
          if (!statement[i].labelName[0]) continue; /* No label */
          if (!m && statement[i].type != (char)p__ &&
              statement[i].type != (char)a__) continue; /* No /ALL switch */
          if (!matches(statement[i].labelName, fullArg[2], '*')) continue;
          let(&str1,cat(str(i)," ",
              statement[i].labelName," $",chr(statement[i].type)," ",NULL));
#define COL 19 /* Characters per column */
          if (j + strlen(str1) > MAX_LEN
              || (linearFlag && j != 0)) { /* j != 0 to suppress 1st CR */
            print2("\n");
            j = 0;
            k = 0;
          }
          if (strlen(str1) > COL || linearFlag) {
            j = j + strlen(str1);
            k = k + strlen(str1) - COL;
            print2(str1);
          } else {
            if (k == 0) {
              j = j + COL;
              print2("%s%s",str1,space(COL - strlen(str1)));
            } else {
              k = k - (COL - strlen(str1));
              if (k > 0) {
                print2(str1);
                j = j + strlen(str1);
              } else {
                print2("%s%s",str1,space(COL - strlen(str1)));
                j = j + COL;
                k = 0;
              }
            }
          }
        }
        print2("\n");
        continue;
    }

    if (cmdMatches("SHOW SOURCE")) {
      for (i = 1; i <= statements; i++) {
        if (!strcmp(fullArg[2],statement[i].labelName)) break;
      }
      if (i > statements) {
        printLongLine(cat("?There is no statement with label \"",
            fullArg[2], "\".  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        showStatement = 0;
        continue;
      }
      showStatement = i;

      let(&str1, "");
      str1 = outputStatement(showStatement);
      let(&str1,edit(str1,128)); /* Trim trailing spaces */
      if (str1[strlen(str1)-1] == '\n') let(&str1, left(str1,
          strlen(str1) - 1));
      printLongLine(str1, "", "");
      let(&str1,""); /* Deallocate vstring */
      continue;
    } /* if (cmdMatches("SHOW SOURCE")) */


    if (cmdMatches("SHOW STATEMENT") && (
        switchPos("/ HTML")
        || switchPos("/ BRIEF_HTML")
        || switchPos("/ ALT_HTML")
        || switchPos("/ BRIEF_ALT_HTML"))) {
      /* Special processing for the / HTML qualifiers - for each matching
         statement, a .html file is opened, the statement is output,
         and depending on statement type a proof or other information
         is output. */

      if (rawArgs != 5) {
        print2("?The HTML qualifiers may not be combined with others.\n");
        continue;
      }

      if (texDefsRead) {
        /* Current limitation - can only read def's from .mm file once */
        if (!htmlFlag) {
          print2("?You cannot use both LaTeX and HTML in the same session.\n");
          print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
          goto htmlDone;
        } else {
          if ((switchPos("/ ALT_HTML") || switchPos("/ BRIEF_ALT_HTML"))
              == (altHtmlFlag == 0)) {
            print2(
              "?You cannot use both HTML and ALT_HTML in the same session.\n");
            print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
            goto htmlDone;
          }
        }
      }

      if (switchPos("/ BRIEF_HTML") || switchPos("/ BRIEF_ALT_HTML")) {
        if (strcmp(fullArg[2], "*")) {
          print2(
              "?For BRIEF_HTML or BRIEF_ALT_HTML, the label must be \"*\"\n");
          goto htmlDone;
        }
        briefHtmlFlag = 1;
      } else {
        briefHtmlFlag = 0;
      }

      if (switchPos("/ ALT_HTML") || switchPos("/ BRIEF_ALT_HTML")) {
        altHtmlFlag = 1;
      } else {
        altHtmlFlag = 0;
      }

      q = 0;

      /* Special feature:  if the match statement has a "*" in it, we
         will also output mmascii.html, mmtheorems.html, and mmdefinitions.html.
         So, with
                 SHOW STATEMENT * / HTML
         these will be output plus all statements; with
                 SHOW STATEMENT ?* / HTML
         these will be output with no statements (since ? is illegal in a
         statement label) */
      if (instr(1, fullArg[2], "*") || briefHtmlFlag) {
        s = -2; /* -2 is for ASCII table; -1 is for theorems;
                    0 is for definitions */
      } else {
        s = 1;
      }

      for (s = s; s <= statements; s++) {

        if (s > 0 && briefHtmlFlag) break; /* Only do summaries */

        /*
           s = -2:  mmascii.html
           s = -1:  mmtheorems.html
           s = 0:   mmdefinitions.html
           s > 0:   normal statement
        */

        if (s > 0) {
          if (!statement[s].labelName[0]) continue; /* No label */
          if (!matches(statement[s].labelName, fullArg[2], '*')) continue;
          if (statement[s].type != (char)a__
              && statement[s].type != (char)p__) continue;
        }

        q = 1; /* Flag that at least one matching statement was found */

        if (s > 0) {
          showStatement = s;
        } else {
          /* We set it to 1 here so we will output the Metamath Proof
             Explorer and not the Hilbert Space Explorer header for
             definitions and theorems lists, when showStatement is
             compared to extHtmlStmt in printTexHeader in mmwtex.c */
          showStatement = 1;
        }


        /*** Open the html file ***/
        htmlFlag = 1;
        /* Open the html output file */
        switch (s) {
          case -2:
            let(&texFileName, "mmascii.html");
            break;
          case -1:
            let(&texFileName, "mmtheorems.html");
            break;
          case 0:
            let(&texFileName, "mmdefinitions.html");
            break;
          default:
            let(&texFileName, cat(statement[showStatement].labelName, ".html",
                NULL));
        }
        print2("Creating HTML file \"%s\"...\n", texFileName);
        texFilePtr = fSafeOpen(texFileName, "w");
        if (!texFilePtr) goto htmlDone; /* Couldn't open it (err msg was
            provided) */
        texFileOpenFlag = 1;
        printTexHeader((s > 0) ? 1 : 0 /*texHeaderFlag*/);
        if (!texDefsRead) {
          /* 9/6/03 If there was an error reading the $t xx.mm statement,
             texDefsRead won't be set, and we should close out file and skip
             further processing.  Otherwise we will be attempting to process
             uninitialized htmldef arrays and such. */
          print2("?HTML generation was aborted due to the error above.\n");
          s = statements + 1; /* To force loop to exit */
          goto ABORT_S; /* Go to end of loop where file is closed out */
        }

        if (s <= 0) {
          outputToString = 1;
          if (s == -2) {
            printLongLine(cat("<CENTER><FONT COLOR=", GREEN_TITLE_COLOR,
                "><B>",
                "Symbol to ASCII Correspondence for Text-Only Browsers",
                " (in order of first appearance)",
                "</B></FONT></CENTER><P>", NULL), "", "\"");
          }
          print2(
              "<CENTER><TABLE BORDER CELLSPACING=0 BGCOLOR=%s\n",
              MINT_BACKGROUND_COLOR);
          /* For bobby.cast.org approval */
          switch (s) {
            case -2:
              print2("SUMMARY=\"Symbol to ASCII correspondences\">\n");
              break;
            case -1:
              print2("SUMMARY=\"List of theorems\">\n");
              break;
            case 0:
              print2("SUMMARY=\"List of syntax, axioms and definitions\">\n");
              break;
          }
          switch (s) {
            case -2:
              print2("<TR ALIGN=LEFT><TD><B>\n");
              break;
            case -1:
              print2(
         "<CAPTION><B>List of Theorems</B></CAPTION><TR ALIGN=LEFT><TD><B>\n");
              break;
            case 0:
              printLongLine(cat(
/*"<CAPTION><B>List of Syntax (not <FONT COLOR=\"#00CC00\">|-&nbsp;</FONT>), ",*/
                  /* 2/9/02 (in case |- suppressed) */
                  "<CAPTION><B>List of Syntax, ",
                  "Axioms (<FONT COLOR=", GREEN_TITLE_COLOR, ">ax-</FONT>) and",
                  " Definitions (<FONT COLOR=", GREEN_TITLE_COLOR,
                  ">df-</FONT>)", "</B></CAPTION><TR ALIGN=LEFT><TD><B>",
                  NULL), "", "\"");
              break;
          }
          switch (s) {
            case -2:
              print2("Symbol</B></TD><TD><B>ASCII\n");
              break;
            case -1:
              print2(
                  "Ref</B></TD><TD><B>Description\n");
              break;
            case 0:
              printLongLine(cat(
                  "Ref</B></TD><TD><B>",
                "Expression (see link for any distinct variable requirements)",
                NULL), "", "\"");
              break;
          }
          print2("</B></TD></TR>\n");
          m = 0; /* Statement number map */
          let(&str3, ""); /* For storing ASCII token list in s=-2 mode */
          for (i = 1; i <= statements; i++) {
            if (i == extHtmlStmt && s != -2) {
              /* Print a row that identifies the start of the extended
                 database (e.g. Hilbert Space Explorer) */
              printLongLine(cat(
                  "<TR><TD COLSPAN=2 ALIGN=CENTER>The ",
                  "<B><FONT COLOR=", GREEN_TITLE_COLOR, ">",
                  extHtmlTitle,
                  "</FONT></B> starts here</TD></TR>", NULL), "", "\"");
            }
            if (statement[i].type == (char)p__ ||
                statement[i].type == (char)a__ ) m++;
            if ((s == -1 && statement[i].type != (char)p__)
                || (s == 0 && statement[i].type != (char)a__)
                || (s == -2 && statement[i].type != (char)c__
                    && statement[i].type != (char)v__)
                ) continue;
            switch (s) {
              case -2:
                /* Print symbol to ASCII table entry */
                /* It's a $c or $v statement, so each token generates a
                   table row */
                for (j = 0; j < statement[i].mathStringLen; j++) {
                  let(&str1, mathToken[(statement[i].mathString)[j]].tokenName);
                  /* Output each token only once in case of multiple decl. */
                  if (!instr(1, str3, cat(" ", str1, " ", NULL))) {
                    let(&str3, cat(str3, " ", str1, " ", NULL));
                    let(&str2, "");
                    str2 = tokenToTex(mathToken[(statement[i].mathString)[j]
                        ].tokenName);
                    /* 2/9/02  Skip any tokens (such as |-) that may be suppressed */
                    if (!str2[0]) continue;
                    /* Convert special characters to HTML entities */
                    for (k = 0; k < strlen(str1); k++) {
                      if (str1[k] == '&') {
                        let(&str1, cat(left(str1, k), "&amp;",
                            right(str1, k + 2), NULL));
                        k = k + 4;
                      }
                      if (str1[k] == '<') {
                        let(&str1, cat(left(str1, k), "&lt;",
                            right(str1, k + 2), NULL));
                        k = k + 3;
                      }
                      if (str1[k] == '>') {
                        let(&str1, cat(left(str1, k), "&gt;",
                            right(str1, k + 2), NULL));
                        k = k + 3;
                      }
                    } /* next k */
                    printLongLine(cat("<TR ALIGN=LEFT><TD>",
                        str2,
                        "</TD><TD><TT>",
                        str1,
                        "</TT></TD></TR>", NULL), "", "\"");
                  }
                } /* next j */
                /* Close out the string now to prevent memory overflow */
                fprintf(texFilePtr, "%s", printString);
                let(&printString, "");
                break;
              case -1: /* Falls through to next case */
              case 0:
                /* Count the number of essential hypotheses k */
                /* Not needed anymore??? since getHTMLHypAndAssertion() */
                /*
                k = 0;
                j = nmbrLen(statement[i].reqHypList);
                for (n = 0; n < j; n++) {
                  if (statement[statement[i].reqHypList[n]].type
                      == (char)e__) {
                    k++;
                  }
                }
                */
                let(&str1, "");
                if (s == 0 || briefHtmlFlag) {
                  let(&str1, "");
                  /* 18-Sep-03 Get HTML hypotheses => assertion */
                  str1 = getHTMLHypAndAssertion(i); /* In mmwtex.c */
                  let(&str1, cat(str1, "</TD></TR>", NULL));
                }
                let(&str2, cat("<TR ALIGN=LEFT><TD><A HREF=\"",
                    statement[i].labelName,
                    ".html\">", statement[i].labelName,
                    "</A>", NULL));
                if (!briefHtmlFlag) {
                  /* Add little pink number */
                  let(&str4, "");
                  str4 = pinkHTML(i);
                  let(&str2, cat(str2, str4, NULL));
                  /* Note: the variable m is the same as the pink statement
                     number here, if we ever need it */
                }
                let(&str1, cat(str2, "</TD><TD>", str1, NULL));
                print2("\n");  /* New line for HTML source readability */
                printLongLine(str1, "", "\"");

                if (s == 0 || briefHtmlFlag) {
                              /* Set s == 0 here for Web site version,
                                 s == s for symbol version of theorem list */
                  /* The below has been replaced by getHTMLHypAndAssertion(i)
                     above. */
                  /*printTexLongMath(statement[i].mathString, "", "", 0, 0);*/
                  /*outputToString = 1;*/ /* Is reset by printTexLongMath */
                } else {
                  /* Theorems are listed w/ description; otherwise file is too
                     big for convenience */
                  let(&str1, "");
                  str1 = getDescription(i);
                  if (strlen(str1) > 29)
                    let(&str1, cat(left(str1, 26), "...", NULL));
                  let(&str1, cat(str1, "</TD></TR>", NULL));
                  printLongLine(str1, "", "\"");

                  /* Close out the string now to prevent overflow */
                  fprintf(texFilePtr, "%s", printString);
                  let(&printString, "");
                }
                break;
            } /* end switch */
          } /* next i (statement number) */
          /* print2("</TABLE></CENTER>\n"); */ /* 8/8/03 Removed - already
              done somewhere else, causing validator.w3.org to fail */
          outputToString = 0;  /* closing will write out the string */

        } else { /* s > 0 */

          /*** Output the html statement body ***/
          typeStatement(showStatement,
              0 /*briefFlag*/,
              0 /*commentOnlyFlag*/,
              1 /*texFlag*/,   /* means latex or html */
              1 /*htmlFlag*/);

        } /* if s <= 0 */

       ABORT_S:
        /*** Close the html file ***/
        printTexTrailer(1 /*texHeaderFlag*/);
        fclose(texFilePtr);
        texFileOpenFlag = 0;
        let(&texFileName,"");

      } /* next s */

      if (!q) {
        /* No matching statement was found */
        printLongLine(cat("?There is no statement whose label matches \"",
            fullArg[2], "\".  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        continue;
      }

      /* Complete the command processing to bypass normal SHOW STATEMENT
         (non-html) below. */
     htmlDone:
      continue;
    } /* if (cmdMatches("SHOW STATEMENT") && switchPos("/ HTML")...) */


    /* If we get here, the user did not specify one of the qualifiers /HTML,
       /BRIEF_HTML, /ALT_HTML, or /BRIEF_ALT_HTML */
    if (cmdMatches("SHOW STATEMENT") && !switchPos("/ HTML")) {

      texFlag = 0;
      if (switchPos("/ TEX") || switchPos("/ HTML")) texFlag = 1;

      briefFlag = 1;
      if (switchPos("/ TEX")) briefFlag = 0;
      if (switchPos("/ FULL")) briefFlag = 0;

      commentOnlyFlag = 0;
      if (switchPos("/ COMMENT")) {
        commentOnlyFlag = 1;
        briefFlag = 1;
      }


      if (switchPos("/ FULL")) {
        briefFlag = 0;
        commentOnlyFlag = 0;
      }

      if (texFlag) {
        if (!texFileOpenFlag) {
          print2(
      "?You have not opened a %s file.  Use the OPEN TEX command first.\n",
              htmlFlag ? "HTML" : "LaTeX");
          continue;
        }
      }

      if (texFlag && (commentOnlyFlag || briefFlag)) {
        print2("?TEX qualifier should be used alone\n");
        continue;
      }

      q = 0;

      for (s = 1; s <= statements; s++) {
        if (!statement[s].labelName[0]) continue; /* No label */
        if (!matches(statement[s].labelName, fullArg[2], '*')) continue;
        if (briefFlag || commentOnlyFlag || texFlag) {
          /* For brief or comment qualifier, if label has wildcards,
             show only $p and $a's */
          if (statement[s].type != (char)p__
              && statement[s].type != (char)a__ && instr(1, fullArg[2], "*"))
            continue;
        }

        if (q && !texFlag) {
          if (!print2("%s\n", string(79, '-'))) /* Put line between
                                                   statements */
            break; /* Break for speedup if user quit */
        }
        if (texFlag) print2("Outputting statement \"%s\"...\n",
            statement[s].labelName);

        q = 1; /* Flag that at least one matching statement was found */

        showStatement = s;


        typeStatement(showStatement,
            briefFlag,
            commentOnlyFlag,
            texFlag,
            htmlFlag);
      } /* Next s */

      if (!q) {
        /* No matching statement was found */
        printLongLine(cat("?There is no statement whose label matches \"",
            fullArg[2], "\".  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        continue;
      }

      if (texFlag && !htmlFlag) {
        print2("The LaTeX source was written to \"%s\".\n", texFileName);
      }
      continue;
    } /* (cmdMatches("SHOW STATEMENT") && !switchPos("/ HTML")) */

    if (cmdMatches("SHOW SETTINGS")) {
      print2("Metamath settings on %s at %s:\n",date(),time_());
      if (commandEcho) {
        print2("(SET ECHO...) Command ECHO is ON.\n");
      } else {
        print2("(SET ECHO...) Command ECHO is OFF.\n");
      }
      if (scrollMode) {
        print2("(SET SCROLL...) SCROLLing mode is PROMPTED.\n");
      } else {
        print2("(SET SCROLL...) SCROLLing mode is CONTINUOUS.\n");
      }
      print2("(SET SCREEN_WIDTH...) SCREEN_WIDTH is %ld.\n", screenWidth);
      if (strlen(input_fn)) {
        print2("(READ...) %ld statements have been read from '%s'.\n",
          statements, input_fn);
      } else {
        print2("(READ...) No source file has been read in yet.\n");
      }
      if (PFASmode) {
        print2("(PROVE...) The statement you are proving is '%s'.\n",
            statement[proveStatement].labelName);
      }
      print2(
   "(SET UNIFICATION_TIMEOUT...) The unification timeout parameter is %ld.\n",
          userMaxUnifTrials);
      print2(
   "(SET SEARCH_LIMIT...) The SEARCH_LIMIT for the IMPROVE command is %ld.\n",
          userMaxProveFloat);
      if (minSubstLen) {
        print2(
    "(SET EMPTY_SUBSTITUTION...) EMPTY_SUBSTITUTION is not allowed (OFF).\n");
      } else {
        print2(
         "(SET EMPTY_SUBSTITUTION...) EMPTY_SUBSTITUTION is allowed (ON).\n");
      }

      if (showStatement) {
        print2("(SHOW...) The default statement for SHOW commands is '%s'.\n",
            statement[showStatement].labelName);
      }
      if (logFileOpenFlag) {
        print2("(OPEN LOG...) The log file '%s' is open.\n", logFileName);
      } else {
        print2("(OPEN LOG...) No log file is currently open.\n");
      }
      if (texFileOpenFlag) {
        print2("The %s file '%s' is open.\n", htmlFlag ? "HTML" : "LaTeX",
            texFileName);
      }
      continue;
    }

    if (cmdMatches("SHOW MEMORY")) {
      /*print2("%ld bytes of data memory have been used.\n",db+db3);*/
      j = 32000000; /* The largest we'ed ever look for */
#ifdef THINK_C
      i = FreeMem();
#else
      i = getFreeSpace(j);
#endif
      if (i > j-3) {
        print2("At least %ld bytes of memory are free.\n",j);
      } else {
        print2("%ld bytes of memory are free.\n",i);
      }
      continue;
    }

    if (cmdMatches("SHOW TRACE_BACK")) {
        for (i = 1; i <= statements; i++) {
          if (!strcmp(fullArg[2],statement[i].labelName)) break;
        }
        if (i > statements) {
          printLongLine(cat("?There is no statement with label \"",
              fullArg[2], "\".  ",
              "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
          showStatement = 0;
          continue;
        }
        showStatement = i;

        essentialFlag = 0;
        axiomFlag = 0;
        endIndent = 0;
        i = switchPos("/ ESSENTIAL");
        if (i) essentialFlag = 1; /* Limit trace to essential steps only */
        i = switchPos("/ ALL");
        if (i) essentialFlag = 0;
        i = switchPos("/ AXIOMS");
        if (i) axiomFlag = 1; /* Limit trace printout to axioms */
        i = switchPos("/ DEPTH"); /* Limit depth of printout */
        if (i) endIndent = val(fullArg[i + 1]);

        i = switchPos("/ TREE");
        if (i) {
          if (axiomFlag) {
            print2(
                "(Note:  The AXIOMS switch is ignored in TREE mode.)\n");
          }
          if (switchPos("/ COUNT_STEPS")) {
            print2(
                "(Note:  The COUNT_STEPS switch is ignored in TREE mode.)\n");
          }
          traceProofTree(showStatement, essentialFlag, endIndent);
        } else {
          if (endIndent != 0) {
           print2(
"(Note:  The DEPTH is ignored if the TREE switch is not used.)\n");
          }

          i = switchPos("/ COUNT_STEPS");
          if (i) {
            countSteps(showStatement, essentialFlag);
          } else {
            traceProof(showStatement, essentialFlag, axiomFlag);
          }
        }

        continue;

    }


    if (cmdMatches("SHOW USAGE")) {
      for (i = 1; i <= statements; i++) {
        if (!strcmp(fullArg[2],statement[i].labelName)) break;
      }
      if (i > statements) {
        printLongLine(cat("?There is no statement with label \"",
            fullArg[2], "\".  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        showStatement = 0;
        continue;
      }
      showStatement = i;

      recursiveFlag = 0;
      i = switchPos("/ RECURSIVE");
      if (i) recursiveFlag = 1; /* Recursive (indirect) usage */
      i = switchPos("/ DIRECT");
      if (i) recursiveFlag = 0; /* Direct references only */

      let(&str1, "");
      str1 = traceUsage(showStatement, recursiveFlag);

      /* Count the number of statements = # of spaces */
      j = strlen(str1) - strlen(edit(str1, 2));

      if (!j) {
        printLongLine(cat("Statement \"",
            statement[showStatement].labelName,
            "\" is not referenced in the proof of any statement.", NULL), "", " ");
      } else {
        if (recursiveFlag) {
          let(&str2, "\" directly or indirectly affects");
        } else {
          let(&str2, "\" is directly referenced in");
        }
        if (j == 1) {
          printLongLine(cat("Statement \"",
              statement[showStatement].labelName,
              str2, " the proof of ",
              str(j), " statement:", NULL), "", " ");
        } else {
          printLongLine(cat("Statement \"",
              statement[showStatement].labelName,
              str2, " the proofs of ",
              str(j), " statements:", NULL), "", " ");
        }
      }

      if (j) {
        let(&str1, cat(" ", str1, NULL));
      } else {
        let(&str1, "  (None)");
      }

      /* Print the output */
      printLongLine(str1, "  ", " ");

      continue;

    }


    if (cmdMatches("SHOW PROOF")
        || cmdMatches("SHOW NEW_PROOF")
        || cmdMatches("SAVE PROOF")
        || cmdMatches("SAVE NEW_PROOF")
        || cmdMatches("MIDI")) {
      if (switchPos("/ HTML")) {
        print2("?HTML qualifier is obsolete - use SHOW STATEMENT * / HTML\n");
        continue;
      }
      if (cmdMatches("SHOW PROOF") || cmdMatches("SAVE PROOF")) {
        pipFlag = 0;
      } else {
        pipFlag = 1; /* Proof-in-progress (NEW_PROOF) flag */
      }
      if (cmdMatches("SHOW")) {
        saveFlag = 0;
      } else {
        saveFlag = 1; /* The command is SAVE PROOF */
      }

      /* Establish defaults for omitted qualifiers */
      startStep = 0;
      endStep = 0;
      startIndent = 0; /* Not used */
      endIndent = 0;
      /*essentialFlag = 0;*/
      essentialFlag = 1; /* 10/9/99 - friendlier default */
      renumberFlag = 0;
      unknownFlag = 0;
      notUnifiedFlag = 0;
      reverseFlag = 0;
      detailStep = 0;
      noIndentFlag = 0;
      splitColumn = DEFAULT_COLUMN;
      texFlag = 0;

      i = switchPos("/ FROM_STEP");
      if (i) startStep = val(fullArg[i + 1]);
      i = switchPos("/ TO_STEP");
      if (i) endStep = val(fullArg[i + 1]);
      i = switchPos("/ DEPTH");
      if (i) endIndent = val(fullArg[i + 1]);
      /* 10/9/99 - ESSENTIAL is retained for downwards compatibility, but is
         now the default, so we ignore it. */
      /*
      i = switchPos("/ ESSENTIAL");
      if (i) essentialFlag = 1;
      */
      i = switchPos("/ ALL");
      if (i) essentialFlag = 0;
      if (i && switchPos("/ ESSENTIAL")) {
        print2("?You may not specify both / ESSENTIAL and / ALL.\n");
        continue;
      }
      i = switchPos("/ RENUMBER");
      if (i) renumberFlag = 1;
      i = switchPos("/ UNKNOWN");
      if (i) unknownFlag = 1;
      i = switchPos("/ NOT_UNIFIED"); /* pip mode only */
      if (i) notUnifiedFlag = 1;
      i = switchPos("/ REVERSE");
      if (i) reverseFlag = 1;
      i = switchPos("/ LEMMON");
      if (i) noIndentFlag = 1;
      i = switchPos("/ COLUMN");
      if (i) splitColumn = val(fullArg[i + 1]);
      i = switchPos("/ TEX") || switchPos("/ HTML");
      if (i) texFlag = 1;


      if (cmdMatches("MIDI")) { /* 8/28/00 */
        midiFlag = 1;
        pipFlag = 0;
        saveFlag = 0;
        let(&labelMatch, fullArg[1]);
        i = switchPos("/ PARAMETER"); /* MIDI only */
        if (i) {
          let(&midiParam, fullArg[i + 1]);
        } else {
          let(&midiParam, "");
        }
      } else {
        midiFlag = 0;
        if (!pipFlag) let(&labelMatch, fullArg[2]);
      }


      if (texFlag) {
        if (!texFileOpenFlag) {
          print2(
     "?You have not opened a %s file.  Use the OPEN %s command first.\n",
              htmlFlag ? "HTML" : "LaTeX",
              htmlFlag ? "HTML" : "TEX");
          continue;
        }
        /**** this is now done after outputting
        print2("The %s source was written to \"%s\".\n",
            htmlFlag ? "HTML" : "LaTeX", texFileName);
        */
      }

      i = switchPos("/ DETAILED_STEP"); /* non-pip mode only */
      if (i) {
        detailStep = val(fullArg[i + 1]);
        if (!detailStep) detailStep = -1; /* To use as flag; error message
                                             will occur in showDetailStep() */
      }

/*??? Need better warnings for switch combinations that don't make sense */

      q = 0;
      for (s = 1; s <= statements; s++) {
        /* If pipFlag (NEW_PROOF), we will iterate exactly once.  This
           loop of course will be entered because there is a least one
           statement, and at the end of the s loop we break out of it. */
        /* If !pipFlag, get the next statement: */
        if (!pipFlag) {
          if (statement[s].type != (char)p__) continue; /* Not $p */
          if (!matches(statement[s].labelName, labelMatch, '*')) continue;
          showStatement = s;
        }

        q = 1; /* Flag that at least one matching statement was found */

        if (detailStep) {
          /* Show the details of just one step */
          showDetailStep(showStatement, detailStep);
          continue;
        }

        if (switchPos("/ STATEMENT_SUMMARY")) { /* non-pip mode only */
          /* Just summarize the statements used in the proof */
          proofStmtSumm(showStatement, essentialFlag, texFlag);
          continue;
        }

        if (switchPos("/ COMPACT") || switchPos("/ NORMAL") ||
            switchPos("/ COMPRESSED") || saveFlag) {
          /*??? Add error msg if other switches were specified. (Ignore them.)*/

          if (!pipFlag) {
            i = showStatement;
          } else {
            i = proveStatement;
          }

          /* Get the amount to indent the proof by */
          indentation = 2 + getSourceIndentation(i);

          if (!pipFlag) {
            parseProof(showStatement);
            /* verifyProof(showStatement); */ /* Not necessary */
            nmbrLet(&nmbrSaveProof, nmbrUnsquishProof(wrkProof.proofString));
          } else {
            nmbrLet(&nmbrSaveProof, proofInProgress.proof);
          }
          if (switchPos("/ COMPACT")  || switchPos("/ COMPRESSED")) {
            nmbrLet(&nmbrSaveProof, nmbrSquishProof(nmbrSaveProof));
          }

          if (switchPos("/ COMPRESSED")) {
            let(&str1, compressProof(nmbrSaveProof, i));
          } else {
            let(&str1, nmbrCvtRToVString(nmbrSaveProof));
          }


          if (saveFlag) {
            /* ??? This is a problem when mixing html and save proof */
            if (printString[0]) bug(1114);
            let(&printString, "");
            outputToString = 1; /* Flag for print2 to add to printString */
          } else {
            if (!print2("Proof of \"%s\":\n", statement[i].labelName))
              break; /* Break for speedup if user quit */
            print2(
"---------Clip out the proof below this line to put it in the source file:\n");
          }
          if (switchPos("/ COMPRESSED")) {
            printLongLine(cat(space(indentation), str1, " $.", NULL),
              space(indentation), "& "); /* "&" is special flag to break
                  compressed part of proof anywhere */
          } else {
            printLongLine(cat(space(indentation), str1, " $.", NULL),
              space(indentation), " ");
          }
          if /*(pipFlag)*/ (1) { /* Add the date proof was created */
            /* 6/13/98 If the proof already has a date stamp, don't add
               a new one.  Note: for the last statement, i + 1
               will refer to a final "dummy" statement containing
               text (comments) through the end of file, stored in its
               labelSectionXxx structure members. */
            let(&str2, space(statement[i + 1].labelSectionLen));
            memcpy(str2, statement[i + 1].labelSectionPtr,
                statement[i + 1].labelSectionLen);
            if (pipFlag && !instr(1, str2, "$([")) {
            /* 6/13/98 end */
              /* No date stamp existed before.  Create one for today's
                 date.  Note that the characters after "$." at the end of
                 the proof normally go in the labelSection of the next
                 statement, but a special mode in outputStatement() (in
                 mmpars.c) will output the date stamp characters for a saved
                 proof. */
              print2("%s$([%s]$)\n", space(indentation), date());
            } else {
              if (saveFlag && instr(1, str2, "$([")) {
                /* An old date stamp existed, and we're saving the proof to
                   the output file.  Make sure the indentation of the old
                   date stamp (which exists in the labelSection of the
                   next statement) matches the indentation of the saved
                   proof.  To do this, we "delete" the indentation spaces
                   on the old date in the labelSection of the next statement,
                   and we put the actual required indentation spaces at
                   the end of the saved proof.  This is done because the
                   labelSectionPtr of the next statement does not point to
                   an isolated string that can be allocated/deallocated but
                   rather to a place in the input source buffer. */
                /* Correct the indentation on old date */
                while ((statement[i + 1].labelSectionPtr)[0] !=
                    '$') {
                  /* "Delete" spaces before old date (by moving source
                     buffer pointer forward), and also "delete"
                     the \n that comes before those spaces */
                  /* If the proof is saved a 2nd time, this loop will
                     not be entered because the pointer will already be
                     at the "$". */
                  (statement[i + 1].labelSectionPtr)++;
                  (statement[i + 1].labelSectionLen)--;
                }
                if (!outputToString) bug(1115);
                /* The final \n will not appear in final output (done in
                   outputStatement() in mmpars.c) because the proofSectionLen
                   below is adjusted to omit it.  This will allow the
                   space(indentation) to appear before the old date without an
                   intervening \n. */
                print2("%s\n", space(indentation));
              }
            }
          }
          if (saveFlag) {
            sourceChanged = 1;
            proofChanged = 0;
            /* ASCII 1 is a flag that string was allocated and not part of
               original source file text buffer */
            let(&printString, cat(chr(1), "\n", printString, NULL));
            if (statement[i].proofSectionPtr[-1] == 1) {
              /* Deallocate old proof if not original source */
              let(&str1, "");
              str1 = statement[i].proofSectionPtr - 1;
              let(&str1, "");
            }
            statement[i].proofSectionPtr = printString + 1;
            /* Subtr 1 char for ASCII 1 at beg, 1 char for "\n" */
            statement[i].proofSectionLen = strlen(printString) - 2;
            printString = "";
            outputToString = 0;
            if (!pipFlag) {
              printLongLine(cat("The proof of \"", statement[i].labelName,
                  "\" has been reformatted and saved internally.",
                  NULL), "", " ");
            } else {
              printLongLine(cat("The new proof of \"", statement[i].labelName,
                  "\" has been saved internally.",
                  NULL), "", " ");
            }
          } else {
            print2(cat(
                "---------The proof of \"",statement[i].labelName,
                "\" to clip out ends above this line.\n",NULL));
          } /* End if saveFlag */
          nmbrLet(&nmbrSaveProof, NULL_NMBRSTRING);
          if (pipFlag) break; /* Only one iteration for NEW_PROOF stuff */
          continue;  /* to next s iteration */
        } /* end if (switchPos("/ COMPACT") || switchPos("/ NORMAL") ||
            switchPos("/ COMPRESSED") || saveFlag) */

        if (saveFlag) bug(1112); /* Shouldn't get here */

        if (!pipFlag) {
          i = showStatement;
        } else {
          i = proveStatement;
        }
        if (texFlag) {
          outputToString = 1; /* Flag for print2 to add to printString */
          if (!htmlFlag) {
            print2("\n");
            print2("\\vspace{1ex}\n");
            printLongLine(cat("Proof of ",
                "{\\tt ",
                asciiToTt(statement[i].labelName),
                "}:", NULL), "", " ");
            print2("\n");
            print2("\n");
          } else {
            bug(1118);
            /*???? The code below is obsolete - now down in show statement*/
            /*
            print2("<CENTER><TABLE BORDER CELLSPACING=0 BGCOLOR=%s>\n",
                MINT_BACKGROUND_COLOR);
            print2("<CAPTION><B>Proof of Theorem <FONT\n");
            printLongLine(cat("   COLOR=", GREEN_TITLE_COLOR, ">",
                asciiToTt(statement[i].labelName),
                "</FONT></B></CAPTION>", NULL), "", "\"");
            print2(
                "<TR><TD><B>Step</B></TD><TD><B>Hyp</B></TD><TD><B>Ref</B>\n");
            print2("</TD><TD><B>Expression</B></TD></TR>\n");
            */
          }
          outputToString = 0;
          /* 8/26/99: Obsolete: */
          /* printTexLongMath in typeProof will do this
          fprintf(texFilePtr, "%s", printString);
          let(&printString, "");
          */
          /* 8/26/99: printTeXLongMath now clears printString in LaTeX
             mode before starting its output, so we must put out the
             printString ourselves here */
          fprintf(texFilePtr, "%s", printString);
          let(&printString, ""); /* We'll clr it anyway */
        } else {
          /* Terminal output - display the statement if wildcard is used */
          if (!pipFlag) {
            if (instr(1, labelMatch, "*")) {
              if (!print2("Proof of \"%s\":\n", statement[i].labelName))
                break; /* Break for speedup if user quit */
            }
          }
        }


        if (texFlag) print2("Outputting proof of \"%s\"...\n",
            statement[s].labelName);

        typeProof(i,
            pipFlag,
            startStep,
            endStep,
            startIndent, /* Not used */
            endIndent,
            essentialFlag,
            renumberFlag,
            unknownFlag,
            notUnifiedFlag,
            reverseFlag,
            noIndentFlag,
            splitColumn,
            texFlag,
            htmlFlag);
        if (texFlag && htmlFlag) {
          outputToString = 1;
          print2("</TABLE></CENTER>\n");
          /* print trailer will close out string later */
          outputToString = 0;
        }

        /*???CLEAN UP */
        /*nmbrLet(&wrkProof.proofString, nmbrSaveProof);*/

        /*E*/ if (0) { /* for debugging: */
          printLongLine(nmbrCvtRToVString(wrkProof.proofString)," "," ");
          print2("\n");

          nmbrLet(&nmbrSaveProof, nmbrSquishProof(wrkProof.proofString));
          printLongLine(nmbrCvtRToVString(nmbrSaveProof)," "," ");
          print2("\n");

          nmbrLet(&nmbrTmp, nmbrUnsquishProof(nmbrSaveProof));
          printLongLine(nmbrCvtRToVString(nmbrTmp)," "," ");

          nmbrLet(&nmbrTmp, nmbrGetTargetHyp(nmbrSaveProof,showStatement));
          printLongLine(nmbrCvtAnyToVString(nmbrTmp)," "," "); print2("\n");

          nmbrLet(&nmbrTmp, nmbrGetEssential(nmbrSaveProof));
          printLongLine(nmbrCvtAnyToVString(nmbrTmp)," "," "); print2("\n");

          cleanWrkProof(); /* Deallocate verifyProof storage */
        } /* end debugging */

        if (pipFlag) break; /* Only one iteration for NEW_PROOF stuff */
      } /* Next s */
      if (!q) {
        /* No matching statement was found */
        printLongLine(cat("?There is no $p statement whose label matches \"",
            fullArg[2],
            "\".  ",
            "Use SHOW LABELS to see list of valid labels.", NULL), "", " ");
      } else {
        if (saveFlag) {
          print2("Remember to use WRITE SOURCE to save changes permanently.\n");
        }
        if (texFlag) {
          print2("The LaTeX source was written to \"%s\".\n", texFileName);
        }
      }

      continue;
    } /* if (cmdMatches("SHOW PROOF")... */

/*E*/ /*???????? DEBUG command for debugging only */
    if (cmdMatches("DBG")) {
      print2("DEBUGGING MODE IS FOR DEVELOPER'S USE ONLY!\n");
      print2("Argument:  %s\n", fullArg[1]);
      nmbrLet(&nmbrTmp, parseMathTokens(fullArg[1], proveStatement));
      for (j = 0; j < 3; j++) {
        print2("Trying depth %ld\n", j);
        nmbrTmpPtr = proveFloating(nmbrTmp, proveStatement, j, 0);
        if (nmbrLen(nmbrTmpPtr)) break;
      }

      print2("Result:  %s\n", nmbrCvtRToVString(nmbrTmpPtr));
      nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);

      continue;
    }
/*E*/ /*???????? DEBUG command for debugging only */

    if (cmdMatches("PROVE")) {
      /*??? Make sure only $p statements are allowed. */
      for (i = 1; i <= statements; i++) {
        if (!strcmp(fullArg[1],statement[i].labelName)) break;
      }
      if (i > statements) {
        printLongLine(cat("?There is no statement with label \"",
            fullArg[1], "\".  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        proveStatement = 0;
        continue;
      }
      proveStatement = i;
      if (statement[proveStatement].type != (char)p__) {
        printLongLine(cat("?Statement \"", fullArg[1],
            "\" is not a $p statement.", NULL), "", " ");
        proveStatement = 0;
        continue;
      }
      print2(
"Entering the Proof Assistant.  HELP PROOF_ASSISTANT for help, EXIT to exit.\n");

      /* Obsolete:
      print2("You will be working on the proof of statement %s:\n",
          statement[proveStatement].labelName);
      printLongLine(cat("  $p ", nmbrCvtMToVString(
          statement[proveStatement].mathString), NULL), "    ", " ");
      */

      PFASmode = 1; /* Set mode for commands here and in mmcmdl.c */
      /* Note:  Proof Assistant mode can equivalently be determined by:
            nmbrLen(proofInProgress.proof) != 0  */

      parseProof(proveStatement);
      verifyProof(proveStatement); /* Necessary??? */
      if (wrkProof.errorSeverity > 1) {
     print2("The starting proof has a severe error.  It will not be used.\n");
        nmbrLet(&nmbrSaveProof, nmbrAddElement(NULL_NMBRSTRING, -(long)'?'));
      } else {
        nmbrLet(&nmbrSaveProof, wrkProof.proofString);
      }
      cleanWrkProof(); /* Deallocate verifyProof storage */

      /* Right now, only non-compact proofs are handled. */
      nmbrLet(&nmbrSaveProof, nmbrUnsquishProof(nmbrSaveProof));

      /* Assign initial proof structure */
      if (nmbrLen(proofInProgress.proof)) bug(1113); /* Should've been deall.*/
      nmbrLet(&proofInProgress.proof, nmbrSaveProof);
      i = nmbrLen(proofInProgress.proof);
      pntrLet(&proofInProgress.target, pntrNSpace(i));
      pntrLet(&proofInProgress.source, pntrNSpace(i));
      pntrLet(&proofInProgress.user, pntrNSpace(i));
      nmbrLet((nmbrString **)(&(proofInProgress.target[i - 1])),
          statement[proveStatement].mathString);
      pipDummyVars = 0;

      /* Assign known subproofs */
      assignKnownSubProofs();

      /* Initialize remaining steps */
      for (j = 0; j < i/*proof length*/; j++) {
        if (!nmbrLen(proofInProgress.source[j])) {
          initStep(j);
        }
      }

      /* Unify whatever can be unified */
      autoUnify(0); /* 0 means no "congrats" message */

      /* Show the user the statement to be proved */
      print2("You will be working on statement (from \"SHOW STATEMENT %s\"):\n",
          statement[proveStatement].labelName);
      typeStatement(proveStatement /*showStatement*/,
          1 /*briefFlag*/,
          0 /*commentOnlyFlag*/,
          0 /*texFlag*/,
          0 /*htmlFlag*/);

      if (!nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
        print2(
        "Note:  The proof you are starting with is already complete.\n");
      } else {

        print2(
     "Unknown step summary (from \"SHOW NEW_PROOF / UNKNOWN\"):\n");
        /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
        typeProof(proveStatement,
            1 /*pipFlag*/,
            0 /*startStep*/,
            0 /*endStep*/,
            0 /*startIndent*/, /* Not used */
            0 /*endIndent*/,
            1 /*essentialFlag*/,
            0 /*renumberFlag*/,
            1 /*unknownFlag*/,
            0 /*notUnifiedFlag*/,
            0 /*reverseFlag*/,
            0 /*noIndentFlag*/,
            0 /*splitColumn*/,
            0 /*texFlag*/,
            0 /*htmlFlag*/);
        /* 6/14/98 end */
      }

      continue;
    }



    if (cmdMatches("UNIFY")) {
      m = nmbrLen(proofInProgress.proof); /* Original proof length */
      proofChangedFlag = 0;
      if (cmdMatches("UNIFY STEP")) {

        s = val(fullArg[2]); /* Step number */
        if (s > m || s < 1) {
          print2("?The step must be in the range from 1 to %ld.\n", m);
          continue;
        }

        interactiveUnifyStep(s - 1, 1); /* 2nd arg. means print msg if
                                           already unified */

        /* (The interactiveUnifyStep handles all messages.) */
        /* print2("... */

        autoUnify(1);
        if (proofChangedFlag) {
          proofChanged = 1; /* Cumulative flag */
        }
        continue;
      }

      /* "UNIFY ALL" */
      if (!switchPos("/ INTERACTIVE")) {
        autoUnify(1);
        if (!proofChangedFlag) {
          print2("No new unifications were made.\n");
        } else {
          proofChanged = 1; /* Cumulative flag */
        }
      } else {
        q = 0;
        while (1) {
          proofChangedFlag = 0; /* This flag is set by autoUnify() and
                                   interactiveUnifyStep() */
          autoUnify(0);
          for (s = m - 1; s >= 0; s--) {
            interactiveUnifyStep(s, 0); /* 2nd arg. means no msg if already
                                           unified */
          }
          autoUnify(1); /* 1 means print congratulations if complete */
          if (!proofChangedFlag) {
            if (!q) print2("No new unifications were made.\n");
            break; /* while (1) */
          } else {
            q = 1; /* Flag that a 2nd pass was done */
            proofChanged = 1; /* Cumulative flag */
          }
        } /* End while (1) */
      }
      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*startIndent*/, /* Not used */
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */
      continue;
    }

    if (cmdMatches("MATCH")) {

      maxEssential = -1; /* Default:  no maximum */
      i = switchPos("/ MAX_ESSENTIAL_HYP");
      if (i) maxEssential = val(fullArg[i + 1]);

      if (cmdMatches("MATCH STEP")) {

        s = val(fullArg[2]); /* Step number */
        m = nmbrLen(proofInProgress.proof); /* Original proof length */
        if (s > m || s < 1) {
          print2("?The step must be in the range from 1 to %ld.\n", m);
          continue;
        }
        if (proofInProgress.proof[s - 1] != -(long)'?') {
          print2(
    "?Step %ld is already assigned.  Only unknown steps can be matched.\n", s);
          continue;
        }

        interactiveMatch(s - 1, maxEssential);
        n = nmbrLen(proofInProgress.proof); /* New proof length */
        if (n != m) {
          if (s != m) {
            printLongLine(cat("Steps ", str(s), ":",
                str(m), " are now ", str(s - m + n), ":", str(n), ".",
                NULL),
                "", " ");
          } else {
            printLongLine(cat("Step ", str(m), " is now step ", str(n), ".",
                NULL),
                "", " ");
          }
        }

        autoUnify(1);
        proofChanged = 1; /* Cumulative flag */

        continue;
      } /* End if MATCH STEP */

      if (cmdMatches("MATCH ALL")) {

        m = nmbrLen(proofInProgress.proof); /* Original proof length */

        k = 0;
        proofChangedFlag = 0;

        if (switchPos("/ ESSENTIAL")) {
          nmbrLet(&nmbrTmp, nmbrGetEssential(proofInProgress.proof));
        }

        for (s = m; s > 0; s--) {
          /* Match only unknown steps */
          if (proofInProgress.proof[s - 1] != -(long)'?') continue;
          /* Match only essential steps if specified */
          if (switchPos("/ ESSENTIAL")) {
            if (!nmbrTmp[s - 1]) continue;
          }

          interactiveMatch(s - 1, maxEssential);
          if (proofChangedFlag) {
            k = s; /* Save earliest step changed */
            proofChangedFlag = 0;
          }
          print2("\n");
        }
        if (k) {
          proofChangedFlag = 1; /* Restore it */
          proofChanged = 1; /* Cumulative flag */
          print2("Steps %ld and above have been renumbered.\n", k);
        }
        autoUnify(1);

        continue;
      } /* End if MATCH ALL */
    }

    if (cmdMatches("LET")) {

      errorCount = 0;
      nmbrLet(&nmbrTmp, parseMathTokens(fullArg[4], proveStatement));
      if (errorCount) {
        /* Parsing issued error message(s) */
        errorCount = 0;
        continue;
      }

      if (cmdMatches("LET VARIABLE")) {
        if (((vstring)(fullArg[2]))[0] != '$') {
          print2(
    "?The target variable must be of the form \"$<integer>\", e.g. \"$23\".\n");
          continue;
        }
        n = val(right(fullArg[2], 2));
        if (n < 1 || n > pipDummyVars) {
          print2("?The target variable must be between $1 and $%ld.\n",
              pipDummyVars);
          continue;
        }

        replaceDummyVar(n, nmbrTmp);

        autoUnify(1);


        proofChangedFlag = 1; /* Flag to push 'undo' stack */
        proofChanged = 1; /* Cumulative flag */

      }
      if (cmdMatches("LET STEP")) {

        s = val(fullArg[2]); /* Step number */
        m = nmbrLen(proofInProgress.proof); /* Original proof length */
        if (s > m || s < 1) {
          print2("?The step must be in the range from 1 to %ld.\n", m);
          continue;
        }

        /* Check to see if the statement selected is allowed */
        if (!checkMStringMatch(nmbrTmp, s - 1)) {
          printLongLine(cat("?Step ", str(s), " cannot be unified with \"",
              nmbrCvtMToVString(nmbrTmp),"\".", NULL), " ", " ");
          continue;
        }

        /* Assign the user string */
        nmbrLet((nmbrString **)(&(proofInProgress.user[s - 1])), nmbrTmp);

        autoUnify(1);
        proofChangedFlag = 1; /* Flag to push 'undo' stack */
        proofChanged = 1; /* Cumulative flag */
      }
      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*startIndent*/, /* Not used */
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */
      continue;
    }


    if (cmdMatches("ASSIGN")) {

      /* 10/4/99 - Added LAST - this means the last unknown step shown
         with SHOW NEW_PROOF/ESSENTIAL/UNKNOWN */
      let(&str1, fullArg[1]); /* To avoid void pointer problems with fullArg */
      if (toupper(str1[0]) == 'L') { /* "ASSIGN LAST" */
        s = 1; /* Temporary until we figure out which step */
      } else {
        s = val(fullArg[1]); /* Step number */
        if (strcmp(fullArg[1], str(s))) {
          print2("?Expected either a number or LAST after ASSIGN.\n");
          continue;
        }
      }

      for (i = 1; i <= statements; i++) {
        if (!strcmp(fullArg[2], statement[i].labelName)) {
          /* If a $e or $f, it must be a hypothesis of the statement
             being proved */
          if (statement[i].type == (char)e__ || statement[i].type == (char)f__){
            if (!nmbrElementIn(1, statement[proveStatement].reqHypList, i) &&
                !nmbrElementIn(1, statement[proveStatement].optHypList, i))
                continue;
          }
          break;
        }
      }
      if (i > statements) {
        printLongLine(cat("?The statement with label \"",
            fullArg[2],
            "\" was not found or is not a hypothesis of the statement ",
            "being proved.  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        continue;
      }
      k = i;

      if (k >= proveStatement) {
        print2(
   "?You must specify a statement that occurs earlier the one being proved.\n");
        continue;
      }

      m = nmbrLen(proofInProgress.proof); /* Original proof length */
      if (s > m || s < 1) {
        print2("?The step must be in the range from 1 to %ld.\n", m);
        continue;
      }

      /* 10/4/99 - For ASSIGN LAST command, figure out the last unknown
         essential step */
      if (toupper(str1[0]) == 'L') { /* "ASSIGN LAST" */
        /* Get the essential step flags */
        nmbrLet(&essentialFlags, nmbrGetEssential(proofInProgress.proof));
        /* Scan proof backwards until last essential unknown step is found */
        s = 0; /* Use as flag that step was found */
        for (i = m; i >= 1; i--) {
          if (essentialFlags[i - 1]
              && proofInProgress.proof[i - 1] == -(long)'?') {
            /* Found it */
            s = i;
            break;
          }
        } /* Next i */
        if (s == 0) {
          print2("?There are no unknown essential steps.\n");
          continue;
        }
      }

      /* Check to see that the step is an unknown step */
      if (proofInProgress.proof[s - 1] != -(long)'?') {
        print2(
        "?Step %ld is already assigned.  You can only assign unknown steps.\n"
            , s);
        continue;
      }

      /* Check to see if the statement selected is allowed */
      if (!checkStmtMatch(k, s - 1)) {
        print2("?Statement \"%s\" cannot be unified with step %ld.\n",
          statement[k].labelName, s);
        continue;
      }

      assignStatement(k /*statement#*/, s - 1 /*step*/);

      n = nmbrLen(proofInProgress.proof); /* New proof length */
      autoUnify(1);

      /* Automatically interact with user if step not unified */
      /* ???We might want to add a setting to defeat this if user doesn't
         like it */
      /* 10/4/99 Since ASSIGN LAST is typically run from a commmand file, don't
         interact so response is predictable */
      if (toupper(str1[0]) != 'L') { /* not "ASSIGN LAST" */
        interactiveUnifyStep(s - m + n - 1, 2); /* 2nd arg. means print msg if
                                                 already unified */
      }
      if (m == n) {
        print2("Step %ld was assigned statement %s.\n",
          s, statement[k].labelName);
      } else {
        if (s != m) {
          printLongLine(cat("Step ", str(s),
              " was assigned statement ", statement[k].labelName,
              ".  Steps ", str(s), ":",
              str(m), " are now ", str(s - m + n), ":", str(n), ".",
              NULL),
              "", " ");
        } else {
          printLongLine(cat("Step ", str(s),
              " was assigned statement ", statement[k].labelName,
              ".  Step ", str(m), " is now step ", str(n), ".",
              NULL),
              "", " ");
        }
      }

      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*startIndent*/, /* Not used */
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */


      proofChangedFlag = 1; /* Flag to push 'undo' stack */
      proofChanged = 1; /* Cumulative flag */
      continue;

    } /* cmdMatches("ASSIGN") */


    if (cmdMatches("REPLACE")) {
      s = val(fullArg[1]); /* Step number */

      /* This limitation is due to the assignKnownSteps call below which
         does not tolerate unknown steps. */
      /******* 10/20/02  Limitation removed
      if (nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
        print2("?The proof must be complete before you can use REPLACE.\n");
        continue;
      }
      *******/

      for (i = 1; i <= statements; i++) {
        if (!strcmp(fullArg[2], statement[i].labelName)) {
          /* If a $e or $f, it must be a hypothesis of the statement
             being proved */
          if (statement[i].type == (char)e__ || statement[i].type == (char)f__){
            if (!nmbrElementIn(1, statement[proveStatement].reqHypList, i) &&
                !nmbrElementIn(1, statement[proveStatement].optHypList, i))
                continue;
          }
          break;
        }
      }
      if (i > statements) {
        printLongLine(cat("?The statement with label \"",
            fullArg[2],
            "\" was not found or is not a hypothesis of the statement ",
            "being proved.  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        continue;
      }
      k = i;

      if (k >= proveStatement) {
        print2(
   "?You must specify a statement that occurs earlier the one being proved.\n");
        continue;
      }

      m = nmbrLen(proofInProgress.proof); /* Original proof length */
      if (s > m || s < 1) {
        print2("?The step must be in the range from 1 to %ld.\n", m);
        continue;
      }
      /* Check to see that the step is a known step */
      if (proofInProgress.proof[s - 1] == -(long)'?') {
        print2(
        "?Step %ld is unknown.  You can only replace known steps.\n"
            , s);
        continue;
      }

      /* 10/20/02  Set a flag that proof has unknown steps (for autoUnify()
         call below) */
      if (nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
        p = 1;
      } else {
        p = 0;
      }

      /* Check to see if the statement selected is allowed */
      if (!checkStmtMatch(k, s - 1)) {
        print2("?Statement \"%s\" cannot be unified with step %ld.\n",
          statement[k].labelName, s);
        continue;
      }

      /* Do the replacement */
      nmbrTmpPtr = replaceStatement(k /*statement#*/, s - 1 /*step*/,
          proveStatement);
      if (!nmbrLen(nmbrTmpPtr)) {
        print2(
           "?Hypotheses of statement \"%s\" do not match known proof steps.\n",
            statement[k].labelName);
        continue;
      }

      /* Get the subproof at step s */
      q = subProofLen(proofInProgress.proof, s - 1);
      deleteSubProof(s - 1);
      addSubProof(nmbrTmpPtr, s - q);

      /* 10/20/02 Replaced "assignKnownSteps" with code from entry of PROVE
         command so REPLACE can be done in partial proofs */
      /*assignKnownSteps(s - q, nmbrLen(nmbrTmpPtr));*/  /* old code */
      /* Assign known subproofs */
      assignKnownSubProofs();
      /* Initialize remaining steps */
      i = nmbrLen(proofInProgress.proof);
      for (j = 0; j < i; j++) {
        if (!nmbrLen(proofInProgress.source[j])) {
          initStep(j);
        }
      }
      /* Unify whatever can be unified */
      /* If proof wasn't complete before (p = 1), but is now, print congrats
         for user */
      autoUnify(p); /* 0 means no "congrats" message */
      /* end 10/20/02 */


      nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);

      n = nmbrLen(proofInProgress.proof); /* New proof length */
      if (m == n) {
        print2("Step %ld was replaced with statement %s.\n",
          s, statement[k].labelName);
      } else {
        if (s != m) {
          printLongLine(cat("Step ", str(s),
              " was replaced with statement ", statement[k].labelName,
              ".  Steps ", str(s), ":",
              str(m), " are now ", str(s - m + n), ":", str(n), ".",
              NULL),
              "", " ");
        } else {
          printLongLine(cat("Step ", str(s),
              " was replaced with statement ", statement[k].labelName,
              ".  Step ", str(m), " is now step ", str(n), ".",
              NULL),
              "", " ");
        }
      }
      /*autoUnify(1);*/

      /* Automatically interact with user if step not unified */
      /* ???We might want to add a setting to defeat this if user doesn't
         like it */
      if (1 /* ???Future setting flag */) {
        interactiveUnifyStep(s - m + n - 1, 2); /* 2nd arg. means print msg if
                                                 already unified */
      }

      proofChangedFlag = 1; /* Flag to push 'undo' stack */
      proofChanged = 1; /* Cumulative flag */
      continue;

    } /* REPLACE */


    if (cmdMatches("IMPROVE")) {

      k = 0; /* Depth */
      i = switchPos("/ DEPTH");
      if (i) k = val(fullArg[i + 1]);

      if (cmdMatches("IMPROVE STEP") || cmdMatches("IMPROVE LAST")) {

        /* 10/4/99 - Added LAST - this means the last unknown step shown
           with SHOW NEW_PROOF/ESSENTIAL/UNKNOWN */
        if (cmdMatches("IMPROVE LAST")) { /* "ASSIGN LAST" */
          s = 1; /* Temporary until we figure out which step */
        } else {
          s = val(fullArg[2]); /* Step number */
        }

        m = nmbrLen(proofInProgress.proof); /* Original proof length */
        if (s > m || s < 1) {
          print2("?The step must be in the range from 1 to %ld.\n", m);
          continue;
        }

        /* 10/4/99 - For IMPROVE LAST command, figure out the last unknown
           essential step */
        if (cmdMatches("IMPROVE LAST")) {
          /* Get the essential step flags */
          nmbrLet(&essentialFlags, nmbrGetEssential(proofInProgress.proof));
          /* Scan proof backwards until last essential unknown step is found */
          s = 0; /* Use as flag that step was found */
          for (i = m; i >= 1; i--) {
            if (essentialFlags[i - 1]
                && proofInProgress.proof[i - 1] == -(long)'?') {
              /* Found it */
              s = i;
              break;
            }
          } /* Next i */
          if (s == 0) {
            print2("?There are no unknown essential steps.\n");
            continue;
          }
        }

        /* Get the subproof at step s */
        q = subProofLen(proofInProgress.proof, s - 1);
        nmbrLet(&nmbrTmp, nmbrSeg(proofInProgress.proof, s - q + 1, s));

        /*???Shouldn't this be just known?*/
        /* Check to see that the subproof has an unknown step. */
        if (!nmbrElementIn(1, nmbrTmp, -(long)'?')) {
          print2(
              "?Step %ld already has a proof and cannot be improved.\n",
              s);
          continue;
        }

        /* Check to see that the step has no dummy variables. */
        j = 0; /* Break flag */
        for (i = 0; i < nmbrLen(proofInProgress.target[s - 1]); i++) {
          if (((nmbrString *)(proofInProgress.target[s - 1]))[i] > mathTokens) {
            j = 1;
            break;
          }
        }
        if (j) {
          print2(
   "?Step %ld target has dummy variables and cannot be improved.\n", s);
          continue;
        }

        nmbrTmpPtr = proveFloating(proofInProgress.target[s - 1],
            proveStatement, k, s - 1);
        if (!nmbrLen(nmbrTmpPtr)) {
          print2("A proof for step %ld was not found.\n", s);
          continue;
        }
        /* If q=1, subproof must be an unknown step, so don't bother to
           delete it */
        /*???Won't q always be 1 here?*/
        if (q > 1) deleteSubProof(s - 1);
        addSubProof(nmbrTmpPtr, s - q);
        assignKnownSteps(s - q, nmbrLen(nmbrTmpPtr));
        nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);

        n = nmbrLen(proofInProgress.proof); /* New proof length */
        if (m == n) {
          print2("A 1-step proof was found for step %ld.\n", s);
        } else {
          if (s != m || q != 1) {
            printLongLine(cat("A ", str(n - m + 1),
                "-step proof was found for step ", str(s),
                ".  Steps ", str(s), ":",
                str(m), " are now ", str(s - q + 1 - m + n), ":", str(n), ".",
                NULL),
                "", " ");
          } else {
            printLongLine(cat("A ", str(n - m + 1),
                "-step proof was found for step ", str(s),
                ".  Step ", str(m), " is now step ", str(n), ".",
                NULL),
                "", " ");
          }
        }

        autoUnify(1); /* To get 'congrats' message if proof complete */
        proofChanged = 1; /* Cumulative flag */

      } /* End if IMPROVE STEP */


      if (cmdMatches("IMPROVE ALL")) {

        m = nmbrLen(proofInProgress.proof); /* Original proof length */

        n = 0; /* Earliest step that changed */
        proofChangedFlag = 0;

        for (s = m; s > 0; s--) {

          /*???Shouldn't this be just known?*/
          /* If the step is known and unified, don't do it, since nothing
             would be accomplished. */
          if (proofInProgress.proof[s - 1] != -(long)'?') {
            if (nmbrEq(proofInProgress.target[s - 1],
                proofInProgress.source[s - 1])) continue;
          }

          /*???Won't q always be 1 here?*/
          /* Get the subproof at step s */
          q = subProofLen(proofInProgress.proof, s - 1);
          nmbrLet(&nmbrTmp, nmbrSeg(proofInProgress.proof, s - q + 1, s));

          /* Improve only subproofs with unknown steps */
          if (!nmbrElementIn(1, nmbrTmp, -(long)'?')) continue;

          nmbrLet(&nmbrTmp, NULL_NMBRSTRING); /* No longer needed - deallocate */

          /* Check to see that the step has no dummy variables. */
          j = 0; /* Break flag */
          for (i = 0; i < nmbrLen(proofInProgress.target[s - 1]); i++) {
            if (((nmbrString *)(proofInProgress.target[s - 1]))[i] >
                mathTokens) {
              j = 1;
              break;
            }
          }
          if (j) {
            /* Step has dummy variables and cannot be improved. */
            continue;
          }

          nmbrTmpPtr = proveFloating(proofInProgress.target[s - 1],
              proveStatement, k, s - 1);
          if (!nmbrLen(nmbrTmpPtr)) {
            /* A proof for the step was not found. */
            continue;
          }

          /* If q=1, subproof must be an unknown step, so don't bother to
             delete it */
          /*???Won't q always be 1 here?*/
          if (q > 1) deleteSubProof(s - 1);
          addSubProof(nmbrTmpPtr, s - q);
          assignKnownSteps(s - q, nmbrLen(nmbrTmpPtr));
          print2("A proof of length %ld was found for step %ld.\n",
              nmbrLen(nmbrTmpPtr), s);
          if (nmbrLen(nmbrTmpPtr) || q != 1) n = s - q + 1;
                                                /* Save earliest step changed */
          nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);
          proofChangedFlag = 1;
          s = s - q + 1; /* Adjust step position to account for deleted subpr */
        } /* Next step s */
        if (n) {
          print2("Steps %ld and above have been renumbered.\n", n);
        }
        autoUnify(1); /* To get 'congrats' msg if done */

        if (!proofChangedFlag) {
          print2("No new subproofs were found.\n");
        } else {
          proofChanged = 1; /* Cumulative flag */
        }

      } /* End if IMPROVE ALL */

      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      if (proofChangedFlag)
        typeProof(proveStatement,
            1 /*pipFlag*/,
            0 /*startStep*/,
            0 /*endStep*/,
            0 /*startIndent*/, /* Not used */
            0 /*endIndent*/,
            1 /*essentialFlag*/,
            0 /*renumberFlag*/,
            1 /*unknownFlag*/,
            0 /*notUnifiedFlag*/,
            0 /*reverseFlag*/,
            0 /*noIndentFlag*/,
            0 /*splitColumn*/,
            0 /*texFlag*/,
            0 /*htmlFlag*/);
      /* 6/14/98 end */

      continue;

    }


    if (cmdMatches("MINIMIZE_WITH")) {
      q = 0; /* Line length */
      s = 0; /* Status flag */
      i = switchPos("/ BRIEF"); /* Non-verbose mode */
      j = switchPos("/ ALLOW_GROWTH"); /* Mode to replace even if
                                       if doesn't reduce proof length */
      p = switchPos("/ NO_DISTINCT"); /* Skip trying statements with $d */
      if (j) j = 1; /* Make sure it's a char value for minimizeProof call */
      for (k = 1; k < proveStatement; k++) {

        if (statement[k].type != (char)p__ && statement[k].type != (char)a__)
          continue;
        if (!matches(statement[k].labelName, fullArg[1], '*')) continue;
        if (p) {
          /* Skip the statement if it has a $d requirement.  This option
             prevents illegal minimizations that would violate $d requirements
             since MINIMIZE_WITH does not check for $d violations. */
          if (nmbrLen(statement[k].reqDisjVarsA)) {
            if (!instr(1, fullArg[1], "*"))
              print2("?\"%s\" has a $d requirement\n", fullArg[1]);
            continue;
          }
        }
        /* Print individual labels */
        if (s == 0) s = 1; /* Matched at least one */
        if (!i) {
          /* Verbose mode */
          q = q + strlen(statement[k].labelName) + 1;
          if (q > 72) {
            q = strlen(statement[k].labelName) + 1;
            print2("\n");
          }
          print2("%s ",statement[k].labelName);
        }

        m = nmbrLen(proofInProgress.proof); /* Original proof length */
        nmbrLet(&nmbrTmp, proofInProgress.proof);

        minimizeProof(k, proveStatement, j);

        n = nmbrLen(proofInProgress.proof); /* New proof length */
        if (!nmbrEq(nmbrTmp, proofInProgress.proof)) {
          if (!i) {
            /* Verbose mode */
            print2("\n");
          }
          if (n < m) print2(
            "Proof length of \"%s\" decreased from %ld to %ld using \"%s\".\n",
              statement[proveStatement].labelName,
              m, n, statement[k].labelName);
          /* ALLOW_GROWTH possibility */
          if (m < n) print2(
            "Proof length of \"%s\" increased from %ld to %ld using \"%s\".\n",
              statement[proveStatement].labelName,
              m, n, statement[k].labelName);
          /* ALLOW_GROWTH possibility */
          if (m == n) print2(
              "Proof length of \"%s\" remained at %ld using \"%s\".\n",
              statement[proveStatement].labelName,
              m, statement[k].labelName);
          /* Distinct variable warning (??? future - add $d to Proof Assis.) */
          if (nmbrLen(statement[k].reqDisjVarsA)) {
            printLongLine(cat("Note: \"", statement[k].labelName,
                "\" has $d constraints.",
                "  SAVE NEW_PROOF then VERIFY PROOF to check them.",
                NULL), "", " ");
          }
          q = 0; /* Line length for label list */
          s = 2; /* Found one */
          proofChangedFlag = 1;
          proofChanged = 1; /* Cumulative flag */
        }

      } /* Next k (statement) */
      if (!i) {
        /* Verbose mode */
        if (s) print2("\n");
      }
      if (s == 1 && !j) print2("No shorter proof was found.\n");
      if (s == 1 && j) print2("The proof was not changed.\n");
      if (!s && !p) print2("?No earlier label matches \"%s\".\n", fullArg[1]);
      if (!s && p) {
        if (instr(1, fullArg[1], "*"))
          print2("?No earlier label (without $d) matches \"%s\".\n",
              fullArg[1]);
     }

      continue;

    } /* End if MINIMIZE_WITH */


    if (cmdMatches("REVERT")) {
      /*??? Implement UNDO stack */
      continue;
    }



    if (cmdMatches("DELETE STEP") || (cmdMatches("DELETE ALL"))) {

      if (cmdMatches("DELETE STEP")) {
        s = val(fullArg[2]); /* Step number */
      } else {
        s = nmbrLen(proofInProgress.proof);
      }
      if (proofInProgress.proof[s - 1] == -(long)'?') {
        print2("?Step %ld is unknown and cannot be deleted.\n", s);
        continue;
      }
      m = nmbrLen(proofInProgress.proof); /* Original proof length */
      if (s > m || s < 1) {
        print2("?The step must be in the range from 1 to %ld.\n", m);
        continue;
      }

      deleteSubProof(s - 1);
      n = nmbrLen(proofInProgress.proof); /* New proof length */
      if (m == n) {
        print2("Step %ld was deleted.\n", s);
      } else {
        if (n > 1) {
          printLongLine(cat("A ", str(m - n + 1),
              "-step subproof at step ", str(s),
              " was deleted.  Steps ", str(s), ":",
              str(m), " are now ", str(s - m + n), ":", str(n), ".",
              NULL),
              "", " ");
        } else {
          print2("The entire proof was deleted.\n");
        }
      }

      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*startIndent*/, /* Not used */
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */

      proofChangedFlag = 1;/* Flag for UNDO stack */
      proofChanged = 1; /* Cumulative flag */

      continue;

    }

    if (cmdMatches("DELETE FLOATING_HYPOTHESES")) {

      /* Get the essential step flags */
      nmbrLet(&nmbrTmp, nmbrGetEssential(proofInProgress.proof));

      m = nmbrLen(proofInProgress.proof); /* Original proof length */

      n = 0; /* Earliest step that changed */
      proofChangedFlag = 0;

      for (s = m; s > 0; s--) {

        /* Skip essential steps and unknown steps */
        if (nmbrTmp[s - 1] == 1) continue; /* Not floating */
        if (proofInProgress.proof[s - 1] == -(long)'?') continue; /* Unknown */

        /* Get the subproof length at step s */
        q = subProofLen(proofInProgress.proof, s - 1);

        deleteSubProof(s - 1);

        n = s - q + 1; /* Save earliest step changed */
        proofChangedFlag = 1;
        s = s - q + 1; /* Adjust step position to account for deleted subpr */
      } /* Next step s */

      if (proofChangedFlag) {
        print2("All floating-hypothesis steps were deleted.\n");

        if (n) {
          print2("Steps %ld and above have been renumbered.\n", n);
        }

        /* 6/14/98 - Automatically display new unknown steps
           ???Future - add switch to enable/defeat this */
        typeProof(proveStatement,
            1 /*pipFlag*/,
            0 /*startStep*/,
            0 /*endStep*/,
            0 /*startIndent*/, /* Not used */
            0 /*endIndent*/,
            1 /*essentialFlag*/,
            0 /*renumberFlag*/,
            1 /*unknownFlag*/,
            0 /*notUnifiedFlag*/,
            0 /*reverseFlag*/,
            0 /*noIndentFlag*/,
            0 /*splitColumn*/,
            0 /*texFlag*/,
            0 /*htmlFlag*/);
        /* 6/14/98 end */

        proofChanged = 1; /* Cumulative flag */
      } else {
        print2("?There are no floating-hypothesis steps to delete.\n");
      }

      continue;

    } /* End if DELETE FLOATING_HYPOTHESES */

    if (cmdMatches("INITIALIZE")) {

      if (cmdMatches("INITIALIZE ALL")) {
        i = nmbrLen(proofInProgress.proof);

        /* Reset the dummy variable counter (all will be refreshed) */
        pipDummyVars = 0;

        /* Initialize all steps */
        for (j = 0; j < i; j++) {
          initStep(j);
        }

        /* Assign known subproofs */
        assignKnownSubProofs();

        print2("All steps have been initialized.\n");
        proofChanged = 1; /* Cumulative flag */
        continue;
      }

      /* cmdMatches("INITIALIZE STEP") */
      s = val(fullArg[2]); /* Step number */
      if (s > nmbrLen(proofInProgress.proof) || s < 1) {
        print2("?The step must be in the range from 1 to %ld.\n",
            nmbrLen(proofInProgress.proof));
        continue;
      }

      initStep(s - 1);
      print2(
          "Step %ld and its hypotheses have been initialized.\n",
          s);

      proofChanged = 1; /* Cumulative flag */
      continue;

    }


    if (cmdMatches("SEARCH")) {
      if (switchPos("/ ALL")) {
        m = 1;  /* Include $e, $f statements */
      } else {
        m = 0;  /* Show $a, $p only */
      }
      if (switchPos("/ COMMENTS")) {
        n = 1;  /* Search comments */
      } else {
        n = 0;  /* Search statement math symbols */
      }

      let(&str1, fullArg[2]); /* String to match */

      if (n) { /* COMMENTS switch */
        /* Trim leading, trailing spaces; reduce white space to space;
           convert to upper case */
        let(&str1, edit(str1, 8 + 16 + 128 + 32));
      } else { /* No COMMENTS switch */
        /* Trim leading, trailing spaces; reduce white space to space */
        let(&str1, edit(str1, 8 + 16 + 128));

        /* Change all spaces to double spaces */
        q = strlen(str1);
        let(&str3, space(q + q));
        s = 0;
        for (p = 0; p < q; p++) {
          str3[p + s] = str1[p];
          if (str1[p] == ' ') {
            s++;
            str3[p + s] = str1[p];
          }
        }
        let(&str1, left(str3, q + s));

        /* Change wildcard to ASCII 2 (to be different from printable chars) */
        /* 1/3/02 (Why are we matching with and without space? I'm not sure.)*/
        /*        For the future should we put this before the initial space */
        /*        reduction? */
        while (1) {
          p = instr(1, str1, " $* ");
          if (!p) break;
          let(&str1, cat(left(str1, p - 1), chr(2), right(str1, p + 4), NULL));
        }
        while (1) {
          p = instr(1, str1, "$*");
          if (!p) break;
          let(&str1, cat(left(str1, p - 1), chr(2), right(str1, p + 2), NULL));
        }
        /* 1/3/02  Also allow a plain $ as a wildcard, for convenience */
        /*         For the future should this be the only wildcard? */
        while (1) {
          p = instr(1, str1, " $ ");
          if (!p) break;
          let(&str1, cat(left(str1, p - 1), chr(2), right(str1, p + 2), NULL));
        }
        while (1) {
          p = instr(1, str1, "$");
          if (!p) break;
          let(&str1, cat(left(str1, p - 1), chr(2), right(str1, p + 1), NULL));
        }
        /* Add wildcards to beginning and end to match middle of any string */
        let(&str1, cat(chr(2), " ", str1, " ", chr(2), NULL));
      } /* End no COMMENTS switch */

      for (i = 1; i <= statements; i++) {
        if (!statement[i].labelName[0]) continue; /* No label */
        if (!m && statement[i].type != (char)p__ &&
            statement[i].type != (char)a__) continue; /* No /ALL switch */
        if (!matches(statement[i].labelName, fullArg[1], '*')) continue;
        if (n) { /* COMMENTS switch */
          let(&str2, "");
          str2 = getDescription(i); /* str2 must be deallocated here */
          /* Strip linefeeds and reduce spaces; cvt to uppercase */
          j = instr(1, edit(str2, 4 + 8 + 16 + 128 + 32), str1);
          if (!j) { /* No match */
            let(&str2, "");
            continue;
          }
          /* Strip linefeeds and reduce spaces */
          let(&str2, edit(str2, 4 + 8 + 16 + 128));
          j = j + (strlen(str1) / 2); /* Center of match location */
          p = screenWidth - 7 - strlen(str(i)) - strlen(statement[i].labelName);
                        /* Longest comment portion that will fit in one line */
          q = strlen(str2); /* Length of comment */
          if (q <= p) { /* Use entire comment */
            let(&str3, str2);
          } else {
            if (q - j <= p / 2) { /* Use right part of comment */
              let(&str3, cat("...", right(str2, q - p + 4), NULL));
            } else {
              if (j <= p / 2) { /* Use left part of comment */
                let(&str3, cat(left(str2, p - 3), "...", NULL));
              } else { /* Use middle part of comment */
                let(&str3, cat("...", mid(str2, j - p / 2, p - 6), "...",
                    NULL));
              }
            }
          }
          print2("%s\n", cat(str(i), " ", statement[i].labelName, " $",
              chr(statement[i].type), " \"", str3, "\"", NULL));
          let(&str2, "");
        } else { /* No COMMENTS switch */
          let(&str2,nmbrCvtMToVString(statement[i].mathString));

          /* Change all spaces to double spaces */
          q = strlen(str2);
          let(&str3, space(q + q));
          s = 0;
          for (p = 0; p < q; p++) {
            str3[p + s] = str2[p];
            if (str2[p] == ' ') {
              s++;
              str3[p + s] = str2[p];
            }
          }
          let(&str2, left(str3, q + s));

          let(&str2, cat(" ", str2, " ", NULL));
          if (!matches(str2, str1, 2/* ascii 2 match char*/)) continue;
          let(&str2, edit(str2, 8 + 16 + 128)); /* Trim leading, trailing
              spaces; reduce white space to space */
          printLongLine(cat(str(i)," ",
              statement[i].labelName, " $", chr(statement[i].type), " ", str2,
              NULL), "    ", " ");
        } /* End no COMMENTS switch */
      } /* Next i */
      continue;
    }


    if (cmdMatches("SET ECHO")) {
      if (cmdMatches("SET ECHO ON")) {
        commandEcho = 1;
        print2("SET ECHO ON\n");
        print2("Command line echoing is now turned on.\n");
      } else {
        commandEcho = 0;
        print2("Command line echoing is now turned off.\n");
      }
      continue;
    }

    if (cmdMatches("SET MEMORY_STATUS")) {
      if (cmdMatches("SET MEMORY_STATUS ON")) {
        print2("Memory status display has been turned on.\n");
        print2("This command is intended for debugging purposes only.\n");
        memoryStatus = 1;
      } else {
        memoryStatus = 0;
        print2("Memory status display has been turned off.\n");
      }
      continue;
    }


    if (cmdMatches("SET HENTY_FILTER")) {
      if (cmdMatches("SET HENTY_FILTER ON")) {
        print2("The unification equivalence filter has been turned on.\n");
        print2("This command is intended for debugging purposes only.\n");
        hentyFilter = 1;
      } else {
        print2("This command is intended for debugging purposes only.\n");
        print2("The unification equivalence filter has been turned off.\n");
        hentyFilter = 0;
      }
      continue;
    }


    if (cmdMatches("SET EMPTY_SUBSTITUTION")) {
      if (cmdMatches("SET EMPTY_SUBSTITUTION ON")) {
        minSubstLen = 0;
        print2("Substitutions with empty symbol sequences is now allowed.\n");
        continue;
      }
      if (cmdMatches("SET EMPTY_SUBSTITUTION OFF")) {
        minSubstLen = 1;
        print2(
"The ability to substitute empty expressions has been turned off.  Note\n");
        print2(
"that this may make the Proof Assistant too restrictive in some cases.\n");
        continue;
      }
    }


    if (cmdMatches("SET SEARCH_LIMIT")) {
      s = val(fullArg[2]); /* Timeout value */
      print2("IMPROVE search limit has been changed from %ld to %ld\n",
          userMaxProveFloat, s);
      userMaxProveFloat = s;
      continue;
    }

    if (cmdMatches("SET SCREEN_WIDTH")) {
      s = val(fullArg[2]); /* Screen width value */
      if (s >= PRINTBUFFERSIZE - 1) {
        print2(
"?Maximum screen width is %ld.  Recompile with larger PRINTBUFFERSIZE in\n",
            (long)(PRINTBUFFERSIZE - 2));
        print2("mminou.h if you need more.\n");
        continue;
      }
      print2("Screen width has been changed from %ld to %ld\n",
          screenWidth, s);
      screenWidth = s;
      continue;
    }


    if (cmdMatches("SET UNIFICATION_TIMEOUT")) {
      s = val(fullArg[2]); /* Timeout value */
      print2("Unification timeout has been changed from %ld to %ld\n",
          userMaxUnifTrials,s);
      userMaxUnifTrials = s;
      continue;
    }


    if (cmdMatches("OPEN LOG")) {
        /* Open a log file */
        let(&logFileName, fullArg[2]);
        logFilePtr = fSafeOpen(logFileName, "w");
        if (!logFilePtr) continue; /* Couldn't open it (err msg was provided) */
        logFileOpenFlag = 1;
        print2("The log file \"%s\" was opened %s %s.\n",logFileName,
            date(),time_());
        continue;
    }

    if (cmdMatches("CLOSE LOG")) {
        /* Close the log file */
        if (!logFileOpenFlag) {
          print2("?Sorry, there is no log file currently open.\n");
        } else {
          print2("The log file \"%s\" was closed %s %s.\n",logFileName,
              date(),time_());
          fclose(logFilePtr);
          logFileOpenFlag = 0;
        }
        let(&logFileName,"");
        continue;
    }

    if (cmdMatches("OPEN TEX") || cmdMatches("OPEN HTML")) {
      if (cmdMatches("OPEN HTML")) {
        print2("?OPEN HTML is obsolete - use SHOW STATEMENT * / HTML\n");
        continue;
      }
      if (texDefsRead) {
        /* Current limitation - can only read .def once */
        if (cmdMatches("OPEN HTML") != htmlFlag) {
          print2("?You cannot use both LaTeX and HTML in the same session.\n");
          print2(
              "?You must EXIT and restart Metamath to switch to the other.\n");
          continue;
        }
      }
      htmlFlag = cmdMatches("OPEN HTML");

      /* Open a TeX file */
      let(&texFileName,fullArg[2]);
      if (switchPos("/ NO_HEADER")) {
        texHeaderFlag = 0;
      } else {
        texHeaderFlag = 1;
      }
      texFilePtr = fSafeOpen(texFileName,"w");
      if (!texFilePtr) continue; /* Couldn't open it (err msg was provided) */
      texFileOpenFlag = 1;
      print2("Created %s output file \"%s\".\n",
          htmlFlag ? "HTML" : "LaTeX", texFileName);
      printTexHeader(texHeaderFlag);
      continue;
    }

    if (cmdMatches("CLOSE TEX") || cmdMatches("CLOSE HTML")) {
      if (cmdMatches("CLOSE HTML")) {
        print2("?CLOSE HTML is obsolete - use SHOW STATEMENT * / HTML\n");
        continue;
      }
      /* Close the TeX file */
      if (!texFileOpenFlag) {
        print2("?Sorry, there is no %s file currently open.\n",
            htmlFlag ? "HTML" : "LaTeX");
      } else {
        print2("The %s output file \"%s\" has been closed.\n",
            htmlFlag ? "HTML" : "LaTeX", texFileName);
        printTexTrailer(texHeaderFlag);
        fclose(texFilePtr);
        texFileOpenFlag = 0;
      }
      let(&texFileName,"");
      continue;
    }

    if (cmdMatches("FILE TYPE")) {
      /* Type the contents of the file on the screen */

      type_fp = fSafeOpen(fullArg[2], "r");
      if (!type_fp) continue; /* Couldn't open it (error msg was provided) */
      fromLine = 0;
      toLine = 0;
      i = switchPos("/ FROM_LINE");
      if (i) fromLine = val(fullArg[i + 1]);
      i = switchPos("/ TO_LINE");
      if (i) toLine = val(fullArg[i + 1]);

      j = 0; /* Line # */
      while (linput(type_fp, NULL, &str1)) {
        j++;
        if (j < fromLine && fromLine != 0) continue;
        if (j > toLine && toLine != 0) break;
        if (!print2("%s\n", str1)) break;
      }

      fclose(type_fp);

      continue;
    }


    if (cmdMatches("FILE SEARCH")) {
      /* Search the contents of a file and type on the screen */

      type_fp = fSafeOpen(fullArg[2], "r");
      if (!type_fp) continue; /* Couldn't open it (error msg was provided) */
      fromLine = 0;
      toLine = 0;
      searchWindow = 0;
      i = switchPos("/ FROM_LINE");
      if (i) fromLine = val(fullArg[i + 1]);
      i = switchPos("/ TO_LINE");
      if (i) toLine = val(fullArg[i + 1]);
      i = switchPos("/ WINDOW");
      if (i) searchWindow = val(fullArg[i + 1]);
      /*??? Implement SEARCH /WINDOW */
      if (i) print2("Sorry, WINDOW has not be implemented yet.\n");

      let(&str2, fullArg[3]); /* Search string */
      let(&str2, edit(str2, 32)); /* Convert to upper case */

      tmpFlag = 0;

      /* Search window buffer */
      pntrLet(&pntrTmp, pntrSpace(searchWindow));

      j = 0; /* Line # */
      m = 0; /* # matches */
      while (linput(type_fp, NULL, &str1)) {
        j++;
        if (j > toLine && toLine != 0) break;
        if (j >= fromLine || fromLine == 0) {
          let(&str3, edit(str1, 32)); /* Convert to upper case */
          if (instr(1, str3, str2)) { /* Match occurred */
            if (!tmpFlag) {
              tmpFlag = 1;
              print2(
                    "The line number in the file is shown before each line.\n");
            }
            m++;
            if (!print2("%ld:  %s\n", j, left(str1,
                MAX_LEN - strlen(str(j)) - 3))) break;
          }
        }
        for (k = 1; k < searchWindow; k++) {
          let((vstring *)(&pntrTmp[k - 1]), pntrTmp[k]);
        }
        if (searchWindow > 0)
            let((vstring *)(&pntrTmp[searchWindow - 1]), str1);
      }
      if (!tmpFlag) {
        print2("There were no matches.\n");
      } else {
        if (m == 1) {
          print2("There was %ld matching line in the file %s.\n", m,
              fullArg[1]);
        } else {
          print2("There were %ld matching lines in the file %s.\n", m,
              fullArg[1]);
        }
      }

      fclose(type_fp);

      /* Deallocate search window buffer */
      for (i = 0; i < searchWindow; i++) {
        let((vstring *)(&pntrTmp[i]), "");
      }
      pntrLet(&pntrTmp, NULL_PNTRSTRING);


      continue;
    }


    if (cmdMatches("SET UNIVERSE") || cmdMatches("ADD UNIVERSE") ||
        cmdMatches("DELETE UNIVERSE")) {

      /*continue;*/ /* ???Not implemented */
    } /* end if xxx UNIVERSE */



    if (cmdMatches("SET DEBUG FLAG")) {
      print2("Notice:  The DEBUG mode is intended for development use only.\n");
      print2("The printout will not be meaningful to the user.\n");
      i = val(fullArg[3]);
      if (i == 4) db4 = 1;
      if (i == 5) db5 = 1;
      if (i == 6) db6 = 1;
      if (i == 7) db7 = 1;
      if (i == 8) db8 = 1;
      if (i == 9) db9 = 1;
      continue;
    }
    if (cmdMatches("SET DEBUG OFF")) {
      db4 = 0;
      db5 = 0;
      db6 = 0;
      db7 = 0;
      db8 = 0;
      db9 = 0;
      print2("The DEBUG mode has been turned off.\n");
      continue;
    }

    if (cmdMatches("ERASE")) {
      if (sourceChanged) {
        print2("Warning:  You have not saved changes to the source.\n");
        str1 = cmdInput1("Do you want to ERASE anyway (Y, N) <N>? ");
        if (str1[0] != 'y' && str1[0] != 'Y') {
          print2("Use WRITE SOURCE to save the changes.\n");
          continue;
        }
        sourceChanged = 0;
      }
      eraseSource();
      showStatement = 0;
      proveStatement = 0;
      print2("Metamath has been reset to the starting state.\n");
      continue;
    }

    if (cmdMatches("VERIFY PROOF")) {
      if (switchPos("/ SYNTAX_ONLY")) {
        verifyProofs(fullArg[2],0); /* Parse only */
      } else {
        verifyProofs(fullArg[2],1); /* Parse and verify */
      }
      continue;
    }

    print2("?This command has not been implemented.\n");
    continue;

  }
} /* command */


/* Compare strings via pointers */
int qsortStringCmp(const void *p1, const void *p2)
{
  vstring tmp = "";
  long i1, i2;
  int r;
  /* Returns -1 if p1 < p2, 0 if equal, 1 if p1 > p2 */
  if (qsortKey[0] == 0) {
    /* No key, use full line */
    return strcmp(*(char * const *)p1, *(char * const *)p2);
  } else {
    i1 = instr(1, *(char * const *)p1, qsortKey);
    i2 = instr(1, *(char * const *)p2, qsortKey);
    r = strcmp(
        right(*(char * const *)p1, i1),
        right(*(char * const *)p2, i2));
    let(&tmp, ""); /* Deallocate temp string stack */
    return r;
  }
}

