/** @file

MODULE						: AdvancedIntraModeH263VlcEncoder

TAG								: AIMH263VE

FILE NAME					: AdvancedIntraModeH263VlcEncoder.cpp

DESCRIPTION				: An advanced Intra mode Vlc encoder implementation 
										as defined in ITU-T Recommendation H.263 (02/98) 
										annex I page 74 Table I.1 with an	IVlcEncoder Interface.

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

#include "AdvancedIntraModeH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
AdvancedIntraModeH263VlcEncoder::AdvancedIntraModeH263VlcEncoder(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end constructor.

AdvancedIntraModeH263VlcEncoder::~AdvancedIntraModeH263VlcEncoder(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Encode the symbol as INTRA_MODE.
The H.263 standard defines a vlc of 1/2 bits. Write the codeword to _bitCode 
and return the num of valid bits.
@param symbol	: INTRA_MODE value to encode.
@return				: Num of bits in the codeword _bitCode.
*/
int AdvancedIntraModeH263VlcEncoder::Encode(int symbol)
{
	_bitCode			= 0;	// If error, this will be returned.
	_numCodeBits	= 0;

	switch(symbol)
	{
		case 0:	// Intramode == 0 (= DC only mode).
			_bitCode			= 0x0000;
			_numCodeBits	= 1;
			break;
		case 1:	// 1 = Vertical DC & AC mode.
			_bitCode			= 0x0002;
			_numCodeBits	= 2;
			break;
		case 2:	// 2 = Horiz DC & AC mode.
			_bitCode			= 0x0003;
			_numCodeBits	= 2;
			break;
	}//end switch symbol...

	return(_numCodeBits);
}//end Encode.

