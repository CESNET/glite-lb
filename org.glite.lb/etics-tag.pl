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

sub ConstructChangeLog {
        my ($CCLtag,$CCLmod,$CCLchl) = @_;

	system("git log --name-status --first-parent $CCLtag..HEAD -- $CCLmod | grep -E -v \"^commit|^Author:|Date:|^\$\" | sed 's/^[ \\t][ \\t]*/- /' >> $CCLchl");
}

getopts('i:c:m:h');

$module = shift;

$usage = qq{
usage: $0 [-i maj|min|rev|age|none|<sigle_word_age>] [-g] [-c <current configuration> ] module.name

	-i	What to increment ('maj'or version, 'min'or version, 'rev'ision, 'age')
		Should you fail to specify the -i option the script will open up a git diff
		output and ask you to specify what to increment interactively.
		'none' means no change -- this basically just generates a configuration.
	-c	Use this version (\d+\.\d+\.\d+-\S+) rather than parsing version.properties
	-m	Use this as a git commit message instead of the script's default.
	-h	Display this help

};

	# **********************************
	# Interpret cmdline options
	# **********************************

	if (defined $opt_h) {die $usage};
	die $usage unless $module;

	#Clean possible trailing '/' (even multiple occurrences :-) from module name
	$module=~s/\/+$//;
	
	$project="emi";

	switch ($opt_i) {
		case "maj" {$increment="j"}
		case "min" {$increment="i"}
		case "rev" {$increment="r"}
		case "age" {$increment="a"}
		case "none" {$increment="n"}
		else {$increment=$opt_i};
	}

	#poor man's subsystem recignition
	if ($module =~ m/^org\.glite\.jobid/) {
		$subsysname="org.glite.lb";
		$subsystem_prefix="glite-lb";
	}
	elsif ($module =~ m/^org\.glite\.lb/) {
		$subsysname="org.glite.lb";
		$subsystem_prefix="glite-lb";
	}
	elsif ($module =~ m/^org\.glite\.px/) {
		$subsysname="org.glite.px";
		$subsystem_prefix="glite-px";
	}
	elsif ($module =~ m/^org\.glite\.canl/) {
		$subsysname="org.glite.canl-c";
		$subsystem_prefix="emi-canl";
	}
	$subsystem_prefix=$subsystem_prefix . "_R_";


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
		# Determine the most recent tag and its components from subsystem's version.properties 
		# **********************************

		open VP, "$subsysname/project/version.properties" or die "$subsysname/project/version.properties: $?\n";

		while ($_ = <VP>) {
			chomp;

			if(/module\.version\s*=\s*(\d*)\.(\d*)\.(\d*)/) {
				$subsystem_major=$1;
				$subsystem_minor=$2;
				$subsystem_revision=$3;
			}
			if(/module\.age\s*=\s*(\S+)/) {
				$subsystem_age=$1;
			}
		}
		close (VP);

		$current_tag="$subsystem_prefix" . "$subsystem_major" . "_$subsystem_minor" . "_$subsystem_revision" . "_$subsystem_age";
	}

	# **********************************
	# Determine the most recent module version and its components from version.properties 
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


	printf("Subsystem tag: $current_tag\n\tModule major: $current_major\n\tModule minor: $current_minor\n\t  Module rev: $current_revision\n\t  Module age: $current_age\n");

	# **********************************
	# Compare the last tag with the current source
	# **********************************

	unless (defined $increment) {
		printf("Diffing...\n");

		printf("*** git diff --relative=\"$module\" $current_tag ***\n");
		system("git diff --relative=\"$module\" $current_tag");
	}

	# **********************************
	# Generate new version
	# **********************************

	printf("\nWhich component do you wish to increment?\n\n\t'j'\tmaJor\n\t'i'\tmInor\n\t'r'\tRevision\n\t'a'\tAge\n\t\'n'\tNo change\n\tfree type\tUse what I have typed (single word) as a new age name (original: $current_age)\n\nType in your choice: ");

	unless (defined $increment) {
		$increment=<STDIN>;
	}

	chomp($increment);

	switch ($increment) {
		case "j" {
			$major=$current_major+1;
			$minor=0;
			$revision=0;
			$age=1;}
		case "i" {
			$major=$current_major;
			$minor=$current_minor+1;
			$revision=0;
			$age=1;}
		case "r" {
			$major=$current_major;
			$minor=$current_minor;
			$revision=$current_revision+1;
			$age=1;} 
		case "a" {
			$major=$current_major;
			$minor=$current_minor;
			$revision=$current_revision;
			$age=$current_age+1;} 
		case "n" {
			$major=$current_major;
			$minor=$current_minor;
			$revision=$current_revision;
			$age=$current_age;} 
		else {
			$major=$current_major;
			$minor=$current_minor;
			$revision=$current_revision;
			$age=$increment;}
	}
	$new_version="$major" . ".$minor" . ".$revision" . "-$age";

	printf("\nNew version: $new_version\n\n");


	# **********************************
	# Create the execution script
	# **********************************

	open EXEC, ">", "$TMPDIR/etics-tag-$module.$major.$minor.$revision-$age.sh" or die $!;

	printf (EXEC "#This script sets up version properties for the $module module, version $major.$minor.$revision-$age\n#Generated automatically by $0\n\n"); 


	# **********************************
	# Update the ChangeLog
	# **********************************

	if (-r "$module/project/ChangeLog") { # ChangeLog exists (where expected). Proceed.

		$tmpChangeLog="$TMPDIR/$module.ChangeLog.$$";
		if ( $project eq "emi" ) { $tmpChangeLog=~s/org\.glite/emi/; }

		system("cp $module/project/ChangeLog $tmpChangeLog");


		unless ($increment eq "n") {system("echo $major.$minor.$revision-$age >> $tmpChangeLog");

			$editline=`cat $tmpChangeLog | wc -l`;
			chomp($editline);

			if ($increment eq "a") {system("echo \"- Module rebuilt\" >> $tmpChangeLog"); system("echo \"\" >> $tmpChangeLog");}
			else { ConstructChangeLog($current_tag, $module, $tmpChangeLog); }
				
			$ChangeLogRet=system("vim +$editline -c \"norm z.\" $tmpChangeLog");
		}
		printf("Modified ChangeLog ready, ret code: $ChangeLogRet\n");

		printf(EXEC "#Update the ChangeLog\ncp $tmpChangeLog $module/project/ChangeLog\n\n");
	}	

	unless ($increment eq "n") {
		# **********************************
		# Update version.properties
		# **********************************
		open V, "$module/project/version.properties" or die "$module/project/version.properties: $?\n";

		printf(EXEC "#Generate new version.properties\ncat >$module/project/version.properties <<EOF\n");
		while ($_ = <V>) {
			chomp;

			$_=~s/module\.version\s*=\s*[.0-9]+/module\.version=$major.$minor.$revision/;
			$_=~s/module\.age\s*=\s*(\S+)/module\.age=$age/;

			$_=~s/\$/\\\$/g;
			printf(EXEC "$_\n");
		}
		close V;
		printf(EXEC "EOF\n\n");

	}


	# **********************************
	# Update configure
	# **********************************
	
	printf(EXEC "#Update the \"configure\" script\ncp $GLITE_LB_LOCATION/configure $module/\n\n");

	# **********************************
	# Commit changes
	# **********************************

	if (defined $opt_m) {$commit_message=$opt_m;}
        else {$commit_message="Updating version, ChangeLog and copying the most recent configure from $GLITE_LB_LOCATION for v. $major.$minor.$revision-$age";}

	
	printf(EXEC "#Commit changes\ngit commit -m \"$commit_message\" $module/project/ChangeLog $module/project/version.properties $module/configure\n\n");


	# **********************************
	# Final bows
	# **********************************

	close(EXEC);

	system("chmod +x \"$TMPDIR/etics-tag-$module.$major.$minor.$revision-$age.sh\"");

	printf("\n\n---------\nDone!\n\nExecution script written in:\t$TMPDIR/etics-tag-$module.$major.$minor.$revision-$age.sh\nChangeLog candidate written in:\t$tmpChangeLog\n");

