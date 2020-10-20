/** @file

MODULE						: IntraDctQuantiserImpl

TAG								: IDQI

FILE NAME					: IntraDctQuantiserImpl.cpp

DESCRIPTION				: A class to implement a scalar quantiser on the 2-D 
										dct coeffs of an Intra coded 8x8 block as defined
										in the H.263 standard. It implements the 
										IScalarQuantiser interface.

REVISION HISTORY	:
									: 

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

#include "IntraDctQuantiserImpl.h"

typedef short qType;

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
// Macros for clipping.
#define IDQI_CLIPAC(x)			( (((x) <= 2047)&&((x) >= -2048))? (x) : ( ((x) < -2048)? -2048:2047 ) )
#define IDQI_CLIPAC1024(x)	( (((x) <= 1023)&&((x) >= -1024))? (x) : ( ((x) < -1024)? -1024:1023 ) )
#define IDQI_CLIPDC(x)			( (((x) <= 2047)&&((x) >= 0))? (x) : ( ((x) < 0)? 0:2047 ) )
#define IDQI_CLIP127(x)			( (((x) <= 127)&&((x) >= -127))? (x) : ( ((x) < -127)? -127:127 ) )

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
void IntraDctQuantiserImpl::quantise(void* block, int quant)
{
	qType* b = (qType *)block;

	/// IntraDC quantiser is fixed.
	b[0] = b[0]/8;
	/// Quantised IntraDC range [1..255]. Vlc Table 14 page 41 Recommendation H.263 (02/98).
	if(b[0] < 1)
		b[0] = 1;
	else if(b[0] > 255)
		b[0] = 255;

	/// AC coeffs. Range [-127..127]. Vlc Table 17 page 44 Recommendation H.263 (02/98). Or
	/// range [-1024..1023] for Modified Quantisation Mode Annex T.4 page 148.
	int q = 2 * quant;
	for(int i = 1; i < 64; i++)
	{
		b[i] = b[i]/q;
		// Clipping.
		if(_mode == 0)
			b[i] = IDQI_CLIP127(b[i]);
		else
			b[i] = IDQI_CLIPAC1024(b[i]);
	}//end for i...

}//end quantise.

/** Inverse quantise the block.
Inverse quantise the input block with the quantisation 
parameter supplied.
@param block	:	Block to reconstruct in place.
@param quant	:	Quantisation parameter.
@return				: none.
*/
void IntraDctQuantiserImpl::inverseQuantise(void* block, int quant)
{
	qType* b = (qType *)block;

	/// IntraDC quantiser is fixed.
	b[0] = b[0] * 8;
	b[0] = IDQI_CLIPDC(b[0]);

	if(quant & 1)	///< Odd.
	{
		for(int i = 1; i < 64; i++)
		{
			/// A zero value is left alone.
			if(b[i] > 0)
				b[i] = quant * (2*b[i] + 1);
			else if(b[i] < 0)
				b[i] = quant * (2*b[i] - 1);
			/// Clipping.
			b[i] = IDQI_CLIPAC(b[i]);
		}//end for i...
	}//end if quant...
	else					///< Even.
	{
		for(int i = 1; i < 64; i++)
		{
			/// A zero value is left alone.
			if(b[i] > 0)
				b[i] = (quant * (2*b[i] + 1)) - 1;
			else if(b[i] < 0)
				b[i] = (quant * (2*b[i] - 1)) + 1;
			/// Clipping.
			b[i] = IDQI_CLIPAC(b[i]);
		}//end for i...
	}//end else...
}//end inverseQuantise.
