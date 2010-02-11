#!/bin/bash


if [ "$1" = "" ]; then 
	echo "No directory given. Using \".\""
	DIR="." 
else
	DIR=$1
fi

if [ "$TMPDIR" = "" ]; then
	echo TMPDIR not set. Using /tmp
	TMPDIR="/tmp"
fi

TOTALFOUND=0
TOTALFIXEDLIC=0
TOTALFIXEDCOP=0

# Check is a specific binary exists
function check_exec()
{
        if [ -z $1 ]; then
                set_error "No binary to check"
                return $TEST_ERROR
        fi
        local ret=`which $1 2> /dev/null`
        if [ -n "$ret" -a -x "$ret" ]; then
                return $TEST_OK
        else
                return $TEST_ERROR
        fi
}


# Check the list of required binaries
function check_binaries()
{
        local ret=$TEST_OK
        for file in $@
        do
		printf "$file... "
                check_exec $file
                if [ $? -gt 0 ]; then
                        echo "$file not found" > /dev/stderr
                        ret=1
		else
			echo "OK"
                fi
		
        done
        return $ret
}

function check_file()
{
file=$1

# strip file
# - comment mark at the beginning of a line
# - comment mark at the end of a line
# - dates
# - spaces

stripped_contents=`cat ${file} |
sed --posix --regexp-extended \
    -e 's:^[[:space:]]*(\#|//|/\*|\*)[[:space:]]*::' \
    -e 's:[[:space:]]*(\#|//|\*/|\*)[[:space:]]*$::' \
    -e 's/[[:digit:]]{4}( ?[,-] ?[0-9]{4})*//' |
tr -d '[[:space:]]'`

# is copyright present?
echo ${stripped_contents} | grep -q ${stripped_copyright} 2>/dev/null
r1=$?

# is license present?
echo ${stripped_contents} | grep -q ${stripped_license} 2>/dev/null
r2=$?

ret=$(( r1 + r2 * 2 ))

#echo ${file}:$((1 - r1)):$((1 - r2))
return $ret
}

function fix_c_style_sources() 
{
srcfile=$1
let TOTALFOUND=$TOTALFOUND+1		

lineno=`grep -n "\$Header$srcfile | sed 's/:.*$//'`
if [ "$lineno" == "" ]; then
	lineno=0
fi
let nextlineno=$lineno+1

printf "$srcfile: Needs fixing ($checkresult), lineno $lineno, $nextlineno\n"

head -n $lineno $srcfile > $TMPDIR/egee_license.$$.swp
printf "/*\n" >> $TMPDIR/egee_license.$$.swp
if [ "$checkresult" == "1" ] || [ "$checkresult" == "3" ]; then
	printf "$COPYRIGHT\n\n" >> $TMPDIR/egee_license.$$.swp
	let TOTALFIXEDCOP=$TOTALFIXEDCOP+1
fi
if [ "$checkresult" -gt "1" ]; then
	printf "$LICENSE\n" >> $TMPDIR/egee_license.$$.swp
	let TOTALFIXEDLIC=$TOTALFIXEDLIC+1
fi
printf "*/\n\n" >> $TMPDIR/egee_license.$$.swp
tail -n +$nextlineno $srcfile >> $TMPDIR/egee_license.$$.swp
}

function fix_sh_style_sources()
{
srcfile=$1
prefix=$2
let TOTALFOUND=$TOTALFOUND+1

lineno=`grep -n "\$Header$srcfile | sed 's/:.*$//'`
shlineno=`head -n 1 $srcfile | grep -n '^#! */' | sed 's/:.*$//'`
if [ "$lineno" == "" ]; then
        lineno=0
fi
if [ "$shlineno" == "" ]; then
        shlineno=0
fi
if [ "$shlineno" -gt "$lineno" ]; then
	lineno=$shlineno
fi

let nextlineno=$lineno+1

printf "$srcfile: Needs fixing ($checkresult), lineno $lineno, $nextlineno\n"

head -n $lineno $srcfile > $TMPDIR/egee_license.$$.swp
if [ "$checkresult" == "1" ] || [ "$checkresult" == "3" ]; then
        printf "$COPYRIGHT\n\n" | sed "s/^/$prefix /" >> $TMPDIR/egee_license.$$.swp
        let TOTALFIXEDCOP=$TOTALFIXEDCOP+1
fi
if [ "$checkresult" -gt "1" ]; then
        printf "$LICENSE\n" | sed "s/^/$prefix /" >> $TMPDIR/egee_license.$$.swp
        let TOTALFIXEDLIC=$TOTALFIXEDLIC+1
fi
tail -n +$nextlineno $srcfile >> $TMPDIR/egee_license.$$.swp
}


##################################################
# This is where the actual execution starts
##################################################

echo Checking binaries

check_binaries head tail find grep sed tr cat

if [ $? -gt 0 ]; then
	echo "$0: Unable to find all binaries" > /dev/stderr
	exit 1
fi;

LICENSE='Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.'

COPYRIGHT='Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.'

stripped_copyright=`printf "$COPYRIGHT" | sed --posix --regexp-extended -e 's/[[:digit:]]{4}( ?[,-] ?[0-9]{4})*//' | tr -d '[[:space:]]'`
stripped_license=`printf "$LICENSE" | tr -d '[[:space:]]'`

# XXX: for testing. May be removed later:
rm $TMPDIR/egee*swp

#ANSI C files
echo Processing ANSI C files
filelist=`find $DIR -name "*.[ch]"`

for srcfile in $filelist
do
	check_file $srcfile
	checkresult=$?

	if [ "$checkresult" -gt "0" ]; then
		fix_c_style_sources $srcfile
	else
		printf "$srcfile $checkresult [OK]\n"
	fi
done

#CPP files
echo Processing C++ files
filelist=`find $DIR -iname "*.cpp"`

for srcfile in $filelist
do
	check_file $srcfile
	checkresult=$?

	if [ "$checkresult" -gt "0" ]; then
		fix_c_style_sources $srcfile
	else
		printf "$srcfile $checkresult [OK]\n"
	fi
done

#Java files
echo Processing Java files
filelist=`find $DIR -iname "*.cpp"`

for srcfile in $filelist
do
	check_file $srcfile
	checkresult=$?

	if [ "$checkresult" -gt "0" ]; then
		fix_c_style_sources $srcfile
	else
		printf "$srcfile $checkresult [OK]\n"
	fi
done

#sh files
echo Processing shell files
filelist=`find $DIR -iname "*.sh"`

for srcfile in $filelist
do
	check_file $srcfile
	checkresult=$?

	if [ "$checkresult" -gt "0" ]; then
		fix_sh_style_sources $srcfile '#'
	else
		printf "$srcfile $checkresult [OK]\n"
	fi
done

#TeX files
echo Processing TeX files
filelist=`find $DIR -iname "*.tex"`

for srcfile in $filelist
do
	check_file $srcfile
	checkresult=$?

	if [ "$checkresult" -gt "0" ]; then
		fix_sh_style_sources $srcfile '%'
	else
		printf "$srcfile $checkresult [OK]\n"
	fi
done

#Perl files
echo Processing Perl files
filelist=`find $DIR -iname "*.pl"`

for srcfile in $filelist
do
	check_file $srcfile
	checkresult=$?

	if [ "$checkresult" -gt "0" ]; then
		fix_sh_style_sources $srcfile '#'
	else
		printf "$srcfile $checkresult [OK]\n"
	fi
done

printf "\n\nTotal files found:\t $TOTALFOUND\nTotal copyrights fixed:\t $TOTALFIXEDCOP\nTotal licenses fixed:\t $TOTALFIXEDLIC\n";
