package org.glite.wsdl.services.lb.example;

import org.apache.log4j.Logger;

import javax.net.ssl.X509KeyManager;
import java.net.Socket;
import java.security.Principal;
import java.security.PrivateKey;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;

/**
 * Implementation of  X509KeyManager, which always returns one pair of a private key and certificate chain.
 */
public class MyX509KeyManager implements X509KeyManager {
    static Logger log = Logger.getLogger(MyX509KeyManager.class);
    private final X509Certificate[] certChain;
    private final PrivateKey key;

    public MyX509KeyManager(Certificate[] cchain, PrivateKey key) {
        this.certChain = new X509Certificate[cchain.length];
        System.arraycopy(cchain, 0, this.certChain, 0, cchain.length);
        this.key = key;
    }

    //not used
    public String[] getClientAliases(String string, Principal[] principals) {
        log.debug("getClientAliases()");
        return null;
    }


    // Intented to be implemented by GUI for user interaction, but we have only one key.
    public String chooseClientAlias(String[] keyType, Principal[] issuers, Socket socket) {
        if (log.isDebugEnabled()) {
            log.debug("chooseClientAlias()");
            for (int i = 0; i < keyType.length; i++) log.debug("keyType[" + i + "]=" + keyType[i]);
            for (int i = 0; i < issuers.length; i++) log.debug("issuers[" + i + "]=" + issuers[i]);
        }
        return "thealias";
    }

    //not used on a client
    public String[] getServerAliases(String string, Principal[] principals) {
        log.debug("getServerAliases()");
        return null;
    }

    //not used on a client
    public String chooseServerAlias(String string, Principal[] principals, Socket socket) {
        log.debug("chooseServerAlias()");
        return null;
    }

    public X509Certificate[] getCertificateChain(String alias) {
        log.debug("getCertificateChain()");
        return certChain;
    }

    public PrivateKey getPrivateKey(String alias) {
        log.debug("getPrivateKey()");
        return key;
    }
}
