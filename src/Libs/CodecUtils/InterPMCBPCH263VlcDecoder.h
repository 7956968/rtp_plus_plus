/** @file

MODULE						: InterPMCBPCH263VlcDecoder

TAG								: IPMCBPCH263VD

FILE NAME					: InterPMCBPCH263VlcDecoder.h

DESCRIPTION				: An MCBPC Vlc decoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.2
										Table 8 page 35 with an	IVlcDecoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTERPMCBPCH263VLCDECODER_H
#define _INTERPMCBPCH263VLCDECODER_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class InterPMCBPCH263VlcDecoder : public IVlcDecoder
{
public:
	InterPMCBPCH263VlcDecoder();
	virtual ~InterPMCBPCH263VlcDecoder();

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

};// end class InterPMCBPCH263VlcDecoder.

#endif	// _INTERPMCBPCH263VLCDECODER_H
