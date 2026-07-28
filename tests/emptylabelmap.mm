$( Test database for compressed proofs whose label reference is outside
   the label list.

   parseCompressedProof() clamps an out-of-range compressed label
   reference to index 0, "something legal to avoid side effects".
   Index 0 is only legal when the label map has at least one entry,
   and it can have none: the map holds the statement's required
   hypotheses followed by the labels in the "( )" list, and both can
   be empty.  Reading the map anyway returned uninitialized heap and
   stored it in the proof.

   Both statements below are provable; only their proofs are broken. $)

$c |- T $.

ax $a |- T $.

$( "th1" has no required hypotheses and an empty "( )" list, so its
   label map is empty and the "A" refers to nothing at all. $)
th1 $p |- T $= ( ) A $.

$( "th2" has a one-entry label map, so "B" is out of range but index 0
   is a real entry and the clamp has something to clamp to.  This pins
   that long-standing behavior down: the step becomes "ax". $)
th2 $p |- T $= ( ax ) B $.
