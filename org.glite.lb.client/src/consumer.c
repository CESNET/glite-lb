#ident "$Header$"
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


#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>

#include "glite/jobid/cjobid.h"
#include "glite/lb/context-int.h"
#include "glite/lb/mini_http.h"
#include "glite/lb/xml_parse.h"
#include "glite/lb/xml_conversions.h"

#include "consumer.h"
#include "connection.h"

static const char* const request_headers[] = {
	"Cache-Control: no-cache",
	"Accept: application/x-dglb",
	"User-Agent: edg_wll_Api/" PROTO_VERSION "/" COMP_PROTO,
	"Content-Type: application/x-dglb",
	NULL
};

int set_server_name_and_port(edg_wll_Context, const edg_wll_QueryRec **);

int edg_wll_QueryEventsExt(
		edg_wll_Context ctx,
		const edg_wll_QueryRec **job_conditions,
		const edg_wll_QueryRec **event_conditions,
		edg_wll_Event **eventsOut)
{
	char	*response	= NULL,
		*message	= NULL,
		*send_mess	= NULL;

	edg_wll_ResetError(ctx);

	if ( edg_wll_QueryEventsRequestToXML(ctx, job_conditions, event_conditions, &send_mess) != 0 )
	{
		edg_wll_SetError(ctx , (edg_wll_ErrorCode) EINVAL, "Invalid query record.");
		goto err;
	}

	ctx->p_tmp_timeout = ctx->p_query_timeout;
	
	if (ctx->isProxy) {
		ctx->isProxy = 0;
		if (edg_wll_http_send_recv_proxy(ctx, "POST /queryEvents HTTP/1.1",
				request_headers,send_mess, &response,NULL,&message))
			goto err;
	}
	else {
		if (set_server_name_and_port(ctx,job_conditions))
			goto err; // XXX is it fatal??

		if (edg_wll_http_send_recv(ctx, "POST /queryEvents HTTP/1.1",
				request_headers,send_mess, &response,NULL,&message))
			goto err;
	}

	if (http_check_status(ctx,response))
		goto err;

	edg_wll_ParseQueryEvents(ctx,message,eventsOut);

err:
	free(response);
	free(message);
	free(send_mess);
	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_QueryEvents(
		edg_wll_Context ctx,
		const edg_wll_QueryRec *job_conditions,
		const edg_wll_QueryRec *event_conditions,
	       	edg_wll_Event **eventsOut)
{
	edg_wll_QueryRec  	**jconds = NULL,
				**econds = NULL;
	int			i,
				njconds, neconds,
				ret;

	if ( job_conditions )
	{
		for ( njconds = 0; job_conditions[njconds].attr != EDG_WLL_QUERY_ATTR_UNDEF ; njconds++ )
			;
		jconds = (edg_wll_QueryRec **) calloc(njconds+1, sizeof(edg_wll_QueryRec *));
		for ( i = 0; i < njconds; i++ )
		{
			jconds[i] = (edg_wll_QueryRec *) calloc(2, sizeof(edg_wll_QueryRec));
			jconds[i][0] = job_conditions[i];
		}
	}

	if ( event_conditions )
	{
		for ( neconds = 0; event_conditions[neconds].attr != EDG_WLL_QUERY_ATTR_UNDEF ; neconds++ )
			;
		econds = (edg_wll_QueryRec **) calloc(neconds+1, sizeof(edg_wll_QueryRec *));
		for ( i = 0; i < neconds; i++ )
		{
			econds[i] = (edg_wll_QueryRec *) calloc(2, sizeof(edg_wll_QueryRec));
			econds[i][0] = event_conditions[i];
		}
	}

	if ( econds && jconds )
		ret = edg_wll_QueryEventsExt(ctx, (const edg_wll_QueryRec **) jconds, 
				    (const edg_wll_QueryRec **) econds, eventsOut);
	if ( econds && !jconds )
		ret = edg_wll_QueryEventsExt(ctx, NULL, (const edg_wll_QueryRec **) econds, eventsOut);
	if ( !econds && jconds )
		ret = edg_wll_QueryEventsExt(ctx, (const edg_wll_QueryRec **) jconds, NULL, eventsOut);
	if ( !econds && !jconds )
		ret = edg_wll_QueryEventsExt(ctx, NULL, NULL, eventsOut);

	if ( jconds )
	{
		for ( i = 0; i < njconds ; i++ )
			free(jconds[i]);
		free(jconds);
	}
	if ( econds )
	{
		for ( i = 0; i < neconds ; i++ )
			free(econds[i]);
		free(econds);
	}

	return ret;
}


int edg_wll_QueryJobsExt(
		edg_wll_Context         ctx,
		const edg_wll_QueryRec **        conditions,
		int                     flags,
		edg_wlc_JobId **        jobsOut,
		edg_wll_JobStat **      statesOut)
{
	char			*response = NULL, *message = NULL, *send_mess = NULL;
	
	edg_wll_ResetError(ctx);

	if (!jobsOut)	flags |= EDG_WLL_STAT_NO_JOBS;
	if (!statesOut) {flags = 0; flags |= EDG_WLL_STAT_NO_STATES;}
	if (edg_wll_QueryJobsRequestToXML(ctx, conditions, flags, &send_mess) != 0) {
		edg_wll_SetError(ctx , (edg_wll_ErrorCode) EINVAL, "Invalid query record.");
		goto err;
	}

	ctx->p_tmp_timeout = ctx->p_query_timeout;

	if (ctx->isProxy){
		ctx->isProxy = 0;
		if (edg_wll_http_send_recv_proxy(ctx, "POST /queryJobs HTTP/1.1",
				request_headers,send_mess,&response,NULL,&message))
			goto err;
	}
	else {				
		if (set_server_name_and_port(ctx, conditions))
			goto err;

		if (edg_wll_http_send_recv(ctx, "POST /queryJobs HTTP/1.1",
					request_headers,send_mess,&response,NULL,&message)) 
			goto err;
	}
         
	if (http_check_status(ctx,response))
		goto err;

  	edg_wll_ParseQueryJobs(ctx,message,jobsOut,statesOut);

err:
	free(response);
	free(message);
	free(send_mess);

	return edg_wll_Error(ctx,NULL,NULL);
}

int edg_wll_QueryJobs(
        	edg_wll_Context         ctx,
	        const edg_wll_QueryRec *        conditions,
        	int                     flags,
	        edg_wlc_JobId **        jobsOut,
        	edg_wll_JobStat **      statesOut)
{
	edg_wll_QueryRec	**conds;
	int			i, nconds, ret;

	if ( !conditions )
		return edg_wll_QueryJobsExt(ctx, NULL, flags, jobsOut, statesOut);

	for ( nconds = 0; conditions[nconds].attr != EDG_WLL_QUERY_ATTR_UNDEF ; nconds++ )
		;
	conds = (edg_wll_QueryRec **) malloc((nconds+1) * sizeof(edg_wll_QueryRec *));
	conds[nconds] = NULL;
	for ( i = 0; i < nconds ; i++ )
	{
		conds[i] = (edg_wll_QueryRec *) malloc(2 * sizeof(edg_wll_QueryRec));
		conds[i][0] = conditions[i];
		conds[i][1].attr = EDG_WLL_QUERY_ATTR_UNDEF;
	}

	ret = edg_wll_QueryJobsExt(ctx, (const edg_wll_QueryRec **) conds, flags, jobsOut, statesOut);

	for ( i = 0; i < nconds ; i++ )
		free(conds[i]);
	free(conds);
		

	return ret;
}



int edg_wll_GetIndexedAttrs(
        edg_wll_Context         ctx,
        edg_wll_QueryRec       ***attrs)
{	
	char			*response = NULL, *send_mess = NULL, *message = NULL;
	
	edg_wll_ResetError(ctx);

	edg_wll_IndexedAttrsRequestToXML(ctx, &send_mess);
	
	if (set_server_name_and_port(ctx, NULL))
		goto err;

	ctx->p_tmp_timeout = ctx->p_query_timeout;

	if (edg_wll_http_send_recv(ctx, "POST /indexedAttrs HTTP/1.1",request_headers, send_mess,
			&response,NULL,&message)) 
		goto err;
         
	if (http_check_status(ctx,response))
		goto err;

  	edg_wll_ParseIndexedAttrs(ctx,message,attrs);

err:
	free(response);
	free(message);

	return edg_wll_Error(ctx,NULL,NULL);
}


/* 
 * wrappers around edg_wll_Query()
 */

int edg_wll_UserJobs(
		edg_wll_Context ctx,
		edg_wlc_JobId **jobsOut,
		edg_wll_JobStat	**statesOut)
{
	edg_wll_QueryRec        j[2];
	
	memset(j,0,sizeof j);
	
	j[0].attr = EDG_WLL_QUERY_ATTR_OWNER;
	j[0].op = EDG_WLL_QUERY_OP_EQUAL;
	j[0].value.c = ctx->peerName;
	
	return edg_wll_QueryJobs(ctx,j,0,jobsOut,statesOut);
}

int edg_wll_JobLog(
	edg_wll_Context ctx,
	glite_jobid_const_t	job,
	edg_wll_Event **eventsOut)
{
	edg_wll_QueryRec	j[2], e[2];

	memset(j,0,sizeof j);
	memset(e,0,sizeof e);

	j[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	j[0].op = EDG_WLL_QUERY_OP_EQUAL;
	j[0].value.j = job;

	e[0].attr = EDG_WLL_QUERY_ATTR_LEVEL;
	e[0].op = EDG_WLL_QUERY_OP_LESS;
	e[0].value.i = ctx->p_level + 1;

	return edg_wll_QueryEvents(ctx,j,e,eventsOut);
}

int edg_wll_JobStatus(
                edg_wll_Context ctx,
                glite_jobid_const_t job,
                int flags,
                edg_wll_JobStat *stat)
{
	edg_wll_QueryRec        j[2];
	edg_wll_JobStat		*statesOut = NULL;
	int 			ret;

	memset(j,0,sizeof j);

	j[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
	j[0].op = EDG_WLL_QUERY_OP_EQUAL;
	j[0].value.j = job;
	j[1].attr = EDG_WLL_QUERY_ATTR_UNDEF;

	ret = edg_wll_QueryJobs(ctx,j,flags,NULL,&statesOut);

	if (ret) return ret;

	if (statesOut) {
		if (statesOut[0].state == EDG_WLL_JOB_UNDEF) {
			memcpy(stat, statesOut, sizeof(edg_wll_JobStat));
			free(statesOut);	
			ret = edg_wll_SetError(ctx , (edg_wll_ErrorCode) ENOENT, "Query returned no result.");
		}
		else {
			/* check wheter there is only one field in status reply */
			assert(statesOut[1].state == EDG_WLL_JOB_UNDEF);
			/* copy only 1st status */
			memcpy(stat, statesOut, sizeof(edg_wll_JobStat));
			/* release only array of states, keep all links unfreed for the previous copy */
			free(statesOut);
		}
	}

	return ret;
}


	
int edg_wll_QueryListener(edg_wll_Context ctx, glite_jobid_const_t job, const char *name, char** host, uint16_t *port) {

	int 		i;
	edg_wll_Event      	*events = NULL;
	int 		errCode = 0;
        edg_wll_QueryRec    jr[2],er[2];
	int		found = 0;

	memset(jr,0,sizeof jr);
	memset(er,0,sizeof er);
        jr[0].attr = EDG_WLL_QUERY_ATTR_JOBID;
        jr[0].op = EDG_WLL_QUERY_OP_EQUAL;
        jr[0].value.j = job;

        er[0].attr = EDG_WLL_QUERY_ATTR_EVENT_TYPE;
        er[0].op = EDG_WLL_QUERY_OP_EQUAL;
        er[0].value.i = EDG_WLL_EVENT_LISTENER;

        if (edg_wll_QueryEvents(ctx, jr, er, &events)) {
		return edg_wll_Error(ctx, NULL, NULL);
	}		
	
	for (i=0; events[i].type != EDG_WLL_EVENT_UNDEF; i++) {
		if (!strcmp(name, events[i].listener.svc_name)) {
			found = 1;
			if (host != NULL)
				*host = strdup(events[i].listener.svc_host);
			if (port != NULL)
				*port = events[i].listener.svc_port;
		}
		edg_wll_FreeEvent(&events[i]);
	}
	free(events);
	
	if (!found)
		errCode = ENOENT;

	return edg_wll_SetError(ctx, errCode, NULL);
}


int set_server_name_and_port(edg_wll_Context ctx, const edg_wll_QueryRec **job_conditions)
{
	int				i = 0, j,
					found = 0,
					error = 0;
	unsigned int				srvPort = 0,
					srvPortTmp;
	char		   *srvName = NULL,
				   *srvNameTmp;


	if ( job_conditions ) for ( j = 0; job_conditions[j]; j++ )
		for ( i = 0; (job_conditions[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
			if ( job_conditions[j][i].attr == EDG_WLL_QUERY_ATTR_JOBID)
			{
				edg_wlc_JobIdGetServerParts(job_conditions[j][i].value.j,&srvNameTmp,&srvPortTmp);
				if ( found )
				{
					if ( strcmp(srvName, srvNameTmp) || (srvPort != srvPortTmp) )
					{
						free(srvNameTmp); free(srvName);
						return edg_wll_SetError(ctx, EINVAL, "Two different servers specifieed in one query");
					}
					free(srvNameTmp);
				}
				else
				{
					srvName = srvNameTmp;
					srvPort = srvPortTmp;
					found = 1;
				}
			} 	

	if ( found && !ctx->p_query_server_override)
	{ 
		if (!ctx->srvName)
		{
			ctx->srvName = strdup(srvName);
			ctx->srvPort = srvPort;
			free(srvName);
		}
		else if (strcmp(srvName, ctx->srvName) || (srvPort != ctx->srvPort))
		{
			free(ctx->srvName);
			ctx->srvName = strdup(srvName);
			ctx->srvPort = srvPort;
			free(srvName);
		}
	}
	else if ( !ctx->srvName || !ctx->srvPort )
	{
		if (found) free(srvName);
		if (!ctx->p_query_server) 
			return(edg_wll_SetError(ctx, (edg_wll_ErrorCode) EINVAL, "Hostname of server to query is not set"));
		else ctx->srvName = strdup(ctx->p_query_server);
		if (!ctx->p_query_server_port)
			return(edg_wll_SetError(ctx, (edg_wll_ErrorCode) EINVAL, "Port of server to query is not set"));
		else ctx->srvPort = ctx->p_query_server_port;
	}
		
	return(error);
}

int edg_wll_QuerySequenceCodeProxy(edg_wll_Context ctx, glite_jobid_const_t jobId, char **code) 
{
	int		error = 0;
	char   *response = NULL,
		   *message	= NULL,
		   *send_mess = NULL;
	
	
	ctx->isProxy = 1;
	edg_wll_ResetError(ctx);

	if ( edg_wll_QuerySequenceCodeToXML(ctx, jobId, &send_mess) != 0 )
	{
		edg_wll_SetError(ctx , (edg_wll_ErrorCode) EINVAL, "Invalid query record.");
		goto err;
	}

	ctx->p_tmp_timeout = ctx->p_query_timeout; 
	
	error = edg_wll_http_send_recv_proxy(ctx, "POST /querySequenceCode HTTP/1.1",
			request_headers, send_mess, &response, NULL, &message);
	if ( error != 0 )
		goto err;

	if (http_check_status(ctx,response))
		goto err;

	edg_wll_ParseQuerySequenceCodeResult(ctx,message,code);

err:
	free(response);
	free(message);
	free(send_mess);

	return edg_wll_Error(ctx,NULL,NULL);
}


/******************************************************************
 * Proxy wrappers
 */


int edg_wll_QueryEventsExtProxy(
		edg_wll_Context ctx,
		const edg_wll_QueryRec **job_conditions,
		const edg_wll_QueryRec **event_conditions,
		edg_wll_Event **eventsOut)
{
	ctx->isProxy = 1;

	return edg_wll_QueryEventsExt(ctx, job_conditions, event_conditions, eventsOut);
}



int edg_wll_QueryEventsProxy(
		edg_wll_Context ctx,
		const edg_wll_QueryRec *job_conditions,
		const edg_wll_QueryRec *event_conditions,
	       	edg_wll_Event **eventsOut)
{
	ctx->isProxy = 1;
	
	return edg_wll_QueryEvents(ctx, job_conditions, event_conditions, eventsOut);
}



int edg_wll_QueryJobsExtProxy(
		edg_wll_Context         ctx,
		const edg_wll_QueryRec **        conditions,
		int                     flags,
		edg_wlc_JobId **        jobsOut,
		edg_wll_JobStat **      statesOut)
{	
	ctx->isProxy = 1;
	
	return edg_wll_QueryJobsExt(ctx, conditions, flags, jobsOut, statesOut);
}



int edg_wll_QueryJobsProxy(
        	edg_wll_Context         ctx,
	        const edg_wll_QueryRec *        conditions,
        	int                     flags,
	        edg_wlc_JobId **        jobsOut,
        	edg_wll_JobStat **      statesOut)
{
	ctx->isProxy = 1;
	
	return edg_wll_QueryJobs(ctx, conditions, flags, jobsOut, statesOut);
}


int edg_wll_UserJobsProxy(
		edg_wll_Context ctx,
		edg_wlc_JobId **jobsOut,
		edg_wll_JobStat	**statesOut)
{
	ctx->isProxy = 1;
	if ( ctx->p_user_lbproxy ) ctx->peerName = strdup(ctx->p_user_lbproxy);
	
	return edg_wll_UserJobs(ctx, jobsOut, statesOut);
}

int edg_wll_JobLogProxy(
	edg_wll_Context ctx,
	glite_jobid_const_t	job,
	edg_wll_Event **eventsOut)
{
	ctx->isProxy = 1;

	return edg_wll_JobLog(ctx, job, eventsOut);
}

int edg_wll_JobStatusProxy(
                edg_wll_Context ctx,
                glite_jobid_const_t job,
                int flags,
                edg_wll_JobStat *stat)
{
	ctx->isProxy = 1;
		
	return edg_wll_JobStatus(ctx, job, flags, stat);
}


	
int edg_wll_QueryListenerProxy(
		edg_wll_Context ctx,
		glite_jobid_const_t job,
		const char *name,
		char** host,
		uint16_t *port) 
{
	ctx->isProxy = 1;

	return edg_wll_QueryListener(ctx, job, name, host, port);
}
