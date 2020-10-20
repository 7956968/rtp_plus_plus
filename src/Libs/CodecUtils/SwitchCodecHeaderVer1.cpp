/** @file

MODULE						: SwitchCodecHeaderVer1

TAG								: SCHV1

FILE NAME					: SwitchCodecHeaderVer1.cpp

DESCRIPTION				: The switching codec contains two independent bit streams
										with a header to describe the contents and their offsets.
										This is a utility class to extract that info.

COPYRIGHT					: (c)CSIR, Meraka Institute 2007-2011 all rights reserved

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR, Meraka Institute and has been 
										classified as CONFIDENTIAL.
===========================================================================
*/
#ifdef _WINDOWS
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN     // Exclude rarely-used stuff from Windows headers
#endif
#include <windows.h>
#else
#include <stdio.h>
#endif

#include "BitStreamReaderMSB.h"
#include "SwitchCodecHeaderVer1.h"

SwitchCodecHeaderVer1::SwitchCodecHeaderVer1()
{
	/// Reset all members.
	_mainStreamByteOffset			= 0;
	_mainStreamByteSize				= 0;
	_mainStreamPicType				= 0;
	_switchStreamByteOffset		= 0;
	_switchStreamByteSize			= 0;
	_switchStreamPicType			= 0;
	_pMainStream							= NULL;
	_pSwitchStream						= NULL;

	/// Create a bit stream reader to use during bit extraction.
	_pBitStreamReader	= NULL;
	_pBitStreamReader = new BitStreamReaderMSB();

}//end constructor.

SwitchCodecHeaderVer1::~SwitchCodecHeaderVer1()
{
	if(_pBitStreamReader != NULL)
		delete _pBitStreamReader;
	_pBitStreamReader = NULL;
}//end destructor.

/** Extract the header from the input stream.
Extract the header and pointers from the input into the
class memebers.
@param pSS		: Stream to extract from.
@param bitLen	: Bit length of input stream.
@return				: 1 = success, 0 = failed.
*/
int SwitchCodecHeaderVer1::Extract(void* pSS, int bitLen)
{
	if( (_pBitStreamReader == NULL)||(pSS == NULL) )
		return(0);

	_pBitStreamReader->SetStream(pSS, bitLen);

	_mainStreamByteOffset			= _pBitStreamReader->Read(16);
	_mainStreamByteSize				= _pBitStreamReader->Read(16);
	_mainStreamPicType				= _pBitStreamReader->Read(16);
	_switchStreamByteOffset		= _pBitStreamReader->Read(16);
	_switchStreamByteSize			= _pBitStreamReader->Read(16);
	_switchStreamPicType			= _pBitStreamReader->Read(16);

	char* p = (char *)pSS;
	_pMainStream							= (void *)(p + _mainStreamByteOffset);
	_pSwitchStream						= (void *)(p + _switchStreamByteOffset);

	return(1);
}//end ExtractHeaderInfo.


