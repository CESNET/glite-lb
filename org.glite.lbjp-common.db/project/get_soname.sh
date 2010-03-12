#! /bin/sh
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

lib=""
filename="lib$1.so"
shift
for prefix in $@; do
	for dir in "$prefix" "$prefix/mysql"; do
		l=`find $dir -maxdepth 1 -name "${filename}"* 2>/dev/null | head -n 1`
		if [ -f "$l" ]; then
			lib=$l
			break
		fi
	done
	if [ x"" != x"$lib" ]; then
		break
	fi
done

if [ x"" != x"$lib" ]; then
	readelf -d $lib | grep SONAME | sed "s/.*\(${filename}.[0-9]\{1,\}\).*/\1/"
else
	echo notfound
fi
