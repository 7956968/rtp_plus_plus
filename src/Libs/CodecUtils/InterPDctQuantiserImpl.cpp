/** @file

MODULE						: InterPDctQuantiserImpl

TAG								: IPDQI

FILE NAME					: InterPDctQuantiserImpl.cpp

DESCRIPTION				: A class to implement a scalar quantiser on the 2-D 
										dct coeffs of an Inter-P coded 8x8 block as defined
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

#include "InterPDctQuantiserImpl.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
typedef short pqType;

/// Macros for clipping.
#define IPDQI_CLIPAC(x)		( (((x) <= 2047)&&((x) >= -2048))? (x) : ( ((x) < -2048)? -2048:2047 ) )
#define IPDQI_CLIP1024(x)	( (((x) <= 1023)&&((x) >= -1024))? (x) : ( ((x) < -1024)? -1024:1023 ) )
#define IPDQI_CLIP127(x)	( (((x) <= 127)&&((x) >= -127))? (x) : ( ((x) < -127)? -127:127 ) )

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
void InterPDctQuantiserImpl::quantise(void* block, int quant)
{
	pqType* b = (pqType *)block;

	/// DC and AC coeffs. Range [-127..127]. Vlc Table 17 page 44 Recommendation H.263 (02/98). Or
	/// range [-1024..1023] for Modified Quantisation Mode Annex T.4 page 148.
	int q = 2 * quant;
	for(int i = 0; i < 64; i++)
	{
		/// Clipping.
		if(_mode == 0)
			b[i] = IPDQI_CLIP127(b[i]/q);
		else
			b[i] = IPDQI_CLIP1024(b[i]/q);

	}//end for i...

}//end quantise.

/** Inverse quantise the block.
Inverse quantise the input block with the quantisation 
parameter supplied.
@param block	:	Block to reconstruct in place.
@param quant	:	Quantisation parameter.
@return				: none.
*/
void InterPDctQuantiserImpl::inverseQuantise(void* block, int quant)
{
	pqType* b = (pqType *)block;

	if(quant & 1)	///< Odd.
	{
		for(int i = 0; i < 64; i++)
		{
			/// A zero value is left alone.
			if(b[i] > 0)
				b[i] = quant * (2*b[i] + 1);
			else if(b[i] < 0)
				b[i] = quant * (2*b[i] - 1);
			/// Clipping.
			b[i] = IPDQI_CLIPAC(b[i]);
		}//end for i...
	}//end if quant...
	else					///< Even.
	{
		for(int i = 0; i < 64; i++)
		{
			/// A zero value is left alone.
			if(b[i] > 0)
				b[i] = (quant * (2*b[i] + 1)) - 1;
			else if(b[i] < 0)
				b[i] = (quant * (2*b[i] - 1)) + 1;
			/// Clipping.
			b[i] = IPDQI_CLIPAC(b[i]);
		}//end for i...
	}//end else...
}//end inverseQuantise.
