#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_JOB_HPP__
#define __EDG_WORKLOAD_LOGGING_CLIENT_JOB_HPP__

#ident "$Header$"

#include "edg/workload/common/jobid/JobId.h"
#include "edg/workload/logging/client/Event.h"
#include "edg/workload/logging/client/JobStatus.h"
#include "edg/workload/logging/client/ServerConnection.h"


/**
 * @file Job.h
 * @version $Revision$
 */

EWL_BEGIN_NAMESPACE;

/** L&B job.
 * Implementation of L&B job-specific calls.
 * Connection to the server is maintained transparently.
*/
  
class Job {
public:
  Job(void);
  Job(const edg::workload::common::jobid::JobId &);
  ~Job();
  
  /** Assign new JobId to an existing instance.
   * Connection to server is preserved if possible.
   */
  
  Job & operator= (const edg::workload::common::jobid::JobId &);

/**
 * Status retrieval bitmasks. Used ORed as Job::status() argument,
 * determine which status fields are actually retrieved.
 */
  static const int STAT_CLASSADS;       /**< various job description fields */
  static const int STAT_CHILDREN;       /**< list of subjob JobId's */
  static const int STAT_CHILDSTAT;      /**< apply the flags recursively to subjobs */

  /** Return job status */
  JobStatus status(int) const;
  
  /** Return all events corresponding to this job */
  void log(std::vector<Event> &) const;
  const std::vector<Event> log(void) const;
  
  /** Return last known address of a listener associated to the job.
   * \param name name of the listener
   * \return hostname and port number
   */
  const std::pair<std::string,uint16_t> queryListener(const std::string & name) const;
  
  /** Manipulate LB parameters, the same as for edg_wll_Context in C */
  void setParam(edg_wll_ContextParam, int); 
  void setParam(edg_wll_ContextParam, const std::string); 
  void setParam(edg_wll_ContextParam, const struct timeval &); 

  int getParamInt(edg_wll_ContextParam) const;
  std::string getParamString(edg_wll_ContextParam) const;
  struct timeval getParamTime(edg_wll_ContextParam) const;
  
private:
  ServerConnection	server;
  edg::workload::common::jobid::JobId			jobId;
};

EWL_END_NAMESPACE;

#endif
