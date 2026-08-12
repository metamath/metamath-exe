$( Regression test for the quiet form of issue #196.

   The companion test issue196.mm covers the loud form: the generated syntax
   breakdown proof is truncated, the truncated last label names nothing, and
   metamath stops with "?BUG CHECK: *** DETECTED BUG 1741".  An exit status is
   enough to catch that one.

   This database covers the form that reports nothing at all.  The truncation
   removes the last character of the last label of the generated proof, so if
   the shortened label still names a statement that yields the same wff, the
   proof still verifies and the page is simply built around the wrong
   statement.  There is no bug check, no error, no warning, and the exit
   status is 0; only the contents of the page say anything is wrong.  That is
   why the tests compare generated output instead of just checking that
   metamath survived.

   The arrangement is deliberate and is the whole point of the file:

     - "wi" and "wii" are interchangeable.  Same hypotheses, same conclusion,
       and "wi" is exactly "wii" with the final character removed.
     - "wii" is declared second because proveFloating picks the later of the
       two, so it is the label that ends the generated breakdown proof.  With
       the declaration order reversed the breakdown ends in "wi", truncation
       yields "w", and the failure becomes the loud kind instead.

The website build generates both formats, so both are generated here.  The
   htmldef renderings are deliberately uppercase where the althtmldef ones are
   lowercase, so the two runs pin two different results and a mixup between the
   tables would show up rather than being hidden by identical output.

   A real database would not carry two names for one syntax construction, so
   this is not a realistic database and is not meant to be.  It is the
   smallest arrangement that makes the quiet failure reproducible. $)

$c wff |- ( ) -> $.
$v ph ps $.

$( Declare ` ph ` as a wff variable. $)
wph $f wff ph $.
$( Declare ` ps ` as a wff variable. $)
wps $f wff ps $.

$( Implication, short name.  This is "wii" with the last character removed. $)
wi $a wff ( ph -> ps ) $.

$( Implication, long name.  Declared after ~ wi , which is the one
   proveFloating picks, so this label ends the generated breakdown proof. $)
wii $a wff ( ph -> ps ) $.

$( Axiom Simp, under a label of its own so that this test and issue196 do not
   write the same page name. $)
ax-sil $a |- ( ph -> ( ps -> ph ) ) $.

$( $t
  htmldef "wff" as "WFF "; althtmldef "wff" as "wff ";
    latexdef "wff" as "\mathrm{wff}";
  htmldef "|-" as "|- "; althtmldef "|-" as "|- ";
    latexdef "|-" as "\vdash";
  htmldef "(" as "("; althtmldef "(" as "("; latexdef "(" as "(";
  htmldef ")" as ")"; althtmldef ")" as ")"; latexdef ")" as ")";
  htmldef "->" as " -> "; althtmldef "->" as " -> ";
    latexdef "->" as "\rightarrow";
  htmldef "ph" as "PH"; althtmldef "ph" as "ph"; latexdef "ph" as "\varphi";
  htmldef "ps" as "PS"; althtmldef "ps" as "ps"; latexdef "ps" as "\psi";
$)
