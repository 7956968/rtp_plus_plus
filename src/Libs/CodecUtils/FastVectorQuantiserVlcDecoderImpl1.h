/** @file

MODULE						: FastVectorQuantiserVlcDecoderImpl1

TAG								: FVQVDI1

FILE NAME					: FastVectorQuantiserVlcDecoderImpl1.h

DESCRIPTION				: A fast vector quantiser Vlc decoder implementation with an
										IVlcDecoder Interface and derived from VectorQuantiserVlcDecoder.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _FASTVECTORQUANTISERVLCDECODERIMPL1_H
#define _FASTVECTORQUANTISERVLCDECODERIMPL1_H

#include "VectorQuantiserVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastVectorQuantiserVlcDecoderImpl1 : public VectorQuantiserVlcDecoder
{
public:
	FastVectorQuantiserVlcDecoderImpl1();
	virtual ~FastVectorQuantiserVlcDecoderImpl1();

public:
	// Interface implementation.
	virtual int Decode(IBitStreamReader* bsr); 

protected:
	// Private utilities.
	int Create(void);
	void Destroy(void);

protected:
	// Fast binary tree arrays.
	int*	_onesTree;
	int*  _zeroTree;
	int*	_indexTree;

};// end class FastVectorQuantiserVlcDecoderImpl1.

#endif	// _FASTVECTORQUANTISERVLCDECODERIMPL1_H
