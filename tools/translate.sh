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
  len = length(specChars);
  split(specChars, specCharsArr, "");
  if (recursive) {
    extraArg = tools;
    gsub(/[\\$"]/, "\\\\&", extraArg);
    extraArg = " \"" extraArg "\"";
  }
} {
  if ($0 == "") print;
  else if (substr($0, 1, 1) == "!")
    print "#" substr($0, 2);
  else if (toupper($1) == "TOOLS")
    ;
  else {
    $1 = toupper($1);
    switch ($1) {
      case "C": $1 = "COPY"; break;
      case "B": $1 = "BEEP"; break;
      case "S": $1 = "SUBSTITUTE"; break;
      case "T": $1 = "TAG"; break;
      case "EXIT":
      case "QUIT":
        exit;
      default: break;
    }
    if (echo) {
      echoLine = $0;
      gsub(/[\\$"]/, "\\\\&", echoLine);
      print "echo \"" echoLine "\"";
    }
    file = tools "/" tolower($1) ".sh";
    switch ($1) {
      case "HELP":
      case "COUNT":
      case "UPDATE":
        $0 = "# " $1 " command not supported";
        break;
      case "BEEP":
        $1 = file;
        break;
      case "TYPE":
        for (i = 3; i < NR; i++) file = file " " $i;
        $0 = file " < " $2;
        break;
      case "SUBMIT":
        args = "";
        for (i = 3; i < NR; i++) args = args " " $i;
        gsub(" ", "", args);
        if (tolower(args) == "/silent") file = file " -q";
        $0 = file extraArg " < " $2;
        break;
      case "NUMBER":
        $0 = file $3 $4 $5 " > " $2;
        break;
      case "COPY":
        out = $(NR-1);
        for (i = 2; i < NR-1; i++)
          out = out " " $i;
        $0 = file " " out;
        break;
      case "CLEAN":
        gsub(",", " ", $3);
        infile = $2;
        $2 = file;
        $1 = tools "/inplace.sh " infile;
        break;
      case "ADD":
      case "DELETE":
      case "SUBSTITUTE":
      case "SWAP":
      case "INSERT":
      case "BREAK":
      case "BUILD":
      case "MATCH":
      case "SORT":
      case "UNDUPLICATE":
      case "DUPLICATE":
      case "UNIQUE":
      case "REVERSE":
      case "RIGHT":
      case "PARALLEL":
      case "TAG":
        infile = $2;
        $2 = file;
        $1 = tools "/inplace.sh " infile;
        break;
      default:
        gsub(/"/, "\\\\&");
        $0 = "# unknown command: \"" $0 "\"";
        break;
    }
    if (echo) {
      echoLine = $0;
      gsub(/[\\$"]/, "\\\\&", echoLine);
      print "echo \" => " echoLine "\"";
    }
    print;
    fflush();
  }
}'
