/*****************************************************************************/
/*        Copyright (C) 2002  NORMAN MEGILL  nm@alum.mit.edu                 */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* Command line syntax specification for Metamath */

#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmdl.h"
#include "mminou.h"
#include "mmpfas.h"
#include "mmunif.h"
#include "mmwtex.h"
#include "mmword.h"

/* Global variables */
pntrString *rawArgPntr = NULL_PNTRSTRING;
nmbrString *rawArgNmbr = NULL_NMBRSTRING;
long rawArgs = 0;
pntrString *fullArg = NULL_PNTRSTRING;
vstring commandPrompt = "";
vstring commandLine = "";
long showStatement = 0;
vstring logFileName = "";
vstring texFileName = "";
flag PFASmode = 0; /* Proof assistant mode, invoked by PROVE command */
flag queryMode = 0; /* If 1, explicit questions will be asked even if
    a field in the input command line is optional */
flag sourceChanged = 0; /* Flag that user made some change to the source file*/
flag proofChanged = 0; /* Flag that user made some change to proof in progress*/
flag commandEcho = 0; /* Echo full command */
flag memoryStatus = 0; /* Always show memory */


flag processCommandLine(void)
{
  vstring defaultArg = "";
  vstring tmpStr = "";
  long i;
  queryMode = 0; /* If 1, explicit questions will be asked even if
    a field is optional */
  pntrLet(&fullArg,NULL_PNTRSTRING);


  if (!toolsMode) {

    if (!PFASmode) {
      /* Normal mode */
      let(&tmpStr,cat("DBG|",
          "HELP|READ|WRITE|PROVE|SHOW|SEARCH|SAVE|SUBMIT|OPEN|CLOSE|",
          "SET|FILE|BEEP|EXIT|QUIT|ERASE|VERIFY|TOOLS|MIDI|<HELP>",NULL));
    } else {
      /* Proof assistant mode */
      let(&tmpStr,cat("DBG|",
          "HELP|WRITE|SHOW|SEARCH|SAVE|SUBMIT|OPEN|CLOSE|",
          "SET|FILE|BEEP|EXIT|QUIT|VERIFY|INITIALIZE|ASSIGN|REPLACE|",
        "LET|UNIFY|IMPROVE|MINIMIZE_WITH|MATCH|DELETE|TOOLS|MIDI|<HELP>",NULL));
    }
    if (!getFullArg(0,tmpStr))
      goto pclbad;

    if (cmdMatches("HELP")) {
      if (!getFullArg(1, cat("LANGUAGE|PROOF_ASSISTANT|BEEP|EXIT|QUIT|",
          "READ|ERASE|",
          "OPEN|CLOSE|SHOW|SEARCH|SET|VERIFY|SUBMIT|SYSTEM|PROVE|FILE|WRITE|",
          "ASSIGN|REPLACE|MATCH|UNIFY|LET|INITIALIZE|DELETE|IMPROVE|",
          "MINIMIZE_WITH|SAVE|DEMO|INVOKE|CLI|EXPLORE|TEX|LATEX|HTML|TOOLS|",
          "MIDI|$|<$>", NULL))) goto pclbad;
      if (cmdMatches("HELP OPEN")) {
        if (!getFullArg(2, "LOG|TEX|HTML|<LOG>")) goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP CLOSE")) {
        if (!getFullArg(2, "LOG|TEX|HTML|<LOG>")) goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP SHOW")) {
        if (!getFullArg(2, cat("MEMORY|SETTINGS|LABELS|SOURCE|STATEMENT|",
            "PROOF|NEW_PROOF|USAGE|TRACE_BACK|<MEMORY>", NULL)))
            goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP SET")) {
        if (!getFullArg(2, cat(
            "ECHO|SCROLL|SCREEN_WIDTH|UNIFICATION_TIMEOUT|",
            "EMPTY_SUBSTITUTION|SEARCH_LIMIT|<ECHO>",NULL)))
            goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP VERIFY")) {
        if (!getFullArg(2,
            "PROOF"))
            goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP WRITE")) {
        if (!getFullArg(2,
            "SOURCE"))
            goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP FILE")) {
        if (!getFullArg(2,
            "TYPE|SEARCH|<TYPE>"))
            goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP SAVE")) {
        if (!getFullArg(2,
            "PROOF|NEW_PROOF|<PROOF>"))
            goto pclbad;
        goto pclgood;
      }
      goto pclgood;
    }

    if (cmdMatches("READ")) {
      if (!getFullArg(1,"& What is the name of the source input file? "))
          goto pclbad;
      /* Get any switches */
      i = 1;
      while (1) {
        i++;
        if (!getFullArg(i,"/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i,"VERIFY|NOVERIFY|<NOVERIFY>")) goto pclbad;
        } else {
          break;
        }
      }
      goto pclgood;
    }

    if (cmdMatches("WRITE")) {
      if (!getFullArg(1,"SOURCE|DICTIONARY|<SOURCE>")) goto pclbad;
      if (cmdMatches("WRITE SOURCE")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,cat(
            "* What is the name of the source output file <",
            input_fn, ">? ", NULL)))
          goto pclbad;
        if (!strcmp(input_fn, fullArg[2])) {
          print2(
          "The input file has been renamed %s~1.\n", input_fn);
        }
        goto pclgood;
      }
      if (cmdMatches("WRITE DICTIONARY")) {
        if (!getFullArg(2,
            "* What is the name of the LaTeX symbol dictionary output file? "))
          goto pclbad;
        goto pclgood;
      }
    }

    if (cmdMatches("OPEN")) {
      if (!getFullArg(1,"LOG|TEX|HTML|<LOG>")) goto pclbad;
      if (cmdMatches("OPEN LOG")) {
        if (logFileOpenFlag) {
          printLongLine(cat(
              "?Sorry, the log file \"",logFileName,"\" is currently open.  ",
  "Type CLOSE LOG to close the current log if you want to open another one."
              ,NULL), "", " ");
          goto pclbad;
        }
        if (!getFullArg(2,"* What is the name of logging output file? "))
          goto pclbad;
      }
      if (cmdMatches("OPEN TEX")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (texFileOpenFlag) {
          printLongLine(cat(
              "?Sorry, the LaTeX file \"",texFileName,"\" is currently open.  ",
              "Type CLOSE TEX to close the current LaTeX file",
              " if you want to open another one."
              ,NULL), "", " ");
          goto pclbad;
        }
        if (!getFullArg(2,"* What is the name of LaTeX output file? "))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
              if (!getFullArg(i,cat(
                  "NO_HEADER|<NO_HEADER>",NULL)))
                goto pclbad;
          } else {
            break;
          }
          break; /* Break if only 1 switch is allowed */
        } /* End while for switch loop */

      }
      if (cmdMatches("OPEN HTML")) {
        if (texFileOpenFlag) {
          printLongLine(cat(
              "?Sorry, the HTML file \"",texFileName,"\" is currently open.  ",
              "Type CLOSE HTML to close the current HTML file",
              " if you want to open another one."
              ,NULL), "", " ");
          goto pclbad;
        }
        if (!getFullArg(2,"* What is the name of HTML output file? "))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
              if (!getFullArg(i,cat(
                  "NO_HEADER|<NO_HEADER>",NULL)))
                goto pclbad;
          } else {
            break;
          }
          break; /* Break if only 1 switch is allowed */
        } /* End while for switch loop */

      }
      goto pclgood;
    }

    if (cmdMatches("CLOSE")) {
      if (!getFullArg(1,"LOG|TEX|HTML|<LOG>")) goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("FILE")) {
      if (!getFullArg(1,cat(
          "ADD|CLEAN|DELETE|SUBSTITUTE|SWAP|COUNT|REVISE|DUPLICATE|",
          "PARALLEL|SPLIT|SORT|COPY|TYPE|SEARCH|RENAME",NULL))) goto pclbad;

      if (cmdMatches("FILE TYPE")) {
        if (!getFullArg(2,"& What is the name of the file to type? "))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (i == 3) {
              if (!getFullArg(i,cat(
                  "FROM_LINE|TO_LINE|<FROM_LINE>",NULL)))
                goto pclbad;
            } else {
              if (!getFullArg(i,cat(
                  "FROM_LINE|TO_LINE|<TO_LINE>",NULL)))
                goto pclbad;
            }
            if (lastArgMatches("FROM_LINE")) {
              i++;
              if (!getFullArg(i,"# From what line number <1>? "))
                goto pclbad;
            }
            if (lastArgMatches("TO_LINE")) {
              i++;
              if (!getFullArg(i,"# To what line number <999999>? "))
                goto pclbad;
            }
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        } /* End while for switch loop */


        goto pclgood;
      } /* End if (cmdMatches("FILE TYPE")) */


      if (cmdMatches("FILE SEARCH")) {
        if (!getFullArg(2,"& What is the name of the file to search? "))
          goto pclbad;
        if (!getFullArg(3,"* What is the string to search for? "))
          goto pclbad;


        /* Get any switches */
        i = 3;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (i == 4) {
              if (!getFullArg(i,cat(
                  "FROM_LINE|TO_LINE|<FROM_LINE>",NULL)))
                goto pclbad;
            } else {
              if (!getFullArg(i,cat(
                  "FROM_LINE|TO_LINE|<TO_LINE>",NULL)))
                goto pclbad;
            }
            if (lastArgMatches("FROM_LINE")) {
              i++;
              if (!getFullArg(i,"# From what line number <1>? "))
                goto pclbad;
            }
            if (lastArgMatches("TO_LINE")) {
              i++;
              if (!getFullArg(i,"# To what line number <999999>? "))
                goto pclbad;
            }
            if (lastArgMatches("WINDOW")) { /* ???Not implemented yet */
              i++;
              if (!getFullArg(i,"# How big a window around matched lines <0>? "))
                goto pclbad;
            }
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        } /* End while for switch loop */


        goto pclgood;
      } /* End if (cmdMatches("FILE SEARCH")) */
      goto pclgood;
    }

    if (cmdMatches("SHOW")) {
      if (!PFASmode) {
        if (!getFullArg(1,
     "SETTINGS|LABELS|STATEMENT|SOURCE|PROOF|MEMORY|TRACE_BACK|USAGE|<SETTINGS>"))
            goto pclbad;
      } else {
        if (!getFullArg(1, cat("NEW_PROOF|",
     "SETTINGS|LABELS|STATEMENT|SOURCE|PROOF|MEMORY|TRACE_BACK|USAGE|<SETTINGS>",
            NULL)))
            goto pclbad;
      }
      if (showStatement) {
        let(&defaultArg,cat(" <",statement[showStatement].labelName,">",NULL));
      } else {
        let(&defaultArg,"");
      }


      if (cmdMatches("SHOW TRACE_BACK")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label",defaultArg,"? ",NULL)))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "ALL|ESSENTIAL|AXIOMS|TREE|DEPTH|COUNT_STEPS",
                "|<ALL>",NULL)))
              goto pclbad;
            if (lastArgMatches("DEPTH")) {
              i++;
              if (!getFullArg(i,"# How many indentation levels <999>? "))
                goto pclbad;
            }
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        }

        goto pclgood;
      } /* End if (cmdMatches("SHOW TRACE_BACK")) */

      if (cmdMatches("SHOW USAGE")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label",defaultArg,"? ",NULL)))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "DIRECT|RECURSIVE",
                "|<DIRECT>",NULL)))
              goto pclbad;
          } else {
            break;
          }
          break;  /* Break if only 1 switch is allowed */
        }

        goto pclgood;
      } /* End if (cmdMatches("SHOW USAGE")) */


      if (cmdMatches("SHOW LABELS")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            "* What are the labels to match (* = wildcard) <*>?"))
          goto pclbad;
        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat("ALL|HTML|<ALL>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          /*break;*/ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      }
      if (cmdMatches("SHOW STATEMENT")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label",defaultArg,"? ",NULL)))
          goto pclbad;
        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
"FULL|COMMENT|TEX|HTML|ALT_HTML|BRIEF_HTML|BRIEF_ALT_HTML|<FULL>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      }
      if (cmdMatches("SHOW SOURCE")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label",defaultArg,"? ",NULL)))
          goto pclbad;
        goto pclgood;
      }


      if (cmdMatches("SHOW PROOF")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label",defaultArg,"? ",NULL)))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "ESSENTIAL|ALL|UNKNOWN|FROM_STEP|TO_STEP|DEPTH",
                /*"|REVERSE|LANGUAGE_MODE|VERBOSE|NORMAL|COMPACT|COMPRESSED",*/
                "|REVERSE|LANGUAGE_MODE|VERBOSE|NORMAL|COMPRESSED",
                "|STATEMENT_SUMMARY|DETAILED_STEP|TEX|HTML|SAVE",
                "|LEMMON|COLUMN|RENUMBER|<ESSENTIAL>",NULL)))
              goto pclbad;
            if (lastArgMatches("FROM_STEP")) {
              i++;
              if (!getFullArg(i,"# From what step <1>? "))
                goto pclbad;
            }
            if (lastArgMatches("TO_STEP")) {
              i++;
              if (!getFullArg(i,"# To what step <9999>? "))
                goto pclbad;
            }
            if (lastArgMatches("DEPTH")) {
              i++;
              if (!getFullArg(i,"# How many indentation levels <999>? "))
                goto pclbad;
            }
            if (lastArgMatches("DETAILED_STEP")) {
              i++;
              if (!getFullArg(i,"# Display details of what step <1>? "))
                goto pclbad;
            }
            if (lastArgMatches("COLUMN")) {
              i++;
              if (!getFullArg(i, cat(
                  "# At what column should the formula start <",
                  str(DEFAULT_COLUMN), ">? ", NULL)))
                goto pclbad;
            }
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      } /* End if (cmdMatches("SHOW PROOF")) */


      if (cmdMatches("SHOW NEW_PROOF")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }

        /* Get any switches */
        i = 1;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "ESSENTIAL|ALL|UNKNOWN|FROM_STEP|TO_STEP|DEPTH",
                /*"|REVERSE|LANGUAGE_MODE|VERBOSE|NORMAL|COMPACT|COMPRESSED",*/
                "|REVERSE|LANGUAGE_MODE|VERBOSE|NORMAL|COMPRESSED",
                "|NOT_UNIFIED|TEX|HTML",
                "|LEMMON|COLUMN|RENUMBER|<ESSENTIAL>",NULL)))
              goto pclbad;
            if (lastArgMatches("FROM_STEP")) {
              i++;
              if (!getFullArg(i,"# From what step <1>? "))
                goto pclbad;
            }
            if (lastArgMatches("TO_STEP")) {
              i++;
              if (!getFullArg(i,"# To what step <9999>? "))
                goto pclbad;
            }
            if (lastArgMatches("DEPTH")) {
              i++;
              if (!getFullArg(i,"# How many indentation levels <999>? "))
                goto pclbad;
            }
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      } /* End if (cmdMatches("SHOW NEW_PROOF")) */


      goto pclgood;
    } /* End of SHOW */

    if (cmdMatches("SEARCH")) {
      if (statements == 0) {
        print2("?No source file has been read in.  Use READ first.\n");
        goto pclbad;
      }
      if (!getFullArg(1,
          "* What are the labels to match (* = wildcard) <*>?"))
        goto pclbad;
      if (!getFullArg(2,"* Search for what math symbol string? "))
          goto pclbad;
      /* Get any switches */
      i = 2;
      while (1) {
        i++;
        if (!getFullArg(i,"/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i,cat("ALL|COMMENTS|<ALL>", NULL)))
            goto pclbad;
        } else {
          break;
        }
        /*break;*/ /* Break if only 1 switch is allowed */
      }
      goto pclgood;

    } /* End of SEARCH */


    if (cmdMatches("SAVE")) {
      if (!PFASmode) {
        if (!getFullArg(1,
            "PROOF|<PROOF>"))
            goto pclbad;
      } else {
        if (!getFullArg(1, cat("NEW_PROOF|",
            "PROOF|<NEW_PROOF>",
            NULL)))
            goto pclbad;
      }
      if (showStatement) {
        let(&defaultArg,cat(" <",statement[showStatement].labelName,">",NULL));
      } else {
        let(&defaultArg,"");
      }


      if (cmdMatches("SAVE PROOF")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label",defaultArg,"? ",NULL)))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "NORMAL|COMPACT|COMPRESSED",
                "|<NORMAL>",NULL)))
              goto pclbad;
          } else {
            break;
          }
          break; /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      } /* End if (cmdMatches("SAVE PROOF")) */


      if (cmdMatches("SAVE NEW_PROOF")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }

        /* Get any switches */
        i = 1;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "NORMAL|COMPACT|COMPRESSED",
                "|<NORMAL>",NULL)))
              goto pclbad;
          } else {
            break;
          }
          break; /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      } /* End if (cmdMatches("SAVE NEW_PROOF")) */


      goto pclgood;
    } /* End of SAVE */


    if (cmdMatches("PROVE")) {
      if (statements == 0) {
        print2("?No source file has been read in.  Use READ first.\n");
        goto pclbad;
      }
      if (!proveStatement) proveStatement = showStatement;
      if (proveStatement) {
        let(&defaultArg,cat(" <",statement[proveStatement].labelName,">",NULL));
      } else {
        let(&defaultArg,"");
      }
      if (!getFullArg(1,
          cat("* What is the label of the statement you want to try proving",
          defaultArg,"? ",NULL)))
        goto pclbad;
      goto pclgood;
    }

    /* Commands in Proof Assistant mode */

    if (cmdMatches("MATCH")) {
      if (!getFullArg(1,
          "STEP|ALL|<ALL>")) goto pclbad;
      if (cmdMatches("MATCH STEP")) {
        if (!getFullArg(2,"# What step number? ")) goto pclbad;
        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "MAX_ESSENTIAL_HYP|<MAX_ESSENTIAL_HYP>", NULL)))
              goto pclbad;
            if (lastArgMatches("MAX_ESSENTIAL_HYP")) {
              i++;
              if (!getFullArg(i,
  "# Maximum number of essential hypotheses to allow for a match <0>? "))
                goto pclbad;
            }
          } else {
            break;
          }
          break;  /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      }
      if (cmdMatches("MATCH ALL")) {
        /* Get any switches */
        i = 1;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "ESSENTIAL|MAX_ESSENTIAL_HYP|<ESSENTIAL>", NULL)))
              goto pclbad;
            if (lastArgMatches("MAX_ESSENTIAL_HYP")) {
              i++;
              if (!getFullArg(i,
  "# Maximum number of essential hypotheses to allow for a match <0>? "))
                goto pclbad;
            }
          } else {
            break;
          }
          /*break;*/  /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      } /* End if (cmdMatches("MATCH ALL")) */
      goto pclgood;
    }

    if (cmdMatches("INITIALIZE")) {
      if (!getFullArg(1,
          "STEP|ALL|<ALL>")) goto pclbad;
      if (cmdMatches("INITIALIZE STEP")) {
        if (!getFullArg(2,"# What step number? ")) goto pclbad;
      }
      goto pclgood;
    }

    if (cmdMatches("IMPROVE")) {
      if (!getFullArg(1,
          "STEP|ALL|LAST|<ALL>")) goto pclbad;

      if (cmdMatches("IMPROVE STEP")) {
        if (!getFullArg(2,"# What step number? ")) goto pclbad;
      }
      if (cmdMatches("IMPROVE STEP") || cmdMatches("IMPROVE ALL")
          || cmdMatches("IMPROVE LAST")) {

        /* Get switches */
        if (cmdMatches("IMPROVE STEP")) {
          i = 2;
        } else {
          i = 1;
        }
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,
                "DEPTH|<DEPTH>")
                ) goto pclbad;
            if (lastArgMatches("DEPTH")) {
              i++;
              if (!getFullArg(i,
  "# What is maximum depth for searching statements with $e hypotheses <0>? "))
                goto pclbad;
            }
          } else {
            break;
          }
          /*break;*/ /* Do this if only 1 switch is allowed */
        } /* end while */
        goto pclgood;
      } /* end if IMPROVE STEP or IMPROVE ALL */
    }

    if (cmdMatches("MINIMIZE_WITH")) {
      if (!getFullArg(1,"* What statement label? ")) goto pclbad;
      /* Get any switches */
      i = 1;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i,cat(
              "BRIEF|ALLOW_GROWTH|NO_DISTINCT|<BRIEF>", NULL)))
            goto pclbad;
        } else {
          break;
        }
        /*break;*/  /* Break if only 1 switch is allowed */
      }
      goto pclgood;
    }

    if (cmdMatches("UNIFY")) {
      if (!getFullArg(1,
          "STEP|ALL|<ALL>")) goto pclbad;
      if (cmdMatches("UNIFY STEP")) {
        if (!getFullArg(2,"# What step number? ")) goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("UNIFY ALL")) {
        /* Get any switches */
        i = 1;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "INTERACTIVE|<INTERACTIVE>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          break;  /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      } /* End if (cmdMatches("UNIFY ALL")) */
    }

    if (cmdMatches("DELETE")) {
      if (!getFullArg(1,
          "STEP|ALL|FLOATING_HYPOTHESES|<STEP>")) goto pclbad;
      if (lastArgMatches("STEP")) {
        if (!getFullArg(2,"# What step number? ")) goto pclbad;
        goto pclgood;
      }
      goto pclgood;
    }

    /*???OBSOL???*/
    if (cmdMatches("ADD")) {
      if (!getFullArg(1,
          "UNIVERSE|<UNIVERSE>")) goto pclbad;
      /* Note:  further parsing below */
    }

    if (cmdMatches("REPLACE")) {
      if (!getFullArg(1,"# Replace what step number? ")) goto pclbad;
      if (!getFullArg(2,"* With what statement label? ")) goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("LET")) {
      if (!getFullArg(1,"STEP|VARIABLE|<STEP>")) goto pclbad;
      if (cmdMatches("LET STEP")) {
        if (!getFullArg(2,"# Assign what step number? ")) goto pclbad;
      }
      if (cmdMatches("LET VARIABLE")) {
        if (!getFullArg(2,"* Assign what variable (format $nnn)? ")) goto pclbad;
      }
      if (!getFullArg(3,"=|<=>")) goto pclbad;
      if (!getFullArg(4,"* With what math symbol string? "))
          goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("ASSIGN")) {
      if (!getFullArg(1,"* What step number, or LAST <LAST>? ")) goto pclbad;
      if (!getFullArg(2,"* With what statement label? ")) goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("REVERT")) {
      goto pclgood;
    }

    if (cmdMatches("SET")) {
      let(&tmpStr, cat(
          /*"ECHO|SCROLL|UNIVERSE|",*/
          "SCREEN_WIDTH|ECHO|SCROLL|",
          "DEBUG|MEMORY_STATUS|SEARCH_LIMIT|UNIFICATION_TIMEOUT|",
          "EMPTY_SUBSTITUTION|HENTY_FILTER|<SCREEN_WIDTH>",NULL));
      if (!getFullArg(1,tmpStr)) goto pclbad;
      if (cmdMatches("SET DEBUG")) {
        if (!getFullArg(2,"FLAG|OFF|<OFF>")) goto pclbad;
        if (lastArgMatches("FLAG")) {
          if (!getFullArg(3,"4|5|6|7|8|9|<5>")) goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SET ECHO")) {
        if (commandEcho) {
          if (!getFullArg(2,"ON|OFF|<OFF>")) goto pclbad;
        } else {
          if (!getFullArg(2,"ON|OFF|<ON>")) goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SET SCROLL")) {
        if (scrollMode == 1) {
          if (!getFullArg(2,"CONTINUOUS|PROMPTED|<CONTINUOUS>")) goto pclbad;
        } else {
          if (!getFullArg(2,"CONTINUOUS|PROMPTED|<PROMPTED>")) goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SET MEMORY_STATUS")) {
        if (memoryStatus) {
          if (!getFullArg(2,"ON|OFF|<OFF>")) goto pclbad;
        } else {
          if (!getFullArg(2,"ON|OFF|<ON>")) goto pclbad;
        }
        goto pclgood;
      }


      if (cmdMatches("SET HENTY_FILTER")) {
        if (hentyFilter) {
          if (!getFullArg(2,"ON|OFF|<OFF>")) goto pclbad;
        } else {
          if (!getFullArg(2,"ON|OFF|<ON>")) goto pclbad;
        }
        goto pclgood;
      }


      if (cmdMatches("SET SEARCH_LIMIT")) {
        if (!getFullArg(2, cat(
            "# What is search limit for IMPROVE command <",
            str(userMaxProveFloat), ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET UNIFICATION_TIMEOUT")) {
        if (!getFullArg(2, cat(
           "# What is maximum number of unification trials <",
            str(userMaxUnifTrials), ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET SCREEN_WIDTH")) {
        if (!getFullArg(2, cat(
           "# What is maximum line length on your screen <",
            str(screenWidth), ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET EMPTY_SUBSTITUTION")) {
        if (minSubstLen == 0) {
          if (!getFullArg(2,"ON|OFF|<OFF>")) goto pclbad;
        } else {
          if (!getFullArg(2,"ON|OFF|<ON>")) goto pclbad;
        }
        goto pclgood;
      }

    } /* end if SET */

    if (cmdMatches("INPUT")) {
      if (!getFullArg(1,"PROOF|<PROOF>")) goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("SET UNIVERSE") || cmdMatches("ADD UNIVERSE") ||
        cmdMatches("DELETE UNIVERSE")) {
      /* Get a list of statement labels */
      i = 1;
      while (1) {
        i++;
        /*??? The user will never be asked this. */
        if (!getFullArg(i,"* Statement label or '*' or '$f'|$<$>? "))
            goto pclbad;
        if (lastArgMatches("")) goto pclgood; /* End of argument list */
      } /* end while */
    } /* end if xxx UNIVERSE */

    if (cmdMatches("ERASE")) {
      goto pclgood;
    }

    if (cmdMatches("TOOLS")) {
      goto pclgood;
    }

    if (cmdMatches("VERIFY")) {
      if (!getFullArg(1,
          "PROOFS|<PROOFS>"))
        goto pclbad;
      if (cmdMatches("VERIFY PROOFS")) {
        if (statements == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            "* What are the labels to match (* = wildcard) <*>?"))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i,"/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i,cat(
                "COMPLETE|SYNTAX_ONLY",
                "|<COMPLETE>",NULL)))
              goto pclbad;
          } else {
            break;
          }
          break;  /* Break if only 1 switch is allowed */
        }

        goto pclgood;
      }
    }

    if (cmdMatches("DBG")) {
      /* The debug command fetches an arbitrary 2nd arg in quotes, to be handled
         in whatever way is needed for debugging. */
      if (!getFullArg(1,"* What is the debugging string? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("MIDI")) {
      if (statements == 0) {
        print2("?No source file has been read in.  Use READ first.\n");
        goto pclbad;
      }
      if (!getFullArg(1,
          "* What are the labels to match (* = wildcard) <*>?"))
        goto pclbad;
      /* Get any switches */
      i = 1;
      while (1) {
        i++;
        if (!getFullArg(i,"/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i,cat("PARAMETER|<PARAMETER>", NULL)))
            goto pclbad;
          i++;
          if (!getFullArg(i,
              "* What is the parameter string <FSH>?"))
            goto pclbad;
        } else {
          break;
        }
        break; /* Break if only 1 switch is allowed */
      }
      goto pclgood;
    }

  } else { /* toolsMode */
    /* Text tools mode */
    let(&tmpStr,cat(
          "HELP|SUBMIT|",
          "ADD|DELETE|SUBSTITUTE|S|SWAP|CLEAN|INSERT|BREAK|BUILD|MATCH|SORT|",
          "UNDUPLICATE|DUPLICATE|UNIQUE|REVERSE|RIGHT|PARALLEL|NUMBER|COUNT|",
          "COPY|C|TYPE|T|TAG|BEEP|B|EXIT|QUIT|<HELP>",NULL));
    if (!getFullArg(0,tmpStr))
      goto pclbad;

    if (cmdMatches("HELP")) {
      if (!getFullArg(1, cat(
          "ADD|DELETE|SUBSTITUTE|S|SWAP|CLEAN|INSERT|BREAK|BUILD|MATCH|SORT|",
          "UNDUPLICATE|DUPLICATE|UNIQUE|REVERSE|RIGHT|PARALLEL|NUMBER|COUNT|",
          "TYPE|T|TAG|BEEP|B|EXIT|QUIT|",
          "COPY|C|SUBMIT|SYSTEM|CLI|",
          "$|<$>", NULL))) goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("ADD")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,"* String to add to beginning of each line <>? "))
        goto pclbad;
      if (!getFullArg(3,"* String to add to end of each line <>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("DELETE")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
"* String from which to start deleting (CR = beginning of line) <>? "))
        goto pclbad;
      if (!getFullArg(3,
"* String at which to stop deleting (CR = end of line) <>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("CLEAN")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
          "* Subcommand(s) (D,B,E,R,Q,T,U,P,G,C,L,V) <B,E,R>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("SWAP")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
"* Character string to match between the halves to be swapped? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("SUBSTITUTE") || cmdMatches("S")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,"* String to replace? "))
        goto pclbad;
      if (!getFullArg(3,"* Replace it with <>? "))
        goto pclbad;
      if (!getFullArg(4,
"* Which occurrence in the line (1,2,... or ALL) <1>? "))
        goto pclbad;
      if (!getFullArg(5,
"* Additional match required on line (null = match all) <>? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("INSERT")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,"* String to insert in each line <!>? "))
        goto pclbad;
      if (!getFullArg(3,"# Column at which to insert the string <1>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("BREAK")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
          "* Special characters to use as token delimiters <()[],=:;{}>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("MATCH")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
"* String to match on each line (null = any non-blank line) <>? "))
        goto pclbad;
      if (!getFullArg(3,
"* Output those lines containing the string (Y) or those not (N) <Y>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("SORT")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
          "* String to start key on each line (null string = column 1) <>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("UNDUPLICATE") || cmdMatches("DUPLICATE") ||
        cmdMatches("UNIQUE") || cmdMatches("REVERSE") || cmdMatches("BUILD")
        || cmdMatches("RIGHT")) {
      if (!getFullArg(1,"& Input/output file? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("COUNT")) {
      if (!getFullArg(1,"& Input file? "))
        goto pclbad;
      if (!getFullArg(2,
"* String to count <;>? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("COPY") || cmdMatches("C")) {
      if (!getFullArg(1,"* Comma-separated list of input files? "))
        goto pclbad;
      if (!getFullArg(2,"* Output file? "))
        goto pclbad;
      goto pclgood;
    }


    if (cmdMatches("NUMBER")) {
      if (!getFullArg(1,"* Output file <n.tmp>? "))
        goto pclbad;
      if (!getFullArg(2,"# First number <1>? "))
        goto pclbad;
      if (!getFullArg(3,"# Last number <10>? "))
        goto pclbad;
      if (!getFullArg(4,"# Increment <1>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("TYPE") || cmdMatches("T")) {
      if (!getFullArg(1,"& File to display on the screen? "))
        goto pclbad;
      if (!getFullArg(2,"* Num. lines to type or ALL (nothing = 10) <$>? "))
        goto pclbad;
      goto pclgood;
    }


    if (cmdMatches("TAG")) {
      print2(
"Warning: Do not comment out code - delete it before running TAG!  If\n");
      print2(
"rerunning TAG, do not tamper with \"start/end of deleted section\" comments!\n");
      print2(
"Edit out tag on header comment line!  Review the output file!\n");
      if (!getFullArg(1,"& Original (reference) program input file? "))
        goto pclbad;
      if (!getFullArg(2,"& Edited program input file? "))
        goto pclbad;
      if (!getFullArg(3,cat(
"* Edited program output file with revisions tagged <",
          fullArg[2], ">? ", NULL)))
        goto pclbad;
      if (!strcmp(fullArg[2], fullArg[3])) {
        print2(
"The input file will be renamed %s~1.\n", fullArg[2]);
      }
      if (!getFullArg(4,
          cat("* Revision tag for added lines </* #", str(highestRevision(
              fullArg[1]) + 1), " */>? ", NULL)))
        goto pclbad;
      if (!getFullArg(5,
"# Successive lines required for match (more = better sync) <3>? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("PARALLEL")) {
      if (!getFullArg(1,"& Left file? "))
        goto pclbad;
      if (!getFullArg(2,"& Right file? "))
        goto pclbad;
      if (!getFullArg(3,cat("* Output file <",
          fullArg[1], ">? ", NULL)))
        goto pclbad;
      if (!getFullArg(4,
          cat("* String to insert between the 2 input lines <>? ", NULL)))
        goto pclbad;
      goto pclgood;
    }


  } /* if !toolsMode ... else ... */

  if (cmdMatches("SUBMIT")) {
    if (toolsMode) {
      let(&tmpStr, " <list.cmd>");
    } else {
      let(&tmpStr, "");
    }
    if (!getFullArg(1,cat("& What is the name of command file to run",
        tmpStr, "? ", NULL)))
      goto pclbad;
    goto pclgood;
  }

  if (cmdMatches("BEEP") || cmdMatches("B")) {
    goto pclgood;
  }

  if (cmdMatches("EXIT") || cmdMatches("QUIT")) {
    goto pclgood;
  }

  /* Command in master list but not intercepted -- really a bug */
  print2("?This command has not been implemented yet.\n");
  print2("(This is really a bug--please report it.)\n");
  goto pclbad;

  /* Should never get here */



 pclgood:

  /* Strip off the last fullArg if a null argument was added by getFullArg
     in the case when "$" (nothing) is allowed */
  if (!strcmp(fullArg[pntrLen(fullArg)-1], chr(3))) {
    let((vstring *)(&fullArg[pntrLen(fullArg)-1]), ""); /* Deallocate */
    pntrLet(&fullArg, pntrLeft(fullArg, pntrLen(fullArg) - 1));
  }

  if (pntrLen(fullArg) > rawArgs) bug(1102);
  if (pntrLen(fullArg) < rawArgs) {
    let(&tmpStr, cat("?Too many arguments.  Use quotes around arguments with special",
        " characters and around Unix file names with \"/\"s.", NULL));
    printCommandError(cat(commandPrompt, commandLine, NULL), pntrLen(fullArg),
        tmpStr);
    goto pclbad;
  }

  /* Deallocate memory */
  let(&defaultArg,"");
  let(&tmpStr,"");
  return (1);

 pclbad:
  /* Deallocate memory */
  let(&defaultArg,"");
  let(&tmpStr,"");
  return (0);

}



flag getFullArg(long arg, vstring cmdList1)
{
  /* This function converts the user's abbreviated keyword in
     rawArgPntr[arg] to a full, upper-case keyword,
     in fullArg[arg], matching
     the available choices in cmdList. */
  /* Special cases:  cmdList = "# xxx <yyy>?" - get an integer */
  /*                 cmdList = "* xxx <yyy>?" - get any string;
                       don't convert to upper case
                     cmdList = "& xxx <yyy>?" - same as * except
                       verify it is a file that exists */
  /* "$" means a null argument is acceptable; put it in as
     special character chr(3) so it can be recognized */

  pntrString *possCmd = NULL_PNTRSTRING;
  long possCmds, i, j, k, m, p, q;
  vstring defaultCmd = "";
  vstring infoStr = "";
  vstring tmpStr = "";
  vstring tmpArg = "";
  vstring errorLine = "";
  vstring keyword = "";
  vstring cmdList = "";
  FILE *tmpFp;

  let(&cmdList,cmdList1); /* In case cmdList1 gets deallocated when it comes
                             directly from a vstring function such as cat() */

  let(&errorLine,cat(commandPrompt,commandLine,NULL));

  /* Handle special case - integer expected */
  if (cmdList[0] == '#') {
    let(&defaultCmd,
        seg(cmdList,instr(1,cmdList,"<"),instr(1,cmdList,">")));

    /* If the argument has not been entered, prompt the user for it */
    if (rawArgs <= arg) {
      pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
      nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, 0));
      rawArgs++;
      if (rawArgs <= arg) bug(1103);

      queryMode = 1;
      tmpArg = cmdInput1(right(cmdList,3));
      let(&errorLine,right(cmdList,3));
      if (tmpArg[0] == 0) { /* Use default argument */
        let(&tmpArg, seg(defaultCmd,2,len(defaultCmd) - 1));
      }
      let((vstring *)(&rawArgPntr[arg]), tmpArg);
      rawArgNmbr[arg] = len(cmdList) - 1;/* Line position for error msgs */

    } /* End of asking user for additional argument */

    /* Make sure that the argument is a non-negative integer */
    let(&tmpArg,rawArgPntr[arg]);
    if (tmpArg[0] == 0) { /* Use default argument */
      /* (This code is needed in case of null string passed directly) */
      let(&tmpArg, seg(defaultCmd,2,len(defaultCmd) - 1));
    }
    let(&tmpStr,str(val(tmpArg)));
    let(&tmpStr,cat(string(len(tmpArg)-len(tmpStr),'0'),tmpStr,NULL));
    if (strcmp(tmpStr,tmpArg)) {
      printCommandError(errorLine, arg,
          "?A number was expected here.");
      goto return0;
    }

    let(&keyword,str(val(tmpArg)));
    goto return1;
  }



  /* Handle special case - any arbitrary string is OK */
  /* However, "|$<$>" also allows null string (no argument) */
  if (cmdList[0] == '*' || cmdList[0] == '&') {
    let(&defaultCmd,
        seg(cmdList,instr(1,cmdList,"<"),instr(1,cmdList,">")));

    /* If the argument has not been entered, prompt the user for it */
    if (rawArgs <= arg) {
      if (!strcmp(defaultCmd,"<$>")) { /* End of command acceptable */
        /* Note:  in this case, user will never be prompted for anything. */
        let(&keyword,chr(3));
        goto return1;
      }
      rawArgs++;
      pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
      nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, 0));
      if (rawArgs <= arg) bug(1104);
      queryMode = 1;
      tmpArg = cmdInput1(right(cmdList,3));

      /* Strip off any quotes around it
         and tolerate lack of trailing quote */
      /******* (This is no longer done - it is confusing to the user.)
      if (tmpArg[0] == '\'' || tmpArg[0] == '\"') {
        if (tmpArg[0] == tmpArg[len(tmpArg) - 1]) {
          let(&tmpArg, right(left(tmpArg, len(tmpArg) - 1), 2));
        } else {
          let(&tmpArg, right(tmpArg, 2));
        }
      }
      *******/

      let(&errorLine,right(cmdList,3));
      if (tmpArg[0] == 0) { /* Use default argument */
        let(&tmpArg, seg(defaultCmd,2,len(defaultCmd) - 1));
      }
      let((vstring *)(&rawArgPntr[arg]), tmpArg);
      rawArgNmbr[arg] = len(cmdList) - 1; /* Line position for error msgs */

    } /* End of asking user for additional argument */

    let(&keyword,rawArgPntr[arg]);
    if (keyword[0] == 0) { /* Use default argument */
      /* This case handles blank arguments on completely input command line */
      let(&keyword, seg(defaultCmd,2,len(defaultCmd) - 1));
    }
    if (cmdList[0] == '&') {
      /* See if file exists */
      tmpFp = fopen(keyword, "r");
      if (!tmpFp) {
        let(&tmpStr,  cat(
            "?Sorry, couldn't open the file \"", keyword, "\".", NULL));
        printCommandError(errorLine, arg, tmpStr);
        goto return0;
      }
      fclose(tmpFp);
    }
    goto return1;
  }



  /* Parse the choices available */
  possCmds = 0;
  p = 0;
  while (1) {
    q = p;
    p = instr(p + 1, cat(cmdList,"|",NULL), "|");
    if (!p) break;
    pntrLet(&possCmd,pntrAddElement(possCmd));
    let((vstring *)(&possCmd[possCmds]),seg(cmdList,q+1,p-1));
    possCmds++;
  }
  if (!strcmp(left(possCmd[possCmds - 1],1),"<")) {
    /* Get default argument, if any */
    defaultCmd = possCmd[possCmds - 1]; /* re-use old allocation */
    if (!strcmp(defaultCmd,"<$>")) {
      let(&defaultCmd,"<nothing>");
    }
    pntrLet(&possCmd,pntrLeft(possCmd,possCmds - 1));
    possCmds--;
  }
  if (!strcmp(possCmd[possCmds - 1],"$")) {
    /* Change "$" to "nothing" for printouts */
    let((vstring *)(&possCmd[possCmds - 1]),"nothing");
  }

  /* Create a string used for queries and error messages */
  if (possCmds < 1) bug(1105);
  if (possCmds == 1) {
    let(&infoStr,possCmd[0]);
  }
  if (possCmds == 2) {
    let(&infoStr,cat(possCmd[0]," or ",
        possCmd[1],NULL));
  }
  if (possCmds > 2) {
    let(&infoStr,"");
    for (i = 0; i < possCmds - 1; i++) {
      let(&infoStr,cat(infoStr,possCmd[i],", ",NULL));
    }
    let(&infoStr,cat(infoStr,"or ",possCmd[possCmds - 1],NULL));
  }

  /* If the argument has not been entered, prompt the user for it */
  if (rawArgs <= arg && (strcmp(possCmd[possCmds - 1],"nothing")
      || queryMode == 1)) {

    let(&tmpStr, infoStr);
    if (defaultCmd[0] != 0) {
      let(&tmpStr,cat(tmpStr," ",defaultCmd,NULL));
    }
    let(&tmpStr,cat(tmpStr,"? ",NULL));
    queryMode = 1;
    if (possCmds != 1) {
      tmpArg = cmdInput1(tmpStr);
    } else {
      /* There is only one possibility, so don't ask user */
      /* Don't print the message when "end-of-list" is the only possibility. */
      if (!strcmp(cmdList,"$|<$>")) {
        let(&tmpArg, possCmd[0]);
        print2("The command so far is:  ");
        for (i = 0; i < arg; i++) {
          print2("%s ", fullArg[i]);
        }
        print2("%s\n", tmpArg);
      }
    }
    let(&errorLine,tmpStr);
    if (tmpArg[0] == 0) { /* Use default argument */
      let(&tmpArg, seg(defaultCmd,2,len(defaultCmd) - 1));
    }

    if (strcmp(tmpArg,"nothing")) {
      pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
      nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, 0));
      rawArgs++;
      if (rawArgs <= arg) bug(1106);
      let((vstring *)(&rawArgPntr[arg]), tmpArg);
      rawArgNmbr[arg] = len(tmpStr) + 1; /* Line position for error msgs */
    }

  } /* End of asking user for additional argument */

  if (rawArgs <= arg) {
    /* No argument was specified, and "nothing" is a valid argument */
    let(&keyword,chr(3));
    goto return1;
  }


  let(&tmpArg,edit(rawArgPntr[arg], 32)); /* Convert to upper case */
  j = 0;
  k = 0;
  m = len(tmpArg);
  let(&tmpStr,"");
  /* Scan the possible arguments for a match */
  for (i = 0; i < possCmds; i++) {
    if (!strcmp(possCmd[i], tmpArg)) {
      /* An exact match was found, so ignore any other matches
         and use this one */
      k = 1;
      j = i;
      break;
    }
    if (!strcmp(left(possCmd[i], m), tmpArg)) {
      if (!k) {
        let(&tmpStr, possCmd[i]);
      } else {
        let(&tmpStr, cat(tmpStr, ", ", possCmd[i], NULL));
      }
      j = i; /* Save match position */
      k++; /* Number of matches */
    }
  }
  if (k < 1 || k > 1) {
    if (k < 1) {
      let(&tmpStr, cat("?Expected ", infoStr, ".", NULL));
    } else {
      if (k == 2) {
        p = instr(1,tmpStr,",");
        let(&tmpStr,cat(left(tmpStr,p-1)," or",right(tmpStr,p+1),NULL));
      } else {
        p = len(tmpStr) - 1;
        while (tmpStr[p] != ',') p--;
        let(&tmpStr,cat(left(tmpStr,p+1)," or",right(tmpStr,p+2),NULL));
      }
      let(&tmpStr, cat("?Ambiguous keyword - please specify ",tmpStr,".",NULL));
    }
    printCommandError(errorLine, arg, tmpStr);
    goto return0;
  }

  let(&keyword,possCmd[j]);
  goto return1;

 return1:
  if (keyword[0] == 0) {
    if (rawArgs > arg && strcmp(defaultCmd, "<>")) {
      /* otherwise, "nothing" was specified */
      printCommandError("", arg,
          "?No default answer is available - please be explicit.");
      goto return0;
    }
  }
  /* Add new field to fullArg */
  pntrLet(&fullArg,pntrAddElement(fullArg));
  if (pntrLen(fullArg) != arg + 1) bug(1107);
  let((vstring *)(&fullArg[arg]),keyword);

  /* Deallocate memory */
  j = pntrLen(possCmd);
  for (i = 0; i < j; i++) let((vstring *)(&possCmd[i]),"");
  pntrLet(&possCmd, NULL_PNTRSTRING);
  let(&defaultCmd,"");
  let(&infoStr,"");
  let(&tmpStr,"");
  let(&tmpArg,"");
  let(&errorLine,"");
  let(&keyword,"");
  let(&cmdList,"");
  return(1);

 return0:
  /* Deallocate memory */
  j = pntrLen(possCmd);
  for (i = 0; i < j; i++) let((vstring *)(&possCmd[i]),"");
  pntrLet(&possCmd, NULL_PNTRSTRING);
  let(&defaultCmd,"");
  let(&infoStr,"");
  let(&tmpStr,"");
  let(&tmpArg,"");
  let(&errorLine,"");
  let(&keyword,"");
  let(&cmdList,"");
  return(0);

}



void parseCommandLine(vstring line)
{
  /* This function breaks up line into individual tokens
     and puts them into rawArgPntr[].  rawArgs is the number of tokens.
     rawArgPntr[] is the starting position of each token on the line;
     the first character on the line has position 1, not 0.

     Spaces, tabs, and newlines are considered white space.  Special
     one-character
     tokens don't have to be surrounded by white space.  Characters
     inside quotes are considered to be one token, and the quotes are
     removed.

  */
  /* Warning:  Don't deallocate these vstring constants */
  /*vstring specialOneCharTokens = "()/,=:";*/
  vstring tokenWhiteSpace = " \t\n";
  vstring tokenComment = "!";


  vstring tmpStr = ""; /* Dummy vstring to clean up temp alloc stack */
  flag mode;
  long tokenStart, i, p, lineLen;

  vstring specialOneCharTokens = "";

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  tokenStart = 0;

  if (!toolsMode) {
    /* 5-Nov-99:  We only really need / and =
       Took out the others so user will have less need to put quotes around
       e.g. single tokens in SEARCH command
    let(&specialOneCharTokens, "()/,=:");
    */
    let(&specialOneCharTokens, "/="); /* List of special one-char tokens */
  } else {
    let(&specialOneCharTokens, "");
  }

  lineLen = len(line);
  /* mode: 0 means look for start of token, 1 means look for end of
     token, 2 means look for trailing single quote, 3 means look for
     trailing double quote */
  /* 2/20/99 - only "!" at beginning of line now acts as comment
     - This was done because sometimes ! might be legal as part of a command */
  mode = 0;
  for (p = 0; p < lineLen; p++) {
    let(&tmpStr, ""); /* Clean up temp alloc stack to prevent overflow */
    if (mode == 0) {
      /* If character is white space, ignore it */
      if (instr(1,tokenWhiteSpace,chr(line[p]))) {
        continue;
      }
      /* If character is comment, we're done */
      if (p == 0 && /* 2/20/99 */ instr(1,tokenComment,chr(line[p]))) {
        break;
      }
      /* If character is a special token, get it but don't change mode */
      if (instr(1,specialOneCharTokens,chr(line[p]))) {
        pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
        nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, p+1));
                                                          /* Save token start */
        let((vstring *)(&rawArgPntr[rawArgs]),chr(line[p]));
        rawArgs++;
        continue;
      }
      /* If character is a quote, set start and change mode */
      if (line[p] == '\'') {
        mode = 2;
        tokenStart = p + 2;
        continue;
      }
      if (line[p] == '\"') {
        mode = 3;
        tokenStart = p + 2;
        continue;
      }
      /* Character must be start of a token */
      mode = 1;
      tokenStart = p + 1;
      continue;
    }
    if (mode == 1) {
      /* If character is white space, end token and change mode */
      if (instr(1,tokenWhiteSpace,chr(line[p]))) {
        pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
        nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, tokenStart));
                                                          /* Save token start */
        let((vstring *)(&rawArgPntr[rawArgs]), seg(line,tokenStart,p));
        rawArgs++;
        mode = 0;
        continue;
      }
      /* If character is comment, we're done */
      /* 2/20/99 - see above
      if (instr(1,tokenComment,chr(line[p]))) {
        pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
        nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, tokenStart));
        let((vstring *)(&rawArgPntr[rawArgs]), seg(line,tokenStart,p));
        rawArgs++;
        mode = 0;
        break;
      }
      2/20/99 */
      /* If character is a special token, get it and change mode */
      if (instr(1,specialOneCharTokens,chr(line[p]))) {
        pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
        nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, tokenStart));
                                                          /* Save token start */
        let((vstring *)(&rawArgPntr[rawArgs]),seg(line,tokenStart,p));
        rawArgs++;
        pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
        nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, p + 1));
                                                          /* Save token start */
        let((vstring *)(&rawArgPntr[rawArgs]),chr(line[p]));
        rawArgs++;
        mode = 0;
        continue;
      }
      /* If character is a quote, set start and change mode */
      if (line[p] == '\'') {
        pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
        nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, tokenStart));
                                                          /* Save token start */
        let((vstring *)(&rawArgPntr[rawArgs]),seg(line,tokenStart,p));
        rawArgs++;
        mode = 2;
        tokenStart = p + 2;
        continue;
      }
      if (line[p] == '\"') {
        pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
        nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, tokenStart));
                                                          /* Save token start */
        let((vstring *)(&rawArgPntr[rawArgs]),seg(line,tokenStart,p));
        rawArgs++;
        mode = 3;
        tokenStart = p + 2;
        continue;
      }
      /* Character must be continuation of the token */
      continue;
    }
    if (mode == 2 || mode == 3) {
      /* If character is a quote, end quote and change mode */
      if (line[p] == '\'' && mode == 2) {
        mode = 0;
        pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
        nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, tokenStart));
                                                          /* Save token start */
        let((vstring *)(&rawArgPntr[rawArgs]), seg(line,tokenStart,p));
        rawArgs++;
        continue;
      }
      if (line[p] == '\"' && mode == 3) {
        mode = 0;
        pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
        nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, tokenStart));
                                                          /* Save token start */
        let((vstring *)(&rawArgPntr[rawArgs]),seg(line,tokenStart,p));
        rawArgs++;
        continue;
      }
      /* Character must be continuation of quoted token */
      continue;
    }
  }

  /* Finished scanning the line.  Finish processing last token. */
  if (mode != 0) {
    pntrLet(&rawArgPntr, pntrAddElement(rawArgPntr));
    nmbrLet(&rawArgNmbr, nmbrAddElement(rawArgNmbr, tokenStart));
                                                          /* Save token start */
    let((vstring *)(&rawArgPntr[rawArgs]),seg(line,tokenStart,p));
    rawArgs++;
  }

  /* Add length of command line prompt to each argument, to
     align the error message pointer */
  for (i = 0; i < rawArgs; i++) {
    rawArgNmbr[i] = rawArgNmbr[i] + len(commandPrompt);
  }

  /* Deallocate */
  let(&specialOneCharTokens, "");
}


