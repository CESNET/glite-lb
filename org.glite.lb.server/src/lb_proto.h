#ifndef GLITE_LB_PROTO_H
#define GLITE_LB_PROTO_H

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
	char **,	/* OUT: HTTP response body */
	int *		/* OUT: HTTP error code */
);

typedef struct _notifInfo{
        char *notifid;
        char *destination;
        char *valid;
	edg_wll_QueryRec **conditions;
	char *conditions_text;
	int  flags;
	char *owner;
	char *jobid;
} notifInfo;

typedef enum _http_admin_option{
	HTTP_ADMIN_OPTION_MY,
	HTTP_ADMIN_OPTION_ALL,
	HTTP_ADMIN_OPTION_FOREIGN,
	HTTP_ADMIN_OPTION_LAST
} http_admin_option;

typedef enum _http_extra_option{
	HTTP_EXTRA_OPTION_NONE,
	HTTP_EXTRA_OPTION_WSDL,
	HTTP_EXTRA_OPTION_TYPES,
	HTTP_EXTRA_OPTION_AGU,
	HTTP_EXTRA_OPTION_VERSION,
	HTTP_EXTRA_OPTION_CONFIGURATION,
	HTTP_EXTRA_OPTION_STATS,
	HTTP_EXTRA_OPTION_QUERY,
        HTTP_EXTRA_OPTION_LAST
} http_extra_option;

extern char *edg_wll_HTTPErrorMessage(int);

int edg_wll_ParseQueryConditions(edg_wll_Context ctx, const char *query, edg_wll_QueryRec ***conditions);

extern int edg_wll_UserJobsServer(edg_wll_Context ctx, int flags, edg_wlc_JobId  **jobs, edg_wll_JobStat **states);

extern int edg_wll_QuerySequenceCodeServer(edg_wll_Context ctx, edg_wlc_JobId jobid, const char *source, char **seqcode);

#endif /* GLITE_LB_PROTO_H */
