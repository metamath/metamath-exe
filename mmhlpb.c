/*****************************************************************************/
/*        Copyright (C) 2020  NORMAN MEGILL  nm at alum.mit.edu              */
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

vstring saveHelpCmd = "";
/* help2() may be called with a temporarily allocated argument (left(),
   cat(), etc.), and the let()s in the eventual print2() calls will
   deallocate and possibly corrupt helpCmd.  So, we grab a non-temporarily
   allocated copy here.  (And after this let(), helpCmd will become invalid
   for the same reason.)  */
let(&saveHelpCmd, helpCmd);

g_printHelp = !strcmp(saveHelpCmd, "HELP");
H("Welcome to Metamath.  Here are some general guidelines.");
H("");
H("To make the most effective use of Metamath, you should become familiar");
H("with the Metamath book.  In particular, you will need to learn");
H("the syntax of the Metamath language.");
H("");
H("For help using the command line, type HELP CLI.");
H("For help invoking Metamath, type HELP INVOKE.");
H("For a summary of the Metamath language, type HELP LANGUAGE.");
H("For a summary of comment markup, type HELP VERIFY MARKUP.");
H("For help getting started, type HELP DEMO.");
H("For help exploring the data base, type HELP EXPLORE.");
H("For help creating a LaTeX file, type HELP TEX.");
H("For help creating Web pages, type HELP HTML.");
H("For help proving new theorems, type HELP PROOF_ASSISTANT.");
H("For a list of help topics, type HELP ? (to force an error message).");
H("For current program settings, type SHOW SETTINGS.");
H("For a simple but general-purpose ASCII file manipulator, type TOOLS.");
H("To exit Metamath, type EXIT (or its synonym QUIT).");
H("");
H(cat("If you need technical support, contact Norman Megill at nm",
    "@", "alum.mit.edu.", NULL));
H("Copyright (C) 2020 Norman Megill  License terms:  GPL 2.0 or later");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP COMMENTS");
H("Comment markup is described near the end of HELP LANGUAGE.  See also");
H("HELP HTML for the $t comment and HTML definitions.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP INVOKE");
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
H("(not single) quotes.  For example, to read the database set.mm, verify");
H("all proofs, and exit the program, type (under Unix)");
H("");
H("  bash$ ./metamath 'read set.mm' 'verify proof *' exit");
H("");
H("Note that in Unix, any directory path with /'s must be surrounded by");
H("quotes so Metamath will not interpret the / as a command qualifier.  So");
H("if set.mm is in the /tmp directory, use for the above example");
H("");
H("  bash$ ./metamath 'read \"/tmp/set.mm\"' 'verify proof *' exit");
H("");
H("For convenience, if the command-line has one argument and no spaces in");
H("the argument, the command is implicitly assumed to be READ.  In this one");
H("special case, /'s are not interpreted as command qualifiers, so you don't");
H("need quotes around a Unix file name.  Thus");
H("");
H("  bash$ ./metamath /tmp/set.mm");
H("");
H("and");
H("");
H("  bash$ ./metamath \"read '/tmp/set.mm'\"");
H("");
H("are equivalent.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW MEMORY");
H("Syntax:  SHOW MEMORY");
H("");
H("This command shows the available memory left.  It is not meaningful");
H("on modern machines with virtual memory.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW SETTINGS");
H("Syntax:  SHOW SETTINGS");
H("");
H("This command shows the state of various parameters.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW ELAPSED_TIME");
H("Syntax:  SHOW ELAPSED_TIME");
H("");
H("This command shows the time elapsed in the session and from any");
H("previous use of SHOW ELAPSED_TIME.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW LABELS");
H("Syntax:  SHOW LABELS <label-match> [/ ALL] [/ LINEAR]");
H("");
H("This command shows the labels of $a and $p statements that match");
H("<label-match>.  <label-match> may contain * and ? wildcard characters;");
H("see HELP SEARCH for wildcard matching rules.");
H("");
H("Optional qualifier:");
H("    / ALL - Include matches for $e and $f statement labels.");
H("    / LINEAR - Display only one label per line.  This can be useful for");
H("        building scripts in conjunction with the TOOLS utility.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW DISCOURAGED");
H("Syntax:  SHOW DISCOURAGED");
H("");
H("This command shows the usage and proof statistics for statements with");
H("\"(Proof modification is discouraged.)\" and \"(New usage is");
H("discouraged.)\" markup tags in their description comments.  The output");
H("is intended to be used by scripts that compare a modified .mm file");
H("to a previous version.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW SOURCE");
H("Syntax:  SHOW SOURCE <label>");
H("");
H("This command shows the ASCII source code associated with a statement.");
H("Normally you should use SHOW STATEMENT for a more meaningful display,");
H("but SHOW SOURCE can be used to see statements with multiple comments");
H("and to see the exact content of the Metamath database.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW STATEMENT");
H("Syntax:  SHOW STATEMENT <label-match> [/ COMMENT] [/ FULL] [/ TEX]");
H("             [/ OLD_TEX] [/ HTML] [/ ALT_HTML] [/ BRIEF_HTML]");
H("             [/ BRIEF_ALT_HTML] [/ NO_VERSIONING] [/ MNEMONICS]");
H("");
H("This command provides information about a statement.  Only statements");
H("that have labels ($f, $e, $a, and $p) may be specified. <label-match>");
H("may contain * and ? wildcard characters; see HELP SEARCH for wildcard");
H("matching rules.");
H("");
H("In Proof Assistant mode (MM-PA prompt), the symbol \"=\" is a synomym");
H("for the label of the statement being proved.  Thus SHOW STATEMENT = will");
H("display the statement being proved.");
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
H("    / OLD_TEX - Same as / TEX, except that LaTeX macros are used to fit");
H("        equations into line.  This mode is obsolete and will be");
H("        removed eventually.");
H("    / HTML - This qualifier invokes a special mode of SHOW STATEMENT which");
H("        creates a Web page for the statement.  It may not be used with");
H("        any other qualifier.  See HELP HTML for more information.");
H("    / ALT_HTML, / BRIEF_HTML, / BRIEF_ALT_HTML - See HELP HTML for more");
H("        information on these.");
H("    / NO_VERSIONING - When used with / HTML or the 3 HTML qualifiers");
H("        above, a backup file suffixed with ~1 is not created (i.e. the");
H("        previous version is overwritten).");
H("    / TIME - When used with / HTML or the 3 HTML qualifiers, prints");
H("        the run time used by each statement.");
H("    / MNEMONICS - Produces the output file mnemosyne.txt for use with");
H("        Mnemosyne http://www.mnemosyne-proj.org/principles.php.  Should");
H("        not be used with any other qualifier.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW PROOF");
H("Syntax:  SHOW PROOF <label-match> [<qualifiers (see below)>]");
H("");
H("This command displays the proof of the specified $p statement various");
H("formats.  The <label-match> may contain \"*\" (0-or-more-character match) and");
H("\"?\" (single-character match) wildcard characters to match multiple");
H("statements.  See HELP SEARCH for other label matching conventions.");
H("Without any qualifiers, only the logical steps will be shown (i.e.");
H("syntax construction steps will be omitted), in indented format.");
H("");
H("Note that a compressed proof will have references to repeated parts of");
H("the proof with \"local labels\" prefixed with \"@\".  To show all steps");
H("in the original RPN proof, you must SAVE PROOF <label-match> / NORMAL before");
H("using SHOW PROOF.");
H("");
H("Most of the time, you will use");
H("    SHOW PROOF <label-match>");
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
H("    / DEPTH <number> - Only steps at less than the specified proof");
H("        tree depth are displayed.  Useful for obtaining an overview of");
H("        the proof.");
H("    / REVERSE - the steps are displayed in reverse order.");
H("    / RENUMBER - when used with / ESSENTIAL, the steps are renumbered");
H("        to correspond only to the essential steps.");
H("    / TEX - the proof is converted to LaTeX and stored in the file opened");
H("        with OPEN TEX.  Tip:  SET WIDTH 120 (or so) to to fit equations");
H("        to LaTeX line.  Then use SHOW PROOF / TEX / LEMMON / RENUMBER.");
H("    / OLD_TEX - same as TEX but uses macros to fit line.  Obsolete and");
H("        will be removed eventually.");
H("    / LEMMON - The proof is displayed in a non-indented format known");
H("        as Lemmon style, with explicit previous step number references.");
H("        If this qualifier is omitted, steps are indented in a tree format.");
H("    / START_COLUMN <number> - Overrides the default column at which");
H("        the formula display starts in a Lemmon style display.  Affects");
H("        only displays using the / LEMMON qualifier.");
H("    / NO_REPEATED_STEPS - When a proof step is identical to an earlier");
H("        step, it will not be repeated.  Instead, a reference to it will be");
H("        changed to a reference to the earlier step.  In particular,");
H("        SHOW PROOF <label-match> / LEMMON / RENUMBER / NO_REPEATED_STEPS");
H("        will have the same proof step numbering as the web page proof");
H("        generated by SHOW STATEMENT  <label-match> / HTML, rather than");
H("        the proof step numbering of the indented format");
H("        SHOW PROOF <label-match> / RENUMBER.  This qualifier affects only");
H("        displays also using the / LEMMON qualifier.");
H("    / STATEMENT_SUMMARY - Summarizes all statements (like a brief SHOW");
H("        STATEMENT) used by the proof.  May not be used with any other");
H("        qualifier except / ESSENTIAL.");
H("    / SIZE - Shows size of the proof in the source.  The size depends on");
H("        how it was last SAVEd (compressed or normal).");
H("    / DETAILED_STEP <step> - Shows the details of what is happening at");
H("        a specific proof step.  May not be used with any other qualifier.");
H("    / NORMAL, / COMPRESSED, / EXPLICIT, / PACKED, / FAST,");
H("        / OLD_COMPRESSION - These qualifiers are the same as for");
H("        SAVE PROOF except that the proof is displayed on the screen in");
H("        a format suitable for manual inclusion in a source file.  See");
H("        HELP SAVE PROOF.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP MIDI");
H("Syntax:  MIDI <label> [/ PARAMETER \"<parameter string>\"]");
H("");
H("This will create a MIDI sound file for the proof of <label>, where <label>");
H("is one of the $p statement labels shown with SHOW LABELS.  The <label> may");
H("contain \"*\" (0-or-more-character match) and \"?\" (single-character");
H("match) wildcard characters to match multiple statements.  For each matched");
H("label, a file will be created called <label>.txt which is a MIDI source");
H("file that can be converted to a MIDI binary file with the \"t2mf\" utility");
H("that can be obtained at:");
H("   http://www.hitsquad.com/smm/programs/mf2t/download.shtml");
H("Note: the MS-DOS version t2mf.exe only handles old-style 8.3 file names,");
H("so files such as pm2.11.txt are rejected and must be renamed to");
H("e.g. pm2_11.txt.");
H("");
H("The parameters are:");
H("");
H("  f = make the tempo fast (default is slow).");
H("  m = make the tempo medium (default is slow).");
H("      Both \"f\" and \"m\" should not be specified simultaneously.");
H("  s = syncopate the melody by silencing repeated notes, using");
H("      a method selected by whether the \"h\" parameter below is also");
H("      present (default is no syncopation).");
H("  h = allow syncopation to hesitate i.e. all notes in a");
H("      sequence of repeated notes are silenced except the first (default");
H("      is no hesitation, which means that every other note in a repeated");
H("      sequence is silenced - this makes it sound slightly more rhythmic).");
H("      The \"h\" parameter is meaningful only if the \"s\" parameter above");
H("      is also present.");
H("  w = use only the white keys on the piano keyboard (default");
H("      is potentially to use all keys).");
H("  b = use only the black keys on the piano keyboard (default");
H("      is all keys).  Both \"w\" and \"b\" should not be specified");
H("      simultaneously.");
H("  i = use an increment of one keyboard note per proof");
H("      indentation level.  The default is to use an automatic increment of");
H("      up to four notes per level based on the dynamic range of the whole");
H("      song.");
H("");
H("Quotes around the parameter string are optional if it has no spaces.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW NEW_PROOF");
H("Syntax:  SHOW NEW_PROOF [<qualifiers (see below)]");
H("");
H("This command (available only in Proof Assistant mode) displays the proof");
H("in progress.  It is identical to the SHOW PROOF command, except that the");
H("statement is not specified (since it is the statement being proved) and");
H("following qualifiers are not available:");
H("    / STATEMENT_SUMMARY");
H("    / DETAILED_STEP");
H("    / FAST");
H("");
H("Also, the following additional qualifiers are available:");
H("    / UNKNOWN - Shows only steps that have no statement assigned.");
H("    / NOT_UNIFIED - Shows only steps that have not been unified.");
H("");
H("Note that / ALL, / DEPTH, / UNKNOWN, and / NOT_UNIFIED may");
H("be used in any combination; each of them effectively filters out (or");
H("\"unfilters\" in the case of / ALL) additional steps from the proof");
H("display.");
H("");
H("See also:  SHOW PROOF");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW USAGE");
H("Syntax:  SHOW USAGE <label-match> [/ RECURSIVE]");
H("");
H("This command lists the statements whose proofs make direct reference to");
H("the statement(s) specified by <label-match>.  <label-match> may contain *");
H("and ? wildcard characters; see HELP SEARCH for wildcard matching rules.");
H("");
H("Optional qualifiers:");
H("    / RECURSIVE - Also include include statements whose proof ultimately");
H("        depend on the statement specified.");
H("    / ALL - Include $e and $f statements.  Without / ALL, $e and $f");
H("        statements are excluded when <label-match> contains wildcard");
H("        characters.");
H("");

