/** @file

MODULE						: ModDQuantH263VlcEncoder

TAG								: MDQH263VE

FILE NAME					: ModDQuantH263VlcEncoder.h

DESCRIPTION				: A modified DQuant Vlc encoder implementation as defined 
										in ITU-T Recommendation H.263 (02/98) Annex T page 147 
										Table T.1 with an	IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MODDQUANTH263VLCENCODER_H
#define _MODDQUANTH263VLCENCODER_H

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class ModDQuantH263VlcEncoder : public IVlcEncoder
{
public:
	ModDQuantH263VlcEncoder(void);
	virtual ~ModDQuantH263VlcEncoder(void);

public:
	// Interface implementation.
	int GetNumCodedBits(void)				{ return(_numCodeBits); }
	int GetCode(void)								{ return(_bitCode); }
	// Undefined for a single symbol in this implementation.
	virtual int Encode(int symbol)	{ return(0); }

	// Optional interface implementation.

	// The modified quant mode requires knowledge of the prior 
	// QUANT value before selecting a vlc for coding. The first
	// symbol, symbol1 = QUANT, and symbol2 = DQUANT.
	virtual int	Encode2(int symbol1, int symbol2);								

protected:
	int 	_numCodeBits;	// Number of coded bits for the encoding.
	int 	_bitCode;			// The code of length numCodeBits.

};// end class ModDQuantH263VlcEncoder.

#endif	// end _MODDQUANTH263VLCENCODER_H.
