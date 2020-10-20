/** @file

MODULE						: MotionEstimatorH263ImplStd

TAG								: MEH263IS

FILE NAME					: MotionEstimatorH263ImplStd.h

DESCRIPTION				: A standard motion estimator implementation for 
										Recommendation H.263 of squared	error distortion 
										measure. No extended boundary or 4V modes. Access 
										via a IMotionEstimator interface.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONESTIMATORH263IMPLSTD_H
#define _MOTIONESTIMATORH263IMPLSTD_H

#include "IMotionEstimator.h"
#include "VectorStructList.h"
#include "OverlayMem2Dv2.h"

/*
---------------------------------------------------------------------------
	Struct definition.
---------------------------------------------------------------------------
*/
typedef struct _MEH263IS_COORD
{
	short int x;
	short int y;
} MEH263IS_COORD;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionEstimatorH263ImplStd : public IMotionEstimator
{
	// Construction.
	public:

		MotionEstimatorH263ImplStd(	const void* pSrc, 
																const void* pRef, 
																int					imgWidth, 
																int					imgHeight);

		virtual ~MotionEstimatorH263ImplStd(void);

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
		is at the limit of its search range or the compensated macroblock falls
		outside the img boundary. Global range = [-16..15]. Note there is always 
		a pel available for half calc in the positive direction.
		@param compx		: X compensated top-left block coord.
		@param compy		: Y compensated top-left block coord.
		@param vecx			: X motion vector coord.
		@param vecy			: Y motion vector coord.
		@param halfPos	: Returned correct struct.
		@return					: Length of the struct.
		*/
		int GetHalfPelSearchStruct(int compx,	int compy, 
															 int vecx,	int vecy,
															 MEH263IS_COORD**	ppHalfPos);

		/** Get the allowed motion range for this block.
		The search area for std H.263 is within the bounds of the image
		dimensions. The range is limited at the corners and edges of the
		images.
		@param x				: Top left x coord of block.
		@param y				: Top left y coord of block.
		@param xlr			: Returned allowed x left range.
		@param xrr			: Returned allowed x right range.
		@param yur			: Returned allowed y up range.
		@param ydr			: Returned allowed y down range.
		@return					: none.
		*/
		void GetMotionRange(int  x,		int  y, 
												int* xlr, int* xrr, 
												int* yur, int* ydr);

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
		OverlayMem2Dv2*			_pRefOver;				// Ref overlay with motion block dim.

		// Temp working block and its overlay.
		short*							_pMBlk;						// Motion block temp mem.
		OverlayMem2Dv2*			_pMBlkOver;				// Motion block overlay of temp mem.

		// Hold the resulting motion vectors in a byte array.
		VectorStructList*	_pMotionVectorStruct;

};//end MotionEstimatorH263ImplStd.


#endif	// _MOTIONESTIMATORH263IMPLSTD_H
