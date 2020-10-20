/** @file

MODULE				: YuvRawFileHandler

TAG						: YUVRFH

FILE NAME			: YuvRawFileHandler.h

DESCRIPTION		: A class to hold raw yuv data used for video codec testing.

COPYRIGHT			:	(c)CSIR 2007-2012 all rights resevered

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
#ifndef _YuvRawFileHandler_H
#define _YuvRawFileHandler_H

#pragma once

#include "RawFileHandlerBase.h"
/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class YuvRawFileHandler : public RawFileHandlerBase
{
public:
	YuvRawFileHandler(void);
	virtual ~YuvRawFileHandler(void);

/// Public interface.
public:

  int   Open(const char* filename, int rw);
  void  Close(void);
  void* GetNextUnit(int* unitLen);

/// Public constants.
public:
	/// Read/In or write/out parameter for Get/PutNextUnit() method.
	static const int YUV4208P	    = 0;
	static const int YUV420NV12   = 1;

	static const int YUV42016P    = 16;

/// Implementation specific interface.
public:

  /// Call these before a call to Open().
  void SetWidth(int width)              { _width = width; }
  void SetHeight(int height)            { _height = height; }
  void SetType(int type)                { _inType = type; }
  void SetType(int intype, int outtype) { _inType = intype; _outType = outtype;}

  void* GetCurrUnit(int* unitLen) { *unitLen = _buffLen; if(_outType == YUV42016P) return((void *)_pBuff16); else return((void *)_pBuff8); }

/// Implementation specific members.
protected:

  short*          _pBuff16;   ///< Units are 16-bit shorts in this implementation.
  unsigned char*  _pBuff8;   ///< Units are 8-bit chars in this implementation.
  int             _buffLen;

  /// YUV 4:2:0 data is stored contiguously as 8/16-bits per component in planar format.
  int     _inType;
  int     _outType; ///< Default = YUV42016P
  int     _width;
  int     _height;

};// end class YuvRawFileHandler.

#endif	//_YuvRawFileHandler_H
