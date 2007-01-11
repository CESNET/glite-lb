#!/usr/bin/perl

# check_version script to be used to compare client-interface and other modules versions
# Usage:
#   - set environment variables VERSION and VERSION_AHEAD of the module
#   - run against ${stagedir}/include/glite/lb/interface_version.h
# Example: 
#    ./check_version.pl interface_version.h

my $version = $ENV{VERSION};
if ($version =~ /(\d+)\.\d+\.\d+/)  {
	$version = $1;
} else {
	print "error: wrong version format ($version)\n";
	exit 1;
}

my $ahead = $ENV{VERSION_AHEAD};
if ($ahead =~ /(\d+)/) {
	$ahead = $1;
} else {
	print "error: wrong version_ahead format ($ahead)\n";
	exit 1;
}

my $iface;

while (<>) {
	/#define GLITE_LB_CLIENT_INTERFACE "(\d+)\.\d+\.\d+"/; 
	$iface = $1; 
}

if ($iface + $ahead != $version) { 
	print "error: Major version of the interface ($iface + $ahead ahead) DOES NOT match implementation ($version)\n" ;
	exit 1;  
} 
