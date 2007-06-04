#ifndef __EDG_WORKLOAD_LOGGING_COMMON_NOTIFID_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_NOTIFID_H__

#ident "$Header$"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup notifid Notification Id (NotifId)
 * \brief NotifId description and handling.
 *@{
 */

/** Notification handle.
 * Refers to a particular registration for receiving notifications.
 */
typedef void *edg_wll_NotifId;

/**
 * Create a Job ID.
 * \param[in] server		notification server hostname
 * \param[in] port		port of the notification server
 * \param[out] notifid		newly created NotifId
 * \retval 0 for success
 * \retval EINVAL invalid notification server
 * \retval ENOMEM if memory allocation fails
 */
int edg_wll_NotifIdCreate(const char *server, int port ,edg_wll_NotifId *notifid);

/**
 * Free the NotifId structure
 * \param[in] notifid 		for dealocation
 */
void edg_wll_NotifIdFree(edg_wll_NotifId notifid);

/** Parse the NotifId string and creates NotifId structure
 * \param[in] notifidstr	string representation of NotifId
 * \param[out] notifid		parsed NotifId
 * \retval 0 for success
 * \retval EINVAL notifidstr can't be parsed
 * \retval ENOMEM if memory allocation fails
 */
int edg_wll_NotifIdParse(const char *notifidstr, edg_wll_NotifId *notifid);

/** Unparse the NotifId (produce the string form of NotifId).
 * \param[in] notifid		NotifId to be converted to string
 * \return allocated string which represents the NotifId
 */
char* edg_wll_NotifIdUnparse(const edg_wll_NotifId notifid);

/**
 * Extract notification server address (address:port)
 * \param[in] notifid 		NotifId from which the address should be extracted
 * \param[in,out] srvName	pointer where to return server name
 * \param[in,out] srvPort	pointer where to return server port
 */
void edg_wll_NotifIdGetServerParts(const edg_wll_NotifId notifid, char **srvName, unsigned int *srvPort);

/**
 * Extract unique string 
 * \param[in] notifid	NotifId 
 * \retval pointer to allocated unique string representing jobid
 * \retval NULL if jobid is 0 or memory allocation fails
 */ 
char *edg_wll_NotifIdGetUnique(const edg_wll_NotifId notifid);

/**
 * Recreate a NotifId by a new unique string
 * \param[in] unique	string which represent created notifid (if NULL then new
 * one is created)
 * \param[in,out] notifid       newly created NotifId 
 * \retval 0 success
 * \retval EINVAL invalid NotifId
 * \retval ENOMEM if memory allocation fails
 */
int edg_wll_NotifIdSetUnique(edg_wll_NotifId *notifid, const char *unique);

/**
 * Duplicate a NotifId
 * \param[in] src	a notifid to duplicate
 * \retval pointer to allocated memory containg the duplicated notifid
 * \retval NULL if memory allocation fails or notifid is invalid
 */
edg_wll_NotifId *edg_wll_NotifIdDup(const edg_wll_NotifId src);

/*
 *@} end of group
 */

#ifdef __cplusplus
}
#endif
#endif
