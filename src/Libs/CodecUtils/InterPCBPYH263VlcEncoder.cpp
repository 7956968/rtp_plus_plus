/** @file

MODULE						: InterPCBPYH263VlcEncoder

TAG								: IPCBPYH263VE

FILE NAME					: InterPCBPYH263VlcEncoder.cpp

DESCRIPTION				: An CBPY Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.5
										page 37 and Table 13 with an IVlcEncoder Interface.

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

#include "InterPCBPYH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
InterPCBPYH263VlcEncoder::InterPCBPYH263VlcEncoder(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end constructor.

InterPCBPYH263VlcEncoder::~InterPCBPYH263VlcEncoder(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Encode CBPY symbols.
The symbol represent the CBPY pattern and is encoded with the number 
of bits returned. The _numCodeBits and _bitCode members are set 
accordingly.
@param symbol	: CBPY
@return				: Num of bits in the codeword.
*/
int	InterPCBPYH263VlcEncoder::Encode(int symbol)
{
	int cbpy	= symbol;

	_bitCode			= 0;	// If error, this will be returned.
	_numCodeBits	= 0;

	switch(cbpy)
	{
		case 15:
			_bitCode			= 0x0003;
			_numCodeBits	= 4;
			break;
		case 14:
			_bitCode			= 0x0005;
			_numCodeBits	= 5;
			break;
		case 13:
			_bitCode			= 0x0004;
			_numCodeBits	= 5;
			break;
		case 12:
			_bitCode			= 0x0009;
			_numCodeBits	= 4;
			break;
		case 11:
			_bitCode			= 0x0003;
			_numCodeBits	= 5;
			break;
		case 10:
			_bitCode			= 0x0007;
			_numCodeBits	= 4;
			break;
		case 9:
			_bitCode			= 0x0002;
			_numCodeBits	= 6;
			break;
		case 8:
			_bitCode			= 0x000B;
			_numCodeBits	= 4;
			break;
		case 7:
			_bitCode			= 0x0002;
			_numCodeBits	= 5;
			break;
		case 6:
			_bitCode			= 0x0003;
			_numCodeBits	= 6;
			break;
		case 5:
			_bitCode			= 0x0005;
			_numCodeBits	= 4;
			break;
		case 4:
			_bitCode			= 0x000A;
			_numCodeBits	= 4;
			break;
		case 3:
			_bitCode			= 0x0004;
			_numCodeBits	= 4;
			break;
		case 2:
			_bitCode			= 0x0008;
			_numCodeBits	= 4;
			break;
		case 1:
			_bitCode			= 0x0006;
			_numCodeBits	= 4;
			break;
		case 0:
			_bitCode			= 0x0003;
			_numCodeBits	= 2;
			break;
	}//end switch cbpy...

	return(_numCodeBits);
}// end Encode.

