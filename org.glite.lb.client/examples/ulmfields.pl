#!/usr/bin/perl -n
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

BEGIN{
	@pf = split /,/,shift;
};

next if /^\s*$/;

@F = split / /;

undef $prev;
undef %f;

for $f (@F) {
	if ($f =~ /^[.A-Z_]+="/) {
#		print $prev,"\n" if $prev;
		@P = split /=/,$prev,2;
		$f{$P[0]} = $P[1];
		$prev = $f;
	}
	else { $prev .= $f; }
}

# print $prev,"\n";
@P = split /=/,$prev,2;
$f{$P[0]} = $P[1];

for $f (@pf) {
	print "$f=$f{$f}\n" if $f{$f};
}
print "\n";
