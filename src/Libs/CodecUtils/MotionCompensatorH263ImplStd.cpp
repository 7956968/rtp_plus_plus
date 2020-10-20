/** @file

MODULE						: MotionCompensatorH263ImplStd

TAG								: MCH263IS

FILE NAME					: MotionCompensatorH263ImplStd.h

DESCRIPTION				: A standard motion compensator implementation for boundary
										limited H.263 of YUV420 planar image reference.	Access via 
										an IMotionCompensator	interface.

REVISION HISTORY	:	

COPYRIGHT					: 

RESTRICTIONS			: 
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


#include	"MotionCompensatorH263ImplStd.h"

/*
--------------------------------------------------------------------------
  Construction. 
--------------------------------------------------------------------------
*/

MotionCompensatorH263ImplStd::MotionCompensatorH263ImplStd(void)
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
	_refSize						= 0;
  _invalid            = 0;      // Invalidate the compensation to force copying on the next compensated vector.

	_pRefLum						= NULL;		// References to the images at creation.
	_pRefChrU						= NULL;
	_pRefChrV						= NULL;

	_pTmpLum						= NULL;		// Tmp space to use for compensation.
	_pTmpChrU						= NULL;
	_pTmpChrV						= NULL;

	_pRefLumOver				= NULL;		// Overlays.
	_pRefChrUOver				= NULL;
	_pRefChrVOver				= NULL;
	_pTmpLumOver				= NULL;
	_pTmpChrUOver				= NULL;
	_pTmpChrVOver				= NULL;

	_pMBlk							= NULL;
	_pMBlkOver					= NULL;

}//end constructor.

MotionCompensatorH263ImplStd::~MotionCompensatorH263ImplStd(void)
{
	Destroy();
}//end destructor.

/*
--------------------------------------------------------------------------
  Public IMotionCompensator Interface. 
--------------------------------------------------------------------------
*/

