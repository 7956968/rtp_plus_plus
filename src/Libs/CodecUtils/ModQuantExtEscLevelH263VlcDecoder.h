/** @file

MODULE						: ModQuantExtEscLevelH263VlcDecoder

TAG								: MQEELH263VD

FILE NAME					: ModQuantExtEscLevelH263VlcDecoder.h

DESCRIPTION				: The Modified Quantisation Mode marks an extended Esc range of
										coefficient values by allowing -128 (normally forbidden) to 
										indicate a further extended 11 bits to represent the true
										coefficient value. Defined in H.263 Recommendation (02/98) 
										Annex T.4 page 148. This is only used after a normal VlcDecoder
										call where the level for that call is -128 and is implemented 
										with an	IVlcDecoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MODQUANTEXTESCLEVELH263VLCDECODER_H
#define _MODQUANTEXTESCLEVELH263VLCDECODER_H

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class ModQuantExtEscLevelH263VlcDecoder : public IVlcDecoder
{
public:
	ModQuantExtEscLevelH263VlcDecoder();
	virtual ~ModQuantExtEscLevelH263VlcDecoder();

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(0); }	// No markers for this decoder.
	virtual int Decode(IBitStreamReader* bsr); 

	// Optional interface implementation.
	// None for this implementation.

protected:
	int _numCodeBits;	// Number of coded bits for this symbol.

};// end class ModQuantExtEscLevelH263VlcDecoder.

#endif	// _MODQUANTEXTESCLEVELH263VLCDECODER_H
