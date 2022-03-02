/*****************************************************************************/
/*        Copyright (C) 2018  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMCMDL_H_
#define METAMATH_MMCMDL_H_

/*!
 * \file mmcmdl.h - includes for accessing the command line interpreter
 */

#include "mmvstr.h"
#include "mmdata.h"

flag processCommandLine(void);
void parseCommandLine(vstring line);
flag lastArgMatches(vstring argString);
flag cmdMatches(vstring cmdString);
long switchPos(vstring swString);
void printCommandError(vstring line, long arg, vstring errorMsg);
void freeCommandLine(void);

#define DEFAULT_COLUMN 16
extern pntrString *g_rawArgPntr;
extern nmbrString *g_rawArgNmbr;
extern long g_rawArgs;
extern pntrString *g_fullArg;
extern vstring g_fullArgString; /* g_fullArg as one string */
/*!
 * \var vstring g_commandPrompt
 * text displayed at the beginning of the line where a user is supposed to
 * enter a new command.  For example 'MM>'.
 */
extern vstring g_commandPrompt;
extern vstring g_commandLine;
extern long g_showStatement;
extern vstring g_logFileName;
extern vstring g_texFileName;
extern flag g_PFASmode; /* Proof assistant mode, invoked by PROVE command */
extern flag g_sourceChanged; /* Flag that user made some change to the source file*/
extern flag g_proofChanged; /* Flag that user made some change to proof in progress*/
extern flag g_commandEcho; /* Echo full command */
/*!
 * \brief indicates whether the user has turned MEMORY STATUS on.
 *
 * If the user issues SET MEMORY_STATUS ON this \a flag is set to 1.  It is
 * reset to 0 again on a SET MEMORY_STATUS OFF command.  When 1, certain
 * memory de/allocations are monitored - see \a db3.
 */
extern flag g_memoryStatus; /* Always show memory */

extern flag g_sourceHasBeenRead; /* 1 if a source file has been read in */
extern vstring g_rootDirectory; /* Directory to use for included files */


#endif /* METAMATH_MMCMDL_H_ */
