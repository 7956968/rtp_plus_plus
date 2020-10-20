/** @file

MODULE						: MotionEstimatorH263ImplUnres

TAG								: MEH263IU

FILE NAME					: MotionEstimatorH263ImplUnres.h

DESCRIPTION				: A fast unrestricted motion estimator implementation for 
										Recommendation H.263 (02/98) Annex D page 53 of absolute 
										error difference measure.	Access via a IMotionEstimator	
										interface. There are 2 mode levels of execution speed vs. 
										quality. The boundary is extended to accomodate the selected 
										motion range.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONESTIMATORH263IMPLUNRES_H
#define _MOTIONESTIMATORH263IMPLUNRES_H

#include "IMotionEstimator.h"
#include "VectorStructList.h"
#include "OverlayMem2Dv2.h"
#include "OverlayExtMem2Dv2.h"

/*
---------------------------------------------------------------------------
	Struct definition.
---------------------------------------------------------------------------
*/
typedef struct _MEH263IU_COORD
{
	short int x;
	short int y;
} MEH263IU_COORD;

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionEstimatorH263ImplUnres : public IMotionEstimator
{
// Construction.
public:

	MotionEstimatorH263ImplUnres(	const void* pSrc, 
																const void* pRef, 
																int					imgWidth, 
																int					imgHeight,
																int					motionRange);

	MotionEstimatorH263ImplUnres(	const void* pSrc, 
																const void* pRef, 
																int					imgWidth, 
																int					imgHeight,
																int					motionRange,
																void*				pDistortionIncluded);

	virtual ~MotionEstimatorH263ImplUnres(void);

/// IMotionEstimator Interface.
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

/// Local methods.
protected:

	/// Used by constructors to reset every member.
	void ResetMembers(void);
	/// Clear alloc mem.
	void Destroy(void);

	/** Get the appropriate half pel position search struct.
	Get the correct struct depending on whether the current motion vector
	is at the limit of its search range. Global range = [-64..63].
	@param x				: X motion vector coord.
	@param y				: Y motion vector coord.
	@param halfPos	: Returned correct struct.
	@return					: Length of the struct.
	*/
	int GetHalfPelSearchStruct(int x, int y, MEH263IU_COORD** ppHalfPos);

	void GetMotionRange(int		x,			int		y, 
											int*	xlr,		int*	xrr, 
											int*	yur,		int*	ydr, 
											int		range,	int		level); 

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
	int									_extBoundary;			// Extended boundary for left, right, up and down.
	OverlayExtMem2Dv2*	_pExtRefOver;			// Extended ref overlay with motion block dim.
	// Level 1: Subsampled ref by 2.		[_mode == 1]
	short*							_pRefL1;					// Ref mem at (_l1Width * _l1Height).
	OverlayMem2Dv2*			_pRefL1Over; 			// Ref overlay with whole level 1 block dim.
	short*							_pExtRefL1;				// Extended ref mem create by ExtendBoundary() call.
	int									_extL1Width;
	int									_extL1Height;
	int									_extL1Boundary;		// Extended boundary for left, right, up and down.
	OverlayExtMem2Dv2*	_pExtRefL1Over;		// Ref overlay with level 1 motion block dim.
	// Level 2: Subsampled ref by 4.		[_mode == 2]
	short*							_pRefL2;					// Ref mem at (_l2Width * _l2Height).
	OverlayMem2Dv2*			_pRefL2Over; 			// Ref overlay with whole level 2 block dim.
	short*							_pExtRefL2;				// Extended ref mem create by ExtendBoundary() call.
	int									_extL2Width;
	int									_extL2Height;
	int									_extL2Boundary;		// Extended boundary for left, right, up and down.
	OverlayExtMem2Dv2*	_pExtRefL2Over;		// Ref overlay with level 2 motion block dim.

	/// Temp working block and its overlay.
	short*							_pMBlk;						///< Motion block temp mem.
	OverlayMem2Dv2*			_pMBlkOver;				///< Motion block overlay of temp mem.

	/// Hold the resulting motion vectors in a byte array.
	VectorStructList*	_pMotionVectorStruct;

	/// A flag per macroblock to include it in the distortion accumulation.
	bool*							_pDistortionIncluded;
};//end MotionEstimatorH263ImplUnres.


#endif // !_MOTIONESTIMATORH263IMPLUNRES_H

