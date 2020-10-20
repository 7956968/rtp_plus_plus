/** @file

MODULE						: DQuantH263VlcDecoder

TAG								: DQH263VD

FILE NAME					: DQuantH263VlcDecoder.h

DESCRIPTION				: A DQuant Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.6
										page 37 Table 12 with an IVlcDecoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _DQUANTH263VLCDECODER_H
#define _DQUANTH263VLCDECODER_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class DQuantH263VlcDecoder : public IVlcDecoder
{
public:
	DQuantH263VlcDecoder();
	virtual ~DQuantH263VlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	virtual int Marker(void) { return(0); }
	virtual int Decode(IBitStreamReader* bsr); 

	// Optional interface implementation.
	// No relavent optional symbol decoding.
protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

};// end class DQuantH263VlcDecoder.

#endif	// _DQUANTH263VLCDECODER_H
