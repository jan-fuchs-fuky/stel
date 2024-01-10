#ARCH = 32
#ARCH = 64
ARCH = `getconf LONG_BIT`

CC = gcc -Wall -m$(ARCH) -no-pie
#CC = gcc -g -Wall -m$(ARCH)

# debug
#CC = gcc -Wall -g -m$(ARCH) -no-pie

SVN_REV = -D'SVN_REV="$(shell git show -s --pretty=format:%h)"'

#LIBXML2 = `pkg-config --libs libxml-2.0`
#LIBXML2_STATIC = `pkg-config --static --libs libxml-2.0`
#LIBXML2_CFLAGS = `pkg-config --cflags libxml-2.0`

LIBGLIB = `pkg-config --libs glib-2.0`
LIBGLIB_STATIC = `pkg-config --static --libs glib-2.0`
LIBGLIB_CFLAGS = `pkg-config --cflags glib-2.0`

#LIBXMLRPC = `xmlrpc-c-config abyss-server --libs`
LIBXMLRPC = -lxmlrpc_server_abyss -lxmlrpc_server -lxmlrpc_abyss \
            -lxmlrpc -lxmlrpc_util -lxmlrpc_xmlparse -lxmlrpc_xmltok \
            -lxmlrpc_client -lcurl

LIBXMLRPC_CFLAGS = `xmlrpc-c-config abyss-server --cflags`

LIBLOG4C = `log4c-config --libs`
LIBLOG4C_CFLAGS = `log4c-config --cflags`

#SLA_LIBS = ./lib/sla/x$(ARCH)/libsla.a /usr/lib/libf2c.a

# http://www.g95.org/
SLA_LIBS = ./lib/sla/x$(ARCH)/libsla.a \
           ./lib/sla/x$(ARCH)/libf95.a \
           ./lib/sla/x$(ARCH)/libcnf.a

# static
#EXPOSED_LIBS = -Wl,-Bstatic $(LIBGLIB_STATIC) $(LIBXMLRPC) $(LIBLOG4C) -lcfitsio \
#               -Wl,-Bdynamic -ldl -lm -lpthread
#
#DUMMY_LIBS = -Wl,-Bstatic -Wl,-Bdynamic -lcfitsio -fPIC -shared
#SAURON_LIBS = -Wl,-Bstatic -Wl,-Bdynamic -lcfitsio -lpvcam -fPIC -shared
#FRODO_LIBS = -Wl,-Bstatic -Wl,-Bdynamic -lcfitsio -fPIC -shared

#LIBASTROPCI = ./lib/frodo/libastropciAPI.a
LIBASTROPCI = -L/opt/lib -lCArcDevice
#LIBSLA = -L/opt/lib -lsla

# dynamic
EXPOSED_LIBS = -lcfitsio $(LIBGLIB) $(LIBXMLRPC) $(LIBLOG4C) -lpthread -ldl -lm -lrt
DUMMY_LIBS =  -lcfitsio -fPIC -shared
SAURON_LIBS = -lcfitsio -lpvcam -fPIC -shared
FRODO_LIBS =  $(LIBASTROPCI) -lcfitsio -fPIC -shared
BILBO_LIBS = -lcfitsio $(LIBLOG4C) -lpthread -fPIC -shared
GANDALF_LIBS = -L/data/pylon_2kb_excelon/lib -lpicam $(LIBLOG4C) -lpthread

INCLUDE = -I/opt/java/include -I/opt/java/include/linux

all: header exposed mod_ccd_dummy.so mod_ccd_sauron.so mod_ccd_frodo.so restore_raw_data

# make header.h header.c
header: ./src/make_header.py
	./src/make_header.py

exposed: ./src/exposed.c ./src/exposed.h socket.o thread.o modules.o header.o fce.o cfg.o spectrograph.o telescope.o bxmlrpc.o
	$(CC) $(SVN_REV) -o ./bin/exposed ./src/exposed.c \
        socket.o thread.o modules.o header.o fce.o cfg.o \
        spectrograph.o telescope.o bxmlrpc.o \
        $(EXPOSED_LIBS) $(SLA_LIBS)

socket.o: ./src/socket.c ./src/socket.h
	$(CC) -c ./src/socket.c

thread.o: ./src/thread.c ./src/thread.h
	$(CC) -fPIC -c ./src/thread.c

modules.o: ./src/modules.c ./src/modules.h
	$(CC) -c ./src/modules.c

header.o: ./src/header.c ./src/header.h
	$(CC) -c ./src/header.c

fce.o: ./src/fce.c ./src/fce.h
	$(CC) -c ./src/fce.c

