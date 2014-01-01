package org.glite.lb;


/**
 * JNI library loader for Fedora/EPEL.
 */
public class JNILoader extends JNIHelper {


/**
 * JNI library load for Fedora/EPEL. It tries to load from /usr/lib64/%{name} and /usr/lib/%{name}, see http://fedoraproject.org/wiki/Packaging:Java#Packaging_JAR_files_that_use_JNI.
 */
public static boolean loadLibrary(String libname) {
	String[] dirs = {"/usr/lib64/glite-lb-client-java", "/usr/lib/glite-lb-client-java"};
	String os;

	os = System.getProperty("os.name");
	if (os.equals("Linux")) {
		return JNIHelper.loadHelper(dirs, "lib" + libname);
	} else {
		System.loadLibrary(libname);
		return true;
	}
}


}
