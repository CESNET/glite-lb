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

import org.glite.lb.*;
import org.glite.jobid.Jobid;

public class CreamTest {

public static void main(String[] args)
{

   int i;
   String srv = null,socket = null,prefix = null,lib = "glite_lb_sendviasocket";

   for (i = 0; i < args.length; i++) {
	if (args[i].equals("-m")) srv = args[++i];
	else if (args[i].equals("-s")) socket = args[++i];
	else if (args[i].equals("-f")) prefix = args[++i];
	else if (args[i].equals("-l")) lib = args[++i];		/* needs java.library.path */
   }

   try {
	String[] srvpart = srv.split(":");
	int srvport = Integer.parseInt(srvpart[1]);
	Jobid	job = new Jobid(srvpart[0],srvport);

	LBCredentials	cred = new LBCredentials(System.getenv("X509_USER_PROXY"),"/etc/grid-security/certificates");


	ContextDirect	ctxd = new ContextDirect(srvpart[0],srvport);
	ctxd.setCredentials(cred);
	ctxd.setSource(new Sources(Sources.CREAM_EXECUTOR));
	ctxd.setJobid(job);
	ctxd.setSeqCode(new SeqCode(SeqCode.CREAM,"no_seqcodes_with_cream"));


/* initial registration goes directly */
	EventRegJob	reg = new EventRegJob();
	reg.setNs("https://where.is.cream:1234");
	reg.setJobtype(EventRegJob.Jobtype.CREAM);
	ctxd.log(reg);

	System.out.println("JOBID="+job);

	ContextIL	ctx = new ContextIL(prefix,socket,lib);
	ctx.setSource(new Sources(Sources.CREAM_EXECUTOR));
	ctx.setJobid(job);
	ctx.setSeqCode(new SeqCode(SeqCode.CREAM,"no_seqcodes_with_cream_cheat_duplicate"));
	ctx.setUser(ctxd.getUser());

/* 2nd registration with JDL, via IL */
	reg.setJdl("[\n\ttest = \"hellow, world\";\n]");
	ctx.log(reg);

	Event e = new EventCREAMStart();
	ctx.log(e);

	EventCREAMStore store = new EventCREAMStore();
	store.setResult(EventCREAMStore.Result.START);
	store.setCommand(EventCREAMStore.Command.CMDSTART);
	ctx.log(store);

	EventCREAMCall call = new EventCREAMCall();
	call.setCallee(new Sources(Sources.LRMS));
	call.setDestid("fake_Torque_ID");
	call.setResult(EventCREAMCall.Result.OK);
	call.setCommand(EventCREAMCall.Command.CMDSTART);
	
	ctx.log(call);

   } catch (Exception e)
   {
	System.err.println("Oops");
	e.printStackTrace();
   }

}



}
