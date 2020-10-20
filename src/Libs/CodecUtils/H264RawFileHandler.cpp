/** @file

MODULE				: H264RawFileHandler

TAG						: H264RFH

FILE NAME			: H264RawFileHandler.cpp

DESCRIPTION		: A class to hold H.264 slice header data for use in the 
								H264v2Codec class.

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

#include "H264RawFileHandler.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
H264RawFileHandler::H264RawFileHandler(void)
{
}//end constructor.

H264RawFileHandler::~H264RawFileHandler(void)
{
}//end destructor.

/*
---------------------------------------------------------------------------
	Public interface methods.
---------------------------------------------------------------------------
*/
/** Get a ptr to the head of the next encoded unit.
Return the ptr and the length of the next encoded frame/unit
of data after the currPos.
@param length : Bytes in the encoded unit
@return       : Ptr to the head of the unit.
*/
void* H264RawFileHandler::GetNextUnit(int* unitLen)
{
  if(_pFile == NULL)
  {
    *unitLen = 0;
    return(NULL);
  }//end if _pFile...

  if(_currPos >= (_length-1)) ///< Wrap around the end.
    _currPos = 0;

  int unitPos = _currPos;     ///< This unit starts here.

  /// From the current pos count the bytes to the start of the next 00 00 00 01 H.264 start code.
  int len = 4;
  _currPos += 4;
  int found = 0;
  for(  ; (_currPos < _length)&&(!found); _currPos++, len++)
  {
    if( (_pFile[_currPos-3] == 0)&&(_pFile[_currPos-2] == 0)&&(_pFile[_currPos-1] == 0)&&(_pFile[_currPos] == 1) )
      found = 1;
  }//end for _currPos...
  if(found)
  {
    _currPos -= 4;
    len -= 4;
  }//end if found...

  *unitLen = len;
  return((void*)( &(_pFile[unitPos]) ));
}//end GetNextFrame.


