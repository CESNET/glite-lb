#ifndef GLITE_LB_SERVERCONNECTION_HPP
#define GLITE_LB_SERVERCONNECTION_HPP

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
 * @file ServerConnection.h
 * @version $Revision$
 */

#include <string.h>
#include <list>

#include "glite/jobid/JobId.h"


#ifdef BUILDING_LB_CLIENT
#include "Event.h"
#include "JobStatus.h"
#include "consumer.h"
#else
#include "glite/lb/Event.h"
#include "glite/lb/JobStatus.h"
#include "glite/lb/consumer.h"
#endif

EWL_BEGIN_NAMESPACE

/** Auxiliary class to hold atomic query condition. 
 *
 * This class is used to construct queries to the L&B database. Each
 * query is composed of multiple atomic conditions in the form of
 * 'attribute' 'predicate' 'value'. QueryRecord represents such an
 * atomic condition. 
 */
class QueryRecord {
public:
	friend class ServerConnection;
	friend edg_wll_QueryRec *convertQueryVector(const std::vector<QueryRecord> &in);

	/* IMPORTANT: must match lbapi.h */
	/** Symbolic names of queryable attributes. 
	 *
	 * The queryable attributes correspond to the table columns in
	 * the bookkeeping server database, they relate both to the
	 * event records  and job records.
	 * \see Event::Attr
	 */
	enum Attr {
		UNDEF=0,	/**< Not-defined value, used to terminate lists etc. */
		JOBID = EDG_WLL_QUERY_ATTR_JOBID,	        /**< Job id. */
		OWNER = EDG_WLL_QUERY_ATTR_OWNER,	        /**< Job owner (certificate subject). */
		STATUS = EDG_WLL_QUERY_ATTR_STATUS,	        /**< Current job status code. */
		LOCATION = EDG_WLL_QUERY_ATTR_LOCATION,	/**< Where is the job being processed. */
		DESTINATION = EDG_WLL_QUERY_ATTR_DESTINATION,	/**< Destination CE. */
		DONECODE = EDG_WLL_QUERY_ATTR_DONECODE,	/**< Minor done status (OK,fail,cancel). */
		USERTAG = EDG_WLL_QUERY_ATTR_USERTAG,	/**< User tag. */
		TIME = EDG_WLL_QUERY_ATTR_TIME,	        /**< Timestamp of the event. */
		LEVEL = EDG_WLL_QUERY_ATTR_LEVEL,	        /**< Logging level. */
		HOST = EDG_WLL_QUERY_ATTR_HOST,	        /**< Hostname where the event was generated. */
		SOURCE = EDG_WLL_QUERY_ATTR_SOURCE,	        /**< Source component that sent the event. */
		INSTANCE = EDG_WLL_QUERY_ATTR_INSTANCE,	/**< Instance of the source component. */
		EVENT_TYPE = EDG_WLL_QUERY_ATTR_EVENT_TYPE,	/**< Event type. */
		CHKPT = EDG_WLL_QUERY_ATTR_CHKPT_TAG,	/**< Checkpoint tag. */
		RESUBMITTED = EDG_WLL_QUERY_ATTR_RESUBMITTED,	/**< Job was resubmitted */
		PARENT = EDG_WLL_QUERY_ATTR_PARENT,	        /**< Id of the parent job. */
		EXITCODE = EDG_WLL_QUERY_ATTR_EXITCODE,	/**< Job system exit code. */
		JDLATTR = EDG_WLL_QUERY_ATTR_JDL_ATTR, 		/**< Arbitrary JDL attribute */
	};

	/** Symbolic names of predicates.
	 *
	 * These are the predicates used for creating atomic query
	 * conditions.
	 */
	enum Op {
		EQUAL=EDG_WLL_QUERY_OP_EQUAL, /**< Equal. */
		LESS=EDG_WLL_QUERY_OP_LESS, /**< Less than. */
		GREATER=EDG_WLL_QUERY_OP_GREATER, /**< Greater than. */
		WITHIN=EDG_WLL_QUERY_OP_WITHIN, /**< Within the
						   range. */
		UNEQUAL=EDG_WLL_QUERY_OP_UNEQUAL /**< Not equal. */
	};
  

	/** Default constructor.
	 *
	 * Initializes empty query condition.
	 */
	QueryRecord();

