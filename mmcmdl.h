/*****************************************************************************/
/*       Copyright (C) 1999  NORMAN D. MEGILL nm@alum.mit.edu                */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

flag processCommandLine(void);
flag getFullArg(long arg, vstring cmdList);
void parseCommandLine(vstring line);
flag lastArgMatches(vstring argString);
flag cmdMatches(vstring cmdString);
long switchPos(vstring swString);
void printCommandError(vstring line, long arg, vstring errorMsg);

#define DEFAULT_COLUMN 16
extern pntrString *rawArgPntr;
extern nmbrString *rawArgNmbr;
extern long rawArgs;
extern pntrString *fullArg;
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


