#ifndef __GLITE_LB_CONTEXT_INT_H__
#define __GLITE_LB_CONTEXT_INT_H__

#ident "$Header$"

#include "glite/security/glite_gss.h"
#include "glite/lb/jobstat.h"
#include "glite/lb/query_rec.h"
#include "glite/lb/lb_plain_io.h"
#include "glite/lb/authz.h"
#include "glite/lb/connpool.h"

#ifdef __cplusplus
extern "C" {
#endif
	
#define EDG_WLL_SEQ_NULL "UI=000000:NS=0000000000:WM=000000:BH=0000000000:JSS=000000:LM=000000:LRMS=000000:APP=000000:LBS=000000"
#define EDG_WLL_SEQ_PBS_NULL "TIMESTAMP=00000000000000:POS=0000000000:EV.CODE=000:SRC=?" 
#define EDG_WLL_SEQ_CONDOR_NULL EDG_WLL_SEQ_PBS_NULL
#define EDG_WLL_SEQ_SIZE        103	/* strlen(EDG_WLL_SEQ_NULL)+1 */
#define EDG_WLL_SEQ_PBS_SIZE	57	/* strlen(EDG_WLL_SEQ_PBS_NULL)+1 */
#define EDG_WLL_SEQ_CONDOR_SIZE EDG_WLL_SEQ_PBS_SIZE

typedef struct _edg_wll_SeqCode {
	unsigned int	type;				/* seq code type    */
	unsigned int	c[EDG_WLL_SOURCE__LAST];	/* glite seq. code  */
	char		pbs[EDG_WLL_SEQ_PBS_SIZE];	/* PBS seq. code    */
				/* 0-24 TIMESTAMP=YYYYMMDDHHMMSS: */
				/* 25-39 POS=%010u: */
				/* 40-51 EV.CODE=%03d: */
				/* 53-56 SRC=%c */
	char		condor[EDG_WLL_SEQ_CONDOR_SIZE];			
} edg_wll_SeqCode;

/* non-gsi one-element analogy of connPool for L&B Proxy server */
typedef struct _edg_wll_ConnProxy {
	edg_wll_PlainConnection	conn;
	char   *buf;
	size_t  bufSize;
	size_t  bufUse;
} edg_wll_ConnProxy;



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
	char		*serverIdentity;		/* DN of server certificate or "anonymous" */
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

	/* TODO: belongs to database part */
	int use_transactions;

	int		greyjobs;
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

#endif /* __GLITE_LB_CONTEXT_INT_H__ */
