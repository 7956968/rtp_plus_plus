/** @file

MODULE						: IntraDCH263VlcDecoder

TAG								: IDH263VD

FILE NAME					: IntraDCH263VlcDecoder.cpp

DESCRIPTION				: A class to implement a Intra DC coeff decoder as defined
										in the ITU-T Recommendation H.263 (02/98) Section 5.4.1
										Page 41. There is no Esc coding	requirements. It implements 
										the IVlcDecoder interface.

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

#include "IntraDCH263VlcDecoder.h"

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode an INTRADC value from the stream.
The H.263 standard defines a fixed length code of 8 bits
where 0 and 128 are not used. Read the codeword from the
bit stream and return it. _numCodeBits is set.
@param bsr	: Bit stream to read from.
@return			: The decoded INTRADC value.
*/
int IntraDCH263VlcDecoder::Decode(IBitStreamReader* bsr)
{
	_numCodeBits = 8;
	// INTRADC is an 8 FLC.
	int coeff = bsr->Read(8);
	if( (coeff == 0)||(coeff == 128) )
		return(0);	// Implies an error.

	if(coeff != 255)
		return(coeff);
	else
		return(128);
}//end Decode.

