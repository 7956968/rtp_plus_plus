/** @file

MODULE						: MotionVectorH263VlcEncoderImplRev

TAG								: MVH263VEIR

FILE NAME					: MotionVectorH263VlcEncoderImplRev.cpp

DESCRIPTION				: The regularly constructed reversible H.263 motion vector Vlc 
										encoder implementation with an IVlcEncoder Interface. As 
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

#include "MotionVectorH263VlcEncoderImplRev.h"

/*
---------------------------------------------------------------------------
	Construction and Destruction.
---------------------------------------------------------------------------
*/
MotionVectorH263VlcEncoderImplRev::MotionVectorH263VlcEncoderImplRev(void)
{
	_numCodeBits	= 0;
	_bitCode			= 0;
}//end construction.

MotionVectorH263VlcEncoderImplRev::~MotionVectorH263VlcEncoderImplRev(void)
{
}//end destruction.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Encode the param pair as an (x,y) vector.
Treat the symbols as independent single encodes and concatinate
them in the correct order of the _bitCode. Be careful that the 
range of the motion vector does not create a code word longer
than 16 bits for each. Sizeof(int) is only 32 bits.
@param symbol1	: X coord of the motion vector.
@param symbol2	: Y coord of the motion vector.
@return					: Total num of coded bits.
*/
int MotionVectorH263VlcEncoderImplRev::Encode2(int symbol1,int symbol2)
{
  int xbits,ybits;
  int xcode = 0;
  int ycode = 0;

  xbits = Encode(symbol1);
	xcode = _bitCode;
  if(!xbits)
    return(0);

  ybits = Encode(symbol2);
  if(!ybits)
    return(0);

	// X in MSB part of codeword.
	_bitCode = _bitCode | (xcode << ybits);
	_numCodeBits = xbits + ybits;

  return(_numCodeBits);
}//end Encode2.

/** Encode the symbol as a motion vector coord.
The symbol is in half pel units. i.e. 3 = 1.5
@param symbol	: Coord of the motion vector.
@return				: Num of coded bits.
*/
int MotionVectorH263VlcEncoderImplRev::Encode(int symbol)
{
	_numCodeBits	= 0;
	_bitCode			= 0;

	// Most likely value is 0 so provide a fast exit.
	if(symbol == 0)
	{
		_numCodeBits	= 1;
		_bitCode			= 1;
		return(1);
	}//end if symbol is zero...

	// Seperate sign and absolute value.
	int sign = 0;
	int absVal = symbol;
	if(symbol < 0)
	{
		absVal	= -(symbol);
		sign		= 1;
	}//end if negative...

	// Next most likely value is +/- 1 so provide another fast exit.
	if(absVal == 1)
	{
		_numCodeBits	= 3;
		_bitCode			= sign << 1;
		return(3);
	}//end if absVal is one...

	// Do all values greater than 2 by testing against a shifting
	// '1'. Work from LSB up to MSB and keep adding 'x1' pairs until
	// out of range. Note that _numCodeBits also indicates the next
	// bit pos in _bitCode.
	_numCodeBits		= 2;			// Start with the sign bit encoding = s0.
	_bitCode				= sign << 1;
	int val					= absVal;	// Copy of absVal to shift.
	int done				= 0;
	int range				= 2;
	// If absVal still in range then add another 'x1' term.
	while(absVal >= range)
	{
		// Add the const '1'.
		_bitCode = _bitCode | (1 << _numCodeBits);
		_numCodeBits++;
		// Add the bit 'x'.
		_bitCode	= _bitCode | ((val & 1) << _numCodeBits);
		val				= val >> 1; // Next bit shifted into LSB pos.
		_numCodeBits++;

		range	= range << 1;
	}//end while absVal...

	// Close the code with a zero in the MSB pos which is already
	// there by default.
	_numCodeBits++;

	return(_numCodeBits);
}//end Encode.

