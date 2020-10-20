/** @file

MODULE						: ImagePlaneEncoder

TAG								: IPE

FILE NAME					: ImagePlaneEncoder.h

DESCRIPTION				: A base class with the common implementations for a 
										family of implementations to quantise, encode and 
										write sequentially to a bit stream each colour plane 
										in an image. The colour plane info is held in 
										ColourPlaneEncoding	objects. The process is as follows:
											// Instantiate.
											ImagePlaneEncoder* p = new ImagePlaneEncoderImpl();
											p->Create();
											p->SetEndOfPlaneMarkerVlc();
											p->SetEndOfImageMarkerVlc();
											.
											.
											// Attach utility classes.
											p->AttachVectorQuantiser();
											.
											.
											p->AttachBitStreamWriter();
											.
											.
											// Use.
											p->Encode();
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
#ifndef _IMAGEPLANEENCODER_H
#define _IMAGEPLANEENCODER_H

#include "IVectorQuantiser.h"
#include "IVlcEncoder.h"
#include "ColourPlaneEncoding.h"
#include "BitStreamWriter.h"

/*
---------------------------------------------------------------------------
	Class constants.
---------------------------------------------------------------------------
*/
#define IPE_LUM							0
#define IPE_CHRU						1
#define IPE_CHRV						2
#define IPE_COLOUR_PLANES		3

/*
---------------------------------------------------------------------------
	Class definitions.
---------------------------------------------------------------------------
*/
class ImagePlaneEncoder
{
// Construction.
public:
	ImagePlaneEncoder(void);
	virtual ~ImagePlaneEncoder(void);

	virtual int Create(	cpeType* srcLum,			cpeType* refLum, 
											int			 lumWidth,		int			 lumHeight,
											cpeType* srcChrU,			cpeType* refChrU,
											cpeType* srcChrV,			cpeType* refChrV,
											int			 chrWidth,		int			 chrHeight,
											int			 vecLumWidth,	int			 vecLumHeight,
											int			 vecChrWidth,	int			 vecChrHeight);

	virtual void Destroy(void);

// Operations.
public:
	/** Encode the image.
	To be implemented by each implementation. No base implementation provided.
	Add up to allowedBits of quantised and vlc'ed samples to the bit stream
	in sequential colour planes with an endOfPlaneMarkerCode delimiter.
	@param	allowedBits	: Stop before exceeding this bit limit.
	@param	bitsUsed		: Return the exact num of bits used.
	@return							: 0 = all coded, 1 = bit limit reached, 2 = coding failure.
	*/
	virtual int Encode(int allowedBits, int* bitsUsed) = 0;

// Member access.
public:
	void AttachVectorQuantiser(IVectorQuantiser* vq)	{ _pVectorQuantiser = vq; }
	void AttachRunVlcEncoder(IVlcEncoder* ve)					{ _pRunVlcEncoder = ve; }
	void AttachVqVlcEncoder(IVlcEncoder* ve)					{ _pVqVlcEncoder = ve; }
	void AttachIntraVlcEncoder(IVlcEncoder* ve)				{ _pIntraVlcEncoder = ve; }
	void AttachBitStreamWriter(BitStreamWriter* bsw)	{ _pBitStreamWriter = bsw; }

	void SetEndOfPlaneMarkerVlc(int code, int numBits)
	{ _endOfPlaneMarkerCode = code; _endOfPlaneMarkerNumBits = numBits; } 
	void SetEndOfImageMarkerVlc(int code, int numBits)
	{ _endOfImageMarkerCode = code; _endOfImageMarkerNumBits = numBits; } 

// Utility methods common to all implementations.
protected:
	int		WriteToStreamWithDecode(int allowedBits, int* bitsUsed);
	void	SetPelValueRange(cpeType min, cpeType max);

// Marker codes.
protected:
	int _endOfPlaneMarkerCode;
	int _endOfPlaneMarkerNumBits;
	int _endOfImageMarkerCode;
	int _endOfImageMarkerNumBits;

// Members.
protected:
	// All info is held in a ColourPlaneEncoding for each colour.
	ColourPlaneEncoding*	_pColourPlane[IPE_COLOUR_PLANES];

	// References to helper classes that are not owned by this.
	IVectorQuantiser*			_pVectorQuantiser;
	IVlcEncoder*					_pRunVlcEncoder;
	IVlcEncoder*					_pVqVlcEncoder;
	IVlcEncoder*					_pIntraVlcEncoder;	// For INTRA coded images.
	// Bit stream to write the encoded bits to.
	BitStreamWriter*			_pBitStreamWriter;

	// A help vector for intermediate calcs.
	short*								_vec;

	// Pel value range.
	cpeType								_minPelValue;
	cpeType								_maxPelValue;
};//end ImagePlaneEncoder.


#endif // _IMAGEPLANEENCODER_H

