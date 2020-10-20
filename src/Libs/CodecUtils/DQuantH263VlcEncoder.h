/** @file

MODULE						: DQuantH263VlcEncoder

TAG								: DQH263VE

FILE NAME					: DQuantH263VlcEncoder.h

DESCRIPTION				: A DQuant Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.6
										page 37 Table 12 with an	IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _DQUANTH263VLCENCODER_H
#define _DQUANTH263VLCENCODER_H

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class DQuantH263VlcEncoder : public IVlcEncoder
{
public:
	DQuantH263VlcEncoder(void);
	virtual ~DQuantH263VlcEncoder(void);

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

};// end class DQuantH263VlcEncoder.

#endif	// end _DQUANTH263VLCENCODER_H.
