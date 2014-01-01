package org.glite.lb;


public class JNILoader {


/**
 * Simple JNI library load.
 */
public static boolean loadLibrary(String libname) {
	System.loadLibrary(libname);
	return true;
}


}
