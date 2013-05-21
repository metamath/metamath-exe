/*****************************************************************************/
/*        Copyright (C) 2012  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* Colors for HTML pages. */
#define GREEN_TITLE_COLOR "\"#006633\""
#define MINT_BACKGROUND_COLOR "\"#EEFFFA\""
#define PINK_NUMBER_COLOR "\"#FA8072\""      /* =salmon; was FF6666 */
#define PURPLISH_BIBLIO_COLOR "\"#FAEEFF\""


/* 29-Jul-2008 nm Sandbox stuff */
#define SANDBOX_COLOR "\"#FFFFD9\""

/* TeX flags */
/* 14-Sep-2010 nm Removed simpleTexFlag; added oldTexFlag */
extern flag oldTexFlag; /* Use macros in output; obsolete; take out someday */

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
extern vstring htmlVarColors; /* Set by htmlvarcolor commands */
/* Added 10/13/02 for use in metamath.c bibliography cross-reference */
extern vstring htmlBibliography; /* Optional; set by htmlbibliography command */
extern vstring extHtmlBibliography; /* Optional; set by exthtmlbibliography
                                       command */

/* 29-Jul-2008 nm Sandbox stuff */
extern long sandboxStmt; /* At this statement and above, use sandbox stuff */

/* TeX word-processor-specific routines */
flag readTexDefs(void);
                                                    /* Returns 1=OK, 0=error */
extern flag texDefsRead;
struct texDef_struct {  /* 27-Oct-2012 nm Made global for "erase" */
  vstring tokenName; /* ASCII token */
  vstring texEquiv; /* Converted to TeX */
};
extern struct texDef_struct *texDefs; /* 27-Oct-2012 nm Now glob for "erase" */


long texDefWhiteSpaceLen(char *ptr);
long texDefTokenLen(char *ptr);
/* Token comparison for qsort */
int texSortCmp(const void *key1, const void *key2);
/* Token comparison for bsearch */
int texSrchCmp(const void *key, const void *data);
/* Convert ascii to a string of \tt tex */
/* (The caller must surround it by {\tt }) */
vstring asciiToTt(vstring s);
vstring tokenToTex(vstring mtoken, long statemNum);
/* Converts a comment section in math mode to TeX.  Each math token
   MUST be separated by white space.   TeX "$" does not surround the output. */
vstring asciiMathToTex(vstring mathComment, long statemNum);
/* Gets the next section of a comment that is in the current mode (text,
   label, or math).  If 1st char. is not "$", text mode is assumed.
   mode = 0 means end of comment reached.  srcptr is left at 1st char.
   of start of next comment section. */
vstring getCommentModeSection(vstring *srcptr, char *mode);
void printTexHeader(flag texHeaderFlag);
/* Prints an embedded comment in TeX.  The commentPtr must point to the first
   character after the "$(" in the comment.  The printout ends when the first
   "$)" or null character is encountered.   commentPtr must not be a temporary
   allocation.  htmlCenterFlag, if 1, means to center the HTML and add a
   "Description:" prefix.*/
void printTexComment(vstring commentPtr, char htmlCenterFlag);
void printTexLongMath(nmbrString *proofStep, vstring startPrefix,
    vstring contPrefix, long hypStmt, long indentationLevel);
void printTexTrailer(flag texHeaderFlag);

/* Added 4-Dec-03
   Function implementing WRITE THEOREM_LIST / THEOREMS_PER_PAGE nn */
void writeTheoremList(long theoremsPerPage, flag showLemmas);

/* 2-Aug-2009 nm - broke this function out from writeTheoremList() */
void getSectionHeadings(long stmt, vstring *bigHdrAddr,
    vstring *smallHdrAddr);

/* TeX symbol dictionary */
extern FILE *tex_dict_fp;     /* File pointers */
extern vstring tex_dict_fn;   /* File names */

/* TeX normal output */
extern flag texFileOpenFlag;
extern FILE *texFilePtr;

/* Pink statement number for HTML pages */
/* 10/10/02 (This is no longer used?) */
/*
long pinkNumber(long statemNum);
*/

/* Pink statement number HTML code for HTML pages - added 10/10/02 */
/* Warning: caller must deallocate returned string */
vstring pinkHTML(long statemNum);

/* Pink statement number range HTML code for HTML pages, separated by a
   "-" - added 24-Aug-04 */
/* Warning: caller must deallocate returned string */
vstring pinkRangeHTML(long statemNum1, long statemNum2);

#define PINK_NBSP "&nbsp;" /* Either "" or "&nbsp;" depending on taste, it is
                  the separator between a statement href and its pink number */

/* 30-Jan-04 nm Comment out the following line to go back to the pink-only
   color for the little statement numbers on the HTML pages */
#define RAINBOW_OPTION /* "Rainbow" instead of pink color for little numbers */

#ifdef RAINBOW_OPTION
/* This function converts a "spectrum" color (1 to maxColor) to an
   RBG value in hex notation for HTML.  The caller must deallocate the
   returned vstring.  color = 1 (red) to maxColor (violet). */
/* ndm 10-Jan-04 */
vstring spectrumToRGB(long color, long maxColor);
#endif

#define INDENT_HTML_PROOFS /* nm 3-Feb-04 - indentation experiment */

/* Added 20-Sep-03 (broken out of printTexLongMath() for better
   modularization) */
/* Returns the HTML code for GIFs (!altHtmlFlag) or Unicode (altHtmlFlag),
   or LaTeX when !htmlFlag, for the math string (hypothesis or conclusion) that
   is passed in. */
/* Warning: The caller must deallocate the returned vstring. */
vstring getTexLongMath(nmbrString *mathString, long statemNum);

/* Added 18-Sep-03 (transferred from metamath.c) */
/* Returns the TeX, or HTML code for GIFs (!altHtmlFlag) or Unicode
   (altHtmlFlag), for a statement's hypotheses and assertion in the form
   hyp & ... & hyp => assertion */
/* Warning: The caller must deallocate the returned vstring. */
/* 14-Sep-2010 nm Changed name from getHTMLHypAndAssertion() */
vstring getTexOrHtmlHypAndAssertion(long statemNum);
