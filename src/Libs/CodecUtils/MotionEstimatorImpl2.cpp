/** @file

MODULE						: MotionEstimatorImpl2

TAG								: MEI2

FILE NAME					: MotionEstimatorImpl2.cpp

DESCRIPTION				: A standard motion estimator implementation of absolute
										error difference measure.	Access via a IMotionEstimator
										interface. There are 2 levels of execution speed vs.
										quality.

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


#include	"SimpleMotionVectorList.h"
#include	"MotionEstimatorImpl2.h"

/*
--------------------------------------------------------------------------
  Constants. 
--------------------------------------------------------------------------
*/
// Calc = ((16[vec dim] * 16[vec dim]) * 2.
#define MEI2_FULL_MOTION_NOISE_FLOOR					512
#define MEI2_MOTION_NOISE_FLOOR								26 //24 //sqrt 512
#define MEI2_L0_MOTION_VECTOR_REFINED_RANGE		2
#define MEI2_L1_MOTION_VECTOR_REFINED_RANGE		2

// Search range coords for centre motion vectors.
#define MEI2_MOTION_HALF_POS_LENGTH 	8
MEI2_COORD MEI2_HalfPos[MEI2_MOTION_HALF_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}
};

// Search range coords for corner motion vectors.
#define MEI2_MOTION_TOP_LEFT_HALF_POS_LENGTH 3		// Top left search range.
MEI2_COORD MEI2_TopLeftHalfPos[MEI2_MOTION_TOP_LEFT_HALF_POS_LENGTH]	= 
{
	{1,0},{0,1},{1,1}
};

#define MEI2_MOTION_TOP_RIGHT_HALF_POS_LENGTH 3	// Top right search range.
MEI2_COORD MEI2_TopRightHalfPos[MEI2_MOTION_TOP_RIGHT_HALF_POS_LENGTH]	= 
{
	{-1,0},{-1,1},{0,1}
};

#define MEI2_MOTION_BOTTOM_LEFT_HALF_POS_LENGTH 3 // Bottom left search range.
MEI2_COORD MEI2_BottomLeftHalfPos[MEI2_MOTION_BOTTOM_LEFT_HALF_POS_LENGTH]	= 
{
	{0,-1},{1,-1},{1,0}
};

#define MEI2_MOTION_BOTTOM_RIGHT_HALF_POS_LENGTH 3	//Bottom right search range.
MEI2_COORD MEI2_BottomRightHalfPos[MEI2_MOTION_BOTTOM_RIGHT_HALF_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{-1,0}
};

// Search range coords for edge motion vectors.
#define MEI2_MOTION_LEFT_HALF_POS_LENGTH 5	// Left edge search range.
MEI2_COORD MEI2_LeftHalfPos[MEI2_MOTION_LEFT_HALF_POS_LENGTH]	= 
{
	{0,-1},{1,-1},{1,0},{0,1},{1,1}
};

#define MEI2_MOTION_RIGHT_HALF_POS_LENGTH 5	// Right edge search range.
MEI2_COORD MEI2_RightHalfPos[MEI2_MOTION_RIGHT_HALF_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{-1,0},{-1,1},{0,1}
};

#define MEI2_MOTION_TOP_HALF_POS_LENGTH 5	// Top edge search range.
MEI2_COORD MEI2_TopHalfPos[MEI2_MOTION_TOP_HALF_POS_LENGTH]	= 
{
	{-1,0},{1,0},{-1,1},{0,1},{1,1}
};

#define MEI2_MOTION_BOTTOM_HALF_POS_LENGTH 5	// Bottom edge search range.
MEI2_COORD MEI2_BottomHalfPos[MEI2_MOTION_BOTTOM_HALF_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{1,-1},{-1,0},{1,0}
};

/*
--------------------------------------------------------------------------
  Construction. 
--------------------------------------------------------------------------
*/

