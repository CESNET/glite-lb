#ifndef __EDG_WORKLOAD_LOGGING_LBSERVER_WS_TYPEREF_H__
#define __EDG_WORKLOAD_LOGGING_LBSERVER_WS_TYPEREF_H__

#ifdef __cplusplus
extern "C" {
#endif

extern void edg_wll_JobStatCodeToSoap(edg_wll_JobStatCode, enum edgwll__JobStatCode *);
extern void edg_wll_SoapToJobStatCode(enum edgwll__JobStatCode, edg_wll_JobStatCode *);

extern void edg_wll_StatusToSoap(struct soap *, edg_wll_JobStat const *, struct edgwll__JobStat **);
extern void edg_wll_SoapToStatus(struct soap *, struct edgwll__JobStat const *, edg_wll_JobStat **);

extern void edg_wll_SoapToJobStatFlags(const struct edgwll__JobStatFlags *, int *);
extern int edg_wll_JobStatFlagsToSoap(struct soap *, const int, struct edgwll__JobStatFlags *);

extern void edg_wll_SoapToAttr(const enum edgwll__QueryAttr, edg_wll_QueryAttr *);
extern void edg_wll_AttrToSoap(const edg_wll_QueryAttr, enum edgwll__QueryAttr *);

extern void edg_wll_SoapToQueryOp(
			const enum edgwll__QueryOp,
			edg_wll_QueryOp *);
extern void edg_wll_QueryOpToSoap(
			const edg_wll_QueryOp,
			enum edgwll__QueryOp *);

extern int edg_wll_SoapToQueryVal(
			const edg_wll_QueryAttr,
			const struct edgwll__QueryRecValue *,
			union edg_wll_QueryVal *);
extern int edg_wll_QueryValToSoap(struct soap *,
			const edg_wll_QueryAttr,
			const union edg_wll_QueryVal *,
			struct edgwll__QueryRecValue *);

extern int edg_wll_SoapToQueryRec(
			const enum edgwll__QueryAttr,
			const struct edgwll__QueryRec *,
			edg_wll_QueryRec *);
extern int edg_wll_QueryRecToSoap(struct soap *,
			const edg_wll_QueryRec *,
			struct edgwll__QueryRec *);


extern int edg_wll_SoapToQueryConds(
			const struct edgwll__QueryCondition *,
			edg_wll_QueryRec **);
extern int edg_wll_QueryCondsToSoap(struct soap *,
			const edg_wll_QueryRec *,
			struct edgwll__QueryCondition **);

extern int edg_wll_SoapToQueryCondsExt(
			const struct edgwll__QueryConditions *,
			edg_wll_QueryRec ***);
extern int edg_wll_QueryCondsExtToSoap(struct soap *,
			const edg_wll_QueryRec **,
			struct edgwll__QueryConditions **);

extern int edg_wll_JobsQueryResToSoap(struct soap *,
			edg_wlc_JobId *,
			edg_wll_JobStat *,
			struct edgwll2__QueryJobsResponse *);

#ifdef __cplusplus
}
#endif

#endif /* __EDG_WORKLOAD_LOGGING_LBSERVER_WS_TYPEREF_H__ */
