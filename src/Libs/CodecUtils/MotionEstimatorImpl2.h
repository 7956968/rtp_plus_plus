/** @file

MODULE						: MotionEstimatorImpl2

TAG								: MEI2

FILE NAME					: MotionEstimatorImpl2.h

DESCRIPTION				: A fast motion estimator implementation of absolute
										error difference measure.	Access via a IMotionEstimator
										interface. There are 2 levels of execution speed vs.
										quality.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONESTIMATORIMPL2_H
#define _MOTIONESTIMATORIMPL2_H

#include "IMotionEstimator.h"
#include "VectorList.h"
#include "OverlayMem2Dv2.h"
#include "OverlayExtMem2Dv2.h"

/*
---------------------------------------------------------------------------
	Struct definition.
---------------------------------------------------------------------------
*/
typedef struct _MEI2_COORD
{
	short int x;
	short int y;
} MEI2_COORD;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionEstimatorImpl2 : public IMotionEstimator
{
// Construction.
public:

	MotionEstimatorImpl2(	const void* pSrc, 
												const void* pRef, 
												int					imgWidth, 
												int					imgHeight,
												int					macroBlkWidth,
												int					macroBlkHeight,
												int					motionRange);

	virtual ~MotionEstimatorImpl2(void);

// IMotionEstimator Interface.
public:
	
	virtual int		Create(void);
	virtual void	Reset(void);
	virtual int		Ready(void)		{ return(_ready); }
	virtual void	SetMode(int mode);
	virtual int		GetMode(void) { return(_mode); }

	/** Motion estimate the source within the reference.
	Do the estimation with the block sizes and image sizes defined in
	the implementation. The returned type holds the vectors.
	@param pSrc		: Input image to estimate.
	@param pRef		: Ref to estimate with.
	@return				: The list of motion vectors.
	*/
	virtual void* Estimate(const void* pSrc, const void* pRef, long* avgDistortion)
		{ return(Estimate(avgDistortion)); }
	virtual void* Estimate(long* avgDistortion);

// Local methods.
protected:

	// Clear alloc mem.
	void Destroy(void);

	/** Get the appropriate half pel position search struct.
	Get the correct struct depending on whether the current motion vector
	is at the limit of its search range. Global range = [-64..63].
	@param x				: X motion vector coord.
	@param y				: Y motion vector coord.
	@param halfPos	: Returned correct struct.
	@return					: Length of the struct.
	*/
	int GetHalfPelSearchStruct(int x, int y, MEI2_COORD** ppHalfPos);

protected:

	int _ready;	// Ready to estimate.
	int _mode;	// Speed mode or whatever. [ 0 = auto, 1 = level 1, 2 = level 2.]

	// Parameters must remain const for the life time of this instantiation.
	int	_imgWidth;				// Width of the src and ref images. 
	int	_imgHeight;				// Height of the src and ref images.
	int	_macroBlkWidth;		// Width of the motion block.
	int	_macroBlkHeight;	// Height of the motion block.
	int	_motionRange;			// (x,y) range of the motion vectors.

	const void*	_pInput;	// References to the images at construction.
	const void* _pRef;

	// Level 0: Input mem overlay members.
	OverlayMem2Dv2*		_pInOver;					// Input overlay with motion block dim.
	// Level 1: Subsampled input by 2. [_mode == 1]
	short*						_pInL1;						// Input mem at (_l1Width * _l1Height).
	int								_l1Width;
	int								_l1Height;
	int								_l1MacroBlkWidth;
	int								_l1MacroBlkHeight;
	int								_l1MotionRange;
	OverlayMem2Dv2*		_pInL1Over;				// Input overlay with level 1 motion block dim.
	// Level 2: Subsampled input by 4.	[_mode == 2]
	short*						_pInL2;						// Input mem at (_l2Width * _l2Height).
	int								_l2Width;
	int								_l2Height;
	int								_l2MacroBlkWidth;
	int								_l2MacroBlkHeight;
	int								_l2MotionRange;
	OverlayMem2Dv2*		_pInL2Over;				// Input overlay with level 2 motion block dim.

	// Level 0: Ref mem overlay members.
	OverlayMem2Dv2*			_pRefOver;				// Ref overlay with whole block dim.
	short*							_pExtRef;					// Extended ref mem created by ExtendBoundary() call.
	int									_extWidth;
	int									_extHeight;
	OverlayExtMem2Dv2*	_pExtRefOverAll;	// Extended ref overlay with whole block dim.
	OverlayExtMem2Dv2*	_pExtRefOver;			// Extended ref overlay with motion block dim.
	// Level 1: Subsampled ref by 2.		[_mode == 1]
	short*							_pRefL1;					// Ref mem at (_l1Width * _l1Height).
	OverlayMem2Dv2*			_pRefL1Over; 			// Ref overlay with whole level 1 block dim.
	short*							_pExtRefL1;				// Extended ref mem create by ExtendBoundary() call.
	int									_extL1Width;
	int									_extL1Height;
	OverlayExtMem2Dv2*	_pExtRefL1OverAll;// Ref overlay with whole block dim.
	OverlayExtMem2Dv2*	_pExtRefL1Over;		// Ref overlay with level 1 motion block dim.
	// Level 2: Subsampled ref by 4.		[_mode == 2]
	short*							_pRefL2;					// Ref mem at (_l2Width * _l2Height).
	OverlayMem2Dv2*			_pRefL2Over; 			// Ref overlay with whole level 2 block dim.
	short*							_pExtRefL2;				// Extended ref mem create by ExtendBoundary() call.
	int									_extL2Width;
	int									_extL2Height;
	OverlayExtMem2Dv2*	_pExtRefL2OverAll;// Ref overlay with whole block dim.
	OverlayExtMem2Dv2*	_pExtRefL2Over;		// Ref overlay with level 2 motion block dim.

	// Temp working block and its overlay.
	short*							_pMBlk;						// Motion block temp mem.
	OverlayMem2Dv2*			_pMBlkOver;				// Motion block overlay of temp mem.

	// Hold the resulting motion vectors in a byte array.
	VectorList*				_pMotionVectorStruct;

};//end MotionEstimatorImpl2.


#endif // !_MOTIONESTIMATORIMPL2_H

