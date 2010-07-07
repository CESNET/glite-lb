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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include "glite/lb/context.h"
#include "glite/lb/context-int.h"
#include "glite/lb/xml_parse.h"
#include "glite/lb/mini_http.h"

#include "statistics.h"
#include "connection.h"



static const char* const request_headers[] = {
	"Cache-Control: no-cache",
	"Accept: application/x-dglb",
	"User-Agent: edg_wll_Api/" PROTO_VERSION "/" COMP_PROTO,
	"Content-Type: application/x-dglb",
	NULL
};

static int set_server_name_and_port(edg_wll_Context, const edg_wll_QueryRec **);



/** Count the number of jobs which entered the specified state.
 */

int edg_wll_StateRate(
	edg_wll_Context	ctx,
	const edg_wll_QueryRec	*group,
	edg_wll_JobStatCode	major,
	int			minor,
	time_t	*from, 
	time_t	*to,
	float	*rate,
	int	*res_from,
	int	*res_to)

{	
	char	*response = NULL, *send_mess = NULL, *message = NULL;
	float	not_returned;
	
	
	edg_wll_ResetError(ctx);

	edg_wll_StatsRequestToXML(ctx, "Rate", group, major, minor, from, to, &send_mess);
	
	if (set_server_name_and_port(ctx, NULL))
		goto err;

	ctx->p_tmp_timeout = ctx->p_query_timeout;

	if (edg_wll_http_send_recv(ctx, "POST /statsRequest HTTP/1.1",request_headers, send_mess,
			&response,NULL,&message)) 
		goto err;
         
	if (http_check_status(ctx,response))
		goto err;

  	edg_wll_ParseStatsResult(ctx,message, from, to, rate, 
			&not_returned, &not_returned, res_from, res_to);

err:
	free(response);
	free(message);

	return edg_wll_Error(ctx,NULL,NULL);
}



/** Compute average time for which jobs stay in the specified state.
 */

int edg_wll_StateDuration(
	edg_wll_Context	ctx,
	const edg_wll_QueryRec	*group,
	edg_wll_JobStatCode	major,
	int			minor,
	time_t	*from, 
	time_t	*to,
	float	*duration,
	int	*res_from,
	int	*res_to)
{
	char	*response = NULL, *send_mess = NULL, *message = NULL;
	float	not_returned;
	
	
	edg_wll_ResetError(ctx);

	edg_wll_StatsRequestToXML(ctx, "Duration", group, major, minor, from, to, &send_mess);
	
	if (set_server_name_and_port(ctx, NULL))
		goto err;

	ctx->p_tmp_timeout = ctx->p_query_timeout;

	if (edg_wll_http_send_recv(ctx, "POST /statsRequest HTTP/1.1",request_headers, send_mess,
			&response,NULL,&message)) 
		goto err;
         
	if (http_check_status(ctx,response))
		goto err;

  	edg_wll_ParseStatsResult(ctx,message, from, to, &not_returned, 
			duration, &not_returned, res_from, res_to);

err:
	free(response);
	free(message);

	return edg_wll_Error(ctx,NULL,NULL);
}



/** Compute average time for which jobs moves from one to second specified state.
 */

int edg_wll_StateDurationFromTo(
        edg_wll_Context ctx,
        const edg_wll_QueryRec  *group,
        edg_wll_JobStatCode     base,
        edg_wll_JobStatCode     final,
	int	minor,
        time_t  *from,
        time_t  *to,
        float   *duration,
	float   *dispersion,
        int     *res_from,
        int     *res_to
)
{
        char    *response = NULL, *send_mess = NULL, *message = NULL;
        float   not_returned;


        edg_wll_ResetError(ctx);

        edg_wll_StatsDurationFTRequestToXML(ctx, "DurationFromTo", group, base, final, minor, from, to, &send_mess);

        if (set_server_name_and_port(ctx, NULL))
                goto err;

        ctx->p_tmp_timeout = ctx->p_query_timeout;

        if (edg_wll_http_send_recv(ctx, "POST /statsRequest HTTP/1.1",request_headers, send_mess,
                        &response,NULL,&message))
                goto err;

        if (http_check_status(ctx,response))
                goto err;

        edg_wll_ParseStatsResult(ctx,message, from, to, &not_returned,
                        duration, dispersion, res_from, res_to);

err:
        free(response);
        free(message);

        return edg_wll_Error(ctx,NULL,NULL);
}



static int set_server_name_and_port(edg_wll_Context ctx, const edg_wll_QueryRec **job_conditions)
{
	int				i = 0, j,
					found = 0,
					error = 0;
	unsigned int			srvPort = 0,
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
		if (!ctx->p_query_server) 
			return(edg_wll_SetError(ctx, (edg_wll_ErrorCode) EINVAL, "Hostname of server to query is not set"));
		else ctx->srvName = strdup(ctx->p_query_server);
		if (!ctx->p_query_server_port)
			return(edg_wll_SetError(ctx, (edg_wll_ErrorCode) EINVAL, "Port of server to query is not set"));
		else ctx->srvPort = ctx->p_query_server_port;
	}
		
	return(error);
}


