#ifndef __GLITE_LB_JOB_HPP__
#define __GLITE_LB_JOB_HPP__

#ident "$Header$"

#include "glite/wmsutils/jobid/JobId.h"

#include "glite/lb/Event.h"

#include "glite/lb/JobStatus.h"
#include "glite/lb/ServerConnection.h"


/**
 * @file Job.h
 * @version $Revision$
 */

EWL_BEGIN_NAMESPACE

/** Class encapsulating the job info stored in the L&B database.
 *
 * This class is the primary interface for getting information about
 * jobs stored in the L&B database. It is constructed from known job
 * id, which uniquely identifies the job as well as the bookkeeping
 * server where the job data is stored. The Job class provides methods
 * for obtaining the data from the bookkeeping server and for setting
 * various parameters of the connection to the bookkeeping server.
 *
 * All query methods have their counterpart in C functions taking
 * taking edg_wll_Context and edg_wll_JobId as their first parameters
 * (in fact, those functions are used to do the actual work).
 */
class Job {
public:
	/** Default constructor.
	 *
	 * Initializes the job as empty, not representing anything.
	 */
	Job(void);
	
	/** Constructor from job id.
	 *
	 * Initializes the job to obtain information for the given job id.
	 * \param[in] jobid 		The job id of the job this object will
	 * represent.
	 * \throws Exception Could not copy the job id.
	 */
	Job(const glite::wmsutils::jobid::JobId &jobid);


	/** Destructor.
	 *
	 * All the actual work is done by member destructors, namely ServerConnection.
	 */
	~Job();
  
	/** Assign new job id  to an existing instance.
	 *
	 * Redirect this instance to obtain information about
	 * different job; connection to the server is preserved, if
	 * possible. 
	 * \param[in] jobid 		New job id.
	 * \returns Reference to this object.
	 * \throws Exception Could not copy the job id.
	 */
	Job & operator= (const glite::wmsutils::jobid::JobId &jobid);

	/*
	 * Status retrieval bitmasks. Used ORed as Job::status() argument,
	 * determine which status fields are actually retrieved.
	 */
	static const int STAT_CLASSADS;       /**< Include the job
					       * description in the
					       * query result. */
	static const int STAT_CHILDREN;       /**< Include the list of
					       * subjob id's in the
					       * query result. */
	static const int STAT_CHILDSTAT;      /**< Apply the flags
					       * recursively to
					       * subjobs.  */

	/** Return job status.
	 *
	 * Obtain the job status (as JobStatus) from the bookkeeping
	 * server.
	 * \param[in] flags 		Specify details of the query.
	 * \returns Status of the job.
	 * \throws Exception Could not query the server.
	 * \see STAT_CLASSADS, STAT_CHILDREN, STAT_CHILDSTAT
	 */
	JobStatus status(int flags) const;
  
	/** Return all events corresponding to this job 
	 * 
	 * Obtain all events corresponding to the job that are stored
	 * in the bookkeeping server database. The maximum number of
	 * returned events can be set by calling setParam().
	 * \param[out] events 		Vector of events (of type Event).
	 * \throws Exception Could not query the server.
	 */
	void log(std::vector<Event> &events) const;

	/** Return all events corresponding to this job 
	 * 
	 * Obtain all events corresponding to the job that are stored
	 * in the bookkeeping server database. The maximum number of
	 * returned events can be set by calling setParam().
	 * \returns Vector of events (of type Event).
	 * \throws Exception Could not query the server.
	 */
	const std::vector<Event> log(void) const;
  
	/** Return last known address of a listener associated to the job.
	 *
	 * Obtains the information about last listener that has been
	 * registered for this job in the bookkeeping server database.
	 * \param[in] name 		Name of the listener.
	 * \returns Hostname and port number of the registered
	 * listener.
	 * \throws Exception Could not query the server.
	 */
	const std::pair<std::string,uint16_t> queryListener(const std::string &name) const;
  
	/** 
	 * Manipulate LB parameters. 
	 *
	 * This method sets integer typed parameters for the server connection.
	 *
	 * \param[in] ctx  		Symbolic name of the parameter to change.
	 * \param[in] val  		New value of the parameter.
	 */
	void setParam(edg_wll_ContextParam ctx, int val); 

	/** 
	 * Manipulate LB parameters. 
	 *
	 * This method sets string typed parameters for the server connection.
	 *
	 * \param[in] ctx  		Symbolic name of the parameter to change.
	 * \param[in] val  		New value of the parameter.
	 */
	void setParam(edg_wll_ContextParam ctx, const std::string val); 

	/** 
	 * Manipulate LB parameters. 
	 *
	 * This method sets timeval typed parameters for the server connection.
	 *
	 * \param[in] ctx  		Symbolic name of the parameter to change.
	 * \param[in] val  		New value of the parameter.
	 */
	void setParam(edg_wll_ContextParam ctx, const struct timeval &val); 

	/** 
	 * Get LB parameters. 
	 *
	 * Obtain value of the named integer parameter.
	 *
	 * \param[in] ctx 		Symbolic name of the paramater to obtain.
	 * \return Value of the parameter.
	 */
	int getParamInt(edg_wll_ContextParam ctx) const;

	/** 
	 * Get LB parameters. 
	 *
	 * Obtain value of the named string parameter.
	 *
	 * \param[in] ctx 		Symbolic name of the paramater to obtain.
	 * \return Value of the parameter.
	 */
	std::string getParamString(edg_wll_ContextParam ctx) const;

	/** 
	 * Get LB parameters. 
	 *
	 * Obtain value of the named timeval parameter.
	 *
	 * \param[in] ctx 		Symbolic name of the paramater to obtain.
	 * \return Value of the parameter.
	 */
	struct timeval getParamTime(edg_wll_ContextParam ctx) const;
  
private:
  ServerConnection	server;
  glite::wmsutils::jobid::JobId			jobId;
};

EWL_END_NAMESPACE

#endif /* __GLITE_LB_JOB_HPP__ */
