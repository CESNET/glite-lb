#ifndef __EDG_WORKLOAD_LOGGING_COMMON_NOTIFID_H__
#define __EDG_WORKLOAD_LOGGING_COMMON_NOTIFID_H__

#ident "$Header$"

#ifdef __cplusplus
extern "C" {
#endif

/** Notification handle.
 * Refers to a particular registration for receiving notifications.
 */
typedef void *edg_wll_NotifId;

/** Parse and unparse the Id. */
int edg_wll_NotifIdParse(const char *,edg_wll_NotifId *);
char* edg_wll_NotifIdUnparse(const edg_wll_NotifId);

int edg_wll_NotifIdCreate(const char *,int,edg_wll_NotifId *);
void edg_wll_NotifIdFree(edg_wll_NotifId);

void edg_wll_NotifIdGetServerParts(const edg_wll_NotifId, char **, unsigned int *);
char *edg_wll_NotifIdGetUnique(const edg_wll_NotifId);
int edg_wll_NotifIdSetUnique(edg_wll_NotifId *, const char *);

#ifdef __cplusplus
}
#endif
#endif
