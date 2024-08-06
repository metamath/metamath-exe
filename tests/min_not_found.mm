$( A shortening not found by the minimizer.  The comparison with min_found.mm
   suggests that the minimizer's inability to detect it is due to the absence
   of statement "C" in any of the steps of the minimized proof.  The
   shortening is detected if proveFloating is set to 1 in replaceStatement
   called by minimizeProof, which allows absent $e statements to be found. $)

  $c A B C D $.

  ax-1 $a A $.
  ax-2 $a B $.
  ax-3 $a C $.

  ${
    in1.1 $e A $.
    in1 $a D $.
  $}

  ${
    in2.1 $e B $.
    in2.2 $e C $.
    in2.3 $e B $.
    in2.4 $e B $.
    in2.5 $e B $.
    in2 $a D $.
  $}

  $( Minimized proof, not found automatically.  (New usage is discouraged.) $)
  th1 $p D $= ( ax-1 in1 ) AB $.

  $( Non-minimized proof. $)
  th2 $p D $= ( ax-2 ax-3 in2 ) ABAAAC $.
