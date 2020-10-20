/** @file

MODULE						: AdvancedIntraDctQuantiserImpl

TAG								: AIDQI

FILE NAME					: AdvancedIntraDctQuantiserImpl.cpp

DESCRIPTION				: A class to implement a scalar quantiser on the 2-D 
										dct coeffs of an Advanced Intra coded 8x8 block as 
										defined	in the H.263 standard. It implements the 
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

#include "AdvancedIntraDctQuantiserImpl.h"

/// Macros for oddification and clipping.
#define AIDQI_ODDIFICATION(x)	( ((x) & 1)? (x) : ((x)+1) )
#define AIDQI_CLIPAC(x)				( (((x) <= 2047)&&((x) >= -2048))? (x) : ( ((x) < -2048)? -2048:2047 ) )
#define AIDQI_CLIP1024(x)			( (((x) <= 1023)&&((x) >= -1024))? (x) : ( ((x) < -1024)? -1024:1023 ) )
#define AIDQI_CLIPDC(x)				( (((x) <= 2047)&&((x) >= 0))? (x) : ( ((x) < 0)? 0:2047 ) )
#define AIDQI_CLIP127(x)				( (((x) <= 127)&&((x) >= -127))? (x) : ( ((x) < -127)? -127:127 ) )

/*
---------------------------------------------------------------------------
	Construction and Destruction.
---------------------------------------------------------------------------
*/
AdvancedIntraDctQuantiserImpl::AdvancedIntraDctQuantiserImpl(void)
{
	_mode = 0;
}//end constructor.

AdvancedIntraDctQuantiserImpl::~AdvancedIntraDctQuantiserImpl(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Quantise the input block.
Quantise the input block with the quantisation parameter 
supplied. All coeffs are treated equally for the forward
quant process regardless of prediction.
@param block	:	Block to quantise in place.
@param quant	:	Quantisation parameter.
@return				: none.
*/
void AdvancedIntraDctQuantiserImpl::quantise(void* block, int quant)
{
	pqType* b = (pqType *)block;

	/// DC and AC coeffs. Range [-127..127]. Vlc Table 17 page 44 
	/// Recommendation H.263 (02/98).
	int q = 2 * quant;
	for(int i = 0; i < 64; i++)
	{
		b[i] = b[i]/q;
		/// Clipping.
		if( _mode == 0)
			b[i] = AIDQI_CLIP127(b[i]);
		else
			b[i] = AIDQI_CLIP1024(b[i]);

	}//end for i...

}//end quantise.

/** Inverse quantise the block.
Inverse quantise the input block with the quantisation 
parameter supplied. The mode specifies which are differential
coeffs and are not to be clipped in this class. It is the
responsibility of the calling procedures to ensure the clipping
is done before the IDCT.
@param block	:	Block to reconstruct in place.
@param quant	:	Quantisation parameter.
@return				: none.
*/
void AdvancedIntraDctQuantiserImpl::inverseQuantise(void* block, int quant)
{
	int			i;
	int			q	= 2 * quant;
	pqType* b = (pqType *)block;

	b[0] = q * b[0];
	// Oddification.
	b[0] = AIDQI_ODDIFICATION(b[0]);
	// Clipping DC.
	b[0] = AIDQI_CLIPDC(b[0]);

	for(i = 1; i < 64; i++)
	{
		b[i] = q * b[i];
		// Clipping AC.
		b[i] = AIDQI_CLIPAC(b[i]);
	}//end for i...

}//end inverseQuantise.
