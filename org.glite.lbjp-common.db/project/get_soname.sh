#! /bin/sh

lib=""
for prefix in $@; do
	for dir in "$prefix" "$prefix/mysql"; do
		l=`find $dir -maxdepth 1 -name libmysqlclient.so* | head -n 1`
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
	readelf -d $lib | grep SONAME | sed 's/.*\(libmysqlclient.so.[0-9]\{1,\}\).*/\1/'
else
	echo notfound
fi
