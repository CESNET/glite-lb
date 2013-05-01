#! /usr/bin/perl -w
use strict;

my (@b64, @b64r);

@b64 = unpack("C64", "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_");

#for my $i (0..$#b64) {
#	printf "%c, ", $b64[$i];
#}
#print "\n";

for my $i (0..$#b64) {
	$b64r[$b64[$i]] = $i;
}

print "static const char b64r[] = {";
for my $i (0..$#b64r) {
	print $b64r[$i] ? $b64r[$i] : 0;
	print ", ";
}
print "};\n";
