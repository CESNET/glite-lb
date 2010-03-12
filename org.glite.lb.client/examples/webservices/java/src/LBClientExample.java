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

package org.glite.wsdl.services.lb.example;

import org.apache.log4j.Logger;
import org.glite.wsdl.services.lb.stubs.LoggingAndBookkeepingPortType;
import org.glite.wsdl.services.lb.stubs.LoggingAndBookkeeping_ServiceLocator;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.net.URL;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.PrivateKey;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.X509Certificate;
import java.util.Enumeration;

/**
 * Example client of LoggingAndBookkeeping web service. Please note that the client depends
 * only on Axis and Log4j libraries, it uses cryptography included in the JDK.
 *
 * @author Martin Kuba makub@ics.muni.cz
 * @version $Id$
 */
public class LBClientExample {
    static Logger log = Logger.getLogger(LBClientExample.class);

    public static void main(String[] args) throws Exception {
        if(args.length<2) {
            System.out.println("usage: java LBClientExample <keystore.p12> <password> [<url>]");
            System.exit(-1);
        }
        File keyfile = new File(args[0]);
        String password = args[1];
        URL url = new URL("https://localhost:9003/");
        if(args.length==3) {
            url = new URL(args[2]);
        }

        //read in a keystore file
        KeyStore ks = readKeyStoreFile(keyfile, password);
        //find key alias (name)
        String alias = null;
        for (Enumeration en = ks.aliases(); en.hasMoreElements();) {
            alias = (String)en.nextElement();
            if (ks.isKeyEntry(alias)) break;
            else alias = null;
        }
        if (alias == null) throw new RuntimeException("the keystore contains no keys");
        //get my private key and certificates
        Certificate[] certs = ks.getCertificateChain(alias);
        PrivateKey key = (PrivateKey) ks.getKey(alias, password.toCharArray());
        //use my CA as the only trusted certificate authority
        X509Certificate[] trustedCertAuths = new X509Certificate[]{(X509Certificate) certs[certs.length - 1]};

        //register our SSL handling
        ExampleSSLSocketFactory.registerForAxis(certs, key, trustedCertAuths);

        //get client stub
        LoggingAndBookkeepingPortType lb = new LoggingAndBookkeeping_ServiceLocator().getLoggingAndBookkeeping(url);

        //call the service
        String version = lb.getVersion(null);
        System.out.println("LB version: " + version);
    }

    public static KeyStore readKeyStoreFile(File ksfile, String password) throws KeyStoreException, CertificateException, NoSuchAlgorithmException, IOException {
        String kstype;
        if (ksfile.getName().endsWith("ks")) kstype = "JKS";
        else if (ksfile.getName().endsWith(".p12")) kstype = "PKCS12";
        else throw new IOException("Only JKS (*ks) and PKCS12 (*.p12) files are supported ");
        KeyStore store = KeyStore.getInstance(kstype);
        store.load(new FileInputStream(ksfile), password != null ? password.toCharArray() : null);
        return store;
    }

}
