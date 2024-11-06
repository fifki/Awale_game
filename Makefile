CC = gcc

CLIENT = client
SERVER = server

all: $(CLIENT) $(SERVER)

$(CLIENT): client.c
	$(CC) client.c -o $(CLIENT)


$(SERVER): server.c
	$(CC) server.c -o $(SERVER)

clean:
	rm -f $(CLIENT) $(SERVER)