#include <stdio.h>
#include <string.h>

#include "notification.h"

int main(int argc,char **argv)
{
	edg_wll_Context	ctx;
	edg_wll_QueryRec	chj[2], chs[2], *ch2[3] = { chj, chs, NULL };
	char	*et,*ed;
	edg_wll_NotifId	id;
	edg_wll_JobStat	stat;
	time_t	valid = 0;

	edg_wll_InitContext(&ctx);

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
