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

getopts('b:c:h');

$module = shift;

$usage = qq{
usage: $0 -b <branch> [-c configuration] module.name

	-b	Name of the 'b'ranch to base the configuration on.
		The branch must already exist in CVS!

	-c      Use this configuration (\d+\.\d+\.\d+-\S+) rather than parsing 
		version.properties.  Bear in mind that  this will only be used
		to set up the version and age in your configuration. 
		Also, the existence of the branch will not be tested. with the
		-c option.

	-h	Display this help

};

	# **********************************
	# Interpret cmdline options
	# **********************************

	if (defined $opt_h) {die $usage};
	die $usage unless $module;

	#Clean possible trailing '/' (even multiple occurrences :-) from module name
	$module=~s/\/+$//;

	if (!(defined $opt_b)) {die "Mandatory argument -b <branch> missing"};
	$branch=$opt_b;

	if (defined $opt_c) {
	
		# **********************************
		# Parse the tag supplied by the user 
		# **********************************

		if ($opt_c=~/(\d+)\.(\d+)\.(\d+)-(\S+?)/) {
				$current_major=$1;
				$current_minor=$2;
				$current_revision=$3;
				$current_age=$4;
		}
		else {die ("tag not stated properly")};

	}
	else {

		# **********************************
		# Determine the most recent tag and its components from version.properties 
		# **********************************

		open VP, "$module/project/version.properties" or die "$module/project/version.properties: $?\n";

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

		
		# **********************************
		# Make sure the branch exists
		# **********************************

		die "Branch $branch does not exist for this module\n" if system("cvs status -v $module/project/version.properties | grep \"(branch:\" | grep -w $branch");

	}

	printf("Current tag: $current_tag\n\tprefix: $current_prefix\n\t major: $current_major\n\t minor: $current_minor\n\t   rev: $current_revision\n\t   age: $current_age\n\tbranch: $branch\n");

	# **********************************
	# Create the execution script
	# **********************************

	open EXEC, ">", "$TMPDIR/etics-tag-$module.$branch.sh" or die $!;

	printf (EXEC "#This script creates creates a _dev configuration for the $module module, branch $branch\n#Generated automatically by $0\n\n"); 


	# **********************************
	# Etics configuration prepare / modify / upload
	# **********************************

	$currentconfig="$module_$module" . "_R_$current_major" . "_$current_minor" . "_$current_revision" . "_$current_age";
	$currentconfig=~s/^org.//;
	$currentconfig=~s/\./-/g;
	$newconfig="$module_$module" . "_$branch";
	$newconfig=~s/^org.//;
	$newconfig=~s/\./-/g;

	$module=~/([^\.]+?)\.([^\.]+?)$/;
	$subsysname=$1;
	$modulename=$2;

	# **********************************
	# Etics configuration prepare / modify / upload
	# **********************************

	printf("Module=$module\nname=$modulename\nsubsys=$subsysname\n");
	system("$GLITE_LB_LOCATION/configure --mode=etics --module $subsysname.$modulename --output $TMPDIR/$newconfig.ini.$$ --version $current_major.$current_minor.$current_revision-$current_age --branch $branch");

	printf (EXEC "#Find out if the configuration already exists in etics\n");
	printf (EXEC "echo Check if configuration exists\n\n"); 
	printf (EXEC "existconf=`etics-list-configuration $module | grep -w glite-$subsysname-${modulename}_$branch`\n\n");
	printf (EXEC "echo Found: \$existconf\n");
	printf (EXEC "if [ \"\$existconf\" = \"\" ]; then\n"); 
	printf (EXEC "    echo New congiguration ... call etics-configuration add\n");
	printf (EXEC "    etics-configuration add -i $TMPDIR/$newconfig.ini.$$\n"); 
	printf (EXEC "else\n"); 
	printf (EXEC "    echo Congiguration already exists ... call etics-configuration modify\n"); 
	printf (EXEC "    etics-configuration modify -i $TMPDIR/$newconfig.ini.$$\n"); 
	printf (EXEC "fi\n\n"); 
	printf (EXEC "etics-commit\n\n");


	# **********************************
	# Final bows
	# **********************************

	close(EXEC);

	system("chmod +x \"$TMPDIR/etics-tag-$module.$branch.sh\"");

	printf("\n\n---------\nFinished!\n\nExecution script written in:\t$TMPDIR/etics-tag-$module.$branch.sh\n");
	printf("Old configuration stored in:\t$TMPDIR/$currentconfig.ini.$$\n") if (defined $opt_g);
	printf("New configuration written in:\t$TMPDIR/$newconfig.ini.$$\n\n");

