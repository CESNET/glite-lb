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

import java.text.SimpleDateFormat;

import java.net.UnknownHostException;
import java.util.Date;
import java.util.Random;
import java.util.SimpleTimeZone;
import org.glite.jobid.Jobid;

/**
 * Class representing a context for some job
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 * @version 15. 3. 2008
 */
public abstract class Context {

    private Sources source;
    private int flag;
    private String host;
    private String user;
    private String prog;
    private String srcInstance;
    private Jobid jobid;
    private SeqCode seqCode;
    
    /**
     * Creates new instance of Context class.
     */
    public Context() {
    }

    /**
     * Creates new instance of Context class.
     *
     * @param src one if paramaters of Sources enumeration
     * @param flag flag to set
     * @param host host name, if null or "", the name is get from host name of this computer
     * @param user user name
     * @param prog if null then is used "egd-wms"
     * @param srcInstance if null then it is set as ""
     * @param jobid jobId to set
     * @throws java.lang.IllegalArgumentException if user or jobid is null
     *  or flag &lt; 0 or source &lt;=0 || &gt;= 9
     *
     */
    public Context(Sources src,
            int flag,
            String host,
            String user,
            String prog,
            String srcInstance,
            Jobid jobid) {

        if (src == null) {
            throw new IllegalArgumentException("Context source");
        }

        if (flag < 0) {
            throw new IllegalArgumentException("Context flag");
        }

        if (host == null || host.equals("")) {
            try {
                host = java.net.InetAddress.getLocalHost().getHostName();
            } catch (UnknownHostException ex) {
                System.err.println(ex);
            }
        }

        if (user == null) {
            throw new IllegalArgumentException("Context user");
        }

        if (prog == null) {
            prog = new String("edg-wms");
        }

        if (srcInstance == null) {
            srcInstance = new String("");
        }

        if (jobid == null) {
            throw new IllegalArgumentException("Context jobid");
        }

        this.source = src;
        this.flag = flag;
        this.host = host;
        this.user = user;
        this.prog = prog;
        this.srcInstance = srcInstance;
        this.jobid = jobid;
    }

    /**
     * Abstract method which will serve as method for sending messages with events.
     * @param event event for which will be created and send message
     * @throws LBException when logging fails
     */
    public abstract void log(Event event) throws LBException;

    /**
     * Creates message prepared to send
     * @param event event for which is message generated
     * @throws IllegalArgumentException if event, source, user or job is null
     * or flag &lt; 0
     * @return output String with message
     */
    protected String createMessage(Event event) {
        if (event == null) {
            throw new IllegalArgumentException("Context event");
        }
        
        if (jobid == null) {
            throw new IllegalArgumentException("Context jobid");
        }

        if (jobid.getBkserver() == null) {
            throw new IllegalArgumentException("Context Jobid bkserver");
        }

        if (jobid.getPort() <= 0 || jobid.getPort() >= 65536) {
            throw new IllegalArgumentException("Context Jobid port");
        }
        
        if (jobid.getUnique() == null) {
            throw new IllegalArgumentException("Context Jobid unique");
        }
        
        if (event == null) {
            throw new IllegalArgumentException("Context event");
        }

        if (seqCode == null) {
            throw new IllegalArgumentException("Context seqCode");
        }

        if (flag < 0) {
            throw new IllegalArgumentException("Context flag");
        }

        if (host == null || host.equals("")) {
            try {
                host = java.net.InetAddress.getLocalHost().getHostName();
            } catch (UnknownHostException ex) {
                System.err.println(ex);
            }
        }

        if (prog == null) {
            prog = new String("edg-wms");
        }

        if (user == null) {
            throw new IllegalArgumentException("Context user");
        }

        if (srcInstance == null) {
            srcInstance = new String("");
        }

	SimpleDateFormat df = new SimpleDateFormat("yyyyMMddHHmmssSSS");
	df.setTimeZone(new SimpleTimeZone(0, "UTC"));
	String date = df.format(new Date()) + "000";
	
        if (seqCode != null) seqCode.incrementSeqCode(source);

        String output = (" DG.USER=\"" + Escape.ulm(user) + "\"" +
                " DATE=" + date +
                " HOST=\"" + Escape.ulm(host) + "\"" +
                " PROG=" + Escape.ulm(prog) +
                " LVL=SYSTEM" +
                " DG.PRIORITY=0" +
                " DG.SOURCE=\"" + source + "\"" +
                " DG.SRC_INSTANCE=\"" + Escape.ulm(srcInstance) + "\"" +
                " DG.EVNT=\"" + event.getEventType() + "\"" +
                " DG.JOBID=\"" + jobid + "\"" +
                " DG.SEQCODE=\"" + Escape.ulm(seqCode.toString()) + "\"" +
                event.ulm());

        return output;
    }

