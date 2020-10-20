/** @file

MODULE						: IntraMCBPCH263VlcDecoder

TAG								: IMCBPCH263VD

FILE NAME					: IntraMCBPCH263VlcDecoder.h

DESCRIPTION				: An MCBPC Vlc decoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.2
										page 34 with an	IVlcDecoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTRAMCBPCH263VLCDECODER_H
#define _INTRAMCBPCH263VLCDECODER_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class IntraMCBPCH263VlcDecoder : public IVlcDecoder
{
public:
	IntraMCBPCH263VlcDecoder();
	virtual ~IntraMCBPCH263VlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(_marker); }
	// Single symbols are not relevant for a MCBPC decoder.
	virtual int Decode(IBitStreamReader* bsr) { _numCodeBits = 0; return(0); } 

	// Optional interface implementation.
	virtual int	Decode2(IBitStreamReader* bsr, int* symbol1, int* symbol2);

protected:
	int _numCodeBits;	// Number of coded bits for this symbol.
	int _marker;			// The only marker code is "stuffing"

};// end class IntraMCBPCH263VlcDecoder.

#endif	// _INTRAMCBPCH263VLCDECODER_H
