/*****************************************************************************/
/*        Copyright (C) 2022  Wolf Lammen                                    */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file mmfatl.c a self-contained set of text printing routines using
 * dedicated pre-allocated memory, designed to likely run even under difficult
 * conditions (corrupt state, out of memory).
 */

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mmfatl.h"

//=================   Regression tests   =====================

#ifdef TEST_MMFATL

/*  automatic testing to prevent regression   */

bool testSuccessMessage(bool silent)
{
    if (!silent)
        printf("Regression tests in " __FILE__ " indicate no error\n\n");
    return true;
}

void test_mmfatl()
{
    bool silent = TEST_MMFATL_SILENT;

    bool ok = true
        && testSuccessMessage(silent);

    if (!ok)
        exit(EXIT_FAILURE);
}

#endif
