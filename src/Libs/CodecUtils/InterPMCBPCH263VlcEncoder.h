/** @file

MODULE						: InterPMCBPCH263VlcEncoder

TAG								: IPMCBPCH263VE

FILE NAME					: InterPMCBPCH263VlcEncoder.h

DESCRIPTION				: An MCBPC Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.2
										Table 8 page 35 with an	IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTERPMCBPCH263VLCENCODER_H
#define _INTERPMCBPCH263VLCENCODER_H

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class InterPMCBPCH263VlcEncoder : public IVlcEncoder
{
public:
	InterPMCBPCH263VlcEncoder(void);
	virtual ~InterPMCBPCH263VlcEncoder(void);

public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }
	// A single symbol has no meaning for MCBPC elements.
	virtual int Encode(int symbol) { return(0); }

	// Optional interface implementation.
	// The 2 symbols represent MB type and CBPC, respectively.
	virtual int	Encode2(int symbol1, int symbol2);

protected:
	int 	_numCodeBits;	// Number of coded bits for the encoding.
	int 	_bitCode;			// The code of length numCodeBits.

};// end class InterPMCBPCH263VlcEncoder.

#endif	// end _INTERPMCBPCH263VLCENCODER_H.
