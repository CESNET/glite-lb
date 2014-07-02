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

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdio.h>
#include <assert.h>
#include <signal.h>
#include <stdsoap2.h>

#include "soap_version.h"
#include "glite_gsplugin-int.h"
#include "glite_gsplugin.h"

#ifdef GSPLUGIN_DEBUG
#  define pdprintf(s)	printf s
#else
#  define pdprintf(s)
#endif

typedef struct _int_plugin_data_t {
	glite_gsplugin_Context	ctx;	/**< data used for connection etc. */
	int						def;	/**< is the context created by plugin? */
} int_plugin_data_t;

static const char plugin_id[] = PLUGIN_ID;

static void glite_gsplugin_delete(struct soap *, struct soap_plugin *);
static int glite_gsplugin_copy(struct soap *, struct soap_plugin *, struct soap_plugin *);

static size_t glite_gsplugin_recv(struct soap *, char *, size_t);
static int glite_gsplugin_send(struct soap *, const char *, size_t);
static int glite_gsplugin_connect(struct soap *, const char *, const char *, int);
static int glite_gsplugin_close(struct soap *);
#if GSOAP_VERSION >= 20700
static int glite_gsplugin_poll(struct soap *);
#endif
static int glite_gsplugin_accept(struct soap *, int, struct sockaddr *, int *);
static int glite_gsplugin_posthdr(struct soap *soap, const char *key, const char *val);

static int get_error(int err, edg_wll_GssStatus *gss_stat, const char *msg, char **desc);

int
glite_gsplugin_init_context(glite_gsplugin_Context *ctx)
{
	glite_gsplugin_Context out = (glite_gsplugin_Context) malloc(sizeof(*out));
	if (!out) return ENOMEM;

	memset(out, 0, sizeof(*out));
	out->cred = NULL;

	/* XXX: some troubles with glite_gss and blocking calls!
	out->timeout.tv_sec = 10000;
	 */

	out->timeout = NULL;
	*ctx = out;

	return 0;
}

int
glite_gsplugin_free_context(glite_gsplugin_Context ctx)
{
	if (ctx == NULL)
	   return 0;

	if ( ctx->internal_credentials && ctx->cred != NULL ) 
		edg_wll_gss_release_cred(&ctx->cred, NULL);
	if ( ctx->internal_connection && ctx->connection ) {
		if ( ctx->connection->context != NULL )
			edg_wll_gss_close(ctx->connection, NULL);
		free(ctx->connection);
	}
	if (ctx->error_msg)
	   free(ctx->error_msg);
	free(ctx);

	return 0;
}

glite_gsplugin_Context
glite_gsplugin_get_context(struct soap *soap)
{
	return ((int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id))->ctx;
}

void *
glite_gsplugin_get_udata(struct soap *soap)
{
	int_plugin_data_t *pdata;
   
	pdata = (int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id);
	assert(pdata);
	return pdata->ctx->user_data;
}

void
glite_gsplugin_set_udata(struct soap *soap, void *d)
{
	int_plugin_data_t *pdata;
   
	pdata = (int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id);
	assert(pdata);
	pdata->ctx->user_data = d;
}

void glite_gsplugin_set_timeout(glite_gsplugin_Context ctx, struct timeval const *to)
{
	if (to) {
		ctx->_timeout = *to;
		ctx->timeout = &ctx->_timeout;
	}
	else ctx->timeout = NULL;
}

int
glite_gsplugin_set_credential(glite_gsplugin_Context ctx,
			      const char *cert,
			      const char *key)
{
   edg_wll_GssStatus gss_code;
   edg_wll_GssCred cred = NULL;
   
   int ret;

   ret = edg_wll_gss_acquire_cred_gsi((char *)cert, (char *)key, &cred, &gss_code);
   if (ret)
	return get_error(ret, &gss_code, "failed to load GSI credentials", &ctx->error_msg);

   if (ctx->internal_credentials && ctx->cred != NULL)
      edg_wll_gss_release_cred(&ctx->cred, NULL);

   ctx->cred = cred;
   ctx->internal_credentials = 1;

   return 0;
}

