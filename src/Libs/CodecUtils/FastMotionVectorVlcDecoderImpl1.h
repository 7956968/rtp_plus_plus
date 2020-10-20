/** @file

MODULE						: FastMotionVectorVlcDecoderImpl1

TAG								: FMVVDI1

FILE NAME					: FastMotionVectorVlcDecoderImpl1.h

DESCRIPTION				: A fast motion vector Vlc decoder implementation with an
										IVlcDecoder Interface and derived from MotionVectorVlcDecoder.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _FASTMOTIONVECTORVLCDECODERIMPL1_H
#define _FASTMOTIONVECTORVLCDECODERIMPL1_H

#include "MotionVectorVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastMotionVectorVlcDecoderImpl1 : public MotionVectorVlcDecoder
{
public:
	FastMotionVectorVlcDecoderImpl1();
	virtual ~FastMotionVectorVlcDecoderImpl1();

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

};// end class FastMotionVectorVlcDecoderImpl1.

#endif	// _FASTMOTIONVECTORVLCDECODERIMPL1_H
