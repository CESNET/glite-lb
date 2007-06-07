#ifndef __GLITE_LB_NOTIF_REC_H__
#define __GLITE_LB_NOTIF_REC_H__

#ident "$Header$"

#ifdef __cplusplus
extern "C" {
#endif

/* Enum used by edg_wll_NotifChange */
typedef enum _edg_wll_NotifChangeOp {
	/** No operation, equal to not defined */
	EDG_WLL_NOTIF_NOOP = 0,
	/** Replace notification registration with new one */
	EDG_WLL_NOTIF_REPLACE,
	/** Add new condition when to be notifed */
	EDG_WLL_NOTIF_ADD,
	/** Remove condition on notification */
	EDG_WLL_NOTIF_REMOVE
/*      if adding new attribute, add conversion string to common/xml_conversions.c too !! */
} edg_wll_NotifChangeOp;

#ifdef __cplusplus
}
#endif

#endif /* __GLITE_LB_NOTIF_REC_H__ */
