/** @file

MODULE						: ImagePlaneEncoderIntraImpl

TAG								: IPEII

FILE NAME					: ImagePlaneEncoderIntraImpl.h

DESCRIPTION				: An Intra frame implementation of the base class
										ImagePlaneEncoder. Each vector block is coded as
										an average for the block.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IMAGEPLANEENCODERINTRAIMPL_H
#define _IMAGEPLANEENCODERINTRAIMPL_H

#include "ImagePlaneEncoder.h"

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ImagePlaneEncoderIntraImpl	: public ImagePlaneEncoder
{
// Construction.
public:
	ImagePlaneEncoderIntraImpl(void);
	virtual ~ImagePlaneEncoderIntraImpl(void);

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

};//end ImagePlaneEncoderIntraImpl.


#endif // _IMAGEPLANEENCODERINTRAIMPL_H

