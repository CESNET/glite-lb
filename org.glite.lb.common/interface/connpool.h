/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef GLITE_LB_CONNPOOL_H
#define GLITE_LB_CONNPOOL_H

#include "glite/security/glite_gss.h"

#ifdef BUILDING_LB_COMMON
#include "padstruct.h"
#include "context.h"
#include "lb_plain_io.h"
#include "authz.h"
#else
#include "glite/lb/padstruct.h"
#include "glite/lb/context.h"
#include "glite/lb/lb_plain_io.h"
#include "glite/lb/authz.h"
#endif

#ifdef GLITE_LB_THREADED
  #include <pthread.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EDG_WLL_CONNPOOL_DECLARED
#define EDG_WLL_CONNPOOL_DECLARED 1

#define	GLITE_LB_COMMON_CONNPOOL_SIZE		50
#define	GLITE_LB_COMMON_CONNPOOL_NOTIF_SIZE	50

glite_lb_padded_struct(_edg_wll_ConnPool,25,
/* address and port where we are connected to */
        char            *peerName;
        unsigned int    peerPort;

/* http(s) stream */
        edg_wll_GssCred   gsiCred;
        edg_wll_GssConnection   gss;
        char            *buf;
	int		bufPtr;
        int             bufUse;
	int		bufSize;

/* timestamp of usage of this entry in ctx.connPool */
        struct timeval  lastUsed;

/* Proxy/Cert file identification */

	struct stat	*certfile;	

/* output buffer */
	char	*bufOut;
	int	bufPtrOut;
	int	bufUseOut;	
	int	bufSizeOut;
);
typedef struct _edg_wll_ConnPool  edg_wll_ConnPool;
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
	int		connToUse;	/* active connection we are working with */

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

/** Initializes connNotif structure */
void edg_wll_initConnNotif(edg_wll_Connections *);


#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_CONNPOOL_H */
