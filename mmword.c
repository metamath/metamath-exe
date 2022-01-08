/*****************************************************************************/
/*        Copyright (C) 2012  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* This file implements the UPDATE command of the TOOLS utility.  revise() is
   the main external call; see the comments preceding it. */

/* The UPDATE command of TOOLS (mmword.c) was custom-written in accordance
   with the version control requirements of a company that used it.  It
   documents the differences between two versions of a program as C-style
   comments embedded in the newer version.  The best way to determine whether
   it suits your similar needs is just to run it and look at its output. */

/* Very old history: this was called mmword.c because it was intended to
   produce RTF output for Word, analogous to the existing LaTeX output.
   Microsoft never responded to a request for the RTF specification, as they
   promised in the circa 1990 manual accompanying Word.  Thus it remained an
   empty shell.  When the need for UPDATE arose, the mmword.c shell was used in
   order to avoid the nuisance of changing some compilation setups and scripts
   existing at that time. */

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <setjmp.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmword.h"

/* Set to 79, 80, etc. - length of line after tag is added */
#define LINE_LENGTH 80


   /* 7000 ! ***** "DIFF" Option ***** from DO LIST */
/*  gosub_7000(f1_name, f2_name, f3_name, &f3_fp, m); */


char strcmpe(vstring s1, vstring s2);
vstring stripAndTag(vstring line, vstring tag, flag tagBlankLines);

long r0,r1,r2,i0,i1_,i2_,d,t,i9;
FILE *f1_fp_;
FILE *f2_fp_;
FILE *f3_fp_;
char eof1, eof2;
vstring ctlz_ = "";
vstring l1_ = "";
vstring l2_ = "";
vstring tmp_ = "";
vstring tmpLine = "";
vstring addTag_ = "";
vstring delStartTag_ = "";
vstring delEndTag_ = "";
flag printedAtLeastOne;
     /*  Declare input and save buffers */
#define MAX_LINES 10000
#define MAX_BUF 1000
     vstring line1_[MAX_LINES];
     vstring line2_[MAX_LINES];
     vstring reserve1_[MAX_BUF];
     vstring reserve2_[MAX_BUF];


/* revise() is called by the UPDATE command of TOOLs.  The idea is to
   keep all past history of a file in the file itself, in the form of
   comments.  In mmcmds.c, see the parsing of the UPDATE command for a
   partial explanation of its arguments.  UPDATE was written for a
   proprietary language with C-style comments (where nested comments were
   allowed) and it may not be generally useful without some modification. */
