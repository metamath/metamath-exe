/*****************************************************************************/
/*        Copyright (C) 2021  NORMAN MEGILL                                  */
/*            License terms:  GNU General Public License                     */
/*****************************************************************************/
/*34567890123456 (79-character line to adjust editor window) 2345678901234567*/

/*!
 * \file
 * Implementation of basic input and output.
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "mmvstr.h"
#include "mmdata.h"
#include "mminou.h"
#include "mmcmdl.h" // for g_commandPrompt global

#ifdef __WATCOMC__
  // Bugs in WATCOMC:
  // 1. #include <conio.h> has compile errors
  // 2. Two consecutive calls to vprintf after va_start causes register dump
  // or corrupts 2nd arg.

// From <conio.h>:
#ifndef _CONIO_H_INCLUDED
extern int cprintf(const char *f__mt,...);
#define _CONIO_H_INCLUDED
#endif
#endif // End #ifdef __WATCOMC__

/*!
 * \def QUOTED_SPACE
 * The general line wrapping algorithm looks out for spaces as break positions.
 * To prevent a quote delimited by __"__ be broken down, spaces are temporarily
 * replaced with 0x03 (ETX, end of transmission), hopefully never used in
 * text in this application.
 */
#define QUOTED_SPACE 3 // ASCII 3 that temporarily zaps a space

int g_errorCount = 0;

// Global variables used by print2()
flag g_logFileOpenFlag = 0;
FILE *g_logFilePtr;
FILE *g_listFile_fp = NULL;
// Global variables used by print2()
flag g_outputToString = 0;
vstring_def(g_printString);
// Global variables used by cmdInput()
long g_commandFileNestingLevel = 0;
FILE *g_commandFilePtr[MAX_COMMAND_FILE_NESTING + 1];
vstring g_commandFileName[MAX_COMMAND_FILE_NESTING + 1];

/*!
 * \var flag g_commandFileSilent[]
 * a 1 for a particular \ref g_commandFileNestingLevel suppresses output for
 * that submit nesting level.  The value for the interactive level
 * (\ref g_commandFileNestingLevel == 0) is ignored.
 */
flag g_commandFileSilent[MAX_COMMAND_FILE_NESTING + 1];
flag g_commandFileSilentFlag = 0; // For SUBMIT ... /SILENT

FILE *g_input_fp; // File pointers
vstring_def(g_input_fn);
vstring_def(g_output_fn); // File names

long g_screenWidth = MAX_LEN; // Width default = 79
// g_screenHeight is one less than the physical screen to account for the
// prompt line after pausing.
long g_screenHeight = SCREEN_HEIGHT; // Default = 23

/*!
 * \var int printedLines
 * Lines printed since last user input (mod screen height).  This value is used
 * to determine when a page of output is completed, and the user needs to be
 * prompted for continuing output.  It counts the number of LF characters,
 * which may differ from the lines actually used because some hard line breaks
 * are enforced by overly long lines.
 */
int printedLines = 0;
flag g_scrollMode = 1; // Flag for continuous (0) or prompted (1) scroll.
flag g_quitPrint = 0; // Flag that user quit the output.

/*!
 * \var flag localScrollMode
 *
 * value 0: temporarily disables page-wise prompted scroll (see
 * \ref g_scrollMode) until enabled (value 1) again.  Normal user input by
 * \ref cmdInput1 allows scrolling during output (value 1).  A value of 0
 * indicates the user has issued command _s_ or _S_ (see \ref pgBackBuffer)
 * to print all pending output without interruption.  In this case the value 0
 * is assigned temporarily to skip prompts.
 */
flag localScrollMode = 1;

/*!
 * \page pgBackBuffer History of Pages of Output
 *
 * Lengthy text can be displayed in a page-wise manner,  if requested.  In such
 * a case text output is broken down into pieces small enough to fit into the
 * screen rectangle of a virtual text device called a __page__ here.  The
 * dimensions of this rectangle or page are given in \ref g_screenWidth and
 * \ref g_screenHeight. An extra line outside of the page is reserved for a
 * prompt and the echo of the user response.
 *
 * Regular output (not prompts or error messages) is added line by line both
 * to the screen and the latest entry of a stack of strings \ref backBuffer,
 * the.__current page__.  The number of lines pushed to it is kept in
 * \ref printedLines.  The moment this value indicates a full page, a new empty
 * page is pushed on the stack, and output is suspended to let the user read
 * displayed contents in rest.  A user prompt asks for resuming pending output,
 * or alternatively step backwards and forward through the sequence of
 * saved pages for redisplay.  The variable \ref backBufferPos tracks which
 * page the user requested last (or points to the currently built-up page).
 *
 * On initialization memory is allocated for the \ref backBuffer, and an empty
 * string (empty page) is pushed onto the stack as a guard.  This is a simple
 * technical means to formally carry out a back step, even when you have just
 * recalled the very first saved page.  The display of this empty guard page
 * has no other effect than repeating the prompt.  The \ref backBufferPos is
 * never updated to point (value 0) to this guard page.
 *
 * During a replay of the history normal user input is intercepted and
 * interpreted as scroll commands.  These commands are at most a single
 * character, followed by a LF.  A _b_ or _B_ backs up one step further in
 * history,  an empty line means one step forward, _s_ or _S_ scrolls to the
 * end, showing all pages in between and all pending output at one swoop,
 * and _q_ or _Q_ skips all pending output and gets you directly back to normal
 * input.  You cannot back up to the guard page (although it is shown on input
 * B when at the very first saved page).  Moving a step forward when at the
 * very last history page resumes pending normal output.  Unrecognized input is
 * simply ignored.  The loop controlling these movements is found in
 * \ref print2, but \ref cmdInput can trigger it as well.
 */

/*!
 * \var pntrString* backBuffer
 * Buffer for B (back) command at end-of-page prompt.  Although formally a
 * \ref pntrString is using void*, this buffer contains pointer to \ref vstring
 * only.  Its element at index 0 is fixed to an empty string (page), a guard
 * representing contents not available.
 *
 * Some longer text (like help texts for example) provide a page wise display
 * with a scroll option, so the user can move freely back and forth in the
 * text.  This is the storage of already displayed text kept for possible
 * redisplay (see \ref pgBackBuffer).
 *
 * The element last displayed, or currently built up, is denoted by
 * \ref backBufferPos.
 *
 * The buffer is allocated and initialized to not-empty by \ref print2.
 */
pntrString_def(backBuffer);

/*!
 * \var backBufferPos
 *
 * Number of entries in the \ref backBuffer that are available for repeatedly
 * scrolling back.  Initialized to 0.
 *
 * \invariant The value 0 indicates an unitialized and empty  \ref backBuffer.
 */
