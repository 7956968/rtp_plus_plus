/** @file

MODULE						: ImagePlaneEncoderIntraImpl

TAG								: IPESI

FILE NAME					: ImagePlaneEncoderIntraImpl.cpp

DESCRIPTION				: An Intra frame implementation of the base class
										ImagePlaneEncoder. Each vector block is coded as
										an average for the block. The dimensions of the
										chr colour components are half that of the lum.

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

#include	"ImagePlaneEncoderIntraImpl.h"

/*
--------------------------------------------------------------------------
  Construction and destruction. 
--------------------------------------------------------------------------
*/
ImagePlaneEncoderIntraImpl::ImagePlaneEncoderIntraImpl(void)
{
}//end constructor.

ImagePlaneEncoderIntraImpl::~ImagePlaneEncoderIntraImpl(void)
{
}//end destructor.

/*
--------------------------------------------------------------------------
  Implementation. 
--------------------------------------------------------------------------
*/
/** Encode the intra image.
To be implemented by each implementation. No base implementation provided.
Add up to allowedBits of quantised and vlc'ed samples to the bit stream
in sequential colour planes with an endOfPlaneMarkerCode delimiter. Determine 
the average value for each Lum, ChrU and ChrV vector and code it as a
differential value. Get the Huffman symbols from the vlc encoders and put	them 
onto the bit stream. The differential value is variable length coded as though 
it were a motion vector coordinate. If the referenced bit limit is exceeded 
then the coding process is halted and the calling function is notified by the 
return parameter. The end of plane marker code is added to the bit stream at 
the end of each colour component.
@param	allowedBits	: Stop before exceeding this bit limit.
@param	bitsUsed		: Return the exact num of bits used.
@return							: 0 = all coded, 1 = bit limit reached, 2 = coding failure.
*/
int ImagePlaneEncoderIntraImpl::Encode(int allowedBits, int* bitsUsed)
{
	int nobits		= 0;
	int bitsSoFar = 0;
	for(int colour = IPE_LUM; colour < IPE_COLOUR_PLANES; colour++)
	{
		short	pix;
		int		i,j;

		// Which vector struct we are dealing with.
		int								length	= _pColourPlane[colour]->GetVecListLength();
		CPE_CodedVector*	vecList	= _pColourPlane[colour]->GetEncodingList();
		cpeType**					src			= _pColourPlane[colour]->GetSrcImgAddr();
		cpeType**					ref			= _pColourPlane[colour]->GetRefImgAddr();
		int	vecX, vecY;
		_pColourPlane[colour]->GetVecDim(&vecX, &vecY);
		int vecLength = vecX * vecY;
		_pRunVlcEncoder->SetEsc(_pColourPlane[colour]->GetEscRunBits(),
														_pColourPlane[colour]->GetEscRunMask());

		// Top left to bottom right vector steps.
		int	ppix	= 0; // Pel prediction.
		int	run		= 0;
		for(int pos = 0; pos < length; pos++)
		{
			int y = vecList[pos].py;
			int x = vecList[pos].px;

			// Continue the coding process if more bits available.
			if(!nobits)
			{
				// Get input vector and calc avg value. 
				pix = 0;
				for(i = 0; i < vecY; i++)
				{
					int row = y+i;
					for(j = 0; j < vecX; j++)
						pix	+= src[row][x+j];
				}//end for i...
				pix = pix/vecLength;
    
				int dpix = pix - ppix;
    
				if(dpix == 0)
					run++;
				else
				{
					// Add the run and diff pix value to the bit stream.
					int  runbits,runcodebits,ibits,icodebits;
					int bits;

					runbits			= _pRunVlcEncoder->Encode(run);
					runcodebits = _pRunVlcEncoder->GetCode();

					// Encode the differnetial of the pel average for the vector.
					ibits			= _pIntraVlcEncoder->Encode(dpix);
					icodebits = _pIntraVlcEncoder->GetCode();
      
          bits = runbits + ibits;
          if(!runbits || !ibits)
          {
						*bitsUsed = bitsSoFar;
            return(2); // Critical error.
          }//end if !runbits...
          if((bitsSoFar + bits) < allowedBits)
          {
            // Add the coded bits to the bit stream.
						_pBitStreamWriter->Write(runbits, runcodebits);
						_pBitStreamWriter->Write(ibits, icodebits);
            bitsSoFar += bits;
          }//end if bitsSoFar...
          else
          {
            nobits = 1;
          }//end else...

					run = 0; // Restart the run.
				}//end else...
    
				ppix = pix; // Update prediction.
			}//end if !nobits...

			// Decode the vector by writing the avg value back.
      for(i = 0; i < vecY; i++)
      {
				// The last value of pred pix was valid.
				memset((void *)(&ref[y+i][x]), ppix, vecX * sizeof(cpeType));
        //int row = y+i;
        //for(j = 0; j < vecX; j++)
        //  ref[row][x+j] = ppix; // The last value of pred pix was valid.
      }//end for i...

		}//end for pos...

		// Add the end-of-plane marker.
		if(!nobits && ((bitsSoFar + _endOfPlaneMarkerNumBits) < allowedBits))
		{
			_pBitStreamWriter->Write(_endOfPlaneMarkerNumBits, _endOfPlaneMarkerCode);
		  bitsSoFar += _endOfPlaneMarkerNumBits;
		}//end if !nobits...

	}//end for colour...

	*bitsUsed = bitsSoFar;
	return(nobits);
}//end Encode.