void revise(FILE *f1_fp, FILE *f2_fp, FILE *f3_fp, vstring addTag, long m)
{
  /******** Figure out the differences (DO LIST subroutine) ******/
  vstring blanksPrefix = "";
  long tmpi;
  long i, j;

  f1_fp_ = f1_fp;
  f2_fp_ = f2_fp;
  f3_fp_ = f3_fp;
  let(&addTag_, addTag);
  let(&delStartTag_, "/******* Start of deleted section *******");
  let(&delEndTag_, "******* End of deleted section *******/");


  /* Initialize vstring arrays */
  for (i = 0; i < MAX_LINES; i++) {
    line1_[i] = "";
    line2_[i] = "";
  }
  for (i = 0; i < MAX_BUF; i++) {
    reserve1_[i] = "";
    reserve2_[i] = "";
  }

  if (m < 1) m = 1;

  r0=r1=r2=i0=i1_=i2_=d=t=i=j=i9=0;


  let(&ctlz_,chr(26));  /* End-of-file character */

  let(&l1_,ctlz_);
  let(&l2_,ctlz_);
  eof1=eof2=0;  /* End-of-file flags */
  d=0;

l7100:  /*
          Lines 7100 through 7300 are a modified version of the compare loop
          In DEC's FILCOM.BAS program.
        */

        if (!strcmpe(l1_,l2_)) { /* The lines are the same */
                if (strcmpe(l2_,ctlz_)) {
                  fprintf(f3_fp_, "%s\n", l2_); /* Use edited version
                                of line when they are the same */
                }
                gosub_7320();
                gosub_7330();
                if (strcmpe(l1_,ctlz_) || strcmpe(l2_,ctlz_) ) {
                        goto l7100;
                } else {
                        fclose(f1_fp_);
                        fclose(f2_fp_);
                        fclose(f3_fp_);

                      /* Deallocate string memory */
                      for (i = 0; i < MAX_LINES; i++) {
                        let(&(line1_[i]), "");
                        let(&(line2_[i]), "");
                      }
                      for (i = 0; i < MAX_BUF; i++) {
                        let(&(reserve1_[i]), "");
                        let(&(reserve2_[i]), "");
                      }
                      let(&ctlz_, "");
                      let(&l1_, "");
                      let(&l2_, "");
                      let(&tmp_, "");
                      let(&tmpLine, "");
                      let(&addTag_, "");
                      let(&delStartTag_, "");
                      let(&delEndTag_, "");
                      let(&blanksPrefix, "");

                        return;
                }
        }
        d=d+1;  /* Number of difference sections found so far
                         (For user information only) */
        i1_=i2_=m-1;
        let(&line1_[0],l1_);
        let(&line2_[0],l2_);
        for (i0 = 1; i0 < m; i0++) {
          gosub_7320();
          let(&line1_[i0],l1_);
        }
        for (i0 = 1; i0 < m; i0++) {
          gosub_7330();
          let(&line2_[i0],l2_);
        }
l7130:  gosub_7320();
        i1_=i1_+1;
        if (i1_ >= MAX_LINES) {
          printf("*** FATAL *** Overflow#1\n");
#if __STDC__
          fflush(stdout);
#endif
          exit(0);
        }
        let(&line1_[i1_],l1_);
        t=0;
        i=0;
l7140:  if (strcmpe(line1_[i1_+t-m+1], line2_[i+t])) {
                t=0;
        } else {

                /* If lines "match", ensure we use the EDITED version for the
                   final output */
                let(&line1_[i1_+t-m+1], line2_[i+t]);

                t=t+1;
                if (t==m) {
                        goto l7200;
                } else {
                        goto l7140;
                }
        }

        i=i+1;
        if (i<=i2_-m+1) {
                goto l7140;
        }
        gosub_7330();
        i2_=i2_+1;
        if (i2_ >= MAX_LINES) {
          printf("*** FATAL *** Overflow#2\n");
#if __STDC__
          fflush(stdout);
#endif
          exit(0);
        }
        let(&line2_[i2_],l2_);
        t=0;
        i=0;
l7170:
        if (strcmpe(line1_[i+t], line2_[i2_+t-m+1])) {
                t=0;
        } else {

                /* If lines "match", ensure we use the EDITED version for the
                   final output */
                let(&line1_[i+t], line2_[i2_+t-m+1]);

                t=t+1;
                if (t==m) {
                        goto l7220;
                } else {
                        goto l7170;
                }
        }
        i=i+1;
        if (i<=i1_-m+1) {
                goto l7170;
        }
        goto l7130;
l7200:  i=i+m-1;
        if (r2) {
          for (j=r2-1; j>=0; j--) {
                let(&reserve2_[j+i2_-i],reserve2_[j]);
          }
        }
        for (j=1; j<=i2_-i; j++) {
          let(&reserve2_[j-1],line2_[j+i]);
        }
        r2=r2+i2_-i;
        if (r2 >= MAX_BUF) {
          printf("*** FATAL *** Overflow#3\n");
#if __STDC__
          fflush(stdout);
#endif
          exit(0);
        }
        i2_=i;
        goto l7240;
l7220:  i=i+m-1;
        if (r1) {
          for (j=r1-1; j>=0; j--) {
                let(&reserve1_[j+i1_-i],reserve1_[j]);
          }
        }

        for (j=1; j<=i1_-i; j++) {
          let(&reserve1_[j-1],line1_[j+i]);
        }
        r1=r1+i1_-i;
        if (r1 >= MAX_BUF) {
          printf("*** FATAL *** Overflow#4\n");
#if __STDC__
          fflush(stdout);
#endif
          exit(0);
        }
        i1_=i;
        goto l7240;
l7240: /* */

       printedAtLeastOne = 0;
       for (i=0; i<=i1_-m; i++) {
         if (strcmpe(line1_[i],ctlz_)) {
           if (!printedAtLeastOne) {
             printedAtLeastOne = 1;

             /* Put out any blank lines before delStartTag_ */
             while (((vstring)(line1_[i]))[0] == '\n') {
               fprintf(f3_fp_, "\n");
               let(&(line1_[i]), right(line1_[i], 2));
             }

             /* Find the beginning blank space */
             tmpi = 0;
             while (((vstring)(line1_[i]))[tmpi] == ' ') tmpi++;
             let(&blanksPrefix, space(tmpi));
             let(&tmpLine, "");
             tmpLine = stripAndTag(cat(blanksPrefix, delStartTag_, NULL),
                 addTag_, 0);
             fprintf(f3_fp_, "%s\n", tmpLine);
           }
           fprintf(f3_fp_, "%s\n", line1_[i]);
                                     /* Output original deleted lines */
           /*let(&tmp_, "");*/ /* Clear vstring stack */
         }
       }
       if (printedAtLeastOne) {
         let(&tmpLine, "");
         tmpLine = stripAndTag(cat(blanksPrefix, delEndTag_, NULL), addTag_
             ,0);
         fprintf(f3_fp_, "%s\n", tmpLine);
       }
       for (i=0; i<=i1_-m; i++) {
         if (i<=i2_-m) {
           if (strcmpe(line2_[i],ctlz_)) {
             let(&tmpLine, "");
             if (i == 0) {
               tmpLine = stripAndTag(line2_[i], addTag_, 0);
             } else {
               /* Put tags on blank lines *inside* of new section */
               tmpLine = stripAndTag(line2_[i], addTag_, 1);
             }
             fprintf(f3_fp_, "%s\n", tmpLine);
                                     /* Output tagged edited lines */
             /*let(&tmp_, "");*/ /* Clear vstring stack */
           }
         }
       }
       for (i=i1_-m+1; i<=i2_-m; i++) {
         if (strcmpe(line2_[i],ctlz_)) {
           let(&tmpLine, "");
           if (i == 0) {
             tmpLine = stripAndTag(line2_[i], addTag_, 0);
           } else {
             /* Put tags on blank lines *inside* of new section */
             tmpLine = stripAndTag(line2_[i], addTag_, 1);
           }
           fprintf(f3_fp_, "%s\n", tmpLine);
                                     /* Print remaining edited lines */
           /*let(&tmp_, "");*/ /* Clear vstring stack */
         }
       }
       for (i=0; i<=m-1; i++) {
         let(&l1_,line1_[i1_-m+1+i]);
         if (strcmpe(l1_,ctlz_)) {
           fprintf(f3_fp_,"%s\n",l1_);  /*  Print remaining matching lines */
         }
       }

       let(&l1_,ctlz_);
       let(&l2_,ctlz_);
       goto l7100;

}



