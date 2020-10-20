/** @file

MODULE						: ColourPlaneDecoding

TAG								: CPD

FILE NAME					: ColourPlaneDecoding.h

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
#ifndef _COLOURPLANEDECODING_H
#define _COLOURPLANEDECODING_H

#pragma once

#include "IVectorQuantiser.h"
#include "IVlcDecoder.h"

typedef short cpdType;

// There is one coded vector struct for each vector in the image.
typedef struct _CPD_DecodedVector
{
	int		px;									// (x,y) coordinate of vector top-left corner.
	int		py;
	int		includeFlag;				// 1 = included, 0 = excluded from further decisions.
	int   stop;								// 1 = terminate decoding here.
	int		value;							// Decoded symbol (codebook index address or avg value).
} CPD_DecodedVector;

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ColourPlaneDecoding
{
// Construction.
public:
	ColourPlaneDecoding(void);
	virtual ~ColourPlaneDecoding(void);

	int Create(cpdType* ref, int imgWidth, int imgHeight, int vecWidth, int vecHeight);

	void Destroy(void);

// Operations.
public:
	/** Decode a vector and add it to the ref.
	Decode a vq index from the vlc, use the vq to get the vector and
	add it to the _Ref member at the vecLocation. If vlc is NULL then
	decode from the _decodedInfo member list. The 2nd variant will
	iterate through all vec locations and decode from the _decodedInfo
	list only (assumes _decodedInfo is pre-populated).
	@param vecLocation	: At this location.
	@param vq						: Use this vector quantiser.
	@param vlc					: Use this vlc decoder.
	@param bsr					: The bit stream for vlc to access.
	@return							: None.
	*/
	void VectorDecode(int vecLocation,	IVectorQuantiser* vq, 
																			IVlcDecoder*			vlc = NULL,
																			IBitStreamReader*	bsr = NULL);
	void VectorDecode(IVectorQuantiser* vq);

	/** Decode a scalar and write it to the ref.
	Decode from the _decodedInfo member list. Iterate through all 
	vec locations and decode from the _decodedInfo	list only 
	(assumes _decodedInfo is pre-populated).
	@return	: None.
	*/
	void ScalarDecode(void);

// Member access.
public:
	cpdType** GetRefImgAddr(void) { return(_Ref); }
	cpdType*	GetRefImg(void) { return(_pRef); }
	void			GetImgDim(int* width, int* height) { *width = _imgWidth; *height = _imgHeight; }
	void			GetVecDim(int* width, int* height) { *width = _vecWidth; *height = _vecHeight; }
	int				GetVecListLength(void) { return(_vecLength); }
	CPD_DecodedVector* GetDecodingList(void) { return(_decodedInfo); }
	int				GetEscRunBits(void) { return(_ESC_RUN_BITS); }
	int				GetEscRunMask(void) { return(_ESC_RUN_MASK); }
	void			SetPelValueRange(cpdType min, cpdType max) { _min = min; _max = max; }

private:
	static void SetEsc(int maxLength, int* escNumBits, int* escBitMask);

private:
	// Colour plane info.
	cpdType*					_pRef;
	cpdType**					_Ref;
	int								_imgWidth;
	int								_imgHeight;
	cpdType						_min;
	cpdType						_max;

  // The vector parameters.
	int								_vecWidth;
	int								_vecHeight;
  int								_vecLength;	// Total no. of vectors.

  // Different size images have different total no. of vectors.	Act as constants.
  int									_ESC_RUN_BITS;
  int									_ESC_RUN_MASK;
	CPD_DecodedVector*	_decodedInfo; // Coded vector detail list.

};//end ColourPlaneDecoding.


#endif // _COLOURPLANEDECODING_H

