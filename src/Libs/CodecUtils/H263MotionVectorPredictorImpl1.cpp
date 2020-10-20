/** @file

MODULE				: H263MotionVectorPredictorImpl1

TAG						: H263MVPI1

FILE NAME			: H263MotionVectorPredictorImpl1.cpp

DESCRIPTION		: A class to predicting motion vectors from the surrounding motion
                vectors of previously encoded/decoded macroblocks. Implements the 
                IMotionVectorPredictor() interface.

COPYRIGHT			: (c)CSIR 2007-2012 all rights resevered

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

#include "H263MotionVectorPredictorImpl1.h"
#include "VectorStructList.h"

/*
--------------------------------------------------------------------------
  Construction. 
--------------------------------------------------------------------------
*/

/*
--------------------------------------------------------------------------
  Public IMotionVectorPredictor Interface. 
--------------------------------------------------------------------------
*/

/** Get the 16x16 2-D prediction for the macroblock/block.
@param pList	: Input list of motion vectors.
@param blk		: Macroblock/block number to get the prediction for.
@param predX  : Output predicted X coordinate
@param predX  : Output predicted Y coordinate
@return	      :	1 = success, 0 = failure.
*/
int H263MotionVectorPredictorImpl1::Get16x16Prediction(	void* pList, int blk, int* predX, int* predY)
{
  VectorStructList* pVL = (VectorStructList *)pList;  ///< Typecast input vector list.

	/// Find the current coords in macroblock units.
	int mby			= blk/_widthInMbs;
	int mbx			= blk % _widthInMbs;

	/// Macroblock to the left.
	int mv1x = 0;
	int mv1y = 0;
	int mb1x = mbx - 1;
	if(mb1x >= 0) ///< Is not outside of the left edge.
	{
		int pos = (mby * _widthInMbs) + mb1x;
		mv1x = pVL->GetSimpleElement(pos, 0);
		mv1y = pVL->GetSimpleElement(pos, 1);
	}//end if mb1...

	/// Macroblock above.
	int mv2x = 0;
	int mv2y = 0;
	int mb2y = mby - 1;
	if(mb2y >= 0) ///< Is not outside the top edge.
	{
		int pos = (mb2y * _widthInMbs) + mbx;
		mv2x = pVL->GetSimpleElement(pos, 0);
		mv2y = pVL->GetSimpleElement(pos, 1);
	}//end if mb2y...
	else
	{
		/// The macroblock motion vector to the left is the predictor
		/// if at top edge, including the top right corner. Note that 
		/// it will also be [0,0] if it is the top left corner macroblock.
		*predX = mv1x;
		*predY = mv1y;
		return(1);
	}//end else...

	/// Macroblock above right.
	int mv3x = 0;
	int mv3y = 0;
	int mb3x = mbx + 1;
	int mb3y = mby - 1;
	if(mb3x < _widthInMbs) ///< Is not outside of the right edge.
	{
		int pos = (mb3y * _widthInMbs) + mb3x;
		mv3x = pVL->GetSimpleElement(pos, 0);
		mv3y = pVL->GetSimpleElement(pos, 1);
	}//end if mb1...

	/// Require the median of each vector component as the predictor.
	*predX = Median(mv1x, mv2x, mv3x);
	*predY = Median(mv1y, mv2y, mv3y);

  return(1);
}//end Get16x16Prediction. 

/*
--------------------------------------------------------------------------
  Private methods. 
--------------------------------------------------------------------------
*/

/** Calc the median of 3 numbers.
@param x	:	1st num.
@param y	:	2nd num.
@param z	:	3rd num.
@return		: The median of x, y and z.
*/
int H263MotionVectorPredictorImpl1::Median(int x, int y, int z)
{
	int min,max;

	// min = MIN(x,MIN(y,z)) and max = MAX(x,MAX(y,z)).
	if( (y - z) < 0 )	{	min = y;	max = z;	}
	else { min = z; max = y; }
	if(x < min) min = x;
	if(x > max) max = x;
	// Median.
	int result = x + y + z - min - max;

	return(result);
}//end Median.



