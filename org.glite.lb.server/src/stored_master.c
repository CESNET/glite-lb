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


#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "glite/security/glite_gss.h"
#include "glite/lb/il_msg.h"
#include "glite/lb/lb_plain_io.h"
#include "glite/lb/context-int.h"

#include "store.h"

#ifdef LB_PERF
#include "srv_perf.h"
#endif


static
int
gss_reader(void *user_data, char *buffer, int max_len)
{
  edg_wll_Context tmp_ctx = (edg_wll_Context)user_data;
  int ret; 
  size_t len;
  edg_wll_GssStatus gss_code;

  ret = edg_wll_gss_read_full(&tmp_ctx->connections->serverConnection->gss,
			      buffer, max_len,
			      &tmp_ctx->p_tmp_timeout,
			      &len, &gss_code);
  if(ret < 0) {
	  switch(ret) {

	  case EDG_WLL_GSS_ERROR_GSS:
		  edg_wll_SetErrorGss(tmp_ctx,"gss_reader",&gss_code);
		  break;

	  case EDG_WLL_GSS_ERROR_TIMEOUT: 
		  edg_wll_SetError(tmp_ctx, ETIMEDOUT, "gss_reader");
		  break;
		  
	  case EDG_WLL_GSS_ERROR_EOF:
		  edg_wll_SetError(tmp_ctx, ENOTCONN, NULL);
		  break;

	  case EDG_WLL_GSS_ERROR_ERRNO:
		  edg_wll_SetError(tmp_ctx, errno, "gss_reader");
		  break;

	  default:
		  edg_wll_SetError(tmp_ctx, EDG_WLL_ERROR_GSS, "gss_reader");
		  break;
	  }
  ret = -2; /* -1 is used by read_il_data internals */
  }

  return(ret);
}


// XXX: for easier merge from RC31_3
//	after merge, it would be possible to glue
//	edg_wll_StoreProtoProxy and edg_wll_StoreProtoServer together
int edg_wll_StoreProto(edg_wll_Context ctx) 
{
	if (ctx->isProxy) return(edg_wll_StoreProtoProxy(ctx));
	else return(edg_wll_StoreProtoServer(ctx));
}


int edg_wll_StoreProtoServer(edg_wll_Context ctx)
{
	char	*buf;
	size_t	len;
	int	ret;
	size_t	total;
	edg_wll_GssStatus	gss_code;

	edg_wll_ResetError(ctx);
	ret = read_il_data(ctx, &buf, gss_reader);
	if (ret == -1) 
	  return edg_wll_SetError(ctx,EIO,"interlogger protocol error");
	if (ret < 0)
	  return edg_wll_Error(ctx,NULL,NULL);
#ifdef LB_PERF
	if (sink_mode == GLITE_LB_SINK_PARSE) glite_wll_perftest_consumeEventIlMsg(buf);
	else
#endif

	handle_il_message(ctx,buf);
	free(buf);

	if ((len = create_reply(ctx,&buf)) > 0) {
		if ((ret = edg_wll_gss_write_full(&ctx->connections->serverConnection->gss,buf,len,&ctx->p_tmp_timeout,&total,&gss_code)) < 0)
			edg_wll_SetError(ctx,
				ret == EDG_WLL_GSS_ERROR_TIMEOUT ? 
					ETIMEDOUT : EDG_WLL_ERROR_GSS,
				"write reply");
		free(buf);
	}
	else edg_wll_SetError(ctx,E2BIG,"create_reply()");

	return edg_wll_Error(ctx,NULL,NULL);
}


static
int
gss_plain_reader(void *user_data, char *buffer, int max_len)
{
  edg_wll_Context tmp_ctx = (edg_wll_Context)user_data;
  int ret;

  ret = edg_wll_plain_read_full(&tmp_ctx->connProxy->conn, buffer, max_len,
				&tmp_ctx->p_tmp_timeout);
  if(ret < 0) {
	edg_wll_SetError(tmp_ctx, errno, "gss_plain_reader");
	return -2;
  }

  return(ret);
}


int edg_wll_StoreProtoProxy(edg_wll_Context ctx)
{
	char	*buf,
			*errd = NULL;
	int		len, ret,
			err = 0;


	edg_wll_ResetError(ctx);
	ret = read_il_data(ctx, &buf, gss_plain_reader);
	if (ret == -1) 
	  return edg_wll_SetError(ctx,EIO,"interlogger protocol error");
	if (ret < 0)
	  return edg_wll_Error(ctx,NULL,NULL);
#ifdef LB_PERF
	if (sink_mode == GLITE_LB_SINK_PARSE) glite_wll_perftest_consumeEventIlMsg(buf);
	else
#endif
	if ( !(ret = handle_il_message(ctx, buf)) ) {
		if ( (err = edg_wll_Error(ctx, NULL, &errd)) ) edg_wll_ResetError(ctx);
	}
	free(buf);

	
	if ( (len = create_reply(ctx, &buf)) > 0 ) {
		if ( edg_wll_plain_write_full(&ctx->connProxy->conn, buf, len, &ctx->p_tmp_timeout) < 0 ) {
			if ( errd ) free(errd);
			return edg_wll_SetError(ctx, errno, "StoreProtoProxy() - sending reply");
		}
		free(buf);
	}
	else ret = edg_wll_SetError(ctx, E2BIG, "create_reply()");

	if ( err ) {
		edg_wll_SetError(ctx, err, errd);
		free(errd);
	}
	return ret? edg_wll_Error(ctx, NULL, NULL): 0;
}
