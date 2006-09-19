#ident "$Header$"

#include "glite/lb/context-int.h"
#include "glite/lb/log_proto.h"

#include "il_lbproxy.h"

#define FCNTL_ATTEMPTS		5
#define FCNTL_TIMEOUT		1
#define FILE_PREFIX		EDG_WLL_LOG_PREFIX_DEFAULT
#define DEFAULT_SOCKET		"/tmp/interlogger.sock"

char *lbproxy_ilog_socket_path = DEFAULT_SOCKET;
char *lbproxy_ilog_file_prefix = FILE_PREFIX;


int
edg_wll_EventSendProxy(
	edg_wll_Context			ctx,
	const edg_wlc_JobId		jobid,
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
		edg_wll_UpdateError(ctx, 0, "edg_wll_log_event_send()");
		_err(-1);
	}

out:
	if ( jobid_s ) free(jobid_s);
	if ( event_file ) free(event_file);

	if ( !err ) return 0;
	edg_wll_UpdateError(ctx, 0, "edg_wll_EventSendProxy()");
	if ( err < 0 ) return 0;
	return edg_wll_Error(ctx, NULL, NULL);
}
