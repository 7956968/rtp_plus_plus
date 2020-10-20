/** @file

MODULE						: IStreamHeader

TAG								: ISH

FILE NAME					: IStreamHeader.h

DESCRIPTION				: A IStreamHeader Interface as an abstract base class to 
										accessing stream headers that have bit/byte 
										representations. The access is defined by text (ASCII)
										strings.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _ISTREAMHEADER_H
#define _ISTREAMHEADER_H

#pragma once

class IStreamHeader
{
public:
	virtual ~IStreamHeader() {}

	/** Set the stream to use.
	All Get/Set members will operate on this stream.
	@param pStream	: The stream ptr.
	@return					: 0 = invalid ptr, 1 = is set.
	*/
	virtual int SetStream(void* pStream) = 0;

	/** Get the bit/byte length of the header.
	This indicates the exact no. of bits/bytes in the header part of
	the stream. If the byte length is not aligned it will return 0.
	@return	: The length.
	*/
	virtual int GetBitLength(void) = 0;
	virtual int GetByteLength(void) = 0;

	/** Get/Set the header value by name.
	This is the primary method of Get/Set for all header variables and caters 
	for all variants by using strings. Care must be taken when unicode strings
	are used. Typical usage:
		int	val,len;
		if( !Impl->Get((const unsigned char*)("width"), &val, &len) )
			errorStr = "Header value does not exist";

	@param name		:	A string of the header variable required.
	@param value	:	The header value required or to write.
	@param bitLen	:	The bit length of the header representation of the value.
	@return				: 1 = found, 0 = no header exists with this name.
	*/
	virtual int Get(const char* name, int* value, int* bitLen) = 0;
	virtual int Set(const char* name, int value, int bitLen) = 0;

};// end class IStreamHeader.

#endif	//_ISTREAMHEADER_H
