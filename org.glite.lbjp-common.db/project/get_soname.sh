#! /bin/sh

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
