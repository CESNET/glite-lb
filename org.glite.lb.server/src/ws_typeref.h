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

#ifndef __EDG_WORKLOAD_LOGGING_LBSERVER_WS_TYPEREF_H__
#define __EDG_WORKLOAD_LOGGING_LBSERVER_WS_TYPEREF_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void edg_wll_JobStatCodeToSoap(edg_wll_JobStatCode, enum lbt__statName *);
extern void edg_wll_SoapToJobStatCode(enum lbt__statName, edg_wll_JobStatCode *);

extern void edg_wll_StatusToSoap(struct soap *, edg_wll_JobStat const *, struct lbt__jobStatus *);
extern void edg_wll_SoapToStatus(struct soap *, struct lbt__jobStatus const *, edg_wll_JobStat *);

extern void edg_wll_SoapToJobStatFlags(struct lbt__jobFlags const *, int *);
extern int edg_wll_JobStatFlagsToSoap(struct soap *, const int, struct lbt__jobFlags *);

extern void edg_wll_SoapToAttr(const enum lbt__queryAttr, edg_wll_QueryAttr *);
extern void edg_wll_AttrToSoap(const edg_wll_QueryAttr, enum lbt__queryAttr *);

extern void edg_wll_SoapToQueryOp(
			const enum lbt__queryOp,
			edg_wll_QueryOp *);
extern void edg_wll_QueryOpToSoap(
			const edg_wll_QueryOp,
			enum lbt__queryOp *);

extern int edg_wll_SoapToQueryVal(
			const edg_wll_QueryAttr,
			const struct lbt__queryRecValue *,
			union edg_wll_QueryVal *);
extern int edg_wll_QueryValToSoap(struct soap *,
			const edg_wll_QueryAttr,
			const union edg_wll_QueryVal *,
			struct lbt__queryRecValue *);

extern int edg_wll_SoapToQueryRec(
			const struct lbt__queryConditions *collection,
			const struct lbt__queryRecord *in,
			edg_wll_QueryRec *out);
extern int edg_wll_QueryRecToSoap(struct soap *,
			const edg_wll_QueryRec *,
			struct lbt__queryRecord *);


extern int edg_wll_SoapToQueryConds(
			const struct lbt__queryConditions *,
			edg_wll_QueryRec **);
extern int edg_wll_QueryCondsToSoap(struct soap *,
			const edg_wll_QueryRec *,
			struct lbt__queryConditions *);

extern int edg_wll_SoapToQueryCondsExt(
			GLITE_SECURITY_GSOAP_LIST_TYPE(lbt, queryConditions) in,
			int __sizecondition,
			edg_wll_QueryRec ***);
extern int edg_wll_QueryCondsExtToSoap(struct soap *,
			const edg_wll_QueryRec **,
			int *,
			GLITE_SECURITY_GSOAP_LIST_TYPE(lbt, queryConditions) *);

extern int edg_wll_JobsQueryResToSoap(struct soap *,
			edg_wlc_JobId *,
			edg_wll_JobStat *,
			struct _lbe__QueryJobsResponse *);

extern int edg_wll_EventsQueryResToSoap(struct soap *,
			const edg_wll_Event *,
			struct _lbe__QueryEventsResponse *);
extern int edg_wll_SoapToEventsQueryRes(
        		struct soap *,
	        	struct _lbe__QueryEventsResponse,
        		edg_wll_Event **);


extern int edg_wll_EventToSoap(struct soap*, const edg_wll_Event *, struct lbt__event *);
extern void edg_wll_FreeSoapEvent(struct soap *, struct lbt__event *);

#ifdef __cplusplus
}
#endif

#endif /* __EDG_WORKLOAD_LOGGING_LBSERVER_WS_TYPEREF_H__ */
