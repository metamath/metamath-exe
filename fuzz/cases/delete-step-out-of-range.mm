$( "DELETE STEP" with a step number past the end of the proof.  metamath.c
   read g_ProofInProgress.proof[s - 1] to see whether the step was unknown
   before anything checked that step s existed. $)

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
th $p |- q $= wp wq a1 a2 mp $.
