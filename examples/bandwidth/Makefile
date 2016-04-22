all: bin/Client.class bin/Server.class bin/Listener.class

bin/Client.class: src/Client.java
	javac -d bin src/Client.java

bin/Server.class: src/Server.java
	javac -d bin src/Server.java

bin/Listener.class: src/Listener.java
	javac -d bin src/Listener.java

client:
	java -cp bin Client 2 64 localhost 2000

server:
	java -cp bin Server 2000
