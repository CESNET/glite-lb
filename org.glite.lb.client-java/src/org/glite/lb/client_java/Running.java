package org.glite.lb.client_java;

import org.glite.jobid.api_java.CheckedString;

/**
 * Class which represents event "running".
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public class Running extends Event {
    
    private String node;
    
    /**
     * Creates new instance.
     */
    public Running() {
    }
    
    /**
     * Creates new instance, where is set variable node.
     * @param node node
     */
    public Running(String node) {
        if (node == null) {
            node = new String("");
        }
        
        this.node = node;
    }

    /**
     * Returns part of message, which will be joined to the of the message. 
     * Its format is " DG.RUNNING.NODE=\"" + node + "\""
     * @return part of message which is specific for event running 
     */
    public String ulm() {  
        return (" DG.RUNNING.NODE=\"" + node + "\"");
    }
    
    /**
     * Returns event type specific for event running ("Running").
     * 
     * @return event type
     */
    public String getEventType() {
        return "Running";
    }
    
    /**
     * Gets node.
     * 
     * @return node
     */
    public String getNode() {
        return node;
    }

    /**
     * Sets node, if node is null then it is "" set.
     * @param node
     */
    public void setNode(String node) {
        if (node == null) {
            node = new String("");
        }
        
        this.node = new CheckedString(node).toString();
    }
}
