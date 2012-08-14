#ident "$Header:"
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

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <assert.h>
#include "glite/lb/context.h"
#include "lb_proto.h"
#include "authz_policy.h"

/* Funny compatibility issue */
int	proxy_purge;
struct _edg_wll_authz_policy authz_policy;
char    *policy_file = NULL;
int debug;

int main() {
	int round, retval, i, j;
	char *query;
	edg_wll_QueryRec **conds;
	edg_wll_Context ctx;

	if (edg_wll_InitContext(&ctx) != 0) {
                fprintf(stderr, "Couldn't create L&B context.\n");
                return 1;
        }


	for (round = 0; 1 ;round++) {
		switch(round) {
			case 0:
				// Simple query
				asprintf(&query,"status=running");
				break;
			case 1:
				// ANDed query
				asprintf(&query,"status=running&owner=testowner");
				break;
			case 2:
				// ANDed and ORed query
				asprintf(&query,"status=running|=submitted&jobtype<>simple");
				break;
			case 3:
				// Bad attribute name
				asprintf(&query,"sXatus=running");
				break;
			case 4:
				// Bad attribute value
				asprintf(&query,"status=ruXning");
				break;
			case 5:
				// Query Empty
				query = strdup("");
				break;
			case 6:
				// No Operator
				query = strdup("status=running&owner");
				break;
			case 7:
				// No Operator in ORed condition
				query = strdup("status=running&owner=I|You");
				break;
			case 8:
				// No rval
				query = strdup("status=running&owner=");
				break;
			case 9:
				// Multiple operators
				query = strdup("status=running=submitted");
				break;
			case 10:
				// Empty AND section
				query = strdup("status=running&");
				break;
			case 11:
				// Empty AND section inbetween
				asprintf(&query,"status=running&&owner=testowner");
				break;
			case 12:
				// Empty OR section inbetween
				asprintf(&query,"status=running|||=submitted&jobtype<>simple");
				break;
			case 13:
				// Confused operator
				asprintf(&query,"status==running");
				break;

			default:
				goto done;
		}

		retval = edg_wll_ParseQueryConditions(ctx, query, &conds);

		//fprintf(stderr, "%s\n", ctx->errDesc);

		switch(round) {
			case 0:
			case 10:
				assert(conds[0] != NULL);
				assert(conds[0][0].attr == EDG_WLL_QUERY_ATTR_STATUS);
				assert(conds[0][0].op == EDG_WLL_QUERY_OP_EQUAL);
				assert(conds[0][0].value.i == EDG_WLL_JOB_RUNNING);
				assert(conds[0][1].attr == EDG_WLL_QUERY_ATTR_UNDEF);
				assert(conds[1] == NULL);
				assert(retval == 0);
				assert(ctx->errDesc == NULL);
				break;
			case 1:
			case 11:
				assert(retval == 0);
				assert(conds[0] != NULL);
				assert(conds[0][0].attr == EDG_WLL_QUERY_ATTR_STATUS);
				assert(conds[0][0].op == EDG_WLL_QUERY_OP_EQUAL);
				assert(conds[0][0].value.i == EDG_WLL_JOB_RUNNING);
				assert(conds[0][1].attr == EDG_WLL_QUERY_ATTR_UNDEF);
				assert(conds[1] != NULL);
				assert(conds[1][0].attr == EDG_WLL_QUERY_ATTR_OWNER);
				assert(conds[1][0].op == EDG_WLL_QUERY_OP_EQUAL);
				assert(!strcmp(conds[1][0].value.c, "testowner"));
				assert(conds[1][1].attr == EDG_WLL_QUERY_ATTR_UNDEF);
				assert(conds[2] == NULL);
				assert(retval == 0);
				assert(ctx->errDesc == NULL);
				break;
			case 2:
			case 12:
				assert(retval == 0);
				assert(conds[0] != NULL);
				assert(conds[0][0].attr == EDG_WLL_QUERY_ATTR_STATUS);
				assert(conds[0][0].op == EDG_WLL_QUERY_OP_EQUAL);
				assert(conds[0][0].value.i == EDG_WLL_JOB_RUNNING);
				assert(conds[0][1].attr == EDG_WLL_QUERY_ATTR_STATUS);
				assert(conds[0][1].op == EDG_WLL_QUERY_OP_EQUAL);
				assert(conds[0][1].value.i == EDG_WLL_JOB_SUBMITTED);
				assert(conds[0][2].attr == EDG_WLL_QUERY_ATTR_UNDEF);
				assert(conds[1] != NULL);
				assert(conds[1][0].attr == EDG_WLL_QUERY_ATTR_JOB_TYPE);
				assert(conds[1][0].op == EDG_WLL_QUERY_OP_UNEQUAL);
				assert(conds[1][0].value.i == EDG_WLL_STAT_SIMPLE);
				assert(conds[1][1].attr == EDG_WLL_QUERY_ATTR_UNDEF);
				assert(conds[2] == NULL);
				assert(retval == 0);
				assert(ctx->errDesc == NULL);
				break;
			case 3:
				assert(retval == EINVAL);
				assert(strstr(ctx->errDesc, "Unknown argument"));
				break;
			case 4:
				assert(retval == EINVAL);
				assert(strstr(ctx->errDesc, "Unknown job state"));
				break;
			case 5:
				assert(retval == 0);
				assert(conds == NULL);
				break;
			case 6:
				assert(retval == EINVAL);
				assert(strstr(ctx->errDesc, "Missing operator in condition"));
				break;
			case 7:
				assert(retval == EINVAL);
				assert(strstr(ctx->errDesc, "Missing operator before"));
				break;
			case 8:
				assert(retval == EINVAL);
				assert(strstr(ctx->errDesc, "No value given for attribute"));
				break;
			case 9:
				assert(retval == EINVAL);
				assert(strstr(ctx->errDesc, "Unknown job state"));
				break;
			case 13:
				assert(retval == EINVAL);
				assert(strstr(ctx->errDesc, "Unknown operator"));
				break;
		}

		edg_wll_ResetError(ctx);
		free(query); query=NULL;
                if (conds) {
                        for (j = 0; conds[j]; j++) {
                                for (i = 0 ; (conds[j][i].attr != EDG_WLL_QUERY_ATTR_UNDEF); i++ )
                                        edg_wll_QueryRecFree(&conds[j][i]);
                                free(conds[j]);
                        }
                        free(conds); conds = NULL;
                }
	}
	


done:
	edg_wll_FreeContext(ctx);
	return 0;
}
