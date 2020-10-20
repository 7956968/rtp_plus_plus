/** @file

MODULE						: DQuantH263VlcEncoder

TAG								: DQH263VE

FILE NAME					: DQuantH263VlcEncoder.cpp

DESCRIPTION				: An DQuant Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.6
										page 37 Table 12 with an	IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include "DQuantH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
DQuantH263VlcEncoder::DQuantH263VlcEncoder(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end constructor.

DQuantH263VlcEncoder::~DQuantH263VlcEncoder(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Encode the symbol as INTRADC.
The H.263 standard defines a fixed length code of 2 bits. Valid symbols
are limited to the set [-1, -2, 1, 2]. Write the codeword to _bitCode 
and return the num of valid bits.
@param symbol	: DQUANT value to encode.
@return				: Num of bits in the codeword _bitCode.
*/
int DQuantH263VlcEncoder::Encode(int symbol)
{
	_bitCode			= 0;	// If error, this will be returned.
	_numCodeBits	= 2;	// Default expects a result.

	switch(symbol)
	{
		case -1:
			_bitCode = 0x0000;
			break;
		case -2:
			_bitCode = 0x0001;
			break;
		case 1:
			_bitCode = 0x0002;
			break;
		case 2:
			_bitCode = 0x0003;
			break;
		default:
			_numCodeBits = 0;	// No encoding for this symbol.
			break;
	}//end switch symbol...

	return(_numCodeBits);
}//end Encode.

