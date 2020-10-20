/** @file

MODULE						: IntraMCBPCH263VlcEncoder

TAG								: IMCBPCH263VE

FILE NAME					: IntraMCBPCH263VlcEncoder.h

DESCRIPTION				: An MCBPC Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.2
										page 34 with an	IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTRAMCBPCH263VLCENCODER_H
#define _INTRAMCBPCH263VLCENCODER_H

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class IntraMCBPCH263VlcEncoder : public IVlcEncoder
{
public:
	IntraMCBPCH263VlcEncoder(void);
	virtual ~IntraMCBPCH263VlcEncoder(void);

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

};// end class IntraMCBPCH263VlcEncoder.

#endif	// end _INTRAMCBPCH263VLCENCODER_H.
