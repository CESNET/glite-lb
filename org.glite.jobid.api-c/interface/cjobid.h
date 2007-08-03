#ifndef _GLITE_JOBID_H
#define _GLITE_JOBID_H

/*!
 * \file cjobid.h
 * \brief L&B consumer API
 */

#ident "$Header$"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _edg_wlc_JobId *glite_jobid_t;
typedef const struct _edg_wlc_JobId *glite_jobid_const_t;
typedef glite_jobid_t edg_wlc_JobId;

#define edg_wlc_JobIdCreate glite_jobid_create
#define edg_wlc_JobIdRecreate glite_jobid_recreate
#define edg_wlc_JobIdDup glite_jobid_dup
#define edg_wlc_JobIdFree glite_jobid_free
#define edg_wlc_JobIdParse glite_jobid_parse
#define edg_wlc_JobIdUnparse glite_jobid_unparse
#define edg_wlc_JobIdGetServer glite_jobid_getServer
#define edg_wlc_JobIdGetServerParts glite_jobid_getServerParts
#define edg_wlc_JobIdGetUnique glite_jobid_getUnique

#define GLITE_JOBID_DEFAULT_PORT 9000 /**< Default port where bookkeeping server listens */
#define GLITE_JOBID_PROTO_PREFIX "https://" /**< JobId protocol prefix */


/* All the pointer functions return malloc'ed objects (or NULL on error) */

/**
 * Create a Job ID.
 * See the lb_draft document for details on its construction and components
 * \param bkserver book keeping server hostname
 * \param port port for the bk service
 * \param jobid new created job id
 * \ret al 0 success
 * \retval EINVAL invalid bkserver
 * \retval ENOMEM if memory allocation fails
 */
int glite_jobid_create(const char * bkserver, int port, glite_jobid_t * jobid);

/**
 * Recreate a Job ID
 * \param bkserver bookkeeping server hostname
 * \param port port for the bk service
 * \param unique string which represent created jobid (if NULL then new
 * one is created)
 * \param jobid new created job id
 * \retval 0 success
 * \retval EINVAL invalid bkserver
 * \retval ENOMEM if memory allocation fails
 */
int glite_jobid_recreate(const char *bkserver, int port, const char * unique, edg_wlc_JobId * jobid);

/**
 * Create copy of Job ID
 * \param in jobid for duplication
 * \param jobid  duplicated jobid
 * \retval 0 for success
 * \retval EINVAL invalid jobid
 * \retval ENOMEM if memory allocation fails
 */
int glite_jobid_dup(glite_jobid_const_t in, glite_jobid_t * jobid);

/*
 * Free jobid structure
 * \param jobid for dealocation
 */
void glite_jobid_free(glite_jobid_t jobid);

/**
 * Parse Job ID string and creates jobid structure
 * \param jobidstr string representation of jobid
 * \param jobid parsed job id
 * \retval 0 for success
 * \retval EINVAL jobidstr can't be parsed
 * \retval ENOMEM if memory allocation fails
 */
int glite_jobid_parse(const char* jobidstr, glite_jobid_t * jobid);

/**
 * Unparse Job ID (produce the string form of JobId).
 * \param jobid to be converted to string
 * \return allocated string which represents jobid
 */
char* glite_jobid_unparse(const glite_jobid_t jobid);

/**
 * Extract bookkeeping server address (address:port)
 * \param jobid from which the bkserver address should be extracted
 * \retval pointer to allocated string with bkserver address
 * \retval NULL if jobid is 0 or memory allocation fails
 */
char* glite_jobid_getServer(glite_jobid_const_t jobid);

/**
 * Extract bookkeeping server address and port
 * \param jobid from which the bkserver address should be extracted
 * \param srvName pointer where to return server name
 * \param srvPort pointer where to return server port
 *     */
void glite_jobid_getServerParts(glite_jobid_const_t jobid, char **srvName, unsigned int *srvPort);

/**
 * Extract bookkeeping server address and port
 * \param jobid from which the bkserver address should be extracted
 * \param srvName pointer where to return server name
 * \param srvPort pointer where to return server port
 *     */
void glite_jobid_getServerParts_internal(glite_jobid_const_t jobid, char **srvName, unsigned int *srvPort);

/**
 * Extract unique string 
 * \param jobid
 * \retval pointer to allocated unique string representing jobid
 * \retval NULL if jobid is 0 or memory allocation fails
 */
char* glite_jobid_getUnique(glite_jobid_const_t jobid);

/**
 * Extract unique string 
 * \param jobid
 * \retval pointer to allocated unique string representing jobid
 * \retval NULL if jobid is 0 or memory allocation fails
 */
char* glite_jobid_getUnique_internal(glite_jobid_const_t jobid);

#ifdef __cplusplus
}
#endif

#endif /* _GLITE_JOBID_H */
