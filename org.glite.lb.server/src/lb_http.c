#ident "$Header$"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "glite/lb/mini_http.h"
#include "glite/lb/context-int.h"

#include "glite/lbu/log.h"

#include "lb_http.h"
#include "lb_proto.h"

extern int debug;
#define dprintf(x) if (debug) printf x


int edg_wll_AcceptHTTP(edg_wll_Context ctx, char **body, char **resp, char ***hdrOut, char **bodyOut, int *httpErr)
{
	char	**hdr = NULL,*req = NULL, *err_desc = NULL;
	edg_wll_ErrorCode	err = 0;
	*httpErr = HTTP_OK;


	if ( ctx->isProxy ) err = edg_wll_http_recv_proxy(ctx,&req,&hdr,body);
	else err = edg_wll_http_recv(ctx,&req,&hdr,body,ctx->connections->serverConnection);

	if (req)
		glite_common_log(LOG_CATEGORY_LB_SERVER_ACCESS, 
			LOG4C_PRIORITY_DEBUG, "[%d] request: %s", 
			getpid(), req);
	else
		glite_common_log(LOG_CATEGORY_LB_SERVER_ACCESS, 
                        LOG4C_PRIORITY_DEBUG, "no request");
	if (body && *body) 
		glite_common_log(LOG_CATEGORY_LB_SERVER_ACCESS, 
                        LOG4C_PRIORITY_DEBUG, "request body:\n%s",*body);

	if (!err) {
		if ((err = edg_wll_Proto(ctx,req,hdr,*body,resp,hdrOut,bodyOut,httpErr)))
			edg_wll_Error(ctx,NULL,&err_desc);
	}

	free(req);
	if (hdr) {
		char 	**h;
		for (h = hdr; *h; h++) free(*h);
		free(hdr);
	}

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
