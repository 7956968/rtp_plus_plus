/** @file

MODULE				: MotionCompensatorH264ImplMultires

TAG						: MCH264IM

FILE NAME			: MotionCompensatorH264ImplMultires.cpp

DESCRIPTION		: A fast unrestricted motion compensator implementation for 
								Recommendation H.264 (03/2005) with both absolute error 
								difference and square error measure. Access is via an 
								IMotionCompensator interface. There are 2 mode levels of 
								execution speed vs. quality. The boundary is extended to 
								accomodate the selected motion range.

COPYRIGHT			: (c)CSIR 2007-2010 all rights resevered

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
#ifdef _WINDOWS
#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#else
#include <stdio.h>
#endif

#include <memory.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include	"MotionCompensatorH264ImplMultires.h"

/// Boundary padding past the motion vector extremes. Required for calculating
/// sub-pixel interpolations. Only required at level 0 resolution.
#define MCH264IM_PADDING	3

/*
--------------------------------------------------------------------------
  Construction. 
--------------------------------------------------------------------------
*/

MotionCompensatorH264ImplMultires::MotionCompensatorH264ImplMultires(int range)
{
	// Parameters must remain const for the life time of this instantiation.
	_imgWidth						= 0;				// Width of the ref images. 
	_imgHeight					= 0;				// Height of the ref images.
	_chrWidth						= 0;
	_chrHeight					= 0;
	_macroBlkWidth			= 0;				// Width of the motion block.
	_macroBlkHeight			= 0;				// Height of the motion block.
	_chrMacroBlkWidth		= 0;
	_chrMacroBlkHeight	= 0;
	_range							= range;
	_refSize						= 0;

	_pRefLum						= NULL;		// References to the images at creation.
	_pRefChrU						= NULL;
	_pRefChrV						= NULL;

	_pRefLumOver				= NULL;		// Overlays.
	_pRefChrUOver				= NULL;
	_pRefChrVOver				= NULL;

	_pExtTmpLum					= NULL;		// Extended boundary temp images.
	_pExtTmpLumOver			= NULL;
	_extLumWidth				= 0;
	_extLumHeight				= 0;
	_pExtTmpChrU				= NULL;
	_pExtTmpChrUOver		= NULL;
	_pExtTmpChrV				= NULL;
	_pExtTmpChrVOver		= NULL;
	_extChrWidth				= 0;
	_extChrHeight				= 0;

	_pMBlk							= NULL;
	_pMBlkOver					= NULL;

}//end constructor.

MotionCompensatorH264ImplMultires::~MotionCompensatorH264ImplMultires(void)
{
	Destroy();
}//end destructor.

/*
--------------------------------------------------------------------------
  Public IMotionCompensator Interface. 
--------------------------------------------------------------------------
*/

