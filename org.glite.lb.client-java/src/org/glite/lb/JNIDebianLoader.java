package org.glite.lb;


/**
 * JNI library loader for Debian.
 */
public class JNILoader {


/**
 * JNI library load for Debian. It loads from /usr/lib/jni, see http://www.debian.org/doc/packaging-manuals/java-policy/x104.html.
 */
public static boolean loadLibrary(String libname) {
	String os;

	os = System.getProperty("os.name");
	if (os.equals("Linux")) {
		System.load("/usr/lib/jni/lib" + libname);
		return true;
	} else {
		System.loadLibrary(libname);
		return true;
	}
}


}
