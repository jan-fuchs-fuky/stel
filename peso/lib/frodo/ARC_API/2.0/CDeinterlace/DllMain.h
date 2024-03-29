#ifndef _ARC_CDEINTERLACE_DLLMAIN_H_
#define _ARC_CDEINTERLACE_DLLMAIN_H_


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the DLLTEST_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// DLLTEST_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
	#ifdef CDEINTERLACE_EXPORTS
		#define CDEINTERLACE_API __declspec( dllexport )
	#else
		#define CDEINTERLACE_API __declspec( dllimport )
	#endif
#else
	#define CDEINTERLACE_API __attribute__((visibility("default")))
//	#define CDEINTERLACE_API
#endif


#endif	// _ARC_CDEINTERLACE_DLLMAIN_H_