void gosub_7320()
{
        /* Subroutine:  get next L1_ from original file */
  vstring tmpLin = "";
  if (r1) {     /*  Get next line from save array */
    let(&l1_,reserve1_[0]);
    r1=r1-1;
    for (i9=0; i9<=r1-1; i9++) {
      let(&reserve1_[i9],reserve1_[i9+1]);
    }
  } else {              /* Get next line from input file */
    if (eof1) {
      let(&l1_,ctlz_);
    } else {
     next_l1:
      if (!linput(f1_fp_,NULL,&l1_)) { /*linput returns 0 if EOF */
        eof1 = 1;
        /* Note that we will discard any blank lines before EOF; this
           should be OK though */
        let(&l1_,ctlz_);
        let(&tmpLin, ""); /* Deallocate */
        return;
      }
      let(&l1_, edit(l1_, 4 + 128 + 2048)); /* Trim garb, trail space; untab */
      if (!l1_[0]) { /* Ignore blank lines for comparison */
        let(&tmpLin, cat(tmpLin, "\n", NULL)); /* Blank line */
        goto next_l1;
      }
    }
  }
  let(&l1_, cat(tmpLin, l1_, NULL)); /* Add any blank lines */
  let(&tmpLin, ""); /* Deallocate */
  return;
}

