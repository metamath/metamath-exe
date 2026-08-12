 $c wff |- e ( , ) $.
 $v x y z w v u v1 v2 v3 v4 v5 v6 v7 v8 v9 v10 v11 $.
  wx $f wff x $. wy $f wff y $. wz $f wff z $. ww $f wff w $.
  wv $f wff v $. wu $f wff u $. wv1 $f wff v1 $. wv2 $f wff v2 $.
  wv3 $f wff v3 $. wv4 $f wff v4 $. wv5 $f wff v5 $. wv6 $f wff v6 $.
  wv7 $f wff v7 $. wv8 $f wff v8 $. wv9 $f wff v9 $. wv10 $f wff v10 $.
  wv11 $f wff v11 $.
 wi $a waf e ( x , y ) $.
 ${
   ax-mp.1 $e |- x $.
   ax-mp.2 $e |- e ( x , y ) $.
   ax-mp $a |- y $.
 $}
 ax-maj $a |- e ( e ( e ( e ( e ( x , e ( y , e ( e ( e ( e ( e ( z , e (
   e ( e ( z , u ) , e ( v , u ) ) , v ) ) , e ( e ( w , e ( e ( e ( w , v6
   ) , e ( v7 , v6 ) ) , v7 ) ) , y ) ) , v8 ) , e ( v9 , v8 ) ) , v9 ) ) )
   , x ) , v10 ) , e ( v11 , v10 ) ) , v11 ) $.
 ax-min $a |- e ( e ( e ( e ( e ( e ( x , e ( e ( y , e ( e ( e ( y , z )
   , e ( u , z ) ) , u ) ) , x ) ) , e ( v , e ( e ( e ( v , w ) , e ( v6 ,
   w ) ) , v6 ) ) ) , v7 ) , v8 ) , e ( v7 , v8 ) ) , e ( v9 , e ( e ( e (
   v9 , v10 ) , e ( v11 , v10 ) ) , v11 ) ) ) $.
 theorem1 $p |- e ( e ( e ( x , e ( y , e ( e ( e ( y , z ) , e ( u , z ) )
   , u ) ) ) , v ) , e ( x , v ) ) $=
   ( wi ax-min ax-maj ax-mp ) ABBCFECFFEFFZFZKDFADFFZAFZFJMFFZKNJFFNFZFAO
   FFZMPAFFZFPFQPFZFLRFFLNKMJAJNQPLAPGPMKBADCEOARLIH $.
$t
$)
