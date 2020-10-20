/** @file

MODULE						: MotionEstimatorImpl1

TAG								: MEI1

FILE NAME					: MotionEstimatorImpl1.h

DESCRIPTION				: A standard motion estimator implementation of squared
										error distortion measure.	Access via a IMotionEstimator
										interface.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONESTIMATORIMPL1_H
#define _MOTIONESTIMATORIMPL1_H

#include "IMotionEstimator.h"
#include "VectorList.h"
#include "OverlayMem2Dv2.h"
#include "OverlayExtMem2Dv2.h"

/*
---------------------------------------------------------------------------
	Struct definition.
---------------------------------------------------------------------------
*/
typedef struct _MEI1_COORD
{
	short int x;
	short int y;
} MEI1_COORD;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionEstimatorImpl1 : public IMotionEstimator
{
	// Construction.
	public:

		MotionEstimatorImpl1(	const void* pSrc, 
													const void* pRef, 
													int					imgWidth, 
													int					imgHeight,
													int					macroBlkWidth,
													int					macroBlkHeight,
													int					motionRange);

		virtual ~MotionEstimatorImpl1(void);

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
		int GetHalfPelSearchStruct(int x, int y, MEI1_COORD** ppHalfPos);

	protected:

		int _ready;	// Ready to estimate.
		int _mode;	// Speed mode or whatever.

		// Parameters must remain const for the life time of this instantiation.
		int	_imgWidth;				// Width of the src and ref images. 
		int	_imgHeight;				// Height of the src and ref images.
		int	_macroBlkWidth;		// Width of the motion block.
		int	_macroBlkHeight;	// Height of the motion block.
		int	_motionRange;			// (x,y) range of the motion vectors.

		const void*	_pInput;	// References to the images at construction.
		const void* _pRef;

	// Input mem overlay members.
	OverlayMem2Dv2*			_pInOver;					// Input overlay with motion block dim.

	// Ref mem overlay members.
	OverlayMem2Dv2*			_pRefOver;				// Ref overlay with whole block dim.
	short*							_pExtRef;					// Extended ref mem created by ExtendBoundary() call.
	int									_extWidth;
	int									_extHeight;
	OverlayExtMem2Dv2*	_pExtRefOverAll;	// Extended ref overlay with whole block dim.
	OverlayExtMem2Dv2*	_pExtRefOver;			// Extended ref overlay with motion block dim.

	// Temp working block and its overlay.
	short*							_pMBlk;						// Motion block temp mem.
	OverlayMem2Dv2*			_pMBlkOver;				// Motion block overlay of temp mem.

	// Hold the resulting motion vectors in a byte array.
	VectorList*				_pMotionVectorStruct;

};//end MotionEstimatorImpl1.


#endif