int MotionCompensatorH264ImplMultires::Create(void* ref, 
																							int imgWidth,			 int imgHeight, 
																							int macroBlkWidth, int macroBlkHeight)
{
	// Clean out old mem.
	Destroy();

	if(ref == NULL)
		return(0);

	// Parameters must remain const for the life time of this instantiation.
	_imgWidth						= imgWidth;				// Width of the ref images. 
	_imgHeight					= imgHeight;			// Height of the ref images.
	_chrWidth						= imgWidth/2;			// YUV 420.
	_chrHeight					= imgHeight/2;
	_macroBlkWidth			= macroBlkWidth;	// Width of the motion block.
	_macroBlkHeight			= macroBlkHeight;	// Height of the motion block.
	_chrMacroBlkWidth		= macroBlkWidth/2;
	_chrMacroBlkHeight	= macroBlkHeight/2;
	_refSize						= (_imgWidth * _imgHeight) + 2*(_chrWidth * _chrHeight);

	// Assign the head of contiguous mem to the lum ref.
	_pRefLum = (short *)ref;
	_pRefChrU = &_pRefLum[_imgWidth * _imgHeight];
	_pRefChrV = &_pRefLum[(_imgWidth * _imgHeight) + (_chrWidth * _chrHeight)];

	// --------------- Configure ref overlays --------------------------------
	// Overlay the reference and set to the motion block size.  
	_pRefLumOver	= new OverlayMem2Dv2( (void *)_pRefLum, 
																		_imgWidth, 
																		_imgHeight, 
																		_macroBlkWidth, 
																		_macroBlkHeight );
	_pRefChrUOver	= new OverlayMem2Dv2( (void *)_pRefChrU, 
																		_chrWidth, 
																		_chrHeight, 
																		_chrMacroBlkWidth, 
																		_chrMacroBlkHeight );
	_pRefChrVOver	= new OverlayMem2Dv2( (void *)_pRefChrV, 
																		_chrWidth, 
																		_chrHeight, 
																		_chrMacroBlkWidth, 
																		_chrMacroBlkHeight );
	if( (_pRefLumOver == NULL)||(_pRefChrUOver == NULL)||(_pRefChrVOver == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pRefLumOver...

	// --------------- Create extended boundary mem ---------------------------
	// Create the new extended boundary temp ref into _pExtTmpLum, _pExtTmpChrU
	// and _pExtTmpChrV. These are required before placing overlays on them.

	if(!OverlayExtMem2Dv2::ExtendBoundary((void *)_pRefLum, 
																			_imgWidth,						
																			_imgHeight, 
																			_range + 1 + MCH264IM_PADDING,	///< Extend left and right by...
																			_range + 1 + MCH264IM_PADDING,	///< Extend top and bottom by...
																			(void **)(&_pExtTmpLum)) )			///< Created in the method.
  {
		Destroy();
	  return(0);
  }//end if !ExtendBoundary...
	_extLumWidth	= _imgWidth + (_range + 1 + MCH264IM_PADDING)*2;
	_extLumHeight	= _imgHeight + (_range + 1 + MCH264IM_PADDING)*2;

	if(!OverlayExtMem2Dv2::ExtendBoundary((void *)_pRefChrU, 
																			_chrWidth,						
																			_chrHeight, 
																			(_range/2) + 2, // Extend left and right by...
																			(_range/2) + 2,	// Extend top and bottom by...
																			(void **)(&_pExtTmpChrU)) )	// Created in the method.
  {
		Destroy();
	  return(0);
  }//end if !ExtendBoundary...
	if(!OverlayExtMem2Dv2::ExtendBoundary((void *)_pRefChrV, 
																			_chrWidth,						
																			_chrHeight, 
																			(_range/2) + 2, // Extend left and right by...
																			(_range/2) + 2,	// Extend top and bottom by...
																			(void **)(&_pExtTmpChrV)) )	// Created in the method.
  {
		Destroy();
	  return(0);
  }//end if !ExtendBoundary...
	_extChrWidth	= _chrWidth + ((_range/2) + 2)*2;
	_extChrHeight	= _chrHeight + ((_range/2) + 2)*2;

	// --------------- Configure temp extended ref overlays -------------------------
	// Overlay the extended temp ref and set to the whole image block size. In use 
	// call the SetOverlayDim() method to switch the block size.

	_pExtTmpLumOver = new OverlayExtMem2Dv2(_pExtTmpLum, 
																				_extLumWidth,		// new mem width.
																				_extLumHeight,	// new mem height.
																				_imgWidth,			// block width.
																				_imgHeight,			// block height.
																				_range + 1 + MCH264IM_PADDING,			// boundary width.
																				_range + 1 + MCH264IM_PADDING);		// boundary height.

	_pExtTmpChrUOver = new OverlayExtMem2Dv2(	_pExtTmpChrU, 
																					_extChrWidth,		// new mem width.
																					_extChrHeight,	// new mem height.
																					_chrWidth,			// block width.
																					_chrHeight,			// block height.
																					(_range/2) + 2,	// boundary width.
																					(_range/2) + 2);// boundary height.

	_pExtTmpChrVOver = new OverlayExtMem2Dv2(	_pExtTmpChrV, 
																					_extChrWidth,		// new mem width.
																					_extChrHeight,	// new mem height.
																					_chrWidth,			// block width.
																					_chrHeight,			// block height.
																					(_range/2) + 2,	// boundary width.
																					(_range/2) + 2);// boundary height.

	if( (_pExtTmpLumOver == NULL)||(_pExtTmpChrUOver == NULL)||(_pExtTmpChrVOver == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pRefLumOver...

	// Alloc some temp mem and overlay it to use for half pel motion compensation. 
	// The block size is the same as the mem size.
	_pMBlk = new short[_macroBlkWidth * _macroBlkHeight];
	_pMBlkOver = new OverlayMem2Dv2(_pMBlk, _macroBlkWidth, _macroBlkHeight, 
																				_macroBlkWidth, _macroBlkHeight);
	if( (_pMBlk == NULL)||(_pMBlkOver == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pMBlk...

	return(1);
}//end Create.

void	MotionCompensatorH264ImplMultires::Reset(void)
{
	memset((void *)_pRefLum, 0, _refSize * sizeof(short));
}//end Reset.

void MotionCompensatorH264ImplMultires::Destroy(void)
{
	if(_pRefLumOver != NULL)
		delete _pRefLumOver;
	_pRefLumOver = NULL;

	if(_pRefChrUOver != NULL)
		delete _pRefChrUOver;
	_pRefChrUOver = NULL;

	if(_pRefChrVOver != NULL)
		delete _pRefChrVOver;
	_pRefChrVOver = NULL;

	_pRefLum		= NULL;
	_pRefChrU		= NULL;
	_pRefChrV		= NULL;

	// Delete extended temp overlays before the underlying mem.
	if(_pExtTmpLumOver != NULL)
		delete _pExtTmpLumOver;
	_pExtTmpLumOver = NULL;
	if(_pExtTmpChrUOver != NULL)
		delete _pExtTmpChrUOver;
	_pExtTmpChrUOver = NULL;
	if(_pExtTmpChrVOver != NULL)
		delete _pExtTmpChrVOver;
	_pExtTmpChrVOver = NULL;

	// Delete extended temp ref mem.
	if(_pExtTmpLum != NULL)
		delete[] _pExtTmpLum;
	_pExtTmpLum = NULL;
	if(_pExtTmpChrU != NULL)
		delete[] _pExtTmpChrU;
	_pExtTmpChrU = NULL;
	if(_pExtTmpChrV != NULL)
		delete[] _pExtTmpChrV;
	_pExtTmpChrV = NULL;

	if(_pMBlkOver != NULL)
		delete _pMBlkOver;
	_pMBlkOver = NULL;

	if(_pMBlk != NULL)
		delete[] _pMBlk;
	_pMBlk = NULL;
}//end Destroy.

/** Motion compensate to the reference.
Do the compensation with the block sizes and image sizes defined in
the implementation and set in Create().
@param pMotionList	: The list of motion vectors.
@return							: None.
*/
void MotionCompensatorH264ImplMultires::Compensate(void* pMotionList)
{
	// Get the motion vector list to work with. Assume SIMPLE2D type list.
	VectorStructList* pL			= (VectorStructList *)pMotionList;
	int								listLen = pL->GetLength();
	int								vecPos  = 0;

	// None to compensate or wrong type.
	if( (listLen == 0)||(pL->GetType() != VectorStructList::SIMPLE2D) )
		return;

  // Dump the entire ref into the tmp so that vectors can be taken 
  // from the tmp and written back to the ref. 
	PrepareForSingleVectorMode();

  // Do compensation in the sequence order from tmp to ref.
	int mvx	= 0;
	int mvy	= 0;
  for(int m = 0; m < _imgHeight; m += _macroBlkHeight)
	  for(int n = 0; n < _imgWidth; n += _macroBlkWidth)
  {
		mvx = pL->GetSimpleElement(vecPos, 0);
		mvy = pL->GetSimpleElement(vecPos, 1);
		vecPos++;

		Compensate(n, m, mvx, mvy);

  }//end for m & n...

}//end Compensate.

/** Prepare the ref for single motion vector compensation mode.
Should be used to copy the ref into a temp location from which to
do the compensation to the ref. Prevents interference and double
compensation.
@return : none.
*/
void MotionCompensatorH264ImplMultires::PrepareForSingleVectorMode(void)
{
	// Write the ref to the temp ref and fill its extended boundary. The 
	// centre part of _pExtTmpLumOver is copied from _pRefLumOver before 
	// filling the boundary.

	// Set pos and block size to whole image for temp and ref.
	_pExtTmpLumOver->SetOrigin(0, 0);
	_pExtTmpLumOver->SetOverlayDim(_imgWidth, _imgHeight);
	_pRefLumOver->SetOrigin(0, 0);
	_pRefLumOver->SetOverlayDim(_imgWidth, _imgHeight);
	// Fill and extend boundary.
	_pExtTmpLumOver->Write(*_pRefLumOver);
	_pExtTmpLumOver->FillBoundaryProxy();
	// Set block sizes back to the macroblock.
	_pExtTmpLumOver->SetOverlayDim(_macroBlkWidth, _macroBlkHeight);
	_pRefLumOver->SetOverlayDim(_macroBlkWidth, _macroBlkHeight);

	_pExtTmpChrUOver->SetOrigin(0, 0);
	_pExtTmpChrUOver->SetOverlayDim(_chrWidth, _chrHeight);
	_pRefChrUOver->SetOrigin(0, 0);
	_pRefChrUOver->SetOverlayDim(_chrWidth, _chrHeight);
	_pExtTmpChrUOver->Write(*_pRefChrUOver);
	_pExtTmpChrUOver->FillBoundaryProxy();
	_pExtTmpChrUOver->SetOverlayDim(_chrMacroBlkWidth, _chrMacroBlkHeight);
	_pRefChrUOver->SetOverlayDim(_chrMacroBlkWidth, _chrMacroBlkHeight);

	_pExtTmpChrVOver->SetOrigin(0, 0);
	_pExtTmpChrVOver->SetOverlayDim(_chrWidth, _chrHeight);
	_pRefChrVOver->SetOrigin(0, 0);
	_pRefChrVOver->SetOverlayDim(_chrWidth, _chrHeight);
	_pExtTmpChrVOver->Write(*_pRefChrVOver);
	_pExtTmpChrVOver->FillBoundaryProxy();
	_pExtTmpChrVOver->SetOverlayDim(_chrMacroBlkWidth, _chrMacroBlkHeight);
	_pRefChrVOver->SetOverlayDim(_chrMacroBlkWidth, _chrMacroBlkHeight);

}//end PrepareForSingleVectorMode.

/** Motion compensate a single vector to the reference.
Do the compensation with the block sizes and image sizes defined in
the implementation and set in Create(). The vector coords are in 
half pel units for this implementation. NOTE: The temp image must
hold a copy of the ref BEFORE using this method and is should be done
in PrepareForSingleVectorMode().
@param tlx	: Top left x coord of block.
@param tly	: Top left y coord of block.
@param mvx	: X coord of the motion vector.
@param mvy	: Y coord of the motion vector.
@return			: None.
*/
void MotionCompensatorH264ImplMultires::Compensate(int tlx, int tly, int mvx, int mvy)
{
	// Don't bother if the vector is zero.
	if( mvx || mvy )
  {
    // Lum first.
    int motion_x			= mvx / 2;
    int motion_y			= mvy / 2;
    int half_motion_x	= mvx % 2;
    int half_motion_y	= mvy % 2;

 		// Position the overlays. The vector is always assumed to fall within
		// the extended img bounds and relies on the range not to generate
		// invalid vectors.
		_pRefLumOver->SetOrigin(tlx, tly);
		_pExtTmpLumOver->SetOrigin(tlx+motion_x, tly+ motion_y);
		if( !half_motion_x && !half_motion_y )	// No half pel implies straight copy.
			_pRefLumOver->Write(*_pExtTmpLumOver);
		else
		{
			// Read the compensated block into a work area.
			_pExtTmpLumOver->HalfReadv2(*_pMBlkOver, half_motion_x, half_motion_y);
			// Write it to the ref.
			_pRefLumOver->Write(*_pMBlkOver);
		}//end else...

    // Chr second.
    int offvecx	= tlx/2;
    int offvecy	= tly/2;
		// Quarter pel is rounded to half pel.
    half_motion_x	= mvx % 4;
		if(half_motion_x < 0)
			half_motion_x = -1;
		else if(half_motion_x > 0)
			half_motion_x = 1;
    half_motion_y	= mvy % 4;
		if(half_motion_y < 0)
			half_motion_y = -1;
		else if(half_motion_y > 0)
			half_motion_y = 1;
    motion_x = mvx / 4;
    motion_y = mvy / 4;

 		// Position the overlays. The vector is always assumed to fall within
		// the extended img bounds and relies on the range not to generate
		// invalid vectors.
		_pRefChrUOver->SetOrigin(offvecx, offvecy);
		_pRefChrVOver->SetOrigin(offvecx, offvecy);
		_pExtTmpChrUOver->SetOrigin(offvecx+motion_x, offvecy+ motion_y);
		_pExtTmpChrVOver->SetOrigin(offvecx+motion_x, offvecy+ motion_y);
		if( !half_motion_x && !half_motion_y )	// No half pel implies straight copy.
		{
			_pRefChrUOver->Write(*_pExtTmpChrUOver);
			_pRefChrVOver->Write(*_pExtTmpChrVOver);
		}//end if !...
		else
		{
			/// Read the compensated block into a work area and write it to the ref.
			_pExtTmpChrUOver->HalfRead(*_pMBlkOver, half_motion_x, half_motion_y);
			_pRefChrUOver->Write(*_pMBlkOver);
			_pExtTmpChrVOver->HalfRead(*_pMBlkOver, half_motion_x, half_motion_y);
			_pRefChrVOver->Write(*_pMBlkOver);
		}//end else...
  }//end if mvx...

}//end Compensate.

/*
--------------------------------------------------------------------------------------
	Redundant code.
--------------------------------------------------------------------------------------
*/
/*
void MotionCompensatorH264ImplMultires::Compensate(int tlx, int tly, int mvx, int mvy)
{
	int x,y;

	// Extract vector and copy.
	if( mvx || mvy )
  {
    // Lum first.
    int motion_x			= mvx / 2;
    int motion_y			= mvy / 2;
    int half_motion_x	= mvx % 2;
    int half_motion_y	= mvy % 2;

   // Treat the half pel conditions differently.
    if(!half_motion_y && !half_motion_x) // Both zero.
    {
      for(y = 0; y < _macroBlkHeight; y++)
      {
        int r_row = tly + y;
        int c_row = tly + y + motion_y;
        if(c_row < 0)
          c_row = 0;
        if(c_row >= _imgHeight)
          c_row = _imgHeight - 1;
        for(x = 0; x < _macroBlkWidth; x++)
        {
          int r_col = tlx + x;
          int c_col = tlx + x + motion_x;
          if(c_col < 0)
            c_col = 0;
          if(c_col >= _imgWidth)
            c_col = _imgWidth - 1;
          _refLum[r_row][r_col] = _tmpLum[c_row][c_col];
        }//end for x...
      }//end for y...
    }//end if half_motion_y...
    else if(half_motion_y && half_motion_x) // Diagonals.
    {
      for(y = 0; y < _macroBlkHeight; y++)
      {
        int r_row = tly + y;
        int c_row1 = tly + y + motion_y;
        int c_row2 = tly + y + motion_y + half_motion_y;
        if(c_row1 < 0)
          c_row1 = 0;
        if(c_row1 >= _imgHeight)
          c_row1 = _imgHeight - 1;
        if(c_row2 < 0)
          c_row2 = 0;
        if(c_row2 >= _imgHeight)
          c_row2 = _imgHeight - 1;
        for(x = 0; x < _macroBlkWidth; x++)
        {
          int r_col	= tlx + x;
          int c_col1 = tlx + x + motion_x;
          int c_col2 = tlx + x + motion_x + half_motion_x;
          if(c_col1 < 0)
            c_col1 = 0;
          if(c_col1 >= _imgWidth)
            c_col1 = _imgWidth - 1;
          if(c_col2 < 0)
            c_col2 = 0;
          if(c_col2 >= _imgWidth)
            c_col2 = _imgWidth - 1;
          _refLum[r_row][r_col] = (_tmpLum[c_row1][c_col1] + _tmpLum[c_row1][c_col2] +
																 _tmpLum[c_row2][c_col1] + _tmpLum[c_row2][c_col2] + 2)/4;
        }//end for x...
      }//end for y...
    }//end else if half_motion_y...
    else // Linear motion.
    {
      for(y = 0; y < _macroBlkHeight; y++)
      {
        int r_row	= tly + y;
        int c_row1 = tly + y + motion_y;
        int c_row2 = tly + y + motion_y + half_motion_y;
        if(c_row1 < 0)
          c_row1 = 0;
        if(c_row1 >= _imgHeight)
          c_row1 = _imgHeight - 1;
        if(c_row2 < 0)
          c_row2 = 0;
        if(c_row2 >= _imgHeight)
          c_row2 = _imgHeight - 1;
        for(x = 0; x < _macroBlkWidth; x++)
        {
          int r_col	= tlx + x;
          int c_col1 = tlx + x + motion_x;
          int c_col2 = tlx + x + motion_x + half_motion_x;
          if(c_col1 < 0)
            c_col1 = 0;
          if(c_col1 >= _imgWidth)
            c_col1 = _imgWidth - 1;
          if(c_col2 < 0)
            c_col2 = 0;
          if(c_col2 >= _imgWidth)
            c_col2 = _imgWidth - 1;
          _refLum[r_row][r_col] = (_tmpLum[c_row1][c_col1] + _tmpLum[c_row2][c_col2] + 1)/2;
        }//end for x...
      }//end for y...
    }//end else...

    // Chr second.
    int offvecx	= tlx/2;
    int offvecy	= tly/2;
    half_motion_x			= motion_x % 2;
    half_motion_y			= motion_y % 2;
    motion_x					= motion_x / 2;
    motion_y					= motion_y / 2;
    // Treat the half pel conditions differently.
    if(!half_motion_y && !half_motion_x) // Both zero.
    {
      for(y = 0; y < _chrMacroBlkHeight; y++)
      {
        int r_row = offvecy + y;
        int c_row = offvecy + y + motion_y;
        if(c_row < 0)
          c_row = 0;
        if(c_row >= _chrHeight)
          c_row = _chrHeight - 1;
        for(x = 0; x < _chrMacroBlkWidth; x++)
        {
          int r_col = offvecx + x;
          int c_col = offvecx + x + motion_x;
          if(c_col < 0)
            c_col = 0;
          if(c_col >= _chrWidth)
            c_col = _chrWidth - 1;
          _refChrU[r_row][r_col] = _tmpChrU[c_row][c_col];
          _refChrV[r_row][r_col] = _tmpChrV[c_row][c_col];
        }//end for x...
      }//end for y...
    }//end if half_motion_y...
    else if(half_motion_y && half_motion_x) // Diagonals.
    {
      for(y = 0; y < _chrMacroBlkHeight; y++)
      {
        int r_row = offvecy + y;
        int c_row1 = offvecy + y + motion_y;
        int c_row2 = offvecy + y + motion_y + half_motion_y;
        if(c_row1 < 0)
          c_row1 = 0;
        if(c_row1 >= _chrHeight)
          c_row1 = _chrHeight - 1;
        if(c_row2 < 0)
          c_row2 = 0;
        if(c_row2 >= _chrHeight)
          c_row2 = _chrHeight - 1;
        for(x = 0; x < _chrMacroBlkWidth; x++)
        {
          int r_col = offvecx + x;
          int c_col1 = offvecx + x + motion_x;
          int c_col2 = offvecx + x + motion_x + half_motion_x;
          if(c_col1 < 0)
            c_col1 = 0;
          if(c_col1 >= _chrWidth)
            c_col1 = _chrWidth - 1;
          if(c_col2 < 0)
            c_col2 = 0;
          if(c_col2 >= _chrWidth)
            c_col2 = _chrWidth - 1;
          _refChrU[r_row][r_col] = (_tmpChrU[c_row1][c_col1] + _tmpChrU[c_row1][c_col2] +
																	_tmpChrU[c_row2][c_col1] + _tmpChrU[c_row2][c_col2] + 2)/4;
          _refChrV[r_row][r_col] = (_tmpChrV[c_row1][c_col1] + _tmpChrV[c_row1][c_col2] +
																	_tmpChrV[c_row2][c_col1] + _tmpChrV[c_row2][c_col2] + 2)/4;
        }//end for x...
      }//end for y...
    }//end else if half_motion_y...
    else // Linear motion.
    {
      for(y = 0; y < _chrMacroBlkHeight; y++)
      {
        int r_row = offvecy + y;
        int c_row1 = offvecy + y + motion_y;
        int c_row2 = offvecy + y + motion_y + half_motion_y;
        if(c_row1 < 0)
          c_row1 = 0;
        if(c_row1 >= _chrHeight)
          c_row1 = _chrHeight - 1;
        if(c_row2 < 0)
          c_row2 = 0;
        if(c_row2 >= _chrHeight)
          c_row2 = _chrHeight - 1;
        for(x = 0; x < _chrMacroBlkWidth; x++)
        {
          int r_col = offvecx + x;
          int c_col1 = offvecx + x + motion_x;
          int c_col2 = offvecx + x + motion_x + half_motion_x;
          if(c_col1 < 0)
            c_col1 = 0;
          if(c_col1 >= _chrWidth)
            c_col1 = _chrWidth - 1;
          if(c_col2 < 0)
            c_col2 = 0;
          if(c_col2 >= _chrWidth)
            c_col2 = _chrWidth - 1;
          _refChrU[r_row][r_col] = (_tmpChrU[c_row1][c_col1] + _tmpChrU[c_row2][c_col2] + 1)/2;
          _refChrV[r_row][r_col] = (_tmpChrV[c_row1][c_col1] + _tmpChrV[c_row2][c_col2] + 1)/2;
        }//end for x...
      }//end for y...
    }//end else...
  }//end if mvx...

}//end Compensate.
*/