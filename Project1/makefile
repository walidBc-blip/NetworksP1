CC = gcc
CFLAGS = -Wall -g

# Define source directories
CLIENT_DIR = client
SERVER_DIR = server
CODE_DIR = code
HEADER_DIR = code

# Define output directories
CLIENT_OUT_DIR = client
SERVER_OUT_DIR = server

# Define targets
TARGETS = $(SERVER_OUT_DIR)/ftp_server $(CLIENT_OUT_DIR)/ftp_client

all: $(TARGETS)

$(SERVER_OUT_DIR)/ftp_server: $(CODE_DIR)/ftp_server.o $(CODE_DIR)/ftp_server.h
	$(CC) $(CFLAGS) -o $@ $(CODE_DIR)/ftp_server.o

$(CLIENT_OUT_DIR)/ftp_client: $(CODE_DIR)/ftp_client.o $(CODE_DIR)/ftp_client.h
	$(CC) $(CFLAGS) -o $@ $(CODE_DIR)/ftp_client.o

$(CODE_DIR)/ftp_server.o: $(CODE_DIR)/ftp_server.c $(HEADER_DIR)/ftp_server.h
	$(CC) $(CFLAGS) -I$(HEADER_DIR) -c $< -o $@

$(CODE_DIR)/ftp_client.o: $(CODE_DIR)/ftp_client.c $(HEADER_DIR)/ftp_client.h
	$(CC) $(CFLAGS) -I$(HEADER_DIR) -c $< -o $@

clean:
	rm -f $(CODE_DIR)/*.o $(TARGETS)
