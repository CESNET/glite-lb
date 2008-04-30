package org.glite.lb;

/**
 * Abstract class which serves as base for all events.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public abstract class Event {
    
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
}
