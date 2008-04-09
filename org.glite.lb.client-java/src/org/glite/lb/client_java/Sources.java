package org.glite.lb.client_java;

/**
 * Enum which represents type if sources.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public enum Sources {
    EDG_WLL_SOURCE_NONE,            /* uninitialized value */
    EDG_WLL_SOURCE_USER_INTERFACE,
    EDG_WLL_SOURCE_NETWORK_SERVER,
    EDG_WLL_SOURCE_WORKLOAD_MANAGER,
    EDG_WLL_SOURCE_BIG_HELPER,
    EDG_WLL_SOURCE_JOB_SUBMISSION,
    EDG_WLL_SOURCE_LOG_MONITOR,
    EDG_WLL_SOURCE_LRMS,
    EDG_WLL_SOURCE_APPLICATION,
    EDG_WLL_SOURCE_LB_SERVER
}