flag lastArgMatches(vstring argString)
{
  /* This functions checks to see if the last field was argString */
  if (!strcmp(argString,fullArg[pntrLen(fullArg)-1])) {
    return (1);
  } else {
    return (0);
  }
}

flag cmdMatches(vstring cmdString)
{
  /* This function checks that fields 0 through n of fullArg match
     cmdString (separated by spaces). */
  long i, j, k;
  vstring tmpStr = "";
  /* Count the number of spaces */
  k = len(cmdString);
  j = 0;
  for (i = 0; i < k; i++) {
    if (cmdString[i] == ' ') j++;
  }
  k = pntrLen(fullArg);
  for (i = 0; i <= j; i++) {
    if (j >= k) {
      /* Command to match is longer than the user's command; assume no match */
      let(&tmpStr,"");
      return (0);
    }
    let(&tmpStr,cat(tmpStr," ",fullArg[i],NULL));
  }
  if (!strcmp(cat(" ",cmdString,NULL),tmpStr)) {
    let(&tmpStr,"");
    return (1);
  } else {
    let(&tmpStr,"");
    return (0);
  }
}

long switchPos(vstring swString)
{
  /* This function checks that fields i through j of fullArg match
     swString (separated by spaces).  The first character of swString
     should be "/" and must be separated from the first field
     of swString with a space.  The position of the "/" in fullArg
     is returned if swString is there, otherwise 0 is returned (the first
     position in fullArg is considered 1, not 0). */
  /* Example:  if fullArg (combined into one string) is
     "DISPLAY PROOF / UNKNOWN / START_STEP = 10 / ESSENTIAL"
     and swString is "/ START_STEP", switchPos will return 5. */
  long i, j, k;
  vstring tmpStr = "";
  vstring swString1 = "";

  if (swString[0] != '/') bug(1108);

  /* Add a space after the "/" if there is none */
  if (swString[1] != ' ') {
    let(&swString1,cat("/ ",right(swString,2)," ",NULL));
  } else {
    let(&swString1,swString);
  }

  /* Build the complete command */
  k = pntrLen(fullArg);
  for (i = 0; i < k; i++) {
    let(&tmpStr, cat(tmpStr,fullArg[i]," ",NULL));
  }

  k = instr(1,tmpStr,swString1);
  if (!k) {
    let(&swString1,"");
    let(&tmpStr,"");
    return (0);
  }

  let(&tmpStr,left(tmpStr,k));
  /* Count the number of spaces - it will be the fullArg position */
  k = len(tmpStr);
  j = 0;
  for (i = 0; i < k; i++) {
    if (tmpStr[i] == ' ') j++;
  }
  let(&tmpStr,"");
  let(&swString1,"");
  return (j + 1);
}


