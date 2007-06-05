#ifndef __GLITE_LB_NOTIFICATION_HPP__
#define __GLITE_LB_NOTIFICATION_HPP__


#include "glite/wmsutils/jobid/JobId.h"

#include "consumer.h"
#include "notification.h"
#include "JobStatus.h"


EWL_BEGIN_NAMESPACE


/** Manage LB notifications.
 * Simplified API, covers only a subset of C API functinality
 */

class Notification {
public:
	/** Create an empty object
	 * to be used for new notifications, i.e. with Register()
	 */
	Notification();

	/** Create from server host,port pair
	 * to be used for new notifications, i.e. with Register()
	 * \param[in] host		host
	 * \param[in] port		port
	 */
	Notification(const std::string host,const u_int16_t port);

	/** Create from NotifId
	 * to be used for existing notifications, i.e. with Bind()
	 * \param[in] notifId		NotifId
	 */
	Notification(const std::string notifId);

	~Notification();

	std::string getNotifId() const;	/**< retrieve NotifId */
	time_t getValid() const;        /**< retrieve time until when it is valid */
	int getFd() const;              /**< retrieve local listener filedescriptor */

	/** Add this job to the list.
	 * Local operation only, Register() has to be called
	 * to propagate changes to server 
	 * \param[in] jobId		JobId
	 */
	void addJob(const glite::wmsutils::jobid::JobId &jobId); 

	/** Remove job from the list, local op again. 
	 * \param[in] jobId		JobId
	 */
	void removeJob(const glite::wmsutils::jobid::JobId &jobId);

	/** Get jobs on the list */
	std::string getJobs();

	/** Receive notifications on these states */
	void setStates(const std::vector<glite::lb::JobStatus::Code> &);

	/** Get states */
	std::string getStates();

	/** Register (or re-register, i.e. change and extend) 
	 * with the server
	 */
	void Register();

	/** Bind to the existing notification at the server
	 * i.e. change the receiving local address
	 * \param[in] address		address override
	 */
	void Bind(const std::string address);

	/** Receive notification.
	 * Blocks at most the specified timeout (maybe 0 for local polling).
	 * \retval 0 OK
	 * \retval 1 timeout 
	 */
	int receive(glite::lb::JobStatus &,timeval &);

private:
	std::vector<glite::wmsutils::jobid::JobId>	jobs;
	std::vector<glite::lb::JobStatus::Code>	states;

	edg_wll_Context	ctx;
	edg_wll_NotifId	notifId;
	time_t		valid;
};


EWL_END_NAMESPACE

#endif /* __GLITE_LB_NOTIFICATION_HPP__ */
