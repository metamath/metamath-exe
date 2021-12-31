/*****************************************************************************/
/*        Copyright (C) 2021  NORMAN MEGILL  nm at alum.mit.edu              */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* Part 1 of help file for Metamath */
/* The content here was split into help0() and help1() because the original
   help() overflowed the lcc compiler (at least before version 3.8; not
   tested with 3.8 and above). */
/* To add a new help entry, you must add the command syntax to mmcmdl.c
   as well as adding it here. */

#include <string.h>
#include <stdio.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmds.h"
#include "mmhlpa.h"

/* help0 is mostly for TOOLS help */
void help0(vstring helpCmd) {

vstring saveHelpCmd = "";
/* help0() may be called with a temporarily allocated argument (left(),
   cat(), etc.), and the let()s in the eventual print2() calls will
   deallocate and possibly corrupt helpCmd.  So, we grab a non-temporarily
   allocated copy here.  (And after this let(), helpCmd will become invalid
   for the same reason.)  */
let(&saveHelpCmd, helpCmd);

g_printHelp = !strcmp(saveHelpCmd, "HELP");
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
H("  TAG - Like ADD, but restricted to a range of lines");
H("  SWAP - Swap the two halves of each line in a file");
H("Other file processing commands:");
H("  BREAK - Break up (parse) a file into a list of tokens (one per line)");
H("  BUILD - Build a file with multiple tokens per line from a list");
H("  COUNT - Count the occurrences in a file of a specified string");
H("  NUMBER - Create a list of numbers");
H("  PARALLEL - Put two files in parallel");
H("  REVERSE - Reverse the order of the lines in a file");
H("  RIGHT - Right-justify lines in a file (useful before sorting numbers)");
H("  SORT - Sort the lines in a file with key starting at specified string");
H("  MATCH - Extract lines containing (or not) a specified string");
H("  UNDUPLICATE - Eliminate duplicate occurrences of lines in a file");
H("  DUPLICATE - Extract first occurrence of any line occurring more than");
H("      once in a file, discarding lines occurring exactly once");
H("  UNIQUE - Extract lines occurring exactly once in a file");
H(
"  (UNDUPLICATE, DUPLICATE, and UNIQUE also sort the lines as a side effect.)");
H("  UPDATE (deprecated) - Update a C program for revision control");
H("  TYPE (10 lines) - Display 10 lines of a file; similar to Unix \"head\"");
H(
"  COPY - Similar to Unix \"cat\" but safe (same input & output name allowed)");
H("  SUBMIT - Run a script containing Tools commands.");
H("");
H("Command syntax ([] means optional):");
H("  From TOOLS prompt:  TOOLS> <command> [<arg1> <arg2>...]");
H("You need to type only as many characters of the command as are needed to");
H("uniquely specify it.  Any arguments will answer questions automatically");
H("until the argument list is exhausted; the remaining questions will be");
H("prompted.  An argument may be optionally enclosed in quotes.  Use \"\" for");
H("default or null argument.");
H("");
H("Notes:");
H("(1) The commands are not case sensitive.  File names and match strings");
H("are case sensitive.");
H("(2) Previous versions of output files (except under VMS) are renamed with");
H("~1 (most recent), ~2,...,~9 (oldest) appended to file name.  You may want");
H("to purge them periodically.");
H("(3) The command B(EEP) will make the terminal beep.  It can be useful to");
H("type it ahead to let you know when the current command is finished.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP ADD");
H("This command adds a character string prefix and/or suffix to each");
H("line in a file.");
H("Syntax:  ADD <iofile> <begstr> <endstr>");

g_printHelp = !strcmp(saveHelpCmd, "HELP TAG");
H("TAG is the same as ADD but has 4 additional arguments that let you");
H("specify a range of lines.  Syntax:");
H("  TAG <iofile> <begstr> <endstr> <startmatch> <s#> <endmatch> <e#>");
H("where");
H("  <iofile> = input/output file");
H("  <begstr> = string to add to beginning of each line");
H("  <endstr> = string to add to end of each line");
H("  <startmatch> = a string to match; if empty, match any line");
H("  <s#> = the 1st, 2nd, etc. occurrence of <startmatch> to start the range");
H("  <endmatch> = a string to match; if empty, match any line");
H("  <e#> = the 1st, 2nd, etc. occurrence of <endmatch> from the");
H("      start of range line (inclusive) after which to end the range");
H("Example:  To add \"!\" to the end of lines 51 through 60 inclusive:");
H("  TAG \"a.txt\" \"\" \"!\" \"\" 51 \"\" 10");
H("Example:  To add \"@@@\" to the beginning of each line in theorem");
H("\"abc\" through the end of its proof:");
H("  TAG \"set.mm\" \"@@@\" \"\" \"abc $p\" 1 \"$.\" 1");
H("so that later, SUBSTITUTE can be used to affect only those lines.  You");
H("can remove the \"@@@\" tags with SUBSTITUTE when done.");

g_printHelp = !strcmp(saveHelpCmd, "HELP DELETE");
H("This command deletes the part of a line between (and including) the first");
H("occurrence of <startstr> and the first occurrence of <endstr> (when both");
H("exist) for all lines in a file.  If either string doesn't exist in a line,");
H("the line will be unchanged.  If <startstr> is blank (''), the deletion");
H("will start from the beginning of the line.  If <endstr> is blank, the");
H("deletion will end at the end of the line.");
H("Syntax:  DELETE <iofile> <startstr> <endstr>");

g_printHelp = !strcmp(saveHelpCmd, "HELP CLEAN");
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

g_printHelp = !strcmp(saveHelpCmd, "HELP SUBSTITUTE")
    || !strcmp(helpCmd, "HELP S");
H("This command performs a simple string substitution in each line of a file.");
H("If the string to be replaced is \"\\n\", then every other line will");
H("be joined to the one below it.  If the replacement string is \"\\n\", then");
H("each line will be split into two if there is a match.");
H("The <matchstr> specifies a string that must also exist on a line");
H("before the substitution takes place; null means match any line.");
H("The <occurrence> is an integer (1 = first occurrence on each line, etc.)");
H("or A for all occurrences on each line.");
H("Syntax:  SUBSTITUTE <iofile> <oldstr> <newstr> <occurrence> <matchstr>");
H("Note: The SUBSTITUTE command may be abbreviated by S.");


g_printHelp = !strcmp(saveHelpCmd, "HELP SWAP");
H("This command swaps the parts of each line before and after a");
H("specified string.");


g_printHelp = !strcmp(saveHelpCmd, "HELP INSERT");
H("This command inserts a string at a specified column in each line");
H("in a file.  It is intended to aid further processing of column-");
H("sensitive files.  Note: the index of the first column is 1, not 0.  If a");
H("line is shorter than <column>, then it is padded with spaces so that");
H("<string> is still added at <column>.");
H("Syntax:  INSERT <iofile> <string> <column>");


g_printHelp = !strcmp(saveHelpCmd, "HELP BREAK");
H("This command breaks up a file into tokens, one per line, breaking at");
H("whitespace and any special characters you specify as delimiters.");
H("Use an explicit (quoted) space as <specchars> to avoid the default");
H("special characters and break only on whitespace.");
H("Syntax:  BREAK <iofile> <specchars>");

g_printHelp = !strcmp(saveHelpCmd, "HELP BUILD");
H("This command combines a list of tokens into multiple tokens per line,");
H("as many as will fit per line, separating them with spaces.");
H("Syntax:  BUILD <iofile>");


g_printHelp = !strcmp(saveHelpCmd, "HELP MATCH");
H("This command extracts from a file those lines containing (Y) or not");
H("containing (N) a specified string.");
H("Syntax:  MATCH <iofile> <matchstr> <Y/N>");


g_printHelp = !strcmp(saveHelpCmd, "HELP SORT");
H("This command sorts a file, comparing lines starting at a key string.");
H("If the key string is blank, the line is compared starting at column 1.");
H("If a line doesn't contain the key, it is compared starting at column 1.");
H("Syntax:  SORT <iofile> <key>");


g_printHelp = !strcmp(saveHelpCmd, "HELP UNDUPLICATE");
H("This command sorts a file then removes any duplicate lines from the output.");
H("Syntax:  UNDUPLICATE <iofile>");


g_printHelp = !strcmp(saveHelpCmd, "HELP DUPLICATE");
H("This command finds all duplicate lines in a file and places them, in");
H("sorted order, into the output file.");
H("Syntax:  DUPLICATE <iofile>");


g_printHelp = !strcmp(saveHelpCmd, "HELP UNIQUE");
H("This command finds all unique lines in a file and places them, in");
H("sorted order, into the output file.");
H("Syntax:  UNIQUE <iofile>");


g_printHelp = !strcmp(saveHelpCmd, "HELP REVERSE");
H("This command reverses the order of the lines in a file.");
H("Syntax:  REVERSE <iofile>");


g_printHelp = !strcmp(saveHelpCmd, "HELP RIGHT");
H("This command right-justifies the lines in a file by putting spaces in");
H("front of them so that they end in the same column as the longest line");
H("in the file.");
H("Syntax:  RIGHT <iofile>");


g_printHelp = !strcmp(saveHelpCmd, "HELP PARALLEL");
H("This command puts two files side-by-side.");
H("The two files should have the same number of lines; if not, a warning is");
H("issued and the longer file paralleled with empty strings at the end.");
H("Syntax:  PARALLEL <inpfile1> <inpfile2> <outfile> <btwnstr>");


g_printHelp = !strcmp(saveHelpCmd, "HELP NUMBER");
H("This command creates a list of numbers.  Hint:  Use the RIGHT command to");
H("right-justify the list after creating it.");
H("Syntax:  NUMBER <outfile> <first> <last> <incr>");


g_printHelp = !strcmp(saveHelpCmd, "HELP COUNT");
H("This command counts the occurrences of a string in a file and displays");
H("some other statistics about the file.  The sum of the lines is obtained");
H("by extracting digits and is only valid if the file consists of genuine");
H("numbers.");
H("Syntax:  COUNT <inpfile> <string>");


g_printHelp = !strcmp(saveHelpCmd, "HELP TYPE") || !strcmp(helpCmd, "HELP T");
H("This command displays (i.e. types out) the first n lines of a file on the");
H("terminal screen.  If n is not specified, it will default to 10.  If n is");
H("the string \"ALL\", then the whole file will be typed.");
H("Syntax:  TYPE <inpfile> <n>");
H("Note: The TYPE command may be abbreviated by T.");


g_printHelp = !strcmp(saveHelpCmd, "HELP COPY") || !strcmp(helpCmd, "HELP C");
H("This command copies (concatenates) all input files in a comma-separated");
H("list (no blanks allowed) to an output file.  The output file may have");
H("the same name as an input file.  Any previous version of the output");
H("file is renamed with a ~1 extension.");
H("Example: \"COPY 1.tmp,1.tmp,2.tmp 1.tmp\" followed by \"UNIQUE 1.tmp\"");
H("will result in 1.tmp containing those lines of 2.tmp that didn't");
H("previously exist in 1.tmp.");
H("Syntax:  COPY <inpfile,inpfile,...> <outfile>");
H("Note: The COPY command may be abbreviated by C.");

g_printHelp = !strcmp(saveHelpCmd, "HELP UPDATE");
H("This command tags edits made to a program source.  The idea is to keep");
H("all past history of a file in the file itself, in the form of comments.");
H("UPDATE was written for a proprietary language that allowed nested C-style");
H("comments, and it may not be generally useful without some modification.");
H("Essentially a (Unix) diff-like algorithm looks for changes between an");
H("original and a revised file and puts the original lines into the revised");
H("file in the form of comments.  Currently it is not well documented and it");
H("may be easiest just to type UPDATE <return> and answer the questions.");
H("Try it on an original and edited version of a test file to see if you");
H("find it useful.");
H("Syntax:  UPDATE <originfile> <editedinfile> <editedoutfile> <tag> <match>");

g_printHelp = !strcmp(saveHelpCmd, "HELP CLI");
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

g_printHelp = !strcmp(saveHelpCmd, "HELP SUBMIT");
H("Syntax:  SUBMIT <filename> [/ SILENT]");
H("");
H("This command causes further command lines to be taken from the specified");
H("file.  Note that any line beginning with an exclamation point (!) is");
H("treated as a comment (i.e. ignored).  Also note that the scrolling");
H("of the screen output is continuous.");
H("");
H("Optional qualifier:");
H("    / SILENT - This qualifier suppresses the screen output of the SUBMIT");
H("        command.");
H("");
H("SUBMIT can be called recursively, i.e., SUBMIT commands are allowed");
H("inside of a command file.");


g_printHelp = !strcmp(saveHelpCmd, "HELP SYSTEM");
H("A line enclosed in single or double quotes will be executed by your");
H("computer's operating system, if it has such a feature.  For example, on a");
H("Unix system,");
H("    Tools> 'ls | more'");
H("will list disk directory contents.  Note that this feature will not work");
H("on the pre-OSX Macintosh, which does not have a command line interface.");
H("");
H("For your convenience, the trailing quote is optional, for example:");
H("    Tools> 'ls | more");
H("");

let(&saveHelpCmd, ""); /* Deallocate memory */

return;
} /* help0 */


