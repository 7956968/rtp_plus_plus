/** @file

MODULE						: ImagePlaneDecoderStdImpl

TAG								: IPDSI

FILE NAME					: ImagePlaneDecoderStdImpl.h

DESCRIPTION				: A standard implementation of the base class
										ImagePlaneDecoder.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IMAGEPLANEDECODERSTDIMPL_H
#define _IMAGEPLANEDECODERSTDIMPL_H

#include "ImagePlaneDecoder.h"

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ImagePlaneDecoderStdImpl	: public ImagePlaneDecoder
{
// Construction.
public:
	ImagePlaneDecoderStdImpl(void);
	virtual ~ImagePlaneDecoderStdImpl(void);

// Operations.
public:
	/** Decode the image.
	To be implemented by each implementation. No base implementation provided.
	Take up to allowedBits of quantised and vlc'ed samples from the bit stream
	in sequential colour planes with an endOfPlaneMarkerCode delimiter.
	@param	allowedBits	: Stop before exceeding this bit limit.
	@param	bitsUsed		: Return the exact num of bits used.
	@return							: 0 = all decoded, 1 = bit limit reached, 2 = coding failure.
	*/
	int Decode(int allowedBits, int* bitsUsed);

};//end ImagePlaneEncoderStdImpl.


#endif // _IMAGEPLANEDECODERSTDIMPL_H

