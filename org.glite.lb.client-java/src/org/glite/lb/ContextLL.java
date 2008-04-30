package org.glite.lb;

import org.glite.jobid.Jobid;
import org.glite.jobid.CheckedString;

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
    private String password;
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
            throw new IllegalArgumentException("ContextProxy prefix");
        }
        if (address == null) {
            throw new IllegalArgumentException("ContextProxy socket");
        }
        if (port < 0) {
            throw new IllegalArgumentException("ContextProxy port");
        }

        this.prefix = new CheckedString(prefix).toString();
        this.address = new CheckedString(address).toString();
    }

    @Override
    public void log(Event event) {
        if (event == null) {
            throw new IllegalArgumentException("ContextProxy event");
        }

        if (prefix == null) {
            throw new IllegalArgumentException("ContextProxy prefix");
        }

        if (address == null) {
            throw new IllegalArgumentException("ContextProxy socket");
        }

        if (port < 0) {
            throw new IllegalArgumentException("ContextProxy port");
        }

        if (sslSend == null) {
            sslSend = new SSLSend();
        }
        
        String message = super.createMessage(event);

        ILFileWriter.write(prefix, message, repeatWriteToFile);
        
        
        sslSend.send(pathToCertificate, password, address, port, timeout, message);
    }

    public String getAddress() {
        return address;
    }

    public void setAddress(String address) {
        if (address == null) {
            throw new IllegalArgumentException("ContextProxy address");
        }
        
        this.address = address;
    }

    public int getPort() {
        return port;
    }

    public void setPort(int port) {
        if (port < 0) {
            throw new IllegalArgumentException("ContextProxy port");
        }
        this.port = port;
    }
    
    
    public String getPrefix() {
        return prefix;
    }

    public void setPrefix(String prefix) {
        if (prefix == null) {
            throw new IllegalArgumentException("ContextProxy prefix");
        }
        
        this.prefix = prefix;
    }

    public int getRepeatWriteToFile() {
        return repeatWriteToFile;
    }

    public void setRepeatWriteToFile(int repeatWriteToFile) {
        if (repeatWriteToFile < 1) {
            throw new IllegalArgumentException("ContextProxy repeatWriteToFile");
        }
        
        this.repeatWriteToFile = repeatWriteToFile;
    }

    public int getTimeout() {
        return timeout;
    }

    public void setTimeout(int timeout) {
        if (timeout < 0) {
            throw new IllegalArgumentException("ContextProxy timout");
        }
        this.timeout = timeout;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        if (password == null) {
            throw new IllegalArgumentException("ContextProxy password");
        }
        
        this.password = password;
    }

    public String getPathToCertificate() {
        return pathToCertificate;
    }

    public void setPathToCertificate(String pathToCertificate) {
        if (pathToCertificate == null) {
            throw new IllegalArgumentException("ContextProxy pathToCertificate");
        }
        
        this.pathToCertificate = pathToCertificate;
    }

    
}
