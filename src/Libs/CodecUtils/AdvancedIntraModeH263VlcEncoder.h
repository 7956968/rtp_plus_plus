/** @file

MODULE						: AdvancedIntraModeH263VlcEncoder

TAG								: AIMH263VE

FILE NAME					: AdvancedIntraModeH263VlcEncoder.h

DESCRIPTION				: An advanced Intra mode Vlc encoder implementation 
										as defined in ITU-T Recommendation H.263 (02/98) 
										annex I page 74 Table I.1 with an	IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _ADVANCEDINTRAMODEH263VLCENCODER_H
#define _ADVANCEDINTRAMODEH263VLCENCODER_H

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class AdvancedIntraModeH263VlcEncoder : public IVlcEncoder
{
public:
	AdvancedIntraModeH263VlcEncoder(void);
	virtual ~AdvancedIntraModeH263VlcEncoder(void);

public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }
	virtual int Encode(int symbol);

	// Optional interface implementation.
	// None required for this simple vlc.
protected:
	int 	_numCodeBits;	// Number of coded bits for the encoding.
	int 	_bitCode;			// The code of length numCodeBits.

};// end class AdvancedIntraModeH263VlcEncoder.

#endif	// end _ADVANCEDINTRAMODEH263VLCENCODER_H.
