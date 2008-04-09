package org.glite.lb.client_java;

import java.net.UnknownHostException;
import java.util.Calendar;
import java.util.Random;
import org.glite.jobid.api_java.Jobid;

/**
 * Class representing a context for some job
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 * @version 15. 3. 2008
 */
public class Context {
    
    private int id;
    private Sources source;
    private int flag;
    private String host;
    private String user;
    private String prog;
    private String srcInstance;
    private Jobid jobid;
    private SeqCode seqCode;
    private String message;
    
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
     * @throws java.lang.IllegalArgumentException if source, user or jobid is null 
     *  or flag < 0 
     * 
     */
    public Context(int id,
                   Sources source,
                   int flag,
                   String host,
                   String user,
                   String prog,
                   String srcInstance,
                   Jobid jobid) {
        if (id < 0) {
            id = new Random().nextInt();
        }
        
        if (source == null)
        {
            throw new IllegalArgumentException("Context source");
        }
        
        if (flag < 0 || flag > 8)
        {
            throw new IllegalArgumentException("Context flag");
        }
        
        if (host == null || host.equals(""))
        {
            try {
                host = java.net.InetAddress.getLocalHost().getHostName();
            } catch (UnknownHostException ex) {
               System.err.println(ex);
            }
        }
        
        if (user == null)
        {
            throw new IllegalArgumentException("Context user");
        }
        
        if (prog == null) {
            prog = new String("edg-wms");
        }
        
        if (srcInstance == null)
        {
            srcInstance = new String("");
        }
        
        if (jobid == null)
        {
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
    private String recognizeSource(Sources sourceEnum) {
        switch (sourceEnum) {
            case EDG_WLL_SOURCE_NONE: return "Undefined";
            case EDG_WLL_SOURCE_USER_INTERFACE: return "UserInterface";
            case EDG_WLL_SOURCE_NETWORK_SERVER: return "NetworkServer";
            case EDG_WLL_SOURCE_WORKLOAD_MANAGER: return "WorkloadManager";
            case EDG_WLL_SOURCE_BIG_HELPER: return "BigHelper";
            case EDG_WLL_SOURCE_JOB_SUBMISSION: return "JobController";
            case EDG_WLL_SOURCE_LOG_MONITOR: return "LogMonitor";
            case EDG_WLL_SOURCE_LRMS: return "LRMS";
            case EDG_WLL_SOURCE_APPLICATION: return "Application";
            case EDG_WLL_SOURCE_LB_SERVER: return "LBServer";
            default: throw new IllegalArgumentException("wrong source type");
        }
    }
    
    public Sources stringToSources(String source) {
        if (source.equals("EDG_WLL_SOURCE_NONE")) return Sources.EDG_WLL_SOURCE_NONE;
        if (source.equals("EDG_WLL_SOURCE_USER_INTERFACE")) return Sources.EDG_WLL_SOURCE_USER_INTERFACE;
        if (source.equals( "EDG_WLL_SOURCE_NETWORK_SERVER")) return Sources.EDG_WLL_SOURCE_NETWORK_SERVER;
        if (source.equals("EDG_WLL_SOURCE_WORKLOAD_MANAGER")) return Sources.EDG_WLL_SOURCE_WORKLOAD_MANAGER;
        if (source.equals("EDG_WLL_SOURCE_BIG_HELPER")) return Sources.EDG_WLL_SOURCE_BIG_HELPER;
        if (source.equals("EDG_WLL_SOURCE_JOB_SUBMISSION")) return Sources.EDG_WLL_SOURCE_JOB_SUBMISSION;
        if (source.equals("EDG_WLL_SOURCE_LOG_MONITOR")) return Sources.EDG_WLL_SOURCE_LOG_MONITOR;
        if (source.equals("EDG_WLL_SOURCE_LRMS")) return Sources.EDG_WLL_SOURCE_LRMS;
        if (source.equals("EDG_WLL_SOURCE_APPLICATION")) return Sources.EDG_WLL_SOURCE_APPLICATION;
        if (source.equals("EDG_WLL_SOURCE_LB_SERVER")) return Sources.EDG_WLL_SOURCE_LB_SERVER;
        throw new IllegalArgumentException("wrong source type");
        
    }
    
    /**
     * Creates message prepared to send
     * @param event event for which is message generated
     * @throws IllegalArgumentException if event, source, user or job is null
     * or flag < 0
     */
    public void log(Event event) { 
        if (event == null) {
            throw new IllegalArgumentException("Context event");
        }
        
        if (source == null)
        {
            throw new IllegalArgumentException("Context source");
        }
        
        if (flag < 0 || flag > 8)
        {
            throw new IllegalArgumentException("Context flag");
        }
        
        if (host == null || host.equals(""))
        {
            try {
                host = java.net.InetAddress.getLocalHost().getHostName();
            } catch (UnknownHostException ex) {
               System.err.println(ex);
            }
        }
        
        if (prog == null) {
            prog = new String("edg-wms");
        }
        
        if (user == null)
        {
            throw new IllegalArgumentException("Context user");
        }
        
        if (srcInstance == null)
        {
            srcInstance = new String("");
        }
        
        if (jobid == null)
        {
            throw new IllegalArgumentException("Context jobid");
        }
        
        String output;
        String date = "";
        String tmp;
        date = String.valueOf(Calendar.getInstance().get(Calendar.YEAR));
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.MONTH) + 1);
        date += "00".substring(0, 2 - tmp.length ()) + tmp;
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.DATE));
        date += "00".substring(0, 2 - tmp.length ()) + tmp;
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.HOUR));
        date += "00".substring(0, 2 - tmp.length ()) + tmp;
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.MINUTE));
        date += "00".substring(0, 2 - tmp.length ()) + tmp;
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.SECOND));
        date += "00".substring(0, 2 - tmp.length ()) + tmp;
	date += ".";
        tmp = String.valueOf(Calendar.getInstance().get(Calendar.MILLISECOND));
        String tmp2 = "000".substring(0, 3 - tmp.length ()) + tmp;
        date += tmp2 + "000000".substring(tmp.length (), 6);
        
        seqCode.setPardOfSeqCode(flag, seqCode.getPardOfSeqCode(flag)+1);
        
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
        
        this.message = output;
    }

    /**
     * Return flag which represents which part of sequence code will be changes
     * 
     * @return flag
     */
    public int getFlag() {
        return flag;
    }

    /**
     * Set flag which represents which part of sequence code will be changes
     * 
     * @param flag
     * @throws java.lang.IllegalArgumentException if flag is lower than 0
     */
    public void setFlag(int flag) {
        if (flag < 0 || flag > 8)
        {
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
        if (host == null)
        {
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
        if (id < 0)
        {
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
        if (jobid == null)
        {
            throw new IllegalArgumentException("Context jobid");
        }
        
        this.jobid = jobid;
    }

    /**
     * Gets message which is prepared to send.
     * @return message
     */
    public String getMessage() {
        return message;
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
     * Gets source.
     * @return source
     */
    public Sources getSource() {
        return source;
    }

    /**
     * Sets source
     * @param source source
     * @throws java.lang.IllegalArgumentException if source is null
     */
    public void setSource(Sources source) {
        if (source == null)
        {
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
        if (srcInstance == null)
        {
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
        if (user == null)
        {
            throw new IllegalArgumentException("Context user");
        }
        
        this.user = (new CheckedString(user)).toString();
    }
}