	/** Copy constructor
	 * 
	 * Initializes an exact copy of the object.
	 * \param[in] src 		Original object.
	 */
	QueryRecord(const QueryRecord &src);

	/** Assignment operator.
	 *
	 * Initializes an exact copy of the object.
	 * \param[in] src 		Original object.
	 * \returns Reference to this object.
	 */
	QueryRecord& operator=(const QueryRecord &src);

	/** Constructor for condition on string typed value.
	 *
	 * Initializes the object to hold condition on string typed
	 * attribute value.
	 * \param[in] name 		Name of the attribute.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] value 		Actual value.
	 * \throw Exception Invalid value type for given attribute.
	 */
	QueryRecord(const Attr name, const Op op, const std::string &value);

	/** Constructor for condition on integer typed value.
	 *
	 * Initializes the object to hold condition on integer typed
	 * attribute value.
	 * \param[in] name 		Name of the attribute.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] value 		Actual value.
	 * \throw Exception Invalid value type for given attribute.
	 */
	QueryRecord(const Attr name, const Op op, const int value);

	/** Constructor for condition on timeval typed value.
	 *
	 * Initializes the object to hold condition on timeval typed
	 * attribute value.
	 * \param[in] name 		Name of the attribute.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] value 		Actual value.
	 * \throw Exception Invalid value type for given attribute.
	 */
	QueryRecord(const Attr name, const Op op, const struct timeval &value);

	/** Constructor for condition on JobId typed value.
	 *
	 * Initializes the object to hold condition on JobId typed
	 * attribute value.
	 * \param[in] name 		Name of the attribute.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] value 		Actual value.
	 * \throw Exception Invalid value type for given attribute.
	 */
	QueryRecord(const Attr name, const Op op, const glite::jobid::JobId &value);

	/* this one is for attr==TIME and particular state */
	/** Constructor for condition on timeval typed value.
	 *
	 * Initializes the object to hold condition on the time the job 
	 * stays in given state.
	 * \param[in] name 		Name of the attribute.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] state 		State of thet job.
	 * \param[in] value 		Actual value.
	 * \throw Exception Invalid value type for given attribute.
	 */
	QueryRecord(const Attr name, const Op op, const int state, const struct timeval &value);
	
	/* constructors for WITHIN operator */
	/** Constructor for condition on string typed interval.
	 *
	 * Initializes the object to hold condition on string typed
	 * attribute interval.
	 * \param[in] name 		Name of the attribute.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] value_min 	Low interval boundary.
	 * \param[in] value_max 	High interval boundary.
	 * \throw Exception Invalid value type for given attribute.
	 */
	QueryRecord(const Attr name, const Op op, const std::string &value_min, const std::string &value_max);

	/** Constructor for condition on integer typed interval.
	 *
	 * Initializes the object to hold condition on integer typed
	 * attribute interval.
	 * \param[in] name 		Name of the attribute.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] value_min 	Low interval boundary.
	 * \param[in] value_max 	High interval boundary.
	 * \throw Exception Invalid value type for given attribute.
	 */
	QueryRecord(const Attr name, const Op op, const int value_min, const int value_max);

	/** Constructor for condition on timeval typed interval.
	 *
	 * Initializes the object to hold condition on timeval typed
	 * attribute interval.
	 * \param[in] name 		Name of the attribute.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] value_min 	Low interval boundary.
	 * \param[in] value_max 	High interval boundary.
	 * \throw Exception Invalid value type for given attribute.
	 */
	QueryRecord(const Attr name, const Op op, const struct timeval &value_min, const struct timeval &value_max);

	/** Constructor for condition on timeval typed interval for
	 * given state.
	 *
	 * Initializes the object to hold condition on the time job
	 * stayed in given state.
	 * \param[in] name 		Name of the attribute.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] state 		State of thet job.
	 * \param[in] value_min 	Low interval boundary.
	 * \param[in] value_max 	High interval boundary.
	 * \throw Exception Invalid value type for given attribute.
	 */
	QueryRecord(const Attr name, const Op op, const int state, const struct timeval &value_min, const struct timeval &value_max);

	/* convenience for user tags */
	/** Convenience constructor for condition on user tags.
	 *
	 * Initializes the object to hold condition on the value of
	 * user tag.
	 * \param[in] tag 		Name of the tag.
	 * \param[in] op 		Symbolic name of the predicate.
	 * \param[in] value 		Value of the tag.
	 */
	QueryRecord(const std::string &tag, const Op op, const std::string &value);

