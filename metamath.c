/*****************************************************************************/
/*               Copyright (C) 1997, NORMAN D. MEGILL                        */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust text window width) 678901234567*/

/* Metamath Proof Verifier - main program */
/* See the book "Metamath" for description of Metamath and run instructions */

/* The overall functionality of the modules is as follows:
    metamath.c - Contains main(); executes or calls commands
    mmcmdl.c - Command line interpreter
    mmcmds.c - Executes SHOW and some other commands
    mmdata.c - Defines global data structures and manipulates arrays
               with functions similar to BASIC string functions;
               memory management; converts between proof formats
    mmhlpa.c - The help file, part 1.
    mmhlpb.c - The help file, part 2.
    mminou.c - Basic input and output interface
    mmmaci.c - Macintosh interface (not used in this version)
    mmpars.c - Parses the source file
    mmpfas.c - Proof Assistant
    mmunif.c - Unification algorithm for Proof Assistant
    mmutil.c - Miscellaneous I/O utilities
    mmveri.c - Proof verifier for source file
    mmvstr.c - BASIC-like string functions
    mmwtex.c - LaTeX source generation
    mmword.c - Microsoft Word source generation (not written yet)
*/

/*****************************************************************************/
/* ------------- Compilation Instructions ---------------------------------- */
/*****************************************************************************/

/* In all cases below, make sure each .c file (except metamath.c) has its
   corresponding .h file present. */

/*****************************************************************************/
/* To compile with gnu c on Unix, use the following command:
  gcc \
  metamath.c \
  mmcmdl.c \
  mmcmds.c \
  mmdata.c \
  mmhlpa.c \
  mmhlpb.c \
  mminou.c \
  mmmaci.c \
  mmpars.c \
  mmpfas.c \
  mmunif.c \
  mmutil.c \
  mmveri.c \
  mmvstr.c \
  mmwtex.c \
  mmword.c \
  -o metamath -O
*/

/*****************************************************************************/
/* To compile, link and run with VAX C on VMS use the following commands:
  $ cc metamath.c
  $ cc mmcmdl.c
  $ cc mmcmds.c
  $ cc mmdata.c
  $ cc mmhlpa.c
  $ cc mmhlpb.c
  $ cc mminou.c
  $ cc mmmaci.c
  $ cc mmpars.c
  $ cc mmpfas.c
  $ cc mmunif.c
  $ cc mmutil.c
  $ cc mmveri.c
  $ cc mmvstr.c
  $ cc mmwtex.c
  $ cc mmword.c
  $ link metamath,mmcmdl,mmcmds,mmhlpa,mmhlpb,mminou,mmmaci,mmpars,mmveri,-
    mmdata,mmvstr,mmutil,mmwtex,mmword,mmpfas,mmunif
  $ run metamath
*/

/*****************************************************************************/
/* To compile and link with gnu c on VMS, use the following commands:
$ gcc metamath.c
$ gcc mmcmdl.c
$ gcc mmcmds.c
$ gcc mmdata.c
$ gcc mmhlpa.c
$ gcc mmhlpb.c
$ gcc mminou.c
$ gcc mmmaci.c
$ gcc mmpars.c
$ gcc mmpfas.c
$ gcc mmunif.c
$ gcc mmutil.c
$ gcc mmveri.c
$ gcc mmvstr.c
$ gcc mmwtex.c
$ gcc mmword.c
$ link metamath,mmcmdl,mmcmds,mmhlpa,mmhlpb,mminou,mmmaci,mmpars,mmveri,-
    mmdata,mmvstr,mmutil,mmwtex,mmword,mmpfas,mmunif,-
sys$input/opt
gnu_cc:[000000]gcclib/lib
sys$share:decw$dxmlibshr/share
sys$share:decw$xmlibshr/share
sys$share:decw$xlibshr/share
sys$share:vaxcrtl/share
$
*/


