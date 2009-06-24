#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "glite/lb/mini_http.h"
#include "glite/lb/context-int.h"

#include "lb_http.h"
#include "lb_proto.h"

extern int debug;
#define dprintf(x) if (debug) printf x


int edg_wll_AcceptHTTP(edg_wll_Context ctx, char **resp, char ***hdrOut, char **bodyOut, int *httpErr)
{
	char	**hdr = NULL,*req = NULL,*body = NULL,
		*err_desc = NULL;
	edg_wll_ErrorCode	err = 0;
	*httpErr = HTTP_OK;


	if ( ctx->isProxy ) err = edg_wll_http_recv_proxy(ctx,&req,&hdr,&body);
	else err = edg_wll_http_recv(ctx,&req,&hdr,&body,ctx->connections->serverConnection);

	if (req) {
		dprintf(("[%d] request: %s\n",getpid(),req));
	} else {
		dprintf(("no request\n"));
	}
	if (body) dprintf(("request body:\n%s\n\n",body));

	if (!err) {
		if ((err = edg_wll_Proto(ctx,req,hdr,body,resp,hdrOut,bodyOut,httpErr)))
			edg_wll_Error(ctx,NULL,&err_desc);
	}

	free(req);
	if (hdr) {
		char 	**h;
		for (h = hdr; *h; h++) free(*h);
		free(hdr);
	}
	free(body);

	if (err != edg_wll_Error(ctx,NULL,NULL)) edg_wll_SetError(ctx,err,err_desc);
	free(err_desc);
	return err;
}

int edg_wll_DoneHTTP(edg_wll_Context ctx, char *resp, char **hdrOut, char *bodyOut)
{
	if (resp) {
	        if ( ctx->isProxy )
        	        edg_wll_http_send_proxy(ctx,resp,(char const * const *)hdrOut,bodyOut);
                else
                        edg_wll_http_send(ctx,resp,(char const * const *)hdrOut,bodyOut,ctx->connections->serverConnection);
        }

	return 0;
}
