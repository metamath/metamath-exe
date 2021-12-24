/*****************************************************************************/
/*        Copyright (C) 2019  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*
mmvstr.c - VMS-BASIC variable length string library routines header
This is an emulation of the string functions available in VMS BASIC.
*/

/*** See the comments in mmvstr.h for an explanation of these functions ******/

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include "mmvstr.h"
/*E*/ /*Next line is need to declare "db" for debugging*/
#include "mmdata.h"
/* 1-Dec-05 nm
   mmdata.h is also used to declare the bug() function that is called in
   several places by mmvstr.c.  To make mmvstr.c and mmvstr.h completely
   independent of the other programs, for use with another project, do the
   following:
     (1) Remove all lines beginning with the "/ *E* /" comment.
     (2) Remove all calls to the bug() function (4 places).
   To see an example of stand-alone usage of the mmvstr.c functions, see
   the program lattice.c and several others included in
     http://us.metamath.org/downloads/quantum-logic.tar.gz
*/

/*E*/long db1=0;
#ifdef NDEBUG
# define INCDB1(x)
#else
# define INCDB1(x) db1 += (x)
#endif

#define MAX_ALLOC_STACK 100
long g_tempAllocStackTop = 0;      /* Top of stack for tempAlloc functon */
long g_startTempAllocStack = 0;    /* Where to start freeing temporary allocation
                                    when let() is called (normally 0, except in
                                    special nested vstring functions) */
void *tempAllocStack[MAX_ALLOC_STACK];

static void freeTempAlloc(void)
{
  /* All memory previously allocated with tempAlloc is deallocated. */
  /* EXCEPT:  When g_startTempAllocStack != 0, the freeing will start at
     g_startTempAllocStack. */
  long i;
  for (i = g_startTempAllocStack; i < g_tempAllocStackTop; i++) {
/*E*/INCDB1(-1 - (long)strlen(tempAllocStack[i]));
/*E* /printf("%ld removing [%s]\n", db1, tempAllocStack[i]);*/
    free(tempAllocStack[i]);
  }
  g_tempAllocStackTop = g_startTempAllocStack;
} /* freeTempAlloc */


static void pushTempAlloc(void *mem)
{
  if (g_tempAllocStackTop >= (MAX_ALLOC_STACK-1)) {
    printf("*** FATAL ERROR ***  Temporary string stack overflow\n");
#if __STDC__
    fflush(stdout);
#endif
    bug(2201);
  }
  tempAllocStack[g_tempAllocStackTop++] = mem;
} /* pushTempAlloc */


static void* tempAlloc(long size)  /* String memory allocation/deallocation */
{
  void* memptr = malloc((size_t)size);
  if (!memptr || size == 0) {
    printf("*** FATAL ERROR ***  Temporary string allocation failed\n");
#if __STDC__
    fflush(stdout);
#endif
    bug(2202);
  }
  pushTempAlloc(memptr);
/*E*/INCDB1(size);
/*E* /printf("%ld adding\n",db1);*/
  return memptr;
} /* tempAlloc */


/* Make string have temporary allocation to be released by next let() */
/* Warning:  after makeTempAlloc() is called, the vstring may NOT be
   assigned again with let() */
void makeTempAlloc(vstring s)
{
  pushTempAlloc(s);
/*E*/INCDB1((long)strlen(s) + 1);
/*E*/db-=(long)strlen(s) + 1;
/*E* /printf("%ld temping[%s]\n", db1, s);*/
} /* makeTempAlloc */


