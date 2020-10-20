/** @file

MODULE				: RateControlImplPow

TAG						: RCIP

FILE NAME			: RateControlImplPow.cpp

DESCRIPTION		: A class to hold a power function  model for the frame buffer rate rate control to
                match an average rate stream.

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

=========================================================================================
*/
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#endif

#include "RateControlImplPow.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
int RateControlImplPow::Create(int numOfFrames)
{
  /// Clean up first.
  Destroy();

  if( (!_meanDiffSamples.Create(numOfFrames))  || (!_modelRateSamples.Create(numOfFrames))  || 
      (!_buffRateSamples.Create(numOfFrames))  || (!_distortionSamples.Create(numOfFrames)) ) 
    return(0);

  _numOfFrames = numOfFrames; 
  _meanDiffSamples.Clear(); 
  _modelRateSamples.Clear(); 
  _buffRateSamples.Clear(); 
  _distortionSamples.Clear();

  _prevMeanDiff   = 0.0;
  _prevModelRate  = 0.0;
  _prevBuffRate   = 0.0;
  _prevDistortion = 0.0;

  _outOfBoundsFlag    = false;

  _Sx   = 0.0;
  _Sy   = 0.0;
  _Sxy  = 0.0;
  _Sx2  = 0.0;

  _SEi  = (double)(numOfFrames*(numOfFrames+1))/2.0;
  _SEi2 = (double)(numOfFrames*(numOfFrames+1)*((2*numOfFrames)+1))/6.0;
  _SEy  = 0.0;
  _SEiy = 0.0;
  _a1   = 1.0;
  _a2   = 0.0;

  _Sbuff = 0.0;

  _RCTableLen = 0;
  _RCTablePos = 0;

#ifdef RCIP_DUMP_RATECNTL
  _RCTableLen = 500;
	_RCTable.Create(8, _RCTableLen);

  _RCTable.SetHeading(0, "Frame");
  _RCTable.SetDataType(0, MeasurementTable::INT);
  _RCTable.SetHeading(1, "Dmax");
  _RCTable.SetDataType(1, MeasurementTable::INT);
  _RCTable.SetHeading(2, "Rate");
  _RCTable.SetDataType(2, MeasurementTable::DOUBLE);
  _RCTable.SetHeading(3, "Pred Rate");
  _RCTable.SetDataType(3, MeasurementTable::DOUBLE);
  _RCTable.SetHeading(4, "Pred Mean Diff");
  _RCTable.SetDataType(4, MeasurementTable::DOUBLE);
  _RCTable.SetHeading(5, "Mean Diff");
  _RCTable.SetDataType(5, MeasurementTable::DOUBLE);
  _RCTable.SetHeading(6, "RD a");
  _RCTable.SetDataType(6, MeasurementTable::DOUBLE);
  _RCTable.SetHeading(7, "RD b");
  _RCTable.SetDataType(7, MeasurementTable::DOUBLE);

  _predMD           = 0.0;
  _MD               = 0.0;
  _predDmax         = 0.0;
  _predRate         = 0.0;
  _rate             = 0.0;
#endif

  return(1);
}//end Create.

void RateControlImplPow::Destroy(void)
{
  _RCTable.Destroy();

  _distortionSamples.Destroy(); 
  _modelRateSamples.Destroy(); 
  _buffRateSamples.Destroy(); 
  _meanDiffSamples.Destroy();

  _numOfFrames = 0;

}//end Destroy.

void RateControlImplPow::Reset(void)
{
  _distortionSamples.MarkAsEmpty(); 
  _modelRateSamples.MarkAsEmpty(); 
  _buffRateSamples.MarkAsEmpty(); 
  _meanDiffSamples.MarkAsEmpty();

}//end Reset.

void RateControlImplPow::Dump(char* filename)
{
  if(_RCTablePos > 0)
    _RCTable.Save(filename, ",", 1);
  _RCTablePos = 0;
}//end Dump.

/*
---------------------------------------------------------------------------
	Public Interface Methods.
---------------------------------------------------------------------------
*/

///---------------------- Post-Encoding Measurements --------------------------

/// Add samples to the fifo buffers and hold the sample that is discarded. The
/// last discarded value is needed in the running sum update process.

