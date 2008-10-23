#ifndef GLITE_LB_PROTO_H
#define GLITE_LB_PROTO_H

#ident "$Header$"

#include "glite/lb/context.h"
#include "glite/lb/query_rec.h"

/* Handle a single request of the LB server protocol 
 * returns a complete response string (may contain a formatted error
 * message)
 * or NULL on fatal error*/

extern edg_wll_ErrorCode edg_wll_Proto(
	edg_wll_Context,	/* INOUT: context */
	char *,		/* IN: HTTP request line */
	char **,	/* IN: HTTP message headers */
	char *,		/* IN: HTTP message body */
	char **, 	/* OUT: HTTP response line */
	char ***,	/* OUT: HTTP response headers */
	char **		/* OUT: HTTP response body */
);

typedef struct _notifInfo{
        char *notifid;
        char *destination;
        char *valid;
	edg_wll_QueryRec **conditions;
	char *conditions_text;
	int  flags;
} notifInfo;

extern char *edg_wll_HTTPErrorMessage(int);

extern int edg_wll_UserJobsServer(edg_wll_Context ctx, edg_wlc_JobId  **jobs, edg_wll_JobStat **states);

extern int edg_wll_QuerySequenceCodeServer(edg_wll_Context ctx, edg_wlc_JobId jobid, const char *source, char **seqcode);

#endif /* GLITE_LB_PROTO_H */