/* 8-Jul-2013 Wolf Lammen - rewritten to simplify it */
void let(vstring *target, vstring source)        /* String assignment */
/* This function must ALWAYS be called to make assignment to */
/* a vstring in order for the memory cleanup routines, etc. */
/* to work properly.  If a vstring has never been assigned before, */
/* it is the user's responsibility to initialize it to "" (the */
/* null string). */
{

  size_t sourceLength = strlen(source);  /* Save its length */
  size_t targetLength = strlen(*target); /* Save its length */
/*E*/if (targetLength) {
/*E*/  db -= (long)targetLength+1;
/*E*/  /* printf("%ld Deleting %s\n",db,*target); */
/*E*/}
/*E*/if (sourceLength) {
/*E*/  db += (long)sourceLength+1;
/*E*/  /* printf("%ld Adding %s\n",db,source); */
/*E*/}
  if (targetLength < sourceLength) { /* Old string has not enough room for new one */
    /* Free old string space and allocate new space */
    if (targetLength)
      free(*target);  /* Free old space */
    *target = malloc(sourceLength + 1); /* Allocate new space */
    if (!*target) {
      printf("*** FATAL ERROR ***  String memory couldn't be allocated\n");
#if __STDC__
      fflush(stdout);
#endif
      bug(2204);
    }
  }
  if (sourceLength) {
    strcpy(*target, source);
  } else {
    /* Empty strings could still be temporaries, so always assign a constant */
    if (targetLength) {
      free(*target);
    }
    *target= "";
  }

  freeTempAlloc(); /* Free up temporary strings used in expression computation */

} /* let */

vstring cat(vstring string1,...)        /* String concatenation */
#define MAX_CAT_ARGS 50
{
  va_list ap;   /* Declare list incrementer */
  vstring arg[MAX_CAT_ARGS];    /* Array to store arguments */
  size_t argPos[MAX_CAT_ARGS]; /* Array of argument positions in result */
  vstring result;
  int i;
  int numArgs = 0;        /* Define "last argument" */

  size_t pos = 0;
  char* curArg = string1;

  va_start(ap, string1); /* Begin the session */
  do {
        /* User-provided argument list must terminate with 0 */
    if (numArgs >= MAX_CAT_ARGS) {
      printf("*** FATAL ERROR ***  Too many cat() arguments\n");
#if __STDC__
      fflush(stdout);
#endif
      bug(2206);
    }
    arg[numArgs] = curArg;
    argPos[numArgs] = pos;
    pos += strlen(curArg);
  } while (++numArgs, (curArg = va_arg(ap,char *)) != 0);
  va_end(ap);           /* End var args session */

  /* Allocate the memory for it */
  result = tempAlloc((long)pos+1);
  /* Move the strings into the newly allocated area */
  for (i = 0; i < numArgs; ++i)
    strcpy(result + argPos[i], arg[i]);
  return result;
} /* cat */


/* 20-Oct-2013 Wolf Lammen - allow unlimited input line lengths */
/* Input a line from the user or from a file */
/* Returns 1 if a (possibly empty) line was successfully read, 0 if EOF */
int linput(FILE *stream, const char* ask, vstring *target)
{                           /* Note: "vstring *target" means "char **target" */
  /*
    BASIC:  linput "what"; a$
    c:      linput(NULL, "what?", &a);

    BASIC:  linput #1, a$                         (error trap on EOF)
    c:      if (!linput(file1, NULL, &a)) break;  (break on EOF)

  */
  /* This function prints a prompt (if 'ask' is not NULL), gets a line from
    the stream, and assigns it to target using the let(&...) function.
    0 is returned when end-of-file is encountered.  The vstring
    *target MUST be initialized to "" or previously assigned by let(&...)
    before using it in linput. */
  char f[10001]; /* Read in chunks up to 10000 characters */
  int result = 0;
  int eol_found = 0;
  if (ask) {
    printf("%s", ask);
#if __STDC__
    fflush(stdout);
#endif
  }
  if (stream == NULL) stream = stdin;
  while (!eol_found && fgets(f, sizeof(f), stream))
  {
    size_t endpos = strlen(f) - 1;
    eol_found = (f[endpos] == '\n');
    /* If the last line in the file has no newline, eol_found will be 0 here.
       The fgets() above will return 0 and prevent another loop iteration. */
    if (eol_found)
      f[endpos] = 0; /* The return string will have any newline stripped. */
    if (result)
      /* Append additional parts of the line to *target */
      /* The let() reallocates *target and copies the concatenation of the
         old *target and the additional input f[] to it */
      let(target /* = &(*target) */, cat(*target, f, NULL));
    else
      /* This is the first time through the loop, and normally
         the only one unless the input line overflows f[] */
      let(target, f);  /* Allocate *target and copy f to it */
    result = 1;
  }
  return result;
} /* linput */


