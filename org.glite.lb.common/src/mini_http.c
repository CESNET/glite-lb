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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <syslog.h>

#include "glite/security/glite_gss.h"

#include "mini_http.h"
#include "lb_plain_io.h"
#include "context-int.h"

#define min(x,y)	((x) < (y) ? (x) : (y))
#define CONTENT_LENGTH	"Content-Length:"

edg_wll_ErrorCode edg_wll_http_recv(edg_wll_Context ctx,char **firstOut,char ***hdrOut,char **bodyOut, edg_wll_ConnPool *connPTR)
{
	char	**hdr = NULL,*first = NULL,*body = NULL;
	enum	{ FIRST, HEAD, BODY, DONE }	pstat = FIRST;
	int	len, nhdr = 0,rdmore = 0,clen = 0,blen = 0;
	int	sock;
	edg_wll_GssStatus gss_code;

#define bshift(shift) {\
	memmove(connPTR->buf,connPTR->buf+(shift),connPTR->bufUse-(shift));\
	connPTR->bufUse -= (shift);\
}
	edg_wll_ResetError(ctx);

	if (connPTR->gss.context != NULL)
   		sock = connPTR->gss.sock;
	else {
		edg_wll_SetError(ctx,ENOTCONN,NULL);
		goto error;
	}

	if (!connPTR->buf) connPTR->buf = malloc(connPTR->bufSize = BUFSIZ);

	do {
		len = edg_wll_gss_read(&connPTR->gss,
			connPTR->buf+connPTR->bufUse,connPTR->bufSize-connPTR->bufUse,&ctx->p_tmp_timeout, &gss_code);

		switch (len) {
			case EDG_WLL_GSS_OK:
				break;
			case EDG_WLL_GSS_ERROR_GSS:
				edg_wll_SetErrorGss(ctx, "receving HTTP request/response", &gss_code);
				goto error;
			case EDG_WLL_GSS_ERROR_ERRNO:
	      			if (errno == ECONNRESET) errno = ENOTCONN;
				edg_wll_SetError(ctx,errno,"edg_wll_gss_read()");
				goto error;
			case EDG_WLL_GSS_ERROR_TIMEOUT:
				edg_wll_SetError(ctx,ETIMEDOUT,NULL);
				goto error; 
			case EDG_WLL_GSS_ERROR_EOF:
				edg_wll_SetError(ctx,ENOTCONN,NULL);
				goto error; 
			/* default: fallthrough */
		}


		connPTR->bufUse += len;
		rdmore = 0;

		if (connPTR->bufUse >= connPTR->bufSize) {
			edg_wll_SetError(ctx,EINVAL,"HTTP Request too long");
			free(connPTR->buf); connPTR->buf = NULL;
			connPTR->bufUse = 0;
			connPTR->bufSize = 0;
			goto error; 
		}

		while (!rdmore && pstat != DONE) switch (pstat) {
			char	*cr; 

			case FIRST:
				if ((cr = memchr(connPTR->buf,'\r',connPTR->bufUse)) &&
					connPTR->bufUse >= cr-connPTR->buf+2 && cr[1] == '\n')
				{
					*cr = 0;
					first = strdup(connPTR->buf);
					bshift(cr-connPTR->buf+2);
					pstat = HEAD;
				} else rdmore = 1;
				break;
			case HEAD:
				if ((cr = memchr(connPTR->buf,'\r',connPTR->bufUse)) &&
					connPTR->bufUse >= cr-connPTR->buf+2 && cr[1] == '\n')
				{
					if (cr == connPTR->buf) {
						bshift(2);
						pstat = clen ? BODY : DONE;
						if (clen) body = malloc(clen+1);
						break;
					}

					*cr = 0;
					hdr = realloc(hdr,(nhdr+2) * sizeof(*hdr));
					hdr[nhdr] = strdup(connPTR->buf);
					hdr[++nhdr] = NULL;

					if (!strncasecmp(connPTR->buf,CONTENT_LENGTH,sizeof(CONTENT_LENGTH)-1))
						clen = atoi(connPTR->buf+sizeof(CONTENT_LENGTH)-1);
	
					bshift(cr-connPTR->buf+2);
				} else rdmore = 1;
				break;
			case BODY:
				if (connPTR->bufUse) {
					int	m = min(connPTR->bufUse,clen-blen);
					memcpy(body+blen,connPTR->buf,m);
					blen += m;
					bshift(m);
				}
				rdmore = 1;
				if (blen == clen) {
					pstat = DONE;
					body[blen] = 0;
				}
				break;
			default:
				break;
		}
	} while (pstat != DONE);

