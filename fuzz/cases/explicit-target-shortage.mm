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
th $p |- q $= wp=wp wq a1 a2 mp $.
