#include "connpool.h"

#ifdef GLITE_LB_THREADED
edg_wll_Connections connectionsHandle = 
  { NULL , NULL , GLITE_LB_COMMON_CONNPOOL_SIZE , 0 , PTHREAD_MUTEX_INITIALIZER , NULL , NULL};
#endif

#ifndef GLITE_LB_THREADED
edg_wll_Connections connectionsHandle =
  { NULL , NULL , GLITE_LB_COMMON_CONNPOOL_SIZE , 0 , NULL};
#endif

/** Lock (try) the pool */
int edg_wll_poolTryLock() {
int RetVal;

  #ifdef GLITE_LB_THREADED  /* Threaded version */
    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Thread %d trying to lock the pool ",pthread_self());
    #endif
    RetVal =
        pthread_mutex_trylock(
           &connectionsHandle.poolLock
        );

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("- result %d\n",RetVal);
    #endif
  #endif


  #ifndef GLITE_LB_THREADED  /* Single-thread version */
    RetVal = 0;

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Dummy - Trying to lock the pool (pthreads not included)\n");
    #endif
  #endif


  return(RetVal);
}


/** Lock the pool (blocking) */
int edg_wll_poolLock() {
int RetVal;

  #ifdef GLITE_LB_THREADED
    
    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Thread %d trying to lock the pool\n",pthread_self());
    #endif
 
    RetVal = pthread_mutex_trylock(&connectionsHandle.poolLock);
   
    if (RetVal) {

      #ifdef EDG_WLL_CONNPOOL_DEBUG
        printf("Thread %d locking the pool\n",pthread_self());
      #endif
      RetVal = pthread_mutex_lock(&connectionsHandle.poolLock);
    }

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Thread %d - lock result %d\n",pthread_self(),RetVal);
    #endif
  #endif

  #ifndef GLITE_LB_THREADED
    RetVal = 0;

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Dummy - Locking the pool (pthreads not included)\n");
    #endif
  #endif


  return(RetVal);
}


/** Unlock the pool */
int edg_wll_poolUnlock() {
int RetVal;

  #ifdef GLITE_LB_THREADED
    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Thread %d unlocking the pool\n",pthread_self());
    #endif
    RetVal = pthread_mutex_unlock(&connectionsHandle.poolLock);

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Thread %d - unlock result %d\n",pthread_self(),RetVal);
    #endif

  #endif

  #ifndef GLITE_LB_THREADED
    RetVal = 0;

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Dummy - Unlocking the pool (pthreads not included)\n");
    #endif

  #endif


  return(RetVal);
}


/** Lock (try) a single connection */
int edg_wll_connectionTryLock(edg_wll_Context ctx, int index) {
int RetVal;

  #ifdef GLITE_LB_THREADED
    RetVal = pthread_mutex_trylock(&connectionsHandle.connectionLock[index]); /* Try to lock the connection */
    if (!RetVal) connectionsHandle.locked_by[index] = (void*)ctx;		/* If lock succeeded, store the
										   locking context address */

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Thread %d trying to lock connection No. [%d] - result %d\n",pthread_self(),index,RetVal);
    #endif

  #endif

  #ifndef GLITE_LB_THREADED
    RetVal = 0;

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Dummy - Trying to lock connection No. [%d] (pthreads not included)\n",index);
    #endif

  #endif


  return(RetVal);
}


/** Lock a single connection (blocking) */
int edg_wll_connectionLock(edg_wll_Context ctx, int index) {
int RetVal;

  #ifdef GLITE_LB_THREADED
    RetVal = pthread_mutex_lock(&connectionsHandle.connectionLock[index]);	/* Lock the connection (wait if
										   not available)*/
    if (!RetVal) connectionsHandle.locked_by[index] = (void*)ctx;               /* If lock succeeded, store the
                                                                         	   locking context address */

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Thread %d locking connection No. [%d] - result %d\n",pthread_self(),index,RetVal);
    #endif

  #endif

  #ifndef GLITE_LB_THREADED
    RetVal = 0;

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Dummy - locking connection No. [%d] (pthreads not included)\n",index);
    #endif

  #endif


  return(RetVal);
}


