/* /cvs/jra1mw/org.glite.lb.server/src/ws_plugin.c,v 1.3 2004/11/29 19:12:51 zsalvet */

#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <stdsoap2.h>

#include "lb_gss.h"

#ifdef PLUGIN_TEST
extern int edg_wll_open(edg_wll_Context);
#endif

#include "ws_plugin.h"

#ifdef WS_PLUGIN_DEBUG
#  define pdprintf(s)	printf s
#else
#  define pdprintf(s)
#endif

static const char plugin_id[] = PLUGIN_ID;

#ifdef PLUGIN_TEST
static int edg_wll_ws_connect(struct soap *, const char *, const char *, int);
#endif
static void edg_wll_ws_delete(struct soap *, struct soap_plugin *);
static size_t edg_wll_ws_recv(struct soap *, char *, size_t);
static int edg_wll_ws_send(struct soap *, const char *, size_t);


int edg_wll_ws_plugin(struct soap *soap, struct soap_plugin *p, void *arg)
{
	/*	The parametr (edg_wll_Context) must be given!  */
	assert(arg != NULL);

	p->id			= plugin_id;
	p->data			= arg;
	p->fdelete		= edg_wll_ws_delete;

#ifdef PLUGIN_TEST
	soap->fconnect	= edg_wll_ws_connect;
#endif
	soap->fsend		= edg_wll_ws_send;
	soap->frecv		= edg_wll_ws_recv;

	/* XXX globus_module_activate(GLOBUS_COMMON_MODULE); */

	return SOAP_OK;
}

#ifdef PLUGIN_TEST
int edg_wll_ws_connect(struct soap *soap, const char *endpoint,
	const char *host, int port)
{
	edg_wll_Context		ctx = (edg_wll_Context)soap_lookup_plugin(soap, plugin_id);


	ctx->srvName = strdup(host);
	ctx->srvPort = port;
	ctx->p_tmp_timeout = ctx->p_query_timeout;
	if ( edg_wll_open(ctx) )
		return edg_wll_Error(ctx, NULL, NULL);

	soap->socket = 2;

	return SOAP_OK;
}
#endif

static void edg_wll_ws_delete(struct soap *soap, struct soap_plugin *p)
{
	/*
	 *	Keep silly gSOAP happy
	 */
}


size_t edg_wll_ws_recv(struct soap *soap, char *buf, size_t bufsz)
{
	edg_wll_ws_Context	ctx = (edg_wll_ws_Context)soap_lookup_plugin(soap, plugin_id);
	edg_wll_GssStatus	gss_code;
	int					len;


	if (ctx->error_msg)
		free(ctx->error_msg);
	ctx->error_msg = NULL;

	if ( ctx->connection->context == GSS_C_NO_CONTEXT )
	{
		soap->errnum = ENOTCONN;
		/* XXX edg_wll_ws_send() returns SOAP_EOF on errors */
		return 0;
	}
	
	len = edg_wll_gss_read(ctx->connection, buf, bufsz, ctx->timeout, &gss_code);

	switch ( len )
	{
	case EDG_WLL_GSS_OK:
		break;

	case EDG_WLL_GSS_ERROR_GSS:
		edg_wll_gss_get_error(&gss_code, "receving WS request",
		      		      &ctx->error_msg);
		soap->errnum = ENOTCONN;
		return 0;

	case EDG_WLL_GSS_ERROR_ERRNO:
		ctx->error_msg = strdup("edg_wll_gss_read()");
		soap->errnum = errno;
		return 0;

	case EDG_WLL_GSS_ERROR_TIMEOUT:
		soap->errnum = ETIMEDOUT;
		return 0;

	case EDG_WLL_GSS_ERROR_EOF:
		soap->errnum = ENOTCONN;
		return 0;

		/* default: fallthrough */
	}

	pdprintf(("\nWS received:\n%s\n\n", buf));
	return len;
}

static int edg_wll_ws_send(struct soap *soap, const char *buf, size_t bufsz)
{
	edg_wll_ws_Context	ctx = (edg_wll_ws_Context) soap_lookup_plugin(soap, plugin_id);
	edg_wll_GssStatus	gss_code;
	struct sigaction	sa, osa;
	size_t			total = 0;
	int			ret;

	if ( ctx->connection->context == GSS_C_NO_CONTEXT )
	{
		soap->errnum = ENOTCONN;
		return SOAP_EOF;
	}

	memset(&sa, 0, sizeof(sa));
	assert(sa.sa_handler == NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, &osa);

	ret = edg_wll_gss_write_full(ctx->connection,
				(void*)buf, bufsz,
				ctx->timeout,
				&total, &gss_code);

	sigaction(SIGPIPE, &osa, NULL);

	switch ( ret )
	{
	case EDG_WLL_GSS_OK:
		pdprintf(("\nWS sent:\n%s\n\n", buf));
		break;

	case EDG_WLL_GSS_ERROR_TIMEOUT:
		ctx->error_msg = strdup("edg_wll_ws_send()");
		soap->errnum = ETIMEDOUT;
		return SOAP_EOF;

	case EDG_WLL_GSS_ERROR_ERRNO:
		if ( errno == EPIPE )
		{
			ctx->error_msg = strdup("edg_wll_ws_send()");
			soap->errnum = ENOTCONN;
		}
		else
		{
			ctx->error_msg = strdup("edg_wll_ws_send()");
			soap->errnum = errno;
		}
		return SOAP_EOF;

	case EDG_WLL_GSS_ERROR_GSS:
	case EDG_WLL_GSS_ERROR_EOF:
	default:
		ctx->error_msg = strdup("edg_wll_ws_send()");
		soap->errnum = ENOTCONN;
		return SOAP_EOF;
	}

	return SOAP_OK;
}
