/*****************************************************************************/
/*               Copyright (C) 1998, NORMAN D. MEGILL                        */
/*****************************************************************************/

/******************************************************************************
 *
 *  This is a collection of useful functions
 *
 ******************************************************************************/

/* Redefine non-ANSII functions for other compilers */
#ifndef w32cfree
#define w32cfree free
#endif
#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/******************************************************************************
 *
 *  Generic C
 *
 ******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>                     /* For choice routines */
#include <ctype.h>                      /* For macro */
#include <time.h>                       /* For CPUTime routines */
#include <stdarg.h>                     /* For print handlers */

#include "mmutil.h"

/* Add additional prototype definitions not in vmc.h */
int vmc_checkTokenBufferSize(int size);
void vmc_toUpper(char *s);


/* Message handling */
#define NORMAL          0x0000
#define STATUS          0x0100
#define WARNING         0x0200
#define ERROR           0x0300
#define FATAL           0x0400
#define CONT            0x0500
#define DEBUG           0x0600
#define OVERLAP         0x1000

/******************************************************************************
        VMC print handler
 ******************************************************************************/
void (*vmc_print)(int severity,char *fmt,...)=vmc_genericPrint;

/* Generic print handler
   Ignore the severity level and print the message as it is specified */
void vmc_genericPrint(int severity,char *fmt,...)
{
  va_list ap;
  va_start(ap,fmt);
  vprintf(fmt,ap);
  va_end(ap);
}

/* Standard print handler
   Print the message with header and trailer depending on the severity level */
void vmc_standardPrint(int severity,char *fmt,...)
{
  va_list ap;
  /* Examine bit 8-11 for the severity level */
  switch(severity & 0x0f00)
  {
    case NORMAL:        break;
    case DEBUG:         break;
    case STATUS:        break;
    case WARNING:       printf("\nWARNING:\n");         break;
    case ERROR:         printf("\nERROR:\n");           break;
    case FATAL:         printf("\nFATAL ERROR:\n");     break;
    case CONT:          break;
    default:            printf("\nPROGRAMMING ERROR!!\n");
                        printf("Unknown severity code %d (Hex %x)\n",severity,
                                severity);
                        severity=FATAL;
#ifdef VMS
                        exit(1);
#else
                        abort();
#endif
  }
  va_start(ap,fmt);
  vprintf(fmt,ap);
  va_end(ap);
  /* Do terminal line erase and CR if needed */
  if (severity & OVERLAP) printf("\033[0J\r");
}

/******************************************************************************
                 String creation/copying routines
 ******************************************************************************/

/* Allocate static memory for a string
   Note: w32cfree should be called to free allocated memory afterwards !! */
char *vmc_strCreate(i)
long i;
{
  char *s;
  if ((s=(char *) calloc(i,sizeof(char))) == NULL) {
    (*vmc_print)(FATAL,
        "In vmc_strCreate: a NULL pointer is returned by calloc\n");
  }
  return (s);
}

/* Allocate static memory and copy string
   Note: w32cfree should be called to free allocated memory afterwards !! */
char *vmc_strSave(c)
char *c;
{
  char *newc;
  newc=vmc_strCreate(strlen(c)+1);
  strcpy(newc,c);
  return (newc);
}

/* Get user input as a string
   Note: w32cfree should be called to free allocated memory afterwards !! */
char *vmc_getLine(void)
{
#define VMC_GETLINE_MAX_CHAR 256
  char c[VMC_GETLINE_MAX_CHAR];
  int i=0;
  while (((c[i]=getchar())!=EOF) && (c[i]!='\n') && (i<VMC_GETLINE_MAX_CHAR-1))
    /* if ((c[i]!='\t') && (c[i]!=' ')) */      /* Ignore tabs and spaces */
      i++;                                      /* Store current character */
  c[i]='\0';            /* Add terminator to terminate string */
  fflush(stdin);        /* Flush input buffer to clear remaining characters */
  return (vmc_strSave(c));
}

/******************************************************************************
                         Ask user for input
 ******************************************************************************/

/* Ask user for an int
   Prompt the user to return a int value.   All tabs, spaces are ignored.
   The user should type in the return value followed by a carriage return.
   If only a carriage return is typed, the default value will be returned.
   If use_default is TRUE, than the default value will be used without
   asking the user.
*/
int vmc_askInt(prompt,default_answer,use_default)
char *prompt;
int default_answer;
int use_default;
{
  char *c;
  int i;
  /* Print prompt string */
  (*vmc_print)(NORMAL,"%s <%d>? ",prompt,default_answer);
  if (!use_default) {
    c=vmc_getLine();
    if (strlen(c)<1)
      i=default_answer;
    else
      i=atol(c);
    w32cfree(c);
  } else {
    (*vmc_print)(NORMAL,"%d\n",default_answer);
    i=default_answer;
  }
  return i;
}

/* Ask user for a long int
   Prompt the user to return a long value.   All tabs, spaces are ignored.
   The user should type in the return value followed by a carriage return.
   If only a carriage return is typed, the default value will be returned.
   If use_default is TRUE, than the default value will be used without
   asking the user.
*/
long vmc_askLong(prompt,default_answer,use_default)
char *prompt;
long default_answer;
int use_default;
{
  char *c;
  long i;
  /* Print prompt string */
  (*vmc_print)(NORMAL,"%s <%d>? ",prompt,default_answer);
  if (!use_default) {
    c=vmc_getLine();
    if (strlen(c)<1)
      i=default_answer;
    else
      i=atol(c);
    w32cfree(c);
  } else {
    (*vmc_print)(NORMAL,"%d\n",default_answer);
    i=default_answer;
  }
  return i;
}

