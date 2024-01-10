INC_DIR		=	-I../			\
			-I../../cfitsio/linux

LIB_DIR		=	-L../../cfitsio/linux

CPP_FILES	=	../CFitsFile.cpp	\
			../DllMain.cpp

OUT_FILE	=	libCFitsFile.so


all: m64 m32

m64:
	g++ -fPIC -c -Wall $(CPP_FILES)
	g++ -shared -o x64/$(OUT_FILE) $(LIB_DIR)/x64 *.o -lcfitsio -lc
	rm *.o

m32:
	g++ -m32 -fPIC -c -Wall $(CPP_FILES)
	g++ -m32 -shared -o x32/$(OUT_FILE) $(LIB_DIR)/x32 *.o -lcfitsio -lc
	rm *.o

clean: clean_m64 clean_m32

clean_m64:
	rm x64/$(OUT_FILE)

clean_m32:
	rm x32/$(OUT_FILE)
