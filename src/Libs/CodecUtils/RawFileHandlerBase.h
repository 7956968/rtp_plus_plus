/** @file

MODULE				: RawFileHandlerBase

TAG						: RFHB

FILE NAME			: RawFileHandlerBase.h

DESCRIPTION		: A base class for reading and writing raw data from and to
                a binary file. Derived classes must provide interpretation
                of headers and unit demarkations.

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
#ifndef _RAWFILEHANDLERBASE_H
#define _RAWFILEHANDLERBASE_H

#pragma once

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class RawFileHandlerBase
{
public:
	RawFileHandlerBase(void);
	virtual ~RawFileHandlerBase(void);

/// Public constants.
public:
	/// Read or write parameter for Open() method.
	static const int READ	    = 0;
	static const int WRITE    = 1;

/// Public interface.
public:

  virtual int   Open(const char* filename, int rw);
  virtual void  Close(void);

  /** Get a ptr to the head of the next data unit.
  Return the ptr and the length of the next frame/unit
  of data after the currPos.
  @param length : Bytes in the encoded unit
  @return       : Ptr to the head of the unit.
  */
  virtual void* GetNextUnit(int* unitLen) { return(NULL); }
  virtual int   PutNextUnit(void* unit, int unitLen);

/// Private members.
protected:

  /// Buffer to hold the file data.
  char* _pFile;
  int   _length;
  int   _currPos; ///< Current position in _pFile.
  char  _filename[256];
  int   _direction; /// READ or WRITE.

};// end class RawFileHandlerBase.

#endif	//_RAWFILEHANDLERBASE_H
