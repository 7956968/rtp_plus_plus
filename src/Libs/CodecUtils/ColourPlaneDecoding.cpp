/** @file

MODULE						: ColourPlaneDecoding

TAG								: CPD

FILE NAME					: ColourPlaneDecoding.cpp

DESCRIPTION				: A structure class to hold colour plane decodings as a
										utility class for image plane decoders. Operations
										are get/set and construction/destruction. Most members
										are therefore public. One instantiation per colour
										component is required.

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

#include	"ColourPlaneDecoding.h"

/*
--------------------------------------------------------------------------
  Constants. 
--------------------------------------------------------------------------
*/

ColourPlaneDecoding::ColourPlaneDecoding(void)
{
  _vecLength			= 0;
  // Different size images have different total no. of vectors.	Act as constants.
  _ESC_RUN_BITS		= 0;
  _ESC_RUN_MASK		= 0;
	_decodedInfo		= NULL; // Decoded vector/scalar detail list.
	// Colour plane info.
	_pRef						= NULL;
	_Ref						= NULL;
	_imgWidth				= 0;
	_imgHeight			= 0;
	_min						= 0;
	_max						= (cpdType)63;
  // The vector parameters.
	_vecWidth				= 0;
	_vecHeight			= 0;
}//end constructor.

ColourPlaneDecoding::~ColourPlaneDecoding(void)
{
	Destroy();
}//end destructor.

int ColourPlaneDecoding::Create(cpdType*	ref, 
																int				imgWidth, int				imgHeight, 
																int				vecWidth, int				vecHeight)
{
	// Clear out any old baggage.
	Destroy();

	// Bounds check.
	if( (ref == NULL)||
			(imgWidth < 1)||(imgHeight < 1)||
			(vecWidth < 1)|| (vecHeight < 1) )
		return(0);

	// Load up knowns.
	_pRef				= ref;
	_imgWidth		= imgWidth;
	_imgHeight	= imgHeight;
	_vecWidth		= vecWidth;
	_vecHeight	= vecHeight;

	// Construct address arrays.
	_Ref = new cpdType* [imgWidth * imgHeight];
	if( _Ref == NULL )
	{
		Destroy();
		return(0);
	}//end if !_Src...

	// Load address arrays.
	for(int i = 0; i < imgHeight; i++)
	{
		_Ref[i] = &(ref[imgWidth * i]);
	}//end for i...

	// 2-D vector (macroblock) assemblies.
	_vecLength = (imgWidth / vecWidth) * (imgHeight / vecHeight);
	_decodedInfo = new CPD_DecodedVector[_vecLength];
	if(_decodedInfo == NULL)
	{
		Destroy();
		return(0);
	}//end if !_decodedInfo...

	SetEsc(_vecLength, &_ESC_RUN_BITS, &_ESC_RUN_MASK);

	// Load top left corner location in imgs.
	int j = 0;
	for(int y = 0; y < imgHeight; y += vecHeight)
		for(int x = 0; x < imgWidth; x += vecWidth, j++)
		{
			_decodedInfo[j].py = y;
			_decodedInfo[j].px = x;
		}//end for y,x ...

	return(1);
}//end Create.

void ColourPlaneDecoding::Destroy(void)
{
	if(_decodedInfo != NULL)
		delete[] _decodedInfo;
	_decodedInfo = NULL;

	if(_Ref != NULL)
		delete[] _Ref;
	_Ref = NULL;

}//end Destroy.