/* Find out the length of a string */
long len(vstring s)
{
  return ((long)strlen(s));
} /* len */


/* Extract sin from character position start to stop into sout */
vstring seg(vstring sin, long start, long stop)
{
  if (start < 1) start = 1;
  return mid(sin, start, stop - start + 1);
} /* seg */


/* Extract sin from character position start for length len */
vstring mid(vstring sin, long start, long length)
{
  vstring sout;
  if (start < 1) start = 1;
  if (length < 0) length = 0;
  sout=tempAlloc(length + 1);
  strncpy(sout,sin + start - 1, (size_t)length);
/*E*/ /*??? Should db be subtracted from if length > end of string? */
  sout[length] = 0;
  return (sout);
} /* mid */


/* Extract leftmost n characters */
vstring left(vstring sin,long n)
{
  return mid(sin, 1, n);
} /* left */


/* Extract after character n */
vstring right(vstring sin, long n)
{
  return seg(sin, n, (long)(strlen(sin)));
} /* right */


/* Emulate VMS BASIC edit$ command */
vstring edit(vstring sin,long control)
#define isblank_(c) ((c == ' ') || (c == '\t'))
    /* 11-Sep-2009 nm Added _ to fix '"isblank" redefined' compiler warning */
#define isblankorlf_(c) ((c == ' ') || (c == '\t') || (c == '\n'))
    /* 8-May-2015 nm added isblankorlf_ */
{
  /* EDIT$ (from VMS BASIC manual)
       Syntax:  str-vbl = EDIT$(str-exp, int-exp)
       Values   Effect
       1        Trim parity bits
       2        Discard all spaces and tabs
       4        Discard characters: CR, LF, FF, ESC, RUBOUT, and NULL
       8        Discard leading spaces and tabs
       16       Reduce spaces and tabs to one space
       32       Convert lowercase to uppercase
       64       Convert [ to ( and ] to )
       128      Discard trailing spaces and tabs
       256      Do not alter characters inside quotes

       (non-BASIC extensions)
       512      Convert uppercase to lowercase
       1024     Tab the line (convert spaces to equivalent tabs)
       2048     Untab the line (convert tabs to equivalent spaces)
       4096     Convert VT220 screen print frame graphics to -,|,+ characters

       (Added 10/24/03:)
       8192     Discard CR only (to assist DOS-to-Unix conversion)

       (Added 8-May-2015 nm:)
       16384    Discard trailing spaces, tabs, and LFs
  */
  vstring sout;
  long i, j, k, m;
  int last_char_is_blank;
  int trim_flag, discardctrl_flag, bracket_flag, quote_flag, case_flag;
  int alldiscard_flag, leaddiscard_flag, traildiscard_flag,
      traildiscardLF_flag, reduce_flag;
  int processing_inside_quote=0;
  int lowercase_flag, tab_flag, untab_flag, screen_flag, discardcr_flag;
  unsigned char graphicsChar;

  /* Set up the flags */
  trim_flag = control & 1;
  alldiscard_flag = control & 2;
  discardctrl_flag = control & 4;
  leaddiscard_flag = control & 8;
  reduce_flag = control & 16;
  case_flag = control & 32;
  bracket_flag = control & 64;
  traildiscard_flag = control & 128;
  traildiscardLF_flag = control & 16384;
  quote_flag = control & 256;

  /* Non-BASIC extensions */
  lowercase_flag = control & 512;
  tab_flag = control & 1024;
  untab_flag = control & 2048;
  screen_flag = control & 4096; /* Convert VT220 screen prints to |,-,+
                                   format */
  discardcr_flag = control & 8192; /* Discard CR's */

  /* Copy string */
  i = (long)strlen(sin) + 1;
  if (untab_flag) i = i * 7; /* Allow for max possible length */
  sout=tempAlloc(i);
  strcpy(sout,sin);

  /* Discard leading space/tab */
  i=0;
  if (leaddiscard_flag)
    while ((sout[i] != 0) && isblank_(sout[i]))
      sout[i++] = 0;

  /* Main processing loop */
  while (sout[i] != 0) {

    /* Alter characters inside quotes ? */
    if (quote_flag && ((sout[i] == '"') || (sout[i] == '\'')))
       processing_inside_quote = ~ processing_inside_quote;
    if (processing_inside_quote) {
       /* Skip the rest of the code and continue to process next character */
       i++; continue;
    }

    /* Discard all space/tab */
    if ((alldiscard_flag) && isblank_(sout[i]))
        sout[i] = 0;

    /* Trim parity (eighth?) bit */
    if (trim_flag)
       sout[i] = sout[i] & 0x7F;

    /* Discard CR,LF,FF,ESC,BS */
    if ((discardctrl_flag) && (
         (sout[i] == '\015') || /* CR  */
         (sout[i] == '\012') || /* LF  */
         (sout[i] == '\014') || /* FF  */
         (sout[i] == '\033') || /* ESC */
         /*(sout[i] == '\032') ||*/ /* ^Z */ /* DIFFERENCE won't work w/ this */
         (sout[i] == '\010')))  /* BS  */
      sout[i] = 0;

    /* Discard CR */
    if ((discardcr_flag) && (
         (sout[i] == '\015')))  /* CR  */
      sout[i] = 0;

    /* Convert lowercase to uppercase */
    /*
    if ((case_flag) && (islower(sout[i])))
       sout[i] = toupper(sout[i]);
    */
    /* 13-Jun-2009 nm The upper/lower case C functions have odd behavior
       with characters > 127, at least in lcc.  So this was rewritten to
       not use them. */
    if ((case_flag) && (sout[i] >= 'a' && sout[i] <= 'z'))
       sout[i] = (char)(sout[i] - ('a' - 'A'));

    /* Convert [] to () */
    if ((bracket_flag) && (sout[i] == '['))
       sout[i] = '(';
    if ((bracket_flag) && (sout[i] == ']'))
       sout[i] = ')';

    /* Convert uppercase to lowercase */
    /*
    if ((lowercase_flag) && (isupper(sout[i])))
       sout[i] = tolower(sout[i]);
    */
    /* 13-Jun-2009 nm The upper/lower case C functions have odd behavior
       with characters > 127, at least in lcc.  So this was rewritten to
       not use them. */
    if ((lowercase_flag) && (sout[i] >= 'A' && sout[i] <= 'Z'))
       sout[i] = (char)(sout[i] + ('a' - 'A'));

    /* Convert VT220 screen print frame graphics to +,|,- */
    if (screen_flag) {
      graphicsChar = (unsigned char)sout[i]; /* Need unsigned char for >127 */
      /* vt220 */
      if (graphicsChar >= 234 && graphicsChar <= 237) sout[i] = '+';
      if (graphicsChar == 241) sout[i] = '-';
      if (graphicsChar == 248) sout[i] = '|';
      if (graphicsChar == 166) sout[i] = '|';
      /* vt100 */
      if (graphicsChar == 218 /*up left*/ || graphicsChar == 217 /*lo r*/
          || graphicsChar == 191 /*up r*/ || graphicsChar == 192 /*lo l*/)
        sout[i] = '+';
      if (graphicsChar == 196) sout[i] = '-';
      if (graphicsChar == 179) sout[i] = '|';
    }

    /* Process next character */
    i++;
  }
  /* sout[i]=0 is the last character at this point */

  /* Clean up the deleted characters */
  for (j = 0, k = 0; j <= i; j++)
    if (sout[j]!=0) sout[k++]=sout[j];
  sout[k] = 0;
  /* sout[k] = 0 is the last character at this point */

  /* Discard trailing space/tab */
  if (traildiscard_flag) {
    --k;
    while ((k >= 0) && isblank_(sout[k])) --k;
    sout[++k] = 0;
  }

  /* 8-May-2015 nm */
  /* Discard trailing space/tab and LF */
  if (traildiscardLF_flag) {
    --k;
    while ((k >= 0) && isblankorlf_(sout[k])) --k;
    sout[++k] = 0;
  }

  /* Reduce multiple space/tab to a single space */
  if (reduce_flag) {
    i = j = last_char_is_blank = 0;
    while (i <= k - 1) {
      if (!isblank_(sout[i])) {
        sout[j++] = sout[i++];
        last_char_is_blank = 0;
      } else {
        if (!last_char_is_blank)
          sout[j++]=' '; /* Insert a space at the first occurrence of a blank */
        last_char_is_blank = 1; /* Register that a blank is found */
        i++; /* Process next character */
      }
    }
    sout[j] = 0;
  }

  /* Untab the line */
  if (untab_flag || tab_flag) {

    /*
    DEF FNUNTAB$(L$)      ! UNTAB LINE L$
    I9%=1%
    I9%=INSTR(I9%,L$,CHR$(9%))
    WHILE I9%
      L$=LEFT(L$,I9%-1%)+SPACE$(8%-((I9%-1%) AND 7%))+RIGHT(L$,I9%+1%)
      I9%=INSTR(I9%,L$,CHR$(9%))
    NEXT
    FNUNTAB$=L$
    FNEND
    */

    /***** old code (doesn't handle multiple lines)
    k = (long)strlen(sout);
    for (i = 1; i <= k; i++) {
      if (sout[i - 1] != '\t') continue;
      for (j = k; j >= i; j--) {
        sout[j + 8 - ((i - 1) & 7) - 1] = sout[j];
      }
      for (j = i; j < i + 8 - ((i - 1) & 7); j++) {
        sout[j - 1] = ' ';
      }
      k = k + 8 - ((i - 1) & 7);
    }
    *****/

    /* Untab string containing multiple lines */ /* 9-Jul-2011 nm */
    /* (Currently this is needed by outputStatement() in mmpars.c) */
    k = (long)strlen(sout);
    m = 0;  /* Position on line relative to last '\n' */
    for (i = 1; i <= k; i++) {
      if (sout[i - 1] == '\n') {
        m = 0;
        continue;
      }
      m++; /* Should equal i for one-line string */
      if (sout[i - 1] != '\t') continue;
      for (j = k; j >= i; j--) {
        sout[j + 8 - ((m - 1) & 7) - 1] = sout[j];
      }
      for (j = i; j < i + 8 - ((m - 1) & 7); j++) {
        sout[j - 1] = ' ';
      }
      k = k + 8 - ((m - 1) & 7);
    }
  }

  /* Tab the line */
  /* (Note that this does not [yet?] handle string with multiple lines) */
  if (tab_flag) {

    /*
    DEF FNTAB$(L$)        ! TAB LINE L$
    I9%=0%
    FOR I9%=8% STEP 8% WHILE I9%<LEN(L$)
      J9%=I9%
      J9%=J9%-1% UNTIL ASCII(MID(L$,J9%,1%))<>32% OR J9%=I9%-8%
      IF J9%<=I9%-2% THEN
        L$=LEFT(L$,J9%)+CHR$(9%)+RIGHT(L$,I9%+1%)
        I9%=J9%+1%
      END IF
    NEXT I9%
    FNTAB$=L$
    FNEND
    */

    k = (long)strlen(sout);
    for (i = 8; i < k; i = i + 8) {
      j = i;

      /* 25-May-2016 nm */
      /* gcc m*.c -o metamath.exe -O2 -Wall was giving:
             mmvstr.c:285:9: warning: assuming signed overflow does not occur
             when assuming that (X - c) <= X is always true [-Wstrict-overflow]
         Here we trick gcc into turning off this optimization by moving
         the computation of i - 2 here, then referencing m instead of i - 2
         below.  Note that if "m = i - 2" is moved _after_ the "while", the
         error message returns. */
      m = i - 2;

      while (sout[j - 1] == ' ' && j > i - 8) j--;
      /*if (j <= i - 2) {*/
      if (j <= m) {  /* 25-May-2016 nm */
        sout[j] = '\t';
        j = i;
        while (sout[j - 1] == ' ' && j > i - 8 + 1) {
          sout[j - 1] = 0;
          j--;
        }
      }
    }
    i = k;
    /* sout[i]=0 is the last character at this point */
    /* Clean up the deleted characters */
    for (j = 0, k = 0; j <= i; j++)
      if (sout[j] != 0) sout[k++] = sout[j];
    sout[k] = 0;
    /* sout[k] = 0 is the last character at this point */
  }

  return (sout);
} /* edit */


