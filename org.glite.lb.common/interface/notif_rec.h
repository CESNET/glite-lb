#ifndef GLITE_LB_NOTIF_REC_H
#define GLITE_LB_NOTIF_REC_H

#ident "$Header$"
/*
Copyright (c) Members of the EGEE Collaboration. 2004-2010.
See http://www.eu-egee.org/partners for details on the copyright holders.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/


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

#endif /* GLITE_LB_NOTIF_REC_H */
