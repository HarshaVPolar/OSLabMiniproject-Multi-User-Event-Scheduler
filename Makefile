# Compiler
CC = gcc

# Flags
CFLAGS = -Wall -pthread
LDFLAGS = -lssl -lcrypto

# Output binaries (IMPORTANT: avoid folder names)
SERVER_BIN = scheduler_server
CLIENT_BIN = client_app

# Directories
SERVER_DIR = server
CLIENT_DIR = client
COMMON_DIR = common

# Source files
SERVER_SRC = $(SERVER_DIR)/server.c \
             $(SERVER_DIR)/scheduler.c \
             $(SERVER_DIR)/worker.c \
             $(COMMON_DIR)/util.c

CLIENT_SRC = $(CLIENT_DIR)/client.c

# ---------------- BUILD ----------------
all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) $(SERVER_SRC) -I$(COMMON_DIR) -o $(SERVER_BIN) $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) $(CLIENT_SRC) -o $(CLIENT_BIN)

# ---------------- CLEAN ----------------
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN) *.o