/* Return a string of the same character */
vstring string(long n, char c)
{
  vstring sout;
  long j = 0;
  if (n < 0) n = 0;
  sout=tempAlloc(n + 1);
  while (j < n) sout[j++] = c;
  sout[j] = 0;
  return (sout);
} /* string */


/* Return a string of spaces */
vstring space(long n)
{
  return (string(n, ' '));
} /* space */


/* Return a character given its ASCII value */
vstring chr(long n)
{
  vstring sout;
  sout = tempAlloc(2);
  sout[0] = (char)(n & 0xFF);
  sout[1] = 0;
  return(sout);
} /* chr */


/* Search for string2 in string1 starting at start_position */
/* If there is no match, 0 is returned */
/* If string2 is "", (length of the string) + 1 is returned */
long instr(long start_position, vstring string1, vstring string2)
{
  char *sp1, *sp2;
  long ls1, ls2;
  long found = 0;
  if (start_position < 1) start_position = 1;
  ls1 = (long)strlen(string1);
  ls2 = (long)strlen(string2);
  if (start_position > ls1) start_position = ls1 + 1;
  sp1 = string1 + start_position - 1;
  while ((sp2 = strchr(sp1, string2[0])) != 0) {
    if (strncmp(sp2, string2, (size_t)ls2) == 0) {
      found = sp2 - string1 + 1;
      break;
    } else
      sp1 = sp2 + 1;
  }
  return (found);
} /* instr */


