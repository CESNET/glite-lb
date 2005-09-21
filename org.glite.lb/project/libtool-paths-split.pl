#!/usr/bin/perl -p -i

next unless /^dependency_libs=/;

s/^dependency_libs=//;
s/\s*'\s*//g;

$out = '';

undef $libs;
undef %paths;

for (split) {
	if (/^-L(.*)/) { $paths{$1} = 'x'; }
	elsif (/^-l(.*)/) { $libs .= " -l$1"; }
	elsif (/^(.*)\/lib(.*)\.(la|so).*$/) { $paths{$1} = 'x'; $libs .= " -l$2" ; }
# XXX	else { $out .= " $_"; }
	else { print STDERR "$_: unknown\n"; }
}

# $_ = "dependency_libs='$out'";
$_ = "dependency_libs='";
$_ .= '-L' . join ' -L',keys %paths if %paths;
$_ .= $libs if $libs;
$_ .= "'";