long backBufferPos = 0;

/*!
 * \var flag backFromCmdInput
 * \brief user entered a B (scroll back command) when a command was expected.
 * This \ref flag is set only by \ref cmdInput that handles some of the user's
 * scroll input commands, in particular the B command for moving backwards
 * in page-wise display.  All further scroll commands are interpreted by
 * \ref print2, to which this flag is directed.  It signals the \ref backBuffer
 * is being scrolled.
 */
flag backFromCmdInput = 0;

// Special: if global flag g_outputToString = 1, then the output is not
// printed but is added to global string g_printString.
// Returns 0 if user typed "q" during scroll prompt; this lets a procedure
// interrupt it's output for speedup (rest of output will be suppressed anyway
// until next command line prompt).
flag print2(const char* fmt, ...) {
  // This performs the same operations as printf, except that if a log file is
  // open, the characters will also be printed to the log file.
  // Also, scrolling is paused at each page if in scroll-prompted mode.
  va_list ap;
  char c;
  long nlpos, lineLen, charsPrinted;
  long i;

  char *printBuffer; // Allocated dynamically

// step (1) initialize backBuffer

  if (backBufferPos == 0) {
    // Initialize backBuffer - 1st time in program
    // Warning:  Don't call bug(), because it calls print2.
    if (pntrLen(backBuffer)) {
      printf("*** BUG #1501\n");
#if __STDC__
      fflush(stdout);
#endif
    }
    backBufferPos = 1;
    pntrLet(&backBuffer, pntrAddElement(backBuffer));
    // Note: pntrAddElement() initializes the added element to the
    // empty string, so we don't need a separate initialization.
    // backBuffer[backBufferPos - 1] = ""; // already done
  }

  if ((!g_quitPrint && g_commandFileNestingLevel == 0 && (g_scrollMode == 1
           && localScrollMode == 1)
      && printedLines >= g_screenHeight && !g_outputToString)
      || backFromCmdInput) {
    // It requires a scrolling prompt

// step (2) perform scrolling

    while(1) {
      if (backFromCmdInput && backBufferPos == pntrLen(backBuffer))
        break; // Exhausted buffer
      if (backBufferPos < 1 || backBufferPos > pntrLen(backBuffer)) {
        // Warning:  Don't call bug(), because it calls print2.
        printf("*** BUG #1502 %ld\n", backBufferPos);
#if __STDC__
        fflush(stdout);
#endif
      }
      if (backBufferPos == 1) {
        printf(
"Press <return> for more, Q <return> to quit, S <return> to scroll to end... "
          );
#if __STDC__
        fflush(stdout);
#endif
      } else {
        printf(
"Press <return> for more, Q <ret> quit, S <ret> scroll, B <ret> back up... "
         );
#if __STDC__
        fflush(stdout);
#endif
      }
      c = (char)(getchar());
      if (c == '\n') {
        if (backBufferPos == pntrLen(backBuffer)) {
          // Normal output
          break;
        } else {
          // Get output from buffer
          backBufferPos++;
          printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
#if __STDC__
          fflush(stdout);
#endif
          continue;
        }
      }
      if (getchar() == '\n') {
        if (c == 'q' || c == 'Q') {
          if (!backFromCmdInput)
            g_quitPrint = 1;
          break;
        }
        if (c == 's' || c == 'S') {

          if (backBufferPos < pntrLen(backBuffer)) {
            // Print rest of buffer to screen
            while (backBufferPos + 1 <= pntrLen(backBuffer)) {
              backBufferPos++;
              printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
#if __STDC__
              fflush(stdout);
#endif
            }
          }
          if (!backFromCmdInput)
            localScrollMode = 0; // Continuous scroll
          break;
        }
        if (backBufferPos > 1) {
          if (c == 'b' || c == 'B') {
            backBufferPos--;
            printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
#if __STDC__
            fflush(stdout);
#endif
            continue;
          }
        }

        printf("%c", 7); // Bell
#if __STDC__
        fflush(stdout);
#endif
        continue;
      }
      while (c != '\n') c = (char)(getchar());
    } // While 1

    if (backFromCmdInput)
      goto PRINT2_RETURN;

// step (3) allocate a new page of output

    printedLines = 0; // Reset the number of lines printed on the screen
    if (!g_quitPrint) {
      backBufferPos++;
      pntrLet(&backBuffer, pntrAddElement(backBuffer));
      // Note: pntrAddElement() initializes the added element to the
      // empty string, so we don't need a separate initialization.
      // backBuffer[backBufferPos - 1] = ""; // already done
    }
  }

  // User typed 'q' above or earlier; don't return if we're outputting to
  // a string since we want to complete the writing to the string.
  if (g_quitPrint && !g_outputToString) goto PRINT2_RETURN;

// step (4) evaluate the output text

  // Allow unlimited output size
  va_start(ap, fmt);
  int bufsiz = vsnprintf(NULL, 0, fmt, ap); // Get the buffer size we need
  va_end(ap);
  // Warning: some older compilers, including lcc-win32 version 3.8 (2004),
  // return -1 instead of the buffer size.
  if (bufsiz == -1) bug(1527);
  printBuffer = malloc((size_t)bufsiz + 1);

  // Let each vs[n]printf have its own va_start...va_end
  // in an attempt to fix crash with some compilers (e.g. gcc 4.9.2).
  va_start(ap, fmt);
  // Put formatted string into buffer.
  charsPrinted = vsprintf(printBuffer, fmt, ap);
  va_end(ap);
  if (charsPrinted != bufsiz) {
    // Give some info with printf in case print2 crashes during bug() call
    printf("For bug #1528: charsPrinted = %ld != bufsiz = %ld\n", charsPrinted,
        (long)bufsiz);
    bug(1528);
  }

  nlpos = instr(1, printBuffer, "\n");
  lineLen = (long)strlen(printBuffer);

// step (5) revert QUOTED_SPACE to space

  // Change any ASCII 3's back to spaces, where they were set in
  // printLongLine to handle the broken quote problem.
  for (i = 0; i < lineLen; i++) {
    if (printBuffer[i] == QUOTED_SPACE) printBuffer[i] = ' ';
  }

  if ((lineLen > g_screenWidth + 1)
         && !g_outputToString /* for HTML */ ) {

// step (6) line wrapping

    // Force wrapping of lines that are too long by recursively calling
    // print2() via printLongLine().  Note:  "+ 1" above accounts for \n.
    // Note that breakMatch is "" so it may break in middle of a word.
    if (!nlpos) {
      // No end of line
      printLongLine(left(printBuffer, lineLen), "", "");
    } else {
      printLongLine(left(printBuffer, lineLen - 1), "", "");
    }
    goto PRINT2_RETURN;
  }

  if (!g_outputToString && !g_commandFileSilentFlag) {
    if (nlpos == 0) { // Partial line (usu. status bar) - print immediately

// step (7) print to screen, part 1

#ifdef __WATCOMC__
      cprintf("%s", printBuffer); // Immediate console I/O (printf buffers it)
#else
      printf("%s", printBuffer);
#endif

#if __STDC__
      // The following change to mminou.c was necessary on my Unix (Linux)
      // system to get the `verify proof *' progress bar to display
      // progressively (rather than all at once). I've conditioned it on
      // __STDC__, since it should be harmless on any ANSI C system.
      // -- Stephen McCamant  smccam @ uclink4.berkeley.edu 12-Sep-00
      fflush(stdout);
#endif
    } else {

// step (7) print to screen, part 2

      printf("%s", printBuffer); // Normal line
#if __STDC__
      fflush(stdout);
#endif
      printedLines++;
      if (!(g_scrollMode == 1 && localScrollMode == 1)) {

// step (8) address overflowed page

        // Even in non-scroll (continuous output) mode, still put paged-mode
        // lines into backBuffer in case user types a "B" command later,
        // so user can page back from end.
        if (printedLines > g_screenHeight) {
          printedLines = 1;
          backBufferPos++;
          pntrLet(&backBuffer, pntrAddElement(backBuffer));
          // Note: pntrAddElement() initializes the added element to the
          // empty string, so we don't need a separate initialization.
          // backBuffer[backBufferPos - 1] = ""; // already done
        }
      }
    }
    // Add line to backBuffer string array.
    // Warning:  Don't call bug(), because it calls print2.
    if (backBufferPos < 1) {
      printf("*** PROGRAM BUG #1504\n");
#if __STDC__
      fflush(stdout);
#endif
    }

// step (9) copy output to backBuffer

    let((vstring *)(&(backBuffer[backBufferPos - 1])), cat(
        (vstring)(backBuffer[backBufferPos - 1]), printBuffer, NULL));
  } // End if !g_outputToString

  if (g_logFileOpenFlag && !g_outputToString) {

// step (10) log output to file

    fprintf(g_logFilePtr, "%s", printBuffer); // Print to log file
#if __STDC__
    fflush(g_logFilePtr);
#endif
  }

  if (g_listMode && g_listFile_fp != NULL && !g_outputToString) {
    // Put line in list.tmp as comment
    fprintf(g_listFile_fp, "! %s", printBuffer); // Print to list command file
  }

  if (g_outputToString) {

// step (11) redirect output to a string

    let(&g_printString, cat(g_printString, printBuffer, NULL));
  }

  // Check for lines too long
  if (lineLen > g_screenWidth + 1) { // The +1 ignores \n
    // Warning:  Do not call bug(), because it calls print2.
    // If this bug occurs, the calling function should be fixed.
    printf("*** PROGRAM BUG #1505 (not serious, but please report it)\n");
    printf("Line exceeds screen width; caller should use printLongLine.\n");
    printf("%ld %s\n", lineLen, printBuffer);
#if __STDC__
    fflush(stdout);
#endif
  }
  // \n not allowed in middle of line
  // If this bug occurs, it means print2() is being called with \n in the
  // middle of the line and should be fixed in the caller.  printLongLine()
  // may be used if this is necessary.
  // Warning:  Don't call bug(), because it calls print2.
  if (nlpos != 0 && nlpos != lineLen) {
    printf("*** PROGRAM BUG #1506\n");
#if __STDC__
    fflush(stdout);
#endif
  }

  free(printBuffer);

 PRINT2_RETURN:
  return !g_quitPrint;
}

