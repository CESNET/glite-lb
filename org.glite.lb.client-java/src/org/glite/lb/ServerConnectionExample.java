package org.glite.lb;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.GregorianCalendar;
import java.util.Iterator;
import java.util.List;
import org.glite.jobid.Jobid;
import org.glite.lb.Event;
import org.glite.wsdl.types.lb.JobStatus;
import org.glite.wsdl.types.lb.QueryAttr;
import org.glite.wsdl.types.lb.QueryConditions;
import org.glite.wsdl.types.lb.QueryOp;
import org.glite.wsdl.types.lb.QueryRecValue;
import org.glite.wsdl.types.lb.QueryRecord;
import org.glite.wsdl.types.lb.StatName;
import org.glite.wsdl.types.lb.Timeval;



/**
 * This is a demonstration class for query API.
 * It contains all possible methodes that can be called on ServerConnection
 * and Job objects.
 * @author Tomas Kramec, 207545@mail.muni.cz
 */
public class ServerConnectionExample {


    /**
     * This method serves for formating output information about given job status.
     * It is only an example of how the data can be presented. It can be changed
     * by user's needs.
     *
     * @param status Job status
     * @return text representation of the given status
     */
    private static String jobStatusToString(JobStatus status) {
        StringBuilder sb = new StringBuilder();
        sb.append("State:      "+status.getState()+"\n");
        sb.append("Job ID:     "+status.getJobId()+"\n");
        sb.append("Owner:      "+status.getOwner()+"\n");
        sb.append("Job type:   "+status.getJobtype()+"\n");
        sb.append("Destination:   "+status.getLocation()+"\n");
        sb.append("Done code:  "+status.getDoneCode()+"\n");
        sb.append("User tags:  ");
        //if there are some user tags write it out.
        if (status.getUserTags() != null) {
            for (int i=0;i<status.getUserTags().length;i++) {
                if (i==0) {
                    sb.append(status.getUserTags()[i].getTag()+" = "+
                            status.getUserTags()[i].getValue()+"\n");
                } else sb.append("            "+status.getUserTags()[i].getTag()+
                        " = "+ status.getUserTags()[i].getValue()+"\n");
            }
        } else sb.append("\n");
        //Write the time info in a readable form.
        Calendar calendar = new GregorianCalendar();
        DateFormat df = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss");
        calendar.setTimeInMillis(status.getStateEnterTime().getTvSec()*1000);

        sb.append("Enter time: "+ df.format(calendar.getTime())+"\n");
        calendar.setTimeInMillis(status.getLastUpdateTime().getTvSec()*1000);
        sb.append("Last update time: "+df.format(calendar.getTime())+"\n");
        return sb.toString();
    }

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) throws LBException {

        if (args.length != 8) {
            System.out.println("To use this class you have to run it with these arguments," +
                    "in this order: \n" +
                    "1. path to the trusted CA's files, default is /etc/grid-security/certificates/\n"+
                    "2. -p for proxy based authentication\n" +
                    "   -c for certificate based authentication\n"+
                    "3. proxy certificate\n"+
                    "4. user certificate\n"+
                    "5. user key\n"+
                    "6. user password\n"+
                    "7. LB server addres in format \"https://somehost:port\"\n"+
                    "8. jobid for queries in format \"https://somehost:port/uniquePart\"\n"+
                    "Enter \"\" for empty option.");

        } else {

            /**
             * create new instance of LBCredentials that holds credentials data
             * used for authentication
             */
            LBCredentials lbCredent = null;

            //if -p option is choosed set the proxy file for authentication
            //if -c option is choosed set user certificate, key and password
            if (args[1].equals("-p")) {
                lbCredent = new LBCredentials(args[2], args[0]);
            } else if (args[1].equals("-c")) {
                lbCredent = new LBCredentials(args[3], args[4], args[5], args[0]);
            } else {
                System.err.println("Wrong option: "+args[1]+". Only -p and -c is allowed.");
                System.exit(-1);
            }
         
            //create new instance of ServerConnection with the given LB server address
            ServerConnection sc = new ServerConnection(args[6], lbCredent);
            
            /**
             * It is also possible to set these attributes of ServerConnection instance:
             * timeout for server response
             * limit for job results
             * limit for event results
             * flag indicating the way of handling too large results
             * loggin level for events
             * LB server addres
             */
//            sc.setQueryTimeout(60);
//            sc.setQueryJobsLimit(100);
//            sc.setQueryEventsLimit(50);
//            sc.setQueryResultsFlag(ServerConnection.QueryResultsFlag.LIMITED);
//            sc.setEventLoggingLevel(Level.LEVEL_DEBUG);
//            sc.setQueryServerAddress("https://changedHost", 1234);

            //Get the server limit for query results
            System.out.println("Limit: " + sc.getServerLimit());

            System.out.println();
            /**
             * Print all indexed attributes from LB database.
             * If the attribute is USERTAG then print its name,
             * if it is TIME then print state name.
             */
            System.out.println("Indexed attributes:");
            List<QueryConditions> idx = sc.getIndexedAttrs();
            for (int i=0;i<idx.size();i++) {
                String name = idx.get(i).getAttr().getValue();
                System.out.print(idx.get(i).getAttr().getValue());
                if(name.equals("USERTAG"))
                    System.out.print(" "+ idx.get(0).getTagName());
                if(name.equals("TIME"))
                    System.out.print(" "+ idx.get(0).getStatName());
                System.out.println();
            }


            /**
             * Print all user's jobs and their states.
             */
            System.out.println();
            System.out.println("-----ALL USER's JOBS and STATES----");
            //get user jobs
            List<Jobid> jobs = sc.userJobs();
            //get their states
            List<JobStatus> js = sc.userJobStates();
            Iterator<Jobid> it = jobs.iterator();
            Iterator<JobStatus> itJs = js.iterator();
            while (it.hasNext()) {
                System.out.println(it.next());
                System.out.println(jobStatusToString(itJs.next()));
            }

            //Demonstration of Job class
            System.out.println();
            System.out.println("----------------JOB----------------");

            //create new Job
            Job myJob = new Job(args[7], sc);
            //print job state info
            System.out.println();
            System.out.println("Status: " + jobStatusToString(myJob.getStatus(null)));

            //print info about job's events
            System.out.println();
            List<Event> events = myJob.getEvents();
            System.out.println("Found "+events.size()+" events:");
            for (int i=0;i<events.size();i++) {
                System.out.println("Event: " + events.get(i).info());
            }

            /**
             * This is demonstration of creating and using query conditions.
             * It prints information about all user's jobs registered in LB
             * submitted in last 3 days.
             */
            if (!jobs.isEmpty()) {
                System.out.println();
                System.out.println("-------------CONDITIONS------------");

                //create query record for job ids from the previous query on all user's jobs.
                List<QueryRecord> recList = new ArrayList<QueryRecord>();
                int port = Integer.parseInt((args[6].split(":"))[2]);
                for (int i=0;i<jobs.size();i++) {
                    if(jobs.get(i).getPort() == port) {
                        QueryRecValue jobIdValue = new QueryRecValue(null, jobs.get(i).toString(), null);
                        recList.add(new QueryRecord(QueryOp.EQUAL, jobIdValue, null));
                        System.out.println(jobIdValue.getC());
                    } 
                }
                QueryRecord[] jobIdRec = new  QueryRecord[]{};
                jobIdRec = recList.toArray(jobIdRec);

                //crete QueryConditions instance this formula:
                //(JOBID='jobId1' or JOBID='jobId2 or ...) where jobId1,... are ids of user's jobs
                QueryConditions condOnJobid = new QueryConditions(QueryAttr.JOBID, null, null, jobIdRec);

                //create query record for time
                long time = System.currentTimeMillis()/1000;
                QueryRecValue timeFrom = new QueryRecValue(null, null, new Timeval(time-259200, 0));
                QueryRecValue timeTo = new QueryRecValue(null, null, new Timeval(System.currentTimeMillis()/1000, 0));
                QueryRecord[] timeRec = new QueryRecord[]{new QueryRecord(QueryOp.WITHIN, timeFrom, timeTo)};
                //create QueryConditions instance representing this formula:
                //(TIME is in <timeFrom, timeTo> interval)
                QueryConditions condOnTime = new QueryConditions(QueryAttr.TIME, null, StatName.SUBMITTED, timeRec);

                //create QueryConditions list representing this formula:
                //(JOBID='jobId1' or JOBID='jobId2 or ...) AND (TIME is in <timeFrom, timeTo> interval)
                //where jobId1,... are ids of user's jobs
                List<QueryConditions> condList = new ArrayList<QueryConditions>();
                condList.add(condOnJobid);
                condList.add(condOnTime);

                //get all jobs matching the given conditions
                List<Jobid> jobResult = sc.queryJobs(condList);
                //get all their states
                List<JobStatus> jobStatesResult = sc.queryJobStates(condList, null);

                //Print information about results
                Calendar calendar = new GregorianCalendar();
                DateFormat df = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss");
                System.out.println();
                System.out.print("Jobs registered ");
                calendar.setTimeInMillis(timeFrom.getT().getTvSec()*1000);
                System.out.print("from "+ df.format(calendar.getTime())+" ");
                calendar.setTimeInMillis(timeTo.getT().getTvSec()*1000);
                System.out.print("to "+ df.format(calendar.getTime())+"\n");
                Iterator<Jobid> jobsit = jobResult.iterator();
                Iterator<JobStatus> statusit = jobStatesResult.iterator();
                while (jobsit.hasNext()) {
                    System.out.println(jobsit.next());
                    System.out.println(jobStatusToString(statusit.next()));
                }
            }

        }
    }
}
