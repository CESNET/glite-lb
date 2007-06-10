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


int edg_wll_ServerHTTP(edg_wll_Context ctx)
{
	char	**hdr = NULL,*req = NULL,*body = NULL,
		**hdrOut = NULL, *resp = NULL, *bodyOut = NULL,
		*err_desc = NULL;
	edg_wll_ErrorCode	err = 0;


	if ( ctx->isProxy ) err = edg_wll_http_recv_proxy(ctx,&req,&hdr,&body);
	else err = edg_wll_http_recv(ctx,&req,&hdr,&body,ctx->connections->serverConnection);

	dprintf(("[%d] %s\n",getpid(),req));
	if (body) dprintf(("\n%s\n\n",body));

	if (!err) {
		if ((err = edg_wll_Proto(ctx,req,hdr,body,&resp,&hdrOut,&bodyOut))) 
			edg_wll_Error(ctx,NULL,&err_desc);

		if (resp) {
			if ( ctx->isProxy )
				edg_wll_http_send_proxy(ctx,resp,(char const * const *)hdrOut,bodyOut);
			else
				edg_wll_http_send(ctx,resp,(char const * const *)hdrOut,bodyOut,ctx->connections->serverConnection);
		}
	}

	free(req);
	free(resp);
	if (hdr) {
		char 	**h;
		for (h = hdr; *h; h++) free(*h);
		free(hdr);
	}
	// hdrOut are static
	free(body);
	free(bodyOut);

	if (err != edg_wll_Error(ctx,NULL,NULL)) edg_wll_SetError(ctx,err,err_desc);
	free(err_desc);
	return err;
}
