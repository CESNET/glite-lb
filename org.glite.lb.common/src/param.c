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

#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>
#include <stdio.h>

#include "glite/jobid/cjobid.h"
#include "context-int.h"
#include "connpool.h"
// XXX:
#include "log_proto.h" // for default log host and port
#include "timeouts.h" // for timeouts

/* XXX: must match edg_wll_ContextParam */
static const char *myenv[] = {
	"GLOBUS_HOSTNAME",
	NULL,
	NULL,
	NULL,
	"%sLOG_DESTINATION",
	"%sLOG_DESTINATION",
	"%sLOG_TIMEOUT",
	"%sLOG_SYNC_TIMEOUT",
	"%sQUERY_SERVER",
	"%sQUERY_SERVER",
	"%sQUERY_SERVER_OVERRIDE",
	"%sQUERY_TIMEOUT",
	"%sQUERY_JOBS_LIMIT",
	"%sQUERY_EVENTS_LIMIT",
	"%sQUERY_RESULTS",
	"%sCONNPOOL_SIZE",
	"%sNOTIF_SERVER",
	"%sNOTIF_SERVER",
	"%sNOTIF_TIMEOUT",
/* don't care about X509_USER_*, GSI looks at them anyway */
	NULL,
	NULL,
	NULL,
	"%sLBPROXY_STORE_SOCK",
	"%sLBPROXY_SERVE_SOCK",
	"%sLBPROXY_USER",
	"%sJPREG_TMPDIR",
	"%sLOG_FILE_PREFIX",
	"%sLOG_IL_SOCK",
	"%sLBPROXY_SERVERNAME",
};

/* XXX: does not parse URL, just hostname[:port] */

static char *mygetenv(int param)
{
	char	*s = NULL;

	if (myenv[param]) {
		char	varname[100];

		sprintf(varname,myenv[param],"GLITE_WMS_");
		s = getenv(varname);

		if (!s) {
			sprintf(varname,myenv[param],"EDG_WL_");
			s = getenv(varname);
		}
	}
	return s;
}

static int extract_port(edg_wll_ContextParam param,int dflt)
{
	char	*p = NULL,*s = mygetenv(param);

        if (s) p = strrchr(s,':');
	return	p ? atoi(p+1) : dflt;
}

static int extract_num(edg_wll_ContextParam param,int dflt)
{
	char *s = mygetenv(param);
	return s ? atoi(s) : dflt;
}

static char *extract_host(edg_wll_ContextParam param,const char *dflt)
{
	char	*p,*s = NULL;

	s = mygetenv(param);
	if (!s && !dflt) return NULL;
	s = strdup(s?s:dflt),
	p = strrchr(s,':');
	if (p) *p = 0;
	return s;
}

static void extract_time(edg_wll_ContextParam param,double dflt,struct timeval *t)
{
	char	*s = NULL;
	double	d;

	s = mygetenv(param);
	d = s ? atof(s) : dflt;
	t->tv_sec = (long) d;
	t->tv_usec = (long) ((d-t->tv_sec)*1e6);
}

static char *my_strndup(const char *s,size_t len)
{
	size_t	l = strlen(s);
	char	*r = malloc(l < len ? l+1 : len+1);

	strncpy(r,s,len);
	if (l >= len) r[len] = 0;
	return r;
}

static char *extract_split(edg_wll_ContextParam param,char by,int index)
{
	int	i;
	char	*s,*e;

	if (!(s = mygetenv(param))) return NULL;
	for (i=0; i<index && (s=strchr(s,by));i++) s++;
	return i==index ? ( (e = strchr(s,by)) ? my_strndup(s,e-s) : strdup(s))
			: NULL;
}


