package org.glite.lb;


/**
 * JNI library helper loader.
 **/
public class JNIHelper {


/**
 * Helper function to try load from several locations.
 *
 * @param dirs list of paths to try
 * @param filename library file name
 * @return JNI successfully loaded
 */
protected static boolean loadHelper(String[] dirs, String filename) {
	int i;

	for (i = 0; i < dirs.length; i++) {
		try {
			System.load(dirs[i] + "/" + filename);
			return true;
		} catch (UnsatisfiedLinkError e) {
		} catch (Exception e) {
		}
	}

	return false;
}


}
