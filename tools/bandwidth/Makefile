all: bin/Client.class bin/Server.class bin/Listener.class

bin/Client.class: src/Client.java
	mkdir -p bin
	javac -d bin src/Client.java

bin/Server.class: src/Server.java
	mkdir -p bin
	javac -d bin src/Server.java

bin/Listener.class: src/Listener.java
	mkdir -p bin
	javac -d bin src/Listener.java

client:
	java -cp bin Client 1 1500 192.168.10.200 7 100

server:
	java -cp bin Server 2000
