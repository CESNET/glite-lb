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
use File::Basename;

$TMPDIR=$ENV{'TMPDIR'};
$GLITE_LB_LOCATION=dirname $0;

if ($TMPDIR eq "") {$TMPDIR="/tmp";}

getopts('i:c:m:gh');

$module='org.gridsite.core';

$usage = qq{
usage: $0 -c <new configuration> 
	-c	Use this configuration (\d+\.\d+\.\d+-\S+) 
	-h	Display this help

};

# **********************************
# Interpret cmdline options
# **********************************

if (defined $opt_h) {die $usage};
die $usage unless $module;


if (defined $opt_c) {

	# **********************************
	# Parse the tag supplied by the user 
	# **********************************

	if ($opt_c=~/(\d+)\.(\d+)\.(\d+)-(\S+?)/) {
			$major=$1;
			$minor=$2;
			$revision=$3;
			$age=$4;
	}
	else {die ("tag not stated properly")};
}
else {
	die $usage;
}

printf("Tag: $tag\n\tprefix: $prefix\n\t major: $major\n\t minor: $minor\n\t   rev: $revision\n\t   age: $age\n");


# **********************************
# Create the execution script
# **********************************

open EXEC, ">", "$TMPDIR/etics-tag-gridsite.$major.$minor.$revision-$age.sh" or die $!;

printf (EXEC "#This script registers tags for the $module module, version $major.$minor.$revision-$age\n#Generated automatically by $0\n\n"); 

# **********************************
# Update version.properties
# **********************************

## printf(EXEC "#Generate new version.properties\ncat >$module/project/version.properties <<EOF\n# generated for LB configure --mode=etics only\nmodule.version=$major.$minor.$revision\nmodule.age=$age\nEOF\n\n");

# **********************************
# Etics configuration prepare / modify / upload
# **********************************

$newconfig="$module" . "_R_$major" . "_$minor" . "_$revision" . "_$age";
$newconfig=~s/^org.//;
$newconfig=~s/\./-/g;

$module=~/([^\.]+?)\.([^\.]+?)$/;
$subsysname=$1;
$modulename=$2;

printf("Module=$module\nname=$modulename\nsubsys=$subsysname\n");
system("$GLITE_LB_LOCATION/configure --mode=etics --module $subsysname.$modulename --output $TMPDIR/$newconfig.ini.$$ --version $major.$minor.$revision-$age");

printf(EXEC "\n#Add new configuration\netics-configuration add -i $TMPDIR/$newconfig.ini.$$ -c $newconfig $module\n"); 

$version="${major}_${minor}_${revision}_${age}";

$template = qq{
[Configuration-gridsite-MODULE_R_$version]
profile = None
moduleName = org.gridsite.MODULE
displayName = gridsite-MODULE_R_$version
description = gridsite-MODULE_R_$version
projectName = org.glite
age = $age
vcsroot = None
tag = None
version = $major.$minor.$revision
path = \${projectName}/org.gridsite.core/\${version}/\${platformName}/gridsite-core-\${version}-\${age}.tar.gz

[Platform-default:VcsCommand]
displayName = None
description = None
tag = None
branch = None
commit = None
checkout = echo No checkout required

[Platform-default:BuildCommand]
checkstyle = None
packaging = echo "building nothing, org.gridsite.core make rpm step will create this"
displayName = None
description = None
doc = None
publish = None
postpublish = None
compile = echo "building nothing, org.gridsite.core make rpm step will create this"
init = None
install = None
prepublish = None
test = None
clean = None
configure = None

[Platform-default:DynamicDependency]
org.glite|org.gridsite.core = B
};


for $mod (qw/apache commands shared/) {
	$conf = $template;
	$conf =~ s/MODULE/$mod/g;

	open CONF,">$TMPDIR/$subsysname-${mod}_R_$version.ini.$$";
	print CONF $conf;
	close CONF;

	print EXEC "etics-configuration add -i $TMPDIR/$subsysname-${mod}_R_$version.ini.$$\n";
}

printf(EXEC "etics-commit\n");

open CONF,">$TMPDIR/${subsysname}_R_$version.ini.$$";

print CONF qq{
[Configuration-gridsite_R_$version]
profile = None
moduleName = org.gridsite
displayName = gridsite_R_$version
description = gridsite_R_$version
projectName = org.glite
age = $age
vcsroot = None
tag = None
version = $major.$minor.$revision
path = None

[Hierarchy]
org.gridsite.apache = gridsite-apache_R_$version
org.gridsite.shared = gridsite-shared_R_$version
org.gridsite.commands = gridsite-commands_R_$version
org.gridsite.core = gridsite-core_R_$version
};

close CONF;

print EXEC "etics-configuration add -i $TMPDIR/${subsysname}_R_$version.ini.$$\n";

printf(EXEC "etics-commit\n");


# **********************************
# Final bows
# **********************************

close(EXEC);

system("chmod +x \"$TMPDIR/etics-tag-gridsite.$major.$minor.$revision-$age.sh\"");

printf("\n\n---------\nDone!\n\nExecution script written in:\t$TMPDIR/etics-tag-gridsite.$major.$minor.$revision-$age.sh\n");
printf("New configuration written in:\t$TMPDIR/${subsysname}_R_$version.ini.$$\n\n");