error:
	if (edg_wll_Error(ctx,NULL,NULL)) {
		if (hdr) {
			char 	**h;
			for (h = hdr; *h; h++) free(*h);
			free(hdr);
		}
		free(first);
		free(body);
	} else {
		if (firstOut) *firstOut = first; else free(first);
		if (hdrOut) *hdrOut = hdr; 
		else if (hdr) {
			char 	**h;
			for (h = hdr; *h; h++) free(*h);
			free(hdr);
		}
		if (bodyOut) *bodyOut = body; else free(body);
	}

	return edg_wll_Error(ctx,NULL,NULL);

#undef bshift
}

edg_wll_ErrorCode edg_wll_http_recv_proxy(edg_wll_Context ctx,char **firstOut,char ***hdrOut,char **bodyOut)
{
	char	**hdr = NULL,*first = NULL,*body = NULL;
	enum	{ FIRST, HEAD, BODY, DONE }	pstat = FIRST;
	int		len, nhdr = 0,rdmore = 0,clen = 0,blen = 0;

#define bshift(shift) {\
	memmove(ctx->connProxy->buf,\
			ctx->connProxy->buf+(shift),\
			ctx->connProxy->bufUse-(shift));\
	ctx->connProxy->bufUse -= (shift);\
}
	edg_wll_ResetError(ctx);

	if ( !ctx->connProxy->buf ) {
		ctx->connProxy->bufSize = BUFSIZ;
		ctx->connProxy->bufUse = 0;
		ctx->connProxy->buf = malloc(ctx->connProxy->bufSize);
	}

	do {
		len = edg_wll_plain_read(&ctx->connProxy->conn,
				ctx->connProxy->buf+ctx->connProxy->bufUse,
				ctx->connProxy->bufSize-ctx->connProxy->bufUse,
				&ctx->p_tmp_timeout);
		if ( len < 0 ) {
			edg_wll_SetError(ctx, errno, "edg_wll_plain_read()");
			goto error;
		}

		ctx->connProxy->bufUse += len;
		rdmore = 0;

		while (!rdmore && pstat != DONE) switch (pstat) {
			char	*cr; 

			case FIRST:
				if ((cr = memchr(ctx->connProxy->buf,'\r',ctx->connProxy->bufUse)) &&
					ctx->connProxy->bufUse >= cr-ctx->connProxy->buf+2 && cr[1] == '\n')
				{
					*cr = 0;
					first = strdup(ctx->connProxy->buf);
					bshift(cr-ctx->connProxy->buf+2);
					pstat = HEAD;
				} else rdmore = 1;
				break;
			case HEAD:
				if ((cr = memchr(ctx->connProxy->buf,'\r',ctx->connProxy->bufUse)) &&
					ctx->connProxy->bufUse >= cr-ctx->connProxy->buf+2 && cr[1] == '\n')
				{
					if (cr == ctx->connProxy->buf) {
						bshift(2);
						pstat = clen ? BODY : DONE;
						if (clen) body = malloc(clen+1);
						break;
					}

					*cr = 0;
					hdr = realloc(hdr,(nhdr+2) * sizeof(*hdr));
					hdr[nhdr] = strdup(ctx->connProxy->buf);
					hdr[++nhdr] = NULL;

					if (!strncasecmp(ctx->connProxy->buf,CONTENT_LENGTH,sizeof(CONTENT_LENGTH)-1))
						clen = atoi(ctx->connProxy->buf+sizeof(CONTENT_LENGTH)-1);
	
					bshift(cr-ctx->connProxy->buf+2);
				} else rdmore = 1;
				break;
			case BODY:
				if (ctx->connProxy->bufUse) {
					int	m = min(ctx->connProxy->bufUse,clen-blen);
					memcpy(body+blen,ctx->connProxy->buf,m);
					blen += m;
					bshift(m);
				}
				rdmore = 1;
				if (blen == clen) {
					pstat = DONE;
					body[blen] = 0;
				}
				break;
			default:
				break;
		}
	} while (pstat != DONE);

error:
	if (edg_wll_Error(ctx,NULL,NULL)) {
		if (hdr) {
			char 	**h;
			for (h = hdr; *h; h++) free(*h);
			free(hdr);
		}
		free(first);
		free(body);
	} else {
		if (firstOut) *firstOut = first; else free(first);
		if (hdrOut) *hdrOut = hdr; 
		else if (hdr) {
			char 	**h;
			for (h = hdr; *h; h++) free(*h);
			free(hdr);
		}
		if (bodyOut) *bodyOut = body; else free(body);
	}

	return edg_wll_Error(ctx,NULL,NULL);