/** Store model measurements after encoding a frame.
All rate measurements are in bpp. This implementation does not use the mae parameter.
@param rate       : Total rate of the frame including headers.
@param coeffrate  : Not used in this implementation.
@param distortion : Implementation specific distortion of the frame.
@param mse        : Implementation specific signal mean square difference between pels in the frame.
@param mae        : Implementation specific signal mean absolute difference between pels in the frame.
@return           : none.
*/
void RateControlImplPow::StoreMeasurements(double rate, double coeffrate, double distortion, double mse, double mae)
{ 
  bool initialisationRequired = (bool)(!ValidData());  ///< Status prior to storing new measurement values.
  double meandiff = mse;

  /// Store all new measurements.
  PutMeanDiff(meandiff, initialisationRequired);      ///<  NB: Mean diff MUST be stored first as the others use its value.
  PutDistortion(distortion, initialisationRequired);  ///< Internally uses meandiff
  PutModelRate(rate, initialisationRequired);
  PutBuffRate(rate, initialisationRequired);

  if(!initialisationRequired) ///< /// Update the running sums associated with the model. 
  { ///< ------------------ Update ----------------------------------------------
    /// Update (ln(Dmax/md), ln(r)) running sums.
    double x = (_distortionSamples.GetBuffer())[0];
    double y = (_modelRateSamples.GetBuffer())[0];

    double x2   = x*x;
    double px2  = _prevDistortion*_prevDistortion;
    _Sx     = UpdateRunningSum(_prevDistortion, x, _Sx);
    _Sy     = UpdateRunningSum(_prevModelRate, y, _Sy);
    _Sxy    = UpdateRunningSum(_prevDistortion*_prevModelRate, x*y, _Sxy);
    _Sx2    = UpdateRunningSum(px2, x2, _Sx2);

    /// Improvements on the current parameters applied to the objective function. Find the Cramer's Rule 
    /// solution and test it against boundary conditions. If the solution is out of bounds then use a gradient
    /// decsent algorithm to modify the current parameters. Choose only those solutions that improve on the
    /// objective function.
    int si;
    /// Cramer's Solution and objective function (aa,bb) with current objective function (_a,_b)
    double bb, aa;
    CramersRuleSoln(_Sx2, _Sx, _Sx, (double)_numOfFrames, _Sxy, _Sy, &bb, &aa);
    double logaa = aa;
    aa = exp(aa);
    double loga           = log(_a);
    double currObjFunc    = 0.0;
    double cramerObjFunc  = 0.0;
    for(si = 0; si < _numOfFrames; si++)
    {
      double crs  = (_modelRateSamples.GetBuffer())[si];
      double ds   = (_distortionSamples.GetBuffer())[si];
      double curr = crs - (_b*ds) - loga;
      currObjFunc += curr*curr;
      double cramer = crs - (bb*ds) - logaa;
      cramerObjFunc += cramer*cramer;
    }//end for si...
    /// Outside boubndary conditions.
    bool cramerDisqualified = (aa < 24.0)||(aa > 100000.0)||(bb < -3.0)||(bb > -0.3);

    /// Choose best improved solution.
    if( !cramerDisqualified && (cramerObjFunc <= currObjFunc) )
    {
      _a = aa;
      _b = bb;
    }//end if !cramerDisqualified...
    else ///< use gradient descent change if Cramer's soln is out of bounds.
    {
      /// Determine the gradient vector at current parameters (_a,_b).
      double dsdb = 2.0*((_b*_Sx2) + (log(_a)*_Sx) - _Sxy);
      double dsda = 2.0*((_b*_Sx) + (log(_a)*(double)_numOfFrames) - _Sy)/_a;
      /// Search for gamma.
      double gamma = 1.0;
      double a, b;
      double gradObjFunc = currObjFunc + 1.0;
      int cnt = 0;
      while( (cnt < 26)&&(gradObjFunc > currObjFunc) )
      {
        b = _b - (gamma * dsdb);
        a = _a - (gamma * dsda);
        loga = log(a);
        gradObjFunc = 0.0;
        for(si = 0; si < _numOfFrames; si++)
        {
          double grad = (_modelRateSamples.GetBuffer())[si] - (b*(_distortionSamples.GetBuffer())[si]) - loga;
          gradObjFunc += grad*grad;
        }//end for si...

        gamma = gamma/2.0;
        cnt++;
      }//end while cnt...
      if(gradObjFunc <= currObjFunc)
      {
        _a = a;
        _b = b;
      }//end if gradObjFunc...
    }//end else...

    /// Update mean diff running sums.
    _SEy  = UpdateRunningSum(_prevMeanDiff, (_meanDiffSamples.GetBuffer())[0], _SEy);
    _SEiy = 0.0;
    for(int i = 0; i < _numOfFrames; i++)
      _SEiy  += (_numOfFrames-i)*(_meanDiffSamples.GetBuffer())[i]; ///< The sample time series are in reverse order in the fifo.
    /// Update linear constants. Cramer's Rule.
    CramersRuleSoln(_SEi2, _SEi, _SEi, (double)_numOfFrames, _SEiy, _SEy, &_a1, &_a2);

    /// Update total buffer rate running sums.
    _Sbuff = UpdateRunningSum(_prevBuffRate, (_buffRateSamples.GetBuffer())[0], _Sbuff);

  }//end if !initialisationRequired...
  else  ///< Indicates that there was no valid data in the model before storing these first samples.
  { ///< ------------------ Initialise ----------------------------------------------
    /// Initialise the fifo buffers for the R-D model, Mean Diff and Model Diff prediction models.
    _Sx   = 0;
    _Sy   = 0;
    _Sxy  = 0;    ///< Power R-D.
    _Sx2  = 0;

    _SEy  = 0.0;  ///< Linear Mean Diff
    _SEiy = 0.0;

    _Sbuff = 0.0; ///< Frame bit buffer.

    for(int i = 0; i < _numOfFrames; i++)
    {
      ///< Power R-D.
      double x = (_distortionSamples.GetBuffer())[i];
      double y = (_modelRateSamples.GetBuffer())[i];

      double x2 = x*x;
      _Sx   += x;
      _Sy   += y;
      _Sxy  += x*y;
      _Sx2  += x2;

      ///< Linear Mean Diff
      x = (_meanDiffSamples.GetBuffer())[i];
      /// _SEi and _SEi2 are constants.
      _SEy   += x;
      /// The sample time series are in reverse order in the fifo. x[fifo(0)] is x[_numOfFrames-1].
      /// The independent axis of the model describes samples at [1..N] where 1 is the oldest and N 
      /// from the last frame. Extrapolation will predict x[N+1].
      _SEiy  += (_numOfFrames-i)*x; ///< The sample time series are in reverse order in the fifo.

      /// Frame bit buffer rate. 
      _Sbuff += (_buffRateSamples.GetBuffer())[i];

    }//end for i...

  }//end else...

#ifdef RCIP_DUMP_RATECNTL
  _MD   = GetMostRecentMeanDiff();
  _rate = GetMostRecentBuffRate();

  /// This method is the last to be called after processing a frame. Therefore dump here.
  if(_RCTablePos < _RCTableLen)
  {
    _RCTable.WriteItem(0, _RCTablePos, _RCTablePos);            ///< Frame
    _RCTable.WriteItem(1, _RCTablePos, (int)(0.5 + _predDmax)); ///< Dmax
    _RCTable.WriteItem(2, _RCTablePos, _rate);                  ///< Rate
    _RCTable.WriteItem(3, _RCTablePos, _predRate);              ///< Pred Rate
    _RCTable.WriteItem(4, _RCTablePos, _predMD);                ///< Pred Mean Diff
    _RCTable.WriteItem(5, _RCTablePos, _MD);                    ///< Mean Diff
    _RCTable.WriteItem(6, _RCTablePos, _a);                     ///< Model parameter a
    _RCTable.WriteItem(7, _RCTablePos, _b);                     ///< Model parameter b

    _RCTablePos++;
  }//end if _RCTablePos... 
#endif

}//end StoreMeasurements.

