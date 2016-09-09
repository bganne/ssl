OPENSSL11?=0
OPENSSL11_DIR:=../../openssl/install

PORT?=55555
CIPHER?=AES256-GCM-SHA384
DURATION?=10

CFLAGS:=-Wall -Werror -g -pthread -std=gnu99
CFLAGS+= -O2
LDFLAGS:=-g -pthread
LDLIBS:=-lssl -lcrypto

ifneq ($(OPENSSL11), 0)
OPENSSL11_BINDIR:=$(OPENSSL11_DIR)/bin
OPENSSL11_LIBDIR:=$(OPENSSL11_DIR)/lib
CFLAGS+=-I ../../openssl/install/include -Wno-deprecated-declarations
LDLIBS+=-L$(OPENSSL11_LIBDIR)
export PATH:=$(OPENSSL11_BINDIR):$(PATH)
export LD_LIBRARY_PATH:=$(OPENSSL11_LIBDIR):$(LD_LIBRARY_PATH)
endif

all: server client

server: server.c sock_helper.o ssl_helper.o

client: client.c tun.o sock_helper.o ssl_helper.o

clean:
	$(RM) server client *.o

test: server client
	sudo ./run_test.sh $(PORT) $(CIPHER) $(DURATION)

.PHONY: all clean test
