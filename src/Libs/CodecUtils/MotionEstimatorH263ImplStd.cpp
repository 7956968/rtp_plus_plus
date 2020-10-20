/** @file

MODULE						: MotionEstimatorH263ImplStd

TAG								: MEH263IS

FILE NAME					: MotionEstimatorH263ImplStd.cpp

DESCRIPTION				: A standard motion estimator implementation for 
										Recommendation H.263 of squared	error distortion 
										measure. No extended boundary or 4V modes. Access 
										via a IMotionEstimator	interface.

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

#include	"MotionEstimatorH263ImplStd.h"

/*
--------------------------------------------------------------------------
  Constants. 
--------------------------------------------------------------------------
*/
// Calc = ((16[vec dim] * 16[vec dim]) * 2.
#define MEH263IS_FULL_MOTION_NOISE_FLOOR 512

// Half motion search range coords for centre motion vectors.
#define MEH263IS_MOTION_HALF_POS_LENGTH 	8
MEH263IS_COORD MEH263IS_HalfPos[MEH263IS_MOTION_HALF_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}
};

// Search range coords for corner motion vectors.
#define MEH263IS_MOTION_TOP_LEFT_HALF_POS_LENGTH 3		// Top left search range.
MEH263IS_COORD MEH263IS_TopLeftHalfPos[MEH263IS_MOTION_TOP_LEFT_HALF_POS_LENGTH]	= 
{
	{1,0},{0,1},{1,1}
};

#define MEH263IS_MOTION_TOP_RIGHT_HALF_POS_LENGTH 3	// Top right search range.
MEH263IS_COORD MEH263IS_TopRightHalfPos[MEH263IS_MOTION_TOP_RIGHT_HALF_POS_LENGTH]	= 
{
	{-1,0},{-1,1},{0,1}
};

#define MEH263IS_MOTION_BOTTOM_LEFT_HALF_POS_LENGTH 3 // Bottom left search range.
MEH263IS_COORD MEH263IS_BottomLeftHalfPos[MEH263IS_MOTION_BOTTOM_LEFT_HALF_POS_LENGTH]	= 
{
	{0,-1},{1,-1},{1,0}
};

#define MEH263IS_MOTION_BOTTOM_RIGHT_HALF_POS_LENGTH 3	//Bottom right search range.
MEH263IS_COORD MEH263IS_BottomRightHalfPos[MEH263IS_MOTION_BOTTOM_RIGHT_HALF_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{-1,0}
};

// Search range coords for edge motion vectors.
#define MEH263IS_MOTION_LEFT_HALF_POS_LENGTH 5	// Left edge search range.
MEH263IS_COORD MEH263IS_LeftHalfPos[MEH263IS_MOTION_LEFT_HALF_POS_LENGTH]	= 
{
	{0,-1},{1,-1},{1,0},{0,1},{1,1}
};

#define MEH263IS_MOTION_RIGHT_HALF_POS_LENGTH 5	// Right edge search range.
MEH263IS_COORD MEH263IS_RightHalfPos[MEH263IS_MOTION_RIGHT_HALF_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{-1,0},{-1,1},{0,1}
};

#define MEH263IS_MOTION_TOP_HALF_POS_LENGTH 5	// Top edge search range.
MEH263IS_COORD MEH263IS_TopHalfPos[MEH263IS_MOTION_TOP_HALF_POS_LENGTH]	= 
{
	{-1,0},{1,0},{-1,1},{0,1},{1,1}
};

#define MEH263IS_MOTION_BOTTOM_HALF_POS_LENGTH 5	// Bottom edge search range.
MEH263IS_COORD MEH263IS_BottomHalfPos[MEH263IS_MOTION_BOTTOM_HALF_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{1,-1},{-1,0},{1,0}
};

// Remaining constrained motion.
#define MEH263IS_MOTION_LEFT_RIGHT_HALF_POS_LENGTH 2	// Left right only search range.
MEH263IS_COORD MEH263IS_LeftRightHalfPos[MEH263IS_MOTION_LEFT_RIGHT_HALF_POS_LENGTH]	= 
{
	{-1,0},{1,0}
};

