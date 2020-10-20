/** @file

MODULE						: MotionVectorH263VlcDecoderImplRev

TAG								: MVH263VDIR

FILE NAME					: MotionVectorH263VlcDecoderImplRev.cpp

DESCRIPTION				: The regularly constructed reversible H.263 motion vector Vlc 
										decoder implementation with an IVlcEncoder Interface. As 
										defined by Recommendation H.263 (02/98) Table D.3 page 56 and 
										section D.2.

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

#include "MotionVectorH263VlcDecoderImplRev.h"

/*
---------------------------------------------------------------------------
	Constructor and Destructor.
---------------------------------------------------------------------------
*/

MotionVectorH263VlcDecoderImplRev::MotionVectorH263VlcDecoderImplRev()
{
	_numCodeBits	= 0;
}//end constructor.

MotionVectorH263VlcDecoderImplRev::~MotionVectorH263VlcDecoderImplRev()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode two motion vector coord symbols from the bit stream.
The 1st symbol is the x coord and the 2nd is the y coord of the vector.
@param bsr			: Bit stream to get from.
@param symbol1	: X coord value.
@param symbol2	: Y coord value.
@return					: Num bits extracted.
*/
int	MotionVectorH263VlcDecoderImplRev::Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2)
{
	*symbol1	= Decode(bsr);
	int xBits = _numCodeBits;
	if(!xBits)
	{
		_numCodeBits = 0;
		return(0);
	}//end if !xBits...

	*symbol2	= Decode(bsr);
	int yBits = _numCodeBits;
	if(!yBits)
	{
		_numCodeBits = 0;
		return(0);
	}//end if !yBits...

	return(xBits + yBits);
}//end Decode2.

/** Decode a symbol from the bit stream.
Extract the next motion vector coord symbol from the input bit stream.
@param bsr			: Bit stream to get from.
@return					: The decoded motion coord.
*/
int MotionVectorH263VlcDecoderImplRev::Decode(IBitStreamReader* bsr)
{
	int coord			= 0;
	_numCodeBits	= 0;

	// Extract the 1st bit and then pairs of bits until the last pair
	// has a '0' in the LSB pos.
	if(bsr->Read())	// 1
	{
		_numCodeBits	= 1;
		coord					= 0;
	}//end if 1...
	else						// 0
	{
		int bitPair		= bsr->Read(2);
		_numCodeBits	= 3;
		coord					= 1;	// This is the implicit MSB.
		while(bitPair & 1)
		{
			coord = (coord << 1) | (bitPair >> 1);
			// Next 2 bits.
			bitPair	= bsr->Read(2);
			_numCodeBits += 2;
		}//end while bitPair...
		if(bitPair & 2)
			coord = -(coord);
	}//end else 0...

  return(coord);
}//end Decode.


