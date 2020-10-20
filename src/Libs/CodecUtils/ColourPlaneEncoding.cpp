/** @file

MODULE						: ColourPlaneEncoding

TAG								: CPE

FILE NAME					: ColourPlaneEncoding.cpp

DESCRIPTION				: A structure class to hold colour plane encodings as a
										utility class for image plane encoders. Operations
										are get/set and construction/destruction. One instantiation 
										per colour component is required.

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

#include	"ColourPlaneEncoding.h"

/*
--------------------------------------------------------------------------
  Constants. 
--------------------------------------------------------------------------
*/

ColourPlaneEncoding::ColourPlaneEncoding(void)
{
  _vecLength			= 0;
  // Different size images have different total no. of vectors.	Act as constants.
  _ESC_RUN_BITS		= 0;
  _ESC_RUN_MASK		= 0;
	_codedInfo			= NULL; // Coded vector detail list.
	for(int i = 0; i < 32; i++)
		_threshold[i] = 0;
	// Colour plane info.
	_pSrc						= NULL;
	_pRef						= NULL;
	_Src						= NULL;
	_Ref						= NULL;
	_imgWidth				= 0;
	_imgHeight			= 0;
  // The vector parameters.
	_vecWidth				= 0;
	_vecHeight			= 0;
	_dVec						= NULL;
}//end constructor.

ColourPlaneEncoding::~ColourPlaneEncoding(void)
{
	Destroy();
}//end destructor.

int ColourPlaneEncoding::Create(cpeType*	src,			cpeType*	ref, 
																int				imgWidth, int				imgHeight, 
																int				vecWidth, int				vecHeight)
{
	// Clear out any old baggage.
	Destroy();

	// Bounds check.
	if( (src == NULL)||(ref == NULL)||
			(imgWidth < 1)||(imgHeight < 1)||
			(vecWidth < 1)|| (vecHeight < 1) )
		return(0);

	// Load up knowns.
	_pSrc				= src;
	_pRef				= ref;
	_imgWidth		= imgWidth;
	_imgHeight	= imgHeight;
	_vecWidth		= vecWidth;
	_vecHeight	= vecHeight;

	// Construct address arrays.
	_Src = new cpeType* [imgWidth * imgHeight];
	_Ref = new cpeType* [imgWidth * imgHeight];
	if( (_Src == NULL)||(_Ref == NULL) )
	{
		Destroy();
		return(0);
	}//end if !_Src...

	// Load address arrays.
	for(int i = 0; i < imgHeight; i++)
	{
		_Src[i] = &(src[imgWidth * i]);
		_Ref[i] = &(ref[imgWidth * i]);
	}//end for i...

	// 2-D vector (macroblock) assemblies.
	_vecLength = (imgWidth / vecWidth) * (imgHeight / vecHeight);
	_codedInfo = new CPE_CodedVector[_vecLength];
	if(_codedInfo == NULL)
	{
		Destroy();
		return(0);
	}//end if !_codedInfo...

	SetEsc(_vecLength, &_ESC_RUN_BITS, &_ESC_RUN_MASK);

	// Load top left corner location in imgs.
	int j = 0;
	for(int y = 0; y < imgHeight; y += vecHeight)
		for(int x = 0; x < imgWidth; x += vecWidth, j++)
		{
			_codedInfo[j].py = y;
			_codedInfo[j].px = x;
		}//end for y,x ...

	// A helper vector for intermediate calc.
	_dVec = new short[vecWidth * vecHeight];
	if(_dVec == NULL)
	{
		Destroy();
		return(0);
	}//end if !_dVec...

	return(1);
}//end Create.

void ColourPlaneEncoding::Destroy(void)
{
	if(_codedInfo != NULL)
		delete[] _codedInfo;
	_codedInfo = NULL;

	if(_Src != NULL)
		delete[] _Src;
	_Src = NULL;

	if(_Ref != NULL)
		delete[] _Ref;
	_Ref = NULL;

	if(_dVec != NULL)
		delete[] _dVec;
	_dVec = NULL;

}//end Destroy.