	/** Convenience constructor for condition on user tags.
	 *
	 * Initializes the object to hold condition on the value of
	 * user tag.
	 * \param[in] tag 		Name of the tag.
	 * \param[in] op 		Symbolic namen of the predicate.
	 * \param[in] value_min 	Minimal value of the tag.
	 * \param[in] value_max 	Maximal value of the tag.
	 * \throws Exception Predicate is not WITHIN.
	 */
	QueryRecord(const std::string &tag, const Op op, const std::string &value_min, const std::string &value_max);
	
	/** Destructor.
	 *
	 * The actual work is done by member destructors.
	 */
	~QueryRecord();
  
	/** Return the string representation of symbolic attribute
	 * name.
	 * \param[in] attr 		Symbolic attribute name.
	 * \returns Printable attribute name.
	 */
	static const std::string AttrName(const Attr attr) ;
  
protected:

	/* conversion to C API type */
	operator edg_wll_QueryRec() const;

private:
	Attr	attr;
	Op	oper;
	std::string tag_name;
	int state;
	std::string string_value;
	glite::jobid::JobId jobid_value;
        int     int_value;
        struct timeval timeval_value;
	std::string string_value2;
        int     int_value2;
        struct timeval timeval_value2;
};


/** Supported aggregate operations. */
enum AggOp { AGG_MIN=1, AGG_MAX, AGG_COUNT };


/**
 * Class representing a connection to the L&B server.
 *
 * This class serves as an interface for queries not related to
 * particular job. The address of the bookkeeping server to query can
 * be set arbitrarily during the lifetime of this object, connection
 * to the server is maintained automatically by the underlying C API
 * layer. This class can be thought of also as an encapsulation of
 * edg_wll_Context from L&B C API.
 *
 * ServerConnection's methods correlate to the L&B C API functions
 * taking edg_wll_Context as their first argument and not having
 * edg_wll_JobId as the second argument.
 * \see edg_wll_Context
 */
class ServerConnection {
public:
	/** \defgroup query Methods for querying the bookkeeping
	 * server.
	 *
	 * These methods serve for obtaining data from the bookkeeping
	 * server. The L&B service queries come in two flavors: 
	 * \li conjunctive query, as in <tt>(cond1) or (cond2)</tt>
	 * \li conjunction of disjunctive queries, as in <tt>( (cond1)
	 * or (cond2) ) and ( (cond3) or (cond4) )</tt>
	 * Methods for both query flavors are provided.
	 *
	 * Query methods actually do communicate with the server and
	 * they are synchronous; their completion can take some time
	 * not exceeding the query timeout.
	 */

        /** \defgroup property Methods for setting and getting
	 * connection properties.
	 *
	 * These methods are used for setting and obtaining various
	 * parameters of the communication (timeouts, user
	 * certificates, limits). Both general methods (taking the
	 * symbolic name of the parameter as an argument) and
	 * convenience methods (for some parameters) are provided.
	 *
	 * The methods are local, no communication takes place.
	 */

	friend class Job;

	/** Default constructor.
	 *
	 * Initializes the context to default values.
	 * \throws OSException Initialization failed.
	 */
	ServerConnection(void);

	/** DEPRECATED.
	 * 
	 * \throws Exception Always.
	 */
	ServerConnection(const std::string &);

	/** DEPRECATED.
	 * 
	 * \throws Exception Always.
	 */
	void open(const std::string &);

	/** DEPRECATED.
	 * 
	 * \throws Exception Always.
	 */
	void close(void);

	/* END DEPRECATED */

	/* set & get parameter methods */

        /** Set bookkeeping server address.
	 * \ingroup property
	 *
	 * Directs the instance to query the given bookkeping server.
	 * \param[in]  host Hostname of the server.
	 * \param[in]  port Service port.
	 * \throws LoggingException Setting parameters failed.
	 */
	void setQueryServer(const std::string& host, int port);

        /** Set query timeout.
	 * \ingroup property
	 *
	 * Sets the time interval to wait for server response.
	 * \param[in] time Time in seconds before the query expires.
	 * \throws LoggingException Setting parameters failed.
	 */
	void setQueryTimeout(int time);

