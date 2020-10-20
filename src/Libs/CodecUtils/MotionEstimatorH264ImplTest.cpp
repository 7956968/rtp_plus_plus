/** @file

MODULE				: MotionEstimatorH264ImplTest

TAG						: MEH264IT

FILE NAME			: MotionEstimatorH264ImplTest.cpp

DESCRIPTION		: A full unrestricted motion estimator implementation for 
								Recommendation H.264 (03/2005) for testing new algorithms. 
                Access is via an IMotionEstimator interface. The boundary 
                is extended to accomodate the selected motion range.

COPYRIGHT			: (c)CSIR 2007-2015 all rights resevered

LICENSE				: Software License Agreement (BSD License)

RESTRICTIONS	: Redistribution and use in source and binary forms, with or without 
								modification, are permitted provided that the following conditions 
								are met:

								* Redistributions of source code must retain the above copyright notice, 
								this list of conditions and the following disclaimer.
								* Redistributions in binary form must reproduce the above copyright notice, 
								this list of conditions and the following disclaimer in the documentation 
								and/or other materials provided with the distribution.
								* Neither the name of the CSIR nor the names of its contributors may be used 
								to endorse or promote products derived from this software without specific 
								prior written permission.

								THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
								"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
								LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
								A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
								CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
								EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
								PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
								PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
								LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
								NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
								SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

#include	"MotionEstimatorH264ImplTest.h"

/*
--------------------------------------------------------------------------
  Constants. 
--------------------------------------------------------------------------
*/
/// Calc = ((16[vec dim] * 16[vec dim]) * 2.
#define MEH264IT_FULL_MOTION_NOISE_FLOOR				512
#define MEH264IT_MOTION_NOISE_FLOOR							 26 //24 //sqrt 512

/// Boundary padding past the motion vector extremes. Required for calculating
/// sub-pixel interpolations. 
#define MEH264IT_PADDING													3	

/// Choose between sqr err distortion or abs diff metric.
#undef MEH264IT_ABS_DIFF

/// Search range coords for centre motion vectors.
#define MEH264IT_MOTION_SUB_POS_LENGTH 	8
MEH264IT_COORD MEH264IT_SubPos[MEH264IT_MOTION_SUB_POS_LENGTH]	= 
{
	{-1,-1},{0,-1},{1,-1},{-1,0},{1,0},{-1,1},{0,1},{1,1}
};

/*
--------------------------------------------------------------------------
  Macros. 
--------------------------------------------------------------------------
*/
#define MEH264IT_CLIP255(x)	( (((x) <= 255)&&((x) >= 0))? (x) : ( ((x) < 0)? 0:255 ) )

/*
--------------------------------------------------------------------------
  Construction. 
--------------------------------------------------------------------------
*/

MotionEstimatorH264ImplTest::MotionEstimatorH264ImplTest(	const void*             pSrc, 
																													const void*             pRef, 
																													int					            imgWidth, 
																													int					            imgHeight,
																													int					            motionRange,
                                                          IMotionVectorPredictor* pMVPred)
{
	ResetMembers();

	/// Parameters must remain const for the life time of this instantiation.
	_imgWidth				= imgWidth;					///< Width of the src and ref images. 
	_imgHeight			= imgHeight;				///< Height of the src and ref images.
	_macroBlkWidth	= 16;								///< Width of the motion block = 16 for H.263.
	_macroBlkHeight	= 16;								///< Height of the motion block = 16 for H.263.
	_motionRange		= motionRange;			///< (4x,4y) range of the motion vectors. _motionRange in 1/4 pel units.
	_pInput					= pSrc;
	_pRef						= pRef;
  _pMVPred        = pMVPred;

}//end constructor.

MotionEstimatorH264ImplTest::MotionEstimatorH264ImplTest(	const void*             pSrc, 
																													const void*             pRef, 
																													int					            imgWidth, 
																													int					            imgHeight,
																													int					            motionRange,
                                                          IMotionVectorPredictor* pMVPred,
																													void*				            pDistortionIncluded)
{
	ResetMembers();

	/// Parameters must remain const for the life time of this instantiation.
	_imgWidth							= imgWidth;					///< Width of the src and ref images. 
	_imgHeight						= imgHeight;				///< Height of the src and ref images.
	_macroBlkWidth				= 16;								///< Width of the motion block = 16 for H.263.
	_macroBlkHeight				= 16;								///< Height of the motion block = 16 for H.263.
	_motionRange					= motionRange;			///< (4x,4y) range of the motion vectors. _motionRange in 1/4 pel units.
	_pInput								= pSrc;
	_pRef									= pRef;
  _pMVPred              = pMVPred;
	_pDistortionIncluded	= (bool *)pDistortionIncluded;

}//end constructor.