// printLongLine automatically puts a newline \n in the output line.
// startNextLine is the string to place before continuation lines.
// breakMatch is a list of characters at which the line can be broken.
// Special:  startNextLine starts with "~" means add tilde after broken line.
// Special:  breakMatch begins with "&" means compressed proof.
// Special:  breakMatch empty means break line anywhere.
// Special:  breakMatch begins with octal 1 means nested tree display (right
//           justify continuation lines); 1 is changed to space for
//           break matching.
// Special:  if breakMatch is \, then put % at end of previous line for LaTeX
// Special:  if breakMatch is " (quote), treat as if space but don't break
//           quotes, and also let lines grow long - use this call for all HTML
//           code.
void printLongLine(const char *line, const char *startNextLine, const char *breakMatch) {
  vstring_def(longLine);
  vstring_def(multiLine);
  vstring_def(prefix);
  vstring_def(startNextLine1);
  vstring_def(breakMatch1);
  long i, p;
  long startNextLineLen;
  flag firstLine;
  flag tildeFlag = 0;
  flag treeIndentationFlag = 0;
  flag quoteMode = 0; // 1 means inside quote
  long saveScreenWidth; // To let g_screenWidth grow temporarily

  long saveTempAllocStack;

  // Blank line (the rest of algorithm would ignore it; output and return)
  if (!line[0]) {
    // Do a dummy let() so caller can always depend on printLongLine
    // to empty the tempalloc string stack (for the rest of this code, the
    // first let() will do this).
    free_vstring(longLine);
    print2("\n");
    return;
  }

  // Change the stack allocation start to prevent arguments from being
  // deallocated.  We need to do this because more than one argument
  // may be passed in with cat(), chr(), etc. and let() can only grab
  // one, destroying the others.
  saveTempAllocStack = g_startTempAllocStack;
  g_startTempAllocStack = g_tempAllocStackTop; // For let() stack cleanup

  // Grab the input arguments
  let(&multiLine, line);
  let(&startNextLine1, startNextLine);
  let(&breakMatch1, breakMatch);
  // Now relax - back to normal; we can let temporary allocation stack die.
  g_startTempAllocStack = saveTempAllocStack;

  // We must copy input argument breakMatch to a variable string because we
  // will be zapping one of its characters, and ordinarily breakMatch is
  // passed in as a constant string.  However, this is now done with argument
  // grabbing above so we're OK.

  // Flag to right justify continuation lines
  if (breakMatch1[0] == 1) {
    treeIndentationFlag = 1;
    breakMatch1[0] = ' '; // Change to a space (the real break character)
  }

  // HTML mode
  // The HTML mode is intended not to break inside quoted HTML tag
  // strings.  All HTML output should be called with this mode.
  // Since we don't parse HTML this method is not perfect.  Only double
  // quotes are inspected, so all HTML strings with spaces must be
  // surrounded by double quotes.  If text quotes surround a tag
  // with a quoted string, this code will not work and must be
  // enhanced.
  // Whenever we are inside of a quote, we change a space to ASCII 3 to
  // prevent matching it.  The reverse is done in the print2() function,
  // where all ASCII 3's are converted back to space.
  // Note added 20-Oct-02: tidy.exe breaks HREF quotes with new line.
  // Check HTML spec - do we really need this code?
  if (breakMatch1[0] == '\"') {
    breakMatch1[0] = ' '; // Change to a space (the real break character)
    // Scan string for quoted strings
    quoteMode = 0;

    // Don't put line breaks in anything inside
    // _double_ quotes that follows an = sign, such as TITLE="abc def". Ignore
    // _single_ quotes (which could be apostrophes). The HTML output code
    // must be (and so far is) written to conform to this.
    i = 0;
    while (multiLine[i]) {
      if (multiLine[i] == '"' && i > 0) {
        if (!quoteMode && multiLine[i - 1] == '=')
          quoteMode = 1;
        else
          quoteMode = 0;
      }
      if (multiLine[i] == ' ' && quoteMode)
        multiLine[i] = QUOTED_SPACE;
      i++;
    }
  } // if (breakMatch1[0] == '\"')

  // The tilde is a special flag for printLongLine to print a
  // tilde before the carriage return in a split line, not after.
  if (startNextLine1[0] == '~') {
    tildeFlag = 1;
    let(&startNextLine1, " ");
  }

  while (multiLine[0]) { // While there are multiple caller-inserted newlines

    // Process caller-inserted newlines
    p = instr(1, multiLine, "\n");
    if (p) {
      // Get the next caller's line
      let(&longLine, left(multiLine, p - 1));
      let(&multiLine, right(multiLine, p + 1));
    } else {
      let(&longLine, multiLine);
      free_vstring(multiLine);
    }

    saveScreenWidth = g_screenWidth;
   HTML_RESTART:
    // Now we will break up one line from the caller i.e. longLine;
    // multiLine has any remaining lines to be processed in next pass.
    firstLine = 1;

    startNextLineLen = (long)strlen(startNextLine1);
    // Prevent infinite loop if next line prefix is longer than screen
    if (startNextLineLen > g_screenWidth - 4) {
      startNextLineLen = g_screenWidth - 4;
      let(&startNextLine1, left(startNextLine1, g_screenWidth - 4));
    }

    // If startNextLine starts with "~" means add tilde
    // after broken line (used for command input comment continuation); if
    // breakMatch is "\\" i.e. single \, then put % at end of previous line
    // for LaTeX.
    // Otherwise, if first line:  use length of longLine;
    // if not first line:  use length of longLine - startNextLineLen.
    while ((signed)(strlen(longLine)) + (1 - firstLine) * startNextLineLen >
        g_screenWidth - (long)tildeFlag - (long)(breakMatch1[0] == '\\')) {
      // Get screen width + 1 (default is 79 + 1)
      p = g_screenWidth - (long)tildeFlag - (long)(breakMatch1[0] == '\\') + 1;
      if (!firstLine) p = p - startNextLineLen;

      if (p < 4) bug(1524); // This may cause out-of-string ref below
      // Assume compressed proof if 1st char of breakMatch1 is "&"
      if (breakMatch1[0] == '&'
          && ((!instr(p, left(longLine, (long)strlen(longLine) - 3), " ")
              && longLine[p - 3] != ' ') // Don't split trailing "$."
            || longLine[p - 4] == ')')) /* Label sect ends in col 77 */ {
        // We're in the compressed proof section; break line anywhere
        p = p + 0; // Don't change position
        // In the case where the last space occurs at column 79 i.e.
        // g_screenWidth, break the line at column 78.  This can happen
        // when compressed proof ends at column 78, followed by space
        // and "$."  It prevents an extraneous trailing space on the line.
        if (longLine[p - 2] == ' ') p--;
      } else {
        if (!breakMatch1[0]) {
          p = p + 0; // Break line anywhere; don't change position
        } else {
          if (p <= 0) bug(1518);
          // For LaTeX, match space, not backslash
          // (Todo:  is backslash match mode really needed?)
          while (strchr(breakMatch1[0] != '\\' ? breakMatch1 : " ",
              longLine[p - 1]) == NULL) {
            p--;
            if (!p) break;
          }
          // We will now not break any line at non-space,
          // since it causes more problems that it solves e.g. with
          // WRITE SOURCE.../REWRAP with long URLs.
          if (p <= 0) {
            // The line couldn't be broken.  Since it's an HTML line, we
            // can increase g_screenWidth until it will fit.
            g_screenWidth++;
            // If this bug happens, we'll have to increase PRINTBUFFERSIZE
            // or change the HTML code being printed.
            // Ugly but another while loop nesting would be even more confusing.
            goto HTML_RESTART;
          }
        } // end if (!breakMatch1[0]) else
      } // end if (breakMatch1[0] == '&' &&... else

      if (p <= 0) {
        // Break character not found; give up at
        // g_screenWidth - (long)tildeFlag  - (long)(breakMatch1[0] == '\\')+ 1
        p = g_screenWidth - (long)tildeFlag  - (long)(breakMatch1[0] == '\\')+ 1;
        if (!firstLine) p = p - startNextLineLen;
        if (p <= 0) p = 1; // If startNextLine too long
      }
      if (!p) bug(1515); // p should never be 0 by this point
      // If we broke at a non-space 1st char, line length won't get reduced
      // Hopefully this will never happen with the breakMatch's we use,
      // otherwise the code will require a rework.
      if (p == 1 && longLine[0] != ' ') bug(1516);
      if (firstLine) {
        firstLine = 0;
        free_vstring(prefix);
      } else {
        let(&prefix, startNextLine1);
        if (treeIndentationFlag) {
          if (startNextLineLen + p - 1 < g_screenWidth) {
            // Right justify output for continuation lines
            let(&prefix, cat(prefix, space(g_screenWidth - startNextLineLen
                - p + 1), NULL));
          }
        }
      }
      if (!tildeFlag) {
        print2("%s\n",cat(prefix, left(longLine,p - 1), NULL));
      } else {
        print2("%s\n",cat(prefix, left(longLine,p - 1), "~", NULL));
      }
      if (longLine[p - 1] == ' ' &&
          breakMatch1[0] /* But not "break anywhere" line */) {
        // (Note:  search for "p--" ~100 lines above for the place
        // where the backward search for space happens.)
        // Remove leading space for neatness.
        if (longLine[p] == ' ') {
          // There could be 2 spaces at the end of a sentence.
          let(&longLine, right(longLine, p + 2));
        } else {
          let(&longLine, right(longLine,p + 1));
        }
      } else {
        let(&longLine, right(longLine,p));
      }
    } // end while longLine too long
    if (!firstLine) {
      if (treeIndentationFlag) {
        // Right justify output for continuation lines
        print2("%s\n",cat(startNextLine1, space(g_screenWidth
            - startNextLineLen - (long)(strlen(longLine))), longLine, NULL));
      } else {
        print2("%s\n",cat(startNextLine1, longLine, NULL));
      }
    } else {
      print2("%s\n",longLine);
    }
    g_screenWidth = saveScreenWidth; // Restore to normal
  } // end while multiLine != ""

  free_vstring(multiLine); // Deallocate
  free_vstring(longLine); // Deallocate
  free_vstring(prefix); // Deallocate
  free_vstring(startNextLine1); // Deallocate
  free_vstring(breakMatch1); // Deallocate

  return;
} // printLongLine