/* 12-Jun-2011 nm Added rinstr */
/* Search for _last_ occurrence of string2 in string1 */
/* 1 = 1st string character; 0 = not found */
/* ??? Future - this could be made more efficient by searching directly,
   backwards from end of string1 */
long rinstr(vstring string1, vstring string2)
{
  long pos = 0;
  long savePos = 0;

  while (1) {  /* Scan until substring no longer found */
    pos = instr(pos + 1, string1, string2);
    if (!pos) break;
    savePos = pos;
  }
  return (savePos);
} /* rinstr */


/* Translate string in sin to sout based on table.
   Table must be 256 characters long!! <- not true anymore? */
vstring xlate(vstring sin,vstring table)
{
  vstring sout;
  long len_table, len_sin;
  long i, j;
  long table_entry;
  char m;
  len_sin = (long)strlen(sin);
  len_table = (long)strlen(table);
  sout = tempAlloc(len_sin+1);
  for (i = j = 0; i < len_sin; i++)
  {
    table_entry = 0x000000FF & (long)sin[i];
    if (table_entry < len_table)
      if ((m = table[table_entry])!='\0')
        sout[j++] = m;
  }
  sout[j]='\0';
  return (sout);
} /* xlate */


/* Returns the ascii value of a character */
long ascii_(vstring c)
{
  return ((long)c[0]);
} /* ascii_ */


