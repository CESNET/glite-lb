#ident "$Header$"
/*
@@@AUTO
*/
@@@LANG: C

#include "glite/wmsutils/jobid/cjobid.h"
#include "producer.h"

@@@{
for my $t (sort { $event->{order}->{$a} <=> $event->{order}->{$b} }
		$event->getTypes) {
	my $tu = uc $t;
	my $a = "(edg_wll_Context context";
	my $b = "(context,EDG_WLL_EVENT_$tu,EDG_WLL_FORMAT_$tu";
	my $decl = "";
	my $free = "";
	my $doc = qq{
 * \\param context\tcontext to work with,
};
	selectType $event $t;
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->getName;
		my $ft;
		my $bfn = $fn;
                if ($f->{codes}) {
#                        $ft = "enum edg_wll\_$t" . ucfirst $fn;
                        $ft = "char *";
                } else {
                        $ft = $f->getType;
                }
		if ($ULMasString{$f->{type}}) {
			$decl .= "\tchar *s_$fn = ".$f->getType()."ToString($fn);\n";
			$free .= "\tfree(s_$fn);\n";
			$bfn = "s_$fn";
		}
		$ft = "const ".$ft;
		my $fc = $f->getComment;
		$a = $a . ", $ft $fn";
		$b = $b . ", $bfn";
		$doc = $doc . " * \\param $fn\t$fc\n";
	}
	$a = $a . ")";
	$b = $b . ")";

	gen qq{
/*!
 * \\fn int edg_wll_Log$t$a; 
 * \\brief simple wrapper around edg_wll_LogEvent for event $t} . $doc . qq{ * \\see edg_wll_LogEvent\(\)
 */
};
#	gen "\nextern int edg_wll_Log$t$a;\n\n";
	gen qq{
int edg_wll_Log$t$a
\{
$decl
	return edg_wll_LogEvent$b;
\}\n
};
	gen qq{
int edg_wll_Log${t}Proxy$a
\{
$decl
	return edg_wll_LogEventProxy$b;
\}\n
};
	for ($event->getFieldsOrdered) {
		my $f = selectField $event $_;
		my $fn = $f->getName;
		my $ft;
                if ($f->{codes}) {
#                        $ft = "enum edg_wll\_$t" . ucfirst $fn;
                        $ft = "char *";
                } else {
                        $ft = $f->getType;
                }
		my $ftreg = $ft;
		$ftreg =~ s/\*/\\\*/g;
		$ftreg = "const ".$ftreg;
		my $fc = $f->getComment;
		if ($f->{codes}) {
			for (@{$f->{codes}}) {
				my $code = uc $_->{name};
				my $c = $a;
				my $d = $b;
				my $e = $doc;
				$c =~ s/, $ftreg $fn//g;
				$d =~ s/$fn/"$code"/g;
				$e =~ s/ \* \\param $fn\t$fc\n//g;
				gen qq{
/*!
 * \\fn int edg_wll_Log$t$code$c; 
 * \\brief simple wrapper around edg_wll_LogEvent for event $t, $fn $code} . $e . qq{ * \\see edg_wll_LogEvent\(\)
 */
};
#				gen "\nextern int edg_wll_Log$t$code$c;\n\n";
				gen qq{
int edg_wll_Log$t$code$c
\{
$decl
	return edg_wll_LogEvent$d;
\}\n
};
				gen qq{
int edg_wll_Log$t${code}Proxy$c
\{
$decl
	return edg_wll_LogEventProxy$d;
\}\n
};
			}
		}
	}
}
@@@}
