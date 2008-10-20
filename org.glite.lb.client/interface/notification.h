#ifndef GLITE_LB_NOTIFICATION_H
#define GLITE_LB_NOTIFICATION_H

#ident "$Header$"

#include "glite/jobid/cjobid.h"
#include "glite/lb/notifid.h"
#include "glite/lb/notif_rec.h"
#include "glite/lb/context.h"

#ifdef BUILDING_LB_CLIENT
#include "consumer.h"
#else
#include "glite/lb/consumer.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \defgroup notifications Notifications handling
 * \brief Notifications handling.
 *@{
 */

/** Register for receiving notifications.
 * Connects to the server specified by EDG_WLL_NOTIF_SERVER context parameter
 * (temporary workaround, should be resolved by registry in future).
 * \param[in,out] context	context to work with
 * \param[in] conditions	the same conditions as for \ref edg_wll_QueryJobsExt.
 * 		currently one or more JOBID's are required.
 * 		Only a single occurence of a specific attribute is allowed
 * 		among ANDed conditions (due to the ability to modify them
 * 		further).
 * \param[in] flags		verbosity of notifications
		0			- send basic job status info
		EDG_WLL_STAT_CLASSADS 	- send also JDL in job status
 * \param[in] fd		= -1 create or reuse the default listening socket (one per context)
 * 	     >= 0 non-default listening socket 
 * \param[in] address_override 	if not NULL, use this address instead of extracting it
 * 		from the connection (useful when multiple interfaces are present,
 * 		circumventing NAT problems etc.)
 * \param[in,out] valid 	until when the registration is valid
				in: 	requested validity (NULL means 'give me server defaults')
				out:	value really set on the server
 * \param[out] id_out		returened NotifId
 * 		the value
 * \retval 0 OK
 * \retval EINVAL restrictions on conditions are not met
 * 
 */
int edg_wll_NotifNew(
	edg_wll_Context		context,
	edg_wll_QueryRec	const * const *conditions,
	int			flags,
	int			fd,
	const char		*address_override,
	edg_wll_NotifId		*id_out,
	time_t			*valid
);


/** Change the receiving local address.
 * Report the new address to the server.
 *
 * \param[in,out] context	context to work with
 * \param[in] id		notification ID you are binding to
 * \param[in] fd		same as for \ref edg_wll_NotifNew 
 * \param[in] address_override 	same as for \ref edg_wll_NotifNew
 * \param[in,out] valid 	same as for \ref edg_wll_NotifNew
 */

int edg_wll_NotifBind(
	edg_wll_Context		context,
	const edg_wll_NotifId	id,
	int			fd,
	const char		*address_override,
	time_t			*valid
);

/** Modify the query conditions for this notification.
 *
 * If op is either EDG_WLL_NOTIF_ADD or EDG_WLL_NOTIF_REMOVE, for the sake
 * of uniqueness the original conditions must have contained only a single
 * OR-ed row of conditions on the attributes infolved in the change.
 * 
 * \param[in,out] context	context to work with
 * \param[in] id		notification ID you are working with
 * \param[in] conditions	same as for \ref edg_wll_NotifNew
 * \param[in] op 		action to be taken on existing conditions,
 * \see edg_wll_NotifChangeOp (defined in common notif_rec.h)
 */
int edg_wll_NotifChange(
	edg_wll_Context		context,
	const edg_wll_NotifId	id,
	edg_wll_QueryRec	const * const * conditions,
	edg_wll_NotifChangeOp	op
);

/** Refresh the registration, i.e. extend its validity period.
 * \param[in,out] context	context to work with
 * \param[in] id		notification ID you are working with
 * \param[in,out] valid 	same as for \ref edg_wll_NotifNew
 */

int edg_wll_NotifRefresh(
	edg_wll_Context		context,
	const edg_wll_NotifId	id,
	time_t			*valid
);

/** Drop the registration.
 * Server is instructed not to send notifications anymore, pending ones
 * are discarded, listening socket is closed, and allocated memory freed.
 * \param[in,out] context	context to work with
 * \param[in] id		notification ID you are working with
 */

int edg_wll_NotifDrop(
	edg_wll_Context		context,
	edg_wll_NotifId		id
);

/** Receive notification.
 * The first incoming notification is returned.
 * \param[in,out] context	context to work with
 * \param[in] fd 		receive on this socket (-1 means the default for the context)
 * \param[in] timeout 		wait atmost this time long. (0,0) means polling, NULL waiting
 * 	indefinitely
 * \param[out] state_out	returned JobStatus
 * \param[out] id_out		returned NotifId
 * \retval 0 notification received, state_out contains the current job state
 * \retval EAGAIN no notification available, timeout occured
 */

int edg_wll_NotifReceive(
	edg_wll_Context		context,
	int			fd,
	const struct timeval	*timeout,
	edg_wll_JobStat		*state_out,
	edg_wll_NotifId		*id_out
);


/** Default socket descriptor where to select(2) for notifications.
 * Even if nothing is available for reading from the socket, 
 * there may be some data cached so calling \ref edg_wll_NotifReceive
 * may return notifications immediately.
 *
 * \param[in,out] context	context to work with
 * \retval >=0 socket descriptor
 * \retval -1 error, details set in context
 */

int edg_wll_NotifGetFd(
	edg_wll_Context		context
);

/** Close the default local listening socket.
 * Useful to force following \ref edg_wll_NotifBind to open
 * a new one.
 * \param[in,out] context	context to work with
 */

int edg_wll_NotifCloseFd(
	edg_wll_Context		context
);

/*
 *@} end of group
 */

#ifdef __cplusplus
}
#endif

#endif /* GLITE_LB_NOTIFICATION_H */
