/** @file

MODULE						: ImagePlaneDecoder

TAG								: IPD

FILE NAME					: ImagePlaneDecoder.cpp

DESCRIPTION				: A base class with the common implementations for a 
										family of implementations to read sequentially from
										a bit stream, decode and inverse quantise each colour 
										plane in an image. The colour plane info is held in 
										ColourPlaneDecoding	objects. The process is as follows:
											// Instantiate.
											ImagePlaneDecoder* p = new ImagePlaneDecoderImpl();
											p->Create();
											p->SetEndOfPlaneMarkerVlc();
											p->SetEndOfImageMarkerVlc();
											.
											.
											// Attach utility classes.
											p->AttachVectorQuantiser();
											.
											.
											p->AttachBitStreamReader();
											.
											.
											// Use.
											p->Decode();
											.
											.
											// Delete.
											p->Destroy()
											delete p;


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

#include	"ImagePlaneDecoder.h"

/*
--------------------------------------------------------------------------
  Construction and destruction. 
--------------------------------------------------------------------------
*/
ImagePlaneDecoder::ImagePlaneDecoder(void)
{
	// Colour plane info.
	for(int i = 0; i < IPD_COLOUR_PLANES; i++)
		_pColourPlane[i] = NULL;

	// References to helper classes not owned by this.
	_pVectorQuantiser		= NULL;
	_pRunVlcDecoder			= NULL;
	_pVqVlcDecoder			= NULL;
	_pIntraVlcDecoder		= NULL;	// For INTRA coded images.
	// Bit stream to read the encoded bits from.
	_pBitStreamReader		= NULL;

	_minPelValue				= 0;	// Default to 6 bit range.
	_maxPelValue				= 63;
	_endOfPlaneMarker		= 32;
	_endOfImageMarker		= 33;

}//end constructor.

ImagePlaneDecoder::~ImagePlaneDecoder(void)
{
	Destroy();
}//end destructor.

int ImagePlaneDecoder::Create(cpdType* refLum, 
															int			 lumWidth,		int			 lumHeight,
															cpdType* refChrU,
															cpdType* refChrV,
															int			 chrWidth,		int			 chrHeight,
															int			 vecLumWidth,	int			 vecLumHeight,
															int			 vecChrWidth,	int			 vecChrHeight)
{
	// Clear out any old baggage.
	Destroy();

	// For decoding an array of decoded data objects are required 
	// for each colour component in a plane. Create the colour 
	// plane info holders.
	for(int colour = 0; colour < IPD_COLOUR_PLANES; colour++)
	{
		_pColourPlane[colour] = new ColourPlaneDecoding();
		if( _pColourPlane[colour] == NULL )
		{
			Destroy();
			return(0);
		}//end if !_pColourPlane[colour]...

		int created = 1;
		switch(colour)
		{
			case IPD_LUM:
				created &= _pColourPlane[colour]->Create(	refLum, 
																									lumWidth,			lumHeight, 
																									vecLumWidth,	vecLumHeight);
				break;
			case IPD_CHRU:
				created &= _pColourPlane[colour]->Create(	refChrU, 
																									chrWidth,			chrHeight, 
																									vecChrWidth,	vecChrHeight);
				break;
			case IPD_CHRV:
				created &= _pColourPlane[colour]->Create(	refChrV, 
																									chrWidth,			chrHeight, 
																									vecChrWidth,	vecChrHeight);
				break;
		}//end switch colour...
		if( !created )
		{
			Destroy();
			return(0);
		}//end if !created...
		// All colours have the same range.
		_pColourPlane[colour]->SetPelValueRange(_minPelValue, _maxPelValue);
	}//end for colour...

	return(1);
}//end Create.

void ImagePlaneDecoder::Destroy(void)
{
	for(int i = 0; i < IPD_COLOUR_PLANES; i++)
	{
		if(_pColourPlane[i] != NULL)
			delete _pColourPlane[i];
		_pColourPlane[i] = NULL;
	}//end for i...

	// References.
	_pVectorQuantiser		= NULL;
	_pRunVlcDecoder			= NULL;
	_pVqVlcDecoder			= NULL;
	_pIntraVlcDecoder		= NULL;
	_pBitStreamReader		= NULL;
}//end Destroy.

/*
--------------------------------------------------------------------------
  Common utility methods. 
--------------------------------------------------------------------------
*/
