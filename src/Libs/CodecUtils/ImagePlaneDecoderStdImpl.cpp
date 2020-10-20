/** @file

MODULE						: ImagePlaneDecoderStdImpl

TAG								: IPDSI

FILE NAME					: ImagePlaneDecoderStdImpl.cpp

DESCRIPTION				: A standard implementation of the base class
										ImagePlaneDecoder.

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

#include	"ImagePlaneDecoderStdImpl.h"

/*
--------------------------------------------------------------------------
  Construction and destruction. 
--------------------------------------------------------------------------
*/
ImagePlaneDecoderStdImpl::ImagePlaneDecoderStdImpl(void)
{
}//end constructor.

ImagePlaneDecoderStdImpl::~ImagePlaneDecoderStdImpl(void)
{
}//end destructor.

/*
--------------------------------------------------------------------------
  Implementation. 
--------------------------------------------------------------------------
*/
/** Decode the image.
To be implemented by each implementation. No base implementation provided.
Take up to allowedBits of quantised and vlc'ed samples from the bit stream
in sequential colour planes with an endOfPlaneMarkerCode delimiter.
@param	allowedBits	: Stop before exceeding this bit limit.
@param	bitsUsed		: Return the exact num of bits used.
@return							: 0 = all decoded, 1 = bit limit reached, 2 = decoding failure.
*/
int ImagePlaneDecoderStdImpl::Decode(int allowedBits, int* bitsUsed)
{
	int	bitsSoFar = 0;
	int noBits		= 0;
	for(int colour = IPD_LUM; (colour < IPD_COLOUR_PLANES)&&(!noBits); colour++)
	{
		int	length	= _pColourPlane[colour]->GetVecListLength();
		// Set the esc bit lengths for the run vlc decoder.
		_pRunVlcDecoder->SetEsc(_pColourPlane[colour]->GetEscRunBits(), 
														_pColourPlane[colour]->GetEscRunMask());

		int morePlaneData = 1;

		// Initialise the starting point of the partition by extracting the 1st
		// run bits from the bit stream and check that it is not an EOI marker.
		int run					= _pRunVlcDecoder->Decode(_pBitStreamReader);
		int runBits			= _pRunVlcDecoder->GetNumDecodedBits();
		int markerFound = _pRunVlcDecoder->Marker();

    bitsSoFar += runBits;
    if(runBits == 0)
    {
      // Loss of bit stream sync, catastrophic failure.
      *bitsUsed	= bitsSoFar;
      return(2);	// Decoding failure.
    }//end if runBits...
    if(markerFound)
    {
      if(run == _endOfImageMarker)
      {
				// Leave now before starting the plane.
				*bitsUsed	= bitsSoFar;
				return(1);
      }//end if run...
			else	// _endOfPlaneMarker
			{
				// This plane is empty.
				morePlaneData = 0;
			}//end else...
    }//end if markerFound...

		for(int pos = 0; (pos < length)&&(morePlaneData); pos++)
		{
			if(run)
				run--;
			else
			{
				// Decode the vq from the bit stream.
				_pColourPlane[colour]->VectorDecode(pos,  _pVectorQuantiser, 
																									_pVqVlcDecoder, 
																									_pBitStreamReader);
				// Get the number of bits extracted from the bit stream.
				int vqIndexBits	= _pVqVlcDecoder->GetNumDecodedBits();
				bitsSoFar += vqIndexBits;
		    if(vqIndexBits == 0)
		    {
		      // Loss of bit stream sync, catastrophic failure.
		      *bitsUsed	= bitsSoFar;
		      return(2);
		    }//end if vqIndexBits...

				// Attempt to get the next run value and check for markers.
				run					= _pRunVlcDecoder->Decode(_pBitStreamReader);
				runBits			= _pRunVlcDecoder->GetNumDecodedBits();
				markerFound = _pRunVlcDecoder->Marker();

    		bitsSoFar += runBits;
    		if(runBits == 0)
    		{
    		  // Loss of bit stream sync, catastrophic failure.
		      *bitsUsed	= bitsSoFar;
    		  return(2);
    		}//end if runBits...
    		if(markerFound)
    		{
					// This plane has no more data and decoding must stop.
    		  if(run == _endOfImageMarker)
						noBits = 1;
					morePlaneData = 0;
					run						= -1; // infinity.
    		}//end if markerFound...

			}//end else...
  
		}//end for pos...

	}//end for colour...

  *bitsUsed	= bitsSoFar;
	return(noBits);
}//end Decode.

