
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
