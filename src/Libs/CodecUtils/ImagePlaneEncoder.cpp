/** @file

MODULE						: ImagePlaneEncoder

TAG								: IPE

FILE NAME					: ImagePlaneEncoder.cpp

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

#include	"ImagePlaneEncoder.h"

/*
--------------------------------------------------------------------------
  Construction and destruction. 
--------------------------------------------------------------------------
*/
ImagePlaneEncoder::ImagePlaneEncoder(void)
{
	// Colour plane info.
	for(int i = 0; i < IPE_COLOUR_PLANES; i++)
		_pColourPlane[i] = NULL;
	// Utility vector.
	_vec	= NULL;

	// References to helper classes not owned by this.
	_pVectorQuantiser		= NULL;
	_pRunVlcEncoder			= NULL;
	_pVqVlcEncoder			= NULL;
	_pIntraVlcEncoder		= NULL;	// For INTRA coded images.
	// Bit stream to write the encoded bits to.
	_pBitStreamWriter		= NULL;

	_minPelValue							= 0;	// Default to 6 bit range.
	_maxPelValue							= 63;
	_endOfPlaneMarkerCode			= 0x00000340;
	_endOfPlaneMarkerNumBits	= 11;
	_endOfImageMarkerCode			= 0x00000740;
	_endOfImageMarkerNumBits	= 11;

}//end constructor.

ImagePlaneEncoder::~ImagePlaneEncoder(void)
{
	Destroy();
}//end destructor.

int ImagePlaneEncoder::Create(cpeType* srcLum,			cpeType* refLum, 
															int			 lumWidth,		int			 lumHeight,
															cpeType* srcChrU,			cpeType* refChrU,
															cpeType* srcChrV,			cpeType* refChrV,
															int			 chrWidth,		int			 chrHeight,
															int			 vecLumWidth,	int			 vecLumHeight,
															int			 vecChrWidth,	int			 vecChrHeight)
{
	// Clear out any old baggage.
	Destroy();

	// For encoding an array of coded data objects are required 
	// for each colour component in a plane. Create the colour 
	// plane info holders.
	for(int colour = 0; colour < IPE_COLOUR_PLANES; colour++)
	{
		_pColourPlane[colour] = new ColourPlaneEncoding();
		if( _pColourPlane[colour] == NULL )
		{
			Destroy();
			return(0);
		}//end if !_pColourPlane[colour]...

		int created = 1;
		switch(colour)
		{
			case IPE_LUM:
				created &= _pColourPlane[colour]->Create(	srcLum,				refLum, 
																									lumWidth,			lumHeight, 
																									vecLumWidth,	vecLumHeight);
				break;
			case IPE_CHRU:
				created &= _pColourPlane[colour]->Create(	srcChrU,			refChrU, 
																									chrWidth,			chrHeight, 
																									vecChrWidth,	vecChrHeight);
				break;
			case IPE_CHRV:
				created &= _pColourPlane[colour]->Create(	srcChrV,			refChrV, 
																									chrWidth,			chrHeight, 
																									vecChrWidth,	vecChrHeight);
				break;
		}//end switch colour...
		if( !created )
		{
			Destroy();
			return(0);
		}//end if !created...
	}//end for colour...

	// A helper vector for intermediate calc. Assume Lum size >= Chr size.
	_vec = new short[vecLumWidth * vecLumHeight];
	if(_vec == NULL)
	{
		Destroy();
		return(0);
	}//end if !_vec...

	return(1);
}//end Create.

void ImagePlaneEncoder::Destroy(void)
{
	for(int i = 0; i < IPE_COLOUR_PLANES; i++)
	{
		if(_pColourPlane[i] != NULL)
			delete _pColourPlane[i];
		_pColourPlane[i] = NULL;
	}//end for i...

	if(_vec != NULL)
		delete[] _vec;
	_vec = NULL;

	// References.
	_pVectorQuantiser		= NULL;
	_pRunVlcEncoder			= NULL;
	_pVqVlcEncoder			= NULL;
	_pIntraVlcEncoder		= NULL;
	_pBitStreamWriter		= NULL;
}//end Destroy.

/*
--------------------------------------------------------------------------
  Common utility methods. 
--------------------------------------------------------------------------
*/
int ImagePlaneEncoder::WriteToStreamWithDecode(int allowedBits, int* bitsUsed)
{
	int	zerorun;
	int nobits		= 0;
	int bitsSoFar = 0;
	// "Load and code" the vector info structs onto the bit stream.
	for(int colour = IPE_LUM; (colour < IPE_COLOUR_PLANES) && (!nobits); colour++)
	{
		// Which vector struct we are dealing with.
		cpeType**					ref			= _pColourPlane[colour]->GetRefImgAddr();
		int								length	= _pColourPlane[colour]->GetVecListLength();
		CPE_CodedVector*	vecList	= _pColourPlane[colour]->GetEncodingList();
		_pRunVlcEncoder->SetEsc(_pColourPlane[colour]->GetEscRunBits(), 
														_pColourPlane[colour]->GetEscRunMask());
		int vecX, vecY;
		_pColourPlane[colour]->GetVecDim(&vecX, &vecY);

		zerorun	= 0;
		for(int pos = 0; (pos < length) && (!nobits); pos++)
		{
			if(vecList[pos].codedFlag)
			{
				// Add the run and vq code to the bit stream.
				int runbits			= _pRunVlcEncoder->Encode(zerorun);
				int runcodebits = _pRunVlcEncoder->GetCode();

				int vqbits			= vecList[pos].numCodeBits;
				int vqcodebits	= vecList[pos].codeWord;
      
        int bits = runbits + vqbits;
        if(!runbits || !vqbits)
        {
					*bitsUsed = bitsSoFar;
          return(2);	// Critical error.
        }//end if !runbits...
        if((bitsSoFar + bits) < allowedBits)
        {
          // Add the coded bits to the bit stream.
					_pBitStreamWriter->Write(runbits, runcodebits);
					_pBitStreamWriter->Write(vqbits, vqcodebits);
          bitsSoFar += bits;

					// Decode the vector by adding to the reference.
					int py = vecList[pos].py;
					int px = vecList[pos].px;
					// Codebook codevector pointer.
					const short* pC = (const short *)_pVectorQuantiser->InverseQuantise(vecList[pos].vqIndex); 
					for(int i = 0; i < vecY; i++)
					{
						int imgrow = py + i;
						int vecrow = i * vecX;
						for(int j = 0; j < vecX; j++)
						{
							int		imgcol	= px + j;
							short pix			= pC[vecrow + j];
							pix	+= ref[imgrow][imgcol];
							if(pix < _minPelValue)
								pix = _minPelValue;
							if(pix > _maxPelValue)
								pix = _maxPelValue;
							ref[imgrow][imgcol] = (signed char) pix;
						}//end for j...
					}//end for i...

        }//end if bitsSoFar...
        else
        {
          nobits = 1;
        }//end else...

				zerorun = 0; // Restart the run.
			}//end if codedFlag...
			else
				zerorun++;
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
}//end WriteToStreamWithDecode.

