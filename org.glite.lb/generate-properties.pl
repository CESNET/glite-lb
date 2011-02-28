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

use Getopt::Std;
use Switch;

$TMPDIR=$ENV{'TMPDIR'};

if ($TMPDIR eq "") {$TMPDIR="/tmp";}



$usage = qq{
usage: $0 [-p] -a|module.name [module.name ...]|-c configuration [configuration ...]

        -h      Display this help
	-p	Single line output for Etics web interface
	-a	Process all subdirectories in . rather than getting lists
	-c	Download subsystem configurations from Etics and parse hierarchy

};

if ($#ARGV < 0) {die $usage};

getopts('phac');

if (defined $opt_h) {die $usage};
if (defined $opt_p) {
	$eq_separator = "=";
	$pair_separator_back = " "; 
	$pair_separator_front = "-p "; }
else {
	$eq_separator = " = ";
	$pair_separator_back = "\n"; 
	$pair_separator_front = ""; }

sub GetDefault {
	# **********************************
	# Determine the most recent tag and its components from version.properties 
	# **********************************
	my($mname) = @_;

	unless (open VP, "$mname/project/version.properties") {
		unless (defined $opt_a) { die "$mname/project/version.properties: $?\n"; }
		else { return 1; }
	}

	while ($_ = <VP>) {
		chomp;

		if(/module\.version\s*=\s*(\d*)\.(\d*)\.(\d*)/) {
			$current_major=$1;
			$current_minor=$2;
			$current_revision=$3;
		}
		if(/module\.age\s*=\s*(\S+)/) {
			$current_age=$1;
		}
	}
	close (VP);

	$current_prefix=$mname;
	$current_prefix=~s/^org\.//;
	$current_prefix=~s/\./-/g;
	$current_prefix="$current_prefix" . "_R_";
	$current_tag="$current_prefix" . "$current_major" . "_$current_minor" . "_$current_revision" . "_$current_age";

	printf "$pair_separator_front$mname.DEFAULT$eq_separator$current_tag$pair_separator_back";

}

if (defined $opt_a) { # All subdirectories
	opendir(LOCAL, ".");
	@contents = readdir(LOCAL);
	closedir(LOCAL); 

	foreach $f (@contents) {
		unless ( ($f eq ".") || ($f eq "..") ) {
			if (-d $f) {
				GetDefault ($f);
			}
		}
	}
}
else { 
if (defined $opt_c) { # Configurations
	my $output = "";
	while ($subsys = shift) {
		$module = $subsys;
		$module =~ s/_[a-zA-Z]*_.*//;	
		$module =~ s/^glite-/org.glite\./;	
		$module =~ s/^gridsite-/org.gridsite\./;
		$module =~ s/^emi-/emi\./;
		system("etics-configuration prepare -o $TMPDIR/subsys.INI.$$.tmp -c $subsys $module");
		open FILE, "$TMPDIR/subsys.INI.$$.tmp" or die $!;
		$hierarchy = 0;
		while (my $line = <FILE>) {
			if ($line =~ /^\[Hierarchy\]/) { $hierarchy = 1; }
			else {
				if ($line =~ /^\[.*\]/) { $hierarchy = 0; }
				else {
					if ($hierarchy eq 1) {
						$line =~ /^(\S*)\s*=\s*(\S*)$/;
						unless ($1 eq "") {
							$output = $output . "$pair_separator_front$1.DEFAULT$eq_separator$2$pair_separator_back";
						}
					}
				}
			}
		}
		close FILE,
		system("rm -f $TMPDIR/subsys.INI.$$.tmp");
	}
	print $output;
}
else {
# Listed subsystems 
	while ($module = shift) {
		chomp($module);
        	#Clean possible trailing '/' (even multiple occurrences :-) from module name
	        $module=~s/\/+$//;
 
		$module=~/\.([^\.]+?)$/;	

		@modules=split(/\s+/, `PATH=\$PATH:./:./org.glite.lb configure --listmodules $1`);

		foreach $m (@modules) {
			GetDefault ($m);
		}
	}
} }

if (defined $opt_p) { printf "\n"; }
