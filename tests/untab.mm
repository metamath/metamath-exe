$( Test database containing tab characters.  outputStatement() runs the
   label and math sections through edit(...,2048) to expand tabs, a code
   path that no other test reaches: no other test .mm file, and no part
   of set.mm, contains a tab. $)

$c	wff	|-	$.
$v	x	$.
vx	$f	wff	x	$.
$(	A	comment	with	tabs	$)
ax1	$a	|-	x	$.
th	$p	|-	x	$=	vx	ax1	$.