void printCommandError(vstring line1, long arg, vstring errorMsg)
{
  /* Warning: errorMsg should not a temporarily allocated string such
     as the direct output of cat() */
  vstring errorPointer = "";
  vstring line = "";
  long column, tokenLength, j;

  let(&line,line1); /* Prevent deallocation in case line1 is
                       direct return from string function such as cat() */
  if (!line[0]) {
    /* Empty line - don't print an error pointer */
    print2("%s\n", errorMsg);
    let(&line, "");
    return;
  }
  column = rawArgNmbr[arg];
  tokenLength = len(rawArgPntr[arg]);
  for (j = 0; j < column - 1; j++) {
    /* Make sure that tabs on the line with the error are accounted for so
       that the error pointer lines up correctly */
    if (j >= len(line)) bug(1109);
    if (line[j] == '\t') {
      let(&errorPointer,cat(errorPointer, "\t", NULL));
    } else {
      if (line[j] == '\n') {
        let(&errorPointer, "");
      } else {
        let(&errorPointer, cat(errorPointer, " ", NULL));
      }
    }
  }
  for (j = 0; j < tokenLength; j++)
    let(&errorPointer,cat(errorPointer, "^", NULL));
  print2("%s\n", errorPointer);
  printLongLine(errorMsg, "", " ");
  let(&errorPointer, "");
  let(&line, "");
}

