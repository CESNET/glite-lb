package org.glite.lb.client_java;

/**
 *
 * @author xpiskac
 */
public class Transfer extends Event {
    
    private String destination;
    private String destHostname;
    private String destInstance;
    private String jobDescription;
    private String result;
    private String reason;
    private String destJobid;
    
    public Transfer() {
    }
    
    public Transfer (String destination,
                     String destHostname,
                     String destInstance,
                     String jobDescription,
                     String result,
                     String reason,
                     String destJobid) {
        
        this.destination = destination;
        this.destHostname = destHostname;
        this.destInstance = destInstance;
        this.jobDescription = jobDescription;
        this.result = result;
        this.reason = reason;
        this.destJobid = destJobid;
    }

    public String ulm() {
        return null; //zatim neimplementovano
    }
    
    public String getEventType() {
        return "Transfer";
    }
    
    public String getDestHostname() {
        return destHostname;
    }

    public void setDestHostname(String destHostname) {
        this.destHostname = destHostname;
    }

    public String getDestInstance() {
        return destInstance;
    }

    public void setDestInstance(String destInstance) {
        this.destInstance = destInstance;
    }

    public String getDestJobid() {
        return destJobid;
    }

    public void setDestJobid(String destJobid) {
        this.destJobid = destJobid;
    }

    public String getDestination() {
        return destination;
    }

    public void setDestination(String destination) {
        this.destination = destination;
    }

    public String getJobDescription() {
        return jobDescription;
    }

    public void setJobDescription(String jobDescription) {
        this.jobDescription = jobDescription;
    }

    public String getReason() {
        return reason;
    }

    public void setReason(String reason) {
        this.reason = reason;
    }

    public String getResult() {
        return result;
    }

    public void setResult(String result) {
        this.result = result;
    }
}
