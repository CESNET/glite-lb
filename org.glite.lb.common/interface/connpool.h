#include "glite/security/glite_gss.h"
#include "glite/lb/consumer.h"
#include "lb_plain_io.h"
#include "authz.h"
#ifdef GLITE_LB_THREADED
  #include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EDG_WLL_CONNPOOL_DECLARED
#define EDG_WLL_CONNPOOL_DECLARED 1

#define	GLITE_LB_COMMON_CONNPOOL_SIZE	50


typedef struct _edg_wll_ConnPool {
/* address and port where we are connected to */
        char            *peerName;
        unsigned int    peerPort;

/* http(s) stream */
        gss_cred_id_t   gsiCred;
        edg_wll_GssConnection   gss;
        char            *buf;
        int             bufUse,bufSize;

/* timestamp of usage of this entry in ctx.connPool */
        struct timeval  lastUsed;
} edg_wll_ConnPool;
#endif


#ifndef EDG_WLL_CONNECIONS_DECLARED
#define EDG_WLL_CONNECIONS_DECLARED 1
typedef struct _edg_wll_Connections {

/* connection pool array */
        edg_wll_ConnPool        *connPool;
	edg_wll_ConnPool	*serverConnection;	/* this is in fact a single-item array intended for use
							   by the server (since the server only uses one connection)*/

/* pool of connections from client */
        int             poolSize;       /* number of connections in the pool */
        int             connOpened;     /* number of opened connections  */

/* Connection pool locks & accessories.*/
#ifdef GLITE_LB_THREADED
        pthread_mutex_t poolLock;       /* Global pool lock (to be used for pool-wide operations carried out locally) */
        pthread_mutex_t *connectionLock;      /* Per-connection lock (used to lock out connections that are in use) */
#endif 
	edg_wll_Context	*locked_by;	/* identifies contexts that have been used to lock a connection since they
                                           do probably have a connToUse value stored in them*/

} edg_wll_Connections;
#endif


/* Connections Global Handle */
extern edg_wll_Connections connectionsHandle;

/** ** Locking Functions (wrappers) **/
/*  Introduced to simplify debugging and switching betwen single/multi-threaded behavior */

/** Lock (try) the pool */
int edg_wll_poolTryLock();

/** Lock the pool (blocking) */
int edg_wll_poolLock();

/** Unlock the pool */
int edg_wll_poolUnlock();


/** Lock (try) a single connection */
int edg_wll_connectionTryLock(edg_wll_Context ctx, int index);

/** Lock a single connection (blocking) */
int edg_wll_connectionLock(edg_wll_Context ctx, int index);

/** Unlock a connection */
int edg_wll_connectionUnlock(edg_wll_Context ctx, int index);


/** Free memory used by the pool and lock array */
void edg_wll_poolFree();

/** Allocate memory for the edg_wll_Connections structure and its properties and return a pointer.
    in case memory has been already allocated, just return a pointer */
edg_wll_Connections* edg_wll_initConnections();

#ifdef __cplusplus
}
#endif