/*****************************************************************************/
/* To compile on Macintosh (System 7.1) under THINK C 5.0.4:
   Install "ANSI" and "MacTraps" libraries.
   Add in each .c file listed above.
   Use a separate segment for each library and .c file.
   
   Make sure your settings are as follows:
   
   [x] or (o) means setting below should be selected.
   [ ] or ( ) means setting should not be selected.
   
   Options... (under Edit menu):
     Language Settings
       ANSI Conformance
         [x] #define __STDC__
         [x] Recognize trigraphs
         [x] enums are always ints
         [x] Check pointer types
       [x] Langauage Extensions
         (o) THINK C
         ( ) THINK C + Objects
       [x] Strict Prototype Enforcement
         ( ) Infer prototypes
         (o) Require prototypes
     Compiler Settings
       [ ] Generate 68020 instructions
       [ ] Generate 68881 instructions
       [x] Classes are indirect by default
       [x] Methods are virtual by default
       [x] Optimize monomorphic methods
       [ ] 4-byte ints
       [ ] 8-byte doubles
       [x] "\p" is unsigned char []
       [ ] Native floating-point format
     Code Optimization
       [x] Defer & combine stack adjusts
       [x] Suppress redundant loads
       [x] Automatic Register Assignment
         [ ] Honor 'register' first
       [ ] Use Global Optimizer
         [x] Induction variable elimination
         [x] CSE elimination
         [x] Code motion
         [x] Register coloring
     Prefix
       #include <MacHeaders>
   Set Project Type... (under Project menu):
     (o) Application
     ( ) Desk Accessory
     ( ) Device Driver
     ( ) Code Resource
     File Type APPL
     Creator MMAT
     Partition (K) 4000
     SIZE Flags 0000
     [x] Far CODE
     [ ] Far DATA
     [x] Separate STRS       
*/

/*****************************************************************************/
/*----------------------------------------------------------------
---------------------DOS 5.0 compilation instructions ------------
------------------------------------------------------------------

Prerequisites:  1. WATCOM C/386 compiler, and
                2. Borland 16-bit C compiler, and
                3. Rational Systems DOS/4GW DOS extender, and
                4. 80386 CPU

Instructions:

1. Place the following files in the Metamath directory:
     All .c files listed above
     All .h files listed above
     DOS4GW.EXE (from Rational Systems DOS extender)
     LOAD.C (from listing below)
     COMPILE.BAT (from listing below)
     LOAD.BAT (from listing below)
     LINK.BAT (from listing below)

2. Type the following commands:
     COMPILE
     LOAD
     LINK

Note:  There will be several compilation warnings during the LOAD command.
These are normal; ignore them.
  
3. The program will be called METAMATH.EXE.  The files DOS4GW.EXE and
METAMATH.EXE should always be together in the same directory.
*/

/*---------------- Begin LOAD.C (16-bit DOS stub) ----------------
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
char *paths_to_check[] = {"PATH"};
char *dos4g_path()
{   static char fullpath[80];
    int i;
    for( i = 0;
	 i < sizeof( paths_to_check ) / sizeof( paths_to_check[0] ); i++ ) {
	_searchenv( "dos4gw.exe", paths_to_check[i], fullpath );
	if( fullpath[0] ) return( &fullpath );
    }   
    for( i = 0;
	 i < sizeof( paths_to_check ) / sizeof( paths_to_check[0] ); i++ ) {
	_searchenv( "dos4g.exe", paths_to_check[i], fullpath );
	if( fullpath[0] ) return( &fullpath );
    }   
    return( "dos4gw.exe" );
}
main( int argc, char *argv[] )
{
    int         i,j;
    char        *cmd,*cp,*ztz="";
    char        *av[4];
    char        buffer[256];
    FILE        *fp;
    char        fn[256];
    av[0] = dos4g_path();
    av[1] = "METAMATH";
    strcpy(buffer,"");
    for (i=1; i<argc; i++) 
    {
      cp=argv[i];
      while (*cp)
      {
	*cp=toupper(*cp);
	cp++;
      }
      if (!strcmp(argv[i],"/XXX")) 
	;
      else
      {
	strcat(buffer," ");
	strcat(buffer,argv[i]);
      }
    }
    cmd=(char *)malloc(strlen(buffer)+1);
    strcpy(cmd,buffer);
    av[2] = cmd;
    av[3] = NULL;
    putenv( "DOS4G=QUIET" );
    execvp( av[0], av );
    printf( "\n");
    puts( "Stub exec failed:" );
    puts( av[0] );
    puts( strerror( errno ) );
    exit( 1 );
}
------------------------------- End of LOAD.C -----------------------------*/

/*----------------------------- Begin COMPILE.BAT --------------------------
@echo off
rem To use debugger, add /d2 and remove /d1 
rem To use WATCOM 9.0 add /p
set ccommand=wcl386 /k32768 /c /zq /j /d2
rem set ccommand=wcl386 /k32768 /c /zq /j /d1
rem *** Which file to compile?
if "%1"=="" goto compile_all
if "%1"=="all" goto compile_all
rem *** Compile one file
@echo on
%ccommand% %1.c
@echo off
goto the_end
rem *** Compile all files
:compile_all
@echo on
%ccommand% *.c
@echo off
:the_end
--------------------------End COMPILE.BAT -------------------------------*/

/*------------------------Begin LOAD.BAT --------------------------------
erase load.bin
erase load.exe
bcc -eload load.c
erase load.obj
rename load.exe load.bin
--------------------------End LOAD.BAT ----------------------------------*/

