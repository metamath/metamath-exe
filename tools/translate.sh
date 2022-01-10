#!/bin/sh
# Shell script helper for translating Metamath tool files to bash
# Mario Carneiro 7-Jan-2022

usage() {
  cat << HELP
TRANSLATE - Translate a TOOLS script into a corresponding bash script

This command will take a script file intended for use with metamath
tools-mode, and translate it into a bash script with the same behavior.
See also SUBMIT. The script is passed the location of the tools
directory (containing the core script files), which is also the location
of this file. It uses 'mm-tools' by default, so if you symlink 'mm-tools'
to the tools directory then these scripts will work.

If the -r (recursive) option is passed, the generated file will translate
SUBMIT calls to also make use of the same TOOLS directory; otherwise
it will leave them off (using the default "mm-tools" directory).

If the -e (echo) option is passed, the generated file will contain an
echo statement for each executed command.

Syntax:  translate.sh [-r] [-e] [TOOLS] < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi

if [ "$1" = "-r" ]; then recursive=1; shift; fi
if [ "$1" = "-e" ]; then echo=1; shift; fi

if [ $# -eq 0 ]; then tools="mm-tools"
elif [ $# -eq 1 ]; then tools=$1
else usage; exit 1; fi

echo "#!/bin/bash"
awk -v tools="$tools" -v echo="$echo" -v recursive="$recursive" \
'BEGIN {
  if (recursive) {
    extraArg = tools;
    gsub(/[\\$"`]/, "\\\\&", extraArg);
    extraArg = " \"" extraArg "\"";
  }
  # These are commands of the form CMD <file> <args...>
  # which have to be replaced with inplace.sh <file> cmd.sh <args...>
  inplaceCommands["ADD"] = 1;
  inplaceCommands["DELETE"] = 1;
  inplaceCommands["SUBSTITUTE"] = 1;
  inplaceCommands["SWAP"] = 1;
  inplaceCommands["INSERT"] = 1;
  inplaceCommands["BREAK"] = 1;
  inplaceCommands["BUILD"] = 1;
  inplaceCommands["MATCH"] = 1;
  inplaceCommands["SORT"] = 1;
  inplaceCommands["UNDUPLICATE"] = 1;
  inplaceCommands["DUPLICATE"] = 1;
  inplaceCommands["UNIQUE"] = 1;
  inplaceCommands["REVERSE"] = 1;
  inplaceCommands["RIGHT"] = 1;
  inplaceCommands["PARALLEL"] = 1;
  inplaceCommands["TAG"] = 1;
} {
  if ($0 == "") print;
  else if (substr($0, 1, 1) == "!")
    print "#" substr($0, 2);
  else if (toupper($1) == "TOOLS")
    ;
  else {
    $1 = toupper($1);
    if ($1 == "EXIT" || $1 == "QUIT") exit;
    if ($1 == "C") $1 = "COPY"; else
    if ($1 == "B") $1 = "BEEP"; else
    if ($1 == "S") $1 = "SUBSTITUTE"; else
    if ($1 == "T") $1 = "TAG";
    if (echo) {
      echoLine = $0;
      gsub(/[\\$"`]/, "\\\\&", echoLine);
      print "echo \"" echoLine "\"";
    }
    file = tools "/" tolower($1) ".sh";
    if (inplaceCommands[$1]) {
      infile = $2;
      $2 = file;
      $1 = tools "/inplace.sh " infile;
    } else if ($1 == "BEEP") {
      $1 = file;
    } else if ($1 == "TYPE") {
      for (i = 3; i < NR; i++) file = file " " $i;
      $0 = file " < " $2;
    } else if ($1 == "SUBMIT") {
      args = "";
      for (i = 3; i < NR; i++) args = args " " $i;
      gsub(" ", "", args);
      if (tolower(args) == "/silent") file = file " -q";
      $0 = file extraArg " < " $2;
    } else if ($1 == "NUMBER") {
      $0 = file $3 $4 $5 " > " $2;
    } else if ($1 == "COPY") {
      out = $(NR-1);
      for (i = 2; i < NR-1; i++)
        out = out " " $i;
      $0 = file " " out;
    } else if ($1 == "CLEAN") {
      gsub(",", " ", $3);
      infile = $2;
      $2 = file;
      $1 = tools "/inplace.sh " infile;
    } else if ($1 == "HELP" || $1 == "COUNT" || $1 == "UPDATE") {
      $0 = "# " $1 " command not supported";
    } else {
      gsub(/[\\$"`]/, "\\\\&");
      $0 = "# unknown command: \"" $0 "\"";
    }
    if (echo) {
      echoLine = $0;
      gsub(/[\\$"`]/, "\\\\&", echoLine);
      print "echo \" => " echoLine "\"";
    }
    print;
    fflush();
  }
}'
