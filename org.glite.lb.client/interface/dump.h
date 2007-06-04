#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_DUMP_H__
#define __EDG_WORKLOAD_LOGGING_CLIENT_DUMP_H__

#ident "$Header$"

#define EDG_WLL_DUMP_NOW	-1
#define EDG_WLL_DUMP_LAST_START	-2
#define EDG_WLL_DUMP_LAST_END	-3
/*      if adding new attribute, add conversion string to common/xml_conversions.c too !! */

typedef struct {
	time_t	from,to;
} edg_wll_DumpRequest;

typedef struct {
	char	*server_file;
	time_t	from,to;
} edg_wll_DumpResult;

/** Dump events in a given time interval
 */

int edg_wll_DumpEvents(
	edg_wll_Context,
	const edg_wll_DumpRequest *,
	edg_wll_DumpResult *
);


#endif

