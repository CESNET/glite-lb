#ident "$Header$"
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
#include <unistd.h>

#include "glite/jobid/cjobid.h"
#ifdef BUILDING_LB_CLIENT
#include "producer.h"
#else
#include "glite/lb/producer.h"
#endif
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
   int operation = EDG_WLL_CHANGEACL_ADD;
   int permission = EDG_WLL_CHANGEACL_READ;
   int permission_type = EDG_WLL_CHANGEACL_ALLOW;
   int user_id_type = EDG_WLL_CHANGEACL_DN;
   edg_wlc_JobId jobid;
   int opt;
   int ret;

   if (argc < 3) {
      usage(argv[0]);
      return 1;
   }

   while ((opt=getopt(argc, argv, "rdg")) != -1)
      switch(opt) {
	 case 'r': operation = EDG_WLL_CHANGEACL_REMOVE; break;
	 case 'd': permission_type = EDG_WLL_CHANGEACL_DENY; break;
	 case 'g': user_id_type = EDG_WLL_CHANGEACL_GROUP; break;
	 default:
		   usage(argv[0]);
		   return 1;
		   break;
      }

   if (edg_wll_InitContext(&ctx) != 0) {
      fprintf(stderr, "Couldn't create L&B context.\n");
      return 1;
   }

   if (edg_wlc_JobIdParse(argv[optind], &jobid)) {
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
