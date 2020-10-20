/** @file

MODULE						: VectorQuantiserBaseImpl1

TAG								: VQBI1

FILE NAME					: VectorQuantiserBaseImpl1.cpp

DESCRIPTION				: A base class to implement the IVectorQuantiser interface for video 
										4x4 (dim = 16) dimension values on colour components. The quantisation
                    and inverse quantisation processes are typically common across all
                    codebooks and are therefore grouped into this class.

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

#include "VectorQuantiserBaseImpl1.h"

/*
---------------------------------------------------------------------------
	Constructors and destructors.
---------------------------------------------------------------------------
*/
VectorQuantiserBaseImpl1::VectorQuantiserBaseImpl1(void)
{
  _qp = 24; ///< Arbitrary default.
  _pVQCodebook  = NULL;
  _pVQParam     = NULL;
}//end constuctor.

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
const void* VectorQuantiserBaseImpl1::InverseQuantise(int index)
{
	if(index < _pVQCodebook[_qp]->GetCodeBookLength())
		return( (void *)( &((_pVQCodebook[_qp]->GetCodeBook())[index][0]) ) );
	return(NULL);
}//end InverseQuantise.

/** Quantise the input vector.
Match the input vector with the implementation code book element
that produces the smallest distortion.
@param vector			:	Input vector to match in the code book.
@param distortion	:	Distortion of the winning vector.
@return						: Winning vector index.
*/

int VectorQuantiserBaseImpl1::Quantise(const void* vector, int* distortion)
{
  int i,j;

  /// Simplify addressing.
  IVQCodebookDim16* pCodebook = _pVQCodebook[_qp];
  const short	(*pCVec)[16]	  = pCodebook->GetCodeBook();
  int tableLen                = pCodebook->GetCodeBookLength();

  VQBI1_VECTOR_PARAMS_STRUCT*  pParams = _pVQParam[_qp];

  /// Find the pos of the zero vector in the ordered param list as a good starting pos.
  int	min_index;
  for(min_index = 0; min_index < tableLen; min_index++)
  {
    if(pParams[min_index].sum == 0)
      break;
  }//end for min_index...

  /// Collect the input vector params and dist with the zero vector.
	const short* pV	= (const short *)vector;
  int	min_dist    = 0;
  int sum         = 0;
  for(i = 0; i < 16; i++)
  {
    int k = (int)pV[i];
    sum      += k;
    min_dist += (k * k);
  }//end for i...
  int min_kd2 = 16 * min_dist;
  int rootk_min_kd2  = (int)(0.5 + sqrt((double)min_kd2));

  /// Variance of the input vector.
  double mean = (double)sum/16.0;
  double var  = 0.0;
  for(i = 0; i < 16; i++)
  {
    double diff = (double)pV[i] - mean;
    var += (diff*diff);
  }//end for i...
  var = sqrt(var);

  /// List is ordered in ascending order of sum values. Start the
  /// comparisons at the point in the list where the codebook vector
  /// sum = input sum - root (k.d2min).
  int startSum = sum - rootk_min_kd2;
  for(j = 0; j < tableLen; j++)
  {
    if(pParams[j].sum >= startSum)
      break;
  }//end for j...

  /// It is not necessary to continue checking the codebook vector
  /// after sum = input sum + root (k.d2min)
  int endSum = sum + rootk_min_kd2;
  for(; j < tableLen; j++)
  {
    /// Further address short cuts.
    VQBI1_VECTOR_PARAMS_STRUCT* param = &(pParams[j]);
    int lclSum = param->sum;

    /// Compare difference of sums squared.
    int sqrsum = sum - lclSum;
    sqrsum = (sqrsum * sqrsum);
    if(sqrsum < min_kd2)
    {
      /// Compare with difference of variance squared.
      double est_kd2 = var - param->var;
      est_kd2 = 16.0*(est_kd2 * est_kd2) + (double)sqrsum;
      if(est_kd2 < (double)min_kd2)
      {
        /// Test the actual dist.
		    int	x,y;
        int	dist	= 0;
        const short* pC	= pCVec[param->index];

        for(int m = 0; m < 16; m += 2)
        {
		      x	= pV[m] - pC[m]; 
          y = pV[m+1] - pC[m+1];
		      dist += ( (x*x) + (y*y) );
		      if(dist >= min_dist)
			      goto VQBI1_early_terminate;
        }//end for m...

        min_dist		  = dist;
        min_kd2       = 16 * dist;
        rootk_min_kd2 = (int)(0.5 + sqrt((double)min_kd2));
        min_index	    = j;

        VQBI1_early_terminate: ; ///< null
      }//end if est_kd2...
    }//end if sqrsum...

    if(lclSum >= endSum)
      break;
  }//end for j...
  
  *distortion = min_dist;
  return(pParams[min_index].index);
}//end Quantise.

