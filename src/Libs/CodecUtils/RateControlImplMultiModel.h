/** @file

MODULE				: RateControlImplMultiModel

TAG						: RCIMM

FILE NAME			: RateControlImplMultiModel.h

DESCRIPTION		: A class to hold a multi-function model for the frame buffer rate rate control to
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
#ifndef _RATECONTROLIMPLMULTIMODEL_H
#define _RATECONTROLIMPLMULTIMODEL_H

#pragma once

#include  <math.h>
#include  "Fifo.h"
#include  "IRateControl.h"
#include  "MeasurementTable.h"

//#define RCIMM_DUMP_RATECNTL  1

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class RateControlImplMultiModel : public IRateControl
{
public:
  RateControlImplMultiModel(void) 
  { _a = 15000.0; _b = -1.3; _numOfFrames = 0; 
    _rateUpper = 24.0; _rateLower = 0.0001; _distUpper = 16777216.0; _distLower = 1024.0;
  }
  RateControlImplMultiModel(double modelparam_a, double modelparam_b) 
  { _a = modelparam_a; _b = modelparam_b; _numOfFrames = 0; 
    _rateUpper = 24.0; _rateLower = 0.0001; _distUpper = 16777216.0; _distLower = 1024.0;
  }
  virtual ~RateControlImplMultiModel() { Destroy(); }

// IRateControl Interface methods.
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
      { if(!_distortionSamples.Empty() && !_meanDiffSamples.Empty() && !_modelRateSamples.Empty() && !_buffRateSamples.Empty()) return(1); return(0); }
  /// Was the prediction calculation out of bounds?
  bool OutOfBounds(void) { return(_outOfBoundsFlag); }
  /// Reset the fifos. Use after a change in frame type.
  void Reset(void);

  /// Utility functions.
  void Dump(char* filename);  ///< Requires RCIP_DUMP_RATECNTL to be set.

/// Further implementation specific public methods.
public:

/// Private methods.
protected:
  /// These following group of methods must be called in this exact order because they operate
  /// on members that are interdependant.
  void PutMeanDiff(double meandiff, bool initialise) 
    { if(meandiff < 1.0) meandiff = 1.0;
      PutLinearMeasure(_meanDiffSamples, meandiff, &_prevMeanDiff, initialise); }
  void PutDistortion(double distortion, bool initialise) ///< Must only be called after a call to PutMeanDiff(). 
    { if(distortion < 1.0) distortion = 1.0;
      PutLogMeasure(_distortionSamples, (distortion/(_meanDiffSamples.GetBuffer())[0]), &_prevDistortion, initialise); }
  void PutModelRate(double rate, bool initialise) 
    { PutLogMeasure(_modelRateSamples, rate, &_prevModelRate, initialise); }
  void PutBuffRate(double rate, bool initialise) 
    { PutLinearMeasure(_buffRateSamples, rate, &_prevBuffRate, initialise); }

  void PutLogMeasure(Fifo& samples, double m, double* prev, bool initialise)
    { if(m <= 0.0) m = 0.0001; 
      if(!initialise) *prev = samples.GetFirstOut(); else *prev = log(m); 
      samples.AddFirstIn(log(m)); }

  void PutLinearMeasure(Fifo& samples, double m, double* prev, bool initialise) 
    { if(!initialise) *prev = samples.GetFirstOut(); else *prev = m; 
      samples.AddFirstIn(m); }

  double GetMostRecentModelRate(void) { return(exp((_modelRateSamples.GetBuffer())[0])); }
  double GetMostRecentBuffRate(void) { return((_buffRateSamples.GetBuffer())[0]); }
  double GetMostRecentMeanDiff(void) { return((_meanDiffSamples.GetBuffer())[0]); }

  /// Model equations.
  double ModelDistortion(double rate, double a, double b, double var) { return(var*exp((log(rate/a))/b)); }
  double ModelRate(double distortion, double a, double b, double var) { return(a*pow((distortion/var), b));}

/// Constant members.
public:

/// Common members.
protected:
	int				_numOfFrames;
  /// Clip the values of the rate and distortion to these limits.
  double    _rateUpper;
  double    _rateLower;
  double    _distUpper;
  double    _distLower;

  Fifo      _meanDiffSamples;     ///< Mean differnce between samples within frames.
  double    _prevMeanDiff;
  Fifo      _buffRateSamples;     ///< Stream rate including headers of frames.
  double    _prevBuffRate;
  Fifo      _modelRateSamples;    ///< Model rate is the ln(stream rate).
  double    _prevModelRate;
  Fifo      _distortionSamples;   ///< Dmax for the frames.
  double    _prevDistortion;
  /// Signal when the predicted values are not valid due to out of range errors. Use
  /// this signal to prevent contributions to the model data.
  bool      _outOfBoundsFlag;

  /// To update the constants, _a and _b, Cramer's Rule is used to solve the linear equation 
  /// for a least mean squares criterion curve fit. Running sums are used.
  double    _Sxy;
  double    _Sx2;
  double    _Sx;
  double    _Sy;
  double    _a; ///< Power model constants.
  double    _b;

  /// The prediction of Mean Diff requires a least mean squares criterion curve fit
  /// to a linear extrapolation model with constants a1 and a2.
  double    _SEi;
  double    _SEi2;
  double    _SEy;
  double    _SEiy;
  double    _a1;  ///< Linear model constants.
  double    _a2;

  /// Buffer rate control for average bit rate.
  double    _Sbuff;

  MeasurementTable  _RCTable;
  int               _RCTableLen;
  int               _RCTablePos;
#ifdef RCIMM_DUMP_RATECNTL
  double            _predMD;
  double            _MD;
  double            _predDmax;
  double            _predRate;
  double            _rate;
#endif
};// end class RateControlImplMultiModel.

#endif	// _RATECONTROLIMPLMULTIMODEL_H
