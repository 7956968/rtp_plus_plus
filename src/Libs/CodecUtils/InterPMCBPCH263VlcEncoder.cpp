/** @file

MODULE						: InterPMCBPCH263VlcEncoder

TAG								: IPMCBPCH263VE

FILE NAME					: InterPMCBPCH263VlcEncoder.cpp

DESCRIPTION				: An MCBPC Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.2
										Table 8 page 35 with an	IVlcEncoder Interface.

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

#include "InterPMCBPCH263VlcEncoder.h"

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
InterPMCBPCH263VlcEncoder::InterPMCBPCH263VlcEncoder(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end constructor.

InterPMCBPCH263VlcEncoder::~InterPMCBPCH263VlcEncoder(void)
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
int	InterPMCBPCH263VlcEncoder::Encode2(int symbol1, int symbol2)
{
	int mbt		= symbol1;
	int cbpc	= symbol2;

	_bitCode			= 0;	// If error, this will be returned.
	_numCodeBits	= 0;

	if(mbt == 0)		// Inter (standard with no Q and no 4V).
	{
		switch(cbpc)
		{
			case 0:
				_bitCode			= 0x0001;
				_numCodeBits	= 1;
				break;
			case 1:
				_bitCode			= 0x0003;
				_numCodeBits	= 4;
				break;
			case 2:
				_bitCode			= 0x0002;
				_numCodeBits	= 4;
				break;
			case 3:
				_bitCode			= 0x0005;
				_numCodeBits	= 6;
				break;
		}//end switch cbpc...
	}//end if type 0...
	else if(mbt == 1)		// Inter + Q.
	{
		switch(cbpc)
		{
			case 0:
				_bitCode			= 0x0003;
				_numCodeBits	= 3;
				break;
			case 1:
				_bitCode			= 0x0007;
				_numCodeBits	= 7;
				break;
			case 2:
				_bitCode			= 0x0006;
				_numCodeBits	= 7;
				break;
			case 3:
				_bitCode			= 0x0005;
				_numCodeBits	= 9;
				break;
		}//end switch cbpc...
	}//end if type 1...
	else if(mbt == 2)		// Inter + 4V.
	{
		switch(cbpc)
		{
			case 0:
				_bitCode			= 0x0002;
				_numCodeBits	= 3;
				break;
			case 1:
				_bitCode			= 0x0005;
				_numCodeBits	= 7;
				break;
			case 2:
				_bitCode			= 0x0004;
				_numCodeBits	= 7;
				break;
			case 3:
				_bitCode			= 0x0005;
				_numCodeBits	= 8;
				break;
		}//end switch cbpc...
	}//end if type 2...
	else if(mbt == 3)		// Intra (within a Inter-P picture).
	{
		switch(cbpc)
		{
			case 0:
				_bitCode			= 0x0003;
				_numCodeBits	= 5;
				break;
			case 1:
				_bitCode			= 0x0004;
				_numCodeBits	= 8;
				break;
			case 2:
				_bitCode			= 0x0003;
				_numCodeBits	= 8;
				break;
			case 3:
				_bitCode			= 0x0003;
				_numCodeBits	= 7;
				break;
		}//end switch cbpc...
	}//end if type 3...
	else if(mbt == 4)		// Intra + Q.
	{
		switch(cbpc)
		{
			case 0:
				_bitCode			= 0x0004;
				_numCodeBits	= 6;
				break;
			case 1:
				_bitCode			= 0x0004;
				_numCodeBits	= 9;
				break;
			case 2:
				_bitCode			= 0x0003;
				_numCodeBits	= 9;
				break;
			case 3:
				_bitCode			= 0x0002;
				_numCodeBits	= 9;
				break;
		}//end switch cbpc...
	}//end if type 4...
	else if(mbt == 5)		// Intra + Q + 4V.
	{
		switch(cbpc)
		{
			case 0:
				_bitCode			= 0x0002;
				_numCodeBits	= 11;
				break;
			case 1:
				_bitCode			= 0x000C;
				_numCodeBits	= 13;
				break;
			case 2:
				_bitCode			= 0x000E;
				_numCodeBits	= 13;
				break;
			case 3:
				_bitCode			= 0x000F;
				_numCodeBits	= 13;
				break;
		}//end switch cbpc...
	}//end if type 5...

	return(_numCodeBits);
}// end Encode2.

