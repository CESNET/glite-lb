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


package org.glite.lb;

import holders.StringArrayHolder;
import java.rmi.RemoteException;
import java.util.ArrayList;
import java.util.List;
import org.apache.axis.client.Stub;
import org.glite.jobid.Jobid;
import org.glite.lb.Event;
import org.glite.lb.Level;
import org.glite.wsdl.services.lb.LoggingAndBookkeepingPortType;
import org.glite.wsdl.types.lb.JobFlagsValue;
import org.glite.wsdl.types.lb.JobStatus;
import org.glite.wsdl.types.lb.QueryConditions;
import org.glite.wsdl.types.lb.holders.JobStatusArrayHolder;

/**
 * This class serves for obtaining data from the bookkeeping
 * server.
 * The L&B service queries come in two flavors:
 * <ol><li type="disc"> conjunctive query, as in <tt>(cond1) or (cond2)</tt></li>
 * <li type="disc"> conjunction of disjunctive queries, as in <tt>( (cond1)
 * or (cond2) ) and ( (cond3) or (cond4) )</tt></li></ol>
 * Methods for both query flavors are provided.
 * Query methods actually do communicate with the server and
 * they are synchronous; their completion can take some time
 * not exceeding the query timeout.
 * 
 * @author Tomas Kramec, 207545@mail.muni.cz
 */
public class ServerConnection {

    private String queryServerAddress;
    private int queryJobsLimit=0;
    private int queryEventsLimit=0;
    private QueryResultsFlag queryResultsFlag = QueryResultsFlag.NONE;
    private Level eventLogLevel = Level.LEVEL_SYSTEM;
    private LoggingAndBookkeepingPortType queryServer;
    private LBCredentials lbCredent;


    /**
     * This enum represents flag to indicate handling of too large results.
     * In case the result limit is reached:
     * <ol>
     * <li><b>NONE</b> - means no results are returned at all.</li>
     * <li><b>LIMITED</b> - means a result contains at most <i>limit</i> items.</li>
     * <li><b>ALL</b> - means all results are returned withouth any limitation.</li>
     * </ol>
     */
    public enum QueryResultsFlag {
        /**
         * No results are returned at all.
         */
        NONE,
        /**
         * Result contains at most <i>limit</i> items. Limits for job and event
         * results can be set by calling appropriate method on the ServerConnection
         * instance.
         */
        LIMITED,
        /**
         * Results are returned withouth any limitation.
         */
        ALL
    }

    /**
	 * Constructor initializes the context and
     * directs new instance to query the given bookkeping server.
     *
     * @param server String containing the server address (e.g. https://harad.ics.muni.cz:9453).
     * @param lbCredent instance of class that holds credentials for SSL authentication
	 */
	public ServerConnection(String server, LBCredentials lbCredent) throws LBException {
        if (server == null) throw new IllegalArgumentException("Server cannot be null");
        if (lbCredent == null) throw new IllegalArgumentException("Credentials cannot be null");

        this.lbCredent = lbCredent;
        setQueryServerAddress(server);
        setQueryTimeout(120);
    }

    /** 
	 * Set bookkeeping server.
	 * Directs the instance to query the given bookkeping server.
     *
	 * @param server String containing the server address (e.g. https://harad.ics.muni.cz:9553).
	 */
	public void setQueryServerAddress(String server) throws LBException {
        if (server == null) throw new IllegalArgumentException("Server cannot be null");

        queryServer = lbCredent.getStub(server);
        queryServerAddress = server;
        
    }

    /**
     * Get address of the bookkeeping server.
	 * Returns address of the bookkeeping server this instance is
	 * bound to.
     *
	 * @return Address (https://hostname:port).
	 */
    public String getQueryServerAddress() {
        return queryServerAddress;
    }

    /**
     * Set query timeout.
	 * Sets the time interval to wait for server response.
     * Default value is 120 seconds.
     *
	 * @param time Time in seconds before the query expires. 0 means no timeout.
	 */
    public void setQueryTimeout(int time) {
        if (time < 0 || time > 1800)
            throw new IllegalArgumentException("timeout must be between 0 and 1800");

        ((Stub) queryServer).setTimeout(time*1000);

    }

