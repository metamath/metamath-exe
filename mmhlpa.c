/*****************************************************************************/
/*        Copyright (C) 2002  NORMAN MEGILL  nm@alum.mit.edu                 */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* Part 1 of help file for Metamath */
/* To add a new help entry, you must add the command syntax to mmcmdl.c
   as well as adding it here. */

#include <string.h>
#include <stdio.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmds.h"
#include "mmhlpa.h"

/* help0 is for listMode */
void help0(vstring helpCmd)
{
printHelp = !strcmp(helpCmd, "HELP");
H("This utility assists with some common file manipulations.");
H("Most commands will perform an identical operation on each line of a file.");
H("Use HELP ? to see list of help topics.");
H("Note:  When an output file is created, any previous version is renamed,");
H(
 "with ~1 appended, and any ~1 renamed to ~2, etc. (up to ~9, which is lost).");
H("Note:  All string-matching command arguments are case-sensitive.");
H("");
H("Line-by-line editing commands:");
H("  ADD - Add a specified string to each line in a file");
H("  CLEAN - Trim spaces and tabs on each line in a file; convert characters");
H("  DELETE - Delete a section of each line in a file");
H("  INSERT - Insert a string at a specified column in each line of a file");
H("  SUBSTITUTE - Make a simple substitution on each line of the file");
/*H("  LSUBSTITUTE - Substitute according to a match-and-substitute list");*/
H("  SWAP - Swap the two halves of each line in a file");
H("Other file processing commands:");
H("  BREAK - Break up (parse) a file into a list of tokens (one per line)");
H("  BUILD - Build a file with multiple tokens per line from a list");
H("  COUNT - Count the occurrences in a file of a specified string");
/*H("  FORMAT - Produce a formatted list of tokens for documentation");*/
H("  NUMBER - Create a list of numbers");
H("  PARALLEL - Put two files in parallel");
H("  REVERSE - Reverse the order of the lines in a file");
H("  RIGHT - Right-justify lines in a file (useful before sorting numbers)");
H("  TAG - Tag edit updates in a program for revision control");
H("  SORT - Sort the lines in a file with key starting at specified string");
H("  MATCH - Extract lines containing (or not) a specified string");
/*H("  LEXTRACT - Extract lines containing (or not) strings from a list");*/
H("  UNDUPLICATE - Eliminate duplicate occurrences of lines in a file");
H("  DUPLICATE - Extract first occurrence of any line occurring more than");
H("      once in a file, discarding lines occurring exactly once");
H("  UNIQUE - Extract lines occurring exactly once in a file");
H(
"  (UNDUPLICATE, DUPLICATE, and UNIQUE also sort the lines as a side effect.)");
H("  TYPE (10 lines) - Similar to Unix \"head\"");
/*H("  COPY, RENAME - Similar to Unix cat, mv but with backups created");*/
H(
"  COPY - Similar to Unix \"cat\" but safe (same input & output name allowed)");
H("  SUBMIT - Run a script containing Tools commands.");
H("");
if (listMode) {
H("Command syntax ([] means optional):");
H("  From TOOLS prompt:  Tools> <command> [<arg1> <arg2>...]");
H("  From VMS shell:  $ DO TOOLS [<command>] [<arg1> <arg2>...]");
H("  From Unix/DOS shell:  tools [<command>] [<arg1> <arg2>...]");
H("You need to type only as many characters of the command as are needed to");
H("uniquely specify it.  Any arguments will answer questions automatically");
H("until the argument list is exhausted; the remaining questions will be");
H("prompted.  An argument may be optionally enclosed in quotes.  Use \"\" for");
H("default or null argument.  [Note:  VMS does not allow single quotes (')");
H("around DCL arguments; use double quotes (\").  Single quotes are OK in");
H("immediate mode.] [Note: Quotes MUST be put around Unix pathname arguments");
H("with a \"/\" in them, except they may be omitted in direct response to");
H("an argument prompt.]");
H("");
H("Notes:");
H("(1) The commands are not case sensitive.  File names and match strings");
H("are case sensitive.");
H("(2) Output files are created only after a command finishes running.");
H("Therefore it is usually safe to hit ^C before a command is completed.");
H("(3) The file \"zztools.tmp\", which is always created, can be used as a");
H("command file to re-run the command sequence with the SUBMIT command.");
H("(4) Previous versions of output files (except under VMS) are renamed with");
H("~1 (most recent), ~2,...,~9 (oldest) appended to file name.  Purge them");
H("periodically.");
H("(5) The command B(EEP) will make the terminal beep.  It can be useful to");
H("type it ahead to let you know when the current command is finished.");
H("(6) It is suggested you use a \".tmp\" file extension for intermediate");
H("results to eliminate directory clutter.");
H("");
H("Please see NDM if you have any suggestions for this program.");
} /* if listMode */


printHelp = !strcmp(helpCmd, "HELP ADD");
H("This command adds a character string prefix and/or suffix to each");
H("line in a file.");
H("Syntax:  ADD <iofile> <begstr> <endstr>");

printHelp = !strcmp(helpCmd, "HELP DELETE");
H("This command deletes the part of a line between (and including)");
H("two specified strings for all lines in a file.");
H("Syntax:  DELETE <iofile> <startstr> <endstr>");

printHelp = !strcmp(helpCmd, "HELP CLEAN");
H("This command processes spaces and tabs in each line of a file");
H("according to the following subcommands:");
H("  D - Delete all spaces and tabs");
H("  B - Delete spaces and tabs at the beginning of each line");
H("  E - Delete spaces and tabs at the end of each line");
H("  R - Reduce multiple spaces and tabs to one space");
H("  Q - Do not alter characters in quotes (ignored by T and U)");
H("  T - (Tab) Convert spaces to equivalent tabs");
H("  U - (Untab) Convert tabs to equivalent spaces");
H("Some other subcommands are also available:");
H("  P - Trim parity (8th) bit from each character");
H("  G - Discard garbage characters CR,FF,ESC,BS");
H("  C - Convert to upper case");
H("  L - Convert to lower case");
H("  V - Convert VT220 screen print frame graphics to -,|,+ characters");
H("Subcommands may be joined with commas (but no spaces), e.g., \"B,E,R,Q\"");
H("Syntax:  CLEAN <iofile> <subcmd,subcmd,...>");

printHelp = !strcmp(helpCmd, "HELP SUBSTITUTE")
    || !strcmp(helpCmd, "HELP S");
H("This command performs a simple string substitution in each line of a file.");
H("If the string to be replaced is \"\\n\", then every other line will");
H("be joined to the one below it.  If the replacement string is \"\\n\", then");
H("each line will be split into two if there is a match.");
H("The <matchstr> specifies a string that must also exist on a line");
H("before the substitution takes place; null means match any line.");
H("Syntax:  SUBSTITUTE <iofile> <oldstr> <newstr> <occurrence> <matchstr>");
H("Note: The SUBSTITUTE command may be abbreviated by S.");

printHelp = !strcmp(helpCmd, "HELP SWAP");
H("This command swaps the parts of each line before and after a");
H("specified string.");


printHelp = !strcmp(helpCmd, "HELP INSERT");
H("This command inserts a string at a specifed column in each line");
H("in a file.  It is intended to aid further processing of column-");
H("sensitive files.");
H("Syntax:  INSERT <iofile> <string> <column>");


printHelp = !strcmp(helpCmd, "HELP BREAK");
H("This command breaks up a file into tokens, one per line, using white");
H("space and any special characters you specify as delimiters.");
H("Syntax:  BREAK <iofile> <specchars>");

printHelp = !strcmp(helpCmd, "HELP BUILD");
H("This command combines a list of tokens into multiple tokens per line,");
H("as many as will fit per line, separating them with spaces.");
H("Syntax:  BUILD <iofile>");


printHelp = !strcmp(helpCmd, "HELP MATCH");
H("This command extracts from a file those lines containing (Y) or not");
H("containing (N) a specified string.");
H("Syntax:  MATCH <iofile> <matchstr> <Y/N>");


printHelp = !strcmp(helpCmd, "HELP SORT");
H("This command sorts a file, comparing lines starting at a key string.");
H("If the key string is blank, the line is compared starting at column 1.");
H("If a line doesn't contain the key, it is compared starting at column 1.");
H("Syntax:  SORT <iofile> <key>");


printHelp = !strcmp(helpCmd, "HELP UNDUPLICATE");
H("This command sorts a file then removes any duplicate lines from the output.");
H("Syntax:  UNDUPLICATE <iofile>");


printHelp = !strcmp(helpCmd, "HELP DUPLICATE");
H("This command finds all duplicate lines in a file and places them, in");
H("sorted order, into the output file.");
H("Syntax:  DUPLICATE <iofile>");


printHelp = !strcmp(helpCmd, "HELP UNIQUE");
H("This command finds all unique lines in a file and places them, in");
H("sorted order, into the output file.");
H("Syntax:  UNIQUE <iofile>");


printHelp = !strcmp(helpCmd, "HELP REVERSE");
H("This command reverses the order of the lines in a file.");
H("Syntax:  REVERSE <iofile>");


printHelp = !strcmp(helpCmd, "HELP RIGHT");
H("This command right-justifies the lines in a file by putting spaces in");
H("front of them so that they end in the same column as the longest line");
H("in the file.");
H("Syntax:  RIGHT <iofile>");


printHelp = !strcmp(helpCmd, "HELP PARALLEL");
H("This command puts two files side-by-side.");
H("The two files should have the same number of lines; if not, a warning is");
H("issued and the longer file paralleled with empty strings at the end.");
H("Syntax:  PARALLEL <inpfile1> <inpfile2> <outfile> <btwnstr>");


printHelp = !strcmp(helpCmd, "HELP NUMBER");
H("This command creates a list of numbers.  Hint:  Use the RIGHT command to");
H("right-justify the list after creating it.");
H("Syntax:  NUMBER <outfile> <first> <last> <incr>");


printHelp = !strcmp(helpCmd, "HELP COUNT");
H("This command counts the occurrences of a string in a file and displays");
H("some other statistics about the file.  The sum of the lines is obtained");
H("by extracting digits and is only valid if the file consists of genuine");
H("numbers.");
H("Syntax:  COUNT <inpfile> <string>");


printHelp = !strcmp(helpCmd, "HELP TYPE") || !strcmp(helpCmd, "HELP T");
H("This command types the first n lines of a file on the terminal screen.");
H("If n is not specified, it will default to 10.  If n is the string \"ALL\"");
H("then the whole file will be typed.  T is an abbreviation for TYPE.");
H("Syntax:  TYPE <inpfile> <n>");


printHelp = !strcmp(helpCmd, "HELP COPY") || !strcmp(helpCmd, "HELP C");
H("This command copies (concatenates) all input files in a comma-separated");
H("list (no blanks allowed) to an output file.  The output file may have");
H("the same name as an input file.  Any previous version of the output");
H("file is renamed with a ~1 extension.  C is an abbreviation for COPY.");
H("Syntax:  COPY <inpfile,inpfile,...> <outfile>");


printHelp = !strcmp(helpCmd, "HELP TAG");
H("This command tags edits made to a program.  Its purpose is to provide");
H("traceability to the previous revision of the file for revision control.");
H("Syntax:  TAG <originfile> <editedinfile> <editedoutfile> <tag> <match>");


printHelp = !strcmp(helpCmd, "HELP CLI");
H("Each command line is an English-like word followed by arguments separated");
H("by spaces, as in SUBMIT abc.cmd.  Commands are not case sensitive, and");
H("only as many letters are needed as are necessary to eliminate ambiguity;");
H("for example, \"a\" would work for the command ADD.  Command arguments");
H("which are file names and match strings are case-sensitive (although file");
H("names may not be on some operating systems).");
H("");
H("A command line is entered typing it in then pressing the <return> key.");
H("");
H("To find out what commands are available, type ? at the \"TOOLS>\" prompt.");
H("");
H("To find out the choices at any point in a command, press <return> and you");
H("will be prompted for them.  The default choice (the one selected if you");
H("just press <return>) is shown in brackets (<>).");
H("");
H("You may also type ? in place of a command word to tell");
H("you what the choices are.  The ? method won't work, though, if a");
H("non-keyword argument such as a file name is expected at that point,");
H("because the CLI will think the ? is the argument.");
H("");
H("Some commands have one or more optional qualifiers which modify the");
H("behavior of the command.  Qualifiers are indicated by a slash (/), such as");
H("in ABC xyz / IJK.  Spaces are optional around the /.  If you need");
H("to use a slash in a command argument, as in a Unix file name, put single");
H("or double quotes around the command argument.");
H("");
H("If the response to a command is more than a screenful, you will be");
H("prompted to \"<return> to continue, Q to quit, or S to scroll to end\".");
H("Q will complete the command internally but suppress further output until");
H("the next \"TOOLS>\" prompt.  S will suppress further pausing until the next");
H("\"TOOLS>\" prompt.");
H("");
H("A command line enclosed in quotes is executed by your operating system.");
H("See HELP SYSTEM.");
H("");
H("Some other commands you may want to review with HELP are:");
H("    SUBMIT");
H("");

printHelp = !strcmp(helpCmd, "HELP SUBMIT");
H("Syntax:  SUBMIT <filename>");
H("");
H("This command causes further command lines to be taken from the specified");
H("file.  Note that any line beginning with an exclamation point (!) is");
H("treated as a comment (i.e. ignored).  Also note that the scrolling");
H("of the screen output is continuous.");


printHelp = !strcmp(helpCmd, "HELP SYSTEM");
H("A line enclosed in quotes will be executed by your computer's operating");
H("system, if it has such a feature.  For example, on a VAX/VMS system,");
H("    Tools> 'dir'");
H("will print disk directory contents.  Note that this feature will not work");
H("on the Macintosh, which does not have a command line interface.");
H("");
H("For your convenience, the trailing quote is optional.");
H("");


} /* help0 */

