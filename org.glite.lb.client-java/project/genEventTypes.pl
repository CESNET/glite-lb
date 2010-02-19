@@@LANG: java
@@@{
	$dest = shift;

	for my $e ($event->getTypesOrdered) {
		my $uc = ucfirst $e;
		my $uuc = uc $e;
		print "generating $dest/Event$uc.java\n";

		open E,">$dest/Event$uc.java" or die "$dest/Event$uc.java: $!\n";

		print E
qq{
package org.glite.lb;
import org.glite.jobid.Jobid;

public class Event$uc extends Event \{
	public Event$uc() \{
	\}

	public String getEventType() \{
		return "$uc";
	\}
};

		selectType $event $e;

		for ($event->getFieldsOrdered) {
			my $f = selectField $event $_;
			my $fn = $f->{name};
			my $fnu = ucfirst $fn;
			my $fnuu = uc $fn;
			my $t;
	
			while ($fnu =~ /_([a-z])/) {
				my $u = uc $1;
				$fnu =~ s/_$1/$u/;
			}
	
			if ($f->{codes}) {
				local $_;
				$t = $fnu;
				print E
qq{	public enum $fnu \{ 
		UNDEFINED,
};
				for (@{$f->{codes}}) {
					my $cu = uc $_->{name};
					print E
qq{		${cu},
};
				}

				print E
qq{	\};
};
#	public static String ${fnu}ToString($fnu e) \{
#		String out = "UNDEF";
#		switch (e) \{
#};
#				for (@{$f->{codes}}) {
#					my $cu = uc $_->{name};
#					print E
#qq{
#			case ${fnu}.${cu}: out = "$cu"; break;
#};
#				}
#				print E
#qq{
#		\}
#		return out;
#	\}
#};
			}
			else {
				$t = $f->getType;
			}
	
	my $init = $f->{null} && $main::DefaultNullValue{$f->{type}} ne $f->{null} ?
		" = $f->{null}" : "";

# XXX: handle nulls in setXX() ?
			print E
qq{	private $t $fn $init;

	public $t get$fnu() \{
		return $fn;
	\}

	public void set$fnu($t val) \{
		this.$fn = val;
	\}
};
		}

		print E
qq{	public String ulm() \{
		return (" " +
};
	

		for ($event->getFieldsOrdered) {
			my $f = selectField $event $_;
			my $fn = $f->{name};
			my $t = getType $f;
			my $fnu = ucfirst $fn;
			while ($fnu =~ /_([a-z])/) {
				my $u = uc $1;
				$fnu =~ s/_$1/$u/;
			}
			my $fnuu = uc $fn;
			my $val = $t eq 'String' ? 
		 		"($fn == null ? \"\" : Escape.ulm($fn))" :
				$f->{codes} ? "Escape.ulm(${fn}.toString())" :
				$fn;

			print E
qq{		" DG.$uuc.$fnuu=\\"" + $val + "\\"" +
};
		}

		print E 
qq{		"");
	\}
\}
};

		close E;
	}

@@@}
