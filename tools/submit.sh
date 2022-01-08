#!/bin/sh
# Shell script equivalent of Metamath SUBMIT tool
# Mario Carneiro 5-Jan-2022

usage() {
  cat << HELP
SUBMIT - Run a script containing Tools commands

Syntax:  submit.sh [-q] [TOOLS] < INFILE

This command causes further command lines to be taken from the specified
file.  Note that any line beginning with an exclamation point (!) is
treated as a comment (i.e. ignored).  Also note that the scrolling
of the screen output is continuous.

The script is passed the location of the tools directory (containing the
core script files), which is also the location of this file. It uses
'mm-tools' by default, so if you symlink 'mm-tools'
to the tools directory then these scripts will work.

SUBMIT can be called recursively, i.e., SUBMIT commands are allowed
inside of a command file.
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi

if [ "$1" = "-q" ]; then args=""; shift; else args="-e"; fi
if [ $# -eq 0 ]; then tools="mm-tools"
elif [ $# -eq 1 ]; then tools=$1
else usage; exit 1; fi

"$tools/translate.sh" -r $args "$tools" | bash
