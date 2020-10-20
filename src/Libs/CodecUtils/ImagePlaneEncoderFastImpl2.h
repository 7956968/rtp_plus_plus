/** @file

MODULE						: ImagePlaneEncoderFastImpl2

TAG								: IPEFI2

FILE NAME					: ImagePlaneEncoderFastImpl2.h

DESCRIPTION				: A faster implementation of the base class	ImagePlaneEncoder 
										by bin'ing the distortion selection. This class inherits from 
										ImagePlaneEncoderFastImpl2 as it merely re-orders the process 
										in the Encode() method.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IMAGEPLANEENCODERFASTIMPL2_H
#define _IMAGEPLANEENCODERFASTIMPL2_H

#include "ImagePlaneEncoderFastImpl1.h"

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ImagePlaneEncoderFastImpl2	: public ImagePlaneEncoderFastImpl1
{
// Construction.
public:
	ImagePlaneEncoderFastImpl2(void);
	virtual ~ImagePlaneEncoderFastImpl2(void);

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
	virtual int Encode(int allowedBits, int* bitsUsed);

};//end ImagePlaneEncoderFastImpl2.


#endif // _IMAGEPLANEENCODERFASTIMPL2_H

