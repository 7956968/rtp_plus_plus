/** @file

MODULE						: MotionEstimatorH263ImplE3SS

TAG								: MEH263IE3SS

FILE NAME					: MotionEstimatorH263ImplE3SS.cpp

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
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include <memory.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include	"MotionEstimatorH263ImplE3SS.h"

/*
--------------------------------------------------------------------------
  Constants. 
--------------------------------------------------------------------------
*/
/// Calc = ((16[vec dim] * 16[vec dim]) * 2.
#define MEH263IE3SS_FULL_MOTION_NOISE_FLOOR					512
#define MEH263IE3SS_MOTION_NOISE_FLOOR								26 //24 //sqrt 512
#define MEH263IE3SS_L0_MOTION_VECTOR_REFINED_RANGE		2
#define MEH263IE3SS_L1_MOTION_VECTOR_REFINED_RANGE		2

/// Choose between sqr err distortion or abs diff metric.
#undef MEH263IE3SS_ABS_DIFF

///-------------------------- Square box pattern -----------------------------------------------
/// Search range coords for centre motion vectors.
#define MEH263IE3SS_MOTION_SQUARE_POS_LENGTH 	8
MEH263IE3SS_COORD MEH263IE3SS_SquarePos[MEH263IE3SS_MOTION_SQUARE_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}
};

/// Search range coords for corner motion vectors.
#define MEH263IE3SS_MOTION_TOP_LEFT_SQUARE_POS_LENGTH 3		///< Top left search range.
MEH263IE3SS_COORD MEH263IE3SS_TopLeftSquarePos[MEH263IE3SS_MOTION_TOP_LEFT_SQUARE_POS_LENGTH]	= 
{
	{1,0},{0,1},{1,1}
};

#define MEH263IE3SS_MOTION_TOP_RIGHT_SQUARE_POS_LENGTH 3	///< Top right search range.
MEH263IE3SS_COORD MEH263IE3SS_TopRightSquarePos[MEH263IE3SS_MOTION_TOP_RIGHT_SQUARE_POS_LENGTH]	= 
{
	{-1,0},{-1,1},{0,1}
};

#define MEH263IE3SS_MOTION_BOTTOM_LEFT_SQUARE_POS_LENGTH 3 ///< Bottom left search range.
MEH263IE3SS_COORD MEH263IE3SS_BottomLeftSquarePos[MEH263IE3SS_MOTION_BOTTOM_LEFT_SQUARE_POS_LENGTH]	= 
{
	{0,-1},{1,-1},{1,0}
};

#define MEH263IE3SS_MOTION_BOTTOM_RIGHT_SQUARE_POS_LENGTH 3	///< Bottom right search range.
MEH263IE3SS_COORD MEH263IE3SS_BottomRightSquarePos[MEH263IE3SS_MOTION_BOTTOM_RIGHT_SQUARE_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{-1,0}
};

/// Search range coords for edge motion vectors.
#define MEH263IE3SS_MOTION_LEFT_SQUARE_POS_LENGTH 5	///< Left edge search range.
MEH263IE3SS_COORD MEH263IE3SS_LeftSquarePos[MEH263IE3SS_MOTION_LEFT_SQUARE_POS_LENGTH]	= 
{
	{0,-1},{1,-1},{1,0},{0,1},{1,1}
};

#define MEH263IE3SS_MOTION_RIGHT_SQUARE_POS_LENGTH 5	///< Right edge search range.
MEH263IE3SS_COORD MEH263IE3SS_RightSquarePos[MEH263IE3SS_MOTION_RIGHT_SQUARE_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{-1,0},{-1,1},{0,1}
};

#define MEH263IE3SS_MOTION_TOP_SQUARE_POS_LENGTH 5	///< Top edge search range.
MEH263IE3SS_COORD MEH263IE3SS_TopSquarePos[MEH263IE3SS_MOTION_TOP_SQUARE_POS_LENGTH]	= 
{
	{-1,0},{1,0},{-1,1},{0,1},{1,1}
};

#define MEH263IE3SS_MOTION_BOTTOM_SQUARE_POS_LENGTH 5	///< Bottom edge search range.
MEH263IE3SS_COORD MEH263IE3SS_BottomSquarePos[MEH263IE3SS_MOTION_BOTTOM_SQUARE_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{1,-1},{-1,0},{1,0}
};

///-------------------------- Diamond pattern -----------------------------------------------
/// Search range coords for centre motion vectors.
#define MEH263IE3SS_MOTION_DIAMOND_POS_LENGTH 	4
MEH263IE3SS_COORD MEH263IE3SS_DiamondPos[MEH263IE3SS_MOTION_DIAMOND_POS_LENGTH]	= 
{
	{-1,0},{1,0},{0,-1},{0,1}
};

/// Search range coords for corner motion vectors.
#define MEH263IE3SS_MOTION_TOP_LEFT_DIAMOND_POS_LENGTH 2		///< Top left search range.
MEH263IE3SS_COORD MEH263IE3SS_TopLeftDiamondPos[MEH263IE3SS_MOTION_TOP_LEFT_DIAMOND_POS_LENGTH]	= 
{
	{1,0},{0,1}
};

