/*****************************************************************************/
/*        Copyright (C) 2002  NORMAN MEGILL  nm@alum.mit.edu                 */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

extern int errorCount;     /* Total error count */

/* Global variables used by print2() */
extern flag logFileOpenFlag;
extern FILE *logFilePtr;
extern FILE *listFile_fp;
/* Global variables used by print2() */
extern flag outputToString;
extern vstring printString;
/* Global variables used by cmdInput() */
extern flag commandFileOpenFlag;
extern FILE *commandFilePtr;
extern vstring commandFileName;

extern FILE *inputDef_fp,*input_fp,*output_fp;  /* File pointers */
extern vstring inputDef_fn,input_fn,output_fn;  /* File names */

/* PRINTBUFFERSIZE should be at least as long as the longest string we
   expect (an unfortunate, dangerous limitation of C?) - although if >79
   chars are output on a line bug #1505 warning will occur */
#define PRINTBUFFERSIZE 1001
/* Warning:  never call print2 with string longer than PRINTBUFFERSIZE - 1 */
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

/* Renames a file with backup of previous version.  If non-zero
   is returned, there was an error. */
int fSafeRename(vstring oldFileName, vstring newFileName);

/* Finds the name of the first file of the form filePrefix +
   nnn + ".tmp" that does not exist.  THE CALLER MUST DEALLOCATE
   THE RETURNED STRING. */
vstring fGetTmpName(vstring filePrefix);

/* This function returns a character string containing the entire contents of
   an ASCII file, or Unicode file with only ASCII characters.   On some
   systems it is faster than reading the file line by line.  THE CALLER
   MUST DEALLOCATE THE RETURNED STRING.  If a NULL is returned, the file
   could not be opened or had a non-ASCII Unicode character or some other
   problem.   If verbose is 0, error and warning messages are suppressed. */
vstring readFileToString(vstring fileName, char verbose);
