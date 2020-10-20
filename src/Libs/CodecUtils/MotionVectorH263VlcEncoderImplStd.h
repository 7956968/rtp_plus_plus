/** @file

MODULE						: MotionVectorH263VlcEncoderImplStd

TAG								: MVH263VEIS

FILE NAME					: MotionVectorH263VlcEncoderImplStd.h

DESCRIPTION				: The base level H.263 motion vector Vlc encoder implementation 
										with an	IVlcEncoder Interface. As defined by Recommendation 
										H.263 (02/98) Table 14 page 39 and section 5.3.7.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONVECTORH263VLCENCODERIMPLSTD_H
#define _MOTIONVECTORH263VLCENCODERIMPLSTD_H

#pragma once

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define MVH263VEIS_TABLE_SIZE 64
#define MVH263VEIS_NUM_BITS		0
#define MVH263VEIS_BIT_CODE		1

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionVectorH263VlcEncoderImplStd : public IVlcEncoder
{
public:
	MotionVectorH263VlcEncoderImplStd(void);
	virtual ~MotionVectorH263VlcEncoderImplStd(void);

public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }

	virtual int Encode(int symbol); 

	// Optional interface implementation.
	virtual int Encode2(int symbol1,int symbol2);

protected:
	int 	_numCodeBits;	// Number of coded bits for this motion vector.
	int 	_bitCode;			// The code of length numCodeBits.

	// Constants.
	static const int VLC_TABLE[MVH263VEIS_TABLE_SIZE][2];
//	static const int INDEX_TABLE[MVH263VEIS_TABLE_SIZE];

};// end class MotionVectorH263VlcEncoderImplStd.

#endif	// _MOTIONVECTORH263VLCENCODERIMPLSTD_H
