import org.glite.lb.SSL;
import java.io.*;
import java.net.Socket;

public class SSLServer {
	public static void main(String[] args) {

		SSL ssl = new SSL();

		try {
			ssl.setProxy(args[0]);
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
