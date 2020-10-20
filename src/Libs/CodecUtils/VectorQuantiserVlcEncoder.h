/** @file

MODULE						: VectorQuantiserVlcEncoder

TAG								: VQVE

FILE NAME					: VectorQuantiserVlcEncoder.h

DESCRIPTION				: A class to implement a vector quantiser encoder where the
										table fully contains all indeces. There is no Esc coding
										requirements. It implements the IVlcEncoder interface.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _VECTORQUANTISERVLCENCODER_H
#define _VECTORQUANTISERVLCENCODER_H

#pragma once

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define VQVE_TABLE_SIZE 256
#define VQVE_NUM_BITS		0
#define VQVE_BIT_CODE		1

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class VectorQuantiserVlcEncoder : public IVlcEncoder
{
public:
	VectorQuantiserVlcEncoder()	{_numCodeBits = 0;_bitCode = 0;}
	virtual ~VectorQuantiserVlcEncoder()	{ }

public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }

	virtual int Encode(int symbol) 
		{_numCodeBits=DoEncode(symbol,&_bitCode);return(_numCodeBits);}

protected:

	static int DoEncode(int index, int* codeWord);

protected:
	int 	_numCodeBits;	// Number of coded bits for this motion vector.
	int 	_bitCode;			// The code of length numCodeBits.

	// Constants.
	static const int VLC_TABLE[VQVE_TABLE_SIZE][2];

};// end class VectorQuantiserVlcEncoder.

#endif	//_VECTORQUANTISERVLCENCODER_H