void
glite_gsplugin_use_credential(glite_gsplugin_Context ctx,
				edg_wll_GssCred cred)
{
	ctx->cred = cred;
	ctx->internal_credentials = 0;
}

int
glite_gsplugin_set_connection(glite_gsplugin_Context ctx, edg_wll_GssConnection *conn)
{
	int						ret = SOAP_OK;

	if ( ctx->connection ) {
		if ( ctx->internal_connection && ctx->connection->context != NULL) {
			pdprintf(("GSLITE_GSPLUGIN: closing gss connection\n"));
			ret = edg_wll_gss_close(ctx->connection, ctx->timeout);
			free(ctx->connection);
		}
	}
	ctx->connection = conn;
	ctx->internal_connection = 0;

	return ret;
}

int
glite_gsplugin(struct soap *soap, struct soap_plugin *p, void *arg)
{
	int_plugin_data_t *pdata = (int_plugin_data_t *)malloc(sizeof(int_plugin_data_t)); 

	pdprintf(("GSLITE_GSPLUGIN: initializing gSOAP plugin\n"));
	if ( !pdata ) return ENOMEM;
	if ( arg ) {
		pdprintf(("GSLITE_GSPLUGIN: Context is given\n"));
		pdata->ctx = (glite_gsplugin_Context)arg;
		pdata->def = 0;
	}
	else {
		edg_wll_GssStatus	gss_code;

		pdprintf(("GSLITE_GSPLUGIN: Creating default context\n"));
		if ( glite_gsplugin_init_context((glite_gsplugin_Context*)&(pdata->ctx)) ) {
			free(pdata);
			return ENOMEM;
		}
		if ( edg_wll_gss_acquire_cred(NULL, NULL, GSS_C_ACCEPT, &pdata->ctx->cred, &gss_code) ) {
			/*	XXX: Let user know, that cred. load failed. Somehow...
			 */
			glite_gsplugin_free_context(pdata->ctx);
			return EINVAL;
		}
		pdata->ctx->internal_credentials = 1;
		pdprintf(("GSLITE_GSPLUGIN: server running with certificate: %s\n",
                         pdata->ctx->cred->name));
		pdata->def = 1;
	}

	p->id			= plugin_id;
	p->data			= pdata;
	p->fdelete		= glite_gsplugin_delete;
	p->fcopy		= glite_gsplugin_copy;

	soap->fopen		= glite_gsplugin_connect;
	soap->fconnect		= NULL;
	soap->fclose		= glite_gsplugin_close;
#if GSOAP_VERSION >= 20700
	soap->fpoll		= glite_gsplugin_poll;
#endif
	soap->faccept		= glite_gsplugin_accept;
	soap->fsend			= glite_gsplugin_send;
	soap->frecv			= glite_gsplugin_recv;
	soap->fposthdr		= glite_gsplugin_posthdr;

	return SOAP_OK;
}


char *glite_gsplugin_errdesc(struct soap *soap)
{
	glite_gsplugin_Context	ctx;

	ctx = ((int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id))->ctx;
	if ( ctx ) return ctx->error_msg;

	return NULL;
}



static int
glite_gsplugin_copy(struct soap *soap, struct soap_plugin *dst, struct soap_plugin *src)
{
	pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_copy()\n"));
	/*	Should be the copy code here?
	 */
	return ENOSYS;
}

static void
glite_gsplugin_delete(struct soap *soap, struct soap_plugin *p)
{
	int_plugin_data_t *d = (int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id);

	pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_delete()\n"));
	if ( d->def ) {
		glite_gsplugin_close(soap);
		glite_gsplugin_free_context(d->ctx);
	}
	free(d);
}


