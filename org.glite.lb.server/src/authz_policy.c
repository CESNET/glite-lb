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
    { ADMIN_ACCESS,	"ADMIN_ACCESS" },
    { STATUS_FOR_RTM,	"STATUS_FOR_RTM" },
    { LOG_WMS_EVENTS,	"LOG_WMS_EVENTS" },
    { LOG_CE_EVENTS,	"LOG_CE_EVENTS" },
    { LOG_GENERAL_EVENTS,	"LOG_GENERAL_EVENTS" },
    { GET_STATISTICS,	"GET_STATISTICS" },
    { REGISTER_JOBS,	"REGISTER_JOBS" },	
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
check_authz_policy(edg_wll_authz_policy policy,
		   edg_wll_GssPrincipal principal,
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
            continue;
	if (strcmp(r->attr_value, ".*") == 0)
	    return 1;
        switch (r->attr_id) {
            case ATTR_SUBJECT:
		if (edg_wll_gss_equal_subj(r->attr_value, principal->name))
		    return 1;
		break;
	    case ATTR_FQAN:
		for (f = principal->fqans; f && *f; f++)
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

const char *
action2name(authz_action a)
{
    int i;

    for (i = 0; i < num_actions; i++)
	if (action_names[i].action == a)
	    return action_names[i].name;
    return NULL;
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

int
blacken_fields(edg_wll_JobStat *stat, int flags)
{
    edg_wll_JobStat new_stat;

    edg_wll_InitStatus(&new_stat);

    if (flags & STATUS_FOR_RTM) {
	new_stat.state = stat->state;
	edg_wlc_JobIdDup(stat->jobId, &new_stat.jobId);
	if (stat->destination)
	    new_stat.destination = strdup(stat->destination);
	if (stat->network_server)
	    new_stat.network_server = strdup(stat->network_server);
	new_stat.stateEnterTime = stat->stateEnterTime;
	new_stat.lastUpdateTime = stat->lastUpdateTime;
	if (stat->stateEnterTimes) {
	    int i = 1 + stat->stateEnterTimes[0];
	    new_stat.stateEnterTimes = malloc(sizeof(*stat->stateEnterTimes)*i);
	    memcpy(new_stat.stateEnterTimes, stat->stateEnterTimes,
		   sizeof(*stat->stateEnterTimes)*i);
	}
	if (stat->ui_host)
	    new_stat.ui_host = strdup(stat->ui_host);
    }

    edg_wll_FreeStatus(stat);
    memset(stat, 0, sizeof(*stat));
    edg_wll_CpyStatus(&new_stat, stat);
    edg_wll_FreeStatus(&new_stat);
    return 0;
}
