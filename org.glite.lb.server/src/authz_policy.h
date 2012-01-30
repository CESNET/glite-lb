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

#ifndef GLITE_LB_AUTHZ_POLICY_H
#define GLITE_LB_AUTHZ_POLICY_H

#include <glite/lb/context-int.h>
#include <glite/lb/authz.h>
#include <glite/security/glite_gss.h>

typedef enum {
    ACTION_UNDEF	= 0,
    ADMIN_ACCESS	= ~0,
    STATUS_FOR_MONITORING = 1 << 1,
    LOG_WMS_EVENTS	= 1 << 2,
    LOG_CE_EVENTS	= 1 << 3,
    LOG_GENERAL_EVENTS	= 1 << 4,
    GET_STATISTICS	= 1 << 5,
    REGISTER_JOBS	= 1 << 6,
    READ_ALL		= 1 << 7,
    PURGE		= 1 << 8,
    GRANT_OWNERSHIP	= 1 << 9,
    READ_ANONYMIZED 	= 1 << 10,
} authz_action;

typedef struct action_name {
    authz_action action;
    const char *name;
} action_name;

typedef enum {
    ATTR_UNDEF		= 0,
    ATTR_SUBJECT	= 1,
    ATTR_FQAN		= 2,
} authz_attr_id;

struct attr_id_name {
    authz_attr_id id;
    const char *name;
} attr_id_name;

int
parse_server_policy(edg_wll_Context ctx, const char *filename, edg_wll_authz_policy policy);

int
check_authz_policy(edg_wll_authz_policy, edg_wll_GssPrincipal, authz_action);

authz_action
find_authz_action(const char *name);

const char *
action2name(authz_action);

authz_attr_id
find_authz_attr(const char *name);

int
blacken_fields(edg_wll_JobStat *, int flags);

int
anonymize_stat(edg_wll_Context, edg_wll_JobStat *);

char *
anonymize_string(edg_wll_Context ctx, char *string);

#endif
