/** @file

MODULE						: ImagePlaneEncoderConstQImpl

TAG								: IPECQI

FILE NAME					: ImagePlaneEncoderConstQImpl.h

DESCRIPTION				: A constant quality implementation of the base class
										ImagePlaneEncoder.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IMAGEPLANEENCODERCONSTQIMPL_H
#define _IMAGEPLANEENCODERCONSTQIMPL_H

#include "ImagePlaneEncoder.h"

#define IPECQI_THRESHOLDS	32

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ImagePlaneEncoderConstQImpl	: public ImagePlaneEncoder
{
// Construction.
public:
	ImagePlaneEncoderConstQImpl(void);
	virtual ~ImagePlaneEncoderConstQImpl(void);

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

// Implementation specific methods.
public:
	void SetThreshold(int threshold);

protected:
	int	_threshold;
	int _distortionThreshold;
	int _distThresholdList[IPECQI_THRESHOLDS];
};//end ImagePlaneEncoderConstQImpl.


#endif // _IMAGEPLANEENCODERCONSTQIMPL_H

