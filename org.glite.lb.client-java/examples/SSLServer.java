import org.glite.lb.SSL;
import java.io.*;

public class SSLServer {
	public static void main(String[] args) {

		SSL ssl = new SSL();

		try {
			ssl.setProxy(args[0]);
			InputStream in = ssl.accept(Integer.parseInt(args[1]),100000);
			System.out.println("accept ok");

			while (true) {
				byte[] buf = new byte[1000];
				int	len = in.read(buf);
				System.out.write(buf,0,len);
			}
		}
		catch (Exception e) {
			e.printStackTrace();
		}
		
	}
}
