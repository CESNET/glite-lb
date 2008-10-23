#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <expat.h>

#include "glite/lb/context-int.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/xml_conversions.h"
#include "glite/jobid/strmd5.h"
#include "glite/lbu/trio.h"

#include "lb_proto.h"
#include "lb_text.h"
#include "lb_html.h"
#include "stats.h"
#include "jobstat.h"
#include "get_events.h"
#include "purge.h"
#include "lb_xml_parse.h"
#include "lb_xml_parse_V21.h"
#include "db_supp.h"


#define METHOD_GET      "GET "
#define METHOD_POST     "POST "

#define KEY_QUERY_JOBS		"/queryJobs "
#define KEY_QUERY_EVENTS	"/queryEvents "
#define KEY_PURGE_REQUEST	"/purgeRequest "
#define KEY_DUMP_REQUEST	"/dumpRequest "
#define KEY_LOAD_REQUEST	"/loadRequest "
#define KEY_INDEXED_ATTRS	"/indexedAttrs "
#define KEY_NOTIF_REQUEST	"/notifRequest "
#define KEY_QUERY_SEQUENCE_CODE	"/querySequenceCode "
#define KEY_STATS_REQUEST	"/statsRequest "
#define KEY_HTTP        	"HTTP/1.1"


#define KEY_ACCEPT      "Accept:"
#define KEY_APP         "application/x-dglb"
#define KEY_AGENT	"User-Agent"


static const char* const response_headers_dglb[] = {
        "Cache-Control: no-cache",
        "Server: edg_wll_Server/" PROTO_VERSION "/" COMP_PROTO,
        "Content-Type: application/x-dglb",
        NULL
};

static const char* const response_headers_html[] = {
        "Cache-Control: no-cache",
        "Server: edg_wll_Server/" PROTO_VERSION "/" COMP_PROTO,
        "Content-Type: text/html",
        NULL
};

extern int edg_wll_NotifNewServer(edg_wll_Context,
				edg_wll_QueryRec const * const *, int flags, char const *,
				const edg_wll_NotifId, time_t *);
extern int edg_wll_NotifBindServer(edg_wll_Context,
				const edg_wll_NotifId, const char *, time_t *);
extern int edg_wll_NotifChangeServer(edg_wll_Context,
				const edg_wll_NotifId, edg_wll_QueryRec const * const *,
				edg_wll_NotifChangeOp);
extern int edg_wll_NotifRefreshServer(edg_wll_Context,
				const edg_wll_NotifId, time_t *);
extern int edg_wll_NotifDropServer(edg_wll_Context, edg_wll_NotifId *);



char *edg_wll_HTTPErrorMessage(int errCode)
{
	char *msg;
	
	switch (errCode) {
		case HTTP_OK: msg = "OK"; break;
		case HTTP_BADREQ: msg = "Bad Request"; break;
		case HTTP_UNAUTH: msg = "Unauthorized"; break;
		case HTTP_NOTFOUND: msg = "Not Found"; break;
		case HTTP_NOTALLOWED: msg = "Method Not Allowed"; break;
		case HTTP_UNSUPPORTED: msg = "Unsupported Media Type"; break;
		case HTTP_NOTIMPL: msg = "Not Implemented"; break;
		case HTTP_INTERNAL: msg = "Internal Server Error"; break;
		case HTTP_UNAVAIL: msg = "Service Unavailable"; break;
		case HTTP_INVALID: msg = "Invalid Data"; break;
		default: msg = "Unknown error"; break;
	}

	return msg;
}


/* returns non-zero if old style (V21) protocols incompatible */
static int is_protocol_incompatibleV21(char *user_agent)
{
        char *version, *comp_proto, *needle;
        double  v, c, my_v = strtod(PROTO_VERSION_V21, (char **) NULL), my_c;


	/* get version od the other side */
        if ((version = strstr(user_agent,"/")) == NULL) return(-1);
        else v = strtod(++version, &needle);

	/* sent the other side list of compatible protocols? */
	if ( needle[0] == '\0' ) return(-2);

	/* test compatibility if server newer*/
        if (my_v > v) {
                comp_proto=COMP_PROTO_V21;
                do {
                        my_c = strtod(comp_proto, &needle);
                        if (my_c == v) return(0);
                        comp_proto = needle + 1;
                } while (needle[0] != '\0');
                return(1);
        }

	/* test compatibility if server is older */
        else if (my_v < v) {
                do {
                        comp_proto = needle + 1;
                        c = strtod(comp_proto, &needle);
                        if (my_v == c) return(0);
                } while (needle[0] != '\0');
                return(1);
        }

	/* version match */
        return(0);
}


