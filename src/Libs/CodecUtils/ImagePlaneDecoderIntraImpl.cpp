/** @file

MODULE						: ImagePlaneDecoderIntraImpl

TAG								: IPDII

FILE NAME					: ImagePlaneDecoderIntraImpl.cpp

DESCRIPTION				: An intra image implementation of the base class
										ImagePlaneDecoder. A scalar value is used to fill
										the vector location.

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

#include	"ImagePlaneDecoderIntraImpl.h"

/*
--------------------------------------------------------------------------
  Construction and destruction. 
--------------------------------------------------------------------------
*/
ImagePlaneDecoderIntraImpl::ImagePlaneDecoderIntraImpl(void)
{
}//end constructor.

ImagePlaneDecoderIntraImpl::~ImagePlaneDecoderIntraImpl(void)
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
int ImagePlaneDecoderIntraImpl::Decode(int allowedBits, int* bitsUsed)
{
	int	bitsSoFar = 0;
	int noBits		= 0;
	for(int colour = IPD_LUM; (colour < IPD_COLOUR_PLANES)&&(!noBits); colour++)
	{
		int	length	= _pColourPlane[colour]->GetVecListLength();
		CPD_DecodedVector* vecList = _pColourPlane[colour]->GetDecodingList();
		// Set the esc bit lengths for the run vlc decoder.
		_pRunVlcDecoder->SetEsc(_pColourPlane[colour]->GetEscRunBits(), 
														_pColourPlane[colour]->GetEscRunMask());

		int morePlaneData = 1;
		int pix						= 0; // Start prediction.

		// Initialise the starting point of the plane by extracting the 1st
		// run bits from the bit stream and check that it is not an EOI marker.
		int run					= _pRunVlcDecoder->Decode(_pBitStreamReader);
		int runBits			= _pRunVlcDecoder->GetNumDecodedBits();
		int markerFound = _pRunVlcDecoder->Marker();

    bitsSoFar += runBits;
    if(runBits == 0)
    {
      // Loss of bit stream sync, catastrophic failure.
      *bitsUsed	= bitsSoFar;
      return(2);
    }//end if runBits...
    if(markerFound)
    {
      if(run == _endOfImageMarker)
      {
				// Leave now before starting the partition.
				*bitsUsed	= bitsSoFar;
				return(1);
      }//end if run...
			else
			{
				// This plane is empty.
				morePlaneData = 0;
				// ...but must be filled with zeros.
				run = -1; // Infinity.
			}//end else...
    }//end if markerFound...

		for(int pos = 0; pos < length; pos++)
		{
			if(run)
				run--;
			else
			{
				// Get the next value off the bit stream.
				int update			= _pIntraVlcDecoder->Decode(_pBitStreamReader);
				int updateBits	= _pIntraVlcDecoder->GetNumDecodedBits();
				bitsSoFar += updateBits;
		    if(updateBits == 0)
		    {
		      // Loss of bit stream sync, catastrophic failure.
					*bitsUsed	= bitsSoFar;
		      return(2);
		    }//end if updateBits...

				pix += update;
				// Bounds check the average pix value.
				if(pix > _maxPelValue)
					pix = _maxPelValue;
				else if(pix < _minPelValue)
					pix = _minPelValue;

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
					// This plane has no more data but must still be decoded.
    		  if(run == _endOfImageMarker)
						noBits = 1;
					morePlaneData = 0;
					run						= -1; // infinity.
    		}//end if markerFound...

			}//end else...

			// Set decoded struct values.
			vecList[pos].includeFlag	= 1;
			vecList[pos].value				= pix;

		}//end for pos...

	}//end for colour...

	// Write the decoded colour plane from the structs.
	for(int colour = IPD_LUM; colour < IPD_COLOUR_PLANES; colour++)
		_pColourPlane[colour]->ScalarDecode();

	*bitsUsed	= bitsSoFar;
  return(noBits);
}//end Decode.

