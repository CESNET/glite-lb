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

import org.glite.lb.SSL;
import org.glite.lb.LBCredentials;
import java.io.*;
import java.net.Socket;

public class SSLServer {
	public static void main(String[] args) {

		SSL ssl = new SSL();

		try {
			ssl.setCredentials(new LBCredentials(args[0],null));
			Socket sock = ssl.accept(Integer.parseInt(args[1]),100000);
			System.out.println("accept ok");
			InputStream in = sock.getInputStream();
			PrintStream out = new PrintStream(sock.getOutputStream(),false);

			while (true) {
				byte[] buf = new byte[1000];
				int	len = in.read(buf);
				System.out.write(buf,0,len);
				out.print("buzz off");
			}
		}
		catch (Exception e) {
			e.printStackTrace();
		}
		
	}
}
