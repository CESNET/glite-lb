/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */
package org.glite.test;

import java.util.Random;
import org.glite.jobid.api_java.Jobid;
import org.glite.lb.client_java.ContextIL;
import org.glite.lb.client_java.Running;
import org.glite.lb.client_java.SeqCode;

/**
 *
 * @author xpiskac
 */
public class Test {

    public static void main(String[] args) {
        
        //how Jobid class works
        /* //unique part is automatically generated
        Jobid jobid = new Jobid("https://somewhere.cz", 5000);
        System.out.println("bkserver "+ jobid.getBkserver());
        System.out.println("port "+ jobid.getPort());
        System.out.println("unique "+ jobid.getUnique());
        System.out.println("-------------------");
        
        //unique part is set by user
        Jobid jobid2 = new Jobid("https://somewhere.cz", 5000, "my_unique_part");
        System.out.println("bkserver "+ jobid2.getBkserver());
        System.out.println("port "+ jobid2.getPort());
        System.out.println("unique "+ jobid2.getUnique());
        System.out.println("-------------------");
        
        //whole jobid is set by user and then parsed
        Jobid jobid3 = new Jobid("https://somewhere.cz:5000/my_unique_part");
        System.out.println("bkserver "+ jobid3.getBkserver());
        System.out.println("port "+ jobid3.getPort());
        System.out.println("unique "+ jobid3.getUnique());
        System.out.println("-------------------");
        
        //each part is set separately
        Jobid jobid4 = new Jobid();
        jobid4.setBkserver("https://somewhere.cz");
        jobid4.setPort(5000);
        jobid4.setUnique("my_unique_part");
        System.out.println("bkserver "+ jobid4.getBkserver());
        System.out.println("port "+ jobid4.getPort());
        System.out.println("unique "+ jobid4.getUnique());
        System.out.println("-------------------");
        
         */ 
        if (args.length == 0) {
            System.out.println("How to use test class:\n" +
                    "you have to set 10 arguments in this order, if the choice is optional \"\" or text has to be set:\n" +
                    "1. jobid in format \"https://somewhere:port/unique_part\" (required)\n" +
                    "2. path to shared library written in c to be able to send messages via unix socket (optional)\n" +
                    "3. source, enum constant from class Sources (required)\n" +
                    "4. flag determines which part of sequence code will be increased\n" +
                    "5. host name, if it is \"\" then is set name of the computer where is test class running (optional)\n" +
                    "6. user name (required)\n" +
                    "7. PID of running process (optional)\n" +
                    "8. path to directory where will be saved files with events for each job (required)\n" +
                    "9. path to unix socket (required if path to shared library is set)\n" +
                    "10. description for event in this case event running (optional)\n");
        } else {
            /* Create new instance of jobid, you can use other constructors too (see org.glite.jobid.api_java.Jobid.java) 
             * Examples:
             * Jobid jobid = new Jobid("https://skurut68-2.cesnet.cz:9000/paja6_test2");
             * Jobid jobid = new Jobid("https://skurut68-2.cesnet.cz", 9000, "paja6_test2");
             * Jobid jobid = new Jobid("https://skurut68-2.cesnet.cz", 9000); //unique part is automatically generated
             * Jobid jobid = new Jobid();
             * jobid.setBkserver("https://skurut68-2.cesnet.cz");
             * jobid.setPort(9000);
             * jobid.setUnique("paja6_test2");
             */
            Jobid jobid = new Jobid(args[0]);

            /* Create sequence code
             * Example:
             * SeqCode seqCode = new SeqCode();
             * Then you can set some parts
             * Example:
             * seqCode.setPardOfSeqCode(0, 95);
             * or whole sequence number
             * Example:
             * int[] seqCodeArray = {1, 2, 3, 4, 5, 6, 7, 8, 9};
             * seqCode.setSeqCode(seqCodeArray);
             */
            SeqCode seqCode = new SeqCode();

            /* Choose type of sending a log messages (at this time is implemented only ContextIL class)
             * You can choose from some constructors (see org.glite.lb.client_java.ContextIL class)
             */
            ContextIL ctx = new ContextIL();

            /* If you chose emtpy ContextIL constructor you have to set some attributes.
             * One of them is pathToNativeLib which sais where java can find shared library written in c.
             * Example: ctx.setPathToNativeLib("/home/paja6/locallogger/build/classes/org/glite/lb/");
             */
            ctx.setPathToNativeLib(args[1]);

            /* Id of the message is some random unique number. 
             */
            ctx.setId(new Random().nextInt(99999999));

            /* Source indicates source of the message, it is constant from org.glite.lb.client_java.Sources class
             * Example: ctx.setSource(ctx.stringToSources("EDG_WLL_SOURCE_LRMS"));
             */
            ctx.setSource(ctx.stringToSources(args[2]));

            /* Flag tells which part of the sequence number will be increased. It is a number in range from 0 to 8
             * Example: ctx.setFlag(0);
             */
            ctx.setFlag(new Integer(args[3]));

            /* Name of the computer where is locallogger running
             * Example: ctx.setHost("pelargir.ics.muni.cz");
             */
            ctx.setHost(args[4]);

            /* Name of the user who owns the job.
             * Example: ctx.setUser("Pavel Piskac");
             */
            ctx.setUser(args[5]);

            /* TODO co to vlastne znamena?
             * Mostly "" is set
             * Example: ctx.setSrcInstance("");
             */
            ctx.setSrcInstance(args[6]);

            /* Set the jobid for the context.
             */
            ctx.setJobid(jobid);

            /* Set the jobid for the context.
             */
            ctx.setSeqCode(seqCode);

            /* Number of connection attempts while sending the message via unix socket.
             * Default value is 3 but you can change it.
             */
            ctx.setConnAttempts(5);

            /* Timeout in seconds for the connection while sending the message via unix socket.
             * Default value is 3 but you can change it.
             */
            ctx.setTimeout(2);

            /* Path to directory where will be saved files with logs until inter-logger sends 
             * the content.
             * Example: ctx.setPrefix("/home/paja6/tmp/dglog." + jobid.getUnique());
             */
            ctx.setPrefix(args[7]);

            /* Path to unix socket.
             * Example: ctx.setPathToSocket("/home/paja6/tmp/il.sock");
             */
            ctx.setPathToSocket(args[8]);

            /* Create new instance of the event which will be logged.
             */
            Running running = new Running();

            /* Set some description for the event.
             * Example: running.setNode("worker node");
             */
            running.setNode(args[9]);

            /* And now is the context and event prepared to work.
             * 
             */
            ctx.log(running);
        }
    }
}
