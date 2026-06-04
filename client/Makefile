CC=gcc

CFLAGS=-Wall

LIBS=-lnewt -lssl -lcrypto

SRC=src/main.c \
    src/network/client_socket.c \
    src/network/ssl_wrapper.c \
    src/ui/login.c \
    src/ui/voting.c \
    src/ui/receipt.c

OUT=build/client

all:
	mkdir -p build
	$(CC) $(SRC) $(CFLAGS) $(LIBS) -o $(OUT)

run: all
	./$(OUT)

clean:
	rm -rf build