void MotionEstimatorH264ImplTest::ResetMembers(void)
{
	_ready	= 0;	///< Ready to estimate.
	_mode		= 1;	///< Speed mode or whatever. Default to slower speed.

	/// Parameters must remain const for the life time of this instantiation.
	_imgWidth				= 0;					///< Width of the src and ref images. 
	_imgHeight			= 0;					///< Height of the src and ref images.
	_macroBlkWidth	= 16;					///< Width of the motion block = 16 for H.264.
	_macroBlkHeight	= 16;					///< Height of the motion block = 16 for H.264.
	_motionRange		= 64;					///< (4x,4y) range of the motion vectors.
	_pInput					= NULL;
	_pRef						= NULL;

	/// Input mem overlay members.
	_pInOver					= NULL;			///< Input overlay with mb motion block dim.
	/// Ref mem overlay members.
	_pRefOver					= NULL;			///< Ref overlay with whole block dim.
	_pExtRef					= NULL;			///< Extended ref mem created by ExtendBoundary() call.
	_extWidth					= 0;
	_extHeight				= 0;
	_extBoundary			= 0;
	_pExtRefOver			= NULL;			///< Extended ref overlay with motion block dim.
  /// A 1/4 pel refinement window.
	_pWin							= NULL;
	_Win							= NULL;

	/// Temp working block and its overlay.
	_pMBlk						= NULL;			///< Motion block temp mem.
	_pMBlkOver				= NULL;			///< Motion block overlay of temp mem.

	/// Hold the resulting motion vectors in a byte array.
	_pMotionVectorStruct = NULL;
  /// Attached motion vector predictor on construction.
  _pMVPred             = NULL;

	/// A flag per macroblock to include it in the distortion accumulation.
	_pDistortionIncluded = NULL;

  /// Measurements.
  _METableLen = 0;
  _METablePos = 0;

}//end ResetMembers.

MotionEstimatorH264ImplTest::~MotionEstimatorH264ImplTest(void)
{
	Destroy();
}//end destructor.

/*
--------------------------------------------------------------------------
  Public IMotionEstimator Interface. 
--------------------------------------------------------------------------
*/

