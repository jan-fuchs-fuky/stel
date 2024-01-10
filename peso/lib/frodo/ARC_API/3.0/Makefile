# This is the super Makefile for the ARC_API libraries.

OWL_DIR			=	/home/streit/Documents/JAVA/Owl/3.0

RELEASE_DIR		=	/home/streit/Documents/ARC_API/3.0/Release
RELEASE_DIR_2		=	../2.0

X64_COPY		=	cp -f x64/*.so $(RELEASE_DIR)/x64
X32_COPY		=	cp -f x32/*.so $(RELEASE_DIR)/x32

X64_OWL_COPY		=	cp -f x64/*.so $(OWL_DIR)/x64/API
X32_OWL_COPY		=	cp -f x32/*.so $(OWL_DIR)/x32/API

all: m32

m64:
	@echo
	@echo ------------------------------------------------------------------------------
	@echo  Bulding CArcDevice x64
	@echo ------------------------------------------------------------------------------
	cd CArcDevice/linux; $(MAKE) m64;

	@echo
	@echo ------------------------------------------------------------------------------
	@echo  Bulding ArcCameraAPI x64
	@echo ------------------------------------------------------------------------------
	cd ArcCameraAPI/linux; $(MAKE) m64;

m32:
	@echo  
	@echo ------------------------------------------------------------------------------
	@echo  Bulding CArcDevice x32 
	@echo ------------------------------------------------------------------------------
	cd CArcDevice/linux; $(MAKE) m32;

	@echo  
	@echo ------------------------------------------------------------------------------
	@echo  Bulding ArcCameraAPI x32 
	@echo ------------------------------------------------------------------------------
	cd ArcCameraAPI/linux; $(MAKE) m32;

clean: clean_m64 clean_m32

clean_m64:
	cd CArcDevice/linux; $(MAKE) clean_m64

clean_m32:
	cd CameraAPI/linux; $(MAKE) clean_m32