#define MEH263IE3SS_MOTION_TOP_RIGHT_DIAMOND_POS_LENGTH 2	///< Top right search range.
MEH263IE3SS_COORD MEH263IE3SS_TopRightDiamondPos[MEH263IE3SS_MOTION_TOP_RIGHT_DIAMOND_POS_LENGTH]	= 
{
	{-1,0},{0,1}
};

#define MEH263IE3SS_MOTION_BOTTOM_LEFT_DIAMOND_POS_LENGTH 2 ///< Bottom left search range.
MEH263IE3SS_COORD MEH263IE3SS_BottomLeftDiamondPos[MEH263IE3SS_MOTION_BOTTOM_LEFT_DIAMOND_POS_LENGTH]	= 
{
	{0,-1},{1,0}
};

#define MEH263IE3SS_MOTION_BOTTOM_RIGHT_DIAMOND_POS_LENGTH 2	///< Bottom right search range.
MEH263IE3SS_COORD MEH263IE3SS_BottomRightDiamondPos[MEH263IE3SS_MOTION_BOTTOM_RIGHT_DIAMOND_POS_LENGTH]	= 
{
	{0,-1},{-1,0}
};

/// Search range coords for edge motion vectors.
#define MEH263IE3SS_MOTION_LEFT_DIAMOND_POS_LENGTH 3	///< Left edge search range.
MEH263IE3SS_COORD MEH263IE3SS_LeftDiamondPos[MEH263IE3SS_MOTION_LEFT_DIAMOND_POS_LENGTH]	= 
{
	{0,-1},{1,0},{0,1}
};

#define MEH263IE3SS_MOTION_RIGHT_DIAMOND_POS_LENGTH 3	///< Right edge search range.
MEH263IE3SS_COORD MEH263IE3SS_RightDiamondPos[MEH263IE3SS_MOTION_RIGHT_DIAMOND_POS_LENGTH]	= 
{
	{0,-1},{-1,0},{0,1}
};

#define MEH263IE3SS_MOTION_TOP_DIAMOND_POS_LENGTH 3	///< Top edge search range.
MEH263IE3SS_COORD MEH263IE3SS_TopDiamondPos[MEH263IE3SS_MOTION_TOP_DIAMOND_POS_LENGTH]	= 
{
	{-1,0},{1,0},{0,1}
};

#define MEH263IE3SS_MOTION_BOTTOM_DIAMOND_POS_LENGTH 3	///< Bottom edge search range.
MEH263IE3SS_COORD MEH263IE3SS_BottomDiamondPos[MEH263IE3SS_MOTION_BOTTOM_DIAMOND_POS_LENGTH]	= 
{
	{0,-1},{-1,0},{1,0}
};

/**
--------------------------------------------------------------------------
  Construction. 
--------------------------------------------------------------------------
*/

MotionEstimatorH263ImplE3SS::MotionEstimatorH263ImplE3SS(	const void* pSrc, 
																													const void* pRef, 
																													int					imgWidth, 
																													int					imgHeight,
																													int					motionRange)
{
	_ready	= 0;	///< Ready to estimate.
	_mode		= 0;	///< Not used.

	/// Parameters must remain const for the life time of this instantiation.
	_imgWidth				= imgWidth;					///< Width of the src and ref images. 
	_imgHeight			= imgHeight;				///< Height of the src and ref images.
	_macroBlkWidth	= 16;								///< Width of the motion block = 16 for H.263.
	_macroBlkHeight	= 16;								///< Height of the motion block = 16 for H.263.
	_motionRange		= motionRange;			///< (2x,2y) range of the motion vectors.
	_pInput					= pSrc;
	_pRef						= pRef;

	/// Input mem overlay members.
	_pInOver					= NULL;			///< Input overlay with motion block dim.

	/// Ref mem overlay members.
	_pRefOver					= NULL;			///< Ref overlay with whole block dim.
	_pExtRef					= NULL;			///< Extended ref mem created by ExtendBoundary() call.
	_extWidth					= 0;
	_extHeight				= 0;
	_extBoundary			= 0;
	_pExtRefOver			= NULL;			///< Extended ref overlay with motion block dim.

	/// Temp working block and its overlay.
	_pMBlk						= NULL;			///< Motion block temp mem.
	_pMBlkOver				= NULL;			///< Motion block overlay of temp mem.
	_pSrcWin					= NULL;
	_ppWin						= NULL;

	/// Hold the resulting motion vectors in a byte array.
	_pMotionVectorStruct = NULL;

}//end constructor.

MotionEstimatorH263ImplE3SS::~MotionEstimatorH263ImplE3SS(void)
{
	Destroy();
}//end destructor.

/**
--------------------------------------------------------------------------
  Public IMotionEstimator Interface. 
--------------------------------------------------------------------------
*/

