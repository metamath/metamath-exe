$( A shortening found by the minimizer.  The comparison with min-not-found.mm
   suggests that the minimizer's ability to detect this one depends on the
   presence of statement "A" in at least one of the steps of the minimized
   proof.  Axiom ax-3 and costant C are not used here, but they are kept
   anyway to lessen the amount of differences with min-found.mm. $)

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
