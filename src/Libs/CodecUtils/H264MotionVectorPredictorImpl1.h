/** @file

MODULE				: H264MotionVectorPredictorImpl1

TAG						: H264MVPI1

FILE NAME			: H264MotionVectorPredictorImpl1.h

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
#ifndef _H264MOTIONVECTORPREDICTORIMPL1_H
#define _H264MOTIONVECTORPREDICTORIMPL1_H

#include "IMotionVectorPredictor.h"
#include "MacroBlockH264.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class H264MotionVectorPredictorImpl1 : public IMotionVectorPredictor
{
/// Construction.
public:

  H264MotionVectorPredictorImpl1(MacroBlockH264* pMb) { _pMb = pMb; }

  virtual ~H264MotionVectorPredictorImpl1(void) {}

/// IMotionVectorPredictor Interface.
public:
  virtual int	Create(void) { return(1); }

	/** Get the 16x16 2-D prediction for the macroblock/block.
  A NULL pList implies that the mb motion vectors are already valid otherwise
  load them from the pList first. The neighbourhood must also be correct from
  prior calls.
	@param pList	: Input list of motion vectors.
	@param blk		: Macroblock/block number to get the prediction for.
  @param predX  : Output predicted X coordinate
  @param predY  : Output predicted Y coordinate
	@return	      :	1 = success, 0 = failure.
	*/
	int Get16x16Prediction(	void* pList, int blk, int* predX, int* predY)
  {
    if(_pMb == NULL)
      return(0);

    MacroBlockH264::GetMbMotionMedianPred( &(_pMb[blk]), predX, predY);
    return(1);
  }

	/** Force a 16x16 motion vector for the macroblock/block.
  Used to set up future predictions.
	@param blk	: Macroblock/block number to set.
  @param mvX  : X coordinate
  @param mvY  : Y coordinate
	@return	    :	none.
	*/
	void Set16x16MotionVector(int blk, int mvX, int mvY)
  {
    if(_pMb == NULL)
      return;

    _pMb[blk]._mvX[MacroBlockH264::_16x16] = mvX;
    _pMb[blk]._mvY[MacroBlockH264::_16x16] = mvY;
  }

/// Local methods.
protected:

/// Local members.
protected:

	// Parameters must remain const for the life time of this instantiation.
  MacroBlockH264* _pMb; ///< List of the active image mbs.

};//end H264MotionVectorPredictorImpl1.

#endif // !_H264MOTIONVECTORPREDICTORIMPL1_H

