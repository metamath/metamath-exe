/*****************************************************************************/
/*       Copyright (C) 2002  NORMAN D. MEGILL nm@alum.mit.edu                */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* HTML flags */
extern flag htmlFlag;  /* HTML flag: 0 = TeX, 1 = HTML */
extern flag altHtmlFlag;  /* Use "althtmldef" instead of "htmldef".  This is
    intended to allow the generation of pages with the old Symbol font
    instead of the individual GIF files. */
extern flag briefHtmlFlag;  /* Output statement only, for statement display
                in other HTML pages, such as the Proof Explorer home page */
extern long extHtmlStmt; /* At this statement and above, use the exthtmlxxx
    variables for title, links, etc.  This was put in to allow proper
    generation of the Hilbert Space Explorer extension to the set.mm
    database. */
extern vstring extHtmlTitle; /* Title of extended section if any; set by
    by exthtmltitle command in special $t comment of database source */

/* TeX word-processor-specific routines */
flag readTexDefs(vstring tex_def_fn, flag promptForFile);
                                                    /* Returns 1=OK, 0=error */
extern flag texDefsRead;
long texDefWhiteSpaceLen(char *ptr);
long texDefTokenLen(char *ptr);
/* Token comparison for qsort */
int texSortCmp(const void *key1, const void *key2);
/* Token comparison for bsearch */
int texSrchCmp(const void *key, const void *data);
/* Convert ascii to a string of \tt tex */
/* (The caller must surround it by {\tt }) */
vstring asciiToTt(vstring s);
vstring tokenToTex(vstring mtoken);
/* Converts a comment section in math mode to TeX.  Each math token
   MUST be separated by white space.   TeX "$" does not surround the output. */
vstring asciiMathToTex(vstring mathComment);
/* Gets the next section of a comment that is in the current mode (text,
   label, or math).  If 1st char. is not "$", text mode is assumed.
   mode = 0 means end of comment reached.  srcptr is left at 1st char.
   of start of next comment section. */
vstring getCommentModeSection(vstring *srcptr, char *mode);
void printTexHeader(flag texHeaderFlag);
/* Prints an embedded comment in TeX.  The commentPtr must point to the first
   character after the "$(" in the comment.  The printout ends when the first
   "$)" or null character is encountered.   commentPtr must not be a temporary
   allocation.  */
void printTexComment(vstring commentPtr);
void printTexLongMath(nmbrString *proofStep, vstring startPrefix,
    vstring contPrefix, long hypStmt);
void printTexTrailer(flag texHeaderFlag);

/* TeX symbol dictionary */
extern FILE *tex_dict_fp;     /* File pointers */
extern vstring tex_dict_fn;   /* File names */

/* TeX normal output */
extern flag texFileOpenFlag;
extern FILE *texFilePtr;


#define MAX_GRAPHIC_DEFS 1000 /* Maximum number of defined math tokens */
#define MAX_LINE_LENGTH 64 /* Maximum output file line length */

/* Storage of graphics for math tokens */
extern vstring graTokenName[MAX_GRAPHIC_DEFS]; /* ASCII abbreviation of token */
extern int graTokenLength[MAX_GRAPHIC_DEFS];    /* length of graTokenName */
extern vstring graTokenDef[MAX_GRAPHIC_DEFS]; /* Graphics definition */
extern int maxGraDef; /* Number of tokens defined so far */
extern int maxGraTokenLength; /* Largest graTokenName length */
