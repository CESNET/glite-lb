#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#include <globus_common.h>

#include "glite/wmsutils/jobid/cjobid.h"
#include "glite/lb/producer.h"
#include "glite/lb/notification.h"
#include "context-int.h"
#include "log_proto.h"


/* XXX: must match edg_wll_ContextParam */
static const char *myenv[] = {
	"GLOBUS_HOSTNAME",
	NULL,
	NULL,
	NULL,
	"EDG_WL_LOG_DESTINATION",
	"EDG_WL_LOG_DESTINATION",
	"EDG_WL_LOG_TIMEOUT",
	"EDG_WL_LOG_SYNC_TIMEOUT",
	"EDG_WL_QUERY_SERVER",
	"EDG_WL_QUERY_SERVER",
	"EDG_WL_QUERY_SERVER_OVERRIDE",
	"EDG_WL_QUERY_TIMEOUT",
	"EDG_WL_QUERY_JOBS_LIMIT",
	"EDG_WL_QUERY_EVENTS_LIMIT",
	"EDG_WL_QUERY_RESULTS",
	"EDG_WL_QUERY_CONNECTIONS",
	"EDG_WL_NOTIF_SERVER",
	"EDG_WL_NOTIF_SERVER",
	"EDG_WL_NOTIF_TIMEOUT",
/* don't care about X509_USER_*, GSI looks at them anyway */
	NULL,
	NULL,
	NULL,
};

/* XXX: does not parse URL, just hostname[:port] */

static int extract_port(edg_wll_ContextParam param,int dflt)
{
	char	*p = NULL,*s = NULL;
	if (myenv[param]) {
		s = getenv(myenv[param]);
        	if (s) p = strchr(s,':');
	}
	return	p ? atoi(p+1) : dflt;
}

static int extract_num(edg_wll_ContextParam param,int dflt)
{
	if (myenv[param]) {
		char *s = getenv(myenv[param]);
        	if (s) return(atoi(s));
	}
	return dflt;
}

static char *extract_host(edg_wll_ContextParam param,const char *dflt)
{
	char	*p,*s = NULL;

	if (myenv[param]) s = getenv(myenv[param]);
	if (!s && !dflt) return NULL;
	s = strdup(s?s:dflt),
	p = strchr(s,':');
	if (p) *p = 0;
	return s;
}

static void extract_time(edg_wll_ContextParam param,double dflt,struct timeval *t)
{
	char	*s = NULL;
	double	d;

	if (myenv[param]) s = getenv(myenv[param]);
	d = s ? atof(s) : dflt;
	t->tv_sec = (long) d;
	t->tv_usec = (long) ((d-t->tv_sec)*1e6);
}

static char *extract_split(edg_wll_ContextParam param,char by,int index)
{
	int	i;
	char	*s,*e;

	if (!myenv[param]) return NULL;
	if (!(s = getenv(myenv[param]))) return NULL;
	for (i=0; i<index && (s=strchr(s,by));i++) s++;
	return i==index ? ( (e = strchr(s,by)) ? strndup(s,e-s) : strdup(s))
			: NULL;
}


int edg_wll_SetParamString(edg_wll_Context ctx,edg_wll_ContextParam param,const char *val)
{
	char	hn[200];

	switch (param) {
		case EDG_WLL_PARAM_HOST:             
			globus_libc_gethostname(hn,sizeof hn);
			free(ctx->p_host);
			ctx->p_host = val ? strdup(val) : extract_host(param,hn);
			break;
		case EDG_WLL_PARAM_INSTANCE:         
			free(ctx->p_instance);
			ctx->p_instance = val ? strdup(val) : extract_split(param,'/',1);
			break;
		case EDG_WLL_PARAM_DESTINATION:      
			free(ctx->p_destination);
			ctx->p_destination = val ? strdup(val) : extract_host(param,EDG_WLL_LOG_HOST_DEFAULT);
			break;
		case EDG_WLL_PARAM_USER_LBPROXY:
			free(ctx->p_user_lbproxy);
			ctx->p_user_lbproxy = val ? strdup(val) : NULL;
			break;
		case EDG_WLL_PARAM_QUERY_SERVER:     
			free(ctx->p_query_server);
			ctx->p_query_server = val ? strdup(val) : extract_host(param,NULL);
			break;
		case EDG_WLL_PARAM_NOTIF_SERVER:     
			free(ctx->p_notif_server);
			ctx->p_notif_server = val ? strdup(val) : extract_host(param,NULL);
			break;
		case EDG_WLL_PARAM_X509_PROXY:       
			free(ctx->p_proxy_filename);
			ctx->p_proxy_filename = val ? strdup(val) : NULL;
			break;
		case EDG_WLL_PARAM_X509_KEY:         
			free(ctx->p_key_filename);
			ctx->p_key_filename = val ? strdup(val) : NULL;
			break;
		case EDG_WLL_PARAM_X509_CERT:        
			free(ctx->p_cert_filename);
			ctx->p_cert_filename = val ? strdup(val) : NULL;
			break;
		case EDG_WLL_PARAM_QUERY_SERVER_OVERRIDE:
			if (!val) val = getenv(myenv[param]);
			if (!val) val = "no";
			ctx->p_query_server_override = !strcasecmp(val,"yes");
			break;
		case EDG_WLL_PARAM_LBPROXY_STORE_SOCK:      
			free(ctx->p_lbproxy_store_sock);
			ctx->p_lbproxy_store_sock = val ? strdup(val): NULL;
			break;
		case EDG_WLL_PARAM_LBPROXY_SERVE_SOCK:      
			free(ctx->p_lbproxy_serve_sock);
			ctx->p_lbproxy_serve_sock = val ? strdup(val): NULL;
			break;
		default:
			return edg_wll_SetError(ctx,EINVAL,"unknown parameter");
	}
	return edg_wll_ResetError(ctx);
}

