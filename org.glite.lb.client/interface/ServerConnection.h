#ifndef __EDG_WORKLOAD_LOGGING_CLIENT_SERVERCONNECTION_HPP__
#define __EDG_WORKLOAD_LOGGING_CLIENT_SERVERCONNECTION_HPP__

#ident "$Header$"

/**
 * @file ServerConnection.h
 * @version $Revision$
 */

#include <string.h>
#include <list>

#include "edg/workload/common/jobid/JobId.h"
#include "edg/workload/logging/client/Event.h"
#include "edg/workload/logging/client/JobStatus.h"

#include "edg/workload/logging/client/consumer.h"

EWL_BEGIN_NAMESPACE;

/** Auxiliary class to hold an atomic query condition. */
class QueryRecord {
public:
	friend class ServerConnection;
	friend edg_wll_QueryRec *convertQueryVector(const std::vector<QueryRecord> &in);

	/* IMPORTANT: must match lbapi.h */
	enum Attr {
		UNDEF=0,	/**< Not-defined value, used to terminate lists etc. */
		JOBID,	        /**< Job Id \see _edg_wll_QueryRec */
		OWNER,	        /**< Job owner \see _edg_wll_QueryRec */
		STATUS,	        /**< Current job status */
		LOCATION,	/**< Where is the job processed */
		DESTINATION,	/**< Destination CE */
		DONECODE,	/**< Minor done status (OK,fail,cancel) */
		USERTAG,	/**< User tag (not implemented yet) */
		TIME,	        /**< Timestamp \see _edg_wll_QueryRec */
		LEVEL,	        /**< Logging level (see "dglog.h") * \see _edg_wll_QueryRec */
		HOST,	        /**< Where the event was generated */
		SOURCE,	        /**< Source component */
		INSTANCE,	/**< Instance of the source component */
		EVENT_TYPE,	/**< Event type \see _edg_wll_QueryRec */
		CHKPT_TAG,	/**< Checkpoint tag */
		RESUBMITTED,	/**< Job was resubmitted */
		PARENT,	        /**< Job was resubmitted */
		EXITCODE,	/**< Unix exit code */
	};

	enum Op {
		EQUAL=EDG_WLL_QUERY_OP_EQUAL,
		LESS=EDG_WLL_QUERY_OP_LESS,
		GREATER=EDG_WLL_QUERY_OP_GREATER,
		WITHIN=EDG_WLL_QUERY_OP_WITHIN,
		UNEQUAL=EDG_WLL_QUERY_OP_UNEQUAL
	};
  
	QueryRecord();

	/* copy and assignment */
	QueryRecord(const QueryRecord &);
	QueryRecord& operator=(const QueryRecord &);

	/* constructors for simple attribute queries */
	QueryRecord(const Attr, const Op, const std::string &);
	QueryRecord(const Attr, const Op, const int);
	QueryRecord(const Attr, const Op, const struct timeval &);
	QueryRecord(const Attr, const Op, const edg::workload::common::jobid::JobId&);
	/* this one is for attr==TIME and particular state */
	QueryRecord(const Attr, const Op, const int, const struct timeval &);
	
	/* constructors for WITHIN operator */
	QueryRecord(const Attr, const Op, const std::string &, const std::string &);
	QueryRecord(const Attr, const Op, const int, const int);
	QueryRecord(const Attr, const Op, const struct timeval &, const struct timeval &);
	QueryRecord(const Attr, const Op, const int, const struct timeval &, const struct timeval &);

	/* convenience for user tags */
	QueryRecord(const std::string &, const Op, const std::string &);
	QueryRecord(const std::string &, const Op, const std::string &, const std::string &);
	
	~QueryRecord();
  
	static const std::string AttrName(const Attr) ;
  
protected:

	/* conversion to C API type */
	operator edg_wll_QueryRec() const;

private:
	Attr	attr;
	Op	oper;
	std::string tag_name;
	int state;
	std::string string_value;
	edg::workload::common::jobid::JobId jobid_value;
        int     int_value;
        struct timeval timeval_value;
	std::string string_value2;
        int     int_value2;
        struct timeval timeval_value2;
};


/** Supported aggregate operations */
enum AggOp { AGG_MIN=1, AGG_MAX, AGG_COUNT };


/**
 * Connection to the L&B server.
 * Maintain connection to the server.
 * Implement non job-specific API calls
 */

class ServerConnection {
public:
	friend class Job;

	ServerConnection(void);

	/* DEPRECATED: do not use
	 * connections are now handled automagically inside the implementation
	 */
	ServerConnection(const std::string &);

	/** Open connection to a given server */
	void open(const std::string &);

	/** Close the current connection */
	void close(void);

	/* END DEPRECATED */

	/* set & get parameter methods */

	/* consumer parameter settings */
	void setQueryServer(const std::string&, int);
	void setQueryTimeout(int);

	void setX509Proxy(const std::string&);
	void setX509Cert(const std::string&, const std::string&);

	std::pair<std::string, int> getQueryServer() const;
	int getQueryTimeout() const;

	std::string getX509Proxy() const;
	std::pair<std::string, std::string> getX509Cert() const;

	/* end of set & get */

	virtual ~ServerConnection();


	/* consumer API */

	/** Retrieve the set of single indexed attributes.
	 * outer vector elements correspond to indices
	 * inner vector elements correspond to index columns
	 * if .first of the pair is USERTAG, .second is its name
	 * if .first is TIME, .second is state name
	 * otherwise .second is meaningless (empty string anyway)
	 */
	std::vector<std::vector<std::pair<QueryRecord::Attr,std::string> > >
		getIndexedAttrs(void);