int edg_wll_SetParamString(edg_wll_Context ctx,edg_wll_ContextParam param,const char *val)
{
	char	hn[200];

	switch (param) {
		case EDG_WLL_PARAM_HOST:             
			edg_wll_gss_gethostname(hn,sizeof hn);
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
			if (!val) val = mygetenv(param);
			if (!val) val = "no";
			ctx->p_query_server_override = !strcasecmp(val,"yes");
			break;
		case EDG_WLL_PARAM_LBPROXY_STORE_SOCK:      
			if (!val) val = mygetenv(param);
			if (!val) val = "/tmp/lb_proxy_store.sock";
			free(ctx->p_lbproxy_store_sock);
			ctx->p_lbproxy_store_sock = val ? strdup(val): NULL;
			break;
		case EDG_WLL_PARAM_LBPROXY_SERVE_SOCK:      
			if (!val) val = mygetenv(param);
			if (!val) val = "/tmp/lb_proxy_serve.sock";
			free(ctx->p_lbproxy_serve_sock);
			ctx->p_lbproxy_serve_sock = val ? strdup(val): NULL;
			break;
		case EDG_WLL_PARAM_LBPROXY_USER:
			if (!val) val = mygetenv(param);
			free(ctx->p_user_lbproxy);
			ctx->p_user_lbproxy = val ? strdup(val) : NULL;
			break;
		case EDG_WLL_PARAM_JPREG_TMPDIR:
			if (!val) val = mygetenv(param);
			free(ctx->jpreg_dir);
			ctx->jpreg_dir = val ? strdup(val) : NULL;
			break;
	        case EDG_WLL_PARAM_LOG_FILE_PREFIX:
			if(!val) val = mygetenv(param);
			free(ctx->p_event_file_prefix);
			ctx->p_event_file_prefix = val ? strdup(val) : NULL;
			break;
        	case EDG_WLL_PARAM_LOG_IL_SOCK:
			if(!val) val = mygetenv(param);
			free(ctx->p_il_sock);
			ctx->p_il_sock = val ? strdup(val) : NULL;
			break;
		case EDG_WLL_PARAM_LBPROXY_SERVERNAME:
			if (!val) val = mygetenv(param);
			free(ctx->p_lbproxy_servername);
			ctx->p_lbproxy_servername = val ? extract_host(param, NULL) : NULL;
			ctx->p_lbproxy_servername_port = val ? extract_port(param, GLITE_JOBID_DEFAULT_PORT) : 0;
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
				extract_port(param,GLITE_JOBID_DEFAULT_PORT);;
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
				if (mygetenv(param)) {
					char	*s = extract_split(param,'/',0);
					if (s) {
						val = edg_wll_StringToQResult(s);
						if (!val) return edg_wll_SetError(ctx,EINVAL,"can't parse query result parameter name");
						ctx->p_query_results = val;
						free(s);
					} else
						return edg_wll_SetError(ctx,EINVAL,"can't parse query result parameter name");
				} // else default EDG_WLL_QUERYRES_UNDEF
			}
			break;
		case EDG_WLL_PARAM_CONNPOOL_SIZE:
			{
				char *s = mygetenv(param);
				
				if (!val && s) val = atoi(s);

				edg_wll_poolLock();
				connectionsHandle.poolSize = val ? val : GLITE_LB_COMMON_CONNPOOL_SIZE;
				edg_wll_poolUnlock();
			}
			break;
		case EDG_WLL_PARAM_SOURCE:           
			if (val) {
				if (val <= EDG_WLL_SOURCE_NONE || val >= EDG_WLL_SOURCE__LAST)
					return edg_wll_SetError(ctx,EINVAL,"Source out of range");

				ctx->p_source = val;
			}
			else {
				if (mygetenv(param)) {
					char	*s = extract_split(param,'/',0);
					if (s) {
						val = edg_wll_StringToSource(s);
						if (!val) return edg_wll_SetError(ctx,EINVAL,"can't parse source name");
						ctx->p_source = val;
						free(s);
					} else
						return edg_wll_SetError(ctx,EINVAL,"can't parse source name");
				} // else default EDG_WLL_SOURCE_NONE
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
/* FIXME - default timeouts - now in timeouts.h: */
			else extract_time(param,EDG_WLL_LOG_TIMEOUT_DEFAULT,&ctx->p_log_timeout);
			break;
		case EDG_WLL_PARAM_LOG_SYNC_TIMEOUT: 
/* XXX: check also if val is not grater than EDG_WLL_LOG_SYNC_TIMEOUT_MAX */
			if (val) memcpy(&ctx->p_sync_timeout,val,sizeof *val);
/* FIXME - default timeouts - now in timeouts.h: */
			else extract_time(param,EDG_WLL_LOG_SYNC_TIMEOUT_DEFAULT,&ctx->p_sync_timeout);
			break;
		case EDG_WLL_PARAM_QUERY_TIMEOUT:    
/* XXX: check also if val is not grater than EDG_WLL_QUERY_TIMEOUT_MAX */
			if (val) memcpy(&ctx->p_query_timeout,val,sizeof *val);
/* FIXME - default timeouts - now in timeouts.h: */
			else extract_time(param,EDG_WLL_QUERY_TIMEOUT_DEFAULT,&ctx->p_query_timeout);
			break;
		case EDG_WLL_PARAM_NOTIF_TIMEOUT:    
/* XXX: check also if val is not grater than EDG_WLL_NOTIF_TIMEOUT_MAX */
			if (val) memcpy(&ctx->p_notif_timeout,val,sizeof *val);
/* FIXME - default timeouts - now in timeouts.h: */
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
	int ret;

	va_start(ap,param);
	switch (param) {
		case EDG_WLL_PARAM_LEVEL:            
		case EDG_WLL_PARAM_DESTINATION_PORT: 
		case EDG_WLL_PARAM_QUERY_SERVER_PORT:
		case EDG_WLL_PARAM_NOTIF_SERVER_PORT:
		case EDG_WLL_PARAM_QUERY_JOBS_LIMIT:      
		case EDG_WLL_PARAM_QUERY_EVENTS_LIMIT:      
		case EDG_WLL_PARAM_QUERY_RESULTS:
		case EDG_WLL_PARAM_CONNPOOL_SIZE:
		case EDG_WLL_PARAM_SOURCE:           
			ret = edg_wll_SetParamInt(ctx,param,va_arg(ap,int));
			break;
		case EDG_WLL_PARAM_HOST:             
		case EDG_WLL_PARAM_INSTANCE:         
		case EDG_WLL_PARAM_DESTINATION:      
		case EDG_WLL_PARAM_QUERY_SERVER:     
		case EDG_WLL_PARAM_NOTIF_SERVER:     
		case EDG_WLL_PARAM_QUERY_SERVER_OVERRIDE:
		case EDG_WLL_PARAM_X509_PROXY:       
		case EDG_WLL_PARAM_X509_KEY:         
		case EDG_WLL_PARAM_X509_CERT:        
		case EDG_WLL_PARAM_LBPROXY_STORE_SOCK:
		case EDG_WLL_PARAM_LBPROXY_SERVE_SOCK:
		case EDG_WLL_PARAM_LBPROXY_USER:
		case EDG_WLL_PARAM_JPREG_TMPDIR:
	        case EDG_WLL_PARAM_LOG_FILE_PREFIX:
	        case EDG_WLL_PARAM_LOG_IL_SOCK:
		case EDG_WLL_PARAM_LBPROXY_SERVERNAME:
			ret = edg_wll_SetParamString(ctx,param,va_arg(ap,char *));
			break;
		case EDG_WLL_PARAM_LOG_TIMEOUT:      
		case EDG_WLL_PARAM_LOG_SYNC_TIMEOUT: 
		case EDG_WLL_PARAM_QUERY_TIMEOUT:    
		case EDG_WLL_PARAM_NOTIF_TIMEOUT:    
			ret = edg_wll_SetParamTime(ctx,param,va_arg(ap,struct timeval *));
			break;
		default: 
			ret = edg_wll_SetError(ctx,EINVAL,"unknown parameter");
			break;
	}
	va_end(ap);

	return ret;
}

int edg_wll_SetParamToDefault(edg_wll_Context ctx,edg_wll_ContextParam param)
{
	switch (param) {
		case EDG_WLL_PARAM_LEVEL:
		case EDG_WLL_PARAM_DESTINATION_PORT:
		case EDG_WLL_PARAM_QUERY_SERVER_PORT:
		case EDG_WLL_PARAM_NOTIF_SERVER_PORT:
		case EDG_WLL_PARAM_QUERY_JOBS_LIMIT:
		case EDG_WLL_PARAM_QUERY_EVENTS_LIMIT:
		case EDG_WLL_PARAM_QUERY_RESULTS:
		case EDG_WLL_PARAM_CONNPOOL_SIZE:
		case EDG_WLL_PARAM_SOURCE:
			return edg_wll_SetParamInt(ctx,param,0);
		case EDG_WLL_PARAM_HOST:
		case EDG_WLL_PARAM_INSTANCE:
		case EDG_WLL_PARAM_DESTINATION:
		case EDG_WLL_PARAM_QUERY_SERVER:
		case EDG_WLL_PARAM_NOTIF_SERVER:
		case EDG_WLL_PARAM_QUERY_SERVER_OVERRIDE:
		case EDG_WLL_PARAM_X509_PROXY:
		case EDG_WLL_PARAM_X509_KEY:
		case EDG_WLL_PARAM_X509_CERT:
		case EDG_WLL_PARAM_LBPROXY_STORE_SOCK:
		case EDG_WLL_PARAM_LBPROXY_SERVE_SOCK:
		case EDG_WLL_PARAM_LBPROXY_USER:
		case EDG_WLL_PARAM_JPREG_TMPDIR:
	        case EDG_WLL_PARAM_LOG_FILE_PREFIX:
	        case EDG_WLL_PARAM_LOG_IL_SOCK:
		case EDG_WLL_PARAM_LBPROXY_SERVERNAME:
			return edg_wll_SetParamString(ctx,param,NULL);
		case EDG_WLL_PARAM_LOG_TIMEOUT:
		case EDG_WLL_PARAM_LOG_SYNC_TIMEOUT:
		case EDG_WLL_PARAM_QUERY_TIMEOUT:
		case EDG_WLL_PARAM_NOTIF_TIMEOUT:
			return edg_wll_SetParamTime(ctx,param,NULL);
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
		case EDG_WLL_PARAM_CONNPOOL_SIZE:
			p_int = va_arg(ap, int *);
			*p_int = connectionsHandle.poolSize;
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
		case EDG_WLL_PARAM_LBPROXY_USER:
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_user_lbproxy);
			break;
		case EDG_WLL_PARAM_JPREG_TMPDIR:
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->jpreg_dir);
			break;
	        case EDG_WLL_PARAM_LOG_FILE_PREFIX:
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_event_file_prefix);
			break;
	        case EDG_WLL_PARAM_LOG_IL_SOCK:
			p_string = va_arg(ap, char **);
			*p_string = estrdup(ctx->p_il_sock);
			break;
		case EDG_WLL_PARAM_LBPROXY_SERVERNAME:
			p_string = va_arg(ap, char **);
			if (ctx->p_lbproxy_servername) 
				asprintf(p_string,"%s:%d", ctx->p_lbproxy_servername, ctx->p_lbproxy_servername_port);
			else *p_string = NULL;
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
			va_end(ap);
			return edg_wll_SetError(ctx, EINVAL, "unknown parameter");
			break;
	}
	va_end(ap);
	return edg_wll_Error(ctx, NULL, NULL);
}

