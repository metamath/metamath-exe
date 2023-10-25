/*****************************************************************************/
/*        Copyright (C) 2021  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

// This module processes LaTeX and HTML output.

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmpars.h" // For rawSourceError and mathSrchCmp and lookupLabel
#include "mmwtex.h"
#include "mmcmdl.h" // For g_texFileName

// All LaTeX and HTML definitions are taken from the source
// file (read in the by READ... command).  In the source file, there should
// be a single comment $( ... $) containing the keyword $t.  The definitions
// start after the $t and end at the $).  Between $t and $), the definition
// source should exist.  See the file set.mm for an example.

flag g_oldTexFlag = 0; // Use TeX macros in output (obsolete)

flag g_htmlFlag = 0; // HTML flag: 0 = TeX, 1 = HTML
// Use "althtmldef" instead of "htmldef".  This is intended to allow the
// generation of pages with the Unicode font instead of the individual GIF files.
flag g_altHtmlFlag = 0;
// Output statement lists only, for statement display in other HTML pages,
// such as the Proof Explorer home page.
flag g_briefHtmlFlag = 0;
// At this statement and above, use the exthtmlxxx
// variables for title, links, etc.  This was put in to allow proper
// generation of the Hilbert Space Explorer extension to the set.mm
// database.
long g_extHtmlStmt = 0;

// Globals to hold mathbox information.  They should be re-initialized
// by the ERASE command (eraseSource()).  g_mathboxStmt = 0 indicates
// it and the other variables haven't been initialized.
// At this statement and above, use SANDBOX_COLOR background for theorem,
// mmrecent, & mmbiblio lists.
// 0 means it hasn't been looked up yet; g_statements + 1 means there is
// no mathbox.
long g_mathboxStmt = 0;
long g_mathboxes = 0; // # of mathboxes
// The following 3 strings are 0-based e.g. g_mathboxStart[0] is for
// mathbox #1.
nmbrString_def(g_mathboxStart); // Start stmt vs. mathbox #
nmbrString_def(g_mathboxEnd); // End stmt vs. mathbox #
pntrString_def(g_mathboxUser); // User name vs. mathbox #

// This is the list of characters causing the space before the opening "`"
// in a math string in a comment to be removed for HTML output.
#define OPENING_PUNCTUATION "(['\""
// This is the list of characters causing the space after the closing "`"
// in a math string in a comment to be removed for HTML output.
#define CLOSING_PUNCTUATION ".,;)?!:]'\"_-"

/*!
 * \def QUOTED_SPACE
 * The general line wrapping algorithm looks out for spaces as break positions.
 * To prevent a quote delimited by __"__ be broken down, spaces are temporarily
 * replaced with 0x03 (ETX, end of transmission), hopefully never used in
 * text in this application.
 */
#define QUOTED_SPACE 3 // ASCII 3 that temporarily zaps a space

// Tex output file
FILE *g_texFilePtr = NULL;
flag g_texFileOpenFlag = 0;

// Global variables
flag g_texDefsRead = 0;
struct texDef_struct *g_TexDefs;

// Variables local to this module (except some $t variables)
long numSymbs;
// Substitute character for $ in converting to obsolete $l,$m,$n
// comments - Use '$' instead of non-printable ASCII 2 for debugging.
#define DOLLAR_SUBST 2

// Variables set by the language in the set.mm etc. $t statement
// Some of these are global; see mmwtex.h
vstring_def(g_htmlCSS); // Set by g_htmlCSS commands
vstring_def(g_htmlFont); // Set by htmlfont commands
vstring_def(g_htmlVarColor); // Set by htmlvarcolor commands
vstring_def(htmlExtUrl); // Set by htmlexturl command
vstring_def(htmlTitle); // Set by htmltitle command
  vstring_def(htmlTitleAbbr); // Extracted from htmlTitle
vstring_def(g_htmlHome); // Set by htmlhome command
  // Future - assign these in the $t set.mm comment instead of g_htmlHome
  vstring_def(g_htmlHomeHREF); // Extracted from g_htmlHome
  vstring_def(g_htmlHomeIMG); // Extracted from g_htmlHome
vstring_def(g_htmlBibliography); // Optional; set by htmlbibliography command
vstring_def(extHtmlLabel); // Set by exthtmllabel command - where extHtml starts
vstring_def(g_extHtmlTitle); // Set by exthtmltitle command (global!)
  vstring_def(g_extHtmlTitleAbbr); // Extracted from htmlTitle
vstring_def(extHtmlHome); // Set by exthtmlhome command
  // Future - assign these in the $t set.mm comment instead of g_htmlHome
  vstring_def(extHtmlHomeHREF); // Extracted from extHtmlHome
  vstring_def(extHtmlHomeIMG); // Extracted from extHtmlHome
vstring_def(extHtmlBibliography); // Optional; set by exthtmlbibliography command
vstring_def(htmlDir); // Directory for GIF version, set by htmldir command
// Directory for Unicode Font version, set by althtmldir command
vstring_def(altHtmlDir);

// Sandbox stuff
vstring_def(sandboxHome);
  vstring_def(sandboxHomeHREF); // Extracted from extHtmlHome
  vstring_def(sandboxHomeIMG); // Extracted from extHtmlHome
vstring_def(sandboxTitle);
  vstring_def(sandboxTitleAbbr);

// Variables holding all HTML <a name..> tags from bibliography pages
vstring_def(g_htmlBibliographyTags);
vstring_def(extHtmlBibliographyTags);

void eraseTexDefs(void) {
  // Deallocate the texdef/htmldef storage
  if (g_texDefsRead) { // If not (already deallocated or never allocated)
    g_texDefsRead = 0;

    for (long i = 0; i < numSymbs; i++) { // Deallocate structure member i
      free_vstring(g_TexDefs[i].tokenName);
      free_vstring(g_TexDefs[i].texEquiv);
    }
    free(g_TexDefs); // Deallocate the structure
  }
  free_vstring(htmlTitle);
  free_vstring(g_htmlHome);
  free_vstring(g_htmlCSS);
  free_vstring(g_htmlFont);
  free_vstring(g_htmlVarColor);
  free_vstring(htmlExtUrl);
  free_vstring(htmlTitle);
  free_vstring(htmlTitleAbbr);
  free_vstring(g_htmlHome);
  free_vstring(g_htmlHomeHREF);
  free_vstring(g_htmlHomeIMG);
  free_vstring(g_htmlBibliography);
  free_vstring(extHtmlLabel);
  free_vstring(g_extHtmlTitle);
  free_vstring(g_extHtmlTitleAbbr);
  free_vstring(extHtmlHome);
  free_vstring(extHtmlHomeHREF);
  free_vstring(extHtmlHomeIMG);
  free_vstring(extHtmlBibliography);
  free_vstring(htmlDir);
  free_vstring(altHtmlDir);
  free_vstring(sandboxHome);
  free_vstring(sandboxHomeHREF);
  free_vstring(sandboxHomeIMG);
  free_vstring(sandboxTitle);
  free_vstring(sandboxTitleAbbr);
  free_vstring(g_htmlBibliographyTags);
  free_vstring(extHtmlBibliographyTags);
  return;
} // eraseTexDefs

// Returns 2 if there were severe parsing errors, 1 if there were warnings but
// no errors, 0 if no errors or warnings.
flag readTexDefs(
  flag errorsOnly, // 1 = suppress non-error messages
  flag gifCheck) // 1 = check for missing GIFs
{

  char *startPtr;
  long lineNumOffset = 0;
  char *fbPtr;
  char *tmpPtr;
  char *tmpPtr2;
  long charCount;
  long i, j, k, p;
  long lineNum;
  long tokenLength;
  char zapChar;
  long cmd;
  long parsePass;
  vstring_def(token);
  vstring_def(partialToken);
  FILE *tmpFp;
  static flag saveHtmlFlag = -1; // -1 to force 1st read
  static flag saveAltHtmlFlag = 1; // -1 to force 1st read
  flag warningFound = 0; // 1 if a warning was found
  // Pointer to label section of statement with the $t comment
  char *dollarTStmtPtr = NULL;
  // bsearch returned values for use in error-checking
  void *g_mathKeyPtr; // bsearch returned value for math symbol lookup
  void *texDefsPtr; // For binary search

  if (saveHtmlFlag != g_htmlFlag || saveAltHtmlFlag != g_altHtmlFlag
      || !g_texDefsRead) {
    // One or both changed - we need to erase and re-read
    eraseTexDefs();
    saveHtmlFlag = g_htmlFlag; // Save for next call to readTexDefs()
    saveAltHtmlFlag = g_altHtmlFlag; // Save for next call to readTexDefs()
    if (g_htmlFlag == 0 /* Tex */ && g_altHtmlFlag == 1) {
      bug(2301); // Nonsensical combination
    }
  } else {
    // Nothing changed; don't need to read again
    return 0; // No errors
  }

  // Initial values below will be overridden if a user assignment exists in the
  // $t comment of the xxx.mm input file.
  // Set by htmltitle command in $t comment
  let(&htmlTitle, "Metamath Test Page");
  // Set by htmlhome command in $t comment
  let(&g_htmlHome, cat("<A HREF=\"mmset.html\"><FONT SIZE=-2 FACE=sans-serif>",
    "<IMG SRC=\"mm.gif\" BORDER=0 ALT=",
    "\"Home\" HEIGHT=32 WIDTH=32 ALIGN=MIDDLE STYLE=\"margin-bottom:0px\">",
    "Home</FONT></A>", NULL));

  if (errorsOnly == 0) {
    print2("Reading definitions from $t statement of %s...\n", g_input_fn);
  }

  // Find the comment with the $t.
  // This used to point to the input file buffer of an external
  // latex.def file; now it's from the xxx.mm $t comment, so we
  // make it a normal string.
  vstring_def(fileBuf);

  // Note that g_Statement[g_statements + 1] is a special (empty) statement whose
  // labelSection holds any comment after the last statement.
  for (i = 1; i <= g_statements + 1; i++) {
    // We do low-level stuff on the xxx.mm input file buffer for speed
    tmpPtr = g_Statement[i].labelSectionPtr;
    j = g_Statement[i].labelSectionLen;
    // Note that for g_Statement[g_statements + 1], the lineNum is one plus the
    // number of lines in the file.
    // Save for later (Don't save if we're in scan to end for double $t detection)
    if (!fileBuf[0]) lineNumOffset = g_Statement[i].lineNum;
    zapChar = tmpPtr[j]; // Save the original character
    tmpPtr[j] = 0; // Create an end-of-string
    if (instr(1, tmpPtr, "$t")) {
      // Found a $t comment.
      // Make sure this isn't a second one in another statement.
      // (A second one in the labelSection of one statement will trigger an error below.)
      if (fileBuf[0]) {
        print2("?Error: There are two comments containing a $t keyword in \"%s\".\n",
            g_input_fn);
        free_vstring(fileBuf);
        return 2;
      }
      let(&fileBuf, tmpPtr);
      tmpPtr[j] = zapChar;
      dollarTStmtPtr = g_Statement[i].labelSectionPtr;
      // break; // Continue to end to detect double $t
    }
    tmpPtr[j] = zapChar; // Restore the xxx.mm input file buffer
  } // next i
  // If the $t wasn't found, fileBuf will be "", causing error message below.
  // Compute line number offset of beginning of g_Statement[i].labelSection for
  // use in error messages.
  j = (long)strlen(fileBuf);
  for (i = 0; i < j; i++) {
    if (fileBuf[i] == '\n') lineNumOffset--;
  }

#define LATEXDEF 1
#define HTMLDEF 2
#define HTMLVARCOLOR 3
#define HTMLTITLE 4
#define HTMLHOME 5
#define ALTHTMLDEF 6
#define EXTHTMLTITLE 7
#define EXTHTMLHOME 8
#define EXTHTMLLABEL 9
#define HTMLDIR 10
#define ALTHTMLDIR 11
#define HTMLBIBLIOGRAPHY 12
#define EXTHTMLBIBLIOGRAPHY 13
#define HTMLCSS 14
#define HTMLFONT 15
#define HTMLEXTURL 16

  startPtr = fileBuf;

  // Find $t command
  while (1) {
    if (startPtr[0] == '$') {
      if (startPtr[1] == 't') {
        startPtr++;
        break;
      }
    }
    if (startPtr[0] == 0) break;
    startPtr++;
  }
  if (startPtr[0] == 0) {
    print2("?Error: There is no $t command in the file \"%s\".\n", g_input_fn);
    print2(
"The file should have exactly one comment of the form $(...$t...$) with\n");
    print2("the LaTeX and HTML definitions between $t and $).\n");
    free_vstring(fileBuf);
    return 2;
  }
  startPtr++; // Move to 1st char after $t

  // Search for the ending $) and zap the $) to be end-of-string
  tmpPtr = startPtr;
  while (1) {
    if (tmpPtr[0] == '$') {
      if (tmpPtr[1] == ')') {
        break;
      }
    }
    if (tmpPtr[0] == 0) break;
    tmpPtr++;
  }
  if (tmpPtr[0] == 0) {
    print2(
  "?Error: There is no $) comment closure after the $t keyword in \"%s\".\n",
        g_input_fn);
    free_vstring(fileBuf);
    return 2;
  }

  // Make sure there aren't two comments with $t commands
  tmpPtr2 = tmpPtr;
  while (1) {
    if (tmpPtr2[0] == '$') {
      if (tmpPtr2[1] == 't') {
        print2(
  "?Error: There are two comments containing a $t keyword in \"%s\".\n",
            g_input_fn);
        free_vstring(fileBuf);
        return 2;
      }
    }
    if (tmpPtr2[0] == 0) break;
    tmpPtr2++;
  }

  // Force end of string at the $ in $)
  tmpPtr[0] = '\n';
  tmpPtr[1] = 0;

  charCount = tmpPtr + 1 - fileBuf; // For bug check

  for (parsePass = 1; parsePass <= 2; parsePass++) {
    // Pass 1 - Count the number of symbols defined and alloc g_TexDefs array.
    // Pass 2 - Assign the texDefs array.
    numSymbs = 0;
    fbPtr = startPtr;

    while (1) {

      // Get next token
      fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
      tokenLength = texDefTokenLen(fbPtr);

      // Process token - command
      if (!tokenLength) break; // End of file
      zapChar = fbPtr[tokenLength]; // Char to restore after zapping source
      fbPtr[tokenLength] = 0; // Create end of string
      cmd = lookup(fbPtr,
          "latexdef,htmldef,htmlvarcolor,htmltitle,htmlhome"
          ",althtmldef,exthtmltitle,exthtmlhome,exthtmllabel,htmldir"
          ",althtmldir,htmlbibliography,exthtmlbibliography"
          ",htmlcss,htmlfont,htmlexturl"
          );
      fbPtr[tokenLength] = zapChar;
      if (cmd == 0) {
        lineNum = lineNumOffset;
        for (i = 0; i < (fbPtr - fileBuf); i++) {
          if (fileBuf[i] == '\n') lineNum++;
        }
        rawSourceError(/* fileBuf */ g_sourcePtr,
            /* fbPtr */ dollarTStmtPtr + (fbPtr - fileBuf), tokenLength,
            cat("Expected \"latexdef\", \"htmldef\", \"htmlvarcolor\",",
            " \"htmltitle\", \"htmlhome\", \"althtmldef\",",
            " \"exthtmltitle\", \"exthtmlhome\", \"exthtmllabel\",",
            " \"htmldir\", \"althtmldir\",",
            " \"htmlbibliography\", \"exthtmlbibliography\",",
            " \"htmlcss\", \"htmlfont\",",
            " or \"htmlexturl\" here.",
            NULL));
        free_vstring(fileBuf);
        return 2;
      }
      fbPtr = fbPtr + tokenLength;

      if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME
          && cmd != EXTHTMLTITLE && cmd != EXTHTMLHOME && cmd != EXTHTMLLABEL
          && cmd != HTMLDIR && cmd != ALTHTMLDIR
          && cmd != HTMLBIBLIOGRAPHY && cmd != EXTHTMLBIBLIOGRAPHY
          && cmd != HTMLCSS && cmd != HTMLFONT && cmd != HTMLEXTURL) {
        // Get next token - string in quotes
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLength = texDefTokenLen(fbPtr);

        // Process token - string in quotes
        if (fbPtr[0] != '\"' && fbPtr[0] != '\'') {
          if (!tokenLength) { // Abnormal end-of-file
            fbPtr--; // Format for error message
            tokenLength++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(/* fileBuf */ g_sourcePtr,
            /* fbPtr */ dollarTStmtPtr + (fbPtr - fileBuf),
              tokenLength,
              "Expected a quoted string here.");
          free_vstring(fileBuf);
          return 2;
        }
        if (parsePass == 2) {
          zapChar = fbPtr[tokenLength - 1]; // Chr to restore after zapping src
          fbPtr[tokenLength - 1] = 0; // Create end of string
          // Get ASCII token; note that leading and trailing quotes are omitted.
          let(&token, fbPtr + 1);
          fbPtr[tokenLength - 1] = zapChar;

          // Change double internal quotes to single quotes.
          // Do this only for double quotes matching the
          // outer quotes.  fbPtr[0] is the quote character.
          if (fbPtr[0] != '\"' && fbPtr[0] != '\'') bug(2329);
          j = (long)strlen(token);
          for (i = 0; i < j - 1; i++) {
            if (token[i] == fbPtr[0] &&
                token[i + 1] == fbPtr[0]) {
              let(&token, cat(left(token,
                  i + 1), right(token, i + 3), NULL));
              j--;
            }
          }

          if ((cmd == LATEXDEF && !g_htmlFlag)
              || (cmd == HTMLDEF && g_htmlFlag && !g_altHtmlFlag)
              || (cmd == ALTHTMLDEF && g_htmlFlag && g_altHtmlFlag)) {
            g_TexDefs[numSymbs].tokenName = "";
            let(&(g_TexDefs[numSymbs].tokenName), token);
          }
        } // if (parsePass == 2)

        fbPtr = fbPtr + tokenLength;
      } // if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME...)

      if (cmd != HTMLVARCOLOR && cmd != HTMLTITLE && cmd != HTMLHOME
          && cmd != EXTHTMLTITLE && cmd != EXTHTMLHOME && cmd != EXTHTMLLABEL
          && cmd != HTMLDIR && cmd != ALTHTMLDIR
          && cmd != HTMLBIBLIOGRAPHY && cmd != EXTHTMLBIBLIOGRAPHY
          && cmd != HTMLCSS && cmd != HTMLFONT && cmd != HTMLEXTURL) {
        // Get next token -- "as"
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLength = texDefTokenLen(fbPtr);
        zapChar = fbPtr[tokenLength]; // Char to restore after zapping source
        fbPtr[tokenLength] = 0; // Create end of string
        if (strcmp(fbPtr, "as")) {
          if (!tokenLength) { // Abnormal end-of-file
            fbPtr--; // Format for error message
            tokenLength++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(/* fileBuf */ g_sourcePtr,
            /* fbPtr */ dollarTStmtPtr + (fbPtr - fileBuf),
              tokenLength,
              "Expected the keyword \"as\" here.");
          free_vstring(fileBuf);
          return 2;
        }
        fbPtr[tokenLength] = zapChar;
        fbPtr = fbPtr + tokenLength;
      } // if (cmd != HTMLVARCOLOR && ...

      if (parsePass == 2) {
        // Initialize LaTeX/HTML equivalent
        let(&token, "");
      }

      // Scan   "<string>" + "<string>" + ...   until ";" found
      while (1) {

        // Get next token - string in quotes
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLength = texDefTokenLen(fbPtr);
        if (fbPtr[0] != '\"' && fbPtr[0] != '\'') {
          if (!tokenLength) { // Abnormal end-of-file
            fbPtr--; // Format for error message
            tokenLength++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') lineNum++;
          }
          rawSourceError(/* fileBuf */ g_sourcePtr,
            /* fbPtr */ dollarTStmtPtr + (fbPtr - fileBuf),
              tokenLength,
              "Expected a quoted string here.");
          free_vstring(fileBuf);
          return 2;
        }
        if (parsePass == 2) {
          zapChar = fbPtr[tokenLength - 1]; // Chr to restore after zapping src
          fbPtr[tokenLength - 1] = 0; // Create end of string
          // Get ASCII token; note that leading and trailing quotes are omitted.
          let(&partialToken, fbPtr + 1);
          fbPtr[tokenLength - 1] = zapChar;

          // Change double internal quotes to single quotes.
          // Do this only for double quotes matching the
          // outer quotes.  fbPtr[0] is the quote character.
          if (fbPtr[0] != '\"' && fbPtr[0] != '\'') bug(2330);
          j = (long)strlen(partialToken);
          for (i = 0; i < j - 1; i++) {
            if (partialToken[i] == fbPtr[0] &&
                partialToken[i + 1] == fbPtr[0]) {
              let(&partialToken, cat(left(partialToken,
                  i + 1), right(partialToken, i + 3), NULL));
              j--;
            }
          }

          // Check that string is on a single line
          tmpPtr2 = strchr(partialToken, '\n');
          if (tmpPtr2 != NULL) {
            lineNum = lineNumOffset;
            for (i = 0; i < (fbPtr - fileBuf); i++) {
              if (fileBuf[i] == '\n') lineNum++;
            }

            rawSourceError(/* fileBuf */ g_sourcePtr,
                /* fbPtr */ dollarTStmtPtr + (fbPtr - fileBuf),
                tmpPtr2 - partialToken + 1 /* tokenLength on current line */,
                "String should be on a single line.");
          }

          // Combine the string part to the main token we're building
          let(&token, cat(token, partialToken, NULL));
        } // (parsePass == 2)

        fbPtr = fbPtr + tokenLength;

        // Get next token - "+" or ";"
        fbPtr = fbPtr + texDefWhiteSpaceLen(fbPtr);
        tokenLength = texDefTokenLen(fbPtr);
        if ((fbPtr[0] != '+' && fbPtr[0] != ';') || tokenLength != 1) {
          if (!tokenLength) { // Abnormal end-of-file
            fbPtr--; // Format for error message
            tokenLength++;
          }
          lineNum = lineNumOffset;
          for (i = 0; i < (fbPtr - fileBuf); i++) {
            if (fileBuf[i] == '\n') {
              lineNum++;
            }
          }
          rawSourceError(/* fileBuf */ g_sourcePtr,
            /* fbPtr */ dollarTStmtPtr + (fbPtr - fileBuf),
              tokenLength, // lineNum, g_input_fn,
              "Expected \"+\" or \";\" here.");
          free_vstring(fileBuf);
         return 2;
        }
        fbPtr = fbPtr + tokenLength;

        if (fbPtr[-1] == ';') break;
      } // End while

      if (parsePass == 2) {
        if ((cmd == LATEXDEF && !g_htmlFlag)
            || (cmd == HTMLDEF && g_htmlFlag && !g_altHtmlFlag)
            || (cmd == ALTHTMLDEF && g_htmlFlag && g_altHtmlFlag)) {
          g_TexDefs[numSymbs].texEquiv = "";
          let(&(g_TexDefs[numSymbs].texEquiv), token);
        }
        if (cmd == HTMLVARCOLOR) {
          let(&g_htmlVarColor, cat(g_htmlVarColor, " ", token, NULL));
        }
        if (cmd == HTMLTITLE) {
          let(&htmlTitle, token);
        }
        if (cmd == HTMLHOME) {
          let(&g_htmlHome, token);
        }
        if (cmd == EXTHTMLTITLE) {
          let(&g_extHtmlTitle, token);
        }
        if (cmd == EXTHTMLHOME) {
          let(&extHtmlHome, token);
        }
        if (cmd == EXTHTMLLABEL) {
          let(&extHtmlLabel, token);
        }
        if (cmd == HTMLDIR) {
          let(&htmlDir, token);
        }
        if (cmd == ALTHTMLDIR) {
          let(&altHtmlDir, token);
        }
        if (cmd == HTMLBIBLIOGRAPHY) {
          let(&g_htmlBibliography, token);
        }
        if (cmd == EXTHTMLBIBLIOGRAPHY) {
          let(&extHtmlBibliography, token);
        }
        if (cmd == HTMLCSS) {
          let(&g_htmlCSS, token);
          // User's CSS
          // Convert characters "\n" to new line - maybe do for other fields too?
          do {
            p = instr(1, g_htmlCSS, "\\n");
            if (p != 0) {
              let(&g_htmlCSS, cat(left(g_htmlCSS, p - 1), "\n",
                  right(g_htmlCSS, p + 2), NULL));
            }
          } while (p != 0);
        }
        if (cmd == HTMLFONT) {
          let(&g_htmlFont, token);
        }
        if (cmd == HTMLEXTURL) {
          let(&htmlExtUrl, token);
        }
      }

      if ((cmd == LATEXDEF && !g_htmlFlag)
          || (cmd == HTMLDEF && g_htmlFlag && !g_altHtmlFlag)
          || (cmd == ALTHTMLDEF && g_htmlFlag && g_altHtmlFlag)) {
        numSymbs++;
      }
    } // End while

    if (fbPtr != fileBuf + charCount) bug(2305);

    if (parsePass == 1 ) {
      if (errorsOnly == 0) {
        print2("%ld typesetting statements were read from \"%s\".\n",
            numSymbs, g_input_fn);
      }
      g_TexDefs = malloc((size_t)numSymbs * sizeof(struct texDef_struct));
      if (!g_TexDefs) outOfMemory("#99 (TeX symbols)");
    }
  } // next parsePass

  // Sort the tokens for later lookup
  qsort(g_TexDefs, (size_t)numSymbs, sizeof(struct texDef_struct), texSortCmp);

  // Check for duplicate definitions
  for (i = 1; i < numSymbs; i++) {
    if (!strcmp(g_TexDefs[i].tokenName, g_TexDefs[i - 1].tokenName)) {
      printLongLine(cat("?Warning: Token ", g_TexDefs[i].tokenName,
          " is defined more than once in ",
          g_htmlFlag
            ? (g_altHtmlFlag ? "an althtmldef" : "an htmldef")
            : "a latexdef",
          " statement.", NULL),
          "", " ");
      warningFound = 1;
    }
  }

  // Check to make sure all definitions are for a real math token
  for (i = 0; i < numSymbs; i++) {
    // Note:  g_mathKey, g_mathTokens, and mathSrchCmp are assigned or defined
    // in mmpars.c.
    g_mathKeyPtr = (void *)bsearch(g_TexDefs[i].tokenName, g_mathKey,
        (size_t)g_mathTokens, sizeof(long), mathSrchCmp);
    if (!g_mathKeyPtr) {
      printLongLine(cat("?Warning: The token \"", g_TexDefs[i].tokenName,
          "\", which was defined in ",
          g_htmlFlag
            ? (g_altHtmlFlag ? "an althtmldef" : "an htmldef")
            : "a latexdef",
          " statement, was not declared in any $v or $c statement.", NULL),
          "", " ");
      warningFound = 1;
    }
  }

  // Check to make sure all math tokens have typesetting definitions
  for (i = 0; i < g_mathTokens; i++) {
    texDefsPtr = (void *)bsearch(g_MathToken[i].tokenName, g_TexDefs,
        (size_t)numSymbs, sizeof(struct texDef_struct), texSrchCmp);
    if (!texDefsPtr) {
      printLongLine(cat("?Warning: The token \"", g_MathToken[i].tokenName,
       "\", which was defined in a $v or $c statement, was not declared in ",
          g_htmlFlag
            ? (g_altHtmlFlag ? "an althtmldef" : "an htmldef")
            : "a latexdef",
          " statement.", NULL),
          "", " ");
      warningFound = 1;
    }
  }

  // Check to make sure all GIFs are present
  if (g_htmlFlag) {
    for (i = 0; i < numSymbs; i++) {
      tmpPtr = g_TexDefs[i].texEquiv;
      k = 0;
      while (1) {
        // Note that only an exact match with "IMG SRC=" is currently handled
        j = instr(k + 1, tmpPtr, "IMG SRC=");
        if (j == 0) break;
        // Get position of trailing quote
        // Future:  use strchr instead of mid() for efficiency?
        k = instr(j + 9, g_TexDefs[i].texEquiv, mid(tmpPtr, j + 8, 1));
        let(&token, seg(tmpPtr, j + 9, k - 1)); // Get name of .gif (.png)
        // Future: we may want to issue "missing trailing quote" warning.
        // (We test k after the let() so that the temporary string stack
        // entry created by mid() is emptied and won't overflow.)
        if (k == 0) break;
        if (gifCheck) {
          tmpFp = fopen(token, "r"); // See if it exists
          if (!tmpFp) {
            printLongLine(cat("?Warning: The file \"", token,
                "\", which is referenced in an htmldef",
                " statement, was not found.", NULL),
                "", " ");
            warningFound = 1;
          } else {
            fclose(tmpFp);
          }
        }
      }
    }
  }

  // Look up the extended database start label
  if (extHtmlLabel[0]) {
    for (i = 1; i <= g_statements; i++) {
      if (!strcmp(extHtmlLabel, g_Statement[i].labelName)) break;
    }
    if (i > g_statements) {
      printLongLine(cat("?Warning: There is no statement with label \"",
          extHtmlLabel,
          "\" (specified by exthtmllabel in the database source $t comment).  ",
          "Use SHOW LABELS for a list of valid labels.", NULL), "", " ");
      warningFound = 1;
    }
    g_extHtmlStmt = i;
  } else {
    // There is no extended database; set threshold to beyond end of db
    g_extHtmlStmt = g_statements + 1;
  }

  assignMathboxInfo();
  // In case there is not extended (Hilbert Space Explorer) section,
  // but there is a sandbox section, make the extended section "empty".
  if (g_extHtmlStmt == g_statements + 1) g_extHtmlStmt = g_mathboxStmt;
  let(&sandboxHome, cat("<A HREF=\"mmtheorems.html#sandbox:bighdr\">",
    "<FONT SIZE=-2 FACE=sans-serif>",
    "<IMG SRC=\"_sandbox.gif\" BORDER=0 ALT=",
    "\"Table of Contents\" HEIGHT=32 WIDTH=32 ALIGN=MIDDLE>",
    "Table of Contents</FONT></A>", NULL));
  let(&sandboxHomeHREF, "mmtheorems.html#sandbox:bighdr");
  let(&sandboxHomeIMG, "_sandbox.gif");
  let(&sandboxTitleAbbr, "Users' Mathboxes");
  let(&sandboxTitle, "Users' Mathboxes");

  // Extract derived variables from the $t variables.
  // (In the future, it might be better to do this directly in the $t.)
  i = instr(1, g_htmlHome, "HREF=\"") + 5;
  if (i == 5) {
    printLongLine(
        "?Warning: In the $t comment, htmlhome has no 'HREF=\"'.", "", " ");
    warningFound = 1;
  }
  j = instr(i + 1, g_htmlHome, "\"");
  let(&g_htmlHomeHREF, seg(g_htmlHome, i + 1, j - 1));
  i = instr(1, g_htmlHome, "IMG SRC=\"") + 8;
  if (i == 8) {
    printLongLine(
        "?Warning: In the $t comment, htmlhome has no 'IMG SRC=\"'.", "", " ");
    warningFound = 1;
  }
  j = instr(i + 1, g_htmlHome, "\"");
  let(&g_htmlHomeIMG, seg(g_htmlHome, i + 1, j - 1));

  // Compose abbreviated title from capital letters
  j = (long)strlen(htmlTitle);
  let(&htmlTitleAbbr, "");
  for (i = 1; i <= j; i++) {
    if (htmlTitle[i - 1] >= 'A' && htmlTitle[i -1] <= 'Z') {
      let(&htmlTitleAbbr, cat(htmlTitleAbbr, chr(htmlTitle[i - 1]), NULL));
    }
  }
  let(&htmlTitleAbbr, cat(htmlTitleAbbr, " Home", NULL));

  if (g_extHtmlStmt < g_statements + 1 // If extended section exists
      && g_extHtmlStmt != g_mathboxStmt) { // and is not an empty dummy section
    i = instr(1, extHtmlHome, "HREF=\"") + 5;
    if (i == 5) {
      printLongLine(
          "?Warning: In the $t comment, exthtmlhome has no 'HREF=\"'.", "", " ");
      warningFound = 1;
    }
    j = instr(i + 1, extHtmlHome, "\"");
    let(&extHtmlHomeHREF, seg(extHtmlHome, i + 1, j - 1));
    i = instr(1, extHtmlHome, "IMG SRC=\"") + 8;
    if (i == 8) {
      printLongLine(
          "?Warning: In the $t comment, exthtmlhome has no 'IMG SRC=\"'.", "", " ");
      warningFound = 1;
    }
    j = instr(i + 1, extHtmlHome, "\"");
    let(&extHtmlHomeIMG, seg(extHtmlHome, i + 1, j - 1));
    // Compose abbreviated title from capital letters
    j = (long)strlen(g_extHtmlTitle);
    let(&g_extHtmlTitleAbbr, "");
    for (i = 1; i <= j; i++) {
      if (g_extHtmlTitle[i - 1] >= 'A' && g_extHtmlTitle[i -1] <= 'Z') {
        let(&g_extHtmlTitleAbbr, cat(g_extHtmlTitleAbbr,
            chr(g_extHtmlTitle[i - 1]), NULL));
      }
    }
    let(&g_extHtmlTitleAbbr, cat(g_extHtmlTitleAbbr, " Home", NULL));
  }

  free_vstring(token); // Deallocate
  free_vstring(partialToken); // Deallocate
  free_vstring(fileBuf);
  g_texDefsRead = 1; // Set global flag that it's been read in
  // Return indicator that parsing passed (0) or had warning(s) (1)
  return warningFound;
} // readTexDefs

// This function returns the length of the white space starting at ptr.
// Comments are considered white space.  ptr should point to the first character
// of the white space.  If ptr does not point to a white space character, 0
// is returned.  If ptr points to a null character, 0 is returned.
long texDefWhiteSpaceLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  char *ptr1;
  while (1) {
    tmpchr = ptr[i];
    if (!tmpchr) return i; // End of string
    if (isalnum((unsigned char)(tmpchr))) return i; // Alphanumeric string
    // Embedded c-style comment - used to ignore comments inside of Metamath
    // comment for LaTeX/HTML definitions.
    if (tmpchr == '/') {
      if (ptr[i + 1] == '*') {
        while (1) {
          ptr1 = strchr(ptr + i + 2, '*');
          if (!ptr1) {
            return i + (long)strlen(&ptr[i]); // Unterminated comment - goto EOF
          }
          if (ptr1[1] == '/') break;
          i = ptr1 - ptr;
        }
        i = ptr1 - ptr + 2;
        continue;
      } else {
        return i;
      }
    }
    if (isgraph((unsigned char)tmpchr)) return i;
    i++;
  }
  bug(2307);
  return 0; // Dummy return - never executed
} // texDefWhiteSpaceLen

