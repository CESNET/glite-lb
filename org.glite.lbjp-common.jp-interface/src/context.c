#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "types.h"
#include "context.h"

int glite_jp_init_context(glite_jp_context_t *ctx)
{
	return (*ctx = calloc(1,sizeof **ctx)) != NULL;
}

void glite_jp_free_context(glite_jp_context_t ctx)
{
	glite_jp_clear_error(ctx);
	free(ctx);
}

char *glite_jp_peer_name(glite_jp_context_t ctx)
{
	return strdup(ctx->peer ? ctx->peer : "unknown");
}

char *glite_jp_error_chain(glite_jp_context_t ctx)
{
	char	*ret = NULL,indent[300] = "";
	int	len = 0,add;
	char	buf[2000];

	glite_jp_error_t *ep = ctx->error;

	do {
		add = snprintf(buf,sizeof buf,"%s%s: %s (%s)\n",
				indent,
				ep->source,
				strerror(ep->code),
				ep->desc ? ep->desc : "");
		ret = realloc(ret,len + add + 1);
		strncpy(ret + len,buf,add); ret[len += add] = 0;
		strcat(indent,"  ");
	} while ((ep = ep->reason));

	return ret;
}

int glite_jp_stack_error(glite_jp_context_t ctx, const glite_jp_error_t *err)
{
	glite_jp_error_t	*reason = ctx->error;

	ctx->error = calloc(1,sizeof *ctx->error);
	ctx->error->code = err->code;
	ctx->error->desc = err->desc ? strdup(err->desc) : NULL;
	ctx->error->source = err->source ? strdup(err->source) : NULL;
	ctx->error->reason = reason;

	return err->code;
}

int glite_jp_clear_error(glite_jp_context_t ctx)
{
	glite_jp_error_t	*e = ctx->error, *r;

	while (e) {
		r = e->reason;
		free((char *) e->source);
		free((char *) e->desc);
		free(e);
		e = r;
	}
	ctx->error = NULL;
	return 0;
}


void glite_jp_free_query_rec(glite_jp_query_rec_t *q)
{
	free(q->attr); 
	free(q->value);
	free(q->value2);
	memset(q,0,sizeof *q);
}

int glite_jp_queryrec_copy(glite_jp_query_rec_t *dst, const glite_jp_query_rec_t *src)
{
	memcpy(dst,src,sizeof *src);
	if (src->attr) dst->attr = strdup(src->attr);
	if (src->value) dst->value = strdup(src->value);
	if (src->value2) dst->value2 = strdup(src->value2);
	return 0;
}

int glite_jp_run_deferred(glite_jp_context_t ctx)
{
	int	i,cnt,ret;

	if (!ctx->deferred_func) return 0;

	glite_jp_clear_error(ctx);
	for (cnt=0;ctx->deferred_func[cnt];cnt++);
	for (i=0; i<cnt; i++) {
		if ((ret = (*ctx->deferred_func)(ctx,*ctx->deferred_arg))) {
			glite_jp_error_t	err;
			char	desc[100];

			sprintf(desc,"calling func #%d, %p",i,*ctx->deferred_func);
			err.code = ret;
			err.desc = desc;
			err.source = "glite_jp_run_deferred()";

			glite_jp_stack_error(ctx,&err);
			return ret;
		}
		else {
			memmove(ctx->deferred_func,ctx->deferred_func+1,
					(cnt-i) * sizeof *ctx->deferred_func);
			memmove(ctx->deferred_arg,ctx->deferred_arg+1,
					(cnt-i) * sizeof *ctx->deferred_arg);
		}
	}
	free(ctx->deferred_func); ctx->deferred_func = NULL;
	free(ctx->deferred_arg); ctx->deferred_arg = NULL;
	return 0;
}

int glite_jp_add_deferred(
		glite_jp_context_t ctx,
		int (*func)(glite_jp_context_t, void *),
		void *arg
)
{
	int 	(**v)(glite_jp_context_t, void *) = ctx->deferred_func;
	int	i;

	for (i=0; v && v[i]; i++);

	ctx->deferred_func = realloc(ctx->deferred_func, (i+2) * sizeof *ctx->deferred_func);
	ctx->deferred_func[i] = func;
	ctx->deferred_func[i+1] = NULL;

	ctx->deferred_arg = realloc(ctx->deferred_arg,(i+2) * sizeof *ctx->deferred_arg);
	ctx->deferred_arg[i] = arg;
	ctx->deferred_arg[i+1] = NULL;

	return 0;
}
