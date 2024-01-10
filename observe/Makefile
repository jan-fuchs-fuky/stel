CC = gcc -Wall
#CC = gcc -Wall -DDBG
CFLAGS_LIBS = `xmlrpc-c-config abyss-server --cflags --libs` `log4c-config --libs` `pkg-config --cflags --libs glib-2.0`
CFLAGS = `pkg-config --cflags glib-2.0`
SVN_REV_SPE = -D'SVN_REV="$(shell svnversion -n ./src/spectrographd)"'
SVN_REV_TLE = -D'SVN_REV="$(shell svnversion -n ./src/telescoped)"'

all: telescoped spectrographd fits_verify_chksum

telescoped: ./src/telescoped/telescoped.c ./src/telescoped/telescoped.h thread.o socket.o str.o
	$(CC) $(CFLAGS_LIBS) $(SVN_REV_TLE) -o ./bin/telescoped \
        ./src/telescoped/telescoped.c thread.o socket.o str.o

spectrographd: ./src/spectrographd/spectrographd.c ./src/spectrographd/spectrographd.h thread.o socket.o str.o
	$(CC) $(CFLAGS_LIBS) $(SVN_REV_SPE) -o ./bin/spectrographd \
        ./src/spectrographd/spectrographd.c thread.o socket.o str.o

thread.o: ./src/telescoped/thread.c ./src/telescoped/thread.h
	$(CC) $(CFLAGS) -c ./src/telescoped/thread.c

socket.o: ./src/telescoped/socket.c ./src/telescoped/socket.h
	$(CC) $(CFLAGS) -c ./src/telescoped/socket.c

str.o: ./src/telescoped/str.c ./src/telescoped/str.h
	$(CC) -c ./src/telescoped/str.c

fits_verify_chksum: ./src/utils/fits_verify_chksum.c
	$(CC) -o ./bin/fits_verify_chksum ./src/utils/fits_verify_chksum.c -lcfitsio -lm

clean:
	-rm *.o
