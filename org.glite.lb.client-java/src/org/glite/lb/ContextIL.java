package org.glite.lb;

import org.glite.jobid.Jobid;

/**
 * Class which is used to send messages to inter-logger using unix socket.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public class ContextIL extends Context {

    private String pathToSocket;
    private String pathToNativeLib;
    private String prefix;
    private int repeatWriteToFile = 5;
    private int connAttempts = 3;
    private int timeout = 3;
    private Boolean useUnixSocket = Boolean.TRUE;

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
    }

    /**
     * Creates new instance of ContextIL.
     * 
     * @param pathToSocket path to unix socket
     * @param prefix path where are stored messages
     */
    public ContextIL(String pathToSocket, String prefix) {
        this.prefix = prefix;
        this.pathToSocket = pathToSocket;
    }

    /**
     * Creates new instance of ContextIL.
     * 
     * @param id message id, if null, random number is generated
     * @param source one if paramaters of Sources enumeration
     * @param flag
     * @param host host name, if null or "", the name is get from host name of this computer
     * @param user user name
     * @param prog if null then is used "egd-wms"
     * @param srcInstance if null then it is set as ""
     * @param jobid jobid
     * @param path path to unix socket
     * @param prefix path where are stored messages
     * @throws java.lang.IllegalArgumentException if source, user, jobid, prefix 
     * or path is null or flag < 0
     */
    public ContextIL(int id,
            int source,
            int flag,
            String host,
            String user,
            String prog,
            String srcInstance,
            Jobid jobid,
            String pathToSocket,
            String prefix) {

        super(id, source, flag, host, user, prog, srcInstance, jobid);

        if (prefix == null) {
            throw new IllegalArgumentException("ContextIL prefix");
        }

        if (pathToSocket == null) {
            throw new IllegalArgumentException("ContextIL path");
        }

        this.prefix = prefix;
        this.pathToSocket = pathToSocket;
    }

    /**
     * Writes file position and message to specified socket.
     * 
     * @param pathToSocket path to unix socket
     * @param fileSize size of the file before new message was written there
     * @param message message which will be send
     * @param conn_attempts count of connection attempts
     * @param time_out connection timeout
     */
    private void writeToSocket(String pathToSocket,
            long fileSize,
            String message) {

        if (useUnixSocket.booleanValue()) {
            try {
                System.loadLibrary("glite_lb_sendviasocket");
            	message += '\n';
		    sendToSocket(pathToSocket,
                        fileSize,
                        message,
                        message.length(),
                        connAttempts,
                        timeout);
                
            } catch (UnsatisfiedLinkError ex) {
                useUnixSocket = Boolean.FALSE;
                System.err.println(ex);
            }
        }
    }

    /**
     * Writes event message to the file and socket.
     * 
     * @param event event
     * @throws java.lang.IllegalArgumentException if event, prefix or path
     */
    public void log(Event event) {
        if (event == null) {
            throw new IllegalArgumentException("ContextIL event");
        }

        if (prefix == null) { 
            throw new IllegalArgumentException("ContextIL prefix");
        }

        if (pathToSocket == null || pathToSocket.equals("")) { 
            pathToSocket = new String("");
            useUnixSocket = Boolean.FALSE;
        }

        if (pathToNativeLib == null || pathToNativeLib.equals("")) { 
            pathToNativeLib = new String("");
            useUnixSocket = Boolean.FALSE;
        }
        
        String message = super.createMessage(event);

        Long fileLength = ILFileWriter.write(prefix, message, repeatWriteToFile);

        writeToSocket(pathToSocket, fileLength.longValue(), message);
    }

    /**
     * Gets path to socket.
     * 
     * @return pathToSocket to socket
     */
    public String getPathToSocket() {
        return pathToSocket;
    }

    /**
     * Sets path to socket.
     * 
     * @param pathToSocket path to socket
     * @throws java.lang.IllegalArgumentException if path is null
     */
    public void setPathToSocket(String pathToSocket) {
        if (pathToSocket == null) {
            throw new IllegalArgumentException("ContextIL pathToSocket");
        }

        this.pathToSocket = pathToSocket;
    }

    /**
     * Gets path to nativelib file which is needed to send messages via unix socket
     * 
     * @return pathToNativeLib to native library (libnativelib.so)
     */
    public String getPathToNativeLib() {
        return pathToNativeLib;
    }

    /**
     * Sets path to nativelib file which is needed to send messages via unix socket
     * @param pathToNativeLib path to shared library (libnativelib.so)
     */
    public void setPathToNativeLib(String pathToNativeLib) {
        if (pathToNativeLib == null) {
            throw new IllegalArgumentException("ContextIL pathToNativeLib");
        }

        this.pathToNativeLib = pathToNativeLib;
    }

    /**
     * Gets path where are stored messages.
     * 
     * @return path where are stored messages
     */
    public String getPrefix() {
        return prefix;
    }

    /**
     * Sets path where are stored messages.
     * 
     * @param prefix path where are stored messages
     */
    public void setPrefix(String prefix) {
        if (prefix == null) {
            throw new IllegalArgumentException("ContextIL prefix");
        }

        this.prefix = prefix;
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

    public Boolean getUseUnixSocket() {
        return useUnixSocket;
    }

    public void setUseUnixSocket(Boolean useUnixSocket) {
        if (useUnixSocket == null) {
            throw new IllegalArgumentException("ContextIL useUnixSocket");
        }

        this.useUnixSocket = useUnixSocket;
    }
}
