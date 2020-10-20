/** @file

MODULE						: AdvancedIntraModeH263VlcDecoder

TAG								: DQH263VD

FILE NAME					: AdvancedIntraModeH263VlcDecoder.cpp

DESCRIPTION				: An advanced Intra mode Vlc decoder implementation 
										as defined in ITU-T Recommendation H.263 (02/98) 
										annex I page 74 Table I.1 with an	IVlcDecoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include "AdvancedIntraModeH263VlcDecoder.h"

AdvancedIntraModeH263VlcDecoder::AdvancedIntraModeH263VlcDecoder()
{
	_numCodeBits	= 0;
}//end constructor.

AdvancedIntraModeH263VlcDecoder::~AdvancedIntraModeH263VlcDecoder()
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Decode an INTRA_MODE value from the stream.
The H.263 standard defines a vlc of 1/2 bits. Read the 
codeword from the bit stream and return it. _numCodeBits is set.
@param bsr	: Bit stream to read from.
@return			: The decoded INTRA_MODE value.
*/
int AdvancedIntraModeH263VlcDecoder::Decode(IBitStreamReader* bsr)
{
	int intramode = 0;
	int code			= bsr->Read();
	_numCodeBits	= 1;

	if(code == 0) 
		intramode = 0;	// 0 = DC only mode.
	else
	{
		code = bsr->Read();
		_numCodeBits++;
		if(code == 0)
			intramode = 1;	// 1 = Vertical DC & AC mode.
		else
			intramode = 2;	// 2 = Horiz DC & AC mode.
		}//end else...

	return(intramode);
}//end Decode.

