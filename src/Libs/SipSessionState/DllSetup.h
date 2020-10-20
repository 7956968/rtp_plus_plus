#pragma once

// The following block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the SipSessionState_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// XML_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.

// Windows
#if defined(WIN32)
  #if defined(SipSessionState_EXPORTS)
    #define SIP_SESSION_STATE_API __declspec(dllexport)
  #else
    #define SIP_SESSION_STATE_API __declspec(dllimport)	
  #endif
#endif

// For Linux compilation && Windows static linking
#if !defined(SIP_SESSION_STATE_API)
	#define SIP_SESSION_STATE_API
#endif
