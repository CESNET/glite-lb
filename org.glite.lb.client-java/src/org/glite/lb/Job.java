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

import java.util.ArrayList;
import java.util.List;
import org.glite.jobid.Jobid;
import org.glite.lb.Event;
import org.glite.wsdl.types.lb.JobFlagsValue;
import org.glite.wsdl.types.lb.JobStatus;
import org.glite.wsdl.types.lb.QueryAttr;
import org.glite.wsdl.types.lb.QueryConditions;
import org.glite.wsdl.types.lb.QueryOp;
import org.glite.wsdl.types.lb.QueryRecValue;
import org.glite.wsdl.types.lb.QueryRecord;

/**
 * Class encapsulating the job info stored in the L&B database.
 *
 * This class is the primary interface for getting information about
 * job stored in the L&B database. It is constructed from known job
 * id, which uniquely identifies the job as well as the bookkeeping
 * server where the job data is stored. The Job class provides methods
 * for obtaining the data from the bookkeeping server.
 *
 * @author Tomas Kramec, 207545@mail.muni.cz
 */
public class Job {
    
    private Jobid jobId;
    private ServerConnection serverConnection;

	/** 
	 * Constructor initializes the job as empty, not representing anything.
	 */
	public  Job() {
    }
	
	/** 
     * Constructor from job id and bookkeeping server connection.
	 * Initializes the job to obtain information for the given job id.
     *
	 * @param jobId The job id of the job this object wil represent.
     * @param serverConnection ServerConnection object providing methods
     *        for obtaining the data from the bookkeeping server.
	 */
	public  Job(Jobid jobId, ServerConnection serverConnection) {
        if (jobId == null) throw new IllegalArgumentException("jobId cannot be null");
        if (serverConnection == null) throw new IllegalArgumentException("server cannot be null");
        
        this.jobId = jobId;
        this.serverConnection = serverConnection;
    }

    /**
     * Constructor from job id and bookkeeping server connection.
	 * Initializes the job to obtain information for the given job id.
     *
	 * @param jobId The job id of the job this object wil represent.
     * @param serverConnection ServerConnection object providing methods
     *        for obtaining the data from the bookkeeping server.
	 */
	public  Job(String jobId, ServerConnection serverConnection) {
        this(new Jobid(jobId), serverConnection);
    }
    
    /**
     * Gets this job ID.
     *
     * @return jobId
     */
    public Jobid getJobId() {
        return jobId;
    }

    /**
     * Sets the jobId to this job.
     *
     * @param jobId
     */
    public void setJobId(Jobid jobId) {
        if (jobId == null) throw new IllegalArgumentException("jobId");

        this.jobId = jobId;
    }

    /**
     * Gets server connection.
     *
     * @return serverConnection
     */
    public ServerConnection getServer() {
        return serverConnection;
    }

    /**
     * Sets server connection instance to this job.
     * 
     * @param serverConnection
     */
    public void setServer(ServerConnection serverConnection) {
        if (serverConnection == null) throw new IllegalArgumentException("server");

        this.serverConnection = serverConnection;
    }


    /**
     * Return job status.
	 * Obtains the job status (as JobStatus) from the bookkeeping server.
	 * 
     * @param flags Specify details of the query. Determines which status
     * fields are actually retrieved.
     * Possible values:<ol>
     * <li type="disc">CLASSADS - Include the job description in the query result.</li>
     * <li type="disc">CHILDREN - Include the list of subjob id's in the query result.</li>
     * <li type="disc">CHILDSTAT - Apply the flags recursively to subjobs.</li>
     * <li type="disc">CHILDHIST_FAST - Include partially complete histogram of child job states.</li>
     * <li type="disc">CHILDHIST_THOROUGH - Include full and up-to date histogram of child job states.</li>
     * </ol>
     *
	 * @return Status of the job.
     * @throws LBException If some communication or server problem occurs.
	 */
	public JobStatus getStatus(JobFlagsValue[] flags) throws LBException {
        if (serverConnection == null)
            throw new IllegalStateException("serverConnection is null, please set it");
        if (jobId == null)
            throw new IllegalStateException("jobId is null, please set it");
        return serverConnection.jobState(jobId.toString(), flags);
    }
  
	/** 
     * Return all events corresponding to this job.
	 * Obtains all events corresponding to the job that are stored
	 * in the bookkeeping server database.
     * <p>
     * Default value for logging level is SYSTEM. If needed, it can be changed
     * by calling <b>setEventLoggingLevel(Level loggingLevel)</b> method
     * on the serverConnection attribute of this class.
	 * </p>
     * 
     * @return events List of events (of type Event).
     * @throws LBException If some communication or server problem occurs.
	 */
	public List<Event> getEvents() throws LBException {
        if (serverConnection == null)
            throw new IllegalStateException("serverConnection is null, please set it");
        if (jobId == null)
            throw new IllegalStateException("jobId is null, please set it");

        QueryRecValue jobIdValue = new QueryRecValue(null, jobId.toString(), null);
        QueryRecord[] jobIdRec = new QueryRecord[] {new QueryRecord(QueryOp.EQUAL, jobIdValue, null)};
        QueryConditions[] query = new QueryConditions[]{new QueryConditions(QueryAttr.JOBID, null, null, jobIdRec)};
        QueryRecValue levelValue = new QueryRecValue(serverConnection.getEventLoggingLevel().getInt()+1, null, null);
        QueryRecord[] levelRec = new QueryRecord[] {new QueryRecord(QueryOp.LESS, levelValue, null)};
        QueryConditions[] queryEvent = new QueryConditions[]{new QueryConditions(QueryAttr.LEVEL, null, null, levelRec)};
        List<QueryConditions> queryList = new ArrayList<QueryConditions>();
        queryList.add(query[0]);
        List<QueryConditions> queryEventList = new ArrayList<QueryConditions>();
        queryEventList.add(queryEvent[0]);

        return serverConnection.queryEvents(queryList, queryEventList);
    }
}