/* Returns the floating-point value of a numeric string */
double val(vstring s)
{
  double v = 0;
  char signFound = 0;
  double power = 1.0;
  long i;
  for (i = (long)strlen(s); i >= 0; i--) {
    switch (s[i]) {
      case '.':
        v = v / power;
        power = 1.0;
        break;
      case '-':
        signFound = 1;
        break;
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        v = v + ((double)(s[i] - '0')) * power;
        power = 10.0 * power;
        break;
    }
  }
  if (signFound) v = - v;
  return v;
  /*
  return (atof(s));
  */
} /* val */


/* Returns current date as an ASCII string */
vstring date()
{
  vstring sout;
  struct tm *time_structure;
  time_t time_val;
  char *month[12];

  /* (Aggregrate initialization is not portable) */
  /* (It must be done explicitly for portability) */
  month[0] = "Jan";
  month[1] = "Feb";
  month[2] = "Mar";
  month[3] = "Apr";
  month[4] = "May";
  month[5] = "Jun";
  month[6] = "Jul";
  month[7] = "Aug";
  month[8] = "Sep";
  month[9] = "Oct";
  month[10] = "Nov";
  month[11] = "Dec";

  time(&time_val); /* Retrieve time */
  time_structure = localtime(&time_val); /* Translate to time structure */
  sout = tempAlloc(15); /* 8-Mar-2019 nm Changed from 12 to 15 to prevent
                           gcc 8.3 warning (patch provided by David Starner) */
  /* "%02d" means leading zeros with min. field width of 2 */
  /* sprintf(sout,"%d-%s-%02d", */
  sprintf(sout,"%d-%s-%04d", /* 10-Apr-06 nm 4-digit year */
      time_structure->tm_mday,
      month[time_structure->tm_mon],
      /* time_structure->tm_year); */ /* old */
      /* (int)((time_structure->tm_year) % 100)); */ /* Y2K */
      (int)((time_structure->tm_year) + 1900)); /* 10-Apr-06 nm 4-digit yr */
  return(sout);
} /* date */


