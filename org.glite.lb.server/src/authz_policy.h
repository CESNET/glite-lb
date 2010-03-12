#ifndef GLITE_LB_AUTHZ_POLICY_H
#define GLITE_LB_AUTHZ_POLICY_H

#include <glite/lb/context-int.h>
#include <glite/lb/authz.h>

typedef enum {
    ACTION_UNDEF	= 0,
    READ_ALL		= 2,
    READ_RTM		= 4,
} authz_action;

typedef struct action_name {
    authz_action action;
    const char *name;
} action_name;

typedef enum {
    ATTR_UNDEF		= 0,
    ATTR_SUBJECT	= 2,
    ATTR_FQAN		= 4,
} authz_attr_id;

struct attr_id_name {
    authz_attr_id id;
    const char *name;
} attr_id_name;

int
parse_server_policy(edg_wll_Context ctx, const char *filename, edg_wll_authz_policy policy);

int
check_authz_policy(edg_wll_Context, edg_wll_authz_policy, authz_action);

authz_action
find_authz_action(const char *name);

authz_attr_id
find_authz_attr(const char *name);

#endif
