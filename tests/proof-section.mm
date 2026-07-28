$( Test database for a "$p" whose "$=" is missing.

   parseKeywords() reports "Expected \"$=\" here." and then used to open
   a proof section at the offending keyword anyway.  On "$p ... $$." the
   next "$" is the very next character, so the proof section was closed
   before it started and its recorded length came out as -1.

   Consumers take that at face value: space(-1) clamps to an empty
   string and memcpy() is then handed (size_t)-1 bytes, and parseProof()
   sizes its work arrays from the same negative length.  SHOW PROOF,
   SAVE PROOF and WRITE SOURCE / EXTRACT each crashed a different way.

   "z3" is here so that WRITE SOURCE / EXTRACT gets past tracing the
   proofs and reaches the statement output loop, which is where the
   memcpy is. $)

$c |- T $.

ax $a |- T $.

$( "z2" has no "$=" at all, so no proof section should be recorded for
   it and its length must stay 0. $)
z2 $p |- T $$.

$( An ordinary $p, so the extract has something it can trace. $)
z3 $p |- T $= ( ax ) A $.
