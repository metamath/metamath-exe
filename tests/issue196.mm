$( Regression test for issue #196: generating the web page for an axiom or a
   definition failed with "?BUG CHECK: *** DETECTED BUG 1741".

   A web page for an $a statement whose expression starts with "|-" carries a
   syntax breakdown.  SHOW STATEMENT builds it by proving the expression is a
   "wff", then temporarily zapping that generated proof into the statement's
   proof section and asking typeProof() to format it.  The generated proof has
   to survive that round trip intact; if the recorded length disagrees with the
   string by even one character, the last label is truncated and the proof is
   no longer valid.

   This database is deliberately arranged so the test exercises both halves of
   that code:

     - ax-1, ax-2, ax-3 and ax-mp are AXIOM pages (label begins "ax-").
     - df-an is a DEFINITION page (starts with "|-", label does not begin
       "ax-").  Both subtypes reach the zap; a SYNTAX page such as wi does not,
       which is why a database of only syntax statements never caught this.
     - a1i and a1iALT give "verify proof *" real proofs to check, in both the
       normal and the compressed format, since the proof tokenizer is the
       code that the truncated length was fed to.

   Generating the pages is what tripped the bug check, so a nonzero exit alone
   catches the original failure.  The test also shows the generated pages, so
   that a quietly wrong breakdown is caught and not just a hard failure: if a
   truncated label happened to still name a valid statement, the page would
   come out wrong with a zero exit status, and only the contents would say so.


   The website build generates both formats, so both are generated here.  The
   htmldef renderings are deliberately uppercase where the althtmldef ones are
   lowercase, so the two runs pin two different results and a mixup between the
   tables would show up rather than being hidden by identical output.

   Only the "Detailed syntax breakdown" table is shown, not the whole page.
   That table is what a truncated proof corrupts, while the description, the
   "used by" sentences and the rest of the page furniture change for unrelated
   reasons; pinning those would mean reblessing this test every time a page
   layout is touched, which is a cost with no matching benefit here. $)

$c wff |- ( ) -> -. /\ $.
$v ph ps ch $.

$( Declare ` ph ` as a wff variable. $)
wph $f wff ph $.
$( Declare ` ps ` as a wff variable. $)
wps $f wff ps $.
$( Declare ` ch ` as a wff variable. $)
wch $f wff ch $.

$( Negation. $)
wn $a wff -. ph $.
$( Implication. $)
wi $a wff ( ph -> ps ) $.
$( Conjunction. $)
wa $a wff ( ph /\ ps ) $.

$( Axiom _Simp_. $)
ax-1 $a |- ( ph -> ( ps -> ph ) ) $.

$( Axiom _Frege_. $)
ax-2 $a |- ( ( ph -> ( ps -> ch ) ) -> ( ( ph -> ps ) -> ( ph -> ch ) ) ) $.

$( Axiom _Transp_. $)
ax-3 $a |- ( ( -. ph -> -. ps ) -> ( ps -> ph ) ) $.

${
  $( Minor premise for modus ponens. $)
  min $e |- ph $.
  $( Major premise for modus ponens. $)
  maj $e |- ( ph -> ps ) $.
  $( Rule of _Modus Ponens_. $)
  ax-mp $a |- ps $.
$}

$( Define conjunction.  This is a DEFINITION page rather than an AXIOM page,
   because the label does not begin with "ax-". $)
df-an $a |- ( ( ph /\ ps ) -> -. ( ph -> -. ps ) ) $.

${
  $( Hypothesis for ~ a1i . $)
  a1i.1 $e |- ph $.
  $( Inference introducing an antecedent.  Proof is in the normal format. $)
  a1i $p |- ( ps -> ph ) $=
    wph wps wph wi a1i.1 wph wps ax-1 ax-mp $.

  $( Same inference as ~ a1i , but with the proof stored in the compressed
     format, so that the compressed proof parser is exercised too. $)
  a1iALT $p |- ( ps -> ph ) $=
    ( wi ax-1 ax-mp ) ABADCABEF $.
$}

$( $t
  htmldef "wff" as "WFF "; althtmldef "wff" as "wff ";
    latexdef "wff" as "\mathrm{wff}";
  htmldef "|-" as "|- "; althtmldef "|-" as "|- ";
    latexdef "|-" as "\vdash";
  htmldef "(" as "("; althtmldef "(" as "("; latexdef "(" as "(";
  htmldef ")" as ")"; althtmldef ")" as ")"; latexdef ")" as ")";
  htmldef "->" as " -> "; althtmldef "->" as " -> ";
    latexdef "->" as "\rightarrow";
  htmldef "-." as "-. "; althtmldef "-." as "-. ";
    latexdef "-." as "\lnot";
  htmldef "/\" as " /\ "; althtmldef "/\" as " /\ ";
    latexdef "/\" as "\wedge";
  htmldef "ph" as "PH"; althtmldef "ph" as "ph"; latexdef "ph" as "\varphi";
  htmldef "ps" as "PS"; althtmldef "ps" as "ps"; latexdef "ps" as "\psi";
  htmldef "ch" as "CH"; althtmldef "ch" as "ch"; latexdef "ch" as "\chi";
$)
