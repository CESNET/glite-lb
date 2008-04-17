package org.glite.lb.client_java;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.RandomAccessFile;
import java.net.InetSocketAddress;
import java.net.UnknownHostException;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;
import java.util.ArrayList;
import javax.net.ssl.SSLSocket;
import javax.net.ssl.SSLSocketFactory;
import org.glite.jobid.api_java.Jobid;
import org.glite.jobid.api_java.CheckedString;

/** NOT IMPLEMENTED YET! THIS IS ONLY SKELETON
 *
 * @author xpiskac
 */
public class ContextProxy extends Context {

    private String address;
    private int port;
    private String prefix;
    private int repeatWriteToFile = 5;
    private int timeout = 30000;
    private String pathToCertificate;
    private String password;

    public ContextProxy() {
    }

    public ContextProxy(String address, int port, String prefix) {
        this.prefix = new CheckedString(prefix).toString();
        this.address = new CheckedString(address).toString();
        this.port = port;
    }

    public ContextProxy(int id,
            Sources source,
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

    /**
     * Writes message to a file and returns size of this file before writing the
     * data
     * @param prefix file path
     * @param message message which will be written
     * @return size of the file before writing the data
     */
    private Long writeToFile(String prefix, String message) {
        FileWriter fileWriter = null;
        Long fileLength = null;
        RandomAccessFile raf = null;
        FileLock fileLock = null;
        File file;
                
        for (int i = 0; i < repeatWriteToFile; i++) {
            try {
                file = new File(prefix);
                raf = new RandomAccessFile(file, "rw");
                FileChannel fileChannel = raf.getChannel();

                fileLock = fileChannel.tryLock();
                if (fileLock != null) {
                    if (!file.exists()) {
                        continue;
                    }
                    fileLength = new Long(raf.length());
                    fileWriter = new FileWriter(file, true);
                    //true means append data to the end of file
                    
                    BufferedWriter bufferedFileWriter = new BufferedWriter(fileWriter);

                    bufferedFileWriter.write(message + '\n');
                    bufferedFileWriter.flush();
                    
                    if (file.exists()) {
                        break;
                    }
                }
            } catch (FileNotFoundException ex) {
                System.err.println(ex);
            } catch (IOException ex) {
                System.err.println(ex);
            } catch (Exception ex) {
                System.err.println(ex);
            } finally {
                if (fileLock != null) {
                    try {
                        fileLock.release();
                    } catch (IOException ex) {
                        System.err.println(ex);
                    }
                }
                
                try {
                    raf.close();
                } catch (IOException ex) {
                    System.err.println(ex);
                }
            }
        }
        
        return fileLength;
    }
    
    private void writeToSocket(String address, int port, int timeout, long fileSize, String message) {
        // From http://sci.civ.zcu.cz/java/lbexample.zip ExampleSSLSocketFactory.java
        SSLSocketFactory sslFactory = null;
        try {
            SSLSocket socket = (SSLSocket) sslFactory.createSocket();
            //enable only SSLv3
            socket.setEnabledProtocols(new String[]{"SSLv3"});   //  SSLv2Hello, SSLv3,TLSv1
            //enable only ciphers without RC4 (some bug in JSSE?)
            String[] ciphers = socket.getEnabledCipherSuites();
//            ArrayList<String> al = new ArrayList<String>(ciphers.length);
            ArrayList al = new ArrayList(ciphers.length);
            for (int i = 0; i < ciphers.length; i++) {
                if (ciphers[i].indexOf("RC4") == -1) al.add(ciphers[i]);
            }
            socket.setEnabledCipherSuites((String [])al.toArray(new String[al.size()]));
            //connect as client
            socket.setUseClientMode(true);
            socket.setSoTimeout(timeout); //read timeout
            socket.connect(new InetSocketAddress(address, port), timeout); //connect timeout
            PrintWriter osw =
                    new PrintWriter(socket.getOutputStream(), true);
            osw.println(fileSize);
            osw.println(message);
            osw.close();
            socket.close();
        } catch (UnknownHostException ex) {
            System.err.println("unknown host " + ex);
        } catch (IOException ex) {
            System.err.println("io exception " + ex);
        }
    }

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

        super.log(event);
        String message = super.getMessage();

        Long fileSize = writeToFile(prefix, message);
        
        //writeToSocket(address, port, timeout, fileSize, message);
        
        //SSLSend sslSend = new SSLSend(keyStoreSender, password, address,
        //    port, timeout, fileSize, message);
    }

    public String getAddress() {
        return address;
    }

    public void setAddress(String address) {
        if (prefix == null) {
            throw new IllegalArgumentException("ContextProxy address");
        }
        
        this.address = new CheckedString(address).toString();
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
        
        this.prefix = new CheckedString(prefix).toString();
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
