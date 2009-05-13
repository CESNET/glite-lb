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
import org.globus.cog.security.cert.request.BouncyCastleOpenSSLKey;
import org.globus.gsi.GlobusCredential;
import org.globus.gsi.GlobusCredentialException;
import org.gridforum.jgss.ExtendedGSSCredential;
import org.gridforum.jgss.ExtendedGSSManager;
import org.ietf.jgss.GSSCredential;
import org.ietf.jgss.GSSException;

public class SSL {

    static final String proxyProp = "X509UserProxy";

    /**
     * Implementation of abstract class X509KeyManager. 
     * It is used to manage X509 certificates which are used to authenticate
     * the local side of a secure socket.
     */
    static class MyX509KeyManager implements X509KeyManager {

        private X509Certificate[] certchain;
        private PrivateKey key;

        public MyX509KeyManager(Certificate[] cchain, PrivateKey key) {
            this.certchain = new X509Certificate[cchain.length];
	    System.arraycopy(cchain, 0, this.certchain, 0, cchain.length); 
            this.key = key;
        }

        public String chooseClientAlias(String[] keyType, Principal[] issuers, Socket
socket) {
            //System.out.println("MyX509KeyManager.chooseClientAlias()");
            //for (int i = 0; i < keyType.length; i++) {
                //System.out.println("MyX509KeyManager.chooseClientAlias() keyType[" + i +
//"]=" + keyType[i]);
            //}
            //for (int i = 0; i < issuers.length; i++) {
                //System.out.println("MyX509KeyManager.chooseClientAlias() issuers[" + i +
//"]=" + issuers[i]);
            //}
            return "";
        }

        public String chooseServerAlias(String keyType, Principal[] issuers, Socket
socket) {
            //System.out.println("MyX509KeyManager.chooseServerAlias(" + keyType + ")");
            return null;
        }

        public X509Certificate[] getCertificateChain(String alias) {
            //System.out.println("MyX509KeyManager.getCertificateChain(" + alias + ")");
            return certchain;
        }

        public String[] getClientAliases(String keyType, Principal[] issuers) {
            //System.out.println("MyX509KeyManager.getClientAliases(" + keyType + ")");
            return null;
        }

        public PrivateKey getPrivateKey(String alias) {
            //System.out.println("MyX509KeyManager.getPrivateKey(" + alias + ")");
            return key;
        }

        public String[] getServerAliases(String keyType, Principal[] issuers) {
            //System.out.println("MyX509KeyManager.getServerAliases(" + keyType + ")");
            return null;
        }
    }

    /**
     * Implementation of abstract class X509TrustManager.
     * It is used to authenticate the remote side of a secure socket.
     */
    static class MyX509TrustManager implements X509TrustManager {

        public X509Certificate[] getAcceptedIssuers() {
            return null;
        }

        public void checkClientTrusted(X509Certificate[] certs, String authType) {
            //System.out.println("X509TrustManager.checkClientTrusted(certs["+certs.length+"],"+authType+")");
        }

        public void checkServerTrusted(X509Certificate[] certs, String authType) throws
                CertificateException {
            //System.out.println("----X509TrustManager.checkServerTrusted-----");
            //System.out.println("number of certs: "+certs.length+", authType="+authType);
            //for(int i=0;i<certs.length;i++) {
            //    System.out.println("cert["+i+"]="+certs[i].getSubjectDN());
            //}
            //System.out.println("--------------------------------------------");
        }
    }

    SSLContext sctx;
    SSLSocket socket;
    SSLSession sess;
    String proxy;
    PrintStream osw = null;

    void init_ctx() throws KeyStoreException,NoSuchAlgorithmException,KeyManagementException {
	    if (sctx == null) {

		if (proxy == null) proxy = System.getProperty(proxyProp);

	        TrustManager[] trustAllCerts = new TrustManager[]{new MyX509TrustManager()};
       	     	X509KeyManager[] myKeyManager = createX509KeyManager(proxy);
       	     
       	     	if (myKeyManager == null) {
                	throw new NullPointerException("myKeyManager is null");
            	}

            	sctx = SSLContext.getInstance("SSLv3");
            	sctx.init(myKeyManager, trustAllCerts, null);
	    }
    }

    public void setProxy(String p) {
	    proxy = p;
    }

    public void connect(String host,int port,int timeout) throws KeyStoreException,NoSuchAlgorithmException,KeyManagementException,SocketException,IOException {

	    init_ctx();
	    
	    osw = null;
	    socket = (SSLSocket) sctx.getSocketFactory().createSocket();

            socket.setEnabledProtocols(new String[]{"SSLv3"});
            socket.setUseClientMode(true);
            socket.setSoTimeout(timeout * 10); //read timeout

            socket.connect(new InetSocketAddress(host, port), timeout); //connect timeout
            socket.startHandshake();

            sess = socket.getSession();
            if (sess == null) {
                throw new NullPointerException("null session");
            }

	    osw = new PrintStream(socket.getOutputStream(), false);
    }

    public void close() throws IOException,SocketException {
	    osw.close();
	    socket.close();
    }

    public void sendString(String message,int timeout) throws IOException,SocketException {
	    socket.setSoTimeout(timeout * 10);
	    osw.print(message);
	    osw.flush();
    }

    public void sendBytes(byte[] message,int len,int timeout) throws IOException,SocketException {
	    socket.setSoTimeout(timeout * 10);
	    osw.write(message,0,len);
	    osw.flush();
    }

    /**
     * This methods reads user's certificate
     * 
     * @param ksfile path to certificate
     * @return instance of KeyStore with certificate
     * @throws java.security.KeyStoreException
     * @throws java.security.cert.CertificateException
     * @throws java.security.NoSuchAlgorithmException
     * @throws java.io.IOException
     */
    X509KeyManager[] createX509KeyManager(String ksfile) throws KeyStoreException {

        if (ksfile.endsWith(".pem") || !ksfile.contains(".")) {
            return readPEM(ksfile);
        }

        throw new KeyStoreException("Unknown key store");
    }

    X509KeyManager[] readPEM(String ksfile) {
        BufferedReader br = null;
        BufferedInputStream pemFile = null;
        ByteArrayInputStream bais = null;

        X509KeyManager[] myX509KeyManager = null;
        
	try {
            // read in the credential data
            File f = new File(ksfile);
            pemFile = new BufferedInputStream(new FileInputStream(f));
            byte [] data = new byte[(int)f.length()];
            pemFile.read(data);
            
            GlobusCredential gc = new GlobusCredential(ksfile);
            Certificate[] cert = gc.getCertificateChain();

            PrivateKey privateKey = gc.getPrivateKey();
            myX509KeyManager = new X509KeyManager[]{new MyX509KeyManager(cert, privateKey)};
        } catch (IOException ex) {
            System.err.println(ex);
        } catch (GlobusCredentialException ex) {
            System.err.println(ex);
        } finally {
            try {
                pemFile.close();
            } catch (IOException ex) {
                System.err.println(ex);
            }
        }

        return myX509KeyManager;
    }
}
