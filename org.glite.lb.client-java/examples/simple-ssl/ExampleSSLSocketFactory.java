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

package org.glite.lb.examples.ssl;

import org.apache.axis.components.net.BooleanHolder;
import org.apache.axis.components.net.DefaultSocketFactory;
import org.apache.axis.components.net.SecureSocketFactory;
import org.apache.log4j.Logger;

import javax.net.ssl.*;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketException;
import java.security.PrivateKey;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Date;
import java.util.Hashtable;

/**
 * Factory for SSLSockets for Axis, which creates sockets for SSLv3 connection.
 * Globus SSL libraries don't support SSLv2 initial packet, so SSLv2Hello protocol
 * on SSLSocket must be disabled.
 *
 * @author Martin Kuba makub@ics.muni.cz
 */
public class ExampleSSLSocketFactory extends DefaultSocketFactory implements SecureSocketFactory {

    static Logger log = Logger.getLogger(ExampleSSLSocketFactory.class);

    static Certificate[] cChain = null;
    static PrivateKey pKey = null;
    static X509Certificate[] certAuths = null;

    static void registerForAxis(Certificate[] certs, PrivateKey privateKey, X509Certificate[] certificateAuthorities) {
        log.debug("registering as Axis SecureSocketFactory");
        cChain = certs;
        pKey = privateKey;
        certAuths = certificateAuthorities;
        System.setProperty("org.apache.axis.components.net.SecureSocketFactory", ExampleSSLSocketFactory.class.getName());
    }

    protected SSLSocketFactory sslFactory = null;

    public ExampleSSLSocketFactory(Hashtable attributes) throws Exception {
        super(attributes);
        SSLContext sctx = SSLContext.getInstance("SSL");
        KeyManager[] myKeys = new KeyManager[]{new MyX509KeyManager(cChain, pKey)};
        TrustManager[] myTrust = new TrustManager[]{new MyX509TrustManager(certAuths)};
        //init SSLContext with our keymanager and trustmanager, and default random device
        sctx.init(myKeys, myTrust, null);
        if (log.isDebugEnabled()) {
            log.debug("attributes: " + attributes);
            log.debug("SSLContext.provider: " + sctx.getProvider().getInfo());
        }
        sslFactory = sctx.getSocketFactory();
    }


    public Socket create(String host, int port, StringBuffer otherHeaders, BooleanHolder useFullURL) throws IOException, SocketException {
	int i;

        log.debug("create(" + host + ":" + port + ")");
        //create SSL socket
        SSLSocket socket = (SSLSocket) sslFactory.createSocket();
        //enable only SSLv3
        socket.setEnabledProtocols(new String[]{"SSLv3"});   //  SSLv2Hello, SSLv3,TLSv1
        //enable only ciphers without RC4 (some bug, probably in older globus)
        String[] ciphers = socket.getEnabledCipherSuites();
        ArrayList al = new ArrayList(ciphers.length);
        for (i = 0; i < ciphers.length; i++) {
            if (ciphers[i].indexOf("RC4") == -1) al.add(ciphers[i]);
        }
        socket.setEnabledCipherSuites((String [])al.toArray(new String[al.size()]));
        //connect as client
        socket.setUseClientMode(true);
        socket.setSoTimeout(30000); //read timeout
        socket.connect(new InetSocketAddress(host, port), 3000); //connect timeout
        //create or join a SSL session
        SSLSession sess = socket.getSession();
        if (sess == null) {
            log.debug("sess is null");
            return socket;
        }

        if (log.isDebugEnabled()) {
            //print all we know
            byte[] id = sess.getId();
            StringBuffer sb = new StringBuffer(id.length * 2);
            for (i = 0; i < id.length; i++) {
                sb.append(Integer.toHexString(id[i] < 0 ? 256 - id[i] : id[i]));
            }
            log.debug("SSLSession.id = " + sb.toString());
//            log.debug("peerHost:Port = " + sess.getPeerHost() + ":" + sess.getPeerPort());
            log.debug("cipherSuite   = " + sess.getCipherSuite());
            log.debug("protocol      = " + sess.getProtocol());
//            log.debug("isValid       = " + sess.isValid());
            log.debug("creationTime    = " + (new Date(sess.getCreationTime())));
            log.debug("lastAccessedTime= " + (new Date(sess.getLastAccessedTime())));
//            log.debug("applicationBufferSize= " + sess.getApplicationBufferSize());
//            log.debug("packetBufferSize= " + sess.getPacketBufferSize());
//            log.debug("localPrincipal= " + sess.getLocalPrincipal());
//            log.debug("peerPrincipal= " + sess.getPeerPrincipal());
        }
        return socket;
    }

}
