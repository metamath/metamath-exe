/*****************************************************************************/
/*        Copyright (C) 2012  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMWORD_H_
#define METAMATH_MMWORD_H_

/*! \file */

#include "mmvstr.h"

/*! Tag file changes with revision number tags */
void revise(FILE *f1_fp, FILE *f2_fp, FILE *f3_fp, vstring addTag, long m);

/*! \brief Get the largest revision number tag in a file

   Tags are assumed to be of format nn or #nn in comment at end of line.
   Used to determine default argument for tag question. */
long highestRevision(vstring fileName);

/*! \brief Get numeric revision from the tag on a line (returns 0 if none)

   Tags are assumed to be of format nn or #nn in comment at end of line */
long getRevision(vstring line);

#endif // METAMATH_MMWORD_H_
