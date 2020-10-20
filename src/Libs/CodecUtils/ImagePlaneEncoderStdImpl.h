/** @file

MODULE						: ImagePlaneEncoderStdImpl

TAG								: IPESI

FILE NAME					: ImagePlaneEncoderStdImpl.h

DESCRIPTION				: A standard implementation of the base class
										ImagePlaneEncoder.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IMAGEPLANEENCODERSTDIMPL_H
#define _IMAGEPLANEENCODERSTDIMPL_H

#include "ImagePlaneEncoder.h"

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ImagePlaneEncoderStdImpl	: public ImagePlaneEncoder
{
// Construction.
public:
	ImagePlaneEncoderStdImpl(void);
	virtual ~ImagePlaneEncoderStdImpl(void);

// Operations.
public:
	/** Encode the image.
	To be implemented by each implementation. No base implementation provided.
	Add up to allowedBits of quantised and vlc'ed samples to the bit stream
	in sequential colour planes with an endOfPlaneMarkerCode delimiter.
	@param	allowedBits	: Stop before exceeding this bit limit.
	@param	bitsUsed		: Return the exact num of bits used.
	@return							: 0 = all coded, 1 = bit limit reached, 2 = coding failure.
	*/
	int Encode(int allowedBits, int* bitsUsed);

};//end ImagePlaneEncoderStdImpl.


#endif // _IMAGEPLANEENCODERSTDIMPL_H

