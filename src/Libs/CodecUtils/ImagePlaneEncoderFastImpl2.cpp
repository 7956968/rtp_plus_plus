/** @file

MODULE						: ImagePlaneEncoderFastImpl2

TAG								: IPEFI2

FILE NAME					: ImagePlaneEncoderFastImpl2.cpp

DESCRIPTION				: A faster implementation of the base class	ImagePlaneEncoder 
										by bin'ing the distortion selection. This class inherits from 
										ImagePlaneEncoderFastImpl2 as it merely re-orders the process 
										in the Encode() method.

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

#include	"ImagePlaneEncoderFastImpl2.h"

/*
--------------------------------------------------------------------------
  Construction and destruction. 
--------------------------------------------------------------------------
*/
ImagePlaneEncoderFastImpl2::ImagePlaneEncoderFastImpl2(void)
{
}//end constructor.

ImagePlaneEncoderFastImpl2::~ImagePlaneEncoderFastImpl2(void)
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
int ImagePlaneEncoderFastImpl2::Encode(int allowedBits, int* bitsUsed)
{
	int								colour;
	int								i,pos,length;
	CPE_CodedVector*	vecList;

	// First, code each vector and load the coded vector structures
	// for each colour partition.
	for(colour = IPE_LUM; colour < IPE_COLOUR_PLANES; colour++)
	{
		length	= _pColourPlane[colour]->GetVecListLength();
		vecList = _pColourPlane[colour]->GetEncodingList();

		int maxEnergy = 0;

		// Scan each vector for the given colour.
		for(pos = 0; pos < length; pos++)
		{
			// Set the encodings for this vector without vq or vlc.
			_pColourPlane[colour]->SetEncoding(pos);

			// Collect thresholding information.
			_energy[pos] = vecList[pos].weightedDistortion;
			if(_energy[pos] > maxEnergy)
				maxEnergy = _energy[pos];
  
		}//end for pos...

		// Calculate the threshold values for this colour partition.
		FindThresholdEnergies(_energy, maxEnergy, pos);

		// Load thresholding array.
		int* threshList = _pColourPlane[colour]->GetThesholdList();
		for(i = 0; i < IPEFI1_THRESHOLDS; i++)
			threshList[i] = _threshold[i];	//_threshold is global loaded in FindThresholdEnergies().

	}//end for colour...

	// Continually insert the next largest until available bits are reached.
	int availableBits = allowedBits - ((IPE_COLOUR_PLANES - 1) * _endOfPlaneMarkerNumBits);

  int	runbits, bin;
	int	distortionthreshold;
	int	bitcount = 0;
	for(bin = 30; (bin >= 0)&&(bitcount < availableBits); bin--)
	{
		// Pick from the list any vector with greater distortion than that defined by the bin.
		for(colour = IPE_LUM; (colour < IPE_COLOUR_PLANES)&&(bitcount < availableBits); colour++)
		{
			// Which vector struct we are dealing with.
			length				= _pColourPlane[colour]->GetVecListLength();
			vecList				= _pColourPlane[colour]->GetEncodingList();
			_pRunVlcEncoder->SetEsc(_pColourPlane[colour]->GetEscRunBits(),
															_pColourPlane[colour]->GetEscRunMask());
			distortionthreshold	= (_pColourPlane[colour]->GetThesholdList())[bin];

			for(pos = 0; (pos < length)&&(bitcount < availableBits); pos++)
			{
				// Only check those that are to be included.
				if(vecList[pos].includeFlag)
				{
					if(vecList[pos].weightedDistortion > distortionthreshold)
					{
						// Add the vector to the list and recalculate the bit count.
						// Corresponding vector struct that we are dealing with.

						// Now set the encodings for this vector with vq and vlc.
						_pColourPlane[colour]->SetEncoding(pos, _pVectorQuantiser, _pVqVlcEncoder);

						// Still not excluded?
						if(vecList[pos].includeFlag)
						{
							// Run count before and after index to be inserted.
							int beforerun = 0;
							int afterrun  = 0;
							i = pos - 1;	// Before.
							while((i >= 0)&&(!vecList[i].codedFlag))
							{
								beforerun++;
								i--;
							}//end while i...

							runbits = _pRunVlcEncoder->Encode(beforerun);
  					
							i = pos + 1;	// After.
							while((i < length)&&(!vecList[i].codedFlag))
							{
								afterrun++;
								i++;
							}//end while i...
							// Only interested in the after run if it did not reach end of sequence.
							if(i < length)
								runbits += _pRunVlcEncoder->Encode(afterrun);

							// Add the new run bits and vq bits.
							bitcount += (runbits + vecList[pos].numCodeBits);
							vecList[pos].codedFlag	 = 1; // Mark as coded.
							vecList[pos].includeFlag = 0; // And exclude from further decisions.
  					
							// Remove old run bits if not at end of sequence.
							if(i < length)
								bitcount -= _pRunVlcEncoder->Encode(beforerun + afterrun + 1);
						
							if(bitcount < 0) // Bounds check.
								bitcount = 0;
						}//end if IncludeFlag...

					}//end if weightedDistortion...
				}//end if includeFlag...
			}//end for pos...

		}//end for colour...

	}//end for bin...

	return(WriteToStreamWithDecode(allowedBits, bitsUsed));

}//end Encode.