/* Return current time as an ASCII string */
vstring time_()
{
  vstring sout;
  struct tm *time_structure;
  time_t time_val;
  int i;
  char *format;
  char *format1 = "%d:%d %s";
  char *format2 = "%d:0%d %s";
  char *am_pm[2];
  /* (Aggregrate initialization is not portable) */
  /* (It must be done explicitly for portability) */
  am_pm[0] = "AM";
  am_pm[1] = "PM";

  time(&time_val); /* Retrieve time */
  time_structure = localtime(&time_val); /* Translate to time structure */
  if (time_structure->tm_hour >= 12)
    i = 1;
  else
    i = 0;
  if (time_structure->tm_hour > 12)
    time_structure->tm_hour -= 12;
  if (time_structure->tm_hour == 0)
    time_structure->tm_hour = 12;
  sout = tempAlloc(12);
  if (time_structure->tm_min >= 10)
    format = format1;
  else
    format = format2;
  sprintf(sout,format,
      time_structure->tm_hour,
      time_structure->tm_min,
      am_pm[i]);
  return(sout);
} /* time */


/* Return a number as an ASCII string */
vstring str(double f)
{
  /* This function converts a floating point number to a string in the */
  /* same way that %f in printf does, except that trailing zeroes after */
  /* the one after the decimal point are stripped; e.g., it returns 7 */
  /* instead of 7.000000000000000. */
  vstring s;
  long i;
  s = tempAlloc(50);
  sprintf(s,"%f", f);
  if (strchr(s, '.') != 0) { /* The string has a period in it */
    for (i = (long)strlen(s) - 1; i > 0; i--) {  /* Scan string backwards */
      if (s[i] != '0') break; /* 1st non-zero digit */
      s[i] = 0; /* Delete the trailing 0 */
    }
    if (s[i] == '.') s[i] = 0; /* Delete trailing period */
/*E*/INCDB1(-(49 - (long)strlen(s)));
  }
  return (s);
} /* str */


/* Return a number as an ASCII string */
/* (This may have differed slightly from str() in BASIC but I forgot how.
   It should be considered deprecated.) */
vstring num1(double f)
{
  return (str(f));
} /* num1 */


/* Return a number as an ASCII string surrounded by spaces */
/* (This should be considered deprecated.) */
vstring num(double f)
{
  return (cat(" ",str(f)," ",NULL));
} /* num */



/*** NEW FUNCTIONS ADDED 11/25/98 ***/

/* Emulate PROGRESS "entry" and related string functions */
/* (PROGRESS is a 4-GL database language) */

