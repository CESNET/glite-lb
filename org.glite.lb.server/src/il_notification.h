#ifndef GLITE_LB_IL_NOTIFICATION_H
#define GLITE_LB_IL_NOTIFICATION_H

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


/* needed for the edg_wll_NotifId */
#include "glite/lb/notifid.h"

/* import the edg_wll_JobStat structure */
#include "glite/lb/jobstat.h"

#ifdef __cplusplus
#extern "C" {
#endif

extern char *notif_ilog_socket_path;
extern char *notif_ilog_file_prefix;

/** Send ULM notification string to interlogger.
 * Stores notification to file according to registration id and send it
 * to interlogger using local socket.
 * \param reg_id  registration id
 * \param host,port address to deliver the notification to.
 *                  If NULL, it means no further notifications will
 *                  follow (the client has unregistered). It always
 *                  overrides previous values (ie. changes the
 *                  reg_id->client address mapping in interlogger).
 * \param owner DN of the registration owner, this will be verified
 *              against client's certificate
 * \param notif_data ULM formatted notification string, may be NULL,
 *                   if there is nothing to be sent to client.
 * \retval 0 OK
 * \retval EINVAL	bad jobId, unknown event code, or the format
 *                      string together with the remaining arguments
 *                      does not form a valid event 
 * \retval ENOSPC	unable to accept the event due to lack of disk
 *                      space etc. 
 * \retval ENOMEM	failed to allocate memory
 * \retval EAGAIN	non blocking return from the call, the event
 *                      did not come through socket, but is backed up
 *                      in file
 */
int
edg_wll_NotifSend(edg_wll_Context       context,
	          edg_wll_NotifId       reg_id,
		  const char           *dest_url,
		  const char           *owner,
		  int			expires,
                  const char           *notif_data);


/** Send job status notification.
 * Creates ULM notification string and sends it using
 * edg_wll_NotifSend(). The job status is encoded into XML and escaped
 * before inclusion into ULM.
 * \param reg_id registration id
 * \param host,port address to deliver the notification to.
 * \param owner DN of the registration owner, this will be verified
 *              against client's certificate
 * \param flags verbosity of returned status 
 * \param notif_job_stat structure describing job status
 * \see edg_wll_NotifSend()
 */
int
edg_wll_NotifJobStatus(edg_wll_Context	context,
		       edg_wll_NotifId	reg_id,
		       const char      *dest_url,
		       const char      *owner,
                       int              flags,
		       int		expires,
		       const edg_wll_JobStat notif_job_stat);


/** Change address for notification delivery.
 * Creates ULM string and uses edg_wll_NotifSend() to pass it
 * to interlogger.
 * \param reg_id registration id
 * \param host,port new delivery address
 * \see edg_wll_NotifSend()
 */
int 
edg_wll_NotifChangeIL(edg_wll_Context context,
                               edg_wll_NotifId reg_id,
                               const char      *dest_url,
			       int		expires);

/** Cancel registration.
 * Creates ULM string and uses edg_wll_NotifSend() to pass it to
 * interlogger.
 * \param reg_id registration id
 */
int
edg_wll_NotifCancelRegId(edg_wll_Context context,
			 edg_wll_NotifId reg_id);


/**
 * Check and perform notifications.
 */
int edg_wll_NotifMatch(edg_wll_Context context, const edg_wll_JobStat *oldstat, const edg_wll_JobStat *stat);

/**
 * Check permissions on job status.
 */
int edg_wll_NotifCheckAuthz(edg_wll_Context ctx,edg_wll_JobStat *stat,int flags,const char *recip);

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_IL_NOTIFICATION_H */
