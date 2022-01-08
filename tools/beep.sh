#!/bin/bash
# Shell script equivalent of Metamath BEEP tool
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
BEEP - Make a beep sound

This command will produce a beep.  By typing it ahead after a long-
running command has started, it will alert you that the command is
finished.
Syntax: beep.sh
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -ne 0 ]; then usage; exit 1; fi

beep
