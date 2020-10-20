/** @file

MODULE						: RunLengthVlcDecoder

TAG								: RLVD

FILE NAME					: RunLengthVlcDecoder.h

DESCRIPTION				: A run length Vlc decoder implementation with an
										IVlcDecoder Interface.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2006  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _RUNLENGTHVLCDECODER_H
#define _RUNLENGTHVLCDECODER_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define RLVD_TABLE_SIZE 32
#define RLVD_NUM_BITS		0
#define RLVD_BIT_CODE		1

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class RunLengthVlcDecoder : public IVlcDecoder
{
public:
	RunLengthVlcDecoder();
	virtual ~RunLengthVlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(_marker); }

	virtual int Decode(IBitStreamReader* bsr); 

	// Optional interface implementation.
	void SetEsc(int numEscBits,int escMask)
		{ _numEscBits = numEscBits; _escMask = escMask;}

public:
	static const int EOP_MARKER;
	static const int EOI_MARKER;

protected:
	int 	_numCodeBits;	// Number of coded bits for this symbol.
	int		_marker;
	int		_numEscBits;	// Number of bits that follow the Esc code.
	int		_escMask;			// Bit mask of numEscBits.

	// Constants.
	static const int NUM_ESC_CODE_BITS;
	static const int ESC_BIT_CODE;

	static const int VLC_TABLE[RLVD_TABLE_SIZE + 3][2];

};// end class RunLengthVlcDecoder.

#endif	// _RUNLENGTHVLCDECODER_H
