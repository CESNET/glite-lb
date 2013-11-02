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
#include <stdlib.h>
#include <string.h>

#include "glite/jobid/cjobid.h"
#include "notifid.h"

int edg_wll_NotifIdParse(const char *s,edg_wll_NotifId *n)
{
	edg_wlc_JobId	j;
	int		ret = edg_wlc_JobIdParse(s,&j);

	if (!ret) *n = (edg_wll_NotifId) j; 
	return ret;
}

char* edg_wll_NotifIdUnparse(const edg_wll_NotifId n)
{
	return edg_wlc_JobIdUnparse((edg_wlc_JobId) n);
}

int edg_wll_NotifIdCreate(const char *server,int port,edg_wll_NotifId *n)
{
	edg_wlc_JobId	j,j2;
	int 	ret = edg_wlc_JobIdCreate(server,port,&j);
	char	*u,*u2;

	if (!ret) {
		u = edg_wlc_JobIdGetUnique(j);
		asprintf(&u2,"NOTIF:%s",u);
		free(u);
		ret = edg_wlc_JobIdRecreate(server,port,u2,&j2);
		free(u2);
		edg_wlc_JobIdFree(j);
		if (!ret) *n = (edg_wll_NotifId) j2;
	}

	return ret;
}

int edg_wll_NotifIdSetUnique(edg_wll_NotifId *n, const char *un)
{
	char		   *aux, *srv;
	int				ret;
	unsigned int	port;


	asprintf(&aux, "NOTIF:%s", un);
	if ( !aux )
		return -1;

	edg_wll_NotifIdGetServerParts(*((edg_wlc_JobId *)n), &srv, &port);
	ret = edg_wlc_JobIdRecreate(srv, port, aux, (edg_wlc_JobId *)n);
	free(aux);
	free(srv);

	return ret;
}

void edg_wll_NotifIdFree(edg_wll_NotifId n)
{
	edg_wlc_JobIdFree((edg_wlc_JobId) n);
}

void edg_wll_NotifIdGetServerParts(const edg_wll_NotifId notifId, char **srvName, unsigned int *srvPort)
{
	edg_wlc_JobIdGetServerParts((edg_wlc_JobId) notifId, srvName, srvPort);
}

char* edg_wll_NotifIdGetUnique(const edg_wll_NotifId notifid)
{
	char	   *id = edg_wlc_JobIdGetUnique((edg_wlc_JobId)notifid);

	if ( id )
	{
		char *s = strchr(id, ':');

		if ( s )
		{
    		char *ret = strdup(s+1);
			free(id);
			return ret;
		}
	}

	free(id);
	return NULL;
}

edg_wll_NotifId *
edg_wll_NotifIdDup(const edg_wll_NotifId src)
{
   char *str;
   edg_wll_NotifId id = NULL;
   int ret;

   str = edg_wll_NotifIdUnparse(src);
   if (str == NULL)
      return NULL;

   ret = edg_wll_NotifIdParse((const char *)str, &id);
   free(str);

   return (ret == 0) ? id : NULL;
}