void gosub_7330() {
        /*  Subroutine:  get next L2_ from edited file */
  vstring tmpLin = "";
  vstring tmpStrPtr; /* pointer only */
  flag stripDeletedSectionMode;
  if (r2) {     /*  Get next line from save array */
    let(&l2_,reserve2_[0]);
    r2=r2-1;
    for (i9 = 0; i9 < r2; i9++) {
      let(&reserve2_[i9],reserve2_[i9+1]);
    }
  } else {              /*  Get next line from input file */
    if (eof2) {
      let(&l2_,ctlz_);
    } else {
     stripDeletedSectionMode = 0;
     next_l2:
      if (!linput(f2_fp_,NULL,&l2_)) { /* linput returns 0 if EOF */
        eof2 = 1;
        /* Note that we will discard any blank lines before EOF; this
           should be OK though */
        let(&l2_, ctlz_);
        let(&tmpLin, ""); /* Deallocate */
        return;
      }
      let(&l2_, edit(l2_, 4 + 128 + 2048)); /* Trim garb, trail space; untab */
      if (!strcmp(edit(delStartTag_, 2), left(edit(l2_, 2 + 4),
          (long)strlen(edit(delStartTag_, 2))))) {
        if (getRevision(l2_) == getRevision(addTag_)) {
          /* We should strip out deleted section from previous run */
          /* (The diff algorithm will put them back from orig. file) */
          stripDeletedSectionMode = 1;
          goto next_l2;
        }
      }
      if (stripDeletedSectionMode) {
        if (!strcmp(edit(delEndTag_, 2), left(edit(l2_, 2 + 4),
            (long)strlen(edit(delEndTag_, 2))))  &&
            getRevision(l2_) == getRevision(addTag_) ) {
          stripDeletedSectionMode = 0;
        }
        goto next_l2;
      }

      /* Strip off tags that match *this* revision (so previous out-of-sync
         runs will be corrected) */
      if (getRevision(l2_) == getRevision(addTag_)) {
        tmpStrPtr = l2_;
        l2_ = stripAndTag(l2_, "", 0);
        let(&tmpStrPtr, ""); /* deallocate old l2_ */
      }

      if (!l2_[0]) { /* Ignore blank lines for comparison */
        let(&tmpLin, cat(tmpLin, "\n", NULL)); /* Blank line */
        goto next_l2;
      }
    }
  }
  let(&l2_, cat(tmpLin, l2_, NULL)); /* Add any blank lines */
  let(&tmpLin, ""); /* Deallocate */
  return;

}


/* Return 0 if difference lines are the same, non-zero otherwise */
char strcmpe(vstring s1, vstring s2)
{
  flag cmpflag;

  /* Option flags - make global if we want to use them */
  flag ignoreSpaces = 1;
  flag ignoreSameLineComments = 1;

  vstring tmps1 = "";
  vstring tmps2 = "";
  long i;
  long i2;
  long i3;
  let(&tmps1, s1);
  let(&tmps2, s2);

  if (ignoreSpaces) {
    let(&tmps1, edit(tmps1, 2 + 4));
    let(&tmps2, edit(tmps2, 2 + 4));
  }

  if (ignoreSameLineComments) {
    while (1) {
      i = instr(1, tmps1, "/*");
      if (i == 0) break;
      i2 = instr(i + 2, tmps1, "*/");
      if (i2 == 0) break;
      i3 = instr(i + 2, tmps1, "/*");
      if (i3 != 0 && i3 < i2) break; /*i = i3;*/ /* Nested comment */
      if (i2 - i > 7) break; /* only ignore short comments (tags) */
      let(&tmps1, cat(left(tmps1, i - 1), right(tmps1, i2 + 2), NULL));
    }
    while (1) {
      i = instr(1, tmps2, "/*");
      if (i == 0) break;
      i2 = instr(i + 2, tmps2, "*/");
      if (i2 == 0) break;
      i3 = instr(i + 2, tmps2, "/*");
      if (i3 != 0 && i3 < i2) break; /*i = i3;*/ /* Nested comment */
      if (i2 - i > 7) break; /* only ignore short comments (tags) */
      let(&tmps2, cat(left(tmps2, i - 1), right(tmps2, i2 + 2), NULL));
    }
  }

  cmpflag = !!strcmp(tmps1, tmps2);
  let(&tmps1, ""); /* Deallocate string */
  let(&tmps2, ""); /* Deallocate string */
  return (cmpflag);
}