/** Unlock a connection */
int edg_wll_connectionUnlock(edg_wll_Context ctx, int index) {
int RetVal;

  #ifdef GLITE_LB_THREADED
    RetVal = pthread_mutex_unlock(&connectionsHandle.connectionLock[index]);
    if (!RetVal) connectionsHandle.locked_by[index] = NULL;

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Thread %d unlocking connection No. [%d] - result %d\n",pthread_self(),index,RetVal);
    #endif

  #endif

  #ifndef GLITE_LB_THREADED
    RetVal = 0;

    #ifdef EDG_WLL_CONNPOOL_DEBUG
      printf("Dummy - unlocking connection No. [%d] (pthreads not included)\n",index);
    #endif

  #endif


  return(RetVal);
}


/** Free all memory used by the pool and lock-related arrays */
void edg_wll_poolFree() {
	int i;
        struct timeval close_timeout = {0, 50000};	/* Declarations taken over from edg_wll_FreeContext() */
        OM_uint32 min_stat;				/* Does not seem to have any use whatsoever - neither
							   here nor in edg_wll_FreeContext() */

        #ifdef EDG_WLL_CONNPOOL_DEBUG
            #ifdef GLITE_LB_THREADED
                printf("Thread %d ",pthread_self());
            #endif
            printf("Entering edg_wll_poolFree\n");
        #endif


        for (i=0; i<connectionsHandle.poolSize; i++) {
		if (connectionsHandle.connPool[i].peerName) free(connectionsHandle.connPool[i].peerName);
                edg_wll_gss_close(&connectionsHandle.connPool[i].gss,&close_timeout);
                if (connectionsHandle.connPool[i].gsiCred)
                	gss_release_cred(&min_stat, &connectionsHandle.connPool[i].gsiCred);
                if (connectionsHandle.connPool[i].buf) free(connectionsHandle.connPool[i].buf);
        }

	edg_wll_poolLock();
#ifdef GLITE_LB_THREADED
	free(connectionsHandle.connectionLock);
	connectionsHandle.connectionLock = NULL;
#endif
	free(connectionsHandle.serverConnection);
	free(connectionsHandle.connPool);
	free(connectionsHandle.locked_by);
	connectionsHandle.serverConnection = NULL;
        connectionsHandle.connPool = NULL;
        connectionsHandle.locked_by = NULL;


}

/** Allocate memory for arrays within the edg_wll_Connections structure */
edg_wll_Connections* edg_wll_initConnections() {

	int i;

    #ifdef EDG_WLL_CONNPOOL_DEBUG
        #ifdef GLITE_LB_THREADED
            printf("Thread %d ",pthread_self());
        #endif
        printf("Entering edg_wll_initConnections\n");
    #endif

    if((connectionsHandle.connPool == NULL) &&
       (connectionsHandle.poolSize > 0)) { /* We need to allocate memory for the connPool and connectionLock arrays */ 

        #ifdef EDG_WLL_CONNPOOL_DEBUG
            #ifdef GLITE_LB_THREADED
                printf("Thread %d ",pthread_self());
            #endif
            printf("Initializng connections AND THE CONNECTION POOL\n");
        #endif


        connectionsHandle.connPool = (edg_wll_ConnPool *) calloc(connectionsHandle.poolSize, sizeof(edg_wll_ConnPool));
        for (i=0; i<connectionsHandle.poolSize; i++) {
                connectionsHandle.connPool[i].gss.sock = -1;
        }

        #ifdef GLITE_LB_THREADED
            connectionsHandle.connectionLock = (pthread_mutex_t *) calloc(connectionsHandle.poolSize, sizeof(pthread_mutex_t));
        #endif

        connectionsHandle.locked_by = (edg_wll_Context) calloc(connectionsHandle.poolSize, sizeof(edg_wll_Context));

    }
    if(connectionsHandle.serverConnection == NULL) {
        connectionsHandle.serverConnection = (edg_wll_ConnPool *) calloc(1, sizeof(edg_wll_ConnPool));
    }


  return (&connectionsHandle);
}


