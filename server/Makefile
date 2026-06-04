CC=gcc

CFLAGS=-Wall

LIBS=-lnewt -lpthread -lsqlite3 -lssl -lcrypto

SRC=src/main.c \
    src/ui/dashboard.c \
    src/ui/results.c \
    src/models/server_state.c \
    src/network/server_socket.c \
    src/network/ssl_wrapper.c \
    src/security/auth.c \
    src/security/rate_limit.c \
    src/security/secure_log.c \
    src/security/validation.c \
    src/database/database.c \
    src/protocol/protocol.c

OUT=build/server

all:
	mkdir -p build
	$(CC) $(SRC) $(CFLAGS) $(LIBS) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -rf build
