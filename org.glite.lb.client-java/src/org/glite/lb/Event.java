package org.glite.lb;

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.GregorianCalendar;
import org.glite.jobid.Jobid;

/**
 * Abstract class which serves as base for all events.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public abstract class Event {

    private Timeval timestamp;
    private Timeval arrived;
    private String host;
    private Level level;
    private Integer priority;
    private Jobid jobId;
    private SeqCode seqcode;
    private String user;
    private Sources source;
    private String srcInstance;

     public Event() {
    }

    public Event(Jobid jobId) {
        if (jobId == null) throw new IllegalArgumentException("jobId");

        this.jobId = jobId;
    }

    public Jobid getJobId() {
        return jobId;
    }

    /**
     * When implemented, this method returns string which is specific for each event.
     * 
     * @return specific string
     */
    public abstract String ulm();

    /**
     * When implemented, this method returns name of event type.
     * 
     * @return name of event
     */
    public abstract String getEventType();

/*
    public String info() {
        String date = "";
        if (arrived != null) {
            Calendar calendar = new GregorianCalendar();
            DateFormat df = new SimpleDateFormat("dd/MM/yyyy HH:mm:ss");
            calendar.setTimeInMillis(arrived.getTvSec()*1000);
            date = df.format(calendar.getTime());
        }
        return "DATE=" + date + " HOST=\"" + host + "\" " +
                "LVL="+level+" DG.PRIORITY=" + priority + " DG.SOURCE=\""+
                source + "\" "+"DG.SRC_INSTANCE=\""+srcInstance+"\" "+
                "DG.EVNT=\""+getEventType()+"\" "+"DG.JOBID=\""+jobId+"\" "+
                "DG.SEQCODE=\""+seqcode+"\" "+"DG.USER=\""+user+"\""+ulm();
    }
*/

    /**
     * Get and set methods for Event attributes.
     */

    public Timeval getArrived() {
        return arrived;
    }

    public void setArrived(Timeval arrived) {
        if (arrived == null) throw new IllegalArgumentException("arrived");
        this.arrived = arrived;
    }

    public String getHost() {
        return host;
    }

    public void setHost(String host) {
        if (host == null) throw new IllegalArgumentException("host");
        this.host = host;
    }


    public void setJobId(Jobid jobId) {
        if (jobId == null) throw new IllegalArgumentException("jobId");
        this.jobId = jobId;
    }

    public Level getLevel() {
        return level;
    }

    public void setLevel(Level level) {
        if (level == null) throw new IllegalArgumentException("level");
        this.level = level;
    }

    public Integer getPriority() {
        return priority;
    }

    public void setPriority(Integer priority) {
        this.priority = priority;
    }

    public SeqCode getSeqcode() {
        return seqcode;
    }

    public void setSeqcode(SeqCode seqcode) {
        if (seqcode == null) throw new IllegalArgumentException("seqcode");
        this.seqcode = seqcode;
    }

    public Sources getSource() {
        return source;
    }

    public void setSource(Sources source) {
        if (source == null) throw new IllegalArgumentException("source");
        this.source = source;
    }

    public String getSrcInstance() {
        return srcInstance;
    }

    public void setSrcInstance(String srcInstance) {
        if (srcInstance == null) throw new IllegalArgumentException("srcInstance");
        this.srcInstance = srcInstance;
    }

    public Timeval getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Timeval timestamp) {
        if (timestamp == null) throw new IllegalArgumentException("timestamp");
        this.timestamp = timestamp;
    }

    public String getUser() {
        return user;
    }

    public void setUser(String user) {
        if (user == null) throw new IllegalArgumentException("user");
        this.user = user;
    }


}
