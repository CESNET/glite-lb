package org.glite.lb;

import java.net.UnknownHostException;
import java.util.Calendar;
import java.util.Random;
import org.glite.jobid.Jobid;
import org.glite.jobid.CheckedString;

/**
 * Class representing a context for some job
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 * @version 15. 3. 2008
 */
public abstract class Context {

    private int id;
    private int source;
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
     * @param id message id, if null, random number is generated
     * @param source one if paramaters of Sources enumeration
     * @param flag
     * @param host host name, if null or "", the name is get from host name of this computer
     * @param user user name
     * @param prog if null then is used "egd-wms"
     * @param srcInstance if null then it is set as ""
     * @param jobid
     * @throws java.lang.IllegalArgumentException if user or jobid is null 
     *  or flag < 0 or source <=0 || >= 9 
     * 
     */
    public Context(int id,
            int source,
            int flag,
            String host,
            String user,
            String prog,
            String srcInstance,
            Jobid jobid) {
        if (id < 0) {
            id = new Random().nextInt();
        }

        if (source <= -1 || source > Sources.EDG_WLL_SOURCE_LB_SERVER) {
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

        this.id = id;
        this.source = source;
        this.flag = flag;
        this.host = new CheckedString(host).toString();
        this.user = new CheckedString(user).toString();
        this.prog = new CheckedString(prog).toString();
        this.srcInstance = new CheckedString(srcInstance).toString();
        this.jobid = jobid;
    }

    /**
     * Converts Sources enum constants to defined string
     * @param sourceEnum Sources enum constant
     * @return String representation of Sources enum constants
     * @throws IllegalArgumentException if wrong source type is set
     */
    private String recognizeSource(int sourceEnum) {
        switch (sourceEnum) {
            case Sources.EDG_WLL_SOURCE_NONE: return "Undefined";
            case Sources.EDG_WLL_SOURCE_USER_INTERFACE: return "UserInterface";
            case Sources.EDG_WLL_SOURCE_NETWORK_SERVER: return "NetworkServer";
            case Sources.EDG_WLL_SOURCE_WORKLOAD_MANAGER: return "WorkloadManager";
            case Sources.EDG_WLL_SOURCE_BIG_HELPER: return "BigHelper";
            case Sources.EDG_WLL_SOURCE_JOB_SUBMISSION: return "JobController";
            case Sources.EDG_WLL_SOURCE_LOG_MONITOR: return "LogMonitor";
            case Sources.EDG_WLL_SOURCE_LRMS: return "LRMS";
            case Sources.EDG_WLL_SOURCE_APPLICATION: return "Application";
            case Sources.EDG_WLL_SOURCE_LB_SERVER: return "LBServer";
            default: throw new IllegalArgumentException("wrong source type");
        }
    }

    /**
     * Abstract method which will serve as method for sending messages with events.
     * @param event event for which will be created and send message
     */
    public abstract void log(Event event);
    
    /**
     * Creates message prepared to send
     * @param event event for which is message generated
     * @throws IllegalArgumentException if event, source, user or job is null
     * or flag < 0
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

        if (source <= -1 || source > Sources.EDG_WLL_SOURCE_LB_SERVER) {
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

        if (prog == null) {
            prog = new String("edg-wms");
        }

        if (user == null) {
            throw new IllegalArgumentException("Context user");
        }

        if (srcInstance == null) {
            srcInstance = new String("");
        }

        String output;
        String date = "";
        String tmp;
        date = String.valueOf(Calendar.getInstance().get(Calendar.YEAR));
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.MONTH) + 1);
        date += "00".substring(0, 2 - tmp.length()) + tmp;
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.DATE));
        date += "00".substring(0, 2 - tmp.length()) + tmp;
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.HOUR));
        date += "00".substring(0, 2 - tmp.length()) + tmp;
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.MINUTE));
        date += "00".substring(0, 2 - tmp.length()) + tmp;
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.SECOND));
        date += "00".substring(0, 2 - tmp.length()) + tmp;
        date += ".";
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.MILLISECOND));
        String tmp2 = "000".substring(0, 3 - tmp.length()) + tmp;
        date += tmp2 + "000000".substring(tmp.length(), 6);

        seqCode.incrementSeqCode(source);

        output = ("DG.LLLID=" + id +
                " DG.USER=\"" + user + "\"" +
                " DATE=" + date +
                " HOST=\"" + host + "\"" +
                " PROG=" + prog +
                " LVL=SYSTEM" +
                " DG.PRIORITY=0" +
                " DG.SOURCE=\"" + recognizeSource(source) + "\"" +
                " DG.SRC_INSTANCE=\"" + srcInstance + "\"" +
                " DG.EVNT=\"" + event.getEventType() + "\"" +
                " DG.JOBID=\"" + jobid + "\"" +
                " DG.SEQCODE=\"" + seqCode + "\"" +
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
     * @param flag
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
     * Gets message id.
     * 
     * @return message id
     */
    public int getId() {
        return id;
    }

    /**
     * Sets message id.
     * 
     * @param id message id
     * @throws java.lang.IllegalArgumentException if id is lower than 0
     */
    public void setId(int id) {
        if (id < 0) {
            throw new IllegalArgumentException("Context id");
        }
        this.id = id;
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
     * @param jobid
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
     * @param prog
     */
    public void setProg(String prog) {
        if (prog == null) {
            prog = new String("edg-wms");
        }

        this.prog = (new CheckedString(prog)).toString();
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
    public int getSource() {
        return source;
    }

    /**
     * Sets source which represents which part of sequence code will be changed
     * @param source source
     * @throws java.lang.IllegalArgumentException if source is null
     */
    public void setSource(int source) {
        if (source <= -1 || source > Sources.EDG_WLL_SOURCE_LB_SERVER) {
            throw new IllegalArgumentException("Context source");
        }

        this.source = source;
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

        this.srcInstance = new CheckedString(srcInstance).toString();
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

        this.user = (new CheckedString(user)).toString();
    }
}