static int
glite_gsplugin_connect(
	struct soap *soap,
	const char *endpoint,
	const char *host,
	int port)
{
	glite_gsplugin_Context	ctx;
	edg_wll_GssStatus		gss_stat;
	int						ret;
	const char *msg = NULL;


	pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_connect()\n"));
#if defined(CHECK_GSOAP_VERSION) && GSOAP_VERSION <= 20700
	if (   GSOAP_VERSION < 20700
		|| (GSOAP_VERSION == 20700
			&& (strlen(GSOAP_MIN_VERSION) < 1 || GSOAP_MIN_VERSION[1] < 'e')) ) {
		fprintf(stderr, "Client connect will work only with gSOAP v2.7.0e and later");
		soap->errnum = ENOSYS;
		return ENOSYS;
	}
#endif

	ctx = ((int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id))->ctx;
	if (ctx->connection) {
		pdprintf(("GSLITE_GSPLUGIN: Warning: connection dropped, new to %s:%d\n", host, port));
	}
	pdprintf(("GSLITE_GSPLUGIN: connection to %s:%d will be established\n", host, port));

	if ( ctx->cred == NULL ) {
		pdprintf(("GSLITE_GSPLUGIN: loading default credentials\n"));
		ret = edg_wll_gss_acquire_cred(NULL, NULL, GSS_C_INITIATE,
                	&ctx->cred, &gss_stat);
		if ( ret ) {
			msg = "failed to load GSI credentials";
			goto err;
		}
		ctx->internal_credentials = 1;
	}

	if ( !(ctx->connection = (edg_wll_GssConnection  *)malloc(sizeof(*ctx->connection))) ) return errno;
	ret = edg_wll_gss_connect(ctx->cred,
				host, port,
				ctx->timeout,
				ctx->connection, &gss_stat);
	if ( ret ) {
		free(ctx->connection);
		ctx->connection = NULL;
		msg = "edg_wll_gss_connect() error";
		goto err;
	}
	ctx->internal_connection = 1;

	soap->errnum = 0;
	return ctx->connection->sock;

err:
	pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_connect() error!\n"));

	soap->errnum = get_error(ret, &gss_stat, msg, &ctx->error_msg);

	switch ( ret ) {
	case EDG_WLL_GSS_ERROR_GSS:
 	  	soap_set_sender_error(soap, "SSL error", "SSL authentication failed in tcp_connect(): check password, key file, and ca file.", SOAP_SSL_ERROR);
		break;
	case EDG_WLL_GSS_ERROR_HERRNO:
		soap_set_sender_error(soap, "connection error", hstrerror(soap->errnum), SOAP_CLI_FAULT);
		break;
	case EDG_WLL_GSS_ERROR_EOF: 
		soap->errnum = ECONNREFUSED;
		soap_set_sender_error(soap, "connection error", strerror(soap->errnum), SOAP_CLI_FAULT);
		break;
	case EDG_WLL_GSS_ERROR_ERRNO:
	case EDG_WLL_GSS_ERROR_TIMEOUT: 
		soap_set_sender_error(soap, "connection error", strerror(soap->errnum), SOAP_CLI_FAULT);
		break;
	default:
		soap_set_sender_error(soap, "unknown error", "", SOAP_CLI_FAULT);
		break;
	}

	return SOAP_INVALID_SOCKET;
}

/**	It is called in soap_closesocket() 
 *
 *	return like errno value
 */
static int
glite_gsplugin_close(struct soap *soap)
{
	glite_gsplugin_Context	ctx;

	pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_close()\n"));
	ctx = ((int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id))->ctx;
	return glite_gsplugin_set_connection(ctx, NULL);
}


