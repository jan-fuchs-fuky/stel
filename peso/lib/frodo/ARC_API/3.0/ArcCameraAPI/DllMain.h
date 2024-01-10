#ifndef _ARC_CAMERA_API_DLLMAIN_H_
#define _ARC_CAMERA_API_DLLMAIN_H_


// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the CARCDEVICE_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// CARCDEVICE_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
	#ifdef ARCCAM_API_EXPORTS
		#define ARCCAM_API __declspec( dllexport )
	#else
		#define ARCCAM_API __declspec( dllimport )
	#endif
#else
	#define ARCCAM_API __attribute__((visibility("default")))
#endif


#endif		// _ARC_CAMERA_API_DLLMAIN_H_
