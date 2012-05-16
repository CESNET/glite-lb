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


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcas/lcas_modules.h"
#include "lcas/lcas_utils.h"
#ifndef NO_GLOBUS_GSSAPI
#include "voms/voms_apic.h"
#endif
#include <glite/lb/context.h>
#include "authz_policy.h"
#include "lb_authz.h"

static char *modname = "lcas_lb";
static char *authfile = NULL;

int
plugin_initialize(int argc, char *argv[])
{
   lcas_log_debug(1, "%s-plugin_initialize()\n",modname);

   return LCAS_MOD_SUCCESS;
}

int
plugin_confirm_authorization(lcas_request_t request, lcas_cred_id_t lcas_cred)
{
   char *user_dn;
   int ret;
   edg_wll_Context ctx;
   struct _edg_wll_GssPrincipal_data princ;
   X509 *cert = NULL;
   STACK_OF(X509) * chain = NULL;
   void *cred = NULL;
   struct vomsdata *voms_info = NULL;
   int err;
   authz_action action;

   memset(&princ, 0, sizeof(princ));

   lcas_log_debug(1,"\t%s-plugin: checking LB access policy\n",
		  modname);

   if (edg_wll_InitContext(&ctx) != 0) {
     lcas_log(0, "Couldn't create L&B context\n");
     ret = LCAS_MOD_FAIL;
     goto end;
   }

   if ((action = find_authz_action(request)) == ACTION_UNDEF) {
      lcas_log(0, "lcas.mod-lb() error: unsupported action\n");
      ret = LCAS_MOD_FAIL;
      goto end;
   }

   user_dn = lcas_get_dn(lcas_cred);
   if (user_dn == NULL) {
      lcas_log(0, "lcas.mod-lb() error: user DN empty\n");
      ret = LCAS_MOD_FAIL;
      goto end;
   }
   princ.name = user_dn;

   cred = lcas_get_gss_cred(lcas_cred);
   if (cred == NULL) {
      lcas_log(0, "lcas.mod-lb() warning: user gss credential empty\n");
#if 0
      ret = LCAS_MOD_FAIL;
      goto end;
#endif
   }

#ifndef NO_GLOBUS_GSSAPI
   if (cred) {
      voms_info = VOMS_Init(NULL, NULL);
      if (voms_info == NULL) {
	  lcas_log(0, "lcas.mod-lb() failed to initialize VOMS\n");
	      ret = LCAS_MOD_FAIL; 
	      goto end;
      }

      ret = VOMS_RetrieveFromCred(cred, RECURSE_CHAIN, voms_info, &err);
      if (ret == 1)
          edg_wll_get_fqans(ctx, voms_info, &princ.fqans);
   }
#endif

   ret = check_authz_policy(edg_wll_get_server_policy(), &princ, action);
   ret = (ret == 1) ? LCAS_MOD_SUCCESS : LCAS_MOD_FAIL;

end:
   edg_wll_FreeContext(ctx);
#ifndef NO_GLOBUS_GSSAPI
   if (voms_info)
      VOMS_Destroy(voms_info);
#endif
   if (cert)
      X509_free(cert);
   if (chain)
      sk_X509_pop_free(chain, X509_free);

   return ret; 
}

int
plugin_terminate()
{
   lcas_log_debug(1, "%s-plugin_terminate(): terminating\n",modname);

   if (authfile) {
      free(authfile);
      authfile = NULL;
   }

   return LCAS_MOD_SUCCESS;
}
