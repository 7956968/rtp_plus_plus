/** @file

MODULE				: RawFileHandlerBase

TAG						: H264RFH

FILE NAME			: RawFileHandlerBase.cpp

DESCRIPTION		: A base class for reading and writing raw data from and to
                a binary file. Derived classes must provide interpretation
                of headers and unit demarkations. 

COPYRIGHT			:	(c)CSIR 2007-2013 all rights resevered

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
#include <string.h>
#endif

#include <fstream>

#include "RawFileHandlerBase.h"

#define INITIAL_BUFFER_SIZE 40960

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
RawFileHandlerBase::RawFileHandlerBase(void)
{
  /// Buffer to hold the file data.
  _pFile      = NULL;
  _length     = 0;
  _currPos    = 0; ///< Current position in _pFile.
  _direction  = READ;
}//end constructor.

RawFileHandlerBase::~RawFileHandlerBase(void)
{
  Close();
}//end destructor.

/*
---------------------------------------------------------------------------
	Public interface methods.
---------------------------------------------------------------------------
*/
int RawFileHandlerBase::Open(const char* filename, int rw)
{
  /// Clear previous stuff.
  Close();

  strcpy(_filename, filename);
  _direction = rw;

  if(rw == READ)
  {
	  std::ifstream in1(filename, std::ios_base::in | std::ios_base::binary);
	  if (in1.is_open())
	  {
		  /// Get length of file:
		  in1.seekg (0, std::ios::end);
      std::streamoff len = in1.tellg();
		  in1.seekg (0, std::ios::beg);

      _length = static_cast<int>(len);
		  _pFile = new char[_length];

		  /// Read data as a block:
		  in1.read (_pFile, len);
		  in1.close();
    }//end if is_open...
    else
      return(0);
  }//end rw...
  else
  {
    /// For the WRITE case, the actual writing to file is delayed to the next call to Close().
    _length = INITIAL_BUFFER_SIZE;
    _pFile = new char[_length];
  }//end else...

  if(_pFile == NULL)
    return(0);

  return(1);
}//end Open.

/** Put an encoded unit onto the file stream buffer.
NOTE: The file stream buffer is only written to the file on a
call to Close(). Add this unit to the end of the stream buffer
references by the currPos.
@param length   : Bytes in the encoded unit.
@param unitLen  : Num of bytes to add.
@return         : Success = 1, Failure = 0.
*/
int RawFileHandlerBase::PutNextUnit(void* unit, int unitLen)
{
  if(_pFile == NULL)
    return(0);

  /// Grow the size of the file buffer if it overflows.
  if((_currPos + unitLen) >= _length)
  {
    /// Create a temp buffer.
    char* temp = NULL;
    temp = new char[_currPos+1];
    if(temp == NULL)
      return(0);
    memcpy(temp, _pFile, sizeof(char) * (_currPos + 1));

    /// Decide on how much to grow the buffer size with the default as double _length.
    if(unitLen > _length)
      _length = 4 * unitLen;
    else
      _length = 2 * _length;

    /// Create the bigger file buffer.
    delete[] _pFile;
    _pFile = NULL;
    _pFile = new char[_length];
    if(_pFile == NULL)
    {
      delete[] temp;
      _length = 0;
      return(0);
    }//end if _pFile...

    /// Copy the temp buffer into the new buffer.
    memcpy(_pFile, temp, sizeof(char) * (_currPos + 1));

    delete[] temp;
    temp = NULL;

  }//end if _currPos...

  /// Load the unit onto the end of the buffer.
  char* p = (char *)unit;
  for(int i = 0; i < unitLen; i++)
    _pFile[_currPos++] = p[i];

  return(1);
}//end PutNextUnit.

void RawFileHandlerBase::Close(void)
{
  /// File writing is delayed until a Close().
  if((_direction == WRITE)&&(_currPos > 0))
  {
    std::ofstream out1(_filename, std::ios_base::out | std::ios_base::binary);
    if(out1.is_open())
    {
      out1.write(_pFile, _currPos);
      out1.close();
      _currPos = 0;
    }//end if is_open...
  }//end if _direction...

  if(_pFile != NULL)
    delete[] _pFile;
  _pFile    = NULL;
  _length   = 0;
  _currPos  = 0;
}//end Close.

