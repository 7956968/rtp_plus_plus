/** @file

MODULE						: ModQuantExtEscLevelH263VlcDecoder

TAG								: ILRLH263VD

FILE NAME					: ModQuantExtEscLevelH263VlcDecoder.cpp

DESCRIPTION				: The Modified Quantisation Mode marks an extended Esc range of
										coefficient values by allowing -128 (normally forbidden) to 
										indicate a further extended 11 bits to represent the true
										coefficient value. Defined in H.263 Recommendation (02/98) 
										Annex T.4 page 148. This is only used after a normal VlcDecoder
										call where the level for that call is -128 and is implemented 
										with an	IVlcDecoder Interface.

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

#include "ModQuantExtEscLevelH263VlcDecoder.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
#define MQEELH263VD_NUM_EXT_LEVEL_BITS		11

ModQuantExtEscLevelH263VlcDecoder::ModQuantExtEscLevelH263VlcDecoder()
{
	_numCodeBits	= 0;
}//end constructor.

ModQuantExtEscLevelH263VlcDecoder::~ModQuantExtEscLevelH263VlcDecoder()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode an extended level symbol from the bit stream.
The 11 bits of the symbol are cyclically rotated to the right by 5 bits
to avoid start code emulation. The _numCodeBits member is set accordingly.
@param bsr			: Bit stream to get from.
@return					: Decoded value.
*/
int	ModQuantExtEscLevelH263VlcDecoder::Decode(IBitStreamReader* bsr)
{
	int level			= 0;
	int x					= bsr->Read(MQEELH263VD_NUM_EXT_LEVEL_BITS);
	_numCodeBits	= MQEELH263VD_NUM_EXT_LEVEL_BITS;

	int a = (x >> 6) & 0x0000001F;
	int b = (x & 0x0000003F) << 5;
	level = a | b;
	int sign = x & 0x0020;
	if(sign)
		level = level | 0xFFFFF800;

  return(level);
}//end Decode.

