/*****************************************************************************/
/*               Copyright (C) 1997, NORMAN D. MEGILL                        */
/*****************************************************************************/

/*34567890123456 (79-character line to adjust text window width) 678901234567*/

/* Part 1 of help file for Metamath */
/* To add a new help entry, you must add the command syntax to mmcmdl.c
   as well as adding it here. */

#include <string.h>
#include <stdio.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmds.h"
#include "mmhlpa.h"

void help1(vstring helpCmd)
{


printHelp = !strcmp(helpCmd, "HELP");
H("");
H("");
H("");
H("");
H("Welcome to Metamath.  Here are some general guidelines.");
H("");
H("To make the most effective use of Metamath, you should become familiar");
H("with the Metamath book.  In particular, you will need to learn");
H("the syntax of the Metamath language.");
H("");
H("For a summary of the Metamath language, type HELP LANGUAGE.");
H("For help using the command line interpreter, type HELP CLI.");
H("For help getting started, type HELP DEMO.");
H("For help exploring the data base, type HELP EXPLORE.");
H("For help creating a LaTeX file, type HELP TEX.");
H("For help using the Proof Assistant, type HELP PROOF_ASSISTANT.");
H("For a list of help topics, type HELP ?.");
H("To exit Metamath, type EXIT.");
H("");
H("If you need technical support, contact Norman Megill via email at");
H("nm@alum.mit.edu, or at 19 Locke Lane, Lexington, MA 02173 USA.");
H("");
H("");
H("");
H("");
H("Copyright, Licensing, and Warranty Information");
H("----------------------------------------------");
H("");
H("Copyright (C) 1997 Norman D. Megill");
H("");
H("Permission to use this software free of charge is granted to individuals");
H("and not-for-profit organizations.  Commercial use of this software, in");
H("part or in whole, is prohibited without permission from the copyright");
H("owner.");
H("");
H("This software is provided without warranty of any kind.");
H("");


printHelp = !strcmp(helpCmd, "HELP LANGUAGE");
H("The language is best learned by reading the book and studying a few proofs");
H("with the Metamath program.  This is a brief summary for reference.");
H("");
H("The database contains a series of tokens separated by white space (spaces,");
H("tabs, returns).  A token is a keyword, a <label>, or a <symbol>.");
H("");
H("The pure language keywords are:  $c $v $a $p $e $f $d ${ $} $. and $=");
H("The auxiliary keywords are:  $( $) $[ and $]");
H("This is the complete set of language keywords.");
H("");
H("<symbol>s and <label>s are user-defined.  <symbol>s may contain any");
H("printable characters other than $ , and <label>s may contain alphanumeric");
H("characters, periods, dashes and underscores.");
H("");
H("Scoping statements:");
H("");
H("  ${ - Start of scope");
H("  $} - End of scope.  All $c, $v, $e, $f, and $d statements in the current");
H("       scope become inactive.  Note that $a and $p statements remain");
H("       active forever.");
H("");
H("Declarations:");
H("");
H("  $c - Constant declaration.  The <symbol>s become active constants.");
H("       Syntax:  \"$c <symbol> ... <symbol> $.\"");
H("");
H("  $v - Variable declaration.  The <symbols>s become active variables.");
H("       Syntax:  \"$v <symbol> ... <symbol> $.\"");
H("");
H("Hypotheses:");
H("");
H("  $f - Floating hypothesis (meaning it is required by a $p or $a statement");
H("         in its scope only if it has variables in common with the $p or $a");
H("         statement or the essential hypotheses of the $p or $a statement).");
H("         Every $d, $e, $p, $a statement variable must have an active $f");
H("         statement to specify the variable type.");
H("       Syntax:  \"<label> $f <constant> <variable> $.\" where both symbols");
H("         are active");
H("");
H("  $e - Essential hypothesis (meaning it is always required by a $p or $a");
H("         statement in its scope)");
H("       Syntax:  \"<label> $e <symbol> ... <symbol> $.\"  where the first");
H("         (and possibly only) <symbol> is a constant");
H("");
H("Assertions:");
H("");
H("  $a - Axiomatic assertion (starting assertion; used for axioms,");
H("         definitions, and language syntax specification)");
H("       Syntax:  \"<label> $a <symbol> ... <symbol> $.\"  where the first");
H("         (and possibly only) <symbol> is a constant");
H("");
H("  $p - Provable assertion (derived assertion; used for deductions and");
H("         theorems; must follow from previous statements as demonstrated by");
H("         its proof)");
H("       Syntax:");
H("         \"<label> $p <symbol> ... <symbol> $= <label> ... <label> $.\"");
H("         where the first (and possibly only) <symbol> is a constant.");
H("         \"$= <label> ... <label> $.\" is the proof; see the book for more");
H("         information.  Proofs may be compressed for storage efficiency.  A");
H("         compressed proof is a series of labels in parentheses followed by");
H("         a string of capital letters.");
H("");
H("  A substitution is the simultaneous replacement of all occurrences of");
H("  each variable with a <symbol> string in an assertion and all of its");
H("  required hypotheses.");
H("");
H("  In a proof, the label of a hypothesis ($e or $f) pushes the stack, and");
H("  the label of an assertion ($a or $p) pops from the stack a number of");
H("  entries equal to the number of the assertion's required hypotheses and");
H("  replaces the stack's top.  Whenever an assertion is specified, a unique");
H("  set of substitutions must exist that makes the assertion's hypotheses");
H("  match the top entries of the stack.  Note that all variables in an");
H("  assertion must exist in one of its required hypotheses.");
H("");
H("  To see a readable proof format, type SHOW PROOF xxx / ESSENTIAL /");
H("  LEMMON / RENUMBER, where xxx is the label of a $p statement.  To convert");
H("  a compressed proof to a sequence of labels, type");
H("  SAVE PROOF xxx / NORMAL, where xxx is a label of a $p statement.  To see");
H("  how the substitutions are made in a proof step, type");
H("  SHOW PROOF xxx / DETAILED_STEP nnn, where nnn is a step number from");
H("  a SHOW PROOF command without the / RENUMBER qualifier.");
H("");
H("Disjoint variable restriction:");
H("");
H("  The substitution of symbol strings into two distinct variables may be");
H("  subject to a $d restriction:");
H("");
H("  $d - Disjoint variable restriction (meaning substititutions may not");
H("         share variables in common)");
H("       Syntax:  \"$d <symbol> ... <symbol> $.\" where <symbol> is active");
H("         and previously declared with $v, and all <symbol>s are distinct");
H("");
H("Auxiliary keywords:");
H("");
H("  $(   Begin comment");
H("  $)   End comment");
H("       Inside of comments:");
H("         `<symbol>` - use graphical <symbol> in LaTeX output; `` means");
H("             literal ` .");
H("         ~<label> - use typewriter font in LaTeX output.");
H("       Note:  Comments may not be nested.");
H("");
H("  $[ <file-name> $] - place contents of file <file-name> here");
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
H("A command line is entered by pressing the <return> key.");
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
H("the next 'MM>' prompt.  S will suppress further pausing until the next");
H("'MM>' prompt.");
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

printHelp = !strcmp(helpCmd, "HELP EXPLORE");
H("When you first enter Metamath, you will first want to READ in a Metamath");
H("source file.  The source file provided for set theory is called set.mm;");
H("to read it type");
H("    READ set.mm");
H("");
H("You may want to look over the contents of the source file with a text");
H("editor, to get an idea of what's in it, before starting to use Metamath.");
H("");
H("The following commands will help you study the source file statements");
H("and their proofs.  Use the help for the individual commands to get");
H("more information about them.");
H("    SEARCH <label-match> \"<symbol-match>\" - Displays statements whose");
H("        labels match <label-match> and that contain <symbol-match>.");
H("    SEARCH <label-match> \"<search-string>\" / COMMENTS - Shows statements");
H("        whose preceding comment contains <search-string>");
H("    SHOW LABELS <label-match> - Lists all labels matching <label-match>,");
H("        in which * may be used as a wildcard:  for example, \"*abc*def\"");
H("        will match all labels containing \"abc\" and ending with \"def\".");
H("    SHOW STATEMENT <label> - Shows the comment, contents, and relevant");
H("        hypotheses associated with a statement.");
H("    SHOW PROOF <label> - Shows the proof of a $p statement in various");
H("        formats, depending on what qualifiers you select.  One of the");
H("        qualifiers, / TEX, lets you create LaTeX source for the proof.");
H("        The / DETAILED_STEP qualifier is useful while you're learning how");
H("        Metamath unifies the sources and targets of a step.  The");
H("        / STATEMENT_SUMMARY qualifier gives you a quick summary of all");
H("        the statements referenced in the proof.");
H("    SHOW TRACE_BACK <label> - Traces a proof back to axioms, depending");
H("        on various qualifiers you select.");
H("    SHOW USAGE <label> - Shows what later proofs make use of this");
H("        statement.");
H("");


printHelp = !strcmp(helpCmd, "HELP PROOF_ASSISTANT");
H("Before using the proof assistant, you must include the statement you want");
H("to prove in a $p statement in your source file.  Its proof should consist");
H("of a single ?, meaning \"unknown step\".  Example:");
H("    eqid $p x = x $= ? $.");
H("");
H("To enter the proof assistant, type PROVE <label>, e.g. PROVE eqid.");
H("Metamath will respond with the MM-PA> prompt.");
H("");
H("Proofs are created working backwards from the statement being proved.  A");
H("proof is complete when all steps are assigned to statements and all");
H("steps are unified and completely known.  During the creation of a proof,");
H("Metamath will allow only operations that are legal based on what is");
H("known up to that point.  For example, it will not allow an ASSIGN of");
H("a statement that cannot be unified with the proof step being assigned.");
H("");
H("The commands available in to help you create a proof are the following.");
H("See the help for the individual commands for more detail.");
H("    SHOW NEW_PROOF [/ ESSENTIAL,...] - Displays the proof in progress.");
H("        You will use this command a lot; see HELP SHOW NEW_PROOF to");
H("        become familiar with its qualifiers.");
H("    ASSIGN <step> <label> - Assigns an unknown step with the statement");
H("        specified by <label>.");
H("    MATCH STEP <step> (or MATCH ALL) - Shows what statements are");
H("        possibilities for the ASSIGN statement.");
H("    LET VARIABLE <variable> = \"<symbol sequence>\" - Forces a symbol");
H("        sequence to replace an unknown variable in a proof.");
H("    LET STEP <step> = \"<symbol sequence>\" - Forces a symbol sequence");
H("        to replace the contents of a proof step, provided it can be");
H("        unified with the existing step contents.");
H("    UNIFY STEP <step> (or UNIFY ALL) - Unifies the source and target of");
H("        a step.  If you specify a specific step, you will be prompted");
H("        to select among the unifications that are possible.  If you");
H("        specify ALL, only those steps with unique unifications will be");
H("        unified.");
H("    INITIALIZE <step> (or ALL) - De-unifies the target and source of");
H("        a step (or all steps), as well as the hypotheses of the source,");
H("        and makes all variables in the source unknown.  Useful when a");
H("        mistake resulted in incorrect unifications.");
H("    DELETE <step> (or ALL or FLOATING_HYPOTHESES) - Deletes the specified");
H("        step(s).  DELETE FLOATING_HYPOTHESES then INITIALIZE ALL is");
H("        useful for recovering from mistakes.");
H("    IMPROVE <step> (or ALL) - Automatically creates a proof for steps");
H("        (with no unknown variables) whose proof requires no statements");
H("        with $e hypotheses.  Useful for filling in proofs of $f");
H("        hypotheses.  The / DEPTH qualifier will also try statements");
H("        whose $e hypotheses contain no new variables.");
H("    SAVE NEW_PROOF - Saves the proof in progress internally in the");
H("        database buffer.  To save it permanently, use WRITE SOURCE after");
H("        SAVE NEW_PROOF.");
H("");
H("The following commands set parameters that may be relevant to your proof:");
H("    SET UNIFICATION_TIMEOUT");
H("    SET SEARCH_LIMIT");
H("    SET EMPTY_SUBSTITUTION");
H("");
H("Type EXIT to exit the MM-PA> prompt and get back to the MM> prompt.");
H("Another EXIT will then get you out of Metamath.");
H("");


printHelp = !strcmp(helpCmd, "HELP LATEX");
H("See HELP TEX.");
H("");

printHelp = !strcmp(helpCmd, "HELP TEX");
H("Metamath will create a \"turn-key\" LaTeX source file which can be");
H("immediately compiled and printed using a TeX program.  The TeX program");
H("must have the following minimum requirements:  the LaTeX style option and");
H("the AMS font set, available from the American Mathematical Society.");
H("");
H("To write out a statement and its proof, use a command sequence similar");
H("to the following example:");
H("    (Enter Metamath)");
H("    READ set.mm");
H("    OPEN TEX example.tex");
H("        (you will be prompted for a definition file, latex.def)");
H("    SHOW STATEMENT uneq2 / TEX");
H("    SHOW PROOF uneq2 / TEX");
H("    CLOSE TEX");
H("");
H("Note that Metamath may be used to help you create a LaTeX source for your");
H("notes and papers using the symbols defined in Appendix A of the Metamath");
H("book; these are often easier to remember and type than the TeX names.  To");
H("do this, create a \"dummy\" Metamath source file that looks like this:");
H("    $c a $.");
H("    $(");
H("       --- your text here, with formulas enclosed in ` ...` (grave");
H("           accents), using `` in place of an actual grave accent.");
H("           You must include your own LaTeX header and trailer.");
H("           Note that a line with only a formula is considered a displayed");
H("           formula, and you must supply the \\begin{equation}...");
H("           \\end{equation} or other math enclosures on the lines above and");
H("           below the formula.");
H("    $)");
H("    a $a a $.");
H("Calling this file mytex.mm, use the following commands to create a LaTeX");
H("source:");
H("    (Enter Metamath)");
H("    READ mytex.mm");
H("    OPEN TEX mytex.tex / NO_HEADER");
H("        (you will be prompted for a definition file, latex.def)");
H("    SHOW STATEMENT a / TEX / COMMENT_ONLY");
H("    CLOSE TEX");
H("    EXIT");
H("");


printHelp = !strcmp(helpCmd, "HELP BEEP");
H("Syntax:  BEEP");
H("");
H("This command will produce a beep.  By typing it ahead after a long-");
H("running command has started, it will alert you that the command is");
H("finished.");
H("");


printHelp = !strcmp(helpCmd, "HELP QUIT");
H("Syntax:  QUIT");
H("");
H("This command is a synonym for EXIT.  See HELP EXIT.");
H("");


printHelp = !strcmp(helpCmd, "HELP EXIT");
H("Syntax:  EXIT");
H("");
H("This command exits from Metamath.  If there have been changes to the");
H("database with the SAVE PROOF or SAVE NEW_PROOF commands, you will be given");
H("an opportunity to WRITE SOURCE to permanently save the changes.");
H("");
H("(In Proof Assistant mode) The EXIT command will return to the MM> prompt.");
H("If there were changes to the proof, you will be given an opportunity to");
H("SAVE NEW_PROOF.");
H("");


printHelp = !strcmp(helpCmd, "HELP READ");
H("Syntax:  READ <file> [/ VERIFY]");
H("");
H("This command will read in a Metamath language source file and any included");
H("files.  Normally it will be the first thing you do when entering Metamath.");
H("Statement syntax is checked, but proof syntax is not checked.");
H("Note that the file name may be enclosed in single or double quotes;");
H("this is useful if the file name contains slashes, as might be the case");
H("under Unix.");
H("");
H("Optional qualifier:");
H("    / VERIFY - Verify all proofs as the database is read in.  This");
H("        qualifier will slow down reading in the file.");
H("");
H("See also ERASE.");
H("");


printHelp = !strcmp(helpCmd, "HELP ERASE");
H("Syntax:  ERASE");
H("");
H("This command will reset Metamath to its starting state, deleting any");
H("database that was READ in.");
H("");


printHelp = !strcmp(helpCmd, "HELP OPEN LOG");
H("Syntax:  OPEN LOG <file>");
H("");
H("This command will open a log file that will store everything you see on");
H("the screen.  It is useful to help recovery from a mistake in a long Proof");
H("Assistant session, or to document bugs.");
H("");
H("The log file can be closed with CLOSE LOG.  It will automatically be");
H("closed upon exiting Metamath.");
H("");


printHelp = !strcmp(helpCmd, "HELP CLOSE LOG");
H("Syntax:  CLOSE LOG");
H("");
H("The CLOSE LOG command closes a log file if one is open.  See also OPEN");
H("LOG.");
H("");


printHelp = !strcmp(helpCmd, "HELP OPEN TEX");
H("Syntax:  OPEN TEX <file>");
H("");
H("This command opens a file for writing LaTeX source and writes a LaTeX");
H("header to the file.  LaTeX source can be written with the SHOW PROOF,");
H("SHOW NEW_PROOF, and SHOW STATEMENT commands using the / TEX qualifier.");
H("The mapping to LaTeX symbols is defined in a file normally called");
H("latex.def, and you will be prompted for the name of this file if it has");
H("not been read in yet.");
H("");
H("To format and print the LaTeX source, you will need a TeX program with");
H("LaTeX and the AMS font set installed.");
H("");
H("Optional qualifier:");
H("    / NO_HEADER - This qualifier prevents a standard LaTeX header and");
H("        trailer from being included with the output LaTeX code.");
H("");
H("See also CLOSE TEX.");
H("");


printHelp = !strcmp(helpCmd, "HELP CLOSE TEX");
H("Syntax:  CLOSE TEX");
H("");
H("This command writes a trailer to any LaTeX file that was opened with OPEN");
H("TEX (unless / NO_HEADER was used with OPEN) and closes the LaTeX file.");
H("");
H("See also OPEN TEX.");
H("");


}