/*------------------------Begin LINK.BAT ---------------------------------
wcl386 /d2 /fe=metamath /k32768 /zq *.obj /"option stub=load.bin"
--------------------------End LINK.BAT ----------------------------------*/

/* You can use this alternate LOAD.C if you want Metamath to use up to
   32 MByte of virtual memory.  The swap file, METAMATH.SWP, is
   normally deleted when you exit Metamth.  You should ensure that 32 MByte
   of disk space is available on your system before running Metamath. */
/*---------------- Begin LOAD.C (virtual memory version) ----------------
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
char *paths_to_check[] = {"PATH"};
char *dos4g_path()
{   static char fullpath[80];
    int i;
    for( i = 0;
	 i < sizeof( paths_to_check ) / sizeof( paths_to_check[0] ); i++ ) {
	_searchenv( "dos4gw.exe", paths_to_check[i], fullpath );
	if( fullpath[0] ) return( &fullpath );
    }   
    for( i = 0;
	 i < sizeof( paths_to_check ) / sizeof( paths_to_check[0] ); i++ ) {
	_searchenv( "dos4g.exe", paths_to_check[i], fullpath );
	if( fullpath[0] ) return( &fullpath );
    }   
    return( "dos4gw.exe" );
}
main( int argc, char *argv[] )
{
    int         i,j;
    char        *cmd,*cp,*ztz="";
    char        *av[4];
    char        buffer[256];
    FILE        *fp;
    char        fn[256];
    av[0] = dos4g_path();
    av[1] = "METAMATH";
    strcpy(buffer,"");
    for (i=1; i<argc; i++) 
    {
      cp=argv[i];
      while (*cp)
      {
	*cp=toupper(*cp);
	cp++;
      }
      if (!strcmp(argv[i],"/XXX")) 
	;
      else
      {
	strcat(buffer," ");
	strcat(buffer,argv[i]);
      }
    }
    cmd=(char *)malloc(strlen(buffer)+1);
    strcpy(cmd,buffer);
    av[2] = cmd;
    av[3] = NULL;
    putenv( "DOS4G=QUIET" );
    cp=getenv("DOS4GVM");
    if (!cp || *cp=='\0' || *cp=='0')
    {
      strcpy(buffer,"DOS4GVM=");
      strcat(buffer,"DELETESWAP ");
      strcat(buffer,"VIRTUALSIZE#32768 ");
      strcat(buffer,"SWAPMIN#8192 ");
      strcat(buffer,"SWAPINC#4096 ");
      if (ztz)
      {
	strcat(buffer,"SWAPNAME#");
	strcat(buffer,ztz);
	strcat(buffer,"\\METAMATH.SWP ");
      }
      putenv( buffer );
    }
    execvp( av[0], av );
    printf( "\n");
    puts( "Stub exec failed:" );
    puts( av[0] );
    puts( strerror( errno ) );
    exit( 1 );
}
------------------------------- End of LOAD.C (virtual memory version) ------*/

/*----------------------------------------------------------------
---------------End of DOS 5.0 compilation instructions -----------
----------------------------------------------------------------*/


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
/* #include "mmword.h" */
#include "mmwtex.h"
#ifdef THINK_C
#include "mmmaci.h"
#endif

void command(void);

main()
{

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

  print2("Metamath - Version 0.06 4-May-97\n");
  print2(
     "Copyright (C) 1997 Norman D. Megill, 19 Locke Ln., Lexington MA 02173\n");
  print2("Type HELP for help, EXIT to exit.\n");

  /* Allocate big arrays */
  initBigArrays();

  /* Process a command line until EXIT */
  command();
}



