/** @file

MODULE						: ImagePlaneDecoderIntraImpl

TAG								: IPDII

FILE NAME					: ImagePlaneDecoderIntraImpl.h

DESCRIPTION				: An intra image implementation of the base class
										ImagePlaneDecoder. A scalar value is used to fill
										the vector location.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IMAGEPLANEDECODERINTRAIMPL_H
#define _IMAGEPLANEDECODERINTRAIMPL_H

#include "ImagePlaneDecoder.h"

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ImagePlaneDecoderIntraImpl	: public ImagePlaneDecoder
{
// Construction.
public:
	ImagePlaneDecoderIntraImpl(void);
	virtual ~ImagePlaneDecoderIntraImpl(void);

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

};//end ImagePlaneDecoderIntraImpl.


#endif // _IMAGEPLANEDECODERINTRAIMPL_H

