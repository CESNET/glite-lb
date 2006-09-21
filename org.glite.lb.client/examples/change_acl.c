#ident "$Header$"

#include <stdio.h>
#include <unistd.h>

#include "glite/lb-utils/cjobid.h"
#include "glite/lb/producer.h"
#include "glite/lb/authz.h"

void
usage(const char *me)
{
   fprintf(stderr,"usage: %s [-r] [-d] [-g] jobid user_id\n"
		  "\t-r \tRemove\n"
		  "\t-d \tOperation is considered as `allow' by default, if -d is given 'deny' will be used\n"
		  "\t-g \tuser_id is treated as DN by default, if -g is given user_id is expectedto be of form VO:group\n",

	   me);
}

int
main(int argc, char *argv[])
{
   edg_wll_Context ctx;
   int operation = EDG_WLL_ACL_ADD;
   int permission = EDG_WLL_PERM_READ;
   int permission_type = EDG_WLL_PERM_ALLOW;
   int user_id_type = EDG_WLL_USER_SUBJECT;
   glite_lbu_JobId jobid;
   int opt;
   int ret;

   if (argc < 3) {
      usage(argv[0]);
      return 1;
   }

   while ((opt=getopt(argc, argv, "rdg")) != -1)
      switch(opt) {
	 case 'r': operation = EDG_WLL_ACL_REMOVE; break;
	 case 'd': permission_type = EDG_WLL_PERM_DENY; break;
	 case 'g': user_id_type = EDG_WLL_USER_VOMS_GROUP; break;
	 default:
		   usage(argv[0]);
		   return 1;
		   break;
      }

   edg_wll_InitContext(&ctx);

   if (glite_lbu_JobIdParse(argv[optind], &jobid)) {
      fprintf(stderr,"can't parse job ID\n");
	  goto err;
   }

   edg_wll_SetParam(ctx, EDG_WLL_PARAM_SOURCE, EDG_WLL_SOURCE_USER_INTERFACE);

   ret = edg_wll_ChangeACL(ctx,
			jobid,
			argv[optind+1], user_id_type,
			permission, permission_type,
			operation);

   if (ret) {
      char    *et, *ed;
      edg_wll_Error(ctx, &et, &ed);
      fprintf(stderr, "%s: edg_wll_LogChangeACL() failed: %s (%s)\n",
	      argv[0], et, ed);
	  goto err;
   }

   edg_wll_FreeContext(ctx);
   return 0;

err:
   edg_wll_FreeContext(ctx);
   return 1;
}