MotionEstimatorImpl2::MotionEstimatorImpl2(	const void* pSrc, 
																						const void* pRef, 
																						int					imgWidth, 
																						int					imgHeight,
																						int					macroBlkWidth,
																						int					macroBlkHeight,
																						int					motionRange)
{
	_ready	= 0;	// Ready to estimate.
	_mode		= 1;	// Speed mode or whatever. Default to slower speed.

	// Parameters must remain const for the life time of this instantiation.
	_imgWidth				= imgWidth;					// Width of the src and ref images. 
	_imgHeight			= imgHeight;				// Height of the src and ref images.
	_macroBlkWidth	= macroBlkWidth;		// Width of the motion block.
	_macroBlkHeight	= macroBlkHeight;		// Height of the motion block.
	_motionRange		= motionRange;			// (x,y) range of the motion vectors.
	_pInput					= pSrc;
	_pRef						= pRef;

	// Level 0: Input mem overlay members.
	_pInOver					= NULL;			// Input overlay with motion block dim.
	// Level 1: Subsampled input by 2.
	_pInL1						= NULL;			// Input mem at (_l1Width * _l1Height).
	_l1Width					= 0;
	_l1Height					= 0;
	_l1MacroBlkWidth	= 0;
	_l1MacroBlkHeight = 0;
	_l1MotionRange		= 0;
	_pInL1Over				= NULL;			// Input overlay with level 1 motion block dim.
	// Level 2: Subsampled input by 4.
	_pInL2						= NULL;			// Input mem at (_l2Width * _l2Height).
	_l2Width					= 0;
	_l2Height					= 0;
	_l2MacroBlkWidth	= 0;
	_l2MacroBlkHeight = 0;
	_l2MotionRange		= 0;
	_pInL2Over				= NULL;			// Input overlay with level 2 motion block dim.

	// Level 0: Ref mem overlay members.
	_pRefOver					= NULL;			// Ref overlay with whole block dim.
	_pExtRef					= NULL;			// Extended ref mem created by ExtendBoundary() call.
	_extWidth					= 0;
	_extHeight				= 0;
	_pExtRefOverAll		= NULL;			// Extended ref overlay with whole block dim.
	_pExtRefOver			= NULL;			// Extended ref overlay with motion block dim.
	// Level 1: Subsampled ref by 2.
	_pRefL1						= NULL;			// Ref mem at (_l1Width * _l1Height).
	_pRefL1Over				= NULL; 		// Ref overlay with whole level 1 block dim.
	_pExtRefL1				= NULL;			// Extended ref mem create by ExtendBoundary() call.
	_extL1Width				= 0;
	_extL1Height			= 0;
	_pExtRefL1OverAll	= NULL;			// Ref overlay with whole block dim.
	_pExtRefL1Over		= NULL;			// Ref overlay with level 1 motion block dim.
	// Level 2: Subsampled ref by 4.
	_pRefL2						= NULL;			// Ref mem at (_l2Width * _l2Height).
	_pRefL2Over				= NULL; 		// Ref overlay with whole level 2 block dim.
	_pExtRefL2				= NULL;			// Extended ref mem create by ExtendBoundary() call.
	_extL2Width				= 0;
	_extL2Height			= 0;
	_pExtRefL2OverAll	= NULL;			// Ref overlay with whole block dim.
	_pExtRefL2Over		= NULL;			// Ref overlay with level 2 motion block dim.

	// Temp working block and its overlay.
	_pMBlk						= NULL;			// Motion block temp mem.
	_pMBlkOver				= NULL;			// Motion block overlay of temp mem.

	// Hold the resulting motion vectors in a byte array.
	_pMotionVectorStruct = NULL;

}//end constructor.

MotionEstimatorImpl2::~MotionEstimatorImpl2(void)
{
	Destroy();
}//end destructor.

/*
--------------------------------------------------------------------------
  Public IMotionEstimator Interface. 
--------------------------------------------------------------------------
*/

