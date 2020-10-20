/** @file

MODULE						: ModDQuantH263VlcEncoder

TAG								: MDQH263VE

FILE NAME					: ModDQuantH263VlcEncoder.cpp

DESCRIPTION				: A modified DQuant Vlc encoder implementation as defined 
										in ITU-T Recommendation H.263 (02/98) Annex T page 147 
										Table T.1 with an	IVlcEncoder Interface.
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

#include "ModDQuantH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
ModDQuantH263VlcEncoder::ModDQuantH263VlcEncoder(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end constructor.

ModDQuantH263VlcEncoder::~ModDQuantH263VlcEncoder(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Encode the symbols as modified DQUANT.
The H.263 standard defines a fixed length code of 2 bits for small changes
or a 6 bit code for arbitrary values. Valid symbols vary depending on the
prior value of QUANT. Write the codeword to _bitCode and return the num of 
valid bits.
@param symbol1	: Prior QUANT value before DQUANT.
@param symbol1	: DQUANT value to encode.
@return					: Num of bits in the codeword _bitCode.
*/
int ModDQuantH263VlcEncoder::Encode2(int symbol1, int symbol2)
{
	_bitCode			= 0;	// If error, this will be returned.
	_numCodeBits	= 0;	// Default no result.

	switch(symbol2)	// Try the 2 bit possibilities 1st.
	{
		case 1:
			if( (symbol1 <= 10)||(symbol1 == 30) )
			{
				_bitCode			= 0x0003;
				_numCodeBits	= 2;
			}//end if symbol1...
			break;
		case -1:
			if( (symbol1 >= 2)&&(symbol1 <= 10) )
			{
				_bitCode			= 0x0002;
				_numCodeBits	= 2;
			}//end if symbol1...
			break;
		case 2:
			if( ((symbol1 >= 11)&&(symbol1 <= 20)) || (symbol1 == 29) )
			{
				_bitCode			= 0x0003;
				_numCodeBits	= 2;
			}//end if symbol1...
			break;
		case -2:
			if( (symbol1 >= 11)&&(symbol1 <= 20) )
			{
				_bitCode			= 0x0002;
				_numCodeBits	= 2;
			}//end if symbol1...
			break;
		case 3:
			if( (symbol1 >= 21)&&(symbol1 <= 28) )
			{
				_bitCode			= 0x0003;
				_numCodeBits	= 2;
			}//end if symbol1...
			break;
		case -3:
			if(symbol1 >= 21)
			{
				_bitCode			= 0x0002;
				_numCodeBits	= 2;
			}//end if symbol1...
			break;
		case -5:
			if(symbol1 == 31)
			{
				_bitCode			= 0x0003;
				_numCodeBits	= 2;
			}//end if symbol1...
			break;
	}//end switch symbol2...

	// _numCodeBits indicates if a result has already been found.
	if(!_numCodeBits)
	{
		// For this case the actual new value of QUANT is signalled.
		int quant = symbol1 + symbol2;
		if(quant < 1)
			quant = 1;
		else if(quant > 31)
			quant = 31;
		_bitCode			= quant;
		_numCodeBits	= 6;
	}//end if !_numCodeBits...

	return(_numCodeBits);
}//end Encode.