#define MEH263IS_MOTION_UP_DOWN_HALF_POS_LENGTH 2	// Up down only search range.
MEH263IS_COORD MEH263IS_UpDownHalfPos[MEH263IS_MOTION_UP_DOWN_HALF_POS_LENGTH]	= 
{
	{0,-1},{0,1}
};

#define MEH263IS_MOTION_LEFT_ONLY_HALF_POS_LENGTH 1	// Left only search range.
MEH263IS_COORD MEH263IS_LeftOnlyHalfPos[MEH263IS_MOTION_LEFT_ONLY_HALF_POS_LENGTH]	= 
{
	{-1,0}
};

#define MEH263IS_MOTION_RIGHT_ONLY_HALF_POS_LENGTH 1	// Right only search range.
MEH263IS_COORD MEH263IS_RightOnlyHalfPos[MEH263IS_MOTION_RIGHT_ONLY_HALF_POS_LENGTH]	= 
{
	{1,0}
};

#define MEH263IS_MOTION_UP_ONLY_HALF_POS_LENGTH 1	// Up only search range.
MEH263IS_COORD MEH263IS_UpOnlyHalfPos[MEH263IS_MOTION_UP_ONLY_HALF_POS_LENGTH]	= 
{
	{0,-1}
};

#define MEH263IS_MOTION_DOWN_ONLY_HALF_POS_LENGTH 1	// Down only search range.
MEH263IS_COORD MEH263IS_DownOnlyHalfPos[MEH263IS_MOTION_DOWN_ONLY_HALF_POS_LENGTH]	= 
{
	{0,1}
};

/*
--------------------------------------------------------------------------
  Construction. 
--------------------------------------------------------------------------
*/

MotionEstimatorH263ImplStd::MotionEstimatorH263ImplStd(	const void* pSrc, 
																												const void* pRef, 
																												int					imgWidth, 
																												int					imgHeight)
{
	_ready	= 0;	// Ready to estimate.
	_mode		= 0;	// Speed mode or whatever.

	// Parameters must remain const for the life time of this instantiation.
	_imgWidth				= imgWidth;		// Width of the src and ref images. 
	_imgHeight			= imgHeight;	// Height of the src and ref images.
	_macroBlkWidth	= 16;					// Width of the motion block. Fixed for H.263
	_macroBlkHeight	= 16;					// Height of the motion block.
	_motionRange		= 16;					// (x,y) range of the motion vectors.
	_pInput					= pSrc;
	_pRef						= pRef;

	// Input mem overlay members.
	_pInOver				= NULL;				// Input overlay with motion block dim.

	// Ref mem overlay members.
	_pRefOver				= NULL;				// Ref overlay with motion block dim.

	// Temp working block and its overlay.
	_pMBlk					= NULL;				// Motion block temp mem.
	_pMBlkOver			= NULL;				// Motion block overlay of temp mem.

	// Hold the resulting motion vectors in a structured array.
	_pMotionVectorStruct = NULL;

}//end constructor.

MotionEstimatorH263ImplStd::~MotionEstimatorH263ImplStd(void)
{
	Destroy();
}//end destructor.

/*
--------------------------------------------------------------------------
  Public IMotionEstimator Interface. 
--------------------------------------------------------------------------
*/