#ifndef VAXC
#ifndef VMS
/* Ask user for a float
   Prompt the user to return a float value.   All tabs, spaces are ignored.
   The user should type in the return value followed by a carriage return.
   If only a carriage return is typed, the default value will be returned.
   If use_default is TRUE, than the default value will be used without
   asking the user.
*/
float vmc_askFloat(char *prompt,float default_answer,int use_default)
{
  char *c;
  float f;
  /* Print prompt string */
  (*vmc_print)(NORMAL,"%s <%f>? ",prompt,default_answer);
  if (!use_default) {
    c=vmc_getLine();
    if (strlen(c)<1)
      f=default_answer;
    else
      sscanf(c,"%f",&f); /* f=atof(c); */
    w32cfree(c);
  } else {
    (*vmc_print)(NORMAL,"%f\n",default_answer);
    f=default_answer;
  }
  return (f);
}
#endif
#endif

/* Ask user for a double
   Prompt the user to return a double value.   All tabs, spaces are ignored.
   The user should type in the return value followed by a carriage return.
   If only a carriage return is typed, the default value will be returned.
   If use_default is TRUE, than the default value will be used without
   asking the user.
*/
double vmc_askDouble(prompt,default_answer,use_default)
char *prompt;
double default_answer;
int use_default;
{
  char *c;
  double f;
  /* Print prompt string */
  (*vmc_print)(NORMAL,"%s <%f>? ",prompt,default_answer);
  if (!use_default) {
    c=vmc_getLine();
    if (strlen(c)<1)
      f=default_answer;
    else
      sscanf(c,"%f",&f); /* f=atof(c); */
    w32cfree(c);
  } else {
    (*vmc_print)(NORMAL,"%f\n",default_answer);
    f=default_answer;
  }
  return ((double)f);
}

/* Ask user for a string
   Prompt the user to return a string.   All tabs, spaces are ignored.
   The user should type in the return value followed by a carriage return.
   If only a carriage return is typed, the default value will be returned.
   If use_default is TRUE, than the default value will be used without
   asking the user.
*/
char *vmc_askString(prompt,default_answer,use_default)
char *prompt;
char *default_answer;
int use_default;
{
  char *c;
  if (strlen(default_answer))
    /* Print prompt string */
    (*vmc_print)(NORMAL,"%s <%s>? ",prompt,default_answer);
  else
    (*vmc_print)(NORMAL,"%s ? ",prompt);        /* Print prompt string */
  if (!use_default) {
    c=vmc_getLine();
    if (strlen(c)<1)
      c=vmc_strSave(default_answer);
  } else {
    (*vmc_print)(NORMAL,"%s\n",default_answer);
    c=vmc_strSave(default_answer);
  }
  return c;
}

/* Ask user to answer Y/N to a yes/no question */
int vmc_askYesNo(char *prompt,char *default_answer,int use_default)
{
  char *s,c;
  s=vmc_askString(prompt,default_answer,use_default);
  c= *s;
  w32cfree(s);
  if (c=='Y' || c=='y') return(TRUE);
  else return(FALSE);
}

/******************************************************************************
                         Open file
 ******************************************************************************/
/* Prompt the user for a file name and open the file.  This function return the
   file pointer and the file name (as the first argument). */
FILE *vmc_fopen(char **fileNamePtr,char *prompt,char *defaultAnswer,char *mode,
        int useDefault)
{
  FILE *fp=NULL;
  while(!fp)
  {
    *fileNamePtr=vmc_askString(prompt,defaultAnswer,useDefault);
    fp=fopen(*fileNamePtr,mode);
    if (!fp)
    {
      (*vmc_print)(ERROR,"%s cannot be open.\n",*fileNamePtr);
      w32cfree(*fileNamePtr);
      if (useDefault)
      {
        (*vmc_print)(CONT,"Program will be aborted\n");
        exit(1);
      }
    }
  }
  return(fp);
}

/*****************************************************************************
                        Input line buffer management
 *****************************************************************************/
/*****************************************************************************
 Buffer for error message and error recovery (processed by token() and
 skip_tokenbuffer() only)
 *****************************************************************************/
#define VMC_MAX_TOKENBUFFER                     10240
char vmc_tokenBuffer[VMC_MAX_TOKENBUFFER];
int vmc_lastBp=0,vmc_currentBp=0;

int vmc_checkTokenBufferSize(int size) {
  if (size>=VMC_MAX_TOKENBUFFER) {
    (*vmc_print)(ERROR,"vmc_tokenBuffer is overflowed!\n");
    (*vmc_print)(CONT,"The buffer will be cleared.\n");
    vmc_clearTokenBuffer();
    return(0);
  } else {
    return(size);
  }
}

void vmc_clearTokenBuffer(void) {
  vmc_lastBp=vmc_currentBp=0;
}

/* Find the end of a statement as indicated by "terminator" and print
   the statement.  If "terminator" is empty, don't read any more tokens
   and just print what has been processed. */
