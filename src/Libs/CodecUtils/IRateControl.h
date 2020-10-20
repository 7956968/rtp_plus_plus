/** @file

MODULE				: IRateControl

TAG						: IRC

FILE NAME			: IRateControl.h

DESCRIPTION		: An interface to rate control implementations.

COPYRIGHT			:	(c)CSIR 2007-2015 all rights resevered

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

=====================================================================================
*/
#ifndef _IRATECONTROL_H
#define _IRATECONTROL_H

/*
---------------------------------------------------------------------------
	Interface definition.
---------------------------------------------------------------------------
*/
class IRateControl
{
public:
	virtual ~IRateControl() {}
	
  virtual int		Create(int numOfFrames) = 0;
  virtual void	Destroy(void) = 0;
  virtual int   GetFameBufferLength(void) = 0;
  virtual void  SetRDLimits(double rateUpper, double rateLower, double distUpper, double distLower) = 0;

  /// Pre-encoding: Apply the rate control model to predict the distortion from the target rate.
  virtual int PredictDistortion(double targetAvgRate, double rateLimit) = 0;

  /// Post-encoding: Add sample measurements to the buffer/s and update the model parameters.
  virtual void StoreMeasurements(double rate, double coeffrate, double distortion, double mse, double mae) = 0;

  /// Return a performance measure that is implementation specific. E.g. Average rate across the buffer.
  virtual double GetPerformanceMeasure(void) = 0;

  /// Are the data in the buffers valid?
  virtual int ValidData(void) = 0;
    
  /// Was the prediction calculation out of bounds?
  virtual bool OutOfBounds(void) = 0;

  /// Reset the buffer to invalid.
  virtual void Reset(void) = 0;

  /// Utility function for testing and research data collection.
  virtual void Dump(char* filename) = 0;

  /// Commonly used utility methods (code refactoring)
protected:
  /// For running sums of a buffer. Adding the new value and dropping the oldest previous value.
  virtual double UpdateRunningSum(double prev, double curr, double currSum) { double newSum = currSum - prev; newSum += curr; return(newSum); }

  /// Solve in 2 variables returning soln in (x0, x1). If the determinant = 0 then leave (x0, x1) unchanged.
  virtual void CramersRuleSoln(double a00, double a01, double a10, double a11, double b0, double b1, double* x0, double* x1)
  { double D = (a00 * a11)-(a10 * a01);
    if(D != 0.0) { *x0 = ((b0 * a11)-(b1 * a01))/D; *x1 = ((a00 * b1)-(a10 * b0))/D; }  }

  /// Model equations.
  virtual double ModelDistortion(double rate, double a, double b, double var) { return(0.0); }
  virtual double ModelRate(double distortion, double a, double b, double var) { return(0.0); }

};//end IRateControl.


#endif	// _IRATECONTROL_H