	/** Retrieve hard and soft result set size limit */
	std::pair<int,int> getLimits(void) const;

	/** Set the soft result set size limit */
	void setQueryJobsLimit(int);
	void setQueryEventsLimit(int);
  
	/** Retrieve all events satisfying the query records
	 * @param job_cond, event_cond - vectors of conditions to be satisfied 
	 *  by jobs as a whole or particular events, conditions are ANDed
	 * @param events vector of returned events
	 */
	void queryEvents(const std::vector<QueryRecord>& job_cond,
			 const std::vector<QueryRecord>& event_cond,
			 std::vector<Event>&) const;
  
	const std::vector<Event> queryEvents(const std::vector<QueryRecord>& job_cond,
					     const std::vector<QueryRecord>& event_cond) const;

	const std::list<Event> queryEventsList(const std::vector<QueryRecord>& job_cond,
				               const std::vector<QueryRecord>& event_cond) const;


	/** The same as queryEvents but return only an aggregate.
	 * @param job_cond, event_cond - vectors of conditions to be satisfied 
	 *  by jobs as a whole or particular events, conditions are ANDed
	 * @param op aggregate operator to apply
	 * @param attr attribute to apply the operation to
	 */
	std::string queryEventsAggregate(const std::vector<QueryRecord>& job_cond,
					 const std::vector<QueryRecord>& event_cond,
					 enum AggOp const op,
					 std::string const attr) const;
  

	/** Retrieve all events satisfying the query records
	 * @param job_cond, event_cond - vectors of vectors of job or event conditions, 
	 * respectively. The inner vectors are logically ANDed, the outer are ORed
	 * (cond1 AND cond2 AND ...) OR (condN AND ...)
	 * @param events vector of returned events
	 */
	void queryEvents(const std::vector<std::vector<QueryRecord> >& job_cond,
			 const std::vector<std::vector<QueryRecord> >& event_cond,
			 std::vector<Event>&) const;
  
	const std::vector<Event> 
	queryEvents(const std::vector<std::vector<QueryRecord> >& job_cond,
		    const std::vector<std::vector<QueryRecord> >& event_cond) const;

	
	/** Retrieve jobs satisfying the query records, including their states
	 * @param query vector of Query records that are anded to form the
	 * 	query
	 * @param ids vector of returned job id's
	 * @param states vector of returned job states
	 */
  
	void queryJobs(const std::vector<QueryRecord>& query,
		       std::vector<edg::workload::common::jobid::JobId>& ids) const;
  
	const std::vector<edg::workload::common::jobid::JobId>
	queryJobs(const std::vector<QueryRecord>& query) const;
  
	
	/** Retrieve jobs satisfying the query records, including their states
	 * @param query vector of Query record vectors that are ORed and ANDed to form the
	 * 	query
	 * @param ids vector of returned job id's
	 * @param states vector of returned job states
	 */
   
	void queryJobs(const std::vector<std::vector<QueryRecord> >& query,
		       std::vector<edg::workload::common::jobid::JobId>& ids) const;
  
	const std::vector<edg::workload::common::jobid::JobId>
	queryJobs(const std::vector<std::vector<QueryRecord> >& query) const;

	/** Retrieve jobs satisfying the query records, including status
	 * information
	 * @param query vector of Query records that are anded to form the
	 * 	query
	 * @param ids vector of returned job id's
	 * @param states vector of returned job states
	 */
	void queryJobStates(const std::vector<QueryRecord>& query, 
			    int flags,
			    std::vector<JobStatus> & states) const;
	const std::vector<JobStatus>  queryJobStates(const std::vector<QueryRecord>& query, 
						     int flags) const;

	const std::list<JobStatus>  queryJobStatesList(const std::vector<QueryRecord>& query,
						     int flags) const;
  
	/** Retrieve jobs satisfying the query records, including status
	 * information
	 * @param query vector of Query records that are anded to form the
	 * 	query
	 * @param ids vector of returned job id's
	 * @param states vector of returned job states
	 */
	void queryJobStates(const std::vector<std::vector<QueryRecord> >& query, 
			    int flags,
			    std::vector<JobStatus> & states) const;
	const std::vector<JobStatus>  
	queryJobStates(const std::vector<std::vector<QueryRecord> >& query, 
		       int flags) const;
  
	/** States of all user's jobs.
	 * Convenience wrapper around queryJobs.
	 */
	void userJobStates(std::vector<JobStatus>& stateList) const;
	const std::vector<JobStatus> userJobStates() const;
  
  
	/** JobId's of all user's jobs.
	 * Convenience wrapper around queryJobs.
	 */
	void userJobs(std::vector<edg::workload::common::jobid::JobId> &) const;
	const std::vector<edg::workload::common::jobid::JobId> userJobs() const;

	/** Manipulate LB parameters, the same as for edg_wll_Context in C */
	void setParam(edg_wll_ContextParam, int); 
	void setParam(edg_wll_ContextParam, const std::string); 
	void setParam(edg_wll_ContextParam, const struct timeval &); 

	int getParamInt(edg_wll_ContextParam) const;
	std::string getParamString(edg_wll_ContextParam) const;
	struct timeval getParamTime(edg_wll_ContextParam) const;
  
protected:

	edg_wll_Context getContext(void) const;

private:
	edg_wll_Context context;
};

EWL_END_NAMESPACE;

#endif
