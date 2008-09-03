#ifndef GLITE_LB_EVENTS_PARSE_H
#define GLITE_LB_EVENTS_PARSE_H

#ident "$Header$"

#ifdef BUILDING_LB_COMMON
#include "events.h"
#else
#include "glite/lb/events.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Parse a ULM line into a edg_wll_Event structure
 * \param context IN: context to work with
 * \param logline IN: ULM string to parse
 * \param event OUT: parsed event
 * 	(may be NULL - syntax checking with no output)
 */
extern edg_wll_ErrorCode edg_wll_ParseEvent(
	edg_wll_Context	context,
	edg_wll_LogLine	logline,
	edg_wll_Event ** event
);

/** 
 * Generate ULM line from edg_wll_Event structure
 * \param context IN: context to work with
 * \param event IN: event to unparse
 */
extern edg_wll_LogLine edg_wll_UnparseEvent(
	edg_wll_Context	context,
	edg_wll_Event *	event
);

/**
 * Check event for completness.
 * Auxiliary function, checks whether all required fields of
 * the event type are present.
 * \param context IN: context to work with
 * \param event IN: event to check
 */
extern edg_wll_ErrorCode edg_wll_CheckEvent(
	edg_wll_Context	context,
	edg_wll_Event *	event
);

/**
 * Compare two event structures
 * Auxiliary function, compares all fields of two event structure 
 * \param context IN: context to work with
 * \param e1 IN: first event to compare
 * \param e2 IN: second event to compare
 */
extern edg_wll_ErrorCode edg_wll_CompareEvents(
        edg_wll_Context context,
        const edg_wll_Event *e1,
        const edg_wll_Event *e2
);


/**
 * Parse "only" jobId from ULM message
 * \param logline IN: ULM string to parse
 * \return jobId string
 */
extern char *edg_wll_GetJobId(edg_wll_LogLine logline);

/**
 * Parse a special Notification ULM line into a edg_wll_Event structure
 * \param context IN: context to work with
 * \param logline IN: ULM string to parse
 * \param event OUT: parsed event
 * 	(may be NULL - syntax checking with no output)
 */
extern edg_wll_ErrorCode edg_wll_ParseNotifEvent(
	edg_wll_Context	context,
	edg_wll_LogLine	logline,
	edg_wll_Event ** event
);

/** 
 * Generate a special Notification ULM line from edg_wll_Event structure
 * \param context IN: context to work with
 * \param event IN: event to unparse
 */
extern edg_wll_LogLine edg_wll_UnparseNotifEvent(
	edg_wll_Context	context,
	edg_wll_Event *	event
);

#ifdef __cplusplus
}
#endif


#endif /* GLITE_LB_EVENTS_PARSE_H */
