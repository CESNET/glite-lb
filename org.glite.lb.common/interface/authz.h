#ifndef GLITE_LB_AUTHZ_H
#define GLITE_LB_AUTHZ_H

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

typedef struct _edg_wll_authz_attr {
	int id;
	char *value;
} _edg_wll_authz_attr;

typedef struct _edg_wll_authz_rule {
	struct _edg_wll_authz_attr *attrs;
	size_t attrs_num;
} _edg_wll_authz_rule;

typedef struct _edg_wll_authz_action {
	int id;
	struct _edg_wll_authz_rule *rules;
	int rules_num;
} _edg_wll_authz_action;
	
typedef struct _edg_wll_authz_policy {
	struct _edg_wll_authz_action *actions;
	int actions_num;
} _edg_wll_authz_policy;

typedef struct _edg_wll_authz_policy *edg_wll_authz_policy;

typedef struct _edg_wll_mapping_rule {
	char *a;
	char *b;
} _edg_wll_mapping_rule;

typedef struct _edg_wll_id_mapping {
	struct _edg_wll_mapping_rule *rules;
	int num;
	//char *mapfile;
} _edg_wll_id_mapping;

int
edg_wll_add_authz_rule(edg_wll_Context ctx,
		       edg_wll_authz_policy policy,
		       int action,
		       struct _edg_wll_authz_rule *);

int
edg_wll_add_authz_attr(edg_wll_Context ctx,
		       struct _edg_wll_authz_rule *rule,
		       int id,
		       char *value);
		       

#ifdef __cplusplus 
}
#endif

#endif /* GLITE_LB_AUTHZ_H */
