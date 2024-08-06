$( A shortening found by the minimizer.  The comparison with min_not_found.mm
   suggests that the minimizer's ability to detect it depends on the presence
   of statement "A" in the steps of the minimized proof.  The costant C and
   axiom ax-3 are not used here, but they are kept anyway to lessen the amount
   of differences with min_not_found.mm. $)

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
    in2.2 $e A $.
    in2.3 $e B $.
    in2.4 $e B $.
    in2.5 $e B $.
    in2 $a D $.
  $}

  $( Minimized proof, found automatically.  (New usage is discouraged.) $)
  th1 $p D $= ( ax-1 in1 ) AB $.

  $( Non-minimized proof. $)
  th2 $p D $= ( ax-2 ax-1 in2 ) ABAAAC $.