void command(void)
{
  /* Command line user interface -- fetches and processes a command; returns 1
    if the command is 'EXIT' and never returns otherwise. */

  /* The variables in command() are static so that they won't be destroyed
    by a longjmp return to setjmp. */
  long i,j,k,m,n,p,q,s,tokenNum,statemNum;
  vstring str1 = "", str2 = "", str3 = "";
  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated directly */
  nmbrString *nmbrTmpPtr1; /* Pointer only; not allocated directly */
  nmbrString *nmbrTmpPtr2; /* Pointer only; not allocated directly */
  nmbrString *nmbrTmp = NULL_NMBRSTRING;
  nmbrString *nmbrSaveProof = NULL_NMBRSTRING;
  pntrString *pntrTmpPtr; /* Pointer only; not allocated directly */
  pntrString *pntrTmpPtr1; /* Pointer only; not allocated directly */
  pntrString *pntrTmpPtr2; /* Pointer only; not allocated directly */
  pntrString *pntrTmp = NULL_PNTRSTRING;
  pntrString *expandedProof = NULL_PNTRSTRING;
  flag type, tmpFlag, tmpFlag1;

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
  
  flag axiomFlag; /* For SHOW TRACE_BACK */
  flag recursiveFlag; /* For SHOW USAGE */
  long fromLine, toLine; /* For TYPE, SEARCH */
  long searchWindow; /* For SEARCH */
  FILE *type_fp; /* For TYPE, SEARCH */
  long maxEssential; /* For MATCH */
  
  flag texHeaderFlag; /* For OPEN TEX, CLOSE TEX */
  flag commentOnlyFlag; /* For SHOW STATEMENT */
  flag briefFlag; /* For SHOW STATEMENT */

  
  while (1) {

    errorCount = 0; /* Reset error count before each read or proof parse. */
    
    /* Deallocate stuff that may have been used in previous pass */
    let(&str1,"");
    let(&str2,"");
    let(&str3,"");
    nmbrLet(&nmbrTmp,NULL_NMBRSTRING);
    pntrLet(&pntrTmp,NULL_PNTRSTRING);
    nmbrLet(&nmbrSaveProof,NULL_NMBRSTRING);
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
    /* (End of space deallocation) */

    let(&commandLine,""); /* Deallocate previous contents */

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

    if (PFASmode) {
      let(&commandPrompt,"MM-PA> ");
    } else {
      let(&commandPrompt,"MM> ");
/*E*/      /* let(&commandPrompt,"LIST> "); */
    }
    commandLine = cmdInput1(commandPrompt);
    
    /* See if it's an operating system command */
    /* (This is a command line that begins with a quote) */
    if (commandLine[0] == '\'' || commandLine[0] == '\"') {
      /* See if this computer has this feature */
      if (!system(NULL)) {
        print2("This computer does not accept an operating system command.\n");
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

    if (commandEcho) {
      /* Build the complete command and print it for the user */
      k = pntrLen(fullArg);
      let(&str1,"");
      for (i = 0; i < k; i++) {
        if (instr(1, fullArg[i], " ")) {
          /* If the argument has spaces, put quotes around it */
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
      /* The tilde is a special flag for printLongLine to print a 
         tilde before the carriage return in a split line, not after */
      printLongLine(left(str1,strlen(str1) - 1),"~"," ");
    }

    if (cmdMatches("BEEP")) {
      /* Print a bell (if user types ahead "B", the bell lets him know when
         his command is finished - useful for long-running commands */
      print2("%c",7);
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

      if (!errorCount) {
        let(&str1, "No errors were found.");
        if (!switchPos("/ VERIFY")) {
            let(&str1, cat(str1, 
       "  However, proofs were not checked.  Use VERIFY PROOF *",
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
        writeInput();
        fclose(output_fp);
        sourceChanged = 0;
        continue;
    }

    if (cmdMatches("WRITE DICTIONARY")) {
        print2("?This command has not been implemented.\n");
/*???*/continue;  /*NOT IMPLEMENTED YET*/
        let(&tex_dict_fn, fullArg[2]);
        tex_dict_fp = fSafeOpen(tex_dict_fn, "w");
        if (!tex_dict_fp) continue;  /* Couldn't open (err msg was provided)*/
        writeDict();
        fclose(tex_dict_fp);
        continue;
    }

    if (cmdMatches("SHOW LABELS")) {
        if (switchPos("/ ALL")) {
          m = 1;  /* Include $e, $f statements */
          print2(
  "The labels that match are shown with statement number, label, and type.\n");
        } else {
          m = 0;  /* Show $a, $p only */
          print2(
"The assertions that match are shown with statement number, label, and type.\n");
        }
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
          if (j + strlen(str1) > MAX_LEN) {
            print2("\n");
            j = 0;
            k = 0;
          }
          if (strlen(str1) > COL) {
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
          printLongLine(cat("?The statement with label \"",
              fullArg[2],
              "\" could not be found.  ",
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
    }

    if (cmdMatches("SHOW STATEMENT")) {
        for (i = 1; i <= statements; i++) {
          if (!strcmp(fullArg[2],statement[i].labelName)) break;
        }
        if (i > statements) {
          printLongLine(cat("?The statement with label \"",
              fullArg[2],
              "\" could not be found.  ",
              "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
          showStatement = 0;
          continue;
        }
        showStatement = i;

        texFlag = 0;
        if (switchPos("/ TEX")) texFlag = 1;
        
        commentOnlyFlag = 0;
        if (switchPos("/ COMMENT_ONLY")) commentOnlyFlag = 1;

        briefFlag = 0;
        if (switchPos("/ BRIEF")) briefFlag = 1;

        if (texFlag) {
          if (!texFileOpenFlag) {
            print2(
       "?You have not opened a LaTeX file.  Use the OPEN TEX command first.\n");
            continue;
          }
        }
        
        /*
        if (commentOnlyFlag && briefFlag) {
          print2("?COMMENT_ONLY and BRIEF make no sense together.\n");
        }
        */

        if (!commentOnlyFlag && !briefFlag) {
          let(&str1, cat("Statement ", str(showStatement),
              " is located on line ", str(statement[showStatement].lineNum),
              " of the file ", NULL));
          if (!texFlag) {
            printLongLine(cat(str1,
              "\"", statement[showStatement].fileName,
              "\".",NULL), "", " ");
          } else {
            let(&printString, "");
            outputToString = 1; /* Flag for print2 to add to printString */
            printLongLine(cat(str1, "{\\tt ",
                asciiToTt(statement[showStatement].fileName),
                "}.", NULL), "", " ");
            outputToString = 0;
          }
        }

        if (!briefFlag || commentOnlyFlag) {
          let(&str1, "");
          str1 = getDescription(showStatement);
          if (str1[0]) {
            if (!texFlag) {
              printLongLine(cat("\"", str1, "\"", NULL), "", " ");
            } else {
              printTexComment(str1);
            }
          }
        }
        if (texFlag) {
          print2("The LaTeX source was written to \"%s\".\n",
              texFileName);
        }
        if (commentOnlyFlag && !briefFlag) continue;
       
        if (briefFlag) {
          /* For BRIEF mode, print $e hypotheses (only) before statement */
          if (texFlag) outputToString = 0;
          j = nmbrLen(statement[showStatement].reqHypList);
          for (i = 0; i < j; i++) {
            k = statement[showStatement].reqHypList[i];
            if (statement[k].type != (char)e__) continue;
            let(&str2, cat("  ",statement[k].labelName,
                " $", chr(statement[k].type), " ", NULL));
            if (!texFlag) {
              printLongLine(cat(str2,
                  nmbrCvtMToVString(statement[k].mathString), " $.", NULL),
                  "      "," ");
            } else {
              let(&str3, space(strlen(str2)));
              printTexLongMath(statement[k].mathString,
                  str2, str3);
            }
          }
        }
        
        let(&str1, "");
        type = statement[showStatement].type;
        if (type == p__) let(&str1, " $= ...");
        let(&str2, cat("  ", statement[showStatement].labelName,
            " $",chr(type), " ", NULL));
        if (!texFlag) {
          printLongLine(cat(str2,
              nmbrCvtMToVString(statement[showStatement].mathString),
              str1, " $.", NULL), "      ", " ");
        } else {
          let(&str3, space(strlen(str2)));
          printTexLongMath(statement[showStatement].mathString,
              str2, str3);
        }
        
        if (briefFlag) continue;
        
        switch (type) {
          case a__:
          case p__:
            if (texFlag) {
              outputToString = 1;
              print2("\n"); /* New paragraph */
            }
            print2("Its mandatory hypotheses in RPN order are:\n");
            if (texFlag) outputToString = 0;
            j = nmbrLen(statement[showStatement].reqHypList);
            for (i = 0; i < j; i++) {
              k = statement[showStatement].reqHypList[i];
              let(&str2, cat("  ",statement[k].labelName,
                  " $", chr(statement[k].type), " ", NULL));
              if (!texFlag) {
                printLongLine(cat(str2,
                    nmbrCvtMToVString(statement[k].mathString), " $.", NULL),
                    "      "," ");
              } else {
                let(&str3, space(strlen(str2)));
                printTexLongMath(statement[k].mathString,
                    str2, str3);
              }
            }
            if (texFlag) {
              outputToString = 1;
              print2("\n"); /* New paragraph */
            }
            if (j == 0) print2("  (None)\n");
            if (texFlag) outputToString = 0;
            nmbrTmpPtr1 = statement[showStatement].reqDisjVarsA;
            nmbrTmpPtr2 = statement[showStatement].reqDisjVarsB;
            i = nmbrLen(nmbrTmpPtr1);
            if (i) {
              let(&str1, "");
              for (k = 0; k < i; k++) {
                let(&str1, cat(str1, ", <",
                    mathToken[nmbrTmpPtr1[k]].tokenName, ",",
                    mathToken[nmbrTmpPtr2[k]].tokenName, ">", NULL));
              }
              printLongLine(cat("Its mandatory disjoint variable pairs are:  ",
                  right(str1,3),NULL),"  "," ");
            }
            if (type == p__ &&
                nmbrLen(statement[showStatement].optHypList)) {
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
              let(&str1, "");
              for (k = 0; k < i; k++) {
                let(&str1, cat(str1, ", <",
                    mathToken[nmbrTmpPtr1[k]].tokenName, ",",
                    mathToken[nmbrTmpPtr2[k]].tokenName, ">", NULL));
              }
              printLongLine(cat("Its optional disjoint variable pairs are:  ",
                  right(str1,3),NULL),"  "," ");
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
          fprintf(texFilePtr, "%s", printString);
          let(&printString, "");
        }
        continue;
    }

    if (cmdMatches("SHOW SETTINGS")) {
      print2("Metamath settings on %s at %s:\n",date(),time_());
      if (commandEcho) {
        print2("Command ECHO is ON.\n");
      } else {
        print2("Command ECHO is OFF.\n");
      }
      if (scrollMode) {
        print2("SCROLLing mode is PROMPTED.\n");
      } else {
        print2("SCROLLing mode is CONTINUOUS.\n");
      }
      print2("SCREEN_WIDTH is %ld.\n", screenWidth);
      if (strlen(input_fn)) {
        print2("%ld statements have been read from '%s'.\n",statements,
          input_fn);
      } else {
        print2("No source file has been read in yet.\n");
      }
      if (PFASmode) {
        print2("The statement you are proving is '%s'.\n",
            statement[proveStatement].labelName);
      }
      print2("The unification timeout parameter is %ld.\n",
          userMaxUnifTrials);
      print2("The SEARCH_LIMIT for the IMPROVE command is %ld.\n",
          userMaxProveFloat);
      if (minSubstLen) {
        print2("EMPTY_SUBSTITUTION is not allowed (OFF).\n");
      } else {
        print2("EMPTY_SUBSTITUTION is allowed (ON).\n");
      }
      
      if (showStatement) {
        print2("The default statement for SHOW commands is '%s'.\n",
            statement[showStatement].labelName);
      }
      if (logFileOpenFlag) {
        print2("The log file '%s' is open.\n", logFileName);
      }
      if (texFileOpenFlag) {
        print2("The LaTeX file '%s' is open.\n", texFileName);
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
          printLongLine(cat("?The statement with label \"",
              fullArg[2],
              "\" could not be found.  ",
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
          printLongLine(cat("?The statement with label \"",
              fullArg[2],
              "\" could not be found.  ",
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
        
        traceUsage(showStatement, recursiveFlag);
        
        continue;
        
    }  


    if (cmdMatches("SHOW PROOF")
        || cmdMatches("SHOW NEW_PROOF")
        || cmdMatches("SAVE PROOF")
        || cmdMatches("SAVE NEW_PROOF")) {
      if (cmdMatches("SHOW PROOF") || cmdMatches("SAVE PROOF")) {
        pipFlag = 0; /* Proof-in-progress (new_proof) flag */
      } else {
        pipFlag = 1;
      }
      if (cmdMatches("SHOW")) {
        saveFlag = 0;
      } else {
        saveFlag = 1;
      }

      if (!pipFlag) {
        for (i = 1; i <= statements; i++) {
          if (!strcmp(fullArg[2],statement[i].labelName)) break;
        }
        if (i > statements) {
          printLongLine(cat("?The statement with label \"",
              fullArg[2],
              "\" could not be found.  ",
              "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
          showStatement = 0; /* ???Why is this needed? */
          continue;
        }
        if (statement[i].type != (char)p__) {
          printLongLine(cat("?Statement \"", fullArg[2],
              "\" is not a $p statement", NULL), "", " ");
          continue;
        }
        showStatement = i;
      }

        startStep = 0;
        endStep = 0;
        startIndent = 0; /* Not used */
        endIndent = 0;
        essentialFlag = 0;
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
        i = switchPos("/ ESSENTIAL");
        if (i) essentialFlag = 1;
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
        i = switchPos("/ TEX");
        if (i) texFlag = 1;

        if (texFlag) {
          if (!texFileOpenFlag) {
            print2(
       "?You have not opened a LaTeX file.  Use the OPEN TEX command first.\n");
            continue;
          }
          print2("The LaTeX source was written to \"%s\".\n",
              texFileName);
        }

        i = switchPos("/ DETAILED_STEP"); /* non-pip mode only */
        if (i) {
          detailStep = val(fullArg[i + 1]);
          if (!detailStep) detailStep = -1; /* To use as flag; error message
                                               will occur in showDetailStep() */
        }

/*??? Need better warnings for switch combinations that don't make sense */

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
            if (!pipFlag) {
              let(&str1, compressProof(nmbrSaveProof, showStatement));
            } else {
              let(&str1, compressProof(nmbrSaveProof, proveStatement));
            }
          } else {
            let(&str1, nmbrCvtRToVString(nmbrSaveProof));
          }


          if (saveFlag) {
            let(&printString, "");
            outputToString = 1; /* Flag for print2 to add to printString */
          } else {
            print2(
"---------Clip out the proof below this line to put it in the source file:\n");
          }
          if (switchPos("/ COMPRESSED")) {
            printLongLine(cat("      ", str1, " $.", NULL),
              "      ", "& "); /* "&" is special flag to break compressed
                                  part of proof anywhere */
          } else {
            printLongLine(cat("      ", str1, " $.", NULL),
              "      ", " ");
          }
          if (pipFlag) { /* Add date proof was created */
            print2("      $([%s]$)\n", date());
          }
          if (!pipFlag) {
            i = showStatement;
          } else {
            i = proveStatement;
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
                  "  Remember to use WRITE SOURCE to save it permanently.",
                  NULL), "", " ");
            } else {
              printLongLine(cat("The new proof of \"", statement[i].labelName,
                  "\" has been saved internally.",
                  "  Remember to use WRITE SOURCE to save it permanently.",
                  NULL), "", " ");
            }
          } else {
            print2(cat(
"---------The proof of '",statement[i].labelName,
                "' to clip out ends above this line.\n",NULL));
          } /* End if saveFlag */
          nmbrLet(&nmbrSaveProof, NULL_NMBRSTRING);
          continue;
        }

        if (saveFlag) bug(1112); /* Shouldn't get here */

        if (!pipFlag) {
          parseProof(showStatement);
          if (wrkProof.errorSeverity > 1) continue; /* Display could crash */

          /*???CLEAN UP*/
          /*nmbrLet(&nmbrSaveProof, wrkProof.proofString);
          nmbrLet(&wrkProof.proofString, nmbrSquishProof(nmbrSaveProof));
          wrkProof.numSteps = nmbrLen(wrkProof.proofString);*/

          verifyProof(showStatement);
        }

        if (!pipFlag) {
          i = showStatement;
        } else {
          i = proveStatement;
        }
        if (texFlag) {
          outputToString = 1; /* Flag for print2 to add to printString */
          print2("\n");
          print2("\\vspace{1ex}\n");
          printLongLine(cat("Proof of ",
              "{\\tt ",
              asciiToTt(statement[i].labelName),
              "}:", NULL), "", " ");
          print2("\n");
          print2("\n");
          outputToString = 0;
          fprintf(texFilePtr, "%s", printString);
          let(&printString, "");
        }
  
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
            texFlag);

        if (!pipFlag) {
          cleanWrkProof(); /* Deallocate verifyProof storage */
        }

        /*???CLEAN UP */
        /*nmbrLet(&wrkProof.proofString, nmbrSaveProof);*/

        continue;
        
/*E*/ /* for debugging: */
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
        
        continue;
        
        
        /*??? Change the below for new proof format */
        /* The result of the expansion is pointed to by
           expandedProof[] */
        /* Add "statement =" to beginning of the proof */
        nmbrLet(&nmbrTmp,
            nmbrCat(nmbrAddElement(NULL_NMBRSTRING,showStatement),
            nmbrAddElement(NULL_NMBRSTRING,-(long)'='),NULL));
        pntrLet(&expandedProof,pntrCat(pntrSpace(1),
            expandedProof,NULL));
        let((vstring *)(&expandedProof[0]),nmbrCvtMToVString(
            statement[showStatement].mathString));
        nmbrLet(&nmbrTmp,nmbrCat(nmbrTmp,statement[showStatement].proofString,NULL));
        typeExpandedProof(nmbrTmp,expandedProof,NULL_NMBRSTRING,0);


        /* Deallocate expandedProof */
        k = pntrLen(expandedProof);
        for (i = 0; i < k; i++) {
          let((vstring *)(&expandedProof[i]),"");
        }
        pntrLet(&expandedProof,NULL_PNTRSTRING);

        continue;
    }

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
          printLongLine(cat("?The statement with label \"",
              fullArg[1],
              "\" could not be found.  ",
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
"Entering the Proof Assistant.  Type HELP for help, EXIT to exit.\n");
        print2("You will be working on the proof of statement %s:\n",
            statement[proveStatement].labelName);
        printLongLine(cat("  $p ", nmbrCvtMToVString(
            statement[proveStatement].mathString), NULL), "    ", " ");
        
        PFASmode = 1;

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
        for (j = 0; j < i; j++) {
          if (!nmbrLen(proofInProgress.source[j])) {
            initStep(j);
          }
        }

        /* Unify whatever can be unified */
        autoUnify(0); /* 0 means no "congrats" message */

        if (!nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
          print2(
          "Note:  The proof you are starting with is already complete.\n");
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

        continue;
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
        continue;
      }
    }
     

    if (cmdMatches("ASSIGN")) {
      s = val(fullArg[1]); /* Step number */

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
      autoUnify(1);        

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
        
    }
      

    if (cmdMatches("REPLACE")) {
      s = val(fullArg[1]); /* Step number */

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
      assignKnownSteps(s - q, nmbrLen(nmbrTmpPtr));
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
      autoUnify(1);        

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
        
    }
      

    if (cmdMatches("IMPROVE")) {

      k = 0; /* Depth */
      i = switchPos("/ DEPTH");
      if (i) k = val(fullArg[i + 1]);

      if (cmdMatches("IMPROVE STEP")) {
        s = val(fullArg[2]); /* Step number */

        m = nmbrLen(proofInProgress.proof); /* Original proof length */
        if (s > m || s < 1) {
          print2("?The step must be in the range from 1 to %ld.\n", m);
          continue;
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
        if (!proofChangedFlag) {
          print2("No new subproofs were found.\n");
        } else {
          proofChanged = 1; /* Cumulative flag */
        }
      
        continue;
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
      
        continue;
      } /* End if IMPROVE ALL */
          
    }
      

    if (cmdMatches("MINIMIZE_WITH")) {
      q = 0; /* Line length */
      s = 0; /* Status flag */
      i = switchPos("/ BRIEF"); /* Non-verbose mode */
      for (k = 1; k < proveStatement; k++) {

        if (statement[k].type != (char)p__ && statement[k].type != (char)a__)
          continue;
        if (!matches(statement[k].labelName, fullArg[1], '*')) continue;
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

        minimizeProof(k, proveStatement);
          
        n = nmbrLen(proofInProgress.proof); /* New proof length */
        if (m > n) {
          if (!i) {
            /* Verbose mode */
            print2("\n");
          }
          print2(
            "Proof length was decreased from %ld to %ld using '%s'.\n", m, n,
            statement[k].labelName);
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
      if (s == 1) print2("No shorter proof was found.\n");
      if (!s) print2("?No earlier labels match '%s'.\n", fullArg[1]);
      
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
        proofChanged = 1; /* Cumulative flag */
      } else {
        print2("?There are no floating-hypothesis steps to delete.\n");
      }
      
      if (n) {
        print2("Steps %ld and above have been renumbered.\n", n);
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
        /* Change wildcard to ASCII 2 (to be different from printable chars) */
        while (1) {
          p = instr(1, str1, "$*");
          if (!p) break;
          /* Take off one space from $* so it will match an empty symbol seq */
          q = p;
          s = p;
          if (p > 1) {
            /* We're not at beginning of the symbol string */
            if (str1[p - 2] == ' ') q--; /* Take off leading space on $* */
          } else {
            /* We're at beginning of the symbol string */
            if (p < strlen(str1) - 1) {
              /* ...and we're not at the end */
              if (str1[p + 1] == ' ') s++; /* Take off trailing space on $* */
            }
          }
          let(&str1, cat(left(str1, q - 1), chr(2), right(str1, s + 2), NULL));
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
          let(&str2, cat(" ", nmbrCvtMToVString(statement[i].mathString), " ",
              NULL));
          if (!matches(str2, str1, 2)) continue;
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
"Substitution with empty symbol sequences is no longer allowed.  Note that\n");
        print2(
"this may make the Proof Assistant too restrictive in some cases.\n");
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
      print2("Screen width has been changed from %ld to %ld\n",
          screenWidth,s);
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
      

    if (cmdMatches("HELP")) {
      /* Build the complete command */
      k = pntrLen(fullArg);
      let(&str1,"");
      for (i = 0; i < k; i++) {
        let(&str1, cat(str1, fullArg[i], " ", NULL));
      }
      help1(left(str1, strlen(str1) - 1));
      help2(left(str1, strlen(str1) - 1));
      continue;
    }
      

    if (cmdMatches("EXIT") || cmdMatches("QUIT")) {
     /*???        || !strcmp(cmd,"^Z")) { */

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
          print2("The LaTeX file \"%s\" has been closed.\n", texFileName);
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
        return; /* Exit */
      }
    }

    if (cmdMatches("SUBMIT")) {
      let(&commandFileName, fullArg[1]);
      commandFilePtr = fSafeOpen(commandFileName, "r");
      if (!commandFilePtr) continue; /* Couldn't open (err msg was provided) */
      print2("Taking command lines from file \"%s\"...\n",commandFileName);
      commandFileOpenFlag = 1;
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

    if (cmdMatches("OPEN TEX")) {
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
      print2("The LaTeX file \"%s\" has been opened.\n", texFileName);
      printTexHeader(texHeaderFlag);
      continue;
    }

    if (cmdMatches("CLOSE TEX")) {
      /* Close the TeX file */
      if (!texFileOpenFlag) {
        print2("?Sorry, there is no LaTeX file currently open.\n");
      } else {
        print2("The LaTeX file \"%s\" has been closed.\n", texFileName);
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

    if (cmdMatches("VERIFY PROOFS")) {
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
}