// This function returns the length of the token (non-white-space) starting at
// ptr.  Comments are considered white space.  ptr should point to the first
// character of the token.  If ptr points to a white space character, 0
// is returned.  If ptr points to a null character, 0 is returned.  If ptr
// points to a quoted string, the quoted string is returned.  A non-alphanumeric
// characters ends a token and is a single token.
long texDefTokenLen(char *ptr)
{
  long i = 0;
  char tmpchr;
  char *ptr1;
  tmpchr = ptr[i];
  if (tmpchr == '\"') {
    while (1) {
      ptr1 = strchr(ptr + i + 1, '\"');
      if (!ptr1) {
        return i + (long)strlen(&ptr[i]); // Unterminated quote - goto EOF
      }
      if (ptr1[1] != '\"') return ptr1 - ptr + 1; // Double quote is literal
      i = ptr1 - ptr + 1;
    }
  }
  if (tmpchr == '\'') {
    while (1) {
      ptr1 = strchr(ptr + i + 1, '\'');
      if (!ptr1) {
        return i + (long)strlen(&ptr[i]); // Unterminated quote - goto EOF
      }
      if (ptr1[1] != '\'') return ptr1 - ptr + 1; // Double quote is literal
      i = ptr1 - ptr + 1;
    }
  }
  if (ispunct((unsigned char)tmpchr)) return 1; // Single-char token
  while (1) {
    tmpchr = ptr[i];
    if (!isalnum((unsigned char)tmpchr)) return i; // End of alphanumeric token
    i++;
  }
  bug(2308);
  return 0; // Dummy return - never executed
} // texDefTokenLen

// Token comparison for qsort
int texSortCmp(const void *key1, const void *key2)
{
  // Returns -1 if key1 < key2, 0 if equal, 1 if key1 > key2
  // Note:  ptr->fld == (*ptr).fld str.fld == (&str)->fld
  return strcmp(((struct texDef_struct *)key1)->tokenName,
      ((struct texDef_struct *)key2)->tokenName);
} // texSortCmp

// Token comparison for bsearch
int texSrchCmp(const void *key, const void *data)
{
  // Returns -1 if key < data, 0 if equal, 1 if key > data
  return strcmp(key,
      ((struct texDef_struct *)data)->tokenName);
} // texSrchCmp

// Convert ascii to a string of \tt tex; must not have control chars
// (The caller must surround it by {\tt })
// ***Note:  The caller must deallocate returned string
vstring asciiToTt(vstring s) {
  vstring_def(ttstr);
  vstring_def(tmp);
  long i, j, k;

  let(&ttstr, s); // In case the input s is temporarily allocated
  j = (long)strlen(ttstr);

  // Put special \tt font characters in a form that TeX can understand
  for (i = 0; i < j; i++) {
    k = 1;
    if (!g_htmlFlag) {
      switch (ttstr[i]) {
        // For all unspecified cases, TeX will accept the character 'as is'
        case ' ':
        case '$':
        case '%':
        case '#':
        case '{':
        case '}':
        case '&':
          let(&ttstr,cat(left(ttstr,i),"\\",right(ttstr,i+1),NULL));
          k = 2;
          break;
        case '^':
          let(&ttstr,cat(left(ttstr,i),"\\^{ }",right(ttstr,i+2),NULL));
          k = 5;
          break;
        case '\\':
        case '|':
        case '<':
        case '>':
        case '"':
        case '~':
        case '_':
          // Note:  this conversion will work for any character, but
          // results in the most TeX source code.
          let(&ttstr,cat(left(ttstr,i),"\\char`\\",right(ttstr,i+1),NULL));
          k = 8;
          break;
      } // End switch mtoken[i]
    } else {
      switch (ttstr[i]) {
        // For all unspecified cases, HTML will accept the character 'as is'.
        // Don't convert to &amp; but leave as is.  This
        // will allow the user to insert HTML entities for Unicode etc.
        // directly in the database source.
        // case '&': ...
        case '<':
          // Leave in HTML tags (case must match)
          if (!strcmp(mid(ttstr, i + 1, 6), "<HTML>")) {
            let(&ttstr, ttstr); // Purge stack to prevent overflow by 'mid'
            i = i + 6;
            break;
          }
          if (!strcmp(mid(ttstr, i + 1, 7), "</HTML>")) {
            let(&ttstr, ttstr); // Purge stack to prevent overflow by 'mid'
            i = i + 7;
            break;
          }
          let(&ttstr,cat(left(ttstr,i),"&lt;",right(ttstr,i+2),NULL));
          k = 4;
          break;
        case '>':
          let(&ttstr,cat(left(ttstr,i),"&gt;",right(ttstr,i+2),NULL));
          k = 4;
          break;
        case '"':
          let(&ttstr,cat(left(ttstr,i),"&quot;",right(ttstr,i+2),NULL));
          k = 6;
          break;
      } // End switch mtoken[i]
    }

    if (k > 1) { // Adjust iteration and length
      i = i + k - 1;
      j = j + k - 1;
    }
  } // Next i

  free_vstring(tmp); // Deallocate
  return ttstr;
} // asciiToTt

temp_vstring asciiToTt_temp(vstring s) {
  return makeTempAlloc(asciiToTt(s));
}

// Convert ascii token to TeX equivalent
// The "$" math delimiter is not placed around the returned arg. here
// *** Note: The caller must deallocate the returned string
vstring tokenToTex(vstring mtoken, long statemNum /* for error msgs */)
{
  vstring_def(tex);
  vstring tmpStr;
  long i, j, k;
  void *texDefsPtr; // For binary search
  flag saveOutputToString;

  if (!g_texDefsRead) {
    bug(2320); // This shouldn't be called if definitions weren't read
  }

  texDefsPtr = (void *)bsearch(mtoken, g_TexDefs, (size_t)numSymbs,
      sizeof(struct texDef_struct), texSrchCmp);
  if (texDefsPtr) { // Found it
    let(&tex, ((struct texDef_struct *)texDefsPtr)->texEquiv);
  } else {
    // If it wasn't found, give user a warning...
    saveOutputToString = g_outputToString;
    g_outputToString = 0;
    // It is possible for statemNum to be 0 when
    // tokenToTex() is called (via getTexLongMath()) from
    // printTexLongMath(), when its hypStmt argument is 0 (= not associated
    // with a statement).  (Reported by Wolf Lammen.)
    if (statemNum < 0 || statemNum > g_statements) bug(2331);
    if (statemNum > 0) { // Include statement label in error message
      printLongLine(cat("?Warning: In the comment for statement \"",
          g_Statement[statemNum].labelName,
          "\", math symbol token \"", mtoken,
          "\" does not have a LaTeX and/or an HTML definition.", NULL),
          "", " ");
    } else { // There is no statement associated with the error message
      printLongLine(cat("?Warning: Math symbol token \"", mtoken,
          "\" does not have a LaTeX and/or an HTML definition.", NULL),
          "", " ");
    }
    g_outputToString = saveOutputToString;
    // ... but we'll still leave in the old default conversion anyway:

    // If it wasn't found, use built-in conversion rules
    let(&tex, mtoken);

    // First, see if it's a tilde followed by a letter.
    // If so, remove the tilde.  (This is actually obsolete.)
    // (The tilde was an escape in the obsolete syntax.)
    if (tex[0] == '~') {
      if (isalpha((unsigned char)(tex[1]))) {
        let(&tex, right(tex, 2)); // Remove tilde
      }
    }

    // Next, convert punctuation characters to tt font
    j = (long)strlen(tex);
    for (i = 0; i < j; i++) {
      if (ispunct((unsigned char)(tex[i]))) {
        tmpStr = asciiToTt(chr(tex[i]));
        if (!g_htmlFlag)
          let(&tmpStr, cat("{\\tt ", tmpStr, "}", NULL));
        k = (long)strlen(tmpStr);
        let(&tex,
            cat(left(tex, i), tmpStr, right(tex, i + 2), NULL));
        i = i + k - 1; // Adjust iteration
        j = j + k - 1; // Adjust length
        free_vstring(tmpStr); // Deallocate
      }
    } // Next i

    // Make all letters Roman in math mode
    if (!g_htmlFlag)
      let(&tex, cat("\\mathrm{", tex, "}", NULL));
  } // End if

  return tex;
} // tokenToTex

// Converts a comment section in math mode to TeX.  Each math token
// MUST be separated by white space.   TeX "$" does not surround the output.
vstring asciiMathToTex(vstring mathComment, long statemNum)
{

  vstring tex;
  vstring_def(texLine);
  vstring_def(lastTex);
  vstring_def(token);
  flag alphnew, alphold, unknownnew, unknownold;
  long i;
  vstring srcptr;

  srcptr = mathComment;

  free_vstring(texLine);
  free_vstring(lastTex);
  while(1) {
    i = whiteSpaceLen(srcptr);
    srcptr = srcptr + i;
    i = tokenLen(srcptr);
    if (!i) break; // Done
    let(&token, space(i));
    memcpy(token, srcptr, (size_t)i);
    srcptr = srcptr + i;
    // Convert token to TeX.
    // tokenToTex allocates tex; we must deallocate it
    tex = tokenToTex(token, statemNum);

    if (!g_htmlFlag) {
      // If this token and previous token begin with letter, add a thin
      // space between them.
      // Also, anything not in table will have space added.
      // Use "!!" here and below because isalpha returns an integer, whose
      // unspecified non-zero value could be truncated to 0 when
      // converted to char.  Thanks to Wolf Lammen for pointing this out.
      alphnew = !!isalpha((unsigned char)(tex[0]));
      unknownnew = 0;
      if (!strcmp(left(tex, 8), "\\mathrm{")) { // token not in table
        unknownnew = 1;
      }
      alphold = !!isalpha((unsigned char)(lastTex[0]));
      unknownold = 0;
      if (!strcmp(left(tex, 8), "\\mathrm{")) { // token not in table
        unknownold = 1;
      }
      // Put thin space only between letters and/or unknowns
      if ((alphold || unknownold) && (alphnew || unknownnew)) {
        // Put additional thin space between two letters
        let(&texLine, cat(texLine, "\\,", tex, " ", NULL));
      } else {
        let(&texLine, cat(texLine, tex, " ", NULL));
      }
    } else {
      let(&texLine, cat(texLine, tex, NULL));
    }
    free_vstring(lastTex); // Deallocate
    lastTex = tex; // Pass deallocation responsibility for tex to lastTex
  } // End while (1)

  free_vstring(lastTex); // Deallocate
  free_vstring(token); // Deallocate

  return texLine;
} // asciiMathToTex

// Gets the next section of a comment that is in the current mode (text,
// label, or math).  If 1st char. is not "$" (DOLLAR_SUBST), text mode is
// assumed.  mode = 0 means end of comment reached.  srcptr is left at 1st
// char. of start of next comment section.
vstring getCommentModeSection(vstring *srcptr, char *mode)
{
  vstring_def(modeSection);
  vstring ptr; // Not allocated
  flag addMode = 0;
  if (!g_outputToString) bug(2319);

  if ((*srcptr)[0] != DOLLAR_SUBST /* '$' */) {
    if ((*srcptr)[0] == 0) { // End of string
      *mode = 0; // End of comment
      return "";
    } else {
      *mode = 'n'; // Normal text
      addMode = 1;
    }
  } else {
    switch ((*srcptr)[1]) {
      case 'l':
      case 'm':
      case 'n':
        *mode = (*srcptr)[1];
        break;
      case ')': // Obsolete
        bug(2317);
        // Leave old code in case user continues through the bug
        *mode = 0; // End of comment
        return "";
        break;
      default:
        *mode = 'n';
        break;
    }
  }

  ptr = (*srcptr) + 1;
  while (1) {
    if (ptr[0] == DOLLAR_SUBST /* '$' */) {
      switch (ptr[1]) {
        case 'l':
        case 'm':
        case 'n':
        case ')': // Obsolete (will never happen)
          if (ptr[1] == ')') bug(2318);
          let(&modeSection, space(ptr - (*srcptr)));
          memcpy(modeSection, *srcptr, (size_t)(ptr - (*srcptr)));
          if (addMode) {
            let(&modeSection, cat(chr(DOLLAR_SUBST), "n", /* "$n" */ modeSection,
                NULL));
          }
          *srcptr = ptr;
          return modeSection;
          break;
      }
    } else {
      if (ptr[0] == 0) {
          let(&modeSection, space(ptr - (*srcptr)));
          memcpy(modeSection, *srcptr, (size_t)(ptr - (*srcptr)));
          if (addMode) {
            let(&modeSection, cat(chr(DOLLAR_SUBST), "n", /* "$n" */ modeSection,
                NULL));
          }
          *srcptr = ptr;
          return modeSection;
      }
    }
    ptr++;
  } // End while
  return NULL; // Dummy return - never executes
} // getCommentModeSection

// The texHeaderFlag means this:
// If !g_htmlFlag (i.e. TeX mode), then 1 means print header
// If g_htmlFlag, then 1 means include "Previous Next" links on page,
// based on the global g_showStatement variable.
void printTexHeader(flag texHeaderFlag)
{

  long i, j, k;
  vstring_def(tmpStr);

  // "Mathbox for <username>" mod
  vstring_def(localSandboxTitle);
  vstring_def(hugeHdr);
  vstring_def(bigHdr);
  vstring_def(smallHdr);
  vstring_def(tinyHdr);
  vstring_def(hugeHdrComment);
  vstring_def(bigHdrComment);
  vstring_def(smallHdrComment);
  vstring_def(tinyHdrComment);

  if (2 /* error */ == readTexDefs(0 /* errorsOnly=0 */, 1 /* gifCheck=1 */)) {
    print2(
       "?There was an error in the $t comment's LaTeX/HTML definitions.\n");
    return;
  }
  // }

  g_outputToString = 1; // Redirect print2 and printLongLine to g_printString
  if (!g_htmlFlag) {
    print2("%s This LaTeX file was created by Metamath on %s %s.\n",
       "%", date(), time_());

    if (texHeaderFlag && !g_oldTexFlag) {
      // LaTeX 2e
      print2("\\documentclass{article}\n");
      print2("\\usepackage{amssymb} %% amssymb must be loaded before phonetic\n");
      print2("\\usepackage{phonetic} %% for \\riota\n");
      // see https://www.ctan.org/pkg/phonetic
      // see https://www.ctan.org/pkg/comprehensive "Reflecting and rotating existing symbols"
      print2("\\usepackage{mathrsfs} %% for \\mathscr\n");
      // see https://www.ctan.org/pkg/mathrsfs
      print2("\\usepackage{mathtools} %% loads package amsmath\n");
      // see https://www.ctan.org/pkg/mathtools
      // see https://www.ctan.org/pkg/amsmath
      print2("\\usepackage{amsthm} %% amsthm must be loaded after amsmath\n");
      // see https://www.ctan.org/pkg/amsthm
      print2("\\usepackage{accents} %% accents should be loaded after mathtools\n");
      // see https://www.ctan.org/pkg/accents
      print2("\\theoremstyle{plain}\n");
      print2("\\newtheorem{theorem}{Theorem}[section]\n");
      print2("\\newtheorem{definition}[theorem]{Definition}\n");
      print2("\\newtheorem{lemma}[theorem]{Lemma}\n");
      print2("\\newtheorem{axiom}{Axiom}\n");
      print2("\\allowdisplaybreaks[1] %% Allow page breaks in {align}\n");
      print2("\\usepackage[plainpages=false,pdfpagelabels]{hyperref}\n");
      // see https://www.ctan.org/pkg/hyperref
      print2("\\hypersetup{colorlinks} %% Get rid of boxes around links\n");
      print2("\\begin{document}\n");
      print2("\n");
    }

    if (texHeaderFlag && g_oldTexFlag) {
      // LaTeX 2e
      print2("\\documentclass[leqno]{article}\n");
      print2("\\usepackage{amssymb} %% amssymb must be loaded before phonetic\n");
      print2("\\usepackage{phonetic} %% for \\riota\n");
      // see https://www.ctan.org/pkg/phonetic
      // see https://www.ctan.org/pkg/comprehensive "Reflecting and rotating existing symbols"
      print2("\\usepackage{mathrsfs} %% for \\mathscr\n");
      // see https://www.ctan.org/pkg/mathrsfs
      print2("\\usepackage{mathtools} %% loads package amsmath\n");
      // see https://www.ctan.org/pkg/mathtools
      // see https://www.ctan.org/pkg/amsmath
      print2("\\usepackage{accents} %% accents should be loaded after mathtools\n");
      // see https://www.ctan.org/pkg/accents
      print2("\\raggedbottom\n");
      print2("\\raggedright\n");
      print2("%%\\title{Your title here}\n");
      print2("%%\\author{Your name here}\n");
      print2("\\begin{document}\n");
      print2("%%\\maketitle\n");
      print2("\\newbox\\mlinebox\n");
      print2("\\newbox\\mtrialbox\n");
      print2("\\newbox\\startprefix  %% Prefix for first line of a formula\n");
      print2("\\newbox\\contprefix  %% Prefix for continuation line of a formula\n");
      print2("\\def\\startm{  %% Initialize formula line\n");
      print2("  \\setbox\\mlinebox=\\hbox{\\unhcopy\\startprefix}\n");
      print2("}\n");
      print2("\\def\\m#1{  %% Add a symbol to the formula\n");
      print2("  \\setbox\\mtrialbox=\\hbox{\\unhcopy\\mlinebox $\\,#1$}\n");
      print2("  \\ifdim\\wd\\mtrialbox>\\hsize\n");
      print2("    \\box\\mlinebox\n");
      print2("    \\setbox\\mlinebox=\\hbox{\\unhcopy\\contprefix $\\,#1$}\n");
      print2("  \\else\n");
      print2("    \\setbox\\mlinebox=\\hbox{\\unhbox\\mtrialbox}\n");
      print2("  \\fi\n");
      print2("}\n");
      print2("\\def\\endm{  %% Output the last line of a formula\n");
      print2("  \\box\\mlinebox\n");
      print2("}\n");
    }
  } else { // g_htmlFlag

    print2("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n");
    print2(     "    \"http://www.w3.org/TR/html4/loose.dtd\">\n");
    print2("<HTML LANG=\"EN-US\">\n");
    print2("<HEAD>\n");
    print2("%s%s\n", "<META HTTP-EQUIV=\"Content-Type\" ",
        "CONTENT=\"text/html; charset=iso-8859-1\">");
    // Improve mobile device display per David A. Wheeler
    print2("<META NAME=\"viewport\" CONTENT=\"width=device-width, initial-scale=1.0\">\n");

    print2("<STYLE TYPE=\"text/css\">\n");
    print2("<!--\n");
    // Optional information but takes unnecessary file space
    // (change @ to * if uncommenting)
    // print2(
    //     "/@ Math symbol images will be shifted down 4 pixels to align with\n");
    // print2(
    //     "   normal text for compatibility with various browsers.  The old\n");
    // print2(
    //     "   ALIGN=TOP for math symbol images did not align in all browsers\n");
    // print2(
    //     "   and should be deleted.  All other images must override this\n");
    // print2(
    //     "   shift with STYLE=\"margin-bottom:0px\". @/\n");
    print2("img { margin-bottom: -4px }\n");
    // Print style sheet for rainbow-colored number that goes after statement label.
    print2(".r { font-family: \"Arial Narrow\";\n");
    print2("     font-size: x-small;\n");
    // There is no color
    print2("   }\n");

    // Indent web proof displays
    // Print style sheet for HTML proof indentation number
    // ??? Future - combine with above style sheet
    print2(".i { font-family: \"Arial Narrow\";\n");
    print2("     font-size: x-small;\n");
    print2("     color: gray;\n");
    print2("   }\n");
    print2("-->\n");
    print2("</STYLE>\n");
    printLongLine(g_htmlCSS, "", " ");

    // Put theorem name before "Metamath Proof Explorer" etc.
    if (g_showStatement < g_extHtmlStmt) {
      print2("%s\n", cat("<TITLE>",
          // Strip off ".html"
          left(g_texFileName, (long)strlen(g_texFileName) - 5),
          " - ", htmlTitle,
          "</TITLE>", NULL));
    } else if (g_showStatement < g_mathboxStmt) { // Sandbox stuff
      print2("%s\n", cat("<TITLE>",
          // Strip off ".html"
          left(g_texFileName, (long)strlen(g_texFileName) - 5),
          " - ", g_extHtmlTitle,
          "</TITLE>", NULL));
    } else {
      // "Mathbox for <username>"
      // Scan from this statement backwards until a big header is found
      for (i = g_showStatement; i > g_mathboxStmt; i--) {
        if (g_Statement[i].type == a_ || g_Statement[i].type == p_) {
          // Note: only bigHdr is used; the other 5 returned strings are
          // ignored.
          getSectionHeadings(i, &hugeHdr, &bigHdr, &smallHdr,
              &tinyHdr,
              &hugeHdrComment, &bigHdrComment, &smallHdrComment,
              &tinyHdrComment,
              0, // fineResolution
              0); // fullComment
          if (bigHdr[0] != 0) break;
        }
      } // next i
      if (bigHdr[0]) {
        // A big header was found; use it for the page title
        let(&localSandboxTitle, bigHdr);
      } else {
        // A big header was not found (should not happen if set.mm is
        // formatted right, but use default just in case).
        let(&localSandboxTitle, sandboxTitle);
      }
      free_vstring(hugeHdr); // Deallocate memory
      free_vstring(bigHdr); // Deallocate memory
      free_vstring(smallHdr); // Deallocate memory
      free_vstring(tinyHdr); // Deallocate memory
      free_vstring(hugeHdrComment); // Deallocate memory
      free_vstring(bigHdrComment); // Deallocate memory
      free_vstring(smallHdrComment); // Deallocate memory
      free_vstring(tinyHdrComment); // Deallocate memory

      printLongLine(cat("<TITLE>",
          // Strip off ".html"
          left(g_texFileName, (long)strlen(g_texFileName) - 5),
          " - ", localSandboxTitle,
          "</TITLE>", NULL), "", "\"");
    }
    // Icon for bookmark
    print2("%s%s\n", "<LINK REL=\"shortcut icon\" HREF=\"favicon.ico\" ",
        "TYPE=\"image/x-icon\">");

    print2("</HEAD>\n");
    print2("<BODY BGCOLOR=\"#FFFFFF\">\n");

    print2("<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 WIDTH=\"100%s\">\n",
         "%");
    print2("  <TR>\n");
    print2("    <TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%s\"><A HREF=\n", "%");
    print2("    \"%s\"><IMG SRC=\"%s\"\n",
        (g_showStatement < g_extHtmlStmt ? g_htmlHomeHREF :
             (g_showStatement < g_mathboxStmt ? extHtmlHomeHREF :
             sandboxHomeHREF)),
        // Note that we assume that the upper-left image is 32x32
        (g_showStatement < g_extHtmlStmt ? g_htmlHomeIMG :
             (g_showStatement < g_mathboxStmt ? extHtmlHomeIMG :
             sandboxHomeIMG)));
    print2("      BORDER=0\n");
    print2("      ALT=\"%s\"\n",
        (g_showStatement < g_extHtmlStmt ? htmlTitleAbbr :
             (g_showStatement < g_mathboxStmt ? g_extHtmlTitleAbbr :
             sandboxTitleAbbr)));
    print2("      TITLE=\"%s\"\n",
        (g_showStatement < g_extHtmlStmt ? htmlTitleAbbr :
             (g_showStatement < g_mathboxStmt ? g_extHtmlTitleAbbr :
             sandboxTitleAbbr)));
    print2(
      "      HEIGHT=32 WIDTH=32 ALIGN=TOP STYLE=\"margin-bottom:0px\"></A>\n");
    print2("    </TD>\n");
    print2(
"    <TD ALIGN=CENTER COLSPAN=2 VALIGN=TOP><FONT SIZE=\"+3\" COLOR=%s><B>\n",
      GREEN_TITLE_COLOR);
    // Allow plenty of room for long titles (although over 79 chars. will
    // trigger bug 1505).
    print2("%s\n",
        (g_showStatement < g_extHtmlStmt ? htmlTitle :
             (g_showStatement < g_mathboxStmt ? g_extHtmlTitle :
             localSandboxTitle)));
    print2("      </B></FONT></TD>\n");

    if (texHeaderFlag) { // For HTML, 1 means to put prev/next links
      // Put Previous/Next links into web page
      print2("    <TD ALIGN=RIGHT VALIGN=TOP WIDTH=\"25%s\">\n", "%");
      print2("      <FONT SIZE=-1 FACE=sans-serif>\n");
      // Find the previous statement with a web page
      j = 0;
      k = 0;
      for (i = g_showStatement - 1; i >= 1; i--) {
        if (g_Statement[i].type == (char)p_ ||
            g_Statement[i].type == (char)a_) {
          j = i;
          break;
        }
      }
      if (j == 0) {
        k = 1; // First statement flag
        // For the first statement, wrap to last one
        for (i = g_statements; i >= 1; i--) {
          if (g_Statement[i].type == (char)p_ ||
              g_Statement[i].type == (char)a_ ) {
            j = i;
            break;
          }
        }
      }
      if (j == 0) bug(2314);
      print2("      <A HREF=\"%s.html\">\n",
          g_Statement[j].labelName);
      if (!k) {
        print2("      &lt; Previous</A>&nbsp;&nbsp;\n");
      } else {
        print2("      &lt; Wrap</A>&nbsp;&nbsp;\n");
      }
      // Find the next statement with a web page
      j = 0;
      k = 0;
      for (i = g_showStatement + 1; i <= g_statements; i++) {
        if (g_Statement[i].type == (char)p_ ||
            g_Statement[i].type == (char)a_) {
          j = i;
          break;
        }
      }
      if (j == 0) {
        k = 1; // Last statement flag
        // For the last statement, wrap to first one
        for (i = 1; i <= g_statements; i++) {
          if (g_Statement[i].type == (char)p_ ||
              g_Statement[i].type == (char)a_) {
            j = i;
            break;
          }
        }
      }
      if (j == 0) bug(2315);
      if (!k) {
        print2("      <A HREF=\"%s.html\">Next &gt;</A>\n",
            g_Statement[j].labelName);
      } else {
        print2("      <A HREF=\"%s.html\">Wrap &gt;</A>\n",
            g_Statement[j].labelName);
      }

      print2("      </FONT><FONT FACE=sans-serif SIZE=-2>\n");

      // 8-Sep-03 nm - ??? Is the closing </FONT> printed if there is no
      // altHtml?  This should be tested.

      // Compute the theorem list page number.  ??? Temporarily
      // we assume it to be 100 (hardcoded).  Todo: This should be fixed to use
      // the same as the THEOREMS_PER_PAGE in WRITE THEOREMS (have a SET
      // global variable in place of THEOREMS_PER_PAGE?).
      i = ((g_Statement[g_showStatement].pinkNumber - 1) / 100) + 1; // Page #
      // All thm pages now have page num after mmtheorems
      // since mmtheorems.html is now just the table of contents.
      let(&tmpStr, cat("mmtheorems", str((double)i), ".html#",
          g_Statement[g_showStatement].labelName, NULL)); // Link to page/stmt
      // Break up lines w/ long labels to prevent bug 1505
      printLongLine(cat("      <BR><A HREF=\"", tmpStr,
            "\">Nearby theorems</A>", NULL), " ", " ");

      print2("      </FONT>\n");
      print2("    </TD>\n");
      print2("  </TR>\n");
      print2("  <TR>\n");
      print2("    <TD COLSPAN=2 ALIGN=LEFT VALIGN=TOP><FONT SIZE=-2\n");
      print2("      FACE=sans-serif>\n");
      print2("      <A HREF=\"../mm.html\">Mirrors</A>&nbsp; &gt;\n");
      print2("      &nbsp;<A HREF=\"../index.html\">Home</A>&nbsp; &gt;\n");
      print2("      &nbsp;<A HREF=\"%s\">%s</A>&nbsp; &gt;\n",
          (g_showStatement < g_extHtmlStmt ? g_htmlHomeHREF :
               (g_showStatement < g_mathboxStmt ? extHtmlHomeHREF :
               g_htmlHomeHREF)),
          (g_showStatement < g_extHtmlStmt ? htmlTitleAbbr :
               (g_showStatement < g_mathboxStmt ? g_extHtmlTitleAbbr :
               htmlTitleAbbr)));
      print2("      &nbsp;<A HREF=\"mmtheorems.html\">Th. List</A>&nbsp; &gt;\n");
      if (g_showStatement >= g_mathboxStmt) {
        print2("      &nbsp;<A HREF=\"mmtheorems.html#sandbox:bighdr\">\n");
        print2("      Mathboxes</A>&nbsp; &gt;\n");
      }
      print2("      &nbsp;%s\n",
          // Strip off ".html"
          left(g_texFileName, (long)strlen(g_texFileName) - 5));
      print2("      </FONT>\n");
      print2("    </TD>\n");
      print2("    <TD COLSPAN=2 ALIGN=RIGHT VALIGN=TOP>\n");
      print2("      <FONT SIZE=-2 FACE=sans-serif>\n");

      // Add link(s) specified by htmlexturl in $t statement
      // The position of the theorem name is indicated with "*" in the
      // htmlexturl $t variable.  If a literal "*" is part of the URL,
      // use the alternate URL encoding "%2A".
      // Example: (take out space in "/ *" below that was put there to prevent
      // compiler warnings).
      //  htmlexturl '<A HREF="http://metamath.tirix.org/ *.html">' +
      //      'Structured version</A>&nbsp;&nbsp;' +
      //      '<A HREF="https://expln.github.io/metamath/asrt/ *.html">' +
      //      'ASCII version</A>&nbsp;&nbsp;';
      let(&tmpStr, htmlExtUrl);
      i = 1;
      while (1) {
        i = instr(i, tmpStr, "*");
        if (i == 0) break;
        let(&tmpStr, cat(left(tmpStr, i - 1),
            g_Statement[g_showStatement].labelName,
            right(tmpStr, i + 1), NULL));
      }
      printLongLine(tmpStr, "", " ");

      // Print the GIF/Unicode Font choice, if directories are specified
      if (htmlDir[0]) {
        if (g_altHtmlFlag) {
          print2("      <A HREF=\"%s%s\">GIF version</A>\n",
                htmlDir, g_texFileName);
        } else {
          print2("      <A HREF=\"%s%s\">Unicode version</A>\n",
                altHtmlDir, g_texFileName);
        }
      }
    } else { // texHeaderFlag=0 for HTML means not to put prev/next links
      // there is no table open (mmascii, mmdefinitions), so don't
      // add </TD> which caused HTML validation failure.
      print2("      <TD ALIGN=RIGHT VALIGN=TOP\n");
      print2("       ><FONT FACE=sans-serif SIZE=-2>\n", "%");

      // Print the GIF/Unicode Font choice, if directories are specified
      if (htmlDir[0]) {
        print2("\n");
        if (g_altHtmlFlag) {
          print2("This is the Unicode version.<BR>\n");
          print2("<A HREF=\"%s%s\">Change to GIF version</A>\n",
              htmlDir, g_texFileName);
        } else {
          print2("This is the GIF version.<BR>\n");
          print2("<A HREF=\"%s%s\">Change to Unicode version</A>\n",
              altHtmlDir, g_texFileName);
        }
      }
      else {
        print2("&nbsp;\n");
      }
    }

    print2("      </FONT>\n");
    print2("    </TD>\n");
    print2("  </TR>\n");
    print2("</TABLE>\n");

    print2("<HR NOSHADE SIZE=1>\n");
  } // g_htmlFlag
  fprintf(g_texFilePtr, "%s", g_printString);
  g_outputToString = 0;
  free_vstring(g_printString);

  // Deallocate strings
  free_vstring(tmpStr);
} // printTexHeader

