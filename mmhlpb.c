/*****************************************************************************/
/*        Copyright (C) 2002  NORMAN MEGILL  nm@alum.mit.edu                 */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* Part 2 of help file for Metamath */

/* To add a new help entry, you must add the command syntax to mmcmdl.c
   as well as adding it here. */

#include <string.h>
#include <stdio.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmds.h"
#include "mmhlpb.h"

void help2(vstring helpCmd)
{

printHelp = !strcmp(helpCmd, "HELP");
H("Welcome to Metamath.  Here are some general guidelines.");
H("");
H("To make the most effective use of Metamath, you should become familiar");
H("with the Metamath book.  In particular, you will need to learn");
H("the syntax of the Metamath language.");
H("");
H("For a summary of the Metamath language, type HELP LANGUAGE.");
H("For help invoking Metamath, type HELP INVOKE.");
H("For help using the command line interpreter, type HELP CLI.");
H("For help getting started, type HELP DEMO.");
H("For help exploring the data base, type HELP EXPLORE.");
H("For help creating a LaTeX file, type HELP TEX.");
H("For help creating Web pages, type HELP HTML.");
H("For help using the Proof Assistant, type HELP PROOF_ASSISTANT.");
H("For a list of help topics, type HELP ?.");
H("For current program settings, type SHOW SETTINGS.");
H("To exit Metamath, type EXIT (or its synonym QUIT).");
H("");
H("If you need technical support, contact Norman Megill at nm@alum.mit.edu.");
H("Copyright (C) 2002 Norman Megill");
H("License terms:  GNU General Public License");
H("");

printHelp = !strcmp(helpCmd, "HELP INVOKE");
H("To invoke Metamath from a Unix/Linux/MacOSX prompt, assuming that the");
H("Metamath program is in the current directory, type");
H("");
H("  bash$ ./metamath");
H("");
H("To invoke Metamath from a Windows DOS or Command Prompt, assuming that");
H("the Metamath program is in the current directory (or in a directory");
H("included in the Path system environment variable), type");
H("");
H("  C:\\metamath>metamath");
H("");
H("To use command-line arguments at invocation, the command-line arguments");
H("should be a list of Metamath commands, surrounded by quotes if they");
H("contain spaces.  In Windows DOS, the surrounding quotes must be double");
H("(not single) quotes.  For example, to read the database set.mm verify");
H("all proofs, and exit the program, type (under Unix)");
H("");
H("  bash$ ./metamath \"read set.mm\" \"verify proof *\" exit");
H("");
H("Note that in Unix, any directory path with /'s must be surrounded by");
H("quotes so Metamath will not interpret the / as a command qualifier.  So");
H("if set.mm is in the \"/tmp\" directory, use for the above example");
H("");
H("  bash$ ./metamath \"read '/tmp/set.mm'\" \"verify proof *\" exit");
H("");
H("For convenience, if the command-line has one argument and no spaces in");
H("the argument, the command is implicitly assumed to be \"READ\".  Thus");
H("");
H("  bash$ ./metamath set.mm");
H("");
H("and");
H("");
H("  bash$ ./metamath \"read set.mm\"");
H("");
H("are equivalent.  To read from the \"/tmp\" directory in Unix, the first");
H("example would be");
H("");
H("  bash$ ./metamath \"'/tmp/set.mm'\"");
H("");

printHelp = !strcmp(helpCmd, "HELP CLI");
H("Each command line is a sequence of English-like words separated by");
H("spaces, as in SHOW SETTINGS.  Command words are not case sensitive, and");
H("only as many letters are needed as are necessary to eliminate ambiguity;");
H("for example, \"sh se\" would work for the command SHOW SETTINGS.  In some");
H("cases arguments such as file names, statement labels, or symbol names are");
H("required; these are case-sensitive (although file names may not be on");
H("some operating systems).");
H("");
H("A command line is entered by typing it in then pressing the <return> key.");
H("");
H("To find out what commands are available, type ? at the MM> prompt.");
H("");
H("To find out the choices at any point in a command, press <return> and you");
H("will be prompted for them.  The default choice (the one selected if you");
H("just press <return>) is shown in brackets (<>).");
H("");
H("You may also type ? in place of a command word to force Metamath to tell");
H("you what the choices are.  The ? method won't work, though, if a");
H("non-keyword argument such as a file name is expected at that point,");
H("because the CLI will think the ? is the argument.");
H("");
H("Some commands have one or more optional qualifiers which modify the");
H("behavior of the command.  Qualifiers are indicated by a slash (/), such as");
H("in READ set.mm / VERIFY.  Spaces are optional around the /.  If you need");
H("to use a slash in a command argument, as in a Unix file name, put single");
H("or double quotes around the command argument.");
H("");
H("The OPEN LOG command will save everything you see on the screen, and is");
H("useful to help you recover should something go wrong in a proof, or if");
H("you want to document a bug.");
H("");
H("If the response to a command is more than a screenful, you will be");
H("prompted to '<return> to continue, Q to quit, or S to scroll to end'.");
H("Q will complete the command internally but suppress further output until");
H("the next \"MM>\" prompt.  S will suppress further pausing until the next");
H("\"MM>\" prompt.  After the first screen, you are also presented with B");
H("to go back a screenful.  Note that B may also be entered at the \"MM>\"");
H("prompt immediately after a command to scroll back through the output of");
H("that command.");
H("");
H("**Warning**  Pressing CTRL-C will abort the Metamath program");
H("unconditionally.  This means any unsaved work will be lost.");
H("");
H("A command line enclosed in quotes is executed by your operating system.");
H("See HELP SYSTEM.");
H("");
H("Some other commands you may want to review with HELP are:");
H("    SET ECHO");
H("    SET SCROLL");
H("    SET SCREEN_WIDTH");
H("    SUBMIT");
H("    FILE SEARCH");
H("    FILE TYPE");
H("");


printHelp = !strcmp(helpCmd, "HELP SHOW MEMORY");
H("Syntax:  SHOW MEMORY");
H("");
H("This command shows the available memory left.  It may not be meaningful");
H("on machines with virtual memory.");
H("");


printHelp = !strcmp(helpCmd, "HELP SHOW SETTINGS");
H("Syntax:  SHOW SETTINGS");
H("");
H("This command shows the state of various parameters.");
H("");


printHelp = !strcmp(helpCmd, "HELP SHOW LABELS");
H("Syntax:  SHOW LABELS <label-match> [/ ALL]");
H("");
H("This command shows the labels of $a and $p statements that match");
H("<label-match>.  A * in <label-match> matches any zero or more characters.");
H("For example, \"*abc*def\" will match all labels containing \"abc\" and");
H("ending with \"def\".");
H("");
H("Optional qualifier:");
H("    / ALL - Include matches for $e and $f statement labels.");
H("");


printHelp = !strcmp(helpCmd, "HELP SHOW SOURCE");
H("Syntax:  SHOW SOURCE <label>");
H("");
H("This command shows the ASCII source code associated with a statement.");
H("Normally you should use SHOW STATEMENT for a more meaningful display,");
H("but SHOW SOURCE can be used to see statements with multiple comments");
H("and to see the exact content of the Metamath database.");
H("");


printHelp = !strcmp(helpCmd, "HELP SHOW STATEMENT");
H(
"Syntax:  SHOW STATEMENT <label> [/ COMMENT] [/ FULL] [/ TEX] [/ HTML]");
H("             [/ ALT_HTML] [/ BRIEF_HTML] [/ BRIEF_ALT_HTML]");
H("");
H("");
H("This command provides information about a statement.  Only statements");
H("that have labels ($f, $e, $a, and $p) may be specified.  If <label>");
H("contains wildcard (\"*\") characters, all matching statements will be");
H("displayed in the order they occur in the database.");
H("");
H("By default, only the statement and its $e hypotheses are shown, and if");
H("the label has wildcards, only $a and $p statements are shown.");
H("");
H("Optional qualifiers (only one qualifier at a time is allowed):");
H("    / COMMENT - This qualifier includes the comment that immediately");
H("        precedes the statement.");
H("    / FULL - Show complete information about each statement, and show all");
H("        statements matching <label> (including $e and $f statements).");
H("    / TEX - This qualifier will write the statement information to the");
H("        LaTeX file previously opened with OPEN TEX.");
H("    / HTML - This qualifier invokes a special mode of SHOW STATEMENT which");
H("        creates a Web page for the statement.  It may not be used with");
H("        any other qualifier.  See HELP HTML for more information.");
H("    / ALT_HTML, / BRIEF_HTML, / BRIEF_ALT_HTML - See HELP HTML for more");
H("        information on these.");
H("");


printHelp = !strcmp(helpCmd, "HELP SHOW PROOF");
H("Syntax:  SHOW PROOF <label> [<qualifiers (see below)>]");
H("");
H("This command displays the proof of the specified $p statement various");
H("formats.  The <label> may contain wildcard (\"*\") characters to match");
H("multiple statements.  Without any qualifiers, only the logical steps will");
H("be shown (i.e. syntax construction steps will be omitted), in");
H("indented format.");
H("");
H("Most of the time, you will use");
H("    SHOW PROOF <label>");
H("to see just the proof steps corresponding to logical deduction.");
H("");
H("Optional qualifiers:");
H("    / ESSENTIAL - the proof tree is trimmed of all $f hypotheses before");
H("        being displayed.  (This is the default, and it is redundant to");
H("        specify it.)");
H("    / ALL - the proof tree is not trimmed of all $f hypotheses before");
H("        being displayed.  / ESSENTIAL and / ALL are mutually exclusive.");
H("    / FROM_STEP <step> - the display starts at the specified step.  If");
H("        this qualifier is omitted, the display starts at the first step.");
H("    / TO_STEP <step> - the display ends at the specified step.  If this");
H("        qualifier is omitted, the display ends at the last step.");
H("    / TREE_DEPTH <number> - Only steps at less than the specified proof");
H("        tree depth are displayed.  Useful for obtaining an overview of");
H("        the proof.");
H("    / REVERSE - the steps are displayed in reverse order.");
H("    / RENUMBER - when used with / ESSENTIAL, the steps are renumbered");
H("        to correspond only to the essential steps.");
H("    / TEX - the proof is converted to LaTeX and stored in the file opened");
H("        with OPEN TEX.");
H("    / LEMMON - The proof is displayed in a non-indented format known");
H("        as Lemmon style, with explicit previous step number references.");
H("        If this qualifier is omitted, steps are indented in a tree format.");
H("    / COLUMN <number> - Overrides the default column at which");
H("        the formula display starts in a Lemmon style display.  May be");
H("        used only in conjuction with / LEMMON.");
H("    / NORMAL - The proof is displayed in normal format suitable for");
H("        inclusion in a source file.  May not be used with any other");
H("        qualifier.");
/*
H("    / COMPACT - The proof is displayed in compact format suitable for");
H("        inclusion in a source file.  May not be used with any other");
H("        qualifier.");
*/
H("    / COMPRESSED - The proof is displayed in compressed format");
H("        suitable for inclusion in a source file.  May not be used with");
H("        any other qualifier.");
H("    / STATEMENT_SUMMARY - Summarizes all statements (like a brief SHOW");
H("        STATEMENT) used by the proof.  May not be used with any other");
H("        qualifier except / ESSENTIAL.");
H("    / DETAILED_STEP <step> - Shows the details of what is happening at");
H("        a specific proof step.  May not be used with any other qualifier.");
H("");


printHelp = !strcmp(helpCmd, "HELP MIDI");
H("Syntax:  MIDI <label> [/ PARAMETER \"<parameter string>\"]");
H("");
H("This will create a MIDI sound file for the proof of <label>, where <label>");
H("is one of the $p statement labels shown with SHOW LABELS.  The <label>");
H("may contain wildcard (\"*\") characters to match multiple statements.");
H("For each matched label, a file will be created called <label>.txt which");
H("is a MIDI source file that can be converted to a MIDI binary file with");
H("the \"t2mf\" utility that can be obtained at:");
H("   http://www.hitsquad.com/smm/programs/mf2t/download.shtml");
H("Note: the MS-DOS version t2mf.exe only handles old-style 8.3 file names,");
H("so files such as pm2.11.txt are rejected and must be renamed to");
H("e.g. pm2_11.txt.");
H("The parameter string characters currently defined are: f - fast speed,");
H("s - syncopation, h - turn on hesitation feature of syncopation.  Quotes");
H("around the parameter string are optional if it has no spaces.");
H("");


printHelp = !strcmp(helpCmd, "HELP SHOW NEW_PROOF");
H("Syntax:  SHOW NEW_PROOF [<qualifiers (see below)]");
H("");
H("This command (available only in Proof Assistant mode) displays the proof");
H("in progress.  It is identical to the SHOW PROOF command, except that the");
H("statement is not specified (since it is the statement being proved) and");
H("following qualifiers are not available:");
H("    / STATEMENT_SUMMARY");
H("    / DETAILED_STEP");
H("");
H("Also, the following additional qualifiers are available:");
H("    / UNKNOWN - Shows only steps that have no statement assigned.");
H("    / NOT_UNIFIED - Shows only steps that have not been unified.");
H("");
H("Note that / ESSENTIAL, / DEPTH, / UNKNOWN, and / NOT_UNIFIED may");
H("be used in any combination; each of them effectively filters out");
H("additional steps from the proof display.");
H("");
H("See also:  SHOW PROOF");
H("");


printHelp = !strcmp(helpCmd, "HELP SHOW USAGE");
H("Syntax:  SHOW USAGE <label> [/ RECURSIVE]");
H("");
H("This command lists the statements whose proofs make direct reference to");
H("the statement specified.");
H("");
H("Optional qualifier:");
H("    / RECURSIVE - Also include include statements whose proof ultimately");
H("        depend on the statement specified.");
H("");


printHelp = !strcmp(helpCmd, "HELP SHOW TRACE_BACK");
H("Syntax:  SHOW TRACE_BACK <label> [/ ESSENTIAL] [/ AXIOMS] [/ TREE]");
H("             [/ DEPTH <number>] [/ COUNT_STEPS]");
H("");
H("This command lists all statements that the proof of the $p statement");
H("specified by <label> depends on.");
H("");
H("Optional qualifiers:");
H("    / ESSENTIAL - Restrict the trace-back to $e hypotheses of proof");
H("        trees.");
H("    / AXIOMS - List only the axioms that the proof ultimately depends on.");
H("    / TREE - Display the trace-back in an indented tree format.");
H("    / DEPTH - Restrict the / TREE traceback to the specified indentation");
H("        depth.");
H("    / COUNT_STEPS - Counts the number of steps the proof would have if");
H("        fully expanded back to axioms.  If / ESSENTIAL is specified,");
H("        expansions of floating hypotheses are not counted.");
H("");


printHelp = !strcmp(helpCmd, "HELP SEARCH");
H("Syntax:  SEARCH <label-match> \"<symbol-match>\" [/ ALL] [/ COMMENTS]");
H("");
H("This command searches all $a and $p statements matching <label-match>");
H("for occurrences of <symbol-match>.  A * in <label-match> matches");
H("any zero or more characters in a label.  A $* in <symbol-match> matches");
H("any sequence of zero or more symbols.  The symbols in <symbol-match> must");
H("be separated by white space.  The quotes surrounding <symbol-match> may");
H("be single or double quotes.  For example, SEARCH b* \"-> $* ch\" will");
H("list all statements whose labels begin with b and contain the symbols ->");
H("and ch surrounding any symbol sequence (including no symbol sequence).");
H("");
H("Optional qualifier:");
H("    / ALL - Also search $e and $f statements.");
H("    / COMMENTS - Search the comment that immediately precedes each");
H("        label-matched statement for <symbol-match>.  In this case");
H("        <symbol-match> is an arbitrary, non-case-sensitive character");
H("        string.");
H("");


printHelp = !strcmp(helpCmd, "HELP SET ECHO");
H("Syntax:  SET ECHO ON or SET ECHO OFF");
H("");
H("The SET ECHO ON command will cause command lines to be echoed with any");
H("abbreviations expanded.  While learning the Metamath commands, this");
H("feature will show you the exact command that your abbreviated input");
H("corresponds to.");
H("");


printHelp = !strcmp(helpCmd, "HELP SET SCROLL");
H("Syntax:  SET SCROLL PROMPTED or SET SCROLL CONTINUOUS");
H("");
H("The Metamath command line interface starts off in the PROMPTED mode,");
H("which means that you will prompted to continue or quit after each");
H("screenful of a long listing.  In CONTINUOUS mode, long listings will be");
H("scrolled without pausing.");
H("");


printHelp = !strcmp(helpCmd, "HELP SET SCREEN_WIDTH");
H("Syntax:  SET SCREEN_WIDTH <number>");
H("");
H("Metamath assumes the width of your screen is 79 characters.  If your");
H("screen is wider or narrower, this command allows you to change the screen");
H("width.  A larger width is advantageous for logging proofs to an output");
H("file to be printed on a wide printer.  A smaller width may be necessary");
H("on some terminals; in this case, the wrapping of the information");
H("messages may sometimes seem somewhat unnatural, however.  In LaTeX, there");
H("is normally a maximum of 61 characters per line with typewriter font.");
H("");


printHelp = !strcmp(helpCmd, "HELP SET UNIFICATION_TIMEOUT");
H("Syntax:  SET UNIFICATION_TIMEOUT <number>");
H("");
H("(This command affects the Proof Assistant only.)");
H("");
H("Sometimes the Proof Assistant will inform you that a unification timeout");
H("occurred.  This may happen when you try to UNIFY formulas with many");
H("unknown variables, since the time to compute unifications may grow");
H("exponentially with the number of variables.  If you want Metamath to try");
H("harder (and you're willing to wait longer) you may increase this");
H("parameter.  SHOW SETTINGS will show you the current value.");
H("");


printHelp = !strcmp(helpCmd, "HELP SET EMPTY_SUBSTITUTION");
H("Syntax:  SET EMPTY_SUBSTITUTION ON or SET EMPTY_SUBSTITUTION OFF");
H("");
H("(This command affects the Proof Assistant only.  It may be issued)");
H("outside of the Proof Assistant.");
H("");
H("The Metamath language allows variables to be substituted with empty");
H("symbol sequences.  However, in most formal systems this will never happen");
H("in a valid proof.  Allowing for this possibility increases the likelihood");
H("of ambiguous unifications during proof creation.  The default is that");
H("empty substitutions are not allowed; for formal systems requiring them,");
H("you must SET EMPTY_SUBSTITUTION ON.  Note that empty substitutions are");
H("always permissable in proof verification (VERIFY PROOFS...) outside the");
H("Proof Assistant.  (See the MIU system in the Metamath book for an example");
H("of a system needing empty substitutions; another example would be a");
H("system that implements a Deduction Rule and in which deductions from");
H("empty assumption lists would be permissable.)");
H("");


printHelp = !strcmp(helpCmd, "HELP SET SEARCH_LIMIT");
H("Syntax:  SET SEARCH_LIMIT <number>");
H("");
H("(This command affects the Proof Assistant only.)");
H("");
H("This command sets a parameter that determines when the IMPROVE command");
H("in Proof Assistant mode gives up.  If you want IMPROVE to search harder,");
H("you may increase it.  The SHOW SETTINGS command tells you its current");
H("value.");
H("");


printHelp = !strcmp(helpCmd, "HELP VERIFY PROOF");
H("Syntax:  VERIFY PROOF <label-match> [/ SYNTAX_ONLY]");
H("");
H("This command verifies the proofs of the specified statements.");
H("<label-match> may contain wildcard characters (*) to verify more than one");
H("proof; for example \"*abc*def\" will match all labels containing \"abc\"");
H("and ending with \"def\".  VERIFY PROOF * will verify all proofs in the");
H("database.");
H("");
H("Optional qualifier:");
H("    / SYNTAX_ONLY - This qualifier will perform a check of syntax and RPN");
H("        stack violations only.  It will not verify that the proof is");
H("        correct.");
H("");


printHelp = !strcmp(helpCmd, "HELP SUBMIT");
H("Syntax:  SUBMIT <filename>");
H("");
H("This command causes further command lines to be taken from the specified");
H("file.  Note that any line beginning with an exclamation point (!) is");
H("treated as a comment (i.e. ignored).  Also note that the scrolling");
H("of the screen output is continuous, so you may have to open a log");
H("file to record the results.");


printHelp = !strcmp(helpCmd, "HELP SYSTEM");
H("A line enclosed in quotes will be executed by your computer's operating");
H("system, if it has such a feature.  For example, on a VAX/VMS system,");
H("    MM> 'dir'");
H("will print disk directory contents.  Note that this feature will not work");
H("on the Macintosh, which does not have a command line interface.");
H("");
H("For your convenience, the trailing quote is optional.");
H("");


printHelp = !strcmp(helpCmd, "HELP PROVE");
H("Syntax:  PROVE <label>");
H("");
H("This command will enter the Proof Assistant, which will allow you to");
H("create or edit the proof of the specified statement.");
H("");
H("See also:  HELP PROOF_ASSISTANT and HELP EXIT");
H("");


printHelp = !strcmp(helpCmd, "HELP FILE TYPE");
H("Syntax:  FILE TYPE <filename> [/ FROM_LINE <number>] [/ TO_LINE <number>]");
H("");
H("This command will type the contents of an ASCII file on your screen,");
H("with an optional range of line numbers.");
H("");


printHelp = !strcmp(helpCmd, "HELP FILE SEARCH");
H("Syntax:  FILE SEARCH <filename> \"<search string>\" [/ FROM_LINE");
H("             <number>] [/ TO_LINE <number>]");
H("");
H("This command will search an ASCII file for the specified string in");
H("quotes, within an optional range of line numbers.");
H("");


printHelp = !strcmp(helpCmd, "HELP WRITE SOURCE");
H("Syntax:  WRITE SOURCE <filename>");
H("");
H("This command will write the contents of the Metamath database into a file.");
H("Note:  The present version of Metamath will not split the database into");
H("its constituent files included with $[ $] keywords.  Instead it will write");
H("the entire database as one big file.");
H("");


printHelp = !strcmp(helpCmd, "HELP ASSIGN");
H("Syntax:  ASSIGN <step> <label>");
H("         ASSIGN LAST <label>");
H("");
H("This command, available in the Proof Assistant only, assigns an unknown");
H("step (one with ? in the SHOW NEW_PROOF listing) with the statement");
H("specified by <label>.  The assignment will not be allowed if the");
H("statement cannot be unified with the step.  To see what statements may be");
H("unified with the step, you may use the MATCH command.");
H("");
H("If LAST is specified instead of <step> number, the last step that is shown");
H("by SHOW NEW_PROOF /ESSENTIAL /UNKNOWN will be used.  This can be useful for");
H("building a proof with a command file (see HELP SUBMIT).  Note that");
H("interactive unification is not done after ASSIGN LAST, so the UNIFY ALL");
H("/INTERACTIVE may have to used after ASSIGN LAST.  (The reason is to");
H("provide predictable responses when SUBMIT command files are run using");
H("ASSIGN LAST to reconstruct parts of proofs.)");
H("");

printHelp = !strcmp(helpCmd, "HELP REPLACE");
H("Syntax:  REPLACE <step> <label>");
H("");
H("This command, available in the Proof Assistant only, replaces the");
H("statement assigned to <step> with statement <label>.  The replacement");
H("will be done only if the subproof at <step> has contains steps (with no");
H("dummy variables) that exactly match the hypotheses of <label>.");
H("");

printHelp = !strcmp(helpCmd, "HELP MATCH");
H("Syntax:  MATCH STEP <step> [/ MAX_ESSENTIAL_HYP <number>]");
H("    or:  MATCH ALL [/ ESSENTIAL_ONLY] [/ MAX_ESSENTIAL_HYP <number>]");
H("");
H("This command, available in the Proof Assistant only, shows what");
H("statements can be unified with the specified step(s).");
H("");
H("Optional qualifiers:");
H("    / MAX_ESSENTIAL_HYP <number> - filters out of the list any statements");
H("        with more than the specified number of $e hypotheses");
H("    / ESSENTIAL_ONLY - in the MATCH ALL statement, only the steps that");
H("        would be listed in SHOW NEW_PROOF / ESSENTIAL display are");
H("        matched.");
H("");


printHelp = !strcmp(helpCmd, "HELP LET");
H("Syntax:  LET VARIABLE <variable> = \"<symbol sequence>\"");
H("         LET STEP <step> = \"<symbol sequence>\"");
H("");
H("These commands, available in the Proof Assistant only, assign an unknown");
H("variable or step with a specific symbol sequence.  They are useful in the");
H("middle of creating a proof, when you know what should be in the proof");
H("step but the unification algorithm doesn't yet have enough information");
H("to completely specify the unknown variables.  An \"unknown\" variable is");
H("one which has the form $nn in the proof display, such as $1, $2, etc.");
H("The <symbol sequence> may contain other unknown variables if desired.");
H("Examples:");
H("    LET VARIABLE $32 = \" A = B \"");
H("    LET VARIABLE $32 = \" A = $35 \"");
H("    LET STEP 10 = \" |- x = x \"");
H("");
H("Any symbol sequence will be accepted for the LET VARIABLE command.  The");
H("step in LET STEP must be an unknown step, and only symbol sequences");
H("that can be unified with the step will be accepted.");
H("");
H("The LET commands are somewhat dangerous in that they \"zap\" the proof");
H("with information that can only be verified when the proof is built up");
H("further. If you make an error, the INITIALIZE commands can help undo what");
H("you did.");
H("");


printHelp = !strcmp(helpCmd, "HELP UNIFY");
H("HELP UNIFY");
H("Syntax:  UNIFY STEP <step>");
H("         UNIFY ALL [/ INTERACTIVE]");
H("");
H("These commands, available in the Proof Assistant only, unify the source");
H("and target of the specified step(s). If you specify a specific step, you");
H("will be prompted to select among the unifications that are possible.  If");
H("you specify ALL, only those steps with unique unifications will be");
H("unified.");
H("");
H("Optional qualifier for UNIFY ALL:");
H("    / INTERACTIVE - You will be prompted to select among the unifications");
H("        that are possible for any steps that do not have unique");
H("        unifications.");
H("");
H("See also SET UNIFICATION_TIMEOUT.  The default is 1000, but increasing it");
H("to 10000 can help difficult cases.  The LET VARIABLE command to manually");
H("assign unknown variables also helps difficult cases.");
H("");


printHelp = !strcmp(helpCmd, "HELP INITIALIZE");
H("Syntax:  INITIALIZE STEP <step>");
H("         INITIALIZE ALL");
H("");
H("These commands, available in the Proof Assistant only, \"de-unify\" the");
H("target and source of a step (or all steps), as well as the hypotheses of");
H("the source, and makes all variables in the source and the source's");
H("hypotheses unknown.  This command is useful to help recover when a");
H("mistake resulted in incorrect unifications.");
H("");
H("See also:  UNIFY and DELETE");
H("");


printHelp = !strcmp(helpCmd, "HELP DELETE");
H("Syntax:  DELETE STEP <step>");
H("         DELETE ALL");
H("         DELETE FLOATING_HYPOTHESES");
H("");
H("These commands are available in the Proof Assistant only. The DELETE STEP");
H("command deletes the proof tree section that branches off of the specified");
H("step and makes the step become unknown.  DELETE ALL is equivalent to");
H("DELETE STEP <step> where <step> is the last step in the proof (i.e. the");
H("beginning of the proof tree).");
H("");
H("DELETE FLOATING_HYPOTHESES will delete all sections of the proof that");
H("branch off of $f statements.  It is sometimes useful to do this before");
H("an INITIALIZE command to recover from an error.  Note that once a proof");
H("step with a $f hypothesis as the target is completely known, the IMPROVE");
H("command can usually fill in the proof for that step.");
H("");


printHelp = !strcmp(helpCmd, "HELP IMPROVE");
H("Syntax:  IMPROVE STEP <step> [/ DEPTH <number>]");
H("         IMPROVE ALL [/ DEPTH <number>]");
H("         IMPROVE LAST [/ DEPTH <number>]");
H("");
H("These commands, available in the Proof Assistant only, try to");
H("automatically find proofs for unknown steps whose symbol sequences are");
H("completely known.  It is primarily useful for filling in proofs of $f");
H("hypotheses.  The search will be restricted to statements having no $e");
H("hypotheses.");
H("");
H("IMPROVE LAST is the same as IMPROVE STEP using the last step that is shown");
H("by SHOW NEW_PROOF /ESSENTIAL /UNKNOWN.");
H("");
H("Optional qualifier:");
H("    / DEPTH <number> - This qualifier will cause the search to include");
H("        statements with $e hypotheses (but no new variables in the $e");
H("        hypotheses), provided that the backtracking has not exceeded the");
H("        specified depth.  **WARNING**:  Try DEPTH 1, then 2, then 3, etc");
H("        in sequence because of possible exponential blowups.  Save your");
H("        work before trying DEPTH greater than 1!!!");
H("");
H("Note:  If memory is limited, IMPROVE ALL on a large proof may overflow");
H("memory.  If you use SET UNIFICATION_TIMEOUT 1 before IMPROVE ALL,");
H("there will usually be sufficient improvement to later easily recover and");
H("completely IMPROVE the proof on a larger computer.  Warning:  Once");
H("memory has overflowed, there is no recovery.  If in doubt, save the");
H("intermediate proof (SAVE NEW_PROOF then WRITE SOURCE) before IMPROVE ALL.");
H("");
H("See also:  HELP SET SEARCH_LIMIT");
H("");


printHelp = !strcmp(helpCmd, "HELP MINIMIZE_WITH");
H("Syntax:  MINIMIZE_WITH <label> [/ BRIEF] [/ ALLOW_GROWTH] [/ NO_DISTINCT]");
H("");
H("This command, available in the Proof Assistant only, checks whether");
H("the proof can be shortened by using statements matching <label>, and");
H("if so, shortens the proof.  <label> can contain wildcards (*) to test");
H("more than one statement, but each statement is still tested");
H("independently from the others.  Warning:  MINIMIZE_WITH does not check");
H("for $d violations, so SAVE PROOF then VERIFY PROOF should be run");
H("afterwards to check for them.");
H("");
H("Optional qualifier:");
H("    / BRIEF - The labels of statements that were tested but didn't reduce");
H("        the proof length will not be listed, for brevity.");
H("    / ALLOW_GROWTH - If a substitution is possible, it will be made even");
H("        if the proof length increases.  This is useful if we are just");
H("        updating the proof with a newer version of an obsolete theorem.");
H("    / NO_DISTINCT - Skip the trial statement if it has a $d requirement.");
H("        This qualifier is useful when <label> has wildcards, to prevent");
H("        illegal shortenings that would violate $d requirements.");
H("");


printHelp = !strcmp(helpCmd, "HELP SAVE PROOF");
H("Syntax:  SAVE PROOF <label> [/ NORMAL] [/ COMPRESSED]");
H("");
H("The SAVE PROOF command will reformat a proof in one of two formats and");
H("replace the existing proof in the database buffer.  It is useful for");
H("converting between proof formats.  Note that a proof will not be");
H("permanently saved until a WRITE SOURCE command is issued.  Multiple");
H("proofs may be saved by using wildcards (\"*\") in <label>.");
H("");
H("Optional qualifiers:");
H("    / NORMAL - The proof is saved in the basic format (i.e., as a sequence");
H("        of labels, which is the defined format of the basic Metamath");
H("        language).  This is the default format which is used if a");
H("        qualifier is omitted.");
/*
H("    / COMPACT - The proof is saved in the compact format, which uses");
H("        local labels to avoid repetition of identical subproofs.");
*/
H("    / COMPRESSED - The proof is saved in the compressed format which");
H("        reduces storage requirements in a source file.");
H("");


printHelp = !strcmp(helpCmd, "HELP SAVE NEW_PROOF");
H("Syntax:  SAVE NEW_PROOF [/ NORMAL] [/ COMPRESSED]");
H("");
H("The SAVE NEW_PROOF command is available in the Proof Assistant only. It");
H("saves the proof in progress in the database buffer.  SAVE NEW_PROOF may be");
H("used to save a completed proof, or it may be used to save a proof in");
H("progress in order to work on it later.  If an incomplete proof is saved,");
H("any user assignments with LET STEP or LET VARIABLE will be lost, as will");
H("any ambiguous unifications that were resolved manually. To help make");
H("recovery easier, it is advisable to IMPROVE ALL before SAVE NEW_PROOF so");
H("that the incomplete proof will have as much information as possible.");
H("");
H("Note that the proof will not be permanently saved until a WRITE SOURCE");
H("command is issued.");
H("");
H("Optional qualifiers:");
H("    / NORMAL - The proof is saved in the NORMAL format (i.e., as a");
H("        sequence of labels, which is the defined format of the basic");
H("        Metamath language).  This is the default format which is used if a");
H("        qualifier is omitted.");
/*
H("    / COMPACT - The proof is saved in the compact format which uses");
H("        local labels to avoid repetition of identical subproofs.");
*/
H("    / COMPRESSED - The proof is saved in the compressed format which");
H("        reduces storage requirements in a source file.");
H("");


printHelp = !strcmp(helpCmd, "HELP DEMO");
H("For a quick demo that enables you to see Metamath do something, type");
H("the following:");
H("    READ set.mm");
H("    SHOW STATEMENT id1");
H("    SHOW PROOF id1 /ESSENTIAL /RENUMBER /LEMMON");
H("This will show you a proof of \"P implies P\" directly from the axioms");
H("of propositional calculus.");
H("");

}
