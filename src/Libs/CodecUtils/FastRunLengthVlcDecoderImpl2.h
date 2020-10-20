/** @file

MODULE						: FastRunLengthVlcDecoderImpl2

TAG								: FRLVDI2

FILE NAME					: FastRunLengthVlcDecoderImpl2.h

DESCRIPTION				: A fast run length Vlc decoder implementation with an
										IVlcDecoder Interface. The table is inline with
										the decode method.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _FASTRUNLENGTHVLCDECODERIMPL2_H
#define _FASTRUNLENGTHVLCDECODERIMPL2_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastRunLengthVlcDecoderImpl2 : public IVlcDecoder
{
public:
	FastRunLengthVlcDecoderImpl2();
	virtual ~FastRunLengthVlcDecoderImpl2();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numDecodedBits); }
	int Marker(void)						{ return(_marker); }

	virtual int Decode(IBitStreamReader* bsr); 

	// Optional interface implementation.
	void SetEsc(int numEscBits,int escMask)
		{ _numEscBits = numEscBits; _escMask = escMask;}

public:
	static const int EOP_MARKER;
	static const int EOI_MARKER;

protected:
	int 	_numDecodedBits;	// Number of decoded bits for this symbol.
	int		_marker;
	int		_numEscBits;			// Number of bits that follow the Esc code.
	int		_escMask;					// Bit mask of numEscBits.

	// Constants.
	static const int NUM_ESC_CODE_BITS;

};// end class FastRunLengthVlcDecoderImpl2.

#endif	// _FASTRUNLENGTHVLCDECODERIMPL2_H