/*!
 * \def CMD_BUFFER_SIZE
 * Number of bytes allocated for prompted text, including the terminating NUL,
 * but excluding the return key stroke the user finishes her/his input with.
 */
#define CMD_BUFFER_SIZE 2000

vstring cmdInput(FILE *stream, const char *ask) {
  // This function prints a prompt (if 'ask' is not NULL) and gets a line from
  // the input stream. NULL is returned when end-of-file is encountered.
  // New memory is allocated each time linput is called. This space must
  // be freed by the caller.
  vstring_def(g); // Always init vstrings to "" for let(&...) to work
  long i;

  while (1) { // For "B" backup loop
    if (ask != NULL && !g_commandFileSilentFlag) {
      printf("%s", ask);
#if __STDC__
      fflush(stdout);
#endif
    }
    let(&g, space(CMD_BUFFER_SIZE)); // Allocate CMD_BUFFER_SIZE+1 bytes
    if (g[CMD_BUFFER_SIZE]) bug(1520); // Bug in let() (improbable)
    g[CMD_BUFFER_SIZE - 1] = 0; // For overflow detection
    if (!fgets(g, CMD_BUFFER_SIZE, stream)) {
      // End of file
      free_vstring(g); // Deallocate memory
      return NULL;
    }
    if (g[CMD_BUFFER_SIZE - 1]) {
      // Detect input overflow
      // Warning:  Don't call bug() - it calls print2 which may call this.
      printf("***BUG #1508\n");
#if __STDC__
      fflush(stdout);
#endif
    }
    i = (long)strlen(g);
/*E*/db = db - (CMD_BUFFER_SIZE - i); // Adjust string usage to detect leaks.
    // Detect operating system bug of inputting no characters.
    if (!i) {
      printf("***BUG #1507\n");
#if __STDC__
      fflush(stdout);
#endif
    } else {
      // The last line in the file has no new-line
      if (g[i - 1] != '\n') {
        // Warning:  Don't call bug() - it calls print2 which may call this.
        if (!feof(stream)) {
          printf("***BUG #1525\n");
#if __STDC__
          fflush(stdout);
#endif
        }
        // Add a new-line so processing below will behave correctly.
        let(&g, cat(g, chr('\n'), NULL));
/*E*/db = db + (CMD_BUFFER_SIZE - i); // Cancel extra piece of string
        i++;
      }
    }

    if (g[1]) {
      i--;
      if (g[i] != '\n') {
        printf("***BUG #1519\n");
#if __STDC__
        fflush(stdout);
#endif
      }
      g[i]=0; // Eliminate new-line character by zapping it
/*E*/db = db - 1;
    } else {
      if (g[0] != '\n') {
        printf("***BUG #1521\n");
#if __STDC__
        fflush(stdout);
#endif
      }
      // Eliminate new-line by deallocating vstring space (if we just zap
      // character [0], let() will later think g is an empty string constant
      // and will never deallocate g).
      free_vstring(g);
    }

    // If user typed "B" (for back), go back to the back buffer to
    // let the user scroll through it.
    if ((!strcmp(g, "B") || !strcmp(g, "b")) // User typed "B"
        // The back-buffer still exists and there was a previous page.
        && pntrLen(backBuffer) > 1
        && g_commandFileNestingLevel == 0
        && (g_scrollMode == 1 && localScrollMode == 1)
        && !g_outputToString) {
      // Set variables so only backup buffer will be looked at in print2()
      backBufferPos = pntrLen(backBuffer) - 1;
      printf("%s", (vstring)(backBuffer[backBufferPos - 1]));
#if __STDC__
      fflush(stdout);
#endif
      backFromCmdInput = 1; // Flag for print2()
      print2(""); // Only the backup buffer will be looked at
      backFromCmdInput = 0;
    } else {
      // If the command line is empty (at main prompt), let user still
      // type "B" for convenience in case too many
      // returns where hit while scrolling.

      // We're taking from a SUBMIT file so break out of loop that looks for "B".
      if (g_commandFileNestingLevel > 0) break;
      if (ask == NULL) {
        // In non-SUBMIT mode 'ask' won't be NULL, so flag non-fatal bug here just in case.
        printf("***BUG #1523\n");
#if __STDC__
        fflush(stdout);
#endif
      }
       // Command line not empty so break out of loop that looks for "B".
      if (g[0]) break;
      if (ask != NULL &&
          // User entered empty command line but not at a prompt.
          // g_commandPrompt is assigned in metamath.c and declared in
          // mmcmdl.h
          strcmp(ask, g_commandPrompt)) {
        break; // Break out of loop that looks for "B"
      }
    }
  } // while 1

  return g;
} // cmdInput

