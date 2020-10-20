/** @file

MODULE						: ModDQuantH263VlcDecoder

TAG								: MDQH263VD

FILE NAME					: ModDQuantH263VlcDecoder.cpp

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

#include "ModDQuantH263VlcDecoder.h"

ModDQuantH263VlcDecoder::ModDQuantH263VlcDecoder()
{
	_numCodeBits	= 0;
}//end constructor.

ModDQuantH263VlcDecoder::~ModDQuantH263VlcDecoder()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode a DQUANT value from the stream.
The H.263 standard defines a fixed length code of 2 bits or 6 bits depending
on the current value of QUANT. Read the codeword from the bit stream and return 
it in symbol2. _numCodeBits is set and is the return value.
@param bsr			: Bit stream to read from.
@param symbol1	: Current QUANT value. (input)
@param symbol2	: Returned DQUANT value.
@return					: Num bits extracted.
*/
int ModDQuantH263VlcDecoder::Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2)
{
	_numCodeBits	= 0;
	int dquant		= 0;
	int quant			= *symbol1;

	// First decoded bit determines if 2 bit or 6 bit value.
	if(bsr->Read())	// 1 = 2 bit type.
	{
		_numCodeBits = 2;
		if(bsr->Read()) // 11
		{
			if( (quant <= 10)||(quant == 30) )
				dquant = 1;
			else if( (quant <= 20)||(quant == 29) )
				dquant = 2;
			else if(quant <= 28)
				dquant = 3;
			else if(quant == 31)
				dquant = -5;
		}//end if 11...
		else						// 10
		{
			if(quant == 1)
				dquant = 2;
			else if(quant <= 10)
				dquant = -1;
			else if(quant <= 20)
				dquant = -2;
			else
				dquant = -3;
		}//end else 10...
	}//end if 1...
	else						// 0 = 6 bit type.
	{
		_numCodeBits = 6;
		int x = bsr->Read(5);
		dquant = x - quant;
	}//end else 0...
	
	*symbol2 = dquant;
	return(_numCodeBits);
}//end Decode2.

