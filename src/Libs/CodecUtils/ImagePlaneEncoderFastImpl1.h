/** @file

MODULE						: ImagePlaneEncoderFastImpl1

TAG								: IPEFI1

FILE NAME					: ImagePlaneEncoderFastImpl1.h

DESCRIPTION				: A faster implementation of the base class
										ImagePlaneEncoder by bin'ing the distortion
										selection.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _IMAGEPLANEENCODERFASTIMPL1_H
#define _IMAGEPLANEENCODERFASTIMPL1_H

#include "ImagePlaneEncoder.h"

#define IPEFI1_THRESHOLDS	32

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ImagePlaneEncoderFastImpl1	: public ImagePlaneEncoder
{
// Construction.
public:
	ImagePlaneEncoderFastImpl1(void);
	virtual ~ImagePlaneEncoderFastImpl1(void);

	virtual int Create(	cpeType* srcLum,			cpeType* refLum, 
											int			 lumWidth,		int			 lumHeight,
											cpeType* srcChrU,			cpeType* refChrU,
											cpeType* srcChrV,			cpeType* refChrV,
											int			 chrWidth,		int			 chrHeight,
											int			 vecLumWidth,	int			 vecLumHeight,
											int			 vecChrWidth,	int			 vecChrHeight);

	virtual void Destroy(void);

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

// Class utilities.
protected:
	void FindThresholdEnergies(int*	energy, int maxEnergy, int seqLength);

protected:
	// Fast encoding histograms.
	int*		_threshold;
	double*	_fThreshold;
	int*		_energyHist;
	int*		_energy;
};//end ImagePlaneEncoderFastImpl1.


#endif // _IMAGEPLANEENCODERFASTIMPL1_H