/* Note: help1 should contain Metamath help */
void help1(vstring helpCmd)
{


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
H("  $} - End of scope.  All $v, $e, $f, and $d statements in the current");
H("       scope become inactive.  Note that $a and $p statements remain");
H("       active forever.  Note that $c's may be used only in the outermost");
H("       scope, so they are always active.  The outermost scope is not");
H("       bracketed by ${...$}.");
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
H("  $f - Variable-type (or \"floating\") hypothesis (meaning it is required");
H("         by a $p or $a statement in its scope only if it has variables in");
H("         common with the $p or $a statement or the essential hypotheses");
H("         of the $p or $a statement).  Every $d, $e, $p, and $a statement");
H("         variable must have an earlier active $f statement to specify the");
H("         variable type.");
H("       Syntax:  \"<label> $f <constant> <variable> $.\" where both symbols");
H("         are active");
H("");
H("  $e - Logical (or \"essential\") hypothesis (meaning it is always");
H("         required by a $p or $a statement in its scope)");
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
H("         a string of capital letters; see book for compression algorithm.");
H("");
H("  A substitution is the replacement of a variable with a <symbol> string");
H("  throughout an assertion and its required hypotheses.");
H("");
H("  In a proof, the label of a hypothesis ($e or $f) pushes the stack, and");
H("  the label of an assertion ($a or $p) pops from the stack a number of");
H("  entries equal to the number of the assertion's required hypotheses and");
H("  replaces the stack's top.  Whenever an assertion is specified, a unique");
H("  set of substitutions must exist that makes the assertion's hypotheses");
H("  match the top entries of the stack.");
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
H("         ` <symbol> ` - use graphical <symbol> in LaTeX/HTML output;");
H("             `` means literal `");
H("         ~ <label> - use typewriter font (URL link) in LaTeX (HTML) output");
H("         $t - flags comment as containing LaTeX and/or HTML typesetting");
H("             definitions; see HELP LATEX or HELP HTML for more information");
H("       Note:  Comments may not be nested.");
H("");
H("  $[ <file-name> $] - place contents of file <file-name> here; a second,");
H("       recursive, or self reference to a file is ignored");
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
H("The commands available to help you create a proof are the following.");
H("See the help for the individual commands for more detail.");
H("    SHOW NEW_PROOF [/ ESSENTIAL,...] - Displays the proof in progress.");
H("        You will use this command a lot; see HELP SHOW NEW_PROOF to");
H("        become familiar with its qualifiers.  The qualifiers / UNKNOWN");
H("        and / NOT_UNIFIED are useful for seeing the work remaining to be");
H("        done.  Normally, / ESSENTIAL should always be used since Metamath");
H("        can usually prove syntax statements (with no unknown variables)");
H("        automatically.  Unknown variables are shown as $1, $2,...");
H("    ASSIGN <step> <label> - Assigns an unknown step with the statement");
H("        specified by <label>.");
H("    MATCH STEP <step> (or MATCH ALL) - Shows what statements are");
H("        possibilities for the ASSIGN statement. (Rarely useful.  Instead,");
H("        use the SEARCH statement for candidates matching specific math");
H("        token combinations.)");
H("    LET VARIABLE <variable> = \"<symbol sequence>\" - Forces a symbol");
H("        sequence to replace an unknown variable in a proof.  It is useful");
H("        for helping difficult unifications.");
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
H("        step(s).  DELETE FLOATING_HYPOTHESES then INITIALIZE ALL is");
H("        useful for recovering from mistakes.");
H("    IMPROVE <step> (or ALL) - Automatically creates a proof for steps");
H("        (with no unknown variables) whose proof requires no statements");
H("        with $e hypotheses.  Useful for filling in proofs of $f");
H("        hypotheses.  The / DEPTH qualifier will also try statements");
H("        whose $e hypotheses contain no new variables, but save your");
H("        proof before using / DEPTH = 1 or greater (the default is 0)");
H("        since search time grows exponentially and may never terminate");
H("        in a reasonable time, and you cannot interrupt the search.");
H("    SAVE NEW_PROOF - Saves the proof in progress internally in the");
H("        database buffer.  To save it permanently, use WRITE SOURCE after");
H("        SAVE NEW_PROOF.  Warning: SAVE and WRITE your proof often since");
H("        there is currently no undo command.");
H("");
H("The following commands set parameters that may be relevant to your proof:");
H("    SET UNIFICATION_TIMEOUT");
H("    SET SEARCH_LIMIT");
H("    SET EMPTY_SUBSTITUTION - note that default is OFF (contrary to book)");
H("");
H("Type EXIT to exit the MM-PA> prompt and get back to the MM> prompt.");
H("Another EXIT will then get you out of Metamath.");
H("");


printHelp = !strcmp(helpCmd, "HELP HTML");
H("To create an HTML output file for a $a or $p statement, use");
H("    SHOW STATEMENT <label> / HTML");
H("When <label> has wildcard (*) characters, all statements with matching");
H("labels will have HTML files produced for them.  Also, when <label> has a");
H("wildcard (*) character, three additional files, mmtheorems.html,");
H("mmdefinitions.html, and mmascii.html will be produced.  Thus:");
H("    SHOW STATEMENT * / HTML");
H("will output a complete HTML proof database in the current directory,");
H("one file per $a and $p statement, along with mmtheorems.html and");
H("mmdefinitions.html.  The statement:");
H("    SHOW STATEMENT ?* / HTML");
H("will produce only mmtheorems.html, mmdefinitions.html, and mmascii.html,");
H("but no other HTML files (since no labels can match \"?*\" because \"?\" is");
H("illegal in a statement label).");
H("");
H("The HTML definitions for the symbols and and other features are");
H("specified by statements in a special typesetting comment in the input");
H("database file.  The typesetting comment is identified by the token \"$t\"");
H("in the comment, and the typesetting statements run until the next \"$)\":");
H("   ...  $( ...  $t ................................ $) ...");
H("                   <-- HTML definitions go here -->");
H("In the HTML definition section, C-style comments /* ... */ are");
H("recognized.  The HTML specification statements are:");
H("    htmltitle \"<HTML code for title>\" ;");
H("    htmlhome \"<HTML code for home link>\" ;");
H("    htmlvarcolors \"<HTML code for variable color list>\" ;");
H("    htmldef \"<mathtoken>\" as \"<HTML code for mathtoken symbol>\" ;");
H("                    ...");
H("    htmldef \"<mathtoken>\" as \"<HTML code for mathtoken symbol>\" ;");
H("Single or double quotes surround the field strings, and fields too long");
H("for a line may be broken up into multiple quoted strings connected with");
H("(whitespace-surrounded) \"+\" signs (no quotes around them).  Inside");
H("quoted strings, the opposite kind of quote may appear.  If both kinds of");
H("quotes are needed, use separate quoted strings connected by \"+\".");
H("Note that a \"$)\" character sequence will flag the end of the");
H("typesetting Metamath comment even if embedded in quotes (which are not");
H("meaningful for the Metamath language parser), so such a sequence must be");
H("broken with \"+\".");
H("");
H("The typesetting Metamath comment may also contain LaTeX definitions");
H("(with \"latexdef\" statements) that are ignored for HTML output.");
H("");
H("Several other qualifiers exist.  The command");
H("    SHOW STATEMENT <label> / ALT_HTML");
H("does the same as SHOW STATEMENT <label> / HTML, except that the HTML code");
H("for the symbols is taken from \"althtmldef\" statements instead of");
H("\"htmldef\" statements in the $(...$t...$) comment.  This is useful when");
H("an alternate representations of symbols is desired, for example one that");
H("uses the Symbol font or Unicode entities instead of GIF images.  Associated");
H("with althtmldef are the statements");
H("    htmldir \"<directory for GIF HTML version>\" ;");
H("    althtmldir \"<directory for Symbol font HTML version>\" ;");
H("that produce links to the alternate version.  See the set.mm database for");
H("examples of these statements.");
H("");
H("The command");
H("    SHOW STATEMENT * / BRIEF_HTML");
H("invokes a special mode that just produces definition and theorem lists");
H("accompanied by their symbol strings, in a format suitable for copying and");
H("pasting into another web page.");
H("");
H("Finally, the command");
H("    SHOW STATEMENT * / BRIEF_ALT_HTML");
H("does the same as SHOW STATEMENT * / BRIEF_HTML for the alternate HTML");
H("symbol representation.");
H("");
H("When two different type of pages need to be produced from a single");
H("database, such as the Hilbert Space Explorer that extends the Metamath");
H("Proof Explorer, \"extended\" variables may be declared in the $t comment:");
H("    exthtmltitle \"<HTML code for title>\" ;");
H("    exthtmlhome \"<HTML code for home link>\" ;");
H("When these are declared, you also must declare");
H("    exthtmllabel \"<label>\" ;");
H("When the output statement is the one declared with \"exthtmllabel\" or");
H("a later one, the HTML code assigned to \"exthtmltitle\" and");
H("\"exthtmlhome\" is used instead of that assigned to \"htmltitle\" and");
H("\"htmlhome\" respectively.  See the set.mm database file for an example.");
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
H("    SHOW STATEMENT uneq2 / TEX");
H("    SHOW PROOF uneq2 / TEX");
H("    CLOSE TEX");
H("");
H("The LaTeX symbol definitions should be included in a special comment");
H("containing a $t token.  See the set.mm file for an example.");
H("");


printHelp = !strcmp(helpCmd, "HELP BEEP") || !strcmp(helpCmd, "HELP B");
H("Syntax:  BEEP");
H("");
H("This command will produce a beep.  By typing it ahead after a long-");
H("running command has started, it will alert you that the command is");
H("finished. B is an abbreviation for BEEP.");
H("");
H("Note: If B is typed at the MM> prompt immediately after the end of a");
H("multiple-page display paged with \"Press <return> for more...\" prompts,");
H("then the B will back up to the previous page rather than perform the BEEP");
H("command.");
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
H("The QUIT command is a synonym for EXIT.");
H("");
H("**Warning**  Pressing CTRL-C will abort the Metamath program");
H("unconditionally.  This means any unsaved work will be lost.");
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

printHelp = !strcmp(helpCmd, "HELP TOOLS");
H("Syntax:  TOOLS");
H("");
H("This command invokes a utility to manipulate ASCII text files.");
H("");


} /* help1 */
