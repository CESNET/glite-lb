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


import java.util.Random;
import org.glite.jobid.Jobid;
import org.glite.lb.*;

/**
 * This class shows how to work with ContextIL.
 * 
 * @author Pavel Piskac
 */
public class ProducerTestLL {

    public static void main(String[] args) {

        if (args.length != 12) {
            System.out.println("How to use test class:\n" +
                    "you have to set 12 arguments in this order, if the choice is optional \"\" or text has to be set:\n" +
                    "1. jobid in format \"https://somewhere:port/unique_part\" (required)\n" +
                    "2. source, enum constant from class Sources, determines which part of sequence code will be increased (required)\n" +
                    "3. flag (required)\n" +
                    "4. host name, if it is \"\" then is set name of the computer where is test class running (optional)\n" +
                    "5. user name (required)\n" +
                    "6. srcInstace (optional)\n" +
                    "7. connection timeout (optional)\n" +
                    "8. proxy server address (required)\n" +
                    "9. proxy server port, default value is 9002 (optional)\n" +
                    "10. path to user's certificate (required)\n" + 
                    "11. path to directory where will be saved files with logs until inter-logger sends the content.\n" +
		    "12. worker node name for example event.");
        } else {
            /* Create new instance of jobid, you can use other constructors too (see org.glite.jobid.api_java.Jobid.java) 
             * Examples:
             * Jobid jobid = new Jobid("https://skurut68-2.cesnet.cz:9000/paja6_test2");
             * Jobid jobid = new Jobid("https://skurut68-2.cesnet.cz", 9000, "paja6_test2");
             * Jobid jobid = new Jobid("https://skurut68-2.cesnet.cz", 9000); //unique part is automatically generated
             * Jobid jobid = new Jobid();
             * jobid.setBkserver("https://skurut68-2.cesnet.cz");
             * jobid.setPort(9000);
             * jobid.setUnique("paja6_testProxy3");
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
            ContextLL ctx = new ContextLL();

            /* Id of the message is some random unique number. 
             * This value will be ceplaced by value from proxy.
             */
            ctx.setId(new Random().nextInt(99999999));

            /* Source indicates source of the message, it is constant from org.glite.lb.client_java.Sources class
             * and determines which part of sequence number will be increased.
             * Example: ctx.setSource(Sources.LRMS);
             * In this case we have to use method which converts args[2] to Sources. In real environment it will
             * not be used.
             */
            ctx.setSource(new Integer(args[1]));
            System.out.println("source: " + args[1]);

            /* Flag.
             * Example: ctx.setFlag(0);
             */
            ctx.setFlag(new Integer(args[2]));
            System.out.println("flag: " + args[2]);

            /* Name of the computer where is locallogger running
             * Example: ctx.setHost("pelargir.ics.muni.cz");
             */
            ctx.setHost(args[3]);
            System.out.println("host: " + args[3]);

            /* Name of the user who owns the job, this attribute will be replaced
             * by value get from certificate.
             * Example: ctx.setUser("Pavel Piskac");
             */
            ctx.setUser(args[4]);
            System.out.println("user: " + args[4]);

            /* TODO co to vlastne znamena?
             * Mostly "" is set
             * Example: ctx.setSrcInstance("");
             */
            ctx.setSrcInstance(args[5]);
            System.out.println("srcInstance: " + args[5]);

            /* Set the jobid for the context.
             */
            ctx.setJobid(jobid);

            /* Set the jobid for the context.
             */
            ctx.setSeqCode(seqCode);

            /* Timeout in seconds for the connection while sending the message via unix socket.
             * Default value is 3 but you can change it.
             */
            ctx.setTimeout(new Integer(args[6]));
            System.out.println("timeout: " + args[6]);

            /* Address to proxy.
             * Example:
             * ctx.setAddress("147.251.3.62");
             */
            ctx.setAddress(args[7]);
            System.out.println("address: " + args[7]);

            /* Proxy server port.
             * Example:
             * ctx.setPort(9002);
             */
            ctx.setPort(new Integer(args[8]));
            System.out.println("port: " + args[8]);

            /* Path to user's certificate. Only *.ks and *.p12 certificates are allowed.
             * Example:
             * ctx.setPathToCertificate("/home/paja6/myCertificate.p12");
             */
            ctx.setCredentials(new LBCredentials(args[9],null));
            System.out.println("pathToCertificate: " + args[9]);

            /* Path to directory where will be saved files with logs until inter-logger sends 
             * the content.
             * Example: ctx.setPrefix("/home/paja6/tmp/dglog." + jobid.getUnique());
             */
            ctx.setPrefix(args[10]);
            System.out.println("prefix: " + args[10]);

            /* Create new instance of the event which will be logged.
             */
            EventRunning running = new EventRunning();

            /* Set some description for the event.
             * Example: running.setNode("worker node");
             */
            running.setNode(args[11]);
            System.out.println("node: " + args[11]);

            /* And now is the context and event prepared to work.
             * 
             */
	    try { 
            	ctx.log(running);
	    } catch (LBException e) {
		e.printStackTrace();
	    }
        }
    }
}