int edg_wll_SetParamInt(edg_wll_Context ctx,edg_wll_ContextParam param,int val)
{
	switch (param) {
		case EDG_WLL_PARAM_LEVEL:
			ctx->p_level = val ? val : EDG_WLL_LEVEL_SYSTEM;
			break;
		case EDG_WLL_PARAM_DESTINATION_PORT:
			ctx->p_dest_port = val ? val : extract_port(param,EDG_WLL_LOG_PORT_DEFAULT);
			break;
		case EDG_WLL_PARAM_QUERY_SERVER_PORT:
			ctx->p_query_server_port = val ? val :
				extract_port(param,GLITE_WMSC_JOBID_DEFAULT_PORT);;
			break;
		case EDG_WLL_PARAM_NOTIF_SERVER_PORT:
			ctx->p_notif_server_port = val ? val :
				extract_port(param,0);;
			// XXX: when default port is known, put it here
			break;
		case EDG_WLL_PARAM_QUERY_JOBS_LIMIT:
			ctx->p_query_jobs_limit = val ? val :
				extract_num(param,0);
			break;
		case EDG_WLL_PARAM_QUERY_EVENTS_LIMIT:
			ctx->p_query_events_limit = val ? val :
				extract_num(param,0);
			break;
		case EDG_WLL_PARAM_QUERY_RESULTS:
			if (val) {
				if (val <= EDG_WLL_QUERYRES_UNDEF || val >= EDG_WLL_QUERYRES__LAST)
					return edg_wll_SetError(ctx,EINVAL,"Query result parameter value out of range");

				ctx->p_query_results = val;
			}
			else {
				char	*s = extract_split(param,'/',0);
				if (s) {
					val = edg_wll_StringToQResult(s);
					if (!val) return edg_wll_SetError(ctx,EINVAL,"can't parse query result parameter name");
					ctx->p_query_results = val;
					free(s);
				}
				return edg_wll_SetError(ctx,EINVAL,"can't parse query result parameter name");
			}
			break;
		case EDG_WLL_PARAM_QUERY_CONNECTIONS:
			{
				char *s = getenv(myenv[param]);
				
				if (!val && s) val = atoi(s);
				ctx->poolSize = val ? val : EDG_WLL_LOG_CONNECTIONS_DEFAULT;
			}
			break;
		case EDG_WLL_PARAM_SOURCE:           
			if (val) {
				if (val <= EDG_WLL_SOURCE_NONE || val >= EDG_WLL_SOURCE__LAST)
					return edg_wll_SetError(ctx,EINVAL,"Source out of range");

				ctx->p_source = val;
			}
			else {
				char	*s = extract_split(param,'/',0);
				if (s) {
					val = edg_wll_StringToSource(s);
					if (!val) return edg_wll_SetError(ctx,EINVAL,"can't parse source name");
					ctx->p_source = val;
					free(s);
				}
				return edg_wll_SetError(ctx,EINVAL,"can't parse source name");
			}
			break;
		default:
			return edg_wll_SetError(ctx,EINVAL,"unknown parameter");
	}
	return edg_wll_ResetError(ctx);
}

