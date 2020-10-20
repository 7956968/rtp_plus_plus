/** @file

MODULE						: IntraDCH263VlcEncoder

TAG								: IDH263VE

FILE NAME					: IntraDCH263VlcEncoder.cpp

DESCRIPTION				: A class to implement a Intra DC coeff encoder as defined
										in the ITU-T Recommendation H.263 (02/98) Section 5.4.1
										Page 41. There is no Esc coding	requirements. It implements 
										the IVlcEncoder interface.

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

#include "IntraDCH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Private Methods.
---------------------------------------------------------------------------
*/
/** Encode the symbol as INTRADC.
The H.263 standard defines a fixed length code of 8 bits
where 0 and 128 are not used. Write the codeword to
_bitCode and return the num of valid bits.
@param symbol	: Intra DC coeff to encode.
@return				: Num of bits in the codeword _bitCode.
*/
int IntraDCH263VlcEncoder::Encode(int symbol)
{
	// Most common case is first test for optimal loop effeciency.
	if( (symbol > 0)&&(symbol != 128)&&(symbol < 255) )
		_bitCode = symbol;
	else if(symbol > 254)
		_bitCode = 254;
	else if(symbol == 128)
		_bitCode = 255;
	else //if(symbol < 1)
		_bitCode = 1;

	_numCodeBits = 8;	// FLC.

	return(_numCodeBits);
}//end Encode.

