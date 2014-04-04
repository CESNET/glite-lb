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
$GLITE_LB_LOCATION="./org.glite.lb";

if ($TMPDIR eq "") {$TMPDIR="/tmp";}

getopts('ch');

$usage = qq{
usage: $0 [-h] subsystem.name [subsystem.name] [...]

	This script checks the consistency of tags in CVS and version.properties

	-c	also verify etics configurations
	-h	Display this help

};

	# **********************************
	# Interpret cmdline options
	# **********************************

	if (defined $opt_h) {die $usage};
	die $usage unless @ARGV[0];

	if (defined $opt_c) {
		printf ("\n\nYou have selected the -c option. Note that etics may require authetication.\n If you cannot see any progress in the script, it is probably waiting for your password. ;-)\n\n"); 
	}

	# **********************************
	# Iterate through subsystems
	# **********************************

	foreach $subsystem (@ARGV) {

		#Clean possible trailing '/' (even multiple occurrences :-) from subsystem name
		$subsystem=~s/\/+$//;

		printf("$subsystem\n");

		$subsystem=~/\.([^\.]+?)$/;

		@modules=split(/\s+/, `PATH=\$PATH:./:./org.glite.lb configure --listmodules $1`);

		unshift (@modules, $subsystem);

		foreach $module (@modules) {

			printf("  %-30s", $module);


			if (open VP, "$module/project/version.properties") {

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

				$current_prefix=$module;
				$current_prefix=~s/^org\.//;
				$current_prefix=~s/\./-/g;
				$current_prefix="$current_prefix" . "_R_";
				$current_tag="$current_prefix" . "$current_major" . "_$current_minor" . "_$current_revision" . "_$current_age";

				if ($module eq $subsystem) { $subsystem_tag = $current_tag; }

				printf("\t $current_major.$current_minor.$current_revision-$current_age");

				unless (system("cvs log -h $module/project/version.properties | grep -E \"\\W$current_tag\\W\" > /dev/null"))
					{ printf ("\t mod. OK"); }
				else {
					printf(STDERR "\nERROR: Tag $current_tag does not exist in module $module!\n"); 
				}
 
				if ($module ne $subsystem ) {
					unless (system("cvs log -h $module/project/version.properties | grep -E \"\\W$subsystem_tag\\W\" > /dev/null"))
						{ printf ("\t subsys. OK"); }
					else {
						printf(STDERR "\nERROR: Tag $subsystem_tag does not exist in module $module!\n"); 
					}

					unless (-e "$module/ChangeLog") {
						printf(STDERR "\nERROR: The ChangeLog file for module $module does not exist!\n");
					}
				}

				unless (-e "$module/configure") {
					printf(STDERR "\nERROR: The configure script for module $module does not exist!\n");
				}

				if (defined $opt_c) {
					unless (system("etics-list-configuration $module | grep \"$current_tag\" &> /dev/null"))
						{ printf ("\t etics OK"); }
					else {
						printf(STDERR "\nERROR: Configuration $current_tag for module $module does not exist!\n"); 
					}
				}

				printf("\n");

			}
			else {
				printf(STDERR "\nERROR: The version.properties file for module $module does not exist!\n"); 
			}
		}
	}
 
