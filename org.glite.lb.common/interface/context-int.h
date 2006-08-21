#ifndef __EDG_WORKLOAD_LOGGING_COMMON_CONTEXT_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_CONTEXT_H__

#ident "$Header$"

#include "glite/security/glite_gss.h"
#include "glite/lb/consumer.h"
#include "lb_plain_io.h"
#include "authz.h"
#include "connpool.h"

#ifdef __cplusplus
extern "C" {
#endif
	
typedef struct _edg_wll_SeqCode {
	unsigned int	c[EDG_WLL_SOURCE__LAST];
} edg_wll_SeqCode;

#define EDG_WLL_SEQ_NULL "UI=000000:NS=0000000000:WM=000000:BH=0000000000:JSS=000000:LM=000000:LRMS=000000:APP=000000"


/* non-gsi one-element analogy of connPool for L&B Proxy server */
typedef struct _edg_wll_ConnProxy {
	edg_wll_PlainConnection	conn;
	char   *buf;
	size_t  bufSize;
	size_t  bufUse;
} edg_wll_ConnProxy;



/* typedef struct _edg_wll_ConnPool { */
/* address and port where we are connected to */
/* 	char		*peerName; */
/* 	unsigned int	peerPort; */
 	
 /* http(s) stream */
/*	gss_cred_id_t	gsiCred; */
/*	edg_wll_GssConnection	gss; */
/*	char		*buf; */
/*	int		bufUse,bufSize; */

/* timestamp of usage of this entry in ctx.connPool */
/*	struct timeval	lastUsed; */
/* } edg_wll_ConnPool; */



struct _edg_wll_Context {
/* Error handling */
	int		errCode;	/* recent error code */
	char 		*errDesc;	/* additional error description */

/* server part */

	void		*mysql;
	edg_wll_Connections	*connections;
	edg_wll_ConnPool	*connPoolNotif;		/* hold _one_ connection from notif-interlogger */
	edg_wll_ConnProxy	*connProxy;		/* holds one plain connection */

	int		semaphores,semset;
	edg_wll_QueryRec	**job_index;
	void		*job_index_cols;
	
	time_t		peerProxyValidity;
	char		*peerName;
	edg_wll_VomsGroups	vomsGroups;
	int		allowAnonymous;
	int		noAuth;		/* if set, you can obtain info about events 	*/
					/* and jobs not belonging to you 		*/
	int		noIndex;	/* don't enforce indices */
	int		rgma_export;
	int		strict_locking;	/* lock jobs for storing event too */

	int             is_V21;         /* true if old (V21) request arrived */
	int		isProxy;	/* LBProxy */

/* server limits */
	int		softLimit;
	int		hardJobsLimit;
	int		hardEventsLimit;

	time_t		notifDuration;

/* purge and dump files storage */
	char		*dumpStorage;
	char		*purgeStorage;

/* maildir location */
	char	*jpreg_dir;

/* flag for function store_event
 * if set then event are loaded from dump file
 */
	int		event_load;

/* address and port we are listening at */
	char		*srvName;
	unsigned int	srvPort;
	
/* pool of connections from client */
//	int		poolSize;
//	int		connOpened;	/* number of opened connections  */
//	int		connToUse;	/* index of connection that will *
//					 *  be used by low-level f-cions */
	// XXX similar variables will be needed for connPoolNotif

	
/* other client stuff */
	int		notifSock;		/* default client socket	*
					   	 * for receiving notifications  */   
	
/* user settable parameters */
	char		*p_host;
	edg_wll_Source	p_source;
	char		*p_instance;
	enum edg_wll_Level	p_level;
	char		*p_destination;
	int		p_dest_port;
	char		*p_lbproxy_store_sock;
	char		*p_lbproxy_serve_sock;
	char		*p_user_lbproxy;
	struct timeval	p_log_timeout,p_sync_timeout,p_query_timeout, p_notif_timeout, p_tmp_timeout;
	char		*p_query_server;
	int		p_query_server_port;
	int		p_query_server_override;
	int		p_query_events_limit;
	int		p_query_jobs_limit;
	edg_wll_QueryResults	p_query_results;
	char		*p_notif_server;
	int		p_notif_server_port;
	char		*p_proxy_filename;
	char		*p_cert_filename;
	char		*p_key_filename;
	time_t		purge_timeout[EDG_WLL_NUMBER_OF_STATCODES];
/* producer part */
	edg_wlc_JobId	p_jobid;
	edg_wll_SeqCode	p_seqcode;
	int		count_statistics;
};

/* to be used internally: set, update and and clear the error information in 
 * context, the desc string (if provided) is strdup()-ed
 *
 * all return the error code for convenience (usage in return statement) */


extern int edg_wll_SetError(
	edg_wll_Context,	/* context */
	int,			/* error code */
	const char *		/* error description */
);

extern int edg_wll_SetErrorGss(
	edg_wll_Context,
	const char *,
	edg_wll_GssStatus *
);

/** update errDesc and errCode */
extern int edg_wll_UpdateError(
	edg_wll_Context,	/* context */
	int,			/* error code */
	const char *		/* error description */
);

/** set errCode to 0, free errDesc */
extern int edg_wll_ResetError(edg_wll_Context);

/** retrieve standard error text wrt. code
 * !! does not allocate memory */
extern const char *edg_wll_GetErrorText(int);

extern int edg_wll_ContextReopen(edg_wll_Context);

extern int edg_wll_SetSequenceCode(edg_wll_Context, const char *, int);
extern int edg_wll_IncSequenceCode(edg_wll_Context ctx);

extern void edg_wll_FreeParams(edg_wll_Context context);


#ifdef __cplusplus
}
#endif
#endif /* __EDG_WORKLOAD_LOGGING_COMMON_CONTEXT_H__ */