// Prints an embedded comment in TeX or HTML.  The commentPtr must point to the first
// character after the "$(" in the comment.  The printout ends when the first
// "$)" or null character is encountered.   commentPtr must not be a temporary
// allocation.   htmlCenterFlag, if 1, means to center the HTML and add a
// "Description:" prefix.
// The output is printed to the global g_texFilePtr.
// Note: the global long "g_showStatement" is referenced to determine whether
// to read bibliography from mmset.html or mmhilbert.html (or other
// g_htmlBibliography or extHtmlBibliography file pair).
// Returns 1 if an error or warning message was printed
flag printTexComment(vstring commentPtr, flag htmlCenterFlag,
    long actionBits, // see below
        // Indicators for actionBits:
        //   #define ERRORS_ONLY 1 - just report errors, don't print output
        //   #define PROCESS_SYMBOLS 2
        //   #define PROCESS_LABELS 4
        //   #define ADD_COLORED_LABEL_NUMBER 8
        //   #define PROCESS_BIBREFS 16
        //   #define PROCESS_UNDERSCORES 32
        //   #define CONVERT_TO_HTML 64 - convert '<' to '&gt;' unless
        //           <HTML>, </HTML> present
        //   #define METAMATH_COMMENT 128 - $) terminates string
        /*   #define PROCESS_EVERYTHING PROCESS_SYMBOLS + PROCESS_LABELS \
               + ADD_COLORED_LABEL_NUMBER + PROCESS_BIBREFS \
               + PROCESS_UNDERSCORES + CONVERT_HTML + METAMATH_COMMENT */

        // 10-Dec-2018 nm - expanded meaning of errorsOnly for MARKUP command:
        //   2 = process as if in <HTML>...</HTML> preformatted mode but
        //       don't strip <HTML>...</HTML> tags
        //   3 = same as 2, but convert ONLY math symbols
        // (These new values were added instead of adding a new argument,
        // so as not to have to modify ~60 other calls to this function).

    flag fileCheck) // 1 = check external files (gifs and bib)
{
  vstring cmtptr; // Not allocated
  vstring srcptr; // Not allocated
  vstring lineStart; // Not allocated
  vstring_def(tmpStr);
  vstring modeSection; // Not allocated
  vstring_def(sourceLine);
  vstring_def(outputLine);
  vstring_def(tmp);
  flag textMode, mode, lastLineFlag, displayMode;
  vstring_def(tmpComment);
  flag preformattedMode = 0; // HTML <HTML> preformatted mode

  // For bibliography hyperlinks
  vstring_def(bibTag);
  vstring_def(bibFileName);
  vstring_def(bibFileContents);
  vstring_def(bibFileContentsUpper); // Uppercase version
  vstring_def(bibTags);
  long pos1, pos2, htmlpos1, htmlpos2, saveScreenWidth;
  flag tmpMathMode;

  // Variables for converting ` ` and ~ to old $m,$n and $l,$n formats in
  // order to re-use the old code.
  // Note that DOLLAR_SUBST will replace the old $.
  vstring_def(cmt);
  vstring_def(cmtMasked); // cmt with math syms blanked // also mask ~ label
  vstring_def(tmpMasked); // tmp with math syms blanked
  vstring_def(tmpStrMasked); // tmpStr w/ math syms blanked
  long i, clen;
  flag returnVal = 0; // 1 means error/warning

  // Internal flags derived from actionBits argument, for MARKUP command use
  flag errorsOnly;
  flag processSymbols;
  flag processLabels;
  flag addColoredLabelNumber;
  flag processBibrefs;
  flag processUnderscores;
  flag convertToHtml;
  flag metamathComment;

  // Assign local Booleans for actionBits mask
  errorsOnly = (actionBits & ERRORS_ONLY ) != 0;
  processSymbols = (actionBits & PROCESS_SYMBOLS ) != 0;
  processLabels = (actionBits & PROCESS_LABELS ) != 0;
  addColoredLabelNumber = (actionBits & ADD_COLORED_LABEL_NUMBER ) != 0;
  processBibrefs = (actionBits & PROCESS_BIBREFS ) != 0;
  processUnderscores = (actionBits & PROCESS_UNDERSCORES ) != 0;
  convertToHtml = (actionBits & CONVERT_TO_HTML ) != 0;
  metamathComment = (actionBits & METAMATH_COMMENT ) != 0;

  // We must let this procedure handle switching output to string mode
  if (g_outputToString) bug(2309);
  // The LaTeX (or HTML) file must be open
  if (errorsOnly == 0) {
    if (!g_texFilePtr) bug(2321);
  }

  cmtptr = commentPtr;

  if (!g_texDefsRead) {
    // TeX defs were not read (error was detected and flagged to the user elsewhere)
    return returnVal;
  }

  // Convert line to the old $m..$n and $l..$n formats (using DOLLAR_SUBST
  // instead of "$") - the old syntax is obsolete but we do this conversion
  // to re-use some old code.
  if (metamathComment != 0) {
    i = instr(1, cmtptr, "$)"); // If it points to source buffer
    if (!i) i = (long)strlen(cmtptr) + 1; // If it's a stand-alone string
  } else {
    i = (long)strlen(cmtptr) + 1;
  }
  let(&cmt, left(cmtptr, i - 1));

  // All actions on cmt should be mirrored on cmdMasked, except that
  // math symbols are replaced with blanks in cmdMasked.
  let(&cmtMasked, cmt);

  // This section is independent and can be removed without side effects
  if (g_htmlFlag) {
    // Convert special characters <, &, etc. to HTML entities.
    // But skip converting math symbols inside ` `.
    // Detect preformatted HTML (this is crude, since it
    // will apply to whole comment - perhaps fine-tune this later).
    if (convertToHtml != 0) {
      if (instr(1, cmt, "<HTML>") != 0) preformattedMode = 1;
    } else {
      preformattedMode = 1; // For MARKUP command - don't convert HTML
    }
    mode = 1; // 1 normal, -1 math token
    let(&tmp, "");
    let(&tmpMasked, "");
    while (1) {
      pos1 = 0;
      while (1) {
        pos1 = instr(pos1 + 1, cmt, "`");
        if (!pos1) break;
        if (cmt[pos1] == '`') {
          pos1++; // Skip `` escape
          continue;
        }
        break;
      }
      if (!pos1) pos1 = (long)strlen(cmt) + 1;
      if (mode == 1 && preformattedMode == 0) {
        free_vstring(tmpStr);
        // asciiToTt() is where "<" is converted to "&lt;" etc.
        tmpStr = asciiToTt(left(cmt, pos1));
        let(&tmpStrMasked, tmpStr);
      } else {
        let(&tmpStr, left(cmt, pos1));
        if (mode == -1) { // Math mode
          // Replace math symbols with spaces to prevent confusing them
          // with markup in sections below.
          let(&tmpStrMasked, cat(space(pos1 - 1),
              mid(cmtMasked, pos1, 1), NULL));
        } else { // Preformatted mode but not math mode
          let(&tmpStrMasked, left(cmtMasked, pos1));
        }
      }
      let(&tmp, cat(tmp, tmpStr, NULL));
      let(&tmpMasked, cat(tmpMasked, tmpStrMasked, NULL));
      let(&cmt, right(cmt, pos1 + 1));
      let(&cmtMasked, right(cmtMasked, pos1 + 1));
      if (!cmt[0]) break;
      mode = (char)(-mode);
    }
    let(&cmt, tmp);
    let(&cmtMasked, tmpMasked);
    free_vstring(tmpStr); // Deallocate
    free_vstring(tmpStrMasked);
  }

  // Add leading and trailing HTML markup to comment here
  // (instead of in caller).  Also convert special characters.
  if (g_htmlFlag) {
    // This used to be done in mmcmds.c
    if (htmlCenterFlag) { // Note:  this should be 0 in MARKUP command
      let(&cmt, cat("<CENTER><TABLE><TR><TD ALIGN=LEFT><B>Description: </B>",
          cmt, "</TD></TR></TABLE></CENTER>", NULL));
      let(&cmtMasked,
          cat("<CENTER><TABLE><TR><TD ALIGN=LEFT><B>Description: </B>",
          cmtMasked, "</TD></TR></TABLE></CENTER>", NULL));
    }
  }

  // Mask out _ (underscore) in labels so they won't become subscripts
  // (reported by Benoit Jubin).
  // This section is independent and can be removed without side effects.
  if (g_htmlFlag != 0) {
    pos1 = 0;
    while (1) { // Look for label start
      pos1 = instr(pos1 + 1, cmtMasked, "~");
      if (!pos1) break;
      if (cmtMasked[pos1] == '~') {
        pos1++; // Skip ~~ escape
        continue;
      }
      // Skip whitespace after ~
      while (1) {
        if (cmtMasked[pos1] == 0) break; // End of line
        if (isspace((unsigned char)(cmtMasked[pos1]))) {
          pos1++;
          continue;
        } else { // Found start of label
          break;
        }
      }
      // Skip non-whitespace after ~ find end of label
      while (1) {
        if (cmtMasked[pos1] == 0) break; // End of line
        if (!(isspace((unsigned char)(cmtMasked[pos1])))) {
          if (cmtMasked[pos1] == '_') {
            // Put an "?" in place of label character in mask
            cmtMasked[pos1] = '?';
          }
          pos1++;
          continue;
        } else { // Found end of label
          break;
        }
      } // while (1)
    } // while (1)
  } // if g_htmlFlag

  // Handle dollar signs in comments converted to LaTeX.
  // This section is independent and can be removed without side effects.
  // This must be done before the underscores below so subscript $'s.
  // won't be converted to \$'s.
  if (!g_htmlFlag) { // LaTeX
    pos1 = 0;
    while (1) {
      pos1 = instr(pos1 + 1, cmt, "$");
      if (!pos1) break;
      // Don't modify anything inside of <HTML>...</HTML> tags
      if (pos1 > instr(1, cmt, "<HTML>") && pos1 < instr(1, cmt, "</HTML>"))
        continue;
      let(&cmt, cat(left(cmt, pos1 - 1), "\\$",
          right(cmt, pos1 + 1), NULL));
      let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "\\$",
          right(cmtMasked, pos1 + 1), NULL));
      pos1 = pos1 + 1; // Adjust for 2-1 extra chars in "let" above
    } // while (1)
  }

  // This section comes BEFORE the underscore handling
  // below, so that "{\em...}" won't be converted to "\}\em...\}".
  // Convert any remaining special characters for LaTeX.
  // This section is independent and can be removed without side effects.
  if (!g_htmlFlag) { // i.e. LaTeX mode.
    // At this point, the comment begins e.g "\begin{lemma}\label{lem:abc}".
    pos1 = instr(1, cmt, "} ");
    if (pos1) {
      pos1++; // Start after the "}"
    } else {
      pos1 = 1; // If not found, start from beginning of line
    }
    pos2 = (long)strlen(cmt);
    tmpMathMode = 0;
    for (pos1 = pos1 + 0; pos1 <= pos2; pos1++) {
      // Don't modify anything inside of math symbol strings
      // (imperfect - only works if `...` is not split across lines?).
      if (cmt[pos1 - 1] == '`') tmpMathMode = (flag)(1 - tmpMathMode);
      if (tmpMathMode) continue;
      if (pos1 > 1) {
        if (cmt[pos1 - 1] == '_' && cmt[pos1 - 2] == '$') {
          // The _ is part of "$_{...}$" earlier conversion
          continue;
        }
      }
      // $%#{}&^\\|<>"~_ are converted by asciiToTt().
      // Omit \ and $ since they be part of an earlier conversion.
      // Omit ~ since it is part of label ref.
      // Omit " since it legal.
      // Because converting to \char` causes later math mode problems due to `,
      // we change |><_ to /)(- (an ugly workaround).
      switch(cmt[pos1 - 1]) {
        case '|': cmt[pos1 - 1] = '/'; break;
        case '<': cmt[pos1 - 1] = '{'; break;
        case '>': cmt[pos1 - 1] = '}'; break;
        case '_': cmt[pos1 - 1] = '-'; break;
      }
      if (strchr("%#{}&^|<>_", cmt[pos1 - 1]) != NULL) {
        free_vstring(tmpStr);
        tmpStr = asciiToTt(chr(cmt[pos1 - 1]));
        let(&cmt, cat(left(cmt, pos1 - 1), tmpStr,
            right(cmt, pos1 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), tmpStr,
            right(cmtMasked, pos1 + 1), NULL));
        pos1 += (long)strlen(tmpStr) - 1;
        pos2 += (long)strlen(tmpStr) - 1;
      }
    } // Next pos1
  } // if (!g_htmlFlag)

  // Handle underscores in comments converted to HTML:  Convert _abc_
  // to <I>abc</I> for book titles, etc.; convert a_n to a<SUB>n</SUB> for
  // subscripts.
  // This section is independent and can be removed without side effects
  if (g_htmlFlag != 0 && processUnderscores != 0) {
    pos1 = 0;
    while (1) {
      // Only look at non-math part of comment
      pos1 = instr(pos1 + 1, cmtMasked, "_");
      if (!pos1) break;
      // Don't modify anything inside of <HTML>...</HTML> tags
      if (pos1 > instr(1, cmt, "<HTML>") && pos1 < instr(1, cmt, "</HTML>"))
        continue;

      // Don't modify external hyperlinks containing "_"
      pos2 = pos1 - 1;
      while (1) { // Get to previous whitespace
        if (pos2 == 0 || isspace((unsigned char)(cmt[pos2]))) break;
        pos2--;
      }
      if (!strcmp(mid(cmt, pos2 + 2, 7), "http://")) {
        continue;
      }
      if (!strcmp(mid(cmt, pos2 + 2, 8), "https://")) {
        continue;
      }
      if (!strcmp(mid(cmt, pos2 + 2, 2), "mm")) {
        continue;
      }
      // Double-underscore handling: double-underscores are "escaped
      // underscores", so replace a double-underscore with a single
      // underscore and do not modify italic or subscript.
      if (cmt[pos1] == '_') {
            if (g_htmlFlag) {  // HTML
              let(&cmt, cat(left(cmt, pos1), // Skip (delete) "_"
                  right(cmt, pos1 + 2), NULL));
              let(&cmtMasked, cat(left(cmtMasked, pos1), // Skip (delete) "_"
                  right(cmtMasked, pos1 + 2), NULL));
              pos1 ++; // Adjust for 1 extra char '_'
            } else {  // LaTeX
              let(&cmt, cat(left(cmt, pos1 - 1),  // Skip (delete) "_"
                  "\\texttt{\\_}",
                  right(cmt, pos1 + 2), NULL));
              let(&cmtMasked, cat(left(cmtMasked, pos1 - 1),  // Skip (delete) "_"
                  "\\texttt{\\_}",
                  right(cmtMasked, pos1 + 2), NULL));
              pos1 = pos1 + 11; // Adjust for 11 extra chars "\texttt{\_}"
            }
        continue;
      } // End of double-underscore handling

      // Opening "_" must be <whitespace>_<alphanum> for <I> tag
      if (pos1 > 1) {
        // Check for not whitespace and not opening punctuation
        if (!isspace((unsigned char)(cmt[pos1 - 2]))
            && strchr(OPENING_PUNCTUATION, cmt[pos1 - 2]) == NULL) {
          // Check for not whitespace and not closing punctuation
          if (!isspace((unsigned char)(cmt[pos1]))
            && strchr(CLOSING_PUNCTUATION, cmt[pos1]) == NULL) {

            // Found <nonwhitespace>_<nonwhitespace> - assume subscript.
            // Locate the whitespace (or end of string) that closes subscript.
            // Note:  This algorithm is not perfect in that the subscript
            // is assumed to end at closing punctuation, which theoretically
            // could be part of the subscript itself, such as a subscript
            // with a comma in it.
            pos2 = pos1 + 1;
            while (1) {
              if (!cmt[pos2]) break; // End of string
              // Look for whitespace or closing punctuation
              if (isspace((unsigned char)(cmt[pos2]))
                  || strchr(OPENING_PUNCTUATION, cmt[pos2]) != NULL
                  || strchr(CLOSING_PUNCTUATION, cmt[pos2]) != NULL) break;
              pos2++; // Move forward through subscript
            }
            pos2++; // Adjust for left, seg, etc. that start at 1 not 0
            if (g_htmlFlag) { // HTML
              // Put <SUB>...</SUB> around subscript
              let(&cmt, cat(left(cmt, pos1 - 1),
                  "<SUB><FONT SIZE=\"-1\">",
                  seg(cmt, pos1 + 1, pos2 - 1), // Skip (delete) "_"
                  "</FONT></SUB>", right(cmt, pos2), NULL));
              let(&cmtMasked, cat(left(cmtMasked, pos1 - 1),
                  "<SUB><FONT SIZE=\"-1\">",
                  seg(cmtMasked, pos1 + 1, pos2 - 1), // Skip (delete) "_"
                  "</FONT></SUB>", right(cmtMasked, pos2), NULL));
              pos1 = pos2 + 33; // Adjust for 34-1 extra chars in "let" above
            } else { // LaTeX
              // Put _{...} around subscript
              let(&cmt, cat(left(cmt, pos1 - 1), "$_{",
                  seg(cmt, pos1 + 1, pos2 - 1), // Skip (delete) "_"
                  "}$", right(cmt, pos2), NULL));
              let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "$_{",
                  seg(cmtMasked, pos1 + 1, pos2 - 1), // Skip (delete) "_"
                  "}$", right(cmtMasked, pos2), NULL));
              pos1 = pos2 + 4; // Adjust for 5-1 extra chars in "let" above
            }
            continue;
          } else {
            // Found <nonwhitespace>_<whitespace> - not an opening "_"
            // Do nothing in this case
            continue;
          }
        }
      }
      if (!isalnum((unsigned char)(cmt[pos1]))) continue;
      // Only look at non-math part of comment
      pos2 = instr(pos1 + 1, cmtMasked, "_");
      if (!pos2) break;
      // Closing "_" must be <alphanum>_<nonalphanum>
      if (!isalnum((unsigned char)(cmt[pos2 - 2]))) continue;
      if (isalnum((unsigned char)(cmt[pos2]))) continue;
      if (g_htmlFlag) { // HTML
        let(&cmt, cat(left(cmt, pos1 - 1), "<I>",
            seg(cmt, pos1 + 1, pos2 - 1),
            "</I>", right(cmt, pos2 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "<I>",
            seg(cmtMasked, pos1 + 1, pos2 - 1),
            "</I>", right(cmtMasked, pos2 + 1), NULL));
        pos1 = pos2 + 5; // Adjust for 7-2 extra chars in "let" above
      } else { // LaTeX
        let(&cmt, cat(left(cmt, pos1 - 1), "{\\em ",
            seg(cmt, pos1 + 1, pos2 - 1),
            "}", right(cmt, pos2 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "{\\em ",
            seg(cmtMasked, pos1 + 1, pos2 - 1),
            "}", right(cmtMasked, pos2 + 1), NULL));
        pos1 = pos2 + 4; // Adjust for 6-2 extra chars in "let" above
      }
    }
  }

  // Convert opening double quote to `` for LaTeX.
  // This section is independent and can be removed without side effects
  if (!g_htmlFlag) { // If LaTeX mode
    i = 1; // Even/odd counter: 1 = left quote, 0 = right quote
    pos1 = 0;
    while (1) {
      // cmtMasked has math symbols blanked
      pos1 = instr(pos1 + 1, cmtMasked, "\"");
      if (pos1 == 0) break;
      if (i == 1) {
        // Warning:  "`" needs to be escaped (i.e. repeated) to prevent it
        // from being treated as a math symbol delimiter below.  So "````"
        // will become "``" in the LaTeX output.
        let(&cmt, cat(left(cmt, pos1 - 1), "````",
            right(cmt, pos1 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), "````",
            right(cmtMasked, pos1 + 1), NULL));
      }
      i = 1 - i; // Count to next even or odd
    }
  }

  // Put bibliography hyperlinks in comments converted to HTML:
  // [Monk2] becomes <A HREF="mmset.html#monk2>[Monk2]</A> etc.
  // This section is independent and can be removed without side effects
  if (g_htmlFlag && processBibrefs != 0) {
    // Assign local tag list and local HTML file name
    if (g_showStatement < g_extHtmlStmt) {
      let(&bibTags, g_htmlBibliographyTags);
      let(&bibFileName, g_htmlBibliography);
    } else if (g_showStatement < g_mathboxStmt) { // Sandbox stuff
      let(&bibTags, extHtmlBibliographyTags);
      let(&bibFileName, extHtmlBibliography);
    } else {
      // Sandbox stuff
      let(&bibTags, g_htmlBibliographyTags); // Go back to Mm Prf Explorer
      let(&bibFileName, g_htmlBibliography);
    }

    pos1 = 0;
    while (1) {
      // Look for any bibliography tags to convert to hyperlinks.
      // The biblio tag should be in brackets e.g. "[Monk2]".
      // Only look at non-math part of comment
      pos1 = instr(pos1 + 1, cmtMasked, "[");
      if (!pos1) break;

      // Escape a double [[
      if (cmtMasked[pos1] == '[') { // This is the char after "[" above
        // Remove the first "["
        let(&cmt, cat(left(cmt, pos1 - 1),
            right(cmt, pos1 + 1), NULL));
        let(&cmtMasked, cat(left(cmtMasked, pos1 - 1),
            right(cmtMasked, pos1 + 1), NULL));
        // The pos1-th position (starting at 1) is now the "[" that remains
        continue;
      }

      // Only look at non-math part of comment
      pos2 = instr(pos1 + 1, cmtMasked, "]");
      if (!pos2) break;

      // Get bibTag from cmtMasked as extra precaution
      let(&bibTag, seg(cmtMasked, pos1, pos2));
      // There should be no white space in the tag
      if ((signed)(strcspn(bibTag, " \n\r\t\f")) < pos2 - pos1 + 1) continue;
      // OK, we have a good tag.  If the file with bibliography has not been
      // read in yet, let's do so here for error-checking.

      // Start of error-checking
      if (fileCheck) {
        if (!bibTags[0]) {
          // The bibliography file has not been read in yet.
          free_vstring(bibFileContents);
          if (bibFileName[0]) {
            // The user specified a bibliography file in the xxx.mm $t comment
            // (otherwise we don't do anything).
            if (errorsOnly == 0) {
              print2("Reading HTML bibliographic tags from file \"%s\"...\n",
                  bibFileName);
            }
            bibFileContents = readFileToString(bibFileName, 0,
                &i /* charCount; not used here */);
          }
          if (!bibFileContents) {
            if (bibFileName[0]) {
              // The file was not found or had some problem (use verbose mode = 1
              // in 2nd argument of readFileToString for debugging).
              printLongLine(cat("?Warning: Couldn't open or read the file \"",
                  bibFileName,
                  "\".  The bibliographic hyperlinks will not be checked for",
                  " correctness.  The first one is \"", bibTag,
                  "\" in the comment for statement \"",
                  g_Statement[g_showStatement].labelName, "\".",
                  NULL), "", " ");
            } else {
              // The file was not found or had some problem (use verbose mode = 1
              // in 2nd argument of readFileToString for debugging).
              printLongLine(cat("?Warning: There is no bibliography, so",
                  " bibliographic hyperlinks will not be checked for",
                  " correctness.  The first one is \"", bibTag,
                  "\" in the comment for statement \"",
                  g_Statement[g_showStatement].labelName, "\".",
                  NULL), "", " ");
            }
            returnVal = 1; // Error/warning printed
            bibFileContents = ""; // Restore to normal string
            // Assign to a nonsense tag that won't match
            // but tells us an attempt was already made to read the file.
            let(&bibTags, "?");
          } else {
            // Note: In an <A NAME=...> tag, HTML is case-insensitive for A and
            // NAME but case-sensitive for the token after the =
            // Strip all whitespace
            let(&bibFileContents, edit(bibFileContents, 2));
            // Uppercase version for HTML tag search
            let(&bibFileContentsUpper, edit(bibFileContents, 32));
            htmlpos1 = 0;
            while (1) { // Look for all <A NAME=...></A> HTML tags
              htmlpos1 = instr(htmlpos1 + 1, bibFileContentsUpper, "<ANAME=");
              // Note stripped space after <A... - not perfectly robust but
              // good enough if HTML file is legal since <ANAME is not an HTML
              // tag (let's not get into a regex discussion though...).
              if (!htmlpos1) break;
              htmlpos1 = htmlpos1 + 7; // Point to beginning of tag name
              // Extract tag, ignoring any surrounding quotes
              if (bibFileContents[htmlpos1 - 1] == '\''
                  || bibFileContents[htmlpos1 - 1] == '"') htmlpos1++;
              htmlpos2 = instr(htmlpos1, bibFileContents, ">");
              if (!htmlpos2) break;
              htmlpos2--; // Move to character before ">"
              if (bibFileContents[htmlpos2 - 1] == '\''
                  || bibFileContents[htmlpos2 - 1] == '"') htmlpos2--;
              if (htmlpos2 <= htmlpos1) continue; // Ignore bad HTML syntax
              let(&tmp, cat("[",
                  seg(bibFileContents, htmlpos1, htmlpos2), "]", NULL));
              // Check if tag is already in list
              if (instr(1, bibTags, tmp)) {
                printLongLine(cat("?Error: There two occurrences of",
                    " bibliographic reference \"",
                    seg(bibFileContents, htmlpos1, htmlpos2),
                    "\" in the file \"", bibFileName, "\".", NULL), "", " ");
                returnVal = 1; // Error/warning printed
              }
              // Add tag to tag list
              let(&bibTags, cat(bibTags, tmp, NULL));
            } // end while
            if (!bibTags[0]) {
              // No tags found; put dummy partial tag meaning "file read"
              let(&bibTags, "[");
            }
          } // end if (!bibFileContents)
        } // end if (noFileCheck == 0)
        // Assign to permanent tag list for next time
        if (g_showStatement < g_extHtmlStmt) {
          let(&g_htmlBibliographyTags, bibTags);
        // } else {
        } else if (g_showStatement < g_mathboxStmt) {
          let(&extHtmlBibliographyTags, bibTags);
        } else {
          let(&g_htmlBibliographyTags, bibTags);
        }
        // Done reading in HTML file with bibliography
      } // end if (!bibTags[0])
      // See if the tag we found is in the bibliography file
      if (bibTags[0] == '[') {
        // We have a tag list from the bibliography file
        if (!instr(1, bibTags, bibTag)) {
          printLongLine(cat("?Error: The bibliographic reference \"", bibTag,
              "\" in statement \"", g_Statement[g_showStatement].labelName,
              "\" was not found as an <A NAME=\"",
              seg(bibTag, 2, pos2 - pos1),
              "\"></A> anchor in the file \"", bibFileName, "\".", NULL),
              "", " ");
          returnVal = 1; // Error/warning printed
        }
      }
      // End of error-checking

      // Make an HTML reference for the tag
      let(&tmp, cat("[<A HREF=\"",
          bibFileName, "#", seg(bibTag, 2, pos2 - pos1), "\">",
          seg(bibTag, 2, pos2 - pos1), "</A>]", NULL));
      let(&cmt, cat(left(cmt, pos1 - 1), tmp, right(cmt,
          pos2 + 1), NULL));
      let(&cmtMasked, cat(left(cmtMasked, pos1 - 1), tmp, right(cmtMasked,
          pos2 + 1), NULL));
      pos1 = pos1 + (long)strlen(tmp) - (long)strlen(bibTag); // Adjust comment position
    } // end while(1)
  } // end of if (g_htmlFlag)

  // All actions on cmt should be mirrored on cmdMasked, except that
  // math symbols are replaced with blanks in cmdMasked.
  if (strlen(cmt) != strlen(cmtMasked)) bug(2334); // Should be in sync

  // Starting here, we no longer use cmtMasked, so syncing it with cmt
  // isn't important anymore.

  clen = (long)strlen(cmt);
  mode = 'n';
  for (i = 0; i < clen; i++) {
    if (cmt[i] == '`') {
      if (cmt[i + 1] == '`') {
        if (processSymbols != 0) {
          // Escaped ` = actual `
          let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
          clen--;
        }
      } else {
        // We will still enter and exit math mode when
        // processSymbols=0 so as to skip ~ in math symbols.  However,
        // we don't insert the "DOLLAR_SUBST mode" so that later on
        // it will look like normal text.
        // Enter or exit math mode
        if (mode != 'm') {
          mode = 'm';
        } else {
          mode = 'n';
        }

        if (processSymbols != 0) {
          let(&cmt, cat(left(cmt, i), chr(DOLLAR_SUBST) /* $ */, chr(mode),
              right(cmt, i+2), NULL));
          clen++;
          i++;
        }

        // If symbol is preceded by opening punctuation and a space, take out
        // the space so it looks better.
        if (mode == 'm' && processSymbols != 0) {
          let(&tmp, mid(cmt, i - 2, 2));
          if (!strcmp("( ", tmp)) {
            let(&cmt, cat(left(cmt, i - 2), right(cmt, i), NULL));
            clen = clen - 1;
          }
          // We include quotes since symbols are often enclosed in them.
          let(&tmp, mid(cmt, i - 8, 8));
          if (!strcmp("&quot; ", right(tmp, 2))
              && strchr("( ", tmp[0]) != NULL) {
            let(&cmt, cat(left(cmt, i - 2), right(cmt, i), NULL));
            clen = clen - 1;
          }
          free_vstring(tmp);
        }
        // If symbol is followed by a space and closing punctuation, take out
        // the space so it looks better.
        if (mode == 'n' && processSymbols != 0) {
          // (Why must it be i + 2 here but i + 1 in label version below?
          // Didn't investigate but seems strange.)
          let(&tmp, mid(cmt, i + 2, 2));
          if (tmp[0] == ' ' && strchr(CLOSING_PUNCTUATION, tmp[1]) != NULL) {
            let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
            clen = clen - 1;
          }
          // We include quotes since symbols are often enclosed in them.
          let(&tmp, mid(cmt, i + 2, 8));
          if (strlen(tmp) < 8)
              let(&tmp, cat(tmp, space(8 - (long)strlen(tmp)), NULL));
          if (!strcmp(" &quot;", left(tmp, 7))
              && strchr(CLOSING_PUNCTUATION, tmp[7]) != NULL) {
            let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
            clen = clen - 1;
          }
          free_vstring(tmp);
        }
      }
    }
    if (cmt[i] == '~' && mode != 'm') {
      if (cmt[i + 1] == '~' /* Escaped ~ */ || processLabels == 0) {
        if (processLabels != 0) {
          // Escaped ~ = actual ~
          let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
          clen--;
        }
      } else {
        // Enter or exit label mode
        if (mode != 'l') {
          mode = 'l';
          // If there is whitespace after the ~, then remove
          // all whitespace immediately after the ~ to join the ~ to
          // the label.  This enhances the Metamath syntax so that
          // whitespace is now allowed between the ~ and the label, which
          // makes it easier to do global substitutions of labels in a
          // text editor.
          while (isspace((unsigned char)(cmt[i + 1])) && clen > i + 1) {
            let(&cmt, cat(left(cmt, i + 1), right(cmt, i + 3), NULL));
            clen--;
          }
        } else {
          // If you see this bug, the most likely cause is a tilde
          // character in a comment that does not prefix a label or hyperlink.
          // The most common problem is the "~" inside a hyperlink that
          // specifies a user's directory.  To fix it, use a double tilde
          // "~~" to escape it, which will become a single tilde on output.
          g_outputToString = 0;
          printLongLine(cat("?Warning: There is a \"~\" inside of a label",
              " in the comment of statement \"",
              g_Statement[g_showStatement].labelName,
              "\".  Use \"~~\" to escape \"~\" in an http reference.",
              NULL), "", " ");
          returnVal = 1; // Error/warning printed
          g_outputToString = 1;
          mode = 'n';
        }
        let(&cmt, cat(left(cmt, i), chr(DOLLAR_SUBST) /* $ */, chr(mode),
            right(cmt, i+2), NULL));
        clen++;
        i++;

        // If label is preceded by opening punctuation and space, take out
        // the space so it looks better.
        let(&tmp, mid(cmt, i - 2, 2));
        // printf("tmp#%s#\n",tmp);
        if (!strcmp("( ", tmp) || !strcmp("[ ", tmp)) {
          let(&cmt, cat(left(cmt, i - 2), right(cmt, i), NULL));
          clen = clen - 1;
        }
        free_vstring(tmp);
      }
    }

    if (processLabels == 0 && mode == 'l') {
      // We should have prevented it from ever getting into label mode
      bug(2344);
    }

    if ((isspace((unsigned char)(cmt[i]))
            // If the label ends the comment,
            // "</TD>" with no space will be appended before this section.
            || cmt[i] == '<')
        && mode == 'l') {
      // Whitespace exits label mode
      mode = 'n';
      let(&cmt, cat(left(cmt, i), chr(DOLLAR_SUBST) /* $ */, chr(mode),
          right(cmt, i+1), NULL));
      clen = clen + 2;
      i = i + 2;

      // If label is followed by space and end punctuation, take out the space
      // so it looks better.
      let(&tmp, mid(cmt, i + 1, 2));
      if (tmp[0] == ' ' && strchr(CLOSING_PUNCTUATION, tmp[1]) != NULL) {
        let(&cmt, cat(left(cmt, i), right(cmt, i + 2), NULL));
        clen = clen - 1;
      }
      free_vstring(tmp);
    }
    // clen should always remain comment length - do a sanity check here
    if ((signed)(strlen(cmt)) != clen) {
      bug(2311);
    }
  } // Next i
  // End convert line to the old $m..$n and $l..$n

  // Put <HTML> and </HTML> at beginning of line so preformattedMode won't
  // be switched on or off in the middle of processing a line.
  // This also fixes the problem where multiple <HTML>...</HTML> on one
  // line aren't all removed in HTML output, causing w3c validation errors.
  // Note:  "Q<HTML><sup>2</sup></HTML>." will have a space around "2" because
  // of this fix.  Instead use "<HTML>Q<sup>2</sup>.</HTML>" or just "Q^2."
  pos1 = -1; // So -1 + 2 = 1 = start of string for instr()
  while (1) {
    pos1 = instr(pos1 + 2 /* skip new \n */, cmt, "<HTML>");
    if (pos1 == 0
      || convertToHtml == 0 // Don't touch <HTML> in MARKUP command
      ) break;

    // If <HTML> begins a line (after stripping spaces), don't put a \n so
    // that we don't trigger new paragraph mode.
    let(&tmpStr, edit(left(cmt, pos1 - 1), 2 /* discard spaces & tabs */));
    i = (long)strlen(tmpStr);
    if (i == 0) continue;
    if (tmpStr[i - 1] == '\n') continue;

    let(&cmt, cat(left(cmt, pos1 - 1), "\n", right(cmt, pos1), NULL));
  }
  pos1 = -1; // So -1 + 2 = 1 = start of string for instr()
  while (1) {
    pos1 = instr(pos1 + 2 /* skip new \n */, cmt, "</HTML>");
    if (pos1 == 0
      || convertToHtml == 0 // Don't touch </HTML> in MARKUP command
      ) break;

    // If </HTML> begins a line (after stripping spaces), don't put a \n so
    // that we don't trigger new paragraph mode.
    let(&tmpStr, edit(left(cmt, pos1 - 1), 2 /* discard spaces & tabs */));
    i = (long)strlen(tmpStr);
    if (i == 0) continue;
    if (tmpStr[i - 1] == '\n') continue;

    let(&cmt, cat(left(cmt, pos1 - 1), "\n", right(cmt, pos1), NULL));
  }

  cmtptr = cmt; // cmtptr is for scanning cmt

  g_outputToString = 1; // Redirect print2 and printLongLine to g_printString

  while (1) {
    // Get a "line" of text, up to the next new-line.  New-lines embedded
    // in $m and $l sections are ignored, so that these sections will not
    // be dangling.
    lineStart = cmtptr;
    textMode = 1;
    lastLineFlag = 0;
    while (1) {
      if (cmtptr[0] == 0) {
        lastLineFlag = 1;
        break;
      }
      if (cmtptr[0] == '\n' && textMode) break;
      // if (cmtptr[0] == '$') {
      if (cmtptr[0] == DOLLAR_SUBST) {
        if (cmtptr[1] == ')') {
          bug(2312); // Obsolete (should never happen)
          lastLineFlag = 1;
          break;
        }
      }
      if (cmtptr[0] == DOLLAR_SUBST /* '$' */) {
        cmtptr++;
        if (cmtptr[0] == 'm') textMode = 0; // Math mode
        if (cmtptr[0] == 'l') textMode = 0; // Label mode
        if (cmtptr[0] == 'n') textMode = 1; // Normal mode
      }
      cmtptr++;
    }
    let(&sourceLine, space(cmtptr - lineStart));
    memcpy(sourceLine, lineStart, (size_t)(cmtptr - lineStart));
    cmtptr++; // Get past new-line to prepare for next line's scan

    // If the line contains only math mode text, use TeX display mode.
    displayMode = 0;
    let(&tmpStr, edit(sourceLine, 8 + 128)); // Trim spaces
    if (!strcmp(right(tmpStr, (long)strlen(tmpStr) - 1), cat(chr(DOLLAR_SUBST), "n",
        NULL))) let(&tmpStr, left(tmpStr, (long)strlen(tmpStr) - 2)); // Strip $n
    srcptr = tmpStr;
    modeSection = getCommentModeSection(&srcptr, &mode);
    free_vstring(modeSection); // Deallocate
    if (mode == 'm') {
      modeSection = getCommentModeSection(&srcptr, &mode);
      free_vstring(modeSection); // Deallocate
    }
    free_vstring(tmpStr); // Deallocate

    // Convert all sections of the line to text, math, or labels
    let(&outputLine, "");
    srcptr = sourceLine;
    while (1) {
      modeSection = getCommentModeSection(&srcptr, &mode);
      if (!mode) break; // Done
      let(&modeSection, right(modeSection, 3)); // Remove mode-change command
      switch (mode) {
        case 'n': // Normal text
          let(&outputLine, cat(outputLine, modeSection, NULL));
          break;
        case 'l': // Label mode

          if (processLabels == 0) {
            // Labels should be treated as normal text
            bug(2345);
          }
          // Discard leading and trailing blanks; reduce spaces to one space
          let(&modeSection, edit(modeSection, 8 + 128 + 16));
          free_vstring(tmpStr);
          tmpStr = asciiToTt(modeSection);
          if (!tmpStr[0]) {
            // This can happen if ~ is followed by ` (start of math string)
            g_outputToString = 0;
            printLongLine(cat("?Error: There is a \"~\" with no label",
                " in the comment of statement \"",
                g_Statement[g_showStatement].labelName,
                "\".  Check that \"`\" inside of a math symbol is",
                " escaped with \"``\".",
                NULL), "", " ");
            returnVal = 1; // Error/warning printed
            g_outputToString = 1;
          }

          if (!strcmp("http://", left(tmpStr, 7))
              || !strcmp("https://", left(tmpStr, 8))
              || !strcmp("mm", left(tmpStr, 2))) {
            // If the "label" begins with 'http://', then
            // assume it is an external hyperlink and not a real label.
            // This is kind of a syntax kludge but it is easy to do.
            // Added starting with 'mm', which is illegal
            // for set.mm labels - e.g. mmtheorems.html#abc

            if (g_htmlFlag) {
              let(&outputLine, cat(outputLine, "<A HREF=\"", tmpStr,
                  "\">", tmpStr, "</A>", tmp, NULL));
            } else {

              // Generate LaTeX version of the URL
              i = instr(1, tmpStr, "\\char`\\~");
              // The url{} function automatically converts ~ to LaTeX
              if (i != 0) {
                let(&tmpStr, cat(left(tmpStr, i - 1), right(tmpStr, i + 7),
                    NULL));
              }
              let(&outputLine, cat(outputLine, "\\url{", tmpStr,
                  "}", tmp, NULL));
            }
          } else {
            // Do binary search through just $a's and $p's (there
            // are no html pages for local labels).
            i = lookupLabel(tmpStr);
            if (i < 0) {
              g_outputToString = 0;
              printLongLine(cat("?Warning: The label token \"", tmpStr,
                  "\" (referenced in comment of statement \"",
                  g_Statement[g_showStatement].labelName,
                  "\") is not a $a or $p statement label.", NULL), "", " ");
              g_outputToString = 1;
              returnVal = 1; // Error/warning printed
            }

            if (!g_htmlFlag) {
              let(&outputLine, cat(outputLine, "{\\tt ", tmpStr,
                 "}", NULL));
            } else {
              free_vstring(tmp);
              if (addColoredLabelNumber != 0) {
                // When the error above occurs, i < 0 will cause pinkHTML()
                // to issue "(future)" for pleasant readability.
                tmp = pinkHTML(i);
              }
              if (i < 0) {
                // Error output - prevent broken link
                let(&outputLine, cat(outputLine, "<FONT COLOR=blue ",
                    ">", tmpStr, "</FONT>", tmp, NULL));
              } else {
                // Normal output - put hyperlink to the statement
                let(&outputLine, cat(outputLine, "<A HREF=\"", tmpStr,
                    ".html\">", tmpStr, "</A>", tmp, NULL));
              }
            }
          } // if (!strcmp("http://", left(tmpStr, 7))) ... else
          free_vstring(tmpStr); // Deallocate
          break;
        case 'm': // Math mode

          if (processSymbols == 0) {
            // Math symbols should be treated as normal text
            bug(2346);
          }

          free_vstring(tmpStr);
          tmpStr = asciiMathToTex(modeSection, g_showStatement);
          if (!g_htmlFlag) {
            if (displayMode) {
              // It the user's responsibility to establish equation environment
              // in displayMode.
              let(&outputLine, cat(outputLine, /* "\\[", */ edit(tmpStr, 128),
                /* "\\]", */ NULL)); // edit = remove trailing spaces
            } else {
              let(&outputLine, cat(outputLine, "$", edit(tmpStr, 128),
                "$", NULL)); // edit = remove trailing spaces
            }
          } else {
            // Trim leading, trailing spaces in case punctuation
            // surrounds the math symbols in the comment.
            let(&tmpStr, edit(tmpStr, 8 + 128));
            // Enclose math symbols in a span to be used for font selection
            let(&tmpStr, cat(
                (g_altHtmlFlag ? cat("<SPAN ", g_htmlFont, ">", NULL) : ""),
                tmpStr,
                (g_altHtmlFlag ? "</SPAN>" : ""),
                NULL));
            let(&outputLine, cat(outputLine, tmpStr, NULL)); // html
          }
          free_vstring(tmpStr); // Deallocate
          break;
      } // End switch(mode)
      free_vstring(modeSection); // Deallocate
    }
    let(&outputLine, edit(outputLine, 128)); // remove trailing spaces

    if (g_htmlFlag) {
      // Change blank lines into paragraph breaks except in <HTML> mode
      if (!outputLine[0]) { // Blank line
        if (preformattedMode == 0
            && convertToHtml == 1 // Not MARKUP command
            ) { // Make it a paragraph break
          let(&outputLine,
              // Prevent space after last paragraph
              "<P STYLE=\"margin-bottom:0em\">");
        }
      }
      // If a statement comment has a section embedded in
      // <HTML>...</HTML>, we keep the contents verbatim.
      pos1 = instr(1, outputLine, "<HTML>");
      if (pos1 != 0 && convertToHtml == 1) {
        // The line below is probably redundant since we
        // set preformattedMode earlier.  Maybe add a bug check to make sure
        // it is 1 here.
        preformattedMode = 1; // So we don't put <P> for blank lines
        // Strip out the "<HTML>" string
        let(&outputLine, cat(left(outputLine, pos1 - 1),
            right(outputLine, pos1 + 6), NULL));
      }
      pos1 = instr(1, outputLine, "</HTML>");
      if (pos1 != 0 && convertToHtml == 1) {
        preformattedMode = 0;
        // Strip out the "</HTML>" string
        let(&outputLine, cat(left(outputLine, pos1 - 1),
            right(outputLine, pos1 + 7), NULL));
      }
    }

    if (!g_htmlFlag) { // LaTeX
      // Convert <PRE>...</PRE> HTML tags to LaTeX.
      // Leave this in for now
      while (1) {
        pos1 = instr(1, outputLine, "<PRE>");
        if (pos1) {
          let(&outputLine, cat(left(outputLine, pos1 - 1), "\\begin{verbatim} ",
              right(outputLine, pos1 + 5), NULL));
        } else {
          break;
        }
      }
      while (1) {
        pos1 = instr(1, outputLine, "</PRE>");
        if (pos1) {
          let(&outputLine, cat(left(outputLine, pos1 - 1), "\\end{verbatim} ",
              right(outputLine, pos1 + 6), NULL));
        } else {
          break;
        }
      }
      // strip out <HTML>, </HTML>
      // The HTML part may screw up LaTeX; maybe we should just take out
      // any HTML code completely in the future?
      while (1) {
        pos1 = instr(1, outputLine, "<HTML>");
        if (pos1) {
          let(&outputLine, cat(left(outputLine, pos1 - 1),
              right(outputLine, pos1 + 6), NULL));
        } else {
          break;
        }
      }
      while (1) {
        pos1 = instr(1, outputLine, "</HTML>");
        if (pos1) {
          let(&outputLine, cat(left(outputLine, pos1 - 1),
              right(outputLine, pos1 + 7), NULL));
        } else {
          break;
        }
      }
    }

    saveScreenWidth = g_screenWidth;
    // in <PRE> mode, we don't want to wrap the HTML
    // output with spurious newlines. Any large value will
    // do; we just need to accommodate the worst case line length that will
    // result from converting ~ label, [author], ` math ` to HTML
    if (preformattedMode) g_screenWidth = 50000;
    if (errorsOnly == 0) {
      printLongLine(outputLine, "", g_htmlFlag ? "\"" : "\\");
    }
    g_screenWidth = saveScreenWidth;

    freeTempAlloc(); // Clear temporary allocation stack

    if (lastLineFlag) break; // Done
  } // end while(1)

  if (g_htmlFlag) {
    if (convertToHtml != 0) { // Not MARKUP command
      print2("\n"); // Don't change what the previous code did
    } else {
      // Add newline if string is not empty and has no newline at end
      if (g_printString[0] != 0) {
        i = (long)strlen(g_printString);
        if (g_printString[i - 1] != '\n')  {
          print2("\n");
        } else {
          // There is an extra \n added by something previous.  Until
          // we figure out what, take it off so that MARKUP output will
          // equal input when no processing qualifiers are used.
          if (i > 1) {
            if (g_printString[i - 2] == '\n') {
              let(&g_printString, left(g_printString, i - 1));
            }
          }
        }
      }
    }
  } else { // LaTeX mode
    if (!g_oldTexFlag) {
      // Suppress blank line for LaTeX.
      // print2("\n");
    } else {
      print2("\n");
    }
  }

  g_outputToString = 0; // Restore normal output
  if (errorsOnly == 0) {
    fprintf(g_texFilePtr, "%s", g_printString);
  }

  free_vstring(g_printString); // Deallocate strings
  free_vstring(sourceLine);
  free_vstring(outputLine);
  free_vstring(cmt);
  free_vstring(cmtMasked);
  free_vstring(tmpComment);
  free_vstring(tmp);
  free_vstring(tmpMasked);
  free_vstring(tmpStr);
  free_vstring(tmpStrMasked);
  free_vstring(bibTag);
  free_vstring(bibFileName);
  free_vstring(bibFileContents);
  free_vstring(bibFileContentsUpper);
  free_vstring(bibTags);

  return returnVal; // 1 if error/warning found
} // printTexComment

