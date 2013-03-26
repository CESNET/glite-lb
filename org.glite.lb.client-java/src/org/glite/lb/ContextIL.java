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

import org.glite.jobid.Jobid;

/**
 * Class which is used to send messages to inter-logger using unix socket.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public class ContextIL extends Context {

    private String socket;
    private String prefix;
    private int repeatWriteToFile = 5;
    private int connAttempts = 3;
    private int timeout = 3;
    private String owner = null;
    private String permissions = "g+rw";

    //tutorial http://java.sun.com/developer/onlineTraining/Programming/JDCBook/jni.html
    //native method which is written in C and imported to Java
    native int sendToSocket(String socket_path,
    long filepos,
    String msg,
    int msg_size,
    int conn_attempts,
    int timeout);
    
    /**
     * Creates new instance of ContextIL.
     */
    public ContextIL() {
	this.prefix = "/var/glite/log/dglogd.log";
    }

    public ContextIL(String prefix) {
        this.prefix = prefix;
    }

    public ContextIL(String prefix,String socket,String lib)
    {
	if (prefix == null) throw new IllegalArgumentException("ContextIL prefix");
	if ((socket != null && lib == null) || (socket == null && lib != null))
		throw new IllegalArgumentException("ContextIL both socket and lib must be set");

	this.prefix = prefix;
	this.socket = socket;

	if (lib != null) System.loadLibrary(lib);
    }

	

    /**
     * Writes event message to the file and socket.
     * 
     * @param event event
     * @throws java.lang.IllegalArgumentException if event, prefix or path
     */
    public void log(Event event) throws LBException {
        if (event == null) {
            throw new IllegalArgumentException("ContextIL event");
        }

        if (prefix == null) { 
            throw new IllegalArgumentException("ContextIL prefix");
        }

        String message = "DG.LLLID=\"0\"" + super.createMessage(event) +"\n";

	String file = prefix + "." + getJobid().getUnique();

        Long fileLength = ILFileWriter.write(file, message, repeatWriteToFile, owner, permissions);

	if (socket != null) sendToSocket(socket,fileLength.longValue(),message,message.length(),connAttempts,timeout);
    }

    public String getPrefix() {
        return prefix;
    }

    /**
     * Gets count of repeated write to file if some exception is thrown.
     * 
     * @return count of repeated write to file
     */
    public int getRepeatWriteToFile() {
        return repeatWriteToFile;
    }

    /**
     * Sets count of repeated write to file if some exception is thrown.
     * 
     * @param repeatWriteToFile count of repeated write to file
     */
    public void setRepeatWriteToFile(int repeatWriteToFile) {
        if (repeatWriteToFile < 1) {
            throw new IllegalArgumentException("ContextIL repeatWriteToFile");
        }

        this.repeatWriteToFile = repeatWriteToFile;
    }

    /**
     * Gets count of connection attempts which is used while sending the message via unix socket.
     * 
     * @return count of connection attempts
     */
    public int getConnAttempts() {
        return connAttempts;
    }

    /**
     * Sets count of connection attempts while sending the message via unix socket.
     * 
     * @param connAttempts count of connection attempts
     */
    public void setConnAttempts(int connAttempts) {
        if (connAttempts < 1) {
            throw new IllegalArgumentException("ContextIL conn_attempts");
        }

        this.connAttempts = connAttempts;
    }

    /**
     * Gets timeout which is used while sending the message via unix socket.
     * 
     * @return timeout
     */
    public int getTimeout() {
        return timeout;
    }

    /**
     * Sets timeout which is used while sending the message via unix socket.
     * 
     * @param timeout timeout
     */
    public void setTimeout(int timeout) {
        if (timeout < 1) {
            throw new IllegalArgumentException("ContextIL time_out");
        }

        this.timeout = timeout;
    }

    public String getPermissions() {
        return permissions;
    }

    public void setPermissions(String permissions) {
        this.permissions = permissions;
    }

    public String getOwner() {
        return owner;
    }

    public void setOwner(String owner) {
        this.owner = owner;
    }

}
