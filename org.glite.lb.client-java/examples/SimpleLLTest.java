import java.util.Random;
import org.glite.jobid.Jobid;
import org.glite.lb.*;

/**
 * This class shows how to work with ContextLL.
 * 
 * @author Pavel Piskac
 */
public class SimpleLLTest {

    public static void main(String[] args) {

     try {
        Jobid jobid = new Jobid("https://skurut68-2.cesnet.cz:9000/paja6_test2");
        SeqCode seqCode = new SeqCode();
        ContextLL ctx = new ContextLL();
        ctx.setId(new Random().nextInt(99999999));
        ctx.setSource(1);
        ctx.setFlag(0);
        ctx.setHost("pelargir.ics.muni.cz");
        ctx.setUser("Pavel Piskac");
        ctx.setSrcInstance("");
        ctx.setJobid(jobid);
        ctx.setSeqCode(seqCode);
        ctx.setTimeout(5);
        ctx.setAddress("localhost");
        ctx.setPort(9002);
        ctx.setCredentials(new LBCredentials(args[0],null));
        ctx.setPrefix("/tmp/dglog.paja6_testProxy6");
        EventRunning running = new EventRunning();
        running.setNode("node");
        ctx.log(running);
      }
      catch (Exception e) { e.printStackTrace(); }
    }
}
