/** @file

MODULE						: AdvancedIntraDctQuantiserImplLookUp

TAG								: IPDQILU

FILE NAME					: AdvancedIntraDctQuantiserImplLookUp.cpp

DESCRIPTION				: A class to implement a scalar quantiser on the 2-D 
										dct coeffs of an Advanced Intra coded 8x8 block as defined
										in the H.263 standard. A large look up table is used.
										The class extends the AdvancedIntraDctQuantiserImpl()
										base class.

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

#include "AdvancedIntraDctQuantiserImplLookUp.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
typedef short pqType;

/// Macros for oddification and clipping.
#define AIDQILU_ODDIFICATION(x)	( ((x) & 1)? (x) : ((x)+1) )
#define AIDQILU_CLIPAC(x)				( (((x) <= 2047)&&((x) >= -2048))? (x) : ( ((x) < -2048)? -2048:2047 ) )
#define AIDQILU_CLIP1024(x)			( (((x) <= 1023)&&((x) >= -1024))? (x) : ( ((x) < -1024)? -1024:1023 ) )
#define AIDQILU_CLIPDC(x)				( (((x) <= 2047)&&((x) >= 0))? (x) : ( ((x) < 0)? 0:2047 ) )
#define AIDQILU_CLIP127(x)			( (((x) <= 127)&&((x) >= -127))? (x) : ( ((x) < -127)? -127:127 ) )

/*
---------------------------------------------------------------------------
	Construction and Destruction.
---------------------------------------------------------------------------
*/
AdvancedIntraDctQuantiserImplLookUp::AdvancedIntraDctQuantiserImplLookUp(void)
{
	_mode = 0;	///< Default.

	int q, level;
	/// Load mode 0 tables.
	for(q = 1; q <=31; q++)
	{
		for(level = -2048; level <= 2047; level++)
			MODE0_Q[q][level + 2048] = AIDQILU_CLIP127(level/(2 * q));
		for(level = -128; level <= 127; level++)
			MODE0_IQ[q][level + 128] = AIDQILU_CLIPAC(2 * q * level);
	}//end for q...

	/// Load mode 1 tables.
	for(q = 1; q <=31; q++)
	{
		for(level = -2048; level <= 2047; level++)
			MODE1_Q[q][level + 2048] = AIDQILU_CLIP1024(level/(2 * q));
		for(level = -1024; level <= 1023; level++)
			MODE1_IQ[q][level + 1024] = AIDQILU_CLIPAC(2 * q * level);
	}//end for q...

}//end constructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Quantise the input block.
Quantise the input block with the quantisation parameter 
supplied.
@param block	:	Block to quantise in place.
@param quant	:	Quantisation parameter.
@return				: none.
*/
void AdvancedIntraDctQuantiserImplLookUp::quantise(void* block, int quant)
{
	pqType* b = (pqType *)block;

	/// DC and AC coeffs. Range [-127..127]. Vlc Table 17 page 44 Recommendation H.263 (02/98). Or
	/// range [-1024..1023] for Modified Quantisation Mode Annex T.4 page 148.
	int i;

	if(_mode == 0)
	{
		for(i = 0; i < 64; i++)
			if(b[i])
				b[i] = MODE0_Q[quant][b[i] + 2048];
	}//end if _mode...
	else
	{
		for(i = 0; i < 64; i++)
			if(b[i])
				b[i] = MODE1_Q[quant][b[i] + 2048];
	}//end else...

}//end quantise.

/** Inverse quantise the block.
Inverse quantise the input block with the quantisation 
parameter supplied.
@param block	:	Block to reconstruct in place.
@param quant	:	Quantisation parameter.
@return				: none.
*/
void AdvancedIntraDctQuantiserImplLookUp::inverseQuantise(void* block, int quant)
{
	pqType* b = (pqType *)block;

	int i;

	/// Oddification & DC clipping.
	b[0] = AIDQILU_CLIPDC(AIDQILU_ODDIFICATION(2 * quant * b[0]));

	if(_mode == 0)
	{
		for(i = 1; i < 64; i++)
			if(b[i])
				b[i] = MODE0_IQ[quant][b[i] + 128];
	}//end if _mode...
	else
	{
		for(i = 1; i < 64; i++)
			if(b[i])
				b[i] = MODE1_IQ[quant][b[i] + 1024];
	}//end else...
}//end inverseQuantise.
