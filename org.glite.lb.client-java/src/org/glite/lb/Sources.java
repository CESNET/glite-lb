package org.glite.lb;

/* FIXME: cleanup required */

/**
 * Enum which represents type if sources.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public class Sources {
    public static final int NONE = 0;            /* uninitialized value */
    public static final int USER_INTERFACE = 1;
    public static final int NETWORK_SERVER = 2;
    public static final int WORKLOAD_MANAGER = 3;
    public static final int BIG_HELPER = 4;
    public static final int JOB_SUBMISSION = 5;
    public static final int LOG_MONITOR = 6;
    public static final int LRMS = 7;
    public static final int APPLICATION = 8;
    public static final int LB_SERVER = 9;
    public static final int CREAM_CORE = 10;
    public static final int BLAH = 11;

    public int source;
    private String[] names = {
	"None",
	"UserInterface",
	"NetworkServer",
	"WorkloadManager",
	"BigHelper",
	"JobSubmission",
	"LogMonitor",
	"LRMS",
	"Application",
	"LBServer",
	"CREAMCore",
	"BLAH",
    };
    
    public Sources() {
	this.source = 0;
    }

    public static void check(int source) {
	if (source < 1 || source > 11) 
		throw new IllegalArgumentException("lb.Source");
    }
    
    public Sources(int source) {
	check(source);
	this.source = source;
    }

    public String toString() {
	return names[source];
    }
}
