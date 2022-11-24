SERVER = Server
CLIENT = Client

all: $(SERVER).c $(CLIENT).c
	gcc $(SERVER).c -o $(SERVER)
	gcc $(CLIENT).c -o $(CLIENT)

server: $(SERVER).c
	./$(SERVER)

client: $(CLIENT).c
	./$(CLIENT)

clean:
	rm $(SERVER) $(CLIENT)