/** @file

MODULE						: MotionCompensatorH263ImplStd

TAG								: MCH263IS

FILE NAME					: MotionCompensatorH263ImplStd.h

DESCRIPTION				: A standard motion compensator implementation for boundary
										limited H.263 of YUV420 planar image reference.	Access via 
										an IMotionCompensator	interface.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONCOMPENSATORH263IMPLSTD_H
#define _MOTIONCOMPENSATORH263IMPLSTD_H

#include "IMotionCompensator.h"
#include "VectorStructList.h"
#include "OverlayMem2Dv2.h"

typedef short mcisType;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionCompensatorH263ImplStd : public IMotionCompensator
{
	// Construction.
	public:

		MotionCompensatorH263ImplStd(void);
		virtual ~MotionCompensatorH263ImplStd(void);

	// IMotionCompensator Interface.
	public:
		
		virtual int	Create(void* ref, int imgWidth,	int imgHeight, int macroBlkWidth, int macroBlkHeight);
		virtual void	Destroy(void);
		virtual void	Reset(void);

		/** Motion compensate to the reference.
		Do the compensation with the block sizes and image sizes defined in
		the implementation and set in Create().
		@param pMotionList	: The list of motion vectors.
		@return							: None.
		*/
		virtual void Compensate(void* pMotionList);

		/** Motion compensate a single vector to the reference.
		Do the compensation with the block sizes and image sizes defined in
		the implementation and set in Create(). The vector coords are in 
		half pel units for this implementation. NOTE: The temp image must
		hold a copy of the ref BEFORE using this method and is should be done
		in PrepareForSingleVectorMode().
		@param tlx	: Top left x coord of block.
		@param tly	: Top left y coord of block.
		@param mvx	: X coord of the motion vector.
		@param mvy	: Y coord of the motion vector.
		@return			: None.
		*/
		virtual void Compensate(int tlx, int tly, int mvx, int mvy);

		/** Prepare the ref for single motion vector compensation mode.
		Should be used to copy the ref into a temp location from which to
		do the compensation to the ref. Prevents interference and double
		compensation.
		@return : none.
		*/
		virtual void PrepareForSingleVectorMode(void);
    virtual void Invalidate(void) { _invalid = 1; }

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
    int _invalid;         // Invalidate the compensation to force copying on the next compensated vector. Cleared after use.

		int _refSize;					// Total size (in mcisType) of the contiguous Lum, ChrU, ChrV.
		// Ref image holders.
		mcisType*		_pRefLum;		// References to the images at creation.
		mcisType*		_pRefChrU;
		mcisType*		_pRefChrV;
		// Temp image store.
		mcisType*		_pTmpLum;
		mcisType*		_pTmpChrU;
		mcisType*		_pTmpChrV;

		// Overlays to the images
		OverlayMem2Dv2*	_pRefLumOver;
		OverlayMem2Dv2*	_pRefChrUOver;
		OverlayMem2Dv2*	_pRefChrVOver;
		OverlayMem2Dv2* _pTmpLumOver;
		OverlayMem2Dv2* _pTmpChrUOver;
		OverlayMem2Dv2* _pTmpChrVOver;

		// A work block.
		mcisType*			_pMBlk;
		OverlayMem2Dv2* _pMBlkOver;
};//end MotionCompensatorH263ImplStd.


#endif	// _MOTIONCOMPENSATORH263IMPLSTD_H
