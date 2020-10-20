/** @file

MODULE				: H264MbImgCache

TAG						: H264MIC

FILE NAME			: H264MbImgCache.h

DESCRIPTION		: A class to cache image blocks aligned with H.264 macroblocks data.

COPYRIGHT			:	(c)CSIR 2007-2014 all rights resevered

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
#ifndef _H264MBIMGCACHE_H
#define _H264MBIMGCACHE_H

#pragma once

#include "OverlayMem2Dv2.h"
#include "MacroBlockH264.h"

/*
---------------------------------------------------------------------------
	Class definition.
---------------------------------------------------------------------------
*/
class H264MbImgCache
{
public:
	H264MbImgCache(void);
	virtual ~H264MbImgCache(void);

/// Cache operations.
public:
	/** Create mem container.
	Set the mem for a 16x16 lum and two 8x8 chr blocks with their overlays.
	@return			: 1 = success, 0 = failed.
	*/
	int Create(void);

	/** Set the image space to operate on.
	Hold references to the image overlays that the mb will aligned with. This
  is done once and then all calls to Cache() or Restore() will reference this
  image.
  @param  lum : Lum overlay reference.
  @param  cb  : Chr Cb overlay reference.
  @param  cr  : Chr Cr overlay reference.
	@return			: 1 = success, 0 = failed.
	*/
  void SetImage(OverlayMem2Dv2* lum, OverlayMem2Dv2* cb, OverlayMem2Dv2* cr)
  {
    _lum = lum; _cb = cb; _cr = cr; 
  }//end SetImage.

	/** Cache the mb aligned image.
	Use the mb data to align the mb with the image position and copy the image
  data into the cache.
  image.
  @param  pMb : Mb to cache.
	@return			: 1 = success, 0 = failed.
	*/
  int Cache(MacroBlockH264* pMb);

	/** Restore the mb aligned image from the cache.
	Use the mb data to align the mb with the image position and copy the cache
  data into the image.
  image.
  @param  pMb : Mb to restore.
	@return			: 1 = success, 0 = failed.
	*/
  int Restore(MacroBlockH264* pMb);

  /** Test if the cache is equal to the image block.
  @param  pMb : Mb position to compare.
  @return     : 1 = Equal, 0 = Not equal.
  */
  int Equal(MacroBlockH264* pMb);

/// Member access.
public:
	OverlayMem2Dv2*	GetLumCache(void)		{ return(_lum16x16); }
	OverlayMem2Dv2*	GetCbCache(void)		{ return(_cb8x8); }
	OverlayMem2Dv2*	GetCrCache(void)		{ return(_cr8x8); }

/// Private methods.
protected:
  void Destroy(void);

/// Private data block members.
protected:
  /// Image references.
  OverlayMem2Dv2* _lum;
  OverlayMem2Dv2* _cb;
  OverlayMem2Dv2* _cr;

  /// Cache blocks and overlays.
  short*          _plumCache;
  OverlayMem2Dv2* _lum16x16;
  short*          _pCbCache;
  OverlayMem2Dv2* _cb8x8;
  short*          _pCrCache;
  OverlayMem2Dv2* _cr8x8;

};// end class H264MbImgCache.

#endif	//_H264MBIMGCACHE_H