static int
glite_gsplugin_accept(struct soap *soap, int s, struct sockaddr *a, int *n)
{
	glite_gsplugin_Context	ctx;
	edg_wll_GssStatus		gss_code;
	int						conn;
	int ret;

	soap->errnum = 0;
	pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_accept()\n"));
	ctx = ((int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id))->ctx;
	if ( (conn = accept(s, (struct sockaddr *)&a, (socklen_t *)n)) < 0 ) return conn;
	if ( !ctx->connection ) {
		if ( !(ctx->connection = (edg_wll_GssConnection *)malloc(sizeof(*ctx->connection))) ) {
			close(conn);
			soap_set_receiver_error(soap, "malloc error", strerror(ENOMEM), ENOMEM);
			return SOAP_INVALID_SOCKET;
		}
		ctx->internal_connection = 1;
	}
	if ( (ret = edg_wll_gss_accept(ctx->cred, conn, ctx->timeout, ctx->connection, &gss_code)) != 0) {
		pdprintf(("GSLITE_GSPLUGIN: Client authentication failed, closing.\n"));
		close(conn);
		get_error(ret, &gss_code, "Client authentication failed", &ctx->error_msg);
		soap->errnum = SOAP_SVR_FAULT;
		soap_set_receiver_error(soap, "SSL error", "SSL authentication failed in tcp_connect(): check password, key file, and ca file.", SOAP_SSL_ERROR);
		return SOAP_INVALID_SOCKET;
	}

	return conn;
}

static size_t
glite_gsplugin_recv(struct soap *soap, char *buf, size_t bufsz)
{
	glite_gsplugin_Context	ctx;
	edg_wll_GssStatus		gss_code;
	int						len;


	ctx = ((int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id))->ctx;
	if ( ctx->error_msg ) { free(ctx->error_msg); ctx->error_msg = NULL; }

	if ( ctx->connection->context == NULL ) {
		soap->errnum = ENOTCONN;
		/* XXX: glite_gsplugin_send() returns SOAP_EOF on errors
		 */
		return 0;
	}
	
	len = edg_wll_gss_read(ctx->connection, buf, bufsz,
					ctx->timeout,
					&gss_code);

	switch ( len ) {
	case EDG_WLL_GSS_OK:
		break;
	case EDG_WLL_GSS_ERROR_GSS:
	case EDG_WLL_GSS_ERROR_ERRNO:
	case EDG_WLL_GSS_ERROR_TIMEOUT:
	case EDG_WLL_GSS_ERROR_EOF:
		soap->errnum = get_error(len, &gss_code, "receiving WS request", &ctx->error_msg);
		return 0;
	/* default: fallthrough */
	}
	pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_recv() = %d\n",len));

	return len;
}

static int
glite_gsplugin_send(struct soap *soap, const char *buf, size_t bufsz)
{
	glite_gsplugin_Context	ctx;
	edg_wll_GssStatus		gss_code;
	struct sigaction		sa, osa;
	size_t					total = 0;
	int						ret;


	ctx = ((int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id))->ctx;
	/* XXX: check whether ctx is initialized
	 *      i.e. ctx->connection != NULL
	 */
	if ( ctx->error_msg ) { free(ctx->error_msg); ctx->error_msg = NULL; }
	if ( ctx->connection == NULL || ctx->connection->context == NULL ) {
		soap->errnum = ENOTCONN;
		return SOAP_EOF;
	}

	memset(&sa, 0, sizeof(sa));
	assert(sa.sa_handler == NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, &osa);

	ret = edg_wll_gss_write_full(ctx->connection,
				(void*)buf, bufsz, ctx->timeout, &total, &gss_code);

	sigaction(SIGPIPE, &osa, NULL);

	pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_send(%d) = %d\n",bufsz,ret));

	soap->errnum = get_error(ret, &gss_code, "sending WS request", &ctx->error_msg);

	switch ( ret ) {
	case EDG_WLL_GSS_OK:
		return SOAP_OK;

	case EDG_WLL_GSS_ERROR_ERRNO:
		if (soap->errnum == EPIPE) soap->errnum = ENOTCONN;
		return SOAP_EOF;

	case EDG_WLL_GSS_ERROR_TIMEOUT:
	case EDG_WLL_GSS_ERROR_GSS:
	case EDG_WLL_GSS_ERROR_EOF:
	default:
		return SOAP_EOF;
	}
}


