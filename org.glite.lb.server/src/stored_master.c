#ident "$Header$"

#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#include "glite/lb/il_string.h"
#include "glite/lb/lb_gss.h"
#include "glite/lb/context-int.h"

#include "store.h"

int edg_wll_StoreProto(edg_wll_Context ctx)
{
	char	fbuf[256],*buf;
	int	len,ret;
	size_t	total;
	edg_wll_GssStatus	gss_code;

	edg_wll_ResetError(ctx);
	if ((ret=edg_wll_gss_read_full(&ctx->connPool[ctx->connToUse].gss,fbuf,17,&ctx->p_tmp_timeout,&total,&gss_code)) < 0) switch (ret) {
		case EDG_WLL_GSS_ERROR_TIMEOUT: return edg_wll_SetError(ctx,ETIMEDOUT,"read message header");
		case EDG_WLL_GSS_ERROR_EOF: return edg_wll_SetError(ctx,ENOTCONN,NULL);
		default: return edg_wll_SetError(ctx,EDG_WLL_ERROR_GSS,"read message header");
	}

	if ((len = atoi(fbuf)) <= 0) return edg_wll_SetError(ctx,EINVAL,"message length");

	buf = malloc(len+1);

	if ((ret=edg_wll_gss_read_full(&ctx->connPool[ctx->connToUse].gss,buf,len,&ctx->p_tmp_timeout,&total,&gss_code)) < 0) {
		free(buf);
		return edg_wll_SetError(ctx,
			ret == EDG_WLL_GSS_ERROR_TIMEOUT ?
				ETIMEDOUT : EDG_WLL_ERROR_GSS,
			"read message");
	}


	buf[len] = 0;

	handle_request(ctx,buf,len);
	free(buf);

	if ((len = create_reply(ctx,fbuf,sizeof fbuf))) {
		if ((ret = edg_wll_gss_write_full(&ctx->connPool[ctx->connToUse].gss,fbuf,len,&ctx->p_tmp_timeout,&total,&gss_code)) < 0)
			edg_wll_SetError(ctx,
				ret == EDG_WLL_GSS_ERROR_TIMEOUT ? 
					ETIMEDOUT : EDG_WLL_ERROR_GSS,
				"write reply");
	}
	else edg_wll_SetError(ctx,E2BIG,"create_reply()");

	return edg_wll_Error(ctx,NULL,NULL);
}
