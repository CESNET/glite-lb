#ifndef __EDG_WORKLOAD_LOGGING_COMMON_MINI_HTTP_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_MINI_HTTP_H__

#ident "$Header$"

#include "glite/lb/consumer.h"

/* XXX: not a good place for the folowing #def's but we ain't got better currently */
/** protocol version */
#define PROTO_VERSION           "3.0"
#define PROTO_VERSION_V21      "2.1"
/** backward protocol compatibility     */
/* version separated by comma           */
/* e.g. "1.0,1.2,1.3"                   */
#define COMP_PROTO              "3.0"
#define COMP_PROTO_V21         "2.0,2.1"


/* subset of HTTP codes we return */
#define HTTP_OK			200
#define HTTP_BADREQ		400
#define HTTP_UNAUTH		401
#define HTTP_NOTFOUND		404
#define HTTP_NOTALLOWED		405
#define HTTP_UNSUPPORTED	415 
#define HTTP_INTERNAL		500
#define HTTP_NOTIMPL		501
#define HTTP_UNAVAIL		503
#define HTTP_INVALID		579

extern edg_wll_ErrorCode edg_wll_http_recv(
	edg_wll_Context,	/* INOUT: context */
	char **,	/* OUT: first line */
	char ***,	/* OUT: null terminated array of headers */
	char **		/* OUT: message body */
);

extern edg_wll_ErrorCode edg_wll_http_send(
	edg_wll_Context,	/* INOUT: context */
	const char *,		/* IN: first line */
	char const * const *,	/* IN: headers */
	const char *		/* IN: message body */
);

#endif /* __EDG_WORKLOAD_LOGGING_COMMON_MINI_HTTP_H__ */
