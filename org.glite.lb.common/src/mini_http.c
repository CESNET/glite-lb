#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <fcntl.h>
#include <syslog.h>

#include "globus_config.h"

#include "mini_http.h"
#include "lb_gss.h"
#include "lb_plain_io.h"
#include "context-int.h"

#define min(x,y)	((x) < (y) ? (x) : (y))
#define CONTENT_LENGTH	"Content-Length:"

edg_wll_ErrorCode edg_wll_http_recv(edg_wll_Context ctx,char **firstOut,char ***hdrOut,char **bodyOut)
{
	char	**hdr = NULL,*first = NULL,*body = NULL;
	enum	{ FIRST, HEAD, BODY, DONE }	pstat = FIRST;
	int	len, nhdr = 0,rdmore = 0,clen = 0,blen = 0;
	int	sock;
	edg_wll_GssStatus gss_code;

#define bshift(shift) {\
	memmove(ctx->connPool[ctx->connToUse].buf,ctx->connPool[ctx->connToUse].buf+(shift),ctx->connPool[ctx->connToUse].bufUse-(shift));\
	ctx->connPool[ctx->connToUse].bufUse -= (shift);\
}
	edg_wll_ResetError(ctx);

	if (ctx->connPool[ctx->connToUse].gss.context != GSS_C_NO_CONTEXT)
   		sock = ctx->connPool[ctx->connToUse].gss.sock;
	else {
		edg_wll_SetError(ctx,ENOTCONN,NULL);
		goto error;
	}

	if (!ctx->connPool[ctx->connToUse].buf) ctx->connPool[ctx->connToUse].buf = malloc(ctx->connPool[ctx->connToUse].bufSize = BUFSIZ);

	do {
		len = edg_wll_gss_read(&ctx->connPool[ctx->connToUse].gss,
			ctx->connPool[ctx->connToUse].buf+ctx->connPool[ctx->connToUse].bufUse,ctx->connPool[ctx->connToUse].bufSize-ctx->connPool[ctx->connToUse].bufUse,&ctx->p_tmp_timeout, &gss_code);

		switch (len) {
			case EDG_WLL_GSS_OK:
				break;
			case EDG_WLL_GSS_ERROR_GSS:
				edg_wll_SetErrorGss(ctx, "receving HTTP request", &gss_code);
				goto error;
			case EDG_WLL_GSS_ERROR_ERRNO:
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


		ctx->connPool[ctx->connToUse].bufUse += len;
		rdmore = 0;

		while (!rdmore && pstat != DONE) switch (pstat) {
			char	*cr; 

			case FIRST:
				if ((cr = memchr(ctx->connPool[ctx->connToUse].buf,'\r',ctx->connPool[ctx->connToUse].bufUse)) &&
					ctx->connPool[ctx->connToUse].bufUse >= cr-ctx->connPool[ctx->connToUse].buf+2 && cr[1] == '\n')
				{
					*cr = 0;
					first = strdup(ctx->connPool[ctx->connToUse].buf);
					bshift(cr-ctx->connPool[ctx->connToUse].buf+2);
					pstat = HEAD;
				} else rdmore = 1;
				break;
			case HEAD:
				if ((cr = memchr(ctx->connPool[ctx->connToUse].buf,'\r',ctx->connPool[ctx->connToUse].bufUse)) &&
					ctx->connPool[ctx->connToUse].bufUse >= cr-ctx->connPool[ctx->connToUse].buf+2 && cr[1] == '\n')
				{
					if (cr == ctx->connPool[ctx->connToUse].buf) {
						bshift(2);
						pstat = clen ? BODY : DONE;
						if (clen) body = malloc(clen+1);
						break;
					}

					*cr = 0;
					hdr = realloc(hdr,(nhdr+2) * sizeof(*hdr));
					hdr[nhdr] = strdup(ctx->connPool[ctx->connToUse].buf);
					hdr[++nhdr] = NULL;

					if (!strncasecmp(ctx->connPool[ctx->connToUse].buf,CONTENT_LENGTH,sizeof(CONTENT_LENGTH)-1))
						clen = atoi(ctx->connPool[ctx->connToUse].buf+sizeof(CONTENT_LENGTH)-1);
	
					bshift(cr-ctx->connPool[ctx->connToUse].buf+2);
				} else rdmore = 1;
				break;
			case BODY:
				if (ctx->connPool[ctx->connToUse].bufUse) {
					int	m = min(ctx->connPool[ctx->connToUse].bufUse,clen-blen);
					memcpy(body+blen,ctx->connPool[ctx->connToUse].buf,m);
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
	memmove(ctx->connPlain->buf,\
			ctx->connPlain->buf+(shift),\
			ctx->connPlain->bufUse-(shift));\
	ctx->connPlain->bufUse -= (shift);\
}
	edg_wll_ResetError(ctx);

	if ( !ctx->connPlain->buf ) {
		ctx->connPlain->bufSize = BUFSIZ;
		ctx->connPlain->buf = malloc(BUFSIZ);
	}

	do {
		len = edg_wll_plain_read(ctx->connPlain,
				ctx->connPlain->buf+ctx->connPlain->bufUse,
				ctx->connPlain->bufSize-ctx->connPlain->bufUse,
				&ctx->p_tmp_timeout);
		if ( len < 0 ) goto error;

		ctx->connPlain->bufUse += len;
		rdmore = 0;

		while (!rdmore && pstat != DONE) switch (pstat) {
			char	*cr; 

			case FIRST:
				if ((cr = memchr(ctx->connPlain->buf,'\r',ctx->connPlain->bufUse)) &&
					ctx->connPlain->bufUse >= cr-ctx->connPlain->buf+2 && cr[1] == '\n')
				{
					*cr = 0;
					first = strdup(ctx->connPlain->buf);
					bshift(cr-ctx->connPlain->buf+2);
					pstat = HEAD;
				} else rdmore = 1;
				break;
			case HEAD:
				if ((cr = memchr(ctx->connPlain->buf,'\r',ctx->connPlain->bufUse)) &&
					ctx->connPlain->bufUse >= cr-ctx->connPlain->buf+2 && cr[1] == '\n')
				{
					if (cr == ctx->connPlain->buf) {
						bshift(2);
						pstat = clen ? BODY : DONE;
						if (clen) body = malloc(clen+1);
						break;
					}

					*cr = 0;
					hdr = realloc(hdr,(nhdr+2) * sizeof(*hdr));
					hdr[nhdr] = strdup(ctx->connPlain->buf);
					hdr[++nhdr] = NULL;

					if (!strncasecmp(ctx->connPlain->buf,CONTENT_LENGTH,sizeof(CONTENT_LENGTH)-1))
						clen = atoi(ctx->connPlain->buf+sizeof(CONTENT_LENGTH)-1);
	
					bshift(cr-ctx->connPlain->buf+2);
				} else rdmore = 1;
				break;
			case BODY:
				if (ctx->connPlain->bufUse) {
					int	m = min(ctx->connPlain->bufUse,clen-blen);
					memcpy(body+blen,ctx->connPlain->buf,m);
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

edg_wll_ErrorCode edg_wll_http_send(edg_wll_Context ctx,const char *first,const char * const *head,const char *body)
{
	const char* const *h;
	int	len = 0, blen;

	edg_wll_ResetError(ctx);

	if (ctx->connPool[ctx->connToUse].gss.context == GSS_C_NO_CONTEXT)
	   return edg_wll_SetError(ctx,ENOTCONN,NULL);

	if (real_write(ctx,&ctx->connPool[ctx->connToUse].gss,first,strlen(first)) < 0 ||
		real_write(ctx,&ctx->connPool[ctx->connToUse].gss,"\r\n",2) < 0) 
		return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");

	if (head) for (h=head; *h; h++) 
		if (real_write(ctx,&ctx->connPool[ctx->connToUse].gss,*h,strlen(*h)) < 0 ||
			real_write(ctx,&ctx->connPool[ctx->connToUse].gss,"\r\n",2) < 0) 
			return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");

	if (body) {
		char	buf[100];

		len = strlen(body);
		blen = sprintf(buf,CONTENT_LENGTH " %d\r\n",len);
		if (real_write(ctx,&ctx->connPool[ctx->connToUse].gss,buf,blen) < 0) 
			return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");
	}

	if (real_write(ctx,&ctx->connPool[ctx->connToUse].gss,"\r\n",2) < 0) 
		return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");
	if (body && real_write(ctx,&ctx->connPool[ctx->connToUse].gss,body,len) < 0)  
		return edg_wll_SetError(ctx,errno,"edg_wll_http_send()");

	return edg_wll_Error(ctx,NULL,NULL);
}

edg_wll_ErrorCode edg_wll_http_send_proxy(edg_wll_Context ctx, const char *first, const char * const *head, const char *body)
{
	const char* const *h;
	int	len = 0, blen;


	edg_wll_ResetError(ctx);

	if (   edg_wll_plain_write_full(ctx->connPlain,
							first, strlen(first), &ctx->p_tmp_timeout) < 0
		|| edg_wll_plain_write_full(ctx->connPlain,
							"\r\n", 2, &ctx->p_tmp_timeout) < 0 ) 
		return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");

	if ( head ) for ( h = head; *h; h++ )
		if (   edg_wll_plain_write_full(ctx->connPlain,
							*h, strlen(*h), &ctx->p_tmp_timeout) < 0
			|| edg_wll_plain_write_full(ctx->connPlain,
							"\r\n", 2, &ctx->p_tmp_timeout) < 0 )
			return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");

	if ( body ) {
		char	buf[100];

		len = strlen(body);
		blen = sprintf(buf, CONTENT_LENGTH " %d\r\n",len);
		if (edg_wll_plain_write_full(ctx->connPlain,
							buf, blen, &ctx->p_tmp_timeout) < 0) 
			return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");
	}

	if ( edg_wll_plain_write_full(ctx->connPlain,
							"\r\n", 2, &ctx->p_tmp_timeout) < 0) 
		return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");
	if ( body && edg_wll_plain_write_full(ctx->connPlain,
							body, len, &ctx->p_tmp_timeout) < 0)  
		return edg_wll_SetError(ctx, errno, "edg_wll_http_send()");

	return edg_wll_Error(ctx,NULL,NULL);
}