void vmc_skipUntil(FILE *fp,char *terminator) {
  /* Skip until a specified terminator is found. Print vmc_tokenBuffer and
     mark the token in question.  Then clear vmc_tokenBuffer. */
  char token[VMC_MAX_TOKENBUFFER];
  int i,length,markerPrinted=0;
  int save_bp1,save_bp2;
  /* Lex control */
  char *cp;
  struct vmc_lexControlStructure save_lexControl;
  int lexControlChanged=0;
  /* Check to see if \n is considered as a single character token */
  if (strchr(vmc_lexControl.single,'\n')) {
    /* Yes it is.  We don't need to do anything */
  } else {
    /* No it isn't.  Flag that we shall change vmc_lexControl */
    lexControlChanged=1;
    /* Save the current vmc_lexControl */
    save_lexControl.single=vmc_lexControl.single;
    save_lexControl.space=vmc_lexControl.space;
    /* Create new single character string with \n */
    i=strlen(vmc_lexControl.single);
    vmc_lexControl.single=vmc_strCreate(i+2);
    strcpy(vmc_lexControl.single,save_lexControl.single);
    strcat(vmc_lexControl.single,"\n");
    /* Make a copy of vmc_lexControl.space */
    vmc_lexControl.space=vmc_strSave(vmc_lexControl.space);
    /* Check to see of \n is used as a space delimiter */
    if (cp=strchr(vmc_lexControl.space,'\n')) {
      /* Yes it is.  Remove it */
      while (*cp!='\0') {
        *cp = *(cp+1);
        cp++;
      }
    }
  }
  /* Save the markers for the current token */
  save_bp1=vmc_lastBp;
  save_bp2=vmc_currentBp;
  /* Print message */
  if (*terminator)
    (*vmc_print)(NORMAL,"The rest of the statement will be ignored:\n");
  else
    (*vmc_print)(NORMAL,"This (partial) statement will be ignored:\n");
  /* Keep reading tokens until the terminator is found */
  token[0]='\0';
  length=0;
  for (;;) {
    if (*terminator)
      length=vmc_lexGetSymbol(token,fp);
    /* Print vmc_tokenBuffer[] if a \n is found.  Otherwise, vmc_tokenBuffer[]
       will be cleared when vmc_tokenBuffer() is executed next time */
    if (*terminator=='\0' || token[0]=='\n' || !strcmp(token,terminator)) {
      /* Insert a \n for printing the buffer */
      if (vmc_tokenBuffer[vmc_currentBp-1]!='\n')
        vmc_tokenBuffer[vmc_currentBp++]='\n';
      /* Terminate the token buffer */
      vmc_tokenBuffer[vmc_checkTokenBufferSize(vmc_currentBp)]='\0';
      (*vmc_print)(CONT,"%s",vmc_tokenBuffer);
      /* Just to be safe, let's clear vmc_tokenBuffer[] */
      vmc_clearTokenBuffer();
      /* Print the marker? */
      if (!markerPrinted) {
        /* Print the markers for the token in question */
        markerPrinted=1;
        for (i=0; i<save_bp1; i++)
                /* vmc_lastBp is pointing to the last character that was read
                   in */
          if (vmc_tokenBuffer[i]=='\n')
            (*vmc_print)(CONT,"%c",'\r');
          else
            (*vmc_print)(CONT,"%c",' ');
        (*vmc_print)(CONT,"%c",'^');
        for (i=save_bp1+1; i<save_bp2-1; i++)
                /* save_bp is pointing to the next character that should be
                   read in */
          if (vmc_tokenBuffer[i]=='\n')
            (*vmc_print)(CONT,"%c",'\r');
          else
            (*vmc_print)(CONT,"%c",'-');
        (*vmc_print)(CONT,"%s","^\n");
      }
    }
    if (*terminator) {
      if (!length || !strcmp(token,terminator)) break;
    } else
      break;
  }
  /* Restore vmc_lexControl if it was changed */
  if (lexControlChanged) {
    w32cfree(vmc_lexControl.single);
    w32cfree(vmc_lexControl.space);
    vmc_lexControl.single=save_lexControl.single;
    vmc_lexControl.space=save_lexControl.space;
  }
}

void vmc_skipTokenBuffer(FILE *fp) {
  int save_bp,c,i;
  (*vmc_print)(NORMAL,"The rest of the line will be ignored in:\n");
  save_bp=vmc_currentBp;
  while ((c=getc(fp))!=EOF) {
    if (c=='\n') break;
    else vmc_tokenBuffer[vmc_checkTokenBufferSize(vmc_currentBp++)]=c;
  }
  vmc_tokenBuffer[vmc_checkTokenBufferSize(vmc_currentBp)]='\0';
  (*vmc_print)(CONT,"%s\n",vmc_tokenBuffer);
  for (i=0; i<vmc_lastBp; i++)
                /* vmc_lastBp is pointing to the last character that was read
                   in */
    if (vmc_tokenBuffer[i]=='\n')
      (*vmc_print)(CONT,"%c",'\r');
    else
      (*vmc_print)(CONT,"%c",' ');
  (*vmc_print)(CONT,"%c",'^');
  for (i=vmc_lastBp+1; i<save_bp-1; i++)
                /* save_bp is pointing to the next character that should be
                   read in */
    if (vmc_tokenBuffer[i]=='\n')
      (*vmc_print)(CONT,"%c",'\r');
    else
      (*vmc_print)(CONT,"%c",'-');
  (*vmc_print)(CONT,"%s","^\n");
  vmc_clearTokenBuffer();
}

/*****************************************************************************
                                Lexical analyzer
 *****************************************************************************/

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
/*
struct vmc_lexControlStructure {        / * Example:                     * /
  char *single;                 /- "();+/-*=<>^"                * /
  char *space;                  /- " ,\t\n"                     * /
  char *commentStart;           /- "!\\"                        * /
  char *commentEnd;             /- "\n\\"                       * /
  char *enclosureStart;         /- "\"\'"                       * /
  char *enclosureEnd;           /- "\"\'"                       * /
} vmc_lexControl;
*/

struct vmc_lexControlStructure vmc_lexControl;

