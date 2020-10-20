/** @file

MODULE						: VectorQuantiserVer2Dim16Impl1

TAG								: VQV2D16I1

FILE NAME					: VectorQuantiserVer2Dim16Impl1.cpp

DESCRIPTION				: A class to implement the IVectorQuantiserVer2 interface for video 
										4x4 (dim = 16) dimension values. A fast implementation with early 
                    exit short cuts.

REVISION HISTORY	:
									: 

COPYRIGHT					: (c)CSIR 2013-2014  all rights resevered 

RESTRICTIONS			: The information/data/code contained within this file is 
										the property of VICS limited and has been classified as 
										CONFIDENTIAL.
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

#include "VectorQuantiserVer2Dim16Impl1.h"

/*
---------------------------------------------------------------------------
	Public Interface.
---------------------------------------------------------------------------
*/
/** Inverse quantise the index.
Return a pointer to the code book vector referenced by the input
index.
@param index	: Code book vector index.
@return				: Reference to the code book vector.
*/
const void* VectorQuantiserVer2Dim16Impl1::InverseQuantise(int index, IVQCodebookDim16* codebook)
{
	if(index < codebook->GetCodeBookLength())
		return( (void *)( &((codebook->GetCodeBook())[index][0]) ) );
	return(NULL);
}//end InverseQuantise.

/** Quantise the input vector.
Match the input vector with the implementation code book element
that produces the smallest distortion.
@param vector			:	Input vector to match in the code book.
@param distortion	:	Distortion of the winning vector.
@return						: Winning vector index.
*/
int VectorQuantiserVer2Dim16Impl1::Quantise(const void* vector, int* distortion, IVQCodebookDim16* codebook)
{
  int book;
  int tableLen = codebook->GetCodeBookLength();
  int	best_dist	= 0x0FFFFFFF;
  int	best_index	= 0;

  for(book = 0; book < tableLen; book++)
  {
		int	x,i,j;
    int						dist	= 0;
    const short*	pC		= (codebook->GetCodeBook())[book];
		const short*	pV		= (const short *)vector;

    for(i = 0; i < 4; i++)  ///< Batches of 4 partial sums.
    {
      for(j = 0; j < 4; j++)  ///< 4 per partial sum.
      {
		    x	= *(pV++) - *(pC++);
		    dist += (x*x);
      }//end for j...
		  if(dist >= best_dist)
			  goto VQV2D16I1_early_terminate;
    }//end for i...

    if(dist < best_dist)  ///< This test may not be necessary.
    {
      best_dist		= dist;
      best_index	= book;
    }//end if dist...

		VQV2D16I1_early_terminate: ;//null

  }//end for book...

  *distortion = best_dist;
  return(best_index);
}//end Quantise.

											

												