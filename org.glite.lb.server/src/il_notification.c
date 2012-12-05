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

/**
 * il_notification.c
 *   - implementation of IL API calls for notifications
 *
 */
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include "glite/lbu/escape.h"
#include "glite/lb/context-int.h"
#include "glite/lb/notifid.h"
#include "glite/lb/events_parse.h"
#include "glite/lb/log_proto.h"
#include "glite/lbu/log.h"

#include "il_notification.h"
#include "lb_xml_parse.h"
#include "authz_policy.h"


#define FCNTL_ATTEMPTS		5
#define FCNTL_TIMEOUT		1
#define NOTIF_TIMEOUT		1

char *notif_ilog_socket_path = EDG_WLL_NOTIF_SOCKET_DEFAULT;
char *notif_ilog_file_prefix = EDG_WLL_NOTIF_PREFIX_DEFAULT;


static
int
notif_create_ulm(
	edg_wll_Context	context,
	edg_wll_NotifId	reg_id,
	const char	*dest_url,
	const char	*owner,
	int		expires,
	const char	*notif_data,
	char		**ulm_data,
	char		**reg_id_s)
{
	int		ret;
	edg_wll_Event	*event=NULL;
	char		*host = NULL;
	uint16_t	port = 0;

	*ulm_data = NULL;
	*reg_id_s = NULL;

	event = edg_wll_InitEvent(EDG_WLL_EVENT_NOTIFICATION);

	gettimeofday(&event->any.timestamp,0);
	if (context->p_host) event->any.host = strdup(context->p_host);
	event->any.level = context->p_level;
	event->any.source = context->p_source;
	if (context->p_instance) event->notification.src_instance = strdup(context->p_instance);
	event->notification.notifId = edg_wll_NotifIdDup(reg_id);
	if (owner) event->notification.owner = strdup(owner);
	if (dest_url) {
		if (strstr(dest_url, "//")) {
		// using complete URL
			event->notification.dest_url = strdup(dest_url);
		} else {
		// using plain host:port
			host = strrchr(dest_url, ':');
			port = atoi(host+1);
			if ( !(host = strndup(dest_url, host-dest_url)) )
			{
				edg_wll_SetError(context, ret = errno, "updating notification records");
				goto out;
			}
			event->notification.dest_host = host;
			event->notification.dest_port = port;
			host = NULL;
		}
	}
	if (notif_data) event->notification.jobstat = strdup(notif_data);

	event->notification.expires = expires;

	if ((*ulm_data = edg_wll_UnparseNotifEvent(context,event)) == NULL) {
		edg_wll_SetError(context, ret = ENOMEM, "edg_wll_UnparseNotifEvent()"); 
		goto out;
	}

	if((*reg_id_s = edg_wll_NotifIdGetUnique(reg_id)) == NULL) {
		edg_wll_SetError(context, ret = ENOMEM, "edg_wll_NotifIdGetUnique()");
		goto out;
	}

	ret = 0;

out:
	if(event) { 
		edg_wll_FreeEvent(event);
		free(event);
	}
	if(ret) edg_wll_UpdateError(context, ret, "notif_create_ulm()");
	return(ret);
}


int
edg_wll_NotifSend(edg_wll_Context       context,
	          edg_wll_NotifId       reg_id,
		  const char           *dest_url,
		  const char           *owner,
		  int			expires,
                  const char           *notif_data)
{
	struct timeval	timeout = {NOTIF_TIMEOUT, 0};
	int				ret;
	long			filepos;
	char		   *ulm_data,
				   *reg_id_s,
				   *event_file = NULL;

	if((ret=notif_create_ulm(context, 
				 reg_id, 
				 dest_url,
				 owner, 
				 expires,
				 notif_data,
				 &ulm_data,
				 &reg_id_s))) {
		goto out;
	}

	asprintf(&event_file, "%s.%s", notif_ilog_file_prefix, reg_id_s);
	if(event_file == NULL) {
		edg_wll_SetError(context, ret=ENOMEM, "asprintf()");
		goto out;
	}

	if ( (ret = edg_wll_log_event_write(context, event_file, ulm_data,
					FCNTL_ATTEMPTS, FCNTL_TIMEOUT, &filepos)) ) {
		edg_wll_UpdateError(context, 0, "edg_wll_log_event_write()");
		goto out;
	}

	if ( (ret = edg_wll_log_event_send(context, notif_ilog_socket_path,
					filepos, ulm_data, strlen(ulm_data), 1, &timeout)) ) {
		edg_wll_UpdateError(context, 0, "edg_wll_log_event_send()");
		goto out;
	}

	ret = 0;

out:
	free(event_file);
	free(ulm_data);
	free(reg_id_s);
	if(ret) edg_wll_UpdateError(context, ret, "edg_wll_NotifSend()");
	return(ret);
}


int
edg_wll_NotifJobStatus(edg_wll_Context	context,
		       edg_wll_NotifId	reg_id,
		       const char      *dest_url,
		       const char      *owner,
                       int		flags,
		       int		expires,
		       const edg_wll_JobStat notif_job_stat,
		       int		authz_flags)
{
	int 		ret=0;
	char   		*xml_data, *xml_esc_data=NULL;
	edg_wll_JobStat	stat = notif_job_stat;

	
	if (flags == 0) {
		stat.jdl = NULL;
		stat.matched_jdl = NULL;
		stat.condor_jdl = NULL;
		stat.rsl = NULL;
	}

	if(edg_wll_JobStatusToXML(context, stat, &xml_data)) 
		goto out;
	
	if((xml_esc_data = glite_lbu_EscapeXML(xml_data)) == NULL) {
		edg_wll_SetError(context, ret=ENOMEM, "glite_lbu_EscapeXML()");
		goto out;
	}

	if ((ret=edg_wll_NotifSend(context, reg_id, dest_url, owner, expires, xml_esc_data))) {
		char *ed = NULL, *et = NULL;

		if(ret) edg_wll_UpdateError(context, ret, "edg_wll_NotifJobStatus()");
		edg_wll_Error(context,&et,&ed);
		glite_common_log(LOG_CATEGORY_CONTROL, LOG_PRIORITY_WARN, 
			"%s - %s\n", ed, et);
		edg_wll_ResetError(context);
		free(et); 
		free(ed);
	}

out:
	if(xml_data) free(xml_data);
	if(xml_esc_data) free(xml_esc_data);
	return(edg_wll_Error(context,NULL,NULL));
}


int 
edg_wll_NotifChangeIL(edg_wll_Context context,
                               edg_wll_NotifId reg_id,
                               const char      *dest_url,
			       int	       expires)
{
	return(edg_wll_NotifSend(context, reg_id, dest_url, context->peerName, expires, ""));
}


int
edg_wll_NotifCancelRegId(edg_wll_Context context,
			 edg_wll_NotifId reg_id)
{
/* XXX: Jan 1 1970 00:00:01 -- quite sure to make it expire immediately */
	return(edg_wll_NotifSend(context, reg_id, NULL, "", 1, ""));
}