int edg_wll_SetParamTime(edg_wll_Context ctx,edg_wll_ContextParam param,const struct timeval *val)
{
	switch (param) {
		case EDG_WLL_PARAM_LOG_TIMEOUT:      
/* XXX: check also if val is not grater than EDG_WLL_LOG_TIMEOUT_MAX */
			if (val) memcpy(&ctx->p_log_timeout,val,sizeof *val);
			else extract_time(param,EDG_WLL_LOG_TIMEOUT_DEFAULT,&ctx->p_log_timeout);
			break;
		case EDG_WLL_PARAM_LOG_SYNC_TIMEOUT: 
/* XXX: check also if val is not grater than EDG_WLL_LOG_SYNC_TIMEOUT_MAX */
			if (val) memcpy(&ctx->p_sync_timeout,val,sizeof *val);
			else extract_time(param,EDG_WLL_LOG_SYNC_TIMEOUT_DEFAULT,&ctx->p_sync_timeout);
			break;
		case EDG_WLL_PARAM_QUERY_TIMEOUT:    
/* XXX: check also if val is not grater than EDG_WLL_QUERY_TIMEOUT_MAX */
			if (val) memcpy(&ctx->p_query_timeout,val,sizeof *val);
			else extract_time(param,EDG_WLL_QUERY_TIMEOUT_DEFAULT,&ctx->p_query_timeout);
			break;
		case EDG_WLL_PARAM_NOTIF_TIMEOUT:    
/* XXX: check also if val is not grater than EDG_WLL_NOTIF_TIMEOUT_MAX */
			if (val) memcpy(&ctx->p_notif_timeout,val,sizeof *val);
			else extract_time(param,EDG_WLL_NOTIF_TIMEOUT_DEFAULT,&ctx->p_notif_timeout);
			break;
		default:
			return edg_wll_SetError(ctx,EINVAL,"unknown parameter");
	}
	return edg_wll_ResetError(ctx);
}

int edg_wll_SetParam(edg_wll_Context ctx,edg_wll_ContextParam param,...)
{
	va_list	ap;

	va_start(ap,param);
	switch (param) {
		case EDG_WLL_PARAM_LEVEL:            
		case EDG_WLL_PARAM_DESTINATION_PORT: 
		case EDG_WLL_PARAM_QUERY_SERVER_PORT:
		case EDG_WLL_PARAM_NOTIF_SERVER_PORT:
		case EDG_WLL_PARAM_QUERY_JOBS_LIMIT:      
		case EDG_WLL_PARAM_QUERY_EVENTS_LIMIT:      
		case EDG_WLL_PARAM_QUERY_RESULTS:
		case EDG_WLL_PARAM_QUERY_CONNECTIONS:
		case EDG_WLL_PARAM_SOURCE:           
			return edg_wll_SetParamInt(ctx,param,va_arg(ap,int));
		case EDG_WLL_PARAM_HOST:             
		case EDG_WLL_PARAM_INSTANCE:         
		case EDG_WLL_PARAM_DESTINATION:      
		case EDG_WLL_PARAM_USER_LBPROXY:
		case EDG_WLL_PARAM_QUERY_SERVER:     
		case EDG_WLL_PARAM_NOTIF_SERVER:     
		case EDG_WLL_PARAM_QUERY_SERVER_OVERRIDE:
		case EDG_WLL_PARAM_X509_PROXY:       
		case EDG_WLL_PARAM_X509_KEY:         
		case EDG_WLL_PARAM_X509_CERT:        
		case EDG_WLL_PARAM_LBPROXY_STORE_SOCK:
		case EDG_WLL_PARAM_LBPROXY_SERVE_SOCK:
			return edg_wll_SetParamString(ctx,param,va_arg(ap,char *));
		case EDG_WLL_PARAM_LOG_TIMEOUT:      
		case EDG_WLL_PARAM_LOG_SYNC_TIMEOUT: 
		case EDG_WLL_PARAM_QUERY_TIMEOUT:    
		case EDG_WLL_PARAM_NOTIF_TIMEOUT:    
			return edg_wll_SetParamTime(ctx,param,va_arg(ap,struct timeval *));
		default: 
			return edg_wll_SetError(ctx,EINVAL,"unknown parameter");
	}
}

