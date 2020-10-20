/** @file

MODULE						: ModQuantExtEscLevelH263VlcEncoder

TAG								: ILRLH263VE

FILE NAME					: ModQuantExtEscLevelH263VlcEncoder.cpp

DESCRIPTION				: The Modified Quantisation Mode marks an extended Esc range of
										coefficient values by allowing -128 (normally forbidden) to 
										indicate a further extended 11 bits to represent the true
										coefficient value. Defined in H.263 Recommendation (02/98) 
										Annex T.4 page 148. This is only used after a normal VlcEncoder
										call where the level for that call is -128 and is implemented 
										with an	IVlcEncoder Interface.

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

#include "ModQuantExtEscLevelH263VlcEncoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define MQEELH263VE_NUM_EXT_LEVEL_BITS		11

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
ModQuantExtEscLevelH263VlcEncoder::ModQuantExtEscLevelH263VlcEncoder(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end constructor.

ModQuantExtEscLevelH263VlcEncoder::~ModQuantExtEscLevelH263VlcEncoder(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Encode an extended level symbol.
The 11 bits of the symbol are cyclically rotated to the right by 5 bits
to avoid start code emulation. The _numCodeBits and _bitCode members are 
set accordingly.
@param symbol	: Level value.
@return				: Num of bits in the codeword.
*/
int	ModQuantExtEscLevelH263VlcEncoder::Encode(int symbol)
{
	_numCodeBits	= MQEELH263VE_NUM_EXT_LEVEL_BITS;
	_bitCode			= 0;

	int x	= symbol;
	int a = ((x & 0x0000003F) << 6);
	int b = (x >> 5) & 0x0000003F;
	_bitCode = a | b;

	return(_numCodeBits);
}// end Encode.


/*
---------------------------------------------------------------------------
	Private Methods.
---------------------------------------------------------------------------
*/