    /**
     * Get query timeout.
	 * Returns the time interval this instance waits for server
	 * response.
     *
	 * @return Number of seconds to wait. 0 means no timeout.
	 */
	public int getQueryTimeout() {
        return ((Stub) queryServer).getTimeout()/1000;
    }

    /**
     * Set logging level.
     * Sets the level for logging the events.
     * Possible values:<ol>
     * <li type="disc">EMERGENCY</li>
     * <li type="disc">ALERT</li>
     * <li type="disc">ERROR</li>
     * <li type="disc">WARNING</li>
     * <li type="disc">AUTH</li>
     * <li type="disc">SECURITY</li>
     * <li type="disc">USAGE</li>
     * <li type="disc">SYSTEM</li>
     * <li type="disc">IMPORTANT</li>
     * <li type="disc">DEBUG </li>
     * </ol>
     * Default value is SYSTEM.
     * @param loggingLevel level for logging the events
     */
    public void setEventLoggingLevel(Level loggingLevel) {
        if (loggingLevel == null) throw new IllegalArgumentException("loggingLevel");

        this.eventLogLevel = loggingLevel;
    }

    /**
     * Get logging level.
     * Returns the level for logging the events.
     *
     * @return level value
     */
    public Level getEventLoggingLevel() {
        return eventLogLevel;
    }

	/**
     * Retrieve the set of single indexed attributes.
	 * Returns the set of attributes that are indexed on the
	 * server. Every query must contain at least one indexed
	 * attribute for performance reason; exception to this rule
	 * requires setting appropriate paramater on the server and is
	 * not advised.
	 *<br/>
	 * In the list returned, each element represents a query condition.
     * Query attribute of the condition corresponds to the indexed attribute.
	 * If the query attribute is USERTAG, tagName is its name;
	 * if it is TIME, statName is state name.
     *
     * @return list of QueryConditions
     * @throws LBException If some communication or server problem occurs.
	 */
    public List<QueryConditions> getIndexedAttrs() throws LBException {
        try {
            QueryConditions[] cond = queryServer.getIndexedAttrs("");
            List<QueryConditions> indexedAttrs = new ArrayList<QueryConditions>();
            for (int i = 0; i < cond.length; i++) {
                indexedAttrs.add(cond[i]);
            }
            return indexedAttrs;
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }
    }

	/**
     * Retrieve hard result set size limit. This
     * property is set at the server side.
	 *
	 * Returns the hard limit on the number of
	 * results returned by the bookkeeping server.
	 *
     * @return Server limit.
     * @throws LBException If some communication or server problem occurs.
	 */	
    public int getServerLimit() throws LBException {
        try {
            return queryServer.getServerLimit("");
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }
    }

	/**
     * Set the soft result set size limit.
	 * Sets the maximum number of results this instance is willing
	 * to obtain when querying for jobs.
     * Default value is 0. It means no limits for results.
     *
	 * @param jobsLimit Maximum number of results. 0 for no limit.
	 */
	public void setQueryJobsLimit(int jobsLimit) {
        if (jobsLimit < 0) throw new IllegalArgumentException("jobsLimit");
        this.queryJobsLimit = jobsLimit;
    }

    /**
     * Get soft result set size limit.
     * Gets the maximum number of results this instance is willing
     * to obtain when querying for jobs.
     * 
     * @return queryJobsLimit Maximum number of results.
     */
    public int getQueryJobsLimit() {
        return queryJobsLimit;
    }

	/**
     * Set the soft result set size limit.
	 * Sets the maximum number of results this instance is willing
	 * to obtain when querying for events.
     * Default value is 0. It means no limits for results.
     *
	 * @param eventsLimit Maximum number of results. 0 for no limit.
	 */
	public void setQueryEventsLimit(int eventsLimit) {
        if (eventsLimit < 0) throw new IllegalArgumentException("eventsLimit");
        this.queryEventsLimit = eventsLimit;
    }

    /**
     * Get soft result set size limit.
     * Gets the maximum number of results this instance is willing
     * to obtain when querying for events.
     * 
     * @return queryEventsLimit Soft limit.
     */
    public int getQueryEventsLimit() {
        return queryEventsLimit;
    }

