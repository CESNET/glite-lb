import org.glite.lb.SSLSend;

public class SSLClient {
	public static void main(String[] args) {

		SSLSend	ssl = new SSLSend();

		try {
			ssl.send(args[0],"localhost",9002,100,"baff");
		}
		catch (Exception e) {
			System.err.println(e);
		}
		
	}
}
