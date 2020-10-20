/** @file

MODULE						: DQuantH263VlcDecoder

TAG								: DQH263VD

FILE NAME					: DQuantH263VlcDecoder.cpp

DESCRIPTION				: A DQuant Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.6
										page 37 Table 12 with an IVlcDecoder Interface.

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

#include "DQuantH263VlcDecoder.h"

DQuantH263VlcDecoder::DQuantH263VlcDecoder()
{
	_numCodeBits	= 0;
}//end constructor.

DQuantH263VlcDecoder::~DQuantH263VlcDecoder()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode an INTRADC value from the stream.
The H.263 standard defines a fixed length code of 2 bits. Read the 
codeword from the bit stream and return it. _numCodeBits is set.
@param bsr	: Bit stream to read from.
@return			: The decoded DQUANT value.
*/
int DQuantH263VlcDecoder::Decode(IBitStreamReader* bsr)
{
	_numCodeBits = 2;
	// DQUANT is a 2 bit FLC.
	int symbol = bsr->Read(2);
	int dquant = 0;

	switch(symbol)
	{
		case 0:
			dquant = -1;
			break;
		case 1:
			dquant = -2;
			break;
		case 2:
			dquant = 1;
			break;
		case 3:
			dquant = 2;
			break;
	}//end switch dquant...

	return(dquant);
}//end Decode.

