/** @file

MODULE						: IntraDCH263VlcDecoder

TAG								: IDH263VD

FILE NAME					: IntraDCH263VlcDecoder.h

DESCRIPTION				: A class to implement a Intra DC coeff decoder as defined
										in the ITU-T Recommendation H.263 (02/98) Section 5.4.1
										Page 41. There is no Esc coding	requirements. It implements 
										the IVlcDecoder interface.

REVISION HISTORY	:

COPYRIGHT					:

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _INTRADCH263VLCDECODER_H
#define _INTRADCH263VLCDECODER_H

#pragma once

#include "IVlcDecoder.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class IntraDCH263VlcDecoder : public IVlcDecoder
{
public:
	IntraDCH263VlcDecoder()	{ _numCodeBits = 0; }
	virtual ~IntraDCH263VlcDecoder()	{ }

public:
	// Interface implementation.
	int GetNumDecodedBits(void)	{ return(_numCodeBits); }
	int Marker(void)						{ return(0); }

	/** Decode an INTRADC value from the stream.
	The H.263 standard defines a fixed length code of 8 bits
	where 0 and 128 are not used. Read the codeword from the
	bit stream and return it. _numCodeBits is set.
	@param bsr	: Bit stream to read from.
	@return			: The decoded INTRADC value.
	*/
	virtual int Decode(IBitStreamReader* bsr); 

protected:
	int 	_numCodeBits;	// Number of coded bits for the last decode.

};// end class IntraDCH263VlcDecoder.

#endif	//_INTRADCH263VLCDECODER_H