void printTexLongMath(nmbrString *mathString,
    // Start prefix in "screen display" mode e.g.
    // "abc $p"; it is converted to the appropriate format.  Non-zero
    // length means proof step in HTML mode, as opposed to assertion etc.
    vstring startPrefix,
    // Prefix for continuation lines.  Not used in
    // HTML mode.  Warning:  contPrefix must not be temporarily allocated
    // (as a cat, left, etc. argument) by caller.
    vstring contPrefix,
    // hypStmt, if non-zero, is the statement number to be
    // referenced next to the hypothesis link in html.
    long hypStmt,
    // Indentation amount of proof step -
    // note that this is 0 for last step of proof.
    long indentationLevel)
{
#define INDENTATION_OFFSET 1
  long i, j, k, n;
  long pos;
  vstring_def(tex);
  vstring_def(texLine);
  vstring_def(texFull);
  vstring_def(sPrefix);
  vstring_def(htmStep);
  vstring_def(htmStepTag);
  vstring_def(htmHyp);
  vstring_def(htmRef);
  vstring_def(htmLocLab);
  vstring_def(tmp);
  vstring_def(descr);
  char refType = '?'; // 'e' means $e, etc.

  let(&sPrefix, startPrefix); // Save it; it may be temp alloc

  if (!g_texDefsRead) return; // TeX defs were not read (error was printed)
  g_outputToString = 1; // Redirect print2 and printLongLine to g_printString

  // Note that the "tex" assignment below will be used only when !g_htmlFlag
  // and g_oldTexFlag, or when g_htmlFlag and len(sPrefix)>0
  free_vstring(tex);
  // asciiToTt allocates; we must deallocate
  // Example: sPrefix = " 4 2,3 ax-mp  $a "
  //          tex = "\ 4\ 2,3\ ax-mp\ \ \$a\ " in !g_htmlFlag mode
  //          tex = " 4 2,3 ax-mp  $a " in g_htmlFlag mode
  tex = asciiToTt(sPrefix);
  let(&texLine, "");

  // Get statement type of proof step reference
  i = instr(1, sPrefix, "$");
  if (i) refType = sPrefix[i]; // Character after the "$"

  if (g_htmlFlag || !g_oldTexFlag) {

    // Process a proof step prefix
    if (strlen(sPrefix)) { // It's a proof step
      // Make each token a separate table column for HTML.
      // This is a kludge that only works with /LEMMON style proofs!
      // Note that asciiToTt() above puts "\ " when not in
      // g_htmlFlag mode, so use sPrefix instead of tex so it will work in
      // !g_oldTexFlag mode.

      // In HTML mode, sPrefix has two possible formats:
      //   "2 ax-1  $a "
      //   "3 1,2 ax-mp  $a "
      // In LaTeX mode (!g_htmlFlag), sPrefix has one format:
      //   "8   maj=ax-1  $a "
      //   "9 a1i=ax-mp $a "

      let(&tex, edit(sPrefix, 8 // no leading spaces
           + 16 // reduce spaces and tabs
           + 128)); // no trailing spaces

      i = 0;
      pos = 1;
      while (pos) {
        pos = instr(1, tex, " ");
        if (pos) {
          if (i > 3) { // added for extra safety for the future
            bug(2316);
          }
          if (i == 0) let(&htmStep, left(tex, pos - 1));
          if (i == 1) let(&htmHyp, left(tex, pos - 1));
          if (i == 2) let(&htmRef, left(tex, pos - 1));
          if (i == 3) let(&htmLocLab, left(tex, pos - 1));

          let(&tex, right(tex, pos + 1));
          i++;
        }
      }

      if (i == 3 && htmRef[0] == '@') {
        // The referenced statement has no hypotheses but has a local
        // label e.g."2 a4s @2: $p"
        let(&htmLocLab, htmRef);
        let(&htmRef, htmHyp);
        let(&htmHyp, "");
      }

      if (i < 3) {
        // The referenced statement has no hypotheses e.g. "4 ax-1 $a"
        let(&htmRef, htmHyp);
        let(&htmHyp, "");

        // Change "maj=ax-1" to "ax-1" so \ref{} produced by
        // "show proof .../tex" will match \label{} produced by
        // "show statement .../tex"
        // Earlier we set the noIndentFlag (Lemmon
        // proof) in the SHOW PROOF.../TEX call in metamath.c, so the
        // hypothesis ref list will be available just like in the HTML
        // output.
        // We now consider "=" a bug since the call via typeProof() in
        // metamath.c now always has noIndentFlag = 1.
        if (!g_htmlFlag) {
          pos = instr(1, htmRef, "=");
          if (pos) bug(2342);
        }
      }
    } // if (strlen(sPrefix)) (end processing proof step prefix)
  }

  if (!g_htmlFlag) {
    if (g_oldTexFlag) {
      // Trim down long start prefixes so they won't overflow line,
      // by putting their tokens into \m macros.
#define TRIMTHRESHOLD 60
      i = (long)strlen(tex);
      while (i > TRIMTHRESHOLD) {
        if (tex[i] == '\\') {
          // Move to math part
          let(&texLine, cat("\\m{\\mbox{\\tt", right(tex, i + 1), "}}",
              texLine, NULL));
          // Take off of prefix part
          let(&tex, left(tex, i));
        }
        i--;
      }

      printLongLine(cat(
          "\\setbox\\startprefix=\\hbox{\\tt ", tex, "}", NULL), "", "\\");
      free_vstring(tex); // Deallocate
      tex = asciiToTt(contPrefix);
      printLongLine(cat(
          "\\setbox\\contprefix=\\hbox{\\tt ", tex, "}", NULL), "", "\\");
      print2("\\startm\n");
    }
  } else { // g_htmlFlag
    if (strlen(sPrefix)) { // It's a proof step

      if (htmHyp[0] == 0)
        let(&htmHyp, "&nbsp;"); // Insert blank field for Lemmon ref w/out hyp

      // Put hyperlinks on hypothesis
      // label references in SHOW STATEMENT * /HTML, ALT_HTML output.
      // Use a separate tag to put into the math cell,
      // so it will link to the top of the math cell.
      let(&htmStepTag, cat("<A NAME=\"", htmStep, "\">","</A>", NULL));
      i = 1;
      pos = 1;
      while (pos && strcmp(htmHyp, "&nbsp;")) {
        pos = instr(i,htmHyp, ",");
        if (!pos) pos = len(htmHyp) + 1;
        let(&htmHyp, cat(left(htmHyp, i - 1),
            "<A HREF=\"#",
            seg(htmHyp, i, pos - 1),
            "\">",
            seg(htmHyp, i, pos - 1),
            "</A>",
            right(htmHyp, pos),
            NULL));
        // Break out of loop if we hit the end
        pos += 16 + len(seg(htmHyp, i, pos - 1)) + 1;
        if (!instr(i, htmHyp, ",")) break;
        i = pos;
      }

      // Add a space after each comma so very long hypotheses
      // lists will wrap in an HTML table cell, e.g. gomaex3 in ql.mm
      pos = instr(1, htmHyp, ",");
      while (pos) {
        let(&htmHyp, cat(left(htmHyp, pos), " ", right(htmHyp, pos + 1), NULL));
        pos = instr(pos + 1, htmHyp, ",");
      }

      if (refType == 'e' || refType == 'f') {
        // A hypothesis - don't include link
        printLongLine(cat("<TR ALIGN=LEFT><TD>", htmStep, "</TD><TD>",
            htmHyp, "</TD><TD>", htmRef,
            "</TD><TD>",
            htmStepTag, // Put the <A NAME=...></A> tag at start of math symbol cell
            NULL), "", "\"");
      } else {
        if (hypStmt <= 0) {
          printLongLine(cat("<TR ALIGN=LEFT><TD>", htmStep, "</TD><TD>",
              htmHyp, "</TD><TD><A HREF=\"", htmRef, ".html\">", htmRef,
              "</A></TD><TD>",
              htmStepTag, // Put the <A NAME=...></A> tag at start of math symbol cell
              NULL), "", "\"");
        } else {
          // Include step number reference.  The idea is that this will
          // help the user to recognized "important" (vs. early trivial
          // logic) steps.  This prints a small pink statement number
          // after the hypothesis statement label.
          free_vstring(tmp);
          tmp = pinkHTML(hypStmt);

          // Get description for mod below
          free_vstring(descr); // Deallocate previous description
          descr = getDescription(hypStmt);
          let(&descr, edit(descr, 4 + 16)); // Discard lf/cr; reduce spaces
#define MAX_DESCR_LEN 87
          if (strlen(descr) > MAX_DESCR_LEN) { // Truncate long lines
            i = MAX_DESCR_LEN - 3;
            while (i >= 0) { // Get to previous word boundary
              if (descr[i] == ' ') break;
              i--;
            }
            let(&descr, cat(left(descr, i), "...", NULL));
          }
          i = 0;
          while (descr[i] != 0) { // Convert double quote to single
            descr[i] = (char)(descr[i] == '"' ? '\'' : descr[i]);
            i++;
          }

          printLongLine(cat("<TR ALIGN=LEFT><TD>", htmStep, "</TD><TD>",
              htmHyp, "</TD><TD><A HREF=\"", htmRef, ".html\"",
              // Put in a TITLE entry for mouseover tooltip,
              // as suggested by Reinder Verlinde.
              " TITLE=\"", descr, "\"",
              ">", htmRef,
              "</A>", tmp,
              "</TD><TD>",
              htmStepTag, // Put the <A NAME=...></A> tag at start of math symbol cell
              NULL), "", "\"");
        }
      }
      // Indent web proof displays
      let(&tmp, "");
      for (i = 1; i <= indentationLevel; i++) {
        let(&tmp, cat(tmp, ". ", NULL));
      }
      let(&tmp, cat("<SPAN CLASS=i>",
          tmp,
          str((double)(indentationLevel + INDENTATION_OFFSET)), "</SPAN>",
          NULL));
      printLongLine(tmp, "", "\"");
      free_vstring(tmp);
    } // strlen(sPrefix)
  } // g_htmlFlag
  free_vstring(sPrefix); // Deallocate

  free_vstring(tex);
  tex = getTexLongMath(mathString, hypStmt);
  let(&texLine, cat(texLine, tex, NULL));

  if (!g_htmlFlag) { // LaTeX
    if (!g_oldTexFlag) {
      if (refType == 'e' || refType == 'f') {
        // A hypothesis - don't include \ref{}
        texFull = cat("  ", // texFull[0] should not be a "{" character.
          // If not first step, so print "\\" LaTeX line break
          !strcmp(htmStep, "1") ? "" : "\\\\ ",
          htmStep, // Step number
          " && ",
          " & ",
          texLine,
          // Don't put space to help prevent bad line break
          "&\\text{Hyp~",
          // The following puts a hypothesis number such as "2" if
          // $e label is "abc.2"; if no ".", will be whole label.
          right(htmRef, instr(1, htmRef, ".") + 1),
          "}\\notag%",
          // Add full label as LaTeX comment - note lack of space after
          // "%" above to prevent bad line break.
          htmRef, NULL),

        // To avoid generating incorrect TeX, line breaking is forbidden inside
        // scopes of curly braces.  However breaking between '\{' and '\}' is
        // allowed.
        // The spaces that should not be matched with 'breakMatch' are
        // temporarily changed to ASCII 3 before the 'printLongLine' procedure
        // is called.  The procedure rewraps the 'line' argument matching the
        // unchanged spaces only, thus ensuring bad breaks will be avoided.
        // The reverse is done in the 'print2()' function, where all ASCII 3's
        // are converted back to spaces.
        // k counts the scope level we are in.
        k = 0;
        n = (long)strlen(texFull);
        for (j = 1; j < n; j++) { // We don't need to check texFull[0].
          // We enter a non "\{" scope.
          if (texFull[j] == '{' && texFull[j - 1] != '\\') k++;
          // We escape a non "\}" scope.
          if (texFull[j] == '}' && texFull[j - 1] != '\\') k--;
          // If k > 0 then we are inside a scope.
          if (texFull[j] == ' ' && k > 0) texFull[j] = QUOTED_SPACE;
        }
        printLongLine(texFull, "    \\notag \\\\ && & \\qquad ", /* Continuation line prefix */ " ");
      } else {
        texFull = cat("  ", // texFull[0] should not be a "{" character.
          // If not first step, so print "\\" LaTeX line break
          !strcmp(htmStep, "1") ? "" : "\\\\ ",
          htmStep, // Step number
          " && ",

          // Local label if any e.g. "@2:"
          (htmLocLab[0] != 0) ? cat(htmLocLab, "\\ ", NULL) : "",

          " & ",
          texLine,
          // Don't put space to help prevent bad line break

          // Surround \ref with \mbox for non-math-mode
          // symbolic labels (due to \tag{..} in mmcmds.c).  Also,
          // move hypotheses to after referenced label.
          "&",
          "(",

          // Don't make local label a \ref
          (htmRef[0] != '@') ?
              cat("\\mbox{\\ref{eq:", htmRef, "}}", NULL)
              : htmRef,

          htmHyp[0] ? "," : "",
          htmHyp,
          ")\\notag", NULL),

        // To avoid generating incorrect TeX, line breaking is forbidden inside
        // scopes of curly braces.  However breaking between '\{' and '\}' is
        // allowed.
        // The spaces that should not be matched with 'breakMatch' are
        // temporarily changed to ASCII 3 before the 'printLongLine' procedure
        // is called.  The procedure rewraps the 'line' argument matching the
        // unchanged spaces only, thus ensuring bad breaks will be avoided.
        // The reverse is done in the 'print2()' function, where all ASCII 3's
        // are converted back to spaces.
        // k counts the scope level we are in.
        k = 0;
        n = (long)strlen(texFull);
        for (j = 1; j < n; j++) { // We don't need to check texFull[0].
          // We enter a non "\{" scope.
          if (texFull[j] == '{' && texFull[j - 1] != '\\') k++;
          // We escape a non "\}" scope.
          if (texFull[j] == '}' && texFull[j - 1] != '\\') k--;
          // If k > 0 then we are inside a scope.
          if (texFull[j] == ' ' && k > 0) texFull[j] = QUOTED_SPACE;
        }
        printLongLine(texFull, "    \\notag \\\\ && & \\qquad ", /* Continuation line prefix */ " ");
      }
    } else {
      printLongLine(texLine, "", "\\");
      print2("\\endm\n");
    }
  } else { // HTML
    printLongLine(cat(texLine, "</TD></TR>", NULL), "", "\"");
  }

  g_outputToString = 0; // Restore normal output
  fprintf(g_texFilePtr, "%s", g_printString);
  free_vstring(g_printString);

  free_vstring(descr); // Deallocate
  free_vstring(htmStep); // Deallocate
  free_vstring(htmStepTag); // Deallocate
  free_vstring(htmHyp); // Deallocate
  free_vstring(htmRef); // Deallocate
  free_vstring(htmLocLab); // Deallocate
  free_vstring(tmp); // Deallocate
  free_vstring(texLine); // Deallocate
  free_vstring(tex); // Deallocate
} // printTexLongMath

