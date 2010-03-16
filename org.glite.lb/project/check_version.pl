#!/usr/bin/perl
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
