# This is the super Makefile for the ARC_API libraries.

OWL_PATH	=	/home/streit/Documents/JAVA/Owl/2.0
RELEASE_PATH	=	/home/streit/Documents/ARC_API/2.0/Release

X64_COPY	=	cp -f x64/*.so $(RELEASE_PATH)/x64
X32_COPY	=	cp -f x32/*.so $(RELEASE_PATH)/x32

X64_OWL_COPY	=	cp -f x64/*.so $(OWL_PATH)/x64/API
X32_OWL_COPY	=	cp -f x32/*.so $(OWL_PATH)/x32/API


all: m32

m64:
	@echo  
	@echo ------------------------------------------------------------------------------
	@echo  Bulding CDeinterlace x64
	@echo ------------------------------------------------------------------------------
	cd CDeinterlace/linux; $(MAKE) m64;

	@echo  
	@echo ------------------------------------------------------------------------------
	@echo  Bulding CFitsFile x64
	@echo ------------------------------------------------------------------------------
	cd CFitsFile/linux; $(MAKE) m64;

	@echo  
	@echo ------------------------------------------------------------------------------
	@echo  Bulding CImage x64
	@echo ------------------------------------------------------------------------------
	cd CImage/linux; $(MAKE) m64;

m32:
	@echo  
	@echo ------------------------------------------------------------------------------
	@echo  Bulding CDeinterlace x32
	@echo ------------------------------------------------------------------------------
	cd CDeinterlace/linux; $(MAKE) m32;

	@echo  
	@echo ------------------------------------------------------------------------------
	@echo  Bulding CFitsFile x32
	@echo ------------------------------------------------------------------------------
	cd CFitsFile/linux; $(MAKE) m32;

	@echo  
	@echo ------------------------------------------------------------------------------
	@echo  Bulding CImage x32
	@echo ------------------------------------------------------------------------------
	cd CImage/linux; $(MAKE) m32;

clean: clean_m64 clean_m32

clean_m64:
	cd CController/linux; $(MAKE) clean_m64
	cd CDeinterlace/linux; $(MAKE) clean_m64
	cd CFitsFile/linux; $(MAKE) clean_m64
	cd CTiffFile/linux; $(MAKE) clean_m64
	cd CXLBuffer/linux; $(MAKE) clean_m64
	cd CDisplay/linux; $(MAKE) clean_m64
	cd CImage/linux; $(MAKE) clean_m64
	cd CArcAPIClient/linux; $(MAKE) clean_m64
	cd CameraAPI/linux; $(MAKE) clean_m64
	cd CameraAPIClient/linux; $(MAKE) clean_m64
	cd ArcAPIServer/linux; $(MAKE) clean_m64

clean_m32:
	cd CController/linux; $(MAKE) clean_m32
	cd CDeinterlace/linux; $(MAKE) clean_m32
	cd CFitsFile/linux; $(MAKE) clean_m32
	cd CTiffFile/linux; $(MAKE) clean_m32
	cd CXLBuffer/linux; $(MAKE) clean_m32
	cd CDisplay/linux; $(MAKE) clean_m32
	cd CImage/linux; $(MAKE) clean_m32
	cd CArcAPIClient/linux; $(MAKE) clean_m32
	cd CameraAPI/linux; $(MAKE) clean_m32
	cd CameraAPIClient/linux; $(MAKE) clean_m32
	cd ArcAPIServer/linux; $(MAKE) clean_m32
