package org.glite.lb;

import javax.net.ssl.*;
import java.net.SocketException;
import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.security.*;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.security.cert.X509Certificate;
import java.util.Enumeration;
import org.glite.security.trustmanager.CRLFileTrustManager;
import org.glite.security.trustmanager.UpdatingKeyManager;

public class SSL {

    static final String proxyProp = "X509UserProxy";


    SSLContext sctx;
    SSLSocket client;
    SSLServerSocket server;
    SSLSession sess;
    LBCredentials creds;

    void init_ctx() throws LBException {
	    if (sctx == null) {
	    	if (creds == null) throw new NullPointerException("credentials must be specfied");

            	sctx = creds.getSSLContext();
	    }
    }

    public void setCredentials(LBCredentials c) {
	    creds = c;
    }

    public Socket connect(String host,int port,int timeout) throws LBException {
	    init_ctx();
	    
	    try {
		    client = (SSLSocket) sctx.getSocketFactory().createSocket();
	
	            client.setEnabledProtocols(new String[]{"SSLv3"});
	            client.setUseClientMode(true);
	            client.setSoTimeout(timeout); //read timeout
	
	            client.connect(new InetSocketAddress(host, port), timeout); //connect timeout
	            client.startHandshake();
	
	            sess = client.getSession();
	            if (sess == null) {
	                throw new NullPointerException("null session");
	            }
	    }
	    catch (IOException e) { throw new LBException(e); }

	    return client;
	    //return new PrintStream(client.getOutputStream(),false);
    }

    public Socket accept(int port,int timeout) throws LBException
    {
	SSLSocket conn;
	init_ctx();

	try {
		server = (SSLServerSocket) sctx.getServerSocketFactory().createServerSocket();
	
		server.setEnabledProtocols(new String[]{"SSLv3"});
		server.setSoTimeout(timeout); 
	
		server.bind(new InetSocketAddress(port));
	
		conn = (SSLSocket) server.accept();
	}
	catch (IOException e) { throw new LBException(e); }

	return conn;
    }

    public void close() throws LBException 
    {
	try {
	    client.close();
	}
	catch (IOException e) { throw new LBException(e); }
    }

}
