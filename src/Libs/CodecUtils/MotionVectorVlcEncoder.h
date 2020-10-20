/** @file

MODULE						: MotionVectorVlcEncoder

TAG								: MVVE

FILE NAME					: MotionVectorVlcEncoder.h

DESCRIPTION				: A motion vector Vlc encoder implementation with an
										IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONVECTORVLCENCODER_H
#define _MOTIONVECTORVLCENCODER_H

#pragma once

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define MVVE_TABLE_SIZE 66
#define MVVE_NUM_BITS		0
#define MVVE_BIT_CODE		1

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionVectorVlcEncoder : public IVlcEncoder
{
public:
	MotionVectorVlcEncoder()	{_numCodeBits = 0;_bitCode = 0;}
	virtual ~MotionVectorVlcEncoder()	{ }

public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }

	virtual int Encode(int symbol) 
		{_numCodeBits=DoEncode(symbol,&_bitCode);return(_numCodeBits);}

	// Optional interface implementation.
	virtual int Encode2(int symbol1,int symbol2)
		{_numCodeBits=DoEncode2(symbol1,symbol2,&_bitCode);return(_numCodeBits);}

public:
	// Operator overloads.
	MotionVectorVlcEncoder operator=(MotionVectorVlcEncoder& mvve)	
		{_numCodeBits=mvve._numCodeBits;_bitCode=mvve._bitCode;return(*this);}

protected:

	static int DoEncode(int coord, int* codeWord);
	static int DoEncode2(int coord1, int coord2, int *codeWord);

protected:
	int 	_numCodeBits;	// Number of coded bits for this motion vector.
	int 	_bitCode;			// The code of length numCodeBits.

	// Constants.
	static const int NUM_ESC_BITS;
	static const int ESC_BIT_CODE;
	static const int ESC_LENGTH;

	static const int VLC_TABLE[MVVE_TABLE_SIZE][2];

};// end class MotionVectorVlcEncoder.

#endif
