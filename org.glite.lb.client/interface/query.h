#ifndef __GLITE_LB_QUERY_H__
#define __GLITE_LB_QUERY_H__

#ident "$Header$"

#include <glite/lb/query_rec.h>
#include <glite/lb/context.h>

/** Client side purge 
 * \retval EAGAIN only partial result returned, call repeatedly to get all
 * 	output data
 */
int edg_wll_Purge(
	edg_wll_Context ctx,
	edg_wll_PurgeRequest *request,
	edg_wll_PurgeResult	*result
);

/** Dump events in a given time interval
 */

int edg_wll_DumpEvents(
	edg_wll_Context,
	const edg_wll_DumpRequest *,
	edg_wll_DumpResult *
);

/** Load events from a given file into the database
 * \retval EPERM	operation not permitted
 * \retval ENOENT	file not found
 */

int edg_wll_LoadEvents(
	edg_wll_Context,
	const edg_wll_LoadRequest *,
	edg_wll_LoadResult *
);

#endif /* __GLITE_LB_QUERY_H__ */