void vmc_lexSetup(void)
{
  /* Initialize control variables for token()
     \ should be treated as a comment delimiter
   */
  vmc_lexControl.single=                "@#$%^&*()-+=:;,|/<>?\n[]{}";/* Single character tokens */
  vmc_lexControl.space=                 " \t"           ;/* Tokens delimiters */
  vmc_lexControl.commentStart=          "!\\"           ;/* Start of comments */
  vmc_lexControl.commentEnd=            "\n\\"          ;/* End of comments */
  vmc_lexControl.enclosureStart=        "\"\'"          ;/* Start of special enclosure tokens */
  vmc_lexControl.enclosureEnd=          "\"\'"          ;/* End of special enclosure tokens  */
}

long vmc_lineCount=1;

int vmc_lexGetSymbol(char *s,FILE *fp) {
  int c,ls;
  char *cp;
  /* Processing state */
  enum processingState
    {space,enclosure,comment,token,endOfToken} state;
  int substate;

  if (vmc_currentBp &&
       (vmc_tokenBuffer[vmc_currentBp-1]=='\n'))
    /* If the last character we looked at was a \n, then the last line can be
       discarded in the error message vmc_tokenBuffer[] */
    vmc_lastBp=vmc_currentBp=0;
  else
    vmc_lastBp=vmc_currentBp; /* Save position where last token terminated */
  /* Do lexical analysis on line line and spit out token */
  state=space; ls=0; s[ls]='\0';
  while (state!=endOfToken) {
    /* Read next character.  If EOF, then exit loop */
    if ((c=getc(fp))==EOF) break;
    /* Change tab and FF into space so that vmc_skipUntil() can print the marker
       correctly */
    if ((c=='\t') || (c=='\f')) c=' ';
    if (c=='\n') vmc_lineCount++;
    /* Save character into buffer for error messages */
    vmc_tokenBuffer[vmc_checkTokenBufferSize(vmc_currentBp++)]=c;
    /* Process character base on current state */
    if (state==space) {
      if (strchr(vmc_lexControl.space,c)) {
        /* Do nothing. Remain in space state */
      } else if (strchr(vmc_lexControl.single,c)) {
        s[ls++]=c;              /* Accept as new token */
        state=endOfToken;       /* Terminate token */
      } else if (cp=strchr(vmc_lexControl.enclosureStart,c)) {
        s[ls++]=c;              /* Accept as new token */
        substate=(int)(cp-vmc_lexControl.enclosureStart);
                                /* Which character is found ?*/
        state=enclosure;
      } else if (cp=strchr(vmc_lexControl.commentStart,c)) {
        substate=(int)(cp-vmc_lexControl.commentStart);
                                /* Which character is found ?*/
        state=comment;
      } else {
        s[ls++]=c;              /* Accept as new token */
        state=token;            /* Terminate token */
      }
    } else if (state==enclosure) {
      s[ls++]=c;                /* Accept character */
      if (c==*(vmc_lexControl.enclosureEnd+substate))
        state=endOfToken;       /* Terminate enclosure token */
    } else if (state==comment) {
      if (c==*(vmc_lexControl.commentEnd+substate)) {
        state=space;            /* Terminate comment */
        vmc_clearTokenBuffer();
      }
    } else if (state==token) {
      if (strchr(vmc_lexControl.space,c)) {
        ungetc(c,fp);
        if (c=='\n') vmc_lineCount--;
        vmc_currentBp--;
        state=endOfToken;       /* Terminate token */
      } else if (strchr(vmc_lexControl.single,c)) {
        ungetc(c,fp);   /* Return character as if it has never been read */
        if (c=='\n') vmc_lineCount--;
        vmc_currentBp--;
        state=endOfToken;       /* Terminate token */
      } else if (cp=strchr(vmc_lexControl.enclosureStart,c)) {
        ungetc(c,fp);   /* Return character as if it has never been read */
        if (c=='\n') vmc_lineCount--;
        vmc_currentBp--;
        state=endOfToken;       /* Terminate token */
      } else if (cp=strchr(vmc_lexControl.commentStart,c)) {
        ungetc(c,fp);   /* Return character as if it has never been read */
        if (c=='\n') vmc_lineCount--;
        vmc_currentBp--;
        state=endOfToken;       /* Terminate token */
      } else {
        s[ls++]=c;              /* Accept character as part of token */
      }
    }
  }
  s[ls]='\0';
  return(strlen(s));
}

/******************************************************************************
                         Pause at breakpoint
 ******************************************************************************/

/* Pause for debugging */
void vmc_debugPause(char *s)
{
  int i;
  if (s) (*vmc_print)(DEBUG,"Pause at breakpoint: %s\n",s);
  (*vmc_print)(CONT,"Hit <return> to continue\n");
  i=getchar();
}

/******************************************************************************
                         Current directory
 ******************************************************************************/

/* Return current directory */
char *vmc_currentDirectory(void)
{
  return(getenv("PATH"));
}

/******************************************************************************
                         Current date/time
 ******************************************************************************/

/* Returns current date as an ASCII string */
char *vmc_date(void)
{
        char *sout;
        struct tm *time_structure;
        time_t time_val;
        char *month[12];

        /* (Aggregrate initialization is not portable) */
        /* (It must be done explicitly for portability) */
        month[0]="Jan";
        month[1]="Feb";
        month[2]="Mar";
        month[3]="Apr";
        month[4]="May";
        month[5]="Jun";
        month[6]="Jul";
        month[7]="Aug";
        month[8]="Sep";
        month[9]="Oct";
        month[10]="Nov";
        month[11]="Dec";

        time(&time_val);                        /* Retrieve time */
        time_structure=localtime(&time_val); /* Translate to time structure */
        sout=vmc_strCreate(12);
        sprintf(sout,"%d-%s-%d",
                time_structure->tm_mday,
                month[time_structure->tm_mon],
                time_structure->tm_year);
        return(sout);
}

