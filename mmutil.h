/*****************************************************************************/
/*               Copyright (C) 1997, NORMAN D. MEGILL                        */
/*****************************************************************************/

/*34567890123456 (79-character line to adjust text window width) 678901234567*/

#ifndef _VMC_   /* Start of VMC.H header */
#define _VMC_
/******************************************************************************
 *  VMC.H - Header file of VMC Library
 *
 *  This is the header file of a collection of useful variable and function
 *  definitions
 *
 ******************************************************************************/

/******************************************************************************
 *
 *  Generic C
 *
 ******************************************************************************/

#include <stdio.h>                                      /* For FILE I/O */

/* String declarations */
/* #define MAX_CHAR_PER_STANDARD_STRING 128 */
/* typedef char string[MAX_CHAR_PER_STANDARD_STRING]; */
typedef char string80[80];
typedef char string128[128];
typedef char string256[256];
typedef char string512[512];
typedef char string1024[1024];
typedef char string2048[2048];
typedef char string4096[4096];

/* Allocate memory and copy string (NOTE: static memory will be allocated !!) */
char *vmc_strCreate(long i);
char *vmc_strSave(char *c);

/* Get a string from the user (NOTE: static memory will be allocated !!) */
char *vmc_getLine(void);

/* Prompt user to get a variable */
int vmc_askInt(char *prompt,int default_answer,int use_default);
long vmc_askLong(char *prompt,long default_answer,int use_default);
/* Don't use this next routine: function prototype problem on VAX */
#ifndef VAXC
#ifndef VMS
float vmc_askFloat(char *prompt,float default_answer,int use_default);
#endif
#endif
double vmc_askDouble(char *prompt,double default_answer,int use_default);
char *vmc_askString(char *prompt,char *default_answer,int use_default);
int vmc_askYesNo(char *prompt,char *default_answer,int use_default);

/* Open file */
FILE *vmc_fopen(char **fileName,char *prompt,char *defaultAnswer,char *mode,
        int useDefault);

/* Lexical analyzer */
/* Initialize the control string for lexical analysis */
void vmc_lexSetup(void);
/* Return the next lexical symbol in s.  (Note: Storage for s should be
   allocated before this function is called!) */
int vmc_lexGetSymbol(char *s,FILE *fp);
/* This string structure customize the token function for lexical analysis.
   The meaning of the character strings are:
   single               Single character tokens.  If one of these characters
                        are found, this means the last token should be
                        terminated and returned.  If the last token has
                        already been returned, then return this single
                        character token. e.g. "();=+*-/<>^"
                        (Note: If +- are included, then "-1.0" will be
                        returned as two separate tokens "-" and "1.0" !!)
   space                Token delimiters.  All tokens, except single character
                        tokens, must be separated by one of these characters.
                        e.g. " ,\t" (blank,comma,tab)
   commentStart
   commentEnd           Start and end of a comment.  Each character in
                        commentStart denotes the start of a comment,
                        and the comment is terminated by the corresponding
                        character position in commentEnd.  Comments
                        are not returned as tokens. e.g.
                                Start of comment        End of comment
                                        !                       \n
                                        \                       \
                        (Note: Each character in commentStart must be
                        unique !!)
   enclosureStart
   enclosureEnd         Start and end of a special enclosure token.
                        Each character in enclosureStart denotes the
                        start of a comment, and the comment is terminated by the
                        corresponding character position in
                        enclosureEnd. All characters within the
                        enclosure are returned as one single token. e.g.
                                Start of enclosure      End of enclosure
                                        "                       "
                                        '                       '
                                        \                       \
                        (Note: Each character in enclosureStart must be
                        unique !!)
  */