/* Strip any old tag from line and put new tag on it */
/* (Caller must deallocate returned string) */
vstring stripAndTag(vstring line, vstring tag, flag tagBlankLines)
{
  long i, j, k, n;
  vstring line1 = "", comment = "";
  long lineLength = LINE_LENGTH;
  flag validTag;
  i = 0;
  let(&line1, edit(line, 128 + 2048)); /* Trim trailing spaces and untab */
  /* Get last comment on line */
  while (1) {
    j = instr(i + 1, line1, "/*");
    if (j == 0) break;
    i = j;
  }
  j = instr(i, line1, "*/");
  if (i && j == (signed)(strlen(line1)) - 1) {
    let(&comment, seg(line1, i + 2, j - 1));
    validTag = 1;
    for (k = 0; k < (signed)(strlen(comment)); k++) {
      /* Check for valid characters that can appear in a tag */
      if (instr(1, " 1234567890#", mid(comment, k + 1, 1))) continue;
      validTag = 0;
      break;
    }
    if (validTag) let(&line1, edit(left(line1, i - 1), 128));
    let(&comment, ""); /* deallocate */
  }

  /* Count blank lines concatenated to the beginning of this line */
  n = 0;
  while (line1[n] == '\n') n++;

  /* Add the tag */
  if (tag[0]) { /* Non-blank tag */
    if ((long)strlen(line1) - n < lineLength - 1 - (long)strlen(tag))
      let(&line1, cat(line1,
          space(lineLength - 1 - (long)strlen(tag) - (long)strlen(line1) + n),
          NULL));
    let(&line1, cat(line1, " ", tag, NULL));
    if ((signed)(strlen(line1)) - n > lineLength) {
      print2(
"Warning: The following line has > %ld characters after tag is added:\n",
          lineLength);
      print2("%s\n", line1);
    }
  }

  /* Add tags to blank lines if tagBlankLines is set */
  /* (Used for blank lines inside of new edited file sections) */
  if (tagBlankLines && n > 0) {
    let(&line1, right(line1, n + 1));
    for (i = 1; i <= n; i++) {
      let(&line1, cat(space(lineLength - (long)strlen(tag)), tag, "\n",
          line1, NULL));
    }
  }

  return line1;
}

/* Get the largest revision number tag in a file */
/* Tags are assumed to be of format nn or #nn in comment at end of line */
/* Used to determine default argument for tag question */
long highestRevision(vstring fileName)
{
  vstring str1 = "";
  long revision;
  long largest = 0;
  FILE *fp;

  fp = fopen(fileName, "r");
  if (!fp) return 0;
  while (linput(fp, NULL, &str1)) {
    revision = getRevision(str1);
    if (revision > largest) largest = revision;
  }
  let(&str1, "");
  fclose(fp);
  return largest;
}

/* Get numeric revision from the tag on a line (returns 0 if none) */
/* Tags are assumed to be of format nn or #nn in comment at end of line */
long getRevision(vstring line)
{
  vstring str1 = "";
  vstring str2 = "";
  vstring tag = "";
  long revision;

  if (instr(1, line, "/*") == 0) return 0; /* Speedup - no comment in line */
  let(&str1, edit(line, 2)); /* This line has the tag not stripped */
  let(&str2, "");
  str2 = stripAndTag(str1, "", 0); /* Strip old tag & add dummy new one */
  let(&str2, edit(str2, 2)); /* This line has the tag stripped */
  if (!strcmp(str1, str2)) {
    revision = 0;  /* No tag */
  } else {
    let(&tag, edit(seg(str1, (long)strlen(str2) + 3,
        (long)strlen(str1) - 2), 2));
    if (tag[0] == '#') let(&tag, right(tag, 2)); /* Remove any # */
    revision = (long)(val(tag));
  }
  let(&tag, "");
  let(&str1, "");
  let(&str2, "");
  return revision;
}

