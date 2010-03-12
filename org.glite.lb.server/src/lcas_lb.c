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

#include <glite/security/lcas/lcas_modules.h>

static char *modname = "lcas_lb";
static char *authfile = NULL;

int
plugin_initialize(int argc, char *argv[])
{
   int i;

   lcas_log_debug(1, "%s-plugin_initialize(): passed arguments:\n",modname);
   for (i=0; i < argc; i++)
      lcas_log_debug(1, "\targ %d is %s\n", i,argv[i]);

   if (argc > 1)
      authfile = lcas_findfile(argv[1]);

   if (authfile == NULL) {
      lcas_log(0,"\t%s-plugin_initialize() error:"
                 ":access control policy file required!\n",
               modname);
      return LCAS_MOD_NOFILE;
   }

   if (lcas_getfexist(1, authfile) == NULL) {
      lcas_log(0, "\t%s-plugin_initialize() error:"
		  "Cannot find access control policy file: %s\n",
	       modname, authfile);
      return LCAS_MOD_NOFILE;
   }

   return LCAS_MOD_SUCCESS;
}

static char *
get_event_name(lcas_request_t request)
{
   char *rsl = (char *) request;

   if (request == NULL)
      return NULL;

   return strdup(rsl);
}

static int
check_db_file(char *event, char *user_dn)
{
   FILE *db_file = NULL;
   char line[1024];
   int found = 0, inside_block = 0, found_event = 0;
   char *p, *q;
   int ret;

   if (event == NULL || user_dn == NULL)
      return LCAS_MOD_FAIL;

   db_file = fopen(authfile, "r");
   if (db_file == NULL) {
      lcas_log_debug(1, "Failed to open policy file %s: %s\n",
                     authfile, strerror(errno));
      return LCAS_MOD_FAIL;
   }

   ret = LCAS_MOD_FAIL;
   while (fgets(line, sizeof(line), db_file) != NULL) {
      p = strchr(line, '\n');
      if (p)
         *p = '\0';
      p = line;
      if (*p == '#')
         continue;

      while (*p == ' ')
         p++;

      if (inside_block) {
        q = strchr(p, '}');
        if (q)
           *q = '\0';
        if (found_event && ((strcmp(p, user_dn) == 0) || *p == '*')) {
           found = 1;
           break;
        }
        if (q) {
           inside_block = 0;
        }
      } else {
        q = strchr(p, '=');
        if (q == NULL)
           continue;
        *q = '\0';
        inside_block = 1;
        if (strncmp(p, event, strlen(event)) == 0 || *p == '*')
           found_event = 1;
      }
   }
   fclose(db_file);

   if (found)
      ret = LCAS_MOD_SUCCESS;

   lcas_log_debug(1, "access %s\n",
                  (ret == LCAS_MOD_SUCCESS) ? "granted" : "denied");

   return ret;
}

int
plugin_confirm_authorization(lcas_request_t request, lcas_cred_id_t lcas_cred)
{
   char *user_dn;
   char *event = NULL;
   int ret;

   lcas_log_debug(1,"\t%s-plugin: checking LB access policy\n",
		  modname);

   event = get_event_name(request);
   if (event == NULL) {
      lcas_log_debug(1,"\t%s-plugin_confirm_authorization(): no event name specified\n",
                     modname);
      return LCAS_MOD_FAIL;
   }

   user_dn = lcas_get_dn(lcas_cred);
   if (user_dn == NULL) {
      lcas_log(0, "lcas.mod-lcas_get_fabric_authorization() error: user DN empty\n");
      ret = LCAS_MOD_FAIL;
      goto end;
   }

   ret = check_db_file(event, user_dn);

end:
   if (event)
      free(event);

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

#if 0
int
main(int argc, char *argv[])
{
   authfile = "lcas_lb.db";

   check_db_file(argv[1], argv[2]);

   return 0;
}
#endif