/* Return current time as an ASCII string */
char *vmc_time(void)
{
        char *sout;
        struct tm *time_structure;
        time_t time_val;
        int i;
        char *format;
        char *format1="%d:%d %s";
        char *format2="%d:0%d %s";
        char *am_pm[2];
        /* (Aggregrate initialization is not portable) */
        /* (It must be done explicitly for portability) */
        am_pm[0]="AM";
        am_pm[1]="PM";

        time(&time_val);                        /* Retrieve time */
        time_structure=localtime(&time_val); /* Translate to time structure */
        if (time_structure->tm_hour>=12) i=1;
        else                             i=0;
        if (time_structure->tm_hour>12) time_structure->tm_hour-=12;
        sout=vmc_strCreate(12);
        if (time_structure->tm_min>=10)
          format=format1;
        else
          format=format2;
        sprintf(sout,format,
                time_structure->tm_hour,
                time_structure->tm_min,
                am_pm[i]);
        return(sout);
}

/******************************************************************************
                         Convert string to float
 ******************************************************************************/
#ifdef VAXC
float vmc_atof(char *c) {
  /* This function does the same atof() is intended */
  /* to do.  However, atof() has a bug, so vmc_atof() */
  /* should be used instead. */
  float f;
  sscanf(c,"%f",&f);
  return f;
}
#else
/* This definition is done in VMC.H
#define vmc_atof(c) atof(c)
*/
#endif

#ifdef VAX
/******************************************************************************
                         Convert string to double
 ******************************************************************************/
double vmc_atod(char *s)
{
  double d;
  float f;
  /* Sometimes %lf doesn't work on the VAX.  (Looks like a bug in sscanf.)
     But %f seems to always work.  So scan the number in as a float and then
     convert it to a double */
  /* sscanf(s,"%lf",&d); */
  sscanf(s,"%f",&f);
  d=f;
  return(d);
}
#endif

#ifdef VAXC     /* Start of VAX C specific code */
/******************************************************************************
 *
 *  VAX C
 *
 ******************************************************************************/

#include <ssdef.h>                      /* For SS$xxx constants */
#include <iodef.h>                      /* For IO$xxx constants in vmc_getKey() */
#include <jpidef.h>                     /* For JPI$_xxx constants in vmc_getJPI */
#include <lnmdef.h>                     /* For LNM$xxx constants */
/* "descrip" is needed for vmc_getccl() and vmc_b2cstring().  It is also
    needed to the user wants to use bstring as declared in vmc.h.  Because
    that "descrip" is included in vmc.h, it is not included here again. */
/* #include <descrip .h>*/

/******************************************************************************
                         Get command line
 ******************************************************************************/

/* Get the command line */
char *vmc_getCCL(void)
{
   /* String buffer */
   char buffer[256];
   /* Length of string returned by lib$get_common(). Note: This must be a
      short int (16 bits), not a long int (32 bits) !! */
   short length;
   /* Make a string descriptor out of buffer */
   $DESCRIPTOR(des_string,buffer);
   /* Get the common core */
   vmc_checkLibraryCall(lib$get_common(&des_string,&length));
   /* Buffer is padded with 0x20 at the unused locations.  Terminate the
      string as we know where it actually ends */
   buffer[length]='\0';
   return(vmc_strSave(buffer));
}

/******************************************************************************
                 Memory allocation routines
 ******************************************************************************/

/*
   The VMS C Run-Time-Library does not seem to free memory space correctly.
   Memory that is supposed to be freed cannot be reused.  The VMS System Library
   functions are called directly here in the following routines.
*/

void *vmc_malloc(int size)
{
  void *ptr;
  int bytePerInt=sizeof(int);
  /* Add extra storage for storing how many bytes are allocated in the
     lib$get_vm() operation */
  size += bytePerInt;
  /* Request VM */
  vmc_checkLibraryCall(lib$get_vm(&size,&ptr));
  /* Store number of bytes allocated in the beginning of the storage */
  *((int *)ptr) = size;
  /* Return pointer to first free storage location */
  return ((void *)((int)ptr+bytePerInt));
}

int vmc_free(void *ptr)
{
  int size,status;
  /* Backup pointer to beginning of storage */
  ptr = (void *)((int)ptr - sizeof(int));
  size = *((int *)ptr);
  status = lib$free_vm(&size,&ptr);
  vmc_checkLibraryCall(status);
  return(status);
}

void *vmc_calloc(int nmemb,int size)
{
  void *ptr;
  int bsize = nmemb * size;
  /* Request VM by calling vmc_malloc() */
  ptr = vmc_malloc(bsize);
  /* Assign all bytes to 0 */
  if (ptr)
  {
    char *initPtr=ptr;
    while (initPtr<((char *)ptr)+bsize) *initPtr++=0;
  }
  return (ptr);
}

int vmc_cfree(void *ptr)
{
  /* Repeat code as in calling: vmc_free(ptr); */
  int size,status;
  /* Backup pointer to beginning of storage */
  ptr = (void *)((int)ptr - sizeof(int));
  size = *((int *)ptr);
  status = lib$free_vm(&size,&ptr);
  vmc_checkLibraryCall(status);
  return(status);
}

/******************************************************************************
                         Virtual memory query
 ******************************************************************************/

