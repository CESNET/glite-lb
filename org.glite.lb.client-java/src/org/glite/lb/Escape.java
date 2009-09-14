package org.glite.lb;

class Escape {
	public static String ulm(String in) {
		String out = in.replaceAll("\"", "\\\"");
		out = out.replaceAll("\n", "\\\n");
		return out;
	}
}