int MotionEstimatorH264ImplTest::Create(void)
{
	/// Clean out old mem.
	Destroy();

	/// --------------- Configure input overlays --------------------------------
	/// Put an overlay on the input image with the block size set to the mb vector 
	/// dim. This is used to access input vectors.
	_pInOver = new OverlayMem2Dv2((void *)_pInput,_imgWidth,_imgHeight,_macroBlkWidth,_macroBlkHeight);
	if(_pInOver == NULL)
	{
		Destroy();
		return(0);
	}//end _pInOver...

	/// --------------- Configure ref overlays --------------------------------
	/// Overlay the whole reference. The reference will have an extended 
	/// boundary for motion estimation and must therefore create its own mem.
	_pRefOver = new OverlayMem2Dv2((void *)_pRef, _imgWidth, _imgHeight, _imgWidth, _imgHeight);
	if(_pRefOver == NULL)
  {
		Destroy();
	  return(0);
  }//end if !_pRefOver...

	/// Create the new extended boundary ref into _pExtRef. The boundary is extended by
	/// the max dimension of the macroblock plus some padding to cater for quarter pel
  /// searches on the edges of the boundary. The mem is allocated in the method call.
	_extBoundary = _macroBlkWidth + MEH264IT_PADDING;
	if(_macroBlkHeight > _macroBlkWidth)
		_extBoundary = _macroBlkHeight + MEH264IT_PADDING;
	if(!OverlayExtMem2Dv2::ExtendBoundary((void *)_pRef, 
																				_imgWidth,						
																				_imgHeight, 
																				_extBoundary,	///< Extend left and right by...
																				_extBoundary,	///< Extend top and bottom by...
																				(void **)(&_pExtRef)) )	///< Created in the method and returned.
  {
		Destroy();
	  return(0);
  }//end if !ExtendBoundary...
	_extWidth	 = _imgWidth + (2 * _extBoundary);
	_extHeight = _imgHeight + (2 * _extBoundary);

	/// Place an overlay on the extended boundary ref with block size set to the mb motion 
  /// vec dim.
	_pExtRefOver = new OverlayExtMem2Dv2(	_pExtRef,				///< Src description created in the ExtendBoundary() call. 
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
	/// Alloc some temp mem and overlay it to use for half/quarter pel motion 
  /// estimation and compensation. The block size is the same as the mem size.
	_pMBlk = new short[_macroBlkWidth * _macroBlkHeight];
	_pMBlkOver = new OverlayMem2Dv2(_pMBlk, _macroBlkWidth, _macroBlkHeight, 
																					_macroBlkWidth, _macroBlkHeight);
	if( (_pMBlk == NULL)||(_pMBlkOver == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pMBlk...

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

	/// --------------- Refinement Window ---------------------------------------
	/// Prepare a 1/4 pel search window block for motion estimation refinement.
	int winWidth	= ((6 + _macroBlkWidth) * 4);
	int winHeight = ((6 + _macroBlkHeight) * 4);
	_pWin					= new short[winWidth * winHeight];
	_Win					= new OverlayMem2Dv2((void *)_pWin, winWidth, winHeight, winWidth, winHeight);
	if( (_pWin == NULL)||(_Win == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pWin...

	/// --------------- Measurements ---------------------------------------
#ifdef MEH264IT_DUMP
  _METableLen = 16;
	_METable.Create(16, _METableLen);
  _METablePos = 0;

  /// Table is all integers and no headings.
  int i;
  for(i = 0; i < 16; i++)
  {
    _METable.SetHeading(i, "");
    _METable.SetDataType(i, MeasurementTable::INT);
  }//end for i...

  /// Zero the winner count and write into the coords of each 16x16 mb position.
  for(i = 0; i < 16; i++)
    for(int j = 0; j < 16; j++)
    {
      _winCount[i][j] = 0;
      _METable.WriteItem(j, i, _winCount[i][j]);
    }//end for i...

#endif

	_ready = 1;
	return(1);
}//end Create.

/** Motion estimate the source within the reference.
Do the estimation with the block sizes and image sizes defined in
the implementation. The returned type holds the vectors. This is
a full search algorithm with extended boundaries with a choice of
absolute difference or squared difference criterioa. 
@param avgDistortion  : Return the motion compensated distortion.
@return				        : The list of motion vectors.
*/
void* MotionEstimatorH264ImplTest::Estimate(long* avgDistortion)
{
  int		i,j,m,n,k,l;
	int		included = 0;
	long	totalDifference = 0;

	/// Set the motion vector struct storage structure.
	int		maxLength	= _pMotionVectorStruct->GetLength();
	int		vecPos		= 0;

	/// Write the ref and fill its extended boundary. The centre part of
	/// _pExtRefOver is copied from _pRefOver before filling the boundary.
	_pExtRefOver->SetOrigin(0, 0);
	_pExtRefOver->SetOverlayDim(_imgWidth, _imgHeight);
	_pExtRefOver->Write(*_pRefOver);	///< _pRefOver dimensions are always set to the whole image.
	_pExtRefOver->FillBoundaryProxy();
	_pExtRefOver->SetOverlayDim(_macroBlkWidth, _macroBlkHeight);

	/// Gather the motion vector absolute differnce/square error data and choose the vector.
	/// m,n step level 0 vec dim = _macroBlkHeight, _macroBlkWidth.
  for(m = 0; m < _imgHeight; m += _macroBlkHeight)
		for(n = 0; n < _imgWidth; n += _macroBlkWidth)
  {
		int mx	= 0;	///< Full pel grid.
		int my	= 0;
		int hmx	= 0;	///< 1/2 pel on 1/4 pel grid.
		int hmy	= 0;
		int qmx	= 0;	///< 1/4 pel grid.
		int qmy	= 0;
		int rmx = 0;	///< Refinement motion vector centre.
		int rmy = 0;

		/// Depending on which img boundary we are on will limit the full search range.
		int xlRng, xrRng, yuRng, ydRng;

		/// The predicted vector difference between the input and ref blocks is the most
		/// likely candidate and is therefore the starting best distortion point.
    int predX, predY, predX0Rnd, predY0Rnd;
    _pMVPred->Get16x16Prediction(NULL, vecPos, &predX, &predY);
    int predXQuart  = predX % 4;
    int predYQuart  = predY % 4;
    int predX0      = predX / 4;
    int predY0      = predY / 4;
    if(predX < 0)             ///< Nearest level 0 pred motion vector.
      predX0Rnd = (predX - 2)/4;
    else
      predX0Rnd = (predX + 2)/4;
    if(predY < 0)
      predY0Rnd = (predY - 2)/4;
    else
      predY0Rnd = (predY + 2)/4;

		/// Set the input and ref blocks to work with.
		_pInOver->SetOrigin(n,m);
		_pExtRefOver->SetOrigin(n,m);

		/// The (0,0) vector is the one to beat with Absolute/Square diff comparison method.
//#ifdef MEH264IT_ABS_DIFF
//		int zeroVecDiff = _pInOver->Tad16x16(*_pExtRefOver);
//#else
//		int zeroVecDiff = _pInOver->Tsd16x16(*_pExtRefOver);
//#endif
    /// Total square diff between input and ref mb pels at mv position [0,0].
    int zeroVecDiff = 0;
    for(k = 0; k < _macroBlkHeight; k++)
      for(l = 0; l < _macroBlkWidth; l++)
      {
        int d = _pInOver->Read(l, k) - _pExtRefOver->Read(l, k);
        zeroVecDiff += (d*d);
      }//end for k & l...

		int minDiff	= zeroVecDiff;	///< Best so far.

    ///--------------------------- Full pel full grid search ---------------------------------------------------
    /// From this mb position determine the full pel search range permitted for the motion vector.
		GetMotionRange(n, m, 0, 0, &xlRng, &xrRng, &yuRng, &ydRng, _motionRange/4); ///< _motionRange is in 1/4 pel units and must be converted.

    for(i = yuRng; i <= ydRng; i++)
    {
			for(j = xlRng; j <= xrRng; j++)
			{
				int blkDiff;
				/// Early exit because zero motion vec already checked.
				if( !(i||j) )	goto MEH264IT_FULL_BREAK;

				/// Set the block to the [j,i] motion vector around the [n,m] reference location.
				_pExtRefOver->SetOrigin(n+j, m+i);

        /// Total square diff between input and ref mb pels at mv position [j,i].
        blkDiff = 0;
        for(k = 0; k < _macroBlkHeight; k++)
          for(l = 0; l < _macroBlkWidth; l++)
          {
            int d = _pInOver->Read(l, k) - _pExtRefOver->Read(l, k);
            blkDiff += (d*d);
            if(blkDiff > minDiff) goto MEH264IT_FULL_BREAK;
          }//end for k & l...

//#ifdef MEH264IT_ABS_DIFF
//				blkDiff = _pInOver->Tad16x16LessThan(*_pExtRefOver, minDiff);
//#else
////				blkDiff = _pInOver->Tsd16x16LessThan(*_pExtRefOver, minDiff);
//				blkDiff = _pInOver->Tsd16x16PartialLessThan(*_pExtRefOver, minDiff);
//#endif
				if(blkDiff <= minDiff)
				{
					/// Weight the equal diff case with the smallest global mv magnitude from the pred mv. 
					if(blkDiff == minDiff)
					{
            int currX = mx - predX0Rnd;
            int currY = my - predY0Rnd;
            int newX  = j - predX0Rnd;
            int newY  = i + predY0Rnd;
						int vecDistDiff = ( (currY*currY)+(currX*currX) )-( (newY*newY)+(newX*newX) );
						if(vecDistDiff < 0)
							goto MEH264IT_FULL_BREAK;
					}//end if blkDiff...

					minDiff = blkDiff;
					mx = j;
					my = i;
				}//end if blkDiff...

				MEH264IT_FULL_BREAK: ; ///< null.
			}//end for j...
    }//end for i...

    ///----------------------- Measurement collection --------------------------------------------
    /// The values of mx, my and minDiff above give the winning full search vector to compare with.

    /// We want to test the case where only 1 sample position in the mb is used for a full search to 
    /// find the best motion vector. This is a search for the 1st sample position in a sampling path. 
    /// The number of times each mb sample position gets the correct mv is accumulated in _winCount[].
    int tmx,tmy,tminDiff;
    for(k = 0; k < _macroBlkHeight; k++) ///< Full mv search for each sampling position in the mb.
      for(l = 0; l < _macroBlkWidth; l++)
      {
				/// Set the mb to the [0,0] motion vector around the [n,m] reference location.
				_pExtRefOver->SetOrigin(n, m);
        int d = _pInOver->Read(l, k) - _pExtRefOver->Read(l, k); ///< A single sample point inside mb.
        tminDiff = (d*d);
        tmx = 0;
        tmy = 0;
        /// Search all other non-zero motion vectors.
        for(i = yuRng; i <= ydRng; i++)
			    for(j = xlRng; j <= xrRng; j++)
          {
				    int tdiff;
				    /// Early exit because zero motion vec already checked.
				    if( !(i||j) )	goto MEH264IT_TEST_BREAK;
				    /// Set the mb to the [j,i] motion vector around the [n,m] reference location.
				    _pExtRefOver->SetOrigin(n+j, m+i);
            d = _pInOver->Read(l, k) - _pExtRefOver->Read(l, k); ///< A single sample point inside mb.
            tdiff= (d*d);
            /// Resolve best vector.
				    if(tdiff <= tminDiff)
				    {
					    /// Weight the equal diff case with the smallest global mv magnitude from the pred mv. 
					    if(tdiff == tminDiff)
					    {
                int currX = tmx - predX0Rnd;
                int currY = tmy - predY0Rnd;
                int newX  = j - predX0Rnd;
                int newY  = i + predY0Rnd;
						    int vecDistDiff = ( (currY*currY)+(currX*currX) )-( (newY*newY)+(newX*newX) );
						    if(vecDistDiff < 0)
							    goto MEH264IT_TEST_BREAK;
					    }//end if tdiff...

					    tminDiff = tdiff;
					    tmx = j;
					    tmy = i;
				    }//end if tdiff...

				    MEH264IT_TEST_BREAK: ; ///< null.
          }//end for i & j...

        /// Accumulate if same as full search mv for this [l,k] position and non-zero mv.
        if( (tmx == mx)&&(tmy == my)&&(tmx || tmy) )
        {
          _winCount[k][l]++;
          _METablePos = 16; ///< Validate the entire table after the 1st entry.
        }///end if tmx...

      }//end for k & l...

		///----------------------- Quarter pel refined search ----------------------------------------
    /// Search around the min diff full pel motion vector on a 1/4 pel grid firstly on the
		/// 1/2 pel positions and then refine the winner on the 1/4 pel positions. 

		int mvx = mx << 2;	///< Convert to 1/4 pel units.
		int mvy = my << 2;

		/// Set the location to the min diff motion vector (mx,my).
		_pExtRefOver->SetOrigin(n+mx, m+my);

		/// Fill the 1/4 pel window with valid values only in the 1/2 pel positions.
		LoadHalfQuartPelWindow(_Win, _pExtRefOver); 

    for(int x = 0; x < MEH264IT_MOTION_SUB_POS_LENGTH; x++)
    {
			int qOffX = 2 * MEH264IT_SubPos[x].x;
			int qOffY = 2 * MEH264IT_SubPos[x].y;

			/// Read the half grid pels into temp.
			QuarterRead(_pMBlkOver, _Win, qOffX, qOffY);

#ifdef MEH264IT_ABS_DIFF
			int blkDiff = _pInOver->Tad16x16LessThan(*_pMBlkOver, minDiff);
#else
//			int blkDiff = _pInOver->Tsd16x16LessThan(*_pMBlkOver, minDiff);
		int blkDiff = _pInOver->Tsd16x16PartialLessThan(*_pMBlkOver, minDiff);
#endif
			if(blkDiff < minDiff)
			{
				minDiff = blkDiff;
				hmx = qOffX;
				hmy = qOffY;
			}//end if blkDiff...
    }//end for x...

		qmx = hmx;
		qmy = hmy;

		/// Fill the 1/4 pel positions around the winning 1/2 pel position (hmx,hmy).
		LoadQuartPelWindow(_Win, hmx, hmy); 

    for(int x = 0; x < MEH264IT_MOTION_SUB_POS_LENGTH; x++)
    {
			int qOffX = hmx + MEH264IT_SubPos[x].x;
			int qOffY = hmy + MEH264IT_SubPos[x].y;

			/// Read the quarter grid pels into temp.
			QuarterRead(_pMBlkOver, _Win, qOffX, qOffY);

#ifdef MEH264IT_ABS_DIFF
			int blkDiff = _pInOver->Tad16x16LessThan(*_pMBlkOver, minDiff);
#else
//			int blkDiff = _pInOver->Tsd16x16LessThan(*_pMBlkOver, minDiff);
			int blkDiff = _pInOver->Tsd16x16PartialLessThan(*_pMBlkOver, minDiff);
#endif
			if(blkDiff < minDiff)
			{
				minDiff = blkDiff;
				qmx = qOffX;
				qmy = qOffY;
			}//end if blkDiff...
    }//end for x...

		/// Add the refinement in 1/4 pel units.
		mvx += qmx;
		mvy += qmy;

		/// Bounds check used for debugging only as the minDiff will not be correct.
		//if(mvx < -_motionRange) 
		//	mvx = -_motionRange;
		//else if(mvx >= _motionRange)
		//	mvx = (_motionRange-1);
		//if(mvy < -_motionRange) 
		//	mvy = -_motionRange;
		//else if(mvy >= _motionRange)
		//	mvy = (_motionRange-1);

		///----------------------- Quarter pel pred vector ----------------------------
    /// Compare this winning vector with the predicted mv but truncate it if it
    /// falls outside of one mb width or height outside the img boundaries.
    if( (predX0+n) > _imgWidth)
      predX0 = _macroBlkWidth;
    if( (predX0+n) < -_macroBlkWidth)
      predX0 = -_macroBlkWidth;
    if( (predY0+m) > _imgHeight)
      predY0 = _macroBlkHeight;
    if( (predY0+m) < -_macroBlkHeight)
      predY0 = -_macroBlkHeight;
    predX = (predX0 * 4) + predXQuart;
    predY = (predY0 * 4) + predYQuart;

		_pExtRefOver->SetOrigin(predX0+n,predY0+m);

    /// Get distortion at pred mv.
    int predVecDiff = 0;
    /// Quarter read first if necessary.
    if(predXQuart || predYQuart)
    {
			/// Read the quarter grid pels into temp.
			_pExtRefOver->QuarterRead(*_pMBlkOver, predXQuart, predYQuart);
		/// Absolute/square diff comparison method.
#ifdef MEH264IT_ABS_DIFF
		  predVecDiff = _pInOver->Tad16x16(*_pMBlkOver);
#else
		  predVecDiff = _pInOver->Tsd16x16(*_pMBlkOver);
#endif
    }//end if predXQuart...
    else
    {
#ifdef MEH264IT_ABS_DIFF
		  predVecDiff = _pInOver->Tad16x16(*_pExtRefOver);
#else
		  predVecDiff = _pInOver->Tsd16x16(*_pExtRefOver);
#endif
    }//end else...

		/// Selection of the final motion vector is weighted with non-linear factors. The
    /// zero vector will be modified to the best of either the predicted mv or the zero mv.
    int zeromvx, zeromvy;
    if(predVecDiff <= zeroVecDiff)
    {
      zeroVecDiff = predVecDiff;
      zeromvx     = predX;
      zeromvy     = predY;
    }//end if predVecDiff...
    else
    {
      zeromvx     = 0;
      zeromvy     = 0;
    }//end else...

		int diffWithZeroDiff	= zeroVecDiff - minDiff;
		int weight						= 0;
		int magSqr						= (mvx * mvx) + (mvy * mvy);

#ifdef MEH264IT_ABS_DIFF
		/// Contribute if motion vector is small.
		if((diffWithZeroDiff * 2) < magSqr)
			weight++;
		/// Contribute if same order as the noise.
		if(zeroVecDiff < MEH264IT_MOTION_NOISE_FLOOR)
			weight++;
		/// Contribute if the zero vector and min diff vector are similar.
		if((diffWithZeroDiff * 7) < minDiff)
			weight++;
#else
		/// Contribute if motion vector is small.
		if(diffWithZeroDiff < magSqr)
			weight++;
		/// Contribute if same order as the noise.
		if(zeroVecDiff < MEH264IT_FULL_MOTION_NOISE_FLOOR)
			weight++;
		/// Contribute if the zero vector and min energy vector are similar.
		if((diffWithZeroDiff * 10) < minDiff)
			weight++;
#endif

		/// Check for inclusion in the distortion calculation.
		bool doIt = true;
		if(_pDistortionIncluded != NULL)
			doIt = _pDistortionIncluded[vecPos];
		if(doIt)
			included++;

		/// Decide whether or not to accept the final motion vector or revert to the zero/pred motion vector.
		if((minDiff < zeroVecDiff)&&(weight < 2))
    {
			if(doIt)
				totalDifference += minDiff;
    }//end if min_energy...
    else
		{
			mvx = zeromvx;
			mvy = zeromvy;
			if(doIt)
				totalDifference += zeroVecDiff;
		}//end else...

		/// Load the selected vector coord.
		if(vecPos < maxLength)
		{
			_pMotionVectorStruct->SetSimpleElement(vecPos, 0, mvx);
			_pMotionVectorStruct->SetSimpleElement(vecPos, 1, mvy);
      /// Set macroblock vector for future predictions.
      _pMVPred->Set16x16MotionVector(vecPos, mvx, mvy);
			vecPos++;
		}//end if vecPos...

  }//end for m & n...

	/// In this context avg distortion is actually avg difference.
//	*avgDistortion = totalDifference/maxLength;
	if(included)	///< Prevent divide by zero error.
		*avgDistortion = totalDifference/included;
	else
		*avgDistortion = 0;
	return((void *)_pMotionVectorStruct);

}//end Estimate.

/*
--------------------------------------------------------------------------
  Public methods. 
--------------------------------------------------------------------------
*/

void MotionEstimatorH264ImplTest::Dump(char* filename)
{
  if(_METablePos > 0)
    _METable.Save(filename, ",", 1);
  _METablePos = 0;
}//end Dump.

/*
--------------------------------------------------------------------------
  Private methods. 
--------------------------------------------------------------------------
*/

void MotionEstimatorH264ImplTest::Destroy(void)
{
  /// Save measuements before closing.
#ifdef MEH264IT_DUMP
  for(int i = 0; i < _METablePos; i++)
    for(int j = 0; j < 16; j++)
      _METable.WriteItem(j, i, _winCount[i][j]);
  if(_METablePos > 0)
    Dump(MEH264IT_PATH_DATA);
#endif
  _METable.Destroy();

	_ready = 0;

	if(_Win != NULL)
		delete _Win;
	_Win = NULL;
	if(_pWin != NULL)
		delete[] _pWin;
	_pWin = NULL;

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

	if(_pMotionVectorStruct != NULL)
		delete _pMotionVectorStruct;
	_pMotionVectorStruct = NULL;

}//end Destroy.

/** Get the allowed motion range for this block.
The search area for unrestricted H.264 is within the bounds of the extended image
dimensions. The range is limited at the corners and edges of the extended
images. The returned values are offset limits from (x,y) image coordinate.
@param x				: X coord of block.
@param y				: Y coord of block.
@param xlr			: Returned allowed left range offset from x.
@param xrr			: Returned allowed right range offset from x.
@param yur			: Returned allowed up range offset from y.
@param ydr			: Returned allowed down range offset from y.
@param range		: Desired range of motion.
@return					: none.
*/
void MotionEstimatorH264ImplTest::GetMotionRange( int  x,			int  y, 
																								  int* xlr,		int* xrr, 
																									int* yur,		int* ydr,
																									int	 range)
{
	int boundary	= _extBoundary - MEH264IT_PADDING;
	int	width			= _imgWidth;
	int	height		= _imgHeight;

	if( (x - range) >= -boundary )	///< Ok and within left extended boundary.
		*xlr = -range;
	else ///< Bring it into the extended boundary edge.
		*xlr = -(x + boundary);
	if( (x + range) < width )	///< Rest of block extends into the bounday region.
		*xrr = range;
	else
		*xrr = width - x;

	if( (y - range) >= -boundary )	///< Ok and within upper extended boundary.
		*yur = -range;
	else ///< Bring it into the extended boundary edge.
		*yur = -(y + boundary);
	if( (y + range) < height )	///< Rest of block extends into the bounday region.
		*ydr = range;
	else
		*ydr = height - y;

}//end GetMotionRange.

/** Get the allowed motion range for this block.
The search area for unrestricted H.264 is within the bounds of the extended image
dimensions. The range is limited at the corners and edges of the extended
images. The range is further checked to ensure that the motion vector is within 
its defined max range. The returned values are offset limits from the (xpos+xoff,ypos+yoff) 
image coordinates.
@param xpos			: X coord of block in the image.
@param ypos			: Y coord of block in the image.
@param xoff			: Current offset (vector) from xpos.
@param yoff			: Current offset (vector) from ypos.
@param xlr			: Returned allowed left range offset from xpos+xoff.
@param xrr			: Returned allowed right range offset from xpos+xoff.
@param yur			: Returned allowed up range offset from ypos+yoff.
@param ydr			: Returned allowed down range offset from ypos+yoff.
@param range		: Desired range of motion.
@return					: none.
*/
void MotionEstimatorH264ImplTest::GetMotionRange(	int  xpos,	int  ypos,
																									int	 xoff,	int  yoff,
																									int* xlr,		int* xrr, 
																									int* yur,		int* ydr,
																									int	 range)
{
	int x = xpos + xoff;
	int y = ypos + yoff;

	int xLRange, xRRange, yURange, yDRange;

	int boundary	= _extBoundary - MEH264IT_PADDING;
	int	width			= _imgWidth;
	int	height		= _imgHeight;
	int	vecRange	= _motionRange/4;	///< Convert 1/4 pel range to full pel units.

	/// Limit the range of the motion vector.
	if( (xoff - range) > -vecRange )
		xLRange = range;
	else
		xLRange = (vecRange-1) + xoff;
	if( (xoff + range) < vecRange )
		xRRange = range;
	else
		xRRange = (vecRange-1) - xoff;
	if( (yoff - range) > -vecRange )
		yURange = range;
	else
		yURange = (vecRange-1) + yoff;
	if( (yoff + range) < vecRange )
		yDRange = range;
	else
		yDRange = (vecRange-1) - yoff;

	if( (x - xLRange) >= -boundary )	///< Ok and within left extended boundary.
		*xlr = -xLRange;
	else ///< Bring it into the extended boundary edge.
		*xlr = -(x + boundary);
	if( (x + xRRange) < width )	///< Rest of block extends into the bounday region.
		*xrr = xRRange;
	else
		*xrr = width - x;

	if( (y - yURange) >= -boundary )	///< Ok and within upper extended boundary.
		*yur = -yURange;
	else ///< Bring it into the extended boundary edge.
		*yur = -(y + boundary);
	if( (y + yDRange) < height )	///< Rest of block extends into the bounday region.
		*ydr = yDRange;
	else
		*ydr = height - y;

}//end GetMotionRange.

/** Load a 1/4 pel window with 1/2 pel values.
The 1/4 pel window must be the macroblock size with a boundary of 3 extra pels on all sides. Only the inner
macroblock size plus 1 extra pel boundary are filled with valid values. This window is used in a cascading 
approach to motion estimation where the best 1/2 pel search around a winning full pel is done first followed 
by the best 1/4 pel around the winning 1/2 pel position. This window is used for the input for the first stage
and therefore only the 1/2 pel positions are valid. This method must be followed by the LoadQuartPelWindow()
method to complete the 1/4 pel values around a winning 1/2 pel position. The reference origin position is 
aligned onto the full pel (3,3) position of the 1/4 pel window.
@param qPelWin	: Window of size (4 * (_macroBlkHeight+6)) x (4 * (_macroBlkWidth+6))
@param extRef		: Reference to derive the 1/4 pel window from.
@return					: none.
*/
void MotionEstimatorH264ImplTest::LoadHalfQuartPelWindow(OverlayMem2Dv2* qPelWin, OverlayMem2Dv2* extRef)
{
	int fullRow, fullCol, quartRow, quartCol, refRow, refCol;

	int			width		= qPelWin->GetWidth()/4;	///< Convert to full pel units.
	int			height	= qPelWin->GetHeight()/4;
	short** window	= qPelWin->Get2DSrcPtr();

	short** ref			= extRef->Get2DSrcPtr();
	int			refXPos = extRef->GetOriginX();
	int			refYPos = extRef->GetOriginY();

	/// Set all the "h" half pel values in the window only at the positions that will be required for the other calcs. No
	/// scaling or clipping is performed until "j" half pel values are completed.
	for(fullRow = 2, quartRow = 10, refRow = refYPos - 1; fullRow < (_macroBlkHeight + 3); fullRow++, quartRow += 4, refRow++)
	{
		for(fullCol = 0, quartCol = 0, refCol = refXPos - 3; fullCol < width; fullCol++, quartCol += 4, refCol++)
		{
			int h =     (int)ref[refRow-2][refCol] -  5*(int)ref[refRow-1][refCol] + 
							 20*(int)ref[refRow][refCol]   + 20*(int)ref[refRow+1][refCol] - 
							  5*(int)ref[refRow+2][refCol] +    (int)ref[refRow+3][refCol];

			window[quartRow][quartCol] = (short)(h);
		}//end for fullCol...
	}//end fullRow...
	
	/// Set all the "b" half pel values in the window only at the positions that will be required for the other calcs. No
	/// scaling or clipping is performed until "j" half pel values are completed.
	for(fullRow = 0, quartRow = 0, refRow	= refYPos - 3; fullRow < height; fullRow++, quartRow += 4, refRow++)
	{
		for(fullCol = 2, quartCol = 10, refCol = refXPos - 1; fullCol < (_macroBlkWidth + 3); fullCol++, quartCol += 4, refCol++)
		{
			int b =     (int)ref[refRow][refCol-2] -  5*(int)ref[refRow][refCol-1] + 
							 20*(int)ref[refRow][refCol]   + 20*(int)ref[refRow][refCol+1] - 
							  5*(int)ref[refRow][refCol+2] +    (int)ref[refRow][refCol+3];

			window[quartRow][quartCol] = (short)(b);
		}//end for fullCol...
	}//end fullRow...

	/// For the "j" half pel values, use the previously calculated "h" and "b" values only in the positions
	/// surrounding the centre of the reference image origin. Now scaling is included for j.
	for(fullRow = 2, quartRow = 10; fullRow < (_macroBlkHeight + 3); fullRow++, quartRow += 4)
	{
		for(fullCol = 2, quartCol = 10; fullCol < (_macroBlkWidth + 3); fullCol++, quartCol += 4)
		{
			int j = (   (int)window[quartRow][quartCol-10] -  5*(int)window[quartRow][quartCol-6] + 
							 20*(int)window[quartRow][quartCol-2]  + 20*(int)window[quartRow][quartCol+2] - 
							  5*(int)window[quartRow][quartCol+6]  +    (int)window[quartRow][quartCol+10] + 512) >> 10;

			window[quartRow][quartCol] = (short)(MEH264IT_CLIP255(j));
		}//end for fullCol...
	}//end fullRow...

	/// Scale and clip the useful "h" and "b" half pel values in place.
	for(fullRow = 2, refRow	= refYPos - 1, quartRow = 8; fullRow < (_macroBlkHeight + 3); fullRow++, refRow++, quartRow += 4)
	{
		for(fullCol = 2, refCol = refXPos - 1, quartCol = 8; fullCol < (_macroBlkWidth + 3); fullCol++, refCol++, quartCol += 4)
		{
			/// "h"
			window[quartRow+2][quartCol] = MEH264IT_CLIP255((window[quartRow+2][quartCol] + 16) >> 5);
			/// "b"
			window[quartRow][quartCol+2] = MEH264IT_CLIP255((window[quartRow][quartCol+2] + 16) >> 5);
			/// Copy full pel "G"
			window[quartRow][quartCol] = ref[refRow][refCol];
		}//end for fullCol...

    /// One further "h" and "G" col at the end of the row.
		/// "h"
		window[quartRow+2][quartCol] = MEH264IT_CLIP255((window[quartRow+2][quartCol] + 16) >> 5);
		/// Copy full pel "G"
		window[quartRow][quartCol] = ref[refRow][refCol];

	}//end fullRow...

   /// One further "b" and "G" row at the end.
	for(fullCol = 2, refCol = refXPos - 1, quartCol = 8; fullCol < (_macroBlkWidth + 3); fullCol++, refCol++, quartCol += 4)
	{
		/// "b"
		window[quartRow][quartCol+2] = MEH264IT_CLIP255((window[quartRow][quartCol+2] + 16) >> 5);
		/// Copy full pel "G"
		window[quartRow][quartCol] = ref[refRow][refCol];
	}//end for fullCol...

  /// ...and one final "G" full pel at the bottom right edge.
	window[quartRow][quartCol] = ref[refRow][refCol];

}//end LoadHalfQuartPelWindow.

/** Load a 1/4 pel window with 1/4 pel values not in 1/2 pel positions.
The 1/4 pel window must be the macroblock size with a boundary of 3 extra pels on all sides. Only the inner
macroblock size plus 1 extra pel boundary are filled with valid values. This window is used in a cascading 
approach to motion estimation where the best 1/2 pel search around a winning full pel is done first followed 
by the best 1/4 pel around the winning 1/2 pel position. This window is used for the input for the second stage
and therefore the 1/2 pel positions are valid from a previous (first stage) call to the LoadHalfQuartPelWindow() 
method. This method will complete the 1/4 pel values around a winning 1/2 pel position. The reference origin 
position is aligned onto the full pel (3,3) position of the 1/4 pel window. The 1/4 pel values are processed
from the values already in the window.
@param qPelWin		: Window of size (4 * (_macroBlkHeight+6)) x (4 * (_macroBlkWidth+6))
@param hPelColOff	: The winning 1/2 pel X offset from the (3,3) window position in 1/4 pel units.
@param hPelRowOff	: The winning 1/2 pel Y offset from the (3,3) window position in 1/4 pel units.
@return						: none.
*/
void MotionEstimatorH264ImplTest::LoadQuartPelWindow(OverlayMem2Dv2* qPelWin, int hPelColOff, int hPelRowOff)
{
	int fullRow, fullCol, quartRow, quartCol;

	/// The 1/4 pels are to calculated for the 8 positions surrounding the 1/2 location at (hPelRowOff,hPelColOff). Note
	/// that hPelRowOff and hPelColOff are still in 1/4 pel units i.e. = range [-2,0,2][-2,0,2].

	short** window	= qPelWin->Get2DSrcPtr();

	for(fullRow = 0, quartRow = (12 + hPelRowOff); fullRow < _macroBlkHeight; fullRow++, quartRow += 4)
	{
		for(fullCol = 0, quartCol = (12 + hPelColOff); fullCol < _macroBlkWidth; fullCol++, quartCol += 4)
		{
			window[quartRow-1][quartCol-1]	= (window[quartRow-2][quartCol-2] + window[quartRow][quartCol] + 1) >> 1;
			window[quartRow-1][quartCol]		= (window[quartRow-2][quartCol]		+ window[quartRow][quartCol] + 1) >> 1;
			window[quartRow-1][quartCol+1]	= (window[quartRow-2][quartCol]		+ window[quartRow][quartCol+2] + 1) >> 1;
			window[quartRow][quartCol-1]		= (window[quartRow][quartCol-2]		+ window[quartRow][quartCol] + 1) >> 1;
			window[quartRow][quartCol+1]		= (window[quartRow][quartCol+2]		+ window[quartRow][quartCol] + 1) >> 1;
			window[quartRow+1][quartCol-1]	= (window[quartRow][quartCol-2]		+ window[quartRow+2][quartCol] + 1) >> 1;
			window[quartRow+1][quartCol]		= (window[quartRow+2][quartCol]		+ window[quartRow][quartCol] + 1) >> 1;
			window[quartRow+1][quartCol+1]	= (window[quartRow+2][quartCol]		+ window[quartRow][quartCol+2] + 1) >> 1;
		}//end for fullCol...
	}//end fullRow...

}//end LoadQuartPelWindow.

/** Read 1/4 pels from window.
The 1/4 pel window must be the macroblock size with a boundary of 3 extra pels on all sides. Only the inner
macroblock size plus 1 extra pel boundary are filled with valid values. Read a block from the window centred
on the (3,3) position with a 1/4 pel offset given by the input params into the destination block.
@param dstBlock		: Destination block.
@param qPelWin		: Window of size (4 * (_macroBlkHeight+6)) x (4 * (_macroBlkWidth+6))
@param qPelColOff	: The 1/4 pel X offset from the (3,3) window position in 1/4 pel units.
@param qPelRowOff	: The 1/4 pel Y offset from the (3,3) window position in 1/4 pel units.
@return						: none.
*/
void MotionEstimatorH264ImplTest::QuarterRead(OverlayMem2Dv2* dstBlock, OverlayMem2Dv2* qPelWin, int qPelColOff, int qPelRowOff)
{
	int fullRow, fullCol, quartRow, quartCol, dstX;

	short** window	= qPelWin->Get2DSrcPtr();	/// The origin of the window is around full pel position (3,3).

	short** dst			= dstBlock->Get2DSrcPtr();
	int			width		= dstBlock->GetWidth();
	int			height	= dstBlock->GetHeight();
	int			dstXPos = dstBlock->GetOriginX();
	int			dstYPos = dstBlock->GetOriginY();

	/// 1/4 pel rows and cols increment in quarter pel units with (3,3) full pel offset + input offset. As
	/// in: quartRow	= ((fullRow + 3) * 4) + qPelRowOff; and quartCol	= ((fullCol + 3) * 4) + qPelColOff;

	for(fullRow = 0, quartRow = (12 + qPelRowOff); fullRow < height; fullRow++, dstYPos++, quartRow += 4)
	{
		for(fullCol = 0, quartCol = (12 + qPelColOff), dstX = dstXPos; fullCol < width; fullCol++, quartCol += 4, dstX++)
			dst[dstYPos][dstX] = window[quartRow][quartCol];
	}//end fullRow...

}//end QuarterRead.






