/** @file

MODULE						: MotionVectorH263VlcDecoderImplStd

TAG								: MVH263VDIS

FILE NAME					: MotionVectorH263VlcDecoderImplStd.h

DESCRIPTION				: The base level H.263 motion vector Vlc decoder implementation 
										with an	IVlcDecoder Interface. As defined by Recommendation 
										H.263 (02/98) Table 14 page 39 and section 5.3.7.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONVECTORH263VLCDECODERIMPLSTD_H
#define _MOTIONVECTORH263VLCDECODERIMPLSTD_H

#include "IVlcDecoder.h"
#include "DualMotionVectorTypeStruct.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define MVH263VDIS_TABLE_LENGTH 64

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionVectorH263VlcDecoderImplStd : public IVlcDecoder
{
public:
	MotionVectorH263VlcDecoderImplStd();
	virtual ~MotionVectorH263VlcDecoderImplStd();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(0); }	// No markers for this decoder.
	// Single symbols are not relevant for a dual possible motion vector decoder.
	virtual int Decode(IBitStreamReader* bsr) { _numCodeBits = 0; return(0); } 

	// Optional interface implementation.
	virtual int	Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2);

protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

	static const DualMotionVectorType VLC_TABLE[MVH263VDIS_TABLE_LENGTH];

};// end class MotionVectorH263VlcDecoderImplStd.

#endif	// _MOTIONVECTORH263VLCDECODERIMPLSTD_H
