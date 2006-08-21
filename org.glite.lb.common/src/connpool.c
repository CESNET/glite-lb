#include "connpool.h"

edg_wll_Connections connectionsHandle = 
  { NULL , NULL , EDG_WLL_LOG_CONNECTIONS_DEFAULT , 0 , PTHREAD_MUTEX_INITIALIZER , NULL , NULL};


/** Lock (try) the pool */
int edg_wll_poolTryLock() {
int RetVal;

  #ifdef THR
    RetVal = pthread_mutex_trylock(&edg_wll_Connections.poolLock);
  #endif

  #ifndef THR
    RetVal = 0;
  #endif


  return(RetVal);
}


/** Lock the pool (blocking) */
int edg_wll_poolLock() {
int RetVal;

  #ifdef THR
    RetVal = pthread_mutex_lock(&edg_wll_Connections.poolLock);
  #endif

  #ifndef THR
    RetVal = 0;
  #endif


  return(RetVal);
}


/** Unlock the pool */
int edg_wll_poolUnlock() {
int RetVal;

  #ifdef THR
    RetVal = pthread_mutex_unlock(&edg_wll_Connections.poolLock);
  #endif

  #ifndef THR
    RetVal = 0;
  #endif


  return(RetVal);
}


/** Lock (try) a single connection */
int edg_wll_connectionTryLock(edg_wll_Context ctx, int index) {
int RetVal;

  #ifdef THR
    RetVal = pthread_mutex_trylock(&edg_wll_Connections.connectionLock[index]); /* Try to lock the connection */
    if (!RetVal) connectionsHandle.locked_by[index] = (void*)ctx;		/* If lock succeeded, store the
										   locking context address */
  #endif

  #ifndef THR
    RetVal = 0;
  #endif


  return(RetVal);
}


/** Lock a single connection (blocking) */
int edg_wll_connectionLock(edg_wll_Context ctx, int index) {
int RetVal;

  #ifdef THR
    RetVal = pthread_mutex_lock(&edg_wll_Connections.connectionLock[index]);	/* Lock the connection (wait if
										   not available)*/
    if (!RetVal) connectionsHandle.locked_by[index] = (void*)ctx;               /* If lock succeeded, store the
                                                                         	   locking context address */
  #endif

  #ifndef THR
    RetVal = 0;
  #endif


  return(RetVal);
}


/** Unlock a connection */
int edg_wll_connectionUnlock(edg_wll_Context ctx, int index) {
int RetVal;

  #ifdef THR
    RetVal = pthread_mutex_unlock(&edg_wll_Connections.connectionLock[index]);
    if (!RetVal) connectionsHandle.locked_by[index] = NULL;
  #endif

  #ifndef THR
    RetVal = 0;
  #endif


  return(RetVal);
}


/** Free all memory used by the pool and lock-related arrays */
void edg_wll_poolFree() {
	int i;
        struct timeval close_timeout = {0, 50000};	/* Declarations taken over from edg_wll_FreeContext() */
        OM_uint32 min_stat;				/* Does not seem to have any use whatsoever - neither
							   here nor in edg_wll_FreeContext() */


        printf("edg_wll_poolFree Checkpoint\n");

        for (i=0; i<connectionsHandle.poolSize; i++) {
		if (connectionsHandle.connPool[i].peerName) free(connectionsHandle.connPool[i].peerName);
                edg_wll_gss_close(&connectionsHandle.connPool[i].gss,&close_timeout);
                if (connectionsHandle.connPool[i].gsiCred)
                	gss_release_cred(&min_stat, &connectionsHandle.connPool[i].gsiCred);
                if (connectionsHandle.connPool[i].buf) free(connectionsHandle.connPool[i].buf);
        }

	edg_wll_poolLock();
	free(connectionsHandle.connectionLock);
	free(connectionsHandle.serverConnection);
	free(connectionsHandle.connPool);
	free(connectionsHandle.locked_by);
	connectionsHandle.connectionLock = NULL;
	connectionsHandle.serverConnection = NULL;
        connectionsHandle.connPool = NULL;
        connectionsHandle.locked_by = NULL;


}

/** Allocate memory for arrays within the edg_wll_Connections structure */
edg_wll_Connections* edg_wll_initConnections() {


  if((connectionsHandle.connPool == NULL) &&
     (connectionsHandle.poolSize > 0)) { /* We need to allocate memory for the connPool and connectionLock arrays */ 
    connectionsHandle.connPool = (edg_wll_ConnPool *) calloc(connectionsHandle.poolSize, sizeof(edg_wll_ConnPool));
    connectionsHandle.connectionLock = (pthread_mutex_t *) calloc(connectionsHandle.poolSize, sizeof(pthread_mutex_t));
    connectionsHandle.locked_by = (edg_wll_Context) calloc(connectionsHandle.poolSize, sizeof(edg_wll_Context));

  }
  if(connectionsHandle.serverConnection == NULL) {
    connectionsHandle.serverConnection = (edg_wll_ConnPool *) calloc(1, sizeof(edg_wll_ConnPool));
  }


  return (&connectionsHandle);
}


