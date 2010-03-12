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

#include <stdlib.h>
#include <string.h>

#include <glite/security/glite_gss.h>
#include "authz_policy.h"

struct action_name action_names[] = {
    { READ_ALL,		"READ_ALL" },
    { READ_RTM,		"READ_RTM" },
};

static int num_actions =
    sizeof(action_names) / sizeof(action_names[0]);

struct attr_id_name attr_id_names[] = {
    { ATTR_SUBJECT,	"subject" },
    { ATTR_FQAN,	"fqan" },
};

static int num_attrs =
    sizeof(attr_id_names) / sizeof(attr_id_names[0]);


int
check_authz_policy(edg_wll_Context ctx, edg_wll_authz_policy policy,
		   authz_action action)
{
    int i;
    char **f;
    _edg_wll_authz_rule *r;

    if (policy == NULL)
        return 0;

    for (i = 0; i < policy->num; i++) {
        r = policy->rules + i;
        if (r->action != action)
            break;
        switch (r->attr_id) {
            case ATTR_SUBJECT:
		if (edg_wll_gss_equal_subj(r->attr_value, ctx->peerName))
		    return 1;
		break;
	    case ATTR_FQAN:
		for (f = ctx->fqans; f && *f; f++)
		    if (strcmp(r->attr_value, *f) == 0)
			return 1;
		break;
	    default:
		break;
        }
    }

    return 0;
}

authz_action
find_authz_action(const char *name)
{
    int i;
    
    for (i = 0; i < num_actions; i++)
	if (strcmp(action_names[i].name, name) == 0)
	    return action_names[i].action;
    return ACTION_UNDEF;
}

authz_attr_id
find_authz_attr(const char *name)
{
    int i;

    for (i = 0; i < num_attrs; i++)
	if (strcmp(attr_id_names[i].name, name) == 0)
	    return attr_id_names[i].id;
    return ATTR_UNDEF;
}
