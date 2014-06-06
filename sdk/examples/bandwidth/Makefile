all: bin/Client.class bin/Server.class bin/Listener.class

bin/Client.class: src/Client.java
	javac src/Client.java -d bin

bin/Server.class: src/Server.java
	javac src/Server.java -d bin

bin/Listener.class: src/Listener.java
	javac src/Listener.java -d bin

client:
	java -cp bin Client 2 64 localhost 2000

server:
	java -cp bin Server 2000
