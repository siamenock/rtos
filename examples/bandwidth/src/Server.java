import java.net.DatagramSocket;
import java.net.DatagramPacket;

public class Server {
	public static void main(String[] args) throws Throwable {
		if(args.length != 1) {
			System.out.println("Usage: java Server [port]");
			return;
		}
		
		DatagramSocket socket = new DatagramSocket(Integer.parseInt(args[0]));
		int size = 1500 -  14 /* ETHER_LEN */ - 20 /* IP_LEN */ - 8 /* UDP_LEN */;
		DatagramPacket packet = new DatagramPacket(new byte[size], size);
		while(true) {
			socket.receive(packet);
			socket.send(packet);
		}
	}
}