/* Global variables shared by vmc_getVM() and vmc_debugVM() */
int vmc_vmByte=0;
int vmc_vmPage=0;

/* Returns number of bytes of virtual memory allocated by LIB$GET_VM /
   LIB$FREE_VM and LIB$GET_VM_PAGE / LIB$FREE_VM_PAGE system library service */
int *vmc_getVM(void)
{
  int code,byte,page;
  char *cp;
  /* VMS does not give up memory when w32cfree() is called.  It seems to hang
     on to the virtual memory allocation until the next calloc() is called.
     (I think this is done to improve efficiency of dynamic memory allocation.)
     Allocating a 0 byte segment and relinqishing it immediately seems to
     eliminate the bufferring. */
  /* The following lines are comment out to avoid interference with normal
     code execution
  cp=(char *)calloc(0,sizeof(char));
  w32cfree(cp);
  */
  /* Call system function and return result */
  code = 3; /* Code to eturn number of bytes allocated by LIB$GET_VM */
  vmc_checkLibraryCall(lib$stat_vm(&code,&byte));
  code = 7; /* Code to return number of pages allocated by LIB$GET_VM_PAGE */
  vmc_checkLibraryCall(lib$stat_vm(&code,&page));
  /*
  (*vmc_print)(DEBUG,"vmc_getVM() returns: Byte = %d Page = %d Total=%d\n",
        byte,page,byte+512*page);
  */
  vmc_vmByte=byte;
  vmc_vmPage=page;
  return(byte);                 /* return(byte + 512*page); */
}

int vmc_vm=0;

/* Call vmc_getVM() to get the virtual memory storage. Print the amount and
   the difference since last interrogation */
void vmc_debugVM(char *s)
{
  int result;
  result=vmc_getVM();
  (*vmc_print)(DEBUG,"%s LIB$xxx_VM statistics:\n",s);
  (*vmc_print)(CONT," Byte = %9d (Page = %7d); Diff = %9d\n",
        result,vmc_vmPage,result-vmc_vm);
  vmc_vm=result;
}

/* Returns number of virtual pages allocated in P0 space and P1 space */
int *vmc_getVirtualPage(void)
{
  int result;
  unsigned int p0,p1;
  /* Get the address of the first free page at the end of the process's program
     region (P0 space).  This is the same as the size of P0 space used */
  p0=vmc_getJPI(JPI$_FREP0VA);
  /* Get the address of the first free page at the end of the process's control
     region (P1 space) */
  p1=vmc_getJPI(JPI$_FREP1VA);
  /* P1 space grows from 0x7fffffff downwards.  Calculate the P1 space used */
  p1 = 0x7fffffff - p1;
  /* Add the p0 and p1 spaces up.  Convert to closest number of pages */
  result = (p0+p1+256)/512;
  return (result);
}

int vmc_virtualPage=0;

void vmc_debugVirtualPage(char *s)
{
  int result;
  result=vmc_getVirtualPage();
  (*vmc_print)(DEBUG,
        "%s Virtual page statistics:\n Total = %7d page; Diff = %7d page\n",
        s,result,result-vmc_virtualPage);
  vmc_virtualPage=result;
}

/******************************************************************************
                         Job/Process query
 ******************************************************************************/

/* Return a piece of job/process information */
unsigned int *vmc_getJPI(unsigned int code)
{
  unsigned int result;
  /* Call system function and return result */
  vmc_checkLibraryCall(lib$getjpi(&code,NULL,NULL,&result,NULL,NULL));
  return(result);
}

/******************************************************************************
                         BASIC string handling
 ******************************************************************************/

/* Convert a basic string (descriptor) into a c string (character array).
   Note: Dynamic memory is allocated in this function!!
*/
char *vmc_b2cString(bstring s)
{
  int i;
  char *cp,*cp1,*cp2;
  /* Create storage for the string */
  cp=(char *)calloc(s->dsc$w_length+1,sizeof(char));
  /* Copy the string */
  cp1=cp;
  cp2=s->dsc$a_pointer;
  for (i=1; i<=s->dsc$w_length; i++)
    *(cp1++) = *(cp2++);
  *cp1='\0';
  return(cp);
}

/* Convert a c string (character array) into a BASIC string (descriptor).
   Note: Dynamic memory is allocated in this function!!
*/
bstring vmc_c2bString(char *s)
{
  bstring bs;
  int i,j;
  char *cp,*cp1,*cp2;
  /* Create storage for the string */
  j=strlen(s);
  cp=(char *)calloc(j,sizeof(char));
  bs=(bstring)calloc(1,sizeof(bstring *));
  bs->dsc$w_length=j;
  bs->dsc$a_pointer=cp;
  /* Copy the string */
  cp1=cp;
  cp2=s;
  for (i=1; i<=j; i++)
    *(cp1++) = *(cp2++);
  *cp1='\0';
  return(bs);
}

/******************************************************************************
                         System call interface
 ******************************************************************************/

