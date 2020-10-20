/** @file

MODULE						: VectorQuantiserVlcDecoder

TAG								: VQVD

FILE NAME					: VectorQuantiserVlcDecoder.h

DESCRIPTION				: A class to implement a vector quantiser decoder where the
										table fully contains all indeces. There is no Esc coding
										requirements. It implements the IVlcDecoder interface.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _VECTORQUANTISERVLCDECODER_H
#define _VECTORQUANTISERVLCDECODER_H

#pragma once

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define VQVD_TABLE_SIZE 256
#define VQVD_NUM_BITS		0
#define VQVD_BIT_CODE		1

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class VectorQuantiserVlcDecoder : public IVlcDecoder
{
public:
	VectorQuantiserVlcDecoder()	{ _numCodeBits = 0; }
	virtual ~VectorQuantiserVlcDecoder()	{ }

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(0); }

	virtual int Decode(IBitStreamReader* bsr); 

protected:
	int 	_numCodeBits;	// Number of coded bits for this motion vector.

	// Constants.
	static const int VLC_TABLE[VQVD_TABLE_SIZE][2];

};// end class VectorQuantiserVlcDecoder.

#endif	//_VECTORQUANTISERVLCDECODER_H
