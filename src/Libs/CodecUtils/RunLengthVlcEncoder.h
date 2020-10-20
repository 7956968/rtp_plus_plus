/** @file

MODULE						: RunLengthVlcEncoder

TAG								: RLVE

FILE NAME					: RunLengthVlcEncoder.h

DESCRIPTION				: A run length Vlc encoder implementation with an
										IVlcEncoder Interface.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)VICS 2000-2005  all rights resevered - info@videocoding.com

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
===========================================================================
*/
#ifndef _RUNLENGTHVLCENCODER_H
#define _RUNLENGTHVLCENCODER_H

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define RLVE_TABLE_SIZE 32
#define RLVE_NUM_BITS		0
#define RLVE_BIT_CODE		1

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class RunLengthVlcEncoder : public IVlcEncoder
{
public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }

	virtual int Encode(int symbol) 
		{_numCodeBits=DoEncode(symbol,_numEscBits,_escMask,&_bitCode);return(_numCodeBits);}

	// Optional interface implementation.
	void SetEsc(int numEscBits,int escMask)
		{_numEscBits=numEscBits;_escMask=escMask;}

public:
	// Operator overloads.
	RunLengthVlcEncoder operator=(RunLengthVlcEncoder& rlve)	
		{_numCodeBits=rlve._numCodeBits;_bitCode=rlve._bitCode;_numEscBits=rlve._numEscBits;_escMask=rlve._escMask;return(*this);}

public:
	RunLengthVlcEncoder()	{_numCodeBits=0;_bitCode=0;_numEscBits=8;_escMask=255;}
	virtual ~RunLengthVlcEncoder()	{ }

protected:

	static int DoEncode(int		run,
											int		runEscBits,
											int		runEscMask,
											int*	codeWord);

protected:
	int 	_numCodeBits;	// Number of coded bits for this motion vector.
	int 	_bitCode;			// The code of length numCodeBits.
	int  _numEscBits;		// Number of bits that follow the Esc code.
	int  _escMask;			// Bit mask of numEscBits.

	// Constants.
	static const int NUM_ESC_CODE_BITS;
	static const int ESC_BIT_CODE;

	static const int VLC_TABLE[RLVE_TABLE_SIZE][2];

};// end class RunLengthVlcEncoder.

#endif
