/** @file

MODULE						: InterPCBPYH263VlcEncoder

TAG								: IPCBPYH263VE

FILE NAME					: InterPCBPYH263VlcEncoder.h

DESCRIPTION				: An CBPY Vlc encoder implementation as defined in
										ITU-T Recommendation H.263 (02/98) section 5.3.5
										page 37 and Table 13 with an IVlcEncoder Interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTERPCBPYH263VLCENCODER_H
#define _INTERPCBPYH263VLCENCODER_H

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class InterPCBPYH263VlcEncoder : public IVlcEncoder
{
public:
	InterPCBPYH263VlcEncoder(void);
	virtual ~InterPCBPYH263VlcEncoder(void);

public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }
	virtual int Encode(int symbol);

protected:
	int 	_numCodeBits;	// Number of coded bits for the encoding.
	int 	_bitCode;			// The code of length numCodeBits.

};// end class InterPCBPYH263VlcEncoder.

#endif	// end _INTERPCBPYH263VLCENCODER_H.
