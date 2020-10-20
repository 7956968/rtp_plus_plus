/** @file

MODULE						: MotionVectorVlcDecoder

TAG								: MVVD

FILE NAME					: MotionVectorVlcDecoder.h

DESCRIPTION				: A motion vector Vlc decoder implementation with an
										IVlcDecoder Interface.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _MOTIONVECTORVLCDECODER_H
#define _MOTIONVECTORVLCDECODER_H

#pragma once

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define MVVD_TABLE_SIZE 66
#define MVVD_NUM_BITS		0
#define MVVD_BIT_CODE		1

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionVectorVlcDecoder : public IVlcDecoder
{
public:
	MotionVectorVlcDecoder()	{ _numDecodedBits = 0; }
	virtual ~MotionVectorVlcDecoder()	{ }

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numDecodedBits); }
	int Marker(void)						{ return(0); }

	virtual int Decode(IBitStreamReader* bsr); 
	virtual int	Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2);

protected:
	int 	_numDecodedBits;	// Number of decoded bits for for last decode.

	// Constants.
	static const int NUM_ESC_BITS;
	static const int ESC_BIT_CODE;
	static const int ESC_LENGTH;

	static const int VLC_TABLE[MVVD_TABLE_SIZE][2];

};// end class MotionVectorVlcDecoder.

#endif	// _MOTIONVECTORVLCDECODER_H
