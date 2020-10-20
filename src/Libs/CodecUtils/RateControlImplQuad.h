/** @file

MODULE				: RateControlImplQuad

TAG						: RCIQ

FILE NAME			: RateControlImplQuad.h

DESCRIPTION		: A class to hold an inverse quadratic model for the frame buffer rate rate control to
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

===========================================================================
*/
#ifndef _RATECONTROLIMPLQUAD_H
#define _RATECONTROLIMPLQUAD_H


#pragma once

#include  <math.h>
#include  "Fifo.h"
#include  "IRateControl.h"
#include  "MeasurementTable.h"

//#define RCIQ_DUMP_RATECNTL  1

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class RateControlImplQuad : public IRateControl
{
public:
  RateControlImplQuad(void) 
  { _a = 15000.0; _b = -1.3; _numOfFrames = 0; _modelMismatchType = RateControlImplQuad::NONE; 
    _rateUpper = 24.0; _rateLower = 0.0001; _distUpper = 16777216.0; _distLower = 1024.0; 
  }
  RateControlImplQuad(double modelparam_a, double modelparam_b, int modelmismatchtype) 
  { _a = modelparam_a; _b = modelparam_b; _numOfFrames = 0; _modelMismatchType = modelmismatchtype; 
    _rateUpper = 24.0; _rateLower = 0.0001; _distUpper = 16777216.0; _distLower = 1024.0;
  }
  virtual ~RateControlImplQuad() { Destroy(); }

// Setup and interface methods.
public:
  int		Create(int numOfFrames);
  void	Destroy(void);
  int   GetFameBufferLength(void) { return(_numOfFrames); }
  void  SetRDLimits(double rateUpper, double rateLower, double distUpper, double distLower) 
    { _rateUpper = rateUpper; _rateLower = rateLower; _distUpper = distUpper; _distLower = distLower; }

  /// Pre-encoding: Apply the rate control model to predict the distortion from the target coeff rate.
  int PredictDistortion(double targetAvgRate, double rateLimit);
  /// Post-encoding: Add samples to the fifo buffers and hold the sample that is discarded.
  void StoreMeasurements(double rate, double coeffrate, double distortion, double mse, double mae);

  /// Return average rate across the buffer as the performance measure.
  double GetPerformanceMeasure(void) { return(_Sbuff/(double)_numOfFrames); }
  /// Is the data in the fifos valid.
  int ValidData(void) 
    { if(!_distortionSamples.Empty() && !_coeffRateSamples.Empty() && !_meanDiffSamples.Empty() && !_totalRateSamples.Empty() && !_headerRateSamples.Empty()) return(1); return(0); }
  /// Was the prediction calculation out of bounds?
  bool OutOfBounds(void) { return(_outOfBoundsFlag); }
  /// Reset the fifos to the last valid sample.
  void Reset(void);

  /// Utility functions.
  void Dump(char* filename);  ///< Requires RCIQ_DUMP_RATECNTL to be set.

/// Further implementation specific public methods.
public:
  /// Set the function type to use for model mismatch.
  void SetModelMismatchFunc(const int funcType) { _modelMismatchType = funcType; }

/// Private methods.
protected:
  /// These following group of methods must be called in this exact order because they operate
  /// on members that are interdependant.
  void PutMeanDiff(double meandiff, bool initialise) 
    { if(meandiff < 1.0) meandiff = 1.0;
      PutLinearMeasure(_meanDiffSamples, meandiff, &_prevMeanDiff, initialise); }
  void PutDistortion(double distortion, bool initialise) ///< Must only be called after a call to PutMeanDiff(). 
    { if(distortion < 1.0) distortion = 1.0;
      PutLinearMeasure(_distortionSamples, ((_meanDiffSamples.GetBuffer())[0]/distortion), &_prevDistortion, initialise); }
  void PutCoeffRate(double rate, bool initialise) 
    { PutLinearMeasure(_coeffRateSamples, rate, &_prevCoeffRate, initialise); } 
  void PutModelRateDiff(double ratediff, bool initialise) 
    { PutLinearMeasure(_modelRateDiffSamples, ratediff, &_prevModelRateDiff, initialise); }
  void PutTotalRate(double rate, bool initialise) 
    { PutLinearMeasure(_totalRateSamples, rate, &_prevTotalRate, initialise); }
  void PutHeaderRate(double headerRate, bool initialise) 
    { PutLinearMeasure(_headerRateSamples, headerRate, &_prevHeaderRate, initialise); }

  void PutLinearMeasure(Fifo& samples, double m, double* prev, bool initialise) 
    { if(!initialise) *prev = samples.GetFirstOut(); else *prev = m; 
      samples.AddFirstIn(m); }

  double GetMostRecentCoeffRate(void) { return((_coeffRateSamples.GetBuffer())[0]); }
  double GetMostRecentTotalRate(void) { return((_totalRateSamples.GetBuffer())[0]); }
  double GetMostRecentHeaderRate(void) { return((_headerRateSamples.GetBuffer())[0]); }
  double GetMostRecentMeanDiff(void) { return((_meanDiffSamples.GetBuffer())[0]); }
  double GetMostRecentModelRateDiff(void) { return((_modelRateDiffSamples.GetBuffer())[0]); }

  /// Model equations.
  double ModelDistortion(double rate, double a, double b, double var) { return( (2.0*var*b)/(sqrt((a*a) + (4.0*b*rate)) - a) ); }
  double ModelRate(double distortion, double a, double b, double var) { return( ((a*var)/distortion) + ((b*var)/(distortion*distortion)) ); }

/// Constant members.
public:
  /// Model mismatch type.
  static const int NONE     = 0;
  static const int CONSTANT = 1;
  static const int LINEAR   = 2;

/// Common members.
protected:
	int				_numOfFrames;
  /// Clip the values of the rate and distortion to these limits.
  double    _rateUpper;
  double    _rateLower;
  double    _distUpper;
  double    _distLower;

  Fifo      _meanDiffSamples;      ///< Mean differnce between samples of frames.
  double    _prevMeanDiff;
  Fifo      _totalRateSamples;     ///< Stream rate including headers of frames.
  double    _prevTotalRate;
  Fifo      _coeffRateSamples;     ///< Coeff rate of frames.
  double    _prevCoeffRate;
  Fifo      _distortionSamples;    ///< (md/Dmax) for the frames.
  double    _prevDistortion;
  Fifo      _headerRateSamples;    ///< Header rate of frames.
  double    _prevHeaderRate;
  Fifo      _modelRateDiffSamples; ///< Model mismatch of frames.
  double    _prevModelRateDiff;
  double    _prevCoeffRatePred;
  /// Signal when the predicted values are not valid due to out of range errors. Use
  /// this signal to prevent contributions to the model data.
  bool      _outOfBoundsFlag;

  /// The model mismatch can be selected.
  int       _modelMismatchType;
  double    _movingAvg; ///< For the CONSTANT mismatch type a cumulative moving average is used.
  int       _movingAvgCount;

  /// For the quadratic model x = md/Dmax and y = R. To update the constants
  /// a and b, quadratic roots are used to solve the linear equation for a least
  /// squares criterion curve fit. Running sums are used.
  double    _Sxy; ///< Sum(r.Dmax/md) Note the inverse of x here.
  double    _Sx2; ///< Sum(md^2/Dmax^2)
  double    _Sx;  ///< Sum(md/Dmax)
  double    _Sy;  ///< Sum(r)
  double    _a;   ///< Quadratic model "constants".
  double    _b;

  /// The prediction of Mean Diff requires a least squares criterion curve fit
  /// to a linear extrapolation model with constants a1 and a2.
  double    _SEi;
  double    _SEi2;
  double    _SEy;
  double    _SEiy;
  double    _a1;  ///< Linear model constants.
  double    _a2;

  /// Buffer rate control for average bit rate.
  double    _Sbuff;

  /// The prediction of frame header rate requires a least mean squares criterion curve fit
  /// to a linear extrapolation model with constants p1 and p2. Uses _SEi and _SEi2 above.
  double    _SPy;
  double    _SPiy;
  double    _p1;  ///< Linear model constants.
  double    _p2;

  /// The prediction of model mismatch in rate requires a least mean squares criterion curve fit
  /// to a linear extrapolation model with constants m1 and m2. Uses _SEi and _SEi2 above.
  double    _SMy;
  double    _SMiy;
  double    _m1;  ///< Linear model constants.
  double    _m2;

  MeasurementTable  _RCTable;
  int               _RCTableLen;
  int               _RCTablePos;
#ifdef RCIQ_DUMP_RATECNTL
  double            _predMD;
  double            _MD;
  double            _predHeaderRate;
  double            _headerRate;
  double            _predDmax;
  double            _predCoeffRate;
  double            _coeffRate;
  double            _rate;
#endif
};// end class RateControlImplQuad.

#endif	// _RATECONTROLIMPLQUAD_H
