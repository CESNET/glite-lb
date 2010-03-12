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

import java.io.IOException;
import java.io.PrintStream;
import java.security.*;
import java.net.Socket;

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
    public void send(LBCredentials cred, String host,
            int port, int timeout, String message) 
	throws LBException
    {

      try {
	SSL lbsock = new SSL();

	lbsock.setCredentials(cred);
	Socket sock = lbsock.connect(host,port,timeout);
	PrintStream s = new PrintStream(sock.getOutputStream(),false);

        s.print(EDG_WLL_LOG_SOCKET_HEADER);

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

        s.write(revertedInt,0,4);
	s.print(message + '\n' + '\0');
	s.flush();
	s.close();
      } 
      catch (IOException e) {
	throw new LBException(e);
      }
    }
}