#undef bshift
}

static int real_write(edg_wll_Context ctx, edg_wll_GssConnection *con,const char *data,int len)
{
	size_t	total = 0;
	struct sigaction	sa,osa;
	edg_wll_GssStatus	gss_code;
	int	ret;

	memset(&sa,0,sizeof(sa)); assert(sa.sa_handler == NULL);
	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE,&sa,&osa);

	ret = edg_wll_gss_write_full(con, (void*)data, len, &ctx->p_tmp_timeout,
	                             &total, &gss_code);
	sigaction(SIGPIPE,&osa,NULL);

	switch(ret) {
	   case EDG_WLL_GSS_OK:
	      return 0;
	   case EDG_WLL_GSS_ERROR_EOF:
	      errno = ENOTCONN;
	      return -1;
	   case EDG_WLL_GSS_ERROR_TIMEOUT:
	      errno = ETIMEDOUT;
	      return -1;
	   case EDG_WLL_GSS_ERROR_ERRNO:
	      if (errno == EPIPE) errno = ENOTCONN;
	      return -1;
	   case EDG_WLL_GSS_ERROR_GSS:
	      errno = EDG_WLL_ERROR_GSS;
	      return -1;
	   default:
	      /* XXX DK: */
	      errno = ENOTCONN;
	      return -1;
	}
}

edg_wll_ErrorCode edg_wll_http_send(edg_wll_Context ctx,const char *first,const char * const *head,const char *body, edg_wll_ConnPool *connPTR)
{
	const char* const *h;
	int	len = 0, blen;

	edg_wll_ResetError(ctx);

	if (connPTR->gss.context == NULL)
	   return edg_wll_SetError(ctx,ENOTCONN,NULL);

	if (real_write(ctx,&connPTR->gss,first,strlen(first)) < 0 ||
		real_write(ctx,&connPTR->gss,"\r\n",2) < 0) 
		return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");

	if (head) for (h=head; *h; h++) 
		if (real_write(ctx,&connPTR->gss,*h,strlen(*h)) < 0 ||
			real_write(ctx,&connPTR->gss,"\r\n",2) < 0) 
			return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");

	if (body) {
		char	buf[100];

		len = strlen(body);
		blen = sprintf(buf,CONTENT_LENGTH " %d\r\n",len);
		if (real_write(ctx,&connPTR->gss,buf,blen) < 0) 
			return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");
	}

	if (real_write(ctx,&connPTR->gss,"\r\n",2) < 0) 
		return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");
	if (body && real_write(ctx,&connPTR->gss,body,len) < 0)  
		return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");

	return edg_wll_Error(ctx,NULL,NULL);
}

edg_wll_ErrorCode edg_wll_http_send_proxy(edg_wll_Context ctx, const char *first, const char * const *head, const char *body)
{
	const char* const *h;
	int	len = 0, blen;


	edg_wll_ResetError(ctx);

	if (   edg_wll_plain_write_full(&ctx->connProxy->conn,
							first, strlen(first), &ctx->p_tmp_timeout) < 0
		|| edg_wll_plain_write_full(&ctx->connProxy->conn,
							"\r\n", 2, &ctx->p_tmp_timeout) < 0 ) 
		return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");

	if ( head ) for ( h = head; *h; h++ )
		if (   edg_wll_plain_write_full(&ctx->connProxy->conn,
							*h, strlen(*h), &ctx->p_tmp_timeout) < 0
			|| edg_wll_plain_write_full(&ctx->connProxy->conn,
							"\r\n", 2, &ctx->p_tmp_timeout) < 0 )
			return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");

	if ( body ) {
		char	buf[100];

		len = strlen(body);
		blen = sprintf(buf, CONTENT_LENGTH " %d\r\n",len);
		if (edg_wll_plain_write_full(&ctx->connProxy->conn,
							buf, blen, &ctx->p_tmp_timeout) < 0) 
			return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");
	}

	if ( edg_wll_plain_write_full(&ctx->connProxy->conn,
							"\r\n", 2, &ctx->p_tmp_timeout) < 0) 
		return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");
	if ( body && edg_wll_plain_write_full(&ctx->connProxy->conn,
							body, len, &ctx->p_tmp_timeout) < 0)  
		return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");

	return edg_wll_Error(ctx,NULL,NULL);
}