/** Decode a vector and add it to the ref.
Decode a vq index from the vlc, use the vq to get the vector and
add it to the _Ref member at the vecLocation. If vlc is NULL then
decode from the _decodedInfo member list. The 2nd variant will
iterate through all vec locations and decode from the _decodedInfo
list only (assumes _decodedInfo is pre-populated).
@param vecLocation	: At this location.
@param vq						: Use this vector quantiser.
@param vlc					: Use this vlc decoder.
@return							: None.
*/
void ColourPlaneDecoding::VectorDecode(int vecLocation, IVectorQuantiser* vq, 
																												IVlcDecoder*			vlc, 
																												IBitStreamReader*	bsr)
{
	int y = _decodedInfo[vecLocation].py;
	int x = _decodedInfo[vecLocation].px;

	int vqIndex;
	if( vlc == NULL)	// Decoded members pre-loaded.
	{
		if(!_decodedInfo[vecLocation].includeFlag)
			return;
		vqIndex = _decodedInfo[vecLocation].value;
	}//end if !vlc..
	else
	{
		vqIndex = vlc->Decode(bsr);
	}//end else...

	short*		pC = (short *)vq->InverseQuantise(vqIndex);
	cpdType*	ref;
	for(int k = 0; k < _vecHeight; k++)
	{
		ref	= &(_Ref[y+k][x]);
		for(int l = _vecWidth; l > 0; l--)
		{
			int v	= *(pC++) + *ref;
			if((v <= _max)&&(v >= _min))
				*(ref++) = (cpdType)v;
			else if(v > _max)
				*(ref++) = _max;
			else
				*(ref++) = _min;
		}//end for l...
	}//end for k...

}//end VectorDecode.

void ColourPlaneDecoding::VectorDecode(IVectorQuantiser* vq)
{
	for(int vecLocation = 0; (vecLocation < _vecLength)&&(!_decodedInfo[vecLocation].stop); vecLocation++)
	{
		if(_decodedInfo[vecLocation].includeFlag)
		{
			int y				= _decodedInfo[vecLocation].py;
			int x				= _decodedInfo[vecLocation].px;
			int vqIndex = _decodedInfo[vecLocation].value;

			short*		pC = (short *)vq->InverseQuantise(vqIndex);
			cpdType*	ref;
			for(int k = 0; k < _vecHeight; k++)
			{
				ref	= &(_Ref[y+k][x]);
				for(int l = _vecWidth; l > 0; l--)
				{
					cpdType v	= *(pC++) + *ref;
					if((v <= _max)&&(v >= _min))
						*(ref++) = v;
					else if(v > _max)
						*(ref++) = _max;
					else
						*(ref++) = _min;
				}//end for l...
			}//end for k...
		}//end if includeFlag...
	}//end for vecLocation...

}//end VectorDecode.

void ColourPlaneDecoding::ScalarDecode(void)
{
	for(int vecLocation = 0; vecLocation < _vecLength; vecLocation++)
	{
		if(_decodedInfo[vecLocation].includeFlag)
		{
			int y					= _decodedInfo[vecLocation].py;
			int x					= _decodedInfo[vecLocation].px;
			cpdType value	= _decodedInfo[vecLocation].value;

			cpdType*	ref;
			for(int k = 0; k < _vecHeight; k++)
			{
				ref	= &(_Ref[y+k][x]);
				for(int l = _vecWidth; l > 0; l--)
				{
					*(ref++) = value;
				}//end for l...
			}//end for k...
		}//end if includeFlag...
	}//end for vecLocation...
}//end ScalarDecode.

/** Set the max escape values.
This defines the number of escape bits and the mask to use during vlc decoding.
It is determined as the max number of vectors in the frame being coded.
@param	maxLength		: Max number of vectors.
@param	escNumBits	: Returned bit count.
@param  escBitMask	: Returned mask.
@return							:	none.
*/
void ColourPlaneDecoding::SetEsc(int maxLength, int* escNumBits, int* escBitMask)
{
	if(maxLength < 1)	// Out of range.
	{
		// Set defaults.
		*escNumBits		= 8;
		*escBitMask		= 0x000000FF;
		return;
	}//end if maxLength...

	int lclLength = maxLength - 1;

	int searchMask	= 0xFFFFFFFF;
	int notFound		= lclLength;
	int bitCount		= 0;

	while(notFound)
	{
		searchMask	= searchMask << 1;
		bitCount++;
		notFound		= lclLength & searchMask;
	}//end while notFound...

	*escNumBits		= bitCount;
	*escBitMask		= ~searchMask;
}//end SetEsc.
