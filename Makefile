CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -g
LDFLAGS=-lssl -lcrypto

OBJS=object.o tree.o index.o commit.o

.PHONY: all clean test-integration

all: pes test_objects test_tree

pes: pes.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_objects: test_objects.o object.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test_tree: test_tree.o tree.o index.o object.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $<

test-integration: all
	bash test_sequence.sh

clean:
	rm -f pes test_objects test_tree *.o
