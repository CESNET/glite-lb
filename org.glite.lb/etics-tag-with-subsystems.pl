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

getopts('c:p:h');

$module = shift;

chomp($module);

$usage = qq{
usage: $0 [-c <current configuration>] module.name

        -c      Use this configuration (\d+\.\d+\.\d+-\S+) rather than parsing version.properties
        -p      Specify project ("org.glite" / "emi"). Default: autodetect. 
        -h      Display this help

};
        if (defined $opt_h) {die $usage};
	die $usage unless $module;

        #Clean possible trailing '/' (even multiple occurrences :-) from module name
        $module=~s/\/+$//;
 
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
                else {die ("tag not specified properly")};

        }
	else {
		# **********************************
		# Determine the most recent tag and its components
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
		$current_tag=~s/^emi-/glite-/;
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

        if ($project eq "emi") { $proj_suffix = "emi" }
        else { $proj_suffix = ""; }


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

	$module=~/\.([^\.]+?)$/;	

        if ($project eq "emi") { $proj_switch = " --project=emi" }
        else { $proj_switch = ""; }

        @modules=split(/\s+/, `PATH=\$PATH:./:./org.glite.lb configure --listmodules $1$proj_switch`);

	my $incmajor=0;
	my $incminor=0;
	my $increvision=0;
	my $incage=0;


	# **********************************
	# Iterate through modules and find out what has changed
	# **********************************

	foreach $m (@modules) {

                if ($project eq "emi") {
                        $m=~s/^emi\./org.glite./;
                }
		printf("***$m\n");

	        if ($project eq "emi") {
			$m=~s/^emi\./org.glite./;
		}

		$old_major=-1; $old_minor=-1; $old_revision=-1; $old_age=-1;
		$new_major=-1; $new_minor=-1; $new_revision=-1; $new_age=-1;

		foreach $l (`cvs diff -r $current_prefix$current_major\_$current_minor\_$current_revision\_$current_age $m/project/version.properties | grep -E "module\.age|module\.version"`) {
			chomp($l);
#			printf("$l\n");

			if($l=~/<\s*module\.version\s*=\s*(\d*)\.(\d*)\.(\d*)/) {
				$old_major=$1;
				$old_minor=$2;
				$old_revision=$3;
			} 
			elsif($l=~/<\s*module\.age\s*=\s*(\S+)/) {
				$old_age=$1;
			}
			elsif($l=~/>\s*module\.version\s*=\s*(\d*)\.(\d*)\.(\d*)/) {
				$new_major=$1;
				$new_minor=$2;
				$new_revision=$3;
			} 
			elsif($l=~/>\s*module\.age\s*=\s*(\S+)/) {
				$new_age=$1;
			}
		}

		

		if ($old_major != $new_major) {
			$incmajor++;
			printf("Major change ($old_major -> $new_major)");
		}
		elsif ($old_minor != $new_minor) {
			$incminor++;
			printf("Minor change ($old_minor -> $new_minor)");
		}
		elsif ($old_revision != $new_revision) {
			$increvision++;
			printf("Revision change ($old_revision -> $new_revision)");
		}
		elsif ($old_age != $new_age) {
			$incage++;
			printf("Age change ($old_age -> $new_age)");
		}
		printf("\n");
		
	}

	printf("Current tag: $current_tag\n\tprefix: $current_prefix\n\t major: $current_major\n\t minor: $current_minor\n\t   rev: $current_revision\n\t   age: $current_age\n");

	# **********************************
	# Generate the new tag name
	# **********************************

	if($incmajor > 0) {
		$major=$current_major+1;
		$minor=0;
		$revision=0;
		$age=1;}
	elsif($incminor > 0) {
		$major=$current_major;
		$minor=$current_minor+1;
		$revision=0;
		$age=1;}
	elsif($increvision > 0) {
		$major=$current_major;
		$minor=$current_minor;
		$revision=$current_revision+1;
		$age=1;} 
	elsif($incage > 0) {
		$major=$current_major;
		$minor=$current_minor;
		$revision=$current_revision;
		$age=$current_age+1;} 
	else {
		printf("No change in either version component.\nAbort by pressing Ctrl+C or enter new age manually.\nUse a number or a word: ");
		$major=$current_major;
		$minor=$current_minor;
		$revision=$current_revision;
		$age=<STDIN>;}

	chomp($age);

	$tag="$current_prefix" . "$major" . "_$minor" . "_$revision" . "_$age";
	$tag=~s/^emi-/glite-/;

	printf("\nNew tag: $tag\n\n");

	die "This tag already exists; reported by assertion" unless system("cvs log -h $module/Makefile | grep \"$tag\"");

	# **********************************
	# Create the execution script
	# **********************************

	open EXEC, ">", "$TMPDIR/tag-with-subsystems-$module.$major.$minor.$revision-$age.sh" or die $!;

	printf (EXEC "#This script registers tags for the $module module, version $major.$minor.$revision-$age\n#Generated automatically by $0\n\n"); 


	# **********************************
	# Update version.properties
	# **********************************
        open V, "$module/project/version.properties" or die "$module/project/version.properties: $?\n";

	printf(EXEC "#Generate new version.properties\ncat >$module/project/version.properties <<EOF\n");
        while ($_ = <V>) {
                chomp;

		$_=~s/module\.version\s*=\s*[.0-9]+/module\.version=$major.$minor.$revision/;
                $_=~s/module\.age\s*=\s*(\S+)/module\.age=$age/;

		printf(EXEC "$_\n");
        }
        close V;
	printf(EXEC "EOF\n\n");
	printf(EXEC "cvs commit -m \"Modified to reflect version $major.$minor.$revision-$age\" $module/project/version.properties\n\n");


	$cwd=`pwd`;
	chomp($cwd);

	printf(EXEC "#Register the new tag\ncd $module\ncvs tag \"$tag\"\n");
	foreach $m (@modules) {
		printf (EXEC "cd \"$cwd/$m\"\ncvs tag \"$tag\"\n");
	}
	printf(EXEC "cd \"$cwd\"\n");
	


	# **********************************
	# Etics configuration prepare / modify / upload
	# **********************************

#	$currentconfig="$module_$module" . "_R_$current_major" . "_$current_minor" . "_$current_revision" . "_$current_age";
#	$currentconfig=~s/^org.//;
#	$currentconfig=~s/\./-/g;
	$newconfig="$module_$module" . "_R_$major" . "_$minor" . "_$revision" . "_$age";
	$newconfig=~s/^org.//;
	$newconfig=~s/\./-/g;
	if ( $project eq "emi" ) { $newconfig=~s/^glite/emi/; }

	$moduleName=$module;
	if ( $project eq "emi" ) { $moduleName=~s/^org.glite/emi/; }

	printf("\nNew configuration:\t$newconfig\n\nPreparing...\n");

	open NEWCONF, ">", "$TMPDIR/$newconfig.ini.$$" or die $!;

	printf (NEWCONF "[Configuration-$newconfig]\nprofile = None\nmoduleName = $moduleName\ndisplayName = $newconfig\ndescription = None\nprojectName = $project\nage = $age\ntag = $tag\nversion = $major.$minor.$revision\npath = None\n\n");

	if ( $project eq "emi" ) { printf (NEWCONF "[Platform-default:Property]\n\n"); }

#	printf (NEWCONF "[Platform-default:VcsCommand]\ndisplayName = None\ndescription = HEAD CVS commands\ntag = cvs -d \${vcsroot} tag -R \${tag} \${moduleName}\nbranch = None\ncommit = None\ncheckout = cvs -d \${vcsroot} co -r \${tag} \${moduleName}\n\n");

#	printf (NEWCONF "[Platform-default:Environment]\nHOME = \${workspaceDir}");

	printf (NEWCONF "\n\n[Hierarchy]\n");

	foreach $m (@modules) {
	        open MOD, "$m/project/version.properties" or die "$m/project/version.properties: $?\n";

		$m_major=0; $m_minor=0; $m_revision=0; $m_age=0;

	        while ($_ = <MOD>) {
        	        chomp;

			if(/module\.version\s*=\s*(\d*)\.(\d*)\.(\d*)/) {
				$m_major=$1;
				$m_minor=$2;
				$m_revision=$3;
			}
                	if(/module\.age\s*=\s*(\S+)/) {
				$m_age=$1;
			}
		}

		$modconfig="$m_$m" . "_R_$m_major" . "_$m_minor" . "_$m_revision" . "_$m_age";
		$modconfig=~s/^org.//;
		$modconfig=~s/\./-/g;
		if ( $project eq "emi" ) { 
			$modconfig=~s/^glite/emi/; 
			$m=~s/^org.glite/emi/;
		}

#		system("echo $m = $modconfig >> $TMPDIR/$newconfig.ini.$$");
		printf(NEWCONF "$m = $modconfig\n");

		close (MOD);
        }

	close(NEWCONF);

	printf(EXEC "\n#Add new configuration\netics-configuration add -i $TMPDIR/$newconfig.ini.$$\n");
	printf(EXEC "etics-commit\n");


	# **********************************
	# Final bows
	# **********************************

	close(EXEC);

	system("chmod +x \"$TMPDIR/tag-with-subsystems-$module.$major.$minor.$revision-$age.sh\"");

	printf("\n\n---------\nDone!\n\nExecution script written in:\t$TMPDIR/tag-with-subsystems-$module.$major.$minor.$revision-$age.sh\nNew configuration written in:\t$TMPDIR/$newconfig.ini.$$\n\n");