	/** Set user's proxy certificate.
	 * \ingroup property
	 *
	 * Instructs the instance to authenticate to the server using
	 * user's X509 proxy certificate.
	 * \param[in] proxy Name of file containing the user's proxy certificate.
	 * \throws LoggingException Setting paramater failed.
	 */
	void setX509Proxy(const std::string& proxy);

	/** Set user's certificate.
	 * \ingroup property
	 *
	 * Instructs the instance to authenticate to the server using
	 * users's full X509 certificate (which is not a good thing).
	 * \param[in] cert Name of file containing the user's certificate.
	 * \param[in] key Name of file containing the user's private
	 * key.
	 * \throws LoggingException Setting parameters failed.
	 */
	void setX509Cert(const std::string& cert, const std::string& key);

	/** Get address of the bookkeeping server.
	 * \ingroup property
	 *
	 * Returns address of the bookkeeping server this instance is
	 * bound to.
	 * \returns Address (hostname,port).
	 * \throws LoggingException Getting parameter failed.
	 */
	std::pair<std::string, int> getQueryServer() const;

	/** Get query timeout.
	 * \ingroup property
	 *
	 * Returns the time interval this instance waits for server
	 * response.
	 * \returns Number of seconds to wait.
	 * \throws LoggingException Getting parameter failed.
	 */
	int getQueryTimeout() const;

	/** Get user's proxy.
	 * \ingroup property
	 *
	 * Returns filename of the user's X509 proxy certificate used
	 * to authenticate to the server.
	 * \returns Filename of the proxy certificate.
	 * \throws LoggingException Getting parameter failed.
	 */
	std::string getX509Proxy() const;

	/** Get user's X509 certificate.
	 * \ingroup property
	 *
	 * Returns filenames of the user's full X509 certificate used
	 * to authenticate to the server.
	 * \returns Pair of (certificate, key) filenames.
	 * \throws LoggingException Getting parameter failed.
	 */
	std::pair<std::string, std::string> getX509Cert() const;

	/* end of set & get */

	/** Destructor.
	 *
	 * Closes the connections and frees the context.
	 */
	virtual ~ServerConnection();


	/* consumer API */

	/** Retrieve the set of single indexed attributes.
	 * \ingroup query
	 *
	 * Returns the set of attributes that are indexed on the
	 * server. Every query must contain at least one indexed
	 * attribute for performance reason; exception to this rule
	 * requires setting appropriate paramater on the server and is
	 * not advised.
	 *
	 * In the vector returned, outer elements correspond to indices,
	 * inner vector elements correspond to index
	 * columns. 
	 * If <tt>.first</tt> of the pair is USERTAG, <tt>.second</tt> is its name;
	 * if <tt>.first</tt> is TIME, <tt>.second</tt> is state name
	 * otherwise <tt>.second</tt> is meaningless (empty string anyway).
	 */
	std::vector<std::vector<std::pair<QueryRecord::Attr,std::string> > >
		getIndexedAttrs(void);

	/* Retrieve hard and soft result set size limit.
	 * \ingroup property
	 *
	 * Returns both the hard and soft limit on the number of
	 * results returned by the bookkeeping server.
	 * \returns Pair (hard, soft) of limits.
	 * \throws 
	 */
	// std::pair<int,int> getLimits(void) const;

	/** Set the soft result set size limit.
	 * \ingroup property
	 *
	 * Sets the maximum number of results this instance is willing
	 * to obtain when querying for jobs.
	 * \param max Maximum number of results.
	 * \throws LoggingException Setting parameter failed.
	 */
	void setQueryJobsLimit(int max);

	/** Set the soft result set size limit.
	 * \ingroup property
	 *
	 * Sets the maximum number of results this instance is willing
	 * to obtain when querying for Events.
	 * \param max Maximum number of results.
	 * \throws LoggingException Setting parameter failed.
	 */
	void setQueryEventsLimit(int max);
  
	/** Retrieve all events satisfying the query records.
	 * \ingroup query
	 *
	 * Returns all events belonging to the jobs specified by \arg
	 * job_cond and in addition satisfying the \arg event_cond. 
	 * \param[in] job_cond Conjunctive query on jobs.
	 * \param[in] event_cond Conjunctive query on events.
	 * \param[out] events Vector of Event objects representing L&B
	 * events.
	 * \throws LoggingException Query failed.
	 */
	void queryEvents(const std::vector<QueryRecord>& job_cond,
			 const std::vector<QueryRecord>& event_cond,
			 std::vector<Event>& events) const;
  
