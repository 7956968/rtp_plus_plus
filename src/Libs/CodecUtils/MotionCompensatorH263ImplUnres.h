/** @file

MODULE						: MotionCompensatorH263ImplUnres

TAG								: MCH263IU

FILE NAME					: MotionCompensatorH263ImplUnres.h

DESCRIPTION				: An unrestricted motion compensator implementation for 
										Recommendation H.263 (02/98) Annex D page 53.	Access via a 
										IMotionCompensator interface. The boundary is extended to 
										accomodate the selected motion range.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONCOMPENSATORH263IMPLUNRES_H
#define _MOTIONCOMPENSATORH263IMPLUNRES_H

#include "IMotionCompensator.h"
#include "VectorStructList.h"
#include "OverlayMem2Dv2.h"
#include "OverlayExtMem2Dv2.h"

typedef short mciuType;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionCompensatorH263ImplUnres : public IMotionCompensator
{
	// Construction.
	public:

		MotionCompensatorH263ImplUnres(int range);
		virtual ~MotionCompensatorH263ImplUnres(void);

	// IMotionCompensator Interface.
	public:
		
		virtual int	Create(	void* ref, 
												int imgWidth,	int imgHeight, 
												int macroBlkWidth, int macroBlkHeight);
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
		int _range;						// Max range of the motion [-_range ... (_range-1)].
    int _invalid;         // Invalidate the compensation to force copying on the next compensated vector. Cleared after use.

		int _refSize;					// Total size (in mcisType) of the contiguous Lum, ChrU, ChrV.
		// Ref image holders.
		mciuType*		_pRefLum;		// References to the images at creation.
		mciuType*		_pRefChrU;
		mciuType*		_pRefChrV;

		// Overlays to the ref image.
		OverlayMem2Dv2*	_pRefLumOver;
		OverlayMem2Dv2*	_pRefChrUOver;
		OverlayMem2Dv2*	_pRefChrVOver;

		// Extended temp ref mem created by ExtendBoundary() calls for each colour
		// component and an extended boundary overlay for each.
		mciuType*					_pExtTmpLum;
		OverlayExtMem2Dv2*	_pExtTmpLumOver;
		int								_extLumWidth;
		int								_extLumHeight;
		mciuType*					_pExtTmpChrU;
		OverlayExtMem2Dv2*	_pExtTmpChrUOver;
		mciuType*					_pExtTmpChrV;
		OverlayExtMem2Dv2*	_pExtTmpChrVOver;
		int								_extChrWidth;
		int								_extChrHeight;

		// A work block.
		mciuType*			_pMBlk;
		OverlayMem2Dv2* _pMBlkOver;
};//end MotionCompensatorH263ImplUnres.


#endif	// _MOTIONCOMPENSATORH263IMPLUNRES_H
