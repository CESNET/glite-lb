/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

package org.glite.lb;

import java.net.MalformedURLException;
import java.net.URL;
import javax.xml.rpc.ServiceException;
import org.apache.axis.AxisProperties;
import org.glite.wsdl.services.lb.LoggingAndBookkeepingLocator;
import org.glite.wsdl.services.lb.LoggingAndBookkeepingPortType;

/**
 * 
 */
public class LBCredentials {

    public LBCredentials(String proxy, String caFiles) {
        if (proxy == null) throw new IllegalArgumentException("Proxy cannot be null");
        if (caFiles == null) throw new IllegalArgumentException("caFiles cannot be null");

        System.setProperty(org.glite.security.trustmanager.ContextWrapper.CREDENTIALS_PROXY_FILE, proxy);
        System.setProperty(org.glite.security.trustmanager.ContextWrapper.CA_FILES, caFiles);
        System.setProperty(org.glite.security.trustmanager.ContextWrapper.SSL_PROTOCOL, "SSLv3");
        AxisProperties.setProperty("axis.socketSecureFactory","org.glite.security.trustmanager.axis.AXISSocketFactory");
    }

    public LBCredentials(String userCert, String userKey, String userPass, String caFiles) {
        if (userCert==null || userKey==null || userPass==null || caFiles==null)
            throw new IllegalArgumentException("One of the parameters was null");

        System.setProperty(org.glite.security.trustmanager.ContextWrapper.CREDENTIALS_CERT_FILE,userCert);
		System.setProperty(org.glite.security.trustmanager.ContextWrapper.CREDENTIALS_KEY_FILE,userKey);
		System.setProperty(org.glite.security.trustmanager.ContextWrapper.CREDENTIALS_KEY_PASSWD,userPass);
        System.setProperty(org.glite.security.trustmanager.ContextWrapper.CA_FILES, caFiles);
        System.setProperty(org.glite.security.trustmanager.ContextWrapper.SSL_PROTOCOL, "SSLv3");
        AxisProperties.setProperty("axis.socketSecureFactory","org.glite.security.trustmanager.axis.AXISSocketFactory");
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
            LoggingAndBookkeepingLocator loc = new LoggingAndBookkeepingLocator();
            return loc.getLoggingAndBookkeeping(queryServerAddress);
        } catch (ServiceException ex) {
            throw new LBException(ex);
        } catch (MalformedURLException ex) {
            throw new LBException(ex);
        }
    }

}
