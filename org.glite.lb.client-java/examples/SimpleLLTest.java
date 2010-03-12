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
