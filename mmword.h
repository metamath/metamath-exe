/*****************************************************************************/
/*               Copyright (C) 1998, NORMAN D. MEGILL                        */
/*****************************************************************************/

/*34567890123456 (79-character line to adjust text window width) 678901234567*/

/* Tag file changes with revision number tags */
void revise(FILE *f1_fp, FILE *f2_fp, FILE *f3_fp, vstring addTag, long m);


/* Get the largest revision number tag in a file */
/* Tags are assumed to be of format nn or #nn in comment at end of line */
/* Used to determine default argument for tag question */
long highestRevision(vstring fileName);


/* Get numeric revision from the tag on a line (returns 0 if none) */
/* Tags are assumed to be of format nn or #nn in comment at end of line */
long getRevision(vstring line);