let(&saveHelpCmd, ""); /* Deallocate memory */

return;
} /* help2 */


/* Split up help2 into help2 and help3 so lcc optimizer wouldn't overflow */
void help3(vstring helpCmd) {

vstring saveHelpCmd = "";
/* help3() may be called with a temporarily allocated argument (left(),
   cat(), etc.), and the let()s in the eventual print2() calls will
   deallocate and possibly corrupt helpCmd.  So, we grab a non-temporarily
   allocated copy here.  (And after this let(), helpCmd will become invalid
   for the same reason.)  */
let(&saveHelpCmd, helpCmd);




g_printHelp = !strcmp(saveHelpCmd, "HELP SHOW TRACE_BACK");
H("Syntax:  SHOW TRACE_BACK <label-match> [/ ESSENTIAL] [/ AXIOMS] [/ TREE]");
H("             [/ DEPTH <number>] [/ COUNT_STEPS] [/MATCH <label-match>]");
H("             [/TO <label-match>]");
H("");
H("This command lists all statements that the proof of the $p statement(s)");
H("specified by <label-match> depends on.  <label-match> may contain *");
H("and ? wildcard characters; see HELP SEARCH for wildcard matching rules.");
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
H("        expansions of floating hypotheses are not counted.  The steps are");
H("        counted based on how the proof is stored (compressed or normal).");
H("    / MATCH <label-match> - include only statements matching <label-match>");
H("        in the output display.  Undisplayed statements are still used to");
H("        compute the list.  For example, / AXIOMS / MATCH ax-* will show");
H("        set.mm axioms but not definitions.");
H("    / TO <label-match> - include only statements  that depend on the");
H("        <label-match> statement(s).  For example,");
H("        SHOW TRACE_BACK ac6s / TO ax-reg will list all statements");
H("        requiring ax-reg that ac6s depends on.  In case there are");
H("        multiple paths from ac6s back to ax-reg, all statements involved");
H("        in all paths are listed.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SEARCH");
H("Syntax:  SEARCH <label-match> \"<symbol-match>\" [/ ALL] [/ COMMENTS]");
H("             [/ JOIN]");
H("");
H("This command searches all $a and $p statements matching <label-match>");
H("for occurrences of <symbol-match>.");
H("");
H("A * in <label-match> matches any zero or more characters in a label.");
H("");
H("A ? in <label-match> matches any single character.  For example,");
H("SEARCH p?4* \"ph <-> ph\" will check statements whose labels have p and 4");
H("in their first and third character positions, and display them when their");
H("math strings contain \"ph <-> ph\".");
H("");
H("A ~ in <label-match> divides the match into two labels, <from>~<to>, and");
H("matches statements located in the source file range between <from> and");
H("<to> inclusive; <from> and <to> may not have * or ? wildcards; if <from>");
H("(or <to>) is empty, the first (or last) statement will be assumed.");
H("");
H("If <label-match> is % (percent sign), it will match all statements with");
H("proofs that were changed in the current session; cannot be used with");
H("wildcards.");
H("");
H("If <label-match> is = (equals sign), it will match the statement being");
H("proved or last proved in the Proof Assistant (MM-PA); note that");
H("\"PROVE =\" is a quick way to return to the previous MM-PA session.");
H("");
H("If <label-match> is #nnn e.g. #1234, it will match the numeric statement");
H("number nnn; digits cannot contain wildcards.");
H("");
H("If <label-match> is @nnn e.g. @1234, it will match the web page statement");
H("number (small colored number) nnn; digits cannot contain wildcards.");
H("");
H("Multiple <label-match> forms may be joined with commas e.g.");
H("SEARCH ab*,cd* ... will match all labels starting with ab or cd.");
H("");
H("");
H("A $* in <symbol-match> matches any sequence of zero or more tokens");
H("in the statement's math string.");
H("");
H("A $? in <symbol-match> matches zero or one character in a math token.");
H("The quotes surrounding <symbol-match> may be single or double quotes.");
H("For example,");
H("SEARCH * 'E. $? A. $? $?$? -> $* E.' using the set.mm database will find");
H("\"E. x A. y ph -> A. y E. x ph\".  As this example shows, $? is");
H("particularly useful when you don't know what variable names were used in");
H("a theorem of interest.");
H("");
H("");
H("Note 1. The first and last characters of <label-match>, if they are not");
H("wildcards (nor part of ~, =, %, #, or @ forms), will be matched against");
H("the first and last characters of the label.  In contrast, <symbol-match>");
H("is a substring match, i.e. it has implicit $* wildcards before and after");
H("it.");
H("");
H("Note 2. An \"unofficial\" <symbol-match> feature is that $ and ? can be");
H("used instead of $* and $? for brevity provided that no math token");
H("contains a ? character.");
H("");
H("Optional qualifiers:");
H("    / ALL - Also search $e and $f statements.");
H("    / COMMENTS - Search the comment that immediately precedes each");
H("        label-matched statement for <symbol-match>, instead of searching");
H("        the statement's math string.  In this mode, <symbol-match> is an");
H("        arbitrary, non-case-sensitive character string.  Wildcards in");
H("        <symbol-match> are not implemented in this mode.");
H("    / JOIN - In the case of a $a or $p statement, prepend its $e");
H("        hypotheses for searching.  / JOIN has no effect in / COMMENTS");
H("        mode.");
H("");
H("See the last section of HELP LET for how to handle quotes and special");
H("characters.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SET ECHO");
H("Syntax:  SET ECHO ON or SET ECHO OFF");
H("");
H("The SET ECHO ON command will cause command lines to be echoed with any");
H("abbreviations expanded.  While learning the Metamath commands, this");
H("feature will show you the exact command that your abbreviated input");
H("corresponds to.  This is also useful to assist creating robust command");
H("files (see HELP SUBMIT) from your log file (see HELP OPEN LOG).  To make");
H("it easier to extract these lines, \"!\" (which you will discard) is");
H("prepended to each echoed command line.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SET SCROLL");
H("Syntax:  SET SCROLL PROMPTED or SET SCROLL CONTINUOUS");
H("");
H("The Metamath command line interface starts off in the PROMPTED mode,");
H("which means that you will prompted to continue or quit after each");
H("screenful of a long listing.  In CONTINUOUS mode, long listings will be");
H("scrolled without pausing.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SET WIDTH");
H("Syntax:  SET WIDTH <number>");
H("");
H("Metamath assumes the width of your screen is 79 characters.  If your");
H("screen is wider or narrower, this command lets you to change the screen");
H("width.  A larger width is advantageous for logging proofs to an output");
H("file to be printed on a wide printer.  A smaller width may be necessary");
H("on some terminals; in this case, the wrapping of the information");
H("messages may sometimes seem somewhat unnatural, however.  In LaTeX, there");
H("is normally a maximum of 61 characters per line with typewriter font.");
H("");
H("Note:  The default width is 79 because Windows Command Prompt issues a");
H("spurious blank line after an 80-character line (try it!).");
H("");
H("Note:  This command was SET SCREEN_WIDTH prior to Version 0.07.9.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SET HEIGHT");
H("Syntax:  SET HEIGHT <number>");
H("");
H("Metamath assumes your screen height is 24 lines of characters.  If your");
H("screen is taller or shorter, this command lets you to change the number");
H("of lines at which the display pauses and prompts you to continue.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP SET DISCOURAGEMENT");
H("Syntax:  SET DISCOURAGEMENT OFF or SET DISCOURAGEMENT ON");
H("");
H("By default this is set to ON, which means that statements whose");
H("description comments have the markup tags \"(New usage is discouraged.)\"");
H("or \"(Proof modification is discouraged.)\" will be blocked from usage");
H("or proof modification.  When this setting is OFF, those actions are no");
H("longer blocked.  This setting is intended only for the convenience of");
H("advanced users who are intimately familiar with the database, for use");
H("when maintaining \"discouraged\" statements.  SHOW SETTINGS will show you");
H("the current value.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP SET CONTRIBUTOR");
H("Syntax:  SET CONTRIBUTOR <name>");
H("");
H("Specify the contributor name for new \"(Contributed by...\" comment");
H("markup added by SAVE PROOF or SAVE NEW_PROOF.  Use quotes (' or \")");
H("around <name> if it contains spaces.  The current contributor is");
H("displayed by SHOW SETTINGS.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP SET ROOT_DIRECTORY");
H("Syntax:  SET ROOT_DIRECTORY <directory path>");
H("");
H("Specify the directory path (relative to the working directory i.e. the");
H("directory from which the Metamath program was launched) which will be");
H("prepended to READ, WRITE SOURCE, and files included with $[...$].");
H("Enclose <directory path> in single or double quotes if the path contains");
H("\"/\".  A trailing \"/\" will be added automatically if missing.  The");
H("current directory path is displayed by SHOW SETTINGS.");
H("");
H("Use a quoted space (' ' or \" \") for <directory path> if you want to");
H("reset it to be the working directory.");

g_printHelp = !strcmp(saveHelpCmd, "HELP SET UNIFICATION_TIMEOUT");
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
H("Often, a better solution to resolve a unification timeout is to manually");
H("assign some or all of the unknowns (see HELP LET) then try to unify");
H("again.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SET EMPTY_SUBSTITUTION");
H("Syntax:  SET EMPTY_SUBSTITUTION ON or SET EMPTY_SUBSTITUTION OFF");
H("");
H("(This command affects the Proof Assistant only.  It may be issued");
H("outside of the Proof Assistant.)");
H("");
H("The Metamath language allows variables to be substituted with empty");
H("symbol sequences.  However, in most formal systems this will never happen");
H("in a valid proof.  Allowing for this possibility increases the likelihood");
H("of ambiguous unifications during proof creation.  The default is that");
H("empty substitutions are not allowed; for formal systems requiring them,");
H("you must SET EMPTY_SUBSTITUTION ON.  Note that empty substitutions are");
H("always permissible in proof verification (VERIFY PROOF...) outside the");
H("Proof Assistant.  (See the MIU system in the Metamath book for an example");
H("of a system needing empty substitutions; another example would be a");
H("system that implements a Deduction Rule and in which deductions from");
H("empty assumption lists would be permissible.)");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SET SEARCH_LIMIT");
H("Syntax:  SET SEARCH_LIMIT <number>");
H("");
H("(This command affects the Proof Assistant only.)");
H("");
H("This command sets a parameter that determines when the IMPROVE command");
H("in Proof Assistant mode gives up.  If you want IMPROVE to search harder,");
H("you may increase it.  The SHOW SETTINGS command tells you its current");
H("value.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SET JEREMY_HENTY_FILTER");
H("Syntax:  SET JEREMY_HENTY_FILTER ON or SET JEREMY_HENTY_FILTER OFF");
H("");
H("(This command affects the Proof Assistant only.)");
H("");
H("The \"Henty filter\" is an ingenious algorithm suggested by Jeremy Henty");
H("that reduces the number of ambiguous unifications by eliminating");
H("\"equivalent\" ones in a sense defined by Henty.  Normally this filter");
H("is ON, and the only reason to turn it off would be for debugging.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP VERIFY PROOF");
H("Syntax:  VERIFY PROOF <label-match> [/ SYNTAX_ONLY]");
H("");
H("This command verifies the proofs of the specified statements.");
H("<label-match> may contain * and ? wildcard characters to verify more than");
H("one proof; for example \"abc?def*\" will match all labels beginning with");
H("\"abc\" followed by any single character followed by \"def\".");
H("VERIFY PROOF * will verify all proofs in the database.");
H("See HELP SEARCH for complete wildcard matching rules.");
H("");
H("Optional qualifier:");
H("    / SYNTAX_ONLY - This qualifier will perform a check of syntax and RPN");
H("        stack violations only.  It will not verify that the proof is");
H("        correct.");
H("");
H("Note: READ, followed by VERIFY PROOF *, will ensure the database is free");
H("from errors in Metamath language but will not check the markup language");
H("in comments.  See HELP VERIFY MARKUP.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP VERIFY MARKUP");
H("Syntax:  VERIFY MARKUP <label-match> [/ DATE_SKIP] [/ TOP_DATE_SKIP]");
H("            [/ FILE_SKIP] [/ UNDERSCORE_SKIP] [/ MATHBOX_SKIP] [/VERBOSE]");
H("");
H("This command checks comment markup and other informal conventions we have");
H("adopted.  It error-checks the latexdef, htmldef, and althtmldef statements");
H("in the $t statement of a Metamath source file.  It error-checks any `...`,");
H("~ <label>, and [bibref] markups in statement descriptions.  It checks that");
H("$p and $a statements have the same content when their labels start with");
H("\"ax\" and \"ax-\" respectively but are otherwise identical, for example");
H("ax4 and ax-4.  It verifies the date consistency of \"(Contributed by...)\",");
H("\"(Revised by...)\", and \"(Proof shortened by...)\" tags in the comment");
H("above each $a and $p statement.  See HELP SEARCH for <label-match> rules.");
H("");
H("Optional qualifiers:");
H("    / DATE_SKIP - This qualifier will skip date consistency checking,");
H("        which is usually not required for databases other than set.mm");
H("    / TOP_DATE_SKIP - This qualifier will check date consistency except");
H("        that the version date at the top of the database file will not");
H("        be checked.  Only one of / DATE_SKIP and / TOP_DATE_SKIP may be");
H("        specified.");
H("    / FILE_SKIP - This qualifier will skip checks that require");
H("        external files to be present, such as checking GIF existence and");
H("        bibliographic links to mmset.html or equivalent.  It is useful");
H("        for doing a quick check from a directory without these files");
H("    / UNDERSCORE_SKIP - This qualifier will skip warnings for labels");
H("        containing underscore (\"_\") characters.  Although they are");
H("        legal per the Metamath spec, they may cause ambiguities with");
H("        certain translators (such as to MM0) that convert \"-\" to \"_\".");
H("        bibliographic links to mmset.html or equivalent.  It is useful");
H("        for doing a quick check from a directory without these files");
H("    / MATHBOX_SKIP - This qualifier will skip checking for mathbox");
H("        independence i.e. that no mathbox proof references a statement");
H("        in another (earlier) mathbox.");
H("    / VERBOSE - Provides more information.  Currently it provides a list");
H("        of axXXX vs. ax-XXX matches.");
H("");
H("See also HELP LANGUAGE, HELP HTML, HELP WRITE THEOREM_LIST, and");
H("HELP SET DISCOURAGEMENT for more details on the markup syntax.");
H("See the 11-May-2016 (\"is discouraged\"), 14-May-2017 (date format), and");
H("21-Dec-2017 (file inclusion) entries in");
H("http://us.metamath.org/mpeuni/mmnotes.txt for further details on several");
H("kinds of markup.  See HELP WRITE THEOREM_LIST for format of section headers.");
H("");
H("For help with modularization tags such as \"$( Begin $[ set-header.mm $] $)\",");
H("see the 21-Dec-2017 entry in http://us.metamath.org/mpeuni/mmnotes.txt .");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SUBMIT");
H("Syntax:  SUBMIT <filename> [/ SILENT]");
H("");
H("This command causes further command lines to be taken from the specified");
H("command file.  Note that any line beginning with an exclamation point (!)");
H("is treated as a comment (i.e. ignored).  Also note that the scrolling");
H("of the screen output is continuous, so you may want to open a log file");
H("(see HELP OPEN LOG) to record the results that fly by on the screen.");
H("After the lines in the command file are exhausted, Metamath returns to");
H("its normal user interface mode.");
H("");
H("SUBMIT commands can occur inside of a SUBMIT command file, up to 10 levels");
H("deep (determined by MAX_COMMAND_FILE_NESTING in mminou.h.");
H("");
H("Optional qualifier:");
H("    / SILENT - This qualifier suppresses the screen output of the SUBMIT");
H("        command.  The output will still be recorded in any log file that");
H("        has been opened with OPEN LOG (or is opened inside the command");
H("        file itself).  The screen output of any operating system commands");
H("        inside the command file (see HELP SYSTEM) is not suppressed.");


g_printHelp = !strcmp(saveHelpCmd, "HELP SYSTEM");
H("A line enclosed in single or double quotes will be executed by your");
H("computer's operating system, if it has such a feature.  For example, on a");
H("GNU/Linux system,");
H("    MM> 'ls | less -EX'");
H("will list disk directory contents.  Note that this feature will not work");
H("on the pre-OSX Macintosh, which does not have a command line interface.");
H("");
H("For your convenience, the trailing quote is optional, for example:");
H("    MM> 'ls | less -EX");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP MM-PA");
H("See HELP PROOF_ASSISTANT");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP MORE");
H("Syntax:  MORE <filename>");
H("");
H("This command will type (i.e. display) the contents of an ASCII file on");
H("your screen.  (This command is provided for convenience but is not very");
H("powerful.  See HELP SYSTEM to invoke your operating system's command to");
H("do this, such as \"less -EX\" in Linux.)");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP FILE SEARCH");
H("Syntax:  FILE SEARCH <filename> \"<search string>\" [/ FROM_LINE");
H("             <number>] [/ TO_LINE <number>]");
H("");
H("This command will search an ASCII file for the specified string in");
H("quotes, within an optional range of line numbers, and display the result");
H("on your screen.  The search is case-insensitive.  (This command is");
H("deprecated.  See HELP SYSTEM to invoke your operating system's");
H("equivalent command, such as \"grep\" in Linux.)");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP PROVE");
H("Syntax:  PROVE <label> [/ OVERRIDE]");
H("");
H("This command will enter the Proof Assistant, which will allow you to");
H("create or edit the proof of the specified statement.");
H("");
H("Optional qualifier:");
H("    / OVERRIDE - By default, PROVE will refuse to enter the Proof");
H("        Assistant if \"(Proof modification is discouraged.)\" is present");
H("        in the statement's description comment.  This qualifier will");
H("        allow the Proof Assistant to be entered.");
H("");
H("See also:  HELP PROOF_ASSISTANT and HELP EXIT");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP PROOF_ASSISTANT");
H("Before using the Proof Assistant, you must add a $p to your source file");
H("(using a text editor) containing the statement you want to prove.  Its");
H("proof should consist of a single ?, meaning \"unknown step.\"  Example:");
H("    eqid $p x = x $= ? $.");
H("");
H("To enter the Proof Assistant, type PROVE <label>, e.g. PROVE eqid.");
H("Metamath will respond with the MM-PA> prompt.");
H("");
H("Proofs are created working backwards from the statement being proved,");
H("primarily using a series of ASSIGN commands.  A proof is complete when");
H("all steps are assigned to statements and all steps are unified and");
H("completely known.  During the creation of a proof, Metamath will allow");
H("only operations that are legal based on what is known up to that point.");
H("For example, it will not allow an ASSIGN of a statement that cannot be");
H("unified with the unknown proof step being assigned.");
H("");
H("IMPORTANT:  You should figure out your first few proofs completely and");
H("write them down by hand, before using the Proof Assistant.  Otherwise you");
H("will become extremely frustrated.  The Proof Assistant is NOT a tool to");
H("help you discover proofs.  It is just a tool to help you add them to the");
H("database.  For a tutorial read Section 2.4 of the Metamath book.  To");
H("practice using the Proof Assistant, you may want to PROVE an existing");
H("theorem, then delete all steps with DELETE ALL, then re-create it with");
H("the Proof Assistant while looking at its proof display (before deletion).");
H("");
H("The commands available to help you create a proof are the following.");
H("See the help for the individual commands for more detail.");
H("    SHOW NEW_PROOF [/ ALL,...] - Displays the proof in progress.");
H("        You will use this command a lot; see HELP SHOW NEW_PROOF to");
H("        become familiar with its qualifiers.  The qualifiers / UNKNOWN");
H("        and / NOT_UNIFIED are useful for seeing the work remaining to be");
H("        done.  The combination / ALL / UNKNOWN is useful identifying");
H("        dummy variables that must be assigned, or attempts to use illegal");
H("        syntax, when IMPROVE ALL is unable to complete the syntax");
H("        constructions.  Unknown variables are shown as $1, $2,...");
H("    ASSIGN <step> <label> - Assigns an unknown step with the statement");
H("        specified by <label>.  This will normally be your most frequently");
H("        used command for creating proofs.  The usual proof entry process");
H("        consists of successively ASSIGNing labels to unknown steps shown");
H("        by SHOW NEW_PROOF / UNKNOWN.");
H("    LET VARIABLE <variable> = \"<symbol sequence>\" - Forces a symbol");
H("        sequence to replace an unknown variable in a proof.  It is useful");
H("        for helping difficult unifications, and is necessary when you");
H("        have dummy variables that must be specified.");
H("    LET STEP <step> = \"<symbol sequence>\" - Forces a symbol sequence");
H("        to replace the contents of a proof step, provided it can be");
H("        unified with the existing step contents.  (Rarely useful.)");
H("    UNIFY STEP <step> (or UNIFY ALL) - Unifies the source and target of");
H("        a step.  If you specify a specific step, you will be prompted");
H("        to select among the unifications that are possible.  If you");
H("        specify ALL, only those steps with unique unifications will be");
H("        unified.  UNIFY ALL / INTERACTIVE goes through all non-unified");
H("        steps.");
H("    INITIALIZE <step> (or ALL) - De-unifies the target and source of");
H("        a step (or all steps), as well as the hypotheses of the source,");
H("        and makes all variables in the source unknown.  Useful after");
H("        an ASSIGN or LET mistake resulted in incorrect unifications.");
H("    DELETE <step> (or ALL or FLOATING_HYPOTHESES) - Deletes the specified");
H("        step(s).  DELETE FLOATING_HYPOTHESES then INITIALIZE ALL then");
H("        UNIFY ALL / INTERACTIVE is useful for recovering from mistakes");
H("        where incorrect unifications assigned wrong math symbol strings to");
H("        variables.");
H("    IMPROVE <step> (or ALL) - Automatically creates a proof for steps");
H("        (with no unknown variables) whose proof requires no statements");
H("        with $e hypotheses.  Useful for filling in proofs of $f");
H("        hypotheses.  The / DEPTH qualifier will also try statements");
H("        whose $e hypotheses contain no new variables.  WARNING: Save your");
H("        work (SAVE NEW_PROOF, WRITE SOURCE) before using / DEPTH = 2 or");
H("        greater, since the search time grows exponentially and may never");
H("        terminate in a reasonable time, and you cannot interrupt the");
H("        search.  I have rarely found / DEPTH = 3 or greater to be useful.");
H("    SAVE NEW_PROOF - Saves the proof in progress internally in the");
H("        database buffer.  To save it permanently, use WRITE SOURCE after");
H("        SAVE NEW_PROOF.  To revert to the last SAVE NEW_PROOF,");
H("        EXIT / FORCE from the Proof Assistant then re-enter the Proof");
H("        Assistant.");
H("    SHOW NEW_PROOF / COMPRESSED - Displays the proof in progress on the");
H("        screen in a format that can be copied and pasted into the");
H("        database source, as an alternative to a SAVE NEW_PROOF.");
H("    MATCH STEP <step> (or MATCH ALL) - Shows what statements are");
H("        possibilities for the ASSIGN statement. (This command is not very");
H("        useful in its present form and hopefully will be improved");
H("        eventually.  In the meantime, use the SEARCH statement for");
H("        candidates matching specific math token combinations.)");
H("    MINIMIZE_WITH - After a proof is complete, this command will attempt");
H("        to match other database theorems to the proof to see if the proof");
H("        size can be reduced as a result.");
H("    UNDO - Undo the effect of a proof-changing command (all but the SHOW");
H("        and SAVE commands above).");
H("    REDO - Reverse the previous UNDO.");
H("");
H("The following commands set parameters that may be relevant to your proof:");
H("    SET UNIFICATION_TIMEOUT");
H("    SET SEARCH_LIMIT");
H("    SET EMPTY_SUBSTITUTION - note that default is OFF (contrary to book)");
H("");
H("Type EXIT to exit the MM-PA> prompt and get back to the MM> prompt.");
H("Another EXIT will then get you out of Metamath.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP UNDO");
H("Syntax:  UNDO");
H("");
H("This command, available in the Proof Assistant only, allows any command");
H("(such as ASSIGN, DELETE, IMPROVE) that affects the proof to be reversed.");
H("See also HELP REDO and HELP SET UNDO.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP REDO");
H("Syntax:  REDO");
H("");
H("This command, available in the Proof Assistant only, reverses the");
H("effect of the last UNDO command.  Note that REDO can be issued only");
H("if no proof-changing commands (such as ASSIGN, DELETE, IMPROVE)");
H("were issued after the last UNDO.  A sequence of REDOs will reverse as");
H("many UNDOs as were issued since the last proof-changing command.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SET UNDO");
H("Syntax:  SET UNDO <number>");
H("");
H("(This command affects the Proof Assistant only.)");
H("");
H("This command changes the maximum number of UNDOs.  The current maximum");
H("can be seen with SHOW SETTINGS.  Making it larger uses more memory,");
H("especially for large proofs.  See also HELP UNDO.");
H("");
H("If this command is issued while inside of the Proof Assistant, the");
H("UNDO stack is reset (i.e. previous possible UNDOs will be lost).");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP ASSIGN");
H("Syntax:  ASSIGN <step> <label> [/ NO_UNIFY] [/ OVERRIDE]");
H("         ASSIGN FIRST <label> [/ NO_UNIFY] [/ OVERRIDE]");
H("         ASSIGN LAST <label> [/ NO_UNIFY] [/ OVERRIDE]");
H("");
H("This command, available in the Proof Assistant only, assigns an unknown");
H("step (one with ? in the SHOW NEW_PROOF listing) with the statement");
H("specified by <label>.  The assignment will not be allowed if the");
H("statement cannot be unified with the step.");
H("");
H("If <step> starts with \"-\", the -<step>th from last unknown step,");
H("as shown by SHOW NEW_PROOF / UNKNOWN, will be used.  ASSIGN -0 will");
H("assign the last unknown step, ASSIGN -1 <label> will assign the");
H("penultimate unknown step, etc.  If <step> starts with \"+\", the <step>th");
H("from the first unknown step will be used.  Otherwise, when the step is");
H("a positive integer (with no \"+\" sign), ASSIGN assumes it is the actual");
H("step number shown by SHOW NEW_PROOF / UNKNOWN.");
H("");
H("ASSIGN FIRST and ASSIGN LAST mean ASSIGN +0 and ASSIGN -0 respectively,");
H("in other words the first and last steps shown by SHOW NEW_PROOF / UNKNOWN.");
H("");
H("Optional qualifiers:");
H("    / NO_UNIFY - do not prompt user to select a unification if there is");
H("        more than one possibility.  This is useful for noninteractive");
H("        command files.  Later, the user can UNIFY ALL / INTERACTIVE.");
H("        (The assignment will still be automatically unified if there is");
H("        only one possibility.)");
H("    / OVERRIDE - By default, ASSIGN will refuse to assign a statement");
H("        if \"(New usage is discouraged.)\" is present in the statement's");
H("        description comment.  This qualifier will allow the assignment.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP REPLACE");
H("Syntax:  REPLACE <step> <label> [/ OVERRIDE]");
H("Syntax:  REPLACE FIRST <label> [/ OVERRIDE]");
H("Syntax:  REPLACE LAST <label> [/ OVERRIDE]");
H("");
H("This command, available in the Proof Assistant only, replaces the");
H("current subproof ending at <step> with a new complete subproof (if one");
H("can be found) ending with statement <label>.  The replacement will be");
H("done only if complete subproofs can be found that match all of the");
H("hypotheses of  <label>.  REPLACE is equivalent to IMPROVE with the");
H("qualifiers / 3 / DEPTH 1 / SUBPROOFS (see HELP IMPROVE), except that");
H("it considers only statement <label> rather than scanning all preceding");
H("statements in the database, and it does somewhat more aggressive");
H("guessing of assignments to work ($nn) variables.");
H("");
H("REPLACE will also assign a complete subproof to a currently unknown");
H("(unassigned) step if a complete subproof can be found.  In many cases,");
H("REPLACE provides an alternative to ASSIGN (with the same command syntax)");
H("that will fill in more missing steps when it is successful.  It is often");
H("useful to try REPLACE first and, if not successful, revert to ASSIGN.");
H("Note that REPLACE may take a long time to run compared to ASSIGN.");
H("");
H("Currently, REPLACE does not allow a $e or $f statement for <label>.  Use");
H("ASSIGN instead.  (These may be allowed in a future version.)");
H("");
H("Occasionally, REPLACE may be too aggressive in guessing assignments to");
H("work ($nn) variables, and a message with recovery instructions is");
H("provided when this could be the case.  Recovery can also be attempted with");
H("DELETE FLOATING_HYPOTHESES then INITIALIZE ALL then");
H("UNIFY ALL / INTERACTIVE; this will usually work and will salvage the");
H("subproof found by REPLACE.  (The too-aggressive guessing behavior may be");
H("improved in a future version.)");
H("");
H("If <step> starts with \"-\", the -<step>th from last unknown step,");
H("as shown by SHOW NEW_PROOF / UNKNOWN, will be used.  REPLACE -0 will");
H("assign the last unknown step, REPLACE -1 <label> will assign the");
H("penultimate unknown step, etc.  If <step> starts with \"+\", the <step>th");
H("from the first unknown step will be used.  Otherwise, when the step is");
H("a positive integer (with no \"+\" sign), REPLACE assumes it is the actual");
H("step number shown by SHOW NEW_PROOF (and can be used whether the step is");
H("known or not).");
H("");
H("REPLACE FIRST and REPLACE LAST mean REPLACE +0 and REPLACE -0");
H("respectively, in other words the first and last steps shown by");
H("SHOW NEW_PROOF / UNKNOWN.");
H("");
H("Optional qualifier:");
H("    / OVERRIDE - By default, REPLACE will refuse to assign a statement");
H("        if \"(New usage is discouraged.)\" is present in the statement's");
H("        description comment.  This qualifier will allow the assignment.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP MATCH");
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
H("        would be listed in SHOW NEW_PROOF display are matched.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP LET");
H("Syntax:  LET VARIABLE <variable> = \"<symbol sequence>\"");
H("         LET STEP <step> = \"<symbol sequence>\"");
H("         LET STEP FIRST = \"<symbol sequence>\"");
H("         LET STEP LAST = \"<symbol sequence>\"");
H("");
H("These commands, available in the Proof Assistant only, assign an unknown");
H("variable or step with a specific symbol sequence.  They are useful in the");
H("middle of creating a proof, when you know what should be in the proof");
H("step but the unification algorithm doesn't yet have enough information");
H("to completely specify the unknown variables.  An \"unknown\" or \"work\"");
H("variable is one which has the form $nn in the proof display, such as $1,");
H("$2, etc.  The <symbol sequence> may contain other unknown variables if");
H("desired.");
H("Examples:");
H("    LET VARIABLE $32 = \" A = B \"");
H("    LET VARIABLE $32 = \" A = $35 \"");
H("    LET STEP 10 = \" |- x = x \"");
H("    LET STEP -2 = \" |- ( $7 <-> ph ) \"");
H("    LET STEP +2 = \" |- ( $7 <-> ph ) \"");
H("");
H("Any symbol sequence will be accepted for the LET VARIABLE command.  In");
H("LET STEP, only symbol sequences that can be unified with the step are");
H("accepted.  LET STEP assignments are prefixed with \"=(User)\" in most");
H("SHOW NEW_PROOF displays.");
H("");
H("If <step> starts with \"-\", the -<step>th from last unknown step,");
H("as shown by SHOW NEW_PROOF / UNKNOWN, will be used.  LET STEP -0 will");
H("assign the last unknown step, LET STEP -1 <label> will assign the");
H("penultimate unknown step, etc.  If <step> starts with \"+\", the <step>th");
H("from the first unknown step will be used.  LET STEP FIRST and LET STEP");
H("LAST means LET STEP +0 and LET STEP -0 respectively.  Otherwise, when");
H("the step is a positive integer (with no \"+\" sign), LET STEP may be");
H("used to assign known as well as unknown steps.");
H("");
H("Note that SAVE PROOF does not save any LET VARIABLE or LET STEP");
H("assignents.  However, IMPROVE ALL prior to SAVE PROOF will usually");
H("preserve the information for steps with no unknown variables.");
H("");
H("Quotes and special characters in command-line arguments");
H("-------------------------------------------------------");
H("");
H("You can use single quotes to surround the math symbol string argument of");
H("a LET or SEARCH command when the argument contains a double quote.");
H("For example,");
H("  MM-PA> LET VARIABLE $2 = '( F \" A )'");
H("");
H("The trailing quote that would be the last character on the line");
H("may be omitted for convenience, to save typing:");
H("  MM-PA> LET VARIABLE $2 = '( F \" A )");
H("");
H("If a math symbol string has both single and double quotes, you must");
H("use the prompted completion feature by pressing RETURN where the symbol");
H("string would go.  Quotes aren't needed (and must not be used) around");
H("the answer to a prompted completion question.");
H("  MM-PA> LET VARIABLE $2 =");
H("  With what math symbol string? ( `' F \" A )");
H("");
H("Quotes are optional around any math symbol string containing a single");
H("symbol and and not containing \"=\", \"/\", a single quote, or a double");
H("quote:");
H("  MM-PA> LET VARIABLE $17 = ph");
H("");
H("Unquoted \"=\" and \"/\" are special in a command line in that they are");
H("implicitly surrounded by white space:");
H("  MM-PA> LET VARIABLE $17=ph");
H("  MM-PA> SHOW NEW_PROOF/UNKNOWN");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP UNIFY");
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
H("See also SET UNIFICATION_TIMEOUT.  The default is 100000, but increasing");
H("it to 1000000 can help difficult cases.  The LET VARIABLE command to");
H("manually assign unknown variables also helps difficult cases.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP INITIALIZE");
H("Syntax:  INITIALIZE ALL");
H("         INITIALIZE USER");
H("         INITIALIZE STEP <step>");
H("");
H("These commands, available in the Proof Assistant only, \"de-unify\" the");
H("target and source of a step (or all steps), as well as the hypotheses of");
H("the source, and make all variables in the source and the source's");
H("hypotheses unknown.  This command is useful to help recover when an");
H("ASSIGN mistake resulted in incorrect unifications.  After you DELETE the");
H("incorrect ASSIGN, use INITIALIZE ALL then UNIFY ALL / INTERACTIVE to");
H("recover the state before the mistake.");
H("");
H("INITIALIZE ALL will void all LET VARIABLE assignments as well as de-unify");
H("all targets and sources.  INITIALIZE USER will delete all LET STEP");
H("assignments but will not de-unify.  INITIALIZE STEP will do all of these");
H("actions for the specified step.");
H("");
H("After de-unification and variable deassignment, proof steps that are");
H("completely known will be automatically re-unified.  If you want to");
H("de-unify these, use DELETE FLOATING_HYPOTHESES then INITIALIZE ALL.");
H("");
H("See also:  UNIFY and DELETE");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP DELETE");
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


g_printHelp = !strcmp(saveHelpCmd, "HELP IMPROVE");
H("Syntax:  IMPROVE <step> [/ DEPTH <number>] [/ NO_DISTINCT] [/ 2] [/ 3]");
H("                       [/ SUBPROOFS] [/ INCLUDE_MATHBOXES] [/ OVERRIDE]");
H("         IMPROVE FIRST [/ DEPTH <number>] [/ NO_DISTINCT] [/ 2] [/ 3]");
H("                       [/ SUBPROOFS] [/ INCLUDE_MATHBOXES] [/ OVERRIDE]");
H("         IMPROVE LAST [/ DEPTH <number>] [/ NO_DISTINCT] [/ 2] [/ 3]");
H("                       [/ SUBPROOFS] [/ INCLUDE_MATHBOXES] [/ OVERRIDE]");
H("         IMPROVE ALL [/ DEPTH <number>] [/ NO_DISTINCT] [/ 2] [/ 3]");
H("                       [/ SUBPROOFS] [/ INCLUDE_MATHBOXES] [/ OVERRIDE]");
H("");
H("This command, available in the Proof Assistant only, tries to");
H("find proofs automatically for unknown steps whose symbol sequences are");
H("completely known.  They are primarily useful for filling in proofs of $f");
H("hypotheses.  By default, the search will be restricted to matching");
H("statements having no $e hypotheses.");
H("");
H("If <step> starts with \"-\", the -<step>th from last unknown step,");
H("as shown by SHOW NEW_PROOF / UNKNOWN, will be used.  IMPROVE -0 will try");
H("to prove the last unknown step, IMPROVE -1 will try to prove the");
H("penultimate unknown step, etc.  If <step> starts with \"+\", the <step>th");
H("from the first unknown step will be used.  Otherwise, when <step> is");
H("a positive integer (with no \"+\" sign), IMPROVE assumes it is the actual");
H("step number shown by SHOW NEW_PROOF (and can be used whether the step is");
H("known or not).");
H("");
H("IMPROVE FIRST and IMPROVE LAST mean IMPROVE +0 and IMPROVE -0");
H("respectively, in other words the first and last steps shown by");
H("SHOW NEW_PROOF / UNKNOWN.");
H("");
H("IMPROVE ALL scans all unknown steps.  If / SUBPROOFS is specified,");
H("it also scans all steps with incomplete subproofs.");
H("");
H("Sometimes IMPROVE will find proofs for additional unknown steps when");
H("it is run a second time.  This can happen when an unknown step is");
H("identical to another step whose proof became completed by the first");
H("IMPROVE run.  (This second pass is not done automatically because it");
H("could double the IMPROVE runtime, usually with no benefit.)");

H("");
H("Optional qualifiers:");
H("    / DEPTH <number> - This qualifier will cause the search to include");
H("        statements with $e hypotheses (but no new variables in their $e");
H("        hypotheses - these are called \"cut-free\" statements), provided");
H("        that the backtracking has not exceeded the specified depth.");
H("        **WARNING**:  Try DEPTH 1, then 2, then 3, etc. in sequence");
H("        because of possible exponential blowups.  Save your work before");
H("        trying DEPTH greater than 1!");
H("    / NO_DISTINCT - Skip trial statements that have $d requirements.");
H("        This qualifier will prevent assignments that might violate $d");
H("        requirements, but it also could miss possible legal assignments.");
H("    / 1 - Use the traditional search algorithm used in earlier versions");
H("        of this program.  It is the default.  It tries to match cut-free");
H("        statements only (those not having variables in their hypotheses");
H("        that are not in the conclusion).  It is the fastest method when");
H("        it can find a proof.");
H("    / 2 - Try to match statements with cuts.  It also tries to match");
H("        steps containing working ($nn) variables when they don't share");
H("        working variables with the rest of the proof.  It runs slower");
H("        than / 1.");
H("    / 3 - Attempt to find (cut-free) proofs of $e hypotheses that result");
H("        from a trial match, unlike / 2, which only attempts (cut-free)");
H("        proofs of $f hypotheses.  It runs much slower than / 1, and you");
H("        may prefer to use it with specific steps.  For example, if");
H("        step 456 is unknown, you may want to use IMPROVE 456 / 3 rather");
H("        than IMPROVE ALL / 3.  Note that / 3 respects the / DEPTH");
H("        qualifier, although at the expense of additional run time.");
H("    / SUBPROOFS - Look at each subproof that isn't completely known, and");
H("        try to see if it can be proved independently.  This qualifier is");
H("        meaningful only for IMPROVE ALL / 2 or IMPROVE ALL / 3.  It may");
H("        take a very long time to run, especially with / 3.");
H("    / INCLUDE_MATHBOXES - By default, MINIMIZE_WITH skips statements");
H("        beyond the one with label \"mathbox\" and not in the mathbox of");
H("        the PROVE argument.  This qualifier allows them to be included.");
H("    / OVERRIDE - By default, IMPROVE skips statements that have");
H("        \"(New usage is discouraged.)\" in their description comment.");
H("        This qualifier tries to use them anyway.");
H("");
H("Note that / 2 includes the search of / 1, and / 3 includes / 2.");
H("Specifying / 1 / 2 / 3 has the same effect as specifying just / 3, so");
H("there is no need to specify more than one.  Finally, since / 1 is the");
H("default, you never need to use it; it is included for completeness (or");
H("in case the default is changed in the future).");
H("");
H("See also:  HELP SET SEARCH_LIMIT");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP MINIMIZE_WITH");
H("Syntax:  MINIMIZE_WITH <label-match> [/ VERBOSE] [/ MAY_GROW]");
H("              [/ EXCEPT <label-match>] [/ INCLUDE_MATHBOXES]");
H("              [/ ALLOW_NEW_AXIOMS <label-match>]");
H("              [/ NO_NEW_AXIOMS_FROM <label-match>] [/ FORBID <label-match>]");
H("              [/ OVERRIDE] [/ TIME]");
H("");
H("This command, available in the Proof Assistant only, checks whether");
H("the proof can be shortened by using earlier $p or $a statements matching");
H("<label-match>, and if so, shortens the proof.  <label-match> is a list of");
H("comma-separated labels which can contain wildcards (* and ?) to test more");
H("than one statement, but each statement is tested independently from the");
H("others.  Note:  In the informational output, if the size is given in");
H("bytes, it refers to the compressed proof size, otherwise it refers to the");
H("number of steps in the uncompressed proof.");
H("");
H("For ordinary use with set.mm, we recommend running it as follows:");
H("   MINIMIZE_WITH * / ALLOW_NEW_AXIOMS * / NO_NEW_AXIOMS_FROM ax-*");
H("For some additional information on the qualifiers see");
H("    https://groups.google.com/d/msg/metamath/f-L91-1jI24/3KJnGa8qCgAJ");
H("");
H("Optional qualifiers:");
H("    / VERBOSE - Shows additional information such as uncompressed proof");
H("        lengths and reverted shortening attempts");
H("    / MAY_GROW - If a substitution is possible, it will be made even");
H("        if the proof length increases.  This is useful if we are just");
H("        updating the proof with a newer version of an obsolete theorem.");
H("        (Note: this qualifier used to be named / ALLOW_GROWTH).");
H("    / EXCEPT <label-match> - Skip trial statements matching <label-match>,");
H("        which may contain * and ? wildcard characters; see HELP SEARCH");
H("        for wildcard matching rules.  Note:  Multiple EXCEPT qualifiers");
H("        are not allowed; use wildcards instead.");
H("    / INCLUDE_MATHBOXES - By default, MINIMIZE_WITH skips statements");
H("        beyond the one with label \"mathbox\" and not in the mathbox of");
H("        the PROVE argument.  This qualifier allows them to be included.");
H("    / ALLOW_NEW_AXIOMS <label-match> - By default, MINIMIZE_WITH skips");
H("        statements that depend on $a statements not already used by the");
H("        proof.  This qualifier allows new $a consequences to be used.");
H("        To better fine-tune which axioms are used, you may use / FORBID");
H("        and / NO_NEW_AXIOMS, which take priority over / ALLOW_NEW_AXOMS.");
H("        Example:  / ALLOW_NEW_AXIOMS df-* will allow new definitions to be");
H("        used. / ALLOW_NEW_AXIOMS * / NO_NEW_AXIOMS_FROM ax-ac*,ax-reg");
H("        will allow any new axioms except those matching ax-ac*,ax-reg.");
H("    / NO_NEW_AXIOMS_FROM <label-match> - skip any trial statement whose");
H("        proof depends on a $a statement matching <label-match> but that");
H("        isn't used by the current proof.  This makes it easier to avoid");
H("        say ax-ac if the current proof doesn't already use ax-ac, but it");
H("        permits ax-ac otherwise.  Example:");
H("        / ALLOW_NEW_AXIOMS * / NO_NEW_AXIOMS_FROM ax-ac*,ax-reg");
H("        will allow any new axioms except those matching ax-ac*,ax-reg.");
H("        Notes:  1. In this example, if ax-reg is already used by the proof,");
H("        statements depending on ax-reg WILL be tried. 2. The use of");
H("        / NO_NEW_AXIOMS_FROM without / ALLOW_NEW_AXIOMS has no effect.");
H("    / FORBID <label-match> - Skip any trial");
H("        statement whose backtrack (from SHOW TRACE_BACK) contains any");
H("        statement matching <label-match>.  This is useful for avoiding");
H("        the use of undesired axioms when reducing proof length.  For");
H("        example, MINIMIZE_WITH ... / FORBID ax-ac,ax-inf* will not shorten");
H("        the proof with any statement that depends on ax-ac, ax-inf, or");
H("        ax-inf2 (in the set.mm as of this writing).  Notes: 1. / FORBID");
H("        can be less useful than / NO_NEW_AXIOMS_FROM because it will");
H("        also suppress trying statements that depend on <label-list> axioms");
H("        already used by the proof.  / FORBID may become deprecated.  2. The");
H("        use of / FORBID without / ALLOW_NEW_AXIOMS has no effect.");
H("    / OVERRIDE - By default, MINIMIZE_WITH skips statements that have");
H("        \"(New usage is discouraged.)\" in their description comment.");
H("        With this qualifier it will try to use them anyway.");
H("    / TIME - prints out the run time used by the MINIMIZE_WITH run.");
H("");



g_printHelp = !strcmp(saveHelpCmd, "HELP EXPAND");
H("Syntax:  EXPAND <label-match>");
H("");
H("This command, available in the Proof Assistant only, replaces any");
H("references to <label-match> in the proof with the proof of <label-match>.");
H("Before and after sizes are printed for the resulting compressed proof (in");
H("bytes of source text).  Any dummy variables in the proof of <label-match>");
H("are replaced with unknown steps and must be assigned by hand, and any $d");
H("statements needed for them must be added by hand.");
H("");
H("Except for early theorems close to the axioms, it is best to use specific");
H("labels rather than EXPAND * because the proof size grows exponentially as");
H("each layer is eliminated.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SAVE PROOF");
H("Syntax:  SAVE PROOF <label-match> [/ <qualifier>] [/ <qualfier>]...");
H("");
H("The SAVE PROOF command will reformat a proof in one of two formats and");
H("replace the existing proof in the database buffer.  It is useful for");
H("converting between proof formats.  Note that a proof will not be");
H("permanently saved until a WRITE SOURCE command is issued.  Multiple");
H("proofs can be saved by using comma-separated list of labels with optional");
H("wildcards (* and ?) in <label-match>.");
H("");
H("Optional qualifiers:");
H("    / NORMAL - The proof is saved in the basic format (i.e., as a sequence");
H("        of labels, which is the defined format of the basic Metamath");
H("        language).  This is the default format which is used if a");
H("        qualifier is omitted.");
H("    / EXPLICIT - The proof is saved with hypothesis assignments shown");
H("        explicitly, allowing hypotheses to be reordered without disturbing");
H("        proofs.");
H("    / PACKED - The proof is saved in an efficient packed format.  It may");
H("        be used together with / NORMAL or / EXPLICIT.");
H("    / COMPRESSED - The proof is saved in the compressed format which");
H("        reduces storage requirements in a source file.");
H("    / FAST - The proof is merely reformatted and not recompressed.");
H("        May be used (only) with / COMPRESSED and / PACKED to speed up");
H("        conversion between formats.");
H("    / OLD_COMPRESSION - When used with / COMPRESSED, specifies an older,");
H("        slightly less space-efficient algorithm.  (Specifically, it does");
H("        not try to rearrange labels to fit evenly on a line.)");
H("    / TIME - prints out the run time used for each proof.");
H("");
H("Important note:  The / PACKED and / EXPLICIT qualifiers save the proof");
H("in formats that are _not_ part of the Metamath standard and that probably");
H("will not be recognized by other Metamath proof verifiers.  They are");
H("primarily intended to assist database maintenance.  For example,");
H("    SAVE PROOF * / EXPLICIT / PACKED / FAST");
H("will allow the order of $e and $f hypotheses to be changed without");
H("affecting any proofs.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP SAVE NEW_PROOF");
H("Syntax:  SAVE NEW_PROOF <label-match> [/ <qualifier>] [/ <qualfier>]...");
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
H("    / NORMAL, / COMPRESSED, / EXPLICIT, / PACKED, / OLD_COMPRESSION -");
H("        These qualifiers are the same as for SAVE PROOF.  See");
H("        HELP SAVE PROOF.");
H("    / OVERRIDE - By default, SAVE NEW_PROOF will refuse to overwrite");
H("        the proof if \"(Proof modification is discouraged.)\" is present");
H("        in the statement's description comment.  This qualifier will");
H("        allow the proof to be saved.");
H("");
H("Note that if no qualifier is specified, / NORMAL is assumed.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP DEMO");
H("For a quick demo that enables you to see Metamath do something, type");
H("the following:");
H("    READ set.mm");
H("    SHOW STATEMENT id1 /COMMENT");
H("    SHOW PROOF id1 /RENUMBER /LEMMON");
H("will show you a proof of \"P implies P\" directly from the axioms");
H("of propositional calculus.");
H("    SEARCH * \"distributive law\" /COMMENTS");
H("will show all the distributive laws in the database.");
H("    SEARCH * \"C_ $* u.\"");
H("will show all statements with subset then union in them.");
H("");

if (strcmp(helpCmd, saveHelpCmd)) bug(1401); /* helpCmd got corrupted */
let(&saveHelpCmd, ""); /* Deallocate memory */

}
