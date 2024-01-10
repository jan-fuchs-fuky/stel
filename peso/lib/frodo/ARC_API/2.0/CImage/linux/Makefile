INC_DIR		=	-I../

CPP_FILES	=	../CImage.cpp	\
				../DllMain.cpp

OUT_FILE	=	libCImage.so


all: m64 m32

m64:
	g++ -fPIC -c -Wall $(INC_DIR) $(CPP_FILES)
	g++ -shared -o x64/$(OUT_FILE) $(LIB_DIR) *.o
	rm *.o

m32:
	g++ -m32 -fPIC -c -Wall $(INC_DIR) $(CPP_FILES)
	g++ -m32 -shared -o x32/$(OUT_FILE) $(LIB_DIR) *.o
	rm *.o

clean: clean_m64 clean_m32

clean_m64:
	rm x64/$(OUT_FILE)

clean_m32:
	rm x32/$(OUT_FILE)