    /**
     * Sets the flag indicating the way of handling of too large query results.
     * Default is NONE.
     *
     * @param flag One of NONE, LIMITED or ALL.
     */
    public void setQueryResultsFlag(QueryResultsFlag flag) {
        if (flag == null) throw new IllegalArgumentException("flag");

        queryResultsFlag = flag;
    }

    /**
     * Gest the flag indicating the way of handling of too large query results.
     *
     * @return queryResultsFlag
     */
    public QueryResultsFlag getQueryResultsFlag() {
        return queryResultsFlag;
    }
    
    /**
     * Retrieve all events satisfying the conjunctive-disjunctive
	 * query records.
	 * Returns all events belonging to the jobs specified by
	 * <tt>jobCond</tt> and satisfying <tt>queryCond</tt>. The
	 * conditions are given in conjunctive-disjunctive form
	 * <tt>((cond1 OR cond2 OR ...) AND ...)</tt>
     *
	 * @param jobCond List of conditions on jobs.
	 * @param eventCond List of coditions on events.
	 * @return eventList List of Event objects representing L&B events.
     * @throws LBException If some communication or server problem occurs.
	 */
	public List<Event> queryEvents(List<QueryConditions> jobCond, 
            List<QueryConditions> eventCond) throws LBException {

        if (jobCond == null) throw new IllegalArgumentException("jobCond cannot be null");
        if (eventCond == null) throw new IllegalArgumentException("eventCond cannot be null");

        org.glite.wsdl.types.lb.Event[] events = null;
        try {
            events = queryServer.queryEvents(jobCond.toArray(new QueryConditions[jobCond.size()]), eventCond.toArray(new QueryConditions[eventCond.size()]));
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }

        List<Event> eventList= new ArrayList<Event>();
        
        if (events!= null) {
            int queryResults;
            //if the events limit is reached
            if (queryEventsLimit!=0 && events.length > queryEventsLimit) {
                queryResults = getResultSetSize(queryEventsLimit, events.length);
            } else queryResults = events.length;

            EventConvertor convertor = new EventConvertor();
            for (int i=0;i<queryResults;i++) {
                eventList.add(convertor.convert(events[i]));
            }
        }

        return eventList;
    }

	/**
     * Retrieve jobs satisfying the query.
	 * Returns all jobs (represented as JobId's) satisfying query given in
     * conjunctive-disjunctive form.
     *
	 * @param query Query in conjunctive-disjunctive form.
	 * @return jobList	List of job id's.
     * @throws LBException If some communication or server problem occurs.
	 */
	public List<Jobid> queryJobs(List<QueryConditions> query) throws LBException {
        if (query == null) throw new IllegalArgumentException("query cannot be null");

        StringArrayHolder jobHolder = new StringArrayHolder();
        try {
            queryServer.queryJobs(query.toArray(new QueryConditions[query.size()]), new JobFlagsValue[]{}, jobHolder, new JobStatusArrayHolder());
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }
           
        List<Jobid> jobList = new ArrayList<Jobid>();

        if (jobHolder.value!= null) {
            int queryResults;
            int jobsCount = jobHolder.value.length;
            //if the jobs limit is reached
            if (queryJobsLimit!=0 && jobsCount > queryJobsLimit) {
                queryResults = getResultSetSize(queryJobsLimit, jobsCount);
            } else queryResults = jobsCount;

            for (int i=0;i<queryResults;i++) {
                jobList.add(new Jobid(jobHolder.value[i]));
            }
        }
        
        return jobList;
    }

	/** 
	 * Retrieve status of jobs satisfying the given query.
	 * Returns states (represented by JobStatus) of all jobs
	 * satisfying the query in conjunctive-disjunctive form.
     *
	 * @param query  Condition on jobs.
	 * @param flags  Determine which status fields are actually retrieved.
	 * @return states States of jobs satysfying the condition.
     * @throws LBException If some communication or server problem occurs.
	 */
	public List<JobStatus> queryJobStates(List<QueryConditions> query,
            JobFlagsValue[] flags) throws LBException {
        if (query == null) throw new IllegalArgumentException("query cannot be null");

        JobStatusArrayHolder jobStatusHolder = new JobStatusArrayHolder();
        try {
            queryServer.queryJobs(query.toArray(new QueryConditions[query.size()]), flags, new StringArrayHolder(), jobStatusHolder);
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }

        List<JobStatus> jobStates= new ArrayList<JobStatus>();

        if (jobStatusHolder.value!= null) {
            int queryResults;
            int jobsCount = jobStatusHolder.value.length;
            //if the jobs limit is reached
            if (queryJobsLimit!=0 && jobsCount > queryJobsLimit) {
                queryResults = getResultSetSize(queryJobsLimit, jobsCount);
            } else queryResults = jobsCount;

            for (int i=0;i<queryResults;i++) {
                jobStates.add(jobStatusHolder.value[i]);
            }
        }

        return jobStates;
    }

