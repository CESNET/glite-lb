#ifndef GLITE_LB_TIMEOUTS_H
#define GLITE_LB_TIMEOUTS_H

/** 
 * default and maximal notif timeout (in seconds)
 */
#define EDG_WLL_NOTIF_TIMEOUT_DEFAULT   120
#define EDG_WLL_NOTIF_TIMEOUT_MAX       1800

/**
 * default and maximal query timeout (in seconds)
 */
#define EDG_WLL_QUERY_TIMEOUT_DEFAULT   120
#define EDG_WLL_QUERY_TIMEOUT_MAX       1800

/**
 * default and maximal logging timeout (in seconds)
 */
#define EDG_WLL_LOG_TIMEOUT_DEFAULT		120
#define EDG_WLL_LOG_TIMEOUT_MAX			300
#define EDG_WLL_LOG_SYNC_TIMEOUT_DEFAULT	120
#define EDG_WLL_LOG_SYNC_TIMEOUT_MAX		600

#endif /* GLITE_LB_TIMEOUTS_H */
