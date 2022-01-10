#!/bin/sh
# Shell script equivalent of Metamath CLEAN tool
# Mario Carneiro 5-Jan-2022

usage() {
  cat << HELP
CLEAN - Trim spaces and tabs on each line in a file; convert characters

This command processes spaces and tabs in each line of a file
according to the following subcommands:
  D - Delete all spaces and tabs
  B - Delete spaces and tabs at the beginning of each line
  E - Delete spaces and tabs at the end of each line
  R - Reduce multiple spaces and tabs to one space
  Q - Do not alter characters in quotes (ignored by T and U)
  T - (Tab) Convert spaces to equivalent tabs (unsupported)
  U - (Untab) Convert tabs to equivalent spaces (unsupported)
Some other subcommands are also available:
  P - Clear parity (8th) bit from each character
  G - Discard garbage characters CR,FF,ESC,BS
  C - Convert to upper case
  L - Convert to lower case
  V - Convert VT220 screen print frame graphics to -,|,+ characters
Subcommands are separated by spaces, e.g., B E R Q
Syntax:  clean.sh subcmd subcmd ... < INFILE > OUTFILE
HELP
}

if [ $# -eq 1 ] && [ "$1" = "-h" -o "$1" = "--help" ]; then usage; exit; fi
if [ $# -eq 0 ]; then usage; exit 1; fi

for var in "$@"; do
  case $var in
    D | d) allDiscard=1;;
    B | b) leadDiscard=1;;
    E | e) trailDiscard=1;;
    R | r) reduce=1;;
    Q | q) quote=1;;
    T | t) echo "T (tab) flag unsupported"; exit 2;;
    U | u) echo "U (untab) flag unsupported"; exit 2;;
    P | p) clearParity=1;;
    G | g) discardCtrl=1;;
    C | c) uppercase=1;;
    L | l) lowercase=1;;
    V | v) screen=1;;
  esac
done

awk \
  -v clearParity=$clearParity \
  -v allDiscard=$allDiscard \
  -v discardCtrl=$discardCtrl \
  -v leadDiscard=$leadDiscard \
  -v reduce=$reduce \
  -v uppercase=$uppercase \
  -v trailDiscard=$trailDiscard \
  -v quote=$quote \
  -v lowercase=$lowercase \
  -v tab=$tab \
  -v untab=$untab \
  -v screen=$screen \
'BEGIN {
  FS="";
  if (clearParity || uppercase || lowercase) {
    for (n=0; n<256; n++) {
      ch = sprintf("%c", n);
      chr[n] = ch;
      ord[ch] = n;
    }
  }
  if (discardCtrl) {
    discardArr["\r"] = 1;
    discardArr["\n"] = 1;
    discardArr["\f"] = 1;
    discardArr["\x27"] = 1;
    discardArr["\x08"] = 1;
  }
  if (screen) {
    gfxArr["\xEA"] = gfxArr["\xEB"] = gfxArr["\xEC"] = gfxArr["\xED"] = "+";
    gfxArr["\xDA"] = gfxArr["\xD9"] = gfxArr["\xBF"] = gfxArr["\xC0"] = "+";
    gfxArr["\xF1"] = gfxArr["\xC4"] = "-";
    gfxArr["\xF8"] = gfxArr["\xA6"] = gfxArr["\xB3"] = "|";
  }
} {
  if (leadDiscard)
    gsub(/^[ \t]+/, "");
  inQuote = 0;
  out = "";
  for (i=1;i<=NF;i++) {
    if (quote && ($i == "\"" || $i == "\x39"))
      inQuote = !inQuote;
    if (inQuote) {
      out=out $i;
      continue;
    }
    if (allDiscard && ($i == " " || $i == "\t")) continue;
    if (clearParity) $i = chr[and(ord[$i], 0x7f)];
    if (discardCtrl && discardArr[$i]) continue;
    if (uppercase && ord[$i] < 128) $i = toupper($i);
    if (lowercase && ord[$i] < 128) $i = tolower($i);
    if (screen && gfxArr[$i]) $i = gfxArr[$i];
    out=out $i;
  }
  if (trailDiscard)
    gsub(/[ \t]+$/, "", out);
  if (reduce)
    gsub(/[ \t]+/, " ", out);
  print out;
}'
