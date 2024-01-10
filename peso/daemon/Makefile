CC = gcc
CFLAGS = -O -Wall -pedantic
LIBPVCAM = -lpvcam -ldl -lpthread
LIBFITSIO = -lcfitsio -lm

all: daemon_sauron

daemon_sauron: daemon_sauron.c daemon_sauron.h socket.o
	$(CC) $(CFLAGS) $(LIBPVCAM) $(LIBFITSIO) -o ./daemon_sauron daemon_sauron.c socket.o

#daemon_sauron: daemon_sauron.c daemon_sauron.h ccd_sauron.o socket.o
#	$(CC) $(CFLAGS) $(LIBPVCAM) $(LIBFITSIO) -o ./daemon_sauron daemon_sauron.c ccd_sauron.o socket.o

ccd_sauron.o: ccd_sauron.c ccd_sauron.h
	$(CC) $(CFLAGS) -I../roper-scientific/k26 -c ccd_sauron.c

socket.o: socket.c socket.h
	$(CC) $(CFLAGS) -c socket.c

thread.o: thread.c
	$(CC) $(CFLAGS) -c thread.c

th_connect.o: th_connect.c th_connect.h
	$(CC) $(CFLAGS) -c th_connect.c
