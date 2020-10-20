/** @file

MODULE						: FastMotionVectorVlcDecoderImpl2

TAG								: FMVVDI2

FILE NAME					: FastMotionVectorVlcDecoderImpl2.h

DESCRIPTION				: A fast motion vector Vlc decoder implementation with an
										IVlcDecoder Interface.

REVISION HISTORY	:
									: 

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _FASTMOTIONVECTORVLCDECODERIMPL2_H
#define _FASTMOTIONVECTORVLCDECODERIMPL2_H

#pragma once

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastMotionVectorVlcDecoderImpl2 : public IVlcDecoder
{
public:
	FastMotionVectorVlcDecoderImpl2()	{ _numDecodedBits = 0; }
	virtual ~FastMotionVectorVlcDecoderImpl2()	{ }

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numDecodedBits); }
	int Marker(void)						{ return(0); }

	virtual int Decode(IBitStreamReader* bsr); 
	virtual int	Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2);

protected:
	int 	_numDecodedBits;	// Number of decoded bits for for last decode.

	// Constants.
	static const int NUM_ESC_BITS;
	static const int ESC_LENGTH;

};// end class FastMotionVectorVlcDecoderImpl2.

#endif	// _FASTMOTIONVECTORVLCDECODERIMPL2_H
