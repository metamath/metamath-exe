/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMWTEX_H_
#define METAMATH_MMWTEX_H_

#include "mmvstr.h"
#include "mmdata.h"

/* Colors for HTML pages. */
#define GREEN_TITLE_COLOR "\"#006633\""
#define MINT_BACKGROUND_COLOR "\"#EEFFFA\""
#define PINK_NUMBER_COLOR "\"#FA8072\""      /* =salmon; was FF6666 */
#define PURPLISH_BIBLIO_COLOR "\"#FAEEFF\""


/* 29-Jul-2008 nm Sandbox stuff */
#define SANDBOX_COLOR "\"#FFFFD9\""

/* TeX flags */
/* 14-Sep-2010 nm Removed simpleTexFlag; added g_oldTexFlag */
extern flag g_oldTexFlag; /* Use macros in output; obsolete; take out someday */

/* HTML flags */
extern flag g_htmlFlag;  /* HTML flag: 0 = TeX, 1 = HTML */
extern flag g_altHtmlFlag;  /* Use "althtmldef" instead of "htmldef".  This is
    intended to allow the generation of pages with the old Symbol font
    instead of the individual GIF files. */
extern flag g_briefHtmlFlag;  /* Output statement only, for statement display
                in other HTML pages, such as the Proof Explorer home page */
extern long g_extHtmlStmt; /* At this statement and above, use the exthtmlxxx
    variables for title, links, etc.  This was put in to allow proper
    generation of the Hilbert Space Explorer extension to the set.mm
    database. */
extern vstring g_extHtmlTitle; /* Title of extended section if any; set by
    by exthtmltitle command in special $t comment of database source */
extern vstring g_htmlVarColor; /* Set by htmlvarcolor commands */
/* Added 26-Aug-2017 nm for use by mmcmds.c */
extern vstring g_htmlHome; /* Set by htmlhome command */
/* Added 10/13/02 for use in metamath.c bibliography cross-reference */
extern vstring g_htmlBibliography; /* Optional; set by htmlbibliography command */
extern vstring g_extHtmlBibliography; /* Optional; set by exthtmlbibliography
                                       command */
extern vstring g_htmlCSS; /* Set by htmlcss commands */  /* 14-Jan-2016 nm */
/* Added 14-Jan-2016 */
extern vstring g_htmlFont; /* Optional; set by g_htmlFont command */

void eraseTexDefs(void); /* Undo readTexDefs() */

/* TeX/HTML/ALT_HTML word-processor-specific routines */
/* Returns 2 if there were severe parsing errors, 1 if there were warnings but
   no errors, 0 if no errors or warnings */
flag readTexDefs(
  flag errorsOnly,  /* 1 = supprees non-error messages */
  flag noGifCheck   /* 1 = don't check for missing GIFs */);

extern flag g_texDefsRead;
struct texDef_struct {  /* 27-Oct-2012 nm Made global for "erase" */
  vstring tokenName; /* ASCII token */
  vstring texEquiv; /* Converted to TeX */
};
extern struct texDef_struct *g_TexDefs; /* 27-Oct-2012 nm Now glob for "erase" */


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
/* void printTexComment(vstring commentPtr, char htmlCenterFlag); */
/* 17-Nov-2015 nm Added 3rd & 4th arguments; returns 1 if error/warning */
flag printTexComment(vstring commentPtr,    /* Sends result to g_texFilePtr */
    flag htmlCenterFlag, /* 1 = htmlCenterFlag */
    long actionBits, /* see indicators below */
    flag noFileCheck /* 1 = noFileCheck */);
/* Indicators for actionBits */
#define ERRORS_ONLY 1
#define PROCESS_SYMBOLS 2
#define PROCESS_LABELS 4
#define ADD_COLORED_LABEL_NUMBER 8
#define PROCESS_BIBREFS 16
#define PROCESS_UNDERSCORES 32
/* CONVERT_TO_HTML means '<' to '&lt;'; unless <HTML> in comment (and strip it) */
#define CONVERT_TO_HTML 64
/* METAMATH_COMMENT means $) (as well as end-of-string) terminates string. */
#define METAMATH_COMMENT 128
/* PROCESS_ALL is for convenience */
#define PROCESS_EVERYTHING PROCESS_SYMBOLS + PROCESS_LABELS \
     + ADD_COLORED_LABEL_NUMBER + PROCESS_BIBREFS \
     + PROCESS_UNDERSCORES + CONVERT_TO_HTML + METAMATH_COMMENT

void printTexLongMath(nmbrString *proofStep, vstring startPrefix,
    vstring contPrefix, long hypStmt, long indentationLevel);
void printTexTrailer(flag texHeaderFlag);

/* Added 4-Dec-03
   Function implementing WRITE THEOREM_LIST / THEOREMS_PER_PAGE nn */
void writeTheoremList(long theoremsPerPage, flag showLemmas,
    flag noVersioning);

