/** @file

MODULE						: VectorQuantiserCrImpl1

TAG								: VQCRI1

FILE NAME					: VectorQuantiserCrImpl1.cpp

DESCRIPTION				: A class to implement the IVectorQuantiser interface for video 
										4x4 (dim = 16) dimension values. Derived from the 
                    VectorQuantiserBaseImpl1() base class to implement the Cr 
                    colour component.

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

#include "VectorQuantiserCrImpl1.h"

#define VQCRI1_NUM_CODEBOOKS  52  ///< Maps to QP = {0..51}
/// VQ Codebooks
//#include "cr-qp4-dist0.41941242197638995-cbs1000-ivs35000.h"
//#include "cr-qp4-dist8.851771585402645-cbs1000-ivs48281.h"
#include "Cr-qp4-dist7.1177516524-cbs2000-ivs48281-RN20.h"

/*
---------------------------------------------------------------------------
	Constructors and destructors.
---------------------------------------------------------------------------
*/
VectorQuantiserCrImpl1::VectorQuantiserCrImpl1(void)
{
  VectorQuantiserBaseImpl1();
}//end constuctor.

VectorQuantiserCrImpl1::~VectorQuantiserCrImpl1(void)
{
  Destroy();  ///< Back up plan in case not called.
}//end destructor.

/** Create and destroy private mem objects.
Methods to alloc and delete memory that is best not
done in the constructor as failure to alloc cannot 
be reported. (Try-catch trap is possible).
@return	: 1 = success, 0 = failure.
*/
int VectorQuantiserCrImpl1::Create(void)  
{
  int i;

  /// Make sure all is cleared.
  Destroy();

  /// Create the codebook list.
	_pVQCodebook  = new IVQCodebookDim16*[VQCRI1_NUM_CODEBOOKS];
  if(_pVQCodebook == NULL)
    return(0);

  /// Load it up.
  for(i = 0; i < VQCRI1_NUM_CODEBOOKS; i++)
    _pVQCodebook[i] = (IVQCodebookDim16 *)(new CrQP4_VQCodebook); ///< All the same for testing
//  for(int i = 0; i < VQCRI1_NUM_CODEBOOKS; i++)
//    _pVQCodebook[i] = NULL;
//  _pVQCodebook[4] = (IVQCodebookDim16 *)(new CrQP4_VQCodebook);

  /// Create the codebook vector param list.
	_pVQParam  = new VQBI1_VECTOR_PARAMS_STRUCT*[VQCRI1_NUM_CODEBOOKS];
  if(_pVQParam == NULL)
  {
    Destroy();
    return(0);
  }//end if _pVQParam...

  /// Calc and load the params for every vector in every codebook.
  for(int book = 0; book < VQCRI1_NUM_CODEBOOKS; book++)
  {
    int vec;

    /// Get the codebook length to define the param list length.
    int tableLen = _pVQCodebook[book]->GetCodeBookLength();
    /// Create the param list.
    _pVQParam[book] = new VQBI1_VECTOR_PARAMS_STRUCT[tableLen];
    for(vec = 0; vec < tableLen; vec++)
    {
      const short* pV = (_pVQCodebook[book]->GetCodeBook())[vec];
      int dim = GetDimension();

      int sum = 0;
      for(i = 0; i < dim; sum += pV[i], i++);
      double mean = (double)sum/16.0;

      double var = 0.0;
      for(i = 0; i < dim; i++)
      {
        double diff = (double)pV[i] - mean;
        var += (diff*diff);
      }//end for i...
      var = sqrt(var);

      _pVQParam[book][vec].index  = vec;
      _pVQParam[book][vec].sum    = sum;
      _pVQParam[book][vec].var    = var;
    }//end for vec...

    /// Bubble sort the params in order of increasing sum.
    VQBI1_VECTOR_PARAMS_STRUCT tmp;
    int sortCnt = 1;
    while(sortCnt > 0)
    {
      sortCnt = 0;
      for(vec = 1; vec < tableLen; vec++)
      {
        if(_pVQParam[book][vec].sum < _pVQParam[book][vec-1].sum)
        {
          /// Swap.
          tmp                     = _pVQParam[book][vec-1];
          _pVQParam[book][vec-1]  = _pVQParam[book][vec];
          _pVQParam[book][vec]    = tmp;
          sortCnt++;
        }//end if sum...
      }//end for vec...
    }//end while sortCnt...

  }//end for book...

  return(1); 
}//end Create.

void VectorQuantiserCrImpl1::Destroy(void)
{
  if(_pVQCodebook != NULL)
  {
    for(int i = 0; i < VQCRI1_NUM_CODEBOOKS; i++) ///< Delete what is in the list.
      if(_pVQCodebook[i] != NULL)
        delete _pVQCodebook[i];
	  delete[] _pVQCodebook;                        ///< Delete the list itself.
  }//end if _pCrVQCodebook...
  _pVQCodebook = NULL;

  if(_pVQParam != NULL)
  {
    for(int i = 0; i < VQCRI1_NUM_CODEBOOKS; i++) ///< Delete what is in the list.
      if(_pVQParam[i] != NULL)
        delete[] _pVQParam[i];
	  delete[] _pVQParam;                        ///< Delete the list itself.
  }//end if _pVQParam...
  _pVQParam = NULL;

}//end Destroy.

											

												