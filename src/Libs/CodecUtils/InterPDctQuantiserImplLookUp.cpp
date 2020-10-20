/** @file

MODULE						: InterPDctQuantiserImplLookUp

TAG								: IPDQILU

FILE NAME					: InterPDctQuantiserImplLookUp.cpp

DESCRIPTION				: A class to implement a scalar quantiser on the 2-D 
										dct coeffs of an Inter-P coded 8x8 block as defined
										in the H.263 standard. A large look up table is used.
										It implements the IScalarQuantiser()interface.

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

#include "InterPDctQuantiserImplLookUp.h"

/*
---------------------------------------------------------------------------
	Constants.
---------------------------------------------------------------------------
*/
typedef short pqType;

/// Macros for clipping.
#define IPDQILU_CLIPAC(x)		( (((x) <= 2047)&&((x) >= -2048))? (x) : ( ((x) < -2048)? -2048:2047 ) )
#define IPDQILU_CLIP1024(x)	( (((x) <= 1023)&&((x) >= -1024))? (x) : ( ((x) < -1024)? -1024:1023 ) )
#define IPDQILU_CLIP127(x)	( (((x) <= 127)&&((x) >= -127))? (x) : ( ((x) < -127)? -127:127 ) )

/*
---------------------------------------------------------------------------
	Construction and Destruction.
---------------------------------------------------------------------------
*/
InterPDctQuantiserImplLookUp::InterPDctQuantiserImplLookUp(void)
{
	_mode = 0;	///< Default.

	int q, level;
	/// Load mode 0 tables.
	for(q = 1; q <=31; q++)
	{
		for(level = -2048; level <= 2047; level++)
			MODE0_Q[q][level + 2048] = IPDQILU_CLIP127(level/(2 * q));
		for(level = -128; level <= 127; level++)
		{
			if(q & 1)	///< Odd.
			{
				if(level > 0)
					MODE0_IQ[q][level + 128] = IPDQILU_CLIPAC(q * (2*level + 1));
				else if(level < 0)
					MODE0_IQ[q][level + 128] = IPDQILU_CLIPAC(q * (2*level - 1));
				else
					MODE0_IQ[q][level + 128] = 0;
			}//end if q...
			else			///< Even.
			{
				if(level > 0)
					MODE0_IQ[q][level + 128] = IPDQILU_CLIPAC((q * (2*level + 1)) - 1);
				else if(level < 0)
					MODE0_IQ[q][level + 128] = IPDQILU_CLIPAC((q * (2*level - 1)) + 1);
				else
					MODE0_IQ[q][level + 128] = 0;
			}//end else...
		}//end for level...
	}//end for q...

	/// Load mode 1 tables.
	for(q = 1; q <=31; q++)
	{
		for(level = -2048; level <= 2047; level++)
			MODE1_Q[q][level + 2048] = IPDQILU_CLIP1024(level/(2 * q));
		for(level = -1024; level <= 1023; level++)
		{
			if(q & 1)	///< Odd.
			{
				if(level > 0)
					MODE1_IQ[q][level + 1024] = IPDQILU_CLIPAC(q * (2*level + 1));
				else if(level < 0)
					MODE1_IQ[q][level + 1024] = IPDQILU_CLIPAC(q * (2*level - 1));
				else
					MODE1_IQ[q][level + 1024] = 0;
			}//end if q...
			else			///< Even.
			{
				if(level > 0)
					MODE1_IQ[q][level + 1024] = IPDQILU_CLIPAC((q * (2*level + 1)) - 1);
				else if(level < 0)
					MODE1_IQ[q][level + 1024] = IPDQILU_CLIPAC((q * (2*level - 1)) + 1);
				else
					MODE1_IQ[q][level + 1024] = 0;
			}//end else...
		}//end for level...
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
void InterPDctQuantiserImplLookUp::quantise(void* block, int quant)
{
	pqType* b = (pqType *)block;
	pqType* pQ;

	/// DC and AC coeffs. Range [-127..127]. Vlc Table 17 page 44 Recommendation H.263 (02/98). Or
	/// range [-1024..1023] for Modified Quantisation Mode Annex T.4 page 148.
	register int i,j,k,l;

	if(_mode != 0)
		pQ = &(MODE1_Q[quant][2048]);
	else
		pQ = &(MODE0_Q[quant][2048]);

	for(i = 0, j = 16, k = 32, l = 48; i < 16; i++, j++, k++, l++)
	{
		b[i] = pQ[b[i]];
		b[j] = pQ[b[j]];
		b[k] = pQ[b[k]];
		b[l] = pQ[b[l]];
	}//end for i...

	/// Slower mem reads would use the simpler form shown here for mode = 1.
	//for(i = 0; i < 64; i++)
	//	b[i] = MODE1_Q[quant][b[i] + 2048];
}//end quantise.

/** Inverse quantise the block.
Inverse quantise the input block with the quantisation 
parameter supplied.
@param block	:	Block to reconstruct in place.
@param quant	:	Quantisation parameter.
@return				: none.
*/
void InterPDctQuantiserImplLookUp::inverseQuantise(void* block, int quant)
{
	pqType* b = (pqType *)block;
	pqType* pQ;

	register int i,j,k,l;

	if(_mode != 0)
		pQ = &(MODE1_IQ[quant][1024]);
	else
		pQ = &(MODE0_IQ[quant][128]);

	for(i = 0, j = 16, k = 32, l = 48; i < 16; i++, j++, k++, l++)
	{
		b[i] = pQ[b[i]];
		b[j] = pQ[b[j]];
		b[k] = pQ[b[k]];
		b[l] = pQ[b[l]];
	}//end for i...

	/// For slower mem reads use the simpler form shown here for mode = 1.
	//for(i = 0; i < 64; i++)
	//	b[i] = MODE1_IQ[quant][b[i] + 1024];

}//end inverseQuantise.
