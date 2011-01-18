#ifndef GLITE_LB_MINI_HTTP_H
#define GLITE_LB_MINI_HTTP_H

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


// #include "glite/lb/consumer.h"
#ifdef BUILDING_LB_COMMON
#include "context.h"
#include "connpool.h"
#else
#include "glite/lb/context.h"
#include "glite/lb/connpool.h"
#endif

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
#define HTTP_ACCEPTED		202
#define HTTP_BADREQ		400
#define HTTP_UNAUTH		401
#define HTTP_NOTFOUND		404
#define HTTP_NOTALLOWED		405
#define HTTP_GONE		410
#define HTTP_UNSUPPORTED	415 
#define HTTP_INTERNAL		500
#define HTTP_NOTIMPL		501
#define HTTP_UNAVAIL		503
#define HTTP_INVALID		579

extern edg_wll_ErrorCode edg_wll_http_recv(
	edg_wll_Context,	/* INOUT: context */
	char **,		/* OUT: first line */
	char ***,		/* OUT: null terminated array of headers */
	char **,		/* OUT: message body */
        edg_wll_ConnPool *connPTR /* IN: Pointer to the connection to use */
);

extern edg_wll_ErrorCode edg_wll_http_send(
	edg_wll_Context,	/* INOUT: context */
	const char *,		/* IN: first line */
	char const * const *,	/* IN: headers */
	const char *,		/* IN: message body */
        edg_wll_ConnPool *connPTR /* IN: Pointer to the connection to use */
);

extern edg_wll_ErrorCode edg_wll_http_recv_proxy(
	edg_wll_Context,	/* INOUT: context */
	char **,	/* OUT: first line */
	char ***,	/* OUT: null terminated array of headers */
	char **		/* OUT: message body */
);

extern edg_wll_ErrorCode edg_wll_http_send_proxy(
	edg_wll_Context,	/* INOUT: context */
	const char *,		/* IN: first line */
	char const * const *,	/* IN: headers */
	const char *		/* IN: message body */
);

#endif /* GLITE_LB_MINI_HTTP_H */
