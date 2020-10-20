/** @file

MODULE				: CodecDistortionDef

TAG						: CDD

FILE NAME			: CodecDistortionDef.h

DESCRIPTION		: A class to define with a macro the distortion metric used in codecs.

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

#ifndef _CODECDISTORTIONDEF_H
#define _CODECDISTORTIONDEF_H

#pragma once

#include <math.h>
#include <stdlib.h>

#define DISTORTIONABSDIFF(x,y) ( abs((x)-(y)) )
#define DISTORTIONSQRDIFF(x,y) ( ((x)-(y))*((x)-(y)) )

#undef USE_ABSOLUTE_DIFFERENCE
//#define USE_ABSOLUTE_DIFFERENCE 1

/// DISTORTION params x = original y = distorted
#ifdef USE_ABSOLUTE_DIFFERENCE
#define DISTORTION(x,y) ( DISTORTIONABSDIFF((x),(y)) )
#else
#define DISTORTION(x,y) ( DISTORTIONSQRDIFF((x),(y)) )
#endif

#endif	// _CODECDISTORTIONDEF_H
