/** @file

MODULE						: InterPCBPYH263VlcDecoder

TAG								: IPCBPYH263VD

FILE NAME					: InterPCBPYH263VlcDecoder.h

DESCRIPTION				: An CBPY Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.5
										page 37 and Table 13 with an IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTERPCBPYH263VLCDECODER_H
#define _INTERPCBPYH263VLCDECODER_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class InterPCBPYH263VlcDecoder : public IVlcDecoder
{
public:
	InterPCBPYH263VlcDecoder();
	virtual ~InterPCBPYH263VlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(0); }
	virtual int Decode(IBitStreamReader* bsr); 

protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

};// end class InterPCBPYH263VlcDecoder.

#endif	// _INTERPCBPYH263VLCDECODER_H
