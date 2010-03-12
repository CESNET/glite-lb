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

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.glite.lb;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.Properties;
import javax.xml.rpc.ServiceException;
import javax.net.ssl.SSLContext;

import org.apache.axis.SimpleTargetedChain;
import org.apache.axis.Handler;
import org.apache.axis.transport.http.HTTPTransport;
import org.apache.axis.transport.http.SocketHolder;
import org.apache.axis.SimpleChain;
import org.apache.axis.configuration.SimpleProvider;

import org.glite.security.trustmanager.ContextWrapper;
import org.glite.security.trustmanager.axis.SSLConfigSender;

import org.glite.wsdl.services.lb.LoggingAndBookkeepingLocator;
import org.glite.wsdl.services.lb.LoggingAndBookkeepingPortType;


/**
 * 
 */
public class LBCredentials {

    private String proxy;
    private String caFiles;
    private String key;
    private String pass;
    private String cert;

    public LBCredentials(String proxy, String caFiles) {
        if (proxy == null) throw new IllegalArgumentException("Proxy cannot be null");

	this.proxy = new String(proxy);
	if (caFiles != null) {
		this.caFiles = new String(caFiles);
	}
    }

    public LBCredentials(String userCert, String userKey, String userPass, String caFiles) {
        if (userCert==null || userKey==null)
            throw new IllegalArgumentException("key and cert must not be null");

	key = new String(userKey);
	cert = new String(userCert);
	if (userPass != null) pass = new String(userPass);
        if (caFiles != null) this.caFiles = new String(caFiles);
    }

    protected LoggingAndBookkeepingPortType getStub(String server) throws LBException {
        if (server == null)
            throw new IllegalArgumentException("Server cannot be null");
        try {
            URL queryServerAddress = new URL(server);
            int port = queryServerAddress.getPort();
            if (port < 1 || port > 65535) {
                throw new IllegalArgumentException("port");
            }
            if (!queryServerAddress.getProtocol().equals("https")) {
                throw new IllegalArgumentException("wrong protocol");
            }

	    Handler transport = new SimpleTargetedChain(new SimpleChain(),
					new SSLConfigSender(makeConfig()),
					new SimpleChain());
	    SimpleProvider transportProvider = new SimpleProvider();
	    transportProvider.deployTransport(HTTPTransport.DEFAULT_TRANSPORT_NAME, transport);
            LoggingAndBookkeepingLocator loc = new LoggingAndBookkeepingLocator(transportProvider);
            return loc.getLoggingAndBookkeeping(queryServerAddress);
        } catch (ServiceException ex) {
            throw new LBException(ex);
        } catch (MalformedURLException ex) {
            throw new LBException(ex);
        } catch (org.apache.axis.AxisFault ex) {
            throw new LBException(ex);
	}
    }

    private Properties makeConfig() {
	Properties cf = new java.util.Properties();

	if (proxy != null) cf.put(ContextWrapper.CREDENTIALS_PROXY_FILE,proxy);
	else {
		cf.put(ContextWrapper.CREDENTIALS_CERT_FILE,cert);
		cf.put(ContextWrapper.CREDENTIALS_KEY_FILE,key);
		if (pass != null) cf.put(ContextWrapper.CREDENTIALS_KEY_PASSWD, pass);
	}

	if (caFiles != null) cf.put(ContextWrapper.CA_FILES,caFiles);
	cf.put(ContextWrapper.SSL_PROTOCOL, "SSLv3");

	return cf;
    }

    protected SSLContext getSSLContext() throws LBException {
	ContextWrapper cw;

	try {
		cw = new ContextWrapper(makeConfig());
	}
	catch (java.io.IOException e) {
		throw new LBException(e);
	}
	catch (java.security.GeneralSecurityException e) {
		throw new LBException(e);
	}
	return cw.getContext();

    }
}
