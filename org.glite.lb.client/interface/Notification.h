#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_NOTIFICATION_HPP__
#define __EDG_WORKLOAD_LOGGING_CLIENT_NOTIFICATION_HPP__

#include "edg/workload/logging/client/consumer.h"
#include "edg/workload/logging/client/notification.h"

#include "edg/workload/common/jobid/JobId.h"
#include "edg/workload/logging/client/JobStatus.h"


EWL_BEGIN_NAMESPACE; 


/** Manage LB notifications.
 * Simplified API, covers only a subset of C API functinality
 */

class Notification {
public:
	Notification();

	/** Create from NotifId */
	Notification(const std::string);

	/** Create from server,port pair */
	Notification(const std::string,const u_int16_t);

	~Notification();

	std::string getNotifId() const;	/**< retrieve NotifId */
	time_t getValid() const;	/**< until when it is valid */
	int getFd() const;		/**< local listener filedescriptor */

	/** Add this job to the list.
	 * Local operation only, Register() has to be called
	 * to propagate changes to server 
	 */
	void addJob(const edg::workload::common::jobid::JobId &); 

	/** Remove job from the list, local op again. */
	void removeJob(const edg::workload::common::jobid::JobId &);

	/** Get jobs on the list */
	std::string getJobs();

	/** Receive notifications on these states */
	void setStates(const std::vector<edg::workload::logging::client::JobStatus::Code> &);

	/** Get states */
	std::string getStates();

	/** Register (or re-register, i.e. change and extend) 
	 * with the server
	 */
	void Register();

	/** Receive notification.
	 * Blocks at most the specified timeout (maybe 0 for local polling).
	 * \retval 0 OK
	 * \retval 1 timeout 
	 */
	int receive(edg::workload::logging::client::JobStatus &,timeval &);

private:
	std::vector<edg::workload::common::jobid::JobId>	jobs;
	std::vector<edg::workload::logging::client::JobStatus::Code>	states;

	edg_wll_Context	ctx;
	edg_wll_NotifId	notifId;
	time_t		valid;
};


EWL_END_NAMESPACE;

#endif
