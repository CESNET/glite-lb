#!/usr/bin/perl

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


	$module=~/\.([^\.]+?)$/;

	$subsysname=$1;

        @modules=split(/\s+/, `PATH=\$PATH:./:./org.glite.lb configure --listmodules $subsysname`);

	$newconfig="glite-$subsysname" . "_$branch";

	# **********************************
	# Create the execution script
	# **********************************

	open EXEC, ">", "$TMPDIR/etics-tag-with-subsystems-${subsysname}_$branch.sh" or die $!;

	printf (EXEC "#This script creates creates a _dev configuration for the $module subsystem, branch $branch\n#Generated automatically by $0\n\n"); 


        printf("\nNew configuration:\t$newconfig\n\nPreparing...\n");

        open NEWCONF, ">", "$TMPDIR/$newconfig.ini.$$" or die $!;

        printf (NEWCONF "[Configuration-$newconfig]\nprofile = None\nmoduleName = $module\ndisplayName = $newconfig\ndescription = None\nprojectName = org.glite\nage = $current_age\ntag = $branch\nversion = $current_major.$current_minor.$current_revision\npath = None\n\n");

        printf (NEWCONF "[Platform-default:VcsCommand]\ndisplayName = None\ndescription = HEAD CVS commands\ntag = cvs -d \${vcsroot} tag -R \${tag} \${moduleName}\nbranch = None\ncommit = None\ncheckout = cvs -d \${vcsroot} co -r \${tag} \${moduleName}\n\n");

        printf (NEWCONF "[Platform-default:Environment]\nHOME = \${workspaceDir}\n\n[Hierarchy]\n");

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

                $modconfig="$m_$m" . "_$branch";
                $modconfig=~s/^org.//;
                $modconfig=~s/\./-/g;

#               system("echo $m = $modconfig >> $TMPDIR/$newconfig.ini.$$");
                printf(NEWCONF "$m = $modconfig\n");

                close (MOD);
        }

        close(NEWCONF);


        printf (EXEC "#Find out if the configuration already exists in etics\n");
        printf (EXEC "echo Check if configuration exists\n\n");
        printf (EXEC "existconf=`etics-list-configuration $module | grep -w glite-${subsysname}_$branch`\n\n");
        printf (EXEC "echo Found: \$existconf\n");
        printf (EXEC "if [ \"\$existconf\" = \"\" ]; then\n");
        printf (EXEC "    echo New congiguration ... call etics-configuration add\n");
        printf (EXEC "    etics-configuration add -i $TMPDIR/$newconfig.ini.$$\n");
        printf (EXEC "else\n");
        printf (EXEC "    echo Congiguration already exists ... call etics-configuration modify\n");
        printf (EXEC "    etics-configuration modify -i $TMPDIR/$newconfig.ini.$$\n");
        printf (EXEC "fi\n\n");
        printf(EXEC "echo etics-commit\n");


        # **********************************
        # Final bows
        # **********************************

        close(EXEC);

        system("chmod +x \"$TMPDIR/etics-tag-with-subsystems-${subsysname}_$branch.sh\"");

        printf("\n\n---------\nFinished!\n\nExecution script written in:\t$TMPDIR/etics-tag-with-subsystems-${subsysname}_$branch.sh\nNew configuration written in:\t$TMPDIR/$newconfig.ini.$$\n\n");

