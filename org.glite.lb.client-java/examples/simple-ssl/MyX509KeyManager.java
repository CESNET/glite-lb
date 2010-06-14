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