/* A "list" is a string of comma-separated elements.  Example:
   "a,b,c" has 3 elements.  "a,b,c," has 4 elements; the last element is
   an empty string.  ",," has 3 elements; each is an empty string.
   In "a,b,c", the entry numbers of the elements are 1, 2 and 3 (i.e.
   the entry numbers start a 1, not 0). */

/* Returns a character string entry from a comma-separated
   list based on an integer position. */
/* If element is less than 1 or greater than number of elements
   in the list, a null string is returned. */
vstring entry(long element, vstring list)
{
  vstring sout;
  long commaCount, lastComma, i, length;
  if (element < 1) return ("");
  lastComma = -1;
  commaCount = 0;
  i = 0;
  while (list[i] != 0) {
    if (list[i] == ',') {
      commaCount++;
      if (commaCount == element) {
        break;
      }
      lastComma = i;
    }
    i++;
  }
  if (list[i] == 0) commaCount++;
  if (element > commaCount) return ("");
  length = i - lastComma - 1;
  if (length < 1) return ("");
  sout = tempAlloc(length + 1);
  strncpy(sout, list + lastComma + 1, (size_t)length);
  sout[length] = 0;
  return (sout);
}

/* Emulate PROGRESS lookup function */
/* Returns an integer giving the first position of an expression
   in a comma-separated list. Returns a 0 if the expression
   is not in the list. */
long lookup(vstring expression, vstring list)
{
  long i, exprNum, exprPos;
  char match;

  match = 1;
  i = 0;
  exprNum = 0;
  exprPos = 0;
  while (list[i] != 0) {
    if (list[i] == ',') {
      exprNum++;
      if (match) {
        if (expression[exprPos] == 0) return exprNum;
      }
      exprPos = 0;
      match = 1;
      i++;
      continue;
    }
    if (match) {
      if (expression[exprPos] != list[i]) match = 0;
    }
    i++;
    exprPos++;
  }
  exprNum++;
  if (match) {
    if (expression[exprPos] == 0) return exprNum;
  }
  return 0;
}


/* Emulate PROGRESS num-entries function */
/* Returns the number of items in a comma-separated list.  If the
   list is the empty string, return 0. */
long numEntries(vstring list)
{
  long i, commaCount;
  if (list[0] == 0) {
    commaCount = -1; /* 26-Apr-2006 nm Return 0 if list empty */
  } else {
    commaCount = 0;
    i = 0;
    while (list[i] != 0) {
      if (list[i] == ',') commaCount++;
      i++;
    }
  }
  return (commaCount + 1);
}

/* Returns the character position of the start of the
   element in a list - useful for manipulating
   the list string directly.  1 means the first string
   character. */
/* If element is less than 1 or greater than number of elements
   in the list, a 0 is returned.  If entry is null, a 0 is
   returned. */
long entryPosition(long element, vstring list)
{
  long commaCount, lastComma, i;
  if (element < 1) return 0;
  lastComma = -1;
  commaCount = 0;
  i = 0;
  while (list[i] != 0) {
    if (list[i] == ',') {
      commaCount++;
      if (commaCount == element) {
        break;
      }
      lastComma = i;
    }
    i++;
  }
  if (list[i] == 0) {
    if (i == 0) return 0;
    if (list[i - 1] == ',') return 0;
    commaCount++;
  }
  if (element > commaCount) return (0);
  if (list[lastComma + 1] == ',') return 0;
  return (lastComma + 2);
}



/* For debugging */
/*

int main(void)
{
  vstringdef(s);
  vstringdef(t);

  printf("Hello\n");
  let(&t,edit(" x y z ",2));
  let(&s,cat(right("abc",2),left("def",len(right("xxx",2))),"ghi",t,NULL));
  printf("%s\n",s);
  printf("num %s\n",num(5));
  printf("str %s\n",str(5.02));
  printf("num1 %s\n",num1(5.02));
  printf("time_ %s\n",time_());
  printf("date %s\n",date());
  printf("val %f\n",val("6.77"));
}

*/
