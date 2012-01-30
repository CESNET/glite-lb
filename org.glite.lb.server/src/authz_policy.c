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
#include <errno.h>

#include <cclassad.h>

#include <glite/security/glite_gss.h>
#include "authz_policy.h"
#include "server_state.h"
#include "crypto.h"

struct action_name action_names[] = {
    { ADMIN_ACCESS,	"ADMIN_ACCESS" },
    { STATUS_FOR_MONITORING,	"STATUS_FOR_MONITORING" },
    { LOG_WMS_EVENTS,	"LOG_WMS_EVENTS" },
    { LOG_CE_EVENTS,	"LOG_CE_EVENTS" },
    { LOG_GENERAL_EVENTS,	"LOG_GENERAL_EVENTS" },
    { GET_STATISTICS,	"GET_STATISTICS" },
    { REGISTER_JOBS,	"REGISTER_JOBS" },	
    { READ_ALL, "READ_ALL" },
    { PURGE, "PURGE" },
    { GRANT_OWNERSHIP, "GRANT_OWNERSHIP" },
    { READ_ANONYMIZED, "READ_ANONYMIZED" },
};

static int num_actions =
    sizeof(action_names) / sizeof(action_names[0]);

struct attr_id_name attr_id_names[] = {
    { ATTR_SUBJECT,	"subject" },
    { ATTR_FQAN,	"fqan" },
};

static int num_attrs =
    sizeof(attr_id_names) / sizeof(attr_id_names[0]);

const char *allowed_jdl_fields[] = {
    "VirtualOrganisation",
    "JobType",
    "Type",
};

static int
check_rule(_edg_wll_authz_rule *rule, edg_wll_GssPrincipal principal)
{
    int i, found;
    char **f;
    _edg_wll_authz_attr *a;

    if (rule->attrs_num == 0)
	return 0;

    for (i = 0; i < rule->attrs_num; i++) {
	a = rule->attrs + i;
	if (strcmp(a->value, ".*") == 0)
	    continue;

	switch (a->id) {
	    case ATTR_SUBJECT:
		if (!edg_wll_gss_equal_subj(a->value, principal->name))
		    return 0;
		break;
	    case ATTR_FQAN:
		found = 0;
		for (f = principal->fqans; f && *f; f++)
		    if (strcmp(a->value, *f) == 0) {
			found = 1;
			break;
		}
		if (!found)
		    return 0;
		break;
	    default:
		return 0;
	}
    }

    return 1;
}


int
check_authz_policy(edg_wll_authz_policy policy,
		   edg_wll_GssPrincipal principal,
		   authz_action action)
{
    int i;
    _edg_wll_authz_action *a;

    if (policy == NULL)
        return 0;

    for (i = 0; i < policy->actions_num; i++) {
	if (policy->actions[i].id == action)
	   break;
    }
    if (i == policy->actions_num)
	/* Access denied by default */
	return 0;

    a = policy->actions + i;
    for (i = 0; i < a->rules_num; i++) {
	if (check_rule(a->rules+i, principal))
	    return 1;
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

    if (flags & STATUS_FOR_MONITORING) {
	new_stat.state = stat->state;
	edg_wlc_JobIdDup(stat->jobId, &new_stat.jobId);
	if (stat->jdl) {
	    struct cclassad *ad, *new_ad;
	    char *str;
	    int i;

	    ad = cclassad_create(stat->jdl);
	    if (ad) {
		new_ad = cclassad_create(NULL);
		for (i = 0; i < sizeof(allowed_jdl_fields)/sizeof(allowed_jdl_fields[0]);i++)
		    if (cclassad_evaluate_to_string(ad, allowed_jdl_fields[i], &str))
			cclassad_insert_string(new_ad, allowed_jdl_fields[i], str);
		new_stat.jdl = cclassad_unparse(new_ad);
		cclassad_delete(ad);
		cclassad_delete(new_ad);
	    }
	}
	new_stat.jobtype = stat->jobtype;
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

int
anonymize_stat(edg_wll_Context ctx, edg_wll_JobStat *stat)
{
    char *salt = NULL, *hash;
    int ret;

    ret = edg_wll_GetServerState(ctx, EDG_WLL_STATE_ANONYMIZATION_SALT, &salt);
    switch (ret) {
	case ENOENT:
	    edg_wll_ResetError(ctx);
	    ret = generate_salt(ctx, &salt);
	    if (ret)
		break;
	    ret = edg_wll_SetServerState(ctx, EDG_WLL_STATE_ANONYMIZATION_SALT, salt);
	    break;
	default:
	    break;
    }
    if (ret)
	goto end;

    ret = sha256_salt(ctx, stat->owner, salt, &hash);
    if (ret)
	goto end;
    free(stat->owner);
    stat->owner = hash;

    if (stat->payload_owner) {
	ret = sha256_salt(ctx, stat->payload_owner, salt, &hash);
	if (ret)
	    goto end;
	free(stat->payload_owner);
	stat->payload_owner = hash;
    }

    ret = 0;

end:
    if (salt)
	free(salt);
    
    return ret;
}

char *anonymize_string(edg_wll_Context ctx, char *string)
{
    static char *salt = NULL;
    char *hash;
    int ret;

    if (!salt) {
	    ret = edg_wll_GetServerState(ctx, EDG_WLL_STATE_ANONYMIZATION_SALT, &salt);
	    switch (ret) {
		case ENOENT:
		    edg_wll_ResetError(ctx);
		    ret = generate_salt(ctx, &salt);
		    if (ret)
			break;
		    ret = edg_wll_SetServerState(ctx, EDG_WLL_STATE_ANONYMIZATION_SALT, salt);
		    break;
		default:
		    break;
	    }
    	    if (ret) goto end;
    }

    sha256_salt(ctx, string, salt, &hash);

    end:

    return hash;
}
