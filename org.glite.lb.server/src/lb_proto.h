#ifndef __GLITE_LB_PROTO_H__
#define __GLITE_LB_PROTO_H__

#ident "$Header$"

#include "glite/lb/context.h"

extern const char* const response_headers[];

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

extern char *edg_wll_HTTPErrorMessage(int);

#endif /* __GLITE_LB_PROTO_H__ */