/** Set encoding members of coded info list.
Get the difference vector between the src and ref and measure the
distortion. Populate the appropriate struct members depending on
the vector quantiser and/or vlc encoder passed in. Only for a single
vector in the list.
@param vecLocation	: At this location.
@param vq						: Use this vector quantiser.
@param vlc					: Use this vlc encoder.
@return							: None.
*/
void ColourPlaneEncoding::SetEncoding(int vecLocation, IVectorQuantiser* vq, IVlcEncoder* vlc)
{
	int y = _codedInfo[vecLocation].py;
	int x = _codedInfo[vecLocation].px;

	// Unroll the loop and interleave for superscalar.
	short			d;
	cpeType*	src;
	cpeType*	ref;
	int				accum	= 0;
	short*		dV		= _dVec;
	for(int k = 0; k < _vecHeight; k++)
	{
		src	= &(_Src[y+k][x]);
		ref	= &(_Ref[y+k][x]);
		for(int l = (_vecWidth-1); l > 0; l--)
		{
			d				= *(src++) - *(ref++);
			*(dV++) = (short) d;
			accum	 += (d*d);
		}//end for l...
		d				= *(src) - *(ref);
		*(dV++) = (short) d;
		accum	 += (d*d);
	}//end for k...

	// Load the basics.
	_codedInfo[vecLocation].uncodedDistortion = accum;
	_codedInfo[vecLocation].codedFlag					= 0;	// Not yet coded.
	if(accum != 0)	// Zero difference vectors.
		_codedInfo[vecLocation].includeFlag = 1;
	else	// ..are excluded for further consideration.
		_codedInfo[vecLocation].includeFlag	= 0;

	// Encode with vq and vlc.
	if( (vq != NULL)&&(vlc != NULL) )
	{
    _codedInfo[vecLocation].vqIndex							= vq->Quantise(_dVec,&(_codedInfo[vecLocation].vqDistortion));
		_codedInfo[vecLocation].numCodeBits					= vlc->Encode(_codedInfo[vecLocation].vqIndex);
		_codedInfo[vecLocation].codeWord						= vlc->GetCode();
		_codedInfo[vecLocation].weightedDistortion	= _codedInfo[vecLocation].uncodedDistortion - _codedInfo[vecLocation].vqDistortion;
		// Make sure we are making an improvement.
		if(_codedInfo[vecLocation].weightedDistortion <= 0)
			_codedInfo[vecLocation].includeFlag = 0;
	}//end if vq...
	else
		_codedInfo[vecLocation].weightedDistortion = accum;	// Needs a default.

}//end SetEncoding.

/** Set encoding members of coded info list.
Get the difference vector between the src and ref and measure the
distortion. Populate the appropriate struct members depending on
the vector quantiser and/or vlc encoder passed in. For the entire
vector list.
@param vq						: Use this vector quantiser.
@param vlc					: Use this vlc encoder.
@return							: None.
*/
void ColourPlaneEncoding::SetEncoding(IVectorQuantiser* vq, IVlcEncoder* vlc)
{
	for(int vecLocation = 0; vecLocation < _vecLength; vecLocation++)
	{
		int y = _codedInfo[vecLocation].py;
		int x = _codedInfo[vecLocation].px;

		// Unroll the loop and interleave for superscalar.
		short			d;
		cpeType*	src;
		cpeType*	ref;
		int				accum	= 0;
		short*		dV		= _dVec;
		for(int k = 0; k < _vecHeight; k++)
		{
			src	= &(_Src[y+k][x]);
			ref	= &(_Ref[y+k][x]);
			for(int l = (_vecWidth-1); l > 0; l--)
			{
				d				= *(src++) - *(ref++);
				*(dV++) = (short) d;
				accum	 += (d*d);
			}//end for l...
			d				= *(src) - *(ref);
			*(dV++) = (short) d;
			accum	 += (d*d);
		}//end for k...

		// Load the basics.
		_codedInfo[vecLocation].uncodedDistortion = accum;
		_codedInfo[vecLocation].codedFlag					= 0;	// Not yet coded.
		if(accum != 0)	// Zero difference vectors.
			_codedInfo[vecLocation].includeFlag = 1;
		else	// ..are excluded for further consideration.
			_codedInfo[vecLocation].includeFlag	= 0;

		// Encode with vq and vlc.
		if( (vq != NULL)&&(vlc != NULL) )
		{
			_codedInfo[vecLocation].vqIndex							= vq->Quantise(_dVec,&(_codedInfo[vecLocation].vqDistortion));
			_codedInfo[vecLocation].numCodeBits					= vlc->Encode(_codedInfo[vecLocation].vqIndex);
			_codedInfo[vecLocation].codeWord						= vlc->GetCode();
			_codedInfo[vecLocation].weightedDistortion	= _codedInfo[vecLocation].uncodedDistortion - _codedInfo[vecLocation].vqDistortion;
			// Make sure we are making an improvement.
			if(_codedInfo[vecLocation].weightedDistortion <= 0)
				_codedInfo[vecLocation].includeFlag = 0;
		}//end if vq...
		else
			_codedInfo[vecLocation].weightedDistortion = accum;	// Needs a default.

	}//end vecLocation...

}//end SetEncoding.

/** Set the max escape values.
This defines the number of escape bits and the mask to use during vlc encoding.
It is determined as the max number of vectors in the frame being coded.
@param	maxLength		: Max number of vectors.
@param	escNumBits	: Returned bit count.
@param  escBitMask	: Returned mask.
@return							:	none.
*/
void ColourPlaneEncoding::SetEsc(int maxLength, int* escNumBits, int* escBitMask)
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
