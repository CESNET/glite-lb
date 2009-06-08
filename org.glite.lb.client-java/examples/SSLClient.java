import org.glite.lb.SSLSend;
import org.glite.lb.LBCredentials;

public class SSLClient {
	public static void main(String[] args) {

		SSLSend	ssl = new SSLSend();

		try {
			ssl.send(new LBCredentials(args[0],null),"localhost",9002,100,"baff");
		}
		catch (Exception e) {
			System.err.println(e);
		}
		
	}
}
