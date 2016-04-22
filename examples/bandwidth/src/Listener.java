import java.io.IOException;
import java.net.InetAddress;
import java.net.DatagramSocket;
import java.net.DatagramPacket;
import java.net.SocketException;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

public class Listener {
	DatagramSocket socket;
	InetAddress addr;
	int port;
	Lock lock;
	
	public int size;
	public int sent;
	public int received;

	Thread threadReceive;
	
	public Listener(InetAddress addr, int port, int size) throws SocketException {
		socket = new DatagramSocket(port);
		System.out.println("Listener[" + socket.getLocalPort() + "] started");
		this.addr = addr;
		this.port = port;
		this.size = size;
		lock = new ReentrantLock();
	}
	
	public void start() {
		threadReceive = new Thread(new Runnable() {
			@Override
			public void run() {
				int myReceived = 0;
				
				byte[] buf = new byte[size - 14 /* ETHER_LEN */ - 20 /* IP_LEN */ - 8 /* UDP_LEN */];
				
				DatagramPacket packet = new DatagramPacket(buf, buf.length);
				while(threadReceive == Thread.currentThread()) {
					try {
						socket.receive(packet);
					} catch(IOException e) {
						break;
					}
					
					myReceived++;
					if(lock.tryLock()) {
						received += myReceived;
						lock.unlock();
						
						myReceived = 0;
					}
				}
			}
		});
		threadReceive.start();
	}
	
	public void stop() {
		threadReceive= null;
	}
	
	public int[] get() {
		lock.lock();
		
		int s = sent;
		sent -= s;
		
		int r = received;
		received -= r;
		
		lock.unlock();
		
		return new int[] { s, r };
	}
	
	public static void main(String[] args) throws Throwable {
		if(args.length != 4) {
			System.out.println("Usage: java Listener [# of threads] [packet size] [address] [port]");
			return;
		}
		
		int size = Integer.parseInt(args[1]);
		InetAddress addr = InetAddress.getByName(args[2]);
		int port = Integer.parseInt(args[3]);
		
		Listener[] clients = new Listener[Integer.parseInt(args[0])];
		for(int i = 0; i < clients.length; i++)
			clients[i] = new Listener(addr, port, size);
		
		long time = System.currentTimeMillis() + 1000;
		
		for(int i = 0; i < clients.length; i++)
			clients[i].start();
		
		while(true) {
			Thread.sleep(time - System.currentTimeMillis());
			time += 1000;
			
			int sent = 0;
			int received = 0;
			
			for(int i = 0; i < clients.length; i++) {
				int[] s = clients[i].get();
				
				sent += s[0];
				received += s[1];
			}
			
			System.out.printf("%,d\t%,d\t%,d\t%,d\n", sent, received, (sent * size * 8), (received * size * 8));
		}
	}
}
