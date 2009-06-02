package org.glite.lb;

import org.glite.jobid.Jobid;

/** 
 * This class provides sending messages using network sockets.
 * 
 * @author Pavel Piskac (173297@mail.muni.cz)
 */
public class ContextLL extends Context {

    private String address;
    private int port = 9002;
    private String prefix;
    private int repeatWriteToFile = 5;
    private int timeout = 30000; //in milliseconds
    private String pathToCertificate;
    private SSLSend sslSend = null;

    public ContextLL() {
    }

    public ContextLL(String address, int port, String prefix) {
        this.prefix = prefix;
        this.address = address;
        this.port = port;
    }

    public ContextLL(int id,
            int source,
            int flag,
            String host,
            String user,
            String prog,
            String srcInstance,
            Jobid jobid,
            String address,
            int port,
            String prefix) {

        super(id, source, flag, host, user, prog, srcInstance, jobid);

        if (prefix == null) {
            throw new IllegalArgumentException("ContextLL prefix");
        }
        if (address == null) {
            throw new IllegalArgumentException("ContextLL socket");
        }
        if (port < 0) {
            throw new IllegalArgumentException("ContextLL port");
        }

        this.prefix = prefix;
        this.address = address;
    }

    @Override
    public void log(Event event) throws LBException {
        if (event == null) {
            throw new IllegalArgumentException("ContextLL event");
        }

        if (prefix == null) {
            throw new IllegalArgumentException("ContextLL prefix");
        }

        if (address == null) {
            throw new IllegalArgumentException("ContextLL socket");
        }

        if (port < 0) {
            throw new IllegalArgumentException("ContextLL port");
        }

        if (sslSend == null) {
            sslSend = new SSLSend();
        }
        
        String message = super.createMessage(event);

        ILFileWriter.write(prefix, message, repeatWriteToFile);
        
        
        sslSend.send(pathToCertificate, address, port, timeout, message);
    }

    public String getAddress() {
        return address;
    }

    public void setAddress(String address) {
        if (address == null) {
            throw new IllegalArgumentException("ContextLL address");
        }
        
        this.address = address;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        if (port < 0) {
            throw new IllegalArgumentException("ContextLL port");
        }
        this.port = port;
    }
    
    
    public String getPrefix() {
        return prefix;
    }

    public void setPrefix(String prefix) {
        if (prefix == null) {
            throw new IllegalArgumentException("ContextLL prefix");
        }
        
        this.prefix = prefix;
    }

    public int getRepeatWriteToFile() {
        return repeatWriteToFile;
    }

    public void setRepeatWriteToFile(int repeatWriteToFile) {
        if (repeatWriteToFile < 1) {
            throw new IllegalArgumentException("ContextLL repeatWriteToFile");
        }
        
        this.repeatWriteToFile = repeatWriteToFile;
    }

    public int getTimeout() {
        return timeout;
    }

    public void setTimeout(int timeout) {
        if (timeout < 0) {
            throw new IllegalArgumentException("ContextLL timout");
        }
        this.timeout = timeout;
    }

    public String getPathToCertificate() {
        return pathToCertificate;
    }

    public void setPathToCertificate(String pathToCertificate) {
        if (pathToCertificate == null) {
            throw new IllegalArgumentException("ContextLL pathToCertificate");
        }
        
        this.pathToCertificate = pathToCertificate;
    }

    
}
