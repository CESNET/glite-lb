#include "glite/lb/context-int.h"
#include "glite/lb/log_proto.h"

#include "il_lbproxy.h"

#define FCNTL_ATTEMPTS		5
#define FCNTL_TIMEOUT		1
#define FILE_PREFIX			"/tmp/dglogd.log"
#define DEFAULT_SOCKET		"/tmp/interlogger.sock"

char *lbproxy_ilog_socket_path = DEFAULT_SOCKET;
char *lbproxy_ilog_file_prefix = FILE_PREFIX;


int
edg_wll_EventSendProxy(
	edg_wll_Context			ctx,
	const edg_wlc_JobId		jobid,
	const char			   *event)
{
	struct timeval	timeout;
	long			filepos;
	char		   *jobid_s,
				   *event_file = NULL;


	edg_wll_ResetError(ctx);

	timeout.tv_sec = EDG_WLL_LOG_TIMEOUT_MAX;
	timeout.tv_usec = 0;	

	jobid_s = edg_wlc_JobIdGetUnique(jobid);
	if ( !jobid_s ) {
		edg_wll_SetError(ctx, ENOMEM, "edg_wlc_JobIdGetUnique()");
		goto out;
	}

	asprintf(&event_file, "%s.%s", lbproxy_ilog_file_prefix, jobid_s);
	if ( !event_file ) {
		edg_wll_SetError(ctx, ENOMEM, "asprintf()");
		goto out;
	}

	if ( edg_wll_log_event_write(ctx, event_file, event,
						FCNTL_ATTEMPTS, FCNTL_TIMEOUT, &filepos) ) {
		edg_wll_UpdateError(ctx, 0, "edg_wll_log_event_write()");
		goto out;
	}

	if ( edg_wll_log_event_send(ctx, lbproxy_ilog_socket_path, filepos,
						event, strlen(event), 1, &timeout) ) {
		edg_wll_UpdateError(ctx, 0, "edg_wll_log_event_send()");
		goto out;
	}

out:
	if ( jobid_s ) free(jobid_s);
	if ( event_file ) free(event_file);

	return  edg_wll_Error(ctx, NULL, NULL)?  edg_wll_UpdateError(ctx, 0, "edg_wll_EventSendProxy()"): 0;
}
