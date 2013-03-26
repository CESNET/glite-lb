#! /bin/sh
#
# generate a classpath with all axis dependencies
#

PREFIXES="${@:-'/usr/lib /usr/share/java'}"
LIST="activation ant-apache-bcel ant-apache-bsf ant-apache-log4j ant-apache-oro ant-apache-regexp ant-apache-resolver ant-apache-xalan2 ant-commons-logging ant-commons-net ant-javamail ant-jdepend ant-jmf ant-jsch ant-junit ant-nodeps ant-swing ant-trax axis axis-jaxrpc axis-saaj jaxrpc saaj commons-codec commons-discovery commons-lang commons-logging-adapters commons-logging-api commons-logging el-api gettext gnome-java-bridge gnumail gnumail-providers inetlib glite-jobid-api-java jsp-api libintl log4j servlet-api wsdl4j xercesImpl xml-apis"

CP=""
for prefix in $PREFIXES; do
	#echo $prefix >&2
	for pkgid in $LIST; do
		#echo $pkgid >&2
		for pkg in `ls -1 ${prefix}/${pkgid}*.jar ${prefix}/apache-${pkgid}*.jar ${prefix}/jakarta-${pkgid}*.jar 2>/dev/null`; do
			if ! test -h ${pkg}; then
				CP="$CP:${pkg}"
			fi
		done
	done
done

echo $CP | sed 's/^://'
