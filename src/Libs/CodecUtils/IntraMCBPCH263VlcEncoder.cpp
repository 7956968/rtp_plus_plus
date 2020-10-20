/** @file

MODULE						: IntraMCBPCH263VlcEncoder

TAG								: IMCBPCH263VE

FILE NAME					: IntraMCBPCH263VlcEncoder.cpp

DESCRIPTION				: An MCBPC Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.2
										page 34 with an	IVlcEncoder Interface.

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

#include "IntraMCBPCH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
IntraMCBPCH263VlcEncoder::IntraMCBPCH263VlcEncoder(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end constructor.

IntraMCBPCH263VlcEncoder::~IntraMCBPCH263VlcEncoder(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Encode MB type and CBPC symbols.
The 2 param symbols represent MB type and CBPC, respectively and are
encoded with the number of bits returned. The _numCodeBits and _bitCode
members are set accordingly.
@param symbol1	: MB type
@param symbol2	: CBPC
@return					: Num of bits in the codeword.
*/
int	IntraMCBPCH263VlcEncoder::Encode2(int symbol1, int symbol2)
{
	int mbt		= symbol1;
	int cbpc	= symbol2;

	// Intra pictures can only have type 3 or 4.
	_bitCode			= 0;	// If error, this will be returned.
	_numCodeBits	= 0;

	if(mbt == 3)
	{
		switch(cbpc)
		{
			case 0:
				_bitCode			= 0x0001;
				_numCodeBits	= 1;
				break;
			case 1:
				_bitCode			= 0x0001;
				_numCodeBits	= 3;
				break;
			case 2:
				_bitCode			= 0x0002;
				_numCodeBits	= 3;
				break;
			case 3:
				_bitCode			= 0x0003;
				_numCodeBits	= 3;
				break;
		}//end switch cbpc...
	}//end if type 3...
	else if(mbt == 4)
	{
		switch(cbpc)
		{
			case 0:
				_bitCode			= 0x0001;
				_numCodeBits	= 4;
				break;
			case 1:
				_bitCode			= 0x0001;
				_numCodeBits	= 6;
				break;
			case 2:
				_bitCode			= 0x0002;
				_numCodeBits	= 6;
				break;
			case 3:
				_bitCode			= 0x0003;
				_numCodeBits	= 6;
				break;
		}//end switch cbpc...
	}//end if type 4...

	return(_numCodeBits);
}// end Encode2.