/* Note: help1 should contain Metamath help */
void help1(vstring helpCmd) {

vstring saveHelpCmd = "";
/* help1() may be called with a temporarily allocated argument (left(),
   cat(), etc.), and the let()s in the eventual print2() calls will
   deallocate and possibly corrupt helpCmd.  So, we grab a non-temporarily
   allocated copy here.  (And after this let(), helpCmd will become invalid
   for the same reason.)  */
let(&saveHelpCmd, helpCmd);


g_printHelp = !strcmp(saveHelpCmd, "HELP CLI");
H("The Metamath program was first developed on a VAX/VMS system, and some");
H("aspects of its command line behavior reflect this heritage.  Hopefully");
H(
"you will find it reasonably user-friendly once you get used to it.");
H("");
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
H("To find out what commands are available, type ? at the MM> prompt,");
H("followed by <return>. (This is actually just a trick to force an error");
H("message, since ? is not a legal command.)");
H("");
H("To find out the choices for the next argument for a command, press");
H("<return> and you will be prompted for it.  The default choice (the one");
H("selected if you just press <return>) is shown in brackets (<>).");
H("");
H("You may also type ? in place of a command word to force Metamath to tell");
H("you what the choices are.  The ? method won't work, though, if a");
H("non-keyword argument such as a file name is expected at that point,");
H("because the CLI will think the ? is the argument.");
H("");
H("Some commands have one or more optional qualifiers that modify the");
H("behavior of the command.  Qualifiers are indicated by a slash (/), such as");
H("in READ set.mm / VERIFY.  Spaces are optional around / and =.  If you need");
H("to use / or = in a command argument, as in a Unix file name, put single");
H("or double quotes around the command argument.  See the last section of");
H("HELP LET for more information on special characters in arguments.");
H("");
H("The OPEN LOG command will save everything you see on the screen, and is");
H("useful to help you recover should something go wrong in a proof, or if");
H("you want to document a bug.");
H("");
H("If the response to a command is more than a screenful, you will be");
H("prompted to '<return> to continue, Q to quit, or S to scroll to end'.");
H("Q will complete the command internally but suppress further output until");
H("the next \"MM>\" prompt.  S will suppress further pausing until the next");
H("\"MM>\" prompt.  After the first screen, you can also choose B to go back");
H("a screenful.  Note that B may also be entered at the \"MM>\" prompt");
H("immediately after a command to scroll back through the output of that");
H("command.  Scrolling can be disabled with SET SCROLL CONTINUOUS.");
H("");
H("**Warning**  Pressing CTRL-C will abort the Metamath program");
H("unconditionally.  This means any unsaved work will be lost.");
H("");
H("A command line enclosed in quotes is executed by your operating system.");
H("See HELP SYSTEM.");
H("");
H("Some additional CLI-related features are explained by:");
H("");
H("    HELP SET ECHO");
H("    HELP SET SCROLL");
H("    HELP SET WIDTH");
H("    HELP SET HEIGHT");
H("    HELP SUBMIT");
H("    HELP UNDO (or REDO) - in Proof Assistant only");
H("");



g_printHelp = !strcmp(saveHelpCmd, "HELP LANGUAGE");
H("The language is best learned by reading the book and studying a few proofs");
H("with the Metamath program.  This is a brief summary for reference.");
H("");
H("The database contains a series of tokens separated by whitespace (spaces,");
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
H("  ${ - Start of scope.");
H("       Syntax:  \"${\"");
H("");
H("  $} - End of scope:  all $v, $e, $f, and $d statements in the current");
H("         scope become inactive.");
H("       Syntax:  \"$}\"");
H("");
H("  Note that $a and $p statements remain active forever.  Note that $c's");
H("  may be used only in the outermost scope, so they are always active.");
H("  The outermost scope is not bracketed by ${ ... $} .  The scope of a $v,");
H("  $e, $f, or $d statement starts where the statement occurs and ends with");
H("  the $} that matches the previous ${.  The scope of a $c, $a, or $p");
H("  statement starts where the statement occurs and ends at the end of the");
H("  database.");
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
H("  $f - Variable-type (or \"floating\") hypothesis (meaning it is");
H("         \"required\" by a $p or $a statement in its scope only if its");
H("         variable occurs in the $p or $a statement or in the essential");
H("         hypotheses of the $p or $a statement).  Every $d, $e, $p, and $a");
H("         statement variable must have an earlier active $f statement to");
H("         specify the variable type.  Non-required i.e. \"optional\" $f");
H("         statements may be referenced inside a proof when dummy variables");
H("         are needed by the proof.");
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
H("         SAVE PROOF <label> / NORMAL will convert a compressed proof to");
H("         its uncompressed form.");
H("");
H("  A substitution is the replacement of a variable with a <symbol> string");
H("  throughout an assertion and its required hypotheses.  The required");
H("  hypotheses are shown as the \"mandatory hypotheses\" listed by");
H("  SHOW STATEMENT <label> / FULL.");
H("");
H("  In a proof, the label of a hypothesis ($e or $f) pushes the stack, and");
H("  the label of an assertion ($a or $p) pops from the stack a number of");
H("  entries equal to the number of the assertion's required hypotheses and");
H("  replaces the stack's top.  Whenever an assertion is specified, a unique");
H("  set of substitutions must exist that makes the assertion's hypotheses");
H("  match the top entries of the stack.");
H("");
H("  To see a readable proof format, type SHOW PROOF <label>, where <label>");
H("  is the label of a $p statement.  To see how substitutions are made in a");
H("  proof step, type SHOW PROOF <label> / DETAILED_STEP <n>, where <n> is");
H("  the step number from the SHOW PROOF <label> listing.");
H("");
H("Disjoint variable restriction:");
H("");
H("  The substitution of symbol strings into variables may be subject to a");
H("  $d restriction:");
H("");
H("  $d - Disjoint variable restriction (meaning substitutions may not");
H("         have variables in common)");
H("       Syntax:  \"$d <symbol> ... <symbol> $.\" where <symbol> is active");
H("         and previously declared with $v, and all <symbol>s are distinct");
H("");
H("Auxiliary keywords:");
H("");
H("  $(   Begin comment");
H("  $)   End comment");
H("       Markup in comments:");
H("         ` <symbol> ` - use graphical <symbol> in LaTeX/HTML output;");
H("             `` means literal `; several <symbol>s may occur inside");
H("             ` ... ` separated by whitespace");
H("         ~ <label> - use typewriter font (hyperlink) in LaTeX (HTML) output;");
H("             if <label> begins with \"http://\", it is assumed to be");
H("             a URL (which is used as-is, except a \"~\" in the URL should");
H("             be specified as \"~~\") rather than a statement label (which");
H("             will have \".html\" appended for the hyperlink); only $a and $p");
H("             statement labels may be used, since $e, $f pages don't exist");
H("         [<author>] - link to bibliography; see HELP HTML and HELP WRITE");
H("             BIBLIOGRAPHY");
H("         $t - flags comment as containing LaTeX and/or HTML typesetting");
H("             definitions; see HELP LATEX or HELP HTML");
H("         _ - Italicize text from <space>_<non-space> to");
H("             <non-space>_<space>; normal punctuation (e.g. trailing");
H("             comma) is ignored when determining <space>");
H("         _ - <non-space>_<non-space-string> will make <non-space-string>");
H("             a subscript");
H("         <HTML> - A comment containing \"<HTML>\" (case-sensitive) is");
H("             bypassed by the algorithm of SHOW PROOF ... / REWRAP.  Also,");
H("             \"<\" is not converted to \"&lt;\" by the algorithm.  The");
H("             \"<HTML>\" is discarded in the generated web page.  Any");
H("             \"</HTML>\" (deprecated) is discarded and ignored.  Note that");
H("             the entire comment (not just sections delineated by");
H("             \"<HTML>...</HTML>\") is treated as HTML code if any");
H("             \"<HTML>\" is present anywhere in the comment.");
H("             See also HELP WRITE SOURCE for more information.");
H("         (Contributed by <author>, <date>.)");
H("         (Revised by <author>, <date>.)");
H("         (Proof shortened by <author>, <date>.)");
H("             The above dates are checked by VERIFY MARKUP.");
H("         (New usage is discouraged.)");
H("         (Proof modification is discouraged.)");
H("             See HELP SHOW DISCOURAGED and HELP SET DISCOURAGEMENT.");
H("       Note:  Comments may not be nested.");
H("");
H("  $[ <file-name> $] - place contents of file <file-name> here; a second,");
H("       recursive, or self reference to a file is ignored");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP MARKUP");
H("(See HELP VERIFY MARKUP for the markup language used in database");
H("comments.)");
H("");
H("Syntax:  MARKUP <inpfile> <outfile> [/ HTML] [/ ALT_HTML] [/ SYMBOLS]");
H("    [/ LABELS] [/ NUMBER_AFTER_LABEL] [/ BIBLIOGRAPHY] [/ UNDERSCORES]");
H("    [/ CSS]");
H("");
H("Note:  In most cases, use / ALT_HTML / SYMBOLS / LABELS / CSS.");
H("");
H("This command will read an arbitrary <inpfile>, normally an HTML file");
H("with markup, treating it as if it were a giant comment in a database file");
H("and translating any markup into HTML.  The translated result is written to");
H("<outfile>.  Note that the file names may be enclosed in single or double");
H("quotes; this is required if a file name contains slashes, as might be the");
H("case with Unix file path names.");
H("");
H("This command requires that a database source file (such as set.mm) be");
H("read.  See HELP READ. The math symbols and other information are taken");
H("from that database.  The use of VERIFY MARKUP * is recommended to help");
H("ensure the database has no errors in its symbol definitions.");
H("");
H("Qualifiers:");
H("    / HTML (/ ALT_HTML) - use the symbols defined by the htmldef");
H("        (althtmldef) statements in the $t comment in the .mm database.");
H("        Usually these are GIF or Unicode math symbols respectively.");
H("        Exactly one of / HTML and / ALT_HTML must always be specified.");
H("    / SYMBOLS - process symbols inside backquotes.");
H("    / LABELS - process labels preceded by tilde.");
H("    / NUMBER_AFTER_LABEL - add colored statement number after each label.");
H("    / BIB_REFS - process bibliographic references in square brackets.");
H("        The file specified by htmlbibliography in the $t comment in the");
H("        .mm database is checked to be sure the references exist.");
H("    / UNDERSCORES - process underscores to produce italic text or");
H("        subscripts.");
H("    / CSS - add CSS before \"</HEAD>\" in the input file.  The CSS is");
H("        specified by htmlcss in the $t comment in .mm database.  If");
H("        \"</HEAD>\" is not present, or the CSS is already present (with");
H("        an exact match), nothing will be done.");
H("");
H("Note:  The existence of GIF files for symbols isn't checked.  Use VERIFY");
H("MARKUP for that.  However, validity of bibliographical references is");
H("checked since VERIFY MARKUP can't do that.  If the required file for");
H("/ BIB_REFS (such as mmset.html) isn't present, a warning will be");
H("displayed.  To avoid literal \"`\", \"~\", and \"[\" from being");
H("interpreted by / SYMBOLS, / LABELS, and / BIB_REFS respectively, escape");
H("them with \"``\", \"~~\", and \"[[\" in the input file.  Literal \"`\"");
H("must always be escaped even if / SYMBOLS is omitted, because the");
H("algorithm will still use \"`...`\" to avoid interpreting special");
H("characters in math symbols.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP EXPLORE");
H("When you first enter Metamath, you will first want to READ in a Metamath");
H("source file.  The source file provided for set theory is called set.mm;");
H("to read it type");
H("    READ set.mm");
H("");
H("You may want to look over the contents of the source file with a text");
H("editor, to get an idea of what's in it, before starting to use Metamath.");
H("");
H("The following commands will help you study the source file statements");
H("and their proofs.  Use the HELP for the individual commands to get");
H("more information about them.");
H("    SEARCH <label-match> \"<symbol-match>\" - Displays statements whose");
H("        labels match <label-match> and that contain <symbol-match>.");
H("    SEARCH <label-match> \"<search-string>\" / COMMENTS - Shows statements");
H("        whose preceding comment contains <search-string>");
H("    SHOW LABELS <label-match> - Lists all labels matching <label-match>,");
H("        with * and ? wildcards:  for example \"abc?def*\" will match all");
H("        labels beginning with \"abc\" followed by any single character");
H("        followed by \"def\".");
H("    SHOW STATEMENT <label> / COMMENT - Shows the comment, contents, and");
H("        logical hypotheses associated with a statement.");
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



g_printHelp = !strcmp(saveHelpCmd, "HELP HTML");
H("(Note: See HELP WRITE SOURCE for the \"<HTML>\" tag in comments.)");
H("To create an HTML output file for a $a or $p statement, use");
H("    SHOW STATEMENT <label> / HTML");
H("The created web page will include a Description taken from the comment");
H("that immediately precedes the $a or $p statement.  A warning will be");
H("issued if this comment is not present.  Optional markup in the comment");
H("will be processed according to the markup syntax described under HELP");
H("LANGUAGE, in the \"Inside of comments\" section.  Warnings will be");
H("issued for any errors in the markup.  Note that all other comments in");
H("the database are ignored, including comments preceding $e statements.");
H("");
H("When <label> has wildcard (* and ?) characters, all statements with");
H("matching labels will have HTML files produced for them.  Also, when");
H("<label> starts with a * wildcard character, three additional files,");
H("mmdefinitions.html, mmtheoremsall.html, and mmascii.html will be");
H("produced.  Thus:");
H("    SHOW STATEMENT * / HTML");
H("will output the complete HTML proof database in the current directory,");
H("one file per $a and $p statement, along with mmdefinitions.html,");
H("mmtheoremsall.html, and mmascii.html.  The statement:");
H("    SHOW STATEMENT *! / HTML");
H("will produce only mmdefinitions.html, mmmmtheoremsall.html, and");
H("mmascii.html, but no other HTML files (because no labels can match \"*!\"");
H("since \"!\" is illegal in a statement label).  The statement:");
H("    SHOW STATEMENT ?* / HTML");
H("will output the complete HTML proof database but will not produce");
H("mmdefinitions.html, etc.  Note added 30-Jan-06:  The mmtheoremsall.html");
H("file produced by this command is deprecated and is replaced by the output");
H("of WRITE THEOREM_LIST.");
H("");
H("The HTML definitions for the symbols and and other features are");
H("specified by statements in a special typesetting comment in the input");
H("database file.  The typesetting comment is identified by the token \"$t\"");
H("in the comment, and the typesetting statements run until the next \"$)\":");
H("   ...  $( ...  $t ................................ $) ...");
H("                   <-- HTML definitions go here -->");
H("See the set.mm database file for an extensive example of a $t comment");
H("illustrating all features described below.  In the HTML definition");
H("section, C-style comments /* ... */ are recognized.  The main HTML");
H("specification statements are:");
H("    htmldef \"<mathtoken>\" as \"<HTML code for mathtoken symbol>\" ;");
H("                    ...");
H("    htmldef \"<mathtoken>\" as \"<HTML code for mathtoken symbol>\" ;");
H("    htmltitle \"<HTML code for title>\" ;");
H("    htmlhome \"<HTML code for home link>\" ;");
H("    htmlvarcolor \"<HTML code for variable color list>\" ;");
H("    htmlbibliography \"<HTML file>\" ;");
H("        (This <HTML file> is assumed to have a <A NAME=...> tag for each");
H("        bibiographic reference in the database comments.  For example");
H("        if \"[Monk]\" occurs in a comment, then \"<A NAME='Monk'>\" must");
H("        be present in the <HTML file>; if not, a warning message is");
H("        given.)");
H("Single or double quotes surround the field strings, and fields too long");
H("for a line may be broken up into multiple quoted strings connected with");
H("(whitespace-surrounded) \"+\" signs (no quotes around them).  Inside");
H("quoted strings, the opposite kind of quote may appear.  If both kinds of");
H("quotes are needed, use separate quoted strings connected by \"+\".");
H("Note that the \"$)\" character sequence will flag the end of the");
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
H("an alternate representation of symbols is desired, for example one that");
H("uses Unicode entities instead of GIF images.  Associated with althtmldef");
H("are the statements");
H("    htmldir \"<directory for GIF HTML version>\" ;");
H("    althtmldir \"<directory for Unicode HTML version>\" ;");
H("that produce links to the alternate version.");
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
H("When two different types of pages need to be produced from a single");
H("database, such as the Hilbert Space Explorer that extends the Metamath");
H("Proof Explorer, \"extended\" variables may be declared in the $t comment:");
H("    exthtmltitle \"<HTML code for title>\" ;");
H("    exthtmlhome \"<HTML code for home link>\" ;");
H("    exthtmlbibliography \"<HTML file>\" ;");
H("When these are declared, you also must declare");
H("    exthtmllabel \"<label>\" ;");
H("When the output statement is the one declared with \"exthtmllabel\" or");
H("a later one, the HTML code assigned to \"exthtmltitle\" and");
H("\"exthtmlhome\" is used instead of that assigned to \"htmltitle\" and");
H("\"htmlhome\" respectively.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP LATEX");
H("See HELP TEX.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP TEX");
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
H("    SHOW PROOF uneq2 / LEMMON / RENUMBER / TEX");
H("    CLOSE TEX");
H("");
H("The LaTeX symbol definitions should be included in a special comment");
H("containing a $t token.  See the set.mm file for an example.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP BEEP") || !strcmp(helpCmd, "HELP B");
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


g_printHelp = !strcmp(saveHelpCmd, "HELP QUIT");
H("Syntax:  QUIT [/ FORCE]");
H("");
H("This command is a synonym for EXIT.  See HELP EXIT.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP EXIT");
H("Syntax:  EXIT [/ FORCE]");
H("");
H("This command exits from Metamath.  If there have been changes to the");
H("database with the SAVE PROOF or SAVE NEW_PROOF commands, you will be given");
H("an opportunity to WRITE SOURCE to permanently save the changes.");
H("");
H("(In Proof Assistant mode) The EXIT command will return to the MM> prompt.");
H("If there were changes to the proof, you will be given an opportunity to");
H("SAVE NEW_PROOF.  In the Proof Assistant, _EXIT_PA is a synonym for EXIT");
H("that gives an error message outside of the Proof Assistant.  This can be");
H("useful to prevent scripts from exiting Metamath due to an error entering");
H("the Proof Assistant.");
H("");
H("The QUIT command is a synonym for EXIT.");
H("");
H("Optional qualifier:");
H("    / FORCE - Do not prompt if changes were not saved.  This qualifier is");
H("        useful in SUBMIT command files (scripts) to ensure predictable");
H("        behavior.");
H("");
H("**Warning**  Pressing CTRL-C will abort the Metamath program");
H("unconditionally.  This means any unsaved work will be lost.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP _EXIT_PA");
H("Syntax:  _EXIT_PA [/ FORCE]");
H("");
H("This command is a synonym for EXIT inside the Proof Assistant but will");
H("generate an error message (and otherwise have no effect) elsewhere.  It");
H("can help prevent accidentally exiting Metamath when a script fails to");
H("enter the Proof Assistant (PROVE command).  See HELP EXIT.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP READ");
H("Syntax:  READ <file> [/ VERIFY]");
H("");
H("This command will read in a Metamath language source file and any included");
H("files.  Normally it will be the first thing you do when entering Metamath.");
H("Statement syntax is checked, but proof syntax is not checked.");
H("Note that the file name may be enclosed in single or double quotes;");
H("this is useful if the file name contains slashes, as might be the case");
H("under Unix.");
H("");
H("If you are getting an \"?Expected VERIFY or NOVERIFY\" error when trying");
H("to read a Unix file name with slashes, you probably haven't quoted it.");
H("");
H("You need nested quotes when a Unix file name with slashes is a Metamath");
H("invocation argument.  See HELP INVOKE for examples.");
H("");
H("If you are prompted for the file name (by pressing <return> after READ)");
H("you should _not_ put quotes around it, even if it is a Unix file name.");
H("with slashes.");
H("");
H("Optional qualifier:");
H("    / VERIFY - Verify all proofs as the database is read in.  This");
H("        qualifier will slow down reading in the file.");
H("");
H("See also HELP ERASE.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP ERASE");
H("Syntax:  ERASE");
H("");
H("This command will delete the database if one was READ in.  It does not");
H("affect parameters listed in SHOW SETTINGS that are unrelated to the");
H("database.  The user will be prompted for confirmation if the database was");
H("changed but not saved with WRITE SOURCE.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP OPEN LOG");
H("Syntax:  OPEN LOG <file>");
H("");
H("This command will open a log file that will store everything you see on");
H("the screen.  It is useful to help recovery from a mistake in a long Proof");
H("Assistant session, or to document bugs.");
H("");
H("The screen output of operating system commands (HELP SYSTEM) is not");
H("logged.");
H("");
H("The log file can be closed with CLOSE LOG.  It will automatically be");
H("closed upon exiting Metamath.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP CLOSE LOG");
H("Syntax:  CLOSE LOG");
H("");
H("The CLOSE LOG command closes a log file if one is open.  See also OPEN");
H("LOG.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP OPEN TEX");
H("Syntax:  OPEN TEX <file> [/ NO_HEADER] [/ OLD_TEX]");
H("");
H("This command opens a file for writing LaTeX source and writes a LaTeX");
H("header to the file.  LaTeX source can be written with the SHOW PROOF,");
H("SHOW NEW_PROOF, and SHOW STATEMENT commands using the / TEX qualifier.");
H("The mapping to LaTeX symbols is defined in a special comment containing");
H("a $t token.  See the set.mm database file for an example.");
H("");
H("To format and print the LaTeX source, you will need the TeX program,");
H("which is standard in most Linux, Unix, and MacOSX installations and");
H("available for Windows.");
H("");
H("Optional qualifiers:");
H("    / NO_HEADER - This qualifier prevents a standard LaTeX header and");
H("        trailer from being included with the output LaTeX code.");
H("    / OLD_TEX - This qualifier produces a header with macro definitions");
H("        for use with / OLD_TEX qualifiers of SHOW STATEMENT and SHOW");
H("        PROOF.  It is obsolete and will be removed eventually.");
H("");
H("See also CLOSE TEX.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP CLOSE TEX");
H("Syntax:  CLOSE TEX");
H("");
H("This command writes a trailer to any LaTeX file that was opened with OPEN");
H("TEX (unless / NO_HEADER was used with OPEN) and closes the LaTeX file.");
H("");
H("See also OPEN TEX.");
H("");


g_printHelp = !strcmp(saveHelpCmd, "HELP TOOLS");
H("Syntax:  TOOLS");
H("");
H("This command invokes a utility to manipulate ASCII text files.  Type TOOLS");
H("to enter this utility, which has its own HELP commands.  Once you are");
H("inside, EXIT will return to Metamath.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP WRITE SOURCE");
H("Syntax:  WRITE SOURCE <filename> [/ FORMAT] [/ REWRAP] [/ SPLIT]");
H("           [/ KEEP_INCLUDES] [/ NO_VERSIONING]");
H("");
H("This command will write the contents of a Metamath source (previously");
H("read with READ) into a file");
H("(or multiple files if / SPLIT is specified).");
H("");
H("Optional qualifiers:");
H("    / FORMAT - Reformats statements and comments according to the");
H("        convention used in the set.mm database.  Proofs are not");
H("        reformatted; use SAVE PROOF * / COMPRESSED to do that.");
H("        Incidentally, SAVE PROOF honors the SET WIDTH parameter");
H("        currently in effect.");
H("    / REWRAP - Same as / FORMAT but more aggressive.  It unwraps the");
H("        lines in the comment before each $a and $p statement, then it");
H("        rewraps the line.  You should compare the output to the original");
H("        to make sure that the desired effect results; if not, go back to");
H("        the original source.  The wrapped line length honors the");
H("        SET WIDTH parameter currently in effect.  Note 1: The only lines");
H("        that are rewrapped are those in comments immediately preceding a");
H("        $a or $p statement.  In particular, formulas (such as the");
H("        argument of a $p statement) are not rewrapped.  Note 2: A comment");
H("        containing the string \"<HTML>\" is not rewrapped (see also");
H("        HELP LANGUAGE and");
H("   https://github.com/metamath/set.mm/pull/1695#issuecomment-652129129 .)");
H("    / SPLIT - Files included in the source with $[ <inclfile> $] will be");
H("        written out separately instead of included in a single output");
H("        file.  The name of each separately written included file will be");
H("        <inclfile> argument of its inclusion command.  See the");
H("        21-Dec-2017 (file inclusion) entry in");
H("        http://us.metamath.org/mpeuni/mmnotes.txt for further details");
H("    / KEEP_INCLUDES - If a source file has includes but is written as a");
H("        single file by omitting / SPLIT, by default the included files will");
H("        be deleted (actually just renamed with a ~1 suffix unless");
H("        / NO_VERSIONING is specified) to prevent the possibly confusing");
H("        source duplication in both the output file and the included file.");
H("        The / KEEP_INCLUDES qualifier will prevent this deletion.");
H("    / NO_VERSIONING - Backup files suffixed with ~1 are not created.");
H("    / EXTRACT <label-match> - Write to the output file only those");
H("        statements needed to support and prove the statements matching");
H("        <label-match>.  See HELP SEARCH for the format of <label-match>.");
H("        A single output file is created.  Note that all includes");
H("        \"$[...$]\", all commented includes \"$( Begin $[...\" etc.,");
H("        and all \"$j\" comments will be discarded.  / EXTRACT and / SPLIT");
H("        may not be used together.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP WRITE THEOREM_LIST");
H("Syntax:  WRITE THEOREM_LIST [/ THEOREMS_PER_PAGE <number>] [/ SHOW_LEMMAS]");
H("               [/ HTML] [/ALT_HTML] [/ NO_VERSIONING]");
H("");
H("Optional qualifiers:");
H("    / THEOREMS_PER_PAGE <number> - specifies the number of theorems to");
H("        write per output file");
H("    / SHOW_LEMMAS - show the math content of lemmas (by default, the math");
H("        content of theorems whose comment begins \"Lemma for\" is");
H("        suppressed to reduce the web page file size).");
H("    / HTML (/ ALT_HTML) - use the symbols defined by the htmldef");
H("        (althtmldef) statements in the $t comment in the .mm database.");
H("        Usually these are GIF or Unicode math symbols respectively.");
H("    / NO_VERSIONING - Backup files suffixed with ~1 are not created.");
H("");
H("This command writes a list of the $a and $p statements in the database");
H("into web page files called \"mmtheorems.html\", \"mmtheorems1.html\",");
H("\"mmtheorems2.html\", etc.  If / THEOREMS_PER_PAGE is omitted, the number");
H("of theorems (and other statements) per page defaults to 100.");
H("[Warning:  A value other than 100 for THEOREMS_PER_PAGE will cause the");
H("list to become out of sync with the \"Related theorems\" links on the");
H("web pages for individual theorems.  This may be corrected in a future");
H("version.]");
H("");
H("If neither / HTML nor / ALT_HTML is specified, the output will default to");
H("GIF format unless ALT_HTML was previously set as shown in SHOW SETTINGS.");
H("");
H("The first output file, \"mmtheorems.html\", includes a Table of Contents.");
H("An entry is triggered in the database by \"$(\" immediately followed by a");
H("new line starting with \"####\" (the marker for a major part header),");
H("\"#*#*\" (for a section header), \"=-=-\" (for a subsection header), or");
H("\"-.-.\" (for a subsubsection header).  The line following the marker line");
H("will be used for the table of contents entry, after trimming spaces.  The");
H("next line should be another (closing) marker line.  Any text after that");
H("but before the closing \"$)\", such as an extended description of the");
H("section, will be included on the mmtheoremsNNN.html page.  In between two");
H("successive statements that generate web pages (i.e. $a and $p statements),");
H("only the last of each header type (part, section, subsection,");
H("subsubsection) will be used, and any smaller header type before a larger");
H("header type (e.g. a subsection header before a section header) will be");
H("ignored.  See the set.mm database file for examples.");
H("");
H("[Warning: For the above matching, white space is NOT ignored.  There");
H("should be 0 or 1 spaces between \"$(\" and the end of the line.  This may");
H("be allowed in a future version.]");
H("");
H("Note:  To create the files mmdefinitions.html and mmascii.html, use");
H("SHOW STATEMENT *! / HTML.  See HELP HTML.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP BIBLIOGRAPHY");
H("See HELP WRITE BIBLIOGRAPHY.");
H("");

g_printHelp = !strcmp(saveHelpCmd, "HELP WRITE BIBLIOGRAPHY");
H("Syntax:  WRITE BIBLIOGRAPHY <filename>");
H("");
H("This command reads an HTML bibliographic cross-reference file, normally");
H("called mmbiblio.html, and updates it per the bibliographic links in");
H("the database comments.  The file is updated between the HTML comment");
H("lines \"<!-- #START# -->\" and \"<!-- #END# -->\".  Any previous content");
H("between these two lines is discarded.  The original input file is renamed");
H("<filename>~1");
H("");
H("A name in square brackets in a statement's description (the comment");
H("before a $a or $p statement) indicates a bibliographic reference.  The");
H("full reference must be of the form");
H("");
H("    <keyword> <identifier> <noise word(s)> [<author>] p. <nnn>");
H("");
H("There should be no comma between \"[<author>]\" and \"p.\".  Whitespace,");
H("comma, period, or semicolon should follow <nnn>.  Example:");
H("");
H("    Theorem 3.1 of [Monk] p. 22,");
H("");
H("The <keyword>, which is not case-sensitive, must be one of the following:");
H("");
H("    Axiom, Chapter, Claim, Compare, Conclusion, Condition, Conjecture,");
H("    Corollary, Definition, Equation, Example, Exercise, Fact, Figure,");
H("    Introduction, Item, Lemma, Lemmas, Line, Lines, Notation, Note,");
H("    Observation, Paragraph, Part, Postulate, Problem, Proof, Property,");
H("    Proposition, Remark, Result, Rule, Scheme, Scolia, Scolion, Section,");
H("    Statement, Subsection, Table, Theorem");
H("");
H("The <identifier> is optional, as in for example \"Remark in [Monk] p. 22\".");
H("");
H("The <noise word(s)> are zero or more from the list:  from, in, of, on.");
H("These are ignored when generating the bibliographic cross-reference.");
H("");
H("The <author> must be present in the file identified with the");
H("htmlbibliography assignment (e.g. mmset.html) in the database $t comment,");
H("in the form <A NAME=\"<author>\"></A> e.g. <A NAME=\"Monk\"></A>.");
H("");
H("The <nnn> may be any alphanumeric string such as an integer or Roman");
H("numeral.");
H("");
H("The <keyword> and <noise word(s)> lists are hard-coded into the program.");
H("Contact Norman Megill if you need to add to these lists.");
H("");
H("Additional notes:  1. The bibliographic reference in square brackets may");
H("not contain whitespace.  If it does, the bracketed text will be treated");
H("like normal text and not assumed to be a bibliographic reference.");
H("2. A double opening bracket \"[[\" escapes the bracket and treats the");
H("bracketed text as normal text, and a single bracket is rendered on the");
H("web page output.  The closing bracket need not be escaped, and \"]]\"");
H("will cause a double bracket to be rendered on the web page.");
H("");
H("See also");
H("https://github.com/metamath/set.mm/pull/1761#issuecomment-672433658");


g_printHelp = !strcmp(saveHelpCmd, "HELP WRITE RECENT_ADDITIONS");
H("Syntax:  WRITE RECENT_ADDITIONS <filename>");
H("");
H("Optional qualifier:");
H("    / LIMIT <number> - specifies the number of most recent theorems to");
H("        write to the output file");
H("    / HTML (/ ALT_HTML) - use GIF (Unicode) math symbols.");
H("");
H("This command reads an HTML Recent Additions file, normally called");
H("\"mmrecent.html\", and updates it with the descriptions of the most recently");
H("added $a and $p statements to the database.  If / LIMIT is omitted, the");
H("number of theorems written defaults to 100.  The file is updated between the");
H("HTML comment lines \"<!-- #START# -->\" and \"<!-- #END# -->\".  The");
H("original input file is renamed to \"<filename>~1\"");
H("");
H("The date used for comparison is the most recent \"(Contributed by...)\",");
H("\"(Revised by...)\", and \"(Proof shortened by...)\" date in the comment");
H("immediately preceding the statement.");
H("");
H("If neither / HTML nor / ALT_HTML is specified, the output will default to");
H("GIF format unless ALT_HTML was previously set as shown in SHOW SETTINGS.");
H("");


let(&saveHelpCmd, ""); /* Deallocate memory */
return;

} /* help1 */
