#ifndef GLITE_LB_AUTHZ_H
#define GLITE_LB_AUTHZ_H

#ident "$Header$"

#include "context.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _edg_wll_VomsGroup {
	char *vo;
	char *name;
} edg_wll_VomsGroup;

typedef struct _edg_wll_VomsGroups {
	size_t len;
	edg_wll_VomsGroup *val;
} edg_wll_VomsGroups;

typedef struct _edg_wll_authz_rule {
	int action;
	int attr_id;
	char *attr_value;
} _edg_wll_authz_rule;

typedef struct _edg_wll_authz_policy {
	struct _edg_wll_authz_rule *rules;
	int num;
} _edg_wll_authz_policy;

typedef struct _edg_wll_authz_policy *edg_wll_authz_policy;

int
edg_wll_add_authz_rule(edg_wll_Context ctx,
		       edg_wll_authz_policy policy,
		       int action,
		       int attr_id,
		       char *attr_value);

#ifdef __cplusplus 
}
#endif

#endif /* GLITE_LB_AUTHZ_H */
