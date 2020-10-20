/** @file

MODULE						: AdvancedIntraModeH263VlcDecoder

TAG								: AIMH263VD

FILE NAME					: AdvancedIntraModeH263VlcDecoder.h

DESCRIPTION				: An advanced Intra mode Vlc decoder implementation 
										as defined in ITU-T Recommendation H.263 (02/98) 
										annex I page 74 Table I.1 with an	IVlcDecoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _ADVANCEDINTRAMODEH263VLCDECODER_H
#define _ADVANCEDINTRAMODEH263VLCDECODER_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class AdvancedIntraModeH263VlcDecoder : public IVlcDecoder
{
public:
	AdvancedIntraModeH263VlcDecoder();
	virtual ~AdvancedIntraModeH263VlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	virtual int Marker(void) { return(0); }
	virtual int Decode(IBitStreamReader* bsr); 

	// Optional interface implementation.
	// No relavent optional symbol decoding.
protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

};// end class AdvancedIntraModeH263VlcDecoder.

#endif	// _ADVANCEDINTRAMODEH263VLCDECODER_H