    /**
	 * Retrieve status of the job with the given jobId.
	 * Returns state (represented by JobStatus) of the job.
     *
	 * @param jobId  jobId of the job.
	 * @param flags  Determine which status fields are actually retrieved.
	 * @return state State of the given job.
     * @throws LBException If some communication or server problem occurs.
	 */
    public JobStatus jobState(String jobId, JobFlagsValue[] flags) throws LBException {
        if (jobId == null) throw new IllegalArgumentException("jobId cannot be null");
        JobStatus state = new JobStatus();
        try {
            state = queryServer.jobStatus(jobId, flags);
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }

        return state;
    }

	/** 
	 * Retrieve states of all user's jobs.
	 * Convenience wrapper around queryJobStates, returns status of all
	 * jobs whose owner is the current user (as named in the X509
	 * certificate subject).
     *
	 * @return jobStates States of jobs owned by this user.
     * @throws LBException If some communication or server problem occurs.
	 */
	public List<JobStatus> userJobStates() throws LBException {
        JobStatusArrayHolder jobStatusHolder = new JobStatusArrayHolder();
        try {
            queryServer.userJobs(new StringArrayHolder(), jobStatusHolder);
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }

        List<JobStatus> jobStates= new ArrayList<JobStatus>();
        
        if (jobStatusHolder.value!= null) {
            int queryResults;
            int jobsCount = jobStatusHolder.value.length;
            //if the jobs limit is reached
            if (queryJobsLimit!=0 && jobsCount > queryJobsLimit) {
                queryResults = getResultSetSize(queryJobsLimit, jobsCount);
            } else queryResults = jobsCount;

            for (int i=0;i<queryResults;i++) {
                jobStates.add(jobStatusHolder.value[i]);
            }
        }

        return jobStates;
    }

	/** 
	 * Find all user's jobs.
	 * Convenience wrapper around queryJobs, returns id's of all
	 * jobs whose owner is the current user (as named in the X509
	 * certificate subject).
     *
	 * @return jobs List of jobs(IDs) owned by this user.
     * @throws LBException If some communication or server problem occurs.
	 */
	public List<Jobid> userJobs() throws LBException {
        StringArrayHolder jobHolder = new StringArrayHolder();
        try {
            queryServer.userJobs(jobHolder, new JobStatusArrayHolder());
        } catch (RemoteException ex) {
            throw new LBException(ex);
        }

        List<Jobid> jobs= new ArrayList<Jobid>();
        
        if (jobHolder.value!= null) {
            int queryResults;
            int jobsCount = jobHolder.value.length;
            //if the jobs limit is reached
            if (queryJobsLimit!=0 && jobsCount > queryJobsLimit) {
                queryResults = getResultSetSize(queryJobsLimit, jobsCount);
            } else queryResults = jobsCount;

            for (int i=0;i<queryResults;i++) {
                jobs.add(new Jobid(jobHolder.value[i]));
            }
        }

        return jobs;
    }

    /**
     * This private method returns result set size number according to
     * queryResultsFlag, which indicates handling of too large results.
     *
     * @param queryLimit The user limit.
     * @param results Result set size returned by LB server.
     * @return number of results this instance is going to return
     */
    private int getResultSetSize(int queryLimit, int results) {
        if (queryLimit < 0) throw new IllegalArgumentException("queryLimit");
        if (results < 0) throw new IllegalArgumentException("results");

        switch (queryResultsFlag) {
            case ALL: return results;
            case LIMITED: return queryLimit;
            case NONE: return 0;
            default: throw new IllegalArgumentException("wrong QueryResult");
        }
    }

}