vstring cmdInput1(const char *ask) {
  // This function gets a line from either the terminal or the command file
  // stream depending on g_commandFileNestingLevel > 0.  It calls cmdInput().
  // Warning: the calling program must deallocate the returned string.
  vstring_def(commandLn);
  vstring_def(ask1);
  long p, i;
  // In case ask is temporarily allocated (i.e in case it will
  // become deallocated at next let().
  let(&ask1, ask);
  // Look for lines too long
  while ((signed)(strlen(ask1)) > g_screenWidth) {
    p = g_screenWidth - 1;
    while (ask1[p] != ' ' && p > 0) p--;
    if (!p) p = g_screenWidth - 1;
    print2("%s\n", left(ask1, p));
    let(&ask1, right(ask1, p + 1));
  }
  // Allow 10 characters for answer
  if ((signed)(strlen(ask1)) > g_screenWidth - 10) {
    p = g_screenWidth - 11;
    while (ask1[p] != ' ' && p > 0) p--;
    if (p) { // (Give up if no spaces)
      print2("%s\n", left(ask1, p));
      let(&ask1, right(ask1, p + 1));
    }
  }

  printedLines = 0; // Reset number of lines printed since last user input
  g_quitPrint = 0; // Reset quit print flag
  localScrollMode = 1; // Reset to prompted scroll

  while (1) {
    if (g_commandFileNestingLevel == 0) {
      commandLn = cmdInput(stdin, ask1);
      if (!commandLn) {
        commandLn = ""; // Init vstring (was NULL)
        // Allow ^D to exit
        if (strcmp(left(ask1, 2), "Do")) {
          // ^Z or ^D found at MM>, MM-PA>, or TOOLS> prompt
          let(&commandLn, "EXIT");
        } else {
          // Detected the question "Do you want to EXIT anyway (Y, N) <N>?"
          // Force exit with Y, to prevent infinite loop
          let(&commandLn, "Y");
        }
        printf("%s\n", commandLn); // Let user see what's happening
      }
      if (g_logFileOpenFlag) fprintf(g_logFilePtr, "%s%s\n", ask1, commandLn);

      // Clear backBuffer from previous scroll session
      for (i = 0; i < pntrLen(backBuffer); i++) {
        free_vstring(*(vstring *)(&backBuffer[i]));
      }
      backBufferPos = 1;
      free_pntrString(backBuffer);
      pntrLet(&backBuffer, pntrAddElement(backBuffer));
      // Note: pntrAddElement() initializes the added element to the
      // empty string, so we don't need a separate initialization.
      // backBuffer[backBufferPos - 1] = ""; // already done

      // Add user's typing to the backup buffer for display on 1st screen
      let((vstring *)(&(backBuffer[backBufferPos - 1])), cat(
          (vstring)(backBuffer[backBufferPos - 1]), ask1,
          commandLn, "\n", NULL));

      if (g_listMode && g_listFile_fp != NULL) {
        // Put line in list.tmp as comment
        fprintf(g_listFile_fp, "! %s\n", commandLn);
      }
    } else { // Get line from SUBMIT file
      commandLn = cmdInput(g_commandFilePtr[g_commandFileNestingLevel], NULL);
      if (!commandLn) { // EOF found
        fclose(g_commandFilePtr[g_commandFileNestingLevel]);
        print2("%s[End of command file \"%s\".]\n", ask1,
            g_commandFileName[g_commandFileNestingLevel]);
        // Deallocate string
        free_vstring(g_commandFileName[g_commandFileNestingLevel]);
        g_commandFileNestingLevel--;
        commandLn = "";
        if (g_commandFileNestingLevel == 0) {
          g_commandFileSilentFlag = 0;
        } else {
          // Revert to previous nesting level's silent flag
          g_commandFileSilentFlag = g_commandFileSilent[g_commandFileNestingLevel];
        }
        break; // continue;
      }

      // Tolerate CRs in SUBMIT files (e.g. created on Windows and run on Linux).
      let(&commandLn, edit(commandLn, 8192 /* remove CR */));

      print2("%s%s\n", ask1, commandLn);
    }
    break;
  }

  free_vstring(ask1);
  return commandLn;
} // cmdInput1

