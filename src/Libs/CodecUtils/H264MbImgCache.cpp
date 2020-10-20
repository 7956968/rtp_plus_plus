/** @file

MODULE				: H264MbImgCache

TAG						: H264MIC

FILE NAME			: H264MbImgCache.cpp

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
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#include <string.h>
#endif

#include <memory.h>
#include "H264MbImgCache.h"

/*
---------------------------------------------------------------------------
	Construction and destruction.
---------------------------------------------------------------------------
*/
H264MbImgCache::H264MbImgCache(void)
{
  /// Image references.
  _lum = NULL;
  _cb  = NULL;
  _cr  = NULL;

  /// Cache blocks and overlays.
  _plumCache  = NULL;
  _lum16x16   = NULL;
  _pCbCache   = NULL;
  _cb8x8      = NULL;
  _pCrCache   = NULL;
  _cr8x8      = NULL;

}//end constructor.

H264MbImgCache::~H264MbImgCache(void)
{
	Destroy();
}//end destructor.

void H264MbImgCache::Destroy(void)
{
	if(_lum16x16 != NULL)
		delete _lum16x16;
	_lum16x16 = NULL;
	if(_plumCache != NULL)
		delete[] _plumCache;
	_plumCache = NULL;

	if(_cb8x8 != NULL)
		delete _cb8x8;
	_cb8x8 = NULL;
	if(_pCbCache != NULL)
		delete[] _pCbCache;
	_pCbCache = NULL;

	if(_cr8x8 != NULL)
		delete _cr8x8;
	_cr8x8 = NULL;
	if(_pCrCache != NULL)
		delete[] _pCrCache;
	_pCrCache = NULL;

}//end Destroy.

/*
---------------------------------------------------------------------------
	Interface Methods.
---------------------------------------------------------------------------
*/
/** Create mem container.
Set the mem for a 16x16 lum and two 8x8 chr blocks with their overlays.
@return			: 1 = success, 0 = failed.
*/
int H264MbImgCache::Create(void)
{
  /// Clean out before starting.
  Destroy();

	_plumCache = new short[256];	///< 16x16 block.
	_lum16x16	= new OverlayMem2Dv2(_plumCache, 16, 16, 16, 16);
	_pCbCache = new short[64];	///< 8x8 blocks for predicition operations.
	_cb8x8	= new OverlayMem2Dv2(_pCbCache, 8, 8, 8, 8);
	_pCrCache = new short[64];
	_cr8x8	= new OverlayMem2Dv2(_pCrCache, 8, 8, 8, 8);

  if( (_plumCache == NULL)||(_lum16x16 == NULL) ||																
			(_pCbCache == NULL) ||(_cb8x8 == NULL) || 
			(_pCrCache == NULL) ||(_cr8x8 == NULL) )
  {
    Destroy();
	  return(0);
  }//end if !_plumCache...

  return(1);
}//end Create.

/** Cache the mb aligned image.
Use the mb data to align the mb with the image position and copy the image
data into the cache image. NOTE: No mem checking is done so SetImage() must 
have been called before this method can be used.
@param  pMb : Mb to cache.
@return			: 1 = success, 0 = failed.
*/
int H264MbImgCache::Cache(MacroBlockH264* pMb)
{
  /// Align the Lum img block with this mb position
	_lum->SetOverlayDim(16, 16);
	_lum->SetOrigin(pMb->_offLumX, pMb->_offLumY);
  /// Read from the Lum img block into the mb cache.
  _lum->Read(*(_lum16x16));
  
  /// Align the Cb img block with this mb position
	_cb->SetOverlayDim(8, 8);
	_cb->SetOrigin(pMb->_offChrX, pMb->_offChrY);
  /// Read from the Cb img block into the mb cache.
  _cb->Read(*(_cb8x8));

  /// Align the Cr img block with this mb position
	_cr->SetOverlayDim(8, 8);
	_cr->SetOrigin(pMb->_offChrX, pMb->_offChrY);
  /// Read from the Cr img block into the mb cache.
  _cr->Read(*(_cr8x8));

  return(1);
}//end Cache.

/** Restore the mb aligned image from the cache.
Use the mb data to align the mb with the image position and copy the cache
data into the image. NOTE: No mem checking is done so SetImage() must 
have been called before this method can be used.
@param  pMb : Mb to restore.
@return			: 1 = success, 0 = failed.
*/
int H264MbImgCache::Restore(MacroBlockH264* pMb)
{
  /// Align the Lum img block with this mb position
	_lum->SetOverlayDim(16, 16);
	_lum->SetOrigin(pMb->_offLumX, pMb->_offLumY);
  /// Read from the mb cache into the Lum img block .
  _lum16x16->Read(*(_lum));
  
  /// Align the Cb img block with this mb position
	_cb->SetOverlayDim(8, 8);
	_cb->SetOrigin(pMb->_offChrX, pMb->_offChrY);
  /// Read from the mb cache into the Cb img block .
  _cb8x8->Read(*(_cb));

  /// Align the Cr img block with this mb position
	_cr->SetOverlayDim(8, 8);
	_cr->SetOrigin(pMb->_offChrX, pMb->_offChrY);
  /// Read from the mb cache into the Cr img block .
  _cr8x8->Read(*(_cr));

  return(1);
}//end Restore.

/** Test if the cache is equal to the image block.
@param  pMb : Mb position to compare.
@return     : 1 = Equal, 0 = Not equal.
*/
int H264MbImgCache::Equal(MacroBlockH264* pMb)
{
  int lumEqual  = 0;
  int cbEqual   = 0;
  int crEqual   = 0;

  /// Align the Lum img block with this mb position
	_lum->SetOverlayDim(16, 16);
	_lum->SetOrigin(pMb->_offLumX, pMb->_offLumY);
  lumEqual = _lum16x16->Equals(*(_lum));
  
  /// Align the Cb img block with this mb position
	_cb->SetOverlayDim(8, 8);
	_cb->SetOrigin(pMb->_offChrX, pMb->_offChrY);
  cbEqual = _cb8x8->Equals(*(_cb));

  /// Align the Cr img block with this mb position
	_cr->SetOverlayDim(8, 8);
	_cr->SetOrigin(pMb->_offChrX, pMb->_offChrY);
  crEqual = _cr8x8->Equals(*(_cr));

  return(lumEqual & cbEqual & crEqual);
}//end Equal.