struct vmc_lexControlStructure {/* Example:                     */
  char *single;                 /* "();+/-*=<>^"                */
  char *space;                  /* " ,\t\n"                     */
  char *commentStart;           /* "!\\"                        */
  char *commentEnd;             /* "\n\\"                       */
  char *enclosureStart;         /* "\"\'"                       */
  char *enclosureEnd;           /* "\"\'"                       */
};
extern struct vmc_lexControlStructure vmc_lexControl;
/* Line count by vmc_lexGetSymbol() */
extern long vmc_lineCount;
/* Clear the accumulative token buffer (for warning/error messages) */
void vmc_clearTokenBuffer(void);
/* Skip until '\n' in file fp, print and clear the accumulative token buffer */
void vmc_skipTokenBuffer(FILE *fp);
/* Skip until "terminator" in file fp, print and clear the accumulative token
   buffer */
void vmc_skipUntil(FILE *fp,char *terminator);

/* Pause for debugging */
void vmc_debugPause(char *s);

/* Current directory */
char *vmc_currentDirectory(void);

/* Current date/time */
char *vmc_date(void);
char *vmc_time(void);

/* Convert from string to double */
#ifdef VAX
double vmc_atod(char *s);
float vmc_atof(char *s);
#else
#define vmc_atof(c) atof(c)
#define vmc_atod(c) atof(c)
#endif

/* Print handler
   All print messages generated by the vmc library procedures are handled by
   (*vmc_print)(), which is defined as vmc_standardPrint() initially.
   The user program can modify this by changing vmc_print. */
extern void vmc_genericPrint(int severity,char *fmt,...);
extern void vmc_standardPrint(int severity,char *fmt,...);
extern void (*vmc_print)(int severity,char *fmt,...);

/* Check to see if s1 is the prefix of s2.
   Return TRUE if it is; FALSE otherwise */
int vmc_isPrefix(char *s1,char *s2);

#ifdef VAXC     /* Start of VAX C specific code */
/******************************************************************************
 *
 *  VAX C
 *
 ******************************************************************************/

#include <descrip.h>                            /* For BASIC string handing */

/* Get ccl (command line) */
char *vmc_getCCL(void);

/* Memory allocation functions */
void *vmc_malloc(int size);
int vmc_free(void *ptr);
void *vmc_calloc(int nmemb,int size);
int vmc_cfree(void *ptr);

/* Get virtual memory allocated:
   Returns number of bytes of virtual memory allocated by
   LIB$GET_VM/LIB$FREE_VM and LIB$GET_VM_PAGE/LIB$FREE_VM_PAGE system
   library service */
int *vmc_getVM(void);
/* Print debug message, vm usage and difference since last interrogation */
void vmc_debugVM(char *s);

/* Get virtual memory allocated:
   Returns number of virtual pages allocated in P0 space and P1 space */
int *vmc_getVirtualPage(void);
/* Print debug message, vm usage and difference since last interrogation */
void vmc_debugVirtualPage(char *s);

/* Return a piece of job/process information */
unsigned int *vmc_getJPI(unsigned int code);

/* BASIC string handling routines */
typedef struct dsc$descriptor_s *bstring;       /* BASIC string */
char *vmc_b2cString(bstring s);  /* Convert a BASIC string into a C string */
bstring vmc_c2bString(char *s);  /* Convert a C string into a BASIC string */

/* Check the return status of a system call */
void vmc_checkSystemCall(char *s,int status);
void vmc_checkLibraryCall(int status);

/* Get a single keystoke from input stream SYS$INPUT.
   If another stream is desired, call vmc_openGetKeyDevice() first before
   calling vmc_getKey(). */
void vmc_openGetKeyDevice(char *device_name);
void vmc_closeGetKeyDevice(void);
int vmc_getKey(void);
/* vmc_getkey() returns an int, which is either an ASCII character
  or a code for an escape sequence as stated in the definitions below */
