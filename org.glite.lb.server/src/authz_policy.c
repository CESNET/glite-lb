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
