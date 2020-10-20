/** @file

MODULE						: MotionCompensatorImpl1

TAG								: MCI1

FILE NAME					: MotionCompensatorImpl1.h

DESCRIPTION				: A standard motion compensator implementation of YUV420
										planar image reference.	Access via a IMotionCompensator
										interface.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONCOMPENSATORIMPL1_H
#define _MOTIONCOMPENSATORIMPL1_H

#include "IMotionCompensator.h"
#include "VectorList.h"

typedef short mciType;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionCompensatorImpl1 : public IMotionCompensator
{
	// Construction.
	public:

		MotionCompensatorImpl1(void);
		virtual ~MotionCompensatorImpl1(void);

	// IMotionCompensator Interface.
	public:
		
		virtual int	Create(void* ref, int imgWidth,				int imgHeight, 
																	int macroBlkWidth,	int macroBlkHeight);
		virtual void	Destroy(void);
		virtual void	Reset(void);

		/** Motion compensate to the reference.
		Do the compensation with the block sizes and image sizes defined in
		the implementation and set in Create().
		@param pMotionList	: The list of motion vectors.
		@return							: None.
		*/
		virtual void Compensate(void* pMotionList);

		// Methods not implemented.
		virtual void Compensate(int tlx, int tly, int mvx, int mvy) {}
		virtual void PrepareForSingleVectorMode(void) {}

	// Local methods.
	protected:

		// Parameters must remain const for the life time of this instantiation.
		int	_imgWidth;				// Width of the ref images. 
		int	_imgHeight;				// Height of the ref images.
		int _chrWidth;
		int _chrHeight;
		int	_macroBlkWidth;		// Width of the motion block.
		int	_macroBlkHeight;	// Height of the motion block.
		int _chrMacroBlkWidth;
		int _chrMacroBlkHeight;

		int _refSize;					// Total size (in mciType) of the contiguous Lum, ChrU, ChrV.
		// Ref image holders.
		mciType*	_pRefLum;		// References to the images at creation.
		mciType**	_refLum;		// Address arrays.
		mciType*	_pRefChrU;
		mciType** _refChrU;
		mciType*	_pRefChrV;
		mciType** _refChrV;
		// Temp image store.
		mciType*	_pTmpLum;
		mciType**	_tmpLum;
		mciType*	_pTmpChrU;
		mciType** _tmpChrU;
		mciType*	_pTmpChrV;
		mciType** _tmpChrV;
};//end MotionCompensatorImpl1.


#endif	// _MOTIONCOMPENSATORIMPL1_H