#define VMC_KEY_DELETE  127
#define VMC_KEY_PF1     256
#define VMC_KEY_PF2     257
#define VMC_KEY_PF3     258
#define VMC_KEY_PF4     259
#define VMC_KEY_KP0     260
#define VMC_KEY_KP1     261
#define VMC_KEY_KP2     262
#define VMC_KEY_KP3     263
#define VMC_KEY_KP4     264
#define VMC_KEY_KP5     265
#define VMC_KEY_KP6     266
#define VMC_KEY_KP7     267
#define VMC_KEY_KP8     268
#define VMC_KEY_KP9     269
#define VMC_KEY_ENTER   270
#define VMC_KEY_MINUS   271
#define VMC_KEY_COMMA   272
#define VMC_KEY_PERIOD  273
#define VMC_KEY_UP      274
#define VMC_KEY_DOWN    275
#define VMC_KEY_LEFT    276
#define VMC_KEY_RIGHT   277
#define VMC_KEY_F6      286
#define VMC_KEY_F7      287
#define VMC_KEY_F8      288
#define VMC_KEY_F9      289
#define VMC_KEY_F10     290
#define VMC_KEY_F11     291
#define VMC_KEY_F12     292
#define VMC_KEY_F13     293
#define VMC_KEY_F14     294
#define VMC_KEY_F15     295
#define VMC_KEY_F16     296
#define VMC_KEY_HELP    295                     /* Same as F15 */
#define VMC_KEY_DO      296                     /* Same as F16 */
#define VMC_KEY_F17     297
#define VMC_KEY_F18     298
#define VMC_KEY_F19     299
#define VMC_KEY_F20     300
#define VMC_KEY_FIND    311
#define VMC_KEY_INSERT_HERE     312
#define VMC_KEY_REMOVE          313
#define VMC_KEY_SELECT          314
#define VMC_KEY_PREV_SCREEN     315
#define VMC_KEY_NEXT_SCREEN     316
#define VMC_KEY_UNKNOWN         511

/* Spawn out a process to execute a DCL command */
void vmc_spawn(char *command);

/* Convert a stream_LF file into a variable length file */
void vmc_convertFile(char *infile,char *outfile);

/* CPU time usage */
void vmc_initCPUTime(void);
int vmc_CPUTime(void);

/* Translate logical name (NOTE: static memory will be allocated !!) */
char *vmc_translateLogical(char *logicalName);

/* Define logical name */
void vmc_defineLogical(char *table,char *logical,char *equivalent);

#endif          /* End of VAX C specific code */

/******************************************************************************
 *
 *  Miscellaneous
 *
 ******************************************************************************/

#ifdef VMC_OLD_NAMES
/* Old function names (for compatibility with older version of vmc) */
#define vmc_ask_int(p1,p2,p3)           vmc_askInt(p1,p2,p3)
#define vmc_ask_long(p1,p2,p3)          vmc_askLong(p1,p2,p3)
#define vmc_ask_float(p1,p2,p3)         vmc_askFloat(p1,p2,p3)
#define vmc_ask_double(p1,p2,p3)        vmc_askDouble(p1,p2,p3)
#define vmc_ask_string(p1,p2,p3)        vmc_askString(p1,p2,p3)
#define vmc_ask_yesno(p1,p2,p3)         vmc_askYesNo(p1,p2,p3)
#define vmc_strsave(c)                  vmc_strSave(c)
#define vmc_strcreate(i)                vmc_strCreate(i)
#define vmc_getline()                   vmc_getLine();
#define vmc_debug_pause(s)              vmc_debugPause(s)
#define initCPUTime()                   vmc_initCPUTime()
#define CPUTime()                       vmc_CPUTime()
#define current_directory()             vmc_currentDirectory()
#define vmc_getccl()                    vmc_getCCL()
#define vmc_parsechoice(i,c)            vmc_parseChoice(i,c)
#define vmc_toupper(s)                  vmc_toUpper(s)
#define vmc_printchoice(i,c)            vmc_printChoice(i,c)
#define vmc_askchoice(i,c,s)            vmc_askChoice(i,c,s)
#define vmc_getvm()                     vmc_getVM()
#define vmc_debugvm(s)                  vmc_debugVM(char *s)
#define CHOICE                          VMC_CHOICE
#define vmc_b2cstring(s)                vmc_b2cString(s)
#define vmc_c2bstring(s)                vmc_c2bString(s)
#endif          /* End of VMC_OLD_NAMES definitions */

#endif          /* End of VMC.H header */