/* Check the return status of a system call */
void vmc_checkSystemCall(char *s,int status)
{
  struct dsc$descriptor_s message;
  char buffer[256];
  char ibuffer[256];
  short int returnLength;
  if (status!=SS$_NORMAL)
  {
    /* Prepare the message buffer as a fix length string descriptor */
    message.dsc$w_length = sizeof(buffer)-1;
    message.dsc$b_dtype = DSC$K_DTYPE_T;
    message.dsc$b_class = DSC$K_CLASS_S;
    message.dsc$a_pointer = buffer;
    /* Get the severity code */
    sys$getmsg(status,&returnLength,&message,4,0);
    buffer[returnLength]='\0';
    if (returnLength>=2)
    {
      /* Check the severity level */
      switch(buffer[1])
      {
        case('I'): strcpy(ibuffer,"Information message"); break;
        case('S'): strcpy(ibuffer,"Status message"); break;
        case('W'): strcpy(ibuffer,"Warning"); break;
        case('E'): strcpy(ibuffer,"Error"); break;
        case('F'): strcpy(ibuffer,"Fatal Error"); break;
        default:   strcpy(ibuffer,"Unknown message"); break;
      }
    }
    /* Get the message */
    sys$getmsg(status,&returnLength,&message,15,0);
    buffer[returnLength]='\0';
    /* Print messages */
    (*vmc_print)(NORMAL,"%s at %s (x'%x'):\n",ibuffer,s,status);
    (*vmc_print)(CONT,"%s\n",buffer);
  }
}

/* Check the return status of a library call */
void vmc_checkLibraryCall(int status)
{
  if (status!=SS$_NORMAL)
  {
    lib$signal(status);
  }
}

/******************************************************************************
                         Get a single keystoke
 ******************************************************************************/

short int vmc_channel=0;

/* Open device_name by assigning a channel to it */
void vmc_openGetKeyDevice(char *device_name)
{
  struct dsc$descriptor_s device;

  /* Get device name */
  device.dsc$b_dtype = DSC$K_DTYPE_T;
  device.dsc$b_class = DSC$K_CLASS_S;
  device.dsc$w_length = strlen(device_name);
  device.dsc$a_pointer = device_name;

  vmc_checkSystemCall("$ASSIGN",sys$assign(&device,&vmc_channel,0,0));
  if (!vmc_channel)
  {
    (*vmc_print)(FATAL,"In vmc_openGetKeyDevice(): Device %s cannot be open\n",
        device.dsc$a_pointer);
  }
  /*
  else
    (*vmc_print)(DEBUG,"Device %s is assigned as channel %d\n",
        device.dsc$a_pointer,vmc_channel);
  */
}

/* Close the opened device by de-assigning the channel to it */
void vmc_closeGetKeyDevice(void)
{
  vmc_checkSystemCall("$DASSGN",sys$dassgn(vmc_channel));
  vmc_channel=0;
}

/* Get a keystroke from SYS$INPUT */
int vmc_getKey(void)
{
  int i;
  char c,c2,c3,c4,c5;

  /* Status block */
  struct statusBlockStructure
  {
    short int status;
    short int offsetTerminator;
    short int firstTerminator;
    short int sizeTerminator;
  } statusBlock;

  /* Is the channel open? */
  if (!vmc_channel)
    vmc_openGetKeyDevice("SYS$INPUT");

  /* Do the qiow */
  vmc_checkSystemCall("$QIOW",
        sys$qiow(0,vmc_channel,
        IO$_READVBLK|IO$M_NOFILTR|IO$M_NOECHO,
        &statusBlock,0,0,
        &c,1,0,0,0,0));
  vmc_checkSystemCall("$QIOW Status Block",statusBlock.status);

/*
  (*vmc_print)(DEBUG,"sys$qiow returned %d character and %d terminator:",
        statusBlock.offsetTerminator,
        statusBlock.sizeTerminator);
*/

  if (c==27) /* Escape */
  {
    /* Read more characters to recognize the escape sequence */
    c2=vmc_getKey();
    if (c2=='O')
    {
      /* Keypad keys */
      c3=vmc_getKey();
      switch(c3)
      {
        case 'P': return(VMC_KEY_PF1);
        case 'Q': return(VMC_KEY_PF2);
        case 'R': return(VMC_KEY_PF3);
        case 'S': return(VMC_KEY_PF4);
        case 'p': return(VMC_KEY_KP0);
        case 'q': return(VMC_KEY_KP1);
        case 'r': return(VMC_KEY_KP2);
        case 's': return(VMC_KEY_KP3);
        case 't': return(VMC_KEY_KP4);
        case 'u': return(VMC_KEY_KP5);
        case 'v': return(VMC_KEY_KP6);
        case 'w': return(VMC_KEY_KP7);
        case 'x': return(VMC_KEY_KP8);
        case 'y': return(VMC_KEY_KP9);
        case 'M': return(VMC_KEY_ENTER);
        case 'm': return(VMC_KEY_MINUS);
        case 'l': return(VMC_KEY_COMMA);
        case 'n': return(VMC_KEY_PERIOD);
        default:  return(VMC_KEY_UNKNOWN);
      }
    } else if (c2=='[') {
      /* Cursor or soft keys */
      c3=vmc_getKey();
      switch(c3)
      {
        case 'A': return(VMC_KEY_UP);
        case 'B': return(VMC_KEY_DOWN);
        case 'C': return(VMC_KEY_RIGHT);
        case 'D': return(VMC_KEY_LEFT);
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
                  break;
        default:  return(VMC_KEY_UNKNOWN);
      }
      /* Read until the tilda key */
      c4=vmc_getKey();
      switch(c4)
      {
        case '~':
          switch(c3)
          {
            case '1': return(VMC_KEY_FIND);
            case '2': return(VMC_KEY_INSERT_HERE);
            case '3': return(VMC_KEY_REMOVE);
            case '4': return(VMC_KEY_SELECT);
            case '5': return(VMC_KEY_PREV_SCREEN);
            case '6': return(VMC_KEY_NEXT_SCREEN);
            default:  return(VMC_KEY_UNKNOWN);
          }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
                  break;
        default:  return(VMC_KEY_UNKNOWN);
      }
      /* Read until the tilda key */
      c5=vmc_getKey();
      switch(c5)
      {
        case '~':
          switch(c3)
          {
            case '1':
              switch(c4)
              {
                case '7': return(VMC_KEY_F6);
                case '8': return(VMC_KEY_F7);
                case '9': return(VMC_KEY_F8);
                default:  return(VMC_KEY_UNKNOWN);
              }
            case '2':
              switch(c4)
              {
                case '0': return(VMC_KEY_F9);
                case '1': return(VMC_KEY_F10);
                case '2': return(VMC_KEY_UNKNOWN);
                case '3': return(VMC_KEY_F11);
                case '4': return(VMC_KEY_F12);
                case '5': return(VMC_KEY_F13);
                case '6': return(VMC_KEY_F14);
                case '7': return(VMC_KEY_UNKNOWN);
                case '8': return(VMC_KEY_HELP);
                case '9': return(VMC_KEY_DO);
                default:  return(VMC_KEY_UNKNOWN);
              }
            case '3':
              switch(c4)
              {
                case '1': return(VMC_KEY_F17);
                case '2': return(VMC_KEY_F18);
                case '3': return(VMC_KEY_F19);
                case '4': return(VMC_KEY_F20);
                default:  return(VMC_KEY_UNKNOWN);
              }
            default: return(VMC_KEY_UNKNOWN);
          }
        default:  return(VMC_KEY_UNKNOWN);
      }
    } else return(VMC_KEY_UNKNOWN);
  } else return(c);
}

