#include <stdlib.h>

#include "glite/lb-utils/db.h"
#include "glite/lb/context-int.h"

int edg_wll_SetErrorDB(edg_wll_Context ctx) {
	int code;
	char *ed;

	if (ctx->dbctx) {
		code = glite_lbu_DBError(ctx->dbctx, NULL, &ed);
		edg_wll_SetError(ctx, code, ed);
		free(ed);
	} else {
		code = EINVAL;
		edg_wll_SetError(ctx, EINVAL, "DB context isn't created");
	}

	return code;
}
