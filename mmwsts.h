/*****************************************************************************/
/*        Copyright (C) 2016  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMVMML_H_
#define METAMATH_MMVMML_H_

#include "mmvstr.h"
#include "mmdata.h"

/* Parse an STS file */
int parseSTSRules(vstring format);

/* Returns the HTML code for MathML, for the math string (hypothesis or
   conclusion) that is passed in. */
/* Warning: The caller must deallocate the returned vstring. */
flag getSTSLongMath(vstring *mmlLine, nmbrString *mathString, flag displayed,
		    long statemNum, flag textwarn);

/* Returns the code to be included in the HTML head element
 * for the given format */
vstring getSTSHeader();

/* Returns the token ID for a given token */
long tokenId(vstring token);

/* Returns the HTML code string  */
vstring stsToken(long tokenId, long stateNum);

/* Converts a string of ASCII text using STS. */
vstring asciiToMathSts(vstring string, long statemNum);

/* Check that there is an STS scheme for each syntax axiom. */
void verifySts(vstring format);

#endif /* METAMATH_MMVMML_H_ */
