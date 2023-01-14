$( demo0-includee.mm  1-Jan-04 $)

$(
                      PUBLIC DOMAIN DEDICATION

This file is placed in the public domain per the Creative Commons Public
Domain Dedication. http://creativecommons.org/licenses/publicdomain/

Norman Megill
$)

$( This file is the introductory formal system example described
   in Chapter 2 of the Meamath book. $)

$( Declare the constant symbols we will use $)
    $c 0 + = -> ( ) term wff |- $.
$( Declare the metavariables we will use $)
    $v t r s P Q $.
$( Specify properties of the metavariables $)
    tt $f term t $.
    tr $f term r $.
    ts $f term s $.
    wp $f wff P $.
    wq $f wff Q $.
$( Define "term" (part 1) $)
    tze $a term 0 $.
$( Define "term" (part 2) $)
    tpl $a term ( t + r ) $.
$( Define "wff" (part 1) $)
    weq $a wff t = r $.
$( Define "wff" (part 2) $)
    wim $a wff ( P -> Q ) $.
$( State axiom a1 $)
    a1 $a |- ( t = r -> ( t = s -> r = s ) ) $.
$( State axiom a2 $)
    a2 $a |- ( t + 0 ) = t $.
    ${
       min $e |- P $.
       maj $e |- ( P -> Q ) $.
$( Define the modus ponens inference rule $)
       mp  $a |- Q $.
    $}

