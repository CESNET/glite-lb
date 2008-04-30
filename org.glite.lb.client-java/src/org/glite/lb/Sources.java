package org.glite.lb;

/**
 * Enum which represents type if sources.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public class Sources {
    public static final int EDG_WLL_SOURCE_NONE = 0;            /* uninitialized value */
    public static final int EDG_WLL_SOURCE_USER_INTERFACE = 1;
    public static final int EDG_WLL_SOURCE_NETWORK_SERVER = 2;
    public static final int EDG_WLL_SOURCE_WORKLOAD_MANAGER = 3;
    public static final int EDG_WLL_SOURCE_BIG_HELPER = 4;
    public static final int EDG_WLL_SOURCE_JOB_SUBMISSION = 5;
    public static final int EDG_WLL_SOURCE_LOG_MONITOR = 6;
    public static final int EDG_WLL_SOURCE_LRMS = 7;
    public static final int EDG_WLL_SOURCE_APPLICATION = 8;
    public static final int EDG_WLL_SOURCE_LB_SERVER = 9;
    
    public int source;
    
    public Sources() {
	this.source = 0;
    }
    
    public Sources(int source) {
	this.source = source;
    }
}
