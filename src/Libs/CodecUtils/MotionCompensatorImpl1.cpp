/** @file

MODULE						: MotionCompensatorImpl1

TAG								: MCI1

FILE NAME					: MotionCompensatorImpl1.h

DESCRIPTION				: A standard motion estimator implementation of squared
										error distortion measure.	Access via a IMotionEstimator
										interface.

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


#include "MotionCompensatorImpl1.h"

/*
--------------------------------------------------------------------------
  Construction. 
--------------------------------------------------------------------------
*/

MotionCompensatorImpl1::MotionCompensatorImpl1(void)
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

	_pRefLum						= NULL;		// References to the images at creation.
	_refLum							= NULL;		// Address array.
	_pRefChrU						= NULL;
	_refChrU						= NULL;
	_pRefChrV						= NULL;
	_refChrV						= NULL;

	_pTmpLum						= NULL;
	_tmpLum							= NULL;
	_pTmpChrU						= NULL;
	_tmpChrU						= NULL;
	_pTmpChrV						= NULL;
	_tmpChrV						= NULL;
}//end constructor.

MotionCompensatorImpl1::~MotionCompensatorImpl1(void)
{
	Destroy();
}//end destructor.

/*
--------------------------------------------------------------------------
  Public IMotionCompensator Interface. 
--------------------------------------------------------------------------
*/

int MotionCompensatorImpl1::Create(void* ref, int imgWidth,				int imgHeight, 
																							int macroBlkWidth,	int macroBlkHeight)
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
	_pRefLum = (mciType *)ref;
	_pRefChrU = &_pRefLum[_imgWidth * _imgHeight];
	_pRefChrV = &_pRefLum[(_imgWidth * _imgHeight) + (_chrWidth * _chrHeight)];

	// Address arrays for fast row access.
	_refLum = new mciType* [_imgHeight + 2*_chrHeight];
	if(_refLum == NULL)
	{
		Destroy();
		return(0);
	}//end if !_refLum...
	_refChrU = &_refLum[_imgHeight];
	_refChrV = &_refLum[_imgHeight + _chrHeight];

	// Construct a temp mem set.
	_pTmpLum = new mciType[_refSize];
	if(_pTmpLum == NULL)
	{
		Destroy();
		return(0);
	}//end if !_pTmpLum...
	_pTmpChrU = &_pTmpLum[_imgWidth * _imgHeight];
	_pTmpChrV = &_pTmpLum[(_imgWidth * _imgHeight) + (_chrWidth * _chrHeight)];

	// Address arrays for fast row access.
	_tmpLum = new mciType* [_imgHeight + 2*_chrHeight];
	if(_tmpLum == NULL)
	{
		Destroy();
		return(0);
	}//end if !_tmpLum...
	_tmpChrU = &_tmpLum[_imgHeight];
	_tmpChrV = &_tmpLum[_imgHeight + _chrHeight];

	// Set the addresses.
	for(int i = 0; i < _imgHeight; i++)
	{
		_refLum[i] = &_pRefLum[i * _imgWidth];
		_tmpLum[i] = &_pTmpLum[i * _imgWidth];
	}//end for i...
	for(int i = 0; i < _chrHeight; i++)
	{
		_refChrU[i] = &_pRefChrU[i * _chrWidth];
		_refChrV[i] = &_pRefChrV[i * _chrWidth];
		_tmpChrU[i] = &_pTmpChrU[i * _chrWidth];
		_tmpChrV[i] = &_pTmpChrV[i * _chrWidth];
	}//end for i...

	return(1);
}//end Create.

void	MotionCompensatorImpl1::Reset(void)
{
	memset((void *)_pRefLum, 0, _refSize * sizeof(mciType));
}//end Reset.

void MotionCompensatorImpl1::Destroy(void)
{
	if(_refLum != NULL)
		delete[] _refLum;
	_refLum			= NULL;
	_pRefLum		= NULL;
	_refChrU		= NULL;
	_pRefChrU		= NULL;
	_refChrV		= NULL;
	_pRefChrV		= NULL;

	if(_pTmpLum	!= NULL)
		delete[] _pTmpLum;
	_pTmpLum		= NULL;
	_pTmpChrU		= NULL;
	_pTmpChrV		= NULL;

	if(_tmpLum != NULL)
		delete[] _tmpLum;
	_tmpLum		= NULL;
	_tmpChrU	= NULL;
	_tmpChrV	= NULL;
}//end Destroy.

/** Motion compensate to the reference.
Do the compensation with the block sizes and image sizes defined in
the implementation and set in Create().
@param pMotionList	: The list of motion vectors.
@return							: None.
*/
void MotionCompensatorImpl1::Compensate(void* pMotionList)
{
  int x,y,m,n;

	// Get the motion vector list to work with. Assume SIMPLE type list.
	VectorList* pL = (VectorList *)pMotionList;
	char* pMv			= (char *)(pL->GetDataPtr());
	int		stride	= pL->GetPatternSize();
	int		listLen = pL->GetLength();

	// None to compensate or wrong type.
	if( (listLen == 0)||(pL->GetType() != VectorList::SIMPLE) )
		return;

  // Dump the entire ref into the tmp so that vectors can be taken 
  // from the tmp and written back to the ref. Do nothing for zero 
	// vectors.
	memcpy(_pTmpLum, _pRefLum, _refSize* sizeof(mciType));

  //int size	= Lum_Y * Lum_X * sizeof(VICS_SBYTE);
  //memcpy(crpLum, rpLum, size);
  //size					= Chr_Y * Chr_X * sizeof(VICS_SBYTE);
  //memcpy(crpChrU, rpChrU, size);
  //memcpy(crpChrV, rpChrV, size);

  // Do compensation in the sequence order from tmp to ref.
	int mvx			= 0;
	int mvy			= 0;
  for(m = 0; m < _imgHeight; m += _macroBlkHeight)
	  for(n = 0; n < _imgWidth; n += _macroBlkWidth)
  {
		mvx = (int)(*pMv);
		mvy = (int)(*(pMv+1));
		pMv += stride;

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
          int r_row = m + y;
          int c_row = m + y + motion_y;
          if(c_row < 0)
            c_row = 0;
          if(c_row >= _imgHeight)
            c_row = _imgHeight - 1;
          for(x = 0; x < _macroBlkWidth; x++)
          {
            int r_col = n + x;
            int c_col = n + x + motion_x;
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
          int r_row = m + y;
          int c_row1 = m + y + motion_y;
          int c_row2 = m + y + motion_y + half_motion_y;
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
            int r_col	= n + x;
            int c_col1 = n + x + motion_x;
            int c_col2 = n + x + motion_x + half_motion_x;
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
          int r_row	= m + y;
          int c_row1 = m + y + motion_y;
          int c_row2 = m + y + motion_y + half_motion_y;
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
            int r_col	= n + x;
            int c_col1 = n + x + motion_x;
            int c_col2 = n + x + motion_x + half_motion_x;
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
      int offvecx	= n/2;
      int offvecy	= m/2;
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

  }//end for m & n...

}//end Compensate.