void errorMessage(vstring line, long lineNum, long column, long tokenLength,
  vstring error, vstring fileName, long statementNum, flag severity)
{
  // Note:  "line" may be terminated with \n.  "error" and "fileName"
  // should NOT be terminated with \n.  This is done for the convenience
  // of the calling functions.
  vstring_def(errorPointer);
  vstring_def(tmpStr);
  vstring_def(prntStr);
  vstring_def(line1);
  int j;

  // Prevent putting error message in g_printString.
  // 9-Jun-2016 nm Revert this change, because 'minimize_with' makes
  // use of the string to hold the DV violation error message.
  // We can reinstate this fix when 'minimize_with' is improved to
  // call a DV-checking function directly.

  // saveOutputToString = g_outputToString;
  // g_outputToString = 0;

  // Make sure vstring argument doesn't get deallocated with another let
// ??? USE SAVETEMPALLOC
  let(&tmpStr, error); // error will get deallocated here
  error = "";
  let(&error, tmpStr); // permanently allocate error

  // Add a newline to line1 if there is none
  if (line) {
    if (line[strlen(line) - 1] != '\n') {
      let(&line1, line);
    } else {
      bug(1509);
    }
  } else {
    line1 = NULL;
  }

  if (fileName) {
    // Put a blank line between error msgs if we are parsing a file
    print2("\n");
  }

  switch (severity) {
    case (char)notice_:
      let(&prntStr, "?Notice"); break;
    case (char)warning_:
      let(&prntStr, "?Warning"); break;
    case (char)error_:
      let(&prntStr, "?Error"); break;
    case (char)fatal_:
      let(&prntStr, "?Fatal error"); break;
  }
  if (lineNum) {
    let(&prntStr, cat(prntStr, " on line ", str((double)lineNum), NULL));
    if (fileName) {
      let(&prntStr, cat(prntStr, " of file \"", fileName, "\"", NULL));
    }
  } else {
    if (fileName) {
      let(&prntStr, cat(prntStr, " in file \"", fileName, "\"", NULL));
    }
  }
  if (statementNum) {
    let(&prntStr, cat(prntStr, " at statement ", str((double)statementNum), NULL));
    if (g_Statement[statementNum].labelName[0]) {
      let(&prntStr, cat(prntStr, ", label \"",
          g_Statement[statementNum].labelName, "\"", NULL));
    }
    let(&prntStr, cat(prntStr, ", type \"$", chr(g_Statement[statementNum].type),
        "\"", NULL));
  }
  printLongLine(cat(prntStr, ":", NULL), "", " ");
  if (line1) printLongLine(line1, "", "");
  if (line1 && column && tokenLength) {
    free_vstring(errorPointer);
    for (j=0; j<column-1; j++) {
      // Make sure that tabs on the line with the error are accounted for so
      // that the error pointer lines up correctly.
      if (line1[j] == '\t') let (&errorPointer,cat(errorPointer,"\t",NULL));
      else let(&errorPointer,cat(errorPointer," ",NULL));
    }
    for (j=0; j<tokenLength; j++)
      let(&errorPointer,cat(errorPointer,"^",NULL));
    printLongLine(errorPointer, "", "");
  }
  printLongLine(error,""," ");
  if (severity == 2) g_errorCount++;

  // ???Should there be a limit?
  // if (g_errorCount > 1000) {
  //  print2("\n"); print2("?Too many errors - aborting Metamath.\n");
  //  exit(0);
  // }

  // Restore output to g_printString if it was enabled before
  // 9-Jun-2016 nm Reverted

  // g_outputToString = saveOutputToString;

  if (severity == 3) {
    print2("Aborting Metamath.\n");
    exit(0);
  }
  free_vstring(errorPointer);
  free_vstring(tmpStr);
  free_vstring(prntStr);
  free_vstring(error);
  if (line1) free_vstring(line1);
} // errorMessage()

