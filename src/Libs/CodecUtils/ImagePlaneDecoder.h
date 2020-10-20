/** @file

MODULE						: ImagePlaneDecoder

TAG								: IPD

FILE NAME					: ImagePlaneDecoder.h

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
#ifndef _IMAGEPLANEDECODER_H
#define _IMAGEPLANEDECODER_H

#include "IVectorQuantiser.h"
#include "IVlcDecoder.h"
#include "ColourPlaneDecoding.h"
#include "BitStreamReader.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define IPD_LUM							0
#define IPD_CHRU						1
#define IPD_CHRV						2
#define IPD_COLOUR_PLANES		3

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ImagePlaneDecoder
{
// Construction.
public:
	ImagePlaneDecoder(void);
	virtual ~ImagePlaneDecoder(void);

	virtual int Create(	cpdType* refLum, 
											int			 lumWidth,		int			 lumHeight,
											cpdType* refChrU,
											cpdType* refChrV,
											int			 chrWidth,		int			 chrHeight,
											int			 vecLumWidth,	int			 vecLumHeight,
											int			 vecChrWidth,	int			 vecChrHeight);

	virtual void Destroy(void);

// Operations.
public:
	/** Decode the image.
	To be implemented by each implementation. No base implementation provided.
	Take up to allowedBits of quantised and vlc'ed samples from the bit stream
	in sequential colour planes with an endOfPlaneMarkerCode delimiter.
	@param	allowedBits	: Stop before exceeding this bit limit.
	@param	bitsUsed		: Return the exact num of bits used.
	@return							: 0 = all decoded, 1 = bit limit reached, 2 = coding failure.
	*/
	virtual int Decode(int allowedBits, int* bitsUsed) = 0;

// Member access.
public:
	void AttachVectorQuantiser(IVectorQuantiser* vq)	{ _pVectorQuantiser = vq; }
	void AttachRunVlcDecoder(IVlcDecoder* ve)					{ _pRunVlcDecoder = ve; }
	void AttachVqVlcDecoder(IVlcDecoder* ve)					{ _pVqVlcDecoder = ve; }
	void AttachIntraVlcDecoder(IVlcDecoder* ve)				{ _pIntraVlcDecoder = ve; }
	void AttachBitStreamReader(BitStreamReader* bsr)	{ _pBitStreamReader = bsr; }

	void SetEndOfPlaneMarker(int endOfPlane)	{ _endOfPlaneMarker = endOfPlane; } 
	void SetEndOfImageMarker(int endOfImage)	{ _endOfImageMarker = endOfImage; } 

// Utility methods common to all implementations.
protected:
	void	SetPelValueRange(cpdType min, cpdType max);

// Marker codes.
protected:
	int _endOfPlaneMarker;
	int _endOfImageMarker;

// Members.
protected:
	// All info is held in a ColourPlaneDecoding for each colour.
	ColourPlaneDecoding*	_pColourPlane[IPD_COLOUR_PLANES];

	// References to helper classes that are not owned by this.
	IVectorQuantiser*			_pVectorQuantiser;
	IVlcDecoder*					_pRunVlcDecoder;
	IVlcDecoder*					_pVqVlcDecoder;
	IVlcDecoder*					_pIntraVlcDecoder;	// For INTRA coded images.
	// Bit stream to read the encoded bits from.
	BitStreamReader*			_pBitStreamReader;

	// Pel value range.
	cpdType								_minPelValue;
	cpdType								_maxPelValue;
};//end ImagePlaneDecoder.


#endif // _IMAGEPLANEDECODER_H

