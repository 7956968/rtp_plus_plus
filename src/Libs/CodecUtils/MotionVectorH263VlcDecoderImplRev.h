/** @file

MODULE						: MotionVectorH263VlcDecoderImplRev

TAG								: MVH263VDIR

FILE NAME					: MotionVectorH263VlcDecoderImplRev.h

DESCRIPTION				: The regularly constructed reversible H.263 motion vector Vlc 
										decoder implementation with an IVlcEncoder Interface. As 
										defined by Recommendation H.263 (02/98) Table D.3 page 56 and 
										section D.2.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONVECTORH263VLCDECODERIMPLREV_H
#define _MOTIONVECTORH263VLCDECODERIMPLREV_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionVectorH263VlcDecoderImplRev : public IVlcDecoder
{
public:
	MotionVectorH263VlcDecoderImplRev();
	virtual ~MotionVectorH263VlcDecoderImplRev();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(0); }	// No markers for this decoder.
	virtual int Decode(IBitStreamReader* bsr); 

	// Optional interface implementation.
	virtual int	Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2);

protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

};// end class MotionVectorH263VlcDecoderImplRev.

#endif	// _MOTIONVECTORH263VLCDECODERIMPLREV_H
