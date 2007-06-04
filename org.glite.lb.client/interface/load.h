#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_LOAD_H__
#define __EDG_WORKLOAD_LOGGING_CLIENT_LOAD_H__

#ident "$Header$"

typedef struct {
	char	*server_file;
} edg_wll_LoadRequest;

typedef struct {
	char	*server_file;
	time_t	from,to;
} edg_wll_LoadResult;

/** Load events from a given file into the database
 * \retval EPERM	operation not permitted
 * \retval ENOENT	file not found
 */

int edg_wll_LoadEvents(
	edg_wll_Context,
	const edg_wll_LoadRequest *,
	edg_wll_LoadResult *
);

#endif

