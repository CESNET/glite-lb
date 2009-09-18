
import java.util.Random;
import org.glite.jobid.Jobid;
import org.glite.lb.*;

/**
 * This class shows how to work with ContextIL.
 * 
 * @author Pavel Piskac
 */
public class ProducerTestIL {

    public static void main(String[] args) {
        
        if (args.length != 10) {
            System.out.println("How to use test class:\n" +
                    "you have to set 10 arguments in this order, if the choice is optional \"\" or text has to be set:\n" +
                    "1. jobid in format \"https://somewhere:port/unique_part\" (required)\n" +
                    "2. path to shared library written in c to be able to send messages via unix socket (optional)\n" +
                    "3. source, enum constant from class Sources, determines which part of sequence code will be increased (required)\n" +
                    "4. flag (required)\n" +
                    "5. host name, if it is \"\" then is set name of the computer where is test class running (optional)\n" +
                    "6. user name (required)\n" +
                    "7. srcInstance (optional)\n" +
                    "8. path to directory where will be saved files with events for each job (required)\n" +
                    "9. path to unix socket (required if path to shared library is set)\n" +
                    "10. description for event in this case event running (required)\n");
        } else {
            /* Create new instance of jobid, you can use other constructors too (see org.glite.jobid.api_java.Jobid.java) 
             * Examples:
             * Jobid jobid = new Jobid("https://skurut68-2.cesnet.cz:9000/paja6_test2"); 
             */
            Jobid jobid = new Jobid(args[0]);
            System.out.println("jobid: " + args[0]);

            /* Create sequence code
             * Example:
             * SeqCode seqCode = new SeqCode();
             * Insert sequence number in format 
             * UI=XXXXXX:NS=XXXXXXXXXX:WM=XXXXXX:BH=XXXXXXXXXX:JSS=XXXXXX:LM=XXXXXX:LRMS=XXXXXX:APP=XXXXXX:LBS=XXXXXX
             * where X is 0-9, or you can just create new instance of SeqCode where all parts are set to 0
             * Example:
             * SeqCode seqCode = new SeqCode();
             * seqCode.getSeqCodeFromString("UI=000001:NS=0000000002:WM=000003:BH=0000000004:" + 
             * "JSS=000005:LM=000006:LRMS=000007:APP=000008:LBS=000009"); 
             * seqCode.incrementSeqCode(Sources.USER_INTERFACE); 
             * resulting sequence code will be 
             * UI=000002:NS=0000000002:WM=000003:BH=0000000004:JSS=000005:LM=000006:LRMS=000007:APP=000008:LBS=000009
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
            System.out.println("pathToNativeLib: " + args[1]);

            /* Id of the message is some random unique number. 
             */
            ctx.setId(new Random().nextInt(99999999));

            /* Source indicates source of the message, it is constant from org.glite.lb.client_java.Sources class
             * and determines which part of sequence number will be increased.
             * Example: ctx.setSource(Sources.LRMS);
             * In this case we have to use method which converts args[2] to Sources. In real environment it will
             * not be used.
             */
            ctx.setSource(new Integer(args[2]));
            System.out.println("source: " + args[2]);

            /* Flag
             * Example: ctx.setFlag(0);
             */
            ctx.setFlag(new Integer(args[3]));
            System.out.println("flag: " + args[3]);

            /* Name of the computer where is locallogger running
             * Example: ctx.setHost("pelargir.ics.muni.cz");
             */
            ctx.setHost(args[4]);
            System.out.println("host: " + args[4]);

            /* Name of the user who owns the job.
             * Example: ctx.setUser("Pavel Piskac");
             */
            ctx.setUser(args[5]);
            System.out.println("user: " + args[5]);

            /* TODO co to vlastne znamena?
             * Mostly "" is set
             * Example: ctx.setSrcInstance("");
             */
            ctx.setSrcInstance(args[6]);
            System.out.println("srcInstance: " + args[6]);

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
            System.out.println("prefix: " + args[7]);

            /* Path to unix socket.
             * Example: ctx.setPathToSocket("/home/paja6/tmp/il.sock");
             */
            ctx.setPathToSocket(args[8]);
            System.out.println("pathToSocket: " + args[8]);

            /* Create new instance of the event which will be logged.
             */
            EventRunning running = new EventRunning();

            /* Set some description for the event.
             * Example: running.setNode("worker node");
             */
            running.setNode(args[9]);
            System.out.println("node: " + args[9]);

            /* And now is the context and event prepared to work.
             * 
             */
            ctx.log(running);
        }
    }
}