/******************************************************************************
                        Spawn a DCL command
 ******************************************************************************/

/* Spawn out a process to execute a DCL command */
void vmc_spawn(char *command)
{
  /* Format command into a string descriptor */
  $DESCRIPTOR(dCommand,"");
  dCommand.dsc$w_length=strlen(command);
  dCommand.dsc$a_pointer=command;
  /* Do the system call */
  vmc_checkLibraryCall(lib$spawn(&dCommand));
  /* lib$spawn() prints its own error messages.  But it does not print a \n
     at the end of the last message */
  (*vmc_print)(NORMAL,"\n");
}

/******************************************************************************
                        File conversion
 ******************************************************************************/

/* Convert a stream_LF file into a variable length file */
void vmc_convertFile(char *infile,char *outfile)
{
  /* Command to send to vmc_spawn */
  char command[256];
  /* Control file for the FDL conversion */
  char *fdl="$VMC:VARIABLE.FDL";
  /* Form the command string */
  strcpy(command,"CONVERT/FDL=");
  strcat(command,fdl);
  strcat(command," ");
  strcat(command,infile);
  strcat(command," ");
  strcat(command,outfile);
  /* Spawn the command */
  vmc_spawn(command);
}

/******************************************************************************
                         CPU Time
 ******************************************************************************/

/* Routines to keep track of cpu time */
static int vmc_startCPUTime=0;
void vmc_initCPUTime(void)
{
  vmc_startCPUTime=clock();
}
int vmc_CPUTime(void)
{
  return((clock()-vmc_startCPUTime)/CLK_TCK);
}

/******************************************************************************
                         Translate logical name
 ******************************************************************************/
/* NOTE: static memory will be allocated !! */
char *vmc_translateLogical(char *logicalName)
{
#define MAX_TRANSLATE_NAME 256
  char translateName[MAX_TRANSLATE_NAME];
  char *cp;
  short int returnLength;
  $DESCRIPTOR(dLogicalName,"");
  $DESCRIPTOR(dTranslateName,"");
  /* Convert logicalName to upper case */
  cp=logicalName;
  while (*cp) { *cp= _toupper(*cp); cp++; }
  /* Set up the descriptors */
  dLogicalName.dsc$w_length=strlen(logicalName);
  dLogicalName.dsc$a_pointer=logicalName;
  dTranslateName.dsc$w_length=MAX_TRANSLATE_NAME-1;
  dTranslateName.dsc$a_pointer=translateName;
  /* Do the system call */
  /* vmc_checkSystemCall("SYS$TRNLOG", */
        sys$trnlog(&dLogicalName,&returnLength,
        &dTranslateName,0,0,0)
        /*)*/;
  /* Analyse result */
  if (returnLength>=MAX_TRANSLATE_NAME) {
    (*vmc_print)(ERROR,
      "sys$trnlog returned too many characters (%d) in translating %s\n",
      returnLength,logicalName);
    returnLength=MAX_TRANSLATE_NAME-1;
  }
  translateName[returnLength]='\0';
  /* Return result */
  return(vmc_strSave(translateName));
#undef MAX_TRANSLATE_NAME
}

/******************************************************************************
                         Translate logical name
 ******************************************************************************/
void vmc_defineLogical(char *table,char *logical,char *equivalent)
{
  struct itemListStructure
  {
    short int bufferLength;
    short int itemCode;
    char *bufferAddress;
    int returnLengthAddress;
    int terminator;
  } itemList;

  $DESCRIPTOR(tableName,"");
  $DESCRIPTOR(logicalName,"");

  tableName.dsc$w_length=strlen(table);
  tableName.dsc$a_pointer=table;
  logicalName.dsc$w_length=strlen(logical);
  logicalName.dsc$a_pointer=logical;

  itemList.bufferLength=strlen(equivalent);
  itemList.itemCode=LNM$_STRING;
  itemList.bufferAddress=equivalent;
  itemList.returnLengthAddress=0;
  itemList.terminator=0;

  vmc_checkSystemCall("SYS$CRELNM",
    SYS$CRELNM(0,&tableName,&logicalName,0,&itemList)
  );
}
#endif          /* End of VAX C specific code */
