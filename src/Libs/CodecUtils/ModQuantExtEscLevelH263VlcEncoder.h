/** @file

MODULE						: ModQuantExtEscLevelH263VlcEncoder

TAG								: MQEELH263VE

FILE NAME					: ModQuantExtEscLevelH263VlcEncoder.h

DESCRIPTION				: The Modified Quantisation Mode marks an extended Esc range of
										coefficient values by allowing -128 (normally forbidden) to 
										indicate a further extended 11 bits to represent the true
										coefficient value. Defined in H.263 Recommendation (02/98) 
										Annex T.4 page 148. This is only used after a normal VlcEncoder
										call where the level for that call is -128 and is implemented 
										with an	IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MODQUANTEXTESCLEVELH263VLCENCODER_H
#define _MODQUANTEXTESCLEVELH263VLCENCODER_H

#include "IVlcEncoder.h"

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
class ModQuantExtEscLevelH263VlcEncoder : public IVlcEncoder
{
public:
	ModQuantExtEscLevelH263VlcEncoder(void);
	virtual ~ModQuantExtEscLevelH263VlcEncoder(void);

public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }
	virtual int Encode(int symbol);

	// Optional interface implementation.
	// There are none for this implementation.

protected:
	int 	_numCodeBits;	// Number of coded bits for the encoding.
	int 	_bitCode;			// The code of length numCodeBits.

};// end class ModQuantExtEscLevelH263VlcEncoder.

#endif	// end _MODQUANTEXTESCLEVELH263VLCENCODER_H.