/* returns non-zero if protocols incompatible */
static int is_protocol_incompatible(char *user_agent)
{
        char *version, *comp_proto, *needle;
        double  v, c, my_v = strtod(PROTO_VERSION, (char **) NULL), my_c;


	/* get version od the other side */
        if ((version = strstr(user_agent,"/")) == NULL) return(-1);
        else v = strtod(++version, &needle);

	/* sent the other side list of compatible protocols? */
	if ( needle[0] == '\0' ) return(-2);

	/* test compatibility if server newer*/
        if (my_v > v) {
                comp_proto=COMP_PROTO;
                do {
                        my_c = strtod(comp_proto, &needle);
                        if (my_c == v) return(0);
                        comp_proto = needle + 1;
                } while (needle[0] != '\0');
                return(1);
        }

	/* test compatibility if server is older */
        else if (my_v < v) {
                do {
                        comp_proto = needle + 1;
                        c = strtod(comp_proto, &needle);
                        if (my_v == c) return(0);
                } while (needle[0] != '\0');
                return(1);
        }

	/* version match */
        return(0);
}


static int outputHTML(char **headers)
{
	int i;

	if (!headers) return 0;

	for (i=0; headers[i]; i++)
		if (!strncmp(headers[i], KEY_ACCEPT, sizeof(KEY_ACCEPT) - 1)) {
			if (strstr(headers[i],KEY_APP)) 
				return 0; 		/* message sent by edg_wll_Api */
			else 
				return 1;		/* message sent by other application */
		}
	return 1;

}


static int drain_text_request(char *request){
	int i = 0;
	while (!isspace(request[i])) i++;
	if (i < 5) 
		return 0;
	if (! strncmp(request+i-5, "?text", 5)){
		if (i == 5)
			strcpy(request+i-4, request+i); // keep '/'
		else
			strcpy(request+i-5, request+i);
		return 1;
	}
	else
		return 0;
}

static int getUserNotifications(edg_wll_Context ctx, char *user, char ***notifids){
        char *q = NULL;
        glite_lbu_Statement notifs = NULL;
        char *notifc[1] = {NULL};

        trio_asprintf(&q, "select notifid "
                "from notif_registrations "
                "where userid='%s'",
                user);
        if (edg_wll_ExecSQL(ctx, q, &notifs) < 0) goto err;
        free(q); q = NULL;

        int n = 0;
        *notifids = NULL;
        while(edg_wll_FetchRow(ctx, notifs, sizeof(notifc)/sizeof(notifc[0]), NULL, notifc)){
                n++;
                *notifids = realloc(*notifids, n*sizeof(**notifids));
                (*notifids)[n-1] = strdup(notifc[n-1]);
                printf("Notif %s found\n", notifc[n-1]);
        }
	if (n){
		*notifids = realloc(*notifids, (n+1)*sizeof(**notifids));
		(*notifids)[n] = NULL;
	}
        return n;

err:
        return 0;
}

static int getNotifInfo(edg_wll_Context ctx, char *notifId, notifInfo *ni){
	char *q = NULL;
        glite_lbu_Statement notif = NULL;
	char *notifc[4] = {NULL, NULL, NULL, NULL};

	trio_asprintf(&q, "select destination, valid, conditions, flags "
                "from notif_registrations "
                "where notifid='%s'",
                notifId);
	if (edg_wll_ExecSQL(ctx, q, &notif) < 0) goto err;
        free(q); q = NULL;

	ni->notifid = strdup(notifId);
	if (edg_wll_FetchRow(ctx, notif, sizeof(notifc)/sizeof(notifc[0]), NULL, notifc)){
		ni->destination = notifc[0];
		ni->valid = notifc[1];
		ni->conditions_text = notifc[2];
		parseJobQueryRec(ctx, notifc[2], strlen(notifc[2]), &(ni->conditions));
		ni->flags = atoi(notifc[3]);
	}
	else 
		goto err;

	return 0;

err:
	return  edg_wll_Error(ctx, NULL, NULL);
}

static void freeNotifInfo(notifInfo *ni){
	if (ni->notifid) free(ni->notifid);
	if (ni->destination) free(ni->destination);
	if (ni->valid) free(ni->valid);
	if (ni->conditions){
		edg_wll_QueryRec **p;
		int i;
		for (p = ni->conditions; *p; p++){
			for (i = 0; (*p)[i].attr; i++)
				edg_wll_QueryRecFree((*p)+i);
			free(*p);
		}
		free(ni->conditions);
	}
	if (ni->conditions_text) free(ni->conditions_text);
}

