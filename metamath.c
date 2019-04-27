/*****************************************************************************/
/* Program name:  metamath                                                   */
/* Copyright (C) 2019 NORMAN MEGILL  nm at alum.mit.edu  http://metamath.org */
/* License terms:  GNU General Public License Version 2 or any later version */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/* Contributors:  In the future, the license may be changed to the MIT license
   or public domain.  Therefore I request that any patches that are contributed
   be free of copyright restrictions (i.e. public domain) in order to provide
   this flexibility.  Thank you. - NM */

/* This program should compile without warnings using:
     gcc m*.c -o metamath -O2 -Wall -Wextra -Wmissing-prototypes \
         -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-align \
         -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
         -Wconversion -Wstrict-prototypes -ansi -pedantic -Wunused-result
   For faster runtime, use:
     gcc m*.c -o metamath -O3 -funroll-loops -finline-functions \
         -fomit-frame-pointer -Wall -ansi -pedantic  -fno-strict-overflow
   With the lcc compiler on Windows, use:
     lc -O m*.c -o metamath.exe
*/

#define MVERSION "0.177 27-Apr-2019"
/* 0.177 27-Apr-2019 nm mmcmds.c -"set" -> "setvar" in htmlAllowedSubst.
   mmhlpb.c - fix typos in HELP IMPROVE. */
/* 0.176 25-Mar-2019 nm metamath.c mmcmds.h mmcmds.c mmcmdl.c mmhlpb.c -
   add /TOP_DATE_SKIP to VERIFY MARKUP */
/* 0.175 8-Mar-2019 nm mmvstr.c - eliminate warning in gcc 8.3 (patch
   provided by David Starner) */
/* 0.174 22-Feb-2019 nm mmwtex.c - fix erroneous warnng when using "[["
   bracket escape in comment */
/* 0.173 3-Feb-2019 nm mmwtex.c - fix infinite loop when "[" was the first
   character in a comment */
/* 0.172 25-Jan-2019 nm mmwtex.c - comment out bug 2343 trap (not a bug) */
/* 0.171 13-Dec-2018 nm metamath.c, mmcmdl.c, mmhlpa.c, mmcmds.c,h, mmwtex.c,h
   - add fine-grained qualfiers to MARKUP command */
/* 0.170 12-Dec-2018 nm mmwtex.c - restore line accidentally deleted in 0.169 */
/* 0.169 10-Dec-2018 nm metamath.c, mmcmds.c,h, mmcmdl.c, mmpars.c, mmhlpa.c,
   mmwtex.c - Add MARKUP command.
   9-Dec-2018 nm mmwtex.c - escape literal "[" with "[[" in comments. */
/* 0.168 8-Dec-2018 nm metamath.c - validate that /NO_REPEATED_STEPS is used
   only with /LEMMON.
   8-Dec-2018 nm mmcmds.c - fix bug #256 reported by Jim Kingdon
   (https://github.com/metamath/set.mm/issues/497). */
/* 0.167 13-Nov-2018 nm mmcmds.c - SHOW TRACE_BACK .../COUNT now uses proof
   the way it's stored (previously, it always uncompressed the proof).  The
   new step count (for compressed proofs) corresponds to the step count the
   user would see on the web pages.
   12-Nov-2018 nm mmcmds.c - added unlimited precision arithmetic
   for SHOW TRACE_BACK .../COUNT/ESSENTIAL */
/* 0.166 31-Oct-2018 nm mmwtex.c - workaround Chrome anchor bug
   30-Oct-2018 nm mmcmds.c - put "This theorem is referenced by" after
   axioms and definitions used in HTML; use "(None)" instead of suppressing
   line if nothing is referenced */
/* 0.165 20-Oct-2018 nm mmwtex.c - added ~ mmtheorems#abc type anchor
   in TOC details.  mmwtex.c - fix bug (reported by Benoit Jubin) that
   changes "_" in labels to subscript.  mmcmdl.c - remove unused COMPLETE
   qualifier from SHOW PROOF.  mmwtex.c - enhance special cases of web page
   spacing identified by Benoit Jubin */
/* 0.164 5-Sep-2018 nm mmwtex.c, mmhlpb.c - added NOTE to bib keywords
   14-Aug-2018 nm metamath.c - added defaultScrollMode to prevent
   SET SCROLL CONTINUOUS from reverting to PROMPTED after a SUBMIT command */
/* 0.163 4-Aug-2018 nm mmwtex.c - removed 2nd "sandbox:bighdr" anchor
   in mmtheorems.html; removed Firefox and IE references; changed breadcrumb
   font to be consistent with other pages; put asterisk next to TOC entries
   that have associated comments */
/* FOR FUTURE REFERENCE: search for "Thierry" in mmwtex.c to modify the link
   to tirix.org structured proof site */
/* 0.162-thierry 3-Jun-2018 nm mmwtex.c - add link to tirix.org structured
   proofs */
/* 0.162 3-Jun-2018 nm mmpars.c - re-enabled error check for $c not in
   outermost scope.  mmhlpa.c mmhlpb.c- improve some help messages.
   mmwtex.c - added "Observation", "Proof", and "Statement" keywords for
   WRITE BIBLIOGRAPHY */
/* 0.161 2-Feb-2018 nm mmpars.c,h mmcmds.c mmwtex.c - fix wrong file name
   and line number in error messages */
/* 0.160 24-Jan-2018 nm mmpars.c - fix bug introduced in version 0.158 */
/* 0.159 23-Jan-2018 nm mmpars.c - fix crash due to missing include file */
/* 0.158 22-Jan-2018 nm mminou.c - strip CRs from Windows SUBMIT files
   run on Linux */
/* 0.157 15-Jan-2018 nm Major rewrite of READ-related functions.
     Added HELP MARKUP.
   9-Jan-2018 nm Track line numbers for error messages in included files
   1-Jan-2018 nm Changed HOME_DIRECTORY to ROOT_DIRECTORY
   31-Dec-2017 nm metamath.c mmcmdl.c,h mmpars.c,h mmcmds.c,h mminou.c,h
     mmwtex.c mmhlpb.c mmdata.h - add virtual includes "$( Begin $[...$] $)",
     $( End $[...$] $)", "$( Skip $[...$] $)" */
/* 0.156 8-Dec-2017 nm mmwtex.c - fix bug that incorrectly gave "verify markup"
   errors when there was a mathbox statement without an "extended" section */
/* 0.155 8-Oct-2017 nm mmcmdl.c - restore accidentally removed HELP HTML;
   mmhlpb.c mmwtex.c mmwtex.h,c mmcmds.c metamath.c - improve HELP and make
   other cosmetic changes per Benoit Jubin's suggestions */
/* 0.154 2-Oct-2017 nm mmunif.h,c mmcmds.c - add 2 more variables to ERASE;
   metamath.c mmcmdl.c - remove obsolete OPEN/CLOSE HTML; mmhlpa.c mmhlpb.c -
   fix typos reported by Benoit Jubin */
/* 0.153 1-Oct-2017 nm mmunif.c,h mmcmds.c - Re-initialize internal nmbrStrings
   in unify() after 'erase' command reported by Benoit Jubin */
/* 0.152 26-Sep-2017 nm mmcmds.c - change default links from mpegif to mpeuni;
   metamath.c - enforce minimum screen width = 3 to prevent crash reported
   by Benoit Jubin */
/* 0.151 20-Sep-2017 nm mmwtex.c - better matching to insert space between
   A and y in "E. x e. ran A y R x" to prevent spurious spaces in thms rncoeq,
   dfiun3g as reported by Benoit Jubin */
/* 0.150 26-Aug-2017 nm mmcmds.c,mmwtex.h - fix hyperlink for Distinct variable
   etc. lists so that it will point to mmset.html on other Explorers like NF.
   Move the "Dummy variables..." to print after the "Proof of Theorem..."
   line. */
/* 0.149 21-Aug-2017 nm mmwtex.c,h mmcmds.c mmhlpb.c - add a subsubsection
     "tiny" header with separator "-.-." to table of contents and theorem list;
     see HELP WRITE THEOREM_LIST
   21-Aug-2017 nm mmcmds.c - remove bug check 255
   19-Aug-2017 nm mmcmds.c - change mmset.html links to
     ../mpeuni/mmset.html so they will work in NF Explorer etc. */
/* 0.148 14-Aug-2017 nm mmcmds.c - hyperlink "Dummy variable(s)" */
/* 0.147 13-Aug-2017 nm mmcmds.c,h - add "Dummy variable x is distinct from all
   other variables." to proof web page */
/* 0.146 26-Jun-2017 nm mmwtex.c - fix handling of local labels in
     'show proof.../tex' (bug 2341 reported by Eric Parfitt) */
/* 0.145 16-Jun-2017 nm metamath.c mmpars.c - fix bug 1741 during
     MINIMIZE_WITH; mmpfas.c - make duplicate bug numbers unique; mmhlpa.c
     mmhlpb.c - adjust to prevent lcc compiler "Function too big for the
     optimizer"
   29-May-2017 nm mmwtex.c mmhlpa.c - take out extraneous  <HTML>...</HTML>
     markup tags in HTML output so w3c validator will pass */
/* 0.144 15-May-2017 nm metamath.c mmcmds.c - add "(Revised by..." tag for
     conversion of legacy .mm's if there is a 2nd date under the proof */
/* 0.143 14-May-2017 nm metamath.c mmdata.c,h mmcmdl.c mmcmds.c mmhlpb.c -
     added SET CONTRIBUTOR; for missing "(Contributed by..." use date
     below proof if it exists, otherwise use today's date, in order to update
     old .mm files.
   14-May-2017 Ari Ferrera - mmcmds.c - fix memory leaks in ERASE */
/* 0.142 12-May-2017 nm metamath.c mmdata.c,h mmcmds.c - added
     "#define DATE_BELOW_PROOF" in mmdata.h that if uncommented, will enable
     use of the (soon-to-be obsolete) date below the proof
   4-May-2017 Ari Ferrera - mmcmds.c metamath.c mmdata.c mmcmdl.c
     mminou.c mminou.h mmcmdl.h mmdata.h - fixed memory leaks and warnings
     found by valgrind.
   3-May-2017 nm - metamath.c mmdata.c,h mmcmds.c,h mmpars.c,h mmhlpb.c
     mmcmdl.c mmwtex.c - added xxChanged flags to statement structure so
     that any part of the source can be changed;  removed /CLEAN qualifier
     of WRITE SOURCE; automatically put "(Contributed by ?who?..." during
     SAVE NEW_PROOF or SAVE PROOF when it is missing; more VERIFY MARKUP
     checking. */
/* 0.141 2-May-2017 nm mmdata.c, metamath.c, mmcmds.c, mmhlpb.c - use
   getContrib() date for WRITE RECENT instead of date below proof.  This lets
   us list recent $a's as well as $p's.  Also, add caching to getContrib() for
   speedup. */
/* 0.140 1-May-2017 nm mmwtex.c, mmcmds.c, metamath.c - fix some LaTeX issues
   reported by Ari Ferrera */
/* 0.139 2-Jan-2017 nm metamath.c - print only one line for
     'save proof * /compressed/fast' */
/* 0.138 26-Dec-2016 nm mmwtex.c - remove extraneous </TD> causing w3c
   validation failure; put space after 1st x in "F/ x x = x";
   mmcmds.c - added checking for lines > 79 chars in VERIFY MARKUP;
   mmcmds.c, mmcmdl.c, metamath.c, mmhlpb.c, mmcmds.h - added /VERBOSE to
   VERIFY MARKUP */
/* 0.137 20-Dec-2016 nm mmcmds.c - check ax-XXX $a vs axXXX $p label convention
     in 'verify markup'
   18-Dec-2016 nm mmwtex.c, mmpars.c, mmdata.h - use true "header area"
     between successive $a/$p for getSectionHeadings()  mmcmds.c - add
     header comment checking
   13-Dec-2016 nm mmdata.c,h - enhanced compareDates() to treat empty string as
     older date.
   13-Dec-2016 nm metamath.c, mmcmds.c - moved mm* and Microsoft illegal file
     name label check to verifyMarkup() (the VERIFY MARKUP command) instead of
     checking on READ; added check of set.mm Version date to verifyMarkup().
   13-Dec-2016 nm mmwtex.c,h - don't treat bracketed description text with
     space as a bib label; add labelMatch parameter to writeBibliography() */
/* 0.136 10-Oct-2016 mminou.c - fix resource leak bug reported by David
   Binderman */
/* 0.135 11-Sep-2016, 14-Sep-2016 metamath.c, mmpfas.c,h, mmdata.c,h,
   mmpars.c,h mmcmds.c, mmcmdl.c, mmhlpb.c - added EXPAND command */
/* 0.134 16-Aug-2016 mmwtex.c - added breadcrumbs to theorem pages;
   metamath.c, mmcmdl.c, mmhlpb.c, mminou.c,.h - added /TIME to SAVE PROOF,
   SHOW STATEMENT.../[ALT}HTML, MINIMIZE_WITH */
/* 0.133 13-Aug-2016 mmwtex.c - improve mobile display with <head> tag
   mmpars.c - use updated Macintosh information */
/* 0.132 10-Jul-2016 metamath.c, mmcmdl.c, mmcmds.c,.h, mmdata.c,.h, mmhlpb.c,
   mmpfas.c - change "restricted" to "discouraged" to match set.mm markup
   tags; add SET DISCOURAGEMENT OFF|ON (default ON) to turn off blocking for
   convenience of advanced users
   6-Jul-2016 metamath.c - add "(void)" in front of "system(...)" to
   suppress -Wunused-result warning */
/* 0.131 10-Jun-2016 mminou.c - reverted change of 22-May-2016 because
   'minimize_with' depends on error message in string to prevent DV violations.
   Todo:  write a DV-checking routine for 'minimize_with', then revert
   the 22-May-2016 fix for bug 126 (which only occurs when writing web
   pages for .mm file with errors).
   9-Jun-2016 mmcmdl.c, metamath.c - added _EXIT_PA for use with
   scripts that will give an error message outside of MM-PA> rather
   than exiting metamath */
/* 0.130 25-May-2016 mmpars.c - workaround clang warning about j = j;
      mmvstr.c - workaround gcc -Wstrict-overflow warning */
/* 0.129 24-May-2016 mmdata.c - fix bug 1393 */
/* 0.128 22-May-2016 mminou.c - fixed error message going to html page
      instead of to screen, triggering bug 126. */
/* 0.127 10-May-2016 metamath.c, mmcmdl.c, mmhlpb.c - added /OVERRIDE to
      PROVE */
/* 0.126 3-May-2016 metamath.c, mmdata.h, mmdata.c, mmcmds.h, mmcmds.c,
      mmcmdl.c, mmhlpb.c, mmpars.c - added getMarkupFlag() in mmdata.c;
      Added /OVERRIDE added to ASSIGN, REPLACE, IMPROVE, MINIMIZE_WITH,
      SAVE NEW_PROOF;  PROVE gives warning about SAVE NEW_PROOF for locked
      proof.  Added SHOW RESTRICTED command.
   3-May-2016 m*.c - fix numerous conversion warnings provided by gcc 5.3.0 */
/* 0.125 10-Mar-2016 mmpars.c - fixed bug parsing /EXPLICIT/PACKED format
   8-Mar-2016 nm mmdata.c - added "#nnn" to SHOW STATEMENT etc. to reference
      statement number e.g. SHOW STATEMENT #58 shows a1i in set.mm.
   7-Mar-2016 nm mmwtex.c - added space between } and { in HTML output
   6-Mar-2016 nm mmpars.c - disabled wrapping of formula lines in
       WRITE SOURCE.../REWRAP
   2-Mar-2016 nm metamat.c, mmcmdl.c, mmhlpb.c - added /FAST to
       SAVE PROOF, SHOW PROOF */
/* 0.123 25-Jan-2016 nm mmpars.c, mmdata.h, mmdata.c, mmpfas.c, mmcmds.,
   metamath.c, mmcmdl.c, mmwtex.c - unlocked SHOW PROOF.../PACKED,
   added SHOW PROOF.../EXPLICIT */
/* 0.122 14-Jan-2016 nm metamath.c, mmcmds.c, mmwtex.c, mmwtex.h - surrounded
      math HTML output with "<SPAN [htmlFont]>...</SPAN>; added htmlcss and
      htmlfont $t commands
   10-Jan-2016 nm mmwtex.c - delete duplicate -4px style; metamath.c -
     add &nbsp; after char on mmascii.html
   3-Jan-2016 nm mmwtex.c - fix bug when doing SHOW STATEMENT * /ALT_HTML after
   VERIFY MARKUP */
/* 0.121 17-Nov-2015 nm metamath.c, mmcmdl.h, mmcmdl.c, mmcmds.h, mmcmds.c,
       mmwtex.h, mmwtex.c, mmdata.h, mmdata.c -
   1. Moved WRITE BIBLIOGRAPHY code from metamath.c to its own function in
      mmwtex.c; moved qsortStringCmp() from metamath.c to mmdata.c
   2. Added $t, comment markup, and bibliography checking to VERIFY MARKUP
   3. Added options to bug() bug-check interception to select aborting,
      stepping to next bug, or ignoring subsequent bugs
   4. SHOW STATEMENT can now use both /HTML and /ALT_HTML in same session
   5. Added /HTML, /ALT_HTML to WRITE THEOREM_LIST and
      WRITE RECENT_ADDITIONS */
/* 0.120 7-Nov-2015 nm metamath.c, mmcmdl.c, mmpars.c - add VERIFY MARKUP
   4-Nov-2015 nm metamath.c, mmcmds.c/h, mmdata.c/h - move getDescription,
       getSourceIndentation from mmcmds.c to mmdata.c.
       metamath.c, mmdata.c - add and call parseDate() instead of in-line
       code; add getContrib(), getProofDate(), buildDate(), compareDates(). */
/* 0.119 18-Oct-2015 nm mmwtex.c - add summary TOC to Theorem List; improve
       math symbol GIF image alignment
   2-Oct-2015 nm metamath.c, mmpfas.c, mmwtex.c - fix miscellaneous small
       bugs or quirks */
/* 0.118 18-Jul-2015 nm metamath.c, mmcmds.h, mmcmds.c, mmcmdl.c, mmhlpb.h,
   mmhlpb.c - added /TO qualifier to SHOW TRACE_BACK.  See
   HELP SHOW TRACE_BACK. */
/* 0.117 30-May-2015
   1. nm mmwtex.c - move <A NAME... tag to math symbol cell in proof pages so
      hyperlink will jump to top of cell (reported by Alan Sare)
   2. daw mmpfas.c - add INLINE speedup if compiler permits
   3. nm metamath.c, mminou.c, mmwtex.c, mmpfas.c - fix clang -Wall warnings
      (reported by David A. Wheeler) */
/* 0.116 9-May-2015 nm mmwtex.c - adjust paragraph break in 'write th';
   Statement List renamed Theorem List;  prevent space in after paragraph
   in Theorem List; remove stray "(";  put header and header comment
   in same table cell; fix <TITLE> of Theorem List pages */
/* 0.115 8-May-2015 nm mmwtex.c - added section header comments to
       WRITE THEOREM_LIST and broke out Table of Contents page
   24-Apr-2015 nm metamath.c - add # bytes to end of "---Clip out the proof";
       reverted to no blank lines there (see 0.113 item 3) */
/* 0.114 22-Apr-2015 nm mmcmds.c - put [-1], [-2],... offsets on 'show
   new_proof/unknown' */
/* 0.113 19-Apr-2015 so, nm metamath.c, mmdata.c
   1. SHOW LABEL % will show statements with changed proofs
   2. SHOW LABEL = will show the statement being proved in MM-PA
   3. Added blank lines before, after "---------Clip out the proof" proof
   4. Generate date only if the proof is complete */
/* 0.112 15-Apr-2015 nm metamath.c - fix bug 1121 (reported by S. O'Rear);
   mwtex.c - add "img { margin-bottom: -4px }" to CSS to align symbol GIFs;
   mwtex.c - remove some hard coding for set.mm, for use with new nf.mm;
   metamath.c - fix comment parsing in WRITE BIBLIOGRAPHY to ignore
   math symbols  */
/* 0.111 22-Nov-2014 nm metamath.c, mmcmds.c, mmcmdl.c, mmhlpb.c - added
   /NO_NEW_AXIOMS_FROM qualifier to MINIMIZE_WITH; see HELP MINIMIZE_WITH.
   21-Nov-2014 Stepan O'Rear mmdata.c, mmhlpb.c - added ~ label range specifier
   to wildcards; see HELP SEARCH */
/* 0.110 2-Nov-2014 nm mmcmds.c - fixed bug 1114 (reported by Stefan O'Rear);
   metamath.c, mmhlpb.c - added "SHOW STATEMENT =" to show the statement
   being proved in MM-PA */
/* 0.109 20-Aug-2014 nm mmwtex.c - fix corrupted HTML caused by misinterpreting
   math symbols as comment markup (math symbols with _ [ ] or ~).  Also,
   allow https:// as well as http:// in ~ label markup.
   11-Jul-2014 wl mmdata.c - fix obscure crash in debugging mode db9 */
/* 0.108 25-Jun-2014 nm
   (1) metamath.c, mmcmdl.c, mmhlpb.c - MINIMIZE_WITH now checks the size
   of the compressed proof, prevents $d violations, and tries forward and
   reverse statment scanning order; /NO_DISTINCT, /BRIEF, /REVERSE
   qualifiers were removed.
   (2) mminou.c - prevent hard breaks (in the middle of a word) in too-long
   lines (e.g. long URLs) in WRITE SOURCE .../REWRAP; just overflow the
   screen width instead.
   (3) mmpfas.c - fixed memory leak in replaceStatement()
   (4) mmpfas.c - suppress inf. growth with MINIMIZE_WITH idi/ALLOW_GROWTH */
/* 0.107 21-Jun-2014 nm metamath.c, mmcmdl.c, mmhlpb.c - added /SIZE qualifier
   to SHOW PROOF; added SHOW ELAPSED_TIME; mmwtex.c - reformatted WRITE
   THEOREM_LIST output; now "$(", newline, "######" starts a "part" */
/* 0.106 30-Mar-2014 nm mmwtex.c - fix bug introduced by 0.105 that disabled
   hyperlinks on literature refs in HTML comment.  metamath.c - improve
   messages */
/* 0.105 15-Feb-2014 nm mmwtex.c - prevented illegal LaTeX output for certain
   special characters in comments. */
/* 0.104 14-Feb-2014 nm mmwtex.c - fixed bug 2312, mmcmds.c - enhanced ASSIGN
   error message. */
/* 0.103 4-Jan-2014 nm mmcmds.c,h - added "Allowed substitution hints" below
   the "Distinct variable groups" list on generated web pages
   mmwtex.c - added "*" to indicate DV's occur in Statement List entries. */
/* 0.102 2-Jan-2014 nm mminou.c - made compressed proof line wrapping more
   uniform at start of compressed part of proof */
/* 0.101 27-Dec-2013 nm mmdata.h,c, mminou.c, mmcmdl.c, mmhlpb.c, mmvstr.c -
   Improved storage efficiency of /COMPRESSED proofs (but with 20% slower run
   time); added /OLD_COMPRESSION to specify old algorithm; removed end-of-line
   space after label list in old algorithm; fixed linput() bug */
/* 0.100 30-Nov-2013 nm mmpfas.c - reversed statement scan order in
   proveFloating(), to speed up SHOW STATEMENT df-* /HTML; metamath.c - remove
   the unknown date place holder in SAVE NEW_PROOF; Wolf Lammen mmvstr.c -
   some cleanup */
/* 0.07.99 1-Nov-2013 nm metamath.c, mmpfas.h,c, mmcmdl.h,c, mmhlpa.c,
   mmhlpb.c - added UNDO, REDO, SET UNDO commands (see HELP UNDO) */
/* 0.07.98 30-Oct-2013 Wolf Lammen mmvstr.c,h, mmiou.c, mmpars.c,
   mmdata.c  - improve code style and program structure */
/* 0.07.97 20-Oct-2013 Wolf Lammen mmvstr.c,h, metamath.c - improved linput();
   nm mmcmds.c, mmdata.c - tolerate bad proofs in SHOW TRACE_BACK etc. */
/* 0.07.96 20-Sep-2013 Wolf Lammen mmvstr.c - revised cat();
   nm mmwtex.c, mminou.c - change a print2 to printLongLine to fix bug 1150 */
/* 0.07.95 18-Sep-2013 Wolf Lammen mmvstr.c - optimized cat();
   nm metamath.c, mmcmds.c, mmdata.c, mmpars.c, mmpfas.c, mmvstr.c,
   mmwtex.c - suppress some clang warnings */
/* 0.07.94 28-Aug-2013 Alexey Merkulov mmcmds.c, mmpars.c - fixed several
   memory leaks found by valgrind --leak-check=full --show-possibly-lost=no */
/* 0.07.93 8-Jul-2013 Wolf Lammen mmvstr.c - simplified let() function;
   also many minor changes in m*.c and m*.h to assist future refactoring */
/* 0.07.92 28-Jun-2013 nm metamath.c mmcmds.c,h mmcmdl.c mmhlpb.c- added
   /NO_REPEATED_STEPS for /LEMMON mode of SHOW PROOF, SHOW NEW_PROOF.
   This reverts the /LEMMON mode default display change of 31-Jan-2010
   and invokes it when desired via /NO_REPEATED_STEPS. */
/* 0.07.91 20-May-2013 nm metamath.c mmpfas.c,h mmcmds.c,h mmcmdl.c
   mmhlpb.c- added /FORBID qualifier to MINIMIZE_WITH */
/* 0.07.90 19-May-2013 nm metamath.c mmcmds.c mmcmdl.c mmhlpb.c - added /MATCH
   qualifier to SHOW TRACE_BACK */
/* 0.07.88 18-Nov-2012 nm mmcmds.c - fixed bug 243 */
/* 0.07.87 17-Nov-2012 nm mmwtex.c - fixed formatting problem when label
   markup ends a comment in SHOW PROOF ... /HTML */
/* 0.07.86 27-Oct-2012 nm mmcmds.c, mmwtex.c, mmwtex.h - fixed ERASE bug
   caused by imperfect re-initialization; reported by Wolf Lammen */
/* 0.07.85 10-Oct-2012 nm metamath.c, mmcmdl.c, mmwtex.c, mmwtex.h, mmhlpb.c -
   added /SHOW_LEMMAS to WRITE THEOREM_LIST to bypass lemma math suppression */
/* 0.07.84 9-Oct-2012 nm mmcmds.c - fixed bug in getStatementNum() */
/* 0.07.83 19-Sep-2012 nm mmwtex.c - fixed bug reported by Wolf Lammen */
/* 0.07.82 16-Sep-2012 nm metamath.c, mmpfas.c - fixed REPLACE infinite loop;
   improved REPLACE message for shared dummy variables */
/* 0.07.81 14-Sep-2012 nm metamath.c, mmcmds.c, mmcmds.h, mmcmdl.c, mmhlpb.c
   - added FIRST, LAST, +nn, -nn where missing from ASSIGN, REPLACE,
   IMPROVE, LET STEP.  Wildcards are allowed for PROVE, ASSIGN, REPLACE
   labels provided there is a unique match. */
/* 0.07.80 4-Sep-2012 nm metamath.c, mmpfas.c, mmpfas.h, mmcmdl.c, mmhlpb.c
   - added / 1, / 2, / 3, / SUBPROOFS to IMPROVE to specify search level */
/* 0.07.79 31-Aug-2012 nm m*.c - clean up some gcc warnings */
/* 0.07.78 28-Aug-2012 nm mmpfas.c - fix bug in 0.07.77. */
/* 0.07.77 25-Aug-2012 nm metamath.c, mmpfas.c - Enhanced IMPROVE algorithm to
   allow non-shared dummy variables in unknown steps */
/* 0.07.76 22-Aug-2012 nm metamath.c, mmpfas.c, mmcmdl.c, mmhlpb.c -
   Enhanced IMPROVE algorithm to also try REPLACE algorithm */
/* 0.07.75 14-Aug-2012 nm metamath.c - MINIMIZE_WITH now checks current
   mathbox (but not other mathboxes) even if /INCLUDE_MATHBOXES is omitted */
/* 0.07.74 18-Mar-2012 nm mmwtex.c, mmcmds.c, metamath.c - improved texToken()
   error message */
/* 0.07.73 26-Dec-2011 nm mmwtex.c, mmpars.c - added <HTML>...</HTML> in
   comments for passing through raw HTML code into HTML files generated with
   SHOw STATEMENT xxx / HTML */
/* 0.07.72 25-Dec-2011 nm (obsolete) */
/* 0.07.71 10-Nov-2011 nm metamath.c, mmcmdl.c - added /REV to MINIMIZE_WITH */
/* 0.07.70 6-Aug-2011 nm mmwtex.c - fix handling of double quotes inside
   of htmldef strings to match spec in Metamath book Appendix A p. 156 */
/* 0.07.69 9-Jul-2011 nm mmpars.c, mmvstr.c - Untab file in WRITE SOURCE
   ... /REWRAP */
/* 0.07.68 3-Jul-2011 nm metamath.c, mminou.h, mminou.c - Nested SUBMIT calls
   (SUBMIT calls inside of a SUBMIT command file) are now allowed.
   Also, mmdata.c - fix memory leak. */
/* 0.07.67 2-Jul-2011 nm metamath.c, mmcmdl.c, mmhlpa.c - Added TAG command
   to TOOLS.  See HELP TAG under TOOLS.  (The old special-purpose TAG command
   was renamed to UPDATE.) */
/* 0.07.66 1-Jul-2011 nm metamath.c, mmcmds.c, mmpars.c, mmpars.h - Added code
   to strip spurious "$( [?] $)" in WRITE SOURCE ... /CLEAN output */
/* 0.07.65 30-Jun-2011 nm mmwtex.c - Prevent processing [...] bibliography
   brackets inside of `...` math strings in comments. */
/* 0.07.64 28-Jun-2011 nm metamath.c, mmcmdl.c - Added /INCLUDE_MATHBOXES
   qualifier to MINIMIZE_WITH; without it, MINIMIZE_WITH * will skip
   checking user mathboxes. */
/* 0.07.63 26-Jun-2011 nm mmwtex.c - check that .gifs exist for htmldefs */
/* 0.07.62 18-Jun-2011 nm mmpars.c - fixed bug where redeclaration of active
   $v was not detected */
/* 0.07.61 12-Jun-2011 nm mmpfas.c, mmcmds.c, metamath.c, mmhlpb.c - added
   /FORMAT and /REWRAP qualifiers to WRITE SOURCE to format according to set.mm
   conventions - set HELP WRITE SOURCE */
/* 0.07.60 7-Jun-2011 nm mmpfas.c - fixed bug 1805 which occurred when doing
   MINIMIZE_WITH weq/ALLOW_GROWTH after DELETE DELETE FLOATING_HYPOTHESES */
/* 0.07.59 11-Dec-2010 nm mmpfas.c - increased default SET SEARCH_LIMIT from
   10000 to 25000 to accomodate df-plig web page in set.mm */
/* 0.07.58 9-Dec-2010 nm mmpars.c - detect if same symbol is used with both
   $c and $v, in order to conform with Metamath spec */
/* 0.07.57 19-Oct-2010 nm mmpars.c - fix bug causing incorrect line count
   for error messages when non-ASCII character was found; mminou.h -
   increase SET WIDTH maximum from 999 to 9999 */
/* 0.07.56 27-Sep-2010 nm mmpars.c, mmpfas.c - check for $a's with
   one token e.g. "$a wff $."; if found, turn SET EMPTY_SUBSTITUTION ON
   automatically.  (Suggested by Mel O'Cat; patent pending.) */
/* 0.07.55 26-Sep-2010 nm mmunif.c, mmcmds.c, mmunif.h - check for mismatched
   brackets in all $a's, so that if there are any, the bracket matching
   algorithm (for fewer ambiguous unifications) in mmunif.c will be turned
   off. */
/* 0.07.54 25-Sep-2010 nm mmpars.c - added $f checking to conform to the
   current Metamath spec, so footnote 2 on p. 92 of Metamath book is
   no longer applicable. */
/* 0.07.53 24-Sep-2010 nm mmveri.c - fixed bug(2106), reported by Michal
   Burger */
/* 0.07.52 14-Sep-2010 nm metamath.c, mmwtex.h, mmwtex.c, mmcmds.c,
   mmcmdl.c, mmhlpb.c - rewrote the LaTeX output for easier hand-editing
   and embedding in LaTeX documents.  The old LaTeX output is still
   available with /OLD_TEX on OPEN TEX, SHOW STATEMENT, and SHOW PROOF,
   but it is obsolete and will be deleted eventually if no one objects.  The
   new /TEX output also replaces the old /SIMPLE_TEX, which was removed. */
/* 0.07.51 9-Sep-2010 Stefan Allen mmwtex.c - put hyperlinks on hypothesis
   label references in SHOW STATEMENT * /HTML, ALT_HTML output */
/* 0.07.50 21-Feb-2010 nm mminou.c - "./metamath < empty", where "empty" is a
   0-byte file, now exits metamath instead of producing an infinite loop.
   Also, ^D now exits metamath.  Reported by Cai Yufei */
/* 0.07.49 31-Jan-2010 nm mmcmds.c - Lemmon-style proofs (SHOW PROOF xxx
   /LEMON/RENUMBER) no longer have steps with dummy labels; instead, steps
   are now the same as in HTML page proofs.  (There is a line to comment
   out if you want to revert to old behavior.) */
/* 0.07.48 11-Sep-2009 nm mmpars.c, mm, mmvstr.c, mmdata.c - Added detection of
   non-whitespace around keywords (mmpars.c); small changes to eliminate
   warnings in gcc 3.4.4 (mmvstr.c, mmdata.c) */
/* 0.07.47 2-Aug-2009 nm mmwtex.c, mmwtex.h - added user name to mathbox
   pages */
/* 0.07.46 24-Jul-2009 nm metamath.c, mmwtex.c - changed name of sandbox
   to "mathbox" */
/* 0.07.45 15-Jun-2009 nm metamath.c, mmhlpb.c - put "!" before each line of
   SET ECHO ON output to make them easy to identity for creating scripts */
/* 0.07.44 12-May-2009 Stefan Allan, nm metamath.c, mmcmdl.c, mmwtex.c -
   added SHOW STATEMENT / MNEMONICS - see HELP SHOW STATEMENT */
/* 0.07.43 29-Aug-2008 nm mmwtex.c - workaround for Unicode huge font bug in
   FireFox 3 */
/* 0.07.42 8-Aug-2008 nm metamath.c - added sandbox, Hilbert Space colors to
   Definition List */
/* 0.07.41 29-Jul-2008 nm metamath.c, mmwtex.h, mmwtex.c - Added handling of
   sandbox section of Metamath Proof Explorer web pages */
/* 0.07.40 6-Jul-2008 nm metamath.c, mmcmdl.c, mmhlpa.c, mmhlpb.c - Added
   / NO_VERSIONING qualifier for SHOW STATEMENT, so website can be regenerated
   in place with less temporary space required.  Also, the wildcard trigger
   for mmdefinitions.html, etc. is more flexible (see HELP HTML). */
/* 0.07.39 21-May-2008 nm metamath.c, mmhlpb.c - Added wildcard handling to
   statement label in SHOW TRACE_BACK.  All wildcards now allow
   comma-separated lists [i.e. matchesList() instead of matches()] */
/* 0.07.38 26-Apr-2008 nm metamath.c, mmdata.h, mmdata.c, mmvstr.c, mmhlpb.c -
   Enhanced / EXCEPT qualifier for MINIMIZE_WITH to handle comma-separated
   wildcard list. */
/* 0.07.37 14-Apr-2008 nm metamath.c, mmcmdl.c, mmhlpb.c - Added / JOIN
   qualifier to SEARCH. */
/* 0.07.36 7-Jan-2008 nm metamath.c, mmcmdl.c, mmhlpb.c - Added wildcard
   handling for labels in SHOW USAGE. */
/* 0.07.35 2-Jan-2008 nm mmcmdl.c, metamath.c, mmhlpb.c - Changed keywords
   COMPACT to PACKED and COLUMN to START_COLUMN so that SAVE/SHOW proof can use
   C to abbreviate COMPRESSED.  (PACKED format is supported but "unofficial,"
   used mainly for debugging purposes, and is not listed in HELP SAVE
   PROOF.) */
/* 0.07.34 19-Nov-2007 nm mmwtex.c, mminou.c - Added tooltips to proof step
   hyperlinks in SHOW STATEMENT.../HTML,ALT_HTML output (suggested by Reinder
   Verlinde) */
/* 0.07.33 19-Jul-2007 nm mminou.c, mmvstr.c, mmdata.c, mmword.c - added fflush
   after each printf() call for proper behavior inside emacs (suggested by
   Frederic Line) */
/* 0.07.32 29-Apr-2007 nm mminou.c - fSafeOpen now stops at gap; e.g. if ~2
   doesn't exist, ~1 will be renamed to ~2, but any ~3, etc. are not touched */
/* 0.07.31 5-Apr-2007 nm mmwtex.c - Don't make "_" in hyperlink a subscript */
/* 0.07.30 8-Feb-2007 nm mmcmds.c, mmwtex.c Added HTML statement number info to
   SHOW STATEMENT.../FULL; friendlier "Contents+1" link in mmtheorems*.html */
/* 0.07.29 6-Feb-2007 Jason Orendorff mmpfas.c - Patch to eliminate the
   duplicate "Exceeded trial limit at step n" messages */
/* 0.07.28 22-Dec-2006 nm mmhlpb.c - Added info on quotes to HELP LET */
/* 0.07.27 23-Oct-2006 nm metamath.c, mminou.c, mmhlpa.c, mmhlpb.c - Added
   / SILENT qualifier to SUBMIT command */
/* 0.07.26 12-Oct-2006 nm mminou.c - Fixed bug when SUBMIT file was missing
   a new-line at end of file (reported by Marnix Klooster) */
/* 0.07.25 10-Oct-2006 nm metamath.c - Fixed bug invoking TOOLS as a ./metamath
   command-line argument */
/* 0.07.24 25-Sep-2006 nm mmcmdl.c Fixed bug in
   SHOW NEW_PROOF/START_COLUMN nn/LEM */
/* 0.07.23 31-Aug-2006 nm mmwtex.c - Added Home and Contents links to bottom of
   WRITE THEOREM_LIST pages */
/* 0.07.22 26-Aug-2006 nm metamath.c, mmcmdl.c, mmhlpb.c - Changed 'IMPROVE
   STEP <step>' to 'IMPROVE <step>' for user convenience and to be consistent
   with ASSIGN <step> */
/* 0.07.21 20-Aug-2006 nm mmwtex.c - Revised small colored numbers so that all
   colors have the same grayscale brightness.. */
/* 0.07.20 19-Aug-2006 nm mmpars.c - Made the error "Required hypotheses may
   not be explicitly declared" in a compressed proof non-severe, so that we
   can still SAVE the proof to reformat and recover it. */
/* 0.07.19 11-Aug-06 nm mmcmds.c - "Distinct variable group(s)" is now
   "group" or "groups" as appropriate. */
/* 0.07.18 31-Jul-06 nm mmwtex.c - added table to contents to p.1 of output of
   WRITE THEOREM_LIST command. */
/* 0.07.17 4-Jun-06 nm mmpars.c - do not allow labels to match math symbols
   (new spec proposed by O'Cat).   mmwtex.c - made theorem name 1st in title,
   for readability in Firefox tabs. */
/* 0.07.16 16-Apr-06 nm metamath.c, mmcmdl.c, mmpfas.c, mmhlpb.c - allow step
   to be negative (relative to end of proof) for ASSIGN, UNIFY, and LET STEP
   (see their HELPs).  Added INITIALIZE USER to delete LET STEP assignments
   (see HELP INITIALIZE).  Fixed bug in LET STEP (mmpfas.c). */
/* 0.07.15 10-Apr-06 nm metamath.c, mmvstr.c - change dates from 2-digit to
   4-digit year; make compatible with older 2-digit year. */
/* 0.07.14 21-Mar-06 nm mmpars.c - fix bug 1722 when compressed proof has
   "Z" at beginning of proof instead of after a proof step. */
/* 0.07.13 3-Feb-06 nm mmpfas.c - minor improvement to MINIMIZE_WITH */
/* 0.07.12 30-Jan-06 nm metamath.c, mmcmds.c, mmdata.c, mmdata.h, mmhlpa.c,
   mmhlpb.c - added "?" wildcard to match single character. See HELP SEARCH. */
/* 0.07.11 7-Jan-06 nm metamath.c, mmcmdl.c, mmhlpb.c - added EXCEPT qualifier
   to MINIMIZE_WITH */
/* 0.07.10 28-Dec-05 nm metamath.c, mmcmds.c - cosmetic tweaks */
/* 0.07.10 11-Dec-05 nm metamath.c, mmcmdl.c, mmhlpb.c - added ASSIGN FIRST
   and IMPROVE FIRST commands.  Also enhanced READ error message */
/* 0.07.9 1-Dec-05 nm mmvstr.c - added comment on how to make portable */
/* 0.07.9 18-Nov-05 nm metamath.c, mminou.c, mminou.h, mmcmdl.c, mmhlpb.c -
   added SET HEIGHT command; changed SET SCREEN_WIDTH to SET WIDTH; changed
   SET HENTY_FILTER to SET JEREMY_HENTY_FILTER (to make H for HEIGHT
   unambiguous); added HELP for SET JEREMY_HENTY_FILTER */
/* 0.07.8 15-Nov-05 nm mmunif.c - now detects wrong order in bracket matching
   heuristic to further reduce ambiguous unifications in Proof Assistant */
/* 0.07.7 12-Nov-05 nm mmunif.c - add "[","]" and "[_","]_" bracket matching
   heuristic to reduce ambiguous unifications in Proof Assistant.
   mmwtex.c - added heuristic for HTML spacing after "sum_" token. */
/* 0.07.6 15-Oct-05 nm mmcmds.c,mmpars.c - fixed compressed proof algorithm
   to match spec in book (with new algorithm due to Marnix Klooster).
   Users are warned to convert proofs when the old compression is found. */
/* 0.07.5 6-Oct-05 nm mmpars.c - fixed bug that reset "severe error in
   proof" flag when a proof with severe error also had unknown steps */
/* 0.07.4 1-Oct-05 nm mmcmds.c - ignored bug 235, which could happen for
   non-standard logics */
/* 0.07.3 17-Sep-05 nm mmpars.c - reinstated duplicate local label checking to
   conform to strict spec */
/* 0.07.2 19-Aug-05 nm mmwtex.c - suppressed math content for lemmas in
   WRITE THEOREMS output */
/* 0.07.1 28-Jul-05 nm Added SIMPLE_TEX qualifier to SHOW STATEMENT */
/* 0.07:  Official 0.07 22-Jun-05 corresponding to Metamath book */
/* 0.07x:  Fixed to work with AMD64 with 64-bit longs by
   Waldek Hebisch; deleted unused stuff in mmdata.c */
/* 0.07w:  .mm date format like "$( [7-Sep-04] $)" is now
   generated and permitted (old one is tolerated too for compatibility) */
/* Metamath Proof Verifier - main program */
/* See the book "Metamath" for description of Metamath and run instructions */

/* The overall functionality of the modules is as follows:
    metamath.c - Contains main(); executes or calls commands
    mmcmdl.c - Command line interpreter
    mmcmds.c - Extends metamath.c command() to execute SHOW and other
               commands; added after command() became too bloated (still is:)
    mmdata.c - Defines global data structures and manipulates arrays
               with functions similar to BASIC string functions;
               memory management; converts between proof formats
    mmhlpa.c - The help file, part 1.
    mmhlpb.c - The help file, part 2.
    mminou.c - Basic input and output interface
    mmmaci.c - THINK C Macintosh interface (probably obsolete now)
    mmpars.c - Parses the source file
    mmpfas.c - Proof Assistant
    mmunif.c - Unification algorithm for Proof Assistant
    mmutil.c - Miscellaneous I/O utilities for non-ANSI compilers (has become
               obsolete and is now an empty shell)
    mmveri.c - Proof verifier for source file
    mmvstr.c - BASIC-like string functions
    mmwtex.c - LaTeX/HTML source generation
    mmword.c - File revision utility (for TOOLS> UPDATE) (not generally useful)
*/

/*****************************************************************************/
/* ------------- Compilation Instructions ---------------------------------- */
/*****************************************************************************/

/* These are the instructions for the gcc compiler (standard in Linux and
   Cygwin for Windows).
   1. Make sure each .c file above is present in the compilation directory and
      that each .c file (except metamath.c) has its corresponding .h file
      present.
   2. In the directory where these files are present, type:
         gcc metamath.c m*.c -o metamath
   3. For better speed and error checking, use these gcc options:
         gcc m*.c -o metamath -O3 -funroll-loops -finline-functions \
             -fomit-frame-pointer -Wall -ansi -pedantic
   4. The Windows version in the download was compiled with LCC-Win32:
         lc -O m*.c -o metamath.exe
*/


/*****************************************************************************/


/*----------------------------------------------------------------------*/


#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
/* #include <time.h> */ /* 21-Jun-2014 nm For ELAPSED_TIME */
#ifdef THINK_C
#include <console.h>
#endif
#include "mmutil.h"
#include "mmvstr.h"
#include "mmdata.h"
#include "mmcmdl.h"
#include "mmcmds.h"
#include "mmhlpa.h"
#include "mmhlpb.h"
#include "mminou.h"
#include "mmpars.h"
#include "mmveri.h"
#include "mmpfas.h"
#include "mmunif.h"
#include "mmword.h"
#include "mmwtex.h"
#ifdef THINK_C
#include "mmmaci.h"
#endif

void command(int argc, char *argv[]);

int main(int argc, char *argv[])
{

/* argc is the number of arguments; argv points to an array containing them */
#ifdef THINK_C
/* Set console attributes */
console_options.pause_atexit = 0; /* No pause at exit */
console_options.title = (unsigned char*)"\pMetamath";
#endif

#ifdef THINK_C
  /* The standard stream triggers the console package to initialize the
     Macintosh Toolbox managers and use the console interface.  cshow must
     be called before using our own window to prevent crashing (THINK C
     Standard Library Reference p. 43). */
  cshow(stdout);
  /* Initialize MacIntosh interface */
  /*ToolBoxInit(); */ /* cshow did this automatically */
  /* Display opening window */
  /*
  WindowInit();
  DrawMyPicture();
  */
  /* Wait for mouse click or key */
  /*while (!Button());*/
#endif


  /****** If listMode is set to 1 here, the startup will be Text Tools
          utilities, and Metamath will be disabled ***************************/
  /* (Historically, this mode was used for the creation of a stand-alone
     "TOOLS>" utility for people not interested in Metamath.  This utility
     was named "LIST.EXE", "tools.exe", and "tools" on VMS, DOS, and Unix
     platforms respectively.  The UPDATE command of TOOLS (mmword.c) was
     custom-written in accordance with the version control requirements of a
     company that used it; it documents the differences between two versions
     of a program as C-style comments embedded in the newer version.) */
  listMode = 0; /* Force Metamath mode as startup */


  toolsMode = listMode;

  if (!listMode) {
    /*print2("Metamath - Version %s\n", MVERSION);*/
    print2("Metamath - Version %s%s", MVERSION, space(27 - (long)strlen(MVERSION)));
  }
  /* if (argc < 2) */ print2("Type HELP for help, EXIT to exit.\n");

  /* Allocate big arrays */
  initBigArrays();

  /* 14-May-2017 nm */
  /* Set the default contributor */
  let(&contributorName, DEFAULT_CONTRIBUTOR);

  /* Process a command line until EXIT */
  command(argc, argv);

  /* Close logging command file */
  if (listMode && listFile_fp != NULL) {
    fclose(listFile_fp);
  }

  return 0;

}




void command(int argc, char *argv[])
{
  /* Command line user interface -- this is an infinite loop; it fetches and
     processes a command; returns only if the command is 'EXIT' or 'QUIT' and
     never returns otherwise. */
  long argsProcessed = 0;  /* Number of argv initial command-line
                                     arguments processed so far */

  long /*c,*/ i, j, k, m, l, n, p, q, r, s /*,tokenNum*/;
  long stmt, step;
  int subType = 0;
#define SYNTAX 4
  vstring str1 = "", str2 = "", str3 = "", str4 = "", str5= "";
  nmbrString *nmbrTmpPtr; /* Pointer only; not allocated directly */
  nmbrString *nmbrTmp = NULL_NMBRSTRING;
  nmbrString *nmbrSaveProof = NULL_NMBRSTRING;
  /*pntrString *pntrTmpPtr;*/ /* Pointer only; not allocated directly */
  pntrString *pntrTmp = NULL_PNTRSTRING;
  pntrString *expandedProof = NULL_PNTRSTRING;
  flag tmpFlag;

  /* 1-Nov-2013 nm proofSavedFlag tells us there was at least one
     SAVE NEW_PROOF during the MM-PA session while the UNDO stack wasn't
     empty, meaning that "UNDO stack empty" is no longer a reliable indication
     that the proof wasn't changed.  It is cleared upon entering MM-PA, and
     set by SAVE NEW_PROOF. */
  flag proofSavedFlag = 0;

  /* Variables for SHOW PROOF */
  flag pipFlag; /* Proof-in-progress flag */
  long outStatement; /* Statement for SHOW PROOF or SHOW NEW_PROOF */
  flag explicitTargets; /* For SAVE PROOF /EXPLICIT */
  long startStep; long endStep;
  /* long startIndent; */
  long endIndent; /* Also for SHOW TRACE_BACK */
  flag essentialFlag; /* Also for SHOW TRACE_BACK */
  flag renumberFlag; /* Flag to use essential step numbering */
  flag unknownFlag;
  flag notUnifiedFlag;
  flag reverseFlag;
  long detailStep;
  flag noIndentFlag; /* Flag to use non-indented display */
  long splitColumn; /* Column at which formula starts in nonindented display */
  flag skipRepeatedSteps; /* NO_REPEATED_STEPS qualifier */ /* 28-Jun-2013 nm */
  flag texFlag; /* Flag for TeX */
  flag saveFlag; /* Flag to save in source */
  flag fastFlag; /* Flag for SAVE PROOF.../FAST */ /* 2-Jan-2017 nm */
  long indentation; /* Number of spaces to indent proof */
  vstring labelMatch = ""; /* SHOW PROOF <label> argument */

  flag axiomFlag; /* For SHOW TRACE_BACK */
  flag treeFlag; /* For SHOW TRACE_BACK */ /* 19-May-2013 nm */
  flag countStepsFlag; /* For SHOW TRACE_BACK */ /* 19-May-2013 nm */
  flag matchFlag; /* For SHOW TRACE_BACK */ /* 19-May-2013 nm */
  vstring matchList = "";  /* For SHOW TRACE_BACK */ /* 19-May-2013 nm */
  vstring traceToList = ""; /* For SHOW TRACE_BACK */ /* 18-Jul-2015 nm */
  flag recursiveFlag; /* For SHOW USAGE */
  long fromLine, toLine; /* For TYPE, SEARCH */
  flag joinFlag; /* For SEARCH */
  long searchWindow; /* For SEARCH */
  FILE *type_fp; /* For TYPE, SEARCH */
  long maxEssential; /* For MATCH */
  nmbrString *essentialFlags = NULL_NMBRSTRING;
                                            /* For ASSIGN/IMPROVE FIRST/LAST */
  long improveDepth; /* For IMPROVE */
  flag searchAlg; /* For IMPROVE */ /* 22-Aug-2012 nm */
  flag searchUnkSubproofs;  /* For IMPROVE */ /* 4-Sep-2012 nm */
  flag dummyVarIsoFlag; /* For IMPROVE */ /* 25-Aug-2012 nm */
  long improveAllIter; /* For IMPROVE ALL */ /* 25-Aug-2012 nm */
  flag proofStepUnk; /* For IMPROVE ALL */ /* 25-Aug-2012 nm */

  flag texHeaderFlag; /* For OPEN TEX, CLOSE TEX */
  flag commentOnlyFlag; /* For SHOW STATEMENT */
  flag briefFlag; /* For SHOW STATEMENT */
  flag linearFlag; /* For SHOW LABELS */
  vstring bgcolor = ""; /* For SHOW STATEMENT definition list */
                                                            /* 8-Aug-2008 nm */

  flag verboseMode, allowGrowthFlag /*, noDistinctFlag*/; /* For MINIMIZE_WITH */
  long prntStatus; /* For MINIMIZE_WITH */
  flag hasWildCard; /* For MINIMIZE_WITH */
  long exceptPos; /* For MINIMIZE_WITH */
  flag mathboxFlag; /* For MINIMIZE_WITH */ /* 28-Jun-2011 nm */
  long thisMathboxStmt; /* For MINIMIZE_WITH */ /* 14-Aug-2012 nm */
  flag forwFlag; /* For MINIMIZE_WITH */ /* 11-Nov-2011 nm */
  long forbidMatchPos;  /* For MINIMIZE_WITH */ /* 20-May-2013 nm */
  vstring forbidMatchList = "";  /* For MINIMIZE_WITH */ /* 20-May-2013 nm */
  long noNewAxiomsMatchPos;  /* For NO_NEW_AXIOMS_FROM */ /* 22-Nov-2014 nm */
  vstring noNewAxiomsMatchList = "";  /* For NO_NEW_AXIOMS_FROM */ /* 22-Nov-2014 */
  vstring traceProofFlags = ""; /* For NO_NEW_AXIOMS_FROM */ /* 22-Nov-2014 nm */
  vstring traceTrialFlags = ""; /* For NO_NEW_AXIOMS_FROM */ /* 22-Nov-2014 nm */
  flag overrideFlag; /* For discouraged statement /OVERRIDE */ /* 3-May-2016 nm */

  struct pip_struct saveProofForReverting = {
       NULL_NMBRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING };
                                   /* For MINIMIZE_WITH */ /* 20-May-2013 nm */
  long origCompressedLength; /* For MINIMIZE_WITH */ /* 25-Jun-2014 nm */
  long oldCompressedLength = 0; /* For MINIMIZE_WITH */ /* 25-Jun-2014 nm */
  long newCompressedLength = 0; /* For MINIMIZE_WITH */ /* 25-Jun-2014 nm */
  long forwardCompressedLength = 0; /* For MINIMIZE_WITH */ /* 25-Jun-2014 nm */
  long forwardLength = 0; /* For MINIMIZE_WITH */ /* 25-Jun-2014 nm */
  vstring saveZappedProofSectionPtr; /* Pointer only */ /* For MINIMIZE_WITH */
  long saveZappedProofSectionLen; /* For MINIMIZE_WITH */ /* 25-Jun-2014 nm */
  flag saveZappedProofSectionChanged; /* For MINIMIZE_WITH */ /* 16-Jun-2017 nm */

  struct pip_struct saveOrigProof = {
       NULL_NMBRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING };
                                   /* For MINIMIZE_WITH */ /* 25-Jun-2014 nm */
  struct pip_struct save1stPassProof = {
       NULL_NMBRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING, NULL_PNTRSTRING };
                                   /* For MINIMIZE_WITH */ /* 25-Jun-2014 nm */
  long forwRevPass; /* 1 = forward pass */ /* 25-Jun-2014 nm */

  long sourceStatement; /* For EXPAND */ /* 11-Sep-2016 nm */

  flag showLemmas; /* For WRITE THEOREM_LIST */ /* 10-Oct-2012 nm */

  /* toolsMode-specific variables */
  flag commandProcessedFlag = 0; /* Set when the first command line processed;
                                    used to exit shell command line mode */
  FILE *list1_fp;
  FILE *list2_fp;
  FILE *list3_fp;
  vstring list2_fname = "", list2_ftmpname = "";
  vstring list3_ftmpname = "";
  vstring oldstr = "", newstr = "";
  long lines, changedLines, oldChangedLines, twoMatches, p1, p2;
  long firstChangedLine;
  flag cmdMode, changedFlag, outMsgFlag;
  double sum;
  vstring bufferedLine = "";
  vstring tagStartMatch = "";  /* 2-Jul-2011 nm For TAG command */
  long tagStartCount = 0;      /* 2-Jul-2011 nm For TAG command */
  vstring tagEndMatch = "";    /* 2-Jul-2011 nm For TAG command */
  long tagEndCount = 0;        /* 2-Jul-2011 nm For TAG command */
  long tagStartCounter = 0;    /* 2-Jul-2011 nm For TAG command */
  long tagEndCounter = 0;      /* 2-Jul-2011 nm For TAG command */

  /* 21-Jun-2014 */
  /* 16-Aug-2016 nm Now in getElapasedTime() */
  /* clock_t timePrevious = 0; */  /* For SHOW ELAPSED_TIME command */
  /* clock_t timeNow = 0; */       /* For SHOW ELAPSED_TIME command */
  /* 16-Aug-2016 nm */
  double timeTotal = 0;
  double timeIncr = 0;
  flag printTime;  /* Set by "/ TIME" in SAVE PROOF and others */

  /* 14-Aug-2018 nm */
  flag defaultScrollMode = 1; /* Default to prompted mode */

  /* Initialization to avoid compiler warning (should not be theoretically
     necessary) */
  p = 0;
  q = 0;
  s = 0;
  texHeaderFlag = 0;
  firstChangedLine = 0;
  tagStartCount = 0;           /* 2-Jul-2011 nm For TAG command */
  tagEndCount = 0;             /* 2-Jul-2011 nm For TAG command */
  tagStartCounter = 0;         /* 2-Jul-2011 nm For TAG command */
  tagEndCounter = 0;           /* 2-Jul-2011 nm For TAG command */

  while (1) {

    if (listMode) {
      /* If called from the OS shell with arguments, do one command
         then exit program. */
      /* (However, let a SUBMIT job complete) */
      if (argc > 1 && commandProcessedFlag &&
             commandFileNestingLevel == 0) return;
    }

    errorCount = 0; /* Reset error count before each read or proof parse. */

    /* Deallocate stuff that may have been used in previous pass */
    let(&str1,"");
    let(&str2,"");
    let(&str3,"");
    let(&str4,"");
    let(&str5,"");
    nmbrLet(&nmbrTmp, NULL_NMBRSTRING);
    pntrLet(&pntrTmp, NULL_PNTRSTRING);
    nmbrLet(&nmbrSaveProof, NULL_NMBRSTRING);
    nmbrLet(&essentialFlags, NULL_NMBRSTRING);
    j = nmbrLen(rawArgNmbr);
    if (j != rawArgs) bug(1110);
    j = pntrLen(rawArgPntr);
    if (j != rawArgs) bug(1111);
    rawArgs = 0;
    for (i = 0; i < j; i++) let((vstring *)(&rawArgPntr[i]), "");
    pntrLet(&rawArgPntr, NULL_PNTRSTRING);
    nmbrLet(&rawArgNmbr, NULL_NMBRSTRING);
    j = pntrLen(fullArg);
    for (i = 0; i < j; i++) let((vstring *)(&fullArg[i]),"");
    pntrLet(&fullArg,NULL_PNTRSTRING);
    j = pntrLen(expandedProof);
    if (j) {
      for (i = 0; i < j; i++) {
        let((vstring *)(&expandedProof[i]),"");
      }
     pntrLet(&expandedProof,NULL_PNTRSTRING);
    }

    let(&list2_fname, "");
    let(&list2_ftmpname, "");
    let(&list3_ftmpname, "");
    let(&oldstr, "");
    let(&newstr, "");
    let(&labelMatch, "");
    /* (End of space deallocation) */

    midiFlag = 0; /* 8/28/00 Initialize here in case SHOW PROOF exits early */

    if (memoryStatus) {
      /*??? Change to user-friendly message */
#ifdef THINK_C
      print2("Memory:  string %ld xxxString %ld free %ld\n",db,db3,(long)FreeMem());
      getPoolStats(&i, &j, &k);
      print2("Pool:  free alloc %ld  used alloc %ld  used actual %ld\n",i,j,k);
#else
      print2("Memory:  string %ld xxxString %ld\n",db,db3);
#endif
      getPoolStats(&i, &j, &k);
      print2("Pool:  free alloc %ld  used alloc %ld  used actual %ld\n",i,j,k);
    }

    if (!toolsMode) {
      if (PFASmode) {
        let(&commandPrompt,"MM-PA> ");
      } else {
        let(&commandPrompt,"MM> ");
      }
    } else {
      if (listMode) {
        let(&commandPrompt,"Tools> ");
      } else {
        let(&commandPrompt,"TOOLS> ");
      }
    }

    let(&commandLine,""); /* Deallocate previous contents */

    if (!commandProcessedFlag && argc > 1 && argsProcessed < argc - 1
        && commandFileNestingLevel == 0) {
      /* if (toolsMode) { */  /* 10-Oct-2006 nm Fix bug: changed to listMode */
      if (listMode) {
        /* If program was compiled in TOOLS mode, the command-line argument
           is assumed to be a single TOOLS command; build the equivalent
           TOOLS command */
        for (i = 1; i < argc; i++) {
          argsProcessed++;
          /* Put quotes around an argument with spaces or tabs or quotes
             or empty string */
          if (instr(1, argv[i], " ") || instr(1, argv[i], "\t")
              || instr(1, argv[i], "\"") || instr(1, argv[i], "'")
              || (argv[i])[0] == 0) {
            /* If it contains a double quote, use a single quote */
            if (instr(1, argv[i], "\"")) {
              let(&str1, cat("'", argv[i], "'", NULL));
            } else {
              /* (??? (TODO)Case of both ' and " is not handled) */
              let(&str1, cat("\"", argv[i], "\"", NULL));
            }
          } else {
            let(&str1, argv[i]);
          }
          let(&commandLine, cat(commandLine, (i == 1) ? "" : " ", str1, NULL));
        }
      } else {
        /* If program was compiled in default (Metamath) mode, each command-line
           argument is considered a full Metamath command.  User is responsible
           for ensuring necessary quotes around arguments are passed in. */
        argsProcessed++;
        scrollMode = 0; /* Set continuous scrolling until completed */
        let(&commandLine, cat(commandLine, argv[argsProcessed], NULL));
        if (argc == 2 && instr(1, argv[1], " ") == 0) {
          /* Assume the user intended a READ command.  This special mode allows
             invocation via "metamath xxx.mm". */
          if (instr(1, commandLine, "\"") || instr(1, commandLine, "'")) {
            /* If it already has quotes don't put quotes */
            let(&commandLine, cat("READ ", commandLine, NULL));
          } else {
            /* Put quotes so / won't be interpreted as qualifier separator */
            let(&commandLine, cat("READ \"", commandLine, "\"", NULL));
          }

          /***** 3-Jan-2017 nm  This is now done with SET ROOT_DIRECTORY
          /@ 31-Dec-2017 nm @/
          /@ This block of code can be removed without side effects @/
          /@ "Hidden" hack for NM's convenience. :)  If there is an =, change
             it to space.  This lets users invoke with "metamath set.mm/h=test"
             or "metamath mbox/aa.mm/home=test" to specify an implicit read with
             /ROOT_DIRECTORY without having a space in the argument (a space
             means don't assume a read command). @/
          i = instr(1, commandLine, "=");
          if (i != 0) {
            /@ Change 'READ "set.mm/h=test"' to 'READ "set.mm" / "h" "test"' @/
            let(&commandLine, cat(left(commandLine, i - 1), "\" \"",
                right(commandLine, i + 1), NULL));
            while (i > 0) {
              if (commandLine[i - 1] == '/') {
                let(&commandLine, cat(left(commandLine, i - 1),
                    "\" / \"", right(commandLine, i + 1), NULL));
                break;
              }
              i--;
            }
            /@ Change ' to " to prevent mismatched quotes @/
            i = (long)strlen(commandLine);
            while (i > 0) {
              if (commandLine[i - 1] == '\'') {
                commandLine[i - 1] = '"';
              }
              i--;
            }
          } /@ if i != 0 (end of 31-Dec-2017 NM hack) @/
          */

        }
      }
      print2("%s\n", cat(commandPrompt, commandLine, NULL));
    } else {
      /* Get command from user input or SUBMIT script file */
      commandLine = cmdInput1(commandPrompt);
    }
    if (argsProcessed == argc && !commandProcessedFlag) {
      commandProcessedFlag = 1;
      scrollMode = defaultScrollMode; /* Set prompted (default) scroll mode */
    }
    if (argsProcessed == argc - 1) {
      argsProcessed++; /* Indicates restore scroll mode next time around */
      if (toolsMode) {
        /* If program was compiled in TOOLS mode, we're only going to execute
           one command; set flag to exit next time around */
        commandProcessedFlag = 1;
      }
    }

    /* See if it's an operating system command */
    /* (This is a command line that begins with a quote) */
    if (commandLine[0] == '\'' || commandLine[0] == '\"') {
      /* See if this computer has this feature */
      if (!system(NULL)) {
        print2("?This computer does not accept an operating system command.\n");
        continue;
      } else {
        /* Strip off quote and trailing quote if any */
        let(&str1, right(commandLine, 2));
        if (commandLine[0]) { /* (Prevent stray pointer if empty string) */
          if (commandLine[0] == commandLine[strlen(commandLine) - 1]) {
            let(&str1, left(str1, (long)(strlen(str1)) - 1));
          }
        }
        /* Do the operating system command */
        (void)system(str1);
#ifdef VAXC
        printf("\n"); /* Last line from VAX doesn't have new line */
#endif
        continue;
      }
    }

    parseCommandLine(commandLine);
    if (rawArgs == 0) {
      continue; /* Empty or comment line */
    }
    if (!processCommandLine()) {
      continue;
    }

    if (commandEcho || (toolsMode && listFile_fp != NULL)) {
      /* Build the complete command and print it for the user */
      k = pntrLen(fullArg);
      let(&str1,"");
      for (i = 0; i < k; i++) {
        if (instr(1, fullArg[i], " ") || instr(1, fullArg[i], "\t")
            || instr(1, fullArg[i], "\"") || instr(1, fullArg[i], "'")
            || ((char *)(fullArg[i]))[0] == 0) {
          /* If the argument has spaces or tabs or quotes
             or is empty string, put quotes around it */
          if (instr(1, fullArg[i], "\"")) {
            let(&str1, cat(str1, "'", fullArg[i], "' ", NULL));
          } else {
            /* (???Case of both ' and " is not handled) */
            let(&str1, cat(str1, "\"", fullArg[i], "\" ", NULL));
          }
        } else {
          let(&str1, cat(str1, fullArg[i], " ", NULL));
        }
      }
      let(&str1, left(str1, (long)(strlen(str1)) - 1)); /* Trim trailing spc */
      if (toolsMode && listFile_fp != NULL) {
        /* Put line in list.tmp as command */
        fprintf(listFile_fp, "%s\n", str1);  /* Print to list command file */
      }
      if (commandEcho) {
        /* 15-Jun-2009 nm Added code line below */
        /* Put special character "!" before line for easier extraction to
           build SUBMIT files; see also SET ECHO ON output below */
        let(&str1, cat("!", str1, NULL));
        /* The tilde is a special flag for printLongLine to print a
           tilde before the carriage return in a split line, not after */
        printLongLine(str1, "~", " ");
      }
    }

    if (cmdMatches("BEEP") || cmdMatches("B")) {
      /* Print a bell (if user types ahead "B", the bell lets him know when
         his command is finished - useful for long-running commands */
      print2("%c",7);
      continue;
    }

    if (cmdMatches("HELP")) {
      /* Build the complete command */
      k = pntrLen(fullArg);
      let(&str1,"");
      for (i = 0; i < k; i++) {
        let(&str1, cat(str1, fullArg[i], " ", NULL));
      }
      let(&str1, left(str1, (long)(strlen(str1)) - 1));
      if (toolsMode) {
        help0(str1);
        help1(str1);
      } else {
        help1(str1);
        help2(str1);
        help3(str1); /* 18-Jul-2015 nm */
      }
      continue;
    }


    if (cmdMatches("SET SCROLL")) {
      if (cmdMatches("SET SCROLL CONTINUOUS")) {
        defaultScrollMode = 0;
        scrollMode = 0;
        print2("Continuous scrolling is now in effect.\n");
      } else {
        defaultScrollMode = 1;
        scrollMode = 1;
        print2("Prompted scrolling is now in effect.\n");
      }
      continue;
    }

    if (cmdMatches("EXIT") || cmdMatches("QUIT")
        || cmdMatches("_EXIT_PA")) { /* 9-Jun-2016 - for MM-PA> exit
            in scripts, so it will error out in MM> (if for some reason
            MM-PA wasn't entered) instead of exiting metamath */
    /*???        || !strcmp(cmd,"^Z")) { */

      /* 9-Jun-2016 */
      if (cmdMatches("_EXIT_PA")) {
        if (!PFASmode || (toolsMode && !listMode)) bug(1127);
                 /* mmcmdl.c should have caught this */
      }

      if (toolsMode && !listMode) {
        /* Quitting tools command from within Metamath */
        if (!PFASmode) {
          print2(
 "Exiting the Text Tools.  Type EXIT again to exit Metamath.\n");
        } else {
          print2(
 "Exiting the Text Tools.  Type EXIT again to exit the Proof Assistant.\n");
        }
        toolsMode = 0;
        continue;
      }

      if (PFASmode) {

        if (proofChanged &&
              /* If proofChanged, but the UNDO stack is empty (and
                 there were no other conditions such as stack overflow),
                 the proof didn't really change, so it is safe to
                 exit MM-PA without warning */
              (processUndoStack(NULL, PUS_GET_STATUS, "", 0)
                 /* However, if the proof was saved earlier, UNDO stack
                    empty no longer indicates proof didn't change */
                 || proofSavedFlag)) {
          print2(
              "Warning:  You have not saved changes to the proof of \"%s\".\n",
              statement[proveStatement].labelName); /* 21-Jan-06 nm */
          /* 17-Aug-04 nm Added / FORCE qualifier */
          if (switchPos("/ FORCE") == 0) {
            str1 = cmdInput1("Do you want to EXIT anyway (Y, N) <N>? ");
            if (str1[0] != 'y' && str1[0] != 'Y') {
              print2("Use SAVE NEW_PROOF to save the proof.\n");
              continue;
            }
          } else {
            /* User specified / FORCE, so answer question automatically */
            print2("Do you want to EXIT anyway (Y, N) <N>? Y\n");
          }
        }

        proofChanged = 0;
        processUndoStack(NULL, PUS_INIT, "", 0);
        proofSavedFlag = 0; /* Will become 1 if proof is ever saved */

        print2(
 "Exiting the Proof Assistant.  Type EXIT again to exit Metamath.\n");

        /* Deallocate proof structure */
        deallocProofStruct(&proofInProgress); /* 20-May-2013 nm */

        /**** old deallocation before 20-May-2013
        i = nmbrLen(proofInProgress.proof);
        nmbrLet(&proofInProgress.proof, NULL_NMBRSTRING);
        for (j = 0; j < i; j++) {
          nmbrLet((nmbrString **)(&((proofInProgress.target)[j])),
              NULL_NMBRSTRING);
          nmbrLet((nmbrString **)(&((proofInProgress.source)[j])),
              NULL_NMBRSTRING);
          nmbrLet((nmbrString **)(&((proofInProgress.user)[j])),
              NULL_NMBRSTRING);
        }
        pntrLet(&proofInProgress.target, NULL_PNTRSTRING);
        pntrLet(&proofInProgress.source, NULL_PNTRSTRING);
        pntrLet(&proofInProgress.user, NULL_PNTRSTRING);
        *** end of old code before 20-May-2013 */

        PFASmode = 0;
        continue;
      } else {
        if (sourceChanged) {
          print2("Warning:  You have not saved changes to the source.\n");
          /* 17-Aug-04 nm Added / FORCE qualifier */
          if (switchPos("/ FORCE") == 0) {
            str1 = cmdInput1("Do you want to EXIT anyway (Y, N) <N>? ");
            if (str1[0] != 'y' && str1[0] != 'Y') {
              print2("Use WRITE SOURCE to save the changes.\n");
              continue;
            }
          } else {
            /* User specified / FORCE, so answer question automatically */
            print2("Do you want to EXIT anyway (Y, N) <N>? Y\n");
          }
          sourceChanged = 0;
        }

        if (texFileOpenFlag) {
          print2("The %s file \"%s\" was closed.\n",
              htmlFlag ? "HTML" : "LaTeX", texFileName);
          printTexTrailer(texHeaderFlag);
          fclose(texFilePtr);
          texFileOpenFlag = 0;
        }
        if (logFileOpenFlag) {
          print2("The log file \"%s\" was closed %s %s.\n",logFileName,
              date(),time_());
          fclose(logFilePtr);
          logFileOpenFlag = 0;
        }

        /* 4-May-2017 Ari Ferrera */
        /* Free remaining allocations before exiting */
        freeCommandLine();
        freeInOu();
        memFreePoolPurge(0);
        eraseSource();
        freeData(); /* Call AFTER eraseSource()(->initBigArrays->malloc) */
        let(&commandPrompt,"");
        let(&commandLine,"");
        let(&input_fn,"");
        let(&contributorName, ""); /* 14-May-2017 nm */

        return; /* Exit from program */
      }
    }

    if (cmdMatches("SUBMIT")) {
      if (commandFileNestingLevel == MAX_COMMAND_FILE_NESTING) {
        printf("?The SUBMIT nesting level has been exceeded.\n");
        continue;
      }
      commandFilePtr[commandFileNestingLevel + 1] = fSafeOpen(fullArg[1], "r",
          0/*noVersioningFlag*/);
      if (!commandFilePtr[commandFileNestingLevel + 1]) continue;
                                      /* Couldn't open (err msg was provided) */
      commandFileNestingLevel++;
      commandFileName[commandFileNestingLevel] = ""; /* Initialize if nec. */
      let(&commandFileName[commandFileNestingLevel], fullArg[1]);

      /* 23-Oct-2006 nm Added / SILENT */
      commandFileSilent[commandFileNestingLevel] = 0;
      if (switchPos("/ SILENT")
          || commandFileSilentFlag /* Propagate silence from outer level */) {
        commandFileSilent[commandFileNestingLevel] = 1;
      } else {
        commandFileSilent[commandFileNestingLevel] = 0;
      }
      commandFileSilentFlag = commandFileSilent[commandFileNestingLevel];
      if (!commandFileSilentFlag)
        print2("Taking command lines from file \"%s\"...\n",
            commandFileName[commandFileNestingLevel]);

      continue;
    }

    if (toolsMode) {
      /* Start of toolsMode-specific commands */
#define ADD_MODE 1
#define DELETE_MODE 2
#define CLEAN_MODE 3
#define SUBSTITUTE_MODE 4
#define SWAP_MODE 5
#define INSERT_MODE 6
#define BREAK_MODE 7
#define BUILD_MODE 8
#define MATCH_MODE 9
#define RIGHT_MODE 10
#define TAG_MODE 11  /* 2-Jul-2011 nm Added TAG command */
      cmdMode = 0;
      if (cmdMatches("ADD")) cmdMode = ADD_MODE;
      else if (cmdMatches("DELETE")) cmdMode = DELETE_MODE;
      else if (cmdMatches("CLEAN")) cmdMode = CLEAN_MODE;
      else if (cmdMatches("SUBSTITUTE") || cmdMatches("S"))
        cmdMode = SUBSTITUTE_MODE;
      else if (cmdMatches("SWAP")) cmdMode = SWAP_MODE;
      else if (cmdMatches("INSERT")) cmdMode = INSERT_MODE;
      else if (cmdMatches("BREAK")) cmdMode = BREAK_MODE;
      else if (cmdMatches("BUILD")) cmdMode = BUILD_MODE;
      else if (cmdMatches("MATCH")) cmdMode = MATCH_MODE;
      else if (cmdMatches("RIGHT")) cmdMode = RIGHT_MODE;
      else if (cmdMatches("TAG")) cmdMode = TAG_MODE;
      if (cmdMode) {
        list1_fp = fSafeOpen(fullArg[1], "r", 0/*noVersioningFlag*/);
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        if (cmdMode == RIGHT_MODE) {
          /* Find the longest line */
          p = 0;
          while (linput(list1_fp, NULL, &str1)) {
            if (p < (signed)(strlen(str1))) p = (long)(strlen(str1));
          }
          rewind(list1_fp);
        }
        let(&list2_fname, fullArg[1]);
        if (list2_fname[strlen(list2_fname) - 2] == '~') {
          let(&list2_fname, left(list2_fname, (long)(strlen(list2_fname)) - 2));
          print2("The output file will be called %s.\n", list2_fname);
        }
        let(&list2_ftmpname, "");
        list2_ftmpname = fGetTmpName("zz~tools");
        list2_fp = fSafeOpen(list2_ftmpname, "w", 0/*noVersioningFlag*/);
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */
        lines = 0;
        changedLines = 0;
        twoMatches = 0;
        changedFlag = 0;
        outMsgFlag = 0;
        switch (cmdMode) {
          case ADD_MODE:
            break;
          case TAG_MODE:  /* 2-Jul-2011 nm Added TAG command */
            let(&tagStartMatch, fullArg[4]);
            tagStartCount = (long)val(fullArg[5]);
            if (tagStartCount == 0) tagStartCount = 1; /* Default */
            let(&tagEndMatch, fullArg[6]);
            tagEndCount = (long)val(fullArg[7]);
            if (tagEndCount == 0) tagEndCount = 1; /* Default */
            tagStartCounter = 0;
            tagEndCounter = 0;
            break;
          case DELETE_MODE:
            break;
          case CLEAN_MODE:
            let(&str4, edit(fullArg[2], 32));
            q = 0;
            if (instr(1, str4, "P") > 0) q = q + 1;
            if (instr(1, str4, "D") > 0) q = q + 2;
            if (instr(1, str4, "G") > 0) q = q + 4;
            if (instr(1, str4, "B") > 0) q = q + 8;
            if (instr(1, str4, "R") > 0) q = q + 16;
            if (instr(1, str4, "C") > 0) q = q + 32;
            if (instr(1, str4, "E") > 0) q = q + 128;
            if (instr(1, str4, "Q") > 0) q = q + 256;
            if (instr(1, str4, "L") > 0) q = q + 512;
            if (instr(1, str4, "T") > 0) q = q + 1024;
            if (instr(1, str4, "U") > 0) q = q + 2048;
            if (instr(1, str4, "V") > 0) q = q + 4096;
            break;
          case SUBSTITUTE_MODE:
            let(&newstr, fullArg[3]); /* The replacement string */
            if (((vstring)(fullArg[4]))[0] == 'A' ||
                ((vstring)(fullArg[4]))[0] == 'a') { /* ALL */
              q = -1;
            } else {
              q = (long)val(fullArg[4]);
              if (q == 0) q = 1;    /* The occurrence # of string to subst */
            }
            s = instr(1, fullArg[2], "\\n");
            if (s) {
              /*s = 1;*/ /* Replace lf flag */
              q = 1; /* Only 1st occurrence makes sense in this mode */
            }
            if (!strcmp(fullArg[3], "\\n")) {
              let(&newstr, "\n"); /* Replace with lf */
            }
            break;
          case SWAP_MODE:
            break;
          case INSERT_MODE:
            p = (long)val(fullArg[3]);
            break;
          case BREAK_MODE:
            outMsgFlag = 1;
            break;
          case BUILD_MODE:
            let(&str4, "");
            outMsgFlag = 1;
            break;
          case MATCH_MODE:
            outMsgFlag = 1;
        } /* End switch */
        let(&bufferedLine, "");
        /*
        while (linput(list1_fp, NULL, &str1)) {
        */
        while (1) {
          if (bufferedLine[0]) {
            /* Get input from buffered line (from rejected \n replacement) */
            let(&str1, bufferedLine);
            let(&bufferedLine, "");
          } else {
            if (!linput(list1_fp, NULL, &str1)) break;
          }
          lines++;
          oldChangedLines = changedLines;
          let(&str2, str1);
          switch (cmdMode) {
            case ADD_MODE:
              let(&str2, cat(fullArg[2], str1, fullArg[3], NULL));
              if (strcmp(str1, str2)) changedLines++;
              break;
            case TAG_MODE:   /* 2-Jul-2011 nm Added TAG command */
              if (tagStartCounter < tagStartCount) {
                if (instr(1, str1, tagStartMatch)) tagStartCounter++;
              }
              if (tagStartCounter == tagStartCount &&
                  tagEndCounter < tagEndCount) { /* We're in tagging range */
                let(&str2, cat(fullArg[2], str1, fullArg[3], NULL));
                if (strcmp(str1, str2)) changedLines++;
                if (instr(1, str1, tagEndMatch)) tagEndCounter++;
              }
              break;
            case DELETE_MODE:
              p1 = instr(1, str1, fullArg[2]);
              if (strlen(fullArg[2]) == 0) p1 = 1;
              p2 = instr(p1, str1, fullArg[3]);
              if (strlen(fullArg[3]) == 0) p2 = (long)strlen(str1) + 1;
              if (p1 != 0 && p2 != 0) {
                let(&str2, cat(left(str1, p1 - 1), right(str1, p2
                    + (long)strlen(fullArg[3])), NULL));
                changedLines++;
              }
              break;
            case CLEAN_MODE:
              if (q) {
                let(&str2, edit(str1, q));
                if (strcmp(str1, str2)) changedLines++;
              }
              break;
            case SUBSTITUTE_MODE:
              let(&str2, str1);
              p = 0;
              p1 = 0;

              k = 1;
              /* See if an additional match on line is required */
              if (((vstring)(fullArg[5]))[0] != 0) {
                if (!instr(1, str2, fullArg[5])) {
                  /* No match on line; prevent any substitution */
                  k = 0;
                }
              }

              if (s && k) { /* We're asked to replace a newline char */
                /* Read in the next line */
                /*
                if (linput(list1_fp, NULL, &str4)) {
                  let(&str2, cat(str1, "\\n", str4, NULL));
                */
                if (linput(list1_fp, NULL, &bufferedLine)) {
                  /* Join the next line and see if the string matches */
                  if (instr(1, cat(str1, "\\n", bufferedLine, NULL),
                      fullArg[2])) {
                    let(&str2, cat(str1, "\\n", bufferedLine, NULL));
                    let(&bufferedLine, "");
                  } else {
                    k = 0; /* No match - leave bufferedLine for next pass */
                  }
                } else { /* EOF reached */
                  print2("Warning: file %s has an odd number of lines\n",
                      fullArg[1]);
                }
              }

              while (k) {
                p1 = instr(p1 + 1, str2, fullArg[2]);
                if (!p1) break;
                p++;
                if (p == q || q == -1) {
                  let(&str2, cat(left(str2, p1 - 1), newstr,
                      right(str2, p1 + (long)strlen(fullArg[2])), NULL));
                  if (newstr[0] == '\n') {
                    /* Replacement string is an lf */
                    lines++;
                    changedLines++;
                  }
                  /* 14-Sep-2010 nm Continue the search after the replacement
                     string, so that "SUBST 1.tmp abbb ab a ''" will change
                     "abbbab" to "abba" rather than "aa" */
                  p1 = p1 + (long)strlen(newstr) - 1;
                  /* p1 = p1 - (long)strlen(fullArg[2]) + (long)strlen(newstr); */ /* bad */
                  if (q != -1) break;
                }
              }
              if (strcmp(str1, str2)) changedLines++;
              break;
            case SWAP_MODE:
              p1 = instr(1, str1, fullArg[2]);
              if (p1) {
                p2 = instr(p1 + 1, str1, fullArg[2]);
                if (p2) twoMatches++;
                let(&str2, cat(right(str1, p1) + (long)strlen(fullArg[2]),
                    fullArg[2], left(str1, p1 - 1), NULL));
                if (strcmp(str1, str2)) changedLines++;
              }
              break;
            case INSERT_MODE:
              if ((signed)(strlen(str2)) < p - 1)
                let(&str2, cat(str2, space(p - 1 - (long)strlen(str2)), NULL));
              let(&str2, cat(left(str2, p - 1), fullArg[2],
                  right(str2, p), NULL));
              if (strcmp(str1, str2)) changedLines++;
              break;
            case BREAK_MODE:
              let(&str2, str1);
              changedLines++;
              for (i = 0; i < (signed)(strlen(fullArg[2])); i++) {
                p = 0;
                while (1) {
                  p = instr(p + 1, str2, chr(((vstring)(fullArg[2]))[i]));
                  if (!p) break;
                  /* Put spaces arount special one-char tokens */
                  let(&str2, cat(left(str2, p - 1), " ",
                      mid(str2, p, 1),
                      " ", right(str2, p + 1), NULL));
                  p++;
                }
              }
              let(&str2, edit(str2, 8 + 16 + 128)); /* Reduce & trim spaces */
              for (p = (long)strlen(str2) - 1; p >= 0; p--) {
                if (str2[p] == ' ') {
                  str2[p] = '\n';
                  changedLines++;
                }
              }
              if (!str2[0]) changedLines--; /* Don't output blank line */
              break;
            case BUILD_MODE:
              if (str2[0] != 0) { /* Ignore blank lines */
                if (str4[0] == 0) {
                  let(&str4, str2);
                } else {
                  if ((long)strlen(str4) + (long)strlen(str2) > 72) {
                    let(&str4, cat(str4, "\n", str2, NULL));
                    changedLines++;
                  } else {
                    let(&str4, cat(str4, " ", str2, NULL));
                  }
                }
                p = instr(1, str4, "\n");
                if (p) {
                  let(&str2, left(str4, p - 1));
                  let(&str4, right(str4, p + 1));
                } else {
                  let(&str2, "");
                }
              }
              break;
            case MATCH_MODE:
              if (((vstring)(fullArg[2]))[0] == 0) {
                /* Match any non-blank line */
                p = str1[0];
              } else {
                p = instr(1, str1, fullArg[2]);
              }
              if (((vstring)(fullArg[3]))[0] == 'n' ||
                  ((vstring)(fullArg[3]))[0] == 'N') {
                p = !p;
              }
              if (p) changedLines++;
              break;
            case RIGHT_MODE:
              let(&str2, cat(space(p - (long)strlen(str2)), str2, NULL));
              if (strcmp(str1, str2)) changedLines++;
              break;
          } /* End switch(cmdMode) */
          if (lines == 1) let(&str3, left(str2, 79)); /* For msg */
          if (oldChangedLines != changedLines && !changedFlag) {
            changedFlag = 1;
            let(&str3, left(str2, 79)); /* For msg */
            firstChangedLine = lines;
            if ((cmdMode == SUBSTITUTE_MODE && newstr[0] == '\n')
                || cmdMode == BUILD_MODE) /* Joining lines */ {
              firstChangedLine = 1; /* Better message */
            }
          }
          if (((cmdMode != BUILD_MODE && cmdMode != BREAK_MODE)
              || str2[0] != 0)
              && (cmdMode != MATCH_MODE || p))
            fprintf(list2_fp, "%s\n", str2);
        } /* Next input line */
        if (cmdMode == BUILD_MODE) {
          if (str4[0]) {
            /* Output last partial line */
            fprintf(list2_fp, "%s\n", str4);
            changedLines++;
            if (!str3[0]) {
              let(&str3, str4); /* For msg */
            }
          }
        }
        /* Remove any lines after lf for readability of msg */
        p = instr(1, str3, "\n");
        if (p) let(&str3, left(str3, p - 1));
        if (!outMsgFlag) {
          /* 18-Aug-2011 nm Make message depend on line counts */
          if (!changedFlag) {
            if (!lines) {
              print2("The file %s has no lines.\n", fullArg[1]);
            } else {
              print2(
"The file %s has %ld line%s; none were changed.  First line:\n",
                list2_fname, lines, (lines == 1) ? "" : "s");
              print2("%s\n", str3);
            }
          } else {
            print2(
"The file %s has %ld line%s; %ld w%s changed.  First changed line is %ld:\n",
                list2_fname,
                lines,  (lines == 1) ? "" : "s",
                changedLines,  (changedLines == 1) ? "as" : "ere",
                firstChangedLine);
            print2("%s\n", str3);
          }
          if (twoMatches > 0) {
            /* For SWAP command */
            print2(
"Warning:  %ld line%s more than one \"%s\".  The first one was used.\n",
                twoMatches, (twoMatches == 1) ? " has" : "s have", fullArg[2]);
          }
        } else {
          /* if (changedLines == 0) let(&str3, ""); */
          print2(
"The input had %ld line%s, the output has %ld line%s.%s\n",
              lines, (lines == 1) ? "" : "s",
              changedLines, (changedLines == 1) ? "" : "s",
              (changedLines == 0) ? "" : " First output line:");
          if (changedLines != 0) print2("%s\n", str3);
        }
        fclose(list1_fp);
        fclose(list2_fp);
        fSafeRename(list2_ftmpname, list2_fname);
        /* Deallocate string memory */
        let(&tagStartMatch, "");  /* 2-Jul-2011 nm Added TAG command */
        let(&tagEndMatch, "");  /* 2-Jul-2011 nm Added TAG command */
        continue;
      } /* end if cmdMode for ADD, etc. */

#define SORT_MODE 1
#define UNDUPLICATE_MODE 2
#define DUPLICATE_MODE 3
#define UNIQUE_MODE 4
#define REVERSE_MODE 5
      cmdMode = 0;
      if (cmdMatches("SORT")) cmdMode = SORT_MODE;
      else if (cmdMatches("UNDUPLICATE")) cmdMode = UNDUPLICATE_MODE;
      else if (cmdMatches("DUPLICATE")) cmdMode = DUPLICATE_MODE;
      else if (cmdMatches("UNIQUE")) cmdMode = UNIQUE_MODE;
      else if (cmdMatches("REVERSE")) cmdMode = REVERSE_MODE;
      if (cmdMode) {
        list1_fp = fSafeOpen(fullArg[1], "r", 0/*noVersioningFlag*/);
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        let(&list2_fname, fullArg[1]);
        if (list2_fname[strlen(list2_fname) - 2] == '~') {
          let(&list2_fname, left(list2_fname, (long)strlen(list2_fname) - 2));
          print2("The output file will be called %s.\n", list2_fname);
        }
        let(&list2_ftmpname, "");
        list2_ftmpname = fGetTmpName("zz~tools");
        list2_fp = fSafeOpen(list2_ftmpname, "w", 0/*noVersioningFlag*/);
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */

        /* Count the lines */
        lines = 0;
        while (linput(list1_fp, NULL, &str1)) lines++;
        if (cmdMode != SORT_MODE  && cmdMode != REVERSE_MODE) {
          print2("The input file has %ld lines.\n", lines);
        }

        /* Close and reopen the input file */
        fclose(list1_fp);
        list1_fp = fSafeOpen(fullArg[1], "r", 0/*noVersioningFlag*/);
        /* Allocate memory */
        pntrLet(&pntrTmp, pntrSpace(lines));
        /* Assign the lines to string array */
        for (i = 0; i < lines; i++) linput(list1_fp, NULL,
            (vstring *)(&pntrTmp[i]));

        /* Sort */
        if (cmdMode != REVERSE_MODE) {
          if (cmdMode == SORT_MODE) {
            qsortKey = fullArg[2]; /* Do not deallocate! */
          } else {
            qsortKey = "";
          }
          qsort(pntrTmp, (size_t)lines, sizeof(void *), qsortStringCmp);
        } else { /* Reverse the lines */
          for (i = lines / 2; i < lines; i++) {
            qsortKey = pntrTmp[i]; /* Use qsortKey as handy tmp var here */
            pntrTmp[i] = pntrTmp[lines - 1 - i];
            pntrTmp[lines - 1 - i] = qsortKey;
          }
        }

        /* Output sorted lines */
        changedLines = 0;
        let(&str3, "");
        for (i = 0; i < lines; i++) {
          j = 0; /* Flag that line should be printed */
          switch (cmdMode) {
            case SORT_MODE:
            case REVERSE_MODE:
              j = 1;
              break;
            case UNDUPLICATE_MODE:
              if (i == 0) {
                j = 1;
              } else {
                if (strcmp((vstring)(pntrTmp[i - 1]), (vstring)(pntrTmp[i]))) {
                  j = 1;
                }
              }
              break;
            case DUPLICATE_MODE:
              if (i > 0) {
                if (!strcmp((vstring)(pntrTmp[i - 1]), (vstring)(pntrTmp[i]))) {
                  if (i == lines - 1) {
                    j = 1;
                  } else {
                    if (strcmp((vstring)(pntrTmp[i]),
                        (vstring)(pntrTmp[i + 1]))) {
                      j = 1;
                    }
                  }
                }
              }
              break;
            case UNIQUE_MODE:
              if (i < lines - 1) {
                if (strcmp((vstring)(pntrTmp[i]), (vstring)(pntrTmp[i + 1]))) {
                  if (i == 0) {
                    j = 1;
                  } else {
                    if (strcmp((vstring)(pntrTmp[i - 1]),
                        (vstring)(pntrTmp[i]))) {
                      j = 1;
                    }
                  }
                }
              } else {
                if (i == 0) {
                  j = 1;
                } else {
                  if (strcmp((vstring)(pntrTmp[i - 1]),
                        (vstring)(pntrTmp[i]))) {
                      j = 1;
                  }
                }
              }
              break;
          } /* end switch (cmdMode) */
          if (j) {
            fprintf(list2_fp, "%s\n", (vstring)(pntrTmp[i]));
            changedLines++;
            if (changedLines == 1)
              let(&str3, left((vstring)(pntrTmp[i]), 79));
          }
        } /* next i */
        print2("The output file has %ld lines.  The first line is:\n",
            changedLines);
        print2("%s\n", str3);

        /* Deallocate memory */
        for (i = 0; i < lines; i++) let((vstring *)(&pntrTmp[i]), "");
        pntrLet(&pntrTmp,NULL_PNTRSTRING);

        fclose(list1_fp);
        fclose(list2_fp);
        fSafeRename(list2_ftmpname, list2_fname);
        continue;
      } /* end if cmdMode for SORT, etc. */

      if (cmdMatches("PARALLEL")) {
        list1_fp = fSafeOpen(fullArg[1], "r", 0/*noVersioningFlag*/);
        list2_fp = fSafeOpen(fullArg[2], "r", 0/*noVersioningFlag*/);
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */
        let(&list3_ftmpname, "");
        list3_ftmpname = fGetTmpName("zz~tools");
        list3_fp = fSafeOpen(list3_ftmpname, "w", 0/*noVersioningFlag*/);
        if (!list3_fp) continue; /* Couldn't open it (error msg was provided) */

        p1 = 1; p2 = 1; /* not eof */
        p = 0; q = 0; /* lines */
        j = 0; /* 1st line flag */
        let(&str3, "");
        while (1) {
          let(&str1, "");
          if (p1) {
            p1 = linput(list1_fp, NULL, &str1);
            if (p1) p++;
            else let(&str1, "");
          }
          let(&str2, "");
          if (p2) {
            p2 = linput(list2_fp, NULL, &str2);
            if (p2) q++;
            else let(&str2, "");
          }
          if (!p1 && !p2) break;
          let(&str4, cat(str1, fullArg[4], str2, NULL));
          fprintf(list3_fp, "%s\n", str4);
          if (!j) {
            let(&str3, str4); /* Save 1st line for msg */
            j = 1;
          }
        }
        if (p == q) {
          print2(
"The input files each had %ld lines.  The first output line is:\n", p);
        } else {
          print2(
"Warning: file \"%s\" had %ld lines while file \"%s\" had %ld lines.\n",
              fullArg[1], p, fullArg[2], q);
          if (p < q) p = q;
          print2("The output file \"%s\" has %ld lines.  The first line is:\n",
              fullArg[3], p);
        }
        print2("%s\n", str3);

        fclose(list1_fp);
        fclose(list2_fp);
        fclose(list3_fp);
        fSafeRename(list3_ftmpname, fullArg[3]);
        continue;
      }


      if (cmdMatches("NUMBER")) {
        list1_fp = fSafeOpen(fullArg[1], "w", 0/*noVersioningFlag*/);
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        j = (long)strlen(str(val(fullArg[2])));
        k = (long)strlen(str(val(fullArg[3])));
        if (k > j) j = k;
        for (i = (long)val(fullArg[2]); i <= val(fullArg[3]);
            i = i + (long)val(fullArg[4])) {
          let(&str1, str((double)i));
          fprintf(list1_fp, "%s\n", str1);
        }
        fclose(list1_fp);
        continue;
      }

      if (cmdMatches("COUNT")) {
        list1_fp = fSafeOpen(fullArg[1], "r", 0/*noVersioningFlag*/);
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        p1 = 0;
        p2 = 0;
        lines = 0;
        q = 0; /* Longest line length */
        i = 0; /* First longest line */
        j = 0; /* Number of longest lines */
        sum = 0.0; /* Sum of numeric content of lines */
        firstChangedLine = 0;
        while (linput(list1_fp, NULL, &str1)) {
          lines++;

          /* Longest line */
          if (q < (signed)(strlen(str1))) {
            q = (long)strlen(str1);
            let(&str4, str1);
            i = lines;
            j = 0;
          }

          if (q == (signed)(strlen(str1))) {
            j++;
          }

          if (instr(1, str1, fullArg[2])) {
            if (!firstChangedLine) {
              firstChangedLine = lines;
              let(&str3, str1);
            }
            p1++;
            p = 0;
            while (1) {
              p = instr(p + 1, str1, fullArg[2]);
              if (!p) break;
              p2++;
            }
          }
          sum = sum + val(str1);
        }
        print2(
"The file has %ld lines.  The string \"%s\" occurs %ld times on %ld lines.\n",
            lines, fullArg[2], p2, p1);
        if (firstChangedLine) {
          print2("The first occurrence is on line %ld:\n", firstChangedLine);
          print2("%s\n", str3);
        }
        print2(
"The first longest line (out of %ld) is line %ld and has %ld characters:\n",
            j, i, q);
        printLongLine(str4, "    "/*startNextLine*/, ""/*breakMatch*/);
            /* breakMatch empty means break line anywhere */  /* 6-Dec-03 */
 /* print2("If each line were a number, their sum would be %s\n", str((double)sum)); */
        printLongLine(cat(
            "Stripping all but digits, \".\", and \"-\", the sum of lines is ",
            str((double)sum), NULL), "    ", " ");
        fclose(list1_fp);
        continue;
      }

      if (cmdMatches("TYPE") || cmdMatches("T")) {
        list1_fp = fSafeOpen(fullArg[1], "r", 0/*noVersioningFlag*/);
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        if (rawArgs == 2) {
          n = 10;
        } else {
          if (((vstring)(fullArg[2]))[0] == 'A' ||
              ((vstring)(fullArg[2]))[0] == 'a') { /* ALL */
            n = -1;
          } else {
            n = (long)val(fullArg[2]);
          }
        }
        for (i = 0; i < n || n == -1; i++) {
          if (!linput(list1_fp, NULL, &str1)) break;
          if (!print2("%s\n", str1)) break;
        }
        fclose(list1_fp);
        continue;
      } /* end TYPE */

      if (cmdMatches("UPDATE")) {
        list1_fp = fSafeOpen(fullArg[1], "r", 0/*noVersioningFlag*/);
        list2_fp = fSafeOpen(fullArg[2], "r", 0/*noVersioningFlag*/);
        if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */
        if (!getRevision(fullArg[4])) {
          print2(
"?The revision tag must be of the form /*nn*/ or /*#nn*/.  Please try again.\n");
          continue;
        }
        let(&list3_ftmpname, "");
        list3_ftmpname = fGetTmpName("zz~tools");
        list3_fp = fSafeOpen(list3_ftmpname, "w", 0/*noVersioningFlag*/);
        if (!list3_fp) continue; /* Couldn't open it (error msg was provided) */

        revise(list1_fp, list2_fp, list3_fp, fullArg[4],
            (long)val(fullArg[5]));

        fSafeRename(list3_ftmpname, fullArg[3]);
        continue;
      }

      if (cmdMatches("COPY") || cmdMatches("C")) {
        let(&list2_ftmpname, "");
        list2_ftmpname = fGetTmpName("zz~tools");
        list2_fp = fSafeOpen(list2_ftmpname, "w", 0/*noVersioningFlag*/);
        if (!list2_fp) continue; /* Couldn't open it (error msg was provided) */
        let(&str4, cat(fullArg[1], ",", NULL));
        lines = 0;
        j = 0; /* Error flag */
        while (1) {
          if (!str4[0]) break; /* Done scanning list */
          p = instr(1, str4, ",");
          let(&str3, left(str4, p - 1));
          let(&str4, right(str4, p + 1));
          list1_fp = fSafeOpen((str3), "r", 0/*noVersioningFlag*/);
          if (!list1_fp) { /* Couldn't open it (error msg was provided) */
            j = 1; /* Error flag */
            break;
          }
          n = 0;
          while (linput(list1_fp, NULL, &str1)) {
            lines++; n++;
            fprintf(list2_fp, "%s\n", str1);
          }
          if (instr(1, fullArg[1], ",")) { /* More than 1 input file */
            print2("The input file \"%s\" has %ld lines.\n", str3, n);
          }
          fclose(list1_fp);
        }
        if (j) continue; /* One of the input files couldn't be opened */
        fclose(list2_fp);
        print2("The output file \"%s\" has %ld lines.\n", fullArg[2], lines);
        fSafeRename(list2_ftmpname, fullArg[2]);
        continue;
      }

      print2("?This command has not been implemented yet.\n");
      continue;
    } /* End of toolsMode-specific commands */

    if (cmdMatches("TOOLS")) {
      print2(
"Entering the Text Tools utilities.  Type HELP for help, EXIT to exit.\n");
      toolsMode = 1;
      continue;
    }

    if (cmdMatches("READ")) {
      /*if (statements) {*/
      /* 31-Dec-2017 nm */
      /* We can't use 'statements > 0' for the test since the source
         could be just a comment */
      if (sourceHasBeenRead == 1) {
        printLongLine(cat(
            "?Sorry, reading of more than one source file is not allowed.  ",
            "The file \"", input_fn, "\" has already been READ in.  ",
                                                             /* 11-Dec-05 nm */
            "You may type ERASE to start over.  Note that additional source ",
        "files may be included in the source file with \"$[ <filename> $]\".",
                                                             /* 11-Dec-05 nm */
            NULL),"  "," ");
        continue;
      }

      let(&input_fn, fullArg[1]);

      /***** 3-Jan-2017 nm  This is now done with SET ROOT_DIRECTORY
      /@ 31-Dec-2017 nm - Added ROOT_DIRECTORY switch @/
      /@ TODO - remove this and just use SET ROOT_DIRECTORY instead @/
      i = switchPos("/ ROOT_DIRECTORY"); /@ Statement match to skip @/
      if (i != 0) {
        let(&rootDirectory, edit(fullArg[i + 1], 2/@discard spaces,tabs@/));
        if (rootDirectory[0] != 0) {  /@ Not an empty directory path @/
          /@ Add trailing "/" to rootDirectory if missing @/
          if (instr(1, rootDirectory, "\\") != 0
              || instr(1, input_fn, "\\") != 0 ) {
            /@ Using Windows-style path (not really supported, but at least
               make full path consistent) @/
            if (rootDirectory[strlen(rootDirectory) - 1] != '\\') {
              let(&rootDirectory, cat(rootDirectory, "\\", NULL));
            }
          } else {
            if (rootDirectory[strlen(rootDirectory) - 1] != '/') {
              let(&rootDirectory, cat(rootDirectory, "/", NULL));
            }
          }
        }
      /@
      } else {
        let(&rootDirectory, "");
      @/
      }
      */

      let(&str1, cat(rootDirectory, input_fn, NULL));
      input_fp = fSafeOpen(str1, "r", 0/*noVersioningFlag*/);
      if (!input_fp) continue; /* Couldn't open it (error msg was provided
                                     by fSafeOpen) */
      fclose(input_fp);

      readInput();
      /*sourceHasBeenRead = 1;*/ /* Global variable - set in readInput() */

      if (switchPos("/ VERIFY")) {
        verifyProofs("*",1); /* Parse and verify */
      } else {
        /* verifyProofs("*",0); */ /* Parse only (for gross error checking) */
      }

      /* 13-Dec-2016 nm Moved to verifyMarkup in mmcmds.c */
      /*
      /@ 10/21/02 - detect Microsoft bugs reported by several users, when the
         HTML output files are named "con.html" etc. @/
      /@ If we want a standard error message underlining token, this could go
         in mmpars.c @/
      /@ From Microsoft's site:
         "The following reserved words cannot be used as the name of a file:
         CON, PRN, AUX, CLOCK$, NUL, COM1, COM2, COM3, COM4, COM5, COM6, COM7,
         COM8, COM9, LPT1, LPT2, LPT3, LPT4, LPT5, LPT6, LPT7, LPT8, and LPT9.
         Also, reserved words followed by an extension - for example,
         NUL.tx7 - are invalid file names." @/
      /@ Check for labels that will lead to illegal Microsoft file names for
         Windows users.  Don't bother checking CLOCK$ since $ is already
         illegal @/
      let(&str1, cat(
         ",CON,PRN,AUX,NUL,COM1,COM2,COM3,COM4,COM5,COM6,COM7,",
         "COM8,COM9,LPT1,LPT2,LPT3,LPT4,LPT5,LPT6,LPT7,LPT8,LPT9,", NULL));
      for (i = 1; i <= statements; i++) {
        let(&str2, cat(",", edit(statement[i].labelName, 32/@uppercase@/), ",",
            NULL));
        if (instr(1, str1, str2) ||
            /@ 5-Jan-04 mm@.html is reserved for mmtheorems.html, etc. @/
            !strcmp(",MM", left(str2, 3))) {
          print2("\n");
          assignStmtFileAndLineNum(j); /@ 9-Jan-2018 nm @/
          printLongLine(cat("?Warning in statement \"",
              statement[i].labelName, "\" at line ",
              str((double)(statement[i].lineNum)),
              " in file \"", statement[i].fileName,
              "\".  To workaround a Microsoft operating system limitation, the",
              " the following reserved words cannot be used for label names:",
              " CON, PRN, AUX, CLOCK$, NUL, COM1, COM2, COM3, COM4, COM5,",
              " COM6, COM7, COM8, COM9, LPT1, LPT2, LPT3, LPT4, LPT5, LPT6,",
              " LPT7, LPT8, and LPT9.  Also, \"mm@.html\" is reserved for",
              " Metamath file names.  Use another name for this label.", NULL),
              "", " ");
          errorCount++;
        }
      }
      /@ 10/21/02 end @/
      */

      if (sourceHasBeenRead == 1) {  /* 9-Jan-2018 nm */
        if (!errorCount) {
          let(&str1, "No errors were found.");
          if (!switchPos("/ VERIFY")) {
              let(&str1, cat(str1,
         "  However, proofs were not checked.  Type VERIFY PROOF *",
         " if you want to check them.",
              NULL));
          }
          printLongLine(str1, "", " ");
        } else {
          print2("\n");
          if (errorCount == 1) {
            print2("One error was found.\n");
          } else {
            print2("%ld errors were found.\n", (long)errorCount);
          }
        }
      } /* sourceHasBeenRead == 1 */

      continue;
    }

    if (cmdMatches("WRITE SOURCE")) {
      let(&output_fn, fullArg[2]);

      /********* Deleted 28-Dec-2013 nm Now opened in writeInput()
      output_fp = fSafeOpen(output_fn, "w", 0/@noVersioningFlag@/);
      if (!output_fp) continue; /@ Couldn't open it (error msg was provided)@/
      ********/

      /******* Deleted 3-May-2017 nm
      /@ Added 24-Oct-03 nm @/
      if (switchPos("/ CLEAN") > 0) {
        c = 1; /@ Clean out any proof-in-progress (that user has flagged
                        with a ? in its date comment field) @/
      } else {
        c = 0; /@ Output all proofs (normal) @/
      }
      *******/

      /* Added 12-Jun-2011 nm */
      if (switchPos("/ REWRAP") > 0) {
        r = 2; /* Re-wrap then format (more aggressive than / FORMAT) */
      } else if (switchPos("/ FORMAT") > 0) {
        r = 1; /* Format output according to set.mm standard */
      } else {
        r = 0; /* Keep formatting as-is */
      }

      /********* Deleted 3-May-2017 nm
      writeInput((char)c, (char)r); /@ Added arg 24-Oct-03 nm 12-Jun-2011 nm @/
      fclose(output_fp);
      if (c == 0) sourceChanged = 0; /@ Don't unset flag if CLEAN option
                                    since some new proofs may not be saved. @/
      ***********/


      /***** 3-Jan-2017 nm  This is now done with SET ROOT_DIRECTORY
      /@ 31-Dec-2017 nm - Added ROOT_DIRECTORY switch @/
      /@ TODO - remove this qualifier; too confusing.
         Use SET ROOT_DIRECTORY instead. @/
      i = switchPos("/ ROOT_DIRECTORY"); /@ Statement match to skip @/
      if (i != 0) {
        let(&rootDirectory, edit(fullArg[i + 1], 2/@discard spaces,tabs@/));
        /@ Add trailing "/" to rootDirectory if missing @/
        if (instr(1, rootDirectory, "\\") != 0
            || instr(1, input_fn, "\\") != 0 ) {
          /@ Using Windows-style path (not really supported, but at least
             make full path consistent) @/
          if (rootDirectory[strlen(rootDirectory) - 1] != '\\') {
            let(&rootDirectory, cat(rootDirectory, "\\", NULL));
          }
        } else {
          if (rootDirectory[strlen(rootDirectory) - 1] != '/') {
            let(&rootDirectory, cat(rootDirectory, "/", NULL));
          }
        }
      /@
      } else {
        let(&rootDirectory, "");
      @/
      }
      */


      /* 3-May-2017 nm */
      writeInput((char)r,  /* Added arg 12-Jun-2011 nm */
        ((switchPos("/ SPLIT") > 0) ? 1 : 0),          /* 31-Dec-2017 nm */
        ((switchPos("/ NO_VERSIONING") > 0) ? 1 : 0),  /* 31-Dec-2017 nm */
        ((switchPos("/ KEEP_INCLUDES") > 0) ? 1 : 0)   /* 31-Dec-2017 nm */
         );
      /*fclose(output_fp);*/
      sourceChanged = 0;

      continue;
    } /* End of WRITE SOURCE */


    if (cmdMatches("WRITE THEOREM_LIST")) {
      /* 4-Dec-03 - Write out an HTML summary of the theorems to
         mmtheorems.html, mmtheorems1.html,... */
      /* THEOREMS_PER_PAGE is the default number of proof descriptions to output. */
#define THEOREMS_PER_PAGE 100
      /* i is the actual number of proof descriptions to output. */
      /* See if the user overrode the default. */
      i = switchPos("/ THEOREMS_PER_PAGE");
      if (i) {
        i = (long)val(fullArg[i + 1]); /* Use user's value */
      } else {
        i = THEOREMS_PER_PAGE; /* Use the default value */
      }
      showLemmas = (switchPos("/ SHOW_LEMMAS") != 0);

      /**** 17-Nov-2015 nm Deleted, no longer need this restriction
      if (!texDefsRead) {
        htmlFlag = 1;
        print2("Reading definitions from $t statement of %s...\n", input_fn);
        if (2/@error@/ == readTexDefs(0 /@ 1 = check errors only @/,
            0 /@ 1 = no GIF file existence check @/ )) {
          continue; /@ An error occurred @/
        }
      } else {
        /@ Current limitation - can only read def's from .mm file once @/
        if (!htmlFlag) {
          print2("?You cannot use both LaTeX and HTML in the same session.\n");
          print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
          continue;
        }
      }
      *** end of 17-Nov-2015 deletion */

      htmlFlag = 1;
      /* If not specified, for backwards compatibility in scripts
         leave altHtmlFlag at current value */
      if (switchPos("/ HTML") != 0) {
        if (switchPos("/ ALT_HTML") != 0) {
          print2("?Please specify only one of / HTML and / ALT_HTML.\n");
          continue;
        }
        altHtmlFlag = 0;
      } else {
        if (switchPos("/ ALT_HTML") != 0) altHtmlFlag = 1;
      }

      if (2/*error*/ == readTexDefs(0 /* 1 = check errors only */,
          0 /* 1 = no GIF file existence check */ )) {
        continue; /* An error occurred */
      }

      /* Output the theorem list */
      writeTheoremList(i, showLemmas); /* (located in mmwtex.c) */
      continue;
    }  /* End of "WRITE THEOREM_LIST" */


    if (cmdMatches("WRITE BIBLIOGRAPHY")) {
      /* 10/10/02 */
      /* This command builds the bibliographical cross-references to various
         textbooks and updates the user-specified file normally called
         mmbiblio.html. */

      /* 17-Nov-2015 nm code moved to writeBibliography() in mmwtex.c */
      /* (Also called by verifyMarkup() in mmcmds.c for error checking) */
      writeBibliography(fullArg[2],
          "*", /* labelMatch - all labels */
          0,  /* 1 = no output, just warning msgs if any */
          0); /* 1 = ignore missing external files (gifs, bib, etc.) */
      continue;
    }  /* End of "WRITE BIBLIOGRAPHY" */


    if (cmdMatches("WRITE RECENT_ADDITIONS")) {
      /* 18-Sep-03 -
         This utility creates a list of recent proof descriptions and updates
         the user-specified file normally called mmrecent.html.
       */

      /* RECENT_COUNT is the default number of proof descriptions to output. */
#define RECENT_COUNT 100
      /* i is the actual number of proof descriptions to output. */
      /* See if the user overrode the default. */
      i = switchPos("/ LIMIT");
      if (i) {
        i = (long)val(fullArg[i + 1]); /* Use user's value */
      } else {
        i = RECENT_COUNT; /* Use the default value */
      }

      /**** 17-Nov-2015 nm Deleted because readTeXDefs() now handles everything
      if (!texDefsRead) {
        htmlFlag = 1;
        print2("Reading definitions from $t statement of %s...\n", input_fn);
        if (2/@error@/ == readTexDefs(0 /@ 1 = check errors only @/,
            0 /@ 1 = no GIF file existence check @/  )) {
          continue; /@ An error occurred @/
        }
      } else {
        /@ Current limitation - can only read def's from .mm file once @/
        if (!htmlFlag) {
          print2("?You cannot use both LaTeX and HTML in the same session.\n");
          print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
          continue;
        }
      }
      **** end of 17-Nov-2015 deletion */

      htmlFlag = 1;
      /* If not specified, for backwards compatibility in scripts
         leave altHtmlFlag at current value */
      if (switchPos("/ HTML") != 0) {
        if (switchPos("/ ALT_HTML") != 0) {
          print2("?Please specify only one of / HTML and / ALT_HTML.\n");
          continue;
        }
        altHtmlFlag = 0;
      } else {
        if (switchPos("/ ALT_HTML") != 0) altHtmlFlag = 1;
      }

      /* readTexDefs() rereads based on changed in htmlFlag, altHtmlFlag */
      if (2/*error*/ == readTexDefs(0 /* 1 = check errors only */,
          0 /* 1 = no GIF file existence check */  )) {
        continue; /* An error occurred */
      }

      tmpFlag = 0; /* Error flag to recover input file */
      list1_fp = fSafeOpen(fullArg[2], "r", 0/*noVersioningFlag*/);
      if (list1_fp == NULL) {
        /* Couldn't open it (error msg was provided)*/
        continue;
      }
      fclose(list1_fp);
      /* This will rename the input mmrecent.html as mmrecent.html~1 */
      list2_fp = fSafeOpen(fullArg[2], "w", 0/*noVersioningFlag*/);
      if (list2_fp == NULL) {
          /* Couldn't open it (error msg was provided)*/
        continue;
      }
      /* Note: in older versions the "~1" string was OS-dependent, but we
         don't support VAX or THINK C anymore...  Anyway we reopen it
         here with the renamed file in case the OS won't let us rename
         an opened file during the fSafeOpen for write above. */
      list1_fp = fSafeOpen(cat(fullArg[2], "~1", NULL), "r",
          0/*noVersioningFlag*/);
      if (list1_fp == NULL) bug(1117);

      /* Transfer the input file up to the special "<!-- #START# -->" comment */
      while (1) {
        if (!linput(list1_fp, NULL, &str1)) {
          print2(
"?Error: Could not find \"<!-- #START# -->\" line in input file \"%s\".\n",
              fullArg[2]);
          tmpFlag = 1; /* Error flag to recover input file */
          break;
        }
        /* 13-May-04 nm Put in "last updated" stamp */
        if (!strcmp(left(str1, 21), "<!-- last updated -->")) {
          let(&str1, cat(left(str1, 21), " <I>Last updated on ", date(),
          /* ??Future: make "EDT"/"EST" or other automatic */
          /*  " at ", time_(), " EST.</I>", NULL)); */
          /* 10-Nov-04 nm Just make it "ET" for "Eastern Time" */
            " at ", time_(), " ET.</I>", NULL));
        }
        fprintf(list2_fp, "%s\n", str1);
        if (!strcmp(str1, "<!-- #START# -->")) break;
      }
      if (tmpFlag) goto wrrecent_error;

      /* Get and parse today's date */
      parseDate(date(), &k /*dd*/, &l /*mmm*/, &m /*yyyy*/); /* 4-Nov-2015 */

      /************ Deleted 4-Nov-2015
      let(&str1, date());
      j = instr(1, str1, "-");
      k = (long)val(left(str1, j - 1)); /@ Day @/
#define MONTHS "JanFebMarAprMayJunJulAugSepOctNovDec"
      l = ((instr(1, MONTHS, mid(str1, j + 1, 3)) - 1) / 3) + 1; /@ 1 = Jan @/
      m = str1[j + 6]; /@ Character after 2-digit year @/  /@ nm 10-Apr-06 @/
      if (m == ' ' || m == ']') {                          /@ nm 10-Apr-06 @/
        /@ Handle 2-digit year @/
        m = (long)val(mid(str1, j + 5, 2));  /@ Year @/
#define START_YEAR 93 /@ Earliest 19xx year in set.mm database @/
        if (m < START_YEAR) {
          m = m + 2000;
        } else {
          m = m + 1900;
        }
      } else {                                            /@ nm 10-Apr-06 @/
        /@ Handle 4-digit year @/                         /@ nm 10-Apr-06 @/
        m = (long)val(mid(str1, j + 5, 4));  /@ Year @/         /@ nm 10-Apr-06 @/
      }                                                   /@ nm 10-Apr-06 @/
      *********** end of 4-Nov-2015 deletion */

#define START_YEAR 93 /* Earliest 19xx year in set.mm database */
      n = 0; /* Count of how many output so far */
      while (n < i /*RECENT_COUNT*/ && m > START_YEAR + 1900 - 1) {

        /* Build date string to match */
        /* 4-Nov-2015 nm */
        /*buildDate(k, l, m, &str5);*/
        buildDate(k, l, m, &str1); /* 2-May-2017 nm */

        /***** Deleted 2-May-2017 nm - we no longer match date below proof
        let(&str5, cat("$([", str5, "]$)", NULL));
        /@ 2-digit year is obsolete, but keep for backwards compatibility @/
        let(&str1, cat(left(str5, (long)strlen(str5) - 7),
            right(str5, (long)strlen(str5) - 4), NULL));
        *******/

        /************ Deleted 4-Nov-2015
#define MONTHS "JanFebMarAprMayJunJulAugSepOctNovDec"
        /@ Match for 2-digit year OBSOLETE @/
        let(&str1, cat("$([", str((double)k), "-", mid(MONTHS, 3 @ l - 2, 3), "-",
            right(str((double)m), 3), "]$)", NULL));
        /@ Match for 4-digit year @/  /@ nm 10-Apr-06 @/
        let(&str5, cat("$([", str((double)k), "-", mid(MONTHS, 3 @ l - 2, 3), "-",
            str((double)m), "]$)", NULL));
        *********** end of 4-Nov-2015 deletion */

        for (stmt = statements; stmt >= 1; stmt--) {

          if (statement[stmt].type != (char)p_
                && statement[stmt].type != (char)a_) {
            continue;
          }

          /******** Deleted 2-May-2017 nm
          /@ Get the comment section after the statement @/
          let(&str2, space(statement[stmt + 1].labelSectionLen));
          memcpy(str2, statement[stmt + 1].labelSectionPtr,
              (size_t)(statement[stmt + 1].labelSectionLen));
          p = instr(1, str2, "$)");
          let(&str2, left(str2, p + 1)); /@ Get 1st comment (if any) @/
          let(&str2, edit(str2, 2)); /@ Discard spaces @/
          ******** end of 2-May-2017 deletion */
          /* 2-May-2017 nm */
          /******* Deleted 3-May-2017 nm
          /@ In the call below, str3 is a dummy variable for a placeholder
             (its value will be undefined because of multiple occurrences) @/
          getContrib(stmt/@stmtNum@/, &str3, &str3, &str3, &str3, &str3, &str3,
              &str2, /@mostRecentDate@/
              0, /@printErrorsFlag@/
              1); /@normal mode@/
          **********/
          /* 3-May-2017 nm */
          let(&str2, "");
          str2 = getContrib(stmt/*stmtNum*/, MOST_RECENT_DATE);

          /* See if the date comment matches */
          /*if (instr(1, str2, str1) || instr(1, str2, str5)) {*/ /* 10-Apr-06 */
          if (!strcmp(str2, str1)) { /* 2-May-2017 nm */
            /* We have a match, so increment the match count */
            n++;
            let(&str3, "");
            str3 = getDescription(stmt);
            let(&str4, "");
            str4 = pinkHTML(stmt); /* Get little pink number */
            /* Output the description comment */
            /* Break up long lines for text editors with printLongLine */
            let(&printString, "");
            outputToString = 1;
            print2("\n"); /* Blank line for HTML human readability */
            printLongLine(cat(

                /*
                (stmt < extHtmlStmt) ?
                     "<TR>" :
                     cat("<TR BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL),
                */

                /* 29-Jul-2008 nm Sandbox stuff */
                (stmt < extHtmlStmt)
                   ? "<TR>"
                   : (stmt < sandboxStmt)
                       ? cat("<TR BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">",
                           NULL)
                       : cat("<TR BGCOLOR=", SANDBOX_COLOR, ">", NULL),

                "<TD NOWRAP>",  /* IE breaks up the date */
                /* mid(str1, 4, (long)strlen(str1) - 6), */ /* Date */
                /* Use 4-digit year */   /* 10-Apr-06 */
                /* mid(str5, 4, (long)strlen(str5) - 6), */ /* Date */ /* 10-Apr-06 */
                str2, /* Date */ /* 2-May-2017 nm */
                "</TD><TD ALIGN=CENTER><A HREF=\"",
                statement[stmt].labelName, ".html\">",
                statement[stmt].labelName, "</A>",
                str4, "</TD><TD ALIGN=LEFT>", NULL),  /* Description */
                            /* 28-Dec-05 nm Added ALIGN=LEFT for IE */
              " ",  /* Start continuation line with space */
              "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

            showStatement = stmt; /* For printTexComment */
            outputToString = 0; /* For printTexComment */
            texFilePtr = list2_fp;
            /* 18-Sep-03 ???Future - make this just return a string??? */
            /* printTexComment(str3, 0); */
            /* 17-Nov-2015 nm Added 3rd & 4th arguments */
            printTexComment(str3,              /* Sends result to texFilePtr */
                0, /* 1 = htmlCenterFlag */
                PROCESS_EVERYTHING, /* actionBits */ /* 13-Dec-2018 nm */
                0  /* 1 = noFileCheck */ );
            texFilePtr = NULL;
            outputToString = 1; /* Restore after printTexComment */

            /* Get HTML hypotheses => assertion */
            let(&str4, "");
            str4 = getTexOrHtmlHypAndAssertion(stmt); /* In mmwtex.c */
            printLongLine(cat("</TD></TR><TR",

                  /*
                  (s < extHtmlStmt) ?
                       ">" :
                       cat(" BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL),
                  */

                  /* 29-Jul-2008 nm Sandbox stuff */
                  (stmt < extHtmlStmt)
                     ? ">"
                     : (stmt < sandboxStmt)
                         ? cat(" BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">",
                             NULL)
                         : cat(" BGCOLOR=", SANDBOX_COLOR, ">", NULL),

                /*** old
                "<TD BGCOLOR=white>&nbsp;</TD><TD COLSPAN=2 ALIGN=CENTER>",
                str4, "</TD></TR>", NULL),
                ****/
                /* 27-Oct-03 nm */
                "<TD COLSPAN=3 ALIGN=CENTER>",
                str4, "</TD></TR>", NULL),

                " ",  /* Start continuation line with space */
                "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

            outputToString = 0;
            fprintf(list2_fp, "%s", printString);
            let(&printString, "");

            if (n >= i /*RECENT_COUNT*/) break; /* We're done */

            /* 27-Oct-03 nm Put separator row if not last theorem */
            outputToString = 1;
            printLongLine(cat("<TR BGCOLOR=white><TD COLSPAN=3>",
                "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>", NULL),
                " ",  /* Start continuation line with space */
                "\""); /* Don't break inside quotes e.g. "Arial Narrow" */

            /* 29-Jul-04 nm Put the previous, current, and next statement
               labels in HTML comments so a script can use them to update
               web site incrementally.  This would be done by searching
               for "For script" and gather label between = and --> then
               regenerate just those statements.  Previous and next labels
               are included to prevent dead links if they don't exist yet. */
            /* This section can be deleted without side effects */
            /* Find the previous statement with a web page */
            j = 0;
            for (q = stmt - 1; q >= 1; q--) {
              if (statement[q].type == (char)p_ ||
                  statement[q].type == (char)a_ ) {
                j = q;
                break;
              }
            }
            /* 13-Dec-2018 nm This isn't used anywhere yet.  But fix error
               in current label and also identify previous, current, next */
            if (j) print2("<!-- For script: previous = %s -->\n",
                statement[j].labelName);
            /* Current statement */
            print2("<!-- For script: current = %s -->\n",
                statement[stmt].labelName);
            /* Find the next statement with a web page */
            j = 0;
            for (q = stmt + 1; q <= statements; q++) {
              if (statement[q].type == (char)p_ ||
                  statement[q].type == (char)a_ ) {
                j = q;
                break;
              }
            }
            if (j) print2("<!-- For script: next = %s -->\n",
                statement[j].labelName);
            /* End of 29-Jul-04 section */

            outputToString = 0;
            fprintf(list2_fp, "%s", printString);
            let(&printString, "");

          }
        } /* Next stmt - statement number */
        /* Decrement date */
        if (k > 1) {
          k--; /* Decrement day */
        } else {
          k = 31; /* Non-existent day 31's will never match, which is OK */
          if (l > 1) {
            l--; /* Decrement month */
          } else {
            l = 12; /* Dec */
            m --; /* Decrement year */
          }
        }
      } /* next while - Scan next date */


      /* Discard the input file up to the special "<!-- #END# -->" comment */
      while (1) {
        if (!linput(list1_fp, NULL, &str1)) {
          print2(
"?Error: Could not find \"<!-- #END# -->\" line in input file \"%s\".\n",
              fullArg[2]);
          tmpFlag = 1; /* Error flag to recover input file */
          break;
        }
        if (!strcmp(str1, "<!-- #END# -->")) {
          fprintf(list2_fp, "%s\n", str1);
          break;
        }
      }
      if (tmpFlag) goto wrrecent_error;

      /* Transfer the rest of the input file */
      while (1) {
        if (!linput(list1_fp, NULL, &str1)) {
          break;
        }

        /* Update the date stamp at the bottom of the HTML page. */
        /* This is just a nicety; no error check is done. */
        if (!strcmp("This page was last updated on ", left(str1, 30))) {
          let(&str1, cat(left(str1, 30), date(), ".", NULL));
        }

        fprintf(list2_fp, "%s\n", str1);
      }

      print2("The %ld most recent theorem(s) were written.\n", n);

     wrrecent_error:
      fclose(list1_fp);
      fclose(list2_fp);
      if (tmpFlag) {
        /* Recover input files in case of error */
        remove(fullArg[2]);  /* Delete output file */
        rename(cat(fullArg[2], "~1", NULL), fullArg[2]);
            /* Restore input file name */
        print2("?The file \"%s\" was not modified.\n", fullArg[2]);
      }
      continue;
    }  /* End of "WRITE RECENT_ADDITIONS" */


    if (cmdMatches("SHOW LABELS")) {
      linearFlag = 0;
      if (switchPos("/ LINEAR")) linearFlag = 1;
      if (switchPos("/ ALL")) {
        m = 1;  /* Include $e, $f statements */
        print2(
"The labels that match are shown with statement number, label, and type.\n");
      } else {
        m = 0;  /* Show $a, $p only */
        print2(
"The assertions that match are shown with statement number, label, and type.\n");
      }
      j = 0;
      k = 0;
      let(&str2, ""); /* Line so far */
#define COL 20 /* Column width */
#define MIN_SPACE 2 /* At least this many spaces between columns */
      for (i = 1; i <= statements; i++) {
        if (!statement[i].labelName[0]) continue; /* No label */
        if (!m && statement[i].type != (char)p_ &&
            statement[i].type != (char)a_) continue; /* No /ALL switch */
        /* 30-Jan-06 nm Added single-character-match wildcard argument */
        if (!matchesList(statement[i].labelName, fullArg[2], '*', '?')) {
          continue;
        }

        /* 2-Oct-2015 nm */
        let(&str1, cat(str((double)i), " ",
            statement[i].labelName, " $", chr(statement[i].type),
            NULL));
        if (!str2[0]) {
          j = 0; /* # of fields on line so far */
        }
        k = ((long)strlen(str2) + MIN_SPACE > j * COL)
            ? (long)strlen(str2) + MIN_SPACE : j * COL;
                /* Position before new str1 starts */
        if (k + (long)strlen(str1) > screenWidth || linearFlag) {
          if (j == 0) {
            /* In case of huge label, force it out anyway */
            printLongLine(str1, "", " ");
          } else {
            /* Line width exceeded, postpone adding str1 */
            print2("%s\n", str2);
            let(&str2, str1);
            j = 1;
          }
        } else {
          /* Add new field to line */
          if (j == 0) {
            let(&str2, str1); /* Don't put space before 1st label on line */
          } else {
            let(&str2, cat(str2, space(k - (long)strlen(str2)), str1, NULL));
          }
          j++;
        }

        /* 2-Oct-2015 nm Deleted
        let(&str1,cat(str((double)i)," ",
            statement[i].labelName," $",chr(statement[i].type)," ",NULL));
#define COL 19 /@ Characters per column @/
        if (j + (long)strlen(str1) > MAX_LEN
            || (linearFlag && j != 0)) { /@ j != 0 to suppress 1st CR @/
          print2("\n");
          j = 0;
          k = 0;
        }
        if (strlen(str1) > COL || linearFlag) {
          j = j + (long)strlen(str1);
          k = k + (long)strlen(str1) - COL;
          print2(str1);
        } else {
          if (k == 0) {
            j = j + COL;
            print2("%s%s",str1,space(COL - (long)strlen(str1)));
          } else {
            k = k - (COL - (long)strlen(str1));
            if (k > 0) {
              print2(str1);
              j = j + (long)strlen(str1);
            } else {
              print2("%s%s",str1,space(COL - (long)strlen(str1)));
              j = j + COL;
              k = 0;
            }
          }
        }
        */

      } /* next i */
      /* print2("\n"); */
      if (str2[0]) {
        print2("%s\n", str2);
        let(&str2, "");
      }
      let(&str1, "");
      continue;
    }

    if (cmdMatches("SHOW DISCOURAGED")) {  /* was SHOW RESTRICTED */
      showDiscouraged(); /* In mmcmds.c */
      continue;
    }

    if (cmdMatches("SHOW SOURCE")) {

      /* 14-Sep-2012 nm */
      /* Currently, SHOW SOURCE only handles one statement at a time,
         so use getStatementNum().  Eventually, SHOW SOURCE may become
         obsolete; I don't think anyone uses it. */
      s = getStatementNum(fullArg[2],
          1/*startStmt*/,
          statements + 1  /*maxStmt*/,
          1/*aAllowed*/,
          1/*pAllowed*/,
          1/*eAllowed*/,
          1/*fAllowed*/,
          0/*efOnlyForMaxStmt*/,
          1/*uniqueFlag*/);
      if (s == -1) {
        continue; /* Error msg was provided */
      }
      showStatement = s; /* Update for future defaults */

      /*********** 14-Sep-2012 replaced by getStatementNum()
      for (i = 1; i <= statements; i++) {
        if (!strcmp(fullArg[2],statement[i].labelName)) break;
      }
      if (i > statements) {
        printLongLine(cat("?There is no statement with label \"",
            fullArg[2], "\".  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        showStatement = 0;
        continue;
      }
      showStatement = i;
      ************** end 14-Sep-2012 *******/

      let(&str1, "");
      str1 = outputStatement(showStatement, /*0, 3-May-2017 */ /* cleanFlag */
          0 /* reformatFlag */);
      let(&str1,edit(str1,128)); /* Trim trailing spaces */
      if (str1[strlen(str1)-1] == '\n') let(&str1, left(str1,
          (long)strlen(str1) - 1));
      printLongLine(str1, "", "");
      let(&str1,""); /* Deallocate vstring */
      continue;
    } /* if (cmdMatches("SHOW SOURCE")) */


    if (cmdMatches("SHOW STATEMENT") && (
        switchPos("/ HTML")
        || switchPos("/ BRIEF_HTML")
        || switchPos("/ ALT_HTML")
        || switchPos("/ BRIEF_ALT_HTML"))) {
      /* Special processing for the / HTML qualifiers - for each matching
         statement, a .html file is opened, the statement is output,
         and depending on statement type a proof or other information
         is output. */
      /* if (rawArgs != 5) { */  /* obsolete */

      /* 16-Aug-2016 nm */
      i = 5;  /* # arguments with only / HTML or / ALT_HTML */
      if (switchPos("/ NO_VERSIONING")) i = i + 2;
      if (switchPos("/ TIME")) i = i + 2;
      if (rawArgs != i) {
        printLongLine(cat("?The HTML qualifiers may not be combined with",
            " others except / NO_VERSIONING and / TIME.\n", NULL), "  ", " ");
        continue;
      }

      /* 16-Aug-2016 nm */
      printTime = 0;
      if (switchPos("/ TIME") != 0) {
        printTime = 1;
      }

      /*** 17-Nov-2014 nm This restriction has been removed.
      if (texDefsRead) {
        /@ Current limitation - can only read def's from .mm file once @/
        if (!htmlFlag) {
          print2("?You cannot use both LaTeX and HTML in the same session.\n");
          print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
          goto htmlDone;
        } else {
          if ((switchPos("/ ALT_HTML") || switchPos("/ BRIEF_ALT_HTML"))
              == (altHtmlFlag == 0)) {
            print2(
              "?You cannot use both HTML and ALT_HTML in the same session.\n");
            print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
            goto htmlDone;
          }
        }
      }
      *** End 17-Nov-2014 deletion */

      htmlFlag = 1; /* 17-Nov-2015 nm */

      if (switchPos("/ BRIEF_HTML") || switchPos("/ BRIEF_ALT_HTML")) {
        if (strcmp(fullArg[2], "*")) {
          print2(
              "?For BRIEF_HTML or BRIEF_ALT_HTML, the label must be \"*\"\n");
          goto htmlDone;
        }
        briefHtmlFlag = 1;
      } else {
        briefHtmlFlag = 0;
      }

      if (switchPos("/ ALT_HTML") || switchPos("/ BRIEF_ALT_HTML")) {
        altHtmlFlag = 1;
      } else {
        altHtmlFlag = 0;
      }

      q = 0;

      /* Special feature:  if the match statement starts with "*", we
         will also output mmascii.html, mmtheoremsall.html, and
         mmdefinitions.html.  So, with
                 SHOW STATEMENT * / HTML
         these will be output plus all statements; with
                 SHOW STATEMENT *! / HTML
         these will be output with no statements (since ! is illegal in a
         statement label); with
                 SHOW STATEMENT ?* / HTML
         all statements will be output, but without mmascii.html etc. */
      /* if (instr(1, fullArg[2], "*") || briefHtmlFlag) { */  /* obsolete */
      if (((char *)(fullArg[2]))[0] == '*' || briefHtmlFlag) {
                                                            /* 6-Jul-2008 nm */
        s = -2; /* -2 is for ASCII table; -1 is for theorems;
                    0 is for definitions */
      } else {
        s = 1;
      }

      for (s = s + 0; s <= statements; s++) {

        if (s > 0 && briefHtmlFlag) break; /* Only do summaries */

        /*
           s = -2:  mmascii.html
           s = -1:  mmtheoremsall.html (used to be mmtheorems.html)
           s = 0:   mmdefinitions.html
           s > 0:   normal statement
        */

        if (s > 0) {
          if (!statement[s].labelName[0]) continue; /* No label */
          /* 30-Jan-06 nm Added single-character-match wildcard argument */
          if (!matchesList(statement[s].labelName, fullArg[2], '*', '?'))
            continue;
          if (statement[s].type != (char)a_
              && statement[s].type != (char)p_) continue;
        }

        q = 1; /* Flag that at least one matching statement was found */

        if (s > 0) {
          showStatement = s;
        } else {
          /* We set it to 1 here so we will output the Metamath Proof
             Explorer and not the Hilbert Space Explorer header for
             definitions and theorems lists, when showStatement is
             compared to extHtmlStmt in printTexHeader in mmwtex.c */
          showStatement = 1;
        }


        /*** Open the html file ***/
        htmlFlag = 1;
        /* Open the html output file */
        switch (s) {
          case -2:
            let(&texFileName, "mmascii.html");
            break;
          case -1:
            let(&texFileName, "mmtheoremsall.html");
            break;
          case 0:
            let(&texFileName, "mmdefinitions.html");
            break;
          default:
            let(&texFileName, cat(statement[showStatement].labelName, ".html",
                NULL));
        }
        print2("Creating HTML file \"%s\"...\n", texFileName);
        if (switchPos("/ NO_VERSIONING") == 0) {
          texFilePtr = fSafeOpen(texFileName, "w", 0/*noVersioningFlag*/);
        } else {
          /* 6-Jul-2008 nm Added / NO_VERSIONING */
          /* Don't create the backup versions ~1, ~2,... */
          texFilePtr = fopen(texFileName, "w");
          if (!texFilePtr) print2("?Could not open the file \"%s\".\n",
              texFileName);
        }
        if (!texFilePtr) goto htmlDone; /* Couldn't open it (err msg was
            provided) */
        texFileOpenFlag = 1;
        printTexHeader((s > 0) ? 1 : 0 /*texHeaderFlag*/);
        if (!texDefsRead) {
          /* 9/6/03 If there was an error reading the $t xx.mm statement,
             texDefsRead won't be set, and we should close out file and skip
             further processing.  Otherwise we will be attempting to process
             uninitialized htmldef arrays and such. */
          print2("?HTML generation was aborted due to the error above.\n");
          s = statements + 1; /* To force loop to exit */
          goto ABORT_S; /* Go to end of loop where file is closed out */
        }

        if (s <= 0) {
          outputToString = 1;
          if (s == -2) {
            printLongLine(cat("<CENTER><FONT COLOR=", GREEN_TITLE_COLOR,
                "><B>",
                "Symbol to ASCII Correspondence for Text-Only Browsers",
                " (in order of appearance in $c and $v statements",
                     " in the database)",
                "</B></FONT></CENTER><P>", NULL), "", "\"");
          }
          /* 13-Oct-2006 nm todo - </CENTER> still appears - where is it? */
          if (!briefHtmlFlag) print2("<CENTER>\n");
          print2("<TABLE BORDER CELLSPACING=0 BGCOLOR=%s\n",
              MINT_BACKGROUND_COLOR);
          /* For bobby.cast.org approval */
          switch (s) {
            case -2:
              print2("SUMMARY=\"Symbol to ASCII correspondences\">\n");
              break;
            case -1:
              print2("SUMMARY=\"List of theorems\">\n");
              break;
            case 0:
              print2("SUMMARY=\"List of syntax, axioms and definitions\">\n");
              break;
          }
          switch (s) {
            case -2:
              print2("<TR ALIGN=LEFT><TD><B>\n");
              break;
            case -1:
              print2(
         "<CAPTION><B>List of Theorems</B></CAPTION><TR ALIGN=LEFT><TD><B>\n");
              break;
            case 0:
              printLongLine(cat(
/*"<CAPTION><B>List of Syntax (not <FONT COLOR=\"#00CC00\">|-&nbsp;</FONT>), ",*/
                  /* 2/9/02 (in case |- suppressed) */
                  "<CAPTION><B>List of Syntax, ",
                  "Axioms (<FONT COLOR=", GREEN_TITLE_COLOR, ">ax-</FONT>) and",
                  " Definitions (<FONT COLOR=", GREEN_TITLE_COLOR,
                  ">df-</FONT>)", "</B></CAPTION><TR ALIGN=LEFT><TD><B>",
                  NULL), "", "\"");
              break;
          }
          switch (s) {
            case -2:
              print2("Symbol</B></TD><TD><B>ASCII\n");
              break;
            case -1:
              print2(
                  "Ref</B></TD><TD><B>Description\n");
              break;
            case 0:
              printLongLine(cat(
                  "Ref</B></TD><TD><B>",
                "Expression (see link for any distinct variable requirements)",
                NULL), "", "\"");
              break;
          }
          print2("</B></TD></TR>\n");
          m = 0; /* Statement number map */
          let(&str3, ""); /* For storing ASCII token list in s=-2 mode */
          let(&bgcolor, MINT_BACKGROUND_COLOR); /* 8-Aug-2008 nm Initialize */
          for (i = 1; i <= statements; i++) {

            /* 8-Aug-2008 nm Commented out: */
            /*
            if (i == extHtmlStmt && s != -2) {
              / * Print a row that identifies the start of the extended
                 database (e.g. Hilbert Space Explorer) * /
              printLongLine(cat(
                  "<TR><TD COLSPAN=2 ALIGN=CENTER><A NAME=\"startext\"></A>",
                  "The list of syntax, axioms (ax-) and definitions (df-) for",
                  " the <B><FONT COLOR=", GREEN_TITLE_COLOR, ">",
                  extHtmlTitle,
                  "</FONT></B> starts here</TD></TR>", NULL), "", "\"");
            }
            */

            /* 8-Aug-2008 nm */
            if (s != -2 && (i == extHtmlStmt || i == sandboxStmt)) {
              /* Print a row that identifies the start of the extended
                 database (e.g. Hilbert Space Explorer) or the user
                 sandboxes */
              if (i == extHtmlStmt) {
                let(&bgcolor, PURPLISH_BIBLIO_COLOR);
              } else {
                let(&bgcolor, SANDBOX_COLOR);
              }
              printLongLine(cat("<TR BGCOLOR=", bgcolor,
                  "><TD COLSPAN=2 ALIGN=CENTER><A NAME=\"startext\"></A>",
                  "The list of syntax, axioms (ax-) and definitions (df-) for",
                  " the <B><FONT COLOR=", GREEN_TITLE_COLOR, ">",
                  (i == extHtmlStmt) ?
                    extHtmlTitle :
                    /*"User Sandboxes",*/
                    /* 24-Jul-2009 nm Changed name of sandbox to "mathbox" */
                    "User Mathboxes",
                  "</FONT></B> starts here</TD></TR>", NULL), "", "\"");
            }

            if (statement[i].type == (char)p_ ||
                statement[i].type == (char)a_ ) m++;
            if ((s == -1 && statement[i].type != (char)p_)
                || (s == 0 && statement[i].type != (char)a_)
                || (s == -2 && statement[i].type != (char)c_
                    && statement[i].type != (char)v_)
                ) continue;
            switch (s) {
              case -2:
                /* Print symbol to ASCII table entry */
                /* It's a $c or $v statement, so each token generates a
                   table row */
                for (j = 0; j < statement[i].mathStringLen; j++) {
                  let(&str1, mathToken[(statement[i].mathString)[j]].tokenName);
                  /* Output each token only once in case of multiple decl. */
                  if (!instr(1, str3, cat(" ", str1, " ", NULL))) {
                    let(&str3, cat(str3, " ", str1, " ", NULL));
                    let(&str2, "");
                    str2 = tokenToTex(mathToken[(statement[i].mathString)[j]
                        ].tokenName, i/*stmt# for error msgs*/);
                    /* 2/9/02  Skip any tokens (such as |- in QL Explorer) that
                       may be suppressed */
                    if (!str2[0]) continue;
                    /* Convert special characters to HTML entities */
                    for (k = 0; k < (signed)(strlen(str1)); k++) {
                      if (str1[k] == '&') {
                        let(&str1, cat(left(str1, k), "&amp;",
                            right(str1, k + 2), NULL));
                        k = k + 4;
                      }
                      if (str1[k] == '<') {
                        let(&str1, cat(left(str1, k), "&lt;",
                            right(str1, k + 2), NULL));
                        k = k + 3;
                      }
                      if (str1[k] == '>') {
                        let(&str1, cat(left(str1, k), "&gt;",
                            right(str1, k + 2), NULL));
                        k = k + 3;
                      }
                    } /* next k */
                    printLongLine(cat("<TR ALIGN=LEFT><TD>",
                        (altHtmlFlag ? cat("<SPAN ", htmlFont, ">", NULL) : ""),
                                                           /* 14-Jan-2016 nm */
                        str2,
                        (altHtmlFlag ? "</SPAN>" : ""),    /* 14-Jan-2016 nm */
                        "&nbsp;", /* 10-Jan-2016 nm This will prevent a
                                     -4px shifted image from overlapping the
                                     lower border of the table cell */
                        "</TD><TD><TT>",
                        str1,
                        "</TT></TD></TR>", NULL), "", "\"");
                  }
                } /* next j */
                /* Close out the string now to prevent memory overflow */
                fprintf(texFilePtr, "%s", printString);
                let(&printString, "");
                break;
              case -1: /* Falls through to next case */
              case 0:
                /* Count the number of essential hypotheses k */
                /* Not needed anymore??? since getTexOrHtmlHypAndAssertion() */
                /*
                k = 0;
                j = nmbrLen(statement[i].reqHypList);
                for (n = 0; n < j; n++) {
                  if (statement[statement[i].reqHypList[n]].type
                      == (char)e_) {
                    k++;
                  }
                }
                */
                let(&str1, "");
                if (s == 0 || briefHtmlFlag) {
                  let(&str1, "");
                  /* 18-Sep-03 Get HTML hypotheses => assertion */
                  str1 = getTexOrHtmlHypAndAssertion(i);
                                                /* In mmwtex.c */
                  let(&str1, cat(str1, "</TD></TR>", NULL));
                }

                /* 13-Oct-2006 nm Made some changes to BRIEF_HTML/_ALT_HTML
                   to use its mmtheoremsall.html output for the Palm PDA */
                if (briefHtmlFlag) {
                  /* Get page number in mmtheorems*.html of WRITE THEOREMS */
                  k = ((statement[i].pinkNumber - 1) /
                      THEOREMS_PER_PAGE) + 1; /* Page # */
                  let(&str2, cat("<TR ALIGN=LEFT><TD ALIGN=LEFT>",
                      /*"<FONT COLOR=\"#FA8072\">",*/
                      "<FONT COLOR=ORANGE>",
                      str((double)(statement[i].pinkNumber)), "</FONT> ",
                      "<FONT COLOR=GREEN><A HREF=\"",
                      "mmtheorems", (k == 1) ? "" : str((double)k), ".html#",
                      statement[i].labelName,
                      "\">", statement[i].labelName,
                      "</A></FONT>", NULL));
                  let(&str1, cat(str2, " ", str1, NULL));
                } else {
                  /* Get little pink (or rainbow-colored) number */
                  let(&str4, "");
                  str4 = pinkHTML(i);
                  let(&str2, cat("<TR BGCOLOR=", bgcolor, /* 8-Aug-2008 nm */
                      " ALIGN=LEFT><TD><A HREF=\"",
                      statement[i].labelName,
                      ".html\">", statement[i].labelName,
                      "</A>", str4, NULL));
                  let(&str1, cat(str2, "</TD><TD>", str1, NULL));
                }
                /* End of 13-Oct-2006 changed section */

                print2("\n");  /* New line for HTML source readability */
                printLongLine(str1, "", "\"");

                if (s == 0 || briefHtmlFlag) {
                              /* Set s == 0 here for Web site version,
                                 s == s for symbol version of theorem list */
                  /* The below has been replaced by
                     getTexOrHtmlHypAndAssertion(i) above. */
                  /*printTexLongMath(statement[i].mathString, "", "", 0, 0);*/
                  /*outputToString = 1;*/ /* Is reset by printTexLongMath */
                } else {
                  /* Theorems are listed w/ description; otherwise file is too
                     big for convenience */
                  let(&str1, "");
                  str1 = getDescription(i);
                  if (strlen(str1) > 29)
                    let(&str1, cat(left(str1, 26), "...", NULL));
                  let(&str1, cat(str1, "</TD></TR>", NULL));
                  printLongLine(str1, "", "\"");
                }
                /* Close out the string now to prevent overflow */
                fprintf(texFilePtr, "%s", printString);
                let(&printString, "");
                break;
            } /* end switch */
          } /* next i (statement number) */
          /* print2("</TABLE></CENTER>\n"); */ /* 8/8/03 Removed - already
              done somewhere else, causing validator.w3.org to fail */
          outputToString = 0;  /* closing will write out the string */
          let(&bgcolor, ""); /* Deallocate (to improve fragmentation) */

        } else { /* s > 0 */

          /* 16-Aug-2016 nm */
          if (printTime == 1) {
            getRunTime(&timeIncr);  /* This call just resets the time */
          }

          /*** Output the html statement body ***/
          typeStatement(showStatement,
              0 /*briefFlag*/,
              0 /*commentOnlyFlag*/,
              1 /*texFlag*/,   /* means latex or html */
              1 /*htmlFlag*/);

          /* 16-Aug-2016 nm */
          if (printTime == 1) {
            getRunTime(&timeIncr);
            print2("SHOW STATEMENT run time = %6.2f sec for \"%s\"\n",
                timeIncr,
                texFileName);
          }

        } /* if s <= 0 */

       ABORT_S:
        /*** Close the html file ***/
        printTexTrailer(1 /*texHeaderFlag*/);
        fclose(texFilePtr);
        texFileOpenFlag = 0;
        let(&texFileName,"");

      } /* next s */

      if (!q) {
        /* No matching statement was found */
        printLongLine(cat("?There is no statement whose label matches \"",
            fullArg[2], "\".  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        continue;
      }

      /* Complete the command processing to bypass normal SHOW STATEMENT
         (non-html) below. */
     htmlDone:
      continue;
    } /* if (cmdMatches("SHOW STATEMENT") && switchPos("/ HTML")...) */


    /******* Section for / MNEMONICS added 12-May-2009 by Stefan Allan *****/
    /* Write mnemosyne.txt */
    if (cmdMatches("SHOW STATEMENT") && switchPos("/ MNEMONICS")) {

      /*** 17-Nov-2015 nm Deleted - removed htmlFlag, altHtmlFlag
            change prohibition
      if (!texDefsRead) {
        htmlFlag = 1;     /@ Use HTML, not TeX section @/
        altHtmlFlag = 1;  /@ Use Unicode, not GIF @/
        print2("Reading definitions from $t statement of %s...\n", input_fn);
        if (2/@error@/ == readTexDefs(0 /@ 1 = check errors only @/,
            0 /@ 1 = no GIF file existence check @/  )) {
          print2(
          "?There was an error in the $t comment's LaTeX/HTML definitions.\n");
          print2("?HTML generation was aborted due to the error above.\n");
          continue; /@ An error occurred @/
        }
      } else {
        /@ Current limitation - can only read def's from .mm file once @/
        if (!htmlFlag) {
          print2("?You cannot use both LaTeX and HTML in the same session.\n");
          print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
          continue;
        } else {
          if (!altHtmlFlag) {
            print2(
              "?You cannot use both HTML and ALT_HTML in the same session.\n");
            print2(
              "You must EXIT and restart Metamath to switch to the other.\n");
            continue;
          }
        }
      }
      *** end of 17-Nov-2015 deletion */

      htmlFlag = 1;     /* Use HTML, not TeX section */
      altHtmlFlag = 1;  /* Use Unicode, not GIF */
      /* readTexDefs() rereads based on changes to htmlFlag, altHtmlFlag */
      if (2/*error*/ == readTexDefs(0 /* 1 = check errors only */,
          0 /* 1 = no GIF file existence check */  )) {
        continue; /* An error occurred */
      }

      let(&texFileName,"mnemosyne.txt");
      texFilePtr = fSafeOpen(texFileName, "w", 0/*noVersioningFlag*/);
      if (!texFilePtr) {
        /* Couldn't open file; error message was provided by fSafeOpen */
        continue;
      }
      print2("Creating Mnemosyne file \"%s\"...\n", texFileName);

      for (s = 1; s <= statements; s++) {
        showStatement = s;
/*
        if (strcmp("|-", mathToken[
         (statement[showStatement].mathString)[0]].tokenName)) {
           subType = SYNTAX;
        }
*/
        if (!statement[s].labelName[0]) continue; /* No label */
        /* 30-Jan-06 nm Added single-character-match wildcard argument */
        if (!matchesList(statement[s].labelName, fullArg[2], '*', '?'))
          continue;
        if (statement[s].type != (char)a_
            && statement[s].type != (char)p_)
          continue;

        let(&str1, cat("<CENTER><B><FONT SIZE=\"+1\">",
            " <FONT COLOR=", GREEN_TITLE_COLOR,
            " SIZE = \"+3\">", statement[showStatement].labelName,
            "</FONT></FONT></B>", "</CENTER>", NULL));
        fprintf(texFilePtr, "%s", str1);

        let(&str1, cat("<TABLE>",NULL));
        fprintf(texFilePtr, "%s", str1);

        j = nmbrLen(statement[showStatement].reqHypList);
        for (i = 0; i < j; i++) {
          k = statement[showStatement].reqHypList[i];
          if (statement[k].type != (char)e_
              && !(subType == SYNTAX
              && statement[k].type == (char)f_))
            continue;

          let(&str1, cat("<TR ALIGN=LEFT><TD><FONT SIZE=\"+2\">",
              statement[k].labelName, "</FONT></TD><TD><FONT SIZE=\"+2\">",
              NULL));
          fprintf(texFilePtr, "%s", str1);

          /* Print hypothesis */
          let(&str1, ""); /* Free any previous allocation to str1 */
          /* getTexLongMath does not return a temporary allocation; must
             assign str1 directly, not with let().  It will be deallocated
             with the next let(&str1,...). */
          str1 = getTexLongMath(statement[k].mathString,
              k/*stmt# for err msgs*/);
          fprintf(texFilePtr, "%s</FONT></TD>", str1);
        }


        let(&str1, "</TABLE>");
        fprintf(texFilePtr, "%s", str1);

        let(&str1, "<BR><FONT SIZE=\"+2\">What is the conclusion?</FONT>");
        fprintf(texFilePtr, "%s\n", str1);

        let(&str1, "<FONT SIZE=\"+3\">");
        fprintf(texFilePtr, "%s", str1);

        let(&str1, ""); /* Free any previous allocation to str1 */
        /* getTexLongMath does not return a temporary allocation */
        str1 = getTexLongMath(statement[s].mathString, s);
        fprintf(texFilePtr, "%s", str1);

        let(&str1, "</FONT>");
        fprintf(texFilePtr, "%s\n",str1);
      } /*  for(s=1;s<statements;++s) */

      fclose(texFilePtr);
      texFileOpenFlag = 0;
      let(&texFileName,"");
      let(&str1,"");
      let(&str2,"");

      continue;
    } /* if (cmdMatches("SHOW STATEMENT") && switchPos("/ MNEMONICS")) */
    /** End of section for / MNEMONICS added 12-May-2009 by Stefan Allan *****/

    /* If we get here, the user did not specify one of the qualifiers /HTML,
       /BRIEF_HTML, /ALT_HTML, or /BRIEF_ALT_HTML */
    if (cmdMatches("SHOW STATEMENT") && !switchPos("/ HTML")) {

      texFlag = 0;
      /* 14-Sep-2010 nm Added OLD_TEX */
      if (switchPos("/ TEX") || switchPos("/ OLD_TEX")
          || switchPos("/ HTML"))
        texFlag = 1;

      briefFlag = 1;
      oldTexFlag = 0;
      if (switchPos("/ TEX")) briefFlag = 0;
      /* 14-Sep-2010 nm Added OLD_TEX */
      if (switchPos("/ OLD_TEX")) briefFlag = 0;
      if (switchPos("/ OLD_TEX")) oldTexFlag = 1;
      if (switchPos("/ FULL")) briefFlag = 0;

      commentOnlyFlag = 0;
      if (switchPos("/ COMMENT")) {
        commentOnlyFlag = 1;
        briefFlag = 1;
      }


      if (switchPos("/ FULL")) {
        briefFlag = 0;
        commentOnlyFlag = 0;
      }

      if (texFlag) {
        if (!texFileOpenFlag) {
          print2(
      "?You have not opened a %s file.  Use the OPEN TEX command first.\n",
              htmlFlag ? "HTML" : "LaTeX");
          continue;
        }
      }

      if (texFlag && (commentOnlyFlag || briefFlag)) {
        print2("?TEX qualifier should be used alone\n");
        continue;
      }

      q = 0;

      for (s = 1; s <= statements; s++) {
        if (!statement[s].labelName[0]) continue; /* No label */
        /* We are not in MM-PA mode, or the statement isn't "=" */
        /* 30-Jan-06 nm Added single-character-match wildcard argument */
        if (!matchesList(statement[s].labelName, fullArg[2], '*', '?'))
          continue;

        if (briefFlag || commentOnlyFlag || texFlag) {
          /* For brief or comment qualifier, if label has wildcards,
             show only $p and $a's */
          /* 30-Jan-06 nm Added single-character-match wildcard argument */
          if (statement[s].type != (char)p_
              && statement[s].type != (char)a_ && (instr(1, fullArg[2], "*")
                  || instr(1, fullArg[2], "?")))
            continue;
        }

        if (q && !texFlag) {
          if (!print2("%s\n", string(79, '-'))) /* Put line between
                                                   statements */
            break; /* Break for speedup if user quit */
        }
        if (texFlag) print2("Outputting statement \"%s\"...\n",
            statement[s].labelName);

        q = 1; /* Flag that at least one matching statement was found */

        showStatement = s;


        typeStatement(showStatement,
            briefFlag,
            commentOnlyFlag,
            texFlag,
            htmlFlag);
      } /* Next s */

      if (!q) {
        /* No matching statement was found */
        printLongLine(cat("?There is no statement whose label matches \"",
            fullArg[2], "\".  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        continue;
      }

      if (texFlag && !htmlFlag) {
        print2("The LaTeX source was written to \"%s\".\n", texFileName);
        /* 14-Sep-2010 nm Added OLD_TEX */
        oldTexFlag = 0;
      }
      continue;
    } /* (cmdMatches("SHOW STATEMENT") && !switchPos("/ HTML")) */

    if (cmdMatches("SHOW SETTINGS")) {
      print2("Metamath settings on %s at %s:\n",date(),time_());
      if (commandEcho) {
        print2("(SET ECHO...) Command ECHO is ON.\n");
      } else {
        print2("(SET ECHO...) Command ECHO is OFF.\n");
      }
      if (defaultScrollMode == 1) {
        print2("(SET SCROLL...) SCROLLing mode is PROMPTED.\n");
      } else {
        print2("(SET SCROLL...) SCROLLing mode is CONTINUOUS.\n");
      }
      print2("(SET WIDTH...) Screen display WIDTH is %ld.\n", screenWidth);
      print2("(SET HEIGHT...) Screen display HEIGHT is %ld.\n",
          screenHeight + 1);
      if (sourceHasBeenRead == 1) {
        print2("(READ...) %ld statements have been read from \"%s\".\n",
            statements, input_fn);
      } else {
        print2("(READ...) No source file has been read in yet.\n");
      }
      print2("(SET ROOT_DIRECTORY...) Root directory is \"%s\".\n",
          rootDirectory);
      print2(
     "(SET DISCOURAGEMENT...) Blocking based on \"discouraged\" tags is %s.\n",
          (globalDiscouragement ? "ON" : "OFF"));
      print2(
     "(SET CONTRIBUTOR...) The current contributor is \"%s\".\n",
          contributorName);
      if (PFASmode) {
        print2("(PROVE...) The statement you are proving is \"%s\".\n",
            statement[proveStatement].labelName);
      }
      print2("(SET UNDO...) The maximum number of UNDOs is %ld.\n",
          processUndoStack(NULL, PUS_GET_SIZE, "", 0));
      print2(
    "(SET UNIFICATION_TIMEOUT...) The unification timeout parameter is %ld.\n",
          userMaxUnifTrials);
      print2(
    "(SET SEARCH_LIMIT...) The SEARCH_LIMIT for the IMPROVE command is %ld.\n",
          userMaxProveFloat);
      if (minSubstLen) {
        print2(
     "(SET EMPTY_SUBSTITUTION...) EMPTY_SUBSTITUTION is not allowed (OFF).\n");
      } else {
        print2(
          "(SET EMPTY_SUBSTITUTION...) EMPTY_SUBSTITUTION is allowed (ON).\n");
      }
      if (hentyFilter) { /* 18-Nov-05 nm Added to the SHOW listing */
        print2(
              "(SET JEREMY_HENTY_FILTER...) The Henty filter is turned ON.\n");
      } else {
        print2(
             "(SET JEREMY_HENTY_FILTER...) The Henty filter is turned OFF.\n");
      }
      if (showStatement) {
        print2("(SHOW...) The default statement for SHOW commands is \"%s\".\n",
            statement[showStatement].labelName);
      }
      if (logFileOpenFlag) {
        print2("(OPEN LOG...) The log file \"%s\" is open.\n", logFileName);
      } else {
        print2("(OPEN LOG...) No log file is currently open.\n");
      }
      if (texFileOpenFlag) {
        print2("The %s file \"%s\" is open.\n", htmlFlag ? "HTML" : "LaTeX",
            texFileName);
      }
      /* 17-Nov-2015 nm */
      print2(
  "(SHOW STATEMENT.../[TEX,HTML,ALT_HTML]) Current output mode is %s.\n",
          htmlFlag
             ? (altHtmlFlag ? "ALT_HTML" : "HTML")
             : "TEX (LaTeX)");
      /* 21-Jun-2014 */
      print2("The program is compiled for a %ld-bit CPU.\n",
          (long)(8 * sizeof(long)));
      continue;
    }

    if (cmdMatches("SHOW MEMORY")) {
      /*print2("%ld bytes of data memory have been used.\n",db+db3);*/
      j = 32000000; /* The largest we'ed ever look for */
#ifdef THINK_C
      i = FreeMem();
#else
      i = getFreeSpace(j);
#endif
      if (i > j-3) {
        print2("At least %ld bytes of memory are free.\n",j);
      } else {
        print2("%ld bytes of memory are free.\n",i);
      }
      continue;
    }

    /* 21-Jun-2014 */
    if (cmdMatches("SHOW ELAPSED_TIME")) {
      timeTotal = getRunTime(&timeIncr);
      print2(
      "Time since last SHOW ELAPSED_TIME command = %6.2f s; total = %6.2f s\n",
          timeIncr, timeTotal);
      continue;
    } /* if (cmdMatches("SHOW ELAPSED_TIME")) */


    if (cmdMatches("SHOW TRACE_BACK")) {


      /* Pre-21-May-2008
        for (i = 1; i <= statements; i++) {
          if (!strcmp(fullArg[2],statement[i].labelName)) break;
        }
        if (i > statements) {
          printLongLine(cat("?There is no statement with label \"",
              fullArg[2], "\".  ",
              "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
          showStatement = 0;
          continue;
        }
       */

      essentialFlag = 0;
      axiomFlag = 0;
      endIndent = 0;
      i = switchPos("/ ESSENTIAL");
      if (i) essentialFlag = 1; /* Limit trace to essential steps only */
      i = switchPos("/ ALL");
      if (i) essentialFlag = 0;
      i = switchPos("/ AXIOMS");
      if (i) axiomFlag = 1; /* Limit trace printout to axioms */
      i = switchPos("/ DEPTH"); /* Limit depth of printout */
      if (i) endIndent = (long)val(fullArg[i + 1]);

       /* 19-May-2013 nm */
      i = switchPos("/ COUNT_STEPS");
      countStepsFlag = (i != 0 ? 1 : 0);
      i = switchPos("/ TREE");
      treeFlag = (i != 0 ? 1 : 0);
      i = switchPos("/ MATCH");
      matchFlag = (i != 0 ? 1 : 0);
      if (matchFlag) {
        let(&matchList, fullArg[i + 1]);
      } else {
        let(&matchList, "");
      }
      i = switchPos("/ TO");
      if (i != 0) {
        let(&traceToList, fullArg[i + 1]);
      } else {
        let(&traceToList, "");
      }
      if (treeFlag) {
        if (axiomFlag) {
          print2(
              "(Note:  The AXIOMS switch is ignored in TREE mode.)\n");
        }
        if (countStepsFlag) {
          print2(
              "(Note:  The COUNT_STEPS switch is ignored in TREE mode.)\n");
        }
        if (matchFlag) {
          print2(
  "(Note: The MATCH list is ignored in TREE mode.)\n");
        }
      } else {
        if (endIndent != 0) {
          print2(
  "(Note:  The DEPTH is ignored if the TREE switch is not used.)\n");
        }
        if (countStepsFlag) {
          if (matchFlag) {
            print2(
  "(Note: The MATCH list is ignored in COUNT_STEPS mode.)\n");
          }
        }
      }

      /* 21-May-2008 nm Added wildcard handling */
      showStatement = 0;
      for (i = 1; i <= statements; i++) {
        if (statement[i].type != (char)p_)
          continue; /* Not a $p statement; skip it */
        /* Wildcard matching */
        if (!matchesList(statement[i].labelName, fullArg[2], '*', '?'))
          continue;

        showStatement = i;
        /*** start of 19-May-2013 deletion
        j = switchPos("/ TREE");
        if (j) {
          if (axiomFlag) {
            print2(
                "(Note:  The AXIOMS switch is ignored in TREE mode.)\n");
          }
          if (switchPos("/ COUNT_STEPS")) {
            print2(
                "(Note:  The COUNT_STEPS switch is ignored in TREE mode.)\n");
          }
          traceProofTree(showStatement, essentialFlag, endIndent);
        } else {
          if (endIndent != 0) {
           print2(
"(Note:  The DEPTH is ignored if the TREE switch is not used.)\n");
          }

          j = switchPos("/ COUNT_STEPS");
          if (j) {
            countSteps(showStatement, essentialFlag);
          } else {
            traceProof(showStatement, essentialFlag, axiomFlag);
          }
        }
        *** end of 19-May-2013 deletion */


        /* 19-May-2013 nm - move /TREE and /COUNT_STEPS to outside loop,
           assigning new variables, for cleaner code. */
        if (treeFlag) {
          traceProofTree(showStatement, essentialFlag, endIndent);
        } else {
          if (countStepsFlag) {
            countSteps(showStatement, essentialFlag);
          } else {
            traceProof(showStatement,
                essentialFlag,
                axiomFlag,
                matchList, /* 19-May-2013 nm */
                traceToList, /* 18-Jul-2015 nm */
                0 /* testOnlyFlag */ /* 20-May-2013 nm */);
          }
        }

      /* 21-May-2008 nm Added wildcard handling */
      } /* next i */
      if (showStatement == 0) {
        printLongLine(cat("?There are no $p labels matching \"",
            fullArg[2], "\".  ",
            "See HELP SHOW TRACE_BACK for matching rules.", NULL), "", " ");
      }

      let(&matchList, ""); /* Deallocate memory */
      let(&traceToList, ""); /* Deallocate memory */
      continue;
    } /* if (cmdMatches("SHOW TRACE_BACK")) */


    if (cmdMatches("SHOW USAGE")) {

      /* 7-Jan-2008 nm Added / ALL qualifier */
      if (switchPos("/ ALL")) {
        m = 1;  /* Always include $e, $f statements */
      } else {
        m = 0;  /* If wildcards are used, show $a, $p only */
      }

      showStatement = 0;
      for (i = 1; i <= statements; i++) { /* 7-Jan-2008 */

        /* 7-Jan-2008 nm Added wildcard handling */
        if (!statement[i].labelName[0]) continue; /* No label */
        if (!m && statement[i].type != (char)p_ &&
            statement[i].type != (char)a_
            /* A wildcard-free user-specified statement is always matched even
               if it's a $e, i.e. it overrides omission of / ALL */
            && (instr(1, fullArg[2], "*")
              || instr(1, fullArg[2], "?")))
          continue; /* No /ALL switch and wildcard and not $p, $a */
        /* Wildcard matching */
        if (!matchesList(statement[i].labelName, fullArg[2], '*', '?'))
          continue;

        showStatement = i;
        recursiveFlag = 0;
        j = switchPos("/ RECURSIVE");
        if (j) recursiveFlag = 1; /* Recursive (indirect) usage */
        j = switchPos("/ DIRECT");
        if (j) recursiveFlag = 0; /* Direct references only */

        let(&str1, "");
        str1 = traceUsage(showStatement,
            recursiveFlag,
            0 /* cutoffStmt */);



        /************* 18-Jul-2015 nm Start of deleted code ************/
        /*
        /@ Count the number of statements = # of spaces @/
        k = (long)strlen(str1) - (long)strlen(edit(str1, 2));

        if (!k) {
          printLongLine(cat("Statement \"",
              statement[showStatement].labelName,
              "\" is not referenced in the proof of any statement.", NULL),
              "", " ");
        } else {
          if (recursiveFlag) {
            let(&str2, "\" directly or indirectly affects");
          } else {
            let(&str2, "\" is directly referenced in");
          }
          if (k == 1) {
            printLongLine(cat("Statement \"",
                statement[showStatement].labelName,
                str2, " the proof of ",
                str((double)k), " statement:", NULL), "", " ");
          } else {
            printLongLine(cat("Statement \"",
                statement[showStatement].labelName,
                str2, " the proofs of ",
                str((double)k), " statements:", NULL), "", " ");
          }
        }

        if (k) {
          let(&str1, cat(" ", str1, NULL));
        } else {
          let(&str1, "  (None)");
        }

        /@ Print the output @/
        printLongLine(str1, "  ", " ");
        */
        /********* 18-Jul-2015 nm End of deleted code ****************/


        /************* 18-Jul-2015 nm Start of new code ************/
        /* 18-Jul-2015 nm */
        /* str1[0] will be 'Y' or 'N' depending on whether there are any
           statements.  str1[i] will be 'Y' or 'N' depending on whether
           statement[i] uses showStatement. */
        /* Count the number of statements k = # of 'Y' */
        k = 0;
        if (str1[0] == 'Y') {
          /* There is at least one statement using showStatement */
          for (j = showStatement + 1; j <= statements; j++) {
            if (str1[j] == 'Y') {
              k++;
            } else {
              if (str1[j] != 'N') bug(1124); /* Must be 'Y' or 'N' */
            }
          }
        } else {
          if (str1[0] != 'N') bug(1125); /* Must be 'Y' or 'N' */
        }

        if (k == 0) {
          printLongLine(cat("Statement \"",
              statement[showStatement].labelName,
              "\" is not referenced in the proof of any statement.", NULL),
              "", " ");
        } else {
          if (recursiveFlag) {
            let(&str2, "\" directly or indirectly affects");
          } else {
            let(&str2, "\" is directly referenced in");
          }
          if (k == 1) {
            printLongLine(cat("Statement \"",
                statement[showStatement].labelName,
                str2, " the proof of ",
                str((double)k), " statement:", NULL), "", " ");
          } else {
            printLongLine(cat("Statement \"",
                statement[showStatement].labelName,
                str2, " the proofs of ",
                str((double)k), " statements:", NULL), "", " ");
          }
        }

        if (k != 0) {
          let(&str3, " "); /* Line buffer */
          for (j = showStatement + 1; j <= statements; j++) {
            if (str1[j] == 'Y') {
              /* Since the output list could be huge, don't build giant
                 string (very slow) but output it line by line */
              if ((long)strlen(str3) + 1 +
                  (long)strlen(statement[j].labelName) > screenWidth) {
                /* Output and reset the line buffer */
                print2("%s\n", str3);
                let(&str3, " ");
              }
              let(&str3, cat(str3, " ", statement[j].labelName, NULL));
            }
          }
          if (strlen(str3) > 1) print2("%s\n", str3);
          let(&str3, "");
        } else {
          print2("  (None)\n");
        } /* if (k != 0) */
        /********* 18-Jul-2015 nm End of new code ****************/


      } /* next i (statement matching wildcard list) */

      if (showStatement == 0) {
        printLongLine(cat("?There are no labels matching \"",
            fullArg[2], "\".  ",
            "See HELP SHOW USAGE for matching rules.", NULL), "", " ");
      }
      continue;
    } /* if cmdMatches("SHOW USAGE") */


    if (cmdMatches("SHOW PROOF")
        || cmdMatches("SHOW NEW_PROOF")
        || cmdMatches("SAVE PROOF")
        || cmdMatches("SAVE NEW_PROOF")
        || cmdMatches("MIDI")) {
      if (switchPos("/ HTML")) {
        print2("?HTML qualifier is obsolete - use SHOW STATEMENT * / HTML\n");
        continue;
      }

      /* 3-May-2016 nm */
      if (cmdMatches("SAVE NEW_PROOF")
          && getMarkupFlag(proveStatement, PROOF_DISCOURAGED)) {
        if (switchPos("/ OVERRIDE") == 0 && globalDiscouragement == 1) {
          /* print2("\n"); */ /* Enable for more emphasis */
          print2(
">>> ?Error: Attempt to overwrite a proof whose modification is discouraged.\n");
          print2(
   ">>> Use SAVE NEW_PROOF ... / OVERRIDE if you really want to do this.\n");
          /* print2("\n"); */ /* Enable for more emphasis */
          continue;
        } else {
          /* print2("\n"); */ /* Enable for more emphasis */
          print2(
">>> ?Warning: You are overwriting a proof whose modification is discouraged.\n");
          /* print2("\n"); */ /* Enable for more emphasis */
        }
      }

      if (cmdMatches("SHOW PROOF") || cmdMatches("SAVE PROOF")) {
        pipFlag = 0;
      } else {
        pipFlag = 1; /* Proof-in-progress (NEW_PROOF) flag */
      }
      if (cmdMatches("SHOW")) {
        saveFlag = 0;
      } else {
        saveFlag = 1; /* The command is SAVE PROOF */
      }

      /* 16-Aug-2016 nm */
      printTime = 0;
      if (switchPos("/ TIME") != 0) {  /* / TIME legal in SAVE mode only */
        printTime = 1;
      }

      /* 27-Dec-2013 nm */
      i = switchPos("/ OLD_COMPRESSION");
      if (i) {
        if (!switchPos("/ COMPRESSED")) {
          print2("?/ OLD_COMPRESSION must be accompanied by / COMPRESSED.\n");
          continue;
        }
      }

      /* 2-Mar-2016 nm */
      i = switchPos("/ FAST");
      if (i) {
        if (!switchPos("/ COMPRESSED") && !switchPos("/ PACKED")) {
          print2("?/ FAST must be accompanied by / COMPRESSED or / PACKED.\n");
          continue;
        }
        fastFlag = 1;
      } else {
        fastFlag = 0;
      }

      /* 2-Mar-2016 nm */
      if (switchPos("/ EXPLICIT")) {
        if (switchPos("/ COMPRESSED")) {
          print2("?/ COMPRESSED and / EXPLICIT may not be used together.\n");
          continue;
        } else if (switchPos("/ NORMAL")) {
          print2("?/ NORMAL and / EXPLICIT may not be used together.\n");
          continue;
        }
      }
      if (switchPos("/ NORMAL")) {
        if (switchPos("/ COMPRESSED")) {
          print2("?/ NORMAL and / COMPRESSED may not be used together.\n");
          continue;
        }
      }


      /* Establish defaults for omitted qualifiers */
      startStep = 0;
      endStep = 0;
      /* startIndent = 0; */ /* Not used */
      endIndent = 0;
      /*essentialFlag = 0;*/
      essentialFlag = 1; /* 10/9/99 - friendlier default */
      renumberFlag = 0;
      unknownFlag = 0;
      notUnifiedFlag = 0;
      reverseFlag = 0;
      detailStep = 0;
      noIndentFlag = 0;
      splitColumn = DEFAULT_COLUMN;
      skipRepeatedSteps = 0; /* 28-Jun-2013 nm */
      texFlag = 0;

      i = switchPos("/ FROM_STEP");
      if (i) startStep = (long)val(fullArg[i + 1]);
      i = switchPos("/ TO_STEP");
      if (i) endStep = (long)val(fullArg[i + 1]);
      i = switchPos("/ DEPTH");
      if (i) endIndent = (long)val(fullArg[i + 1]);
      /* 10/9/99 - ESSENTIAL is retained for downwards compatibility, but is
         now the default, so we ignore it. */
      /*
      i = switchPos("/ ESSENTIAL");
      if (i) essentialFlag = 1;
      */
      i = switchPos("/ ALL");
      if (i) essentialFlag = 0;
      if (i && switchPos("/ ESSENTIAL")) {
        print2("?You may not specify both / ESSENTIAL and / ALL.\n");
        continue;
      }
      i = switchPos("/ RENUMBER");
      if (i) renumberFlag = 1;
      i = switchPos("/ UNKNOWN");
      if (i) unknownFlag = 1;
      i = switchPos("/ NOT_UNIFIED"); /* pip mode only */
      if (i) notUnifiedFlag = 1;
      i = switchPos("/ REVERSE");
      if (i) reverseFlag = 1;
      i = switchPos("/ LEMMON");
      if (i) noIndentFlag = 1;
      i = switchPos("/ START_COLUMN");
      if (i) splitColumn = (long)val(fullArg[i + 1]);
      i = switchPos("/ NO_REPEATED_STEPS"); /* 28-Jun-2013 nm */
      if (i) skipRepeatedSteps = 1;         /* 28-Jun-2013 nm */

      /* 8-Dec-2018 nm */
      /* If NO_REPEATED_STEPS is specified, indentation (tree) mode will be
         misleading because a hypothesis assignment will be suppressed if the
         same assignment occurred earlier, i.e. it is no longer a "tree". */
      if (skipRepeatedSteps == 1 && noIndentFlag == 0) {
        print2("?You must specify / LEMMON with / NO_REPEATED_STEPS\n");
        continue;
      }

      i = switchPos("/ TEX") || switchPos("/ HTML")
          /* 14-Sep-2010 nm Added OLDE_TEX */
          || switchPos("/ OLD_TEX");
      if (i) texFlag = 1;

      /* 14-Sep-2010 nm Added OLD_TEX */
      oldTexFlag = 0;
      if (switchPos("/ OLD_TEX")) oldTexFlag = 1;

      if (cmdMatches("MIDI")) { /* 8/28/00 */
        midiFlag = 1;
        pipFlag = 0;
        saveFlag = 0;
        let(&labelMatch, fullArg[1]);
        i = switchPos("/ PARAMETER"); /* MIDI only */
        if (i) {
          let(&midiParam, fullArg[i + 1]);
        } else {
          let(&midiParam, "");
        }
      } else {
        midiFlag = 0;
        if (!pipFlag) let(&labelMatch, fullArg[2]);
      }


      if (texFlag) {
        if (!texFileOpenFlag) {
          print2(
     "?You have not opened a %s file.  Use the OPEN %s command first.\n",
              htmlFlag ? "HTML" : "LaTeX",
              htmlFlag ? "HTML" : "TEX");
          continue;
        }
        /**** this is now done after outputting
        print2("The %s source was written to \"%s\".\n",
            htmlFlag ? "HTML" : "LaTeX", texFileName);
        */
      }

      i = switchPos("/ DETAILED_STEP"); /* non-pip mode only */
      if (i) {
        detailStep = (long)val(fullArg[i + 1]);
        if (!detailStep) detailStep = -1; /* To use as flag; error message
                                             will occur in showDetailStep() */
      }

/*??? Need better warnings for switch combinations that don't make sense */

      /* 2-Jan-2017 nm */
      /* Print a single message for "/compressed/fast" */
      if (switchPos("/ COMPRESSED") && fastFlag
          && !strcmp("*", labelMatch)) {
        print2(
            "Reformatting and saving (but not recompressing) all proofs...\n");
      }


      q = 0;  /* Flag that at least one matching statement was found */
      for (stmt = 1; stmt <= statements; stmt++) {
        /* If pipFlag (NEW_PROOF), we will iterate exactly once.  This
           loop of course will be entered because there is a least one
           statement, and at the end of the s loop we break out of it. */
        /* If !pipFlag, get the next statement: */
        if (!pipFlag) {
          if (statement[stmt].type != (char)p_) continue; /* Not $p */
          /* 30-Jan-06 nm Added single-character-match wildcard argument */
          if (!matchesList(statement[stmt].labelName, labelMatch, '*', '?'))
            continue;
          showStatement = stmt;
        }

        q = 1; /* Flag that at least one matching statement was found */

        if (detailStep) {
          /* Show the details of just one step */
          showDetailStep(showStatement, detailStep);
          continue;
        }

        if (switchPos("/ STATEMENT_SUMMARY")) { /* non-pip mode only */
          /* Just summarize the statements used in the proof */
          proofStmtSumm(showStatement, essentialFlag, texFlag);
          continue;
        }

        /* 21-Jun-2014 */
        if (switchPos("/ SIZE")) { /* non-pip mode only */
          /* Just print the size of the stored proof and continue */
          let(&str1, space(statement[showStatement].proofSectionLen));
          memcpy(str1, statement[showStatement].proofSectionPtr,
              (size_t)(statement[showStatement].proofSectionLen));
          n = instr(1, str1, "$.");
          if (n == 0) {
            /* The original source truncates the proof before $. */
            n = statement[showStatement].proofSectionLen;
          } else {
            /* If a proof is saved, it includes the $. (Should we
               revisit or document better how/why this is done?) */
            n = n - 1;
          }
          print2("The proof source for \"%s\" has %ld characters.\n",
              statement[showStatement].labelName, n);
          continue;
        }

        if (switchPos("/ PACKED") || switchPos("/ NORMAL") ||
            switchPos("/ COMPRESSED") || switchPos("/ EXPLICIT") || saveFlag) {
          /*??? Add error msg if other switches were specified. (Ignore them.)*/

          /* 16-Aug-2016 nm */
          if (saveFlag) {
            if (printTime == 1) {
              getRunTime(&timeIncr);  /* This call just resets the time */
            }
          }

          if (!pipFlag) {
            outStatement = showStatement;
          } else {
            outStatement = proveStatement;
          }

          explicitTargets = (switchPos("/ EXPLICIT") != 0) ? 1 : 0;

          /* Get the amount to indent the proof by */
          indentation = 2 + getSourceIndentation(outStatement);

          if (!pipFlag) {
            parseProof(showStatement);  /* Prints message if severe error */
            if (wrkProof.errorSeverity > 1) {  /* 21-Aug-04 nm */
                              /* Prevent bugtrap in nmbrSquishProof -> */
                              /* nmbrGetSubProofLen if proof corrupted */
              print2(
          "?The proof has a severe error and cannot be displayed or saved.\n");
              continue;
            }
            /* verifyProof(showStatement); */ /* Not necessary */
            if (fastFlag) { /* 2-Mar-2016 nm */
              /* 2-Mar-2016 nm */
              /* Use the proof as is */
              nmbrLet(&nmbrSaveProof, wrkProof.proofString);
            } else {
              /* Make sure the proof is uncompressed */
              nmbrLet(&nmbrSaveProof, nmbrUnsquishProof(wrkProof.proofString));
            }
          } else {
            nmbrLet(&nmbrSaveProof, proofInProgress.proof);
          }
          if (switchPos("/ PACKED")  || switchPos("/ COMPRESSED")) {
            if (!fastFlag) { /* 2-Mar-2016 nm */
              nmbrLet(&nmbrSaveProof, nmbrSquishProof(nmbrSaveProof));
            }
          }

          if (switchPos("/ COMPRESSED")) {
            let(&str1, compressProof(nmbrSaveProof,
                outStatement, /* showStatement or proveStatement based on pipFlag */
                (switchPos("/ OLD_COMPRESSION")) ? 1 : 0  /* 27-Dec-2013 nm */
                ));
          } else {
            let(&str1, nmbrCvtRToVString(nmbrSaveProof,
                /* 25-Jan-2016 nm */
                explicitTargets, /*explicitTargets*/
                outStatement /*statemNum, used only if explicitTargets*/));
          }


          if (saveFlag) {
            /* ??? This is a problem when mixing html and save proof */
            if (printString[0]) bug(1114);
            let(&printString, "");

            /* Set flag for print2() to put output in printString instead
               of displaying it on the screen */
            outputToString = 1;

          } else {
            if (!print2("Proof of \"%s\":\n", statement[outStatement].labelName))
              break; /* Break for speedup if user quit */
            print2(
"---------Clip out the proof below this line to put it in the source file:\n");
            /* 19-Apr-2015 so */
            /* 24-Apr-2015 nm Reverted */
            /*print2("\n");*/ /* Add a blank line to make clipping easier */
          }
          if (switchPos("/ COMPRESSED")) {
            printLongLine(cat(space(indentation), str1, " $.", NULL),
              space(indentation), "& "); /* "&" is special flag to break
                  compressed part of proof anywhere */
          } else {
            printLongLine(cat(space(indentation), str1," $.", NULL),
              space(indentation), " ");
          }

          /* 24-Apr-2015 nm */
          l = (long)(strlen(str1)); /* Save length for printout below */

          /* 3-May-2017 nm Rewrote the if block below */
          /* 12-Jun-2011 nm Removed pipFlag condition so that a date
             stamp will always be created if it doesn't exist */
          if /*(pipFlag)*/ (1  /* Add the date proof was created */
              /* 19-Apr-2015 so */
              /* SOREAR Only generate date if the proof looks complete.
                 This is not intended as a grading mechanism, just trying
                 to avoid premature output */
              && !nmbrElementIn(1, nmbrSaveProof, -(long)'?')) {

            /* 3-May-2017 nm */
            /* Add a "(Contributed by...)" date if it isn't there */
            let(&str2, "");
            str2 = getContrib(outStatement, CONTRIBUTOR);
            if (str2[0] == 0) { /* The is no contributor, so add one */

              /* 14-May-2017 nm */
              /* See if there is a date below the proof (for converting old
                 .mm files).  Someday this will be obsolete, with str3 and
                 str4 always returned as "". */
              getProofDate(outStatement, &str3, &str4);
              /* If there are two dates below the proof, the first on is
                 the revision date and the second the "Contributed by" date. */
              if (str4[0] != 0) { /* There are 2 dates below the proof */
                let(&str5, str3); /* 1st date is Revised by... */
                let(&str3, str4); /* 2nd date is Contributed by... */
              } else {
                let(&str5, "");
              }
              /* If there is no date below proof, use today's date */
              if (str3[0] == 0) let(&str3, date());
              let(&str4, cat("\n", space(indentation + 1),
                  /*"(Contributed by ?who?, ", date(), ".) ", NULL));*/
                  "(Contributed by ", contributorName,
                      ", ", str3, ".) ", NULL)); /* 14-May-2017 nm */
              /* If there is a 2nd date below proof, add a "(Revised by..."
                 tag */
              if (str5[0] != 0) {
                /* Use the DEFAULT_CONTRIBUTOR ?who? because we don't
                   know the reviser name (using the contributor name may
                   be incorrect).  Also, this will trigger a warning in
                   VERIFY MARKUP since it may be a proof shortener rather than
                   a reviser. */
                let(&str4, cat(str4, "\n", space(indentation + 1),
                    "(Revised by ", DEFAULT_CONTRIBUTOR,
                        ", ", str5, ".) ", NULL)); /* 15-May-2017 nm */
              }

              let(&str3, space(statement[outStatement].labelSectionLen));
              /* str3 will have the statement's label section w/ comment */
              memcpy(str3, statement[outStatement].labelSectionPtr,
                  (size_t)(statement[outStatement].labelSectionLen));
              i = rinstr(str3, "$)");  /* The last "$)" occurrence */
              if (i != 0    /* A description comment exists */
                  && saveFlag) { /* and we are saving the proof */
                /* This isn't a perfect wrapping but we assume
                   'write source .../rewrap' will be done eventually. */
                /* str3 will have the updated comment */
                let(&str3, cat(left(str3, i - 1), str4, right(str3, i), NULL));
                if (statement[outStatement].labelSectionChanged == 1) {
                  /* Deallocate old comment if not original source */
                  let(&str4, ""); /* Deallocate any previous str4 content */
                  str4 = statement[outStatement].labelSectionPtr;
                  let(&str4, ""); /* Deallocate the old content */
                }
                /* Set flag that this is not the original source */
                statement[outStatement].labelSectionChanged = 1;
                statement[outStatement].labelSectionLen = (long)strlen(str3);
                /* We do a direct assignment instead of let(&...) because
                   labelSectionPtr may point to the middle of the giant input
                   file buffer, which we don't want to deallocate */
                statement[outStatement].labelSectionPtr = str3;
                /* Reset str3 without deallocating with let(), since it
                   was assigned to labelSectionPtr */
                str3 = "";
                /* Reset the cache for this statement in getContrib() */
                str3 = getContrib(outStatement, GC_RESET_STMT);
              } /* if i != 0 */

#ifdef DATE_BELOW_PROOF /* 12-May-2017 nm */

              /* Add a date below the proof.  It actually goes in the
                 label section of the next statement; the proof section
                 is not changed. */
              /* (This will become obsolete eventually) */
              let(&str3, space(statement[outStatement + 1].labelSectionLen));
              /* str3 will have the next statement's label section w/ comment */
              memcpy(str3, statement[outStatement + 1].labelSectionPtr,
                  (size_t)(statement[outStatement + 1].labelSectionLen));
              let(&str5, ""); /* We need to guarantee this
                                for the screen printout later */
              if (instr(1, str3, "$( [") == 0) {
                /* There is no date below proof (if there is, don't do
                   anything; if it is wrong, 'verify markup' will check it) */

                /* Save str5 for screen printout later! */
                let(&str5, cat(space(indentation),
                    "$( [", date(), "] $)", NULL)); /* str4 will be used
                                          for the screen printout later */

                if (saveFlag) { /* save proof, not show proof */

                  if (statement[outStatement + 1].labelSectionChanged == 1) {
                    /* Deallocate old comment if not original source */
                    let(&str4, ""); /* Deallocate any previous str4 content */
                    str4 = statement[outStatement + 1].labelSectionPtr;
                    let(&str4, ""); /* Deallocate the old content */
                  }
                  /* str3 starts after the "$." ending the proof, and should
                     start with "\n" */
                  let(&str3, edit(str3, 8/* Discard leading spaces and tabs */));
                  if (str3[0] != '\n') let(&str3, cat("\n", str3, NULL));
                  /* Add the date after the proof */
                  let(&str3, cat("\n", str5, str3, NULL));
                  /* Set flag that this is not the original source */
                  statement[outStatement + 1].labelSectionChanged = 1;
                  statement[outStatement + 1].labelSectionLen
                      = (long)strlen(str3);
                  /* We do a direct assignment instead of let(&...) because
                     labelSectionPtr may point to the middle of the giant input
                     file buffer, which we don't want to deallocate */
                  statement[outStatement + 1].labelSectionPtr = str3;
                  /* Reset str3 without deallocating with let(), since it
                     was assigned to labelSectionPtr */
                  str3 = "";
                  /* Reset the cache for this statement in getContrib() */
                  str3 = getContrib(outStatement + 1, GC_RESET_STMT);
                } /* if saveFlag */
              } /* if (instr(1, str3, "$( [") == 0) */

#endif /* end #ifdef DATE_BELOW_PROOF */ /* 12-May-2017 nm */

            } /* if str2[0] == 0 */

            /* At this point, str4 contains the "$( [date] $)" comment
               if it would have been added to the saved proof, for use
               by "show proof" */


            /********* deleted 3-May-2017 - date below proof is obsolete
            /@ 12-Jun-2011 nm Removed pipFlag condition so that a date
               stamp will always be created if it doesn't exist @/
            if ( /@ pipFlag && @/ !instr(1, str2, "$([")
                /@ 7-Sep-04 Allow both "$([<date>])$" and "$( [<date>] )$" @/
                /@ 19-Apr-2015 so @/
                /@ SOREAR Only generate date if the proof looks complete.
                   This is not intended as a grading mechanism, just trying
                   to avoid premature output @/
                && !nmbrElementIn(1, nmbrSaveProof, -(long)'?')
                && !instr(1, str2, "$( [")) {
            /@ 6/13/98 end @/
              /@ No date stamp existed before.  Create one for today's
                 date.  Note that the characters after "$." at the end of
                 the proof normally go in the labelSection of the next
                 statement, but a special mode in outputStatement() (in
                 mmpars.c) will output the date stamp characters for a saved
                 proof. @/
              /@ print2("%s$([%s]$)\n", space(indentation), date()); @/
              /@ 4/23/04 nm Initialize with a "?" date followed by today's
                 date.  The "?" date can be edited by the user when the
                 proof is becomes "official." @/
              /@print2("%s$([?]$) $([%s]$)\n", space(indentation), date());@/
              /@ 7-Sep-04 Put space around "[<date>]" @/
              /@print2("%s$( [?] $) $( [%s] $)\n", space(indentation), date());@/
              /@ 30-Nov-2013 remove the unknown date placeholder @/
              print2("%s$( [%s] $)\n", space(indentation), date());
            } else {
              if (saveFlag && (instr(1, str2, "$([")
                  /@ 7-Sep-04 Allow both "$([<date>])$" and "$( [<date>] )$" @/
                  || instr(1, str2, "$( ["))) {
                /@ An old date stamp existed, and we're saving the proof to
                   the output file.  Make sure the indentation of the old
                   date stamp (which exists in the labelSection of the
                   next statement) matches the indentation of the saved
                   proof.  To do this, we "delete" the indentation spaces
                   on the old date in the labelSection of the next statement,
                   and we put the actual required indentation spaces at
                   the end of the saved proof.  This is done because the
                   labelSectionPtr of the next statement does not point to
                   an isolated string that can be allocated/deallocated but
                   rather to a place in the input source buffer. @/
                /@ Correct the indentation on old date @/
                while ((statement[outStatement + 1].labelSectionPtr)[0] !=
                    '$') {
                  /@ "Delete" spaces before old date (by moving source
                     buffer pointer forward), and also "delete"
                     the \n that comes before those spaces @/
                  /@ If the proof is saved a 2nd time, this loop will
                     not be entered because the pointer will already be
                     at the "$". @/
                  (statement[outStatement + 1].labelSectionPtr)++;
                  (statement[outStatement + 1].labelSectionLen)--;
                }
                if (!outputToString) bug(1115);
                /@ The final \n will not appear in final output (done in
                   outputStatement() in mmpars.c) because the proofSectionLen
                   below is adjusted to omit it.  This will allow the
                   space(indentation) to appear before the old date without an
                   intervening \n. @/
                print2("%s\n", space(indentation));
              }
            }
            ******** end of deletion 3-May-2017 ******/
          } /* if / *(pipFlag)* / (1) */

          if (saveFlag) {
            sourceChanged = 1;
            proofChanged = 0;
            if (processUndoStack(NULL, PUS_GET_STATUS, "", 0)) {
              /* The UNDO stack may not be empty */
              proofSavedFlag = 1; /* UNDO stack empty no longer reliably
                             indicates that proof hasn't changed */
            }

            /******** deleted 3-May-2017 nm (before proofSectionChanged added)
            /@ ASCII 1 is a flag that string was allocated and not part of
               original source file text buffer @/
            let(&printString, cat(chr(1), "\n", printString, NULL));
            if (statement[outStatement].proofSectionPtr[-1] == 1) {
              /@ Deallocate old proof if not original source @/
              let(&str1, ""); /@ Deallocate any previous str1 content @/
              str1 = statement[outStatement].proofSectionPtr - 1;
              let(&str1, ""); /@ Deallocate the proof section @/
            }
            statement[outStatement].proofSectionPtr = printString + 1;
            /@ Subtr 1 char for ASCII 1 at beg, 1 char for "\n" at the end
               (which is the first char of next statement's labelSection) @/
            statement[outStatement].proofSectionLen
                = (long)strlen(printString) - 2;
            /@ Reset printString without deallocating @/
            printString = "";
            outputToString = 0;
            **********/

            /* 3-May-2017 nm */
            /* Add an initial \n which will go after the "$=" and the
               beginning of the proof */
            let(&printString, cat("\n", printString, NULL));
            if (statement[outStatement].proofSectionChanged == 1) {
              /* Deallocate old proof if not original source */
              let(&str1, ""); /* Deallocate any previous str1 content */
              str1 = statement[outStatement].proofSectionPtr;
              let(&str1, ""); /* Deallocate the proof section */
            }
            /* Set flag that this is not the original source */
            statement[outStatement].proofSectionChanged = 1;
            if (strcmp(" $.\n",
                right(printString, (long)strlen(printString) - 3))) {
              bug(1128);
            }
            /* Note that printString ends with "$.\n", but those 3 characters
               should not be in the proofSection.  (The "$." keyword is
               added between proofSection and next labelSection when the
               output is written by writeOutput.)  Thus we subtract 3
               from the length.  But there is no need to truncate the
               string; later deallocation will take care of the whole
               string. */
            statement[outStatement].proofSectionLen
                = (long)strlen(printString) - 3;
            /* We do a direct assignment instead of let(&...) because
               proofSectionPtr may point to the middle of the giant input
               file string, which we don't want to deallocate */
            statement[outStatement].proofSectionPtr = printString;
            /* Reset printString without deallocating with let(), since it
               was assigned to proofSectionPtr */
            printString = "";
            outputToString = 0;


            if (!pipFlag) {
              if (!(fastFlag && !strcmp("*", labelMatch))) {
                printLongLine(
                    cat("The proof of \"", statement[outStatement].labelName,
                    "\" has been reformatted and saved internally.",
                    NULL), "", " ");
              }
            } else {
              printLongLine(cat("The new proof of \"", statement[outStatement].labelName,
                  "\" has been saved internally.",
                  NULL), "", " ");
            }

            /* 16-Aug-2016 nm */
            if (printTime == 1) {
              getRunTime(&timeIncr);
              print2("SAVE PROOF run time = %6.2f sec for \"%s\"\n", timeIncr,
                  statement[outStatement].labelName);
            }

          } else {

#ifdef DATE_BELOW_PROOF /* 12-May-2017 nm */

            /* Print the date on the screen if it would be added to the file */
            if (str5[0] != 0) print2("%s\n", str5);

#endif /*#ifdef DATE_BELOW_PROOF*/ /* 12-May-2017 nm */

            /* 19-Apr-2015 so */
            /* 24-Apr-2015 nm Reverted */
            /*print2("\n");*/ /* Add a blank line to make clipping easier */
            print2(cat(
                "---------The proof of \"", statement[outStatement].labelName,
                /* "\" to clip out ends above this line.\n",NULL)); */
                /* 24-Apr-2015 nm */
                "\" (", str((double)l), " bytes) ends above this line.\n", NULL));
          } /* End if saveFlag */
          nmbrLet(&nmbrSaveProof, NULL_NMBRSTRING);
          if (pipFlag) break; /* Only one iteration for NEW_PROOF stuff */
          continue;  /* to next s iteration */
        } /* end if (switchPos("/ PACKED") || switchPos("/ NORMAL") ||
            switchPos("/ COMPRESSED") || switchPos("/ EXPLICIT") || saveFlag) */

        if (saveFlag) bug(1112); /* Shouldn't get here */

        if (!pipFlag) {
          outStatement = showStatement;
        } else {
          outStatement = proveStatement;
        }
        if (texFlag) {
          outputToString = 1; /* Flag for print2 to add to printString */
          if (!htmlFlag) {
            if (!oldTexFlag) {
              /* 14-Sep-2010 nm */
              print2("\\begin{proof}\n");
              print2("\\begin{align}\n");
            } else {
              print2("\n");
              print2("\\vspace{1ex} %%1\n");
              printLongLine(cat("Proof of ",
                  "{\\tt ",
                  asciiToTt(statement[outStatement].labelName),
                  "}:", NULL), "", " ");
              print2("\n");
              print2("\n");
            }
          } else { /* htmlFlag */
            bug(1118);
            /*???? The code below is obsolete - now down in show statement*/
            /*
            print2("<CENTER><TABLE BORDER CELLSPACING=0 BGCOLOR=%s>\n",
                MINT_BACKGROUND_COLOR);
            print2("<CAPTION><B>Proof of Theorem <FONT\n");
            printLongLine(cat("   COLOR=", GREEN_TITLE_COLOR, ">",
                asciiToTt(statement[outStatement].labelName),
                "</FONT></B></CAPTION>", NULL), "", "\"");
            print2(
                "<TR><TD><B>Step</B></TD><TD><B>Hyp</B></TD><TD><B>Ref</B>\n");
            print2("</TD><TD><B>Expression</B></TD></TR>\n");
            */
          }
          outputToString = 0;
          /* 8/26/99: Obsolete: */
          /* printTexLongMath in typeProof will do this
          fprintf(texFilePtr, "%s", printString);
          let(&printString, "");
          */
          /* 8/26/99: printTeXLongMath now clears printString in LaTeX
             mode before starting its output, so we must put out the
             printString ourselves here */
          fprintf(texFilePtr, "%s", printString);
          let(&printString, ""); /* We'll clr it anyway */
        } else { /* !texFlag */
          /* Terminal output - display the statement if wildcard is used */
          if (!pipFlag) {
            /* 30-Jan-06 nm Added single-character-match wildcard argument */
            if (instr(1, labelMatch, "*") || instr(1, labelMatch, "?")) {
              if (!print2("Proof of \"%s\":\n", statement[outStatement].labelName))
                break; /* Break for speedup if user quit */
            }
          }
        }


        if (texFlag) print2("Outputting proof of \"%s\"...\n",
            statement[outStatement].labelName);

        typeProof(outStatement,
            pipFlag,
            startStep,
            endStep,
            endIndent,
            essentialFlag,

            /* 1-May-2017 nm */
            /* In SHOW PROOF xxx /TEX, we use renumber steps mode so that
               the first step is step 1.  The step number is checked for
               step 1 in mmwtex.c to prevent a spurious \\ (newline) at the
               start of the proof.  Note that
               SHOW PROOF is not available in HTML mode, so texFlag will
               always mean LaTeX mode here. */
            (texFlag ? 1 : renumberFlag),
            /*was: renumberFlag,*/

            unknownFlag,
            notUnifiedFlag,
            reverseFlag,

            /* 1-May-2017 nm */
            /* In SHOW PROOF xxx /TEX, we use Lemmon mode so that the
               hypothesis reference list will be available.  Note that
               SHOW PROOF is not available in HTML mode, so texFlag will
               always mean LaTeX mode here. */
            (texFlag ? 1 : noIndentFlag),
            /*was: noIndentFlag,*/

            splitColumn,
            skipRepeatedSteps, /* 28-Jun-2013 nm */
            texFlag,
            htmlFlag);
        if (texFlag) {
          if (!htmlFlag) {
            /* 14-Sep-2010 nm */
            if (!oldTexFlag) {
              outputToString = 1;
              print2("\\end{align}\n");
              print2("\\end{proof}\n");
              print2("\n");
              outputToString = 0;
              fprintf(texFilePtr, "%s", printString);
              let(&printString, "");
            } else {
            }
          } else { /* htmlFlag */
            outputToString = 1;
            print2("</TABLE>\n");
            print2("</CENTER>\n");
            /* print trailer will close out string later */
            outputToString = 0;
          }
        }

        /*???CLEAN UP */
        /*nmbrLet(&wrkProof.proofString, nmbrSaveProof);*/

        /*E*/ if (0) { /* for debugging: */
          printLongLine(nmbrCvtRToVString(wrkProof.proofString,
                /* 25-Jan-2016 nm */
                0, /*explicitTargets*/
                0 /*statemNum, used only if explicitTargets*/)," "," ");
          print2("\n");

          nmbrLet(&nmbrSaveProof, nmbrSquishProof(wrkProof.proofString));
          printLongLine(nmbrCvtRToVString(nmbrSaveProof,
                /* 25-Jan-2016 nm */
                0, /*explicitTargets*/
                0 /*statemNum, used only if explicitTargets*/)," "," ");
          print2("\n");

          nmbrLet(&nmbrTmp, nmbrUnsquishProof(nmbrSaveProof));
          printLongLine(nmbrCvtRToVString(nmbrTmp,
                /* 25-Jan-2016 nm */
                0, /*explicitTargets*/
                0 /*statemNum, used only if explicitTargets*/)," "," ");

          nmbrLet(&nmbrTmp, nmbrGetTargetHyp(nmbrSaveProof,showStatement));
          printLongLine(nmbrCvtAnyToVString(nmbrTmp)," "," "); print2("\n");

          nmbrLet(&nmbrTmp, nmbrGetEssential(nmbrSaveProof));
          printLongLine(nmbrCvtAnyToVString(nmbrTmp)," "," "); print2("\n");

          cleanWrkProof(); /* Deallocate verifyProof storage */
        } /* end debugging */

        if (pipFlag) break; /* Only one iteration for NEW_PROOF stuff */
      } /* Next stmt */
      if (!q) {
        /* No matching statement was found */
        printLongLine(cat("?There is no $p statement whose label matches \"",
            (cmdMatches("MIDI")) ? fullArg[1] : fullArg[2],
            "\".  ",
            "Use SHOW LABELS to see list of valid labels.", NULL), "", " ");
      } else {
        if (saveFlag) {
          print2("Remember to use WRITE SOURCE to save changes permanently.\n");
        }
        if (texFlag) {
          print2("The LaTeX source was written to \"%s\".\n", texFileName);
         /* 14-Sep-2010 nm Added OLD_TEX */
         oldTexFlag = 0;
        }
      }

      continue;
    } /* if (cmdMatches("SHOW/SAVE [NEW_]PROOF" or" MIDI") */


/*E*/ /*???????? DEBUG command for debugging only */
    if (cmdMatches("DBG")) {
      print2("DEBUGGING MODE IS FOR DEVELOPER'S USE ONLY!\n");
      print2("Argument:  %s\n", fullArg[1]);
      nmbrLet(&nmbrTmp, parseMathTokens(fullArg[1], proveStatement));
      for (j = 0; j < 3; j++) {
        print2("Trying depth %ld\n", j);
        nmbrTmpPtr = proveFloating(nmbrTmp, proveStatement, j, 0, 0,
            1/*overrideFlag*/);
        if (nmbrLen(nmbrTmpPtr)) break;
      }

      print2("Result:  %s\n", nmbrCvtRToVString(nmbrTmpPtr,
                /* 25-Jan-2016 nm */
                0, /*explicitTargets*/
                0 /*statemNum, used only if explicitTargets*/));
      nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);

      continue;
    }
/*E*/ /*???????? DEBUG command for debugging only */

    if (cmdMatches("PROVE")) {

      /* 14-Sep-2012 nm */
      /* Get the unique statement matching the fullArg[1] pattern */
      i = getStatementNum(fullArg[1],
          1/*startStmt*/,
          statements + 1  /*maxStmt*/,
          0/*aAllowed*/,
          1/*pAllowed*/,
          0/*eAllowed*/,
          0/*fAllowed*/,
          0/*efOnlyForMaxStmt*/,
          1/*uniqueFlag*/);
      if (i == -1) {
        continue; /* Error msg was provided if not unique */
      }
      proveStatement = i;


      /* 10-May-2016 nm */
      /* 1 means to override usage locks */
      overrideFlag = ( (switchPos("/ OVERRIDE")) ? 1 : 0)
         || globalDiscouragement == 0;
      if (getMarkupFlag(proveStatement, PROOF_DISCOURAGED)) {
        if (overrideFlag == 0) {
          /* print2("\n"); */ /* Enable for more emphasis */
          print2(
 ">>> ?Error: Modification of this statement's proof is discouraged.\n"
              );
          print2(
 ">>> You must use PROVE ... / OVERRIDE to work on it.\n");
          /* print2("\n"); */ /* Enable for more emphasis */
          continue;
        }
      }


      /*********** 14-Sep-2012 replaced by getStatementNum()
      /@??? Make sure only $p statements are allowed. @/
      for (i = 1; i <= statements; i++) {
        if (!strcmp(fullArg[1],statement[i].labelName)) break;
      }
      if (i > statements) {
        printLongLine(cat("?There is no statement with label \"",
            fullArg[1], "\".  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        proveStatement = 0;
        continue;
      }
      proveStatement = i;
      if (statement[proveStatement].type != (char)p_) {
        printLongLine(cat("?Statement \"", fullArg[1],
            "\" is not a $p statement.", NULL), "", " ");
        proveStatement = 0;
        continue;
      }
      ************** end of 14-Sep-2012 deletion ************/

      print2(
"Entering the Proof Assistant.  HELP PROOF_ASSISTANT for help, EXIT to exit.\n");

      /* Obsolete:
      print2("You will be working on the proof of statement %s:\n",
          statement[proveStatement].labelName);
      printLongLine(cat("  $p ", nmbrCvtMToVString(
          statement[proveStatement].mathString), NULL), "    ", " ");
      */

      PFASmode = 1; /* Set mode for commands here and in mmcmdl.c */
      /* Note:  Proof Assistant mode can equivalently be determined by:
            nmbrLen(proofInProgress.proof) != 0  */

      parseProof(proveStatement);
      verifyProof(proveStatement); /* Necessary to set RPN stack ptrs
                                      before calling cleanWrkProof() */
      if (wrkProof.errorSeverity > 1) {
        print2(
             "The starting proof has a severe error.  It will not be used.\n");
        nmbrLet(&nmbrSaveProof, nmbrAddElement(NULL_NMBRSTRING, -(long)'?'));
      } else {
        nmbrLet(&nmbrSaveProof, wrkProof.proofString);
      }
      cleanWrkProof(); /* Deallocate verifyProof storage */

      /* 11-Sep-2016 nm */
      /* Initialize the structure needed for the Proof Assistant */
      initProofStruct(&proofInProgress, nmbrSaveProof, proveStatement);

      /****** 11-Sep-2016 nm Replaced by initProofStruct()
      /@ Right now, only non-packed proofs are handled. @/
      nmbrLet(&nmbrSaveProof, nmbrUnsquishProof(nmbrSaveProof));

      /@ Assign initial proof structure @/
      if (nmbrLen(proofInProgress.proof)) bug(1113); /@ Should've been deall.@/
      nmbrLet(&proofInProgress.proof, nmbrSaveProof);
      i = nmbrLen(proofInProgress.proof);
      pntrLet(&proofInProgress.target, pntrNSpace(i));
      pntrLet(&proofInProgress.source, pntrNSpace(i));
      pntrLet(&proofInProgress.user, pntrNSpace(i));
      nmbrLet((nmbrString @@)(&((proofInProgress.target)[i - 1])),
          statement[proveStatement].mathString);
      pipDummyVars = 0;

      /@ Assign known subproofs @/
      assignKnownSubProofs();

      /@ Initialize remaining steps @/
      for (j = 0; j < i/@proof length@/; j++) {
        if (!nmbrLen((proofInProgress.source)[j])) {
          initStep(j);
        }
      }

      /@ Unify whatever can be unified @/
      autoUnify(0); /@ 0 means no "congrats" message @/
      **** end of 11-Sep-2016 deletion ************/

/*
      print2(
"Periodically save your work with SAVE NEW_PROOF and WRITE SOURCE.\n");
      print2(
"There is no UNDO command yet.  You can OPEN LOG to track what you've done.\n");
*/
      /* Show the user the statement to be proved */
      print2("You will be working on statement (from \"SHOW STATEMENT %s\"):\n",
          statement[proveStatement].labelName);
      typeStatement(proveStatement /*showStatement*/,
          1 /*briefFlag*/,
          0 /*commentOnlyFlag*/,
          0 /*texFlag*/,
          0 /*htmlFlag*/);

      if (!nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
        print2(
        "Note:  The proof you are starting with is already complete.\n");
      } else {

        print2(
     "Unknown step summary (from \"SHOW NEW_PROOF / UNKNOWN\"):\n");
        /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
        typeProof(proveStatement,
            1 /*pipFlag*/,
            0 /*startStep*/,
            0 /*endStep*/,
            0 /*endIndent*/,
            1 /*essentialFlag*/,
            0 /*renumberFlag*/,
            1 /*unknownFlag*/,
            0 /*notUnifiedFlag*/,
            0 /*reverseFlag*/,
            0 /*noIndentFlag*/,
            0 /*splitColumn*/,
            0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
            0 /*texFlag*/,
            0 /*htmlFlag*/);
        /* 6/14/98 end */
      }

      /* 3-May-2016 nm */
      if (getMarkupFlag(proveStatement, PROOF_DISCOURAGED)) {
        /* print2("\n"); */ /* Enable for more emphasis */
        print2(
">>> ?Warning: Modification of this statement's proof is discouraged.\n"
            );
        /*
        print2(
">>> You must use SAVE NEW_PROOF ... / OVERRIDE to save it.\n");
        */
        /* print2("\n"); */ /* Enable for more emphasis */
      }

      processUndoStack(NULL, PUS_INIT, "", 0); /* Optional? */
      /* Put the initial proof into the UNDO stack; we don't need
         the info string since it won't be undone */
      processUndoStack(&proofInProgress, PUS_PUSH, "", 0);
      continue;
    }


    /* 1-Nov-2013 nm Added UNDO */
    if (cmdMatches("UNDO")) {
      processUndoStack(&proofInProgress, PUS_UNDO, "", 0);
      proofChanged = 1; /* Maybe make this more intelligent some day */
      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */
      continue;
    }

    /* 1-Nov-2013 nm Added REDO */
    if (cmdMatches("REDO")) {
      processUndoStack(&proofInProgress, PUS_REDO, "", 0);
      proofChanged = 1; /* Maybe make this more intelligent some day */
      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */
      continue;
    }

    if (cmdMatches("UNIFY")) {
      m = nmbrLen(proofInProgress.proof); /* Original proof length */
      proofChangedFlag = 0;
      if (cmdMatches("UNIFY STEP")) {

        s = (long)val(fullArg[2]); /* Step number */
        if (s > m || s < 1) {
          print2("?The step must be in the range from 1 to %ld.\n", m);
          continue;
        }

        interactiveUnifyStep(s - 1, 1); /* 2nd arg. means print msg if
                                           already unified */

        /* (The interactiveUnifyStep handles all messages.) */
        /* print2("... */

        autoUnify(1);
        if (proofChangedFlag) {
          proofChanged = 1; /* Cumulative flag */
          processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
        }
        continue;
      }

      /* "UNIFY ALL" */
      if (!switchPos("/ INTERACTIVE")) {
        autoUnify(1);
        if (!proofChangedFlag) {
          print2("No new unifications were made.\n");
        } else {
          print2(
  "Steps were unified.  SHOW NEW_PROOF / NOT_UNIFIED to see any remaining.\n");
          proofChanged = 1; /* Cumulative flag */
          processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
        }
      } else {
        q = 0;
        while (1) {
          /* Repeat the unifications over and over until done, since
             a later unification may improve the ability of an aborted earlier
             one to be done without timeout */
          proofChangedFlag = 0; /* This flag is set by autoUnify() and
                                   interactiveUnifyStep() */
          autoUnify(0);
          for (s = m - 1; s >= 0; s--) {
            interactiveUnifyStep(s, 0); /* 2nd arg. means no msg if already
                                           unified */
          }
          autoUnify(1); /* 1 means print congratulations if complete */
          if (!proofChangedFlag) {
            if (!q) {
              print2("No new unifications were made.\n");
            } else {
              /* If q=1, then we are in the 2nd or later pass, which means
                 there was a change in the 1st pass. */
              print2(
  "Steps were unified.  SHOW NEW_PROOF / NOT_UNIFIED to see any remaining.\n");
              proofChanged = 1; /* Cumulative flag */
              processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
            }
            break; /* while (1) */
          } else {
            q = 1; /* Flag that we're starting a 2nd or later pass */
          }
        } /* End while (1) */
      }
      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */
      continue;
    }

    if (cmdMatches("MATCH")) {

      maxEssential = -1; /* Default:  no maximum */
      i = switchPos("/ MAX_ESSENTIAL_HYP");
      if (i) maxEssential = (long)val(fullArg[i + 1]);

      if (cmdMatches("MATCH STEP")) {

        s = (long)val(fullArg[2]); /* Step number */
        m = nmbrLen(proofInProgress.proof); /* Original proof length */
        if (s > m || s < 1) {
          print2("?The step must be in the range from 1 to %ld.\n", m);
          continue;
        }
        if ((proofInProgress.proof)[s - 1] != -(long)'?') {
          print2(
    "?Step %ld is already assigned.  Only unknown steps can be matched.\n", s);
          continue;
        }

        interactiveMatch(s - 1, maxEssential);
        n = nmbrLen(proofInProgress.proof); /* New proof length */
        if (n != m) {
          if (s != m) {
            printLongLine(cat("Steps ", str((double)s), ":",
                str((double)m), " are now ", str((double)(s - m + n)), ":",
                str((double)n), ".",
                NULL),
                "", " ");
          } else {
            printLongLine(cat("Step ", str((double)m), " is now step ", str((double)n), ".",
                NULL),
                "", " ");
          }
        }

        autoUnify(1);
        proofChanged = 1; /* Cumulative flag */
        /* 1-Nov-2013 nm Why is proofChanged set unconditionally above?
           Do we need the processUndoStack() call? */
        processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);

        continue;
      } /* End if MATCH STEP */

      if (cmdMatches("MATCH ALL")) {

        m = nmbrLen(proofInProgress.proof); /* Original proof length */

        k = 0;
        proofChangedFlag = 0;

        if (switchPos("/ ESSENTIAL")) {
          nmbrLet(&nmbrTmp, nmbrGetEssential(proofInProgress.proof));
        }

        for (s = m; s > 0; s--) {
          /* Match only unknown steps */
          if ((proofInProgress.proof)[s - 1] != -(long)'?') continue;
          /* Match only essential steps if specified */
          if (switchPos("/ ESSENTIAL")) {
            if (!nmbrTmp[s - 1]) continue;
          }

          interactiveMatch(s - 1, maxEssential);
          if (proofChangedFlag) {
            k = s; /* Save earliest step changed */
            proofChangedFlag = 0;
          }
          print2("\n");
        }
        if (k) {
          proofChangedFlag = 1; /* Restore it */
          proofChanged = 1; /* Cumulative flag */
          processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
          print2("Steps %ld and above have been renumbered.\n", k);
        }
        autoUnify(1);

        continue;
      } /* End if MATCH ALL */
    }

    if (cmdMatches("LET")) {

      errorCount = 0;
      nmbrLet(&nmbrTmp, parseMathTokens(fullArg[4], proveStatement));
      if (errorCount) {
        /* Parsing issued error message(s) */
        errorCount = 0;
        continue;
      }

      if (cmdMatches("LET VARIABLE")) {
        if (((vstring)(fullArg[2]))[0] != '$') {
          print2(
   "?The target variable must be of the form \"$<integer>\", e.g. \"$23\".\n");
          continue;
        }
        n = (long)val(right(fullArg[2], 2));
        if (n < 1 || n > pipDummyVars) {
          print2("?The target variable must be between $1 and $%ld.\n",
              pipDummyVars);
          continue;
        }

        replaceDummyVar(n, nmbrTmp);

        autoUnify(1);


        proofChangedFlag = 1; /* Flag to push 'undo' stack */
        proofChanged = 1; /* Cumulative flag */
        processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);

      }

      if (cmdMatches("LET STEP")) {

        /* 14-Sep-2012 nm */
        s = getStepNum(fullArg[2], proofInProgress.proof,
            0 /* ALL not allowed */);
        if (s == -1) continue;  /* Error; message was provided already */

        /************** 14-Sep-2012 replaced by getStepNum()
        s = (long)val(fullArg[2]); /@ Step number @/

        /@ 16-Apr-06 nm Added LET STEP n where n <= 0: 0 = last,
           -1 = penultimate, etc. _unknown_ step @/
        /@ Unlike ASSIGN LAST/FIRST and IMPROVE LAST/FIRST, it probably
           doesn't make sense to add LAST/FIRST to LET STEP since known
           steps can also be LET.  The main purpose of LET STEP n, n<=0, is
           to use with scripting for mmj2 imports. @/
        offset = 0;
        if (s <= 0) {
          offset = - s + 1;
          s = 1; /@ Temp. until we figure out which step @/
        }
        /@ End of 16-Apr-06 @/

        m = nmbrLen(proofInProgress.proof); /@ Original proof length @/
        if (s > m || s < 1) {
          print2("?The step must be in the range from 1 to %ld.\n", m);
          continue;
        }

        /@ 16-Apr-06 nm Added LET STEP n where n <= 0: 0 = last,
           1 = penultimate, etc. _unknown_ step @/
        if (offset > 0) {  /@ step <= 0 @/
          /@ Get the essential step flags @/
          s = 0; /@ Use as flag that step was found @/
          nmbrLet(&essentialFlags, nmbrGetEssential(proofInProgress.proof));
          /@ Scan proof backwards until last essential unknown step found @/
          /@ 16-Apr-06 - count back 'offset' unknown steps @/
          j = offset;
          for (i = m; i >= 1; i--) {
            if (essentialFlags[i - 1]
                && (proofInProgress.proof)[i - 1] == -(long)'?') {
              j--;
              if (j == 0) {
                /@ Found it @/
                s = i;
                break;
              }
            }
          } /@ Next i @/
          if (s == 0) {
            if (offset == 1) {
              print2("?There are no unknown essential steps.\n");
            } else {
              print2("?There are not at least %ld unknown essential steps.\n",
                offset);
            }
            continue;
          }
        } /@ if offset > 0 @/
        /@ End of 16-Apr-06 @/
        ******************** end 14-Sep-2012 deletion ********/

        /* Check to see if the statement selected is allowed */
        if (!checkMStringMatch(nmbrTmp, s - 1)) {
          printLongLine(cat("?Step ", str((double)s), " cannot be unified with \"",
              nmbrCvtMToVString(nmbrTmp),"\".", NULL), " ", " ");
          continue;
        }

        /* Assign the user string */
        nmbrLet((nmbrString **)(&((proofInProgress.user)[s - 1])), nmbrTmp);

        autoUnify(1);
        proofChangedFlag = 1; /* Flag to push 'undo' stack */
        proofChanged = 1; /* Cumulative flag */
        processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
      }
      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */
      continue;
    }


    if (cmdMatches("ASSIGN")) {

      /* 10/4/99 - Added LAST - this means the last unknown step shown
         with SHOW NEW_PROOF/ESSENTIAL/UNKNOWN */
      /* 11-Dec-05 nm - Added FIRST - this means the first unknown step shown
         with SHOW NEW_PROOF/ESSENTIAL/UNKNOWN */
      /* 16-Apr-06 nm - Handle nonpositive step number: 0 = last,
         -1 = penultimate, etc.*/
      /* 14-Sep-2012 nm All the above now done by getStepNum() */
      s = getStepNum(fullArg[1], proofInProgress.proof,
          0 /* ALL not allowed */);
      if (s == -1) continue;  /* Error; message was provided already */

      /* 3-May-2016 nm */
      /* 1 means to override usage locks */
      overrideFlag = ( (switchPos("/ OVERRIDE")) ? 1 : 0)
         || globalDiscouragement == 0;

      /****** replaced by getStepNum()  nm 14-Sep-2012
      offset = 0; /@ 16-Apr-06 @/
      let(&str1, fullArg[1]); /@ To avoid void pointer problems with fullArg @/
      if (toupper((unsigned char)(str1[0])) == 'L'
          || toupper((unsigned char)(str1[0])) == 'F') {
                                          /@ "ASSIGN LAST" or "ASSIGN FIRST" @/
                                          /@ 11-Dec-05 nm @/
        s = 1; /@ Temporary until we figure out which step @/
        offset = 1;          /@ 16-Apr-06 @/
      } else {
        s = (long)val(fullArg[1]); /@ Step number @/
        if (strcmp(fullArg[1], str((double)s))) {
          print2("?Expected either a number or FIRST or LAST after ASSIGN.\n");
                                                             /@ 11-Dec-05 nm @/
          continue;
        }
        if (s <= 0) {         /@ 16-Apr-06 @/
          offset = - s + 1;   /@ 16-Apr-06 @/
          s = 1; /@ Temporary until we figure out which step @/ /@ 16-Apr-06 @/
        }                     /@ 16-Apr-06 @/
      }
      ******************** end 14-Sep-2012 deletion ********/

      /* 14-Sep-2012 nm */
      /* Get the unique statement matching the fullArg[2] pattern */
      k = getStatementNum(fullArg[2],
          1/*startStmt*/,
          proveStatement  /*maxStmt*/,
          1/*aAllowed*/,
          1/*pAllowed*/,
          1/*eAllowed*/,
          1/*fAllowed*/,
          1/*efOnlyForMaxStmt*/,
          1/*uniqueFlag*/);
      if (k == -1) {
        continue; /* Error msg was provided if not unique */
      }

      /*********** 14-Sep-2012 replaced by getStatementNum()
      for (i = 1; i <= statements; i++) {
        if (!strcmp(fullArg[2], statement[i].labelName)) {
          /@ If a $e or $f, it must be a hypothesis of the statement
             being proved @/
          if (statement[i].type == (char)e_ || statement[i].type == (char)f_){
            if (!nmbrElementIn(1, statement[proveStatement].reqHypList, i) &&
                !nmbrElementIn(1, statement[proveStatement].optHypList, i))
                continue;
          }
          break;
        }
      }
      if (i > statements) {
        printLongLine(cat("?The statement with label \"",
            fullArg[2],
            "\" was not found or is not a hypothesis of the statement ",
            "being proved.  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        continue;
      }
      k = i;

      if (k >= proveStatement) {
        print2(
   "?You must specify a statement that occurs earlier the one being proved.\n");
        continue;
      }
      ***************** end of 14-Sep-2012 deletion ************/

      /* 3-May-2016 nm */
      if (getMarkupFlag(k, USAGE_DISCOURAGED)) {
        if (overrideFlag == 0) {
          /* print2("\n"); */ /* Enable for more emphasis */
          print2(
">>> ?Error: Attempt to assign a statement whose usage is discouraged.\n");
          print2(
       ">>> Use ASSIGN ... / OVERRIDE if you really want to do this.\n");
          /* print2("\n"); */ /* Enable for more emphasis */
          continue;
        } else {
          /* print2("\n"); */ /* Enable for more emphasis */
          print2(
">>> ?Warning: You are assigning a statement whose usage is discouraged.\n");
          /* print2("\n"); */ /* Enable for more emphasis */
        }
      }

      m = nmbrLen(proofInProgress.proof); /* Original proof length */


      /****** replaced by getStepNum()  nm 14-Sep-2012
      if (s > m || s < 1) {
        print2("?The step must be in the range from 1 to %ld.\n", m);
        continue;
      }

      /@ 10/4/99 - For ASSIGN FIRST/LAST command, figure out the last unknown
         essential step @/                      /@ 11-Dec-05 nm - Added LAST @/
      /@if (toupper(str1[0]) == 'L' || toupper(str1[0]) == 'F') {@/
                                /@ "ASSIGN LAST or FIRST" @/ /@ 11-Dec-05 nm @/
      if (offset > 0) {  /@ LAST, FIRST, or step <= 0 @/ /@ 16-Apr-06 @/
        /@ Get the essential step flags @/
        s = 0; /@ Use as flag that step was found @/
        nmbrLet(&essentialFlags, nmbrGetEssential(proofInProgress.proof));
        /@ if (toupper((unsigned char)(str1[0])) == 'L') { @/
        if (toupper((unsigned char)(str1[0])) != 'F') {   /@ 16-Apr-06 @/
          /@ Scan proof backwards until last essential unknown step is found @/
          /@ 16-Apr-06 - count back 'offset' unknown steps @/
          j = offset;      /@ 16-Apr-06 @/
          for (i = m; i >= 1; i--) {
            if (essentialFlags[i - 1]
                && (proofInProgress.proof)[i - 1] == -(long)'?') {
              j--;          /@ 16-Apr-06 @/
              if (j == 0) {  /@ 16-Apr-06 @/
                /@ Found it @/
                s = i;
                break;
              }             /@ 16-Apr-06 @/
            }
          } /@ Next i @/
        } else {
          /@ 11-Dec-05 nm Added ASSIGN FIRST @/
          /@ Scan proof forwards until first essential unknown step is found @/
          for (i = 1; i <= m; i++) {
            if (essentialFlags[i - 1]
                && (proofInProgress.proof)[i - 1] == -(long)'?') {
              /@ Found it @/
              s = i;
              break;
            }
          } /@ Next i @/
        }
        if (s == 0) {
          if (offset == 1) {                                /@ 16-Apr-06 @/
            print2("?There are no unknown essential steps.\n");
          } else {                                          /@ 16-Apr-06 @/
            print2("?There are not at least %ld unknown essential steps.\n",
              offset);                                      /@ 16-Apr-06 @/
          }                                                 /@ 16-Apr-06 @/
          continue;
        }
      }
      ******************** end 14-Sep-2012 deletion ********/

      /* Check to see that the step is an unknown step */
      if ((proofInProgress.proof)[s - 1] != -(long)'?') {
        print2(
        "?Step %ld is already assigned.  You can only assign unknown steps.\n"
            , s);
        continue;
      }

      /* Check to see if the statement selected is allowed */
      if (!checkStmtMatch(k, s - 1)) {
        print2("?Statement \"%s\" cannot be unified with step %ld.\n",
          statement[k].labelName, s);
        continue;
      }

      assignStatement(k /*statement#*/, s - 1 /*step*/);

      n = nmbrLen(proofInProgress.proof); /* New proof length */
      autoUnify(1);

      /* Automatically interact with user if step not unified */
      /* ???We might want to add a setting to defeat this if user doesn't
         like it */
      /* 8-Apr-05 nm Since ASSIGN LAST is often run from a commmand file, don't
         interact if / NO_UNIFY is specified, so response is predictable */
      if (switchPos("/ NO_UNIFY") == 0) {
        interactiveUnifyStep(s - m + n - 1, 2); /* 2nd arg. means print msg if
                                                 already unified */
      } /* if NO_UNIFY flag not set */

      /* 8-Apr-05 nm Commented out:
      if (m == n) {
        print2("Step %ld was assigned statement %s.\n",
          s, statement[k].labelName);
      } else {
        if (s != m) {
          printLongLine(cat("Step ", str((double)s),
              " was assigned statement ", statement[k].labelName,
              ".  Steps ", str((double)s), ":",
              str((double)m), " are now ", str((double)(s - m + n)), ":", str((double)n), ".",
              NULL),
              "", " ");
        } else {
          printLongLine(cat("Step ", str((double)s),
              " was assigned statement ", statement[k].labelName,
              ".  Step ", str((double)m), " is now step ", str((double)n), ".",
              NULL),
              "", " ");
        }
      }
      */
      /* 8-Apr-05 nm Added: */
      /* 1-Nov-2013 nm No longer needed because of UNDO
      printLongLine(cat("To undo the assignment, DELETE STEP ",
              str((double)(s - m + n)), " and if needed INITIALIZE, UNIFY.",
              NULL),
              "", " ");
      */

      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */


      proofChangedFlag = 1; /* Flag to push 'undo' stack (future) */
      proofChanged = 1; /* Cumulative flag */
      processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
      continue;

    } /* cmdMatches("ASSIGN") */


    if (cmdMatches("REPLACE")) {

      /* s = (long)val(fullArg[1]);  obsolete */ /* Step number */

      /* 3-May-2016 nm */
      /* 1 means to override usage locks */
      overrideFlag = ( (switchPos("/ OVERRIDE")) ? 1 : 0)
         || globalDiscouragement == 0;

      /* 14-Sep-2012 nm */
      step = getStepNum(fullArg[1], proofInProgress.proof,
          0 /* ALL not allowed */);
      if (step == -1) continue;  /* Error; message was provided already */

      /* This limitation is due to the assignKnownSteps call below which
         does not tolerate unknown steps. */
      /******* 10/20/02  Limitation removed
      if (nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
        print2("?The proof must be complete before you can use REPLACE.\n");
        continue;
      }
      *******/

      /* 14-Sep-2012 nm */
      /* Get the unique statement matching the fullArg[2] pattern */
      stmt = getStatementNum(fullArg[2],
          1/*startStmt*/,
          proveStatement  /*maxStmt*/,
          1/*aAllowed*/,
          1/*pAllowed*/,
          0/*eAllowed*/, /* Not implemented (yet?) */
          0/*fAllowed*/, /* Not implemented (yet?) */
          1/*efOnlyForMaxStmt*/,
          1/*uniqueFlag*/);
      if (stmt == -1) {
        continue; /* Error msg was provided if not unique */
      }

      /*********** 14-Sep-2012 replaced by getStatementNum()
      for (i = 1; i <= statements; i++) {
        if (!strcmp(fullArg[2], statement[i].labelName)) {
          /@ If a $e or $f, it must be a hypothesis of the statement
             being proved @/
          if (statement[i].type == (char)e_ || statement[i].type == (char)f_){
            if (!nmbrElementIn(1, statement[proveStatement].reqHypList, i) &&
                !nmbrElementIn(1, statement[proveStatement].optHypList, i))
                continue;
          }
          break;
        }
      }
      if (i > statements) {
        printLongLine(cat("?The statement with label \"",
            fullArg[2],
            "\" was not found or is not a hypothesis of the statement ",
            "being proved.  ",
            "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
        continue;
      }
      k = i;

      if (k >= proveStatement) {
        print2(
   "?You must specify a statement that occurs earlier the one being proved.\n");
        continue;
      }
      ****************************** end of 14-Sep-2012 deletion *********/

      /* 3-May-2016 nm */
      if (getMarkupFlag(stmt, USAGE_DISCOURAGED)) {
        if (overrideFlag == 0) {
          /* print2("\n"); */ /* Enable for more emphasis */
          print2(
">>> ?Error: Attempt to assign a statement whose usage is discouraged.\n");
          print2(
       ">>> Use REPLACE ... / OVERRIDE if you really want to do this.\n");
          /* print2("\n"); */ /* Enable for more emphasis */
          continue;
        } else {
          /* print2("\n"); */ /* Enable for more emphasis */
          print2(
">>> ?Warning: You are assigning a statement whose usage is discouraged.\n");
          /* print2("\n"); */ /* Enable for more emphasis */
        }
      }

      m = nmbrLen(proofInProgress.proof); /* Original proof length */

      /************** 14-Sep-2012 replaced by getStepNum()
      if (s > m || s < 1) {
        print2("?The step must be in the range from 1 to %ld.\n", m);
        continue;
      }
      ************* end of 14-Sep-2012 deletion **************/

      /* Check to see that the step is a known step */
      /* 22-Aug-2012 nm This check was deleted because it is unnecessary  */
      /*
      if ((proofInProgress.proof)[s - 1] == -(long)'?') {
        print2(
        "?Step %ld is unknown.  You can only replace known steps.\n"
            , s);
        continue;
      }
      */

      /* 10/20/02  Set a flag that proof has unknown steps (for autoUnify()
         call below) */
      if (nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
        p = 1;
      } else {
        p = 0;
      }

      /* Check to see if the statement selected is allowed */
      if (!checkStmtMatch(stmt, step - 1)) {
        print2("?Statement \"%s\" cannot be unified with step %ld.\n",
          statement[stmt].labelName, step);
        continue;
      }

      /* 16-Sep-2012 nm */
      /* Check dummy variable status of step */
      /* For use in message later */
      dummyVarIsoFlag = checkDummyVarIsolation(step - 1);
            /* 0=no dummy vars, 1=isolated dummy vars, 2=not isolated*/

      /* Do the replacement */
      nmbrTmpPtr = replaceStatement(stmt /*statement#*/,
          step - 1 /*step*/,
          proveStatement,
          0,/*scan whole proof to maximize chance of a match*/
          0/*noDistinct*/,
          1/* try to prove $e's */,
          1/*improveDepth*/,
          overrideFlag   /* 3-May-2016 nm */
          );
      if (!nmbrLen(nmbrTmpPtr)) {
        print2(
           "?Hypotheses of statement \"%s\" do not match known proof steps.\n",
            statement[stmt].labelName);
        continue;
      }

      /* Get the subproof at step s */
      q = subproofLen(proofInProgress.proof, step - 1);
      deleteSubProof(step - 1);
      addSubProof(nmbrTmpPtr, step - q);

      /* 10/20/02 Replaced "assignKnownSteps" with code from entry of PROVE
         command so REPLACE can be done in partial proofs */
      /*assignKnownSteps(s - q, nmbrLen(nmbrTmpPtr));*/  /* old code */
      /* Assign known subproofs */
      assignKnownSubProofs();
      /* Initialize remaining steps */
      i = nmbrLen(proofInProgress.proof);
      for (j = 0; j < i; j++) {
        if (!nmbrLen((proofInProgress.source)[j])) {
          initStep(j);
        }
      }
      /* Unify whatever can be unified */
      /* If proof wasn't complete before (p = 1), but is now, print congrats
         for user */
      autoUnify((char)p); /* 0 means no "congrats" message */
      /* end 10/20/02 */

      nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING); /* Deallocate memory */

      n = nmbrLen(proofInProgress.proof); /* New proof length */
      if (nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
        /* The proof is not complete, so print step numbers that changed */
        if (m == n) {
          print2("Step %ld was replaced with statement %s.\n",
            step, statement[stmt].labelName);
        } else {
          if (step != m) {
            printLongLine(cat("Step ", str((double)step),
                " was replaced with statement ", statement[stmt].labelName,
                ".  Steps ", str((double)step), ":",
                str((double)m), " are now ", str((double)(step - m + n)), ":",
                str((double)n), ".",
                NULL),
                "", " ");
          } else {
            printLongLine(cat("Step ", str((double)step),
                " was replaced with statement ", statement[stmt].labelName,
                ".  Step ", str((double)m), " is now step ", str((double)n), ".",
                NULL),
                "", " ");
          }
        }
      }
      /*autoUnify(1);*/

      /************ delete 19-Sep-2012 nm - not needed for REPLACE *******
      /@ Automatically interact with user if step not unified @/
      /@ ???We might want to add a setting to defeat this if user doesn't
         like it @/
      if (1 /@ ???Future setting flag @/) {
        interactiveUnifyStep(step - m + n - 1, 2); /@ 2nd arg. means print
                                         msg if already unified @/
      }
      *************** end 19-Sep-2012 deletion ********************/

      proofChangedFlag = 1; /* Flag to push 'undo' stack */
      proofChanged = 1; /* Cumulative flag */
      processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);

      /* 16-Sep-2012 nm */
      /*
      if (dummyVarIsoFlag == 2 && proofChangedFlag) {
        printLongLine(cat(
     "Assignments to shared working variables ($nn) are guesses.  If "
     "incorrect, to undo DELETE STEP ",
              str((double)(step - m + n)),
      ", INITIALIZE, UNIFY, then assign them manually with LET ",
      "and try REPLACE again.",
              NULL),
              "", " ");
      }
      */
      /* 25-Feb-2014 nm */
      if (dummyVarIsoFlag == 2 && proofChangedFlag) {
        printLongLine(cat(
     "Assignments to shared working variables ($nn) are guesses.  If "
     "incorrect, UNDO then assign them manually with LET ",
      "and try REPLACE again.",
              NULL),
              "", " ");
      }


      /* 14-Sep-2012 nm - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      if (proofChangedFlag)
        typeProof(proveStatement,
            1 /*pipFlag*/,
            0 /*startStep*/,
            0 /*endStep*/,
            0 /*endIndent*/,
            1 /*essentialFlag*/,
            0 /*renumberFlag*/,
            1 /*unknownFlag*/,
            0 /*notUnifiedFlag*/,
            0 /*reverseFlag*/,
            0 /*noIndentFlag*/,
            0 /*splitColumn*/,
            0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
            0 /*texFlag*/,
            0 /*htmlFlag*/);
      /* 14-Sep-2012 end */


      continue;

    } /* REPLACE */


    if (cmdMatches("IMPROVE")) {

      improveDepth = 0; /* Depth */
      i = switchPos("/ DEPTH");
      if (i) improveDepth = (long)val(fullArg[i + 1]);
      if (switchPos("/ NO_DISTINCT")) p = 1; else p = 0;
                        /* p = 1 means don't try to use statements with $d's */
      /* 22-Aug-2012 nm Added */
      searchAlg = 1; /* Default */
      if (switchPos("/ 1")) searchAlg = 1;
      if (switchPos("/ 2")) searchAlg = 2;
      if (switchPos("/ 3")) searchAlg = 3;
      /* 4-Sep-2012 nm Added */
      searchUnkSubproofs = 0;
      if (switchPos("/ SUBPROOFS")) searchUnkSubproofs = 1;

      /* 3-May-2016 nm */
      /* 1 means to override usage locks */
      overrideFlag = ( (switchPos("/ OVERRIDE")) ? 1 : 0)
         || globalDiscouragement == 0;

      /* 14-Sep-2012 nm */
      s = getStepNum(fullArg[1], proofInProgress.proof,
          1 /* ALL not allowed */);
      if (s == -1) continue;  /* Error; message was provided already */

      if (s != 0) {  /* s=0 means ALL */

      /**************** 14-Sep-2012 nm replaced with getStepNum()
      /@ 26-Aug-2006 nm Changed 'IMPROVE STEP <step>' to 'IMPROVE <step>' @/
      let(&str1, fullArg[1]); /@ To avoid void pointer problems with fullArg @/
      if (toupper((unsigned char)(str1[0])) != 'A') {
        /@ 16-Apr-06 nm - Handle nonpositive step number: 0 = last,
           -1 = penultimate, etc.@/
        offset = 0; /@ 16-Apr-06 @/
        /@ 10/4/99 - Added LAST - this means the last unknown step shown
           with SHOW NEW_PROOF/ESSENTIAL/UNKNOWN @/
        if (toupper((unsigned char)(str1[0])) == 'L'
            || toupper((unsigned char)(str1[0])) == 'F') {
                                        /@ 'IMPROVE LAST' or 'IMPROVE FIRST' @/
          s = 1; /@ Temporary until we figure out which step @/
          offset = 1;          /@ 16-Apr-06 @/
        } else {
          if (toupper((unsigned char)(str1[0])) == 'S') {
            print2(
           "?\"IMPROVE STEP <step>\" is obsolete.  Use \"IMPROVE <step>\".\n");
            continue;
          }
          s = (long)val(fullArg[1]); /@ Step number @/
          if (strcmp(fullArg[1], str((double)s))) {
            print2(
                "?Expected a number or FIRST or LAST or ALL after IMPROVE.\n");
            continue;
          }
          if (s <= 0) {         /@ 16-Apr-06 @/
            offset = - s + 1;   /@ 16-Apr-06 @/
            s = 1; /@ Temporary until we figure out step @/ /@ 16-Apr-06 @/
          }                     /@ 16-Apr-06 @/
        }
        /@ End of 26-Aug-2006 change @/

      /@ ------- Old code before 26-Aug-2006 -------
      if (cmdMatches("IMPROVE STEP") || cmdMatches("IMPROVE LAST") ||
          cmdMatches("IMPROVE FIRST")) {                     /# 11-Dec-05 nm #/

        /# 16-Apr-06 nm - Handle nonpositive step number: 0 = last,
           -1 = penultimate, etc.#/
        offset = 0; /# 16-Apr-06 #/
        /# 10/4/99 - Added LAST - this means the last unknown step shown
           with SHOW NEW_PROOF/ESSENTIAL/UNKNOWN #/
        if (cmdMatches("IMPROVE LAST") || cmdMatches("IMPROVE FIRST")) {
                               /# "IMPROVE LAST or FIRST" #/ /# 11-Dec-05 nm #/
          s = 1; /# Temporary until we figure out which step #/
          offset = 1;          /# 16-Apr-06 #/
        } else {
          s = val(fullArg[2]); /# Step number #/
          if (s <= 0) {         /# 16-Apr-06 #/
            offset = - s + 1;   /# 16-Apr-06 #/
            s = 1; /# Temp. until we figure out which step #/ /# 16-Apr-06 #/
          }                     /# 16-Apr-06 #/
        }
        ------- End of old code ------- @/
        **************** end of 14-Sep-2012 nm ************/

        m = nmbrLen(proofInProgress.proof); /* Original proof length */


        /**************** 14-Sep-2012 nm replaced with getStepNum()
        if (s > m || s < 1) {
          print2("?The step must be in the range from 1 to %ld.\n", m);
          continue;
        }


        /@ 10/4/99 - For IMPROVE FIRST/LAST command, figure out the last
           unknown essential step @/           /@ 11-Dec-05 nm - Added FIRST @/
        /@if (cmdMatches("IMPROVE LAST") || cmdMatches("IMPROVE FIRST")) {@/
                               /@ IMPROVE LAST or FIRST @/ /@ 11-Dec-05 nm @/
        if (offset > 0) {  /@ LAST, FIRST, or step <= 0 @/ /@ 16-Apr-06 @/
          /@ Get the essential step flags @/
          s = 0; /@ Use as flag that step was found @/
          nmbrLet(&essentialFlags, nmbrGetEssential(proofInProgress.proof));
          /@if (cmdMatches("IMPROVE LAST")) {@/
          /@if (!cmdMatches("IMPROVE FIRST")) {@/   /@ 16-Apr-06 @/
          if (toupper((unsigned char)(str1[0])) != 'F') { /@ 26-Aug-2006 @/
            /@ Scan proof backwards until last essential unknown step found @/
            /@ 16-Apr-06 - count back 'offset' unknown steps @/
            j = offset;      /@ 16-Apr-06 @/
            for (i = m; i >= 1; i--) {
              if (essentialFlags[i - 1]
                  && (proofInProgress.proof)[i - 1] == -(long)'?') {
                j--;           /@ 16-Apr-06 @/
                if (j == 0) {  /@ 16-Apr-06 @/
                  /@ Found it @/
                  s = i;
                  break;
                }              /@ 16-Apr-06 @/
              }
            } /@ Next i @/
          } else {
            /@ 11-Dec-05 nm Added IMPROVE FIRST @/
            /@ Scan proof forwards until first essential unknown step found @/
            for (i = 1; i <= m; i++) {
              if (essentialFlags[i - 1]
                  && (proofInProgress.proof)[i - 1] == -(long)'?') {
                /@ Found it @/
                s = i;
                break;
              }
            } /@ Next i @/
          }
          if (s == 0) {
            if (offset == 1) {                                /@ 16-Apr-06 @/
              print2("?There are no unknown essential steps.\n");
            } else {                                          /@ 16-Apr-06 @/
              print2("?There are not at least %ld unknown essential steps.\n",
                offset);                                      /@ 16-Apr-06 @/
            }                                                 /@ 16-Apr-06 @/
            continue;
          }
        } /@ if offset > 0 @/
        **************** end of 14-Sep-2012 nm ************/

        /* Get the subproof at step s */
        q = subproofLen(proofInProgress.proof, s - 1);
        nmbrLet(&nmbrTmp, nmbrSeg(proofInProgress.proof, s - q + 1, s));

        /*???Shouldn't this be just known?*/
        /* Check to see that the subproof has an unknown step. */
        if (!nmbrElementIn(1, nmbrTmp, -(long)'?')) {
          print2(
              "?Step %ld already has a proof and cannot be improved.\n",
              s);
          continue;
        }

        /* 25-Aug-2012 nm */
        /* Check dummy variable status of step */
        dummyVarIsoFlag = checkDummyVarIsolation(s - 1);
              /* 0=no dummy vars, 1=isolated dummy vars, 2=not isolated*/
        if (dummyVarIsoFlag == 2) {
          print2(
  "?Step %ld target has shared dummy variables and cannot be improved.\n", s);
          continue; /* Don't try to improve
                                 dummy variables that aren't isolated */
        }

        /********* Deleted old code 25-Aug-2012 nm
        /@ Check to see that the step has no dummy variables. @/
        j = 0; /@ Break flag @/
        for (i = 0; i < nmbrLen((proofInProgress.target)[s - 1]); i++) {
          if (((nmbrString @)((proofInProgress.target)[s - 1]))[i] > mathTokens) {
            j = 1;
            break;
          }
        }
        if (j) {
          print2(
   "?Step %ld target has dummy variables and cannot be improved.\n", s);
          continue;
        }
        ********/


        if (dummyVarIsoFlag == 0) { /* No dummy vars */ /* 25-Aug-2012 nm */
          /* Only use proveFloating if no dummy vars */
          nmbrTmpPtr = proveFloating((proofInProgress.target)[s - 1],
              proveStatement, improveDepth, s - 1, (char)p/*NO_DISTINCT*/,
              overrideFlag  /* 3-May-2016 nm */
              );
        } else {
          nmbrTmpPtr = NULL_NMBRSTRING; /* Initialize */ /* 25-Aug-2012 nm */
        }
        if (!nmbrLen(nmbrTmpPtr)) {
          /* A proof for the step was not found with proveFloating(). */

          /* 22-Aug-2012 nm Next, try REPLACE algorithm */
          if (searchAlg == 2 || searchAlg == 3) {
            nmbrTmpPtr = proveByReplacement(proveStatement,
              s - 1/*prfStep*/, /* 0 means step 1 */
              (char)p/*NO_DISTINCT*/, /* 1 means don't try stmts with $d's */
              dummyVarIsoFlag,
              (char)(searchAlg - 2), /*0=proveFloat for $fs, 1=$e's also */
              improveDepth, /* 4-Sep-2012 */
              overrideFlag  /* 3-May-2016 nm */
              );
          }
          if (!nmbrLen(nmbrTmpPtr)) {
            print2("A proof for step %ld was not found.\n", s);
            /* REPLACE algorithm also failed */
            continue;
          }
        }

        /* If q=1, subproof must be an unknown step, so don't bother to
           delete it */
        /*???Won't q always be 1 here?*/
        if (q > 1) deleteSubProof(s - 1);
        addSubProof(nmbrTmpPtr, s - q);
        assignKnownSteps(s - q, nmbrLen(nmbrTmpPtr));
        nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);

        n = nmbrLen(proofInProgress.proof); /* New proof length */
        if (m == n) {
          print2("A 1-step proof was found for step %ld.\n", s);
        } else {
          if (s != m || q != 1) {
            printLongLine(cat("A ", str((double)(n - m + 1)),
                "-step proof was found for step ", str((double)s),
                ".  Steps ", str((double)s), ":",
                str((double)m), " are now ", str((double)(s - q + 1 - m + n)),
                ":", str((double)n), ".",
                NULL),
                "", " ");
          } else {
            printLongLine(cat("A ", str((double)(n - m + 1)),
                "-step proof was found for step ", str((double)s),
                ".  Step ", str((double)m), " is now step ", str((double)n), ".",
                NULL),
                "", " ");
          }
        }

        autoUnify(1); /* To get 'congrats' message if proof complete */
        proofChanged = 1; /* Cumulative flag */
        processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);

        /* End if s != 0 i.e. not IMPROVE ALL */   /* 14-Sep-2012 nm */
      } else {
        /* Here, getStepNum() returned 0, meaning ALL */  /* 14-Sep-2012 nm */

        /*if (cmdMatches("IMPROVE ALL")) {*/  /* obsolete */

        if (!nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
          print2("The proof is already complete.\n");
          continue;
        }

        n = 0; /* Earliest step that changed */

        proofChangedFlag = 0;

        for (improveAllIter = 1; improveAllIter <= 4; improveAllIter++) {
                                                           /* 25-Aug-2012 nm */
          if (improveAllIter == 1 && (searchAlg == 2 || searchAlg == 3))
            print2("Pass 1:  Trying to match cut-free statements...\n");
          if (improveAllIter == 2) {
            if (searchAlg == 2) {
              print2("Pass 2:  Trying to match all statements...\n");
            } else {
              print2(
"Pass 2:  Trying to match all statements, with cut-free hypothesis matches...\n"
                  );
            }
          }
          if (improveAllIter == 3 && searchUnkSubproofs)
            print2("Pass 3:  Trying to replace incomplete subproofs...\n");
          if (improveAllIter == 4) {
            if (searchUnkSubproofs) {
              print2("Pass 4:  Repeating pass 1...\n");
            } else {
              print2("Pass 3:  Repeating pass 1...\n");
            }
          }
          /* improveAllIter = 1: run proveFloating only */
          /* improveAllIter = 2: run proveByReplacement on unknown steps */
          /* improveAllIter = 3: run proveByReplacement on steps with
                                   incomplete subproofs */
          /* improveAllIter = 4: if something changed, run everything again */

          if (improveAllIter == 3 && !searchUnkSubproofs) continue;

          m = nmbrLen(proofInProgress.proof); /* Original proof length */

          for (s = m; s > 0; s--) {

            proofStepUnk = ((proofInProgress.proof)[s - 1] == -(long)'?')
                ? 1 : 0;   /* 25-Aug-2012 nm Added for clearer code */

            /* 22-Aug-2012 nm I think this is really too conservative, even
               with the old algorithm, but keep it to imitate the old one */
            if (improveAllIter == 1 || searchAlg == 1) { /* 22-Aug-2012 nm */
              /* If the step is known and unified, don't do it, since nothing
                 would be accomplished. */
              if (!proofStepUnk) {
                if (nmbrEq((proofInProgress.target)[s - 1],
                    (proofInProgress.source)[s - 1])) continue;
              }
            }

            /* Get the subproof at step s */
            q = subproofLen(proofInProgress.proof, s - 1);
            if (proofStepUnk && q != 1) {
              bug(1120); /* 25-Aug-2012 nm Consistency check */
            }
            nmbrLet(&nmbrTmp, nmbrSeg(proofInProgress.proof, s - q + 1, s));

            /* Improve only subproofs with unknown steps */
            if (!nmbrElementIn(1, nmbrTmp, -(long)'?')) continue;

            nmbrLet(&nmbrTmp, NULL_NMBRSTRING); /* No longer needed - dealloc */

            /* 25-Aug-2012 nm */
            /* Check dummy variable status of step */
            dummyVarIsoFlag = checkDummyVarIsolation(s - 1);
                  /* 0=no dummy vars, 1=isolated dummy vars, 2=not isolated*/
            if (dummyVarIsoFlag == 2) continue; /* Don't try to improve
                                     dummy variables that aren't isolated */

            /********* Deleted old code now done by checkDummyVarIsolation()
                       25-Aug-2012 nm
            /@ Check to see that the step has no dummy variables. @/
            j = 0; /@ Break flag @/
            for (i = 0; i < nmbrLen((proofInProgress.target)[s - 1]); i++) {
              if (((nmbrString @)((proofInProgress.target)[s - 1]))[i] >
                  mathTokens) {
                j = 1;
                break;
              }
            }
            if (j) {
              /@ Step has dummy variables and cannot be improved. @/
              continue;
            }
            ********/

            if (dummyVarIsoFlag == 0
                && (improveAllIter == 1
                  || improveAllIter == 4)) {
                /* No dummy vars */ /* 25-Aug-2012 nm */
              /* Only use proveFloating if no dummy vars */
              nmbrTmpPtr = proveFloating((proofInProgress.target)[s - 1],
                  proveStatement, improveDepth, s - 1,
                  (char)p/*NO_DISTINCT*/,
                  overrideFlag  /* 3-May-2016 nm */
                  );
            } else {
              nmbrTmpPtr = NULL_NMBRSTRING; /* Init */ /* 25-Aug-2012 nm */
            }
            if (!nmbrLen(nmbrTmpPtr)) {
              /* A proof for the step was not found with proveFloating(). */

              /* 22-Aug-2012 nm Next, try REPLACE algorithm */
              if ((searchAlg == 2 || searchAlg == 3)
                  && ((improveAllIter == 2 && proofStepUnk)
                    || (improveAllIter == 3 && !proofStepUnk)
                    /*|| improveAllIter == 4*/)) {
                nmbrTmpPtr = proveByReplacement(proveStatement,
                  s - 1/*prfStep*/, /* 0 means step 1 */
                  (char)p/*NO_DISTINCT*/, /* 1 means don't try stmts w/ $d's */
                  dummyVarIsoFlag,
                  (char)(searchAlg - 2),/*searchMethod: 0 or 1*/
                  improveDepth,                        /* 4-Sep-2012 */
                  overrideFlag   /* 3-May-2016 nm */
                  );

              }
              if (!nmbrLen(nmbrTmpPtr)) {
                /* REPLACE algorithm also failed */
                continue;
              }
            }

            /* If q=1, subproof must be an unknown step, so don't bother to
               delete it */
            if (q > 1) deleteSubProof(s - 1);
            addSubProof(nmbrTmpPtr, s - q);
            assignKnownSteps(s - q, nmbrLen(nmbrTmpPtr));
            print2("A proof of length %ld was found for step %ld.\n",
                nmbrLen(nmbrTmpPtr), s);
            if (nmbrLen(nmbrTmpPtr) || q != 1) n = s - q + 1;
                                               /* Save earliest step changed */
            nmbrLet(&nmbrTmpPtr, NULL_NMBRSTRING);
            proofChangedFlag = 1;
            s = s - q + 1; /* Adjust step position to account for deleted subpr */
          } /* Next step s */

          if (proofChangedFlag) {
            autoUnify(0); /* 0 = No 'Congrats' if done */
          }

          if (!proofChangedFlag
              && ( (improveAllIter == 2 && !searchUnkSubproofs)
                 || improveAllIter == 3
                 || searchAlg == 1)) {
            print2("No new subproofs were found.\n");
            break; /* out of improveAllIter loop */
          }
          if (proofChangedFlag) {
            proofChanged = 1; /* Cumulative flag */
          }

          if (!nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
            break; /* Proof is complete */
          }

          if (searchAlg == 1) break; /* Old algorithm does just 1st pass */

        } /* Next improveAllIter */

        if (proofChangedFlag) {
          if (n > 0) {
            /* n is the first step number changed.  It will be 0 if
               the numbering didn't change e.g. a $e was assigned to
               an unknown step. */
            print2("Steps %ld and above have been renumbered.\n", n);
          }
          processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
        }
        if (!nmbrElementIn(1, proofInProgress.proof, -(long)'?')) {
          /* This is a redundant call; its purpose is just to give
             the message if the proof is complete */
          autoUnify(1); /* 1 = 'Congrats' if done */
        }

      } /* End if IMPROVE ALL */

      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      if (proofChangedFlag)
        typeProof(proveStatement,
            1 /*pipFlag*/,
            0 /*startStep*/,
            0 /*endStep*/,
            0 /*endIndent*/,
            1 /*essentialFlag*/,
            0 /*renumberFlag*/,
            1 /*unknownFlag*/,
            0 /*notUnifiedFlag*/,
            0 /*reverseFlag*/,
            0 /*noIndentFlag*/,
            0 /*splitColumn*/,
            0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
            0 /*texFlag*/,
            0 /*htmlFlag*/);
      /* 6/14/98 end */

      continue;

    } /* cmdMatches("IMPROVE") */


    if (cmdMatches("MINIMIZE_WITH")) {

      /* 16-Aug-2016 nm */
      printTime = 0;
      if (switchPos("/ TIME") != 0) {
        printTime = 1;
      }
      if (printTime == 1) {
        getRunTime(&timeIncr);  /* This call just resets the time */
      }

      /* q = 0; */ /* Line length */  /* 25-Jun-2014 deleted */
      prntStatus = 0; /* Status flag to help determine messages
                         0 = no statement was matched during scan
                         1 = a statement was matched but no shorter proof
                         2 = shorter proof found */
      /* verboseMode = (switchPos("/ BRIEF") == 0); */ /* Non-verbose mode */
      /* 4-Feb-2013 nm VERBOSE is now default */
      verboseMode = (switchPos("/ VERBOSE") != 0); /* Verbose mode */
      /* 30-Jan-06 nm Added single-character-match wildcard argument */
      if (!(instr(1, fullArg[1], "*") || instr(1, fullArg[1], "?"))) i = 1;
          /* 16-Feb-05 If no wildcard was used, switch to non-verbose mode
             since there is no point to it and an annoying extra blank line
             results */
      allowGrowthFlag = (switchPos("/ ALLOW_GROWTH") != 0);
                  /* Mode to replace even if it doesn't reduce proof length */
      /* 25-Jun-2014 nm /NO_DISTINCT is obsolete
      noDistinctFlag = (switchPos("/ NO_DISTINCT") != 0);
      */
                                          /* Skip trying statements with $d */
      exceptPos = switchPos("/ EXCEPT"); /* Statement match to skip */
                                                               /* 7-Jan-06 */

      /* 20-May-2013 nm */
      forbidMatchPos = switchPos("/ FORBID");
      if (forbidMatchPos != 0) {
        let(&forbidMatchList, fullArg[forbidMatchPos + 1]);
      } else {
        let(&forbidMatchList, "");
      }

      /* 20-May-2013 nm */
      noNewAxiomsMatchPos = switchPos("/ NO_NEW_AXIOMS_FROM");
      if (noNewAxiomsMatchPos != 0) {
        let(&noNewAxiomsMatchList, fullArg[noNewAxiomsMatchPos + 1]);
      } else {
        let(&noNewAxiomsMatchList, "");
      }

      mathboxFlag = (switchPos("/ INCLUDE_MATHBOXES") != 0); /* 28-Jun-2011 */
      /* 25-Jun-2014 nm /REVERSE is obsolete
      forwFlag = (switchPos("/ REVERSE") != 0); /@ 10-Nov-2011 nm @/
      */
      if (sandboxStmt == 0) { /* Look up "mathbox" label if it hasn't been */
        sandboxStmt = lookupLabel("mathbox");
        if (sandboxStmt == -1)
          sandboxStmt = statements + 1;  /* Default beyond db end if none */
      }

      /* 3-May-2016 nm */
      /* Flag to override any "usage locks" placed in the comment markup */
      overrideFlag = (switchPos("/ OVERRIDE") != 0)
           || globalDiscouragement == 0;

      /* 25-Jun-2014 nm */
      /* If a single statement is specified, don't bother to do certain
         actions or print some of the messages */
      hasWildCard = (instr(1, fullArg[1], "*")
          || instr(1, fullArg[1], "?")
          || instr(1, fullArg[1], ",")
          || instr(1, fullArg[1], "~") /* 3-May-2014 nm label~label range */
          );

      proofChangedFlag = 0;

      /* Added 14-Aug-2012 nm */
      /* Always scan statements in current mathbox, even if
         "/ INCLUDE_MATHBOXES" is omitted */
      thisMathboxStmt = sandboxStmt;
                /* Will become start of current (proveStatement's) mathbox */
      if (proveStatement > sandboxStmt) {
        /* We're in a mathbox */
        for (k = proveStatement; k >= sandboxStmt; k--) {
          let(&str1, left(statement[k].labelSectionPtr,
              statement[k].labelSectionLen));
          /* Heuristic to match beginning of mathbox */
          if (instr(1, str1, "Mathbox for") != 0) {
             /* Found beginning of current mathbox */
             thisMathboxStmt = k;
             break;
          }
        }
      }

      /* 25-Jun-2014 nm */
      copyProofStruct(&saveOrigProof, proofInProgress);

      /* 12-Sep-2016 nm TODO replace this w/ compressedProofSize */
      /* 25-Jun-2014 nm Get the current (original) compressed proof length
         to compare it when a shorter non-compressed proof is found, to see
         if the compressed proof also decreased in size */
      nmbrLet(&nmbrSaveProof, proofInProgress.proof);   /* Redundant? */
      nmbrLet(&nmbrSaveProof, nmbrSquishProof(proofInProgress.proof));
      /* We only care about length; str1 will be discarded */
      let(&str1, compressProof(nmbrSaveProof,
          proveStatement, /* statement being proved */
          0 /* Normal (not "fast") compression */
          ));
      origCompressedLength = (long)strlen(str1);
      print2(
"Bytes refer to compressed proof size, steps to uncompressed length.\n");

      /* 25-Jun-2014 nm forwRevPass outer loop added */
      /* Scan forward, then reverse, then pick best result */
      for (forwRevPass = 1; forwRevPass <= 2; forwRevPass++) {

        /* 25-Jun-2014 nm */
        if (forwRevPass == 1) {
          if (hasWildCard) print2("Scanning forward through statements...\n");
          forwFlag = 1;
        } else {
          /* If growth allowed, don't bother with reverse pass */
          if (allowGrowthFlag) break;
          /* If nothing was found on forward pass, don't bother with rev pass */
          if (!proofChangedFlag) break;
          /* If only one statement was specified, don't bother with rev pass */
          if (!hasWildCard) break;
          print2("Scanning backward through statements...\n");
          forwFlag = 0;
          /* Save proof and length from 1st pass; re-initialize */
          copyProofStruct(&save1stPassProof, proofInProgress);
          forwardLength = nmbrLen(proofInProgress.proof);
          forwardCompressedLength = oldCompressedLength;
          /* Start over from original proof */
          copyProofStruct(&proofInProgress, saveOrigProof);
        }

        /* 20-May-2013 nm */
        /*
        if (forbidMatchList[0]) { /@ User provided a /FORBID list @/
          /@ Save the proof structure in case we have to revert a
             forbidden match. @/
          copyProofStruct(&saveProofForReverting, proofInProgress);
        }
        */
        /* 25-Jun-2014 nm */
        copyProofStruct(&saveProofForReverting, proofInProgress);

        oldCompressedLength = origCompressedLength;

        /* for (k = 1; k < proveStatement; k++) { */
        /* 10-Nov-2011 nm */
        /* We use bottom-up scanning as the default (forwFlag=0) since empirically
           it seems to lead to shorter proofs */
        /* If forwFlag is 0, scan from proveStatement-1 to 1
           If forwFlag is 1, scan from 1 to proveStatement-1 */
        for (k = forwFlag ? 1 : (proveStatement - 1);
             k * (forwFlag ? 1 : -1) < (forwFlag ? proveStatement : 0);
             k = k + (forwFlag ? 1 : -1)) {
          /* 28-Jun-2011 */
          /* Scan mathbox statements only if INCLUDE_MATHBOXES specified */
          /*if (!mathboxFlag && k >= sandboxStmt) continue;*/
          /* 14-Aug-2012 nm */
          if (!mathboxFlag && k >= sandboxStmt && k < thisMathboxStmt) continue;

          if (statement[k].type != (char)p_ && statement[k].type != (char)a_)
            continue;
          /* 30-Jan-06 nm Added single-character-match wildcard argument */
          if (!matchesList(statement[k].labelName, fullArg[1], '*', '?'))
            continue;
          /* 25-Jun-2014 nm /NO_DISTINCT is obsolete
          if (noDistinctFlag) {
            /@ Skip the statement if it has a $d requirement.  This option
               prevents illegal minimizations that would violate $d requirements
               since MINIMIZE_WITH does not check for $d violations. @/
            if (nmbrLen(statement[k].reqDisjVarsA)) {
              /@ 30-Jan-06 nm Added single-character-match wildcard argument @/
              if (!(instr(1, fullArg[1], "@") || instr(1, fullArg[1], "?")))
                print2("?\"%s\" has a $d requirement\n", fullArg[1]);
              continue;
            }
          }
          */

          /* 7-Jan-06 nm - Added EXCEPT switch */
          if (exceptPos != 0) {
            /* Skip any match to the EXCEPT argument */
            /* 30-Jan-06 nm Added single-character-match wildcard argument */
            if (matchesList(statement[k].labelName, fullArg[exceptPos + 1],
                '*', '?'))
              continue;
          }

          /* 20-May-2013 nm */
          if (forbidMatchList[0]) { /* User provided a /FORBID list */
            /* First, we check to make sure we're not trying a statement
               in the forbidMatchList directly (traceProof() won't find
               this) */
            if (matchesList(statement[k].labelName, forbidMatchList, '*', '?'))
              continue;
          }

          /* 3-May-2016 nm */
          /* Check to see if statement comment specified a usage
             restriction */
          if (!overrideFlag) {
            if (getMarkupFlag(k, USAGE_DISCOURAGED)) {
              continue;
            }
          }

          /* Print individual labels */
          if (prntStatus == 0) prntStatus = 1; /* Matched at least one */
          /* 25-Jun-2014 nm Don't list matched statements anymore
          if (verboseMode) {
            q = q + (long)strlen(statement[k].labelName) + 1;
            if (q > 72) {
              q = (long)strlen(statement[k].labelName) + 1;
              print2("\n");
            }
            print2("%s ",statement[k].labelName);
          }
          */

          m = nmbrLen(proofInProgress.proof); /* Original proof length */
          nmbrLet(&nmbrTmp, proofInProgress.proof);
          minimizeProof(k /* trial statement */,
              proveStatement /* statement being proved in MM-PA */,
              (char)allowGrowthFlag /* allowGrowthFlag */);

          n = nmbrLen(proofInProgress.proof); /* New proof length */
          if (!nmbrEq(nmbrTmp, proofInProgress.proof)) {
            /* The proof got shorter (or it changed if ALLOW_GROWTH) */

            /* 20-May-2013 nm Because of the slow speed of traceBack(),
               we only want to check the /FORBID list in the relatively
               rare case where a minimization occurred.  If the FORBID
               list is matched, we then need to revert back to the
               original proof. */
            if (forbidMatchList[0]) { /* User provided a /FORBID list */
              if (statement[k].type == (char)p_) {
                /* We only care about tracing $p statements */
                /* See if the TRACE_BACK list includes a match to the
                   /FORBID argument */
                if (traceProof(k,
                    0, /*essentialFlag*/
                    0, /*axiomFlag*/
                    forbidMatchList,
                    "", /*traceToList*/ /* 18-Jul-2015 nm */
                    1 /* testOnlyFlag */)) {
                  /* Yes, a forbidden statement occurred in traceProof() */
                  /* Revert the proof to before minimization */
                  copyProofStruct(&proofInProgress, saveProofForReverting);
                  /* Skip further printout and flag setting */
                  continue; /* Continue at 'Next k' loop end below */
                }
              }
            }


            /* 22-Nov-2014 nm Because of the slow speed of traceBack(),
               we only want to check the /NO_NEW_AXIOMS_FROM list in the
               relatively rare case where a minimization occurred.  If the
               NO_NEW_AXIOMS_FROM condition applies, we then need to revert
               back to the original proof. */
            if (noNewAxiomsMatchList[0]) { /* User provided /NO_NEW_AXIOMS_FROM */
              /* If we haven't called trace yet for the theorem being proved,
                 do it now. */
              if (traceProofFlags[0] == 0) {
                traceProofWork(proveStatement,
                    1 /*essentialFlag*/,
                    "", /*traceToList*/ /* 18-Jul-2015 nm */
                    &traceProofFlags, /* y/n list of flags */
                    &nmbrTmp /* unproved list - ignored */);
                nmbrLet(&nmbrTmp, NULL_NMBRSTRING); /* Discard */
              }
              let(&traceTrialFlags, "");
              traceProofWork(k,
                  1 /*essentialFlag*/,
                  "", /*traceToList*/ /* 18-Jul-2015 nm */
                  &traceTrialFlags, /* Y/N list of flags */
                  &nmbrTmp /* unproved list - ignored */);
              nmbrLet(&nmbrTmp, NULL_NMBRSTRING); /* Discard */
              j = 1; /* 1 = ok to use trial statement */
              for (i = 1; i < proveStatement; i++) {
                if (statement[i].type != (char)a_) continue; /* Not $a */
                if (traceProofFlags[i] == 'Y') continue;
                         /* If the axiom is already used by the proof, we
                            don't care if the trial statement depends on it */
                if (matchesList(statement[i].labelName, noNewAxiomsMatchList,
                    '*', '?') != 1) {
                  /* If the axiom isn't in the list to avoid, we don't
                     care if the trial statement depends on it */
                  continue;
                }
                if (traceTrialFlags[i] == 'Y') {
                  /* The trial statement uses an axiom that the current
                     proof should avoid, so we abort it */
                  j = 0; /* 0 = don't use trial statement */
                  break;
                }
              }
              if (j == 0) {
                /* A forbidden axiom is used by the trial proof */
                /* Revert the proof to before minimization */
                copyProofStruct(&proofInProgress, saveProofForReverting);
                /* Skip further printout and flag setting */
                continue; /* Continue at 'Next k' loop end below */
              }
            } /* end if noNewAxiomsMatchList[0] */


            /* 25-Jun-2014 nm Make sure the compressed proof length
               decreased, otherwise revert.  Also, we will use the
               compressed proof for the $d check next */
            if (nmbrLen(statement[k].reqDisjVarsA) || !allowGrowthFlag) {
              nmbrLet(&nmbrSaveProof, proofInProgress.proof);
              nmbrLet(&nmbrSaveProof, nmbrSquishProof(proofInProgress.proof));
              let(&str1, compressProof(nmbrSaveProof,
                  proveStatement, /* statement being proved in MM-PA */
                  0 /* Normal (not "fast") compression */
                  ));
              newCompressedLength = (long)strlen(str1);
              if (!allowGrowthFlag && newCompressedLength > oldCompressedLength) {
                /* The compressed proof length increased, so don't use it.
                   (If it stayed the same, we will use it because the uncompressed
                   length did decrease.) */
                /* Revert the proof to before minimization */
                if (verboseMode) {
                  print2(
 "Reverting \"%s\": Uncompressed steps:  old = %ld, new = %ld\n",
                      statement[k].labelName,
                      m, n );
                  print2(
 "    but compressed size:  old = %ld bytes, new = %ld bytes\n",
                      oldCompressedLength, newCompressedLength);
                }
                copyProofStruct(&proofInProgress, saveProofForReverting);
                /* Skip further printout and flag setting */
                continue; /* Continue at 'Next k' loop end below */
              }
            } /* if (nmbrLen(statement[k].reqDisjVarsA) || !allowGrowthFlag) */

            /* 25-Jun-2014 nm */
            /* Make sure there are no $d violations, otherwise revert */
            /* This requires the str1 from above */
            if (nmbrLen(statement[k].reqDisjVarsA)) {
              /* There is currently no way to verify a proof that doesn't
                 read and parse the source directly.  This should be
                 changed in the future to make the program more modular.  But
                 for now, we temporarily zap the source with new compressed
                 proof and see if there are any $d violations by looking at
                 the error message output */
              saveZappedProofSectionPtr
                  = statement[proveStatement].proofSectionPtr;
              saveZappedProofSectionLen
                  = statement[proveStatement].proofSectionLen;

              /****** Obsolete due to 3-May-2017 update
              /@ (search for "chr(1)" above for explanation) @/
              let(&str1, cat(chr(1), "\n", str1, " $.\n", NULL));
              statement[proveStatement].proofSectionPtr = str1 + 1; /@ Compressed
                                                     proof generated above @/
              statement[proveStatement].proofSectionLen =
                  newCompressedLength + 4;
              *******/

              /******** start of 16-Jun-2017 nm ************/
              saveZappedProofSectionChanged
                  = statement[proveStatement].proofSectionChanged;
              /* Set flag that this is not the original source */
              statement[proveStatement].proofSectionChanged = 1;
              /* str1 has the new compressed trial proof after minimization */
              /* Put space before and after to satisfy "space around token"
                 requirement, to prevent possible error messages, and also
                 add "$." since parseCompressedProof() expects it */
              let(&str1, cat(" ", str1, " $.", NULL));
              /* Don't include the "$." in the length */
              statement[proveStatement].proofSectionLen = (long)strlen(str1) - 2;
              /* We don't deallocate previous proofSectionPtr content because
                 we will recover it after the verifyProof() */
              statement[proveStatement].proofSectionPtr = str1;
              /******** end of 16-Jun-2017 nm ************/

              outputToString = 1; /* Suppress error messages */
              /* i = parseProof(proveStatement); */
              /* if (i != 0) bug(1121); */
              /* i = verifyProof(proveStatement); */
              /* if (i != 0) bug(1122); */
              /* 15-Apr-2015 nm parseProof, verifyProof, cleanWkrProof must be
                 called in sequence to assign the wrkProof structure, verify
                 the proof, and deallocate the wrkProof structure.  Either none
                 of them or all of them must be called. */
              parseProof(proveStatement);
              verifyProof(proveStatement); /* Must be called even if error
                                  occurred in parseProof, to init RPN stack
                                  for cleanWrkProof() */
              /* 15-Apr-2015 nm - don't change proof if there is an error
                 (which could be pre-existing). */
              i = (wrkProof.errorSeverity > 1);
              /**** Here we look at the screen output sent to a string.
                    This is rather crude, and someday the ability to
                    check proofs and $d violations should be modularized *****/
              j = instr(1, printString,
                  "There is a disjoint variable ($d) violation");
              outputToString = 0; /* Restore to normal output */
              let(&printString, ""); /* Clear out the stored error messages */
              cleanWrkProof(); /* Deallocate verifyProof storage */
              statement[proveStatement].proofSectionPtr
                  = saveZappedProofSectionPtr;
              statement[proveStatement].proofSectionLen
                  = saveZappedProofSectionLen;
              statement[proveStatement].proofSectionChanged
                  = saveZappedProofSectionChanged; /* 16-Jun-2017 nm */
              if (i != 0 || j != 0) {
                /* There was a verify proof error (j!=0) or $d violation (i!=0)
                   so don't used minimized proof */
                /* Revert the proof to before minimization */
                copyProofStruct(&proofInProgress, saveProofForReverting);
                /* Skip further printout and flag setting */
                continue; /* Continue at 'Next k' loop end below */
              }
            } /* if (nmbrLen(statement[k].reqDisjVarsA)) */

            /* 25-Jun-2014 nm - not needed since trials now suppressed */
            /*
            if (verboseMode) {
              print2("\n");
            }
            */

            /*if (nmbrLen(statement[k].reqDisjVarsA) || !allowGrowthFlag) {*/

            /* 3-May-2016 nm */
            /* Warn user if a discouraged statement is overridden */
            if (getMarkupFlag(k, USAGE_DISCOURAGED)) {
              if (!overrideFlag) bug(1126);
              /* print2("\n"); */ /* Enable for more emphasis */
              print2(
                  ">>> ?Warning: Overriding discouraged usage of \"%s\".\n",
                  statement[k].labelName);
              /* print2("\n"); */ /* Enable for more emphasis */
            }

            if (!allowGrowthFlag) {
              /* Note:  this is the length BEFORE indentation and wrapping,
                 so it is less than SHOW PROOF ... /SIZE */
              if (newCompressedLength < oldCompressedLength) {
                print2(
     "Proof of \"%s\" decreased from %ld to %ld bytes using \"%s\".\n",
                    statement[proveStatement].labelName,
                    oldCompressedLength, newCompressedLength,
                    statement[k].labelName);
              } else {
                if (newCompressedLength > oldCompressedLength) bug(1123);
                print2(
     "Proof of \"%s\" stayed at %ld bytes using \"%s\".\n",
                    statement[proveStatement].labelName,
                    oldCompressedLength,
                    statement[k].labelName);
                print2(
    "    (Uncompressed steps decreased from %ld to %ld).\n",
                    m, n );
              }
              /* (We don't care about compressed length if ALLOW_GROWTH) */
              oldCompressedLength = newCompressedLength;
            }

            if (n < m && (allowGrowthFlag || verboseMode)) {
              print2(
      "%sProof of \"%s\" decreased from %ld to %ld steps using \"%s\".\n",
                (allowGrowthFlag ? "" : "    "),
                statement[proveStatement].labelName,
                m, n, statement[k].labelName);
            }
            /* ALLOW_GROWTH possibility */
            if (m < n) print2(
      "Proof of \"%s\" increased from %ld to %ld steps using \"%s\".\n",
                statement[proveStatement].labelName,
                m, n, statement[k].labelName);
            /* ALLOW_GROWTH possibility */
            if (m == n) print2(
                "Proof of \"%s\" remained at %ld steps using \"%s\".\n",
                statement[proveStatement].labelName,
                m, statement[k].labelName);
            /* Distinct variable warning (obsolete) */
            /*
            if (nmbrLen(statement[k].reqDisjVarsA)) {
              printLongLine(cat("Note: \"", statement[k].labelName,
                  "\" has $d constraints.",
                  "  SAVE NEW_PROOF then VERIFY PROOF to check them.",
                  NULL), "", " ");
            }
            */
            /* q = 0; */ /* Line length for label list */ /* 25-Jun-2014 del */
            prntStatus = 2; /* Found one */
            proofChangedFlag = 1;

            /* 20-May-2012 nm */
            /*
            if (forbidMatchList[0]) { /@ User provided a /FORBID list @/
              /@ Save the changed proof in case we have to restore
                 it later @/
              copyProofStruct(&saveProofForReverting, proofInProgress);
            }
            */
            /* 25-Jun-2014 nm */
            /* Save the changed proof in case we have to restore
               it later */
            copyProofStruct(&saveProofForReverting, proofInProgress);

          }

        } /* Next k (statement) */
        /* 25-Jun-2014 nm - not needed since trials now suppressed */
        /*
        if (verboseMode) {
          if (prntStatus) print2("\n");
        }
        */

        if (proofChangedFlag && forwRevPass == 2) {
          /* 25-Jun-2014 nm */
          /* Check whether the reverse pass found a better proof than the
             forward pass */
          if (verboseMode) {
            print2(
"Forward vs. backward: %ld vs. %ld bytes; %ld vs. %ld steps\n",
                      forwardCompressedLength,
                      oldCompressedLength,
                      forwardLength,
                      nmbrLen(proofInProgress.proof));
          }
          if (oldCompressedLength < forwardCompressedLength
               || (oldCompressedLength == forwardCompressedLength &&
                   nmbrLen(proofInProgress.proof) < forwardLength)) {
            /* The reverse pass was better */
            print2("The backward scan results were used.\n");
          } else {
            copyProofStruct(&proofInProgress, save1stPassProof);
            print2("The forward scan results were used.\n");
          }
        }

      } /* next forwRevPass */

      if (prntStatus == 1 && !allowGrowthFlag)
        print2("No shorter proof was found.\n");
      if (prntStatus == 1 && allowGrowthFlag)
        print2("The proof was not changed.\n");
      if (!prntStatus /* && !noDistinctFlag */)
        print2("?No earlier %s$p or $a label matches \"%s\".\n",
            (overrideFlag ? "" : "(allowed) "),  /* 3-May-2016 nm */
            fullArg[1]);
      /* 25-Jun-2014 nm /NO_DISTINCT is obsolete
      if (!prntStatus && noDistinctFlag) {
        /@ 30-Jan-06 nm Added single-character-match wildcard argument @/
        if (instr(1, fullArg[1], "@") || instr(1, fullArg[1], "?"))
          print2("?No earlier $p or $a label (without $d) matches \"%s\".\n",
              fullArg[1]);
      }
      */
      /* 28-Jun-2011 nm */
      if (!mathboxFlag && proveStatement >= sandboxStmt) {
        print2(
  "(Other mathboxes were not checked.  Use / INCLUDE_MATHBOXES to include them.)\n");
      }

      /* 16-Aug-2016 nm */
      if (printTime == 1) {
        getRunTime(&timeIncr);
        print2("MINIMIZE_WITH run time = %7.2f sec for \"%s\"\n", timeIncr,
            statement[proveStatement].labelName);
      }

      /* 20-May-2013 nm */
      if (forbidMatchList[0]) { /* User provided a /FORBID list */
        /*deallocProofStruct(&saveProofForReverting);*/ /* Deallocate memory */
        let(&forbidMatchList, ""); /* Deallocate memory */
      }

      /* 22-Nov-2014 nm */
      if (noNewAxiomsMatchList[0]) { /* User provided /NO_NEW_AXIOMS_FROM list */
        let(&noNewAxiomsMatchList, ""); /* Deallocate memory */
        let(&traceProofFlags, ""); /* Deallocate memory */
        let(&traceTrialFlags, ""); /* Deallocate memory */
      }

      /* 25-Jun-2014 nm */
      deallocProofStruct(&saveProofForReverting); /* Deallocate memory */
      deallocProofStruct(&saveOrigProof); /* Deallocate memory */
      deallocProofStruct(&save1stPassProof); /* Deallocate memory */

      if (proofChangedFlag) {
        proofChanged = 1; /* Cumulative flag */
        processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
      }
      continue;

    } /* End if MINIMIZE_WITH */


    /* 11-Sep-2016 nm Added EXPAND command */
    if (cmdMatches("EXPAND")) {

      proofChangedFlag = 0;
      nmbrLet(&nmbrSaveProof, proofInProgress.proof);
      s = compressedProofSize(nmbrSaveProof, proveStatement);

      for (i = proveStatement - 1; i >= 1; i--) {
        if (statement[i].type != (char)p_) continue; /* Not a $p */
        /* 30-Jan-06 nm Added single-character-match wildcard argument */
        if (!matchesList(statement[i].labelName, fullArg[1], '*', '?')) {
          continue;
        }
        sourceStatement = i;

        nmbrTmp = expandProof(nmbrSaveProof,
            sourceStatement /*, proveStatement*/);

        if (!nmbrEq(nmbrTmp, nmbrSaveProof)) {
          proofChangedFlag = 1;
          n = compressedProofSize(nmbrTmp, proveStatement);
          printLongLine(cat("Proof of \"",
            statement[proveStatement].labelName, "\" ",
            (s == n ? cat("stayed at ", str((double)s), NULL)
                : cat((s < n ? "increased from " : " decreased from "),
                    str((double)s), " to ", str((double)n), NULL)),
            " bytes after expanding \"",
            statement[sourceStatement].labelName, "\".", NULL), " ", " ");
          s = n;
          nmbrLet(&nmbrSaveProof, nmbrTmp);
        }
      }

      if (proofChangedFlag) {
        proofChanged = 1; /* Cumulative flag */
        /* Clear the existing proof structure */
        deallocProofStruct(&proofInProgress);
        /* Then rebuild proof structure from new proof nmbrTmp */
        initProofStruct(&proofInProgress, nmbrTmp, proveStatement);
        /* Save the new proof structure on the undo stack */
        processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
      } else {
        print2("No expansion occurred.  The proof was not changed.\n");
      }
      nmbrLet(&nmbrSaveProof, NULL_NMBRSTRING);
      nmbrLet(&nmbrTmp, NULL_NMBRSTRING);
      continue;
    } /* EXPAND */


    if (cmdMatches("DELETE STEP") || (cmdMatches("DELETE ALL"))) {

      if (cmdMatches("DELETE STEP")) {
        s = (long)val(fullArg[2]); /* Step number */
      } else {
        s = nmbrLen(proofInProgress.proof);
      }
      if ((proofInProgress.proof)[s - 1] == -(long)'?') {
        print2("?Step %ld is unknown and cannot be deleted.\n", s);
        continue;
      }
      m = nmbrLen(proofInProgress.proof); /* Original proof length */
      if (s > m || s < 1) {
        print2("?The step must be in the range from 1 to %ld.\n", m);
        continue;
      }

      deleteSubProof(s - 1);
      n = nmbrLen(proofInProgress.proof); /* New proof length */
      if (m == n) {
        print2("Step %ld was deleted.\n", s);
      } else {
        if (n > 1) {
          printLongLine(cat("A ", str((double)(m - n + 1)),
              "-step subproof at step ", str((double)s),
              " was deleted.  Steps ", str((double)s), ":",
              str((double)m), " are now ", str((double)(s - m + n)), ":",
              str((double)n), ".",
              NULL),
              "", " ");
        } else {
          print2("The entire proof was deleted.\n");
        }
      }

      /* 6/14/98 - Automatically display new unknown steps
         ???Future - add switch to enable/defeat this */
      typeProof(proveStatement,
          1 /*pipFlag*/,
          0 /*startStep*/,
          0 /*endStep*/,
          0 /*endIndent*/,
          1 /*essentialFlag*/,
          0 /*renumberFlag*/,
          1 /*unknownFlag*/,
          0 /*notUnifiedFlag*/,
          0 /*reverseFlag*/,
          0 /*noIndentFlag*/,
          0 /*splitColumn*/,
          0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
          0 /*texFlag*/,
          0 /*htmlFlag*/);
      /* 6/14/98 end */

      proofChanged = 1; /* Cumulative flag */
      processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);

      continue;

    }

    if (cmdMatches("DELETE FLOATING_HYPOTHESES")) {

      /* Get the essential step flags */
      nmbrLet(&nmbrTmp, nmbrGetEssential(proofInProgress.proof));

      m = nmbrLen(proofInProgress.proof); /* Original proof length */

      n = 0; /* Earliest step that changed */
      proofChangedFlag = 0;

      for (s = m; s > 0; s--) {

        /* Skip essential steps and unknown steps */
        if (nmbrTmp[s - 1] == 1) continue; /* Not floating */
        if ((proofInProgress.proof)[s - 1] == -(long)'?') continue; /* Unknown */

        /* Get the subproof length at step s */
        q = subproofLen(proofInProgress.proof, s - 1);

        deleteSubProof(s - 1);

        n = s - q + 1; /* Save earliest step changed */
        proofChangedFlag = 1;
        s = s - q + 1; /* Adjust step position to account for deleted subpr */
      } /* Next step s */

      if (proofChangedFlag) {
        print2("All floating-hypothesis steps were deleted.\n");

        if (n) {
          print2("Steps %ld and above have been renumbered.\n", n);
        }

        /* 6/14/98 - Automatically display new unknown steps
           ???Future - add switch to enable/defeat this */
        typeProof(proveStatement,
            1 /*pipFlag*/,
            0 /*startStep*/,
            0 /*endStep*/,
            0 /*endIndent*/,
            1 /*essentialFlag*/,
            0 /*renumberFlag*/,
            1 /*unknownFlag*/,
            0 /*notUnifiedFlag*/,
            0 /*reverseFlag*/,
            0 /*noIndentFlag*/,
            0 /*splitColumn*/,
            0 /*skipRepeatedSteps*/, /* 28-Jun-2013 nm */
            0 /*texFlag*/,
            0 /*htmlFlag*/);
        /* 6/14/98 end */

        proofChanged = 1; /* Cumulative flag */
        processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
      } else {
        print2("?There are no floating-hypothesis steps to delete.\n");
      }

      continue;

    } /* End if DELETE FLOATING_HYPOTHESES */

    if (cmdMatches("INITIALIZE")) {

      if (cmdMatches("INITIALIZE ALL")) {
        i = nmbrLen(proofInProgress.proof);

        /* Reset the dummy variable counter (all will be refreshed) */
        pipDummyVars = 0;

        /* Initialize all steps */
        for (j = 0; j < i; j++) {
          initStep(j);
        }

        /* Assign known subproofs */
        assignKnownSubProofs();

        print2("All steps have been initialized.\n");
        proofChanged = 1; /* Cumulative flag */
        processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
        continue;
      }

        /* Added 16-Apr-06 nm */
      if (cmdMatches("INITIALIZE USER")) {
        i = nmbrLen(proofInProgress.proof);
        /* Delete all LET STEP assignments */
        for (j = 0; j < i; j++) {
          nmbrLet((nmbrString **)(&((proofInProgress.user)[j])),
              NULL_NMBRSTRING);
        }
        print2(
      "All LET STEP user assignments have been initialized (i.e. deleted).\n");
        proofChanged = 1; /* Cumulative flag */
        processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
        continue;
      }
      /* End 16-Apr-06 */

      /* cmdMatches("INITIALIZE STEP") */
      s = (long)val(fullArg[2]); /* Step number */
      if (s > nmbrLen(proofInProgress.proof) || s < 1) {
        print2("?The step must be in the range from 1 to %ld.\n",
            nmbrLen(proofInProgress.proof));
        continue;
      }

      initStep(s - 1);

      /* Also delete LET STEPs, per HELP INITIALIZE */          /* 16-Apr-06 */
      nmbrLet((nmbrString **)(&((proofInProgress.user)[s - 1])),  /* 16-Apr-06 */
              NULL_NMBRSTRING);                                 /* 16-Apr-06 */

      print2(
          "Step %ld and its hypotheses have been initialized.\n",
          s);

      proofChanged = 1; /* Cumulative flag */
      processUndoStack(&proofInProgress, PUS_PUSH, fullArgString, 0);
      continue;

    }


    if (cmdMatches("SEARCH")) {
      if (switchPos("/ ALL")) {
        m = 1;  /* Include $e, $f statements */
      } else {
        m = 0;  /* Show $a, $p only */
      }

      /* 14-Apr-2008 nm added */
      if (switchPos("/ JOIN")) {
        joinFlag = 1;  /* Join $e's to $a,$p for matching */
      } else {
        joinFlag = 0;  /* Search $a,$p by themselves */
      }

      if (switchPos("/ COMMENTS")) {
        n = 1;  /* Search comments */
      } else {
        n = 0;  /* Search statement math symbols */
      }

      let(&str1, fullArg[2]); /* String to match */

      if (n) { /* COMMENTS switch */
        /* Trim leading, trailing spaces; reduce white space to space;
           convert to upper case */
        let(&str1, edit(str1, 8 + 16 + 128 + 32));
      } else { /* No COMMENTS switch */
        /* Trim leading, trailing spaces; reduce white space to space */
        let(&str1, edit(str1, 8 + 16 + 128));

        /* Change all spaces to double spaces */
        q = (long)strlen(str1);
        let(&str3, space(q + q));
        s = 0;
        for (p = 0; p < q; p++) {
          str3[p + s] = str1[p];
          if (str1[p] == ' ') {
            s++;
            str3[p + s] = str1[p];
          }
        }
        let(&str1, left(str3, q + s));

        /* 30-Jan-06 nm Added single-character-match wildcard argument "$?"
           (or "?" for convenience).  Use ASCII 3 for the exactly-1-char
           wildcard character.  This is a single-character
           match, not a single-token match:  we need "??" to match "ph". */
        while (1) {
          p = instr(1, str1, "$?");
          if (!p) break;
          let(&str1, cat(left(str1, p - 1), chr(3), right(str1, p + 2), NULL));
        }
        /* Allow just "?" for convenience. */
        while (1) {
          p = instr(1, str1, "?");
          if (!p) break;
          let(&str1, cat(left(str1, p - 1), chr(3), right(str1, p + 1), NULL));
        }
        /* End of 30-Jan-06 addition */

        /* Change wildcard to ASCII 2 (to be different from printable chars) */
        /* 1/3/02 (Why are we matching with and without space? I'm not sure.)*/
        /* 30-Jan-06 nm Answer:  We need the double-spacing, and the removal
           of space in the "with spaces" case, so that "ph $ ph" will match
           "ph  ph" (0-token case) - "ph  $  ph" won't match this in the
           (character-based, not token-based) matches().  The "with spaces"
           case is for matching whole tokens, whereas the "without spaces"
           case is for matching part of a token. */
        while (1) {
          p = instr(1, str1, " $* ");
          if (!p) break;
          /* This removes the space before and after the $* */
          let(&str1, cat(left(str1, p - 1), chr(2), right(str1, p + 4), NULL));
        }
        while (1) {
          p = instr(1, str1, "$*");
          if (!p) break;
          /* This simply replaces $* with chr(2) */
          let(&str1, cat(left(str1, p - 1), chr(2), right(str1, p + 2), NULL));
        }
        /* 1/3/02  Also allow a plain $ as a wildcard, for convenience. */
        while (1) {
          p = instr(1, str1, " $ ");
          if (!p) break;
          /* 30-Jan-06 nm Bug fix - changed "2" to "3" below in order to
             properly match 0 tokens */
          let(&str1, cat(left(str1, p - 1), chr(2), right(str1, p + 3), NULL));
        }
        while (1) {
          /* Note: the "$" shortcut must be done last to avoid picking up
             "$*" and "$?". */
          p = instr(1, str1, "$");
          if (!p) break;
          let(&str1, cat(left(str1, p - 1), chr(2), right(str1, p + 1), NULL));
        }

        /* Add wildcards to beginning and end to match middle of any string */
        let(&str1, cat(chr(2), " ", str1, " ", chr(2), NULL));
      } /* End no COMMENTS switch */

      for (i = 1; i <= statements; i++) {
        if (!statement[i].labelName[0]) continue; /* No label */
        if (!m && statement[i].type != (char)p_ &&
            statement[i].type != (char)a_) {
          continue; /* No /ALL switch */
        }
        /* 30-Jan-06 nm Added single-character-match wildcard argument */
        if (!matchesList(statement[i].labelName, fullArg[1], '*', '?'))
          continue;
        if (n) { /* COMMENTS switch */
          let(&str2, "");
          str2 = getDescription(i); /* str2 must be deallocated here */
          /* Strip linefeeds and reduce spaces; cvt to uppercase */
          j = instr(1, edit(str2, 4 + 8 + 16 + 128 + 32), str1);
          if (!j) { /* No match */
            let(&str2, "");
            continue;
          }
          /* Strip linefeeds and reduce spaces */
          let(&str2, edit(str2, 4 + 8 + 16 + 128));
          j = j + ((long)strlen(str1) / 2); /* Center of match location */
          p = screenWidth - 7 - (long)strlen(str((double)i)) - (long)strlen(statement[i].labelName);
                        /* Longest comment portion that will fit in one line */
          q = (long)strlen(str2); /* Length of comment */
          if (q <= p) { /* Use entire comment */
            let(&str3, str2);
          } else {
            if (q - j <= p / 2) { /* Use right part of comment */
              let(&str3, cat("...", right(str2, q - p + 4), NULL));
            } else {
              if (j <= p / 2) { /* Use left part of comment */
                let(&str3, cat(left(str2, p - 3), "...", NULL));
              } else { /* Use middle part of comment */
                let(&str3, cat("...", mid(str2, j - p / 2, p - 6), "...",
                    NULL));
              }
            }
          }
          print2("%s\n", cat(str((double)i), " ", statement[i].labelName, " $",
              chr(statement[i].type), " \"", str3, "\"", NULL));
          let(&str2, "");
        } else { /* No COMMENTS switch */
          let(&str2,nmbrCvtMToVString(statement[i].mathString));

          /* 14-Apr-2008 nm JOIN flag */
          tmpFlag = 0; /* Flag that $p or $a is already in string */
          if (joinFlag && (statement[i].type == (char)p_ ||
              statement[i].type == (char)a_)) {
            /* If $a or $p, prepend $e's to string to match */
            k = nmbrLen(statement[i].reqHypList);
            for (j = k - 1; j >= 0; j--) {
              p = statement[i].reqHypList[j];
              if (statement[p].type == (char)e_) {
                let(&str2, cat("$e ",
                    nmbrCvtMToVString(statement[p].mathString),
                    tmpFlag ? "" : cat(" $", chr(statement[i].type), NULL),
                    " ", str2, NULL));
                tmpFlag = 1; /* Flag that a $p or $a was added */
              }
            }
          }

          /* Change all spaces to double spaces */
          q = (long)strlen(str2);
          let(&str3, space(q + q));
          s = 0;
          for (p = 0; p < q; p++) {
            str3[p + s] = str2[p];
            if (str2[p] == ' ') {
              s++;
              str3[p + s] = str2[p];
            }
          }
          let(&str2, left(str3, q + s));

          let(&str2, cat(" ", str2, " ", NULL));
          /* 30-Jan-06 nm Added single-character-match wildcard argument */
          /* We should use matches() and not matchesList() here, because
             commas can be legal token characters in math symbols */
          if (!matches(str2, str1, 2/* ascii 2 0-or-more-token match char*/,
              3/* ascii 3 single-token-match char*/))
            continue;
          let(&str2, edit(str2, 8 + 16 + 128)); /* Trim leading, trailing
              spaces; reduce white space to space */
          printLongLine(cat(str((double)i)," ",
              statement[i].labelName,
              tmpFlag ? "" : cat(" $", chr(statement[i].type), NULL),
              " ", str2,
              NULL), "    ", " ");
        } /* End no COMMENTS switch */
      } /* Next i */
      continue;
    }


    if (cmdMatches("SET ECHO")) {
      if (cmdMatches("SET ECHO ON")) {
        commandEcho = 1;
        /* 15-Jun-2009 nm Added "!" (see 15-Jun-2009 note above) */
        print2("!SET ECHO ON\n");
        print2("Command line echoing is now turned on.\n");
      } else {
        commandEcho = 0;
        print2("Command line echoing is now turned off.\n");
      }
      continue;
    }

    if (cmdMatches("SET MEMORY_STATUS")) {
      if (cmdMatches("SET MEMORY_STATUS ON")) {
        print2("Memory status display has been turned on.\n");
        print2("This command is intended for debugging purposes only.\n");
        memoryStatus = 1;
      } else {
        memoryStatus = 0;
        print2("Memory status display has been turned off.\n");
      }
      continue;
    }


    if (cmdMatches("SET JEREMY_HENTY_FILTER")) {
      if (cmdMatches("SET JEREMY_HENTY_FILTER ON")) {
        print2("The unification equivalence filter has been turned on.\n");
        print2("This command is intended for debugging purposes only.\n");
        hentyFilter = 1;
      } else {
        print2("This command is intended for debugging purposes only.\n");
        print2("The unification equivalence filter has been turned off.\n");
        hentyFilter = 0;
      }
      continue;
    }


    if (cmdMatches("SET EMPTY_SUBSTITUTION")) {
      if (cmdMatches("SET EMPTY_SUBSTITUTION ON")) {
        minSubstLen = 0;
        print2("Substitutions with empty symbol sequences is now allowed.\n");
        continue;
      }
      if (cmdMatches("SET EMPTY_SUBSTITUTION OFF")) {
        minSubstLen = 1;
        printLongLine(cat("The ability to substitute empty expressions",
            " for variables  has been turned off.  Note that this may",
            " make the Proof Assistant too restrictive in some cases.",
            NULL),
            "", " ");
        continue;
      }
    }


    if (cmdMatches("SET SEARCH_LIMIT")) {
      s = (long)val(fullArg[2]); /* Timeout value */
      print2("IMPROVE search limit has been changed from %ld to %ld\n",
          userMaxProveFloat, s);
      userMaxProveFloat = s;
      continue;
    }

    if (cmdMatches("SET WIDTH")) { /* 18-Nov-85 nm Was SCREEN_WIDTH */
      s = (long)val(fullArg[2]); /* Screen width value */
      if (s >= PRINTBUFFERSIZE - 1) {
        print2(
"?Maximum screen width is %ld.  Recompile with larger PRINTBUFFERSIZE in\n",
            (long)(PRINTBUFFERSIZE - 2));
        print2("mminou.h if you need more.\n");
        continue;
      }
      /* 26-Sep-2017 nm */
      /* TODO: figure out why s=2 crashes program! */
      if (s < 3) s = 3; /* Less than 3 may cause a segmentation fault */
      i = screenWidth;
      screenWidth = s; /* 26-Sep-2017 nm - print with new screen width */
      print2("Screen width has been changed from %ld to %ld.\n",
          i, s);
      continue;
    }


    if (cmdMatches("SET HEIGHT")) {  /* 18-Nov-05 nm Added */
      s = (long)val(fullArg[2]); /* Screen height value */
      if (s < 2) s = 2;  /* Less than 2 makes no sense */
      i = screenHeight;
      screenHeight = s - 1;
      print2("Screen height has been changed from %ld to %ld.\n",
          i + 1, s);
      /* screenHeight is one less than the physical screen to account for the
         prompt line after pausing. */
      continue;
    }


    /* 10-Jul-2016 nm */
    if (cmdMatches("SET DISCOURAGEMENT")) {
      if (!strcmp(fullArg[2], "ON")) {
        globalDiscouragement = 1;
        print2("\"(...is discouraged.)\" markup tags are now honored.\n");
      } else if (!strcmp(fullArg[2], "OFF")) {
        print2(
    "\"(...is discouraged.)\" markup tags are no longer honored.\n");
        /* print2("\n"); */ /* Enable for more emphasis */
        print2(
">>> ?Warning: This setting is intended for advanced users only.  Please turn\n");
        print2(
">>> it back ON if you are not intimately familiar with this database.\n");
        /* print2("\n"); */ /* Enable for more emphasis */
        globalDiscouragement = 0;
      } else {
        bug(1129);
      }
      continue;
    }


    /* 14-May-2017 nm */
    if (cmdMatches("SET CONTRIBUTOR")) {
      print2("\"Contributed by...\" name was changed from \"%s\" to \"%s\"\n",
          contributorName, fullArg[2]);
      let(&contributorName, fullArg[2]);
      continue;
    }


    /* 31-Dec-2017 nm */
    if (cmdMatches("SET ROOT_DIRECTORY")) {
      let(&str1, rootDirectory); /* Save previous one */
      let(&rootDirectory, edit(fullArg[2], 2/*discard spaces,tabs*/));
      if (rootDirectory[0] != 0) {  /* Not an empty directory path */
        /* Add trailing "/" to rootDirectory if missing */
        if (instr(1, rootDirectory, "\\") != 0
            || instr(1, input_fn, "\\") != 0
            || instr(1, output_fn, "\\") != 0 ) {
          /* Using Windows-style path (not really supported, but at least
             make full path consistent) */
          if (rootDirectory[strlen(rootDirectory) - 1] != '\\') {
            let(&rootDirectory, cat(rootDirectory, "\\", NULL));
          }
        } else {
          if (rootDirectory[strlen(rootDirectory) - 1] != '/') {
            let(&rootDirectory, cat(rootDirectory, "/", NULL));
          }
        }
      }
      if (strcmp(str1, rootDirectory)){
        print2("Root directory was changed from \"%s\" to \"%s\"\n",
            str1, rootDirectory);
      }
      let(&str1, "");
      continue;
    }


    /* 1-Nov-2013 nm Added UNDO */
    if (cmdMatches("SET UNDO")) {
      s = (long)val(fullArg[2]); /* Maximum UNDOs */
      if (s < 0) s = 0;  /* Less than 0 UNDOs makes no sense */
      /* Reset the stack size if it changed */
      if (processUndoStack(NULL, PUS_GET_SIZE, "", 0) != s) {
        print2(
            "The maximum number of UNDOs was changed from %ld to %ld\n",
            processUndoStack(NULL, PUS_GET_SIZE, "", 0), s);
        processUndoStack(NULL, PUS_NEW_SIZE, "", s);
        if (PFASmode == 1) {
          /* If we're in the Proof Assistant, assign the first stack
             entry with the current proof (the stack was erased) */
          processUndoStack(&proofInProgress, PUS_PUSH, "", 0);
        }
      } else {
        print2("The maximum number of UNDOs was not changed.\n");
      }
      continue;
    }


    if (cmdMatches("SET UNIFICATION_TIMEOUT")) {
      s = (long)val(fullArg[2]); /* Timeout value */
      print2("Unification timeout has been changed from %ld to %ld\n",
          userMaxUnifTrials,s);
      userMaxUnifTrials = s;
      continue;
    }


    if (cmdMatches("OPEN LOG")) {
        /* Open a log file */
        let(&logFileName, fullArg[2]);
        logFilePtr = fSafeOpen(logFileName, "w", 0/*noVersioningFlag*/);
        if (!logFilePtr) continue; /* Couldn't open it (err msg was provided) */
        logFileOpenFlag = 1;
        print2("The log file \"%s\" was opened %s %s.\n",logFileName,
            date(),time_());
        continue;
    }

    if (cmdMatches("CLOSE LOG")) {
        /* Close the log file */
        if (!logFileOpenFlag) {
          print2("?Sorry, there is no log file currently open.\n");
        } else {
          print2("The log file \"%s\" was closed %s %s.\n",logFileName,
              date(),time_());
          fclose(logFilePtr);
          logFileOpenFlag = 0;
        }
        let(&logFileName,"");
        continue;
    }

    if (cmdMatches("OPEN TEX")) {
    /* 2-Oct-2017 nm OPEN HTML is very obsolete, no need to warn anymore
    if (cmdMatches("OPEN TEX") || cmdMatches("OPEN HTML") ) {
      if (cmdMatches("OPEN HTML")) {
        print2("?OPEN HTML is obsolete - use SHOW STATEMENT * / HTML\n");
        continue;
      }
    */

      /* 17-Nov-2015 TODO: clean up mixed LaTeX/HTML attempts (check
         texFileOpenFlag when switching to HTML & close LaTeX file) */

      if (texDefsRead) {
        /* Current limitation - can only read .def once */
        /* 2-Oct-2017 nm OPEN HTML is obsolete */
        /*if (cmdMatches("OPEN HTML") != htmlFlag) {*/
        if (htmlFlag) {
          /* Actually it isn't clear to me this is still the case, but
             to be safe I left it in */
          print2("?You cannot use both LaTeX and HTML in the same session.\n");
          print2(
              "?You must EXIT and restart Metamath to switch to the other.\n");
          continue;
        }
      }
      /* 2-Oct-2017 nm OPEN HTML is obsolete */
      /*htmlFlag = cmdMatches("OPEN HTML");*/

      /* Open a TeX file */
      let(&texFileName,fullArg[2]);
      if (switchPos("/ NO_HEADER")) {
        texHeaderFlag = 0;
      } else {
        texHeaderFlag = 1;
      }
      /* 14-Sep-2010 nm Added OLD_TEX */
      if (switchPos("/ OLD_TEX")) {
        oldTexFlag = 1;
      } else {
        oldTexFlag = 0;
      }
      texFilePtr = fSafeOpen(texFileName, "w", 0/*noVersioningFlag*/);
      if (!texFilePtr) continue; /* Couldn't open it (err msg was provided) */
      texFileOpenFlag = 1;
      /* 2-Oct-2017 nm OPEN HTML is obsolete */
      print2("Created %s output file \"%s\".\n",
          htmlFlag ? "HTML" : "LaTeX", texFileName);
      printTexHeader(texHeaderFlag);
      oldTexFlag = 0;
      continue;
    }

    /* 2-Oct-2017 nm CLOSE HTML is obsolete */
    /******
    if (cmdMatches("CLOSE TEX") || cmdMatches("CLOSE HTML")) {
      if (cmdMatches("CLOSE HTML")) {
        print2("?CLOSE HTML is obsolete - use SHOW STATEMENT @ / HTML\n");
        continue;
      }
      /@ Close the TeX file @/
      if (!texFileOpenFlag) {
        print2("?Sorry, there is no %s file currently open.\n",
            htmlFlag ? "HTML" : "LaTeX");
      } else {
        print2("The %s output file \"%s\" has been closed.\n",
            htmlFlag ? "HTML" : "LaTeX", texFileName);
        printTexTrailer(texHeaderFlag);
        fclose(texFilePtr);
        texFileOpenFlag = 0;
      }
      let(&texFileName,"");
      continue;
    }
    *****/
    if (cmdMatches("CLOSE TEX")) {
      /* Close the TeX file */
      if (!texFileOpenFlag) {
        print2("?Sorry, there is no LaTeX file currently open.\n");
      } else {
        print2("The LaTeX output file \"%s\" has been closed.\n",
            texFileName);
        printTexTrailer(texHeaderFlag);
        fclose(texFilePtr);
        texFileOpenFlag = 0;
      }
      let(&texFileName,"");
      continue;
    }

    /* Similar to Unix 'more' */
    if (cmdMatches("MORE")) {
      list1_fp = fSafeOpen(fullArg[1], "r", 0/*noVersioningFlag*/);
      if (!list1_fp) continue; /* Couldn't open it (error msg was provided) */
      while (1) {
        if (!linput(list1_fp, NULL, &str1)) break; /* End of file */
        /* Print a line on the screen */
        if (!print2("%s\n", str1)) break; /* User typed Q */
      }
      fclose(list1_fp);
      continue;
    } /* end MORE */


    if (cmdMatches("FILE SEARCH")) {
      /* Search the contents of a file and type on the screen */

      type_fp = fSafeOpen(fullArg[2], "r", 0/*noVersioningFlag*/);
      if (!type_fp) continue; /* Couldn't open it (error msg was provided) */
      fromLine = 0;
      toLine = 0;
      searchWindow = 0;
      i = switchPos("/ FROM_LINE");
      if (i) fromLine = (long)val(fullArg[i + 1]);
      i = switchPos("/ TO_LINE");
      if (i) toLine = (long)val(fullArg[i + 1]);
      i = switchPos("/ WINDOW");
      if (i) searchWindow = (long)val(fullArg[i + 1]);
      /*??? Implement SEARCH /WINDOW */
      if (i) print2("Sorry, WINDOW has not be implemented yet.\n");

      let(&str2, fullArg[3]); /* Search string */
      let(&str2, edit(str2, 32)); /* Convert to upper case */

      tmpFlag = 0;

      /* Search window buffer */
      pntrLet(&pntrTmp, pntrSpace(searchWindow));

      j = 0; /* Line # */
      m = 0; /* # matches */
      while (linput(type_fp, NULL, &str1)) {
        j++;
        if (j > toLine && toLine != 0) break;
        if (j >= fromLine || fromLine == 0) {
          let(&str3, edit(str1, 32)); /* Convert to upper case */
          if (instr(1, str3, str2)) { /* Match occurred */
            if (!tmpFlag) {
              tmpFlag = 1;
              print2(
                    "The line number in the file is shown before each line.\n");
            }
            m++;
            if (!print2("%ld:  %s\n", j, left(str1,
                MAX_LEN - (long)strlen(str((double)j)) - 3))) break;
          }
        }
        for (k = 1; k < searchWindow; k++) {
          let((vstring *)(&pntrTmp[k - 1]), pntrTmp[k]);
        }
        if (searchWindow > 0)
            let((vstring *)(&pntrTmp[searchWindow - 1]), str1);
      }
      if (!tmpFlag) {
        print2("There were no matches.\n");
      } else {
        if (m == 1) {
          print2("There was %ld matching line in the file %s.\n", m,
              fullArg[2]);
        } else {
          print2("There were %ld matching lines in the file %s.\n", m,
              fullArg[2]);
        }
      }

      fclose(type_fp);

      /* Deallocate search window buffer */
      for (i = 0; i < searchWindow; i++) {
        let((vstring *)(&pntrTmp[i]), "");
      }
      pntrLet(&pntrTmp, NULL_PNTRSTRING);


      continue;
    }


    if (cmdMatches("SET UNIVERSE") || cmdMatches("ADD UNIVERSE") ||
        cmdMatches("DELETE UNIVERSE")) {

      /*continue;*/ /* ???Not implemented */
    } /* end if xxx UNIVERSE */



    if (cmdMatches("SET DEBUG FLAG")) {
      print2("Notice:  The DEBUG mode is intended for development use only.\n");
      print2("The printout will not be meaningful to the user.\n");
      i = (long)val(fullArg[3]);
      if (i == 4) db4 = 1;  /* Not used */
      if (i == 5) db5 = 1;  /* mmpars.c statistics; mmunif.c overview */
      if (i == 6) db6 = 1;  /* mmunif.c details */
      if (i == 7) db7 = 1;  /* mmunif.c more details; mmveri.c */
      if (i == 8) db8 = 1;  /* mmpfas.c unification calls */
      if (i == 9) db9 = 1;  /* memory */ /* use SET MEMORY_STATUS ON instead */
      continue;
    }
    if (cmdMatches("SET DEBUG OFF")) {
      db4 = 0;
      db5 = 0;
      db6 = 0;
      db7 = 0;
      db8 = 0;
      db9 = 0;
      print2("The DEBUG mode has been turned off.\n");
      continue;
    }

    if (cmdMatches("ERASE")) {
      if (sourceChanged) {
        print2("Warning:  You have not saved changes to the source.\n");
        str1 = cmdInput1("Do you want to ERASE anyway (Y, N) <N>? ");
        if (str1[0] != 'y' && str1[0] != 'Y') {
          print2("Use WRITE SOURCE to save the changes.\n");
          continue;
        }
        sourceChanged = 0;
      }
      eraseSource();
      sourceHasBeenRead = 0; /* Global variable */ /* 31-Dec-2017 nm */
      showStatement = 0;
      proveStatement = 0;
      print2("Metamath has been reset to the starting state.\n");
      continue;
    }

    if (cmdMatches("VERIFY PROOF")) {
      if (switchPos("/ SYNTAX_ONLY")) {
        verifyProofs(fullArg[2],0); /* Parse only */
      } else {
        verifyProofs(fullArg[2],1); /* Parse and verify */
      }
      continue;
    }

    /* 7-Nov-2015 nm Added */  /* 17-Nov-2015 nm Updated */
    if (cmdMatches("VERIFY MARKUP")) {
      i = (switchPos("/ DATE_SKIP") != 0) ? 1 : 0;
      m = (switchPos("/ TOP_DATE_SKIP") != 0) ? 1 : 0;
      j = (switchPos("/ FILE_SKIP") != 0) ? 1 : 0;
      k = (switchPos("/ VERBOSE") != 0) ? 1 : 0;
      if (i == 1 && m == 1) {
        printf(
            "?Only one of / DATE_SKIP and / TOP_DATE_SKIP may be specified.\n");
        continue;
      }
      verifyMarkup(fullArg[2],
          (flag)i, /* 1 = skip checking date consistency */
          (flag)m, /* 1 = skip checking top date only */
          (flag)j, /* 1 = skip checking external files GIF, mmset.html,... */
          (flag)k); /* 1 = verbose mode */  /* 26-Dec-2016 nm */
      continue;
    }

    /* 10-Dec-2018 nm Added */
    if (cmdMatches("MARKUP")) {
      htmlFlag = 1;
      altHtmlFlag = (switchPos("/ ALT_HTML") != 0);
      if ((switchPos("/ HTML") != 0) == (switchPos("/ ALT_HTML") != 0)) {
        print2("?Please specify exactly one of / HTML and / ALT_HTML.\n");
        continue;
      }
      i = 0;
      i = ((switchPos("/ SYMBOLS") != 0) ? PROCESS_SYMBOLS : 0)
          + ((switchPos("/ LABELS") != 0) ? PROCESS_LABELS : 0)
          + ((switchPos("/ NUMBER_AFTER_LABEL") != 0) ? ADD_COLORED_LABEL_NUMBER : 0)
          + ((switchPos("/ BIB_REFS") != 0) ? PROCESS_BIBREFS : 0)
          + ((switchPos("/ UNDERSCORES") != 0) ? PROCESS_UNDERSCORES : 0);
      processMarkup(fullArg[1], /* Input file */
          fullArg[2],  /* Output file */
          (switchPos("/ CSS") != 0),
          i); /* Action bits */
      continue;
    }

    print2("?This command has not been implemented.\n");
    continue;

  }
} /* command */


