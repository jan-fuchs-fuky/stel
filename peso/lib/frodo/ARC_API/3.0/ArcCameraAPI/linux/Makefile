INC_DIR		=	-I../					\
			-I../../../3.0/CArcDevice		\
			-I../../../2.0/CDeinterlace		\
			-I../../../2.0/CImage			\
			-I../../../2.0/CFitsFile		\
			-I../../../2.0/cfitsio/linux

LIB_DIR_X64	=	-L../../../3.0/CArcDevice/linux/x64	\
			-L../../../2.0/CDeinterlace/linux/x64	\
			-L../../../2.0/CImage/linux/x64		\
			-L../../../2.0/CFitsFile/linux/x64	\
			-L../../../2.0/cfitsio/linux/x64

LIB_DIR_X32	=	-L../../../3.0/CArcDevice/linux/x32	\
			-L../../../2.0/CDeinterlace/linux/x32	\
			-L../../../2.0/CImage/linux/x32		\
			-L../../../2.0/CFitsFile/linux/x32	\
			-L../../../2.0/cfitsio/linux/x32

LIB_FILES	=	-lCArcDevice			\
			-lCDeinterlace			\
			-lCFitsFile			\
			-lCImage			\
			-lcfitsio			\
			-lm -lnsl

CPP_FILES	=	../ArcCameraAPI.cpp		\
			../DllMain.cpp

OUT_FILE	=	libArcCameraAPI.so


all: m64 m32

m64:
	gcc -fPIC -c -Wall $(INC_DIR) $(INC_DIR_X64) $(CPP_FILES)
	gcc -shared -o x64/$(OUT_FILE) $(LIB_DIR_X64) *.o $(LIB_FILES)
	rm *.o

m32:
	gcc -m32 -fPIC -c -Wall $(INC_DIR) $(INC_DIR_X32) $(CPP_FILES)
	gcc -m32 -shared -o x32/$(OUT_FILE) $(LIB_DIR_X32) *.o $(LIB_FILES)
	rm *.o

clean: clean_m64 clean_m32

clean_m64:
	rm x64/$(OUT_FILE)

clean_m32:
	rm x32/$(OUT_FILE)

