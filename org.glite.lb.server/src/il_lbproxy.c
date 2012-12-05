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


#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "glite/lb/context-int.h"
#include "glite/lb/log_proto.h"
#include "glite/lbu/log.h"

#include "il_lbproxy.h"

#define FCNTL_ATTEMPTS		5
#define FCNTL_TIMEOUT		1

char *lbproxy_ilog_socket_path = EDG_WLL_PROXY_SOCKET_DEFAULT;
char *lbproxy_ilog_file_prefix = EDG_WLL_PROXY_PREFIX_DEFAULT;


int
edg_wll_EventSendProxy(
	edg_wll_Context			ctx,
	glite_jobid_const_t		jobid,
	const char			   *event)
{
	long			filepos;
	char		   *jobid_s,
				   *event_file = NULL;
	int				err = 0;

#define _err(n)		{ err = n; goto out; }

	edg_wll_ResetError(ctx);

	jobid_s = edg_wlc_JobIdGetUnique(jobid);
	if ( !jobid_s ) {
		edg_wll_SetError(ctx, ENOMEM, "edg_wlc_JobIdGetUnique()");
		_err(1);
	}

	asprintf(&event_file, "%s.%s", lbproxy_ilog_file_prefix, jobid_s);
	if ( !event_file ) {
		edg_wll_SetError(ctx, ENOMEM, "asprintf()");
		_err(1);
	}

	if ( edg_wll_log_event_write(ctx, event_file, event,
					(ctx->p_tmp_timeout.tv_sec > FCNTL_ATTEMPTS ?
						ctx->p_tmp_timeout.tv_sec : FCNTL_ATTEMPTS),
					FCNTL_TIMEOUT, &filepos) ) {

		edg_wll_UpdateError(ctx, 0, "edg_wll_log_event_write()");
		_err(1);
	}

	if ( edg_wll_log_event_send(ctx, lbproxy_ilog_socket_path, filepos,
						event, strlen(event), 1, &ctx->p_tmp_timeout) ) {
		char *errt, *errd;
		errt = errd = NULL;
	
		edg_wll_UpdateError(ctx, EDG_WLL_IL_PROTO, "edg_wll_log_event_send()");
		edg_wll_Error(ctx, &errt, &errd);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_ERROR, 
			"%s (%s)", errt, errd);
		free(errt); free(errd);
		_err(-1);
	}

out:
	if ( jobid_s ) free(jobid_s);
	if ( event_file ) free(event_file);

	if ( !err ) return 0;
	if ( err < 0 ) {
		/* do not propagate IL errors */
		edg_wll_ResetError(ctx);
		return 0;
	} else {
		edg_wll_UpdateError(ctx, 0, "edg_wll_EventSendProxy()");
		return edg_wll_Error(ctx, NULL, NULL);
	}
}