int MotionEstimatorH263ImplStd::Create(void)
{
	// Clean out old mem.
	Destroy();

	// --------------- Configure input overlays --------------------------------
	// Put an overlay on the input with the block size set to the vector dim. This
	// is used to access input vectors.
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

	// --------------- Configure ref overlays --------------------------------
	// Overlay the reference and set to the motion block size.  
	_pRefOver	= new OverlayMem2Dv2((void *)_pRef, 
															 _imgWidth, 
															 _imgHeight, 
															 _macroBlkWidth, 
															 _macroBlkHeight);
	if(_pRefOver == NULL)
  {
		Destroy();
	  return(0);
  }//end if !_pRefOver...

	// --------------- Configure temp overlays --------------------------------
	// Alloc some temp mem and overlay it to use for half pel motion estimation and 
	// compensation. The block size is the same as the mem size.
	_pMBlk = new short[_macroBlkWidth * _macroBlkHeight];
	_pMBlkOver = new OverlayMem2Dv2(_pMBlk, _macroBlkWidth, _macroBlkHeight, 
																				_macroBlkWidth, _macroBlkHeight);
	if( (_pMBlk == NULL)||(_pMBlkOver == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pMBlk...

	// --------------- Configure result ---------------------------------------
	// The structure container for the motion vectors.
	_pMotionVectorStruct = new VectorStructList(VectorStructList::SIMPLE2D);
//	_pMotionVectorStruct = new SimpleMotionVectorList();
	if(_pMotionVectorStruct != NULL)
	{
		// How many motion vectors will there be at the block dim.
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

void	MotionEstimatorH263ImplStd::Reset(void)
{
}//end Reset.

void	MotionEstimatorH263ImplStd::SetMode(int mode)
{
	_mode = mode;
}//end SetMode.

/** Motion estimate the source within the reference.
Do the estimation with the block sizes and image sizes defined in
the implementation. The returned type holds the vectors. This is
a standard block full search algorithm without extended boundaries.
@param pSrc		: Input image to estimate (not used).
@param pRef		: Ref to estimate with (not used).
@return				: The list of motion vectors.
*/
void* MotionEstimatorH263ImplStd::Estimate(long* avgDistortion)
{
  int		x,y,m,n;
	// Set the motion vector storage structure.
	int		maxLength	= _pMotionVectorStruct->GetLength();
	int		vecPos		= 0;

	// Accumulate the total error energy for this estimation.
	long	totalEnergy = 0;

  // Do estimation in the sequence order.	Gather the motion vector 
	// MSE data and choose the vector.
  for(m = 0; m < _imgHeight; m += _macroBlkHeight)
		for(n = 0; n < _imgWidth; n += _macroBlkWidth)
  {
		int mx	= 0;
		int my	= 0;
		int hmx = 0;
		int hmy = 0;

		// Set the input and ref block to work with.
		_pInOver->SetOrigin(n,m);
		_pRefOver->SetOrigin(n,m);

		// Measure the [0,0] motion vector as a min reference to beat for 
		// all other motion vectors.
		int zeroVecEnergy = _pInOver->Tsd(*_pRefOver);

		// [0,0] motion vector is the best so far.
		int minEnergy = zeroVecEnergy;

		// Depending on which img boundary we are on will limit the
		// full search range.
		int xlRng, xrRng, yuRng, ydRng;
		GetMotionRange(n, m, &xlRng, &xrRng, &yuRng, &ydRng);

    // Search on a full pel grid over the defined motion range.
    for(y = yuRng; y <= ydRng; y++)
    {
      for(x = xlRng; x <= xrRng; x++)
      {
				// Set the block to the [x,y] motion vector around the [n,m] reference
				// location.
				_pRefOver->SetOrigin(n+x, m+y);

				int vecEnergy = _pInOver->TsdLessThan(*_pRefOver, minEnergy);
				if(vecEnergy < minEnergy)
				{
					minEnergy = vecEnergy;
					mx = x;
					my = y;
				}//end if vecEnergy...
      }//end for x...
    }//end for y...

		// Set the location to the min diff motion vector [mx,my].
		_pRefOver->SetOrigin(n+mx, m+my);

		// Adjust the half pel search range to stay within the global bounds.
		MEH263IS_COORD* pHalfPos;
		int len = GetHalfPelSearchStruct( n+mx, m+my, mx, my, (MEH263IS_COORD **)(&pHalfPos));

    // Search around the min energy full pel motion vector on a half pel grid.
    for(x = 0; x < len; x++)
    {
			// Read the half grid pels into temp.
			_pRefOver->HalfRead(*_pMBlkOver, pHalfPos[x].x, pHalfPos[x].y); 
			int vecEnergy = _pInOver->TsdLessThan(*_pMBlkOver, minEnergy);
			if(vecEnergy < minEnergy)
			{
				minEnergy = vecEnergy;
				hmx = pHalfPos[x].x;
				hmy = pHalfPos[x].y;
			}//end if vecEnergy...
    }//end for x...
  
		// Motion vectors are described in half pel units.
    int mvx = (mx << 1) + hmx;
    int mvy = (my << 1) + hmy;

		// Bounds check used for debugging.
		if(mvx < -32) 
			mvx = -32;
		else if(mvx > 31)
			mvx = 31;
		if(mvy < -32) 
			mvy = -32;
		else if(mvy > 31)
			mvy = 31;

		// Validity of the motion vector is weighted with non-linear factors.
		int weight							= 0;
		int diffWithZeroEnergy	= zeroVecEnergy - minEnergy;
		int magSqr							= (mvx * mvx) + (mvy * mvy);

		// Contribute if motion vector is small.
		if((diffWithZeroEnergy >> 2) < magSqr)
			weight++;
		// Contribute if same order as the noise.
		if(zeroVecEnergy < MEH263IS_FULL_MOTION_NOISE_FLOOR)
			weight++;
		// Contribute if the zero vector and min energy vector are similar.
		if((diffWithZeroEnergy * 10) < minEnergy)
			weight++;

		// Decide whether or not to include this motion vector.
		if((minEnergy < zeroVecEnergy)&&(weight < 2))
    {
			totalEnergy += minEnergy;
    }//end if minEnergy...
    else
		{
			mvx = 0;
			mvy = 0;
			totalEnergy += zeroVecEnergy;
		}//end else...

		// Load the selected vector coord.
		if(vecPos < maxLength)
		{
			_pMotionVectorStruct->SetSimpleElement(vecPos, 0, mvx);
			_pMotionVectorStruct->SetSimpleElement(vecPos, 1, mvy);
			vecPos++;
		}//end if vecPos...

  }//end for m & n...

	*avgDistortion = totalEnergy/maxLength;
	return((void *)_pMotionVectorStruct);
}//end Estimate.

/*
--------------------------------------------------------------------------
  Private methods. 
--------------------------------------------------------------------------
*/

void MotionEstimatorH263ImplStd::Destroy(void)
{
	_ready = 0;

	if(_pInOver != NULL)
		delete _pInOver;
	_pInOver = NULL;

	if(_pRefOver != NULL)
		delete _pRefOver;
	_pRefOver	= NULL;

	if(_pMBlk != NULL)
		delete[] _pMBlk;
	_pMBlk = NULL;

	if(_pMBlkOver != NULL)
		delete _pMBlkOver;
	_pMBlkOver = NULL;

	if(_pMotionVectorStruct != NULL)
		delete _pMotionVectorStruct;
	_pMotionVectorStruct = NULL;

}//end Destroy.

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
int MotionEstimatorH263ImplStd::GetHalfPelSearchStruct(int compx,	int compy, 
																											 int vecx,	int vecy,
																											 MEH263IS_COORD**	ppHalfPos)
{
	int allowedLeft		= 1;	
	int allowedRight	= 1;
	int allowedUp			= 1;		
	int allowedDown		= 1;

	// Disallow motion outside the image boundaries.
	if(compx <= 0)
		allowedLeft = 0;
	else if(compx >= (_imgWidth - 16))
		allowedRight = 0;

	if(compy <= 0)
		allowedUp = 0;
	else if(compy >= (_imgHeight - 16))
		allowedDown = 0;

	// At negative extremes of the motion vector no further motion is allowed
	// in the half negative direction. Override the freedom of motion.
	if(vecx == -_motionRange)
		allowedLeft = 0;
	if(vecy == -_motionRange)
		allowedUp = 0;

	// Within range most likely case.
	if( allowedLeft && allowedRight && allowedUp && allowedDown )
	{
		*ppHalfPos	= MEH263IS_HalfPos;
		return(MEH263IS_MOTION_HALF_POS_LENGTH);
	}//end if inner...
	else if( allowedLeft && allowedRight && !allowedUp && allowedDown )
	{
		*ppHalfPos	= MEH263IS_TopHalfPos;
		return(MEH263IS_MOTION_TOP_HALF_POS_LENGTH);
	}//end if top...
	else if( allowedLeft && allowedRight && allowedUp && !allowedDown )
	{
		*ppHalfPos	= MEH263IS_BottomHalfPos;
		return(MEH263IS_MOTION_BOTTOM_HALF_POS_LENGTH);
	}//end if bottom...
	else if( !allowedLeft && allowedRight && allowedUp && allowedDown )
	{
		*ppHalfPos	= MEH263IS_LeftHalfPos;
		return(MEH263IS_MOTION_LEFT_HALF_POS_LENGTH);
	}//end if left...
	else if( allowedLeft && !allowedRight && allowedUp && allowedDown )
	{
		*ppHalfPos	= MEH263IS_RightHalfPos;
		return(MEH263IS_MOTION_RIGHT_HALF_POS_LENGTH);
	}//end if right...
	else if( !allowedLeft && allowedRight && !allowedUp && allowedDown )
	{
		*ppHalfPos	= MEH263IS_TopLeftHalfPos;
		return(MEH263IS_MOTION_TOP_LEFT_HALF_POS_LENGTH);
	}//end if top left...
	else if( allowedLeft && !allowedRight && !allowedUp && allowedDown )
	{
		*ppHalfPos	= MEH263IS_TopRightHalfPos;
		return(MEH263IS_MOTION_TOP_RIGHT_HALF_POS_LENGTH);
	}//end if top right...
	else if( !allowedLeft && allowedRight && allowedUp && !allowedDown )
	{
		*ppHalfPos	= MEH263IS_BottomLeftHalfPos;
		return(MEH263IS_MOTION_BOTTOM_LEFT_HALF_POS_LENGTH);
	}//end if bottom left...
	else if( allowedLeft && !allowedRight && allowedUp && !allowedDown )
	{
		*ppHalfPos	= MEH263IS_BottomRightHalfPos;
		return(MEH263IS_MOTION_BOTTOM_RIGHT_HALF_POS_LENGTH);
	}//end if bottom right...
	else if( allowedLeft && allowedRight && !allowedUp && !allowedDown )
	{
		*ppHalfPos	= MEH263IS_LeftRightHalfPos;
		return(MEH263IS_MOTION_LEFT_RIGHT_HALF_POS_LENGTH);
	}//end if left right...
	else if( !allowedLeft && !allowedRight && allowedUp && allowedDown )
	{
		*ppHalfPos	= MEH263IS_UpDownHalfPos;
		return(MEH263IS_MOTION_UP_DOWN_HALF_POS_LENGTH);
	}//end if up down...
	else if( allowedLeft && !allowedRight && !allowedUp && !allowedDown )
	{
		*ppHalfPos	= MEH263IS_LeftOnlyHalfPos;
		return(MEH263IS_MOTION_LEFT_ONLY_HALF_POS_LENGTH);
	}//end if left only...
	else if( !allowedLeft && allowedRight && !allowedUp && !allowedDown )
	{
		*ppHalfPos	= MEH263IS_RightOnlyHalfPos;
		return(MEH263IS_MOTION_RIGHT_ONLY_HALF_POS_LENGTH);
	}//end if right only...
	else if( !allowedLeft && !allowedRight && allowedUp && !allowedDown )
	{
		*ppHalfPos	= MEH263IS_UpOnlyHalfPos;
		return(MEH263IS_MOTION_UP_ONLY_HALF_POS_LENGTH);
	}//end if up only...
	else if( !allowedLeft && !allowedRight && !allowedUp && allowedDown )
	{
		*ppHalfPos	= MEH263IS_DownOnlyHalfPos;
		return(MEH263IS_MOTION_DOWN_ONLY_HALF_POS_LENGTH);
	}//end if down only...

	*ppHalfPos	= MEH263IS_HalfPos;
	return(0);
}//end GetHalfPelSearchStruct.

/** Get the allowed motion range for this block.
The search area for std H.263 is within the bounds of the image
dimensions. The range is limited at the corners and edges of the
images.
@param x				: X coord of block.
@param y				: Y coord of block.
@param xlr			: Returned allowed x left range.
@param xrr			: Returned allowed x right range.
@param yur			: Returned allowed y up range.
@param ydr			: Returned allowed y down range.
@return					: none.
*/
void MotionEstimatorH263ImplStd::GetMotionRange(int  x,		int  y, 
																								int* xlr, int* xrr, 
																								int* yur, int* ydr)
{
	if( (x - _motionRange) >= 0 )
		*xlr = -_motionRange;	// -16
	else
		*xlr = 0;
	if( (x + _motionRange) < _imgWidth )
		*xrr = _motionRange - 1;	// 15
	else
		*xrr = 0;

	if( (y - _motionRange) >= 0 )
		*yur = -_motionRange;	// -16
	else
		*yur = 0;
	if( (y + _motionRange) < _imgHeight )
		*ydr = _motionRange - 1;	// 15
	else
		*ydr = 0;

}//end GetMotionRange.