#define HUGE_DECORATION "####"
#define BIG_DECORATION "#*#*"
#define SMALL_DECORATION "=-=-"
#define TINY_DECORATION "-.-."

/* 2-Aug-2009 nm - broke this function out from writeTheoremList() */
/* 20-Jun-2014 nm - added hugeHdrAddr */
/* 21-Aug-2017 nm - added tinyHdrAddr */
/* 6-Aug-2019 nm - changed return type from void to flag (=char) */
/* 24-Aug-2020 nm - added fineResolution */
/* 12-Sep-2020 nm - added fullComment */
flag getSectionHeadings(long stmt, vstring *hugeHdrTitle,
    vstring *bigHdrTitle,
    vstring *smallHdrTitle,
    vstring *tinyHdrTitle,
    /* Added 8-May-2015 nm */
    vstring *hugeHdrComment,
    vstring *bigHdrComment,
    vstring *smallHdrComment,
    vstring *tinyHdrComment,
    /* Added 24-Aug-2020 nm */
    flag fineResolution, /* 0 = consider just successive $a/$p, 1 = all stmts */
    flag fullComment /* 1 = put $( + header + comment + $) into xxxHdrTitle */
    );

/****** 15-Aug-2020 nm Obsolete
/@ TeX symbol dictionary @/
extern FILE @g_tex_dict_fp;     /@ File pointers @/
extern vstring g_tex_dict_fn;   /@ File names @/
******/

/* TeX normal output */
extern flag g_texFileOpenFlag;
extern FILE *g_texFilePtr;

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

/* This function converts a "spectrum" color (1 to maxColor) to an
   RBG value in hex notation for HTML.  The caller must deallocate the
   returned vstring.  color = 1 (red) to maxColor (violet). */
/* ndm 10-Jan-04 */
vstring spectrumToRGB(long color, long maxColor);

/* Added 20-Sep-03 (broken out of printTexLongMath() for better
   modularization) */
/* Returns the HTML code for GIFs (!g_altHtmlFlag) or Unicode (g_altHtmlFlag),
   or LaTeX when !g_htmlFlag, for the math string (hypothesis or conclusion) that
   is passed in. */
/* Warning: The caller must deallocate the returned vstring. */
vstring getTexLongMath(nmbrString *mathString, long statemNum);

/* Added 18-Sep-03 (transferred from metamath.c) */
/* Returns the TeX, or HTML code for GIFs (!g_altHtmlFlag) or Unicode
   (g_altHtmlFlag), for a statement's hypotheses and assertion in the form
   hyp & ... & hyp => assertion */
/* Warning: The caller must deallocate the returned vstring. */
/* 14-Sep-2010 nm Changed name from getHTMLHypAndAssertion() */
vstring getTexOrHtmlHypAndAssertion(long statemNum);

/* Added 17-Nov-2015 (broken out of metamath.c for better modularization) */
/* For WRITE BIBLIOGRAPHY command and error checking by VERIFY MARKUP */
/* Returns 0 if OK, 1 if error or warning found */
flag writeBibliography(vstring bibFile,
    vstring labelMatch, /* Normally "*" except by verifyMarkup() */
    flag errorsOnly,  /* 1 = no output, just warning msgs if any */
    flag noFileCheck); /* 1 = ignore missing external files (gifs, bib, etc.) */

/* 5-Aug-2020 nm */
/* Globals to hold mathbox information.  They should be re-initialized
   by the ERASE command (eraseSource()).  g_mathboxStmt = 0 indicates
   it and the other variables haven't been initialized. */
extern long g_mathboxStmt; /* stmt# of "mathbox" label; statements+1 if none */
extern long g_mathboxes; /* # of mathboxes */
/* The following 3 "strings" are 0-based e.g. g_mathboxStart[0] is for
   mathbox #1 */
extern nmbrString *g_mathboxStart; /* Start stmt vs. mathbox # */
extern nmbrString *g_mathboxEnd; /* End stmt vs. mathbox # */
extern pntrString *g_mathboxUser; /* User name vs. mathbox # */

/* 5-Aug-2020 nm */
/* Returns 1 if statements are in different mathboxes */
flag inDiffMathboxes(long stmt1, long stmt2);
/* Returns the user of the mathbox that a statement is in, or ""
   if the statement is not in a mathbox. */
/* Caller should NOT deallocate returned string (it points directly to
   g_mathboxUser[] entry); use directly in print2() messages */
vstring getMathboxUser(long stmt);
/* Returns the mathbox number (starting at 1) that stmt is in, or 0 if not
   in a mathbox */
long getMathboxNum(long stmt);
/* Populates mathbox information */
void assignMathboxInfo(void);
/* Creates lists of mathbox starts and user names */
long getMathboxLoc(nmbrString **mathboxStart, nmbrString **mathboxEnd,
    pntrString **mathboxUser);

#endif /* METAMATH_MMWTEX_H_ */
