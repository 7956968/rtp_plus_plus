/** @file

MODULE						: ImagePlaneEncoderConstQImpl

TAG								: IPECQI

FILE NAME					: ImagePlaneEncoderConstQImpl.cpp

DESCRIPTION				: A constant quality implementation of the base class
										ImagePlaneEncoder.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include <memory.h>
#include <string.h>
#include <stdlib.h>

#include	"ImagePlaneEncoderConstQImpl.h"

/*
--------------------------------------------------------------------------
  Construction and destruction. 
--------------------------------------------------------------------------
*/
ImagePlaneEncoderConstQImpl::ImagePlaneEncoderConstQImpl(void)
{
	_threshold						= 0;
	_distortionThreshold	= 0;

	for(int lclthresh = 0; lclthresh < IPECQI_THRESHOLDS; lclthresh++)
	{
		int t = 1 << (lclthresh/2);
		_distThresholdList[lclthresh] = (lclthresh%2)*(t - (t>>1)) + t;
	}//end for lclthresh...
}//end constructor.

ImagePlaneEncoderConstQImpl::~ImagePlaneEncoderConstQImpl(void)
{
}//end destructor.

/*
--------------------------------------------------------------------------
  Implementation. 
--------------------------------------------------------------------------
*/

void ImagePlaneEncoderConstQImpl::SetThreshold(int threshold)
{
	if(threshold < 0)
		_threshold = 0;
	else if(threshold >= IPECQI_THRESHOLDS)
		_threshold = IPECQI_THRESHOLDS - 1;
	else
		_threshold = threshold;

	_distortionThreshold = _distThresholdList[_threshold];
}//end SetThreshold.

/** Encode the image.
To be implemented by each implementation. No base implementation provided.
Add up to allowedBits of quantised and vlc'ed samples to the bit stream
in sequential colour planes with an endOfPlaneMarkerCode delimiter.
@param	allowedBits	: Stop before exceeding this bit limit.
@param	bitsUsed		: Return the exact num of bits used.
@return							: 0 = all coded, 1 = bit limit reached, 2 = coding failure.
*/
int ImagePlaneEncoderConstQImpl::Encode(int allowedBits, int* bitsUsed)
{
	int								colour;
	int								i,pos,length;
	CPE_CodedVector*	vecList;
	int								distThreshold;

	// First, code each vector and load the coded vector structures
	// for each colour partition.
	for(colour = IPE_LUM; colour < IPE_COLOUR_PLANES; colour++)
	{
		// Set the encodings for this colour.
		_pColourPlane[colour]->SetEncoding(_pVectorQuantiser, _pVqVlcEncoder);

		if(colour == IPE_LUM)
			distThreshold = _distortionThreshold;
		else	// Chr values are smaller.
			distThreshold = _distortionThreshold;

		// Scan the encodings and exclude those vectors that are below the threshold.
		vecList	= _pColourPlane[colour]->GetEncodingList();
		length	= _pColourPlane[colour]->GetVecListLength();
		for(pos = 0; pos < length; pos++)
		{
			if(vecList[pos].uncodedDistortion <= distThreshold)
				vecList[pos].includeFlag = 0;
		}//end for pos...
	}//end for colour...

	// Continually insert the next largest until available bits are reached.
	int availableBits = allowedBits - ((IPE_COLOUR_PLANES - 1) * _endOfPlaneMarkerNumBits);

  int		runbits;
	long	bitcount		= 0;
	int		largefound	= 1; // Try at least once.
	while((bitcount < availableBits) && largefound)
	{
		// Pick the next largest from the list.
		largefound					= 0;
		int largeindex			= 0;
		int largepartition	= 0;
		int largedistortion	= -1;

		for(colour = IPE_LUM; colour < IPE_COLOUR_PLANES; colour++)
		{
			// Which vector struct we are dealing with.
			length	= _pColourPlane[colour]->GetVecListLength();
			vecList	= _pColourPlane[colour]->GetEncodingList();

			for(pos = 0; pos < length; pos++)
			{
				// Only check those that are to be included.
				if(vecList[pos].includeFlag)
				{
					if(vecList[pos].weightedDistortion > largedistortion)
					{
						largefound			= 1;
						largeindex			= pos;
						largepartition	= colour;
						largedistortion = vecList[pos].weightedDistortion;
					}//end if weightedDistortion...
				}//end if !includedFlag...
			}//end for pos...
		}//end for colour...

		if(largefound)
		{
			// Add the largest from the partition list and recalculate the bit count.
			// Corresponding vector struct that we are dealing with.
			length	= _pColourPlane[largepartition]->GetVecListLength();
			vecList	= _pColourPlane[largepartition]->GetEncodingList();
			_pRunVlcEncoder->SetEsc(_pColourPlane[largepartition]->GetEscRunBits(), 
															_pColourPlane[largepartition]->GetEscRunMask());

			// Run count before and after index to be inserted.
			int beforerun = 0;
			int afterrun  = 0;
			i = largeindex - 1;	// Before.
			while((i >= 0)&&(!vecList[i].codedFlag))
			{
				beforerun++;
				i--;
			}//end while i...

			runbits = _pRunVlcEncoder->Encode(beforerun);
  
			i = largeindex + 1;	// After.
			while((i < length)&&(!vecList[i].codedFlag))
			{
				afterrun++;
				i++;
			}//end while i...
			// Only interested in the after run if it did not reach end of sequence.
			if(i < length)
				runbits += _pRunVlcEncoder->Encode(afterrun);
  
			// Add the new run bits and vq bits.
			bitcount += (runbits + vecList[largeindex].numCodeBits);
			vecList[largeindex].codedFlag		= 1; // Mark as coded.
			vecList[largeindex].includeFlag = 0; // And exclude from further decisions.
  
			// Remove old run bits if not at end of sequence.
			if(i < length)
				bitcount -= _pRunVlcEncoder->Encode(beforerun + afterrun + 1);

			if(bitcount < 0) // Bounds check.
				bitcount = 0;
		}//end if largefound...

	}//end while bitcount...

	return(WriteToStreamWithDecode(allowedBits, bitsUsed));

}//end Encode.