	/** Convenience form of queryEvents.
	 *
	 */
	const std::vector<Event> queryEvents(const std::vector<QueryRecord>& job_cond,
					     const std::vector<QueryRecord>& event_cond) const;

        /** Another modification of queryEvents.
	 *
	 * The same method, but the results are returned as a list
	 * instead of vector.
	 */
	const std::list<Event> queryEventsList(const std::vector<QueryRecord>& job_cond,
				               const std::vector<QueryRecord>& event_cond) const;


	/** NOT IMPLEMENTED.
	 * \param[in] job_cond 
	 * \param[in]  event_cond	Vectors of conditions to be satisfied 
	 *  by jobs as a whole or particular events.
	 * \param[in] op 			Aggregate operator to apply.
	 * \param[in] attr 			Attribute to apply the operation to.
	 */
	std::string queryEventsAggregate(const std::vector<QueryRecord>& job_cond,
					 const std::vector<QueryRecord>& event_cond,
					 enum AggOp const op,
					 std::string const attr) const;
  

	/** Retrieve all events satisfying the conjunctive-disjunctive
	 * query.
	 *
	 * Returns all events belonging to the jobs specified by
	 * <tt>job_cond</tt> and satisfying <tt>query_cond</tt>. The
	 * conditions are given in conjunctive-disjunctive form
	 * <tt>((cond1 OR cond2 OR ...) AND ...)</tt> 
	 * \param[in] job_cond Vector of conditions on jobs.
	 * \param[in] event_cond Vector of coditions on events.
	 * \param[out] eventList Returned Event's.
	 * \throws LoggingException Query failed.
	 */
	void queryEvents(const std::vector<std::vector<QueryRecord> >& job_cond,
			 const std::vector<std::vector<QueryRecord> >& event_cond,
			 std::vector<Event>& eventList) const;
  
	/** Convenience form of queryEvents.
	 *
	 * The same as previous, but the resulting vector is passed as
	 * a return value.
	 */
	const std::vector<Event> 
	queryEvents(const std::vector<std::vector<QueryRecord> >& job_cond,
		    const std::vector<std::vector<QueryRecord> >& event_cond) const;

	
	/** Retrieve jobs satisfying the query.
	 *
	 * Finds all jobs (represented as JobId's) satisfying given
	 * query.
	 * \param[in] query Query in conjunctive form.
	 * \param[out] jobList	List of job id's.
	 * \throws LoggingException Query failed.
	 */
	void queryJobs(const std::vector<QueryRecord>& query,
		       std::vector<glite::jobid::JobId>& jobList) const;

        /** Convenience form of queryJobs.
	 *
	 * The same as above, but job id's are passed as a return
	 * value.
	 */
	const std::vector<glite::jobid::JobId>
	queryJobs(const std::vector<QueryRecord>& query) const;
  
	
	/** Retrieve jobs satisfying the query.
	 *
	 * Finds all jobs satisfying query given in
	 * conjunctive-disjunctive form.
	 * \param[in] query Conjunction of disjunctive queries.
	 * \param[out] jobList	Job id's of found jobs.
	 * \throws LoggingException Query failed.
	 */
	void queryJobs(const std::vector<std::vector<QueryRecord> >& query,
		       std::vector<glite::jobid::JobId>& jobList) const;
  
        /** Convenience form of queryJobs.
	 *
	 * Same as above, but result is passed as a retutrn value.
	 */
	const std::vector<glite::jobid::JobId>
	queryJobs(const std::vector<std::vector<QueryRecord> >& query) const;

	/** Retrieve status of jobs satisfying the given simple query.
	 *
	 * Returns states (represented by JobStatus) of all jobs
	 * satisfying the query in conjunctive form.
	 * \param[in] query  Condition on jobs.
	 * \param[in] flags  The same as Job::status() flags.
	 * \param[out] states States of jobs satysfying the condition.
	 * \throws LoggingException Query failed.
	 */
	void queryJobStates(const std::vector<QueryRecord>& query, 
			    int flags,
			    std::vector<JobStatus> & states) const;

	/** Convenience form of queryJobStates.
	 *
	 * Same as above, but the result is passed as a return value.
	 */
	const std::vector<JobStatus>  queryJobStates(const std::vector<QueryRecord>& query, 
						     int flags) const;

