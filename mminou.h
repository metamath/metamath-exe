/*****************************************************************************/
/*               Copyright (C) 1997, NORMAN D. MEGILL                        */
/*****************************************************************************/

/*34567890123456 (79-character line to adjust text window width) 678901234567*/
extern int errorCount;     /* Total error count */

/* Global variables used by print2() */
extern flag logFileOpenFlag;
extern FILE *logFilePtr;
/* Global variables used by print2() */
extern flag outputToString;
extern vstring printString;
/* Global variables used by cmdInput() */
extern flag commandFileOpenFlag;
extern FILE *commandFilePtr;
extern vstring commandFileName;

extern FILE *inputDef_fp,*input_fp,*output_fp;  /* File pointers */
extern vstring inputDef_fn,input_fn,output_fn;  /* File names */

/* print2 returns 0 if the user has quit the printout. */
flag print2(char* fmt,...);
extern long screenWidth; /* Width of screen */
#define MAX_LEN 79 /* Default width of screen */
#define SCREEN_HEIGHT 23 /* Lines on screen */
extern flag scrollMode; /* Flag for continuous or prompted scroll */
extern flag quitPrint; /* Flag that user typed 'q' to last scrolling prompt */

/* printLongLine automatically puts a newline \n in the output line. */
void printLongLine(vstring line, vstring startNextLine, vstring breakMatch);
vstring cmdInput(FILE *stream,vstring ask);
vstring cmdInput1(vstring ask);

enum severity {_notice,_warning,_error,_fatal};
void errorMessage(vstring line, long lineNum, short column, short tokenLength,
  vstring error, vstring fileName, long statementNum, flag warnFlag);

/* Opens files with error message; opens output files with
   backup of previous version.   Mode must be "r" or "w". */
FILE *fSafeOpen(vstring fileName, vstring mode);

