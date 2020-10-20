/** @file

MODULE						: FastRunLengthVlcDecoderImpl1

TAG								: FRLVDI1

FILE NAME					: FastRunLengthVlcDecoderImpl1.h

DESCRIPTION				: A fast run length Vlc decoder implementation with an
										IVlcDecoder Interface and derived from RunLengthVlcDecoder.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _FASTRUNLENGTHVLCDECODERIMPL1_H
#define _FASTRUNLENGTHVLCDECODERIMPL1_H

#include "RunLengthVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class FastRunLengthVlcDecoderImpl1 : public RunLengthVlcDecoder
{
public:
	FastRunLengthVlcDecoderImpl1();
	virtual ~FastRunLengthVlcDecoderImpl1();

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

};// end class FastRunLengthVlcDecoderImpl1.

#endif	// _FASTRUNLENGTHVLCDECODERIMPL1_H
