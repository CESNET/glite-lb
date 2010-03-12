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
import java.net.Socket;

public class ContextDirect extends Context
{
	String	server;
	int	port;
	LBCredentials	cred;
	ILProto	il = null;
	int	timeout = 20000;

	public ContextDirect()
	{
	}

	public ContextDirect(String server,int port)
	{
		if (server == null) {
			throw new IllegalArgumentException("server is null");
		}
		if (port < 1 || port > 65535) {
			throw new IllegalArgumentException("port is not valid range");
		}
		this.server = server;
		this.port = port;
	}

	public void setCredentials(LBCredentials cred)
	{
		this.cred = cred;
		il = null;
	}

	@Override
	public void log(Event event) throws LBException {
		if (il == null) {
			SSL ssl = new SSL();
			ssl.setCredentials(cred);
			Socket sock = ssl.connect(server,port+1,timeout);
			setUser(ssl.myDN());
			try { il = new ILProto(sock); }
			catch (Throwable e) { throw new LBException(e); }
		}

		String msg = super.createMessage(event);
		int	maj;

		try {
			il.sendMessage(msg);
			maj = il.receiveReply();
		}
		catch (Throwable e) { throw new LBException(e); }

		if (maj > 0) {
			int	min = il.errMin();
			String	err = il.errMsg();

			throw new LBException("IL proto: " + maj + " " + min + " " + err);
		}
	}
}
