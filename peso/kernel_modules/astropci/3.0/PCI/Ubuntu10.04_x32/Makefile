obj-m += astropci.o

default:
	make -C /lib/modules/`uname -r`/build/ SUBDIRS=`pwd` modules

install:
	make -C /lib/modules/`uname -r`/build/ SUBDIRS=`pwd` modules_install

clean:
	make -C /lib/modules/`uname -r`/build SUBDIRS=`pwd` clean
