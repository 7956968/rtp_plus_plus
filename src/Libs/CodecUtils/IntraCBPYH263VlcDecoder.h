/** @file

MODULE						: IntraCBPYH263VlcDecoder

TAG								: ICBPYH263VD

FILE NAME					: IntraCBPYH263VlcDecoder.h

DESCRIPTION				: An CBPY Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.5
										page 37 and Table 13 with an IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTRACBPYH263VLCDECODER_H
#define _INTRACBPYH263VLCDECODER_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class IntraCBPYH263VlcDecoder : public IVlcDecoder
{
public:
	IntraCBPYH263VlcDecoder();
	virtual ~IntraCBPYH263VlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(0); }
	virtual int Decode(IBitStreamReader* bsr); 

protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

};// end class IntraCBPYH263VlcDecoder.

#endif	// _INTRACBPYH263VLCDECODER_H