/*
int VectorQuantiserBaseImpl1::Quantise(const void* vector, int* distortion)
{
  int vec,i;
  int tableLen    = _pVQCodebook[_qp]->GetCodeBookLength();
  int	best_dist   = 8192*8192;
  int best_ikd2   = 16 * best_dist;
  double best_kd2 = 16.0 * (double)best_dist;
  int	best_index	= 0;
  int cnt = 0;

  /// Collect the input vector params.
	const short* pV	= (const short *)vector;
  int sum = 0;
  for(i = 0; i < 16; sum += pV[i], i++);

  double mean = (double)sum/16.0;
  double var  = 0.0;
  for(i = 0; i < 16; i++)
  {
    double diff = (double)pV[i] - mean;
    var += (diff*diff);
  }//end for i...
  var = sqrt(var);

  for(vec = 0; vec < tableLen; vec++)
  {
    /// Step 1: Compare difference of sums squared.
    int sqrsum = sum - _pVQParam[_qp][vec].sum;
    sqrsum = (sqrsum * sqrsum);
    if(sqrsum < best_ikd2)
    {
      /// Step 2: Compare with difference of variance squared.
      double est_kd2 = var - _pVQParam[_qp][vec].var;
      est_kd2 = 16.0*(est_kd2 * est_kd2) + (double)sqrsum;
      if(est_kd2 < best_kd2)
      {
        /// Final stage is to test the actual dist.
		    int	x;
        int						dist	= 0;
        const short*	pC		= (_pVQCodebook[_qp]->GetCodeBook())[_pVQParam[_qp][vec].index];
		    pV		              = (const short *)vector;

        for(i = 0; i < 16; i++)
        {
          cnt++;
		      x	= *(pV++) - *(pC++);
		      dist += (x*x);
		      if(dist >= best_dist)
			      goto VQBI1_early_terminate;
        }//end for i...

        best_dist		= dist;
        best_ikd2   = 16 * dist;
        best_kd2    = 16.0 * (double)dist;
        best_index	= vec;

		    VQBI1_early_terminate: ;//null

      }//end if est_kd2: step 2...

    }//end if sqrsum: step 1...

  }//end for book...

  *distortion = best_dist;
  return(_pVQParam[_qp][best_index].index);
}//end Quantise.
*/
/* Full search algorithm.
int VectorQuantiserBaseImpl1::Quantise(const void* vector, int* distortion)
{
  int book;
  int tableLen = _pVQCodebook[_qp]->GetCodeBookLength();
  int	best_dist	= 0x0FFFFFFF;
  int	best_index	= 0;

  for(book = 0; book < tableLen; book++)
  {
		int	x,i,j;
    int						dist	= 0;
    const short*	pC		= (_pVQCodebook[_qp]->GetCodeBook())[book];
		const short*	pV		= (const short *)vector;

    for(i = 0; i < 4; i++)  ///< Batches of 4 partial sums.
    {
      for(j = 0; j < 4; j++)  ///< 4 per partial sum.
      {
		    x	= *(pV++) - *(pC++);
		    dist += (x*x);
      }//end for j...
		  if(dist >= best_dist)
			  goto VQBI1_early_terminate;
    }//end for i...

    if(dist < best_dist)  ///< This test may not be necessary.
    {
      best_dist		= dist;
      best_index	= book;
    }//end if dist...

		VQBI1_early_terminate: ;//null

  }//end for book...

  *distortion = best_dist;
  return(best_index);
}//end Quantise.
*/
											

												