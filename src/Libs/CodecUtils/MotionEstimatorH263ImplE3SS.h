/** @file

MODULE						: MotionEstimatorH263ImplE3SS

TAG								: MEH263IE3SS

FILE NAME					: MotionEstimatorH263ImplE3SS.h

DESCRIPTION				: A fast unrestricted motion estimator implementation for 
										Recommendation H.263 (02/98) Annex D page 53 of absolute 
										error difference measure.	Access via a IMotionEstimator	
										interface. The boundary is extended to accomodate the 
										selected motion range.The algorithm is based on the
										Efficient Three-Step Search (E3SS) method defined in the
										article: X. Jing and L. Chau, "An Efficient Three-Step
										Search Algorithm for Block Motion	Estimation," IEEE Trans. 
										on Multimedia, Vol. 6, No. 3, June 2004.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
===========================================================================
*/
#ifndef _MOTIONESTIMATORH263IMPLE3SS_H
#define _MOTIONESTIMATORH263IMPLE3SS_H

#include "IMotionEstimator.h"
#include "VectorStructList.h"
#include "OverlayMem2Dv2.h"
#include "OverlayExtMem2Dv2.h"

/**
---------------------------------------------------------------------------
	Struct definition.
---------------------------------------------------------------------------
*/
typedef struct _MEH263IE3SS_COORD
{
	short int x;
	short int y;
} MEH263IE3SS_COORD;

/**
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class MotionEstimatorH263ImplE3SS : public IMotionEstimator
{
/// Construction.
public:

	MotionEstimatorH263ImplE3SS(	const void* pSrc, 
																const void* pRef, 
																int					imgWidth, 
																int					imgHeight,
																int					motionRange);

	virtual ~MotionEstimatorH263ImplE3SS(void);

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

	/// Clear alloc mem.
	void Destroy(void);

	/** Get the appropriate square/diamond pel position search struct.
	Get the correct struct depending on whether the current motion vector under test
	is at the limit of its search range.
	@param x				: X motion vector coord.
	@param y				: Y motion vector coord.
	@param halfPos	: Returned correct struct.
	@return					: Length of the struct.
	*/
	int GetSquarePelSearchStruct(int x, int y, MEH263IE3SS_COORD** ppSquarePos);
	int GetDiamondPelSearchStruct(int x, int y, MEH263IE3SS_COORD** ppDiamondPos);

	void GetMotionRange(int		x,			int		y, 
											int*	xlr,		int*	xrr, 
											int*	yur,		int*	ydr, 
											int		range); 

protected:

	int _ready;	///< Ready to estimate.
	int _mode;	///< Not used.

	/// Parameters must remain const for the life time of this instantiation.
	int	_imgWidth;				///< Width of the src and ref images. 
	int	_imgHeight;				///< Height of the src and ref images.
	int	_macroBlkWidth;		///< Width of the motion block.
	int	_macroBlkHeight;	///< Height of the motion block.
	int	_motionRange;			///< (x,y) range of the motion vectors.

	const void*	_pInput;	///< References to the images at construction.
	const void* _pRef;

	/// Input mem overlay members.
	OverlayMem2Dv2*			_pInOver;					///< Input overlay with motion block dim.

	/// Ref mem overlay members.
	OverlayMem2Dv2*			_pRefOver;				///< Ref overlay with whole block dim.
	short*							_pExtRef;					///< Extended ref mem created by ExtendBoundary() call.
	int									_extWidth;
	int									_extHeight;
	int									_extBoundary;			///< Extended boundary for left, right, up and down.
	OverlayExtMem2Dv2*	_pExtRefOver;			///< Extended ref overlay with motion block dim.

	/// Temp working block and its overlay.
	short*							_pMBlk;						///< Motion block temp mem.
	OverlayMem2Dv2*			_pMBlkOver;				///< Motion block overlay of temp mem.
	/// For every search window mark each position that is tested.
	int*							_pSrcWin;
	int**							_ppWin;
	int								_winOffx;					///< Offset into the 2-D array marking the [0,0] pt.
	int								_winOffy;

	/// Hold the resulting motion vectors in a byte array.
	VectorStructList*	_pMotionVectorStruct;

};//end MotionEstimatorH263ImplE3SS.


#endif // !_MOTIONESTIMATORH263IMPLE3SS_H

