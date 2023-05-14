/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* Command line syntax specification for Metamath */

#include <string.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmdl.h"
#include "mminou.h"
#include "mmpfas.h"
#include "mmunif.h" /* For g_hentyFilter, g_userMaxUnifTrials, g_minSubstLen */
#include "mmwtex.h"
#include "mmword.h"

/* Global variables */
pntrString_def(g_rawArgPntr);
nmbrString_def(g_rawArgNmbr);
long g_rawArgs = 0;
pntrString_def(g_fullArg);
vstring_def(g_fullArgString); /* g_fullArg as one string */
vstring_def(g_commandPrompt);
vstring_def(g_commandLine);
long g_showStatement = 0;
vstring_def(g_logFileName);
vstring_def(g_texFileName);
flag g_PFASmode = 0; /* Proof assistant mode, invoked by PROVE command */
flag g_queryMode = 0; /* If 1, explicit questions will be asked even if
    a field in the input command line is optional */
flag g_sourceChanged = 0; /* Flag that user made some change to the source file*/
flag g_proofChanged = 0; /* Flag that user made some change to proof in progress*/
flag g_commandEcho = 0; /* Echo full command */
flag g_memoryStatus = 0; /* Always show memory */
flag g_sourceHasBeenRead = 0; /* 1 if a source file has been read in */
vstring_def(g_rootDirectory); /* Directory prefix to use for included files */

static flag getFullArg(long arg, const char *cmdList);