void edg_wll_FreeParams(edg_wll_Context ctx) {
	if (ctx->p_jobid) edg_wlc_JobIdFree(ctx->p_jobid);
	if (ctx->p_host) free(ctx->p_host);
	if (ctx->p_instance) free(ctx->p_instance);
	if (ctx->p_destination) free(ctx->p_destination);
	if (ctx->p_user_lbproxy) free(ctx->p_user_lbproxy);
	if (ctx->p_query_server) free(ctx->p_query_server);
	if (ctx->p_notif_server) free(ctx->p_notif_server);
	if (ctx->p_proxy_filename) free(ctx->p_proxy_filename);
	if (ctx->p_cert_filename) free(ctx->p_cert_filename);
	if (ctx->p_key_filename) free(ctx->p_key_filename);
	if (ctx->p_lbproxy_store_sock) free(ctx->p_lbproxy_store_sock);
	if (ctx->p_lbproxy_serve_sock) free(ctx->p_lbproxy_serve_sock);
	if (ctx->p_event_file_prefix) free(ctx->p_event_file_prefix);
	if (ctx->p_il_sock) free(ctx->p_il_sock);

	ctx->p_jobid = NULL;
	ctx->p_host = NULL;
	ctx->p_instance = NULL;
	ctx->p_destination = NULL;
	ctx->p_user_lbproxy = NULL;
	ctx->p_query_server = NULL;
	ctx->p_notif_server = NULL;
	ctx->p_proxy_filename = NULL;
	ctx->p_cert_filename = NULL;
	ctx->p_key_filename = NULL;
	ctx->p_lbproxy_store_sock = NULL;
	ctx->p_lbproxy_serve_sock = NULL;
	ctx->p_event_file_prefix = NULL;
	ctx->p_il_sock = NULL;

	/* do not free (references only)
	 * ctx->job_index
	 * ctx->job_index_cols
	 * ctx->mysql */

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
