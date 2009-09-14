import org.glite.lb.*;
import org.glite.jobid.Jobid;

public class CreamTest {

public static void main(String[] args)
{

   try {
	String[] srvpart = args[0].split(":");
	int srvport = Integer.parseInt(srvpart[1]);
	Jobid	job = new Jobid(srvpart[0],srvport);

	LBCredentials	cred = new LBCredentials(System.getenv("X509_USER_PROXY"),"/etc/grid-security/certificates");


	ContextDirect	ctxd = new ContextDirect(srvpart[0],srvport);
	ctxd.setCredentials(cred);
	ctxd.setSource(Sources.EDG_WLL_SOURCE_CREAM_CORE);
	ctxd.setJobid(job);
	ctxd.setSeqCode(new SeqCode(SeqCode.CREAM,"no_seqcodes_with_cream"));

	EventRegJob	reg = new EventRegJob();
	reg.setNs("https://where.is.cream:1234");
	reg.setJobtype(EventRegJob.Jobtype.JOBTYPE_CREAM);

	ctxd.log(reg);

	System.out.println("JOBID="+job);



/*
	reg.setJdl


	ContextIL	ctx = new ContextIL();
*/
	

   } catch (Exception e)
   {
	System.err.println("Oops");
	e.printStackTrace();
   }

}



}