#if GSOAP_VERSION >= 20700
static int
glite_gsplugin_poll(struct soap *soap)
{
	glite_gsplugin_Context	ctx;
	fd_set readfds;
	struct timeval timeout = {tv_sec: 0, tv_usec: 0};
	int ret;

	ctx = ((int_plugin_data_t *)soap_lookup_plugin(soap, plugin_id))->ctx;
	if (!ctx->connection || !ctx->connection->context) {
		pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_poll, no connection\n"));
		return SOAP_EOF;
	}

	FD_ZERO(&readfds);
	FD_SET(ctx->connection->sock, &readfds);
	ret = select(ctx->connection->sock + 1, &readfds, NULL, NULL, &timeout);
	if (ret < 0) {
		pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_poll, select(%d) failed\n", ctx->connection->sock));
		return SOAP_TCP_ERROR;
	} else if (ret == 0) {
		/* has been OK but server can disconnect anytime */
		return SOAP_OK;
	} else {
		pdprintf(("GSLITE_GSPLUGIN: glite_gsplugin_poll, select(%d) = %d\n", ctx->connection->sock, ret));
		return SOAP_EOF;
	}
}
#endif


static int http_send_header(struct soap *soap, const char *s) {
	const char *t;

	do {
		t = strchr(s, '\n'); /* disallow \n in HTTP headers */
		if (!t) t = s + strlen(s);
		if (soap_send_raw(soap, s, t - s)) return soap->error;
		s = t + 1;
	} while (*t);

	return SOAP_OK;
}


static int glite_gsplugin_posthdr(struct soap *soap, const char *key, const char *val) {
	if (key) {
		if (strcmp(key, "Status") == 0) {
			snprintf(soap->tmpbuf, sizeof(soap->tmpbuf), "HTTP/%s", soap->http_version);
			if (http_send_header(soap, soap->tmpbuf)) return soap->error;
			if (val && (soap_send_raw(soap, " ", 1) || http_send_header(soap, val)))
				return soap->error;
		} else {
			if (http_send_header(soap, key)) return soap->error;
			if (val && (soap_send_raw(soap, ": ", 2) || http_send_header(soap, val)))
				return soap->error;
		}
	}

	return soap_send_raw(soap, "\r\n", 2);
}


static int get_error(int err, edg_wll_GssStatus *gss_stat, const char *msg, char **desc) {
	int num;
	const char *s;

	if (desc) {
		free(*desc);
		*desc = NULL;
	}

	switch (err) {
	case EDG_WLL_GSS_OK:
		return 0;
	case EDG_WLL_GSS_ERROR_GSS:
		if (desc) edg_wll_gss_get_error(gss_stat, msg, desc);
		return ENOTCONN;
	case EDG_WLL_GSS_ERROR_HERRNO:
		num = h_errno;
		s = hstrerror(num);
		if (desc) {
			if (s) {
				if (asprintf(desc, "%s: %s",  msg, s) == -1) *desc = NULL;
			} else {
				if (asprintf(desc, "%s: herrno %d", msg, num) == -1) *desc = NULL;
			}
		}
		return num;
	case EDG_WLL_GSS_ERROR_ERRNO:
		num = errno;
		break;
	case EDG_WLL_GSS_ERROR_TIMEOUT:
		num = ETIMEDOUT;
		break;
	case EDG_WLL_GSS_ERROR_EOF:
		num = ENOTCONN;
		break;
	default:
		if (desc) {
			if (asprintf(desc, "%s: unknown error type %d from glite_gss", msg, err) == -1)
				*desc = NULL;
		}
		return EINVAL;
	}

	if (desc) {
		if (asprintf(desc, "%s: %s", msg, strerror(num)) == -1)
			*desc = NULL;
	}
	return num;
}