void printTexTrailer(flag texTrailerFlag) {

  if (texTrailerFlag) {
    g_outputToString = 1; // Redirect print2 and printLongLine to g_printString
    // May have stuff to be printed
    if (!g_htmlFlag) let(&g_printString, "");
    if (!g_htmlFlag) {
      print2("\\end{document}\n");
    } else {
      print2("</TABLE></CENTER>\n");
      print2("<TABLE BORDER=0 WIDTH=\"100%s\">\n", "%");
      print2("<TR><TD WIDTH=\"25%s\">&nbsp;</TD>\n", "%");
      print2("<TD ALIGN=CENTER VALIGN=BOTTOM>\n");
      print2("<FONT SIZE=-2 FACE=sans-serif>\n");
      print2("Copyright terms:\n");
      print2("<A HREF=\"../copyright.html#pd\">Public domain</A>\n");
      print2("</FONT></TD><TD ALIGN=RIGHT VALIGN=BOTTOM WIDTH=\"25%s\">\n", "%");
      print2("<FONT SIZE=-2 FACE=sans-serif>\n");
      print2("<A HREF=\"http://validator.w3.org/check?uri=referer\">\n");
      print2("W3C validator</A>\n");
      print2("</FONT></TD></TR></TABLE>\n");
      print2("</BODY></HTML>\n");
    }
    g_outputToString = 0; // Restore normal output
    fprintf(g_texFilePtr, "%s", g_printString);
    free_vstring(g_printString);
  }
} // printTexTrailer

