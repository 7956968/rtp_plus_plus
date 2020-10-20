/** @file

MODULE						: ImagePlaneEncoderStdImpl

TAG								: IPESI

FILE NAME					: ImagePlaneEncoderStdImpl.cpp

DESCRIPTION				: A base class with the common implementations for a 
										family of implementations to quantise, encode and 
										write sequentially to a bit stream each colour plane 
										in an image. The colour plane info is held in 
										ColourPlaneEncoding	objects.

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

#include	"ImagePlaneEncoderStdImpl.h"

/*
--------------------------------------------------------------------------
  Construction and destruction. 
--------------------------------------------------------------------------
*/
ImagePlaneEncoderStdImpl::ImagePlaneEncoderStdImpl(void)
{
}//end constructor.

ImagePlaneEncoderStdImpl::~ImagePlaneEncoderStdImpl(void)
{
}//end destructor.

/*
--------------------------------------------------------------------------
  Implementation. 
--------------------------------------------------------------------------
*/
/** Encode the image.
To be implemented by each implementation. No base implementation provided.
Add up to allowedBits of quantised and vlc'ed samples to the bit stream
in sequential colour planes with an endOfPlaneMarkerCode delimiter.
@param	allowedBits	: Stop before exceeding this bit limit.
@param	bitsUsed		: Return the exact num of bits used.
@return							: 0 = all coded, 1 = bit limit reached, 2 = coding failure.
*/
int ImagePlaneEncoderStdImpl::Encode(int allowedBits, int* bitsUsed)
{
	int								colour;
	int								i,pos,length;
	CPE_CodedVector*	vecList;

	// First, code each vector and load the coded vector structures
	// for each colour partition.
	for(colour = IPE_LUM; colour < IPE_COLOUR_PLANES; colour++)
	{
		// Set the encodings for this colour.
		_pColourPlane[colour]->SetEncoding(_pVectorQuantiser, _pVqVlcEncoder); 
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

