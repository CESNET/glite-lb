#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "glite/lb-utils/cjobid.h"
#include "glite/lb/notifid.h"

int edg_wll_NotifIdParse(const char *s,edg_wll_NotifId *n)
{
	glite_lbu_JobId	j;
	int		ret = glite_lbu_JobIdParse(s,&j);

	if (!ret) *n = (edg_wll_NotifId) j; 
	return ret;
}

char* edg_wll_NotifIdUnparse(const edg_wll_NotifId n)
{
	return glite_lbu_JobIdUnparse((glite_lbu_JobId) n);
}

int edg_wll_NotifIdCreate(const char *server,int port,edg_wll_NotifId *n)
{
	glite_lbu_JobId	j,j2;
	int 	ret = glite_lbu_JobIdCreate(server,port,&j);
	char	*u,*u2;

	if (!ret) {
		u = glite_lbu_JobIdGetUnique(j);
		asprintf(&u2,"NOTIF:%s",u);
		free(u);
		ret = glite_lbu_JobIdRecreate(server,port,u2,&j2);
		free(u2);
		glite_lbu_JobIdFree(j);
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

	edg_wll_NotifIdGetServerParts(*((glite_lbu_JobId *)n), &srv, &port);
	ret = glite_lbu_JobIdRecreate(srv, port, aux, (glite_lbu_JobId *)n);
	free(aux);
	free(srv);

	return ret;
}

void edg_wll_NotifIdFree(edg_wll_NotifId n)
{
	glite_lbu_JobIdFree((glite_lbu_JobId) n);
}

void edg_wll_NotifIdGetServerParts(const edg_wll_NotifId notifId, char **srvName, unsigned int *srvPort)
{
	glite_lbu_JobIdGetServerParts((glite_lbu_JobId) notifId, srvName, srvPort);
}

char* edg_wll_NotifIdGetUnique(const edg_wll_NotifId notifid)
{
	char	   *id = glite_lbu_JobIdGetUnique((glite_lbu_JobId)notifid);

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