int MotionEstimatorH263ImplE3SS::Create(void)
{
	/// Clean out old mem.
	Destroy();

	/// --------------- Configure input overlays --------------------------------
	/// Put an overlay on the input with the block size set to the vector 
	/// dim. This is used to access input vectors.
	_pInOver = new OverlayMem2Dv2((void *)_pInput, 
															_imgWidth, 
															_imgHeight, 
															_macroBlkWidth, 
															_macroBlkHeight);
	if(_pInOver == NULL)
	{
		Destroy();
		return(0);
	}//end _pInOver...

	/// --------------- Configure ref overlays --------------------------------
	/// Overlay the whole reference. The reference will have an extended 
	/// boundary for motion estimation and must therefore have its own mem.
	_pRefOver = new OverlayMem2Dv2((void *)_pRef, _imgWidth, _imgHeight, _imgWidth, _imgHeight);
	if(_pRefOver == NULL)
  {
		Destroy();
	  return(0);
  }//end if !_pRefOver...

	/// Create the new extended boundary ref into _pExtRef. The boundary is extended by
	/// the max dimension of the macroblock.
	_extBoundary = _macroBlkWidth + 1;
	if(_macroBlkHeight > _macroBlkWidth)
		_extBoundary = _macroBlkHeight + 1;
	if(!OverlayExtMem2Dv2::ExtendBoundary((void *)_pRef, 
																			_imgWidth,						
																			_imgHeight, 
																			_extBoundary,	///< Extend left and right by...
																			_extBoundary,	///< Extend top and bottom by...
																			(void **)(&_pExtRef)) )	///< Created in the method.
  {
		Destroy();
	  return(0);
  }//end if !ExtendBoundary...
	_extWidth	 = _imgWidth + (2 * _extBoundary);
	_extHeight = _imgHeight + (2 * _extBoundary);

	/// Place an overlay on the extended boundary ref with block 
	/// size set to the motion vec dim..
	_pExtRefOver = new OverlayExtMem2Dv2(_pExtRef,				///< Src description. 
																		 _extWidth, 
																		 _extHeight,
																		 _macroBlkWidth,	///< Block size description.
																		 _macroBlkHeight,
																		 _extBoundary,		///< Boundary size for both left and right.
																		 _extBoundary  );
	if(_pExtRefOver == NULL)
  {
		Destroy();
	  return(0);
  }//end if !_pExtRefOver...

	/// --------------- Configure temp overlays --------------------------------
	/// Alloc some temp mem and overlay it to use for half pel motion estimation and 
	/// compensation. The block size is the same as the mem size.
	_pMBlk = new short[_macroBlkWidth * _macroBlkHeight];
	_pMBlkOver = new OverlayMem2Dv2(_pMBlk, _macroBlkWidth, _macroBlkHeight, 
																				_macroBlkWidth, _macroBlkHeight);
	if( (_pMBlk == NULL)||(_pMBlkOver == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pMBlk...

	/// --------------- Configure search window test ---------------------------
	/// Alloc mem to hold flags at each motion vector pos to determine if that
	/// pos has previously been checked. _motionRange is in half pel units
	/// and must be converted.
	int range = _motionRange/2;	///< Ranges are [-range..0..(range-1)].
	_pSrcWin = new int[4*range*range];
	_ppWin	 = new int*[2*range];
	if( (_pSrcWin == NULL)||(_ppWin == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pSrcWin...
	/// Load the address array.
	for(int i = 0; i < (2*range); i++)
		_ppWin[i] = &(_pSrcWin[i * (2*range)]);
	_winOffx = range;
	_winOffy = range;

	/// --------------- Configure result ---------------------------------------
	/// The structure container for the motion vectors.
	_pMotionVectorStruct = new VectorStructList(VectorStructList::SIMPLE2D);
	if(_pMotionVectorStruct != NULL)
	{
		/// How many motion vectors will there be at the block dim.
		int numVecs = (_imgWidth/_macroBlkWidth) * (_imgHeight/_macroBlkHeight);
		if(!_pMotionVectorStruct->SetLength(numVecs))
		{
			Destroy();
			return(0);
		}//end _pMotionVectorStruct...
	}//end if _pMotionVectorStruct...
	else
  {
		Destroy();
	  return(0);
  }//end if else...

	_ready = 1;
	return(1);
}//end Create.

void	MotionEstimatorH263ImplE3SS::Reset(void)
{
}//end Reset.

/** Set the mode.
Not used for this implementation.
*/
void	MotionEstimatorH263ImplE3SS::SetMode(int mode)
{
	_mode = mode;
}//end SetMode.

/** Motion estimate the source within the reference.
Do the estimation with the block sizes and image sizes defined in
the implementation. The returned type holds the vectors. This is
a fast E3SS search algorithm with extended boundaries and a
absolute difference criterion. 
@param avgDistortion	: Return the total average difference for the estimation.
@return								: The list of motion vectors.
*/
void* MotionEstimatorH263ImplE3SS::Estimate(long* avgDistortion)
{
  int		i,j,m,n,x,len;
	long	totalDifference = 0;

	/// Set the motion vector struct storage structure.
	int		maxLength	= _pMotionVectorStruct->GetLength();
	int		vecPos		= 0;

	/// Write the level 0 ref and fill its extended boundary. The centre part of
	/// _pExtRefOver is copied from _pRefOver before filling the boundary.
	_pExtRefOver->SetOrigin(0, 0);
	_pExtRefOver->SetOverlayDim(_imgWidth, _imgHeight);
	_pExtRefOver->Write(*_pRefOver);	///< _pRefOver is always set to the whole image.
	_pExtRefOver->FillBoundaryProxy();
	_pExtRefOver->SetOverlayDim(_macroBlkWidth, _macroBlkHeight);

	/// Gathers the motion vector absolute differnce data and choose the vector.
	/// m,n step vec dim = _macroBlkHeight, _macroBlkWidth.
	int fullPelMotionRange = _motionRange/2;
  for(m = 0; m < _imgHeight; m += _macroBlkHeight)
		for(n = 0; n < _imgWidth; n += _macroBlkWidth)
  {
		/// Initialise motion vectors.
		int mx	= 0;	///< Full pel grid.
		int my	= 0;
		int hmx	= 0;	///< Half pel grid.
		int hmy	= 0;

		/// To mark those motion vectors that have been tested, reset the window.
		memset((void *)_pSrcWin, 0, _motionRange * _motionRange * sizeof(int));

		/// ----------------- Zero Motion Vector -----------------------------------
		/// The [0,0] vector difference between the input and ref blocks is the most
		/// likely candidate and is therefore the starting point.

		/// Set the input and ref blocks to work with.
		_pInOver->SetOrigin(n,m);
		_pExtRefOver->SetOrigin(n,m);
		/// Absolute diff comparison method.
#ifdef MEH263IE3SS_ABS_DIFF
		int zeroVecDiff = _pInOver->Tad16x16(*_pExtRefOver);
#else
		int zeroVecDiff = _pInOver->Tsd16x16(*_pExtRefOver);
#endif
		int minDiff			= zeroVecDiff;	///< Best so far.
		_ppWin[_winOffy][_winOffx] = 1;	///< Flag [0,0] as tested.

		/// ----------------- Small Diamond Search ---------------------------------
		/// A small diamond search around the zero vector is always within bounds
		/// of the extended boundary.
		MEH263IE3SS_COORD* pDiamondPos = MEH263IE3SS_DiamondPos;
    for(x = 0; x < MEH263IE3SS_MOTION_DIAMOND_POS_LENGTH; x++)
    {
			/// Set the block to the motion vector around the [n,m] reference
			/// location.
			_pExtRefOver->SetOrigin(n + pDiamondPos[x].x, m + pDiamondPos[x].y);
#ifdef MEH263IE3SS_ABS_DIFF
			int	blkDiff = _pInOver->Tad16x16LessThan(*_pExtRefOver, minDiff);
#else
			int	blkDiff = _pInOver->Tsd16x16LessThan(*_pExtRefOver, minDiff);
#endif
			_ppWin[_winOffy + pDiamondPos[x].y][_winOffx + pDiamondPos[x].x] = 1;	///< Flag this motion vector as tested.
			if(blkDiff < minDiff)
			{
				minDiff = blkDiff;
				mx = pDiamondPos[x].x;
				my = pDiamondPos[x].y;
			}//end if blkDiff...
    }//end for x...

		/// Depending on which img boundary we are on will limit the
		/// full search range. _motionRange is in half pel units and must
		/// be converted to full pel units.
		int xlRng, xrRng, yuRng, ydRng;
		GetMotionRange(n, m, &xlRng, &xrRng, &yuRng, &ydRng, fullPelMotionRange);

		/// ----------------- Sampled Grid Search -------------------------------------
		/// The grid is spaced 4 pels apart from [0,0] but excludes the edges (unknown
		/// reason) of the search window.
		int numGrid = (fullPelMotionRange - 1)/4; ///< No. of grid points either side of [0,0].
    for(i = -(4*numGrid); i <= (4*numGrid); i += 4)
    {
			if( (i >= (yuRng+2))&&(i <= (ydRng-2)) )	///< Only do this if row is within 2 pels of image bounds.
			{
				for(j = -(4*numGrid); j <= (4*numGrid); j += 4)
				{
					if( (j >= (xlRng+2))&&(j <= (xrRng-2)) )	///< Only do this if col is within 2 pels of image bounds.
					{
						/// Early exit because zero motion vec already checked.
						if( !(i||j) )	goto MEH263IE3SS_GRID_BREAK;
						/// Set the block to the [j,i] motion vector around the [n,m] reference location.
						_pExtRefOver->SetOrigin(n+j, m+i);
#ifdef MEH263IE3SS_ABS_DIFF
						int blkDiff = _pInOver->Tad16x16LessThan(*_pExtRefOver, minDiff);
#else
						int blkDiff = _pInOver->Tsd16x16LessThan(*_pExtRefOver, minDiff);
#endif
						_ppWin[_winOffy + i][_winOffx + j] = 1;	///< Flag this motion vector as tested.
						if(blkDiff <= minDiff)
						{
							/// Weight the equal diff with the smallest motion vector magnitude. 
							if(blkDiff == minDiff)
							{
								int vecDistDiff = ( (my*my)+(mx*mx) )-( (i*i)+(j*j) );
								if(vecDistDiff < 0)
									goto MEH263IE3SS_GRID_BREAK;
							}//end if blkDiff...

							minDiff = blkDiff;
							mx = j;
							my = i;
						}//end if blkDiff...
					}//end if j...
					MEH263IE3SS_GRID_BREAK: ; // null.
				}//end for j...
			}//end if i...
    }//end for i...

		///----------------------- Diamond or Square Search Choice ------------------------
		int absVector = abs(mx) + abs(my);
		if(absVector != 0)	///< Only proceed if not the zero vector.
		{
			if(absVector == 1)	///< Proceed with diamond searching around [n+mx,m+my].
			{
				/// The diamond search is unrestricted and therefore bounds checking is required, or
				/// at least after the first 5 iterations.
				int rmx = 0;	///< Refined winning pos.
				int rmy = 0;
				int iterations;

				for(iterations = 0;(rmx || rmy)||(!iterations); iterations++)
				{
					rmx = 0;		///< Reset refinement to centre.
					rmy = 0;
					MEH263IE3SS_COORD* pDiamondPos = MEH263IE3SS_DiamondPos;
					len = MEH263IE3SS_MOTION_DIAMOND_POS_LENGTH;
					if(iterations > 5)
						len = GetDiamondPelSearchStruct(mx, my, (MEH263IE3SS_COORD **)(&pDiamondPos));
					for(x = 0; x < len; x++)
					{
						/// Only those that have not been previously tested.
						if(!_ppWin[_winOffy + my + pDiamondPos[x].y][_winOffx + mx + pDiamondPos[x].x])
						{
							/// Set the block to the motion vector around the new [n+mx,m+my] reference pos.
							_pExtRefOver->SetOrigin(n + mx + pDiamondPos[x].x, m + my + pDiamondPos[x].y);
#ifdef MEH263IE3SS_ABS_DIFF
							int	blkDiff = _pInOver->Tad16x16LessThan(*_pExtRefOver, minDiff);
#else
							int	blkDiff = _pInOver->Tsd16x16LessThan(*_pExtRefOver, minDiff);
#endif
							_ppWin[_winOffy + my + pDiamondPos[x].y][_winOffx + mx + pDiamondPos[x].x] = 1;	///< Flag this motion vector as tested.
							if(blkDiff < minDiff)
							{
								minDiff = blkDiff;
								rmx = pDiamondPos[x].x;
								rmy = pDiamondPos[x].y;
							}//end if blkDiff...
						}//end if !_ppWin...
					}//end for x...
					mx += rmx;	///< Update motion vector with search refinement.
					my += rmy;

				}//end for iterations...

			}//end if absVector...
			else								///< Proceed with square search in fixed 5x5 grid around [n+mx,m+my].
			{
				/// The grid search in the 1st step above ensured that there is room for the two
				/// following steps (written inline below) within every search window so no bounds 
				/// checking is required.

				int rmx = 0;	///< Refined winning pos.
				int rmy = 0;
				/// First 3x3 search unhindered.
				MEH263IE3SS_COORD* pSquarePos = MEH263IE3SS_SquarePos;
				for(x = 0; x < MEH263IE3SS_MOTION_SQUARE_POS_LENGTH; x++)
				{
					/// Set the block to the motion vector around the [n+mx,m+my] reference pos.
					_pExtRefOver->SetOrigin(n + mx + pSquarePos[x].x, m + my + pSquarePos[x].y);
#ifdef MEH263IE3SS_ABS_DIFF
					int	blkDiff = _pInOver->Tad16x16LessThan(*_pExtRefOver, minDiff);
#else
					int	blkDiff = _pInOver->Tsd16x16LessThan(*_pExtRefOver, minDiff);
#endif
					_ppWin[_winOffy + my + pSquarePos[x].y][_winOffx + mx + pSquarePos[x].x] = 1;	///< Flag this motion vector as tested.
					if(blkDiff < minDiff)
					{
						minDiff = blkDiff;
						rmx = pSquarePos[x].x;
						rmy = pSquarePos[x].y;
					}//end if blkDiff...
				}//end for x...

				/// Second refined search if [rmx,rmy] not zero.
				if(rmx || rmy)
				{
					mx += rmx;	///< Update motion vector with first search refinement.
					my += rmy;
					rmx = 0;		///< Reset refinement to centre.
					rmy = 0;
					for(x = 0; x < MEH263IE3SS_MOTION_SQUARE_POS_LENGTH; x++)
					{
						/// Only those that have not been previously tested.
						if(!_ppWin[_winOffy + my + pSquarePos[x].y][_winOffx + mx + pSquarePos[x].x])
						{
							/// Set the block to the motion vector around the new [n+mx,m+my] reference pos.
							_pExtRefOver->SetOrigin(n + mx + pSquarePos[x].x, m + my + pSquarePos[x].y);
#ifdef MEH263IE3SS_ABS_DIFF
							int	blkDiff = _pInOver->Tad16x16LessThan(*_pExtRefOver, minDiff);
#else
							int	blkDiff = _pInOver->Tsd16x16LessThan(*_pExtRefOver, minDiff);
#endif
							_ppWin[_winOffy + my + pSquarePos[x].y][_winOffx + mx + pSquarePos[x].x] = 1;	///< Flag this motion vector as tested.
							if(blkDiff < minDiff)
							{
								minDiff = blkDiff;
								rmx = pSquarePos[x].x;
								rmy = pSquarePos[x].y;
							}//end if blkDiff...
						}//end if !_ppWin...
					}//end for x...
					mx += rmx;	///< Update motion vector with second search refinement.
					my += rmy;

				}//end if rmx...

			}//end else...

		}//end if mx..

		///----------------------- Half pel refined search ------------------------
    /// Search around the min diff full pel motion vector on a half pel grid.

		/// Set the location to the min diff motion vector [mx,my].
		_pExtRefOver->SetOrigin(n+mx, m+my);

		/// Adjust the search range to stay within the global bounds.
		MEH263IE3SS_COORD* pSquarePos;
		len = GetSquarePelSearchStruct(mx, my, (MEH263IE3SS_COORD **)(&pSquarePos));

    for(x = 0; x < len; x++)
    {
			/// Read the half grid pels into temp.
			_pExtRefOver->HalfRead(*_pMBlkOver, pSquarePos[x].x, pSquarePos[x].y); 
#ifdef MEH263IE3SS_ABS_DIFF
			int blkDiff = _pInOver->Tad16x16LessThan(*_pMBlkOver, minDiff);
#else
			int blkDiff = _pInOver->Tsd16x16LessThan(*_pMBlkOver, minDiff);
#endif
			if(blkDiff < minDiff)
			{
				minDiff = blkDiff;
				hmx = pSquarePos[x].x;
				hmy = pSquarePos[x].y;
			}//end if blkDiff...
    }//end for x...

		/// Convert and add the refinement in half pel units.
		int mvx = (mx << 1) + hmx;
		int mvy = (my << 1) + hmy;

		/// Bounds check in half pel units used for debugging.
		if(mvx < -_motionRange) 
			mvx = -_motionRange;
		else if(mvx > (_motionRange-1))
			mvx = (_motionRange-1);
		if(mvy < -_motionRange) 
			mvy = -_motionRange;
		else if(mvy > (_motionRange-1))
			mvy = (_motionRange-1);

		/// Validity of the motion vector is weighted with non-linear factors.
		int weight						= 0;
		int diffWithZeroDiff	= zeroVecDiff - minDiff;
		int magSqr						= (mvx * mvx) + (mvy * mvy);

#ifdef MEH263IE3SS_ABS_DIFF
		/// Contribute if motion vector is small.
		if((diffWithZeroDiff >> 1) < magSqr)
			weight++;
		/// Contribute if same order as the noise.
		if(zeroVecDiff < MEH263IE3SS_MOTION_NOISE_FLOOR)
			weight++;
		/// Contribute if the zero vector and min diff vector are similar.
		if((diffWithZeroDiff * 7) < minDiff)
			weight++;
#else
		/// Contribute if motion vector is small.
		if((diffWithZeroDiff >> 2) < magSqr)
			weight++;
		/// Contribute if same order as the noise.
		if(zeroVecDiff < MEH263IE3SS_FULL_MOTION_NOISE_FLOOR)
			weight++;
		/// Contribute if the zero vector and min energy vector are similar.
		if((diffWithZeroDiff * 10) < minDiff)
			weight++;
#endif

		/// Determine the basic run-length motion.
		if((minDiff < zeroVecDiff)&&(weight < 2))
    {
			totalDifference += minDiff;
    }//end if min_energy...
    else
		{
			mvx = 0;
			mvy = 0;
			totalDifference += zeroVecDiff;
		}//end else...

		/// Load the selected vector coord.
		if(vecPos < maxLength)
		{
			_pMotionVectorStruct->SetSimpleElement(vecPos, 0, mvx);
			_pMotionVectorStruct->SetSimpleElement(vecPos, 1, mvy);
			vecPos++;
		}//end if vecPos...

  }//end for m & n...

	/// In this context avg distortion is actually avg difference.
	*avgDistortion = totalDifference/maxLength;
	return((void *)_pMotionVectorStruct);

}//end Estimate.

/*
--------------------------------------------------------------------------
  Private methods. 
--------------------------------------------------------------------------
*/

void MotionEstimatorH263ImplE3SS::Destroy(void)
{
	_ready = 0;

	if(_pInOver != NULL)
		delete _pInOver;
	_pInOver = NULL;

	if(_pRefOver != NULL)
		delete _pRefOver;
	_pRefOver	= NULL;

	if(_pExtRef != NULL)
		delete[] _pExtRef;
	_pExtRef = NULL;

	if(_pExtRefOver != NULL)
		delete _pExtRefOver;
	_pExtRefOver = NULL;

	if(_pMBlk != NULL)
		delete[] _pMBlk;
	_pMBlk = NULL;

	if(_pMBlkOver != NULL)
		delete _pMBlkOver;
	_pMBlkOver = NULL;

	if(_ppWin != NULL)
		delete[] _ppWin;
	_ppWin = NULL;

	if(_pSrcWin != NULL)
		delete[] _pSrcWin;
	_pSrcWin = NULL;

	if(_pMotionVectorStruct != NULL)
		delete _pMotionVectorStruct;
	_pMotionVectorStruct = NULL;

}//end Destroy.

/** Get the appropriate square pel position search struct.
Get the correct struct depending on whether the current motion vector under test
is at the limit of its search range.
@param x						: X motion vector coord.
@param y						: Y motion vector coord.
@param ppSquarePos	: Returned correct struct.
@return							: Length of the struct.
*/
int MotionEstimatorH263ImplE3SS::GetSquarePelSearchStruct(int x, int y, MEH263IE3SS_COORD** ppSquarePos)
{
	int	 range						= _motionRange/2;	///< Convert from half pel to full pel.
	bool atRightExtreme		= (x == (range - 1));
	bool atLeftExtreme		= (x == -range);
	bool atBottomExtreme	= (y == (range - 1));
	bool atTopExtreme			= (y == -range);
	/// Within range most likely case.
	if( !(atRightExtreme || atLeftExtreme || atBottomExtreme || atTopExtreme) )
	{
		*ppSquarePos	= MEH263IE3SS_SquarePos;
		return(MEH263IE3SS_MOTION_SQUARE_POS_LENGTH);
	}//end if inner...
	else if( !atRightExtreme && !atLeftExtreme && !atBottomExtreme && atTopExtreme )
	{
		*ppSquarePos	= MEH263IE3SS_TopSquarePos;
		return(MEH263IE3SS_MOTION_TOP_SQUARE_POS_LENGTH);
	}//end if top...
	else if( !atRightExtreme && !atLeftExtreme && atBottomExtreme && !atTopExtreme )
	{
		*ppSquarePos	= MEH263IE3SS_BottomSquarePos;
		return(MEH263IE3SS_MOTION_BOTTOM_SQUARE_POS_LENGTH);
	}//end if bottom...
	else if( !atRightExtreme && atLeftExtreme && !atBottomExtreme && !atTopExtreme )
	{
		*ppSquarePos	= MEH263IE3SS_LeftSquarePos;
		return(MEH263IE3SS_MOTION_LEFT_SQUARE_POS_LENGTH);
	}//end if left...
	else if( atRightExtreme && !atLeftExtreme && !atBottomExtreme && !atTopExtreme )
	{
		*ppSquarePos	= MEH263IE3SS_RightSquarePos;
		return(MEH263IE3SS_MOTION_RIGHT_SQUARE_POS_LENGTH);
	}//end if right...
	else if( !atRightExtreme && atLeftExtreme && !atBottomExtreme && atTopExtreme )
	{
		*ppSquarePos	= MEH263IE3SS_TopLeftSquarePos;
		return(MEH263IE3SS_MOTION_TOP_LEFT_SQUARE_POS_LENGTH);
	}//end if top left...
	else if( atRightExtreme && !atLeftExtreme && !atBottomExtreme && atTopExtreme )
	{
		*ppSquarePos	= MEH263IE3SS_TopRightSquarePos;
		return(MEH263IE3SS_MOTION_TOP_RIGHT_SQUARE_POS_LENGTH);
	}//end if top right...
	else if( !atRightExtreme && atLeftExtreme && atBottomExtreme && !atTopExtreme )
	{
		*ppSquarePos	= MEH263IE3SS_BottomLeftSquarePos;
		return(MEH263IE3SS_MOTION_BOTTOM_LEFT_SQUARE_POS_LENGTH);
	}//end if bottom left...
	else if( atRightExtreme && !atLeftExtreme && atBottomExtreme && !atTopExtreme )
	{
		*ppSquarePos	= MEH263IE3SS_BottomRightSquarePos;
		return(MEH263IE3SS_MOTION_BOTTOM_RIGHT_SQUARE_POS_LENGTH);
	}//end if bottom right...

	*ppSquarePos	= MEH263IE3SS_SquarePos;
	return(MEH263IE3SS_MOTION_SQUARE_POS_LENGTH);
}//end GetSquarePelSearchStruct.

/** Get the appropriate diamond pel position search struct.
Get the correct struct depending on whether the current motion vector under test
is at the limit of its search range.
@param x						: X motion vector coord.
@param y						: Y motion vector coord.
@param ppDiamondPos	: Returned correct struct.
@return							: Length of the struct.
*/
int MotionEstimatorH263ImplE3SS::GetDiamondPelSearchStruct(int x, int y, MEH263IE3SS_COORD** ppDiamondPos)
{
	int	 range						= _motionRange/2;	///< Convert from half pel to full pel.
	bool atRightExtreme		= (x == (range - 1));
	bool atLeftExtreme		= (x == -range);
	bool atBottomExtreme	= (y == (range - 1));
	bool atTopExtreme			= (y == -range);

	/// Within range most likely case.
	if( !(atRightExtreme || atLeftExtreme || atBottomExtreme || atTopExtreme) )
	{
		*ppDiamondPos	= MEH263IE3SS_DiamondPos;
		return(MEH263IE3SS_MOTION_DIAMOND_POS_LENGTH);
	}//end if inner...
	else if( !atRightExtreme && !atLeftExtreme && !atBottomExtreme && atTopExtreme )
	{
		*ppDiamondPos	= MEH263IE3SS_TopDiamondPos;
		return(MEH263IE3SS_MOTION_TOP_DIAMOND_POS_LENGTH);
	}//end if top...
	else if( !atRightExtreme && !atLeftExtreme && atBottomExtreme && !atTopExtreme )
	{
		*ppDiamondPos	= MEH263IE3SS_BottomDiamondPos;
		return(MEH263IE3SS_MOTION_BOTTOM_DIAMOND_POS_LENGTH);
	}//end if bottom...
	else if( !atRightExtreme && atLeftExtreme && !atBottomExtreme && !atTopExtreme )
	{
		*ppDiamondPos	= MEH263IE3SS_LeftDiamondPos;
		return(MEH263IE3SS_MOTION_LEFT_DIAMOND_POS_LENGTH);
	}//end if left...
	else if( atRightExtreme && !atLeftExtreme && !atBottomExtreme && !atTopExtreme )
	{
		*ppDiamondPos	= MEH263IE3SS_RightDiamondPos;
		return(MEH263IE3SS_MOTION_RIGHT_DIAMOND_POS_LENGTH);
	}//end if right...
	else if( !atRightExtreme && atLeftExtreme && !atBottomExtreme && atTopExtreme )
	{
		*ppDiamondPos	= MEH263IE3SS_TopLeftDiamondPos;
		return(MEH263IE3SS_MOTION_TOP_LEFT_DIAMOND_POS_LENGTH);
	}//end if top left...
	else if( atRightExtreme && !atLeftExtreme && !atBottomExtreme && atTopExtreme )
	{
		*ppDiamondPos	= MEH263IE3SS_TopRightDiamondPos;
		return(MEH263IE3SS_MOTION_TOP_RIGHT_DIAMOND_POS_LENGTH);
	}//end if top right...
	else if( !atRightExtreme && atLeftExtreme && atBottomExtreme && !atTopExtreme )
	{
		*ppDiamondPos	= MEH263IE3SS_BottomLeftDiamondPos;
		return(MEH263IE3SS_MOTION_BOTTOM_LEFT_DIAMOND_POS_LENGTH);
	}//end if bottom left...
	else if( atRightExtreme && !atLeftExtreme && atBottomExtreme && !atTopExtreme )
	{
		*ppDiamondPos	= MEH263IE3SS_BottomRightDiamondPos;
		return(MEH263IE3SS_MOTION_BOTTOM_RIGHT_DIAMOND_POS_LENGTH);
	}//end if bottom right...

	*ppDiamondPos	= MEH263IE3SS_DiamondPos;
	return(MEH263IE3SS_MOTION_DIAMOND_POS_LENGTH);
}//end GetDiamondPelSearchStruct.

/** Get the allowed motion range for this block.
The search area for unrestricted H.263 is within the bounds of the extended image
dimensions. The range is limited at the corners and edges of the extended
images.
@param x				: X coord of block.
@param y				: Y coord of block.
@param xlr			: Returned allowed x left range.
@param xrr			: Returned allowed x right range.
@param yur			: Returned allowed y up range.
@param ydr			: Returned allowed y down range.
@param range		: Desired range of motion.
@return					: none.
*/
void MotionEstimatorH263ImplE3SS::GetMotionRange(int  x,			int  y, 
																									int* xlr,		int* xrr, 
																									int* yur,		int* ydr,
																									int	 range)
{
	if( (x - range) >= -_extBoundary )	///< Ok and within left extended boundary.
		*xlr = -range;
	else ///< Bring it into the extended boundary edge.
		*xlr = -(x + _extBoundary);
	if( (x + range) < _imgWidth )	///< Rest of block extends into the bounday region.
		*xrr = range;
	else
		*xrr = _imgWidth - x;

	if( (y - range) >= -_extBoundary )	///< Ok and within upper extended boundary.
		*yur = -range;
	else ///< Bring it into the extended boundary edge.
		*yur = -(y + _extBoundary);
	if( (y + range) < _imgHeight )	///< Rest of block extends into the bounday region.
		*ydr = range;
	else
		*ydr = _imgHeight - y;

}//end GetMotionRange.




