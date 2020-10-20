/** @file

MODULE						: IntraDCH263VlcEncoder

TAG								: IDH263VE

FILE NAME					: IntraDCH263VlcEncoder.h

DESCRIPTION				: A class to implement a Intra DC coeff encoder as defined
										in the ITU-T Recommendation H.263 (02/98) Section 5.4.1
										Page 41. There is no Esc coding	requirements. It implements 
										the IVlcEncoder interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTRADCH263VLCENCODER_H
#define _INTRADCH263VLCENCODER_H

#pragma once

#include "IVlcEncoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class IntraDCH263VlcEncoder : public IVlcEncoder
{
public:
	IntraDCH263VlcEncoder()	{_numCodeBits = 0;_bitCode = 0;}
	virtual ~IntraDCH263VlcEncoder()	{ }

public:
	// Interface implementation.
	int GetNumCodedBits(void)	{ return(_numCodeBits); }
	int GetCode(void)					{ return(_bitCode); }

	virtual int Encode(int symbol); 

protected:
	int 	_numCodeBits;	// Number of coded bits for this bitCode.
	int 	_bitCode;			// The code of length numCodeBits.

};// end class IntraDCH263VlcEncoder.

#endif	//_INTRADCH263VLCENCODER_H