int MotionCompensatorH263ImplStd::Create(void* ref, int imgWidth,			 int imgHeight, 
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
	_pRefLum = (mcisType *)ref;
	_pRefChrU = &_pRefLum[_imgWidth * _imgHeight];
	_pRefChrV = &_pRefLum[(_imgWidth * _imgHeight) + (_chrWidth * _chrHeight)];

	// Construct a temp mem set.
	_pTmpLum = new mcisType[_refSize];
	if(_pTmpLum == NULL)
	{
		Destroy();
		return(0);
	}//end if !_pTmpLum...
	_pTmpChrU = &_pTmpLum[_imgWidth * _imgHeight];
	_pTmpChrV = &_pTmpLum[(_imgWidth * _imgHeight) + (_chrWidth * _chrHeight)];

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

	// --------------- Configure temp overlays --------------------------------
	// Overlay the temp and set to the motion block size.  
	_pTmpLumOver	= new OverlayMem2Dv2( (void *)_pTmpLum, 
																		_imgWidth, 
																		_imgHeight, 
																		_macroBlkWidth, 
																		_macroBlkHeight );
	_pTmpChrUOver	= new OverlayMem2Dv2( (void *)_pTmpChrU, 
																		_chrWidth, 
																		_chrHeight, 
																		_chrMacroBlkWidth, 
																		_chrMacroBlkHeight );
	_pTmpChrVOver	= new OverlayMem2Dv2( (void *)_pTmpChrV, 
																		_chrWidth, 
																		_chrHeight, 
																		_chrMacroBlkWidth, 
																		_chrMacroBlkHeight );
	if( (_pTmpLumOver == NULL)||(_pTmpChrUOver == NULL)||(_pTmpChrVOver == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pRefLumOver...

	// Alloc some temp mem and overlay it to use for half pel motion compensation. 
	// The block size is the same as the mem size.
	_pMBlk = new mcisType[_macroBlkWidth * _macroBlkHeight];
	_pMBlkOver = new OverlayMem2Dv2(_pMBlk, _macroBlkWidth, _macroBlkHeight, 
																				_macroBlkWidth, _macroBlkHeight);
	if( (_pMBlk == NULL)||(_pMBlkOver == NULL) )
  {
		Destroy();
	  return(0);
  }//end if !_pMBlk...

	return(1);
}//end Create.

void	MotionCompensatorH263ImplStd::Reset(void)
{
	memset((void *)_pRefLum, 0, _refSize * sizeof(mcisType));
}//end Reset.

void MotionCompensatorH263ImplStd::Destroy(void)
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

	if(_pTmpLumOver != NULL)
		delete _pTmpLumOver;
	_pTmpLumOver = NULL;

	if(_pTmpChrUOver != NULL)
		delete _pTmpChrUOver;
	_pTmpChrUOver = NULL;

	if(_pTmpChrVOver != NULL)
		delete _pTmpChrVOver;
	_pTmpChrVOver = NULL;

	if(_pTmpLum	!= NULL)
		delete[] _pTmpLum;
	_pTmpLum		= NULL;
	_pTmpChrU		= NULL;
	_pTmpChrV		= NULL;

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
void MotionCompensatorH263ImplStd::Compensate(void* pMotionList)
{
	// Get the motion vector list to work with. Assume SIMPLE2D type list.
	VectorStructList* pL			= (VectorStructList *)pMotionList;
	int								listLen = pL->GetLength();
	int								vecPos  = 0;

	// None to compensate or wrong type.
	if( (listLen == 0)||(pL->GetType() != VectorStructList::SIMPLE2D) )
		return;

  // Dump the entire ref into the tmp so that vectors can be taken 
  // from the tmp and written back to the ref. Do nothing for zero 
	// vectors. Note the use of contiguous mem to include Chr components.
	memcpy(_pTmpLum, _pRefLum, _refSize* sizeof(mcisType));

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
void MotionCompensatorH263ImplStd::PrepareForSingleVectorMode(void)
{
  // Dump the entire ref into the tmp so that vectors can be taken 
  // from the tmp and written back to the ref. Note the use of 
	// contiguous mem to include Chr components.
	memcpy(_pTmpLum, _pRefLum, _refSize* sizeof(mcisType));
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
void MotionCompensatorH263ImplStd::Compensate(int tlx, int tly, int mvx, int mvy)
{
	// Don't bother if the vector is zero unless invalidated.
	if( mvx || mvy  || _invalid )
  {
    // Lum first.
    _pMBlkOver->SetOverlayDim(16,16);

    int motion_x			= mvx / 2;
    int motion_y			= mvy / 2;
    int half_motion_x	= mvx % 2;
    int half_motion_y	= mvy % 2;

 		// Position the overlays. The vector is always assumed to fall within
		// the img bounds and relies on the estimation process not to generate
		// invalid vectors.
		_pRefLumOver->SetOrigin(tlx, tly);
		_pTmpLumOver->SetOrigin(tlx+motion_x, tly+ motion_y);
		if( !half_motion_x && !half_motion_y )	// No half pel implies straight copy.
			_pRefLumOver->Write(*_pTmpLumOver);
		else
		{
			// Read the compensated block into a work area.
			_pTmpLumOver->HalfRead(*_pMBlkOver, half_motion_x, half_motion_y);
			// Write it to the ref.
			_pRefLumOver->Write(*_pMBlkOver);
		}//end else...

    // Chr second.
    _pMBlkOver->SetOverlayDim(8,8);

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
		// the img bounds and relies on the estimation process not to generate
		// invalid vectors.
		_pRefChrUOver->SetOrigin(offvecx, offvecy);
		_pRefChrVOver->SetOrigin(offvecx, offvecy);
		_pTmpChrUOver->SetOrigin(offvecx+motion_x, offvecy+ motion_y);
		_pTmpChrVOver->SetOrigin(offvecx+motion_x, offvecy+ motion_y);
		if( !half_motion_x && !half_motion_y )	// No half pel implies straight copy.
		{
			_pRefChrUOver->Write(*_pTmpChrUOver);
			_pRefChrVOver->Write(*_pTmpChrVOver);
		}//end if !...
		else
		{
			// Read the compensated block into a work area and write it to the ref.
			_pTmpChrUOver->HalfRead(*_pMBlkOver, half_motion_x, half_motion_y);
			_pRefChrUOver->Write(*_pMBlkOver);
			_pTmpChrVOver->HalfRead(*_pMBlkOver, half_motion_x, half_motion_y);
			_pRefChrVOver->Write(*_pMBlkOver);
		}//end else...

    /// Reset the invalidation.
    _invalid = 0;
  }//end if mvx...

}//end Compensate.

/*
--------------------------------------------------------------------------------------
	Redundant code.
--------------------------------------------------------------------------------------
*/
/*
void MotionCompensatorH263ImplStd::Compensate(int tlx, int tly, int mvx, int mvy)
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
