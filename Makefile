CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2

all: ep3

ep3: ep3.c ep3.h
	$(CC) $(CFLAGS) ep3.c -o ep3

clean:
	rm -f ep3 *.o
