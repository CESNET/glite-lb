#!/usr/bin/perl -n

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