int MotionEstimatorImpl2::Create(void)
{
	// Clean out old mem.
	Destroy();

	// --------------- Configure input overlays --------------------------------
	// Level 0: Put an overlay on the input with the block size set to the vector 
	// dim. This is used to access input vectors.
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

	// Level 1 is Level 0 sub sampled by 2.
	_l1Width					= _imgWidth/2;
	_l1Height					= _imgHeight/2;
	_l1MacroBlkWidth	= _macroBlkWidth/2;
	_l1MacroBlkHeight = _macroBlkHeight/2;
	_l1MotionRange		= _motionRange/2;

	// Level 1: Input mem at (_l1Width * _l1Height) must be alloc.
	_pInL1 = new short[_l1Width * _l1Height];

	// Level 1: Input overlay with level 1 motion block dim.
	if(_pInL1 != NULL)
	{
		_pInL1Over = new OverlayMem2Dv2(_pInL1, 
																	_l1Width, 
																	_l1Height, 
																	_l1MacroBlkWidth, 
																	_l1MacroBlkHeight);
		if(_pInL1Over == NULL)
		{
			Destroy();
			return(0);
		}//end if !_pInL1Over...
	}//end if ...
	else
	{
		Destroy();
		return(0);
	}//end else...

	// Level 2 is Level 1 sub sampled by 2.
	_l2Width					= _l1Width/2;
	_l2Height					= _l1Height/2;
	_l2MacroBlkWidth	= _l1MacroBlkWidth/2;
	_l2MacroBlkHeight = _l1MacroBlkHeight/2;
	_l2MotionRange		= _l1MotionRange/2;

	// Level 2: Input mem at (_l2Width * _l2Height) must be alloc.
	_pInL2 = new short[_l2Width * _l2Height];

	// Level 2: Input overlay with level 2 motion block dim.
	if(_pInL2 != NULL)
	{
		_pInL2Over = new OverlayMem2Dv2(_pInL2, 
																	_l2Width, 
																	_l2Height, 
																	_l2MacroBlkWidth, 
																	_l2MacroBlkHeight);
		if(_pInL2Over == NULL)
		{
			Destroy();
			return(0);
		}//end if !_pInL2Over...
	}//end if ...
	else
	{
		Destroy();
		return(0);
	}//end else...

	// --------------- Configure ref overlays --------------------------------
	// Level 0: Overlay the whole reference. The reference will have an extended 
	// boundary for motion estimation and must therefore have its own mem.
	_pRefOver = new OverlayMem2Dv2((void *)_pRef, _imgWidth, _imgHeight, _imgWidth, _imgHeight);
	if(_pRefOver == NULL)
  {
		Destroy();
	  return(0);
  }//end if !_pRefOver...

	if(!OverlayExtMem2Dv2::ExtendBoundary((void *)_pRef, 
																			_imgWidth,						
																			_imgHeight, 
																			_motionRange + 1, // Extend left and right by...
																			_motionRange + 1,	// Extend top and bottom by...
																			(void **)(&_pExtRef)) )	// Created in the method.
  {
		Destroy();
	  return(0);
  }//end if !ExtendBoundary...
	_extWidth	 = _imgWidth + (_motionRange + 1)*2;
	_extHeight = _imgHeight + (_motionRange + 1)*2;

	// Level 0: Place the overlay onto the whole ext boundary mem.
	_pExtRefOverAll = new OverlayExtMem2Dv2(_pExtRef, 
																				_extWidth,				// new mem width.
																				_extHeight,				// new mem height.
																				_imgWidth,				// block width.
																				_imgHeight,				// block height.
																				_motionRange + 1,	// boundary width.
																				_motionRange + 1);// boundary height.

	// Level 0: Place another overlay on the extended boundary ref with block 
	// size set to the motion vec dim..
	_pExtRefOver = new OverlayExtMem2Dv2(_pExtRef,					// Src description. 
																		 _extWidth, 
																		 _extHeight,
																		 _macroBlkWidth,		// Block size description.
																		 _macroBlkHeight,
																		 _motionRange + 1,	// Boundary size for both left and right.
																		 _motionRange + 1 );
	if( (_pExtRefOverAll == NULL)||(_pExtRefOver == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pExtRefOverAll...

	// Level 1: Ref mem at (_l1Width * _l1Height).
	_pRefL1 = new short[_l1Width * _l1Height];

	// Level 1: Ref overlay with whole level 1 block dim.
	if(_pRefL1 != NULL)
	{
		_pRefL1Over = new OverlayMem2Dv2(_pRefL1, _l1Width, _l1Height, _l1Width, _l1Height);
		if(_pRefL1Over == NULL)
		{
			Destroy();
			return(0);
		}//end if _pRefL1Over...
	}//end if _pRefL1...
	else
	{
		Destroy();
		return(0);
	}//end else...

	// Level 1: Extended ref mem _pExtRefL1 created by ExtendBoundary() call.
	if(!OverlayExtMem2Dv2::ExtendBoundary((void *)_pRefL1, 
																			_l1Width,						
																			_l1Height, 
																			_l1MotionRange + 1,
																			_l1MotionRange + 1,
																			(void **)(&_pExtRefL1)) )
  {
    Destroy();
	  return(0);
  }//end if !ExtendBoundary...
	_extL1Width		= _l1Width + (_l1MotionRange + 1)*2;
	_extL1Height	= _l1Height + (_l1MotionRange + 1)*2;

	// Level 1: Ref overlay with whole block dim.
	_pExtRefL1OverAll = new OverlayExtMem2Dv2(_pExtRefL1, 
																					_extL1Width,				// new mem width.
																					_extL1Height,				// new mem height.
																					_l1Width,						// block width.
																					_l1Height,					// block height.
																					_l1MotionRange + 1, // boundary width.
																					_l1MotionRange + 1);// boundary height.

	// Level 1: Ref overlay with level 1 motion block dim.
	_pExtRefL1Over = new OverlayExtMem2Dv2(_pExtRefL1, 
																			 _extL1Width,					// new mem width.
																			 _extL1Height,				// new mem height.
																			 _l1MacroBlkWidth,		// block width.
																			 _l1MacroBlkHeight,		// block height.
																			 _l1MotionRange + 1,	// boundary width.
																			 _l1MotionRange + 1);	// boundary height.
	if( (_pExtRefL1OverAll == NULL)||(_pExtRefL1Over == NULL) )
  {
    Destroy();
	  return(0);
  }//end if !_pExtRefL1OverAll...

	// Level 2: Ref mem at (_l2Width * _l2Height).
	_pRefL2 = new short[_l2Width * _l2Height];

	// Level 2: Ref overlay with whole level 2 block dim.
	if(_pRefL2 != NULL)
	{
		_pRefL2Over = new OverlayMem2Dv2(_pRefL2, _l2Width, _l2Height, _l2Width, _l2Height);
		if(_pRefL2Over == NULL)
		{
			Destroy();
			return(0);
		}//end if _pRefL2Over...
	}//end if _pRefL2...
	else
	{
		Destroy();
		return(0);
	}//end else...

	// Level 2: Extended ref mem _pExtRefL2 created by ExtendBoundary() call.
	if(!OverlayExtMem2Dv2::ExtendBoundary((void *)_pRefL2, 
																			_l2Width,						
																			_l2Height, 
																			_l2MotionRange + 1,
																			_l2MotionRange + 1,
																			(void **)(&_pExtRefL2)) )
  {
    Destroy();
	  return(0);
  }//end if !ExtendBoundary...
	_extL2Width		= _l2Width + (_l2MotionRange + 1)*2;
	_extL2Height	= _l2Height + (_l2MotionRange + 1)*2;

	// Level 2: Ref overlay with whole block dim.
	_pExtRefL2OverAll = new OverlayExtMem2Dv2(_pExtRefL2, 
																					_extL2Width,				// new mem width.
																					_extL2Height,				// new mem height.
																					_l2Width,						// block width.
																					_l2Height,					// block height.
																					_l2MotionRange + 1, // boundary width.
																					_l2MotionRange + 1);// boundary height.

	// Level 2: Ref overlay with level 2 motion block dim.
	_pExtRefL2Over = new OverlayExtMem2Dv2(_pExtRefL2, 
																			 _extL2Width,					// new mem width.
																			 _extL2Height,				// new mem height.
																			 _l2MacroBlkWidth,		// block width.
																			 _l2MacroBlkHeight,		// block height.
																			 _l2MotionRange + 1,	// boundary width.
																			 _l2MotionRange + 1);	// boundary height.
	if( (_pExtRefL2OverAll == NULL)||(_pExtRefL2Over == NULL) )
  {
    Destroy();
	  return(0);
  }//end if !_pExtRefL2OverAll...

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
	_pMotionVectorStruct = new SimpleMotionVectorList();
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

void	MotionEstimatorImpl2::Reset(void)
{
}//end Reset.

/** Set the speed mode.
This is a multresolution algorithm and the speed is increased by estimating
at lower levels but it is less accurate. Mode = 1 implies level 1 and mode = 2
for level 2. Mode = 0 is auto mode that selects an appropriate mode depending
on the resolution of the image. The selection is set to 2 if the image has
an area larger than 200x200.

@param pRef		: Ref to estimate with.
@return				: The list of motion vectors.
*/
void	MotionEstimatorImpl2::SetMode(int mode)
{
	if(mode == 0)	// Auto mode.
	{
		int area = _imgWidth * _imgHeight;
		if(area > 40000)
			_mode = 2;
		else
			_mode = 1;
	}//end if mode...
	else
		_mode = mode;
}//end SetMode.

/** Motion estimate the source within the reference.
Do the estimation with the block sizes and image sizes defined in
the implementation. The returned type holds the vectors. This is
a multiresolution search algorithm with extended boundaries and a
absolute difference criterion. The mode setting (_mode) determines
the level of resolution.
@param pSrc		: Input image to estimate.
@param pRef		: Ref to estimate with.
@return				: The list of motion vectors.
*/
void* MotionEstimatorImpl2::Estimate(long* avgDistortion)
{
  int		i,j,m,n,p,q,k,l;
	long	totalDifference = 0;

	// Set the motion vector byte storage structure.
	char* pMv				= (char *)(_pMotionVectorStruct->GetDataPtr());
	int		maxLength	= _pMotionVectorStruct->GetLength();
	int   step			= _pMotionVectorStruct->GetPatternSize();
	int		length		= 0;

	// Level 1 motion range is adjusted to a refinement if level 2 (_mode = 2)
	// is to be used.
	int lclL1MotionRange = _l1MotionRange;
	if(_mode == 2)
		lclL1MotionRange = MEI2_L1_MOTION_VECTOR_REFINED_RANGE;

	// Write the level 0 ref and fill its extended boundary. The centre part of
	// _pExtRefOverAll is copied from _pRefOver before filling the boundary.
	_pExtRefOverAll->Write(*_pRefOver);
	_pExtRefOverAll->FillBoundaryProxy();

// NOTE: These 2 calls can be collapsed into 1 by using the offsets to write straight
//			 into the extended boundary ref if level 2 is not required.
	// Subsample level 0 ref (_pRefOver) to produce level 1 ref (_pRefL1Over).
	OverlayMem2Dv2::Half( (void **)(_pRefOver->Get2DSrcPtr()),			// Src 2D ptr.
										  _imgWidth, 																// Src width.
											_imgHeight, 															// Src height.
											(void **)(_pRefL1Over->Get2DSrcPtr()) );	// Dest 2D ptr.
	// Write the level 1 extended ref and fill its boundary. 
	_pExtRefL1OverAll->Write(*_pRefL1Over);
	_pExtRefL1OverAll->FillBoundaryProxy();

	if(_mode == 2)
	{
		// Subsample level 1 ref (_pRefL1Over) to produce level 2 ref (_pRefL2Over).
		OverlayMem2Dv2::Half( (void **)(_pRefL1Over->Get2DSrcPtr()),		// Src 2D ptr.
												_l1Width, 																// Src width.
												_l1Height, 																// Src height.
												(void **)(_pRefL2Over->Get2DSrcPtr()) );	// Dest 2D ptr.
		// Write the level 2 extended ref and fill its boundary. 
		_pExtRefL2OverAll->Write(*_pRefL2Over);
		_pExtRefL2OverAll->FillBoundaryProxy();
	}//end if _mode...

	// Subsample level 0 input (_pInOver) to produce level 1 input (_pInL1Over).
	OverlayMem2Dv2::Half( (void **)(_pInOver->Get2DSrcPtr()),			// Src 2D ptr.
										  _imgWidth, 															// Src width.
											_imgHeight, 														// Src height.
											(void **)(_pInL1Over->Get2DSrcPtr()) ); // Dest 2D ptr.

	if(_mode == 2)
	{
		// Subsample level 1 input (_pInL1Over) to produce level 2 input (_pInL2Over).
		OverlayMem2Dv2::Half( (void **)(_pInL1Over->Get2DSrcPtr()),		// Src 2D ptr.
												_l1Width, 															// Src width.
												_l1Height, 															// Src height.
												(void **)(_pInL2Over->Get2DSrcPtr()) ); // Dest 2D ptr.
	}//end if _mode...

	// Gathers the motion vector absolute differnce data and choose the vector.
	// m,n step level 0 vec dim = _macroBlkHeight, _macroBlkWidth.
	// p,q step level 1 vec dim = _l1MacroBlkHeight, _l1MacroBlkWidth.
	// k,l step level 2 vec dim = _l2MacroBlkHeight, _l2MacroBlkWidth.
  for(m = 0, p = 0, k = 0; m < _imgHeight; m += _macroBlkHeight, p += _l1MacroBlkHeight, k += _l2MacroBlkHeight)
		for(n = 0, q = 0, l = 0; n < _imgWidth; n += _macroBlkWidth, q += _l1MacroBlkWidth, l += _l2MacroBlkWidth)
  {
		int mx	= 0;	// Full pel grid.
		int my	= 0;
		int hmx	= 0;	// Half pel grid.
		int hmy	= 0;

		// The [0,0] vector	difference between the input and ref blocks is the most
		// likely candidate and is therefore the starting point.

		// Level 0: Set the input and ref blocks to work with.
		_pInOver->SetOrigin(n,m);
		_pExtRefOver->SetOrigin(n,m);
		// Absolute diff comparison method.
		int zeroVecDiff = _pInOver->Tad16x16(*_pExtRefOver);
		int minDiff			= zeroVecDiff;	// Best so far.

		//----------------------- Level 2 full pel search --------------------------------
		if(_mode == 2)
		{
			// Level 2: Set the input and ref blocks.
			_pInL2Over->SetOrigin(l,k);
			_pExtRefL2Over->SetOrigin(l,k);
			// Absolute diff comparison method.
			int minDiffL2 = _pInL2Over->Tad4x4(*_pExtRefL2Over);
			
    	// Level 2: Search on a full pel grid over the defined L2 motion range.
    	for(i = -(_l2MotionRange); i <= _l2MotionRange; i++)
    	{
				int blkDiff;
    	  for(j = -(_l2MotionRange); j <= _l2MotionRange; j++)
    	  {
					// Early exit because zero motion vec already checked.
					if( !(i||j) )	goto MEI2_LEVEL2_BREAK;
			
					// Set the block to the [j,i] motion vector around the [l,k] reference
					// location.
					_pExtRefL2Over->SetOrigin(l+j, k+i);
					blkDiff = _pInL2Over->Tad4x4LessThan(*_pExtRefL2Over, minDiffL2);
					if(blkDiff <= minDiffL2)
					{
						// Weight the equal diff with the smallest motion vector magnitude. 
						if(blkDiff == minDiffL2)
						{
							int vecDistDiff = ( (my*my)+(mx*mx) )-( (i*i)+(j*j) );
							if(vecDistDiff < 0)
								goto MEI2_LEVEL2_BREAK;
						}//end if blkDiff...
			
						minDiffL2 = blkDiff;
						mx = j;
						my = i;
					}//end if blkDiff...
			
					MEI2_LEVEL2_BREAK: ; // null.
    	  }//end for j...
    	}//end for i...
			
			mx = mx * 2; // Convert level 2 to level 1 grid units ( x2 ).
			my = my * 2;
			
			// Make sure we stay within the level 1 range before the search.
			if( (mx + MEI2_L1_MOTION_VECTOR_REFINED_RANGE) >= _l1MotionRange )
				mx = (_l1MotionRange - 1 - MEI2_L1_MOTION_VECTOR_REFINED_RANGE);
			else if( (mx - MEI2_L1_MOTION_VECTOR_REFINED_RANGE) < -(_l1MotionRange) )
				mx = -(_l1MotionRange - MEI2_L1_MOTION_VECTOR_REFINED_RANGE);
			if( (my + MEI2_L1_MOTION_VECTOR_REFINED_RANGE) >= _l1MotionRange )
				my = (_l1MotionRange - 1 - MEI2_L1_MOTION_VECTOR_REFINED_RANGE);
			else if( (my - MEI2_L1_MOTION_VECTOR_REFINED_RANGE) < -(_l1MotionRange) )
				my = -(_l1MotionRange - MEI2_L1_MOTION_VECTOR_REFINED_RANGE);
		}//end if _mode...

		//----------------------- Level 1 full pel search --------------------------------
		// Level 1: Set the input and ref blocks.
		_pInL1Over->SetOrigin(q,p);
		_pExtRefL1Over->SetOrigin(mx+q,my+p);
		// Absolute diff comparison method.
		int minDiffL1 = _pInL1Over->Tad8x8(*_pExtRefL1Over);

    // Level 1: Search on a full pel grid over the defined L1 motion range. The
		//					range is a refinement if Level 2 was done (_mode = 2).
		int rmx = 0;	// Refinement motion vector centre.
		int rmy = 0;
    for(i = -(lclL1MotionRange); i <= lclL1MotionRange; i++)
    {
			int blkDiff;
      for(j = -(lclL1MotionRange); j <= lclL1MotionRange; j++)
      {
				// Early exit because zero (centre) motion vec already checked.
				if( !(i||j) )	goto MEI2_LEVEL1_BREAK;

				// Set the block to the [j,i] motion vector around the [mx+p,my+q] reference
				// location.
				_pExtRefL1Over->SetOrigin(mx+q+j, my+p+i);
				blkDiff = _pInL1Over->Tad8x8LessThan(*_pExtRefL1Over, minDiffL1);
				if(blkDiff <= minDiffL1)
				{
					// Weight the equal diff with the smallest motion vector magnitude. 
					if(blkDiff == minDiffL1)
					{
						int vecDistDiff = ( ((my+rmy)*(my+rmy))+((mx+rmx)*(mx+rmx)) )-( ((my+i)*(my+i))+((mx+j)*(mx+j)) );
						if(vecDistDiff < 0)
							goto MEI2_LEVEL1_BREAK;
					}//end if blkDiff...

					minDiffL1 = blkDiff;
					rmx = j;
					rmy = i;
				}//end if blkDiff...

				MEI2_LEVEL1_BREAK: ; // null.
      }//end for j...
    }//end for i...

		mx = (mx+rmx) * 2; // Convert level 1 to level 0 grid units ( x2 ) and add the refinement.
		my = (my+rmy) * 2;

		// Make sure we stay within the global range before the search. Global = [-64..63]
		if( (mx + MEI2_L0_MOTION_VECTOR_REFINED_RANGE) >= _motionRange )
			mx = (_motionRange - 1 - MEI2_L0_MOTION_VECTOR_REFINED_RANGE);
		else if( (mx - MEI2_L0_MOTION_VECTOR_REFINED_RANGE) < -(_motionRange) )
			mx = -(_motionRange - MEI2_L0_MOTION_VECTOR_REFINED_RANGE);
		if( (my + MEI2_L0_MOTION_VECTOR_REFINED_RANGE) >= _motionRange )
			my = (_motionRange - 1 - MEI2_L0_MOTION_VECTOR_REFINED_RANGE);
		else if( (my - MEI2_L0_MOTION_VECTOR_REFINED_RANGE) < -(_motionRange) )
			my = -(_motionRange - MEI2_L0_MOTION_VECTOR_REFINED_RANGE);

		//----------------------- Level 0 full pel refined search ------------------------
    // Level 0: Search on a full pel grid over the defined L0 refined motion range.

		// Get the min diff at this location in level 0 grid units.
		_pExtRefOver->SetOrigin(n+mx,m+my);
		minDiff = _pInOver->Tad16x16(*_pExtRefOver);

		// Look for an improvement on the motion vector calc above within the refined range.
		rmx = 0;	// Refinement motion vector centre.
		rmy = 0;
    for(i = -(MEI2_L0_MOTION_VECTOR_REFINED_RANGE); i <= MEI2_L0_MOTION_VECTOR_REFINED_RANGE; i++)
    {
			int blkDiff;
      for(j = -(MEI2_L0_MOTION_VECTOR_REFINED_RANGE); j <= MEI2_L0_MOTION_VECTOR_REFINED_RANGE; j++)
      {
				// Early exit because zero motion vec already checked.
				if( !(i||j) )	goto MEI2_LEVEL0_BREAK;
				// Set the block to the [j,i] motion vector around the [n+mx,m+my] reference
				// location.
				_pExtRefOver->SetOrigin(n+mx+j, m+my+i);
				blkDiff = _pInOver->Tad16x16LessThan(*_pExtRefOver, minDiff);
				if(blkDiff <= minDiff)
				{
					// Weight the equal diff with the smallest global motion vector magnitude. 
					if(blkDiff == minDiff)
					{
						int vecDistDiff = ( ((my+rmy)*(my+rmy))+((mx+rmx)*(mx+rmx)) )-( ((my+i)*(my+i))+((mx+j)*(mx+j)) );
						if(vecDistDiff < 0)
							goto MEI2_LEVEL0_BREAK;
					}//end if blkDiff...

					minDiff = blkDiff;
					rmx = j;
					rmy = i;
				}//end if blkDiff...

				MEI2_LEVEL0_BREAK: ; // null.
      }//end for j...
    }//end for i...
		// Add the refinement.
		mx += rmx;
		my += rmy;

		//----------------------- Level 0 half pel refined search ------------------------
    // Search around the min diff full pel motion vector on a half pel grid.

		// Set the location to the min diff motion vector [mx,my].
		_pExtRefOver->SetOrigin(n+mx, m+my);

		// Adjust the search range to stay within the global bounds.
		MEI2_COORD* pHalfPos;
		int len = GetHalfPelSearchStruct(mx, my, (MEI2_COORD **)(&pHalfPos));

    for(int x = 0; x < len; x++)
    {
			// Read the half grid pels into temp.
			_pExtRefOver->HalfRead(*_pMBlkOver, pHalfPos[x].x, pHalfPos[x].y); 
			int blkDiff = _pInOver->Tad16x16LessThan(*_pMBlkOver, minDiff);
			if(blkDiff < minDiff)
			{
				minDiff = blkDiff;
				hmx = pHalfPos[x].x;
				hmy = pHalfPos[x].y;
			}//end if blkDiff...
    }//end for x...

		// Add the refinement in half pel units.
		int mvx = (mx << 1) + hmx;
		int mvy = (my << 1) + hmy;

		// Bounds check used for debugging.
		//if(mvx < -64) 
		//	mvx = -64;
		//else if(mvx > 63)
		//	mvx = 63;
		//if(mvy < -64) 
		//	mvy = -64;
		//else if(mvy > 63)
		//	mvy = 63;

		// Validity of the motion vector is weighted with non-linear factors.
		int weight							= 0;
		int diffWithZeroDiff	= zeroVecDiff - minDiff;
		int magSqr							= (mvx * mvx) + (mvy * mvy);

		// Contribute if motion vector is small.
		if((diffWithZeroDiff >> 1) < magSqr)
			weight++;
		// Contribute if same order as the noise.
		if(zeroVecDiff < MEI2_MOTION_NOISE_FLOOR)
			weight++;
		// Contribute if the zero vector and min diff vector are similar.
		if((diffWithZeroDiff * 7) < minDiff)
			weight++;

		// Determine the basic run-length motion.
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

		// Load the selected vector coord.
		if(length < maxLength)
		{
			*pMv			= (char)mvx;
			*(pMv+1)	= (char)mvy;
			pMv += step;
			length++;
		}//end if length...

  }//end for m & n...

	// In this context avg distortion is actually avg difference.
	*avgDistortion = totalDifference/maxLength;
	return((void *)_pMotionVectorStruct);

}//end Estimate.

/*
--------------------------------------------------------------------------
  Private methods. 
--------------------------------------------------------------------------
*/

void MotionEstimatorImpl2::Destroy(void)
{
	_ready = 0;

	if(_pInOver != NULL)
		delete _pInOver;
	_pInOver = NULL;

	if(_pInL1 != NULL)
		delete[] _pInL1;
	_pInL1 = NULL;

	if(_pInL1Over != NULL)
		delete _pInL1Over;
	_pInL1Over = NULL;

	if(_pInL2 != NULL)
		delete[] _pInL2;
	_pInL2 = NULL;

	if(_pInL2Over != NULL)
		delete _pInL2Over;
	_pInL2Over = NULL;

	if(_pRefOver != NULL)
		delete _pRefOver;
	_pRefOver	= NULL;

	if(_pExtRef != NULL)
		delete[] _pExtRef;
	_pExtRef = NULL;

	if(_pExtRefOverAll != NULL)
		delete _pExtRefOverAll;
	_pExtRefOverAll	= NULL;

	if(_pExtRefOver != NULL)
		delete _pExtRefOver;
	_pExtRefOver = NULL;

	if(_pRefL1 != NULL)
		delete[] _pRefL1;
	_pRefL1	= NULL;

	if(_pRefL1Over != NULL)
		delete _pRefL1Over;
	_pRefL1Over	= NULL;

	if(_pExtRefL1 != NULL)
		delete[] _pExtRefL1;
	_pExtRefL1 = NULL;

	if(_pExtRefL1OverAll != NULL)
		delete _pExtRefL1OverAll;
	_pExtRefL1OverAll	= NULL;

	if(_pExtRefL1Over != NULL)
		delete _pExtRefL1Over;
	_pExtRefL1Over = NULL;

	if(_pRefL2 != NULL)
		delete[] _pRefL2;
	_pRefL2	= NULL;

	if(_pRefL2Over != NULL)
		delete _pRefL2Over;
	_pRefL2Over	= NULL;

	if(_pExtRefL2 != NULL)
		delete[] _pExtRefL2;
	_pExtRefL2 = NULL;

	if(_pExtRefL2OverAll != NULL)
		delete _pExtRefL2OverAll;
	_pExtRefL2OverAll	= NULL;

	if(_pExtRefL2Over != NULL)
		delete _pExtRefL2Over;
	_pExtRefL2Over = NULL;

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
is at the limit of its search range. Global range = [-64..63].
@param x				: X motion vector coord.
@param y				: Y motion vector coord.
@param halfPos	: Returned correct struct.
@return					: Length of the struct.
*/
int MotionEstimatorImpl2::GetHalfPelSearchStruct(int x, int y, MEI2_COORD** ppHalfPos)
{
	bool atRightExtreme		= (x == (_motionRange - 1));
	bool atLeftExtreme		= (x == -_motionRange);
	bool atBottomExtreme	= (y == (_motionRange - 1));
	bool atTopExtreme			= (y == -_motionRange);
	// Within range most likely case.
	if( !(atRightExtreme || atLeftExtreme || atBottomExtreme || atTopExtreme) )
	{
		*ppHalfPos	= MEI2_HalfPos;
		return(MEI2_MOTION_HALF_POS_LENGTH);
	}//end if inner...
	else if( !atRightExtreme && !atLeftExtreme && !atBottomExtreme && atTopExtreme )
	{
		*ppHalfPos	= MEI2_TopHalfPos;
		return(MEI2_MOTION_TOP_HALF_POS_LENGTH);
	}//end if top...
	else if( !atRightExtreme && !atLeftExtreme && atBottomExtreme && !atTopExtreme )
	{
		*ppHalfPos	= MEI2_BottomHalfPos;
		return(MEI2_MOTION_BOTTOM_HALF_POS_LENGTH);
	}//end if bottom...
	else if( !atRightExtreme && atLeftExtreme && !atBottomExtreme && !atTopExtreme )
	{
		*ppHalfPos	= MEI2_LeftHalfPos;
		return(MEI2_MOTION_LEFT_HALF_POS_LENGTH);
	}//end if left...
	else if( atRightExtreme && !atLeftExtreme && !atBottomExtreme && !atTopExtreme )
	{
		*ppHalfPos	= MEI2_RightHalfPos;
		return(MEI2_MOTION_RIGHT_HALF_POS_LENGTH);
	}//end if right...
	else if( !atRightExtreme && atLeftExtreme && !atBottomExtreme && atTopExtreme )
	{
		*ppHalfPos	= MEI2_TopLeftHalfPos;
		return(MEI2_MOTION_TOP_LEFT_HALF_POS_LENGTH);
	}//end if top left...
	else if( atRightExtreme && !atLeftExtreme && !atBottomExtreme && atTopExtreme )
	{
		*ppHalfPos	= MEI2_TopRightHalfPos;
		return(MEI2_MOTION_TOP_RIGHT_HALF_POS_LENGTH);
	}//end if top right...
	else if( !atRightExtreme && atLeftExtreme && atBottomExtreme && !atTopExtreme )
	{
		*ppHalfPos	= MEI2_BottomLeftHalfPos;
		return(MEI2_MOTION_BOTTOM_LEFT_HALF_POS_LENGTH);
	}//end if bottom left...
	else if( atRightExtreme && !atLeftExtreme && atBottomExtreme && !atTopExtreme )
	{
		*ppHalfPos	= MEI2_BottomRightHalfPos;
		return(MEI2_MOTION_BOTTOM_RIGHT_HALF_POS_LENGTH);
	}//end if bottom right...

	*ppHalfPos	= MEI2_HalfPos;
	return(MEI2_MOTION_HALF_POS_LENGTH);
}//end GetHalfPelSearchStruct.





