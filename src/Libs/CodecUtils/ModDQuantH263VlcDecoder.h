/** @file

MODULE						: ModDQuantH263VlcDecoder

TAG								: MDQH263VD

FILE NAME					: ModDQuantH263VlcDecoder.h

DESCRIPTION				: A modified DQuant Vlc decoder implementation as defined 
										in ITU-T Recommendation H.263 (02/98) Annex T page 147 
										Table T.1 with an	IVlcDecoder Interface.
REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MODDQUANTH263VLCDECODER_H
#define _MODDQUANTH263VLCDECODER_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class ModDQuantH263VlcDecoder : public IVlcDecoder
{
public:
	ModDQuantH263VlcDecoder();
	virtual ~ModDQuantH263VlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	virtual int Marker(void) { return(0); }
	virtual int Decode(IBitStreamReader* bsr) { return(0); } 

	// Optional interface implementation.
	virtual int	Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2);

protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

};// end class ModDQuantH263VlcDecoder.

#endif	// _MODDQUANTH263VLCDECODER_H
