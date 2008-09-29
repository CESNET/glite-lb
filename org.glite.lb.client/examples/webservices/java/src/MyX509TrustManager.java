package org.glite.wsdl.services.lb.example;

import org.apache.log4j.Logger;

import javax.net.ssl.X509TrustManager;
import java.security.InvalidAlgorithmParameterException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.*;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

/**
 * Simple trust manager validating server certificates against supplied trusted CAs.
 *
 * @author Martin Kuba makub@ics.muni.cz
 * @version $Id$
 */
public class MyX509TrustManager implements X509TrustManager {
    static Logger log = Logger.getLogger(MyX509TrustManager.class);
    private final X509Certificate[] certificateAuthorities;
    private final Set trustAnchors;

    /**
     * Creates an instance with supplied trusted root CAs.
     * @param certificateAuthorities
     */
    public MyX509TrustManager(X509Certificate[] certificateAuthorities) {
        int i;

        this.certificateAuthorities = certificateAuthorities;
        this.trustAnchors = new HashSet();
        for (i = 0; i < certificateAuthorities.length; i++) {
            this.trustAnchors.add(new TrustAnchor(certificateAuthorities[i], null));
        }
    }

    //not used on a client
    public X509Certificate[] getAcceptedIssuers() {
        log.debug("getAcceptedIssuers()");
        return this.certificateAuthorities;
    }

    //not used on a client
    public void checkClientTrusted(X509Certificate[] certs, String authType) {
        log.debug("checkClientTrusted()");
    }

    /**
     * Validates certificate chain sent by a server against trusted CAs.
     * @param certs
     * @param authType
     * @throws CertificateException
     */
    public void checkServerTrusted(X509Certificate[] certs, String authType) throws CertificateException {
        if (log.isDebugEnabled()) {
            log.debug("checkServerTrusted(certs: "+ certs.length + ", authType=" + authType+")");
            for (int i = 0; i < certs.length; i++) {
                log.debug("cert[" + i + "]=" + certs[i].getSubjectX500Principal().toString());
            }
        }
        //validate server certificate
        try {
            PKIXParameters pkixParameters = new PKIXParameters(this.trustAnchors);
            pkixParameters.setRevocationEnabled(false);
            CertificateFactory certFact = CertificateFactory.getInstance("X.509");
            CertPath path = certFact.generateCertPath(Arrays.asList(certs));
            CertPathValidator certPathValidator = CertPathValidator.getInstance("PKIX");
            certPathValidator.validate(path, pkixParameters);
        } catch (NoSuchAlgorithmException e) {
            log.error(e.getMessage(), e);
        } catch (InvalidAlgorithmParameterException e) {
            log.error(e.getMessage(), e);
        } catch (CertPathValidatorException e) {
            CertificateException ce;
            log.error(e.getMessage(), e);
            ce = new CertificateException(e.getMessage());
            ce.setStackTrace(e.getStackTrace());
            throw ce;
        }
        log.debug("server is trusted");
    }
}
