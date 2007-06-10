#ident "$Header$"

#include "glite/lb/context-int.h"

#include "lbs_db.h"

edg_wll_ErrorCode edg_wll_Open(edg_wll_Context ctx, char *cs)
{
	return edg_wll_DBConnect(ctx,cs) ? edg_wll_Error(ctx,NULL,NULL) : 0;
}

edg_wll_ErrorCode edg_wll_Close(edg_wll_Context ctx)
{
	return edg_wll_ResetError(ctx);
}
