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

#include <stdio.h>
#include <string.h>

#include "notification.h"

int main(int argc,char **argv)
{
	edg_wll_Context	ctx;
	edg_wll_QueryRec	chj[2], chs[2];
	const edg_wll_QueryRec const	*ch2[3] = { chj, chs, NULL };
	char	*et,*ed;
	edg_wll_NotifId	id;
	edg_wll_JobStat	stat;
	time_t	valid = 0;

	if (edg_wll_InitContext(&ctx) != 0) {
		fprintf(stderr, "Couldn't create L&B context.\n");
		return 1;
	}

	memset(&chj,0,sizeof chj);
	chj[0].op = EDG_WLL_QUERY_OP_CHANGED;
	chj[0].attr = EDG_WLL_QUERY_ATTR_JDL_ATTR;
	chj[0].attr_id.tag = NULL;

	memset(&chs,0,sizeof chs);
	chs[0].op = EDG_WLL_QUERY_OP_EQUAL;
	chs[0].attr = EDG_WLL_QUERY_ATTR_STATUS;
	chs[0].value.i = EDG_WLL_JOB_WAITING;

	if (edg_wll_NotifNew(ctx,ch2,EDG_WLL_STAT_CLASSADS,-1,NULL,&id,&valid)) {
		edg_wll_Error(ctx,&et,&ed);
		fprintf(stderr,"edg_wll_NotifNew(): %s %s\n",et,ed);
		return 1;
	}

	printf("notif %s\n",edg_wll_NotifIdUnparse(id));

	for (;;) {
		struct timeval	tv = {120,0};
		if (edg_wll_NotifReceive(ctx,-1,&tv,&stat,&id)) {
			edg_wll_Error(ctx,&et,&ed);
			fprintf(stderr,"edg_wll_NotifReceive(): %s %s\n",et,ed);
			return 1;
		}

		printf("JDL: %s\n",stat.jdl);
	}
}
