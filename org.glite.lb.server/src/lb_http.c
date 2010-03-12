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

	if (req) {
		dprintf(("[%d] request: %s\n",getpid(),req));
	} else {
		dprintf(("no request\n"));
	}
	if (body) dprintf(("request body:\n%s\n\n",body));

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