// Opens files with error message; opens output files with
// backup of previous version.   Mode must be "r" or "w" or "d" (delete).
FILE *fSafeOpen(const char *fileName, const char *mode, flag noVersioningFlag) {
  FILE *fp;
  vstring_def(prefix);
  vstring_def(postfix);
  vstring_def(bakName);
  vstring_def(newBakName);
  long v;
  long lastVersion; // Last version before gap

  if (!strcmp(mode, "r")) {
    fp = fopen(fileName, "r");
    if (!fp) {
      print2("?Sorry, couldn't open the file \"%s\".\n", fileName);
    }
    return fp;
  }

  if (!strcmp(mode, "w") || !strcmp(mode, "d")) {
    if (noVersioningFlag) goto skip_backup;
    // See if the file already exists.
    fp = fopen(fileName, "r");

    if (fp) {
      fclose(fp);

#define VERSIONS 9
      // The file exists. Rename it.

#if defined __WATCOMC__ // MSDOS
      // Make sure file name before extension is 8 chars or less
      i = instr(1, fileName, ".");
      if (i) {
        let(&prefix, left(fileName, i - 1));
        let(&postfix, right(fileName, i));
      } else {
        let(&prefix, fileName);
        free_vstring(postfix);
      }
      let(&prefix, cat(left(prefix, 5), "~", NULL));
      let(&postfix, cat("~", postfix, NULL));
      if (0) goto skip_backup; // Prevent compiler warning

#elif defined __GNUC__ // Assume unix
      let(&prefix, cat(fileName, "~", NULL));
      free_vstring(postfix);

#else // Unknown; assume unix standard
      // if (1) goto skip_backup; // [if no backup desired]
      let(&prefix, cat(fileName, "~", NULL));
      free_vstring(postfix);

#endif

      // See if the lowest version already exists.
      let(&bakName, cat(prefix, str(1), postfix, NULL));
      fp = fopen(bakName, "r");
      if (fp) {
        fclose(fp);
        // The lowest version already exists; rename all to higher versions.

        // Find last version before gap, if any
        lastVersion = 0;
        for (v = 1; v <= VERSIONS; v++) {
          let(&bakName, cat(prefix, str((double)v), postfix, NULL));
          fp = fopen(bakName, "r");
          if (!fp) break; // Version gap found; skip rest of scan
          fclose(fp);
          lastVersion = v;
        }

        // If there are no gaps before version VERSIONS, delete it.
        if (lastVersion == VERSIONS) {
          let(&bakName, cat(prefix, str((double)VERSIONS), postfix, NULL));
          fp = fopen(bakName, "r");
          if (fp) {
            fclose(fp);
            remove(bakName);
          }
          lastVersion--;
        }

        for (v = lastVersion; v >= 1; v--) {
          let(&bakName, cat(prefix, str((double)v), postfix, NULL));
          fp = fopen(bakName, "r");
          if (!fp) continue;
          fclose(fp);
          let(&newBakName, cat(prefix, str((double)v + 1), postfix, NULL));
          rename(bakName /* old */, newBakName /* new */);
        }
      }
      let(&bakName, cat(prefix, str(1), postfix, NULL));
      rename(fileName, bakName);

      // printLongLine(cat("The file \"", fileName,
      //     "\" already exists.  The old file is being renamed to \"",
      //     bakName, "\".", NULL), "  ", " ");
    } // End if file already exists
   skip_backup:

    if (!strcmp(mode, "w")) {
      fp = fopen(fileName, "w");
      if (!fp) {
        print2("?Sorry, couldn't open the file \"%s\".\n", fileName);
      }
    } else {
      if (strcmp(mode, "d")) {
        bug(1526);
      }
      // For "safe" delete, the file was renamed to ~1, so there is nothing to do.
      fp = NULL;
      // For non-safe (noVersioning) delete, we actually delete
      if (noVersioningFlag) {
        if(remove(fileName) != 0) {
          print2("?Sorry, couldn't delete the file \"%s\".\n", fileName);
        }
      }
    }

    // Deallocate local strings
    free_vstring(prefix);
    free_vstring(postfix);
    free_vstring(bakName);
    free_vstring(newBakName);

    return fp;
  } // End if mode = "w" or "d"

  bug(1510); // Illegal mode
  return NULL;
} // fSafeOpen()

// Renames a file with backup of previous version. If non-zero
// is returned, there was an error.
int fSafeRename(const char *oldFileName, const char *newFileName) {
  int error = 0;
  int i;
  FILE *fp;
  // Open the new file to force renaming of existing ones
  fp = fSafeOpen(newFileName, "w", 0 /* noVersioningFlag */);
  if (!fp) error = -1;
  // Delete the file just created
  if (!error) {
    error = fclose(fp);
    if (error) print2("?Empty \"%s\" couldn't be closed.\n", newFileName);
  }
  if (!error) {
    error = remove(newFileName);
    // On Windows 95, the first attempt may not succeed.
    if (error) {
      for (i = 2; i < 1000; i++) {
        error = remove(newFileName);
        if (!error) break;
      }
      if (!error)
        print2("OS WARNING: File delete succeeded only after %i attempts.", i);
    }

    if (error) print2("?Empty \"%s\" couldn't be deleted.\n", newFileName);
  }
  // Rename the old one to it
  if (!error) {
    error = rename(oldFileName, newFileName);
    if (error) print2("?Rename of \"%s\" to \"%s\" failed.\n", oldFileName,
        newFileName);
  }
  if (error) {
    print2("?Sorry, couldn't rename the file \"%s\" to \"%s\".\n", oldFileName,
        newFileName);
    print2("\"%s\" may be empty; try recovering from \"%s\".\n", newFileName,
        oldFileName);
  }
  return error;
} // fSafeRename

// Finds the name of the first file of the form filePrefix +
// nnn + ".tmp" that does not exist.  THE CALLER MUST DEALLOCATE
// THE RETURNED STRING [i.e. assign function return directly
// to a local empty vstring with = and not with let(), e.g.
//      free_vstring(str1);
//      str1 = fTmpName("zz~list");  ]
// The file whose name is the returned string is not left open;
// the caller must separately open the file.
vstring fGetTmpName(const char *filePrefix) {
  FILE *fp;
  vstring_def(fname);
  static long counter = 0;
  while (1) {
    counter++;
    let(&fname, cat(filePrefix, str((double)counter), ".tmp", NULL));
    fp = fopen(fname, "r");
    if (!fp) break;
    fclose(fp);

    if (counter > 1000) {
      print2("?Warning: too many %snnn.tmp files - will reuse %s\n",
          filePrefix, fname);
      break;
    }
  }
  return fname; // Caller must deallocate!
} // fGetTmpName()

