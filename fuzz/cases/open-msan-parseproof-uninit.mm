$( Test database for proofs saved in /EXPLICIT format, where each step
   is written as <target hypothesis>=<source>.  Parsing those targets
   and rearranging the hypotheses to match is a distinct code path in
   parseProof(), which no other test exercises. $)

$c wff |- ( -> ) $.
$v p q $.
wp $f wff p $.
wq $f wff q $.
wim $a wff ( p -> q ) $.
${
  mi $e |- p $.
  mj $e |- ( p -> q ) $.
  mp $a |- q $.
$}
a1 $a |- p $.
a2 $a |- ( p -> q ) $.

$( A theorem whose proof is stored in /EXPLICIT form. $)
th $p |- q $= wp=wp wq=wq wp=wp mi=a1 wp=wp wq=wq mj=a2 th=mp $.
