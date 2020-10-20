/** @file

MODULE						: ColourPlaneEncoding

TAG								: CPE

FILE NAME					: ColourPlaneEncoding.h

DESCRIPTION				: A structure class to hold colour plane encodings as a
										utility class for image plane encoders. Operations
										are get/set and construction/destruction. Most members
										are therefore public. One instantiation per colour
										component is required.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _COLOURPLANEENCODING_H
#define _COLOURPLANEENCODING_H

#pragma once

#include "IVectorQuantiser.h"
#include "IVlcEncoder.h"

typedef short cpeType;

// There is one coded vector struct for each vector in the image.
typedef struct _CPE_CodedVector
{
	int		px;									// (x,y) coordinate of vector top-left corner.
	int		py;
	int		codedFlag;					// 1 = coded, 0 = not coded.
	int		includeFlag;				// 1 = included, 0 = excluded from further decisions.
	int		vqIndex;						// Codebook index address.
	int		vqDistortion;       // Codebook distortion.
	int		numCodeBits;				// Code word rate.
	int		codeWord;						// Code word.
	int		uncodedDistortion;	// Zero Vq distortion.
	int		weightedDistortion;	// Test against this distortion measure for inclusion.
} CPE_CodedVector;

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ColourPlaneEncoding
{
// Construction.
public:
	ColourPlaneEncoding(void);
	virtual ~ColourPlaneEncoding(void);

	int Create(	cpeType* src,				cpeType* ref, 
							int			 imgWidth,	int			 imgHeight, 
							int			 vecWidth,	int			 vecHeight);

	void Destroy(void);

// Operations.
public:
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
	void SetEncoding(int vecLocation, IVectorQuantiser* vq = NULL, IVlcEncoder* vlc = NULL);
	/** Set encoding members of coded info list.
	Get the difference vector between the src and ref and measure the
	distortion. Populate the appropriate struct members depending on
	the vector quantiser and/or vlc encoder passed in. For the entire
	vector list.
	@param vq						: Use this vector quantiser.
	@param vlc					: Use this vlc encoder.
	@return							: none.
	*/
	void SetEncoding(IVectorQuantiser* vq = NULL, IVlcEncoder* vlc = NULL);

// Member access.
public:
	cpeType** GetSrcImgAddr(void) { return(_Src); }
	cpeType** GetRefImgAddr(void) { return(_Ref); }
	cpeType*	GetSrcImg(void) { return(_pSrc); }
	cpeType*	GetRefImg(void) { return(_pRef); }
	void GetImgDim(int* width, int* height) { *width = _imgWidth; *height = _imgHeight; }
	void GetVecDim(int* width, int* height) { *width = _vecWidth; *height = _vecHeight; }
	int GetVecListLength(void) { return(_vecLength); }
	CPE_CodedVector* GetEncodingList(void) { return(_codedInfo); }
	int GetEscRunBits(void) { return(_ESC_RUN_BITS); }
	int GetEscRunMask(void) { return(_ESC_RUN_MASK); }
	int* GetThesholdList(void) { return(_threshold); }

private:
	static void SetEsc(int maxLength, int* escNumBits, int* escBitMask);

private:
	// Colour plane info.
	cpeType*					_pSrc;
	cpeType*					_pRef;
	cpeType**					_Src;	// Row address arrays.
	cpeType**					_Ref;
	int								_imgWidth;
	int								_imgHeight;

  // The vector parameters.
	int								_vecWidth;
	int								_vecHeight;
  int								_vecLength;	// Total no. of vectors.

  // Different size images have different total no. of vectors.	Act as constants.
  int								_ESC_RUN_BITS;
  int								_ESC_RUN_MASK;
	CPE_CodedVector*	_codedInfo; // Coded vector detail list.
	int								_threshold[32];

	// A help vector difference calc and vector quantisation.
	short*						_dVec;

};//end ColourPlaneEncoding.


#endif // _COLOURPLANEENCODING_H

