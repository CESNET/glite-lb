package org.glite.lb;

import java.io.IOException;
import java.security.*;

/**
 * This class opens secure connection using SSLv3 and then sends message to set
 * address.
 * 
 * @author Pavel Piskac
 */
public class SSLSend {

    private static final String EDG_WLL_LOG_SOCKET_HEADER = "DGLOG";

    /**
     * This method is used to send messages using a secure socket.
     * 
     * @param keyStoreSender path to user's certificate
     * @param host host name
     * @param port port number
     * @param timeout connection timeout
     * @param message message which will be send
     */
    public void send(String keyStoreSender, String host,
            int port, int timeout, String message) 
    throws KeyStoreException,IOException,NoSuchAlgorithmException,KeyManagementException
    {

	SSL lbsock = new SSL();

	lbsock.setProxy(keyStoreSender);
	lbsock.connect(host,port,timeout);

        lbsock.sendString(EDG_WLL_LOG_SOCKET_HEADER,timeout);

        message = message.replaceFirst("DG.LLLID=[0-9]* ", "");
        message = message.replaceFirst("DG.USER=\\x22[a-zA-Z ]*\\x22 ", "");

        int messageSize = message.length() + 2;
        byte revertedInt[] = new byte[4];
        revertedInt[0] = (byte) (messageSize % 256);
        messageSize >>= 8;
        revertedInt[1] = (byte) (messageSize % 256);
        messageSize >>= 8;
        revertedInt[2] = (byte) (messageSize % 256);
        messageSize >>= 8;
        revertedInt[3] = (byte) (messageSize);

        lbsock.sendBytes(revertedInt,4,timeout);
	lbsock.sendString(message + '\n' + '\0',timeout);
	lbsock.close();

    }

}
