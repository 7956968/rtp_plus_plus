/** @file

MODULE						: FastVectorQuantiserVlcDecoderImpl2

TAG								: FVQVDI2

FILE NAME					: FastVectorQuantiserVlcDecoderImpl2.h

DESCRIPTION				: A class to implement a fast vector quantiser decoder where 
										the	binary table fully contains all indeces. The table is
										inline with the decode function. There is no Esc coding 
										requirements. It implements the IVlcDecoder interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _FASTVECTORQUANTISERVLCDECODERIMPL2_H
#define _FASTVECTORQUANTISERVLCDECODERIMPL2_H

#pragma once

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastVectorQuantiserVlcDecoderImpl2 : public IVlcDecoder
{
public:
	FastVectorQuantiserVlcDecoderImpl2()	{ _numDecodedBits = 0; }
	virtual ~FastVectorQuantiserVlcDecoderImpl2()	{ }

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numDecodedBits); }
	int Marker(void)						{ return(0); }

	virtual int Decode(IBitStreamReader* bsr); 

protected:
	int 	_numDecodedBits;	// Number of decoded bits for this decode.

};// end class FastVectorQuantiserVlcDecoderImpl2.

#endif	//_FASTVECTORQUANTISERVLCDECODERIMPL2_H