flag processCommandLine(void) {
  vstring_def(defaultArg);
  vstring_def(tmpStr);
  long i;
  g_queryMode = 0; /* If 1, explicit questions will be asked even if
    a field is optional */
  free_pntrString(g_fullArg);

  if (!g_toolsMode) {

    if (!g_PFASmode) {
      /* Normal mode */
      let(&tmpStr, cat("DBG|",
          "HELP|READ|WRITE|PROVE|SHOW|SEARCH|SAVE|SUBMIT|OPEN|CLOSE|",
          "SET|FILE|BEEP|EXIT|QUIT|ERASE|VERIFY|MARKUP|MORE|TOOLS|",
          "MIDI|<HELP>",
          NULL));
    } else {
      /* Proof assistant mode */
      let(&tmpStr, cat("DBG|",
          "HELP|WRITE|SHOW|SEARCH|SAVE|SUBMIT|OPEN|CLOSE|",
          "SET|FILE|BEEP|EXIT|_EXIT_PA|QUIT|VERIFY|INITIALIZE|ASSIGN|REPLACE|",
          "LET|UNIFY|IMPROVE|MINIMIZE_WITH|EXPAND|MATCH|DELETE|UNDO|REDO|",
          "MARKUP|MORE|TOOLS|MIDI|<HELP>",
          NULL));
    }
    if (!getFullArg(0, tmpStr)) {
      goto pclbad;
    }

    if (cmdMatches("HELP")) {
      if (!getFullArg(1, cat("LANGUAGE|PROOF_ASSISTANT|MM-PA|",
          "BEEP|EXIT|QUIT|READ|ERASE|",
          "OPEN|CLOSE|SHOW|SEARCH|SET|VERIFY|SUBMIT|SYSTEM|PROVE|FILE|WRITE|",
          "MARKUP|ASSIGN|REPLACE|MATCH|UNIFY|LET|INITIALIZE|DELETE|IMPROVE|",
          "MINIMIZE_WITH|EXPAND|UNDO|REDO|SAVE|DEMO|INVOKE|CLI|EXPLORE|TEX|",
          "LATEX|HTML|COMMENTS|BIBLIOGRAPHY|MORE|",
          "TOOLS|MIDI|$|<$>", NULL))) goto pclbad;
      if (cmdMatches("HELP OPEN")) {
        if (!getFullArg(2, "LOG|TEX|<LOG>")) goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP CLOSE")) {
        if (!getFullArg(2, "LOG|TEX|<LOG>")) goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP SHOW")) {
        if (!getFullArg(2, cat("MEMORY|SETTINGS|LABELS|SOURCE|STATEMENT|",
            "PROOF|NEW_PROOF|USAGE|TRACE_BACK|ELAPSED_TIME|",
            "DISCOURAGED|<MEMORY>",
            NULL)))
            goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP SET")) {
        if (!getFullArg(2, cat(
            "ECHO|SCROLL|WIDTH|HEIGHT|UNDO|UNIFICATION_TIMEOUT|",
            "DISCOURAGEMENT|",
            "CONTRIBUTOR|",
            "ROOT_DIRECTORY|",
            "EMPTY_SUBSTITUTION|SEARCH_LIMIT|JEREMY_HENTY_FILTER|<ECHO>",
            NULL)))
            goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP VERIFY")) {
        if (!getFullArg(2, "PROOF|MARKUP|<PROOF>"))
            goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP WRITE")) {
        if (!getFullArg(2,
            "SOURCE|THEOREM_LIST|BIBLIOGRAPHY|RECENT_ADDITIONS|<SOURCE>"))
            goto pclbad;
        goto pclgood;
      }
      if (cmdMatches("HELP FILE")) {
        if (!getFullArg(2, "SEARCH"))
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
    } /* cmdMatches("HELP") */

    if (cmdMatches("READ")) {
      if (!getFullArg(1, "& What is the name of the source input file? "))
          goto pclbad;
      /* Get any switches */
      i = 1;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, "VERIFY|<VERIFY>")) goto pclbad;
        } else {
          break;
        }
        break; /* Break if only 1 switch is allowed */
      } /* End while for switch loop */
      goto pclgood;
    }

    if (cmdMatches("WRITE")) {
      if (!getFullArg(1,
          "SOURCE|THEOREM_LIST|BIBLIOGRAPHY|RECENT_ADDITIONS|<SOURCE>"))
        goto pclbad;
      if (cmdMatches("WRITE SOURCE")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2, cat(
            "* What is the name of the source output file <",
            g_input_fn, ">? ", NULL)))
          goto pclbad;
        if (!strcmp(g_input_fn, g_fullArg[2])) {
          print2("The input file will be renamed %s~1.\n", g_input_fn);
        }

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "FORMAT|REWRAP",
                "|SPLIT|NO_VERSIONING|KEEP_INCLUDES|EXTRACT",
                "|<REWRAP>", NULL)))
              goto pclbad;
            if (lastArgMatches("EXTRACT")) {
              i++;
              if (!getFullArg(i, "* What statement label? "))
                goto pclbad;
            }
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        } /* End while for switch loop */

        goto pclgood;
      }
      if (cmdMatches("WRITE THEOREM_LIST")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        /* Get any switches */
        i = 1;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "THEOREMS_PER_PAGE|SHOW_LEMMAS|HTML|ALT_HTML|NO_VERSIONING",
                "|<THEOREMS_PER_PAGE>", NULL)))
              goto pclbad;
            if (lastArgMatches("THEOREMS_PER_PAGE")) {
              i++;
              if (!getFullArg(i, "# How many theorems per page <100>? "))
                goto pclbad;
            }
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      }
      if (cmdMatches("WRITE BIBLIOGRAPHY")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2, cat(
            "* What is the bibliography HTML input/output file <",
            "mmbiblio.html", ">? ", NULL)))
          goto pclbad;
        print2(
          "The old file will be renamed %s~1.\n", g_fullArg[2]);
        goto pclgood;
      }
      if (cmdMatches("WRITE RECENT_ADDITIONS")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2, cat(
            "* What is the Recent Additions HTML input/output file <",
            "mmrecent.html", ">? ", NULL)))
          goto pclbad;
        print2(
          "The old file will be renamed %s~1.\n", g_fullArg[2]);

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "LIMIT|HTML|ALT_HTML",
                "|<LIMIT>", NULL)))
              goto pclbad;
            if (lastArgMatches("LIMIT")) {
              i++;
              if (!getFullArg(i, "# How many most recent theorems <100>? "))
                goto pclbad;
            }
          } else {
            break;
          }
          /*break;*/ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      }
    }

    if (cmdMatches("OPEN")) {
      if (!getFullArg(1, "LOG|TEX|<LOG>")) goto pclbad;
      if (cmdMatches("OPEN LOG")) {
        if (g_logFileOpenFlag) {
          printLongLine(cat(
              "?Sorry, the log file \"", g_logFileName, "\" is currently open.  ",
              "Type CLOSE LOG to close the current log if you want to open another one.",
              NULL), "", " ");
          goto pclbad;
        }
        if (!getFullArg(2, "* What is the name of logging output file? "))
          goto pclbad;
      }
      if (cmdMatches("OPEN TEX")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (g_texFileOpenFlag) {
          printLongLine(cat(
              "?Sorry, the LaTeX file \"", g_texFileName, "\" is currently open.  ",
              "Type CLOSE TEX to close the current LaTeX file",
              " if you want to open another one."
              , NULL), "", " ");
          goto pclbad;
        }
        if (!getFullArg(2, "* What is the name of LaTeX output file? "))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "NO_HEADER|OLD_TEX|<NO_HEADER>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        } /* End while for switch loop */
      }
      goto pclgood;
    }

    if (cmdMatches("CLOSE")) {
      if (!getFullArg(1, "LOG|TEX|<LOG>")) goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("FILE")) {
      if (!getFullArg(1, cat("SEARCH", NULL))) goto pclbad;

      if (cmdMatches("FILE SEARCH")) {
        if (!getFullArg(2, "& What is the name of the file to search? "))
          goto pclbad;
        if (!getFullArg(3, "* What is the string to search for? "))
          goto pclbad;

        /* Get any switches */
        i = 3;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (i == 4) {
              if (!getFullArg(i, cat(
                  "FROM_LINE|TO_LINE|<FROM_LINE>", NULL)))
                goto pclbad;
            } else {
              if (!getFullArg(i, cat(
                  "FROM_LINE|TO_LINE|<TO_LINE>", NULL)))
                goto pclbad;
            }
            if (lastArgMatches("FROM_LINE")) {
              i++;
              if (!getFullArg(i, "# From what line number <1>? "))
                goto pclbad;
            }
            if (lastArgMatches("TO_LINE")) {
              i++;
              if (!getFullArg(i, "# To what line number <999999>? "))
                goto pclbad;
            }
            if (lastArgMatches("WINDOW")) { /* ???Not implemented yet */
              i++;
              if (!getFullArg(i, "# How big a window around matched lines <0>? "))
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
      if (!g_PFASmode) {
        if (!getFullArg(1, cat(
     "SETTINGS|LABELS|STATEMENT|SOURCE|PROOF|MEMORY|TRACE_BACK|",
     "USAGE|ELAPSED_TIME|DISCOURAGED|<SETTINGS>", NULL)))
            goto pclbad;
      } else {
        if (!getFullArg(1, cat("NEW_PROOF|",
     "SETTINGS|LABELS|STATEMENT|SOURCE|PROOF|MEMORY|TRACE_BACK|",
     "USAGE|ELAPSED_TIME|DISCOURAGED|<SETTINGS>",
            NULL)))
            goto pclbad;
      }
      if (g_showStatement) {
        if (g_showStatement < 1 || g_showStatement > g_statements) bug(1110);
        let(&defaultArg, cat(" <",g_Statement[g_showStatement].labelName, ">",
            NULL));
      } else {
        let(&defaultArg, "");
      }

      if (cmdMatches("SHOW TRACE_BACK")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label", defaultArg, "? ", NULL)))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "ALL|ESSENTIAL|AXIOMS|TREE|DEPTH|COUNT_STEPS|MATCH|TO",
                "|<ALL>", NULL)))
              goto pclbad;
            if (lastArgMatches("DEPTH")) {
              i++;
              if (!getFullArg(i, "# How many indentation levels <999>? "))
                goto pclbad;
            }
            if (lastArgMatches("MATCH")) {
              i++;
              if (!getFullArg(i, "* What statement label? "))
                goto pclbad;
            }
            if (lastArgMatches("TO")) {
              i++;
              if (!getFullArg(i, "* What statement label? "))
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
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label", defaultArg, "? ", NULL)))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "DIRECT|RECURSIVE|ALL",
                "|<DIRECT>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          /* break; */  /* Break if only 1 switch is allowed */
        }

        goto pclgood;
      } /* End if (cmdMatches("SHOW USAGE")) */

      if (cmdMatches("SHOW LABELS")) {
        if (g_sourceHasBeenRead == 0) {
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
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat("ALL|LINEAR|<ALL>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          /*break;*/ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      }
      if (cmdMatches("SHOW STATEMENT")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label", defaultArg, "? ", NULL)))
          goto pclbad;
        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "FULL|COMMENT|TEX|OLD_TEX|HTML|ALT_HTML|TIME|BRIEF_HTML",
                "|BRIEF_ALT_HTML|MNEMONICS|NO_VERSIONING|<FULL>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      }
      if (cmdMatches("SHOW SOURCE")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label", defaultArg, "? ", NULL))) {
          goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SHOW PROOF")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label", defaultArg, "? ", NULL)))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "ESSENTIAL|ALL|UNKNOWN|FROM_STEP|TO_STEP|DEPTH",
                "|REVERSE|VERBOSE|NORMAL|PACKED|COMPRESSED|EXPLICIT",
                "|FAST|OLD_COMPRESSION",
                "|STATEMENT_SUMMARY|DETAILED_STEP|TEX|OLD_TEX|HTML",
                "|LEMMON|START_COLUMN|NO_REPEATED_STEPS",
                "|RENUMBER|SIZE|<ESSENTIAL>", NULL)))
              goto pclbad;
            if (lastArgMatches("FROM_STEP")) {
              i++;
              if (!getFullArg(i, "# From what step <1>? "))
                goto pclbad;
            }
            if (lastArgMatches("TO_STEP")) {
              i++;
              if (!getFullArg(i, "# To what step <9999>? "))
                goto pclbad;
            }
            if (lastArgMatches("DEPTH")) {
              i++;
              if (!getFullArg(i, "# How many indentation levels <999>? "))
                goto pclbad;
            }
            if (lastArgMatches("DETAILED_STEP")) {
              i++;
              if (!getFullArg(i, "# Display details of what step <1>? "))
                goto pclbad;
            }
            if (lastArgMatches("START_COLUMN")) {
              i++;
              if (!getFullArg(i, cat(
                  "# At what column should the formula start <",
                  str((double)DEFAULT_COLUMN), ">? ", NULL)))
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
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }

        /* Get any switches */
        i = 1;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "ESSENTIAL|ALL|UNKNOWN|FROM_STEP|TO_STEP|DEPTH",
                "|REVERSE|VERBOSE|NORMAL|PACKED|COMPRESSED|EXPLICIT",
                "|OLD_COMPRESSION",
                "|NOT_UNIFIED|TEX|HTML",
                "|LEMMON|START_COLUMN|NO_REPEATED_STEPS",
                "|RENUMBER|<ESSENTIAL>", NULL)))
              goto pclbad;
            if (lastArgMatches("FROM_STEP")) {
              i++;
              if (!getFullArg(i, "# From what step <1>? "))
                goto pclbad;
            }
            if (lastArgMatches("TO_STEP")) {
              i++;
              if (!getFullArg(i, "# To what step <9999>? "))
                goto pclbad;
            }
            if (lastArgMatches("DEPTH")) {
              i++;
              if (!getFullArg(i, "# How many indentation levels <999>? "))
                goto pclbad;
            }
            if (lastArgMatches("START_COLUMN")) {
              i++;
              if (!getFullArg(i, cat(
                  "# At what column should the formula start <",
                  str((double)DEFAULT_COLUMN), ">? ", NULL)))
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
      if (g_sourceHasBeenRead == 0) {
        print2("?No source file has been read in.  Use READ first.\n");
        goto pclbad;
      }
      if (!getFullArg(1,
          "* What are the labels to match (* = wildcard) <*>?"))
        goto pclbad;
      if (!getFullArg(2, "* Search for what math symbol string? "))
          goto pclbad;
      /* Get any switches */
      i = 2;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, cat("ALL|COMMENTS|JOIN|<ALL>", NULL)))
            goto pclbad;
        } else {
          break;
        }
        /*break;*/ /* Break if only 1 switch is allowed */
      }
      goto pclgood;
    } /* End of SEARCH */

    if (cmdMatches("SAVE")) {
      if (!g_PFASmode) {
        if (!getFullArg(1,
            "PROOF|<PROOF>"))
            goto pclbad;
      } else {
        if (!getFullArg(1, cat("NEW_PROOF|",
            "PROOF|<NEW_PROOF>",
            NULL)))
            goto pclbad;
      }
      if (g_showStatement) {
        if (g_showStatement < 0) bug(1111);
        let(&defaultArg, cat(" <",g_Statement[g_showStatement].labelName, ">", NULL));
      } else {
        let(&defaultArg, "");
      }

      if (cmdMatches("SAVE PROOF")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }
        if (!getFullArg(2,
            cat("* What is the statement label", defaultArg, "? ", NULL)))
          goto pclbad;

        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "NORMAL|PACKED|COMPRESSED|EXPLICIT",
                "|FAST|OLD_COMPRESSION",
                "|TIME|<NORMAL>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          /* break; */ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      } /* End if (cmdMatches("SAVE PROOF")) */

      if (cmdMatches("SAVE NEW_PROOF")) {
        if (g_sourceHasBeenRead == 0) {
          print2("?No source file has been read in.  Use READ first.\n");
          goto pclbad;
        }

        /* Get any switches */
        i = 1;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "NORMAL|PACKED|COMPRESSED|EXPLICIT",
                "|OLD_COMPRESSION|OVERRIDE",
                "|<NORMAL>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          /*break;*/ /* Break if only 1 switch is allowed */
        }
        goto pclgood;
      } /* End if (cmdMatches("SAVE NEW_PROOF")) */

      goto pclgood;
    } /* End of SAVE */

    if (cmdMatches("PROVE")) {
      if (g_sourceHasBeenRead == 0) {
        print2("?No source file has been read in.  Use READ first.\n");
        goto pclbad;
      }
      if (!g_proveStatement) g_proveStatement = g_showStatement;
      if (g_proveStatement) {
        let(&defaultArg, cat(" <",g_Statement[g_proveStatement].labelName, ">", NULL));
      } else {
        let(&defaultArg, "");
      }
      if (!getFullArg(1,
          cat("* What is the label of the statement you want to try proving",
          defaultArg, "? ", NULL)))
        goto pclbad;

      /* Get any switches */
      i = 1;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, "OVERRIDE|<OVERRIDE>")) goto pclbad;
        } else {
          break;
        }
        break; /* Break if only 1 switch is allowed */
      } /* End while for switch loop */

      goto pclgood;
    }

    /* Commands in Proof Assistant mode */

    if (cmdMatches("MATCH")) {
      if (!getFullArg(1,
          "STEP|ALL|<ALL>")) goto pclbad;
      if (cmdMatches("MATCH STEP")) {
        if (!getFullArg(2, "# What step number? ")) goto pclbad;
        /* Get any switches */
        i = 2;
        while (1) {
          i++;
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
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
            if (!getFullArg(i, cat(
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
          "STEP|ALL|USER|<ALL>")) goto pclbad;
      if (cmdMatches("INITIALIZE STEP")) {
        if (!getFullArg(2, "# What step number? ")) goto pclbad;
      }
      goto pclgood;
    }

    if (cmdMatches("IMPROVE")) {
      if (!getFullArg(1,
        "* What step number, or FIRST, or LAST, or ALL <ALL>? ")) goto pclbad;
      /* Get any switches */
      i = 1;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, "DEPTH|NO_DISTINCT|1|2|3"
            "|SUBPROOFS|OVERRIDE|INCLUDE_MATHBOXES|<DEPTH>")) goto pclbad;
          if (lastArgMatches("DEPTH")) {
            i++;
            if (!getFullArg(i, "# What is maximum depth for "
              "searching statements with $e hypotheses <0>? ")) goto pclbad;
          }
        } else {
          break;
        }
        /*break;*/ /* Do this if only 1 switch is allowed */
      } /* end while */
      goto pclgood;
    } /* end if IMPROVE */

    if (cmdMatches("MINIMIZE_WITH")) {
      if (!getFullArg(1, "* What statement label? ")) goto pclbad;
      /* Get any switches */
      i = 1;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, cat(
              "VERBOSE|MAY_GROW|EXCEPT|OVERRIDE|INCLUDE_MATHBOXES|",
              "ALLOW_NEW_AXIOMS|NO_NEW_AXIOMS_FROM|FORBID|TIME|<VERBOSE>",
              NULL)))
            goto pclbad;

          if (lastArgMatches("EXCEPT")) {
            i++;
            if (!getFullArg(i, "* What statement label match pattern? "))
              goto pclbad;
          }
          if (lastArgMatches("ALLOW_NEW_AXIOMS")) {
            i++;
            if (!getFullArg(i, "* What statement label match pattern? "))
              goto pclbad;
          }
          if (lastArgMatches("NO_NEW_AXIOMS_FROM")) {
            i++;
            if (!getFullArg(i, "* What statement label match pattern? "))
              goto pclbad;
          }
          if (lastArgMatches("FORBID")) {
            i++;
            if (!getFullArg(i, "* What statement label match pattern? "))
              goto pclbad;
          }
        } else {
          break;
        }
        /*break;*/  /* Break if only 1 switch is allowed */
      }
      goto pclgood;
    } /* end of MINIMIZE_WITH */

    if (cmdMatches("EXPAND")) {
      if (!getFullArg(1, "* What statement label? ")) goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("UNIFY")) {
      if (!getFullArg(1,
          "STEP|ALL|<ALL>")) goto pclbad;
      if (cmdMatches("UNIFY STEP")) {
        if (!getFullArg(2, "# What step number? ")) goto pclbad;
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
            if (!getFullArg(i, cat(
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
        if (!getFullArg(2, "# What step number? ")) goto pclbad;
        goto pclgood;
      }
      goto pclgood;
    }

    /*???OBSOLETE???*/
    if (cmdMatches("ADD")) {
      if (!getFullArg(1,
          "UNIVERSE|<UNIVERSE>")) goto pclbad;
      /* Note:  further parsing below */
    }

    if (cmdMatches("REPLACE")) {
      if (!getFullArg(1, "* What step number, or FIRST, or LAST <LAST>? "))
          goto pclbad;
      if (!getFullArg(2, "* With what statement label? ")) goto pclbad;
      /* Get any switches */
      i = 2;

      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, cat(
              "OVERRIDE|<OVERRIDE>", NULL)))
            goto pclbad;
        } else {
          break;
        }
        break; /* Break if only 1 switch is allowed */
      }

      goto pclgood;
    }

    if (cmdMatches("LET")) {
      if (!getFullArg(1, "STEP|VARIABLE|<STEP>")) goto pclbad;
      if (cmdMatches("LET STEP")) {
        if (!getFullArg(2, "* What step number, or FIRST, or LAST <LAST>? "))
          goto pclbad;
      }
      if (cmdMatches("LET VARIABLE")) {
        if (!getFullArg(2, "* Assign what variable (format $nn)? ")) goto pclbad;
      }
      if (!getFullArg(3, "=|<=>")) goto pclbad;
      if (!getFullArg(4, "* With what math symbol string? "))
          goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("ASSIGN")) {
      if (!getFullArg(1, "* What step number, or FIRST, or LAST <LAST>? ")) goto pclbad;
      if (!getFullArg(2, "* With what statement label? ")) goto pclbad;
      /* Get any switches */
      i = 2;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, "NO_UNIFY|OVERRIDE|<NO_UNIFY>")) goto pclbad;
        } else {
          break;
        }
        /*break;*/  /* Break if only 1 switch is allowed */
      }
      goto pclgood;
    }

    if (cmdMatches("UNDO")) {
      goto pclgood;
    }

    if (cmdMatches("REDO")) {
      goto pclgood;
    }

    if (cmdMatches("SET")) {
      let(&tmpStr, cat(
          "WIDTH|HEIGHT|UNDO|ECHO|SCROLL|",
          "DEBUG|MEMORY_STATUS|SEARCH_LIMIT|UNIFICATION_TIMEOUT|",
          "DISCOURAGEMENT|",
          "CONTRIBUTOR|",
          "ROOT_DIRECTORY|",
          "EMPTY_SUBSTITUTION|JEREMY_HENTY_FILTER|<WIDTH>", NULL));
      if (!getFullArg(1,tmpStr)) goto pclbad;
      if (cmdMatches("SET DEBUG")) {
        if (!getFullArg(2, "FLAG|OFF|<OFF>")) goto pclbad;
        if (lastArgMatches("FLAG")) {
          if (!getFullArg(3, "4|5|6|7|8|9|<5>")) goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SET ECHO")) {
        if (g_commandEcho) {
          if (!getFullArg(2, "ON|OFF|<OFF>")) goto pclbad;
        } else {
          if (!getFullArg(2, "ON|OFF|<ON>")) goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SET SCROLL")) {
        if (g_scrollMode == 1) {
          if (!getFullArg(2, "CONTINUOUS|PROMPTED|<CONTINUOUS>")) goto pclbad;
        } else {
          if (!getFullArg(2, "CONTINUOUS|PROMPTED|<PROMPTED>")) goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SET DISCOURAGEMENT")) {
        if (g_globalDiscouragement) {
          if (!getFullArg(2, "ON|OFF|<OFF>")) goto pclbad;
        } else {
          if (!getFullArg(2, "ON|OFF|<ON>")) goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SET MEMORY_STATUS")) {
        if (g_memoryStatus) {
          if (!getFullArg(2, "ON|OFF|<OFF>")) goto pclbad;
        } else {
          if (!getFullArg(2, "ON|OFF|<ON>")) goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SET JEREMY_HENTY_FILTER")) {
        if (g_hentyFilter) {
          if (!getFullArg(2, "ON|OFF|<OFF>")) goto pclbad;
        } else {
          if (!getFullArg(2, "ON|OFF|<ON>")) goto pclbad;
        }
        goto pclgood;
      }

      if (cmdMatches("SET CONTRIBUTOR")) {
        if (!getFullArg(2, cat(
            "* What is the contributor name for SAVE (NEW_)PROOF <",
            g_contributorName, ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET ROOT_DIRECTORY")) {
        if (!getFullArg(2, cat(
            "* What is the root directory path (use space if none) <",
            g_rootDirectory, ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET SEARCH_LIMIT")) {
        if (!getFullArg(2, cat(
            "# What is search limit for IMPROVE command <",
            str((double)g_userMaxProveFloat), ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET UNIFICATION_TIMEOUT")) {
        if (!getFullArg(2, cat(
           "# What is maximum number of unification trials <",
            str((double)g_userMaxUnifTrials), ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET WIDTH")) {
        if (!getFullArg(2, cat(
           "# What is maximum line length on your screen <",
            str((double)g_screenHeight), ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET HEIGHT")) {
        if (!getFullArg(2, cat(
           "# What is number of lines your screen displays <",
            str((double)g_screenHeight), ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET UNDO")) {
        if (!getFullArg(2, cat(
           "# What is the maximum number of UNDOs <",
            str((double)(processUndoStack(NULL, PUS_GET_SIZE, "", 0))),
            ">? ", NULL)))
          goto pclbad;
        goto pclgood;
      }

      if (cmdMatches("SET EMPTY_SUBSTITUTION")) {
        if (g_minSubstLen == 0) {
          if (!getFullArg(2, "ON|OFF|<OFF>")) goto pclbad;
        } else {
          if (!getFullArg(2, "ON|OFF|<ON>")) goto pclbad;
        }
        goto pclgood;
      }
    } /* end if SET */

    if (cmdMatches("ERASE")) {
      goto pclgood;
    }

    if (cmdMatches("MORE")) {
      if (!getFullArg(1,
         "* What is the name of the file to display? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("TOOLS")) {
      goto pclgood;
    }

    if (cmdMatches("VERIFY")) {
      if (!getFullArg(1,
          "PROOF|MARKUP|<PROOF>"))
        goto pclbad;
      if (cmdMatches("VERIFY PROOF")) {
        if (g_sourceHasBeenRead == 0) {
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
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "SYNTAX_ONLY",
                "|<SYNTAX_ONLY>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          break;  /* Break if only 1 switch is allowed */
        }

        goto pclgood;
      }

      if (cmdMatches("VERIFY MARKUP")) {
        if (g_sourceHasBeenRead == 0) {
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
          if (!getFullArg(i, "/|$|<$>")) goto pclbad;
          if (lastArgMatches("/")) {
            i++;
            if (!getFullArg(i, cat(
                "DATE_SKIP|FILE_SKIP|TOP_DATE_SKIP|VERBOSE",
                "|FILE_CHECK|TOP_DATE_CHECK",
                "|UNDERSCORE_SKIP|MATHBOX_SKIP|<DATE_SKIP>", NULL)))
              goto pclbad;
          } else {
            break;
          }
          /* break; */  /* Break if only 1 switch is allowed */
        }

        goto pclgood;
      }
    }

    if (cmdMatches("DBG")) {
      /* The debug command fetches an arbitrary 2nd arg in quotes, to be handled
         in whatever way is needed for debugging. */
      if (!getFullArg(1, "* What is the debugging string? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("MARKUP")) {
      if (g_sourceHasBeenRead == 0) {
        print2("?No source file has been read in.  Use READ first.\n");
        goto pclbad;
      }
      if (!getFullArg(1,
          "* What is the name of the input file with markup? "))
        goto pclbad;
      if (!getFullArg(2,
          "* What is the name of the HTML output file? "))
        goto pclbad;

      /* Get any switches */
      i = 2;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, cat(
              "HTML|ALT_HTML|SYMBOLS|LABELS|NUMBER_AFTER_LABEL|BIB_REFS",
              "|UNDERSCORES|CSS|<ALT_HTML>", NULL)))
            goto pclbad;
        } else {
          break;
        }
        /*break;*/ /* Break if only 1 switch is allowed */
      }
      goto pclgood;
    }

    if (cmdMatches("MIDI")) {
      if (g_sourceHasBeenRead == 0) {
        print2("?No source file has been read in.  Use READ first.\n");
        goto pclbad;
      }
      if (!getFullArg(1,
         "* Statement label to create MIDI for (* matches any substring) <*>?"))
        goto pclbad;
      /* Get any switches */
      i = 1;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, cat("PARAMETER|<PARAMETER>", NULL)))
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

    if (cmdMatches("EXIT") || cmdMatches("QUIT") || cmdMatches("_EXIT_PA")) {

      /* Get any switches */
      i = 0;
      while (1) {
        i++;
        if (!getFullArg(i, "/|$|<$>")) goto pclbad;
        if (lastArgMatches("/")) {
          i++;
          if (!getFullArg(i, cat(
              "FORCE|<FORCE>", NULL)))
            goto pclbad;
        } else {
          break;
        }
        break; /* Break if only 1 switch is allowed */
      } /* End while for switch loop */

      goto pclgood;
    }
  } else { /* g_toolsMode */
    /* Text tools mode */
    let(&tmpStr, cat(
          "HELP|SUBMIT|",
          "ADD|DELETE|SUBSTITUTE|S|SWAP|CLEAN|INSERT|BREAK|BUILD|MATCH|SORT|",
          "UNDUPLICATE|DUPLICATE|UNIQUE|REVERSE|RIGHT|PARALLEL|NUMBER|COUNT|",
          "COPY|C|TYPE|T|TAG|UPDATE|BEEP|B|EXIT|QUIT|<HELP>", NULL));
    if (!getFullArg(0, tmpStr))
      goto pclbad;

    if (cmdMatches("HELP")) {
      if (!getFullArg(1, cat(
          "ADD|DELETE|SUBSTITUTE|S|SWAP|CLEAN|INSERT|BREAK|BUILD|MATCH|SORT|",
          "UNDUPLICATE|DUPLICATE|UNIQUE|REVERSE|RIGHT|PARALLEL|NUMBER|COUNT|",
          "TYPE|T|TAG|UPDATE|BEEP|B|EXIT|QUIT|",
          "COPY|C|SUBMIT|SYSTEM|CLI|",
          "$|<$>", NULL))) goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("ADD") || cmdMatches("TAG")) {
      if (!getFullArg(1, "& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2, "* String to add to beginning of each line <>? "))
        goto pclbad;
      if (!getFullArg(3, "* String to add to end of each line <>? "))
        goto pclbad;
      if (cmdMatches("TAG")) {
        if (!getFullArg(4,
            "* String to match to start range (null = any line) <>? "))
          goto pclbad;
        if (!getFullArg(5,
            "# Which occurrence of start match to start range <1>? "))
          goto pclbad;
        if (!getFullArg(6,
            "* String to match to end range (null = any line) <>? "))
          goto pclbad;
        if (!getFullArg(7,
            "# Which occurrence of end match to end range <1>? "))
          goto pclbad;
      }
      goto pclgood;
    }
    if (cmdMatches("DELETE")) {
      if (!getFullArg(1, "& Input/output file? "))
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
      if (!getFullArg(1, "& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
          "* Subcommand(s) (D,B,E,R,Q,T,U,P,G,C,L,V) <B,E,R>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("SWAP")) {
      if (!getFullArg(1, "& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
"* Character string to match between the halves to be swapped? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("SUBSTITUTE") || cmdMatches("S")) {
      if (!getFullArg(1, "& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2, "* String to replace? "))
        goto pclbad;
      if (!getFullArg(3, "* Replace it with <>? "))
        goto pclbad;
      if (!getFullArg(4,
"* Which occurrence in the line (1,2,... or ALL or EACH) <1>? "))
        goto pclbad;
      if (!getFullArg(5,
"* Additional match required on line (null = match all) <>? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("INSERT")) {
      if (!getFullArg(1, "& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2, "* String to insert in each line <!>? "))
        goto pclbad;
      if (!getFullArg(3, "# Column at which to insert the string <1>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("BREAK")) {
      if (!getFullArg(1, "& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
          "* Special characters to use as token delimiters <()[],=:;{}>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("MATCH")) {
      if (!getFullArg(1, "& Input/output file? "))
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
      if (!getFullArg(1, "& Input/output file? "))
        goto pclbad;
      if (!getFullArg(2,
          "* String to start key on each line (null string = column 1) <>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("UNDUPLICATE") || cmdMatches("DUPLICATE") ||
        cmdMatches("UNIQUE") || cmdMatches("REVERSE") || cmdMatches("BUILD")
        || cmdMatches("RIGHT")) {
      if (!getFullArg(1, "& Input/output file? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("COUNT")) {
      if (!getFullArg(1, "& Input file? "))
        goto pclbad;
      if (!getFullArg(2,
"* String to count <;>? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("COPY") || cmdMatches("C")) {
      if (!getFullArg(1, "* Comma-separated list of input files? "))
        goto pclbad;
      if (!getFullArg(2, "* Output file? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("NUMBER")) {
      if (!getFullArg(1, "* Output file <n.tmp>? "))
        goto pclbad;
      if (!getFullArg(2, "# First number <1>? "))
        goto pclbad;
      if (!getFullArg(3, "# Last number <10>? "))
        goto pclbad;
      if (!getFullArg(4, "# Increment <1>? "))
        goto pclbad;
      goto pclgood;
    }
    if (cmdMatches("TYPE") || cmdMatches("T")) {
      if (!getFullArg(1, "& File to display on the screen? "))
        goto pclbad;
      if (!getFullArg(2, "* Num. lines to type or ALL (nothing = 10) <$>? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("UPDATE")) {
      print2("Warning: Do not comment out code - delete it before running UPDATE!  If\n");
      print2("rerunning UPDATE, do not tamper with \"start/end of deleted section\" comments!\n");
      print2("Edit out tag on header comment line!  Review the output file!\n");
      if (!getFullArg(1, "& Original (reference) program input file? "))
        goto pclbad;
      if (!getFullArg(2, "& Edited program input file? "))
        goto pclbad;
      if (!getFullArg(3, cat("* Edited program output file with revisions tagged <",
          g_fullArg[2], ">? ", NULL)))
        goto pclbad;
      if (!strcmp(g_fullArg[2], g_fullArg[3])) {
        print2("The input file will be renamed %s~1.\n", g_fullArg[2]);
      }
      if (!getFullArg(4,
          cat("* Revision tag for added lines </* #",
          str((double)(highestRevision(g_fullArg[1]) + 1)), " */>? ", NULL)))
        goto pclbad;
      if (!getFullArg(5, "# Successive lines required for match (more = better sync) <3>? "))
        goto pclbad;
      goto pclgood;
    }

    if (cmdMatches("PARALLEL")) {
      if (!getFullArg(1, "& Left file? "))
        goto pclbad;
      if (!getFullArg(2, "& Right file? "))
        goto pclbad;
      if (!getFullArg(3, cat("* Output file <",
          g_fullArg[1], ">? ", NULL)))
        goto pclbad;
      if (!getFullArg(4,
          cat("* String to insert between the 2 input lines <>? ", NULL)))
        goto pclbad;
      goto pclgood;
    }

    /* g_toolsMode - no qualifiers for EXIT */
    if (cmdMatches("EXIT") || cmdMatches("QUIT")) {
      goto pclgood;
    }
  } /* if !g_toolsMode ... else ... */

  if (cmdMatches("SUBMIT")) {
    if (g_toolsMode) {
      let(&tmpStr, " <tools.cmd>");
    } else {
      let(&tmpStr, " <mm.cmd>");
    }
    if (!getFullArg(1, cat("& What is the name of command file to run",
        tmpStr, "? ", NULL))) {
      goto pclbad;
    }

    /* Get any switches */
    i = 1; /* Number of command words before switch */
    while (1) {
      i++;
      if (!getFullArg(i, "/|$|<$>")) goto pclbad;
      if (lastArgMatches("/")) {
        i++;
        if (!getFullArg(i, cat(
            "SILENT",
            "|<SILENT>", NULL)))
          goto pclbad;
      } else {
        break;
      }
      break; /* Break if only 1 switch is allowed */
    } /* End while for switch loop */

    goto pclgood;
  }

  if (cmdMatches("BEEP") || cmdMatches("B")) {
    goto pclgood;
  }

  /* Command in master list but not intercepted -- really a bug */
  print2("?This command has not been implemented yet.\n");
  print2("(This is really a bug--please report it.)\n");
  goto pclbad;

  /* Should never get here */

 pclgood:
  /* Strip off the last g_fullArg if a null argument was added by getFullArg
     in the case when "$" (nothing) is allowed */
  if (!strcmp(g_fullArg[pntrLen(g_fullArg) - 1], chr(3))) {
    free_vstring(*(vstring *)(&g_fullArg[pntrLen(g_fullArg) - 1])); /* Deallocate */
    pntrLet(&g_fullArg, pntrLeft(g_fullArg, pntrLen(g_fullArg) - 1));
  }

  if (pntrLen(g_fullArg) > g_rawArgs) bug(1102);
  if (pntrLen(g_fullArg) < g_rawArgs) {
    let(&tmpStr, cat("?Too many arguments.  Use quotes around arguments with special",
        " characters and around Unix file names with \"/\"s.", NULL));
    printCommandError(cat(g_commandPrompt, g_commandLine, NULL), pntrLen(g_fullArg),
        tmpStr);
    goto pclbad;
  }

  /* Create a single string containing the g_fullArg tokens */
  let(&g_fullArgString, "");
  for (i = 0; i < pntrLen(g_fullArg); i++) {
    let(&g_fullArgString, cat(g_fullArgString, " ", g_fullArg[i], NULL));
  }
  let(&g_fullArgString, right(g_fullArgString, 2)); /* Strip leading space */

  /* Deallocate memory */
  free_vstring(defaultArg);
  free_vstring(tmpStr);
  return 1;

 pclbad:
  /* Deallocate memory */
  free_vstring(defaultArg);
  free_vstring(tmpStr);
  return 0;
} /* processCommandLine */

/* This function converts the user's abbreviated keyword in
   g_rawArgPntr[arg] to a full, upper-case keyword,
   in g_fullArg[arg], matching
   the available choices in cmdList. */
/* Special cases:  cmdList = "# xxx <yyy>?" - get an integer */
/*                 cmdList = "* xxx <yyy>?" - get any string;
                     don't convert to upper case
                   cmdList = "& xxx <yyy>?" - same as * except
                     verify it is a file that exists */
/* "$" means a null argument is acceptable; put it in as
   special character chr(3) so it can be recognized */
static flag getFullArg(long arg, const char *cmdList1) {
  pntrString_def(possCmd);
  flag ret = 1;
  vstring_def(defaultCmd);
  vstring_def(infoStr);
  vstring_def(errorLine);
  vstring_def(keyword);

  vstring_def(cmdList);
  let(&cmdList, cmdList1); /* In case cmdList1 gets deallocated when it comes
                             directly from a vstring function such as cat() */

  let(&errorLine, cat(g_commandPrompt, g_commandLine, NULL));

  /* Handle special case - integer expected */
  if (cmdList[0] == '#') {
    let(&defaultCmd, seg(cmdList, instr(1, cmdList, "<"), instr(1, cmdList, ">")));

    /* If the argument has not been entered, prompt the user for it */
    if (g_rawArgs <= arg) {
      pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
      nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, 0));
      g_rawArgs++;
      if (g_rawArgs <= arg) bug(1103);

      g_queryMode = 1;
      vstring argLine = cmdInput1(right(cmdList, 3));
      let(&errorLine, right(cmdList, 3));
      if (argLine[0] == 0) { /* Use default argument */
        let(&argLine, seg(defaultCmd, 2, len(defaultCmd) - 1));
      }
      let((vstring *)(&g_rawArgPntr[arg]), argLine);
      free_vstring(argLine);
      g_rawArgNmbr[arg] = len(cmdList) - 1;/* Line position for error msgs */
    } /* End of asking user for additional argument */

    /* Make sure that the argument is a non-negative integer */
    vstring_def(tmpArg);
    let(&tmpArg, g_rawArgPntr[arg]);
    if (tmpArg[0] == 0) { /* Use default argument */
      /* (This code is needed in case of null string passed directly) */
      let(&tmpArg, seg(defaultCmd, 2, len(defaultCmd) - 1));
    }
    vstring_def(tmpStr);
    let(&tmpStr, str(val(tmpArg)));
    let(&tmpStr, cat(string(len(tmpArg)-len(tmpStr),'0'), tmpStr, NULL));
    if (strcmp(tmpStr, tmpArg)) {
      printCommandError(errorLine, arg, "?A number was expected here.");
      ret = 0;
    } else {
      let(&keyword, str(val(tmpArg)));
    }

    free_vstring(tmpArg);
    free_vstring(tmpStr);
    goto getFullArg_ret;
  }

  /* Handle special case - any arbitrary string is OK */
  /* '*' means any string, '&' means a file */
  /* However, "|$<$>" also allows null string (no argument) */
  if (cmdList[0] == '*' || cmdList[0] == '&') {
    let(&defaultCmd, seg(cmdList,instr(1, cmdList, "<"), instr(1, cmdList, ">")));

    /* If the argument has not been entered, prompt the user for it */
    if (g_rawArgs <= arg) {
      if (!strcmp(defaultCmd, "<$>")) { /* End of command acceptable */
        /* Note:  in this case, user will never be prompted for anything. */
        let(&keyword, chr(3));
        goto getFullArg_ret;
      }
      g_rawArgs++;
      pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
      nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, 0));
      if (g_rawArgs <= arg) bug(1104);
      g_queryMode = 1;
      vstring tmpArg = cmdInput1(right(cmdList,3));

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

      let(&errorLine, right(cmdList,3));
      if (tmpArg[0] == 0) { /* Use default argument */
        let(&tmpArg, seg(defaultCmd, 2, len(defaultCmd) - 1));
      }
      let((vstring *)(&g_rawArgPntr[arg]), tmpArg);
      g_rawArgNmbr[arg] = len(cmdList) - 1; /* Line position for error msgs */
      free_vstring(tmpArg);
    } /* End of asking user for additional argument */

    let(&keyword, g_rawArgPntr[arg]);

    /* Convert abbreviations of FIRST, LAST, ALL to
       full keywords.  The rest of the program works fine without doing this,
       but it provides better cosmetic appearance when the command is echoed
       such as in during the UNDO command. */
    if (cmdList[0] == '*') {
      if ((keyword[0] == 'f' || keyword[0] == 'F')
          && instr(1, cmdList, " FIRST") != 0)
        let(&keyword, "FIRST");
      if ((keyword[0] == 'l' || keyword[0] == 'L')
          && instr(1, cmdList, " LAST") != 0)
        let(&keyword, "LAST");
      if ((keyword[0] == 'a' || keyword[0] == 'A')
          && instr(1, cmdList, " ALL") != 0)
        let(&keyword, "ALL");
    }

    if (keyword[0] == 0) { /* Use default argument */
      /* This case handles blank arguments on completely input command line */
      let(&keyword, seg(defaultCmd,2,len(defaultCmd) - 1));
    }
    if (cmdList[0] == '&') {
      /* See if file exists */
      vstring_def(tmpStr);
      let(&tmpStr, cat(g_rootDirectory, keyword, NULL));
      FILE *tmpFp = fopen(tmpStr, "r");
      if (!tmpFp) {
        let(&tmpStr, cat("?Sorry, couldn't open the file \"", tmpStr, "\".", NULL));
        printCommandError(errorLine, arg, tmpStr);
        ret = 0;
      } else {
        fclose(tmpFp);
      }
      free_vstring(tmpStr);
    }
    goto getFullArg_ret;
  }

  /* Parse the choices available */
  long possCmds = 0;
  long p = 0;
  long q = 0;
  while (1) {
    p = instr(p + 1, cat(cmdList, "|", NULL), "|");
    if (!p) break;
    pntrLet(&possCmd, pntrAddElement(possCmd));
    let((vstring *)(&possCmd[possCmds]), seg(cmdList, q+1, p-1));
    possCmds++;
    q = p;
  }
  if (!strcmp(left(possCmd[possCmds - 1],1), "<")) {
    // free_vstring(defaultCmd); // Not needed because defaultCmd is already empty
    /* Get default argument, if any */
    defaultCmd = possCmd[possCmds - 1]; /* re-use old allocation */

    if (!strcmp(defaultCmd, "<$>")) {
      let(&defaultCmd, "<nothing>");
    }
    pntrLet(&possCmd, pntrLeft(possCmd, possCmds - 1));
    possCmds--;
  }
  if (!strcmp(possCmd[possCmds - 1], "$")) {
    /* Change "$" to "nothing" for printouts */
    let((vstring *)(&possCmd[possCmds - 1]), "nothing");
  }

  /* Create a string used for queries and error messages */
  if (possCmds < 1) {
    bug(1105);
    ret = 0;
    goto getFullArg_ret;
  }
  if (possCmds == 1) {
    let(&infoStr,possCmd[0]);
  } else if (possCmds == 2) {
    let(&infoStr, cat(possCmd[0], " or ",
        possCmd[1], NULL));
  } else /* possCmds > 2 */ {
    let(&infoStr, "");
    for (long i = 0; i < possCmds - 1; i++) {
      let(&infoStr, cat(infoStr, possCmd[i], ", ", NULL));
    }
    let(&infoStr, cat(infoStr, "or ", possCmd[possCmds - 1], NULL));
  }

  /* If the argument has not been entered, prompt the user for it */
  if (g_rawArgs <= arg && (strcmp(possCmd[possCmds - 1], "nothing")
      || g_queryMode == 1)) {

    vstring_def(tmpStr);
    let(&tmpStr, infoStr);
    if (defaultCmd[0] != 0) {
      let(&tmpStr, cat(tmpStr, " ",defaultCmd, NULL));
    }
    let(&tmpStr, cat(tmpStr, "? ", NULL));
    g_queryMode = 1;
    vstring_def(tmpArg);
    if (possCmds != 1) {
      tmpArg = cmdInput1(tmpStr);
    } else {
      /* There is only one possibility, so don't ask user */
      /* Don't print the message when "end-of-list" is the only possibility. */
      if (!strcmp(cmdList, "$|<$>")) {
        let(&tmpArg, possCmd[0]);
        print2("The command so far is:  ");
        for (long i = 0; i < arg; i++) {
          print2("%s ", g_fullArg[i]);
        }
        print2("%s\n", tmpArg);
      }
    }
    let(&errorLine, tmpStr);
    if (tmpArg[0] == 0) { /* Use default argument */
      let(&tmpArg, seg(defaultCmd, 2, len(defaultCmd) - 1));
    }

    if (strcmp(tmpArg, "nothing")) {
      pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
      nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, 0));
      g_rawArgs++;
      if (g_rawArgs <= arg) bug(1106);
      let((vstring *)(&g_rawArgPntr[arg]), tmpArg);
      g_rawArgNmbr[arg] = len(tmpStr) + 1; /* Line position for error msgs */
    }

    free_vstring(tmpStr);
    free_vstring(tmpArg);
  } /* End of asking user for additional argument */

  if (g_rawArgs <= arg) {
    /* No argument was specified, and "nothing" is a valid argument */
    let(&keyword, chr(3));
    goto getFullArg_ret;
  }

  vstring_def(tmpArg);
  let(&tmpArg, edit(g_rawArgPntr[arg], 32)); /* Convert to upper case */
  long j = 0;
  long k = 0;
  long m = len(tmpArg);
  vstring_def(tmpStr);
  /* Scan the possible arguments for a match */
  for (long i = 0; i < possCmds; i++) {
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
  free_vstring(tmpArg);
  if (k < 1 || k > 1) {
    if (k < 1) {
      let(&tmpStr, cat("?Expected ", infoStr, ".", NULL));
    } else {
      if (k == 2) {
        p = instr(1, tmpStr, ", ");
        let(&tmpStr, cat(left(tmpStr, p-1), " or", right(tmpStr, p+1), NULL));
      } else {
        p = len(tmpStr) - 1;
        while (tmpStr[p] != ',') p--;
        let(&tmpStr, cat(left(tmpStr, p+1), " or", right(tmpStr, p+2), NULL));
      }
      let(&tmpStr, cat("?Ambiguous keyword - please specify ", tmpStr, ".", NULL));
    }
    printCommandError(errorLine, arg, tmpStr);
    free_vstring(tmpStr);
    ret = 0;
    goto getFullArg_ret;
  }
  free_vstring(tmpStr);

  let(&keyword, possCmd[j]);

getFullArg_ret:
  if (ret) {
    if (keyword[0] == 0) {
      if (g_rawArgs > arg && strcmp(defaultCmd, "<>")) {
        /* otherwise, "nothing" was specified */
        printCommandError("", arg, "?No default answer is available - please be explicit.");
        ret = 0;
        goto getFullArg_ret;
      }
    }
    /* Add new field to g_fullArg */
    pntrLet(&g_fullArg, pntrAddElement(g_fullArg));
    if (pntrLen(g_fullArg) != arg + 1) bug(1107);
    else let((vstring *)(&g_fullArg[arg]), keyword);
  }

  /* Deallocate memory */
  long len = pntrLen(possCmd);
  for (long i = 0; i < len; i++) free_vstring(*(vstring *)(&possCmd[i]));
  free_pntrString(possCmd);
  free_vstring(defaultCmd);
  free_vstring(infoStr);
  free_vstring(errorLine);
  free_vstring(keyword);
  free_vstring(cmdList);
  return ret;
} /* getFullArg */

/* This function breaks up line into individual tokens
   and puts them into g_rawArgPntr[].  g_rawArgs is the number of tokens.
   g_rawArgPntr[] is the starting position of each token on the line;
   the first character on the line has position 1, not 0.

   Spaces, tabs, and newlines are considered white space.  Special
   one-character
   tokens don't have to be surrounded by white space.  Characters
   inside quotes are considered to be one token, and the quotes are
   removed.
*/
void parseCommandLine(vstring line) {
  /*const char *specialOneCharTokens = "()/,=:";*/
  const char *tokenWhiteSpace = " \t\n";
  const char *tokenComment = "!";

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  long tokenStart = 0;

  /* List of special one-char tokens */
  const char* specialOneCharTokens = g_toolsMode ? "" : "/=";

  long lineLen = len(line);
  /* only "!" at beginning of line acts as comment.
     This is done because sometimes ! might be legal as part of a command */
  enum mode_t {
    MODE_START, // look for start of token
    MODE_END, // look for end of token
    MODE_SQUOTE, // look for trailing single quote
    MODE_DQUOTE, // look for trailing double quote
  } mode = MODE_START;
  long p = 0;
  for (; p < lineLen; p++) {
    freeTempAlloc(); /* Clean up temp alloc stack to prevent overflow */
    switch (mode) {
      case MODE_START: {
        /* If character is white space, ignore it */
        if (instr(1, tokenWhiteSpace, chr(line[p]))) continue;
        /* If character is comment, we're done */
        if (p == 0 && instr(1, tokenComment, chr(line[p]))) goto parseCommandLine_ret;

        /* If character is a special token, get it but don't change mode */
        if (instr(1, specialOneCharTokens, chr(line[p]))) {
          pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
          nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, p+1));
                                                            /* Save token start */
          let((vstring *)(&g_rawArgPntr[g_rawArgs]), chr(line[p]));
          g_rawArgs++;
          continue;
        }
        /* If character is a quote, set start and change mode */
        if (line[p] == '\'') {
          mode = MODE_SQUOTE;
          tokenStart = p + 2;
          continue;
        }
        if (line[p] == '\"') {
          mode = MODE_DQUOTE;
          tokenStart = p + 2;
          continue;
        }
        /* Character must be start of a token */
        mode = MODE_END;
        tokenStart = p + 1;
      } break;

      case MODE_END: {
        /* If character is white space, end token and change mode */
        if (instr(1, tokenWhiteSpace, chr(line[p]))) {
          pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
          nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, tokenStart));
                                                            /* Save token start */
          let((vstring *)(&g_rawArgPntr[g_rawArgs]), seg(line, tokenStart, p));
          g_rawArgs++;
          mode = MODE_START;
          continue;
        }

        /* If character is a special token, get it and change mode */
        if (instr(1, specialOneCharTokens, chr(line[p]))) {
          pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
          nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, tokenStart));
                                                            /* Save token start */
          let((vstring *)(&g_rawArgPntr[g_rawArgs]),seg(line, tokenStart, p));
          g_rawArgs++;
          pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
          nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, p + 1));
                                                            /* Save token start */
          let((vstring *)(&g_rawArgPntr[g_rawArgs]), chr(line[p]));
          g_rawArgs++;
          mode = MODE_START;
          continue;
        }

        /* If character is a quote, set start and change mode */
        if (line[p] == '\'') {
          pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
          nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, tokenStart));
                                                            /* Save token start */
          let((vstring *)(&g_rawArgPntr[g_rawArgs]),seg(line, tokenStart, p));
          g_rawArgs++;
          mode = MODE_SQUOTE;
          tokenStart = p + 2;
          continue;
        }
        if (line[p] == '\"') {
          pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
          nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, tokenStart));
                                                            /* Save token start */
          let((vstring *)(&g_rawArgPntr[g_rawArgs]),seg(line, tokenStart, p));
          g_rawArgs++;
          mode = MODE_DQUOTE;
          tokenStart = p + 2;
          continue;
        }
        /* Character must be continuation of the token */
      } break;

      case MODE_SQUOTE:
      case MODE_DQUOTE: {
        /* If character is a quote, end quote and change mode */
        if (line[p] == (mode == MODE_SQUOTE ? '\'' : '\"')) {
          mode = MODE_START;
          pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
          nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, tokenStart));
                                                            /* Save token start */
          let((vstring *)(&g_rawArgPntr[g_rawArgs]),seg(line, tokenStart, p));
          g_rawArgs++;
          continue;
        }
        /* Character must be continuation of quoted token */
      } break;
    }
  }

  /* Finished scanning the line.  Finish processing last token. */
  if (mode != MODE_START) {
    pntrLet(&g_rawArgPntr, pntrAddElement(g_rawArgPntr));
    nmbrLet(&g_rawArgNmbr, nmbrAddElement(g_rawArgNmbr, tokenStart));
                                                          /* Save token start */
    let((vstring *)(&g_rawArgPntr[g_rawArgs]),seg(line, tokenStart, p));
    g_rawArgs++;
  }

parseCommandLine_ret:
  /* Add length of command line prompt to each argument, to
     align the error message pointer */
  for (long i = 0; i < g_rawArgs; i++) {
    g_rawArgNmbr[i] = g_rawArgNmbr[i] + len(g_commandPrompt);
  }
} /* parseCommandLine */

flag lastArgMatches(vstring argString) {
  /* This functions checks to see if the last field was argString */
  if (!strcmp(argString, g_fullArg[pntrLen(g_fullArg)-1])) {
    return (1);
  } else {
    return (0);
  }
} /* lastArgMatches */

flag cmdMatches(vstring cmdString) {
  /* This function checks that fields 0 through n of g_fullArg match
     cmdString (separated by spaces). */
  long i, j, k;
  /* Count the number of spaces */
  k = len(cmdString);
  j = 0;
  for (i = 0; i < k; i++) {
    if (cmdString[i] == ' ') j++;
  }
  k = pntrLen(g_fullArg);
  vstring_def(tmpStr);
  for (i = 0; i <= j; i++) {
    if (j >= k) {
      /* Command to match is longer than the user's command; assume no match */
      free_vstring(tmpStr);
      return 0;
    }
    let(&tmpStr, cat(tmpStr, " ", g_fullArg[i], NULL));
  }
  if (!strcmp(cat(" ", cmdString, NULL), tmpStr)) {
    free_vstring(tmpStr);
    return 1;
  } else {
    free_vstring(tmpStr);
    return 0;
  }
} /* cmdMatches */

// This function checks that field i of g_fullArg matches "/", and
// field i+1 matches swString (which must not contain spaces).
// The position of the "/" in g_fullArg is returned if swString is there,
// otherwise 0 is returned (the first position in g_fullArg is considered 1, not 0).
//
// Example:  if g_fullArg (combined into one string) is
// "DISPLAY PROOF / UNKNOWN / START_STEP = 10 / ESSENTIAL"
// and swString is "START_STEP", switchPos will return 5.
long switchPos(const char *swString) {
  if (instr(1, swString, " ")) bug(1108);

  long k = pntrLen(g_fullArg);
  for (long i = 0; i < k; i++) {
    if (strcmp(g_fullArg[i], "/") == 0) {
      if (i+1 < k && strcmp(g_fullArg[i+1], swString) == 0) {
        return i + 1;
      }
    }
  }
  return 0;
} /* switchPos */

void printCommandError(vstring line1, long arg, vstring errorMsg)
{
  /* Warning: errorMsg should not a temporarily allocated string such
     as the direct output of cat() */
  vstring_def(errorPointer);
  vstring_def(line);
  long column, tokenLength, j;

  let(&line,line1); /* Prevent deallocation in case line1 is
                       direct return from string function such as cat() */
  if (!line[0]) {
    /* Empty line - don't print an error pointer */
    print2("%s\n", errorMsg);
    free_vstring(line);
    return;
  }
  column = g_rawArgNmbr[arg];
  tokenLength = len(g_rawArgPntr[arg]);
  for (j = 0; j < column - 1; j++) {
    /* Make sure that tabs on the line with the error are accounted for so
       that the error pointer lines up correctly */
    if (j >= len(line)) bug(1109);
    if (line[j] == '\t') {
      let(&errorPointer, cat(errorPointer, "\t", NULL));
    } else {
      if (line[j] == '\n') {
        let(&errorPointer, "");
      } else {
        let(&errorPointer, cat(errorPointer, " ", NULL));
      }
    }
  }
  for (j = 0; j < tokenLength; j++)
    let(&errorPointer, cat(errorPointer, "^", NULL));
  print2("%s\n", errorPointer);
  printLongLine(errorMsg, "", " ");
  free_vstring(errorPointer);
  free_vstring(line);
} /* printCommandError */

void freeCommandLine(void) {
  long i, j;
  j = pntrLen(g_rawArgPntr);
  for (i = 0; i < j; i++) free_vstring(*(vstring *)(&g_rawArgPntr[i]));
  j = pntrLen(g_fullArg);
  for (i = 0; i < j; i++) free_vstring(*(vstring *)(&g_fullArg[i]));
  free_pntrString(g_fullArg);
  free_pntrString(g_rawArgPntr);
  free_nmbrString(g_rawArgNmbr);
  free_vstring(g_fullArgString);
}