int edg_wll_GetParam(edg_wll_Context ctx,edg_wll_ContextParam param,...)
{
	va_list	ap;
	int	*p_int;
	char	**p_string;
	struct timeval	*p_tv;

	edg_wll_ResetError(ctx);

	va_start(ap,param);
	switch (param) {
		case EDG_WLL_PARAM_LEVEL:            
			p_int = va_arg(ap, int *);
			*p_int = ctx->p_level;
			break;
		case EDG_WLL_PARAM_DESTINATION_PORT: 
			p_int = va_arg(ap, int *);
			*p_int = ctx->p_dest_port;
			break;
		case EDG_WLL_PARAM_QUERY_SERVER_PORT:
			p_int = va_arg(ap, int *);
			*p_int = ctx->p_query_server_port;
			break;
		case EDG_WLL_PARAM_NOTIF_SERVER_PORT:
			p_int = va_arg(ap, int *);
			*p_int = ctx->p_notif_server_port;
			break;
		case EDG_WLL_PARAM_QUERY_JOBS_LIMIT:      
			p_int = va_arg(ap, int *);
			*p_int = ctx->p_query_jobs_limit;
			break;
		case EDG_WLL_PARAM_QUERY_EVENTS_LIMIT:      
			p_int = va_arg(ap, int *);
			*p_int = ctx->p_query_events_limit;
			break;
		case EDG_WLL_PARAM_QUERY_RESULTS:
			p_int = va_arg(ap, int *);
			*p_int = ctx->p_query_results;
			break;
		case EDG_WLL_PARAM_QUERY_CONNECTIONS:
			p_int = va_arg(ap, int *);
			*p_int = ctx->poolSize;
			break;
		case EDG_WLL_PARAM_SOURCE:           
			p_int = va_arg(ap, int *);
			*p_int = ctx->p_source;
			break;

#define estrdup(x) ((x) ? strdup(x) : x)

		case EDG_WLL_PARAM_HOST:             
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_host);
			break;
		case EDG_WLL_PARAM_INSTANCE:         
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_instance);
			break;
		case EDG_WLL_PARAM_DESTINATION:      
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_destination);
			break;
		case EDG_WLL_PARAM_USER_LBPROXY:
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_user_lbproxy);
			break;
		case EDG_WLL_PARAM_QUERY_SERVER:     
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_query_server);
			break;
		case EDG_WLL_PARAM_NOTIF_SERVER:     
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_notif_server);
			break;
		case EDG_WLL_PARAM_QUERY_SERVER_OVERRIDE:
			p_string = va_arg(ap, char **);
			*p_string = strdup(ctx->p_query_server_override ? "yes" : "no");
			break;
		case EDG_WLL_PARAM_X509_PROXY:       
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_proxy_filename);
			break;
		case EDG_WLL_PARAM_X509_KEY:         
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_key_filename);
			break;
		case EDG_WLL_PARAM_X509_CERT:        
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_cert_filename);
			break;
		case EDG_WLL_PARAM_LBPROXY_STORE_SOCK:      
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_lbproxy_store_sock);
			break;
		case EDG_WLL_PARAM_LBPROXY_SERVE_SOCK:      
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_lbproxy_serve_sock);
			break;

		case EDG_WLL_PARAM_LOG_TIMEOUT:      
			p_tv = va_arg(ap,struct timeval *);
			*p_tv = ctx->p_log_timeout;
			break;
		case EDG_WLL_PARAM_LOG_SYNC_TIMEOUT: 
			p_tv = va_arg(ap,struct timeval *);
			*p_tv = ctx->p_sync_timeout;
			break;
		case EDG_WLL_PARAM_QUERY_TIMEOUT:    
			p_tv = va_arg(ap,struct timeval *);
			*p_tv = ctx->p_query_timeout;
			break;
		case EDG_WLL_PARAM_NOTIF_TIMEOUT:    
			p_tv = va_arg(ap,struct timeval *);
			*p_tv = ctx->p_notif_timeout;
			break;

		default: 
			return edg_wll_SetError(ctx, EINVAL, "unknown parameter");
			break;
	}
	va_end(ap);
	return edg_wll_Error(ctx, NULL, NULL);
}

#if 0
/* only for reference */
edg_wll_ErrorCode edg_wll_SetLoggingParams(edg_wll_Context ctx, 
				const char	*jobid,
				const char      *service,
				const char      *hostname,
				const char      *instance,
				enum edg_wll_Level      level,
				const char      *proxy_filename,
				const char      *cert_filename,
				const char      *key_filename)
{
	edg_wll_ResetError(ctx);

	if (jobid != NULL) ctx->p_jobid = strdup(jobid);
	if (service != NULL) ctx->p_service = strdup(service);
	if (hostname != NULL) ctx->p_hostname = strdup(hostname);
	if (instance != NULL) ctx->p_instance = strdup(instance);

        ctx->p_level = level;

	if (proxy_filename != NULL) ctx->p_proxy_filename = strdup(proxy_filename);
	if (cert_filename != NULL) ctx->p_cert_filename = strdup(cert_filename);
	if (key_filename != NULL) ctx->p_key_filename = strdup(key_filename);

	return edg_wll_Error(ctx, NULL, NULL);
}
#endif
