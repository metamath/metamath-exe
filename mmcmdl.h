/*****************************************************************************/
/*        Copyright (C) 2018  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

#ifndef METAMATH_MMCMDL_H_
#define METAMATH_MMCMDL_H_

#include "mmvstr.h"
#include "mmdata.h"

flag processCommandLine(void);
flag getFullArg(long arg, vstring cmdList);
void parseCommandLine(vstring line);
flag lastArgMatches(vstring argString);
flag cmdMatches(vstring cmdString);
long switchPos(vstring swString);
void printCommandError(vstring line, long arg, vstring errorMsg);
void freeCommandLine(void); /* 4-May-2017 Ari Ferrera */

#define DEFAULT_COLUMN 16
extern pntrString *rawArgPntr;
extern nmbrString *rawArgNmbr;
extern long rawArgs;
extern pntrString *fullArg;
extern vstring fullArgString; /* 1-Nov-2013 nm fullArg as one string */
extern vstring commandPrompt;
extern vstring commandLine;
extern long showStatement;
extern vstring logFileName;
extern vstring texFileName;
extern flag PFASmode; /* Proof assistant mode, invoked by PROVE command */
extern flag queryMode; /* If 1, explicit questions will be asked even if
                          a field in the input command line is optional */
extern flag sourceChanged; /* Flag that user made some change to the source
                              file*/
extern flag proofChanged; /* Flag that user made some change to proof in
                             progress*/
extern flag commandEcho; /* Echo full command */
extern flag memoryStatus; /* Always show memory */

/* 31-Dec-2017 nm */
extern flag sourceHasBeenRead; /* 1 if a source file has been read in */
/* 31-Dec-2017 nm */
extern vstring rootDirectory; /* Directory to use for included files */


#endif /* METAMATH_MMCMDL_H_ */
