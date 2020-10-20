/** @file

MODULE						: SwitchCodecHeaderVer1

TAG								: SCHV1

FILE NAME					: SwitchCodecHeaderVer1.h

DESCRIPTION				: The switching codec contains two independent bit streams
										with a header to describe the contents and their offsets.
										This is a utility class to extract that info.

COPYRIGHT					: (c)CSIR, Meraka Institute 2007-2012 all rights reserved

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of CSIR, Meraka Institute and has been 
										classified as CONFIDENTIAL.
===========================================================================
*/
#ifndef _SWITCHCODECHEADERVER1_H
#define _SWITCHCODECHEADERVER1_H

#pragma once

#include "IStreamHeaderReader.h"
#include "IBitStreamReader.h"

/**
---------------------------------------------------------------------------
	Switching Stream definition.
---------------------------------------------------------------------------
*/
/**
	Header:
		unsigned short	MainStreamByteOffset;			///< Offset from byte 0 to MainStream[]
		unsigned short	MainStreamByteSize;
		unsigned short	MainStreamPicType;
		unsigned short	SwitchStreamByteOffset;		///< Offset from byte 0 to SwitchStream[]
		unsigned short	SwitchStreamByteSize;
		unsigned short	SwitchStreamPicType;
	Main stream:
		byte						MainStream[MainStreamByteSize];
	Switching stream:
		byte						SwitchStream[SwitchStreamByteSize];
*/

/**
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class SwitchCodecHeaderVer1: public IStreamHeaderReader
{
public:
	SwitchCodecHeaderVer1();
	virtual ~SwitchCodecHeaderVer1();

/// Interface implementation.
public:
	/** Extract the header from the input stream.
	Extract the header and pointers from the input into the
	class members.
	@param pSS		: Stream to extract from.
	@param bitLen	: Bit length of input stream.
	@return				: 1 = success, 0 = failed.
	*/
	int Extract(void* pSS, int bitLen);

	int GetHeaderBitLength(void) { return(6*16); }

	int Get(const char* name, int* value) { return(0); }

	/// Implementation specific access methods.
public:
	/** Get the header info after extraction.
	The data returned is that from the last call to ExtractHeaderInfo().
	into the current stream position.
	@return	: Requested header value.
	*/
	unsigned short GetMainStreamByteOffset(void)		{ return(_mainStreamByteOffset); }
	unsigned short GetMainStreamByteSize(void)			{ return(_mainStreamByteSize); }
	unsigned short GetMainStreamPicType(void)				{ return(_mainStreamPicType); }
	unsigned short GetSwitchStreamByteOffset(void)	{ return(_switchStreamByteOffset); }
	unsigned short GetSwitchStreamByteSize(void)		{ return(_switchStreamByteSize); }
	unsigned short GetSwitchStreamPicType(void)			{ return(_switchStreamPicType); }
	void*					 GetMainStreamPtr(void)						{ return(_pMainStream); }
	void*					 GetSwitchStreamPtr(void)					{ return(_pSwitchStream); }

///	Persistant stream header data valid after an Extract() call.
protected:
	unsigned short		_mainStreamByteOffset;			///< Offset from byte 0 to MainStream[]
	unsigned short		_mainStreamByteSize;
	unsigned short		_mainStreamPicType;
	unsigned short		_switchStreamByteOffset;		///< Offset from byte 0 to SwitchStream[]
	unsigned short		_switchStreamByteSize;
	unsigned short		_switchStreamPicType;
	void*							_pMainStream;
	void*							_pSwitchStream;

	IBitStreamReader*	_pBitStreamReader;

};// end class SwitchCodecHeaderVer1.

#endif	// _SWITCHCODECHEADERVER1_H
