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

import javax.net.ssl.*;
import java.net.SocketException;
import java.io.*;
import java.net.InetSocketAddress;
import java.net.Socket;

public class SSL {



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

    private static String slashDN(String dn) {
	    String f[] = dn.split(",");
	    int	i;
	    String out = new String();

	    /* XXX: proxy */
	    for (i=f.length-1; i>=0 && f[i].indexOf("=proxy") == -1; i--) 
		    out += "/" + f[i];

	    return out;
    }

    public String myDN()
    {
	    java.security.cert.Certificate[] cert = sess.getLocalCertificates();
	    java.security.cert.X509Certificate xcert =
		    (java.security.cert.X509Certificate) cert[0];
	
	    return slashDN(xcert.getSubjectX500Principal().getName());
    }


}
