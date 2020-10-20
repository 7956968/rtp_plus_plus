/** @file

MODULE						: MotionVectorH263VlcEncoderImplRev

TAG								: MVH263VEIR

FILE NAME					: MotionVectorH263VlcEncoderImplRev.h

DESCRIPTION				: The regularly constructed reversible H.263 motion vector Vlc 
										encoder implementation with an IVlcEncoder Interface. As 
										defined by Recommendation H.263 (02/98) Table D.3 page 56 and 
										section D.2.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONVECTORH263VLCENCODERIMPLREV_H
#define _MOTIONVECTORH263VLCENCODERIMPLREV_H

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
class MotionVectorH263VlcEncoderImplRev : public IVlcEncoder
{
public:
	MotionVectorH263VlcEncoderImplRev(void);
	virtual ~MotionVectorH263VlcEncoderImplRev(void);

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

};// end class MotionVectorH263VlcEncoderImplRev.

#endif	// _MOTIONVECTORH263VLCENCODERIMPLREV_H
