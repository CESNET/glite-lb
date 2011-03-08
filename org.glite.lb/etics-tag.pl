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

getopts('i:c:m:p:gh');

$module = shift;

$usage = qq{
usage: $0 [-i maj|min|rev|age|none|<sigle_word_age>] [-g] [-c <current configuration> ] module.name

	-i	What to increment ('maj'or version, 'min'or version, 'rev'ision, 'age')
		Should you fail to specify the -i option the script will open up a cvs diff
		output and ask you to specify what to increment interactively.
		'none' means no change -- this basically just generates a configuration.
	-g	Generate old configuration for comparison
	-c	Use this configuration (\d+\.\d+\.\d+-\S+) rather than parsing version.properties
	-m	Use this as a CVS commit message instead of the script's default.
	-p	Specify project ("org.glite" / "emi"). Default: autodetect. 
	-h	Display this help

};

	# **********************************
	# Interpret cmdline options
	# **********************************

	if (defined $opt_h) {die $usage};
	die $usage unless $module;

	#Clean possible trailing '/' (even multiple occurrences :-) from module name
	$module=~s/\/+$//;

	switch ($opt_i) {
		case "maj" {$increment="j"}
		case "min" {$increment="i"}
		case "rev" {$increment="r"}
		case "age" {$increment="a"}
		case "none" {$increment="n"}
		else {$increment=$opt_i};
	}


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
	}

	if (defined $opt_p) {
		$project=$opt_p;
		if (($project ne "emi")&&($project ne "org.glite")) {die "Only projects \"emi\" or \"org.glite\" are recognized";}
	}
	else {
		system("etics-list-configuration > $TMPDIR/etics-tag-proj_configs.$$.tmp");

		if ( !system("grep -x \"org.glite.HEAD\" $TMPDIR/etics-tag-proj_configs.$$.tmp > /dev/null") ) { $project = "org.glite"; }
		else {
			if ( !system("grep -x \"emi.HEAD\" $TMPDIR/etics-tag-proj_configs.$$.tmp > /dev/null") ) { $project = "emi"; }
			else { die "Unable to autodetect project. Run from workspace root or specify by -p" }
		}

		system("rm $TMPDIR/etics-tag-proj_configs.$$.tmp");
	}

	if ($project eq "emi") { $proj_opt = " --project=emi"; }
	else { $proj_opt = "--project=glite"; }
	
	# According to the documentation, symbolic names in the 'cvs log' output are sorted by age so this should be OK
	#$current_tag=`cvs log -h $module/Makefile | grep \"_R_\" | head -n 1`;
	#$current_tag=~s/^\s//;
	#$current_tag=~s/:.*?$//;
	#chomp($current_tag);

	#$current_tag=~/(.*_R_)(\d*?)_(\d*?)_(\d*?)_(.*)/;
	#$current_prefix=$1;
	#$current_major=$2;
	#$current_minor=$3;
	#$current_revision=$4;
	#$current_age=$5;

	printf("Current tag: $current_tag\n\tprefix: $current_prefix\n\t major: $current_major\n\t minor: $current_minor\n\t   rev: $current_revision\n\t   age: $current_age\n");

	# **********************************
	# Compare the last tag with the current source
	# **********************************

	unless (defined $increment) {
		printf("Diffing...\n");

		system("cvs diff -r $current_tag $module | less");
	}

	# **********************************
	# Generate the new tag name
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
	$tag="$current_prefix" . "$major" . "_$minor" . "_$revision" . "_$age";

	printf("\nNew tag: $tag\n\n");

	die "This tag already exists; reported by assertion" unless (($increment eq 'n') || system("cvs log -h $module/Makefile | grep \"$tag\""));

	# **********************************
	# Create the execution script
	# **********************************

	open EXEC, ">", "$TMPDIR/etics-tag-$module.$major.$minor.$revision-$age.sh" or die $!;

	printf (EXEC "#This script registers tags for the $module module, version $major.$minor.$revision-$age\n#Generated automatically by $0\n\n"); 


	# **********************************
	# Update the ChangeLog
	# **********************************

	if (-r "$module/project/ChangeLog") { # ChangeLog exists (where expected). Proceed.

		$tmpChangeLog="$TMPDIR/$module.ChangeLog.$$";
		if ( $project eq "emi" ) { $tmpChangeLog=~s/org\.glite/emi/; }

		system("cp $module/project/ChangeLog $tmpChangeLog");

		unless ($increment eq "n") {system("echo $major.$minor.$revision-$age >> $tmpChangeLog");
			if ($increment eq "a") {system("echo \"- Module rebuilt\" >> $tmpChangeLog"); system("echo \"\" >> $tmpChangeLog");}
			else { system("cvs log -S -N -r" . "$current_tag" . ":: $module | egrep -v \"^locks:|^access list:|^keyword substitution:|^total revisions:|^branch:|^description:|^head:|^RCS file:|^date:|^---|^===|^revision \" >> $tmpChangeLog"); }

			$ChangeLogRet=system("vim $tmpChangeLog");
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

	
	printf(EXEC "#Commit changes\ncvs commit -m \"$commit_message\" $module/project/ChangeLog $module/project/version.properties $module/configure\n\n");


	unless ($increment eq "n") {
		# **********************************
		# Run CVS Tag
		# **********************************

		$cwd=`pwd`;
		chomp($cwd);

		printf(EXEC "#Register the new tag\ncd $module\ncvs tag \"$tag\"\ncd \"$cwd\"\n");
	}

	# **********************************
	# Etics configuration prepare / modify / upload
	# **********************************

	$currentconfig="$module_$module" . "_R_$current_major" . "_$current_minor" . "_$current_revision" . "_$current_age";
	$currentconfig=~s/^org.//;
	$currentconfig=~s/\./-/g;
	$newconfig="$module_$module" . "_R_$major" . "_$minor" . "_$revision" . "_$age";
	$newconfig=~s/^org.//;
	$newconfig=~s/\./-/g;
	if ( $project eq "emi" ) { $newconfig=~s/^glite/emi/; }


	$module=~/([^\.]+?)\.([^\.]+?)$/;
	$subsysname=$1;
	$modulename=$2;

	printf("Project=$project\nModule=$module\nname=$modulename\nsubsys=$subsysname\n");
	system("$GLITE_LB_LOCATION/configure --mode=etics --module $subsysname.$modulename --output $TMPDIR/$newconfig.ini.$$ --version $major.$minor.$revision-$age $proj_opt");

#	printf("\nCurrent configuration:\t$currentconfig\nNew configuration:\t$newconfig\n\nPreparing...\n");
#
	if (defined $opt_g) {
		system("etics-configuration prepare -o $TMPDIR/$currentconfig.ini.$$ -c $currentconfig $module");
	}

#	open OLDCONF, "$TMPDIR/$currentconfig.ini.$$" or die $!; 
#	open NEWCONF, ">", "$TMPDIR/$newconfig.ini.$$" or die $!;

#       while ($_ = <OLDCONF>) {
#                chomp;

##                $_=~s/module\.age\s*=\s*(\S+)/module\.age=$age/;
#		$_=~s/$currentconfig/$newconfig/;
#		$_=~s/^\s*version\s*=\s*[.0-9]+/version = $major.$minor.$revision/;
#                $_=~s/^\s*age\s*=\s*\S+/age = $age/;

#		printf(NEWCONF "$_\n");
#        }

#	close(OLDCONF);
#	close(NEWCONF);

	if ($increment eq "n") { # There was no version change and the configuration should already exist
		printf(EXEC "\n#Modify new configuration\netics-configuration modify -i $TMPDIR/$newconfig.ini.$$\n"); }
	else { # New configuration needs to be created
	printf(EXEC "\n#Add new configuration\netics-configuration add -i $TMPDIR/$newconfig.ini.$$\n"); }
	printf(EXEC "etics-commit\n");


	# **********************************
	# Final bows
	# **********************************

	close(EXEC);

	system("chmod +x \"$TMPDIR/etics-tag-$module.$major.$minor.$revision-$age.sh\"");

	printf("\n\n---------\nDone!\n\nExecution script written in:\t$TMPDIR/etics-tag-$module.$major.$minor.$revision-$age.sh\nChangeLog candidate written in:\t$tmpChangeLog\n");
	printf("Old configuration stored in:\t$TMPDIR/$currentconfig.ini.$$\n") if (defined $opt_g);
	printf("New configuration written in:\t$TMPDIR/$newconfig.ini.$$\n\n");

