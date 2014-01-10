#
# Copyright (c) Members of the EGEE Collaboration. 2004-2010.
# See http://www.eu-egee.org/partners for details on the copyright holders.
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#     http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
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
			my $init;
	
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
				$init = " = ${fnu}.UNDEFINED";
				my $count = 0;
				my $n = $#{$f->{codes}};
				my $sep = ",";
				for (@{$f->{codes}}) {
					my $cu = uc $_->{name};
					if ($count >= $n) { $sep = ""; };
					print E
qq{		${cu}$sep
};
					$count++;
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
				$init = $f->{null} && $main::DefaultNullValue{$f->{type}} ne $f->{null} ? " = $f->{null}" : "";
			}
	

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
