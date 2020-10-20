/** @file

MODULE				: YuvRawFileHandler

TAG						: YUVRFH

FILE NAME			: YuvRawFileHandler.cpp

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
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include <fstream>

#include "YuvRawFileHandler.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
YuvRawFileHandler::YuvRawFileHandler(void): RawFileHandlerBase()
{
  _pBuff16  = NULL;
  _pBuff8   = NULL;
  _buffLen  = 0;
  _inType   = YUV4208P;
  _outType  = YUV42016P;
  _width    = 352;
  _height   = 288;
}//end constructor.

YuvRawFileHandler::~YuvRawFileHandler(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Public interface methods.
---------------------------------------------------------------------------
*/
int YuvRawFileHandler::Open(const char* filename, int rw)
{
   /// Clear previous stuff.
  Close();

 if(!RawFileHandlerBase::Open(filename, rw))
    return(0);

  /// Buffer for 1 frame of planar YUV. 
  _buffLen  = (_width * _height) + ((_width * _height)/2); ///< YUV 4:2:0
  if(_outType == YUV42016P)
  {
    _pBuff16 = new short[_buffLen];
    if(_pBuff16 == NULL)
    {
      Close();
      return(0);
    }//end if !_pBuff16...
  }//end if _outType...
  else
  {
    _pBuff8 = new unsigned char[_buffLen];
    if(_pBuff8 == NULL)
    {
      Close();
      return(0);
    }//end if !_pBuff8...
  }//end else...

  return(1);
}//end Open.

void YuvRawFileHandler::Close(void)
{
  if(_pBuff16 != NULL)
    delete[] _pBuff16;
  _pBuff16    = NULL;

  if(_pBuff8 != NULL)
    delete[] _pBuff8;
  _pBuff8    = NULL;

  _buffLen  = 0;

  RawFileHandlerBase::Close();
}//end Close.

/** Get a ptr to the head of the next yuv unit.
Return the ptr and the length of the next frame/unit
of 16-bit data after the currPos.
@param unitLen  : Total num of pels of the YUV frame in the unit
@return         : Ptr to the head of the unit.
*/
void* YuvRawFileHandler::GetNextUnit(int* unitLen)
{
  if((_pFile == NULL)||((_pBuff16 == NULL)&&(_pBuff8 == NULL)))
  {
    *unitLen = 0;
    return(NULL);
  }//end if _pFile...

  int cnt = 0;

  if(_currPos >= (_length-1)) ///< Wrap around the end.
    _currPos = 0;

  /// From the current pos typecast and copy one full frame of YUV 4:2:0 pels to the buffer.
  if(_inType == YUV4208P)
  {
    if(_outType == YUV42016P)
    {
      for(cnt = 0; (cnt < _buffLen)&&(_currPos < _length); cnt++, _currPos++)
        _pBuff16[cnt] = (short) ((unsigned char *)_pFile)[_currPos];
    }//end if _outType = YUV42016P...
    else if(_outType == YUV4208P)
    {
      for(cnt = 0; (cnt < _buffLen)&&(_currPos < _length); cnt++, _currPos++)
        _pBuff8[cnt] = (unsigned char) ((unsigned char *)_pFile)[_currPos];
    }//end if _outType = YUV4208P...
  }//end else if YUV4208P...
  else if(_inType == YUV420NV12)
  {
    int lumLen = _width * _height;    ///< Y plane.
    int chrLen = lumLen/4;            ///< UV packed.

    if(_outType == YUV42016P)
    {
      /// Lum is planar.
      for(cnt = 0; (cnt < lumLen)&&(_currPos < _length); cnt++, _currPos++)
        _pBuff16[cnt] = (short) ((unsigned char *)_pFile)[_currPos];

      /// Unpack the UV adjacent components into planes.
      for(; (cnt < (lumLen + chrLen))&&(_currPos < _length); cnt++)
      {
        _pBuff16[cnt]           = (short) ((unsigned char *)_pFile)[_currPos++];  ///< U.
        _pBuff16[cnt + chrLen]  = (short) ((unsigned char *)_pFile)[_currPos++];  ///< V.
      }//end for cnt...
      cnt += chrLen;
    }//end if _outType = YUV42016P...
  }//end if YUV420NV12...

  *unitLen = cnt;
  if(_outType == YUV4208P)
    return((void*)( _pBuff8 ));
  return((void*)( _pBuff16 ));
}//end GetNextFrame.