///---------------------- Pre-Encoding Predictions --------------------------

/** Predict the distortion for the next frame from the desired average rate.
The distortion measure is predicted from the coeff rate (total rate - header rate) using
an appropriate R-D model. The header rate and the signal mean diff are predicted from the
previous measured frame data using linear extrapolation.
@param  targetAvgRate : Total targeted average rate (including headers).
@param  rateLimit     : Upper limit to predicted rate.
@return               : Predicted distortion.
*/
int RateControlImplPow::PredictDistortion(double targetAvgRate, double rateLimit)
{
  _outOfBoundsFlag = false;

  /// Default settings for the non-valid data case.
  double targetRate      = targetAvgRate;
  double targetRateLimit = rateLimit;

  ///------------------ Rate Prediction --------------------------------------
  if(ValidData())
  {
    /// Buffer averaging model: What must the target rate for the
    /// next N frames be to make the average over the rate buffer equal
    /// to the average target rate specified in the parameter?

    /// N past frames and recover in k frames: target rate = target avg rate*(N+k)/k - total buff/k.
    int numRecoverFrames = _numOfFrames;  ///< Make recovery same as past num of frames until further notice.
    targetRate = ((targetAvgRate * (double)(_numOfFrames + numRecoverFrames))/(double)numRecoverFrames) - (_Sbuff/(double)numRecoverFrames);

  }//end if ValidData...

  /// Keep the target rate within the upper rate limit.
  if(targetRate > targetRateLimit)
    targetRate = targetRateLimit;

  /// No less than the coeff floor rate.
  if(targetRate < 0.00)
    targetRate = _rateLower;

  ///--------------- Distortion Prediction -----------------------------------------

  /// Predict Mean Diff with linear extrapolation model.
  double predMD = (_a1*(double)(_numOfFrames+1)) + _a2;
  /// Can never be negative. Use last value if prediction is negative.
  if(predMD < 0.0)
    predMD = (_meanDiffSamples.GetBuffer())[0]; ///< Mean diff samples are in reverse order.

//  double distd = predMD*exp((log(targetRate/_a))/_b);
  double distd = ModelDistortion(targetRate, _a, _b, predMD);

  /// Cannot be greater than 16x16x(256^2)
  if(distd > _distUpper)
  {
    distd = _distUpper;
    _outOfBoundsFlag = true;
  }//end if distd...
  /// Cannot be less than 16x16x(2^2)
  if(distd < _distLower)
  {
    distd = _distLower;
    _outOfBoundsFlag = true;
  }//end if distd...

  /// Update the target  rate with changes in Dmax boundaries.
  if(_outOfBoundsFlag)
    targetRate = ModelRate(distd, _a, _b, predMD);

#ifdef RCIP_DUMP_RATECNTL
  _predRate = targetRate;
  _predMD   = predMD;
  _predDmax = distd;
#endif

  return((int)(distd + 0.5));
}//end PredictDistortion.

