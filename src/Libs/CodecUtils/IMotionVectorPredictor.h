/** @file

MODULE				: IMotionVectorPredictor

TAG						: IMVP

FILE NAME			: IMotionVectorPredictor.h

DESCRIPTION		: An interface to class implementations that predict the motion
                vector for the current macroblock/block from the surrounding
                previously encoded/decoded motion vectors. 

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
#ifndef _IMOTIONVECTORPREDICTOR_H
#define _IMOTIONVECTORPREDICTOR_H

/*
---------------------------------------------------------------------------
	Interface definition.
---------------------------------------------------------------------------
*/
class IMotionVectorPredictor
{
	public:
		virtual ~IMotionVectorPredictor() {}
		
		/** Create any private memory required.
		This is primarily to encourage a two stage construction process where
		mem is not alloc in the implementation constructor.
		@return	:	1 = success, 0 = failure.
		*/
		virtual int Create(void) = 0;

		/** Get the 16x16 2-D prediction for the macroblock/block.
		@param pList	: Input list of motion vectors.
		@param blk		: Macroblock/block number to get the prediction for.
    @param predX  : Output predicted X coordinate
    @param predX  : Output predicted Y coordinate
		@return	      :	1 = success, 0 = failure.
		*/
		virtual int Get16x16Prediction(	void* pList, int blk, int* predX, int* predY) = 0;

	  /** Force a 16x16 motion vector for the macroblock/block.
    Used to set up future predictions.
	  @param blk	: Macroblock/block number to set.
    @param mvX  : X coordinate
    @param mvY  : Y coordinate
	  @return	    :	none.
	  */
    virtual void Set16x16MotionVector(int blk, int mvX, int mvY) {}

};//end IMotionVectorPredictor.

#endif  /// _IMOTIONVECTORPREDICTOR_H