edg_wll_ErrorCode edg_wll_ProtoV21(edg_wll_Context ctx,
	char *request,char **headers,char *messageBody,
	char **response,char ***headersOut,char **bodyOut)
{
	char *requestPTR, *message = NULL;
	int	ret = HTTP_OK;
	int 	html = outputHTML(headers);
	int	i;

	edg_wll_ResetError(ctx);

	for (i=0; headers[i]; i++) /* find line with version number in headers */
		if ( strstr(headers[i], KEY_AGENT) ) break;
  
	if (headers[i] == NULL) { ret = HTTP_BADREQ; goto errV21; } /* if not present */
	switch (is_protocol_incompatibleV21(headers[i])) { 
		case 0  : /* protocols compatible */
			  ctx->is_V21 = 1;
			  break;
		case -1 : /* malformed 'User Agent:' line */
			  ret = HTTP_BADREQ;
			  goto errV21;
			  break;
		case -2 : /* version of one protocol unknown */
			  /* fallthrough */
		case 1  : /* protocols incompatible */
			  /* fallthrough */
		default : ret = HTTP_UNSUPPORTED; 
			  edg_wll_SetError(ctx,ENOTSUP,"Protocol versions are incompatible.");
			  goto errV21; 
			  break;
	}


/* GET */
	if (!strncmp(request, METHOD_GET, sizeof(METHOD_GET)-1)) {
		// Not supported
		ret = HTTP_BADREQ;
/* POST */
	} else if (!strncmp(request,METHOD_POST,sizeof(METHOD_POST)-1)) {

		requestPTR = request + sizeof(METHOD_POST)-1;
	
		if (!strncmp(requestPTR,KEY_QUERY_EVENTS,sizeof(KEY_QUERY_EVENTS)-1)) { 
        	        edg_wll_Event *eventsOut = NULL;
			edg_wll_QueryRec **job_conditions = NULL, **event_conditions = NULL;
			int i,j;

        	        if (parseQueryEventsRequestV21(ctx, messageBody, &job_conditions, &event_conditions)) 
				ret = HTTP_BADREQ;
				
	                else {
				int	fatal = 0;

				switch (edg_wll_QueryEventsServer(ctx,ctx->noAuth,
				    (const edg_wll_QueryRec **)job_conditions, 
				    (const edg_wll_QueryRec **)event_conditions, &eventsOut)) {

					case 0: if (html) ret = HTTP_NOTIMPL;
						else      ret = HTTP_OK; 
						break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM : ret = HTTP_UNAUTH; break;
					case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAUTH; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				/* glue errors (if eny) to XML responce */ 
				if (!html && !fatal)
					if (edg_wll_QueryEventsToXMLV21(ctx, eventsOut, &message))
						ret = HTTP_INTERNAL;
			}

			if (job_conditions) {
	                        for (j = 0; job_conditions[j]; j++) {
	                                for (i = 0 ; (job_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
        	                                edg_wll_QueryRecFree(&job_conditions[j][i]);
                	                free(job_conditions[j]);
                        	}
	                        free(job_conditions);
        	        }

	                if (event_conditions) {
        	                for (j = 0; event_conditions[j]; j++) {
                	                for (i = 0 ; (event_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
                        	                edg_wll_QueryRecFree(&event_conditions[j][i]);
                                	free(event_conditions[j]);
	                        }
        	                free(event_conditions);
        	        }
	
			if (eventsOut != NULL) {
	                	for (i=0; eventsOut[i].type != EDG_WLL_EVENT_UNDEF; i++) 
					edg_wll_FreeEvent(&(eventsOut[i]));
	                	edg_wll_FreeEvent(&(eventsOut[i])); /* free last line */
				free(eventsOut);
			}
        	}
		else if (!strncmp(requestPTR,KEY_QUERY_JOBS,sizeof(KEY_QUERY_JOBS)-1)) { 
        	        edg_wlc_JobId *jobsOut = NULL;
			edg_wll_JobStat *statesOut = NULL;
			edg_wll_QueryRec **conditions = NULL;
			int i,j, flags = 0;

        	        if (parseQueryJobsRequestV21(ctx, messageBody, &conditions, &flags))
				ret = HTTP_BADREQ;

	                else { 
				int		fatal = 0,
						retCode;
 
				if (flags & EDG_WLL_STAT_NO_JOBS) { 
					flags -= EDG_WLL_STAT_NO_JOBS;
					jobsOut = NULL;
					if (flags & EDG_WLL_STAT_NO_STATES) {
						flags -= EDG_WLL_STAT_NO_STATES;
						statesOut = NULL;
						retCode = edg_wll_QueryJobsServer(ctx, (const edg_wll_QueryRec **)conditions, flags, NULL, NULL);
					}
					else
						retCode = edg_wll_QueryJobsServer(ctx, (const edg_wll_QueryRec **)conditions, flags, NULL, &statesOut);
				}
				else {
					if (flags & EDG_WLL_STAT_NO_STATES) {
						flags -= EDG_WLL_STAT_NO_STATES;
						statesOut = NULL;
						retCode = edg_wll_QueryJobsServer(ctx, (const edg_wll_QueryRec **)conditions, flags, &jobsOut, NULL);
					}
					else
						retCode = edg_wll_QueryJobsServer(ctx, (const edg_wll_QueryRec **)conditions, flags, &jobsOut, &statesOut);
				}
				
				switch ( retCode ) {
					// case EPERM : ret = HTTP_UNAUTH;
					//              /* soft-error fall through */
					case 0: if (html) ret =  HTTP_NOTIMPL;
						else ret = HTTP_OK;

						break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM: ret = HTTP_UNAUTH; break;
					case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAUTH; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				if (!html && !fatal)
					if (edg_wll_QueryJobsToXMLV21(ctx, jobsOut, statesOut, &message))
						ret = HTTP_INTERNAL;
			}

	                if (conditions) {
        	                for (j = 0; conditions[j]; j++) {
                	                for (i = 0; (conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
                        	                edg_wll_QueryRecFree(&conditions[j][i]);
                                	free(conditions[j]);
	                        }
        	                free(conditions);
                	}

			if (jobsOut) {
				for (i=0; jobsOut[i]; i++) edg_wlc_JobIdFree(jobsOut[i]);
				free(jobsOut);
			}
			if (statesOut) {
	                	for (i=0; statesOut[i].state != EDG_WLL_JOB_UNDEF; i++) 
					edg_wll_FreeStatus(&(statesOut[i]));
	                	edg_wll_FreeStatus(&(statesOut[i])); /* free last line */
				free(statesOut);
			}
        	}
        /* POST [something else]: not understood */
		else ret = HTTP_BADREQ;

/* other HTTP methods */
	} else ret = HTTP_NOTALLOWED;

errV21:	asprintf(response,"HTTP/1.1 %d %s",ret,edg_wll_HTTPErrorMessage(ret));
	*headersOut = (char **) (html? response_headers_html : response_headers_dglb);
	if ((ret != HTTP_OK) && html)
		*bodyOut = edg_wll_ErrorToHTML(ctx,ret);
	else
		*bodyOut = message;

	return edg_wll_Error(ctx,NULL,NULL);
}



edg_wll_ErrorCode edg_wll_Proto(edg_wll_Context ctx,
	char *request,char **headers,char *messageBody,
	char **response,char ***headersOut,char **bodyOut)
{
	char *requestPTR = NULL, *message = NULL;
	int	ret = HTTP_OK;
	int 	html = outputHTML(headers);
	int 	text = 0; //XXX: Plain text communication is special case of html here, hence when text=1, html=1 too
	int	i;

	edg_wll_ResetError(ctx);

	for (i=0; headers[i]; i++) /* find line with version number in headers */
		if ( strstr(headers[i], KEY_AGENT) ) break;
  
	if (headers[i] == NULL) { ret = HTTP_BADREQ; goto err; } /* if not present */
	if (!html) {
		switch (is_protocol_incompatible(headers[i])) { 
			case 0  : /* protocols compatible */
				  ctx->is_V21 = 0;
				  break;
			case -1 : /* malformed 'User Agent:' line */
				  ret = HTTP_BADREQ;
				  goto err;
				  break;
			case 1  : /* protocols incompatible */
				  /* try old (V21) version compatibility */
				  edg_wll_ProtoV21(ctx, request, headers, messageBody, 
						  response, headersOut, bodyOut);
						  
				  /* and propagate errors or results */
				  return edg_wll_Error(ctx,NULL,NULL);
				  break;
			case -2 : /* version of one protocol unknown */
				  /* fallthrough */
			default : ret = HTTP_UNSUPPORTED; 
				  edg_wll_SetError(ctx,ENOTSUP,"Protocol versions are incompatible.");
				  goto err; 
				  break;
		}
	}


/* GET */
	if (!strncmp(request, METHOD_GET, sizeof(METHOD_GET)-1)) {
		
		requestPTR = strdup(request + sizeof(METHOD_GET)-1);
		if (html) text = drain_text_request(requestPTR);


	/* GET /: Current User Jobs */
		if (requestPTR[0]=='/' && (requestPTR[1]==' ' || requestPTR[1]=='?')) {
                	edg_wlc_JobId *jobsOut = NULL;
			int	i, flags;
			
			flags = (requestPTR[1]=='?') ? edg_wll_string_to_stat_flags(requestPTR + 2) : 0;

			//char **notifids = NULL;
			char *can_peername = edg_wll_gss_normalize_subj(ctx->peerName, 0);
		        /*char *userid = strmd5(can_peername, NULL);
			free(can_peername);
			getUserNotifications(ctx, userid, &notifids);*/

// FIXME: edg_wll_UserJobs should take flags as parameter
			switch (edg_wll_UserJobsServer(ctx,&jobsOut,NULL)) {
				case 0: if (text)
						edg_wll_UserInfoToText(ctx, jobsOut, &message);
					else if (html)
						edg_wll_UserInfoToHTML(ctx, jobsOut, &message);
					else ret = HTTP_OK;
					break;
				case ENOENT: ret = HTTP_NOTFOUND; break;
				case EPERM: ret = HTTP_UNAUTH; break;
				case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAVAIL; break;
				default: ret = HTTP_INTERNAL; break;
			}
			if (!html && (ret != HTTP_INTERNAL)) 
				if (edg_wll_UserJobsToXML(ctx, jobsOut, &message))
					ret = HTTP_INTERNAL;

			if (jobsOut) {
				for (i=0; jobsOut[i]; i++) edg_wlc_JobIdFree(jobsOut[i]);
				free(jobsOut);
			}
	        } 

	/* GET /[jobId]: Job Status */
		else if (*requestPTR=='/' 
			&& strncmp(requestPTR, "/notif", strlen("/notif"))
			&& *(requestPTR+strlen("/notif")-1) != '/'
			&& !isspace(*(requestPTR+strlen("/notif")-1))) {
			edg_wlc_JobId jobId = NULL;
			char *pom1,*fullid = NULL;
			edg_wll_JobStat stat;
			char *pomCopy;

			if (ctx->srvName == NULL) {
				edg_wll_SetError(ctx, EDG_WLL_ERROR_SERVER_RESPONSE,
							"no server name on GET /jobid");
				ret = HTTP_INTERNAL;
				goto err;
			}
			memset(&stat,0,sizeof(stat));
			pomCopy = strdup(requestPTR + 1);
			for (pom1=pomCopy; *pom1 && !isspace(*pom1); pom1++);
			*pom1 = 0;

			asprintf(&fullid,GLITE_JOBID_PROTO_PREFIX"%s:%u/%s",ctx->srvName,ctx->srvPort,pomCopy);
			free(pomCopy);	

			if (edg_wlc_JobIdParse(fullid, &jobId)) {
				edg_wll_SetError(ctx,EDG_WLL_ERROR_JOBID_FORMAT,fullid);
				ret = HTTP_BADREQ;
			}
			else switch (edg_wll_JobStatusServer(ctx,jobId,EDG_WLL_STAT_CLASSADS,&stat)) {
				case 0: if (text) 
						edg_wll_JobStatusToText(ctx,stat,&message); 
					else if (html)
						edg_wll_JobStatusToHTML(ctx,stat,&message);
					else ret = HTTP_OK;
					break;
				case ENOENT: ret = HTTP_NOTFOUND; break;
				case EINVAL: ret = HTTP_INVALID; break;
				case EPERM : ret = HTTP_UNAUTH; break;
				default: ret = HTTP_INTERNAL; break;
			}
			if (!html && (ret != HTTP_INTERNAL))
				if (edg_wll_JobStatusToXML(ctx,stat,&message))
					ret = HTTP_INTERNAL;

			free(fullid);
			edg_wlc_JobIdFree(jobId);
			edg_wll_FreeStatus(&stat);
	/*GET /notif[/]: All user's notifications*/
		} else if (strncmp(requestPTR, "/notif", strlen("/notif")) == 0 
			&& (*(requestPTR+strlen("/notif/")-1) == '/' && isspace(*(requestPTR+strlen("/notif/"))))
			|| isspace(*(requestPTR+strlen("/notif/")-1))){
			char **notifids = NULL;
			char *can_peername = edg_wll_gss_normalize_subj(ctx->peerName, 0);
                        char *userid = strmd5(can_peername, NULL);
			getUserNotifications(ctx, userid, &notifids);
			free(can_peername);
			if (text)
	                        edg_wll_UserNotifsToText(ctx, notifids, &message);
                        else if (html)
	                        edg_wll_UserNotifsToHTML(ctx, notifids, &message);
                        else ret = HTTP_OK;

	/*GET /notif/[notifId]: Notification info*/
		} else if (strncmp(requestPTR, "/notif/", strlen("/notif/")) == 0){
			notifInfo ni;
			char *pomCopy, *pom;
			pomCopy = strdup(requestPTR + 1);
                        for (pom=pomCopy; *pom && !isspace(*pom); pom++);
                        *pom = 0;
			getNotifInfo(ctx, strrchr(pomCopy, '/')+1, &ni);
			//TODO handle error
			free(pomCopy);

			if (text)
				edg_wll_NotificationToText(ctx, &ni, &message);
			else
				edg_wll_NotificationToHTML(ctx, &ni, &message);

			freeNotifInfo(&ni);

	/* GET [something else]: not understood */
		} else ret = HTTP_BADREQ;
		free(requestPTR); requestPTR = NULL;

/* POST */
	} else if (!strncmp(request,METHOD_POST,sizeof(METHOD_POST)-1)) {

		requestPTR = strdup(request + sizeof(METHOD_POST)-1);
		if (html) text = drain_text_request(requestPTR);
	
		if (!strncmp(requestPTR,KEY_QUERY_EVENTS,sizeof(KEY_QUERY_EVENTS)-1)) { 
        	        edg_wll_Event *eventsOut = NULL;
			edg_wll_QueryRec **job_conditions = NULL, **event_conditions = NULL;
			int i,j;

        	        if (parseQueryEventsRequest(ctx, messageBody, &job_conditions, &event_conditions)) 
				ret = HTTP_BADREQ;
				
	                else {
				int	fatal = 0;

				switch (edg_wll_QueryEventsServer(ctx,ctx->noAuth,
				    (const edg_wll_QueryRec **)job_conditions, 
				    (const edg_wll_QueryRec **)event_conditions, &eventsOut)) {

					case 0: if (html) ret = HTTP_NOTIMPL;
						else      ret = HTTP_OK; 
						break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM : ret = HTTP_UNAUTH; break;
					case E2BIG: ret = HTTP_UNAUTH; break;
					case EINVAL: ret = HTTP_UNAUTH; break;
					case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAUTH; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				/* glue errors (if eny) to XML responce */ 
				if (!html && !fatal)
					if (edg_wll_QueryEventsToXML(ctx, eventsOut, &message))
						ret = HTTP_INTERNAL;
			}

			if (job_conditions) {
	                        for (j = 0; job_conditions[j]; j++) {
	                                for (i = 0 ; (job_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
        	                                edg_wll_QueryRecFree(&job_conditions[j][i]);
                	                free(job_conditions[j]);
                        	}
	                        free(job_conditions);
        	        }

	                if (event_conditions) {
        	                for (j = 0; event_conditions[j]; j++) {
                	                for (i = 0 ; (event_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
                        	                edg_wll_QueryRecFree(&event_conditions[j][i]);
                                	free(event_conditions[j]);
	                        }
        	                free(event_conditions);
        	        }
	
			if (eventsOut != NULL) {
	                	for (i=0; eventsOut[i].type != EDG_WLL_EVENT_UNDEF; i++) 
					edg_wll_FreeEvent(&(eventsOut[i]));
	                	edg_wll_FreeEvent(&(eventsOut[i])); /* free last line */
				free(eventsOut);
			}
        	}
		else if (!strncmp(requestPTR,KEY_QUERY_JOBS,sizeof(KEY_QUERY_JOBS)-1)) { 
        	        edg_wlc_JobId *jobsOut = NULL;
			edg_wll_JobStat *statesOut = NULL;
			edg_wll_QueryRec **conditions = NULL;
			int i,j, flags = 0;

        	        if (parseQueryJobsRequest(ctx, messageBody, &conditions, &flags))
				ret = HTTP_BADREQ;

	                else { 
				int		fatal = 0,
						retCode;
 
				if (flags & EDG_WLL_STAT_NO_JOBS) { 
					flags -= EDG_WLL_STAT_NO_JOBS;
					jobsOut = NULL;
					if (flags & EDG_WLL_STAT_NO_STATES) {
						flags -= EDG_WLL_STAT_NO_STATES;
						statesOut = NULL;
						retCode = edg_wll_QueryJobsServer(ctx, (const edg_wll_QueryRec **)conditions, flags, NULL, NULL);
					}
					else
						retCode = edg_wll_QueryJobsServer(ctx, (const edg_wll_QueryRec **)conditions, flags, NULL, &statesOut);
				}
				else {
					if (flags & EDG_WLL_STAT_NO_STATES) {
						flags -= EDG_WLL_STAT_NO_STATES;
						statesOut = NULL;
						retCode = edg_wll_QueryJobsServer(ctx, (const edg_wll_QueryRec **)conditions, flags, &jobsOut, NULL);
					}
					else
						retCode = edg_wll_QueryJobsServer(ctx, (const edg_wll_QueryRec **)conditions, flags, &jobsOut, &statesOut);
				}
				
				switch ( retCode ) {
					case 0: if (html) ret =  HTTP_NOTIMPL;
						else ret = HTTP_OK;

						break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM: ret = HTTP_UNAUTH; break;
					case E2BIG: ret = HTTP_UNAUTH; break;
					case EINVAL: ret = HTTP_UNAUTH; break;
					case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAUTH; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				if (!html && !fatal)
					if (edg_wll_QueryJobsToXML(ctx, jobsOut, statesOut, &message))
						ret = HTTP_INTERNAL;
			}

	                if (conditions) {
        	                for (j = 0; conditions[j]; j++) {
                	                for (i = 0; (conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
                        	                edg_wll_QueryRecFree(&conditions[j][i]);
                                	free(conditions[j]);
	                        }
        	                free(conditions);
                	}

			if (jobsOut) {
				for (i=0; jobsOut[i]; i++) edg_wlc_JobIdFree(jobsOut[i]);
				free(jobsOut);
			}
			if (statesOut) {
	                	for (i=0; statesOut[i].state != EDG_WLL_JOB_UNDEF; i++) 
					edg_wll_FreeStatus(&(statesOut[i]));
	                	edg_wll_FreeStatus(&(statesOut[i])); /* free last line */
				free(statesOut);
			}
        	}
		else if (!strncmp(requestPTR,KEY_PURGE_REQUEST,sizeof(KEY_PURGE_REQUEST)-1)) {
			edg_wll_PurgeRequest    request;
			edg_wll_PurgeResult	result;
			int			fatal = 0;

			ctx->p_tmp_timeout.tv_sec = 86400;  

			memset(&request,0,sizeof(request));
			memset(&result,0,sizeof(result));

			if ( !parsePurgeRequest(ctx,messageBody,(int (*)()) edg_wll_StringToStat,&request) ) {
				switch ( edg_wll_PurgeServer(ctx, (const edg_wll_PurgeRequest *)&request, &result)) {
					case 0: if (html) ret =  HTTP_NOTIMPL;
						else ret = HTTP_OK;

						break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM: ret = HTTP_UNAUTH; break;
					case EINVAL: ret = HTTP_INVALID; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				if (!html && !fatal) {
					if (edg_wll_PurgeResultToXML(ctx, &result, &message))
						ret = HTTP_INTERNAL;
					else
						printf("%s", message);
				}				

				/* result is now packed in message, free it */	
				if ( result.server_file )
					free(result.server_file);
				if ( result.jobs )
				{
					for ( i = 0; result.jobs[i]; i++ )
						free(result.jobs[i]);
					free(result.jobs);
				}

			}

			if ( request.jobs )
			{
				int i;
				for ( i = 0; request.jobs[i]; i++ )
					free(request.jobs[i]);
				free(request.jobs);
			}

		}
		else if (!strncmp(requestPTR,KEY_DUMP_REQUEST,sizeof(KEY_DUMP_REQUEST)-1)) {
			edg_wll_DumpRequest	request;
			edg_wll_DumpResult	result;
		
			ctx->p_tmp_timeout.tv_sec = 86400;  

			memset(&request,0,sizeof(request));	
			memset(&result,0,sizeof(result));
			
        	        if (parseDumpRequest(ctx, messageBody, &request))
				ret = HTTP_BADREQ;
			else {
				int     fatal = 0;
				
				switch (edg_wll_DumpEventsServer(ctx,(const edg_wll_DumpRequest *) &request, &result)) {
					case 0: if (html) ret = HTTP_NOTIMPL;
						else      ret = HTTP_OK; 
						break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM : ret = HTTP_UNAUTH; break;
					case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAUTH; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				/* glue errors (if eny) to XML responce */ 
				if (!html && !fatal)
					if (edg_wll_DumpResultToXML(ctx, &result, &message))
						ret = HTTP_INTERNAL;
			}

			free(result.server_file);
		}
		else if (!strncmp(requestPTR,KEY_LOAD_REQUEST,sizeof(KEY_LOAD_REQUEST)-1)) {
			edg_wll_LoadRequest	request;
			edg_wll_LoadResult	result;
		
			ctx->p_tmp_timeout.tv_sec = 86400;  

			memset(&request,0,sizeof(request));	
			memset(&result,0,sizeof(result));
			
        	        if (parseLoadRequest(ctx, messageBody, &request))
				ret = HTTP_BADREQ;
			else {
				int     fatal = 0;
				
				switch (edg_wll_LoadEventsServer(ctx,(const edg_wll_LoadRequest *) &request, &result)) {
					case 0: if (html) ret = HTTP_NOTIMPL;
						else      ret = HTTP_OK; 
						break;
					case EEXIST: ret = HTTP_OK; break;
					case EINVAL: ret = HTTP_INVALID; break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM : ret = HTTP_UNAUTH; break;
					case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAUTH; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				/* glue errors (if eny) to XML responce */ 
				if (!html && !fatal)
					if (edg_wll_LoadResultToXML(ctx, &result, &message))
						ret = HTTP_INTERNAL;
			}

			free(result.server_file);
		}
		else if (!strncmp(requestPTR,KEY_INDEXED_ATTRS,sizeof(KEY_INDEXED_ATTRS)-1)) {
			if (!html)
				if (edg_wll_IndexedAttrsToXML(ctx, &message))
					ret = HTTP_INTERNAL;
		}
		else if (!strncmp(requestPTR,KEY_NOTIF_REQUEST,sizeof(KEY_NOTIF_REQUEST)-1)) {
			char *function, *address;
			edg_wll_NotifId	notifId;
			edg_wll_NotifChangeOp op;
			edg_wll_QueryRec **conditions;
			int flags;
			time_t validity = -1;
			int i,j;
			
			
        	        if (parseNotifRequest(ctx, messageBody, &function, &notifId, 
						&address, &op, &validity, &conditions, &flags))
				ret = HTTP_BADREQ;
			else {
				int     fatal = 0, err = 0;
				
				// XXX presne poradi parametru zatim neni znamo
				// navratove chyby nejsou zname, nutno predelat dle aktualni situace
				if (!strcmp(function,"New")) 
					err = edg_wll_NotifNewServer(ctx,
								(edg_wll_QueryRec const * const *)conditions, flags,
								address, notifId, &validity);
				else if (!strcmp(function,"Bind"))
					err = edg_wll_NotifBindServer(ctx, notifId, address, &validity);
				else if (!strcmp(function,"Change"))
					err = edg_wll_NotifChangeServer(ctx, notifId,
								(edg_wll_QueryRec const * const *)conditions, op);
				else if (!strcmp(function,"Refresh"))
					err = edg_wll_NotifRefreshServer(ctx, notifId, &validity);
				else if (!strcmp(function,"Drop"))
					err = edg_wll_NotifDropServer(ctx, notifId);
				
				switch (err) {
					case 0: if (html) ret = HTTP_NOTIMPL;
						else      ret = HTTP_OK; 
						break;
					case EEXIST: ret = HTTP_OK; break;
					case EINVAL: ret = HTTP_INVALID; break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM : ret = HTTP_UNAUTH; break;
					case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAUTH; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				/* glue errors (if eny) to XML responce */ 
				if (!html && !fatal)
					if (edg_wll_NotifResultToXML(ctx, validity, &message))
						ret = HTTP_INTERNAL;
			}
			
			free(function);
			free(address);
			edg_wll_NotifIdFree(notifId);
	                if (conditions) {
        	                for (j = 0; conditions[j]; j++) {
                	                for (i = 0; (conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
                        	                edg_wll_QueryRecFree(&conditions[j][i]);
                                	free(conditions[j]);
	                        }
        	                free(conditions);
                	}
		}
		else if (!strncmp(requestPTR,KEY_QUERY_SEQUENCE_CODE,sizeof(KEY_QUERY_SEQUENCE_CODE)-1)) {
			char		*source = NULL;
			char		*seqCode = NULL;
			edg_wlc_JobId	jobId = NULL;
			

			if (parseQuerySequenceCodeRequest(ctx, messageBody, &jobId, &source))
				ret = HTTP_BADREQ;
			else {
				int     fatal = 0;
				
				switch (edg_wll_QuerySequenceCodeServer(ctx, jobId, source, &seqCode)) {
					case 0: if (html) ret = HTTP_NOTIMPL;
						else      ret = HTTP_OK; 
						break;
					case EEXIST: ret = HTTP_OK; break;
					case EINVAL: ret = HTTP_INVALID; break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM : ret = HTTP_UNAUTH; break;
					case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAUTH; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				
				/* glue errors (if eny) to XML responce */ 
				if (!html && !fatal)
					if (edg_wll_QuerySequenceCodeResultToXML(ctx, seqCode, &message))
						ret = HTTP_INTERNAL;
			}

			if ( source ) free(source);
			if ( seqCode ) free(seqCode);
			edg_wlc_JobIdFree(jobId);
		}
		else if (!strncmp(requestPTR,KEY_STATS_REQUEST,sizeof(KEY_STATS_REQUEST)-1)) {
			char *function;
			edg_wll_QueryRec **conditions;
			edg_wll_JobStatCode major = EDG_WLL_JOB_UNDEF;
			time_t from, to;
			int i, j, minor, res_from, res_to;
			float rate = 0, duration = 0;
			
			
			
        	        if (parseStatsRequest(ctx, messageBody, &function, &conditions, 
						&major, &minor, &from, &to))
				ret = HTTP_BADREQ;
			else {
				int     fatal = 0, err = 0;
				
				// XXX presne poradi parametru zatim neni znamo
				// navratove chyby nejsou zname, nutno predelat dle aktualni situace
				if (!strcmp(function,"Rate")) 
					err = edg_wll_StateRateServer(ctx,
						conditions[0], major, minor, 
						&from, &to, &rate, &res_from, &res_to); 
				else if (!strcmp(function,"Duration"))
					err = edg_wll_StateDurationServer(ctx,
						conditions[0], major, minor, 
						&from, &to, &duration, &res_from, &res_to); 
				
				switch (err) {
					case 0: if (html) ret = HTTP_NOTIMPL;
						else      ret = HTTP_OK; 
						break;
					case ENOSYS: ret = HTTP_NOTIMPL; break;
					case EEXIST: ret = HTTP_OK; break;
					case EINVAL: ret = HTTP_INVALID; break;
					case ENOENT: ret = HTTP_NOTFOUND; break;
					case EPERM : ret = HTTP_UNAUTH; break;
					case EDG_WLL_ERROR_NOINDEX: ret = HTTP_UNAUTH; break;
					case ENOMEM: fatal = 1; ret = HTTP_INTERNAL; break;
					default: ret = HTTP_INTERNAL; break;
				}
				/* glue errors (if eny) to XML responce */ 
				if (!html && !fatal)
					if (edg_wll_StatsResultToXML(ctx, from, to, rate, 
							duration, res_from, res_to, &message))
						ret = HTTP_INTERNAL;
			}
			
			free(function);
	                if (conditions) {
        	                for (j = 0; conditions[j]; j++) {
                	                for (i = 0; (conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
                        	                edg_wll_QueryRecFree(&conditions[j][i]);
                                	free(conditions[j]);
	                        }
        	                free(conditions);
                	}
		}

			
        /* POST [something else]: not understood */
		else ret = HTTP_BADREQ;

		free(requestPTR); requestPTR = NULL;

/* other HTTP methods */
	} else ret = HTTP_NOTALLOWED;

err:	asprintf(response,"HTTP/1.1 %d %s",ret,edg_wll_HTTPErrorMessage(ret));
	*headersOut = (char **) (html ? response_headers_html : response_headers_dglb);
	if ((ret != HTTP_OK) && text)
                *bodyOut = edg_wll_ErrorToText(ctx,ret);
	else if ((ret != HTTP_OK) && html)
		*bodyOut = edg_wll_ErrorToHTML(ctx,ret);
	else
		*bodyOut = message;

	if (requestPTR) free(requestPTR);

	return edg_wll_Error(ctx,NULL,NULL);
}
