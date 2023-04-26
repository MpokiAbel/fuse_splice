CC=gcc
CFLAGS=-Wall $(shell pkg-config fuse3 --cflags)
LIBS=$(shell pkg-config fuse3 --libs)

PROGRAMS = stackfs_client stackfs_server 
OUT = stackfs_client.out stackfs_server.out
.PHONY: all $(PROGRAMS)

all: $(PROGRAMS)


$(PROGRAMS):
	$(CC) $(CFLAGS) $@.c socket.c $(LIBS) -o $@.out

clean:
	rm -f $(OUT)

