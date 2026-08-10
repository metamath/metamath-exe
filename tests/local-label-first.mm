$( Test database for a local label character "Z" that occurs before any
   proof step.

   parseCompressedProof() appends a label map entry for every "Z", using
   -1000 - (step - 1) to name the step the label stands for.  With no
   step yet that is -1000 - (0 - 1), or -999.  A local label reference
   is -1000 or less, so -999 is not one, and a later label that resolved
   to it made verifyProof() abort with bug(2101).

   All three statements below are provable; only their proofs are
   broken. $)

$c |- T $.

ax $a |- T $.

$( "z1" declares a local label before any proof step, then refers to it.
   The "A" resolves to the entry the leading "Z" left behind. $)
z1 $p |- T $= ( ) ZA $.

$( "z2" is the same with a non-empty "( )" list, so the bad entry sits
   at index 1 rather than index 0. $)
z2 $p |- T $= ( ax ) ZB $.

$( "z3" has its "Z" in the ordinary position, after a proof step, so its
   entry stays a real local label reference and the "B" resolves to it.
   The proof still fails, on the RPN stack, but only after the local
   label was accepted.  A "Z" inside a proof that verifies is covered by
   big-unifier.mm. $)
z3 $p |- T $= ( ax ) AZB $.
