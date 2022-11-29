SERVER = server
CLIENT = client
FILEFUNCS = filefuncs

all: $(SERVER).c $(CLIENT).c $(FILEFUNCS).c $(FILEFUNCS).h
	gcc $(SERVER).c $(FILEFUNCS).c -o $(SERVER)
	gcc $(CLIENT).c $(FILEFUNCS).c -o $(CLIENT)

ser: $(SEVER)
	./$(SERVER)

cli: $(CLIENT)
	./$(CLIENT)

clean:
	rm $(SERVER) $(CLIENT)
	rm clientRoot/ -rf