	/** Convenience form of queryJobStates.
	 *
	 * Same as above, but results are returned as list instead of
	 * vector.
	 */
	const std::list<JobStatus>  queryJobStatesList(const std::vector<QueryRecord>& query,
						     int flags) const;
  
	/** Retrieve status of jobs satisfying the given
	 * conjunctive-disjunctive query.
	 *
	 * Returns states (represented by JobStatus) of all jobs
	 * satisfying the query in conjunctive form.
	 * \param[in] query  Condition on jobs.
	 * \param[in] flags  The same as Job::status() flags.
	 * \param[out] states States of jobs satysfying the condition.
	 * \throws LoggingException Query failed.
	 */
	void queryJobStates(const std::vector<std::vector<QueryRecord> >& query, 
			    int flags,
			    std::vector<JobStatus> & states) const;

	/** Convenience form of queryJobStates.
	 *
	 * Same as above, but the result is passed as a return value.
	 */
	const std::vector<JobStatus>  
	queryJobStates(const std::vector<std::vector<QueryRecord> >& query, 
		       int flags) const;
  
	/** Return states of all user's jobs.
	 *
	 * Convenience wrapper around queryJobStates, returns status of all
	 * jobs whose owner is the current user (as named in the X509
	 * certificate subject).
	 * \param[out] stateList States of jobs owned by this user.
	 * \throws LoggingException Query failed.
	 */
	void userJobStates(std::vector<JobStatus>& stateList) const;
	const std::vector<JobStatus> userJobStates() const;
  
  
	/** Find all user's jobs.
	 *
	 * Convenience wrapper around queryJobs, returns id's of all
	 * jobs whose owner is the current user (as named in the X509
	 * certificate subject).
	 * \param[out] jobs Id's of jobs owned by this user.
	 * \throws LoggingException Query failed.
	 */
	void userJobs(std::vector<glite::jobid::JobId> &jobs) const;

	/** Convenience form of userJobs.
	 *
	 * Same as above, but results are passed as a return value.
	 */
	const std::vector<glite::jobid::JobId> userJobs() const;

	/** Set communication parameters of integer type.
	 *
	 * Sets the named parameter to the given integer value.
	 * \param[in] name Symbolic name of the parameter.
	 * \param[in] value Value.
	 * \throws LoggingException Setting parameter failed.
	 * \see edg_wll_SetParam()
	 */
	void setParam(edg_wll_ContextParam name, int value); 

	/** Set communication parameters of string type.
	 *
	 * Sets the named parameter to the given string value.
	 * \param[in] name Symbolic name of the parameter.
	 * \param[in] value Value.
	 * \throws LoggingException Setting parameter failed.
	 * \see edg_wll_SetParam()
	 */
	void setParam(edg_wll_ContextParam name, const std::string &value); 

	/** Set communication parameters of timeval type.
	 *
	 * Sets the named parameter to the given timeval value.
	 * \param[in] name Symbolic name of the parameter.
	 * \param[in] value Value.
	 * \throws LoggingException Setting parameter failed.
	 * \see edg_wll_SetParam()
	 */
	void setParam(edg_wll_ContextParam name, const struct timeval &value); 

	/** Get communication parameters of integer type.
	 *
	 * Gets the named parameter of integer type.
	 * \param[in] name Symbolic name of the parameter.
	 * \throws LoggingException Getting parameter failed.
	 * \see edg_wll_GetParam()
	 */
	int getParamInt(edg_wll_ContextParam name) const;

	/** Get communication parameters of string type.
	 *
	 * Gets the named parameter of string type.
	 * \param[in] name Symbolic name of the parameter.
	 * \throws LoggingException Getting parameter failed.
	 * \see edg_wll_GetParam()
	 */
	std::string getParamString(edg_wll_ContextParam name) const;

	/** Get communication parameters of timeval type.
	 *
	 * Gets the named parameter of timeval type.
	 * \param[in] name Symbolic name of the parameter.
	 * \throws LoggingException Getting parameter failed.
	 * \see edg_wll_GetParam()
	 */
	struct timeval getParamTime(edg_wll_ContextParam name) const;
  
protected:

	edg_wll_Context getContext(void) const;

private:
	edg_wll_Context context;
};

EWL_END_NAMESPACE

#endif /* GLITE_LB_SERVERCONNECTION_HPP */