// This function returns a character string containing the entire contents of
// an ASCII file, or Unicode file with only ASCII characters. On some
// systems it is faster than reading the file line by line. The caller
// must deallocate the returned string. If a NULL is returned, the file
// could not be opened or had a non-ASCII Unicode character or some other
// problem. If verbose is 0, error and warning messages are suppressed.
vstring readFileToString(const char *fileName, char verbose, long *charCount) {
  FILE *inputFp;
  long fileBufSize;
  char *fileBuf;
  long i, j;

  // Find out the upper limit of the number of characters in the file.
  // Do this by opening the file in binary and seeking to the end.
  inputFp = fopen(fileName, "rb");
  if (!inputFp) {
    if (verbose) print2("?Sorry, couldn't open the file \"%s\".\n", fileName);
    return NULL;
  }
#ifndef SEEK_END
// An older GCC compiler didn't have this ANSI standard constant defined.
#define SEEK_END 2
#endif
  if (fseek(inputFp, 0, SEEK_END)) { // fseek returns non-zero on error
    // Error message which occurs if input "file" is a piped stream
    if (verbose) print2(
        "?Sorry, \"%s\" doesn't seem to be a regular file.\n",
        fileName);
    return NULL;
  }
  fileBufSize = ftell(inputFp);

  // Close and reopen the input file in text mode.
  // Text mode is needed for VAX, DOS, etc. with non-Unix end-of-lines.
  fclose(inputFp);
  inputFp = fopen(fileName, "r");
  if (!inputFp) bug(1512);

  // Allocate space for the entire input file
  // Add a factor for unknown text formats (just a guess)
  fileBufSize = fileBufSize + 10;

  fileBuf = malloc((size_t)fileBufSize);
  if (!fileBuf) {
    if (verbose) print2(
        "?Sorry, there was not enough memory to read the file \"%s\".\n",
        fileName);
    fclose(inputFp);
    return NULL;
  }

  // Put the entire input file into the buffer as a giant character string
  (* charCount) = (long)fread(fileBuf, sizeof(char), (size_t)fileBufSize - 2,
      inputFp);
  if (!feof(inputFp)) {
    print2("Note:  This bug will occur if there is a disk file read error.\n");
    // If this bug occurs (due to obscure future format such as compressed
    // text files) we'll have to add a realloc statement.
    bug(1513);
  }
  fclose(inputFp);

  fileBuf[(*charCount)] = 0;

  // See if it's Unicode
  // This only handles the case where all chars are in the ASCII subset
  if ((*charCount) > 1) {
    if (fileBuf[0] == '\377' && fileBuf[1] == '\376') {
      // Yes, so strip out null high-order bytes
      if (2 * ((*charCount) / 2) != (*charCount)) {
        if (verbose) print2(
"?Sorry, there are an odd number of characters (%ld) %s \"%s\".\n",
            (*charCount), "in Unicode file", fileName);
        free(fileBuf);
        return NULL;
      }
      i = 0; // ASCII character position
      j = 2; // Unicode character position
      while (j < (*charCount)) {
        if (fileBuf[j + 1] != 0) {
          if (verbose) print2(
              "?Sorry, the Unicode file \"%s\" %s %ld at byte %ld.\n",
              fileName, "has a non-ASCII \ncharacter code",
              (long)(fileBuf[j]) + ((long)(fileBuf[j + 1]) * 256), j);
          free(fileBuf);
          return NULL;
        }
        if (fileBuf[j] == 0) {
          if (verbose) print2(
              "?Sorry, the Unicode file \"%s\" %s at byte %ld.\n",
              fileName, "has a null character", j);
          free(fileBuf);
          return NULL;
        }
        fileBuf[i] = fileBuf[j];
        // Suppress any carriage-returns
        if (fileBuf[i] == '\r') {
          i--;
        }
        i++;
        j = j + 2;
      }
      fileBuf[i] = 0; // ASCII string terminator
      (*charCount) = i;
    }
  }

  // Make sure the file has no carriage-returns
  if (strchr(fileBuf, '\r') != NULL) {
    if (verbose) print2(
       "?Warning: the file \"%s\" has carriage-returns.  Cleaning them up...\n",
        fileName);
    // Clean up the file, e.g. DOS or Mac file on Unix
    i = 0;
    j = 0;
    while (j <= (*charCount)) {
      if (fileBuf[j] == '\r') {
        if (fileBuf[j + 1] == '\n') {
          // DOS file - skip '\r'
          j++;
        } else {
          // Mac file - change '\r' to '\n'
          fileBuf[j] = '\n';
        }
      }
      fileBuf[i] = fileBuf[j];
      i++;
      j++;
    }
    (*charCount) = i - 1;
  }

  // Make sure the last line is not a partial line
  if (fileBuf[(*charCount) - 1] != '\n') {
    if (verbose) print2(
        "?Warning: the last line in file \"%s\" is incomplete.\n",
        fileName);
    // Add the end-of-line
    fileBuf[(*charCount)] = '\n';
    (*charCount)++;
    fileBuf[(*charCount)] = 0;
  }

  if (fileBuf[(*charCount)] != 0) {
    bug(1522); // Keeping track of charCount went wrong somewhere
  }

  // Make sure there aren't null characters
  i = (long)strlen(fileBuf);
  if ((*charCount) != i) {
    if (verbose) {
      print2(
          "?Warning: the file \"%s\" is not an ASCII file.\n",
          fileName);
      print2(
          "Its size is %ld characters with null at character %ld.\n",
          (*charCount), strlen(fileBuf));
    }
  }
/*E*/db = db + i; // For memory usage tracking (ignore stuff after null)

  //******* For debugging
  // print2("In binary mode the file has %ld bytes.\n", fileBufSize - 10);
  // print2("In text mode the file has %ld bytes.\n", (*charCount));
  //*******

  return (char *)fileBuf;
} // readFileToString

// Returns total elapsed time in seconds since starting session (for the
// lcc compiler) or the CPU time used (for the gcc compiler). The
// argument is assigned the time since the last call to this function.
double getRunTime(double *timeSinceLastCall) {
#ifdef CLOCKS_PER_SEC
  static clock_t timePrevious = 0;
  clock_t timeNow;
  timeNow = clock();
  *timeSinceLastCall = (double)((1.0 * (double)(timeNow - timePrevious))
          /CLOCKS_PER_SEC);
  timePrevious = timeNow;
  return (double)((1.0 * (double)timeNow)/CLOCKS_PER_SEC);
  // Example of printing double format:
  // print2("Total elapsed time = %4.2f s\n", t)
#else
  print2("The clock() function is not implemented on this computer.\n");
  *timeSinceLastCall = 0;
  return 0;
#endif
}

void freeInOu(void) {
  long i, j;
  j = pntrLen(backBuffer);
  for (i = 0; i < j; i++) free_vstring(*(vstring *)(&backBuffer[i]));
  free_pntrString(backBuffer);
}
