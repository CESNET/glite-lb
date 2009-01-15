#!/usr/bin/perl

use Getopt::Std;
use Switch;

#getopts('i:');

$TMPDIR=$ENV{'TMPDIR'};

$module = shift;

$usage = qq{
usage: $0 module.name
};

	die $usage unless $module;

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

	@modules=split(/\s+/, `PATH=\$PATH:./:./org.glite.lb configure --listmodules lb`);

	my $incmajor=0;
	my $incminor=0;
	my $increvision=0;
	my $incage=0;


	# **********************************
	# Iterate through modules and find out what has changed
	# **********************************

	foreach $m (@modules) {
		printf("\n***$m\n");

		$old_major=-1; $old_minor=-1; $old_revision=-1; $old_age=-1;
		$new_major=-1; $new_minor=-1; $new_revision=-1; $new_age=-1;

		foreach $l (`cvs diff -r GLITE_RELEASE_3_0_0 $m/project/version.properties | grep -E "module\.age|module\.version"`) {
			chomp($l);
			#printf("$l\n");

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
		elsif ($old_minor != $new_minor) {
			$incminor++;
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
		$age=1+1;} 
	else {
		printf("No change in either version component.\nAbort by pressing Ctrl+C or enter new age manually.\nUse a number or a word: ");
		$major=$current_major;
		$minor=$current_minor;
		$revision=$current_revision;
		$age=<STDIN>;}

	$tag="$current_prefix" . "$major" . "_$minor" . "_$revision" . "_$age";

	printf("\nNew tag: $tag\n\n");

	die "This tag already exists; reported by assertion" unless system("cvs log -h $module/Makefile | grep \"$tag\"");

	# **********************************
	# Create the execution script
	# **********************************

	open EXEC, ">", "./tag-with-subsystems-$module.$major.$minor.$revision-$age.sh" or die $!;

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


	printf("\nNew configuration:\t$newconfig\n\nPreparing...\n");

	open NEWCONF, ">", "$TMPDIR/$newconfig.ini.$$" or die $!;

	printf (NEWCONF "[Configuration-$newconfig]\nprofile = None\nmoduleName = $module\ndisplayName = $newconfig\ndescription = None\nprojectName = org.glite\nage = $age\ntag = $tag\nversion = $major.$minor.$revision\npath = None\n\n");

	printf (NEWCONF "[Platform-default:VcsCommand]\ndisplayName = None\ndescription = HEAD CVS commands\ntag = cvs -d \${vcsroot} tag -R \${tag} \${moduleName}\nbranch = None\ncommit = None\ncheckout = cvs -d \${vcsroot} co -r \${tag} \${moduleName}\n\n");

	printf (NEWCONF "[Platform-default:Environment]\nHOME = \${workspaceDir}\n\n[Hierarchy]\n");

	close(NEWCONF);

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

		system("echo $m = $modconfig >> $TMPDIR/$newconfig.ini.$$");

		close (MOD);
        }

	printf(EXEC "\n#Add new configuration\netics-configuration add -i $TMPDIR/$newconfig.ini.$$ -c $newconfig $module\n");


	# **********************************
	# Final bows
	# **********************************

	close(EXEC);

	system("chmod +x \"./tag-with-subsystems-$module.$major.$minor.$revision-$age.sh\"");

	printf("\n\n---------\nFinished!\n\nExecution script written in:\t./tag-with-subsystems-$module.$major.$minor.$revision-$age.sh\nNew configuration written in:\t$TMPDIR/$newconfig.ini.$$\n\n");