    /**
     * Return flag
     * 
     * @return flag
     */
    public int getFlag() {
        return flag;
    }

    /**
     * Set flag
     * 
     * @param flag flag to set
     * @throws java.lang.IllegalArgumentException if flag is lower than 0
     */
    public void setFlag(int flag) {
        if (flag < 0) {
            throw new IllegalArgumentException("Context flag");
        }

        this.flag = flag;
    }

    /**
     * Returns host name
     * 
     * @return host name
     */
    public String getHost() {
        return host;
    }

    /**
     * Sets host name
     * @param host host name
     * @throws java.lang.IllegalArgumentException if host is null
     */
    public void setHost(String host) {
        if (host == null) {
            throw new IllegalArgumentException("Context host");
        }

        this.host = host;
    }

    /**
     * Gets jobid.
     * 
     * @return jobid
     */
    public Jobid getJobid() {
        return jobid;
    }

    /**
     * Sets jobid.
     * 
     * @param jobid jobId to set
     * @throws java.lang.IllegalArgumentException if jobid is null
     */
    public void setJobid(Jobid jobid) {
        if (jobid == null) {
            throw new IllegalArgumentException("Context jobid");
        }

        this.jobid = jobid;
    }

    /**
     * Gets prog.
     * @return prog
     */
    public String getProg() {
        return prog;
    }

    /**
     * Sets prog, if prog is null then is set default value "edg-wms"
     * @param prog program to set or null (default is "edg-wms")
     */
    public void setProg(String prog) {
        if (prog == null) {
            prog = new String("edg-wms");
        }

        this.prog = prog;
    }

    /**
     * Gets sequence code.
     * 
     * @return sequence code
     */
    public SeqCode getSeqCode() {
        return seqCode;
    }

    /**
     * Sets sequence code.
     * @param seqCode sequence code
     * @throws java.lang.IllegalArgumentException if seqCode is null
     */
    public void setSeqCode(SeqCode seqCode) {
        if (seqCode == null) {
            throw new IllegalArgumentException("Context seqCode");
        }

        this.seqCode = seqCode;
    }

    /**
     * Gets source which represents which part of sequence code will be changed
     * @return source
     */
    public Sources getSource() {
        return source;
    }

    /**
     * Sets source which represents which part of sequence code will be changed
     * @param src source
     * @throws java.lang.IllegalArgumentException if source is null
     */
    public void setSource(Sources src) {
        this.source = src;
    }

    /**
     * Gets srcInstance.
     * @return srcInstance
     */
    public String getSrcInstance() {
        return srcInstance;
    }

    /**
     * Sets srcInstace, if srcInstace null then is set "".
     * @param srcInstance srcInstance
     */
    public void setSrcInstance(String srcInstance) {
        if (srcInstance == null) {
            srcInstance = new String("");
        }

        this.srcInstance = srcInstance;
    }

    /**
     * Gets user name.
     * @return user name
     */
    public String getUser() {
        return user;
    }

    /**
     * Sets user name.
     * @param user user name
     * @throws java.lang.IllegalArgumentException if user is null
     */
    public void setUser(String user) {
        if (user == null) {
            throw new IllegalArgumentException("Context user");
        }

        this.user = user;
    }
}