st_fce: ./src/fce.c ./src/fce.h cfg.o
	$(CC) $(SVN_REV) -DSELF_TEST_FCE -o ./bin/st_fce ./src/fce.c cfg.o $(EXPOSED_LIBS)

cfg.o: ./src/cfg.c ./src/cfg.h
	$(CC) $(LIBGLIB_CFLAGS) -c ./src/cfg.c

spectrograph.o: ./src/spectrograph.c ./src/spectrograph.h
	$(CC) $(SVN_REV) -c ./src/spectrograph.c

telescope.o: ./src/telescope.c ./src/telescope.h
	$(CC) $(SVN_REV) -c ./src/telescope.c

st_telescope: ./src/telescope.c ./src/telescope.h bxmlrpc.o
	$(CC) $(SVN_REV) -DSELF_TELESCOPE -o ./bin/st_telescope ./src/telescope.c \
	$(EXPOSED_LIBS) $(SLA_LIBS) bxmlrpc.o

bxmlrpc.o: ./src/bxmlrpc.c ./src/bxmlrpc.h
	$(CC) $(SVN_REV) -c ./src/bxmlrpc.c

mod_ccd_dummy.so: ./src/mod_ccd_dummy.c mod_ccd.o thread.o
	$(CC) $(DUMMY_LIBS) -o ./modules/mod_ccd_dummy.so ./src/mod_ccd_dummy.c mod_ccd.o thread.o

mod_ccd_sauron.so: ./src/mod_ccd_sauron.c mod_ccd.o thread.o
	$(CC) $(SAURON_LIBS) -o ./modules/mod_ccd_sauron.so ./src/mod_ccd_sauron.c mod_ccd.o thread.o

mod_ccd_frodo.so: ./src/mod_ccd_frodo.c mod_ccd.o thread.o
	$(CC) $(FRODO_LIBS) -o ./modules/mod_ccd_frodo.so ./src/mod_ccd_frodo.c mod_ccd.o thread.o

mod_ccd_gandalf.so: ./src/mod_ccd_gandalf.c mod_ccd.o thread.o
	$(CC) $(GANDALF_LIBS) -o ./modules/mod_ccd_gandalf.so -fPIC -shared \
        ./src/mod_ccd_gandalf.c mod_ccd.o thread.o

mod_ccd_rpcgandalf.so: ./src/mod_ccd_rpcgandalf.c mod_ccd.o thread.o
	$(CC) $(SVN_REV) $(LIBGLIB_CFLAGS) -o ./modules/mod_ccd_rpcgandalf.so -fPIC -shared \
        ./src/mod_ccd_rpcgandalf.c mod_ccd.o thread.o

st_gandalf: ./src/mod_ccd_gandalf.c mod_ccd.o thread.o cfg.o
	$(CC) -DSELF_TEST_GANDALF $(GANDALF_LIBS) $(EXPOSED_LIBS) \
        -o ./bin/st_gandalf \
        ./src/mod_ccd_gandalf.c mod_ccd.o thread.o cfg.o

st_rpcgandalf: ./src/mod_ccd_rpcgandalf.c mod_ccd.o thread.o cfg.o
	$(CC) -DSELF_TEST_RPCGANDALF $(SVN_REV) $(EXPOSED_LIBS) $(LIBGLIB_CFLAGS) $(LIBGLIB) \
        -o ./bin/st_rpcgandalf \
        ./src/mod_ccd_rpcgandalf.c mod_ccd.o thread.o cfg.o

st_frodo: ./src/mod_ccd_frodo.c mod_ccd.o thread.o cfg.o
	$(CC) -DSELF_TEST_FRODO $(LIBASTROPCI) $(EXPOSED_LIBS) \
        -o ./bin/st_frodo \
        ./src/mod_ccd_frodo.c mod_ccd.o thread.o cfg.o
	
mod_ccd_bilbo.so: ./src/mod_ccd_bilbo.c mod_ccd.o thread.o
	$(CC) $(BILBO_LIBS) -o ./modules/mod_ccd_bilbo.so ./src/mod_ccd_bilbo.c mod_ccd.o thread.o	

mod_ccd.o: ./src/mod_ccd.c ./src/mod_ccd.h
	$(CC) $(SVN_REV) -fPIC -c ./src/mod_ccd.c
	
frodo_expose: ./src/examples/frodo_expose.c
	$(CC) $(LIBASTROPCI) -o ./bin/frodo_expose ./src/examples/frodo_expose.c

restore_raw_data: ./src/utils/restore_raw_data.c header.o
	$(CC) $(LIBGLIB_CFLAGS) $(LIBGLIB) -o ./bin/restore_raw_data ./src/utils/restore_raw_data.c \
        -lcfitsio -lm header.o

clean:
	-rm *.o