// WRITE THEOREM_LIST command:  Write out theorem list
// into mmtheorems.html, mmtheorems1.html,...
void writeTheoremList(long theoremsPerPage, flag showLemmas, flag noVersioning)
{
  nmbrString_def(nmbrStmtNmbr);
  long pages, page, assertion, assertions, lastAssertion;
  long s, p, i1, i2;
  vstring_def(str1);
  vstring_def(str3);
  vstring_def(str4);
  vstring_def(prevNextLinks);
  long partCntr; // Counter for hugeHdr
  long sectionCntr; // Counter for bigHdr
  long subsectionCntr; // Counter for smallHdr
  long subsubsectionCntr; // Counter for tinyHdr
  vstring_def(outputFileName);
  FILE *outputFilePtr;
  long passNumber; // for summary/detailed table of contents

  // for table of contents
  vstring_def(hugeHdr);
  vstring_def(bigHdr);
  vstring_def(smallHdr);
  vstring_def(tinyHdr);
  vstring_def(hugeHdrComment);
  vstring_def(bigHdrComment);
  vstring_def(smallHdrComment);
  vstring_def(tinyHdrComment);
  long stmt, i;
  pntrString_def(pntrHugeHdr);
  pntrString_def(pntrBigHdr);
  pntrString_def(pntrSmallHdr);
  pntrString_def(pntrTinyHdr);
  pntrString_def(pntrHugeHdrComment);
  pntrString_def(pntrBigHdrComment);
  pntrString_def(pntrSmallHdrComment);
  pntrString_def(pntrTinyHdrComment);
  vstring_def(hdrCommentMarker);
  vstring_def(hdrCommentAnchor);
  flag hdrCommentAnchorDone = 0;

  // Populate the statement map.
  // ? ? ? Future:  is assertions same as g_Statement[g_statements].pinkNumber?
  nmbrLet(&nmbrStmtNmbr, nmbrSpace(g_statements + 1));
  assertions = 0; // Number of $p's + $a's
  for (s = 1; s <= g_statements; s++) {
    if (g_Statement[s].type == a_ || g_Statement[s].type == p_) {
      assertions++; // Corresponds to pink number
      nmbrStmtNmbr[assertions] = s;
    }
  }
  if (assertions != g_Statement[g_statements].pinkNumber) bug(2328);

  // Table of contents.
  // Allocate array for section headers found.
  pntrLet(&pntrHugeHdr, pntrSpace(g_statements + 1));
  pntrLet(&pntrBigHdr, pntrSpace(g_statements + 1));
  pntrLet(&pntrSmallHdr, pntrSpace(g_statements + 1));
  pntrLet(&pntrTinyHdr, pntrSpace(g_statements + 1));
  pntrLet(&pntrHugeHdrComment, pntrSpace(g_statements + 1));
  pntrLet(&pntrBigHdrComment, pntrSpace(g_statements + 1));
  pntrLet(&pntrSmallHdrComment, pntrSpace(g_statements + 1));
  pntrLet(&pntrTinyHdrComment, pntrSpace(g_statements + 1));

  pages = ((assertions - 1) / theoremsPerPage) + 1;
  for (page = 0; page <= pages; page++) {
    // Open file
    let(&outputFileName,
        cat("mmtheorems", (page > 0) ? str((double)page) : "", ".html", NULL));
    print2("Creating %s\n", outputFileName);
    outputFilePtr = fSafeOpen(outputFileName, "w", noVersioning);
    if (!outputFilePtr) goto TL_ABORT; // Couldn't open it (error msg was provided)

    // Output header
    // TODO 14-Jan-2016: why aren't we using printTexHeader?

    g_outputToString = 1;
    print2("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Transitional//EN\"\n");
    print2(     "    \"http://www.w3.org/TR/html4/loose.dtd\">\n");
    print2("<HTML LANG=\"EN-US\">\n");
    print2("<HEAD>\n");
    print2("%s%s\n", "<META HTTP-EQUIV=\"Content-Type\" ",
        "CONTENT=\"text/html; charset=iso-8859-1\">");
    // Improve mobile device display per David A. Wheeler
    print2("<META NAME=\"viewport\" "
      "CONTENT=\"width=device-width, initial-scale=1.0\">\n");

    print2("<STYLE TYPE=\"text/css\">\n");
    print2("<!--\n");
    // align math symbol images to text
    print2("img { margin-bottom: -4px }\n");
    // Print style sheet for colored number that goes after statement label
    print2(".r { font-family: \"Arial Narrow\";\n");
    print2("     font-size: x-small;\n");
    print2("   }\n");
    print2("-->\n");
    print2("</STYLE>\n");
    printLongLine(g_htmlCSS, "", " ");

    // print2("%s\n", cat("<TITLE>", htmlTitle, " - ",
    //   // Strip off ".html"
    //   left(outputFileName, (long)strlen(outputFileName) - 5), "</TITLE>", NULL));
    // Put page name before "Metamath Proof Explorer" etc.
    printLongLine(cat("<TITLE>",
        ((page == 0)
            ? "TOC of Theorem List"
            : cat("P. ", str((double)page), " of Theorem List", NULL)),

        " - ",
        htmlTitle,
        "</TITLE>",
        NULL), "", "\"");
    // Icon for bookmark
    print2("%s%s\n", "<LINK REL=\"shortcut icon\" HREF=\"favicon.ico\" ",
        "TYPE=\"image/x-icon\">");

    // Image alignment fix
    print2("<STYLE TYPE=\"text/css\">\n");
    print2("<!--\n");
    // Optional information but takes unnecessary file space.
    // print2("/* Math symbol GIFs will be shifted down 4 pixels to align with\n");
    // print2("   normal text for compatibility with various browsers.  The old\n");
    // print2("   ALIGN=TOP for math symbol images did not align in all browsers\n");
    // print2("   and should be deleted.  All other images must override this\n");
    // print2("   shift with STYLE=\"margin-bottom:0px\". */\n");
    print2("img { margin-bottom: -4px }\n");
    print2("-->\n");
    print2("</STYLE>\n");

    print2("</HEAD>\n");
    print2("<BODY BGCOLOR=\"#FFFFFF\">\n");
    print2("<TABLE BORDER=0 WIDTH=\"100%s\"><TR>\n", "%");
    print2("<TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%s\"\n", "%");
    printLongLine(cat("ROWSPAN=2>", g_htmlHome, "</TD>", NULL), "", "\"");
    printLongLine(cat(
        "<TD NOWRAP ALIGN=CENTER ROWSPAN=2><FONT SIZE=\"+3\" COLOR=",
        GREEN_TITLE_COLOR, "><B>", htmlTitle, "</B></FONT>",
        "<BR><FONT SIZE=\"+2\" COLOR=",
        GREEN_TITLE_COLOR,
        "><B>Theorem List (",
        ((page == 0)
            ? "Table of Contents"
            : cat("p. ", str((double)page), " of ", str((double)pages), NULL)),
        ")</B></FONT>",
        NULL), "", "\"");

    // Put Previous/Next links into web page
    print2("</TD><TD NOWRAP ALIGN=RIGHT VALIGN=TOP WIDTH=\"25%c\"><FONT\n", '%');
    print2(" SIZE=-1 FACE=sans-serif>\n");

    // Output title with current page
    // Output previous and next

    // Assign prevNextLinks once here since it is used 3 times
    let(&prevNextLinks, cat("<A HREF=\"mmtheorems",
        (page > 0)
            ? ((page - 1 > 0) ? str((double)page - 1) : "")
            : ((pages > 0) ? str((double)pages) : ""),
        ".html\">", NULL));
    if (page > 0) {
      let(&prevNextLinks, cat(prevNextLinks,
          "&lt; Previous</A>&nbsp;&nbsp;", NULL));
    } else {
      let(&prevNextLinks, cat(prevNextLinks, "&lt; Wrap</A>&nbsp;&nbsp;", NULL));
    }
    let(&prevNextLinks, cat(prevNextLinks, "<A HREF=\"mmtheorems",
        (page < pages)
            ? str((double)page + 1)
            : "",
        ".html\">", NULL));
    if (page < pages) {
      let(&prevNextLinks, cat(prevNextLinks, "Next &gt;</A>", NULL));
    } else {
      let(&prevNextLinks, cat(prevNextLinks, "Wrap &gt;</A>", NULL));
    }

    printLongLine(prevNextLinks,
        " ", // Start continuation line with space
        "\""); // Don't break inside quotes e.g. "Arial Narrow"

    // Finish up header
    // Print the GIF/Unicode Font choice, if directories are specified
    if (htmlDir[0]) {
      if (g_altHtmlFlag) {
        print2("</FONT></TD></TR><TR><TD ALIGN=RIGHT><FONT FACE=sans-serif\n");
        print2("SIZE=-2>Bad symbols? Try the\n");
        print2("<BR><A HREF=\"%s%s\">GIF\n", htmlDir, outputFileName);
        print2("version</A>.</FONT></TD>\n");
      } else {
        print2("</FONT></TD></TR><TR><TD ALIGN=RIGHT><FONT FACE=sans-serif\n");
        print2("SIZE=-2>Browser slow? Try the\n");
        print2("<BR><A HREF=\"%s%s\">Unicode\n", altHtmlDir, outputFileName);
        print2("version</A>.</FONT></TD>\n");
      }
    }

    // Make breadcrumb font to match other pages
    print2("<TR>\n");
    print2(
      "<TD COLSPAN=3 ALIGN=LEFT VALIGN=TOP><FONT SIZE=-2 FACE=sans-serif>\n");
    print2("<BR>\n"); // Add a little more vertical space

    // Print some useful links
    print2("<A HREF=\"../mm.html\">Mirrors</A>\n");
    print2("&nbsp;&gt;&nbsp;<A HREF=\"../index.html\">\n");
    print2("Metamath Home Page</A>\n");

    // Normally, g_htmlBibliography in the .mm file will have the
    // project home page, and we depend on this here rather than
    // extracting from g_htmlHome.
    print2("&nbsp;&gt;&nbsp;<A HREF=\"%s\">\n", g_htmlBibliography);

    // Put a meaningful abbreviation for the project home page
    // by extracting capital letters from title.
    let(&str1, "");
    s = (long)strlen(htmlTitle);
    for (i = 0; i < s; i++) {
      if (htmlTitle[i] >= 'A' && htmlTitle[i] <= 'Z') {
        let(&str1, cat(str1, chr(htmlTitle[i]), NULL));
      }
    }
    print2("%s Home Page</A>\n", str1);

    if (page != 0) {
      print2("&nbsp;&gt;&nbsp;<A HREF=\"mmtheorems.html\">\n");
      print2("Theorem List Contents</A>\n");
    } else {
      print2("&nbsp;&gt;&nbsp;\n");
      print2("Theorem List Contents\n");
    }

    // Assume there is a Most Recent page when the .mm has a mathbox stmt
    // (currently set.mm and iset.mm).
    if (g_mathboxStmt < g_statements + 1) {
      print2("&nbsp;&gt;&nbsp;<A HREF=\"mmrecent.html\">\n");
      print2("Recent Proofs</A>\n");
    }

    print2("&nbsp; &nbsp; &nbsp; <B><FONT COLOR=%s>\n", GREEN_TITLE_COLOR);
    print2("This page:</FONT></B> \n");

    if (page == 0) {
      print2("&nbsp;<A HREF=\"#mmdtoc\">Detailed Table of Contents</A>&nbsp;\n");
    }

    print2("<A HREF=\"#mmpglst\">Page List</A>\n");

    // Change breadcrumb font to match other pages
    print2("</FONT>\n");
    print2("</TD>\n");
    print2("</TR></TABLE>\n");

    print2("<HR NOSHADE SIZE=1>\n");

    // Write out HTML page so far
    fprintf(outputFilePtr, "%s", g_printString);
    g_outputToString = 0;
    free_vstring(g_printString);

    // Add table of contents to first WRITE THEOREM page
    if (page == 0) { // We're on ToC page

      // Pass 1: table of contents summary; pass 2: detail
      for (passNumber = 1; passNumber <= 2; passNumber++) {
        g_outputToString = 1;
        if (passNumber == 1) {
          print2("<P><CENTER><B>Table of Contents Summary</B></CENTER>\n");
        } else {
          print2("<P><CENTER><A NAME=\"mmdtoc\"></A><B>Detailed Table of Contents</B><BR>\n");
          print2("<B>(* means the section header has a description)</B></CENTER>\n");
        }

        fprintf(outputFilePtr, "%s", g_printString);

        g_outputToString = 0;
        free_vstring(g_printString);

        free_vstring(hugeHdr);
        free_vstring(bigHdr);
        free_vstring(smallHdr);
        free_vstring(tinyHdr);
        free_vstring(hugeHdrComment);
        free_vstring(bigHdrComment);
        free_vstring(smallHdrComment);
        free_vstring(tinyHdrComment);
        partCntr = 0; // Initialize counters
        sectionCntr = 0;
        subsectionCntr = 0;
        subsubsectionCntr = 0;
        for (stmt = 1; stmt <= g_statements; stmt++) {
          // Output the headers for $a and $p statements
          if (g_Statement[stmt].type == p_ || g_Statement[stmt].type == a_) {
            hdrCommentAnchorDone = 0;
            getSectionHeadings(stmt, &hugeHdr, &bigHdr, &smallHdr,
                &tinyHdr,
                &hugeHdrComment, &bigHdrComment, &smallHdrComment,
                &tinyHdrComment,
                0, // fineResolution
                0); // fullComment
            if (hugeHdr[0] || bigHdr[0] || smallHdr[0] || tinyHdr[0]) {
              // Write to the table of contents
              g_outputToString = 1;
              i = ((g_Statement[stmt].pinkNumber - 1) / theoremsPerPage)
                  + 1; // Page #
              // Note that page 1 has no number after mmtheorems
              // let(&str3, cat("mmtheorems", (i == 1) ? "" : str(i), ".html#",
              let(&str3, cat("mmtheorems", str((double)i), ".html#",
                  // g_Statement[stmt].labelName, NULL));
                  // Link to page/location - no theorem can be named "mm*"
                  "mm", str((double)(g_Statement[stmt].pinkNumber)), NULL));
              free_vstring(str4);
              str4 = pinkHTML(stmt);
              if (hugeHdr[0]) {

                // Create part number
                partCntr++;
                sectionCntr = 0;
                subsectionCntr = 0;
                subsubsectionCntr = 0;
                let(&hugeHdr, cat("PART ", str((double)partCntr), "&nbsp;&nbsp;",
                    hugeHdr, NULL));

                // Put an asterisk before the header if the header has a comment
                if (hugeHdrComment[0] != 0 && passNumber == 2) {
                  let(&hdrCommentMarker, "*");
                  if (hdrCommentAnchorDone == 0) {
                    let(&hdrCommentAnchor, cat(
                        "<A NAME=\"",
                        g_Statement[stmt].labelName, "\"></A>",

                        // "&#8203;" is a "zero-width" space to
                        // workaround Chrome bug that jumps to wrong anchor.
                        "&#8203;",

                        NULL));
                    hdrCommentAnchorDone = 1;
                  } else {
                    let(&hdrCommentAnchor, "");
                  }
                } else {
                  let(&hdrCommentMarker, "");
                  let(&hdrCommentAnchor, "");
                }

                printLongLine(cat(
                    // In detailed section, add an anchor to reach it from
                    // summary section.
                    (passNumber == 2) ?
                        cat("<A NAME=\"dtl:", str((double)partCntr),
                            "\"></A>",
                            // "&#8203;" is a "zero-width" space to
                            // workaround Chrome bug that jumps to wrong anchor.
                            "&#8203;",
                            NULL) : "",

                    // Add an anchor to the "sandbox" theorem for use by mmrecent.html
                    // We use "sandbox:bighdr" for both here and
                    // below so that either huge or big header type could
                    // be used to start mathbox sections.
                    (stmt == g_mathboxStmt && bigHdr[0] == 0
                          && passNumber == 1 // Only in summary TOC
                          ) ?
                        // Note the colon so it won't conflict w/ theorem name anchor
                        // "&#8203;" is a "zero-width" space to
                        // workaround Chrome bug that jumps to wrong anchor.
                        "<A NAME=\"sandbox:bighdr\"></A>&#8203;" : "",

                    hdrCommentAnchor,
                    "<A HREF=\"",

                    (passNumber == 1) ?
                        cat("#dtl:", str((double)partCntr), NULL) // Link to detailed toc
                        : cat(str3, "h", NULL), // Link to thm list

                    "\"><B>",
                    hdrCommentMarker,
                    hugeHdr, "</B></A>",
                    "<BR>", NULL),
                    " ", // Start continuation line with space
                    "\""); // Don't break inside quotes e.g. "Arial Narrow"
                if (passNumber == 2) {
                  // Assign to array for use during theorem output
                  let((vstring *)(&pntrHugeHdr[stmt]), hugeHdr);
                  let((vstring *)(&pntrHugeHdrComment[stmt]), hugeHdrComment);
                }
                free_vstring(hugeHdr);
                free_vstring(hugeHdrComment);
              }
              if (bigHdr[0]) {

                // Create section number
                sectionCntr++;
                subsectionCntr = 0;
                subsubsectionCntr = 0;
                let(&bigHdr, cat(str((double)partCntr), ".", str((double)sectionCntr),
                    "&nbsp;&nbsp;",
                    bigHdr, NULL));

                // Put an asterisk before the header if the header has a comment
                if (bigHdrComment[0] != 0 && passNumber == 2) {
                  let(&hdrCommentMarker, "*");
                  if (hdrCommentAnchorDone == 0) {
                    let(&hdrCommentAnchor, cat(
                        "<A NAME=\"",
                        g_Statement[stmt].labelName, "\"></A>",

                        // "&#8203;" is a "zero-width" space to
                        // workaround Chrome bug that jumps to wrong anchor.
                        "&#8203;",

                        NULL));
                    hdrCommentAnchorDone = 1;
                  } else {
                    let(&hdrCommentAnchor, "");
                  }
                } else {
                  let(&hdrCommentMarker, "");
                  let(&hdrCommentAnchor, "");
                }

                printLongLine(cat(
                    "&nbsp; &nbsp; &nbsp; ", // Indentation spacing

                    // In detailed section, add an anchor to reach it from summary section.
                    (passNumber == 2) ?
                        cat("<A NAME=\"dtl:", str((double)partCntr), ".",
                            str((double)sectionCntr), "\"></A>",

                            // "&#8203;" is a "zero-width" space to
                            // workaround Chrome bug that jumps to wrong anchor.
                            "&#8203;",

                            NULL)
                        : "",

                    // Add an anchor to the "sandbox" theorem
                    // for use by mmrecent.html
                    (stmt == g_mathboxStmt
                          && passNumber == 1 // Only in summary TOC
                          ) ?
                        // Note the colon so it won't conflict w/ theorem
                        // name anchor.
                        // "&#8203;" is a "zero-width" space to
                        // workaround Chrome bug that jumps to wrong anchor.
                        "<A NAME=\"sandbox:bighdr\"></A>&#8203;" : "",
                    hdrCommentAnchor,
                    "<A HREF=\"",

                    (passNumber == 1) ?
                         cat("#dtl:", str((double)partCntr), ".",
                             str((double)sectionCntr),
                             NULL) // Link to detailed toc
                        : cat(str3, "b", NULL), // Link to thm list

                    "\"><B>",
                    hdrCommentMarker,
                    bigHdr, "</B></A>",
                    "<BR>", NULL),
                    " ",  // Start continuation line with space
                    "\""); // Don't break inside quotes e.g. "Arial Narrow"
                if (passNumber == 2) {
                  // Assign to array for use during theorem list output
                  let((vstring *)(&pntrBigHdr[stmt]), bigHdr);
                  let((vstring *)(&pntrBigHdrComment[stmt]), bigHdrComment);
                }
                free_vstring(bigHdr);
                free_vstring(bigHdrComment);
              }
              if (smallHdr[0]
                  && passNumber == 2) { // Skip in pass 1 (summary)

                // Create subsection number
                subsectionCntr++;
                subsubsectionCntr = 0;
                let(&smallHdr, cat(str((double)partCntr), ".",
                    str((double)sectionCntr),
                    ".", str((double)subsectionCntr), "&nbsp;&nbsp;",
                    smallHdr, NULL));

                // Put an asterisk before the header if the header has a comment
                if (smallHdrComment[0] != 0 && passNumber == 2) {
                  let(&hdrCommentMarker, "*");
                  if (hdrCommentAnchorDone == 0) {
                    let(&hdrCommentAnchor, cat("<A NAME=\"",
                        g_Statement[stmt].labelName, "\"></A>",

                        // "&#8203;" is a "zero-width" space to
                        // workaround Chrome bug that jumps to wrong anchor.
                        "&#8203;",

                        NULL));
                    hdrCommentAnchorDone = 1;
                  } else {
                    let(&hdrCommentAnchor, "");
                  }
                } else {
                  let(&hdrCommentMarker, "");
                  let(&hdrCommentAnchor, "");
                }

                printLongLine(cat("&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; ",
                    hdrCommentAnchor,
                    "<A HREF=\"", str3, "s\">",
                    hdrCommentMarker,
                    smallHdr, "</A>",
                    " &nbsp; <A HREF=\"",
                    g_Statement[stmt].labelName, ".html\">",
                    g_Statement[stmt].labelName, "</A>",
                    str4,
                    "<BR>", NULL),
                    " ", // Start continuation line with space
                    "\""); // Don't break inside quotes e.g. "Arial Narrow"
                // Assign to array for use during theorem output
                let((vstring *)(&pntrSmallHdr[stmt]), smallHdr);
                free_vstring(smallHdr);
                let((vstring *)(&pntrSmallHdrComment[stmt]), smallHdrComment);
                free_vstring(smallHdrComment);
              }

              if (tinyHdr[0] && passNumber == 2) { // Skip in pass 1 (summary)

                // Create subsection number
                subsubsectionCntr++;
                let(&tinyHdr, cat(str((double)partCntr), ".",
                    str((double)sectionCntr),
                    ".", str((double)subsectionCntr),
                    ".", str((double)subsubsectionCntr), "&nbsp;&nbsp;",
                    tinyHdr, NULL));

                // Put an asterisk before the header if the header has a comment
                if (tinyHdrComment[0] != 0 && passNumber == 2) {
                  let(&hdrCommentMarker, "*");
                  if (hdrCommentAnchorDone == 0) {
                    let(&hdrCommentAnchor, cat("<A NAME=\"",
                        g_Statement[stmt].labelName, "\"></A> ",

                        // "&#8203;" is a "zero-width" space to
                        // workaround Chrome bug that jumps to wrong anchor.
                        "&#8203;",

                        NULL));
                    hdrCommentAnchorDone = 1;
                  } else {
                    let(&hdrCommentAnchor, "");
                  }
                } else {
                  let(&hdrCommentMarker, "");
                  let(&hdrCommentAnchor, "");
                }

                printLongLine(cat(
                    "&nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; &nbsp; ",
                    hdrCommentAnchor,
                    "<A HREF=\"", str3, "s\">",
                    hdrCommentMarker,
                    tinyHdr, "</A>",
                    " &nbsp; <A HREF=\"",
                    g_Statement[stmt].labelName, ".html\">",
                    g_Statement[stmt].labelName, "</A>",
                    str4,
                    "<BR>", NULL),
                    " ", // Start continuation line with space
                    "\""); // Don't break inside quotes e.g. "Arial Narrow"
                // Assign to array for use during theorem output
                let((vstring *)(&pntrTinyHdr[stmt]), tinyHdr);
                free_vstring(tinyHdr);
                let((vstring *)(&pntrTinyHdrComment[stmt]), tinyHdrComment);
                free_vstring(tinyHdrComment);
              }

              fprintf(outputFilePtr, "%s", g_printString);
              g_outputToString = 0;
              free_vstring(g_printString);
            } // if huge or big or small or tiny header
          } // if $a or $p
        } // next stmt
        // 8-May-2015 nm Do we need the HR below?
        fprintf(outputFilePtr, "<HR NOSHADE SIZE=1>\n");
      } // next passNumber
    } // if page 0
    // End table of contents

    // Just skip over instead of a big if indent
    if (page == 0) goto SKIP_LIST;

    // Put in color key
    g_outputToString = 1;
    print2("<A NAME=\"mmstmtlst\"></A>\n");
    if (g_extHtmlStmt < g_mathboxStmt) { // g_extHtmlStmt >= g_mathboxStmt in ql.mm
      // ?? Currently this is customized for set.mm only!!
      print2("<P>\n");
      print2("<CENTER><TABLE CELLSPACING=0 CELLPADDING=5\n");
      print2("SUMMARY=\"Color key\"><TR>\n");
      print2("\n");
      print2("<TD>Color key:&nbsp;&nbsp;&nbsp;</TD>\n");
      print2("<TD BGCOLOR=%s NOWRAP><A\n", MINT_BACKGROUND_COLOR);
      print2("HREF=\"mmset.html\"><IMG SRC=\"mm.gif\" BORDER=0\n");
      print2("ALT=\"Metamath Proof Explorer\" HEIGHT=32 WIDTH=32\n");
      print2("ALIGN=MIDDLE> &nbsp;Metamath Proof Explorer</A>\n");

      free_vstring(str3);
      if (g_Statement[g_extHtmlStmt].pinkNumber <= 0) bug(2332);
      str3 = pinkRangeHTML(nmbrStmtNmbr[1],
          nmbrStmtNmbr[g_Statement[g_extHtmlStmt].pinkNumber - 1]);
      printLongLine(cat("<BR>(", str3, ")", NULL),
        " ", // Start continuation line with space
        "\""); // Don't break inside quotes e.g. "Arial Narrow"

      print2("</TD>\n");
      print2("\n");
      print2("<TD WIDTH=10>&nbsp;</TD>\n");
      print2("\n");

      // Hilbert Space Explorer
      print2("<TD BGCOLOR=%s NOWRAP><A\n", PURPLISH_BIBLIO_COLOR);
      print2(" HREF=\"mmhil.html\"><IMG SRC=\"atomic.gif\"\n");
      print2(
 "BORDER=0 ALT=\"Hilbert Space Explorer\" HEIGHT=32 WIDTH=32 ALIGN=MIDDLE>\n");
      print2("&nbsp;Hilbert Space Explorer</A>\n");

      free_vstring(str3);
      if (g_Statement[g_mathboxStmt].pinkNumber <= 0) bug(2333);
      str3 = pinkRangeHTML(g_extHtmlStmt,
         nmbrStmtNmbr[g_Statement[g_mathboxStmt].pinkNumber - 1]);
      printLongLine(cat("<BR>(", str3, ")", NULL),
        " ", // Start continuation line with space
        "\""); // Don't break inside quotes e.g. "Arial Narrow"

      print2("</TD>\n");
      print2("\n");
      print2("<TD WIDTH=10>&nbsp;</TD>\n");
      print2("\n");

      // Mathbox stuff
      print2("<TD BGCOLOR=%s NOWRAP><A\n", SANDBOX_COLOR);
      print2(" HREF=\"mathbox.html\"><IMG SRC=\"_sandbox.gif\"\n");
      print2("BORDER=0 ALT=\"Users' Mathboxes\" HEIGHT=32 WIDTH=32 ALIGN=MIDDLE>\n");
      print2("&nbsp;Users' Mathboxes</A>\n");

      free_vstring(str3);
      str3 = pinkRangeHTML(g_mathboxStmt, nmbrStmtNmbr[assertions]);
      printLongLine(cat("<BR>(", str3, ")", NULL),
        " ", // Start continuation line with space
        "\""); // Don't break inside quotes e.g. "Arial Narrow"

      print2("</TD>\n");
      print2("\n");
      print2("<TD WIDTH=10>&nbsp;</TD>\n");
      print2("\n");

      print2("</TR></TABLE></CENTER>\n");
    } // end if (g_extHtmlStmt < g_mathboxStmt)

    // Write out HTML page so far
    fprintf(outputFilePtr, "%s", g_printString);
    g_outputToString = 0;
    let(&g_printString, "");

    // Write the main table header
    g_outputToString = 1;
    print2("\n");
    print2("<P><CENTER>\n");
    print2("<TABLE BORDER CELLSPACING=0 CELLPADDING=3 BGCOLOR=%s\n",
        MINT_BACKGROUND_COLOR);
    print2("SUMMARY=\"Theorem List for %s\">\n", htmlTitle);
    free_vstring(str3);
    if (page < 1) bug(2335); // Page 0 ToC should have been skipped
    str3 = pinkHTML(nmbrStmtNmbr[(page - 1) * theoremsPerPage + 1]);
    let(&str3, right(str3, (long)strlen(PINK_NBSP) + 1)); // Discard "&nbsp;"
    free_vstring(str4);
    str4 = pinkHTML((page < pages) ?
        nmbrStmtNmbr[page * theoremsPerPage] :
        nmbrStmtNmbr[assertions]);
    let(&str4, right(str4, (long)strlen(PINK_NBSP) + 1)); // Discard "&nbsp;"
    printLongLine(cat("<CAPTION><B>Theorem List for ", htmlTitle,
        " - </B>", str3, "<B>-</B>", str4,
        " &nbsp; *Has distinct variable group(s)"
        "</CAPTION>",NULL),
        " ", // Start continuation line with space
        "\""); // Don't break inside quotes e.g. "Arial Narrow"
    print2("\n");
    print2("<TR><TH>Type</TH><TH>Label</TH><TH>Description</TH></TR>\n");
    print2("<TR><TH COLSPAN=3>Statement</TH></TR>\n");
    print2("\n");
    print2("<TR BGCOLOR=white><TD COLSPAN=3><FONT SIZE=-3>&nbsp;</FONT></TD></TR>\n");
    print2("\n");
    fprintf(outputFilePtr, "%s", g_printString);
    g_outputToString = 0;
    free_vstring(g_printString);

    // Find the last assertion that will be printed on the page, so
    // we will know when a separator between theorems is not needed.
    lastAssertion = 0;
    for (assertion = (page - 1) * theoremsPerPage + 1;
        assertion <= page * theoremsPerPage; assertion++) {
      if (assertion > assertions) break; // We're beyond the end

      lastAssertion = assertion;
    }

    // Output theorems on the page
    for (assertion = (page - 1) * theoremsPerPage + 1;
        assertion <= page * theoremsPerPage; assertion++) {
      if (assertion > assertions) break; // We're beyond the end

      s = nmbrStmtNmbr[assertion]; // Statement number

      // Construct the statement type label
      if (g_Statement[s].type == p_) {
        let(&str1, "Theorem");
      } else if (!strcmp("ax-", left(g_Statement[s].labelName, 3))) {
        let(&str1, "<B><FONT COLOR=red>Axiom</FONT></B>");
      } else if (!strcmp("df-", left(g_Statement[s].labelName, 3))) {
        let(&str1, "<B><FONT COLOR=blue>Definition</FONT></B>");
      } else {
        let(&str1, "<B><FONT COLOR=\"#00CC00\">Syntax</FONT></B>");
      }

      if (s == s + 0) goto skip_date;
      // OBSOLETE
      // Get the date in the comment section after the statement
      let(&str1, space(g_Statement[s + 1].labelSectionLen));
      memcpy(str1, g_Statement[s + 1].labelSectionPtr,
          (size_t)(g_Statement[s + 1].labelSectionLen));
      let(&str1, edit(str1, 2)); // Discard spaces and tabs
      i1 = instr(1, str1, "$([");
      i2 = instr(i1, str1, "]$)");
      if (i1 && i2) {
        let(&str1, seg(str1, i1 + 3, i2 - 1));
      } else {
        let(&str1, "");
      }
     skip_date:

      free_vstring(str3);
      str3 = getDescription(s);
      free_vstring(str4);
      str4 = pinkHTML(s); // Get little pink number
      // Output the description comment
      // Break up long lines for text editors with printLongLine
      free_vstring(g_printString);
      g_outputToString = 1;
      print2("\n"); // Blank line for HTML source human readability

      // Table of contents
      if (((vstring)(pntrHugeHdr[s]))[0]) { // There is a major part break
        printLongLine(cat(
                 // The header
                 "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3",
                 "><CENTER><FONT SIZE=\"+1\"><B>",
                 "<A NAME=\"mm", str((double)(g_Statement[s].pinkNumber)),
                     "h\"></A>", // Anchor for table of contents
                 (vstring)(pntrHugeHdr[s]),
                 "</B></FONT></CENTER>",
                 NULL),
            " ", // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"

        // The comment part of the header, if any
        if (((vstring)(pntrHugeHdrComment[s]))[0]) {

          // Open the table row
          // keep comment in same table cell
          print2("%s\n", "<P STYLE=\"margin-bottom:0em\">");

          // We are currently printing to g_printString to allow use of
          // printLongLine(); however, the rendering function
          // printTexComment uses g_printString internally, so we have to
          // flush the current g_printString and turn off g_outputToString mode
          // in order to call the rendering function printTexComment.
          // (Question:  why do the calls to printTexComment for statement
          // descriptions, later, not need to flush the g_printString?  Is the
          // flushing code here redundant?)
          // Clear out the g_printString output in prep for printTexComment
          g_outputToString = 0;
          fprintf(outputFilePtr, "%s", g_printString);
          let(&g_printString, "");
          g_showStatement = s; // For printTexComment
          g_texFilePtr = outputFilePtr; // For printTexComment
          printTexComment( // Sends result to g_texFilePtr
              (vstring)(pntrHugeHdrComment[s]),
              0, // 1 = htmlCenterFlag
              PROCESS_EVERYTHING, // actionBits
              1); // 1 = fileCheck
          g_texFilePtr = NULL;
          g_outputToString = 1; // Restore after printTexComment
        }

        // Close the table row
        print2("%s\n", "</TD></TR>");

        printLongLine(cat(
                 // Separator row
                 "<TR BGCOLOR=white><TD COLSPAN=3>",
                 "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>",
                 NULL),
            " ", // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"
      }
      if (((vstring)(pntrBigHdr[s]))[0]) { // There is a major section break
        printLongLine(cat(
                 // The header
                 "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3",
                 "><CENTER><FONT SIZE=\"+1\"><B>",
                 "<A NAME=\"mm", str((double)(g_Statement[s].pinkNumber)),
                     "b\"></A>", // Anchor for table of contents
                 (vstring)(pntrBigHdr[s]),
                 "</B></FONT></CENTER>",
                 NULL),
            " ", // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"

        // The comment part of the header, if any
        if (((vstring)(pntrBigHdrComment[s]))[0]) {

          // Open the table row.
          // keep comment in same table cell
          print2("%s\n", "<P STYLE=\"margin-bottom:0em\">");

          // We are currently printing to g_printString to allow use of
          // printLongLine(); however, the rendering function
          // printTexComment uses g_printString internally, so we have to
          // flush the current g_printString and turn off g_outputToString mode
          // in order to call the rendering function printTexComment.
          // (Question:  why do the calls to printTexComment for statement
          // descriptions, later, not need to flush the g_printString?  Is the
          // flushing code here redundant?)
          // Clear out the g_printString output in prep for printTexComment
          g_outputToString = 0;
          fprintf(outputFilePtr, "%s", g_printString);
          free_vstring(g_printString);
          g_showStatement = s; // For printTexComment
          g_texFilePtr = outputFilePtr; // For printTexComment
          printTexComment( // Sends result to g_texFilePtr
              (vstring)(pntrBigHdrComment[s]),
              0, // 1 = htmlCenterFlag
              PROCESS_EVERYTHING, // actionBits
              1); // 1 = fileCheck
          g_texFilePtr = NULL;
          g_outputToString = 1; // Restore after printTexComment
        }

        // Close the table row
        print2("%s\n", "</TD></TR>");

        printLongLine(cat(
                 // Separator row
                 "<TR BGCOLOR=white><TD COLSPAN=3>",
                 "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>",
                 NULL),
            " ",  // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"
      }
      if (((vstring)(pntrSmallHdr[s]))[0]) { // There is a minor sec break
        printLongLine(cat(
                 // The header
                 "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3",
                 "><CENTER><B>",
                 "<A NAME=\"mm", str((double)(g_Statement[s].pinkNumber)),
                     "s\"></A>", // Anchor for table of contents
                 (vstring)(pntrSmallHdr[s]),
                 "</B></CENTER>",
                 NULL),
            " ",  // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"

        // The comment part of the header, if any
        if (((vstring)(pntrSmallHdrComment[s]))[0]) {

          // Open the table row
          // keep comment in same table cell
          print2("%s\n", "<P STYLE=\"margin-bottom:0em\">");

          // We are currently printing to g_printString to allow use of
          // printLongLine(); however, the rendering function
          // printTexComment uses g_printString internally, so we have to
          // flush the current g_printString and turn off g_outputToString mode
          // in order to call the rendering function printTexComment.
          // (Question:  why do the calls to printTexComment for statement
          // descriptions, later, not need to flush the g_printString?  Is the
          // flushing code here redundant?)
          // Clear out the g_printString output in prep for printTexComment
          g_outputToString = 0;
          fprintf(outputFilePtr, "%s", g_printString);
          free_vstring(g_printString);
          g_showStatement = s; // For printTexComment
          g_texFilePtr = outputFilePtr; // For printTexComment
          printTexComment( // Sends result to g_texFilePtr
              (vstring)(pntrSmallHdrComment[s]),
              0, // 1 = htmlCenterFlag
              PROCESS_EVERYTHING, // actionBits
              1); // 1 = fileCheck
          g_texFilePtr = NULL;
          g_outputToString = 1; // Restore after printTexComment
        }

        // Close the table row
        print2("%s\n", "</TD></TR>");

        printLongLine(cat(
                 // Separator row
                 "<TR BGCOLOR=white><TD COLSPAN=3>",
                 "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>",
                 NULL),
            " ", // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"
      }

      if (((vstring)(pntrTinyHdr[s]))[0]) { // There is a subsubsection break
        printLongLine(cat(
                 // The header
                 "<TR BGCOLOR=\"#FFFFF2\"><TD COLSPAN=3",
                 "><CENTER><B>",
                 "<A NAME=\"mm", str((double)(g_Statement[s].pinkNumber)),
                     "s\"></A>", // Anchor for table of contents
                 (vstring)(pntrTinyHdr[s]),
                 "</B></CENTER>",
                 NULL),
            " ", // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"

        // The comment part of the header, if any
        if (((vstring)(pntrTinyHdrComment[s]))[0]) {

          // Open the table row
          // keep comment in same table cell
          print2("%s\n", "<P STYLE=\"margin-bottom:0em\">");

          // We are currently printing to g_printString to allow use of
          // printLongLine(); however, the rendering function
          // printTexComment uses g_printString internally, so we have to
          // flush the current g_printString and turn off g_outputToString mode
          // in order to call the rendering function printTexComment.
          // (Question:  why do the calls to printTexComment for statement
          // descriptions, later, not need to flush the g_printString?  Is the
          // flushing code here redundant?)
          // Clear out the g_printString output in prep for printTexComment
          g_outputToString = 0;
          fprintf(outputFilePtr, "%s", g_printString);
          free_vstring(g_printString);
          g_showStatement = s; // For printTexComment
          g_texFilePtr = outputFilePtr; // For printTexComment
          printTexComment( // Sends result to g_texFilePtr
              (vstring)(pntrTinyHdrComment[s]),
              0, // 1 = htmlCenterFlag
              PROCESS_EVERYTHING, // actionBits
              1); // 1 = fileCheck
          g_texFilePtr = NULL;
          g_outputToString = 1; // Restore after printTexComment
        }

        // Close the table row
        print2("%s\n", "</TD></TR>");

        printLongLine(cat(
                 // Separator row
                 "<TR BGCOLOR=white><TD COLSPAN=3>",
                 "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>",
                 NULL),
            " ",  // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"
      }

      printLongLine(cat(
            (s < g_extHtmlStmt)
               ? "<TR>"
               : (s < g_mathboxStmt)
                   ? cat("<TR BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL)
                   : cat("<TR BGCOLOR=", SANDBOX_COLOR, ">", NULL),
            "<TD NOWRAP>", // IE breaks up the date
            str1, // Date
            "</TD><TD ALIGN=CENTER><A HREF=\"",
            g_Statement[s].labelName, ".html\">",
            g_Statement[s].labelName, "</A>",
            str4,

            // Add asterisk if statement has distinct var groups
            (nmbrLen(g_Statement[s].reqDisjVarsA) > 0) ? "*" : "",

            "</TD><TD ALIGN=LEFT>",
            // Add anchor for hyperlinking to the table row
            "<A NAME=\"", g_Statement[s].labelName, "\"></A>",

            NULL), // Description
          " ",  // Start continuation line with space
          "\""); // Don't break inside quotes e.g. "Arial Narrow"

      g_showStatement = s; // For printTexComment
      g_outputToString = 0; // For printTexComment
      g_texFilePtr = outputFilePtr; // For printTexComment
      printTexComment( // Sends result to g_texFilePtr
          str3,
          0, // 1 = htmlCenterFlag
          PROCESS_EVERYTHING, // actionBits
          1); // 1 = fileCheck
      g_texFilePtr = NULL;
      g_outputToString = 1; // Restore after printTexComment

      // Get HTML hypotheses => assertion
      free_vstring(str4);
      str4 = getTexOrHtmlHypAndAssertion(s); // In mmwtex.c

      // Suppress the math content of lemmas, which can
      // be very big and not interesting.
      if (!strcmp(left(str3, 10), "Lemma for ") && !showLemmas) {
        // Suppress the table row with the math content
        print2(" <I>[Auxiliary lemma - not displayed.]</I></TD></TR>\n");
      } else {
        // Output the table row with the math content
        printLongLine(cat("</TD></TR><TR",
              (s < g_extHtmlStmt)
                ? ">"
                : (s < g_mathboxStmt)
                  ? cat(" BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL)
                  : cat(" BGCOLOR=", SANDBOX_COLOR, ">", NULL),
              "<TD COLSPAN=3 ALIGN=CENTER>",
              str4, "</TD></TR>", NULL),
            " ", // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"
      }

      g_outputToString = 0;
      fprintf(outputFilePtr, "%s", g_printString);
      free_vstring(g_printString);

      if (assertion != lastAssertion) {
        // Put separator row if not last theorem
        g_outputToString = 1;
        printLongLine(cat("<TR BGCOLOR=white><TD COLSPAN=3>",
            "<FONT SIZE=-3>&nbsp;</FONT></TD></TR>", NULL),
            " ", // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"
        g_outputToString = 0;
        fprintf(outputFilePtr, "%s", g_printString);
        free_vstring(g_printString);
      }
    } // next assertion

    // Output trailer
    g_outputToString = 1;
    print2("</TABLE></CENTER>\n");
    print2("\n");

   SKIP_LIST: // (skipped when page == 0)
    g_outputToString = 1; // To compensate for skipped assignment above

    // Put extra Prev/Next hyperlinks here for convenience
    print2("<TABLE BORDER=0 WIDTH=\"100%c\">\n", '%');
    print2("  <TR>\n");
    print2("    <TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%c\">\n", '%');
    print2("      &nbsp;\n");
    print2("    </TD>\n");
    print2("    <TD NOWRAP ALIGN=CENTER>&nbsp;</TD>\n");
    print2("    <TD NOWRAP ALIGN=RIGHT VALIGN=TOP WIDTH=\"25%c\"><FONT\n", '%');
    print2("      SIZE=-1 FACE=sans-serif>\n");
    printLongLine(cat("      ", prevNextLinks, NULL),
        " ", // Start continuation line with space
        "\""); // Don't break inside quotes e.g. "Arial Narrow"
    print2("      </FONT></TD>\n");
    print2("  </TR>\n");
    print2("</TABLE>\n");

    print2("<HR NOSHADE SIZE=1>\n");

    g_outputToString = 0;
    fprintf(outputFilePtr, "%s", g_printString);
    free_vstring(g_printString);

    fprintf(outputFilePtr, "<A NAME=\"mmpglst\"></A>\n");
    fprintf(outputFilePtr, "<CENTER>\n");
    fprintf(outputFilePtr, "<B>Page List</B>\n");
    fprintf(outputFilePtr, "</CENTER>\n");

    // Output links to the other pages
    fprintf(outputFilePtr, "Jump to page: \n");
    for (p = 0; p <= pages; p++) {

      // Construct the pink number range
      free_vstring(str3);
      if (p > 0) {
        str3 = pinkRangeHTML(
            nmbrStmtNmbr[(p - 1) * theoremsPerPage + 1],
            (p < pages) ?
              nmbrStmtNmbr[p * theoremsPerPage] :
              nmbrStmtNmbr[assertions]);
      }

      if (p == page) {
        // Current page shouldn't have link to self
        let(&str1, (p == 0) ? "Contents" : str((double)p));
      } else {
        let(&str1, cat("<A HREF=\"mmtheorems",
            (p == 0) ? "" : str((double)p),

            // Friendlier, because you can start
            // scrolling through the page before it finishes loading,
            // without its jumping to #mmtc (start of Table of Contents)
            // when it's done.
            // (p == 1) ? ".html#mmtc\">" : ".html\">",
            ".html\">",

            (p == 0) ? "Contents" : str((double)p),
            "</A>", NULL));
      }
      let(&str1, cat(str1, PINK_NBSP, str3, NULL));
      fprintf(outputFilePtr, "%s\n", str1);
    }

    g_outputToString = 1;
    print2("<HR NOSHADE SIZE=1>\n");
    print2("<TABLE BORDER=0 WIDTH=\"100%c\">\n", '%');
    print2("  <TR>\n");
    print2("    <TD ALIGN=LEFT VALIGN=TOP WIDTH=\"25%c\">\n", '%');
    print2("      &nbsp;\n");
    print2("    </TD>\n");
    print2("    <TD NOWRAP ALIGN=CENTER><FONT SIZE=-2\n");
    print2("      FACE=ARIAL>\n");
    print2("Copyright terms:\n");
    print2("<A HREF=\"../copyright.html#pd\">Public domain</A>\n");
    print2("      </FONT></TD>\n");
    print2("    <TD NOWRAP ALIGN=RIGHT VALIGN=TOP WIDTH=\"25%c\"><FONT\n", '%');
    print2("      SIZE=-1 FACE=sans-serif>\n");
    printLongLine(cat("      ", prevNextLinks, NULL),
        " ",  // Start continuation line with space
        "\""); // Don't break inside quotes e.g. "Arial Narrow"
    print2("      </FONT></TD>\n");
    print2("  </TR>\n");
    print2("</TABLE>\n");
    print2("</BODY></HTML>\n");
    g_outputToString = 0;
    fprintf(outputFilePtr, "%s", g_printString);
    free_vstring(g_printString);

    // Close file
    fclose(outputFilePtr);
  } // next page

 TL_ABORT:
  // Deallocate memory
  free_vstring(str1);
  free_vstring(str3);
  free_vstring(str4);
  free_vstring(prevNextLinks);
  free_vstring(outputFileName);
  free_vstring(hugeHdr);
  free_vstring(bigHdr);
  free_vstring(smallHdr);
  free_vstring(tinyHdr);
  free_vstring(hdrCommentMarker);
  for (i = 0; i <= g_statements; i++) free_vstring(*(vstring *)(&pntrHugeHdr[i]));
  free_pntrString(pntrHugeHdr);
  for (i = 0; i <= g_statements; i++) free_vstring(*(vstring *)(&pntrBigHdr[i]));
  free_pntrString(pntrBigHdr);
  for (i = 0; i <= g_statements; i++) free_vstring(*(vstring *)(&pntrSmallHdr[i]));
  free_pntrString(pntrSmallHdr);
  for (i = 0; i <= g_statements; i++) free_vstring(*(vstring *)(&pntrTinyHdr[i]));
  free_pntrString(pntrTinyHdr);
} // writeTheoremList

// This function extracts any section headers in the comment sections
// prior to the label of statement stmt.   If a huge (####...) header isn't
// found, *hugeHdrTitle will be set to the empty string.
// If a big (#*#*...) header isn't found (or isn't after the last huge header),
// *bigHdrTitle will be set to the empty string.  If a small
// (=-=-...) header isn't found (or isn't after the last huge header or
// the last big header), *smallHdrTitle will be set to the empty string.
// In all 3 cases, only the last occurrence of a header is considered.
// If a tiny
// (-.-....) header isn't found (or isn't after the last huge header or
// the last big header or the last small header), *tinyHdrTitle will be
// set to the empty string.
// In all 4 cases, only the last occurrence of a header is considered.

// 20-Jun-2015 metamath Google group email:
// https://groups.google.com/g/metamath/c/QE0lwg9f5Ho/m/J8ekl_lH1I8J
//
// There are 4 kinds of section headers, big (####...), medium (#*#*#*...),
// small (=-=-=-), and tiny (-.-.-.).
//
// Call the collection of (outside-of-statement) comments between two
// successive $a/$p statements (i.e. those statements that generate web
// pages) a "header area".  The algorithm scans the header area for the
// _last_ header of each type (big, medium, small, tiny) and discards all
// others. Then, if there is a medium and it appears before a big, the
// medium is discarded.  If there is a small and it appears before a big or
// medium, the small is discarded.  In other words, a maximum of one header
// of each type is kept, in the order big, medium, small, and tiny.
//
// There are two reasons for doing this:  (1) it disregards headers used
// for other purposes such as the headers for the set.mm-specific
// information at the top that is not of general interest and (2) it
// ignores headers for empty sections; for example, a mathbox user might
// have a bunch of headers for sections planned for the future, but we
// ignore them if those sections are empty (no $a or $p in them).

// Return 1 if error found, 0 otherwise
flag getSectionHeadings(long stmt,
    vstring *hugeHdrTitle,
    vstring *bigHdrTitle,
    vstring *smallHdrTitle,
    vstring *tinyHdrTitle,
    vstring *hugeHdrComment,
    vstring *bigHdrComment,
    vstring *smallHdrComment,
    vstring *tinyHdrComment,
    flag fineResolution,
    flag fullComment) {

  // for table of contents
  vstring_def(labelStr);
  long pos, pos1, pos2, pos3, pos4;
  flag errorFound = 0;
  flag saveOutputToString;

  // Print any error messages to screen
  saveOutputToString = g_outputToString; // To restore when returning
  g_outputToString = 0;

  // (This initialization seems to be done redundantly by caller elsewhere,
  // but for  WRITE SOURCE ... / EXTRACT we need to do it explicitly.)
  free_vstring(*hugeHdrTitle);
  free_vstring(*bigHdrTitle);
  free_vstring(*smallHdrTitle);
  free_vstring(*tinyHdrTitle);
  free_vstring(*hugeHdrComment);
  free_vstring(*bigHdrComment);
  free_vstring(*smallHdrComment);
  free_vstring(*tinyHdrComment);

  // We now process only $a or $p statements
  if (fineResolution == 0) {
    if (g_Statement[stmt].type != a_ && g_Statement[stmt].type != p_) bug(2340);
  }

  // Get header area between this statement and the statement after the
  // previous $a or $p statement.
  // pos3 and pos4 are used temporarily here; not related to later use
  if (fineResolution == 0) {
    // Statement immediately after the previous $a or $p statement
    // (will be this statement if previous statement is $a or $p).
    pos3 = g_Statement[stmt].headerStartStmt;
  } else {
    // For WRITE SOURCE ... / EXTRACT, we want every statement treated equally
    pos3 = stmt;
  }
  if (pos3 == 0 || pos3 > stmt) bug(2241);
  pos4 = (g_Statement[stmt].labelSectionPtr
        - g_Statement[pos3].labelSectionPtr)
        + g_Statement[stmt].labelSectionLen; // Length of the header area
  let(&labelStr, space(pos4));
  memcpy(labelStr, g_Statement[pos3].labelSectionPtr,
      (size_t)(pos4));

  pos = 0;
  pos2 = 0;
  while (1) { // Find last "huge" header, if any
    // 4-Nov-2007 nm:  Obviously, the match below will not work if the
    // $( line has a trailing space, which some editors might insert.
    // The symptom is a missing table of contents entry.  But to detect
    // this (and for the #*#* and =-=- matches below) would take a little work
    // and perhaps slow things down, and I don't think it is worth it.  I
    // put a note in HELP WRITE THEOREM_LIST.
    pos1 = pos;
    pos = instr(pos + 1, labelStr, "$(\n" HUGE_DECORATION);

    // 23-May-2008 nm Tolerate one space after "$(", to handle case of
    // one space added to the end of each line with TOOLS to make global
    // label changes are easier (still a kludge; this should be made
    // white-space insensitive some day).
    pos1 = instr(pos1 + 1, labelStr, "$( \n" HUGE_DECORATION);
    if (pos1 > pos) pos = pos1;

    if (!pos) break;
    if (pos) pos2 = pos;
  } // while(1)
  if (pos2) { // Extract "huge" header
    pos1 = pos2; // Save "$(" position
    pos = instr(pos2 + 4, labelStr, "\n"); // Get to end of #### line
    pos2 = instr(pos + 1, labelStr, "\n"); // Find end of title line

    // Error check - can't have more than 1 title line
    if (strcmp(mid(labelStr, pos2 + 1, 4), HUGE_DECORATION)) {
      print2(
       "?Warning: missing closing \"%s\" decoration above statement \"%s\".\n",
          HUGE_DECORATION, g_Statement[stmt].labelName);
      errorFound = 1;
    }

    pos3 = instr(pos2 + 1, labelStr, "\n"); // Get to end of 2nd #### line
    while (labelStr[(pos3 - 1) + 1] == '\n') pos3++; // Skip 1st blank lines
    pos4 = instr(pos3, labelStr, "$)"); // Get to end of title comment
    if (fullComment == 0) {
      let(&(*hugeHdrTitle), seg(labelStr, pos + 1, pos2 - 1));
      // Trim leading, trailing sp
      let(&(*hugeHdrTitle), edit((*hugeHdrTitle), 8 + 128));
      let(&(*hugeHdrComment), seg(labelStr, pos3 + 1, pos4 - 2));
      // Trim leading sp, trailing sp & lf
      let(&(*hugeHdrComment), edit((*hugeHdrComment), 8 + 16384));
    } else {
      // Put entire comment in hugeHdrTitle and hugeHdrComment for /EXTRACT
      // Search backwards for non-space or beginning of string:
      pos = pos1; // pos1 is the "$" in "$("
      pos--;
      while (pos > 0) {
        if (labelStr[pos - 1] != ' '
              && labelStr[pos - 1] != '\n') break;
        pos--;
      }
      // pos + 1 is the start of whitespace preceding "$("
      // pos4 is the "$" in "$)"
      // pos3 is the \n after the 2nd decoration line
      let(&(*hugeHdrTitle), seg(labelStr, pos + 1, pos3));
      let(&(*hugeHdrComment), seg(labelStr, pos3 + 1, pos4 + 1));
    }
  }
  // pos = 0; // Leave pos alone so that we start with "huge" header pos,
  // to ignore any earlier "tiny" or "small" or "big" header.
  pos2 = 0;
  while (1) { // Find last "big" header, if any
    pos1 = pos;
    pos = instr(pos + 1, labelStr, "$(\n" BIG_DECORATION);
    pos1 = instr(pos1 + 1, labelStr, "$( \n" BIG_DECORATION);
    if (pos1 > pos) pos = pos1;

    if (!pos) break;
    if (pos) pos2 = pos;
  }
  if (pos2) { // Extract "big" header
    pos1 = pos2; // Save "$(" position
    pos = instr(pos2 + 4, labelStr, "\n"); // Get to end of #*#* line
    pos2 = instr(pos + 1, labelStr, "\n"); // Find end of title line

    // Error check - can't have more than 1 title line
    if (strcmp(mid(labelStr, pos2 + 1, 4), BIG_DECORATION)) {
      print2(
       "?Warning: missing closing \"%s\" decoration above statement \"%s\".\n",
          BIG_DECORATION, g_Statement[stmt].labelName);
      errorFound = 1;
    }

    pos3 = instr(pos2 + 1, labelStr, "\n"); // Get to end of 2nd #*#* line
    while (labelStr[(pos3 - 1) + 1] == '\n') pos3++; // Skip 1st blank lines
    pos4 = instr(pos3, labelStr, "$)"); // Get to end of title comment
    if (fullComment == 0) {
      let(&(*bigHdrTitle), seg(labelStr, pos + 1, pos2 - 1));
      // Trim leading, trailing sp
      let(&(*bigHdrTitle), edit((*bigHdrTitle), 8 + 128));
      let(&(*bigHdrComment), seg(labelStr, pos3 + 1, pos4 - 2));
      // Trim leading sp, trailing sp & lf
      let(&(*bigHdrComment), edit((*bigHdrComment), 8 + 16384));
    } else {
      // Put entire comment in bigHdrTitle and bigHdrComment for /EXTRACT
      // Search backwards for non-space or beginning of string:
      pos = pos1; // pos1 is the "$" in "$("
      pos--;
      while (pos > 0) {
        if (labelStr[pos - 1] != ' '
              && labelStr[pos - 1] != '\n') break;
        pos--;
      }
      // pos + 1 is the start of whitespace preceding "$("
      // pos4 is the "$" in "$)"
      // pos3 is the \n after the 2nd decoration line
      let(&(*bigHdrTitle), seg(labelStr, pos + 1, pos3));
      let(&(*bigHdrComment), seg(labelStr, pos3 + 1, pos4 + 1));
    }
  }
  // pos = 0; // Leave pos alone so that we start with "big" header pos,
  // to ignore any earlier "tiny" or "small" header
  pos2 = 0;
  while (1) { // Find last "small" header, if any
    pos1 = pos;
    pos = instr(pos + 1, labelStr, "$(\n" SMALL_DECORATION);
    pos1 = instr(pos1 + 1, labelStr, "$( \n" SMALL_DECORATION);
    if (pos1 > pos) pos = pos1;

    if (!pos) break;
    if (pos) pos2 = pos;
  }
  if (pos2) { // Extract "small" header
    pos1 = pos2; // Save "$(" position
    pos = instr(pos2 + 4, labelStr, "\n"); // Get to end of =-=- line
    pos2 = instr(pos + 1, labelStr, "\n"); // Find end of title line

    // Error check - can't have more than 1 title line
    if (strcmp(mid(labelStr, pos2 + 1, 4), SMALL_DECORATION)) {
      print2(
       "?Warning: missing closing \"%s\" decoration above statement \"%s\".\n",
          SMALL_DECORATION, g_Statement[stmt].labelName);
      errorFound = 1;
    }

    pos3 = instr(pos2 + 1, labelStr, "\n"); // Get to end of 2nd =-=- line
    while (labelStr[(pos3 - 1) + 1] == '\n') pos3++; // Skip 1st blank lines
    pos4 = instr(pos3, labelStr, "$)"); // Get to end of title comment
    if (fullComment == 0) {
      let(&(*smallHdrTitle), seg(labelStr, pos + 1, pos2 - 1));
      // Trim leading, trailing sp
      let(&(*smallHdrTitle), edit((*smallHdrTitle), 8 + 128));
      let(&(*smallHdrComment), seg(labelStr, pos3 + 1, pos4 - 2));
      // Trim leading sp, trailing sp & lf
      let(&(*smallHdrComment), edit((*smallHdrComment), 8 + 16384));
    } else {
      // Put entire comment in smallHdrTitle and smallHdrComment for /EXTRACT
      // Search backwards for non-space or beginning of string:
      pos = pos1; // pos1 is the "$" in "$("
      pos--;
      while (pos > 0) {
        if (labelStr[pos - 1] != ' '
              && labelStr[pos - 1] != '\n') break;
        pos--;
      }
      // pos + 1 is the start of whitespace preceding "$("
      // pos4 is the "$" in "$)"
      // pos3 is the \n after the 2nd decoration line
      let(&(*smallHdrTitle), seg(labelStr, pos + 1, pos3));
      let(&(*smallHdrComment), seg(labelStr, pos3 + 1, pos4 + 1));
    }
  }

  // pos = 0; // Leave pos alone so that we start with "small" header pos,
  // to ignore any earlier "tiny" header.
  pos2 = 0;
  while (1) { // Find last "tiny" header, if any
    pos1 = pos;
    pos = instr(pos + 1, labelStr, "$(\n" TINY_DECORATION);
    pos1 = instr(pos1 + 1, labelStr, "$( \n" TINY_DECORATION);
    if (pos1 > pos) pos = pos1;

    if (!pos) break;
    if (pos) pos2 = pos;
  }
  if (pos2) { // Extract "tiny" header
    pos1 = pos2; // Save "$(" position
    pos = instr(pos2 + 4, labelStr, "\n"); // Get to end of -.-. line
    pos2 = instr(pos + 1, labelStr, "\n"); // Find end of title line

    // Error check - can't have more than 1 title line
    if (strcmp(mid(labelStr, pos2 + 1, 4), TINY_DECORATION)) {
      print2(
       "?Warning: missing closing \"%s\" decoration above statement \"%s\".\n",
          TINY_DECORATION, g_Statement[stmt].labelName);
      errorFound = 1;
    }

    pos3 = instr(pos2 + 1, labelStr, "\n"); // Get to end of 2nd -.-. line
    while (labelStr[(pos3 - 1) + 1] == '\n') pos3++; // Skip 1st blank lines
    pos4 = instr(pos3, labelStr, "$)"); // Get to end of title comment
    if (fullComment == 0) {
      let(&(*tinyHdrTitle), seg(labelStr, pos + 1, pos2 - 1));
      // Trim leading, trailing sp
      let(&(*tinyHdrTitle), edit((*tinyHdrTitle), 8 + 128));
      let(&(*tinyHdrComment), seg(labelStr, pos3 + 1, pos4 - 2));
      // Trim leading sp, trailing sp & lf
      let(&(*tinyHdrComment), edit((*tinyHdrComment), 8 + 16384));
    } else {
      // Put entire comment in tinyHdrTitle and tinyHdrComment for /EXTRACT
      // Search backwards for non-space or beginning of string:
      pos = pos1; // pos1 is the "$" in "$("
      pos--;
      while (pos > 0) {
        if (labelStr[pos - 1] != ' '
              && labelStr[pos - 1] != '\n') break;
        pos--;
      }
      // pos + 1 is the start of whitespace preceding "$("
      // pos4 is the "$" in "$)"
      // pos3 is the \n after the 2nd decoration line
      let(&(*tinyHdrTitle), seg(labelStr, pos + 1, pos3));
      let(&(*tinyHdrComment), seg(labelStr, pos3 + 1, pos4 + 1));
    }
  }

  if (errorFound == 1) {
    print2("  (Note that section titles may not be longer than one line.)\n");
  }
  // Restore output stream
  g_outputToString = saveOutputToString;

  free_vstring(labelStr); // Deallocate string memory
  return errorFound;
} // getSectionHeadings

// Returns HTML for the pink number to print after the statement labels
// in HTML output.  (Note that "pink" means "rainbow colored" number now.)
// Warning: The caller must deallocate the returned vstring (i.e. this
// function cannot be used in let statements but must be assigned to
// a local vstring for local deallocation).
vstring pinkHTML(long statemNum)
{
  long statemMap;
  vstring_def(htmlCode);
  vstring_def(hexValue);

  if (statemNum > 0) {
    statemMap = g_Statement[statemNum].pinkNumber;
  } else {
    // -1 means the label wasn't found
    statemMap = -1;
  }

  // Note: we put "(future)" when the label wasn't found (an error message
  // was also generated previously).

  // With style sheet and explicit color
  free_vstring(hexValue);
  hexValue = spectrumToRGB(statemMap, g_Statement[g_statements].pinkNumber);
  let(&htmlCode, cat(PINK_NBSP,
      "<SPAN CLASS=r STYLE=\"color:#", hexValue, "\">",
      (statemMap != -1) ? str((double)statemMap) : "(future)", "</SPAN>", NULL));
  free_vstring(hexValue);

  return htmlCode;
} // pinkHTML

// Returns HTML for a range of pink numbers separated by a "-".
// Warning: The caller must deallocate the returned vstring (i.e. this
// function cannot be used in let statements but must be assigned to
// a local vstring for local deallocation).
vstring pinkRangeHTML(long statemNum1, long statemNum2) {
  vstring_def(htmlCode);
  vstring_def(str3);
  vstring_def(str4);

  // Construct the HTML for a pink number range
  free_vstring(str3);
  str3 = pinkHTML(statemNum1);
  let(&str3, right(str3, (long)strlen(PINK_NBSP) + 1)); // Discard "&nbsp;"
  free_vstring(str4);
  str4 = pinkHTML(statemNum2);
  let(&str4, right(str4, (long)strlen(PINK_NBSP) + 1)); // Discard "&nbsp;"
  let(&htmlCode, cat(str3, "-", str4, NULL));
  free_vstring(str3); // Deallocate
  free_vstring(str4); // Deallocate
  return htmlCode;
} // pinkRangeHTML

// This function converts a "spectrum" color (1 to maxColor) to an
// RBG value in hex notation for HTML.  The caller must deallocate the
// returned vstring to prevent memory leaks.  color = 1 (red) to maxColor
// (violet).  A special case is the color -1, which just returns black.
vstring spectrumToRGB(long color, long maxColor) {
  vstring_def(str1);
  double fraction, fractionInPartition;
  long j, red, green, blue, partition;
// Change PARTITIONS whenever the table below has entries added or removed!
#define PARTITIONS 28
  static double redRef[PARTITIONS +  1];   // Made these
  static double greenRef[PARTITIONS +  1]; // static for
  static double blueRef[PARTITIONS +  1];  // speedup
  static long i = -1;                      // below

  if (i > -1) goto SKIP_INIT;
  i = -1; // For safety

  // Here, we use the maximum saturation possible for a fixed L*a*b color L
  // (lightness) value of 53, which corresponds to 50% gray scale.
  // Each pure color had either brightness reduced or saturation reduced,
  // as required, to achieve L = 53.
  //
  // An initial partition was formed from hues 0, 15, 30, ..., 345, then the
  // result was divided into 1000 subpartitions, and the new partitions were
  // determined by reselecting partition boundaries based on where their color
  // difference was distinguishable (i.e. could semi-comfortably read letters
  // of one color with the other as a background, on an LCD display).  Some
  // human judgment was involved, and it is probably not completely uniform or
  // optimal.
  //
  // A Just Noticeable Difference (JND) algorithm for spacing might be more
  // accurate, especially if averaged over several subjects and different
  // monitors.  I wrote a program for that - asking the user to identify a word
  // in one hue with an adjacent hue as a background, in order to score a
  // point - but it was taking too much time, and I decided life is too short.
  // I think this is "good enough" though, perhaps not even noticeably
  // non-optimum.
  //
  // The comment at the end of each line is the hue 0-360 mapped linearly
  // to 1-1043 (345 = 1000, i.e. "extreme" purple or almost red).  Partitions
  // at the end can be commented out if we want to stop at violet instead of
  // almost wrapping around to red via the purples, in order to more accurately
  // emulate the color spectrum.  Be sure to update the PARTITIONS constant
  // above if a partition is commented out, to avoid a bug trap.
  i++; redRef[i] = 251; greenRef[i] = 0; blueRef[i] = 0;       // 1
  i++; redRef[i] = 247; greenRef[i] = 12; blueRef[i] = 0;      // 10
  i++; redRef[i] = 238; greenRef[i] = 44; blueRef[i] = 0;      // 34
  i++; redRef[i] = 222; greenRef[i] = 71; blueRef[i] = 0;      // 58
  i++; redRef[i] = 203; greenRef[i] = 89; blueRef[i] = 0;      // 79
  i++; redRef[i] = 178; greenRef[i] = 108; blueRef[i] = 0;     // 109
  i++; redRef[i] = 154; greenRef[i] = 122; blueRef[i] = 0;     // 140
  i++; redRef[i] = 127; greenRef[i] = 131; blueRef[i] = 0;     // 181
  i++; redRef[i] = 110; greenRef[i] = 136; blueRef[i] = 0;     // 208
  i++; redRef[i] = 86; greenRef[i] = 141; blueRef[i] = 0;      // 242
  i++; redRef[i] = 60; greenRef[i] = 144; blueRef[i] = 0;      // 276
  i++; redRef[i] = 30; greenRef[i] = 147; blueRef[i] = 0;      // 313
  i++; redRef[i] = 0; greenRef[i] = 148; blueRef[i] = 22;      // 375
  i++; redRef[i] = 0; greenRef[i] = 145; blueRef[i] = 61;      // 422
  i++; redRef[i] = 0; greenRef[i] = 145; blueRef[i] = 94;      // 462
  i++; redRef[i] = 0; greenRef[i] = 143; blueRef[i] = 127;     // 504
  i++; redRef[i] = 0; greenRef[i] = 140; blueRef[i] = 164;     // 545
  i++; redRef[i] = 0; greenRef[i] = 133; blueRef[i] = 218;     // 587
  i++; redRef[i] = 3; greenRef[i] = 127; blueRef[i] = 255;     // 612
  i++; redRef[i] = 71; greenRef[i] = 119; blueRef[i] = 255;    // 652
  i++; redRef[i] = 110; greenRef[i] = 109; blueRef[i] = 255;   // 698
  i++; redRef[i] = 137; greenRef[i] = 99; blueRef[i] = 255;    // 740
  i++; redRef[i] = 169; greenRef[i] = 78; blueRef[i] = 255;    // 786
  i++; redRef[i] = 186; greenRef[i] = 57; blueRef[i] = 255;    // 808
  i++; redRef[i] = 204; greenRef[i] = 33; blueRef[i] = 249;    // 834
  i++; redRef[i] = 213; greenRef[i] = 16; blueRef[i] = 235;    // 853
  i++; redRef[i] = 221; greenRef[i] = 0; blueRef[i] = 222;     // 870
  i++; redRef[i] = 233; greenRef[i] = 0; blueRef[i] = 172;     // 916
  i++; redRef[i] = 239; greenRef[i] = 0; blueRef[i] = 132;     // 948
  // i++; redRef[i] = 242; greenRef[i] = 0; blueRef[i] = 98;   // 973
  // *i++; redRef[i] = 244; greenRef[i] = 0; blueRef[i] = 62;  // 1000

  if (i != PARTITIONS) { // Double-check future edits
    print2("? %ld partitions but PARTITIONS = %ld\n", i, (long)PARTITIONS);
    bug(2326); // Don't go further to prevent out-of-range references
  }

 SKIP_INIT:
  if (color == -1) {
    // Return black for "(future)" color for labels with missing theorems in comments
    let(&str1, "000000");
    return str1;
  }

  if (color < 1 || color > maxColor) {
    bug(2327);
  }
  // Fractional position in "spectrum"
  fraction = (1.0 * ((double)color - 1)) / (double)maxColor;
  partition = (long)(PARTITIONS * fraction); // Partition number (integer)
  if (partition >= PARTITIONS) bug(2325); // Round-off error?
  fractionInPartition = 1.0 * (fraction - (1.0 * (double)partition) / PARTITIONS)
      * PARTITIONS; // The fraction of this partition it covers
  red = (long)(1.0 * (redRef[partition] +
          fractionInPartition *
              (redRef[partition + 1] - redRef[partition])));
  green = (long)(1.0 * (greenRef[partition] +
          fractionInPartition *
              (greenRef[partition + 1] - greenRef[partition])));
  blue = (long)(1.0 * (blueRef[partition] +
          fractionInPartition *
              (blueRef[partition + 1] - blueRef[partition])));
  // debug

  // i=1;if (g_outputToString==0) {i=0;g_outputToString=1;}
  // print2("p%ldc%ld\n", partition, color); g_outputToString=i;
  // printf("red %ld green %ld blue %ld\n", red, green, blue);

  if (red < 0 || green < 0 || blue < 0
      || red > 255 || green > 255 || blue > 255) {
    print2("%ld %ld %ld\n", red, green, blue);
    bug(2323);
  }
  let(&str1, "      ");
  j = sprintf(str1, "%02X%02X%02X", (unsigned int)red, (unsigned int)green,
      (unsigned int)blue);
  if (j != 6) bug(2324);
  // debug
  // printf("<FONT COLOR='#%02X%02X%02X'>a </FONT>\n", red, green, blue);
  return str1;
} // spectrumToRGB

// Returns the HTML code for GIFs (!g_altHtmlFlag) or Unicode (g_altHtmlFlag),
// or LaTeX when !g_htmlFlag, for the math string (hypothesis or conclusion) that
// is passed in.
// Warning: The caller must deallocate the returned vstring.
vstring getTexLongMath(nmbrString *mathString, long statemNum)
{
  long pos;
  vstring_def(tex);
  vstring_def(texLine);
  vstring_def(lastTex);
  flag alphnew, alphold, unknownnew, unknownold;

  if (!g_texDefsRead) bug(2322); // TeX defs were not read
  let(&texLine, "");

  let(&lastTex, "");
  for (pos = 0; pos < nmbrLen(mathString); pos++) {
    free_vstring(tex);
    // tokenToTex allocates tex; we must deallocate it
    tex = tokenToTex(g_MathToken[mathString[pos]].tokenName, statemNum);
    if (!g_htmlFlag) { // LaTeX
      // If this token and previous token begin with letter, add a thin
      // space between them.
      // Also, anything not in table will have space added
      alphnew = !!isalpha((unsigned char)(tex[0]));
      unknownnew = 0;
      if (!strcmp(left(tex, 8), "\\mathrm{")) { // token not in table
        unknownnew = 1;
      }
      alphold = !!isalpha((unsigned char)(lastTex[0]));
      unknownold = 0;
      if (!strcmp(left(tex, 8), "\\mathrm{")) { // token not in table
        unknownold = 1;
      }
      // Put thin space only between letters and/or unknowns
      if ((alphold || unknownold) && (alphnew || unknownnew)) {
        // Put additional thin space between two letters
        if (!g_oldTexFlag) {
          let(&texLine, cat(texLine, "\\,", tex, " ", NULL));
        } else {
          let(&texLine, cat(texLine, "\\m{\\,", tex, "}", NULL));
        }
      } else {
        if (!g_oldTexFlag) {
          let(&texLine, cat(texLine, "", tex, " ", NULL));
        } else {
          let(&texLine, cat(texLine, "\\m{", tex, "}", NULL));
        }
      }
    } else { // HTML

      // When we have something like "E. x e. om x = y", the lack of
      // space between om and x looks ugly in HTML.  This kludge adds it in
      // for restricted quantifiers not followed by parenthesis, in order
      // to make the web page look a little nicer.  E.g. onminex.
      // Note that the space is put between the pos-1 and the pos tokens
      if (pos >=4) {
        if (!strcmp(g_MathToken[mathString[pos - 2]].tokenName, "e.")
            && (!strcmp(g_MathToken[mathString[pos - 4]].tokenName, "E.")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "A.")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "prod_")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "E*")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "iota_")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "Disj_")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "E!")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "sum_")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "X_")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "U_")
              || !strcmp(g_MathToken[mathString[pos - 4]].tokenName, "|^|_"))
            && strcmp(g_MathToken[mathString[pos]].tokenName, ")")
            // It also shouldn't be restricted _to_ an expression in parens.
            && strcmp(g_MathToken[mathString[pos - 1]].tokenName, "(")
            // ...or restricted _to_ a union or intersection
            && strcmp(g_MathToken[mathString[pos - 1]].tokenName, "U.")
            && strcmp(g_MathToken[mathString[pos - 1]].tokenName, "|^|")
            // ...or restricted _to_ an expression in braces
            && strcmp(g_MathToken[mathString[pos - 1]].tokenName, "{")) {
          let(&texLine, cat(texLine, " ", NULL)); // Add a space
        }
      }
      // This one puts a space between the 2 x's in a case like "E. x x = y".  E.g. cla4egf
      if (pos >=2) {
        // Match a token starting with a letter
        if (isalpha((unsigned char)(g_MathToken[mathString[pos]].tokenName[0]))) {
          // and make sure its length is 1
          if (!(g_MathToken[mathString[pos]].tokenName[1])) {

            // Make sure previous token is a letter also, to prevent unneeded
            // space in "ran ( A ..." (e.g. rncoeq, dfiun3g).
            // Match a token starting with a letter
            if (isalpha((unsigned char)(g_MathToken[mathString[pos - 1]].tokenName[0]))) {
              // and make sure its length is 1
              if (!(g_MathToken[mathString[pos - 1]].tokenName[1])) {

                // See if it's 1st letter in a quantified expression
                if (!strcmp(g_MathToken[mathString[pos - 2]].tokenName, "E.")
                    || !strcmp(g_MathToken[mathString[pos - 2]].tokenName, "A.")
                    || !strcmp(g_MathToken[mathString[pos - 2]].tokenName, "F/")
                    || !strcmp(g_MathToken[mathString[pos - 2]].tokenName, "E!")
                    // space btwn A,x in "E! x e. dom A x A y"
                    || !strcmp(g_MathToken[mathString[pos - 2]].tokenName, "ran")
                    || !strcmp(g_MathToken[mathString[pos - 2]].tokenName, "dom")
                    || !strcmp(g_MathToken[mathString[pos - 2]].tokenName, "E*")) {
                  let(&texLine, cat(texLine, " ", NULL)); // Add a space
                }
              }
            }
          }
        }
      }
      // This one puts a space after a letter followed by a word token
      // e.g. "A" and "suc" in "A. x e. U. A suc" in limuni2.
      if (pos >= 1) {
        // See if the next token is "suc"
        if (!strcmp(g_MathToken[mathString[pos]].tokenName, "suc")) {
          // Match a token starting with a letter for the current token
          if (isalpha(
              (unsigned char)(g_MathToken[mathString[pos - 1]].tokenName[0]))) {
            // and make sure its length is 1
            if (!(g_MathToken[mathString[pos - 1]].tokenName[1])) {
              let(&texLine, cat(texLine, " ", NULL)); // Add a space
            }
          }
        }
      }
      // This one puts a space before any "-." that doesn't come after
      // a parentheses e.g. ax-6 has both cases.
      if (pos >=1) {
        // See if we have a non-parenthesis followed by not
        if (strcmp(g_MathToken[mathString[pos - 1]].tokenName, "(")
            && !strcmp(g_MathToken[mathString[pos]].tokenName, "-.")) {
          let(&texLine, cat(texLine, " ", NULL)); // Add a space
        }
      }
      // This one puts a space between "S" and "(" in df-iso.
      if (pos >=4) {
        if (!strcmp(g_MathToken[mathString[pos - 4]].tokenName, "Isom")
            && !strcmp(g_MathToken[mathString[pos - 2]].tokenName, ",")
            && !strcmp(g_MathToken[mathString[pos]].tokenName, "(")) {
          let(&texLine, cat(texLine, " ", NULL)); // Add a space
        }
      }
      // This one puts a space between "}" and "(" in funcnvuni proof.
      if (pos >=1) {
        // See if we have "}" followed by "("
        if (!strcmp(g_MathToken[mathString[pos - 1]].tokenName, "}")
            && !strcmp(g_MathToken[mathString[pos]].tokenName, "(")) {
          let(&texLine, cat(texLine, " ", NULL)); // Add a space
        }
      }
      // This one puts a space between "}" and "{" in konigsberg proof.
      if (pos >=1) {
        // See if we have "}" followed by "("
        if (!strcmp(g_MathToken[mathString[pos - 1]].tokenName, "}")
            && !strcmp(g_MathToken[mathString[pos]].tokenName, "{")) {
          let(&texLine, cat(texLine, " ", NULL)); // Add a space
        }
      }

      let(&texLine, cat(texLine, tex, NULL));
    } // if !g_htmlFlag
    let(&lastTex, tex); // Save for next pass
  } // Next pos

  // x Discard redundant white space to reduce HTML file size
  let(&texLine, edit(texLine, 8 + 16 + 128));

  // Enclose math symbols in a span to be used for font selection
  let(&texLine, cat(
      (g_altHtmlFlag ? cat("<SPAN ", g_htmlFont, ">", NULL) : ""),
      texLine,
      (g_altHtmlFlag ? "</SPAN>" : ""), NULL));

  free_vstring(tex);
  free_vstring(lastTex);
  return texLine;
} // getTexLongMath

// Returns the TeX, or HTML code for GIFs (!g_altHtmlFlag) or Unicode
// (g_altHtmlFlag), for a statement's hypotheses and assertion in the form
// hyp & ... & hyp => assertion.
// Warning: The caller must deallocate the returned vstring (i.e. this
// function cannot be used in let statements but must be assigned to
// a local vstring for local deallocation).
vstring getTexOrHtmlHypAndAssertion(long statemNum) {
  long reqHyps, essHyps, n;
  nmbrString *nmbrTmpPtr; // Pointer only; not allocated directly
  vstring_def(texOrHtmlCode);
  vstring_def(str2);
  // Count the number of essential hypotheses essHyps
  essHyps = 0;
  reqHyps = nmbrLen(g_Statement[statemNum].reqHypList);
  let(&texOrHtmlCode, "");
  for (n = 0; n < reqHyps; n++) {
    if (g_Statement[g_Statement[statemNum].reqHypList[n]].type
        == (char)e_) {
      essHyps++;
      if (texOrHtmlCode[0]) { // Add '&' between hypotheses
        if (!g_htmlFlag) {
          // Hard-coded for set.mm!
          let(&texOrHtmlCode, cat(texOrHtmlCode,
                 "\\quad\\&\\quad "
              ,NULL));
        } else {
          if (g_altHtmlFlag) {
            // Hard-coded for set.mm!
            let(&texOrHtmlCode, cat(texOrHtmlCode,
                "<SPAN ", g_htmlFont, ">",
                "&nbsp;&nbsp;&nbsp; &amp;&nbsp;&nbsp;&nbsp;",
                "</SPAN>",
                NULL));
          } else {
            // Hard-coded for set.mm!
            let(&texOrHtmlCode, cat(texOrHtmlCode,
          "&nbsp;&nbsp;&nbsp;<IMG SRC='amp.gif' WIDTH=12 HEIGHT=19 ALT='&amp;'"
                ," ALIGN=TOP>&nbsp;&nbsp;&nbsp;"
                ,NULL));
          }
        }
      } // if texOrHtmlCode[0]
      // Construct HTML hypothesis
      nmbrTmpPtr = g_Statement[g_Statement[statemNum].reqHypList[n]].mathString;
      let(&str2, "");
      str2 = getTexLongMath(nmbrTmpPtr, statemNum);
      let(&texOrHtmlCode, cat(texOrHtmlCode, str2, NULL));
    }
  }
  if (essHyps) { // Add big arrow if there were hypotheses
    if (!g_htmlFlag) {
      // Hard-coded for set.mm!
      let(&texOrHtmlCode, cat(texOrHtmlCode,
                 "\\quad\\Rightarrow\\quad "
          ,NULL));
    } else {
      if (g_altHtmlFlag) {
        // Hard-coded for set.mm!
        let(&texOrHtmlCode, cat(texOrHtmlCode,
            // sans-serif to work around FF3 bug that produces
            // huge character heights otherwise.
            "&nbsp;&nbsp;&nbsp; <FONT FACE=sans-serif>&#8658;</FONT>&nbsp;&nbsp;&nbsp;",
            NULL));
      } else {
        // Hard-coded for set.mm!
        let(&texOrHtmlCode, cat(texOrHtmlCode,
            "&nbsp;&nbsp;&nbsp;<IMG SRC='bigto.gif' WIDTH=15 HEIGHT=19 ALT='=&gt;'",
            " ALIGN=TOP>&nbsp;&nbsp;&nbsp;",
            NULL));
      }
    }
  }
  // Construct TeX or HTML assertion
  nmbrTmpPtr = g_Statement[statemNum].mathString;
  free_vstring(str2);
  str2 = getTexLongMath(nmbrTmpPtr, statemNum);
  let(&texOrHtmlCode, cat(texOrHtmlCode, str2, NULL));

  // Deallocate memory
  free_vstring(str2);
  return texOrHtmlCode;
} // getTexOrHtmlHypAndAssertion

// Called by the WRITE BIBLIOGRAPHY command and also by VERIFY MARKUP
// for error checking.
// Returns 0 if OK, 1 if warning(s), 2 if any error
flag writeBibliography(vstring bibFile,
    vstring labelMatch, // Normally "*" except when called by verifyMarkup()
    flag errorsOnly, // 1 = no output, just warning msgs if any
    flag fileCheck) // 1 = check external files (gifs and bib)
{
  flag errFlag;
  FILE *list1_fp = NULL;
  FILE *list2_fp = NULL;
  long lines, p2, i, j, jend, k, l, m, n, p, q, s, pass1refs;
  vstring_def(str1);
  vstring_def(str2);
  vstring_def(str3);
  vstring_def(str4);
  vstring_def(newstr);
  vstring_def(oldstr);
  pntrString_def(pntrTmp);
  flag warnFlag;

  n = 0; // Old gcc 4.6.3 wrongly says may be uninit ln 5506
  pass1refs = 0; // gcc 4.5.3 wrongly says may be uninit
  if (!fileCheck && errorsOnly == 0) {
    bug(2336); // If we aren't opening files, a non-error run can't work
  }

  // This utility builds the bibliographical cross-references to various
  // textbooks and updates the user-specified file normally called
  // mmbiblio.html.
  warnFlag = 0; // 1 means warning was found, 2 that error was found
  errFlag = 0; // Error flag to recover input file and to set return value 2
  if (fileCheck) {
    list1_fp = fSafeOpen(bibFile, "r", 0 /* noVersioningFlag */);
    if (list1_fp == NULL) {
      // Couldn't open it (error msg was provided)
      return 1;
    }
    if (errorsOnly == 0) {
      fclose(list1_fp);
      // This will rename the input mmbiblio.html as mmbiblio.html~1
      list2_fp = fSafeOpen(bibFile, "w", 0 /* noVersioningFlag */);
      if (list2_fp == NULL) {
        // Couldn't open it (error msg was provided)
        return 1;
      }
      // Note: in older versions the "~1" string was OS-dependent, but we
      // do not support VAX or THINK C anymore...  Anyway we reopen it
      // here with the renamed file in case the OS won't let us rename
      // an opened file during the fSafeOpen for write above.
      list1_fp = fSafeOpen(cat(bibFile, "~1", NULL), "r", 0 /* noVersioningFlag */);
      if (list1_fp == NULL) bug(2337);
    }
  }
  if (!g_texDefsRead) {
    g_htmlFlag = 1;
    if (2 /* error */ == readTexDefs(errorsOnly, fileCheck)) {
      errFlag = 2; // Error flag to recover input file
      goto BIB_ERROR; // An error occurred
    }
  }

  // Transfer the input file up to the special "<!-- #START# -->" comment
  if (fileCheck) {
    while (1) {
      if (!linput(list1_fp, NULL, &str1)) {
        print2("?Error: Could not find \"<!-- #START# -->\" line in input file \"%s\".\n",
            bibFile);
        errFlag = 2; // Error flag to recover input file
        break;
      }
      if (errorsOnly == 0) {
        fprintf(list2_fp, "%s\n", str1);
      }
      if (!strcmp(str1, "<!-- #START# -->")) break;
    }
    if (errFlag) goto BIB_ERROR;
  }

  p2 = 1; // Pass 1 or 2 flag
  lines = 0;
  while (1) {

    if (p2 == 2) { // Pass 2
      // Allocate memory for sorting
      pntrLet(&pntrTmp, pntrSpace(lines));
      lines = 0;
    }

    // Scan all $a and $p statements
    for (i = 1; i <= g_statements; i++) {
      if (g_Statement[i].type != (char)p_ &&
        g_Statement[i].type != (char)a_) continue;

      // Normally labelMatch is *, but may be more specific for
      // use by verifyMarkup().
      if (!matchesList(g_Statement[i].labelName, labelMatch, '*', '?')) {
        continue;
      }

      // Omit ...OLD (obsolete) and ...NEW (to be implemented) statements
      if (instr(1, g_Statement[i].labelName, "NEW")) continue;
      if (instr(1, g_Statement[i].labelName, "OLD")) continue;
      let(&str1, "");
      str1 = getDescription(i); // Get the statement's comment
      if (!instr(1, str1, "[")) continue;
      l = (signed)(strlen(str1));
      for (j = 0; j < l; j++) {
        if (str1[j] == '\n') str1[j] = ' '; // Change newlines to spaces
        if (str1[j] == '\r') bug(2338);
      }
      let(&str1, edit(str1, 8 + 128 + 16)); // Reduce & trim whitespace

      // Clear out math symbols in backquotes to prevent false matches
      // to [reference] bracket.
      k = 0; // Math symbol mode if 1
      l = (signed)(strlen(str1));
      for (j = 0; j < l - 1; j++) {
        if (k == 0) {
          if (str1[j] == '`') {
            k = 1; // Start of math mode
          }
        } else { // In math mode
          if (str1[j] == '`') { // A backquote
            if (str1[j + 1] == '`') {
              // It is an escaped backquote
              str1[j] = ' ';
              str1[j + 1] = ' ';
            } else {
              k = 0; // End of math mode
            }
          } else { // Not a backquote
            str1[j] = ' '; // Clear out the math mode part
          }
        } // end if k == 0
      } // next j

      // Put spaces before page #s (up to 4 digits) for sorting
      j = 0;
      while (1) {
        j = instr(j + 1, str1, " p. "); // Heuristic - match " p. "
        if (!j) break;
        if (j) {
          for (k = j + 4; k <= (signed)(strlen(str1)) + 1; k++) {
            if (!isdigit((unsigned char)(str1[k - 1]))) {
              let(&str1, cat(left(str1, j + 2),
                  space(4 - (k - (j + 4))), right(str1, j + 3), NULL));
              // Add ### after page number as marker
              let(&str1, cat(left(str1, j + 7), "###", right(str1, j + 8),
                  NULL));
              break;
            }
          }
        }
      }
      // Process any bibliographic references in comment
      j = 0;
      n = 0;
      while (1) {
        j = instr(j + 1, str1, "["); // Find reference (not robust)
        if (!j) break;

        // Fix mmbiblio.html corruption caused by
        // "[Copying an angle]" in df-ibcg in
        // set.mm.2016-09-18-JB-mbox-cleanup.
        // Skip if there is no trailing "]"
        jend = instr(j, str1, "]");
        if (!jend) break;
        // Skip bracketed text with spaces.
        // This is a somewhat ugly workaround that lets us tolerate user
        // comments in [...] is there is at least one space.
        // The printTexComment() above handles this case with the
        // "strcspn(bibTag, " \n\r\t\f")" above; here we just have to look
        // for space since we already reduced \n \t to space (\f is probably
        // overkill, and any \r's are removed during the READ command).
        if (instr(1, seg(str1, j, jend), " ")) continue;
        // Skip escaped bracket "[["
        if (str1[j] == '[') { // (j+1)th character in str1
          j++; // Skip over 2nd "["
          continue;
        }
        if (!isalnum((unsigned char)(str1[j]))) continue; // Not start of reference
        n++;
        // Backtrack from [reference] to a starting keyword
        m = 0;
        let(&str2, edit(str1, 32)); // to uppercase

        // (The string search below is rather inefficient; maybe improve
        // the algorithm if speed becomes a problem.)
        for (k = j - 1; k >= 1; k--) {
          // **IMPORTANT** Make sure to update mmhlpa.c HELP WRITE BIBLIOGRAPHY
          // if new items are added to this list.
          if (0
              // The first five keywords are more frequent so are put first for
              // efficiency; the rest is in alphabetical order.
              || !strcmp(mid(str2, k, (long)strlen("THEOREM")), "THEOREM")
              || !strcmp(mid(str2, k, (long)strlen("EQUATION")), "EQUATION")
              || !strcmp(mid(str2, k, (long)strlen("DEFINITION")), "DEFINITION")
              || !strcmp(mid(str2, k, (long)strlen("LEMMA")), "LEMMA")
              || !strcmp(mid(str2, k, (long)strlen("EXERCISE")), "EXERCISE")
              // ---- end of optimized search -----
              || !strcmp(mid(str2, k, (long)strlen("AXIOM")), "AXIOM")
              || !strcmp(mid(str2, k, (long)strlen("CHAPTER")), "CHAPTER")
              || !strcmp(mid(str2, k, (long)strlen("CLAIM")), "CLAIM")
              || !strcmp(mid(str2, k, (long)strlen("COMPARE")), "COMPARE")
              || !strcmp(mid(str2, k, (long)strlen("CONCLUSION")), "CONCLUSION")
              || !strcmp(mid(str2, k, (long)strlen("CONDITION")), "CONDITION")
              || !strcmp(mid(str2, k, (long)strlen("CONJECTURE")), "CONJECTURE")
              || !strcmp(mid(str2, k, (long)strlen("COROLLARY")), "COROLLARY")
              || !strcmp(mid(str2, k, (long)strlen("CRITERIA")), "CRITERIA")
              || !strcmp(mid(str2, k, (long)strlen("CRITERION")), "CRITERION")
              || !strcmp(mid(str2, k, (long)strlen("EXAMPLE")), "EXAMPLE")
              || !strcmp(mid(str2, k, (long)strlen("FACT")), "FACT")
              || !strcmp(mid(str2, k, (long)strlen("FIGURE")), "FIGURE")
              || !strcmp(mid(str2, k, (long)strlen("INTRODUCTION")), "INTRODUCTION")
              || !strcmp(mid(str2, k, (long)strlen("ITEM")), "ITEM")
              || !strcmp(mid(str2, k, (long)strlen("LEMMAS")), "LEMMAS")
              || !strcmp(mid(str2, k, (long)strlen("LINE")), "LINE")
              || !strcmp(mid(str2, k, (long)strlen("LINES")), "LINES")
              || !strcmp(mid(str2, k, (long)strlen("NOTATION")), "NOTATION")
              || !strcmp(mid(str2, k, (long)strlen("NOTE")), "NOTE")
              || !strcmp(mid(str2, k, (long)strlen("OBSERVATION")), "OBSERVATION")
              || !strcmp(mid(str2, k, (long)strlen("PARAGRAPH")), "PARAGRAPH")
              || !strcmp(mid(str2, k, (long)strlen("PART")), "PART")
              || !strcmp(mid(str2, k, (long)strlen("POSTULATE")), "POSTULATE")
              || !strcmp(mid(str2, k, (long)strlen("PROBLEM")), "PROBLEM")
              || !strcmp(mid(str2, k, (long)strlen("PROOF")), "PROOF")
              || !strcmp(mid(str2, k, (long)strlen("PROPERTY")), "PROPERTY")
              || !strcmp(mid(str2, k, (long)strlen("PROPOSITION")), "PROPOSITION")
              || !strcmp(mid(str2, k, (long)strlen("REMARK")), "REMARK")
              || !strcmp(mid(str2, k, (long)strlen("RESULT")), "RESULT")
              || !strcmp(mid(str2, k, (long)strlen("RULE")), "RULE")
              || !strcmp(mid(str2, k, (long)strlen("SCHEME")), "SCHEME")
              || !strcmp(mid(str2, k, (long)strlen("SCOLIA")), "SCOLIA")
              || !strcmp(mid(str2, k, (long)strlen("SCOLION")), "SCOLION")
              || !strcmp(mid(str2, k, (long)strlen("SECTION")), "SECTION")
              || !strcmp(mid(str2, k, (long)strlen("STATEMENT")), "STATEMENT")
              || !strcmp(mid(str2, k, (long)strlen("SUBSECTION")), "SUBSECTION")
              || !strcmp(mid(str2, k, (long)strlen("TABLE")), "TABLE")
              ) {
            m = k;
            break;
          }
          freeTempAlloc(); // Clear tmp alloc stack created by "mid"
        }
        if (!m) {
          if (p2 == 1) {
            print2(
             "?Warning: Bibliography keyword missing in comment for \"%s\".\n",
                g_Statement[i].labelName);
            print2(
                "    (See HELP WRITE BIBLIOGRAPHY for list of keywords.)\n");
            warnFlag = 1;
          }
          continue; // Not a bib ref - ignore
        }
        // m is at the start of a keyword
        p = instr(m, str1, "["); // Start of bibliography reference
        q = instr(p, str1, "]"); // End of bibliography reference
        if (q == 0) {
          if (p2 == 1) {
            print2("?Warning: Bibliography reference not found in HTML file in \"%s\".\n",
              g_Statement[i].labelName);
            warnFlag = 1;
          }
          continue; // Pretend it is not a bib ref - ignore
        }
        s = instr(q, str1, "###"); // Page number marker
        if (!s) {
          if (p2 == 1) {
            print2("?Warning: No page number after [<author>] bib ref in \"%s\".\n",
              g_Statement[i].labelName);
            warnFlag = 1;
          }
          continue; // No page number given - ignore
        }
        // Now we have a real reference; increment reference count
        lines++;
        if (p2 == 1) continue; // In 1st pass, we just count refs

        let(&str2, seg(str1, m, p - 1));     // "Theorem #"
        let(&str3, seg(str1, p + 1, q - 1)); // "[bibref]" w/out []
        let(&str4, seg(str1, q + 1, s - 1)); // " p. nnnn"
        str2[0] = (char)(toupper((unsigned char)(str2[0])));
        // Eliminate noise like "of" in "Theorem 1 of [bibref]"
        for (k = (long)strlen(str2); k >=1; k--) {
          if (0
              || !strcmp(mid(str2, k, (long)strlen(" of ")), " of ")
              || !strcmp(mid(str2, k, (long)strlen(" in ")), " in ")
              || !strcmp(mid(str2, k, (long)strlen(" from ")), " from ")
              || !strcmp(mid(str2, k, (long)strlen(" on ")), " on ")
              ) {
            let(&str2, left(str2, k - 1));
            break;
          }
          freeTempAlloc();
        }

        free_vstring(newstr);
        newstr = pinkHTML(i); // Get little pink number
        let(&oldstr, cat(
            // Construct the sorting key.
            // The space() helps Th. 9 sort before Th. 10 on same page
            str3, " ", str4, space(20 - (long)strlen(str2)), str2,
            "|||", // ||| means end of sort key
            // Construct just the statement href for combining dup refs
            "<A HREF=\"", g_Statement[i].labelName,
            ".html\">", g_Statement[i].labelName, "</A>",
            newstr,
            "&&&", // &&& means end of statement href.
            // Construct actual HTML table row (without ending tag
            // so duplicate references can be added).
            (i < g_extHtmlStmt)
               ? "<TR>"
               : (i < g_mathboxStmt)
                   ? cat("<TR BGCOLOR=", PURPLISH_BIBLIO_COLOR, ">", NULL)
                   : cat("<TR BGCOLOR=", SANDBOX_COLOR, ">", NULL),

            "<TD NOWRAP>[<A HREF=\"",

            (i < g_extHtmlStmt)
               ? g_htmlBibliography
               : (i < g_mathboxStmt)
                   ? extHtmlBibliography
                   // Note that the sandbox uses the mmset.html bibliography
                   : g_htmlBibliography,

            "#",
            str3,
            "\">", str3, "</A>]", str4,
            "</TD><TD>", str2, "</TD><TD><A HREF=\"",
            g_Statement[i].labelName,
            ".html\">", g_Statement[i].labelName, "</A>",
            newstr, NULL));
        // Put construction into string array for sorting
        let((vstring *)(&pntrTmp[lines - 1]), oldstr);
      } // while(1)
    } // next i

    // 'lines' should be the same in both passes
    if (p2 == 1) {
      pass1refs = lines;
    } else {
      if (pass1refs != lines) bug(2339);
    }

    if (errorsOnly == 0 && p2 == 2) {
      // print2("Pass %ld finished.  %ld references were processed.\n", p2, lines);
      print2("%ld references were processed.\n", lines);
    }
    if (p2 == 2) break;
    p2++; // Increment from pass 1 to pass 2
  } // while(1)

  // Sort
  g_qsortKey = "";
  qsort(pntrTmp, (size_t)lines, sizeof(void *), qsortStringCmp);

  // Combine duplicate references
  let(&str1, ""); // Last biblio ref
  for (i = 0; i < lines; i++) {
    j = instr(1, (vstring)(pntrTmp[i]), "|||");
    let(&str2, left((vstring)(pntrTmp[i]), j - 1));
    if (!strcmp(str1, str2)) {
      // Combine last with this
      k = instr(j, (vstring)(pntrTmp[i]), "&&&");
      // Extract statement href
      let(&str3, seg((vstring)(pntrTmp[i]), j + 3, k -1));
      let((vstring *)(&pntrTmp[i]),
          cat((vstring)(pntrTmp[i - 1]), " &nbsp;", str3, NULL));
      let((vstring *)(&pntrTmp[i - 1]), ""); // Clear previous line
    }
    let(&str1, str2);
  }

  // Write output
  if (fileCheck && errorsOnly == 0) {
    n = 0; // Table rows written
    for (i = 0; i < lines; i++) {
      j = instr(1, (vstring)(pntrTmp[i]), "&&&");
      if (j) { // Don't print blanked out combined lines
        n++;
        // Take off prefixes and reduce spaces
        let(&str1, edit(right((vstring)(pntrTmp[i]), j + 3), 16));
        j = 1;
        // Break up long lines for text editors
        let(&g_printString, "");
        g_outputToString = 1;
        printLongLine(cat(str1, "</TD></TR>", NULL),
            " ", // Start continuation line with space
            "\""); // Don't break inside quotes e.g. "Arial Narrow"
        g_outputToString = 0;
        fprintf(list2_fp, "%s", g_printString);
        free_vstring(g_printString);
      }
    }
  }

  // Discard the input file up to the special "<!-- #END# -->" comment
  if (fileCheck) {
    while (1) {
      if (!linput(list1_fp, NULL, &str1)) {
        print2(
  "?Error: Could not find \"<!-- #END# -->\" line in input file \"%s\".\n",
            bibFile);
        errFlag = 2; // Error flag to recover input file
        break;
      }
      if (!strcmp(str1, "<!-- #END# -->")) {
        if (errorsOnly == 0) {
          fprintf(list2_fp, "%s\n", str1);
        }
        break;
      }
    }
    if (errFlag) goto BIB_ERROR;
  }

  if (fileCheck && errorsOnly == 0) {
    // Transfer the rest of the input file
    while (1) {
      if (!linput(list1_fp, NULL, &str1)) {
        break;
      }

      // Update the date stamp at the bottom of the HTML page.
      // This is just a nicety; no error check is done.
      if (!strcmp("This page was last updated on ", left(str1, 30))) {
        let(&str1, cat(left(str1, 30), date(), ".", NULL));
      }

      fprintf(list2_fp, "%s\n", str1);
    }

    print2("%ld table rows were written.\n", n);
    // Deallocate string array
    for (i = 0; i < lines; i++) free_vstring(*(vstring *)(&pntrTmp[i]));
    free_pntrString(pntrTmp);
  }

 BIB_ERROR:
  if (fileCheck) {
    fclose(list1_fp);
    if (errorsOnly == 0) {
      fclose(list2_fp);
    }
    if (errorsOnly == 0) {
      if (errFlag) {
        // Recover input files in case of error
        remove(bibFile); // Delete output file
        // Restore input file name
        rename(cat(bibFile, "~1", NULL), g_fullArg[2]);
        print2("?The file \"%s\" was not modified.\n", g_fullArg[2]);
      }
    }
  }
  if (errFlag == 2) warnFlag = 2;
  return warnFlag;
} // writeBibliography

// Returns 1 if stmt1 and stmt2 are in different mathboxes, 0 if
// they are in the same mathbox or if one of them is not in a mathbox.
flag inDiffMathboxes(long stmt1, long stmt2) {
  long mbox1, mbox2;
  mbox1 = getMathboxNum(stmt1);
  mbox2 = getMathboxNum(stmt2);
  if (mbox1 == 0 || mbox2 == 0) return 0;
  if (mbox1 != mbox2) return 1;
  return 0;
}

// Returns the user of the mathbox that a statement is in, or ""
// if the statement is not in a mathbox.
// Caller should NOT deallocate returned string (it points directly to
// g_mathboxUser[] entry).
vstring getMathboxUser(long stmt) {
  long mbox;
  mbox = getMathboxNum(stmt);
  if (mbox == 0) return "";
  return g_mathboxUser[mbox - 1];
}

// Given a statement number, find out what mathbox it's in (numbered starting
// at 1) mainly for error messages; if it's not in a mathbox, return 0.
// We assume the number of mathboxes is small enough that a linear search
// won't slow things too much.
long getMathboxNum(long stmt) {
  long mbox;
  assignMathboxInfo(); // In case it's not yet initialized
  for (mbox = 0; mbox < g_mathboxes; mbox++) {
    if (stmt < g_mathboxStart[mbox]) break;
  }
  return mbox;
} // getMathboxNum

// Assign the global variable g_mathboxStmt, the statement number with the
// label "mathbox", as well as g_mathboxes, g_mathboxStart[], g_mathboxEnd[],
// and g_mathboxUser[].  For speed, we do the lookup only if it hasn't been
// done yet.   Note that the ERASE command (eraseSource()) should set
// g_mathboxStmt to zero as well as deallocate the strings.
// This function will just return if g_mathboxStmt is already nonzero.
#define MB_LABEL "mathbox"
void assignMathboxInfo(void) {
  if (g_mathboxStmt == 0) { // Look up "mathbox" label if it hasn't been
    g_mathboxStmt = lookupLabel(MB_LABEL);
    if (g_mathboxStmt == -1) { // There are no mathboxes
      g_mathboxStmt = g_statements + 1; // Default beyond db end if none
      g_mathboxes = 0;
    } else {
      // Population mathbox information variables
      g_mathboxes = getMathboxLoc(&g_mathboxStart, &g_mathboxEnd,
          &g_mathboxUser);
    }
  }
  return;
} // assignMathboxInfo

// Returns the number of mathboxes, while assigning start statement, end
// statement, and mathbox name.
#define MB_TAG "Mathbox for "
long getMathboxLoc(nmbrString **mathboxStart, nmbrString **mathboxEnd,
    pntrString **mathboxUser) {
  long m, p, q, tagLen, stmt;
  long mathboxes = 0;
  vstring_def(comment);
  vstring_def(user);
  assignMathboxInfo(); // Assign g_mathboxStmt
  tagLen = (long)strlen(MB_TAG);
  // Ensure lists are initialized
  if (pntrLen((pntrString *)(*mathboxUser)) != 0) bug(2347);
  if (nmbrLen((nmbrString *)(*mathboxStart)) != 0) bug(2348);
  if (nmbrLen((nmbrString *)(*mathboxEnd)) != 0) bug(2349);
  for (stmt = g_mathboxStmt + 1; stmt <= g_statements; stmt++) {
    // Heuristic to match beginning of mathbox
    let(&comment, left(g_Statement[stmt].labelSectionPtr,
        g_Statement[stmt].labelSectionLen));
    p = 0;
    // This loop will skip empty mathboxes i.e. it will get the last
    // "Mathbox for " in the label section comment(s).
    while (1) {
      q = instr(p + 1, comment, MB_TAG);
      if (q == 0) break;
      p = q; // Save last "Mathbox for "
    }
    if (p == 0) continue; // No "Mathbox for " in this statement's comment

    // Found a mathbox; assign user and start statement
    mathboxes++;
    q = instr(p, comment, "\n");
    if (q == 0) bug(2350); // No end of line
    let(&user, seg(comment, p + tagLen, q - 1));
    pntrLet(&(*mathboxUser), pntrAddElement(*mathboxUser));
    (*mathboxUser)[mathboxes - 1] = "";
    let((vstring *)(&((*mathboxUser)[mathboxes - 1])), user);
    nmbrLet(&(*mathboxStart), nmbrAddElement(*mathboxStart, stmt));
  } // next stmt
  if (mathboxes == 0) goto RETURN_POINT;
  // Assign end statements
  nmbrLet(&(*mathboxEnd), nmbrSpace(mathboxes)); // Pre-allocate
  for (m = 0; m < mathboxes - 1; m++) {
    (*mathboxEnd)[m] = (*mathboxStart)[m + 1] - 1;
  }
  (*mathboxEnd)[mathboxes - 1] = g_statements; // Assumed end of last mathbox
 RETURN_POINT:
  free_vstring(comment);
  free_vstring(user);
  return mathboxes;
} // getMathboxLoc
